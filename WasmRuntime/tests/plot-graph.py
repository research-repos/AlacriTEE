#!/usr/bin/env python3
# -*- coding:utf-8 -*-
###
# Copyright (c) 2024 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.
###


import argparse
import json
import os
import statistics
import time
from typing import Any, List

import plotly.graph_objects as go


CURR_DIR = os.path.dirname(os.path.abspath(__file__))

WARMUP_TIMES = 2


def SelectDataset(
	env: str,
	group: str,
	measurements: dict
) -> dict:
	# select all the elapsed time for each test case
	dataset = {
		k: [ x[2] for x in v[0][env][group] ]
			for k,v in measurements.items()
	}
	# remove the warmup times
	for k,v in dataset.items():
		if len(v) - WARMUP_TIMES < 3:
			raise RuntimeError(f'Not enough data for {env} - {group} - {k}')

	dataset = {
		k: v[WARMUP_TIMES:]
			for k,v in dataset.items()
	}
	# sort the valid data
	dataset = {
		k: sorted(v)
			for k,v in dataset.items()
	}

	# print the result in text
	print(f'{env} - {group} dataset:')
	for k, vs in dataset.items():
		errPlus = vs[-1] - statistics.median(vs)
		errPlus = errPlus / 1000
		errMinus = statistics.median(vs) - vs[0]
		errMinus = errMinus / 1000

		outStr = f'{k:20}: Elapsed '
		for v in vs:
			v = v / 1000
			outStr += f'{v:10.3f}ms, '
		outStr += 'Errors: -' + f'{errMinus:10.3f}ms, +' + f'{errPlus:10.3f}ms'
		print(outStr)

	return dataset


def DatasetNormalize(dataset: dict, baseDataset: dict) -> dict:
	assert len(dataset) == len(baseDataset), 'Dataset length mismatch'
	assert set(dataset.keys()) == set(baseDataset.keys()), 'Dataset key mismatch'

	res = {}
	for k, vs in dataset.items():
		baseVs = baseDataset[k]
		baseVMed = statistics.median(baseVs)
		#baseVAvg = statistics.mean(baseVs)
		res[k] = [ v / baseVMed for v in vs ]

	return res


def DatasetPlotBar(
	name: str,
	xData: List[Any],
	yDataset: dict
) -> go.Bar:
	yMid = [ statistics.median(yDataset[x]) for x in xData ] # if x != 'floyd-warshall'
	yPlus = [ (yDataset[x][-1] - statistics.median(yDataset[x])) for x in xData ] # if x != 'floyd-warshall'
	yMinus = [ (statistics.median(yDataset[x]) - yDataset[x][0]) for x in xData ] # if x != 'floyd-warshall'
	return go.Bar(
		name=name,
		x=xData,
		y=yMid,
		error_y=dict(
			type='data',
			symmetric=False,
			array=yPlus,
			arrayminus=yMinus,
		),
	)

def main() -> None:
	argParser = argparse.ArgumentParser(
		description='Plot the benchmark result graph',
	)
	argParser.add_argument(
		'--input',
		type=str,
		help='The input benchmark result file',
	)
	argParser.add_argument(
		'--output-formats',
		type=str,
		nargs='+',
		required=False,
		default=[ 'png', 'pdf', ],
		help='The output formats',
	)
	args = argParser.parse_args()

	inputPath = os.path.abspath(args.input)
	if not os.path.exists(inputPath):
		raise FileNotFoundError(f'Input file not found: {inputPath}')
	outputDir, outputFilename = os.path.split(inputPath)
	outputFilename, _ = os.path.splitext(outputFilename)

	with open(inputPath, 'r') as f:
		benchmarkRes = json.load(f)

	measurements = benchmarkRes['measurement']

	testCases = [ x for x in measurements.keys() ]

	# Collecting data
	## Native - Plain
	nativePlainDurations = (SelectDataset(
		'Native',
		'plain',
		benchmarkRes['measurement']
	))
	## Untrusted - Plain
	untrustedPlainDurations = (SelectDataset(
		'Untrusted',
		'plain',
		benchmarkRes['measurement']
	))
	# Untrusted - Instrumented
	untrustedInstDurations = (SelectDataset(
		'Untrusted',
		'instrumented',
		benchmarkRes['measurement']
	))
	# Enclave - Plain
	enclavePlainDurations = (SelectDataset(
		'Enclave',
		'plain',
		benchmarkRes['measurement']
	))
	# Enclave - Instrumented
	enclaveInstDurations = (SelectDataset(
		'Enclave',
		'instrumented',
		benchmarkRes['measurement']
	))


	# Normalizing data
	untrustedPlainDurations = DatasetNormalize(
		untrustedPlainDurations,
		nativePlainDurations,
	)
	untrustedInstDurations = DatasetNormalize(
		untrustedInstDurations,
		nativePlainDurations,
	)
	enclavePlainDurations = DatasetNormalize(
		enclavePlainDurations,
		nativePlainDurations,
	)
	enclaveInstDurations = DatasetNormalize(
		enclaveInstDurations,
		nativePlainDurations,
	)


	# Plotting Bars
	untrustedPlainBar = DatasetPlotBar(
		'WASM',
		testCases,
		untrustedPlainDurations,
	)
	untrustedInstBar = DatasetPlotBar(
		'WASM Instrumented',
		testCases,
		untrustedInstDurations,
	)
	enclavePlainBar = DatasetPlotBar(
		'Enclave WASM',
		testCases,
		enclavePlainDurations,
	)
	enclaveInstBar = DatasetPlotBar(
		'Enclave WASM Instrumented',
		testCases,
		enclaveInstDurations,
	)


	fig = go.Figure(
		data=[
			untrustedPlainBar,
			untrustedInstBar,
			enclavePlainBar,
			enclaveInstBar,
		],
	)
	fig.update_layout(barmode='group')

	# Add horizontal line to represent the native runtime
	fig.add_hline(
		y=1.0,
	)
	fig.add_annotation(dict(
		font=dict(color="black",size=12),
		#x=x_loc,
		#x=LabelDateB,
		x=1.04,
		y=1.0,
		showarrow=False,
		text='<b>Native</b>',
		textangle=0,
		xref="paper",
		yref="y",
	))

	fig.update_layout(
		autosize=False,
		width=1200,
		height=600,
	)

	fig.update_layout(legend=dict(
		orientation = "h",
		xanchor = "center",
		x = 0.5,
		y = -0.1,
	))

	fig.update_layout(
		margin=dict(l=40, r=80, t=40, b=40),
	)

	# fig.update_yaxes(type="log")

	htmlRendered = False
	for fmt in args.output_formats:
		if (
			((fmt.lower() == 'pdf') or (fmt.lower() == 'html')) and
			(not htmlRendered)
		):
			outputPath = os.path.join(outputDir, f'{outputFilename}.html')
			fig.write_html(outputPath)
			time.sleep(1) # Wait for the HTML elements to be loaded
			htmlRendered = True
		if fmt.lower() != 'html':
			outputPath = os.path.join(outputDir, f'{outputFilename}.{fmt}')
			fig.write_image(outputPath)
		print(f'Output file generated: {outputPath}')


if __name__ == '__main__':
	main()

