#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import logging
import os
import random

from typing import Union

import eth_account
import rlp
import web3
import web3.contract
import web3.contract.contract

from cryptography import x509
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ec import (
	derive_private_key,
	generate_private_key,
	ECDH,
	EllipticCurvePrivateKey,
	EllipticCurvePublicNumbers,
	SECP256K1
)
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

from PyEthHelper import EthContractHelper
from PyEthHelper.ContractEventHelper import ContractEventHelper


UINT64_MAX = 2**64 - 1
UINT32_MAX = 2**32 - 1


class SlaProvider(object):

	def __init__(
		self,
		ecdsaKeyInt: Union[int, None] = None,
		ethAddr: Union[str, None] = None,
	) -> None:
		super(SlaProvider, self).__init__()

		self.logger = logging.getLogger(f'{__name__}.{self.__class__.__name__}')

		if ecdsaKeyInt is not None and ethAddr is not None:
			raise ValueError('Only one of ecdsaKeyInt and ethAddr should be provided')
		if ecdsaKeyInt is None and ethAddr is None:
			raise ValueError('One of ecdsaKeyInt and ethAddr should be provided')

		if ecdsaKeyInt is not None:
			# we are going to mock the provider
			self.mockMode = True
			self.ethKey = eth_account.Account.from_key(ecdsaKeyInt)
			self.ethAddr = self.ethKey.address
		else:
			self.mockMode = False
			self.ethAddr = ethAddr

		self.logger.info('SLA Provider Ethereum address: ' + self.ethAddr)

		self.logger.info(
			'SLA Provider mock mode is ' + ('ON' if self.mockMode else 'OFF')
		)

		if self.mockMode:
			self.ecdhKey = generate_private_key(SECP256K1())
			self.chkptSeqNum = 0

	def GetBalance(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
	) -> int:
		return int(EthContractHelper.CallContractFunc(
			w3=w3,
			contract=slaMgr,
			funcName='getProviderBalance',
			arguments=[  ],
			confirmPrompt=False # don't prompt for confirmation
		))

	def GetClientBalance(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
	) -> int:
		return int(EthContractHelper.CallContractFunc(
			w3=w3,
			contract=slaMgr,
			funcName='getClientBalance',
			arguments=[  ],
			confirmPrompt=False # don't prompt for confirmation
		))

	def MockRegister(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
		decentSvrCertPath: Union[str, os.PathLike],
		decentAppCertPath: Union[str, os.PathLike],
		rate: int = 1,
	) -> None:

		with open(decentSvrCertPath, 'rb') as f:
			self.decentSvrCert = x509.load_pem_x509_certificate(f.read())
		self.decentSvrCertDer = self.decentSvrCert.public_bytes(
			serialization.Encoding.DER
		)

		with open(decentAppCertPath, 'rb') as f:
			self.decentAppCert = x509.load_pem_x509_certificate(f.read())
		self.decentAppCertDer = self.decentAppCert.public_bytes(
			serialization.Encoding.DER
		)

		self.logger.info('Registering provider...')
		self.registerTxReceipt = EthContractHelper.CallContractFunc(
			w3=w3,
			contract=slaMgr,
			funcName='registerProvider',
			arguments=[
				rate,
				self.ecdhKey.public_key().public_numbers().x.to_bytes(32, 'big'),
				self.ecdhKey.public_key().public_numbers().y.to_bytes(32, 'big'),
				self.decentSvrCertDer,
				self.decentAppCertDer,
			],
			privKey=self.ethKey,
			confirmPrompt=False # don't prompt for confirmation
		)
		self.logger.info('Provider registered')

	def MockWaitForContractProposal(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
	) -> None:
		proposeLogs = ContractEventHelper.WaitForContractEvent(
			w3=w3,
			contract=slaMgr,
			eventName='SlaProposal',
			fromBlock=self.registerTxReceipt.blockNumber,
		)
		assert len(proposeLogs) == 1
		self.proposeLog = proposeLogs[0]

		self.contractId = self.proposeLog['args']['contractId']
		self.logger.info(f'Proposed Contract received contract ID: {self.contractId}')

		cltEcdhKeyX = self.proposeLog['args']['cltDhKeyX']
		cltEcdhKeyY = self.proposeLog['args']['cltDhKeyY']
		self.cltEcdhKey = EllipticCurvePublicNumbers(
			x=int.from_bytes(cltEcdhKeyX, 'big'),
			y=int.from_bytes(cltEcdhKeyY, 'big'),
			curve=SECP256K1()
		).public_key()
		self.sharedRootKey = self.ecdhKey.exchange(ECDH(), self.cltEcdhKey)
		self.logger.info(f'Shared root key: {self.sharedRootKey.hex()}')

	def MockAcceptProposal(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
		hostAddr: str = '127.0.0.1',
		hostPort: int = 65432
	) -> None:
		self.logger.debug('Generating provider message...')
		providerMsg = rlp.encode([
			hostAddr.encode('utf-8'),
			hostPort.to_bytes(2, 'big'),
		])
		iv = random.randbytes(12)
		aesgcm = AESGCM(self.sharedRootKey)
		encMsg = aesgcm.encrypt(iv, providerMsg, None)
		encMsg, tag = encMsg[:-16], encMsg[-16:]
		providerMsg = rlp.encode([ iv, tag, encMsg, ])

		self.logger.info('Accepting proposal...')
		self.acceptTxReceipt = EthContractHelper.CallContractFunc(
			w3=w3,
			contract=slaMgr,
			funcName='acceptProposal',
			arguments=[ self.contractId, providerMsg ],
			privKey=self.ethKey,
			value=w3.to_wei(100, 'ether'),
			confirmPrompt=False # don't prompt for confirmation
		)
		self.logger.info('Proposal accepted')

	def MockWaitForContractAcceptance(
		self,
		w3: web3.Web3,
		slaMgr: web3.contract.contract.Contract,
	) -> None:
		# waiting for the provider to accept
		acceptLogs = ContractEventHelper.WaitForContractEvent(
			w3=w3,
			contract=slaMgr,
			eventName='SlaProposalAccepted',
			fromBlock=self.acceptTxReceipt.blockNumber,
		)
		assert len(acceptLogs) == 1
		self.acceptLog = acceptLogs[0]
		assert self.contractId == self.acceptLog['args']['contractId']
		self.slaAddr = self.acceptLog['args']['slaAddr']

	def AttachToSlaContract(
		self,
		w3: web3.Web3,
		slaAbiPath: Union[str, os.PathLike],
		slaBinPath: Union[str, os.PathLike],
	) -> None:

		self.sla = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(slaAbiPath, slaBinPath),
			contractName='SLA',
			address=self.slaAddr,
		)

	def MockCheckpointReport(
		self,
		w3: web3.Web3,
		numOfRequests: int,
		acceptedRate: float = 0.95,
		minUnitsUsed: int = 1,
		maxUnitsUsed: int = UINT64_MAX, # max uint64
	) -> None:
		REPORT_DATA_SIZE_PER_ENTRY = 8 + 4
		report = []

		for reqId in range(numOfRequests):
			isAccepted = random.uniform(0.0, 1.0) < acceptedRate
			unitsUsed = random.randint(minUnitsUsed, maxUnitsUsed)
			microSecPerUnit = random.uniform(0.05, 0.1)
			report.append((reqId, isAccepted, unitsUsed, microSecPerUnit))

		# encode report to checkpointData
		reportData = b''
		for reqId, isAccepted, unitsUsed, microSecPerUnit in report:
			# isAccepted = 1 if isAccepted else 0
			# isAccepted = isAccepted.to_bytes(1, 'big')
			unitsUsed = 0 if not isAccepted else unitsUsed
			assert unitsUsed <= UINT64_MAX
			unitsUsedBytes = unitsUsed.to_bytes(8, 'big')
			assert len(unitsUsedBytes) == 8

			elapsedTime = int(microSecPerUnit * unitsUsed)
			assert elapsedTime <= UINT32_MAX
			elapsedTimeBytes = elapsedTime.to_bytes(4, 'big')
			assert len(elapsedTimeBytes) == 4

			reportDataEntry = unitsUsedBytes + elapsedTimeBytes
			assert len(reportDataEntry) == REPORT_DATA_SIZE_PER_ENTRY

			reportData += reportDataEntry
		assert len(reportData) == REPORT_DATA_SIZE_PER_ENTRY * numOfRequests

		# submit report
		self.logger.info(
			f'Submitting checkpoint report with seq# {self.chkptSeqNum}...'
		)
		self.chkptRepTxReceipt = EthContractHelper.CallContractFunc(
			w3=w3,
			contract=self.sla,
			funcName='hostCheckpointReport',
			arguments=[ self.chkptSeqNum, numOfRequests, reportData ],
			privKey=self.ethKey,
			confirmPrompt=False # don't prompt for confirmation
		)
		self.logger.info('Checkpoint report submitted')
		self.chkptSeqNum += 1

		prdBal = self.GetBalance(w3, self.sla)
		cltBal = self.GetClientBalance(w3, self.sla)
		self.logger.info(f'Provider balance: {prdBal}, Client balance: {cltBal}')

