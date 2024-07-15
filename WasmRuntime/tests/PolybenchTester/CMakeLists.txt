# Copyright (c) 2023 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


include(DecentEnclaveIntelSgx)


decent_enclave_print_config_sgx()


set(PolybenchTester_COMMON_DEF
	DECENTENCLAVE_DEV_LEVEL_0
	POLYBENCHTESTER_SRC_DIR="${CMAKE_CURRENT_LIST_DIR}"
	"WASMRUNTIME_LOGGING_HEADER=<WasmRuntime/LoggingImpl.hpp>"
	"WASMRUNTIME_LOGGER_FACTORY=typename ::WasmRuntime::LoggerFactoryImpl"
)


decent_enclave_add_target_sgx(PolybenchTester
	UNTRUSTED_SOURCE
		${WASMRUNTIME_SRC_FILES}
		${CMAKE_CURRENT_LIST_DIR}/Main.cpp
	UNTRUSTED_DEF
		${PolybenchTester_COMMON_DEF}
	UNTRUSTED_INCL_DIR
		""
	UNTRUSTED_COMP_OPT
		$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>
		$<$<CONFIG:DebugSimulation>:${DEBUG_OPTIONS}>
		$<$<CONFIG:Release>:${RELEASE_OPTIONS}>
	UNTRUSTED_LINK_OPT ""
	UNTRUSTED_LINK_LIB
		WasmRuntime
		iwasm_static
	TRUSTED_SOURCE
		${WASMRUNTIME_SRC_FILES}
		${CMAKE_CURRENT_LIST_DIR}/Enclave.cpp
	TRUSTED_DEF
		${PolybenchTester_COMMON_DEF}
	TRUSTED_INCL_DIR
		""
	TRUSTED_COMP_OPT
		$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>
		$<$<CONFIG:DebugSimulation>:${DEBUG_OPTIONS}>
		$<$<CONFIG:Release>:${RELEASE_OPTIONS}>
	TRUSTED_LINK_OPT   ""
	TRUSTED_LINK_LIB
		IntelSGX::Trusted::pthread
		vmlib_decent_sgx
		WasmRuntime
		EnclaveWasmWat_core_trusted
		EnclaveWasmWat_trusted
	EDL_PATH
		${CMAKE_CURRENT_LIST_DIR}/Enclave.edl
	EDL_INCLUDE
		""
	EDL_OUTPUT_DIR
		${CMAKE_CURRENT_LIST_DIR}
	SIGN_CONFIG
		${CMAKE_CURRENT_LIST_DIR}/Enclave.config.xml
	SIGN_KEY
		${CMAKE_CURRENT_LIST_DIR}/Enclave_private.pem
)
