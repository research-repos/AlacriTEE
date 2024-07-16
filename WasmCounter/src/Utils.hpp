// Copyright (c) 2023 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <array>

#include <src/ir.h>


namespace WasmCounter
{


inline const char* GetModuleFieldTypeName(wabt::ModuleFieldType type) {
	static constexpr std::array<const char*, 12> sk_moduleFieldTypeName = {
		"Func",
		"Global",
		"Import",
		"Export",
		"Type",
		"Table",
		"ElemSegment",
		"Memory",
		"DataSegment",
		"Start",
		"Tag",
		"Unknown",
	};
	static constexpr size_t sk_moduleFieldTypeNameN =
		std::tuple_size<decltype(sk_moduleFieldTypeName)>::value;

	return static_cast<size_t>(type) < sk_moduleFieldTypeNameN ?
		sk_moduleFieldTypeName[static_cast<size_t>(type)] :
		sk_moduleFieldTypeName[sk_moduleFieldTypeNameN - 1];
}

inline bool VarEq(const wabt::Var& lhs, const wabt::Var& rhs)
{
	return lhs.type() == rhs.type() &&
		(
			lhs.is_index() ? // is var an index?
				lhs.index() == rhs.index() : // it's an index, compare index
				lhs.name()  == rhs.name() // otherwise, it's a name, compare name
		);
}


} // namespace WasmCounter

