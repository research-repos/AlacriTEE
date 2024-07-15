// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstdint>

#include <limits>
#include <vector>

#include "Exception.hpp"
#include "WasmExecEnv.hpp"


namespace WasmRuntime
{


class ExecEnvUserData
{
public: // static members:

	using UserDataDeleterType = void(*)(void*);

public:

	ExecEnvUserData() :
		m_startTime(0),
		m_endTime(0),
		m_iCount(0),
		m_hasCountExceed(false),
		m_eventId(),
		m_eventData()
	{}

	ExecEnvUserData(const ExecEnvUserData&) = delete;

	ExecEnvUserData(ExecEnvUserData&& other) :
		m_startTime(other.m_startTime),
		m_endTime(other.m_endTime),
		m_iCount(other.m_iCount),
		m_hasCountExceed(other.m_hasCountExceed),
		m_eventId(std::move(other.m_eventId)),
		m_eventData(std::move(other.m_eventData))
	{}

	virtual ~ExecEnvUserData() {}

	ExecEnvUserData& operator=(const ExecEnvUserData&) = delete;

	ExecEnvUserData& operator=(ExecEnvUserData&& other)
	{
		if (this != &other)
		{
			// basic data
			m_startTime = other.m_startTime;
			m_endTime = other.m_endTime;
			m_iCount = other.m_iCount;
			m_hasCountExceed = other.m_hasCountExceed;
			m_eventId = std::move(other.m_eventId);
			m_eventData = std::move(other.m_eventData);

			// basic data - clear the other object
			other.m_startTime = 0;
			other.m_endTime = 0;
			other.m_iCount = 0;
			other.m_hasCountExceed = false;
		}
		return *this;
	}

	void StartStopwatch(const WasmExecEnv& execEnv)
	{
		if (m_startTime != 0 || m_endTime != 0)
		{
			throw Exception("Stopwatch already started");
		}

		execEnv.NativePrintCStr("Starting stopwatch...");

		m_startTime = execEnv.GetSystemIO().GetTimestampUs();
	}

	void StopStopwatch(const WasmExecEnv& execEnv)
	{
		if (m_startTime == 0 || m_endTime != 0)
		{
			throw Exception("Stopwatch not started or already stopped");
		}

		m_endTime = execEnv.GetSystemIO().GetTimestampUs();

		std::string msg =
			"Stopwatch stopped. "
			"(Started @ " + std::to_string(m_startTime) + " us,"
			" ended @ " + std::to_string(m_endTime) + " us)";
		execEnv.NativePrintStr(msg);
	}

	void ResetStopwatch()
	{
		m_startTime = 0;
		m_endTime = 0;
	}

	uint64_t GetStopwatchStartTime() const { return m_startTime; }
	uint64_t GetStopwatchEndTime() const { return m_endTime; }

	void SetICount(uint64_t iCount) { m_iCount = iCount; }
	uint64_t GetICount() const { return m_iCount; }

	void SetHasCountExceed(bool hasCountExceed) { m_hasCountExceed = hasCountExceed; }
	bool GetHasCountExceed() const { return m_hasCountExceed; }

	void SetEventId(const std::vector<uint8_t>& eventId)
	{
		if (eventId.size() > std::numeric_limits<uint32_t>::max())
		{
			throw Exception("The given event ID is larger than what WASM32 can handle");
		}
		m_eventId = eventId;
	}
	const std::vector<uint8_t>& GetEventId() const { return m_eventId; }

	void SetEventData(const std::vector<uint8_t>& eventData)
	{
		if (eventData.size() > std::numeric_limits<uint32_t>::max())
		{
			throw Exception("The given event data is larger than what WASM32 can handle");
		}
		m_eventData = eventData;
	}
	const std::vector<uint8_t>& GetEventData() const { return m_eventData; }

private:

	uint64_t m_startTime;
	uint64_t m_endTime;

	uint64_t m_iCount;
	bool m_hasCountExceed;

	std::vector<uint8_t> m_eventId;
	std::vector<uint8_t> m_eventData;

}; // class ExecEnvUserData


} // namespace WasmRuntime

