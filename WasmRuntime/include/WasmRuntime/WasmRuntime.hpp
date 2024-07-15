// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "EnclaveWasmNatives.hpp"
#include "Logging.hpp"
#include "SystemIO.hpp"


namespace WasmRuntime
{


class WasmRuntime
{
protected: // static members:

	static void CRuntimeLogCallback(const char* msg)
	{
		static Logger s_logger = LoggerFactory::GetLogger(
			"WasmRuntime::WasmRuntime::CRuntime"
		);

		s_logger.Info(msg);
	}

public:

	WasmRuntime(std::unique_ptr<SystemIO> sysIO) :
		m_logger(LoggerFactory::GetLogger("WasmRuntime::WasmRuntime")),
		m_sysIO(std::move(sysIO))
	{
#ifdef DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
		wasm_os_set_print_function(CRuntimeLogCallback);
#endif // DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
	}

	WasmRuntime(const WasmRuntime&) = delete;

	WasmRuntime(WasmRuntime&&) = delete;

	virtual ~WasmRuntime() noexcept
	{}

	WasmRuntime& operator=(const WasmRuntime&) = delete;

	WasmRuntime& operator=(WasmRuntime&&) = delete;

	const SystemIO& GetSystemIO() const
	{
		return *m_sysIO;
	}

protected:

	Logger m_logger;

private:

	std::unique_ptr<SystemIO> m_sysIO;
}; // class WasmRuntime


} // namespace WasmRuntime

