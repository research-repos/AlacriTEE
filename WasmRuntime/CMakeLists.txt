# Copyright (c) 2023 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


cmake_minimum_required(VERSION 3.18)

project(WasmRuntime LANGUAGES C CXX VERSION 0.0.1)

option(WASMRUNTIME_BUILD_TESTS "Build tests" ON)

################################################################################
# Set compile options
################################################################################

if(MSVC)
	set(COMMON_OPTIONS /W4 /WX /EHsc /MP /GR /Zc:__cplusplus
		/wd4815 # Mitigate issue in SGXSDK about zero-sized array
		/wd4310 # Mitigate issue in SGXSDK about cast truncates in #include <limits>
	)
	set(DEBUG_OPTIONS /MTd /Od /Zi /DDEBUG)
	set(RELEASE_OPTIONS /MT /Ox /Oi /Ob2 /fp:fast)# /DNDEBUG
#	set(CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG)
else()
	set(COMMON_OPTIONS -pthread -Wall -Wextra -Werror)
	set(DEBUG_OPTIONS -O0 -g -DDEBUG)
	set(RELEASE_OPTIONS -O2) #-DNDEBUG defined by default
endif()

set(DEBUG_OPTIONS ${COMMON_OPTIONS} ${DEBUG_OPTIONS})
set(RELEASE_OPTIONS ${COMMON_OPTIONS} ${RELEASE_OPTIONS})

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

################################################################################
# Setup CMake environment for enclave targets
################################################################################

#Removed Basic Runtime Checks in MSVC
if(MSVC)
	STRING (REGEX REPLACE "/RTC(su|[1su])" ""
		CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" ""
		CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" ""
		CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	STRING (REGEX REPLACE "/RTC(su|[1su])" ""
		CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

#Remove all standard libraries dependency here so that enclave DLL can be
# compiled properly. And it will be added back later for non-enclave apps.
set(UNTRUSTED_CXX_STANDARD_LIBRARIES ${CMAKE_CXX_STANDARD_LIBRARIES_INIT})
set(UNTRUSTED_C_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES_INIT})
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

# Add DebugSimulation to CMake configuration types
set(CMAKE_CONFIGURATION_TYPES Release Debug DebugSimulation)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug DebugSimulation)

set(CMAKE_CXX_FLAGS_DEBUGSIMULATION           ${CMAKE_CXX_FLAGS_DEBUG})
set(CMAKE_C_FLAGS_DEBUGSIMULATION             ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS_DEBUGSIMULATION    ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
set(CMAKE_SHARED_LINKER_FLAGS_DEBUGSIMULATION ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
set(CMAKE_STATIC_LINKER_FLAGS_DEBUGSIMULATION ${CMAKE_STATIC_LINKER_FLAGS_DEBUG})

##################################################
# Fetch external dependencies
##################################################

include(FetchContent)

add_subdirectory(libs/SimpleCMakeScripts)
simplecmakescripts_enable()

# Setup WASM options
set(
	WAMR_BUILD_FAST_JIT     1
	CACHE INTERNAL
	"Enable WAMR Fast JIT"
	FORCE
)
set(
	WAMR_BUILD_AOT          1
	CACHE INTERNAL
	"Enable WAMR AoT support"
	FORCE
)
set(
	WAMR_BUILD_LIBC_WASI    0
	CACHE INTERNAL
	"Disable WAMR Libc WASI support"
	FORCE
)
set(
	WAMR_BUILD_MULTI_MODULE 1
	CACHE INTERNAL
	"Enable WAMR Multiple modules support"
	FORCE
)
set(
	ASMJIT_STATIC           TRUE
	CACHE BOOL
	"Buid ASMJIT as static library"
	FORCE
)

add_subdirectory(libs/wasm-micro-runtime/product-mini/platforms/sgx)
add_subdirectory(libs/wasm-micro-runtime)
target_include_directories(
	iwasm_static
	PUBLIC ${WAMR_ROOT_DIR}/core/iwasm/include
)

add_subdirectory(libs/wabt/sgx)

##################################################
# Add source directories
##################################################

add_subdirectory(include)

if (WASMRUNTIME_BUILD_TESTS)
	add_subdirectory(tests)
endif()
