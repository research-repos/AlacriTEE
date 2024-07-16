// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <memory>
#include <string>
#include <vector>

#include <DecentEnclave/Common/Platform/Print.hpp>
#include <DecentEnclave/Common/Sgx/MbedTlsInit.hpp>

#include <DecentEnclave/Untrusted/Config/AuthList.hpp>
#include <DecentEnclave/Untrusted/Config/EndpointsMgr.hpp>
#include <DecentEnclave/Untrusted/Hosting/BoostAsioService.hpp>
#include <DecentEnclave/Untrusted/Hosting/HeartbeatEmitterService.hpp>
#include <DecentEnclave/Untrusted/Hosting/LambdaFuncServer.hpp>

#include <SimpleConcurrency/Threading/ThreadPool.hpp>
#include <SimpleJson/SimpleJson.hpp>
#include <SimpleRlp/SimpleRlp.hpp>
#include <SimpleObjects/Internal/make_unique.hpp>
#include <SimpleObjects/Codec/Hex.hpp>
#include <SimpleObjects/SimpleObjects.hpp>
#include <SimpleSysIO/SysCall/Files.hpp>

#include "End2EndEnclave.hpp"
#include "RunUntilSignal.hpp"


using namespace DecentEnclave;
using namespace DecentEnclave::Common;
using namespace DecentEnclave::Untrusted;
using namespace SimpleConcurrency::Threading;
using namespace SimpleObjects;
using namespace SimpleSysIO::SysCall;


std::shared_ptr<ThreadPool> GetThreadPool()
{
	static  std::shared_ptr<ThreadPool> threadPool =
		std::make_shared<ThreadPool>(5);

	return threadPool;
}


int main(int argc, char* argv[])
{
	std::string configPath;
	if (argc == 1)
	{
		configPath = END2END_SRC_DIR "/components_config.json";
	}
	else if (argc == 2)
	{
		configPath = argv[1];
	}
	else
	{
		Common::Platform::Print::StrErr("Unexpected number of arguments.");
		Common::Platform::Print::StrErr(
			"Only the path to the components configuration file is needed."
		);
		return -1;
	}

	// Init MbedTLS
	Common::Sgx::MbedTlsInit::Init();

	// Thread pool
	std::shared_ptr<ThreadPool> threadPool = GetThreadPool();


	// Read in components config
	auto configJson = RBinaryFile::Open(configPath)->ReadBytes<std::string>();
	auto config = SimpleJson::LoadStr(configJson);
	std::vector<uint8_t> authListAdvRlp = Config::ConfigToAuthListAdvRlp(config);


	// Boost IO Service
	std::unique_ptr<Hosting::BoostAsioService> asioService =
		SimpleObjects::Internal::make_unique<Hosting::BoostAsioService>();


	// Endpoints Manager
	auto endpointMgr = Config::EndpointsMgr::GetInstancePtr(
		&config,
		asioService->GetIoService()
	);

	// SLA
	const auto& slaConfig = config.AsDict()[String("SLA")].AsDict();
	uint64_t chainId = slaConfig[String("ChainID")].AsCppUInt64();
	std::string slaMgrAddrHex = slaConfig[String("ManagerAddr")].AsString().c_str();
	std::vector<uint8_t> slaMgrAddr = Codec::Hex::Decode<std::vector<uint8_t> >(slaMgrAddrHex);

	// Enclave
	const auto& imgConfig = config.AsDict()[String("EnclaveImage")].AsDict();
	std::string imgPath = imgConfig[String("ImagePath")].AsString().c_str();
	std::string tokenPath = imgConfig[String("TokenPath")].AsString().c_str();
	std::shared_ptr<End2End::End2EndEnclave> enclave =
		std::make_shared<End2End::End2EndEnclave>(
			chainId,
			slaMgrAddr,
			authListAdvRlp,
			imgPath,
			tokenPath
		);

	// API call server
	Hosting::LambdaFuncServer lambdaFuncSvr(
		endpointMgr,
		threadPool
	);
	// Setup Lambda call handlers and start to run multi-threaded-ly
	//lambdaFuncSvr.AddFunction("DecentEthereum", enclave);

	// Start IO service
	threadPool->AddTask(std::move(asioService));


	// WASM module
	const auto& wasmConfig = config.AsDict()[String("WasmModule")].AsDict();
	std::string wasmPath = wasmConfig[String("ModulePath")].AsString().c_str();
	enclave->LoadWasm(wasmPath);

	std::vector<uint8_t> eventId = { 0x01, 0x02, 0x03, 0x04 };
	std::vector<uint8_t> msg = { 0x05, 0x06, 0x07, 0x08, 0x09 };
	enclave->RunFunc(eventId, msg);


	End2End::RunUntilSignal(
		[&]()
		{
			threadPool->Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	);


	threadPool->Terminate();


	return 0;
}
