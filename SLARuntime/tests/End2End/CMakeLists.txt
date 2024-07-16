# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


include(DecentEnclaveIntelSgx)


decent_enclave_print_config_sgx()


set(End2End_COMMON_DEF
	DECENTENCLAVE_DEV_LEVEL_0
	END2END_SRC_DIR="${CMAKE_CURRENT_LIST_DIR}"
	"SLARUNTIME_LOGGING_HEADER=<DecentEnclave/Common/Logging.hpp>"
	"SLARUNTIME_LOGGER_FACTORY=typename ::DecentEnclave::Common::LoggerFactory"
	"WASMRUNTIME_LOGGING_HEADER=<DecentEnclave/Common/Logging.hpp>"
	"WASMRUNTIME_LOGGER_FACTORY=typename ::DecentEnclave::Common::LoggerFactory"
)

decent_enclave_add_target_sgx(End2End
	UNTRUSTED_SOURCE
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/SysIO_u.cpp
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/NetIO_u.cpp
		${CMAKE_CURRENT_LIST_DIR}/Untrusted/Main.cpp
	UNTRUSTED_DEF
		SIMPLESYSIO_ENABLE_SYSCALL
		CURL_STATICLIB
		${End2End_COMMON_DEF}
	UNTRUSTED_INCL_DIR
		""
	UNTRUSTED_COMP_OPT
		$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>
		$<$<CONFIG:DebugSimulation>:${DEBUG_OPTIONS}>
		$<$<CONFIG:Release>:${RELEASE_OPTIONS}>
	UNTRUSTED_LINK_OPT ""
	UNTRUSTED_LINK_LIB
		SimpleUtf
		SimpleObjects
		SimpleJson
		SimpleRlp
		SimpleSysIO
		SimpleConcurrency
		DecentEnclave
		mbedTLScpp
		mbedcrypto
		mbedx509
		mbedtls
		# libcurl
		# Boost::asio
	TRUSTED_SOURCE
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/AppLambdaHandler_t.cpp
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/Attestation_t.cpp
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/Crypto_t.cpp
		${DECENTENCLAVE_INCLUDE}/DecentEnclave/SgxEdgeSources/SysIO_t.cpp
		${CMAKE_CURRENT_LIST_DIR}/Trusted/Enclave.cpp
		${WASMRUNTIME_SRC_FILES}
	TRUSTED_DEF
		${End2End_COMMON_DEF}
	TRUSTED_INCL_DIR
		""
	TRUSTED_COMP_OPT
		$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>
		$<$<CONFIG:DebugSimulation>:${DEBUG_OPTIONS}>
		$<$<CONFIG:Release>:${RELEASE_OPTIONS}>
	TRUSTED_LINK_OPT   ""
	TRUSTED_LINK_LIB
		IntelSGX::Trusted::pthread
		SimpleUtf
		SimpleObjects
		SimpleJson
		SimpleRlp
		SimpleSysIO
		DecentEnclave
		WasmRuntime
		SLARuntime
		WasmCounter_trusted
		EclipseMonitor
		mbedTLScpp
		mbedcrypto_enclave
		mbedx509_enclave
		mbedtls_enclave
		vmlib_decent_sgx
	EDL_PATH
		${CMAKE_CURRENT_LIST_DIR}/Trusted/Enclave.edl
	EDL_INCLUDE
		${DECENTENCLAVE_INCLUDE}
	EDL_OUTPUT_DIR
		${CMAKE_CURRENT_LIST_DIR}
	SIGN_CONFIG
		${CMAKE_CURRENT_LIST_DIR}/Enclave.config.xml
	SIGN_KEY
		${CMAKE_CURRENT_LIST_DIR}/Enclave_private.pem
)
