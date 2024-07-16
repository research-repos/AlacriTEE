// Copyright (c) 2023 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <stdint.h>
#include <stdio.h>

extern void enclave_wasm_counter_exceed(void);
extern void enclave_wasm_print_string(const char*);

void enclave_wasm_prerequisite_imports(void)
{
	enclave_wasm_counter_exceed();
}

void enclave_wasm_debug_marked_point(uint32_t idx)
{
	char buf[128];
	sprintf(buf, "ENCLAVE_DEBUG_MP %d\n", idx);
	enclave_wasm_print_string(buf);
}

