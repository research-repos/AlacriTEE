// Copyright (c) 2024 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <cstdio>
#include <cstdint>

#include <chrono>
#include <string>
#include <vector>


uint64_t gs_startUs = 0;
uint64_t gs_stopUs = 0;


class Logger
{
public: // static members:

	static void PrintStr(
		const std::string name,
		const std::string level,
		const std::string& str
	)
	{
		printf("[Native] %s(%s): %s\n", name.c_str(), level.c_str(), str.c_str());
	}

public:
	Logger(const std::string& name) :
		m_name(name)
	{}

	void Info(const std::string& msg) const
	{
		PrintStr(m_name, "INFO", msg);
	}

private:

	std::string m_name;
}; // class Logger


static Logger gs_logger("WasmRuntime::WasmExecEnv");


void NativePrintStr(const std::string& str)
{
	// The logger should add new lines to the end of each call
	// so we want to remove spaces or new lines at the end of the string
	// to avoid extra new lines.

	size_t endPos = str.find_last_not_of(" \t\n\r");

	gs_logger.Info(
		(endPos != std::string::npos) ?
			str.substr(0, endPos + 1) :
			str
	);
}


uint64_t GetTimestampUs()
{
	auto now = std::chrono::system_clock::now();
	auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
		now.time_since_epoch()
	);
	return static_cast<uint64_t>(nowUs.count());
}


extern "C" int32_t enclave_wasm_main(
	uint32_t eIdSecSize, uint32_t msgSecSize
);


extern "C" void enclave_wasm_exit(int32_t exitCode)
{
	std::string msg = "Exit with code " + std::to_string(exitCode) + ".";
	NativePrintStr(msg);
	exit(exitCode);
}


extern "C" void enclave_wasm_print_string(const char * msg)
{
	NativePrintStr(msg);
}


extern "C" void enclave_wasm_start_benchmark()
{
	if (gs_startUs != 0 || gs_stopUs != 0)
	{
		throw std::runtime_error("Stopwatch already started");
	}

	NativePrintStr("Starting stopwatch...");

	gs_startUs = GetTimestampUs();
}

extern "C" void enclave_wasm_counter_exceed()
{
	NativePrintStr("Counter exceeded.");
	exit(1);
}

extern "C" void enclave_wasm_stop_benchmark()
{
	if (gs_startUs == 0 || gs_stopUs != 0)
	{
		throw std::runtime_error("Stopwatch not started or already stopped");
	}

	gs_stopUs = GetTimestampUs();

	std::string msg =
		"Stopwatch stopped. "
		"(Started @ " + std::to_string(gs_startUs) + " us,"
		" ended @ " + std::to_string(gs_stopUs) + " us)";
	NativePrintStr(msg);
}

int main(int argc, char* argv[])
{
	static constexpr size_t sk_repeatTimes = 5;

	Logger logger("PolybenchTester::EnclaveWasmMain");

	for (size_t i = 0; i < sk_repeatTimes; ++i)
	{
		gs_startUs = 0;
		gs_stopUs = 0;
		logger.Info("=====> Starting to run Enclave WASM program (type=plain)...");

		enclave_wasm_main(
			static_cast<uint32_t>(argc),
			static_cast<uint32_t>(argc)
		);

		auto duration  = gs_stopUs - gs_startUs;
		std::string endMsg =
			"<===== Finished to run Enclave WASM program; report: {"
				"\"type\":\"plain\", "
				"\"start_time\":" + std::to_string(gs_startUs) + ", "
				"\"end_time\":"   + std::to_string(gs_stopUs)   + ", "
				"\"duration\":"   + std::to_string(duration)  + ""
			"}";
		logger.Info(endMsg);
	}
	return 0;
}

