// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <cstdint>

#include <atomic>
#include <memory>
#include <string>

#include <AdvancedRlp/AdvancedRlp.hpp>

#include <DecentEnclave/Common/CertStore.hpp>
#include <DecentEnclave/Common/DecentTlsConfig.hpp>
#include <DecentEnclave/Common/Logging.hpp>
#include <DecentEnclave/Common/TlsSocket.hpp>
#include <DecentEnclave/Trusted/DecentLambdaClt.hpp>
#include <DecentEnclave/Trusted/HeartbeatRecvMgr.hpp>
#include <DecentEnclave/Trusted/PlatformId.hpp>

#include <EclipseMonitor/Eth/AbiParser.hpp>
#include <EclipseMonitor/Eth/AbiWriter.hpp>
#include <EclipseMonitor/Eth/DataTypes.hpp>
#include <EclipseMonitor/Eth/Keccak256.hpp>
#include <EclipseMonitor/Eth/Transaction/ContractFunction.hpp>
#include <EclipseMonitor/Eth/Transaction/Ecdsa.hpp>

#include <mbedTLScpp/DefaultRbg.hpp>
#include <mbedTLScpp/EcKey.hpp>

#include <SimpleObjects/SimpleObjects.hpp>
#include <SimpleRlp/SimpleRlp.hpp>

#include "SLAContract.hpp"


namespace SLARuntime
{
namespace Common
{


class SLARuntime
{
public: // static members:

	using LoggerFactory = typename DecentEnclave::Common::LoggerFactory;
	using LoggerType = typename LoggerFactory::LoggerType;

	using EthKeyPairType = mbedTLScpp::EcKeyPair<mbedTLScpp::EcType::SECP256K1>;
	using EthPublicKeyType = mbedTLScpp::EcPublicKey<mbedTLScpp::EcType::SECP256K1>;

	using FuncAbiRegister =
		EclipseMonitor::Eth::AbiWriterStaticTuple<
			// param 1 - uint64 rate
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Integer,
				EclipseMonitor::Eth::AbiUInt64
			>,
			// param 2 - bytes32 DH key X
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Bytes,
				EclipseMonitor::Eth::AbiSize<32>
			>,
			// param 3 - bytes32 DH key Y
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Bytes,
				EclipseMonitor::Eth::AbiSize<32>
			>,
			// param 4 - bytes memory svrCertDer
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Bytes,
				std::true_type
			>,
			// param 5 - bytes memory appCertDer
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Bytes,
				std::true_type
			>
		>;

	using FuncAbiAccept =
		EclipseMonitor::Eth::AbiWriterStaticTuple<
			// param 1 - uint64 contractId
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Integer,
				EclipseMonitor::Eth::AbiUInt64
			>,
			// param 2 - bytes memory providerMessage
			EclipseMonitor::Eth::AbiWriter<
				SimpleObjects::ObjCategory::Bytes,
				std::true_type
			>
		>;

	static std::unique_ptr<SLARuntime> MakeUnique(
		std::shared_ptr<EthKeyPairType> ethKey,
		std::shared_ptr<EthKeyPairType> dhKey,
		uint64_t chainId,
		const EclipseMonitor::Eth::ContractAddr& slaMgrAddr
	)
	{
		return SimpleObjects::Internal::make_unique<SLARuntime>(
			std::move(ethKey),
			std::move(dhKey),
			chainId,
			slaMgrAddr
		);
	}

public:

	SLARuntime(
		std::shared_ptr<EthKeyPairType> ethKey,
		std::shared_ptr<EthKeyPairType> dhKey,
		uint64_t chainId,
		const EclipseMonitor::Eth::ContractAddr& slaMgrAddr
	) :
		m_logger(LoggerFactory::GetLogger("SLARuntime::Common::SLARuntime")),
		m_rand(),
		m_ethKey(std::move(ethKey)),
		m_dhKey(std::move(dhKey)),
		m_hostAddr("127.0.0.1"),
		m_hostPort(5000),
		m_chainId(chainId),
		m_nonce(0),
		m_funcReg(slaMgrAddr, "registerProvider"),
		m_funcAccept(slaMgrAddr, "acceptProposal")
	{}

	~SLARuntime() = default;

