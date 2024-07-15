// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "WamrUniquePtr.hpp"

#include <memory>

#include <wasm_export.h>

#include "Exception.hpp"
#include "WasmModule.hpp"


namespace WasmRuntime
{


template<typename _GlobalType>
struct WasmModuleInstanceGlobalGetter;

template<>
struct WasmModuleInstanceGlobalGetter<uint64_t>
{
	static uint64_t Get(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst
	)
	{
		uint64_t retVal = 0;
		if (!wasm_runtime_get_global_i64(
				module_inst,
				global_inst,
				reinterpret_cast<int64_t*>(&retVal))
		)
		{
			throw Exception("Failed to get global value.");
		}
		return retVal;
	}

	static uint64_t& GetRef(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst
	)
	{
		void* p = wasm_runtime_get_global_addr(module_inst, global_inst);
		return p == nullptr ?
			throw Exception("Failed to get global address.") :
			*reinterpret_cast<uint64_t*>(p);
	}

	static void Set(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst,
		uint64_t val
	)
	{
		GetRef(module_inst, global_inst) = val;
	}
}; // struct WasmModuleInstanceGlobalGetter<uint64_t>

template<>
struct WasmModuleInstanceGlobalGetter<uint32_t>
{
	static uint32_t Get(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst
	)
	{
		uint32_t retVal = 0;
		if (!wasm_runtime_get_global_i32(
				module_inst,
				global_inst,
				reinterpret_cast<int32_t*>(&retVal))
		)
		{
			throw Exception("Failed to get global value.");
		}
		return retVal;
	}

	static uint32_t& GetRef(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst
	)
	{
		void* p = wasm_runtime_get_global_addr(module_inst, global_inst);
		return p == nullptr ?
			throw Exception("Failed to get global address.") :
			*reinterpret_cast<uint32_t*>(p);
	}

	static void Set(
		wasm_module_inst_t const module_inst,
		wasm_global_inst_t const global_inst,
		uint32_t val
	)
	{
		GetRef(module_inst, global_inst) = val;
	}
}; // struct WasmModuleInstanceGlobalGetter<uint32_t>


struct WasmModuleDeinstantiate
{
	void operator()(wasm_module_inst_t ptr) noexcept
	{
		wasm_runtime_deinstantiate(ptr);
	}
}; // struct WasmModuleDeinstantiate


class WasmModuleInstance :
	public WamrUniquePtr<
		typename std::remove_pointer<wasm_module_inst_t>::type,
		WasmModuleDeinstantiate
	>
{
public: // static members

	using Base = WamrUniquePtr<
		typename std::remove_pointer<wasm_module_inst_t>::type,
		WasmModuleDeinstantiate
	>;
	using pointer       = typename Base::pointer;
	using const_pointer = typename Base::const_pointer;

	friend class WasmExecEnv;

	template<typename _ValType>
	friend class InstMemPtrBase;

	static  WasmModuleInstance Instantiate(
		std::shared_ptr<WasmModule> module,
		uint32_t stackSize,
		uint32_t heapSize
	)
	{
		char errorBuf[512];

		wasm_module_inst_t ptr = wasm_runtime_instantiate(
			module->get(),
			stackSize,
			heapSize,
			errorBuf,
			sizeof(errorBuf)
		);

		if (ptr == nullptr)
		{
			throw Exception(errorBuf);
		}

		return WasmModuleInstance(ptr, module);
	}

public:

	WasmModuleInstance(
		wasm_module_inst_t ptr,
		std::shared_ptr<WasmModule> module
	) noexcept :
		Base(ptr), // base constructor is noexcept
		m_module(module) // shared_ptr copy is noexcept
	{}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WasmModuleInstance(const WasmModuleInstance&) = delete;

	/**
	 * @brief Construct a new WASM module instance object by moving.
	 *
	 * @param other Another WASM module instance object.
	 */
	WasmModuleInstance(WasmModuleInstance&& other) noexcept :
		Base(std::forward<Base>(other)), // base move is noexcept
		m_module(std::move(other.m_module)) // shared_ptr move is noexcept
	{}

	virtual ~WasmModuleInstance()
	{
		// wasm_runtime_deinstantiate(get());
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 * @return WasmModuleInstance&
	 */
	WasmModuleInstance& operator=(const WasmModuleInstance&) = delete;

	/**
	 * @brief Move assignment operator.
	 *
	 * @param other Another WASM module instance object.
	 * @return This WASM module instance object after moving.
	 */
	WasmModuleInstance& operator=(WasmModuleInstance&& other)
	{
		Base::operator=(std::forward<Base>(other));
		m_module = std::move(other.m_module);
		return *this;
	}

	template<typename _RetType>
	_RetType GetGlobal(const std::string& name) const
	{
		pointer ptr = const_cast<pointer>(get());
		auto global = wasm_runtime_lookup_global(ptr, name.c_str());
		if (global == nullptr)
		{
			throw Exception("Failed to find global with name " + name);
		}
		return WasmModuleInstanceGlobalGetter<_RetType>::Get(ptr, global);
	}

	template<typename _ValType>
	void SetGlobal(const std::string& name, const _ValType& val)
	{
		pointer ptr = const_cast<pointer>(get());
		auto global = wasm_runtime_lookup_global(ptr, name.c_str());
		if (global == nullptr)
		{
			throw Exception("Failed to find global with name " + name);
		}
		WasmModuleInstanceGlobalGetter<_ValType>::Set(ptr, global, val);
	}

	bool HasException() const noexcept
	{
		pointer ptr = const_cast<pointer>(get());
		return wasm_runtime_get_exception(ptr) != nullptr;
	}

	std::string GetExceptionMsg() const
	{
		pointer ptr = const_cast<pointer>(get());
		return wasm_runtime_get_exception(ptr);
	}

	const SystemIO& GetSystemIO() const
	{
		return m_module->GetSystemIO();
	}

private:

	std::shared_ptr<WasmModule> m_module;

}; // class WasmModuleInstance


} // namespace WasmRuntime

