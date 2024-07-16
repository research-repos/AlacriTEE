# Copyright 2023 WasmCounter
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

option(
	WASMCOUNTER_INSTALL_HEADERS
	"Install WASM Counter headers."
	ON
)

add_library(WasmCounter INTERFACE)
target_include_directories(WasmCounter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(WASMCOUNTER_INSTALL_HEADERS)

	file(GLOB headers "WasmCounter/*.hpp")

	install(FILES ${headers}
		DESTINATION include/WasmCounter
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)

	# add_subdirectory(WasmCounter)

endif(WASMCOUNTER_INSTALL_HEADERS)