	void FinishAndSendTransaction(
		EclipseMonitor::Eth::Transaction::DynFee& txn
	)
	{
		// m_logger.Info(
		// 	"Transaction Data: " +
		// 	SimpleObjects::Codec::Hex::Encode<std::string>(txn.get_Data())
		// );

		txn.SetChainID(m_chainId);
		txn.SetNonce(m_nonce++);
		txn.SetMaxPriorFeePerGas(300000000);
		txn.SetMaxFeePerGas(300000000);

		EclipseMonitor::Eth::Transaction::SignTransaction(txn, *m_ethKey);

		auto rlp = txn.RlpSerializeSigned();

		m_logger.Info("Sending transaction...");

		EthSendRawTransaction(rlp);
	}

	std::vector<uint8_t> BuildConnectMsg() const
	{
		SimpleObjects::List list = {
			SimpleObjects::Bytes(m_hostAddr.begin(), m_hostAddr.end()),
			SimpleObjects::Bytes({
				static_cast<uint8_t>(m_hostPort >> 8),
				static_cast<uint8_t>(m_hostPort & 0xFF)
			}),
		};

		return SimpleRlp::WriteRlp(list);
	}

	void RegisterProvider(
		uint64_t rate,
		const std::string& svrCertName,
		const std::string& appCertName
	)
	{
		auto rateObj = SimpleObjects::UInt64(rate);

		auto dhXVec = m_dhKey->BorrowPubPointX().Bytes</*_LitEndian=*/false>();
		auto dhYVec = m_dhKey->BorrowPubPointY().Bytes</*_LitEndian=*/false>();
		SimpleObjects::Bytes dhX;
		dhX.reserve(32);
		std::fill_n(std::back_inserter(dhX), 32 - dhXVec.size(), 0);
		std::copy(dhXVec.begin(), dhXVec.end(), std::back_inserter(dhX));
		SimpleObjects::Bytes dhY;
		dhY.reserve(32);
		std::fill_n(std::back_inserter(dhY), 32 - dhYVec.size(), 0);
		std::copy(dhYVec.begin(), dhYVec.end(), std::back_inserter(dhY));

		const auto& certStore = DecentEnclave::Common::CertStore::GetInstance();
		auto svrCert = certStore[svrCertName].GetCertBase();
		auto appCert = certStore[appCertName].GetCertBase();
		auto svrCertDer = SimpleObjects::Bytes(svrCert->GetDer());
		auto appCertDer = SimpleObjects::Bytes(appCert->GetDer());

		// auto tn = FuncAbiRegister().GetTypeName();
		// m_logger.Info("RegisterProvider - Abi type name: " + tn);

		auto txn = m_funcReg.CallByTxn(
			rateObj,
			dhX,
			dhY,
			svrCertDer,
			appCertDer
		);
		// based on our test, the gas cost is around 1247007
		txn.SetGasLimit(5000000);

		m_logger.Info("Generated transaction to register provider");
		// auto svrCertDerHex = SimpleObjects::Codec::Hex::Encode<std::string>(svrCertDer);
		// auto appCertDerHex = SimpleObjects::Codec::Hex::Encode<std::string>(appCertDer);
		// m_logger.Debug("Server certificate DER:\n" + svrCertDerHex);
		// m_logger.Debug("App certificate DER:\n" + appCertDerHex);

		FinishAndSendTransaction(txn);
	}

	const EclipseMonitor::Eth::ContractAddr& GetSlaManagerAddr() const
	{
		return m_funcReg.GetContractAddr();
	}

	void AcceptSLAProposal(
		std::unique_ptr<SLAContract> contract
	)
	{
		static constexpr uint64_t sk_1ether = 1ULL * 1000000000ULL * 1000000000ULL;

		auto contractIdObj = SimpleObjects::UInt64(contract->GetContractId());
		auto providerMsg = SimpleObjects::Bytes(
			contract->EncryptData(BuildConnectMsg())
		);

		auto txn = m_funcAccept.CallByTxn(
			contractIdObj,
			providerMsg
		);
		// based on our test, the gas cost is around ...
		txn.SetGasLimit(5000000);
		// deposit 1 ether
		txn.SetAmount(sk_1ether);

		m_logger.Info("Generated transaction to accept proposal");

		FinishAndSendTransaction(txn);
	}

	void ProcessSLAProposal(
		std::unique_ptr<SLAContract> contract
	)
	{
		// for now, we just accept the proposal
		AcceptSLAProposal(std::move(contract));
	}

