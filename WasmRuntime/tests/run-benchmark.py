#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import json
import os
import re
import subprocess
import sys
import time

from typing import Dict, List, Tuple

NICE_ADJUST = -20
AFFINITY = { 3,}

CURR_DIR = os.path.dirname(os.path.abspath(__file__))
PROJ_BUILD_DIR = os.path.join(CURR_DIR, os.pardir, 'build-release')
BENCHMARK_BUILD_DIR = os.path.join(PROJ_BUILD_DIR, 'tests', 'PolybenchTester')
BENCHMARKER_BIN = 'PolybenchTester'

TEST_CASES_DIR = os.path.join(CURR_DIR, 'polybench')

TEST_CASES = [
	# datamining - 2
	'correlation',
	'covariance',

	# linear-algebra/blas - 7
	'gemm',
	'gemver',
	'gesummv',
	'symm',
	'syr2k',
	'syrk',
	'trmm',

	# linear-algebra/kernels - 6
	'2mm',
	'3mm',
	'atax',
	'bicg',
	'doitgen',
	'mvt',

	# linear-algebra - solvers - 6
	'cholesky',
	'durbin',
	'gramschmidt',
	'lu',
	'ludcmp',
	'trisolv',

	# medley - 3
	'deriche',
	'floyd-warshall',
	'nussinov',

	# stencils - 6
	'adi',
	'fdtd-2d',
	'heat-3d',
	'jacobi-1d',
	'jacobi-2d',
	'seidel-2d',
]


def TryParseFinishedLogLine(state: dict, line: str) -> bool:
	LOG_FINISH_R = r'^\[(\w+)\]\s*PolybenchTester::EnclaveWasmMain\(INFO\):\s+<=====\s*Finished to run Enclave WASM program;\s*report:\s*(\{.+)$'
	LOG_FINISH_REGEX = re.compile(LOG_FINISH_R)

	m = LOG_FINISH_REGEX.search(line)
	if m is None:
		return False
	else:
		execEnv = m.group(1)
		execReport = m.group(2)
		execReportDict = json.loads(execReport)

		execType = execReportDict['type']
		startTime = execReportDict['start_time']
		endTime = execReportDict['end_time']
		duration = execReportDict['duration']

		measurements = [ startTime, endTime, duration ]

		# optional fields
		if 'threshold' in execReportDict:
			measurements.append([
				execReportDict['threshold'],
				execReportDict['counter'],
			])

		state['res'][execEnv][execType].append(measurements)

		return True


def ParseAllEnvTimePrintout(
	printoutLines: List[str]
) -> Dict[str, List[int]]:


	state = {
		'res': {
			'Untrusted': {
				'plain': [],
				'instrumented': [],
			},
			'Enclave': {
				'plain': [],
				'instrumented': [],
			},
			'Native': {
				'plain': [],
			},
		},
	}

	for line in printoutLines:
		if TryParseFinishedLogLine(state, line):
			continue

	return state['res']


def SetPriorityAndAffinity() -> None:
	# Set nice
	os.nice(NICE_ADJUST)

	# Set real-time priority
	schedParam = os.sched_param(os.sched_get_priority_max(os.SCHED_FIFO))
	os.sched_setscheduler(0, os.SCHED_FIFO, schedParam)

	# Set affinity
	os.sched_setaffinity(0, AFFINITY)


def RunProgram(cmd: List[str]) -> Tuple[str, str, int]:
	cmdStr = ' '.join(cmd)
	print(f'Running: {cmdStr}')

	with subprocess.Popen(
		cmd,
		stdout=subprocess.PIPE,
		stderr=subprocess.PIPE,
		cwd=BENCHMARK_BUILD_DIR,
		preexec_fn=lambda : SetPriorityAndAffinity(),
	) as proc:
		stdout, stderr = proc.communicate()
		stdout = stdout.decode('utf-8', errors='replace')
		stderr = stderr.decode('utf-8', errors='replace')

		time.sleep(5) # Wait for the system to settle down

		if proc.returncode != 0:
			print('Benchmark failed')

			print('##################################################')
			print('## STDOUT')
			print('##################################################')
			print(stdout)
			print()

			print('##################################################')
			print('## STDERR')
			print('##################################################')
			print(stderr)
			print()

			raise RuntimeError('Benchmark failed')

		return stdout, stderr, proc.returncode


def RunTestsAndCollectData() -> None:
	REPEAT_TIMES = 1

	output = {
		'measurement': {},
		'raw': {},
	}

	timeStr = time.strftime('%Y%m%d%H%M%S', time.localtime())

	for testCase in TEST_CASES:
		testCasePath = os.path.join(TEST_CASES_DIR, testCase)
		benchmarkPath = os.path.join(BENCHMARK_BUILD_DIR, BENCHMARKER_BIN)
		nativePath = testCasePath + '.app'

		output['raw'][testCase] = []
		output['measurement'][testCase] = []

		wasmCmd = [
			benchmarkPath,
			testCasePath + '.wasm',
			testCasePath + '.nopt.wasm',
		]

		nativeCmd = [ nativePath ]

		for i in range(REPEAT_TIMES):
				wasmStdout, wasmStderr, wasmRetcode = RunProgram(wasmCmd)
				nativeStdout, nativeStderr, nativeRetcode = RunProgram(nativeCmd)

				stdout = wasmStdout + '\n' + nativeStdout
				stderr = wasmStderr + '\n' + nativeStderr
				assert wasmRetcode == nativeRetcode

				output['raw'][testCase].append({
					'stdout': stdout,
					'stderr': stderr,
					'returncode': wasmRetcode,
				})
				printoutLines = stdout.splitlines()
				output['measurement'][testCase].append(
					ParseAllEnvTimePrintout(printoutLines)
				)

		with open(os.path.join(PROJ_BUILD_DIR, f'benchmark-{timeStr}.json'), 'w') as f:
			json.dump(output, f, indent='\t')


def ReProcRawData(jsonFilePath: str) -> None:
	with open(jsonFilePath, 'r') as f:
		jsonFile = json.load(f)

	rawData = jsonFile['raw']

	newMeasurements = {}

	for testCase, results in rawData.items():
		newMeasurements[testCase] = []
		for repeatRes in results:
			printoutLines = repeatRes['stdout'].splitlines()
			newMeasurements[testCase].append(ParseAllEnvTimePrintout(printoutLines))

	jsonFile['measurement'] = newMeasurements

	with open(jsonFilePath, 'w') as f:
		json.dump(jsonFile, f, indent='\t')


def main() -> None:
	if len(sys.argv) > 1:
		if sys.argv[1] == 'reproc':
			ReProcRawData(sys.argv[2])
			return
		else:
			print('Unknown command')
			return
	else:
		RunTestsAndCollectData()


if __name__ == '__main__':
	main()

