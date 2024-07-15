// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstdint>
#include <cstring>

#include <memory>
#include <stdexcept>
#include <string>

#ifdef DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED

#include <sgx_error.h>

extern "C" sgx_status_t ocall_decent_untrusted_timestamp_us(uint64_t* ret_val);

#else // !DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED

extern "C" uint64_t ocall_decent_untrusted_timestamp_us();

#endif // DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED


#include <WasmRuntime/Internal/make_unique.hpp>
#include <WasmRuntime/SystemIO.hpp>


namespace PolybenchTester
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
#ifdef DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
		uint64_t ret = 0;
		sgx_status_t sgxRet = ocall_decent_untrusted_timestamp_us(&ret);
		if (sgxRet != SGX_SUCCESS)
		{
			throw std::runtime_error("Failed to get timestamp from ocall");
		}
		return ret;
#else // !DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
		return ocall_decent_untrusted_timestamp_us();
#endif // DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
	}

}; // class SystemIO


} // namespace PolybenchTester