	void OnProposeEvent(const SimpleObjects::BytesBaseObj& logData)
	{
		// m_logger.Debug(
		// 	"Received log data: " +
		// 	SimpleObjects::Codec::Hex::Encode<std::string>(logData)
		// );

		auto abiBegin = logData.begin();

		std::vector<uint8_t> clientAddrBytes;
		std::tie(clientAddrBytes, abiBegin) =
			_Byte32Parser().ToPrimitive(abiBegin, logData.end(), logData.begin());
		std::vector<uint8_t> hardwareIDBytes;
		std::tie(hardwareIDBytes, abiBegin) =
			_Byte32Parser().ToPrimitive(abiBegin, logData.end(), logData.begin());

		m_logger.Debug(
			"SLA proposal for Hardware ID: " +
			SimpleObjects::Codec::Hex::Encode<std::string>(hardwareIDBytes)
		);
		if (hardwareIDBytes != DecentEnclave::Trusted::PlatformId::GetId())
		{
			m_logger.Debug("Received a SLA proposal event for a different platform");
			return;
		}
		m_logger.Info("Received SLA proposal event");

		uint64_t contractID;
		std::tie(contractID, abiBegin) =
			_UIntParser().ToPrimitive(abiBegin, logData.end(), logData.begin());
		std::vector<uint8_t> clientKeyXBytes;
		std::tie(clientKeyXBytes, abiBegin) =
			_Byte32Parser().ToPrimitive(abiBegin, logData.end(), logData.begin());
		std::vector<uint8_t> clientKeyYBytes;
		std::tie(clientKeyYBytes, abiBegin) =
			_Byte32Parser().ToPrimitive(abiBegin, logData.end(), logData.begin());

		auto contractAddr = EclipseMonitor::Eth::ContractAddr();
		std::copy(
			clientAddrBytes.begin() + 12,
			clientAddrBytes.end(),
			contractAddr.begin()
		);

		m_logger.Debug(
			"Contract address: " +
			SimpleObjects::Codec::Hex::Encode<std::string>(contractAddr)
		);
		m_logger.Debug("Contract ID: " + std::to_string(contractID));
		m_logger.Debug(
			std::string("Client public key:") +
			"\nX:" + SimpleObjects::Codec::Hex::Encode<std::string>(clientKeyXBytes) +
			"\nY:" + SimpleObjects::Codec::Hex::Encode<std::string>(clientKeyYBytes)
		);

		auto pubX = mbedTLScpp::BigNum(
			mbedTLScpp::CtnFullR(clientKeyXBytes),
			/*isPositive=*/true,
			/*isLittleEndian=*/false
		);
		auto pubY = mbedTLScpp::BigNum(
			mbedTLScpp::CtnFullR(clientKeyYBytes),
			/*isPositive=*/true,
			/*isLittleEndian=*/false
		);

		auto peerDhKey = EthPublicKeyType::FromPublicNum(pubX, pubY);
		auto sharedKey = m_dhKey->DeriveSharedKeyInBigNum(peerDhKey, m_rand).
			SecretBytes</*_LitEndian=*/false>();
		m_logger.Debug(
			"Shared root key: " +
			SimpleObjects::Codec::Hex::Encode<std::string>(sharedKey)
		);
		ProcessSLAProposal(
			SLAContract::MakeUnique(
				contractID,
				contractAddr,
				std::move(peerDhKey),
				std::move(sharedKey)
			)
		);
	}

private:

	using _Byte32Parser =
		EclipseMonitor::Eth::AbiParser<
			SimpleObjects::ObjCategory::Bytes,
			EclipseMonitor::Eth::AbiSize<32>
		>;
	using _UIntParser =
		EclipseMonitor::Eth::AbiParser<
			SimpleObjects::ObjCategory::Integer,
			EclipseMonitor::Eth::AbiUInt64
		>;

	static DecentEnclave::Common::DetMsg BuildSendRawTransactionMsg(
		const std::vector<uint8_t>& txn
	)
	{
		DecentEnclave::Common::DetMsg msg;

		msg.get_MsgId().get_MsgType() = SimpleObjects::String("Transaction.SendRaw");
		msg.get_MsgContent() = SimpleObjects::Bytes(txn);

		return msg;
	}

	static void EthSendRawTransaction(const std::vector<uint8_t>& txn)
	{
		auto msg = BuildSendRawTransactionMsg(txn);
		DecentEnclave::Trusted::MakeLambdaCall(
			"DecentEthereum",
			DecentEnclave::Common::DecentTlsConfig::MakeTlsConfig(
				false,
				"Secp256r1",
				"Secp256r1"
			),
			msg // lvalue reference needed
		);
	}

