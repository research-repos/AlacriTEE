// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <string>


#ifndef WASMRUNTIME_LOGGING_HEADER
	// Logging is disabled


namespace WasmRuntime
{
namespace Internal
{


class DummyLogger
{
public:

	DummyLogger(const std::string&)
	{}

	~DummyLogger() = default;

	void Debug(const std::string&) const
	{}

	void Info(const std::string&) const
	{}

	void Warn(const std::string&) const
	{}

	void Error(const std::string&) const
	{}
}; // class DummyLogger


struct DummyLoggerFactory
{
	using LoggerType = DummyLogger;

	static LoggerType GetLogger(const std::string& name)
	{
		return LoggerType(name);
	}
}; // struct DummyLoggerFactory


} // namespace Internal


using LoggerFactory = Internal::DummyLoggerFactory;


} // namespace WasmRuntime


#else // !WASMRUNTIME_LOGGING_HEADER
	// Logging is enabled


#	include WASMRUNTIME_LOGGING_HEADER


namespace WasmRuntime
{


using LoggerFactory = WASMRUNTIME_LOGGER_FACTORY;


} // namespace WasmRuntime


#endif // !WASMRUNTIME_LOGGING_HEADER


namespace WasmRuntime
{


using Logger = typename LoggerFactory::LoggerType;


} // namespace WasmRuntime

