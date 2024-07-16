# Copyright (c) 2022 WasmCounter
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.18)


##############################
# WasmCounter
##############################

# untrusted version

add_library(
	WasmCounter_untrusted
	STATIC
		WasmCounter.cpp
)
if (NOT DEFINED WABT_SOURCES_ROOT_DIR)
	message(FATAL_ERROR "Failed to find WABT source directory")
endif()
target_include_directories(
	WasmCounter_untrusted
	PRIVATE
		${WABT_SOURCES_ROOT_DIR}
)
target_compile_options(
	WasmCounter_untrusted
	PRIVATE
		$<$<CONFIG:Debug>:${WASMCOUNTER_DEBUG_OPTIONS}>
		$<$<CONFIG:DebugSimulation>:${WASMCOUNTER_DEBUG_OPTIONS}>
		$<$<CONFIG:Release>:${WASMCOUNTER_RELEASE_OPTIONS}>
)
target_link_libraries(
	WasmCounter_untrusted
	PUBLIC
		WasmWat_untrusted
		WasmCounter
)
set_property(
	TARGET WasmCounter_untrusted
	PROPERTY
		MSVC_RUNTIME_LIBRARY
		MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:DebugSimulation>>:Debug>
)
set_property(
	TARGET WasmCounter_untrusted
	PROPERTY
		CXX_STANDARD 17
)


# trusted version

if(WASMCOUNTER_ENABLE_ENCLAVE)
	include(DecentEnclaveIntelSgx)

	add_library(
		WasmCounter_trusted
		STATIC
			WasmCounter.cpp
	)
	if (NOT DEFINED WABT_SOURCES_ROOT_DIR)
		message(FATAL_ERROR "Failed to find WABT source directory")
	endif()
	target_include_directories(
		WasmCounter_trusted
		PRIVATE
			${WABT_SOURCES_ROOT_DIR}
			${DECENTENCLAVE_SGXSDK_INCLUDE}
			${DECENTENCLAVE_SGXSDK_INCLUDE_C}
			${DECENTENCLAVE_SGXSDK_INCLUDE_CXX}
	)
	target_compile_definitions(
		WasmCounter_trusted
		PRIVATE
			DECENT_ENCLAVE_TRUSTED
			DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
	)
	target_compile_options(
		WasmCounter_trusted
		PRIVATE
			${DECENTENCLAVE_SGXSDK_TRUSTED_C_FLAGS}
			$<$<COMPILE_LANGUAGE:CXX>:${DECENTENCLAVE_SGXSDK_TRUSTED_CXX_FLAGS}>
			$<$<CONFIG:Debug>:${WASMCOUNTER_DEBUG_OPTIONS}>
			$<$<CONFIG:DebugSimulation>:${WASMCOUNTER_DEBUG_OPTIONS}>
			$<$<CONFIG:Release>:${WASMCOUNTER_RELEASE_OPTIONS}>
	)
	target_link_libraries(
		WasmCounter_trusted
		PUBLIC
			WasmWat_trusted
			WasmCounter
	)
	target_link_options(
		WasmCounter_trusted
		PRIVATE
			${DECENTENCLAVE_SGXSDK_TRUSTED_LINKER_FLAGS}
	)
	set_property(
		TARGET WasmCounter_trusted
		PROPERTY
			MSVC_RUNTIME_LIBRARY
			MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:DebugSimulation>>:Debug>
	)
	set_property(
		TARGET WasmCounter_trusted
		PROPERTY
			CXX_STANDARD 17
	)
endif()


##############################
# WasmCounterUtils
##############################

if (WASMCOUNTER_ENABLE_UTIL)

	include(${CMAKE_CURRENT_LIST_DIR}/../cmake/StandaloneDeps.cmake)

	add_executable(
		WasmCounterUtils
			UtilsMain.cpp
	)
	target_include_directories(
		WasmCounterUtils
		PRIVATE
			${WABT_SOURCES_ROOT_DIR}
	)
	target_compile_definitions(
		WasmCounterUtils
		PRIVATE
			SIMPLESYSIO_ENABLE_SYSCALL
	)
	target_compile_options(
		WasmCounterUtils
		PRIVATE
			$<$<CONFIG:Debug>:${WASMCOUNTER_DEBUG_OPTIONS}>
			$<$<CONFIG:DebugSimulation>:${WASMCOUNTER_DEBUG_OPTIONS}>
			$<$<CONFIG:Release>:${WASMCOUNTER_RELEASE_OPTIONS}>
	)
	target_link_libraries(
		WasmCounterUtils
		PUBLIC
			SimpleUtf
			SimpleObjects
			SimpleJson
			SimpleSysIO
			WasmCounter_untrusted
	)
	set_property(
		TARGET WasmCounterUtils
		PROPERTY
			MSVC_RUNTIME_LIBRARY
			MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:DebugSimulation>>:Debug>
	)
	set_property(
		TARGET WasmCounterUtils
		PROPERTY
			CXX_STANDARD 17
	)

endif()

