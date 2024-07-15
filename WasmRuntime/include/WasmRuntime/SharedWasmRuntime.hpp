// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "SharedObject.hpp"

#include <memory>

#include "Internal/make_unique.hpp"
#include "WasmRuntime.hpp"
#include "SharedWasmModule.hpp"


namespace WasmRuntime
{


class SharedWasmRuntime :
	public SharedObject<WasmRuntime>
{
public: // static members

	using Base = SharedObject<WasmRuntime>;

public:

	using Base::Base;

	SharedWasmModule LoadModule(const std::vector<uint8_t>& bytecode)
	{
		WasmModule mod = WasmModule::Load(get(), bytecode);
		return SharedWasmModule(
			Internal::make_unique<WasmModule>(std::move(mod))
		);
	}

}; // class SharedWasmRuntime


} // namespace WasmRuntime



