// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "SharedObject.hpp"

#include <memory>

#include "Internal/make_unique.hpp"
#include "WasmExecEnv.hpp"


namespace WasmRuntime
{


class SharedWasmExecEnv :
	public SharedObject<WasmExecEnv>
{
public: // static members

	using Base = SharedObject<WasmExecEnv>;

public:

	using Base::Base;


}; // class SharedWasmExecEnv


} // namespace WasmRuntime



