// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <vector>

#include <wasm_export.h>

#include "Exception.hpp"


namespace WasmRuntime
{

inline wasm_val_t ToWasmVal(uint32_t val) noexcept
{
	wasm_val_t wasmVal;
	wasmVal.kind = wasm_valkind_enum::WASM_I32;
	reinterpret_cast<uint32_t&>(wasmVal.of.i32) = val;
	return wasmVal;
}


inline wasm_val_t ToWasmVal(int32_t val) noexcept
{
	wasm_val_t wasmVal;
	wasmVal.kind = wasm_valkind_enum::WASM_I32;
	wasmVal.of.i32 = val;
	return wasmVal;
}


inline wasm_val_t ToWasmVal(uint64_t val) noexcept
{
	wasm_val_t wasmVal;
	wasmVal.kind = wasm_valkind_enum::WASM_I64;
	reinterpret_cast<uint64_t&>(wasmVal.of.i64) = val;
	return wasmVal;
}


inline wasm_val_t ToWasmVal(int64_t val) noexcept
{
	wasm_val_t wasmVal;
	wasmVal.kind = wasm_valkind_enum::WASM_I64;
	wasmVal.of.i64 = val;
	return wasmVal;
}


template<typename _DestIt>
inline _DestIt PushWasmVal(_DestIt dest, uint32_t val)
{
	*dest++ = ToWasmVal(val);
	return dest;
}


template<typename _DestIt>
inline _DestIt PushWasmVal(_DestIt dest, int32_t val)
{
	*dest++ = ToWasmVal(val);
	return dest;
}


template<typename _DestIt>
inline _DestIt PushWasmVal(_DestIt dest, uint64_t val)
{
	*dest++ = ToWasmVal(val);
	return dest;
}


template<typename _DestIt>
inline _DestIt PushWasmVal(_DestIt dest, int64_t val)
{
	*dest++ = ToWasmVal(val);
	return dest;
}


template<typename _DestIt>
inline _DestIt ToWasmValList(_DestIt dest)
{
	return dest;
}


template<typename _DestIt, typename _Arg1, typename... _Args>
inline _DestIt ToWasmValList(_DestIt dest, _Arg1&& arg1, _Args&&... args)
{
	dest = PushWasmVal(dest, std::forward<_Arg1>(arg1));
	return ToWasmValList(dest, std::forward<_Args>(args)...);
}


template<typename... _Args>
inline std::vector<wasm_val_t> ToWasmValVector(_Args&&... args)
{
	std::vector<wasm_val_t> res;
	ToWasmValList(std::back_inserter(res), std::forward<_Args>(args)...);
	return res;
}


inline void ReadWasmVal(int32_t& dest, const wasm_val_t& src)
{
	if (src.kind != wasm_valkind_enum::WASM_I32)
	{
		throw Exception("Unmatched wasm_val_t kind");
	}
	dest = src.of.i32;
}


inline void ReadWasmVal(int64_t& dest, const wasm_val_t& src)
{
	if (src.kind != wasm_valkind_enum::WASM_I64)
	{
		throw Exception("Unmatched wasm_val_t kind");
	}
	dest = src.of.i64;
}


inline void ReadWasmVal(float& dest, const wasm_val_t& src)
{
	if (src.kind != wasm_valkind_enum::WASM_F32)
	{
		throw Exception("Unmatched wasm_val_t kind");
	}
	dest = src.of.f32;
}


inline void ReadWasmVal(double& dest, const wasm_val_t& src)
{
	if (src.kind != wasm_valkind_enum::WASM_F64)
	{
		throw Exception("Unmatched wasm_val_t kind");
	}
	dest = src.of.f64;
}


template<typename _TupleType, size_t _NumItemLeft>
struct WasmValListToTuple;


template<typename _TupleType>
struct WasmValListToTuple<_TupleType, 0>
{
	static void Read(
		_TupleType&,
		const wasm_val_t (&src)[std::tuple_size<_TupleType>::value + 1]
	)
	{
		(void)src;
	}
}; // struct WasmValListToTuple<_TupleType, 0>


template<typename _TupleType, size_t _NumItemLeft>
struct WasmValListToTuple
{
	static void Read(
		_TupleType& dest,
		const wasm_val_t (&src)[std::tuple_size<_TupleType>::value + 1]
	)
	{
		static constexpr size_t sk_currIdx =
			std::tuple_size<_TupleType>::value - _NumItemLeft;

		ReadWasmVal(std::get<sk_currIdx>(dest), src[sk_currIdx]);
		WasmValListToTuple<_TupleType, _NumItemLeft - 1>::Read(dest, src);
	}
}; // struct WasmValListToTuple<_TupleType, _NumItemLeft>

} // namespace WasmRuntime

