// Copyright (c) 2023 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <csignal>

#include <atomic>

#include <DecentEnclave/Common/Platform/Print.hpp>


namespace End2End
{

volatile std::atomic_int g_sigVal(0);

inline const char* GetSignalName(int sig)
{
	switch (sig)
	{
	case SIGINT:
		return "SIGINT";
	case SIGTERM:
		return "SIGTERM";
	default:
		return "Unknown";
	}
}

} // namespace End2End


extern "C" inline void SignalHandler(int sig)
{
	using namespace End2End;

	DecentEnclave::Common::Platform::Print::StrInfo(
		std::string("Signal received: ") + GetSignalName(sig)
	);

	g_sigVal = sig;
}


namespace End2End
{

template<typename _FuncType>
inline void RunUntilSignal(
	_FuncType&& func
)
{
	// We will handle the signal ourselves
	std::signal(SIGINT, SignalHandler);
	std::signal(SIGTERM, SignalHandler);

	while (g_sigVal == 0)
	{
		func();
	}

	// Restore the default signal handler
	std::signal(SIGINT, SIG_DFL);
	std::signal(SIGTERM, SIG_DFL);
}

} // namespace End2End
