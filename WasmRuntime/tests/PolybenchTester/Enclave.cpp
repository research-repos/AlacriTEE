// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "EnclaveMain.hpp"


extern "C" {

void ecall_enclave_wasm_main(
	const uint8_t *wasm_file, size_t wasm_file_size,
	const uint8_t *wasm_nopt_file, size_t wasm_nopt_file_size
)
{
	PolybenchTester::EnclaveWasmMain(
		wasm_file, wasm_file_size,
		wasm_nopt_file, wasm_nopt_file_size
	);
}

} // extern "C"
