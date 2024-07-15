# Copyright (c) 2024 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


set(
	WASMRUNTIME_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/enclave_wasm_natives.c
	${CMAKE_CURRENT_SOURCE_DIR}/EnclaveWasmNatives.cpp
	CACHE INTERNAL "WasmRuntime source files" FORCE
)

