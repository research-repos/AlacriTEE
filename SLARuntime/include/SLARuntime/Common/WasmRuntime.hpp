// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstddef>

#include <memory>

#include <WasmRuntime/Internal/make_unique.hpp>
#include <WasmRuntime/MainRunner.hpp>
#include <WasmRuntime/SharedWasmRuntime.hpp>
#include <WasmRuntime/SystemIO.hpp>
#include <WasmRuntime/WasmRuntimeStaticHeap.hpp>

#include <SimpleObjects/SimpleObjects.hpp>
#include <SimpleJson/SimpleJson.hpp>

#include "WasmCounter.hpp"
#include "Logging.hpp"


namespace SLARuntime
{
namespace Common
{


class WasmRuntime
{
public: // static members:

	static const std::string& sk_globalCounterName()
	{
		static const std::string sk_globalCounterName = "enclave_wasm_counter";
		return sk_globalCounterName;
	}

public:

	WasmRuntime(
		std::unique_ptr<WasmRuntime::SystemIO> sysIO,
		size_t heapSize,
		uint32_t modStackSize,
		uint32_t modHeapSize,
		uint32_t execStackSize
	) :
		m_logger(Common::LoggerFactory::GetLogger("WasmRuntime")),
		m_wrt(
			WasmRuntime::WasmRuntimeStaticHeap::MakeUnique(
				std::move(sysIO),
				heapSize
			)
		),
		m_modStackSize(modStackSize),
		m_modHeapSize(modHeapSize),
		m_execStackSize(execStackSize),

		m_mod(nullptr)
	{}

	virtual ~WasmRuntime()
	{}

	void LoadPlainModule(const std::vector<uint8_t>& bytecode)
	{
		m_logger.Debug("Instrumenting wasm...");
		std::vector<uint8_t> instrumentedWasm =
			SLARuntime::Common::WasmCounter::InstrumentWasm(bytecode);
		m_logger.Debug("Instrumentation done.");

		LoadInstModule(instrumentedWasm);
	}

	void LoadInstModule(const std::vector<uint8_t>& bytecode)
	{
		m_mod = m_wrt.LoadModule(bytecode);
	}

	void RunModule(
		const std::vector<uint8_t>& eventId,
		const std::vector<uint8_t>& msgContent,
		uint64_t threshold
	)
	{

		auto modInst = m_mod.Instantiate(m_modStackSize, m_modHeapSize);
		auto execEnv = modInst.CreateExecEnv(m_execStackSize);

		std::unique_ptr<WasmRuntime::ExecEnvUserData> execEnvUserData =
			WasmRuntime::Internal::make_unique<WasmRuntime::ExecEnvUserData>();
		execEnvUserData->SetEventId(eventId);
		execEnvUserData->SetEventData(msgContent);

		execEnv->SetUserData(std::move(execEnvUserData));


		using MainRetType = std::tuple<int32_t>;

		const auto& execEnvRef = *(execEnv.get());
		execEnv->GetUserData().StartStopwatch(execEnvRef);
		auto mainRetVals = execEnv->ExecFunc<MainRetType>(
			"enclave_wasm_injected_main",
			static_cast<uint32_t>(execEnv->GetUserData().GetEventId().size()),
			static_cast<uint32_t>(execEnv->GetUserData().GetEventData().size()),
			static_cast<uint64_t>(threshold)
		);
		execEnv->GetUserData().StopStopwatch(execEnvRef);

		// Collecting data for SLA report
		uint64_t counter = modInst->GetGlobal<uint64_t>(sk_globalCounterName());

		int32_t retCode = std::get<0>(mainRetVals);

		uint64_t startTime = execEnv->GetUserData().GetStopwatchStartTime();
		uint64_t endTime = execEnv->GetUserData().GetStopwatchEndTime();
		uint64_t deltaTime = endTime - startTime;

		// Construct SLA report
		SimpleObjects::Dict slaReport;
		slaReport[SimpleObjects::String("counter")] = SimpleObjects::UInt64(counter);
		slaReport[SimpleObjects::String("retCode")] = SimpleObjects::Int32(retCode);
		slaReport[SimpleObjects::String("startTime")] = SimpleObjects::UInt64(startTime);
		slaReport[SimpleObjects::String("endTime")] = SimpleObjects::UInt64(endTime);
		slaReport[SimpleObjects::String("deltaTime")] = SimpleObjects::UInt64(deltaTime);

		// Print SLA report
		std::string slaReportStr = SimpleJson::DumpStr(slaReport);
		m_logger.Info("SLA report: " + slaReportStr);
	}


private:
	Common::Logger m_logger;
	WasmRuntime::SharedWasmRuntime m_wrt;
	uint32_t m_modStackSize;
	uint32_t m_modHeapSize;
	uint32_t m_execStackSize;

	WasmRuntime::SharedWasmModule m_mod;
}; // class WasmRuntime


} // namespace Common
} // namespace SLARuntime

