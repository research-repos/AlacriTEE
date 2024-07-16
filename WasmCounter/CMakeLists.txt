# Copyright (c) 2024 WasmCounter
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.18)


project(WasmCounter LANGUAGES C CXX VERSION 0.0.1)


option(WASMCOUNTER_ENABLE_ENCLAVE "Enable enclave targets" ON)
option(WASMCOUNTER_ENABLE_TEST    "Enable test targets"   OFF)
option(WASMCOUNTER_ENABLE_UTIL    "Enable util targets"   OFF)


option(WASMCOUNTER_STANDALONE_BUILD    "Enable standalone builds"   OFF)

if(WASMCOUNTER_STANDALONE_BUILD)
	include(cmake/StandaloneBuild.cmake)
endif()

################################################################################
# Set compile options
################################################################################

if(MSVC)
	set(WASMCOUNTER_COMMON_OPTIONS /W3 /WX /EHsc /MP /GR /Zc:__cplusplus
		# Mitigate issues in SGXSDK
		/wd4815 # zero-sized array
		/wd4310 # cast truncates in #include <limits>
		# Mitigate issues in WABT library
		/wd4100 # unreferenced formal parameter
		/wd4201 # nameless struct/union
		/wd4244 # conversion from 'x' to 'y', possible loss of data
		/wd4245 # conversion from 'x' to 'y', signed/unsigned mismatch
		/wd4267 # conversion from 'x' to 'y', possible loss of data
	)
	set(WASMCOUNTER_DEBUG_OPTIONS /MTd /Od /Zi /DDEBUG)
	set(WASMCOUNTER_RELEASE_OPTIONS /MT /Ox /Oi /Ob2 /fp:fast)# /DNDEBUG
else()
	set(WASMCOUNTER_COMMON_OPTIONS -pthread -Wall -Werror)
	set(WASMCOUNTER_DEBUG_OPTIONS -O0 -g -DDEBUG)
	set(WASMCOUNTER_RELEASE_OPTIONS -O2) #-DNDEBUG defined by default
endif()

set(
	WASMCOUNTER_DEBUG_OPTIONS
		${WASMCOUNTER_COMMON_OPTIONS}
		${WASMCOUNTER_DEBUG_OPTIONS}
)
set(
	WASMCOUNTER_RELEASE_OPTIONS
		${WASMCOUNTER_COMMON_OPTIONS}
		${WASMCOUNTER_RELEASE_OPTIONS}
)

##################################################
# subdirectories
##################################################

add_subdirectory(include)
add_subdirectory(src)

if(${WASMCOUNTER_ENABLE_TEST})
	enable_testing()
	add_subdirectory(test)
endif()

