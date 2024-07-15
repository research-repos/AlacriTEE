# Copyright (c) 2024 WasmRuntime
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


option(
	WASMRUNTIME_INSTALL_HEADERS
	"Install WASM Runtime headers."
	ON
)

add_library(WasmRuntime INTERFACE)
target_include_directories(WasmRuntime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(WASMRUNTIME_INSTALL_HEADERS)

	file(GLOB headers "WasmRuntime/*.hpp")

	install(
		FILES ${headers}
		DESTINATION include/WasmRuntime
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)

	# add_subdirectory(WasmRuntime)

endif(WASMRUNTIME_INSTALL_HEADERS)

