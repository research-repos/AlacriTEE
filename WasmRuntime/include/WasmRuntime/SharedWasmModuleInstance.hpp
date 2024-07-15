// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "SharedObject.hpp"

#include <memory>

#include "Internal/make_unique.hpp"
#include "WasmModuleInstance.hpp"
#include "SharedWasmExecEnv.hpp"
#include "InstMemPtr.hpp"


namespace WasmRuntime
{


class SharedWasmModuleInstance :
	public SharedObject<WasmModuleInstance>
{
public: // static members

	using Base = SharedObject<WasmModuleInstance>;

public:

	using Base::Base;

	SharedWasmExecEnv CreateExecEnv(uint32_t stackSize)
	{
		auto execEnv = WasmExecEnv::Create(
			get(),
			stackSize
		);
		return SharedWasmExecEnv(
			Internal::make_unique<WasmExecEnv>(std::move(execEnv))
		);
	}


	template<class _T, class... _Args>
	inline typename Internal::InstMemPtrIf<_T>::Single
	NewMem(_Args&&... args)
	{
		return MakeInstMemPtr<_T>(get(), std::forward<_Args>(args)...);
	}


	template<class _T>
	inline typename Internal::InstMemPtrIf<_T>::ArrayUnknownBound
	NewMem(size_t n)
	{
		return MakeInstMemPtr<_T>(get(), n);
	}


	template<class _T, class... _Args>
	typename Internal::InstMemPtrIf<_T>::ArrayKnownBound
	NewMem(_Args&&... args) = delete;

}; // class SharedWasmModuleInstance


} // namespace WasmRuntime



