// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <cstdint>
#include <cstring>

#include <wasm_export.h>

#include <WasmRuntime/ExecEnvUserData.hpp>
#include <WasmRuntime/WasmExecEnv.hpp>


extern "C" void emscripten_memcpy_js(
	wasm_exec_env_t exec_env,
	void* dest,
	const void* src,
	size_t n
)
{
	(void)exec_env;
	std::memcpy(dest, src, n);
}

extern "C" int enclave_wasm_sum(wasm_exec_env_t exec_env, int a, int b)
{
	(void)exec_env;
	return a + b;
}


extern "C" void enclave_wasm_print_string(wasm_exec_env_t exec_env, const char * msg)
{
	using namespace WasmRuntime;
	const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);
	execEnv.NativePrintCStr(msg);
}


extern "C" void enclave_wasm_start_benchmark(wasm_exec_env_t exec_env)
{
	using namespace WasmRuntime;

	try
	{
		auto& execEnv = WasmExecEnv::FromUserData(exec_env);
		execEnv.GetUserData().StartStopwatch(execEnv);
	}
	catch (const std::exception& e)
	{
		wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
		wasm_runtime_set_exception(module_inst, e.what());
	}
}


extern "C" void enclave_wasm_stop_benchmark(wasm_exec_env_t exec_env)
{
	using namespace WasmRuntime;

	try
	{
		auto& execEnv = WasmExecEnv::FromUserData(exec_env);
		execEnv.GetUserData().StopStopwatch(execEnv);
	}
	catch (const std::exception& e)
	{
		wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
		wasm_runtime_set_exception(module_inst, e.what());
	}
}


extern "C" uint32_t enclave_wasm_get_event_id_len(wasm_exec_env_t exec_env)
{
	using namespace WasmRuntime;
	const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);
	return execEnv.GetUserData().GetEventId().size();
}


extern "C" uint32_t enclave_wasm_get_event_data_len(wasm_exec_env_t exec_env)
{
	using namespace WasmRuntime;
	const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);
	return execEnv.GetUserData().GetEventData().size();
}


extern "C" uint32_t enclave_wasm_get_event_id(
	wasm_exec_env_t exec_env,
	void* nativePtr,
	uint32_t len
)
{
	using namespace WasmRuntime;
	const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);

	uint8_t* ptr = static_cast<uint8_t*>(nativePtr);

	const auto& eventId = execEnv.GetUserData().GetEventId();
	size_t cpSize = len <= eventId.size() ? len : eventId.size();

	std::copy(eventId.begin(), eventId.begin() + cpSize, ptr);

	return eventId.size();
}


extern "C" uint32_t enclave_wasm_get_event_data(
	wasm_exec_env_t exec_env,
	void* nativePtr,
	uint32_t len
)
{
	using namespace WasmRuntime;
	const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);

	uint8_t* ptr = static_cast<uint8_t*>(nativePtr);

	const auto& eventData = execEnv.GetUserData().GetEventData();
	size_t cpSize = len <= eventData.size() ? len : eventData.size();

	std::copy(eventData.begin(), eventData.begin() + cpSize, ptr);

	return execEnv.GetUserData().GetEventData().size();
}


extern "C" void enclave_wasm_exit(wasm_exec_env_t exec_env, int exit_code)
{
	(void)exit_code;
	wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
	/* Here throwing exception is just to let wasm app exit,
		the upper layer should clear the exception and return
		as normal */
	wasm_runtime_set_exception(module_inst, "enclave wasm exit");
}


extern "C" void enclave_wasm_counter_exceed(wasm_exec_env_t exec_env)
{
	using namespace WasmRuntime;

	static const std::string sk_globalThresholdName = "enclave_wasm_threshold";
	static const std::string sk_globalCounterName = "enclave_wasm_counter";

	try
	{
		const auto& execEnv = WasmExecEnv::FromConstUserData(exec_env);

		uint64_t threshold = execEnv.GetModuleInstance().
			GetGlobal<uint64_t>(sk_globalThresholdName);
		uint64_t counter = execEnv.GetModuleInstance().
			GetGlobal<uint64_t>(sk_globalCounterName);
		wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);

		std::string msg = "counter exceed. ( "
			"Threshold: " + std::to_string(threshold) + ", "
			"Counter: " + std::to_string(counter) + ")" ;
		execEnv.NativePrintStr(msg);

		/* Here throwing exception is just to let wasm app exit,
		the upper layer should clear the exception and return
		as normal */
		wasm_runtime_set_exception(module_inst, "counter exceed");
	}
	catch (const std::exception& e)
	{
		wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
		wasm_runtime_set_exception(module_inst, e.what());
	}
}

