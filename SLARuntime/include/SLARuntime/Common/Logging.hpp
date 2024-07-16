// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <string>


#ifndef SLARUNTIME_LOGGING_HEADER
	// Logging is disabled


namespace SLARuntime
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


namespace Common
{


using LoggerFactory = Internal::DummyLoggerFactory;


} // namespace Common
} // namespace SLARuntime


#else // !SLARUNTIME_LOGGING_HEADER
	// Logging is enabled


#	include SLARUNTIME_LOGGING_HEADER


namespace SLARuntime
{
namespace Common
{


using LoggerFactory = SLARUNTIME_LOGGER_FACTORY;


} // namespace Common
} // namespace SLARuntime


#endif // !SLARUNTIME_LOGGING_HEADER


namespace SLARuntime
{
namespace Common
{


using Logger = typename LoggerFactory::LoggerType;


} // namespace Common
} // namespace SLARuntime

