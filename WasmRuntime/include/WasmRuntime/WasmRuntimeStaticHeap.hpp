// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "WasmRuntime.hpp"

#include <memory>

#include <wasm_export.h>

#include "Internal/make_unique.hpp"
#include "EnclaveWasmNatives.hpp"
#include "Exception.hpp"


namespace WasmRuntime
{


class WasmRuntimeStaticHeap :
	public WasmRuntime
{
public:

	static std::unique_ptr<WasmRuntimeStaticHeap> MakeUnique(
		std::unique_ptr<SystemIO> sysIO,
		uint32_t heapSize
	)
	{
		return Internal::make_unique<WasmRuntimeStaticHeap>(
			std::move(sysIO),
			heapSize
		);
	}

	using Base = WasmRuntime;

public:

	WasmRuntimeStaticHeap(
		std::unique_ptr<SystemIO> sysIO,
		uint32_t heapSize
	) :
		Base(std::move(sysIO)),
		m_heapSize(heapSize),
		m_heap(Internal::make_unique<uint8_t[]>(heapSize))
	{
		RuntimeInitArgs init_args;
		std::memset(&init_args, 0, sizeof(RuntimeInitArgs));

		init_args.mem_alloc_type = Alloc_With_Pool;
		init_args.mem_alloc_option.pool.heap_buf = m_heap.get();
		init_args.mem_alloc_option.pool.heap_size = heapSize;

		/* initialize runtime environment */
		if (!wasm_runtime_full_init(&init_args))
		{
			throw Exception("Init runtime environment failed");
		}

		if (!enclave_wasm_reg_natives())
		{
			throw Exception("Failed to register Enclave WASM native symbols");
		}
	}

	WasmRuntimeStaticHeap(const WasmRuntimeStaticHeap&) = delete;

	WasmRuntimeStaticHeap(WasmRuntimeStaticHeap&& other) = delete;

	virtual ~WasmRuntimeStaticHeap() noexcept
	{
		//wasm_runtime_memory_destroy();
		enclave_wasm_unreg_natives();
		wasm_runtime_destroy();
		m_heap.reset();
	}

	WasmRuntimeStaticHeap& operator=(const WasmRuntimeStaticHeap&) = delete;

	WasmRuntimeStaticHeap& operator=(WasmRuntimeStaticHeap&&) = delete;

private:

	uint32_t m_heapSize;
	std::unique_ptr<uint8_t[]> m_heap;

}; // class WasmRuntimeStaticHeap


} // namespace WasmRuntime

