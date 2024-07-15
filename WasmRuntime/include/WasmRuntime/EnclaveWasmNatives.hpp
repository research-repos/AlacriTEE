// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <memory>


typedef void (*os_print_function_t)(const char *message);


extern "C" {


void wasm_os_set_print_function(os_print_function_t pf);


bool enclave_wasm_reg_natives();


bool enclave_wasm_unreg_natives();


} // extern "C"

