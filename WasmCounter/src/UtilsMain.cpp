// Copyright (c) 2023 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <iostream>

#include <WasmWat/WasmWat.h>
#include <WasmCounter/WasmCounter.hpp>
#include <SimpleSysIO/SysCall/Files.hpp>

#include "AdjacencyJson.hpp"


static std::string GetHelpStr(const std::string& progName)
{
	return
		"Usage: " + progName + " <command>\n"
		"  Available commands:\n"
		"    Instrument - Instrument WASM/WAT code\n"
		"    AdjJson    - Generate adjacency list in JSON for given WASM/WAT code\n"
		"  Usage for each command:\n"
		"    Instrument <input file> <output file>\n"
		"    AdjJson    <input file> <output file>\n";
		;
}


static void PrintHelpAndExit(const std::string& progName, int exitCode = 1)
{
	std::cout << GetHelpStr(progName) << std::endl;
	exit(exitCode);
}


static WasmWat::ModWrapper ReadModule(
	const std::string& progName,
	const std::string& inputPath
)
{
	bool isWat = false;
	if (
		inputPath.size() > 4 &&
		inputPath.substr(inputPath.size() - 4) == ".wat"
	)
	{
		isWat = true;
	}
	else if (
		inputPath.size() > 5 &&
		inputPath.substr(inputPath.size() - 5) == ".wasm"
	)
	{
		isWat = false;
	}
	else
	{
		std::cout << "Input file must be either .wat or .wasm" << std::endl;
		PrintHelpAndExit(progName);
	}

	auto inputFile = SimpleSysIO::SysCall::RBinaryFile::Open(inputPath);
	WasmWat::ModWrapper mod(nullptr);

	if (isWat)
	{
		auto input = inputFile->ReadBytes<std::string>();
		mod = WasmWat::Wat2Mod(
			inputPath,
			input,
			WasmWat::ReadWatConfig()
		);
	}
	else
	{
		auto input = inputFile->ReadBytes<std::vector<uint8_t>>();
		mod = WasmWat::Wasm2Mod(
			inputPath,
			input,
			WasmWat::ReadWasmConfig()
		);
	}

	return mod;
}


static void WriteModule(
	const std::string& progName,
	const std::string& outputPath,
	const WasmWat::ModWrapper& mod
)
{
	bool isWat = false;
	if (
		outputPath.size() > 4 &&
		outputPath.substr(outputPath.size() - 4) == ".wat"
	)
	{
		isWat = true;
	}
	else if (
		outputPath.size() > 5 &&
		outputPath.substr(outputPath.size() - 5) == ".wasm"
	)
	{
		isWat = false;
	}
	else
	{
		std::cout << "Output file must be either .wat or .wasm" << std::endl;
		PrintHelpAndExit(progName);
	}

	auto outputFile = SimpleSysIO::SysCall::WBinaryFile::Create(outputPath);

	if (isWat)
	{
		auto output = WasmWat::Mod2Wat(
			*(mod.m_ptr),
			WasmWat::WriteWatConfig()
		);
		outputFile->WriteBytes(output);
	}
	else
	{
		auto output = WasmWat::Mod2Wasm(
			*(mod.m_ptr),
			WasmWat::WriteWasmConfig()
		);
		outputFile->WriteBytes(output);
	}
}


static int CommandAdjJson(int argc, char* argv[])
{
	const std::string progName = argv[0];
	if (argc != 4)
	{
		PrintHelpAndExit(progName);
	}

	const std::string inputPath = argv[2];
	const std::string outputPath = argv[3];

	auto mod = ReadModule(progName, inputPath);

	std::vector<WasmCounter::GraphPtr> outGraphs;

	WasmCounter::Instrument(
		*(mod.m_ptr),
		&outGraphs
	);

	SimpleObjects::List jsonGraphs;

	for (size_t i = 0; i < outGraphs.size(); ++i)
	{
		const auto& graph = *(outGraphs[i]);
		std::cout <<
			"Generating adjacency JSON for func [" << i << "]" <<
			graph.m_funcName << std::endl;

		jsonGraphs.push_back(
			WasmCounter::Block2AdjacencyJson(graph)
		);
	}

	SimpleObjects::Dict jsonOutput;
	jsonOutput[SimpleObjects::String("graphs")] = std::move(jsonGraphs);

	SimpleJson::WriterConfig writerCfg;
	writerCfg.m_indent = '\t';
	auto strOutput = SimpleJson::DumpStr(jsonOutput, writerCfg);
	SimpleSysIO::SysCall::WBinaryFile::Create(outputPath)->WriteBytes(strOutput);

	return 0;
}


static int CommandInstrument(int argc, char* argv[])
{
	const std::string progName = argv[0];
	if (argc != 4)
	{
		PrintHelpAndExit(progName);
	}

	const std::string inputPath = argv[2];
	const std::string outputPath = argv[3];

	auto mod = ReadModule(progName, inputPath);

	WasmCounter::Instrument(*(mod.m_ptr));

	WriteModule(progName, outputPath, mod);

	return 0;
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		PrintHelpAndExit(argv[0]);
	}

	const std::string cmd = argv[1];

	if (cmd == "Instrument")
	{
		return CommandInstrument(argc, argv);
	}
	else if (cmd == "AdjJson")
	{
		return CommandAdjJson(argc, argv);
	}
	else
	{
		std::cout << "Unknown command: " << cmd << std::endl;
		PrintHelpAndExit(argv[0]);
	}
}
