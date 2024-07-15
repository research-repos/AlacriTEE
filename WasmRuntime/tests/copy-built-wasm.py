#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import os
import shutil


CURR_DIR = os.path.dirname(os.path.abspath(__file__))
COUNTER_PROJ_DIR = os.path.join(CURR_DIR, os.pardir, os.pardir, os.pardir, 'WasmCounter')
COUNTER_POLY_DIR = os.path.join(COUNTER_PROJ_DIR, 'test', 'polybench')
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
FILENAME_FORMATS = [
	(os.path.join('native', '{testname}.app'),     '{testname}.app'),
	(os.path.join('wasm', '{testname}.wasm'),      '{testname}.wasm'),
	(os.path.join('wasm', '{testname}.nopt.wasm'), '{testname}.nopt.wasm'),
]


TEST_CASES_FILES = [ (ySrc.format(testname=x), yDst.format(testname=x)) for ySrc, yDst in FILENAME_FORMATS for x in TEST_CASES ]

for srcFilebase, dstFilebase in TEST_CASES_FILES:
	src = os.path.join(COUNTER_POLY_DIR, srcFilebase)
	dst = os.path.join(CURR_DIR, dstFilebase)
	shutil.copy(src, dst)

print(f'Copied {len(TEST_CASES_FILES)} files from {COUNTER_POLY_DIR} to {CURR_DIR}')

