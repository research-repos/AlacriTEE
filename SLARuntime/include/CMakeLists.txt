# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


option(
	SLARUNTIME_INSTALL_HEADERS
	"Install SLA Runtime headers."
	ON
)

add_library(SLARuntime INTERFACE)
target_include_directories(SLARuntime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(SLARUNTIME_INSTALL_HEADERS)

	file(GLOB headers "SLARuntime/*.hpp")

	install(
		FILES ${headers}
		DESTINATION include/SLARuntime
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)

	add_subdirectory(SLARuntime)

endif(SLARUNTIME_INSTALL_HEADERS)

