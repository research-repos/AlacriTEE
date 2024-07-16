// Copyright (c) 2022 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include <vector>
#include <memory>

#include <WasmWat/WasmWat.h>

namespace WasmCounter
{


namespace Internal
{

template<typename _T>
struct InCmpPtr : public std::unique_ptr<_T>
{
	using Base = std::unique_ptr<_T>;

	using Base::Base;

	InCmpPtr(Base&& other) :
		Base(std::forward<Base>(other))
	{}

	InCmpPtr(const InCmpPtr&) = delete;
	InCmpPtr(InCmpPtr&& other) :
		Base(std::forward<Base>(other))
	{}

	InCmpPtr& operator=(Base&& other)
	{
		Base::operator=(std::forward<Base>(other));
		return *this;
	}
	InCmpPtr& operator=(const InCmpPtr&) = delete;
	InCmpPtr& operator=(InCmpPtr&& other)
	{
		Base::operator=(std::forward<Base>(other));
		return *this;
	}

	~InCmpPtr();
}; // struct InCmpPtr

} // namespace Internal

struct Graph;
using GraphPtr = Internal::InCmpPtr<Graph>;

void Instrument(
	wabt::Module& mod,
	std::vector<GraphPtr >* outGraphs = nullptr
);

} // namespace WasmCounter
