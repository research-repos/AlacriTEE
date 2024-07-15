// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <string>

#ifdef DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED

#include <sgx_error.h>

extern "C" sgx_status_t ocall_print(const char* str);

#else // !DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED

extern "C" void ocall_print(const char* str);

#endif // DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED


namespace WasmRuntime
{

class LoggerImpl
{
public:

	LoggerImpl(const std::string& name) :
		m_name(name)
	{}

	LoggerImpl(const LoggerImpl& other) = delete;

	LoggerImpl(LoggerImpl&& other) :
		m_name(std::move(other.m_name))
	{}

	~LoggerImpl() = default;

	LoggerImpl& operator=(const LoggerImpl& other) = delete;

	LoggerImpl& operator=(LoggerImpl&& other)
	{
		m_name = std::move(other.m_name);
		return *this;
	}

	void Debug(const std::string& msg) const
	{
		return Log("DEBUG", msg);
	}

	void Info(const std::string& msg) const
	{
		return Log("INFO", msg);
	}

	void Warn(const std::string& msg) const
	{
		return Log("WARN", msg);
	}

	void Error(const std::string& msg) const
	{
		return Log("ERROR", msg);
	}

private:

	void Log(const std::string& level, const std::string& msg) const
	{
#ifdef DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
		static const std::string sk_header = "[Enclave] ";
#else // !DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED
		static const std::string sk_header = "[Untrusted] ";
#endif // DECENT_ENCLAVE_PLATFORM_SGX_TRUSTED

		ocall_print((
			sk_header + m_name + "(" + level + "): " + msg + "\n"
		).c_str());
	}

	std::string m_name;
}; // class LoggerImpl


struct LoggerFactoryImpl
{
	using LoggerType = LoggerImpl;

	static LoggerType GetLogger(const std::string& name)
	{
		return LoggerType(name);
	}
}; // struct LoggerFactoryImpl


} // namespace WasmRuntime

