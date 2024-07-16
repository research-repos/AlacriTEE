#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import json
import logging
import os

from typing import Union

import eth_account
import web3

from cryptography import x509
from cryptography.hazmat.primitives import serialization

from PyEthHelper import EthContractHelper


class SlaManager(object):

	def __init__(
		self,
		ecdsaKeyInt: int,
		iasRootCertPath: Union[str, os.PathLike],
		# iasReportCertPath: Union[str, os.PathLike],
	):
		super(SlaManager, self).__init__()

		self.logger = logging.getLogger(f'{__name__}.{self.__class__.__name__}')

		self.ethKey = eth_account.Account.from_key(ecdsaKeyInt)

		self.logger.info('Deployer Ethereum address: ' + self.ethKey.address)

		with open(iasRootCertPath, 'rb') as f:
			self.iasRootCert = x509.load_pem_x509_certificate(f.read())
		self.iasRootCertDer = self.iasRootCert.public_bytes(
			serialization.Encoding.DER
		)

		# with open(iasReportCertPath, 'rb') as f:
		# 	self.iasReportCert = x509.load_pem_x509_certificate(f.read())
		# self.iasReportCertDer = self.iasReportCert.public_bytes(
		# 	serialization.Encoding.DER
		# )

	def DeployIasRootCertMgr(
		self,
		w3: web3.Web3,
		iasRootCertMgrAbiPath: Union[str, os.PathLike],
		iasRootCertMgrBinPath: Union[str, os.PathLike],
	) -> None:

		self.logger.info('Deploying IASRootCertMgr contract...')
		c = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(iasRootCertMgrAbiPath, iasRootCertMgrBinPath),
			contractName='IASRootCertMgr',
		)
		self.iasRootCertMgrTxReceipt = EthContractHelper.DeployContract(
			w3=w3,
			contract=c,
			arguments=[ self.iasRootCertDer ],
			privKey=self.ethKey,
			confirmPrompt=False,
		)
		self.iasRootCertMgrAddr = self.iasRootCertMgrTxReceipt.contractAddress
		self.logger.info(f'IASRootCertMgr deployed @ {self.iasRootCertMgrAddr}')

	def DeployIasReportCertMgr(
		self,
		w3: web3.Web3,
		iasReportCertMgrAbiPath: Union[str, os.PathLike],
		iasReportCertMgrBinPath: Union[str, os.PathLike],
	) -> None:
		self.logger.info('Deploying IASReportCertMgr contract...')
		c = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(iasReportCertMgrAbiPath, iasReportCertMgrBinPath),
			contractName='IASReportCertMgr',
		)
		self.iasReportCertMgrTxReceipt = EthContractHelper.DeployContract(
			w3=w3,
			contract=c,
			arguments=[ self.iasRootCertMgrAddr ],
			privKey=self.ethKey,
			confirmPrompt=False,
		)
		self.iasReportCertMgrAddr = self.iasReportCertMgrTxReceipt.contractAddress
		self.logger.info(f'IASReportCertMgr deployed @ {self.iasReportCertMgrAddr}')

	def DeployDecentSvrCertMgr(
		self,
		w3: web3.Web3,
		decentSvrCertMgrAbiPath: Union[str, os.PathLike],
		decentSvrCertMgrBinPath: Union[str, os.PathLike],
	) -> None:
		self.logger.info('Deploying DecentServerCertMgr contract...')
		c = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(decentSvrCertMgrAbiPath, decentSvrCertMgrBinPath),
			contractName='DecentServerCertMgr',
		)
		self.decentSvrCertMgrTxReceipt = EthContractHelper.DeployContract(
			w3=w3,
			contract=c,
			arguments=[ self.iasReportCertMgrAddr ],
			privKey=self.ethKey,
			confirmPrompt=False,
		)
		self.decentSvrCertMgrAddr = self.decentSvrCertMgrTxReceipt.contractAddress
		self.logger.info(f'DecentServerCertMgr deployed @ {self.decentSvrCertMgrAddr}')

		self.decentSvrCertMgr = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(decentSvrCertMgrAbiPath, decentSvrCertMgrBinPath),
			contractName='DecentServerCertMgr',
			address=self.decentSvrCertMgrAddr,
		)

	def VerifyDecentSvrCert(
		self,
		w3: web3.Web3,
		decentSvrCertPath: Union[str, os.PathLike],
	) -> None:
		with open(decentSvrCertPath, 'rb') as f:
			self.decentSvrCert = x509.load_pem_x509_certificate(f.read())
		self.decentSvrCertDer = self.decentSvrCert.public_bytes(
			serialization.Encoding.DER
		)

		self.decentSvrVrfyReceipt = EthContractHelper.CallContractFunc(
			w3=w3,
			contract=self.decentSvrCertMgr,
			funcName='verifyCert',
			arguments=[ self.decentSvrCertDer ],
			privKey=self.ethKey,
			confirmPrompt=False # don't prompt for confirmation
		)

	def DeploySlaManager(
		self,
		w3: web3.Web3,
		slaMgrAbiPath: Union[str, os.PathLike],
		slaMgrBinPath: Union[str, os.PathLike],
	) -> None:
		self.logger.info('Deploying SlaManager contract...')
		c = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(slaMgrAbiPath, slaMgrBinPath),
			contractName='SlaManager',
		)
		self.slaMgrTxReceipt = EthContractHelper.DeployContract(
			w3=w3,
			contract=c,
			arguments=[ self.decentSvrCertMgrAddr ],
			privKey=self.ethKey,
			confirmPrompt=False,
		)
		self.slaMgrAddr: str = self.slaMgrTxReceipt.contractAddress
		self.slaMgrBlkNum = self.slaMgrTxReceipt.blockNumber
		self.logger.info(f'SlaManager deployed @ {self.slaMgrAddr}')

		self.slaMgr = EthContractHelper.LoadContract(
			w3=w3,
			projConf=(slaMgrAbiPath, slaMgrBinPath),
			contractName='SlaManager',
			address=self.slaMgrAddr,
		)

	def Deploy(
		self,
		w3: web3.Web3,
		iasRootCertMgrAbiPath: Union[str, os.PathLike],
		iasRootCertMgrBinPath: Union[str, os.PathLike],
		iasReportCertMgrAbiPath: Union[str, os.PathLike],
		iasReportCertMgrBinPath: Union[str, os.PathLike],
		decentSvrCertMgrAbiPath: Union[str, os.PathLike],
		decentSvrCertMgrBinPath: Union[str, os.PathLike],
		slaMgrAbiPath: Union[str, os.PathLike],
		slaMgrBinPath: Union[str, os.PathLike],
		decentSvrCertPath: Union[str, os.PathLike, None] = None,
	) -> None:
		self.w3 = w3

		self.DeployIasRootCertMgr(
			w3=self.w3,
			iasRootCertMgrAbiPath=iasRootCertMgrAbiPath,
			iasRootCertMgrBinPath=iasRootCertMgrBinPath,
		)

		self.DeployIasReportCertMgr(
			w3=self.w3,
			iasReportCertMgrAbiPath=iasReportCertMgrAbiPath,
			iasReportCertMgrBinPath=iasReportCertMgrBinPath,
		)

		self.DeployDecentSvrCertMgr(
			w3=self.w3,
			decentSvrCertMgrAbiPath=decentSvrCertMgrAbiPath,
			decentSvrCertMgrBinPath=decentSvrCertMgrBinPath,
		)

		if decentSvrCertPath is not None:
			self.VerifyDecentSvrCert(
				w3=self.w3,
				decentSvrCertPath=decentSvrCertPath,
			)

		self.DeploySlaManager(
			w3=self.w3,
			slaMgrAbiPath=slaMgrAbiPath,
			slaMgrBinPath=slaMgrBinPath,
		)

	def UpdateComponentConfigFile(
		self,
		configFilePath: Union[str, os.PathLike],
	) -> None:
		with open(configFilePath) as f:
			cmpConfig = json.load(f)

		cmpConfig['SLA']['ChainID'] = self.w3.eth.chain_id

		cmpConfig['SLA']['ManagerAddr'] = \
			self.slaMgrAddr[2:] \
			if self.slaMgrAddr.startswith('0x') \
				else self.slaMgrAddr

		with open(configFilePath, 'w') as f:
			f.write(json.dumps(cmpConfig, indent='\t') + '\n')

