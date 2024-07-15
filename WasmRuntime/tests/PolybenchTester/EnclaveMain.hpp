// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstdint>
#include <cstring>

#include <vector>

#include <WasmRuntime/MainRunner.hpp>
#include <WasmRuntime/Logging.hpp>
#include <WasmRuntime/WasmRuntimeStaticHeap.hpp>

#include "SystemIO.hpp"


namespace PolybenchTester
{


inline bool EnclaveWasmMain(
	const uint8_t *wasm_file, size_t wasm_file_size,
	const uint8_t *wasm_nopt_file, size_t wasm_nopt_file_size
)
{
	using namespace WasmRuntime;
	static constexpr size_t sk_repeatTime = 5;

	auto logger = LoggerFactory::GetLogger("PolybenchTester::EnclaveWasmMain");

	try
	{
		std::vector<uint8_t> wasmBytecode(
			wasm_file,
			wasm_file + wasm_file_size
		);
		std::vector<uint8_t> instWasmBytecode(
			wasm_nopt_file,
			wasm_nopt_file + wasm_nopt_file_size
		);

		auto wasmRt = SharedWasmRuntime(
			WasmRuntimeStaticHeap::MakeUnique(
				PolybenchTester::SystemIO::MakeUnique(),
				70 * 1024 * 1024 // 70 MB
			)
		);

		std::vector<uint8_t> eventId = {
			'D', 'e', 'c', 'e', 'n', 't', '\0'
		};
		std::vector<uint8_t> msgContent = {
			'E', 'v', 'e', 'n', 't', 'M', 'e', 's', 's', 'a', 'g', 'e', '\0'
		};
		uint64_t threshold = std::numeric_limits<uint64_t>::max() / 2;

		{
			auto runner = MainRunner(
				wasmRt,
				wasmBytecode,
				eventId,
				msgContent,
				1 * 1024 * 1024,  // mod stack:  1 MB
				64 * 1024 * 1024, // mod heap:  64 MB
				1 * 1024 * 1024   // exec stack: 1 MB
			);
			for (size_t i = 0; i < sk_repeatTime; ++i)
			{
				logger.Info("=====> Starting to run Enclave WASM program (type=plain)...");
				runner.RunPlain();

				auto startTime = runner.GetUserData().GetStopwatchStartTime();
				auto endTime   = runner.GetUserData().GetStopwatchEndTime();
				auto duration  = endTime - startTime;

				std::string msg =
					"<===== Finished to run Enclave WASM program; report: {"
						"\"type\":\"plain\", "
						"\"start_time\":" + std::to_string(startTime) + ", "
						"\"end_time\":"   + std::to_string(endTime)   + ", "
						"\"duration\":"   + std::to_string(duration)  + ""
					"}";
				logger.Info(msg);

				runner.GetUserData().ResetStopwatch();
			}
		}

		{
			auto runner = MainRunner(
				wasmRt,
				instWasmBytecode,
				eventId,
				msgContent,
				1 * 1024 * 1024,  // mod stack:  1 MB
				64 * 1024 * 1024, // mod heap:  64 MB
				1 * 1024 * 1024   // exec stack: 1 MB
			);
			for (size_t i = 0; i < sk_repeatTime; ++i)
			{
				logger.Info("=====> Starting to run Enclave WASM program (type=instrumented)...");
				runner.RunInstrumented(threshold);

				auto startTime = runner.GetUserData().GetStopwatchStartTime();
				auto endTime   = runner.GetUserData().GetStopwatchEndTime();
				auto duration  = endTime - startTime;

				auto threshold = runner.GetThreshold();
				auto counter    = runner.GetCounter();

				std::string msg =
					"<===== Finished to run Enclave WASM program; report: {"
						"\"type\":\"instrumented\", "
						"\"start_time\":" + std::to_string(startTime) + ", "
						"\"end_time\":"   + std::to_string(endTime)   + ", "
						"\"duration\":"   + std::to_string(duration)  + ", "
						"\"threshold\":"  + std::to_string(threshold) + ", "
						"\"counter\":"    + std::to_string(counter)   + ""
					"}";
				logger.Info(msg);

				runner.ResetThresholdAndCounter();
				runner.GetUserData().ResetStopwatch();
			}
		}

		return true;
	}
	catch(const std::exception& e)
	{
		logger.Error(e.what());
		return false;
	}
}


} // namespace PolybenchTester

