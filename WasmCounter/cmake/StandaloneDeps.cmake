# Copyright (c) 2024 WasmCounter
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.


cmake_minimum_required(VERSION 3.18)


include_guard(GLOBAL)


##################################################
# Fetch external dependencies
##################################################

include(FetchContent)

add_subdirectory(libs/SimpleCMakeScripts)
simplecmakescripts_enable()

add_subdirectory(libs/wabt/sgx)

add_subdirectory(libs/SimpleUtf)
add_subdirectory(libs/SimpleObjects)
add_subdirectory(libs/SimpleJson)
add_subdirectory(libs/SimpleSysIO)

