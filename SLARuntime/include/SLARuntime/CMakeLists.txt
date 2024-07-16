# Copyright (c) 2024 SLARuntime Authors
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

if(SLARUNTIME_INSTALL_HEADERS)

	file(GLOB headers_common "Common/*.hpp")

	install(
		FILES ${headers_common}
		DESTINATION include/SLARuntime/Common
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)


	file(GLOB headers_untrusted "Untrusted/*.hpp")

	install(
		FILES ${headers_untrusted}
		DESTINATION include/SLARuntime/Untrusted
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)


	file(GLOB headers_trusted "Trusted/*.hpp")

	install(
		FILES ${headers_trusted}
		DESTINATION include/SLARuntime/Trusted
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)

endif(SLARUNTIME_INSTALL_HEADERS)
