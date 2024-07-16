#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import datetime
import logging
import os

from typing import Union

import eth_account
import rlp
import web3

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric.ec import (
	derive_private_key,
	generate_private_key,
	ECDH,
	EllipticCurvePrivateKey,
	EllipticCurvePublicNumbers,
	SECP256K1
)
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.x509.oid import NameOID

from PyEthHelper import EthContractHelper
from PyEthHelper.ContractEventHelper import ContractEventHelper


def CreateSelfSignedCert(key: EllipticCurvePrivateKey) -> str:
	subject = issuer =  x509.Name([
		x509.NameAttribute(NameOID.COMMON_NAME, 'sla-client'),
	])
	cert = x509.CertificateBuilder().subject_name(
			subject
		).issuer_name(
			issuer
		).public_key(
			key.public_key()
		).serial_number(
			x509.random_serial_number()
		).not_valid_before(
			datetime.datetime.now(datetime.timezone.utc)
		).not_valid_after(
			datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=30)
		).sign(key, hashes.SHA256())
	return cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')


class SlaClient(object):

	def __init__(
		self,
		ecdsaKeyInt: int,
	) -> None:
		super(SlaClient, self).__init__()

		self.logger = logging.getLogger(f'{__name__}.{self.__class__.__name__}')

		self.ecdsaKeyInt = ecdsaKeyInt
		self.ecdsaKey = derive_private_key(ecdsaKeyInt, SECP256K1())
		self.ecdsaKeyPem = self.ecdsaKey.private_bytes(
			encoding=serialization.Encoding.PEM,
			format=serialization.PrivateFormat.TraditionalOpenSSL,
			encryption_algorithm=serialization.NoEncryption()
		).decode('utf-8')
		self.certPem = CreateSelfSignedCert(self.ecdsaKey)
		self.ecdhKey = generate_private_key(SECP256K1())

		self.logger.info('Client ECDSA private key:\n' + self.ecdsaKeyPem)
		self.logger.info('Client self-signed certificate:\n' + self.certPem)

		self.ethKey = eth_account.Account.from_key(ecdsaKeyInt)

		self.logger.info('Client Ethereum address: ' + self.ethKey.address)

	def AttachToSlaManager(
		self,
		w3: web3.Web3,
		slaMgrAbiPath: Union[str, os.PathLike],
		slaMgrBinPath: Union[str, os.PathLike],
		slaMgrAddr: str,
		slaMgrBlkNum: int,
	) -> None:
		self.w3 = w3
		self.slaMgrAbiPath = slaMgrAbiPath
		self.slaMgrBinPath = slaMgrBinPath
		self.slaMgrAddr = slaMgrAddr
		self.slaMgrBlkNum = slaMgrBlkNum

		self.slaManager = EthContractHelper.LoadContract(
			w3=self.w3,
			projConf=(self.slaMgrAbiPath, self.slaMgrBinPath),
			contractName='SlaManager',
			address=self.slaMgrAddr,
		)

		# verify the ServiceDeployed event
		svcDeplyedLogs = ContractEventHelper.WaitForContractEvent(
			w3=self.w3,
			contract=self.slaManager,
			eventName='ServiceDeployed',
			fromBlock=self.slaMgrBlkNum,
		)
		assert len(svcDeplyedLogs) == 1
		self.svcDeplyedLog = svcDeplyedLogs[0]
		assert self.svcDeplyedLog['blockNumber'] == self.slaMgrBlkNum
		assert self.svcDeplyedLog['address'] == self.slaMgrAddr
		assert self.svcDeplyedLog['event'] == 'ServiceDeployed'
		assert self.svcDeplyedLog['args']['serviceAddr'] == self.slaMgrAddr
		self.logger.info('ServiceDeployed event verified via ServiceDeployed log')

	def Register(self) -> None:
		# register client
		self.logger.info('Registering client...')
		self.regTxReceipt = EthContractHelper.CallContractFunc(
			w3=self.w3,
			contract=self.slaManager,
			funcName='registerClient',
			arguments=[
				self.ecdhKey.public_key().public_numbers().x.to_bytes(32, 'big'),
				self.ecdhKey.public_key().public_numbers().y.to_bytes(32, 'big'),
			],
			privKey=self.ethKey,
			confirmPrompt=False # don't prompt for confirmation
		)
		self.logger.info('Client registered')

	def WaitForProviderRegister(self) -> None:
		# waiting for the provider to register
		self.logger.info('Waiting for the provider to register...')
		provRegLogs = ContractEventHelper.WaitForContractEvent(
			w3=self.w3,
			contract=self.slaManager,
			eventName='ProviderRegistered',
			fromBlock=self.slaMgrBlkNum,
		)
		assert len(provRegLogs) == 1
		self.provRegLog = provRegLogs[0]
		self.svrHardwareID = self.provRegLog['args']['hardwareId']
		svrDHKeyX = self.provRegLog['args']['dhKeyX']
		svrDHKeyY = self.provRegLog['args']['dhKeyY']
		self.unitRate   = self.provRegLog['args']['rate']
		self.svrEcdhKey = EllipticCurvePublicNumbers(
			x=int.from_bytes(svrDHKeyX, 'big'),
			y=int.from_bytes(svrDHKeyY, 'big'),
			curve=SECP256K1()
		).public_key()
		self.sharedRootKey = self.ecdhKey.exchange(ECDH(), self.svrEcdhKey)
		self.logger.info(
			f'Provider registered: HardwareID: {self.svrHardwareID}, '
			f'rate: {self.unitRate}'
		)
		self.logger.info(f'Shared root key: {self.sharedRootKey.hex()}')

	def ProposeContract(self) -> None:

		# propose a contract
		self.logger.info('Proposing a contract...')
		self.proposeTxReceipt = EthContractHelper.CallContractFunc(
			w3=self.w3,
			contract=self.slaManager,
			funcName='proposeContract',
			arguments=[ self.svrHardwareID, ],
			value=self.w3.to_wei(100, 'ether'),
			privKey=self.ethKey,
			confirmPrompt=False # don't prompt for confirmation
		)
		proposeLogs = ContractEventHelper.WaitForContractEvent(
			w3=self.w3,
			contract=self.slaManager,
			eventName='SlaProposal',
			fromBlock=self.proposeTxReceipt.blockNumber,
		)
		assert len(proposeLogs) == 1
		self.proposeLog = proposeLogs[0]
		self.contractId = self.proposeLog['args']['contractId']
		self.logger.info(f'Proposed Contract received contract ID: {self.contractId}')

	def WaitForContractAcceptance(self) -> None:
		# waiting for the provider to accept
		acceptLogs = ContractEventHelper.WaitForContractEvent(
			w3=self.w3,
			contract=self.slaManager,
			eventName='SlaProposalAccepted',
			fromBlock=self.proposeTxReceipt.blockNumber,
		)
		assert len(acceptLogs) == 1
		self.acceptLog = acceptLogs[0]
		assert self.contractId == self.acceptLog['args']['contractId']
		self.slaAddr = self.acceptLog['args']['slaAddr']
		providerMessage = self.acceptLog['args']['providerMessage']
		iv, tag, encMsg = rlp.decode(providerMessage)
		aesgcm = AESGCM(self.sharedRootKey)
		providerMessage = aesgcm.decrypt(iv, encMsg + tag, None)
		hostAddr, hostPort = rlp.decode(providerMessage)
		self.hostAddr = hostAddr.decode('utf-8')
		self.hostPort = int.from_bytes(hostPort, 'big')
		self.logger.info(f'Contract accepted with SLA contract created @ {self.slaAddr}')
		self.logger.info(f'Provider message: {self.hostAddr}:{self.hostPort}')

	def SlaHandshake(self) -> None:
		self.Register()
		self.WaitForProviderRegister()
		self.ProposeContract()
		self.WaitForContractAcceptance()

