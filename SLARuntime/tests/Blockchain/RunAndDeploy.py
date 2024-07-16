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
import sys
import time

import web3

from PyEthHelper.GethNodeGuard import GethDevNodeGuard


GETH_BIN      = 'geth'
GETH_PORT     = 8550
BLOCK_PERIOD  = 1
# BLOCK_PERIOD  = GethDevNodeGuard.DEV_BLOCK_PERIOD

TEST_APP_WALLET     = '0x2b1D07aB2d2f7479637ee12eC13978251Bc0DDf1'
TEST_DEPLOY_KEY_INT = int('0x4c0883a69102937d6231471b5dbb6204fe5129617082792ae468d01a3f362318', 16)
TEST_CLIENT_KEY_INT = int('0x1799f7c8abf08e0c404038b0b1cb7148ee4fcf0bc2e8dd332f85dcc084e7dddc', 16)


THIS_DIR  = os.path.dirname(os.path.abspath(__file__))
TESTS_DIR = os.path.dirname(THIS_DIR)
REPO_DIR  = os.path.dirname(TESTS_DIR)
ALL_REPOS_DIR = os.path.dirname(REPO_DIR)

TEST_END2END_DIR = os.path.join(TESTS_DIR, 'End2End')

RA_REPO_DIR = os.path.join(ALL_REPOS_DIR, 'libs', 'ra-onchain')
RA_BUILD_DIR = os.path.join(RA_REPO_DIR, 'build')
RA_TESTS_DIR = os.path.join(RA_REPO_DIR, 'tests')
RA_CERTS_DIR = os.path.join(RA_TESTS_DIR, 'certs')

SLA_REPO_DIR  = os.path.join(ALL_REPOS_DIR, 'sla-onchain')
SLA_BUILD_DIR = os.path.join(SLA_REPO_DIR, 'build')


sys.path.append(THIS_DIR)
import SlaClient
import SlaManager
import SlaProvider


def main() -> None:

	logging.basicConfig(
		level=logging.INFO,
		format='%(asctime)s [%(levelname)s] %(name)s: %(message)s',
		datefmt='%Y-%m-%d %H:%M:%S',
	)

	logger = logging.getLogger(f'{__name__}.{main.__name__}')

	with GethDevNodeGuard(gethBin=GETH_BIN, httpPort=GETH_PORT, blockPeriod=BLOCK_PERIOD) as guard:
		w3: web3.Web3 = guard.w3

		slaMgr = SlaManager.SlaManager(
			ecdsaKeyInt=TEST_DEPLOY_KEY_INT,
			iasRootCertPath=os.path.join(RA_CERTS_DIR, 'CertIASRoot.pem'),
		)
		guard.FillAccount(slaMgr.ethKey.address, amountEth=1000.0)
		slaClt = SlaClient.SlaClient(
			ecdsaKeyInt = TEST_CLIENT_KEY_INT,
		)
		guard.FillAccount(slaClt.ethKey.address, amountEth=1000.0)
		slaPrd = SlaProvider.SlaProvider(
			ethAddr=TEST_APP_WALLET
		)
		guard.FillAccount(slaPrd.ethAddr, amountEth=1000.0)

		slaMgr.Deploy(
			w3=w3,
			iasRootCertMgrAbiPath=os.path.join(RA_BUILD_DIR, 'contracts', 'IASRootCertMgr.abi'),
			iasRootCertMgrBinPath=os.path.join(RA_BUILD_DIR, 'contracts', 'IASRootCertMgr.bin'),
			iasReportCertMgrAbiPath=os.path.join(RA_BUILD_DIR, 'contracts', 'IASReportCertMgr.abi'),
			iasReportCertMgrBinPath=os.path.join(RA_BUILD_DIR, 'contracts', 'IASReportCertMgr.bin'),
			decentSvrCertMgrAbiPath=os.path.join(RA_BUILD_DIR, 'contracts', 'DecentServerCertMgr.abi'),
			decentSvrCertMgrBinPath=os.path.join(RA_BUILD_DIR, 'contracts', 'DecentServerCertMgr.bin'),
			slaMgrAbiPath=os.path.join(SLA_BUILD_DIR, 'contracts', 'SlaManager.abi'),
			slaMgrBinPath=os.path.join(SLA_BUILD_DIR, 'contracts', 'SlaManager.bin'),
			decentSvrCertPath=os.path.join(RA_CERTS_DIR, 'CertDecentServer.pem'),
		)

		# Write SLA Manager address to config file:
		slaMgr.UpdateComponentConfigFile(
			configFilePath=os.path.join(TEST_END2END_DIR, 'components_config.json'),
		)

		slaClt.AttachToSlaManager(
			w3=w3,
			slaMgrAbiPath=os.path.join(SLA_BUILD_DIR, 'contracts', 'SlaManager.abi'),
			slaMgrBinPath=os.path.join(SLA_BUILD_DIR, 'contracts', 'SlaManager.bin'),
			slaMgrAddr=slaMgr.slaMgrAddr,
			slaMgrBlkNum=slaMgr.slaMgrBlkNum,
		)
		slaClt.SlaHandshake()

		while True:
			time.sleep(1)


if __name__ == '__main__':
	main()

