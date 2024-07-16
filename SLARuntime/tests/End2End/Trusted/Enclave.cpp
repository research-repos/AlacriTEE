// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.


#include <cstdint>

#include <memory>
#include <vector>

#include <sgx_edger8r.h>

#include <DecentEnclave/Common/Platform/Print.hpp>
#include <DecentEnclave/Common/Sgx/MbedTlsInit.hpp>

#include <DecentEnclave/Trusted/AppCertRequester.hpp>
#include <DecentEnclave/Trusted/PlatformId.hpp>

#include <SLARuntime/Common/SLARuntime.hpp>
#include <SLARuntime/Common/WasmRuntime.hpp>

#include <EclipseMonitor/Eth/DataTypes.hpp>

#include "Certs.hpp"
#include "Keys.hpp"
#include "SystemIO.hpp"


namespace End2End
{


static std::shared_ptr<SLARuntime::Common::SLARuntime> gs_slaRt;


static SLARuntime::Common::WasmRuntime gs_rt(
	End2End::SystemIO::MakeUnique(),
	10 * 1024 * 1024, // 10MB - Total heap size
	 2 * 1024 * 1024, // 2MB - Module stack size
	 7 * 1024 * 1024, // 7MB - Module heap size
	 1 * 1024 * 1024  // 1MB - Execution stack size
);


void GlobalInitialization()
{
	using namespace DecentEnclave::Common;

	// Initialize mbedTLS
	Sgx::MbedTlsInit::Init();

	using namespace DecentEnclave::Trusted;
	// Register keys
	DecentKey_Secp256r1::Register();
	DecentKey_Secp256k1::Register();
	DecentKey_Secp256k1DH::Register();

	// Register certificates
	DecentCert_Secp256r1::Register();
	DecentCert_Secp256k1::Register();
	DecentCert_ServerSecp256k1::Register();
}


void PrintMyInfo()
{
	using namespace DecentEnclave::Common;
	using namespace DecentEnclave::Trusted;

	Platform::Print::StrInfo(
		"My platform ID is              : " + PlatformId::GetIdHex()
	);

	const auto& selfHash = DecentEnclave::Trusted::Sgx::EnclaveIdentity::GetSelfHashHex();
	Platform::Print::StrInfo(
		"My enclave hash is             : " + selfHash
	);

	std::string secp256r1KeyFp =
		DecentKey_Secp256r1::GetInstance().GetKeySha256Hex();
	std::string secp256k1KeyFp =
		DecentKey_Secp256k1::GetInstance().GetKeySha256Hex();
	std::string keyringHash = Keyring::GetInstance().GenHashHex();
	Platform::Print::StrInfo(
		"My key fingerprint (SECP256R1) : " + secp256r1KeyFp
	);
	Platform::Print::StrInfo(
		"My key fingerprint (SECP256K1) : " + secp256k1KeyFp
	);
	Platform::Print::StrInfo(
		"My keyring hash is             : " + keyringHash
	);
}


template<typename _AppCertStoreCertType>
inline void RequestAppCert(
	DecentEnclave::Trusted::AppCertRequester& appCertRequester
)
{
	std::string pem = appCertRequester.Request();
	auto cert = std::make_shared<mbedTLScpp::X509Cert>(
		mbedTLScpp::X509Cert::FromPEM(pem)
	);

	_AppCertStoreCertType::Update(cert);
}


template<typename _ServerCertStoreCertType>
inline void RequestServerCert(
	DecentEnclave::Trusted::AppCertRequester& appCertRequester
)
{
	std::string serverPem = appCertRequester.GetServerCert();
	auto serverCert = std::make_shared<mbedTLScpp::X509Cert>(
		mbedTLScpp::X509Cert::FromPEM(serverPem)
	);

	_ServerCertStoreCertType::Update(serverCert);
}


template<typename _AppCertStoreCertType>
inline void RequestAppCert(const std::string& keyName)
{
	DecentEnclave::Trusted::AppCertRequester appCertRequester(
		"DecentServer",
		keyName
	);

	RequestAppCert<_AppCertStoreCertType>(appCertRequester);
}


template<typename _ServerCertStoreCertType, typename _AppCertStoreCertType>
inline void RequestAppCertAndServerCert(const std::string& keyName)
{
	DecentEnclave::Trusted::AppCertRequester appCertRequester(
		"DecentServer",
		keyName
	);

	RequestAppCert<_AppCertStoreCertType>(appCertRequester);
	RequestServerCert<_ServerCertStoreCertType>(appCertRequester);
}


void Init(
	uint64_t chainId,
	const EclipseMonitor::Eth::ContractAddr& slaMgrAddr
)
{

	GlobalInitialization();
	PrintMyInfo();

	RequestAppCert<DecentCert_Secp256r1>("Secp256r1");
	RequestAppCertAndServerCert<
		DecentCert_ServerSecp256k1,
		DecentCert_Secp256k1
	>("Secp256k1");

	gs_slaRt = SLARuntime::Common::SLARuntime::MakeUnique(
		DecentKey_Secp256k1::GetKeySharedPtr(),
		DecentKey_Secp256k1DH::GetKeySharedPtr(),
		chainId,
		slaMgrAddr
	);
	gs_slaRt->RegisterProvider(
		/*rate=*/100,
		/*svrCertName=*/"ServerSecp256k1",
		/*appCertName=*/"Secp256k1"
	);

	SLARuntime::Common::SubscribeToSlaProposeEvent(gs_slaRt);
}


} // namespace End2End


extern "C" sgx_status_t ecall_end2end_init(
	uint64_t chain_id,
	const uint8_t* in_sla_addr,
	size_t in_sla_addr_size
)
{
	try
	{
		auto slaMgrAddr = EclipseMonitor::Eth::ContractAddr();
		if (in_sla_addr_size != slaMgrAddr.size())
		{
			throw std::invalid_argument("SLA manager address size mismatch.");
		}
		std::copy(in_sla_addr, in_sla_addr + in_sla_addr_size, slaMgrAddr.begin());

		End2End::Init(chain_id, slaMgrAddr);

		return SGX_SUCCESS;
	}
	catch(const std::exception& e)
	{
		using namespace DecentEnclave::Common;
		Platform::Print::StrErr(e.what());
		return SGX_ERROR_UNEXPECTED;
	}
}

extern "C" sgx_status_t ecall_end2end_load_wasm(
	const uint8_t* in_wasm,
	size_t in_wasm_size
)
{
	try
	{
		std::vector<uint8_t> wasm(in_wasm, in_wasm + in_wasm_size);

		// Load wasm module
		End2End::gs_rt.LoadPlainModule(wasm);

		return SGX_SUCCESS;
	}
	catch(const std::exception& e)
	{
		using namespace DecentEnclave::Common;
		Platform::Print::StrErr(e.what());
		return SGX_ERROR_UNEXPECTED;
	}
}

extern "C" sgx_status_t ecall_end2end_run_func(
	const uint8_t* in_event_id,
	size_t in_event_id_size,
	const uint8_t* in_msg,
	size_t in_msg_size
)
{
	try
	{
		std::vector<uint8_t> eventId(in_event_id, in_event_id + in_event_id_size);
		std::vector<uint8_t> msg(in_msg, in_msg + in_msg_size);

		uint64_t threshold = std::numeric_limits<uint64_t>::max();

		End2End::gs_rt.RunModule(eventId, msg, threshold);

		return SGX_SUCCESS;
	}
	catch(const std::exception& e)
	{
		using namespace DecentEnclave::Common;
		Platform::Print::StrErr(e.what());
		return SGX_ERROR_UNEXPECTED;
	}
}

