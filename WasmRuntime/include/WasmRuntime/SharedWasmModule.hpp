// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "SharedObject.hpp"

#include <memory>

#include "Internal/make_unique.hpp"
#include "WasmModule.hpp"
#include "SharedWasmModuleInstance.hpp"


namespace WasmRuntime
{


class SharedWasmModule :
	public SharedObject<WasmModule>
{
public: // static members

	using Base = SharedObject<WasmModule>;

public:

	using Base::Base;

	SharedWasmModuleInstance Instantiate(
		uint32_t stackSize,
		uint32_t heapSize
	)
	{
		WasmModuleInstance inst = WasmModuleInstance::Instantiate(
			get(),
			stackSize,
			heapSize
		);
		return SharedWasmModuleInstance(
			Internal::make_unique<WasmModuleInstance>(std::move(inst))
		);
	}
}; // class SharedWasmModule


} // namespace WasmRuntime



