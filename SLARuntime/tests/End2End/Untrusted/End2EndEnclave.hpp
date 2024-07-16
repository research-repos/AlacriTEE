// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <string>
#include <vector>

#include <DecentEnclave/Common/Sgx/Exceptions.hpp>
#include <DecentEnclave/Untrusted/Sgx/DecentSgxEnclave.hpp>

#include <SimpleSysIO/SysCall/Files.hpp>

extern "C" sgx_status_t ecall_end2end_init(
	sgx_enclave_id_t eid,
	sgx_status_t*    retval,
	uint64_t         in_chain_id,
	const uint8_t*   in_sla_addr,
	size_t           in_sla_addr_size
);

extern "C" sgx_status_t ecall_end2end_load_wasm(
	sgx_enclave_id_t eid,
	sgx_status_t*    retval,
	const uint8_t*   in_wasm,
	size_t           in_wasm_size
);

extern "C" sgx_status_t ecall_end2end_run_func(
	sgx_enclave_id_t eid,
	sgx_status_t*    retval,
	const uint8_t*   in_event_id,
	size_t           in_event_id_size,
	const uint8_t*   in_msg,
	size_t           in_msg_size
);


namespace End2End
{


class End2EndEnclave :
	public DecentEnclave::Untrusted::Sgx::DecentSgxEnclave
{
public: // static members:

	using Base = DecentEnclave::Untrusted::Sgx::DecentSgxEnclave;


public:
	End2EndEnclave(
		uint64_t chainId,
		const std::vector<uint8_t>& slaMgrAddr,
		const std::vector<uint8_t>& authList,
		const std::string& enclaveImgPath = DECENT_ENCLAVE_PLATFORM_SGX_IMAGE,
		const std::string& launchTokenPath = DECENT_ENCLAVE_PLATFORM_SGX_TOKEN
	) :
		Base(authList, enclaveImgPath, launchTokenPath)
	{
		DECENTENCLAVE_SGX_ECALL_CHECK_ERROR_E_R(
			ecall_end2end_init,
			m_encId,
			chainId,
			slaMgrAddr.data(),
			slaMgrAddr.size()
		);
	}

	virtual ~End2EndEnclave() = default;

	void LoadWasm(const std::vector<uint8_t>& wasmCode)
	{
		DECENTENCLAVE_SGX_ECALL_CHECK_ERROR_E_R(
			ecall_end2end_load_wasm,
			m_encId,
			wasmCode.data(),
			wasmCode.size()
		);
	}

	void LoadWasm(const std::string& wasmPath)
	{
		std::vector<uint8_t> wasmCode =
			SimpleSysIO::SysCall::RBinaryFile::Open(wasmPath)->
				ReadBytes<std::vector<uint8_t> >();
		LoadWasm(wasmCode);
	}

	void RunFunc(
		const std::vector<uint8_t>& eventId,
		const std::vector<uint8_t>& msg
	)
	{
		DECENTENCLAVE_SGX_ECALL_CHECK_ERROR_E_R(
			ecall_end2end_run_func,
			m_encId,
			eventId.data(),
			eventId.size(),
			msg.data(),
			msg.size()
		);
	}

}; // class End2EndEnclave


} // namespace End2End

