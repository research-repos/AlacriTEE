// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "WamrUniquePtr.hpp"

#include <cstdint>

#include <memory>
#include <vector>

#include <wasm_export.h>

#include "Internal/make_unique.hpp"
#include "Exception.hpp"
#include "WasmRuntime.hpp"


namespace WasmRuntime
{


struct WasmModuleUnload
{
	void operator()(wasm_module_t ptr) noexcept
	{
		wasm_runtime_unload(ptr);
	}
}; // struct WasmModuleUnload


class WasmModule :
	public WamrUniquePtr<
		typename std::remove_pointer<wasm_module_t>::type,
		WasmModuleUnload
	>
{
public: // static members

	using Base = WamrUniquePtr<
		typename std::remove_pointer<wasm_module_t>::type,
		WasmModuleUnload
	>;

	friend class WasmModuleInstance;

	static WasmModule Load(
		std::shared_ptr<WasmRuntime> runtime,
		const std::vector<uint8_t>& wasm
	)
	{
		char errorBuf[512];

		std::unique_ptr<std::vector<uint8_t> > wasmCopy =
			Internal::make_unique<std::vector<uint8_t> >(wasm);

		wasm_module_t ptr = wasm_runtime_load(
			wasmCopy->data(),
			wasmCopy->size(),
			errorBuf,
			sizeof(errorBuf)
		);

		if (ptr == nullptr)
		{
			throw Exception(errorBuf);
		}

		return WasmModule(ptr, std::move(wasmCopy), runtime);
	}

public:

	WasmModule(
		wasm_module_t ptr,
		std::unique_ptr<std::vector<uint8_t> > wasm,
		std::shared_ptr<WasmRuntime> runtime
	) noexcept :
		Base(ptr),
		m_wasm(std::move(wasm)), // unique_ptr move is noexcept
		m_runtime(std::move(runtime)) // shared_ptr move is noexcept
	{}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WasmModule(const WasmModule&) = delete;

	/**
	 * @brief Construct a new WASM module object by moving.
	 *
	 * @param other Another WASM module object.
	 */
	WasmModule(WasmModule&& other) noexcept :
		Base(std::forward<Base>(other)), // base move is noexcept
		m_wasm(std::move(other.m_wasm)), // unique_ptr move is noexcept
		m_runtime(std::move(other.m_runtime)) // shared_ptr move is noexcept
	{}

	virtual ~WasmModule()
	{
		// wasm_runtime_unload(get());
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WasmModule& operator=(const WasmModule&) = delete;

	/**
	 * @brief Move assignment.
	 *
	 * @param other Another WASM module object.
	 * @return This WASM module object.
	 */
	WasmModule& operator=(WasmModule&& other)
	{
		Base::operator=(std::forward<Base>(other));
		m_wasm = std::move(other.m_wasm); // unique_ptr move is noexcept
		m_runtime = std::move(other.m_runtime); // shared_ptr move is noexcept
		return *this;
	}

	const SystemIO& GetSystemIO() const
	{
		return m_runtime->GetSystemIO();
	}

private:

	std::unique_ptr<std::vector<uint8_t> > m_wasm;
	std::shared_ptr<WasmRuntime> m_runtime;

}; // class WasmModule


} // namespace WasmRuntime

