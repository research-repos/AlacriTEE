// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <stddef.h>

#include <wasm_export.h>

typedef void (*os_print_function_t)(const char *message);
extern void emscripten_memcpy_js(
	wasm_exec_env_t exec_env,
	void* dest,
	const void* src,
	size_t n
);
extern int enclave_wasm_sum(wasm_exec_env_t exec_env , int a, int b);
extern void enclave_wasm_print_string(wasm_exec_env_t exec_env, const char * msg);
extern void enclave_wasm_start_benchmark(wasm_exec_env_t exec_env);
extern void enclave_wasm_stop_benchmark(wasm_exec_env_t exec_env);
extern void enclave_wasm_exit(wasm_exec_env_t exec_env, int exit_code);
extern void enclave_wasm_counter_exceed(wasm_exec_env_t exec_env);
extern uint32_t enclave_wasm_get_event_id_len(wasm_exec_env_t exec_env);
extern uint32_t enclave_wasm_get_event_data_len(wasm_exec_env_t exec_env);
extern uint32_t enclave_wasm_get_event_id(wasm_exec_env_t exec_env, void* wasmPtr, uint32_t len);
extern uint32_t enclave_wasm_get_event_data(wasm_exec_env_t exec_env, uint32_t wasmPtr, uint32_t len);


static NativeSymbol gs_EnclaveWasmNatives[] =
{
	{
		"emscripten_memcpy_js", // WASM function name
		emscripten_memcpy_js,   // the native function pointer
		"(**~)",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_sum", // WASM function name
		enclave_wasm_sum,   // the native function pointer
		"(ii)i",           // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_print", // WASM function name
		enclave_wasm_print_string,   // the native function pointer
		"($)",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_print_string", // WASM function name
		enclave_wasm_print_string,   // the native function pointer
		"($)",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_start_benchmark", // WASM function name
		enclave_wasm_start_benchmark,   // the native function pointer
		"()",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_stop_benchmark", // WASM function name
		enclave_wasm_stop_benchmark,   // the native function pointer
		"()",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_get_event_id_len", // WASM function name
		enclave_wasm_get_event_id_len,   // the native function pointer
		"()i",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_get_event_data_len", // WASM function name
		enclave_wasm_get_event_data_len,   // the native function pointer
		"()i",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_get_event_id", // WASM function name
		enclave_wasm_get_event_id,   // the native function pointer
		"(*~)i",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_get_event_data", // WASM function name
		enclave_wasm_get_event_data,   // the native function pointer
		"(*~)i",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_exit", // WASM function name
		enclave_wasm_exit,   // the native function pointer
		"(i)",               // the function prototype signature
		NULL,
	},
	{
		"enclave_wasm_counter_exceed", // WASM function name
		enclave_wasm_counter_exceed,   // the native function pointer
		"()",               // the function prototype signature
		NULL,
	},
};


// Register WASM native functions
// Ref: https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md


bool enclave_wasm_reg_natives()
{
	const int symNum = sizeof(gs_EnclaveWasmNatives) / sizeof(NativeSymbol);
	return wasm_runtime_register_natives(
			"env",
			gs_EnclaveWasmNatives,
			symNum
		);
}


bool enclave_wasm_unreg_natives()
{
	return wasm_runtime_unregister_natives(
		"env",
		gs_EnclaveWasmNatives
	);
}
