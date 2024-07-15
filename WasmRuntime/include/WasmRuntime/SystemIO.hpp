// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <string>

#include "Internal/make_unique.hpp"


namespace WasmRuntime
{


class SystemIO
{
public:
	SystemIO() = default;

	virtual ~SystemIO() = default;

	virtual uint64_t GetTimestampUs() const = 0;

}; // class SystemIO


class SystemIONull :
	public SystemIO
{
public: // static members:

	static std::unique_ptr<SystemIONull> MakeUnique()
	{
		return Internal::make_unique<SystemIONull>();
	}

public:
	SystemIONull() = default;

	virtual ~SystemIONull() = default;

	virtual uint64_t GetTimestampUs() const override
	{
		return 0;
	}

}; // class SystemIONull


} // namespace WasmRuntime

