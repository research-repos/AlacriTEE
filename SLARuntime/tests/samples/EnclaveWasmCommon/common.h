// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef ENCLAVE_NATIVES_H
#define ENCLAVE_NATIVES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void enclave_wasm_print_string(const char * msg);

extern uint32_t enclave_wasm_get_event_id_len();

extern uint32_t enclave_wasm_get_event_data_len();

extern uint32_t enclave_wasm_get_event_id(uint8_t* buf, uint32_t buf_len);

extern uint32_t enclave_wasm_get_event_data(uint8_t* buf, uint32_t buf_len);

extern void enclave_wasm_counter_exceed(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ENCLAVE_NATIVES_H
