// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <memory>


#include <DecentEnclave/Common/Time.hpp>
#include <WasmRuntime/Internal/make_unique.hpp>
#include <WasmRuntime/SystemIO.hpp>


namespace End2End
{


class SystemIO :
	public WasmRuntime::SystemIO
{
public: // static members:

	static std::unique_ptr<SystemIO> MakeUnique()
	{
		return WasmRuntime::Internal::make_unique<SystemIO>();
	}

public:

	SystemIO() = default;

	virtual ~SystemIO() = default;

	virtual uint64_t GetTimestampUs() const override
	{
		return DecentEnclave::Common::UntrustedTime::TimestampMicrSec();
	}

}; // class SystemIO


} // namespace End2End