	LoggerType m_logger;

	mbedTLScpp::DefaultRbg m_rand;
	std::shared_ptr<EthKeyPairType> m_ethKey;
	std::shared_ptr<EthKeyPairType> m_dhKey;

	std::string m_hostAddr;
	uint16_t m_hostPort;

	uint64_t m_chainId;
	std::atomic_uint64_t m_nonce;

	EclipseMonitor::Eth::Transaction::ContractFuncStaticDef<
		FuncAbiRegister
	> m_funcReg;
	EclipseMonitor::Eth::Transaction::ContractFuncStaticDef<
		FuncAbiAccept
	> m_funcAccept;

}; // class SLARuntime


inline DecentEnclave::Common::DetMsg BuildSubscribeMsg(
	const EclipseMonitor::Eth::ContractAddr& publisherAddr,
	const SimpleObjects::Bytes& eventTopic
)
{
	static const SimpleObjects::String sk_labelContract("contract");
	static const SimpleObjects::String sk_labelTopics("topics");

	SimpleObjects::Dict msgContent;
	msgContent[sk_labelContract] = SimpleObjects::Bytes(
		publisherAddr.begin(),
		publisherAddr.end()
	);
	msgContent[sk_labelTopics] = SimpleObjects::List({
		eventTopic,
	});

	DecentEnclave::Common::DetMsg msg;
	//msg.get_Version() = 1;
	msg.get_MsgId().get_MsgType() = SimpleObjects::String("Receipt.Subscribe");
	msg.get_MsgContent() = SimpleObjects::Bytes(
		AdvancedRlp::GenericWriter::Write(msgContent)
	);

	return msg;
}


inline DecentEnclave::Common::DetMsg BuildSubMsgSlaProposeEvent(
	const EclipseMonitor::Eth::ContractAddr& publisherAddr
)
{
	static const auto sk_signTopic = EclipseMonitor::Eth::Keccak256(
		std::string("SlaProposal(address,bytes32,uint256,bytes32,bytes32)")
	);
	static const SimpleObjects::Bytes  sk_signTopicBytes(
		std::vector<uint8_t>(sk_signTopic.begin(), sk_signTopic.end())
	);

	return BuildSubscribeMsg(publisherAddr, sk_signTopicBytes);
}


template<typename _NotifyFunc>
inline
typename DecentEnclave::Trusted::HeartbeatRecvMgr::RecvFunc
BuildFuncNotifyOnEventLog(_NotifyFunc func)
{
	auto retFunc = [func](std::vector<uint8_t> heartbeatMsg)
	{
		static const SimpleObjects::String sk_labelReceipts("Receipts");

		auto msg = AdvancedRlp::Parse(heartbeatMsg);

		const auto& msgDict = msg.AsDict();
		const auto& recQueue = msgDict[sk_labelReceipts].AsList();

		for (const auto& receipt: recQueue)
		{
			const auto& logFields = receipt.AsList();
			const auto& logData = logFields[2].AsBytes();
			func(logData);
		}
	};
	return retFunc;
}


inline void SubscribeToSlaProposeEvent(std::shared_ptr<SLARuntime> slaRt)
{
	auto subMsg = BuildSubMsgSlaProposeEvent(slaRt->GetSlaManagerAddr());

	std::shared_ptr<DecentEnclave::Common::TlsSocket> pubsubTlsSocket =
		DecentEnclave::Trusted::MakeLambdaCall(
			"DecentEthereum",
			DecentEnclave::Common::DecentTlsConfig::MakeTlsConfig(
				false,
				"Secp256r1",
				"Secp256r1"
			),
			subMsg // lvalue reference needed
		);

	auto pubsubHbConstraint =
		std::make_shared<DecentEnclave::Trusted::HeartbeatTimeConstraint<uint64_t> >(
			1000
		);

	DecentEnclave::Trusted::HeartbeatRecvMgr::GetInstance().AddRecv(
		pubsubHbConstraint,
		pubsubTlsSocket,
		BuildFuncNotifyOnEventLog(
			[slaRt](const SimpleObjects::BytesBaseObj& logData)
			{
				slaRt->OnProposeEvent(logData);
			}
		),
		true
	);
}


} // namespace Common
} // namespace SLARuntime

