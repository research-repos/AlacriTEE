// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <memory>

#include <DecentEnclave/Common/Logging.hpp>

#include <EclipseMonitor/Eth/DataTypes.hpp>

#include <mbedTLScpp/DefaultRbg.hpp>
#include <mbedTLScpp/Gcm.hpp>
#include <mbedTLScpp/SecretVector.hpp>

#include <SimpleObjects/Internal/make_unique.hpp>
#include <SimpleObjects/SimpleObjects.hpp>

#include <SimpleRlp/SimpleRlp.hpp>


namespace SLARuntime
{
namespace Common
{

class SLARuntime;

class SLAContract
{
public: // static members:

	using LoggerFactory = typename DecentEnclave::Common::LoggerFactory;
	using LoggerType = typename LoggerFactory::LoggerType;

	using EthPublicKeyType = mbedTLScpp::EcPublicKey<mbedTLScpp::EcType::SECP256K1>;

	template<typename _ContractAddr, typename _EthPublicKey, typename _SharedKey>
	static std::unique_ptr<SLAContract> MakeUnique(
		uint64_t contractId,
		_ContractAddr&& contractAddr,
		_EthPublicKey&& cltDhKey,
		_SharedKey&& sharedRootKey
	)
	{
		return SimpleObjects::Internal::make_unique<SLAContract>(
			contractId,
			std::forward<_ContractAddr>(contractAddr),
			std::forward<_EthPublicKey>(cltDhKey),
			std::forward<_SharedKey>(sharedRootKey)
		);
	}

public:
	SLAContract(
		uint64_t contractId,
		EclipseMonitor::Eth::ContractAddr contractAddr,
		EthPublicKeyType cltDhKey,
		mbedTLScpp::SecretVector<uint8_t> sharedRootKey
	) :
		m_rand(),
		m_contractId(contractId),
		m_contractAddr(std::move(contractAddr)),
		m_cltDhKey(std::move(cltDhKey)),
		m_sharedRootKey(std::move(sharedRootKey)),
		m_logger(
			LoggerFactory::GetLogger(
				"SLARuntime::Common::SLAContract_ID_" +
				std::to_string(m_contractId)
			)
		)
	{}

	~SLAContract() = default;

	uint64_t GetContractId() const
	{
		return m_contractId;
	}

	const EclipseMonitor::Eth::ContractAddr& GetContractAddr() const
	{
		return m_contractAddr;
	}

	std::vector<uint8_t> EncryptData(const std::vector<uint8_t>& data) const
	{
		std::vector<uint8_t> iv;
		iv.resize(12);
		m_rand.Rand(iv.data(), iv.size());

		std::vector<uint8_t> empty;

		mbedTLScpp::Gcm<mbedTLScpp::CipherType::AES, 256> gcm(
			mbedTLScpp::CtnFullR(m_sharedRootKey)
		);

		std::vector<uint8_t> cipher;
		std::array<uint8_t, 16> tag;

		std::tie(cipher, tag) = gcm.Encrypt(
			mbedTLScpp::CtnFullR(data),
			mbedTLScpp::CtnFullR(iv),
			mbedTLScpp::CtnFullR(empty)
		);

		SimpleObjects::List package = {
			SimpleObjects::Bytes(iv),
			SimpleObjects::Bytes(tag.begin(), tag.end()),
			SimpleObjects::Bytes(cipher),
		};

		return SimpleRlp::WriteRlp(package);
	}

private:
	mutable mbedTLScpp::DefaultRbg m_rand;

	uint64_t m_contractId;
	EclipseMonitor::Eth::ContractAddr m_contractAddr;
	EthPublicKeyType m_cltDhKey;
	mbedTLScpp::SecretVector<uint8_t> m_sharedRootKey;

	LoggerType m_logger;
}; // class SLAContract


} // namespace Common
} // namespace SLARuntime

