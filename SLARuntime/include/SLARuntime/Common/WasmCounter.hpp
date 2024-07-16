// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstdint>

#include <vector>

#include <WasmWat/WasmWat.h>
#include <WasmCounter/WasmCounter.hpp>


namespace SLARuntime
{
namespace Common
{
namespace WasmCounter
{


inline std::vector<uint8_t> InstrumentWasm(
	const std::vector<uint8_t>& wasmCode
)
{
	auto mod = WasmWat::Wasm2Mod(
		"filename.wat",
		wasmCode,
		WasmWat::ReadWasmConfig()
	);

	WasmCounter::Instrument(*(mod.m_ptr));

	auto instWasmCode = WasmWat::Mod2Wasm(
		*(mod.m_ptr),
		WasmWat::WriteWasmConfig()
	);

	return instWasmCode;
}


} // namespace WasmCounter
} // namespace Common
} // namespace SLARuntime

