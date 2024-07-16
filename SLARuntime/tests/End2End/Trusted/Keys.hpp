// Copyright (c) 2022 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <DecentEnclave/Common/Keyring.hpp>
#include <DecentEnclave/Common/Platform/Print.hpp>
#include <DecentEnclave/Common/Sgx/Crypto.hpp>
#include <DecentEnclave/Common/Sgx/Exceptions.hpp>
#include <DecentEnclave/Trusted/Sgx/Random.hpp>

#include <EclipseMonitor/Eth/Transaction/Ecdsa.hpp>

#include <mbedTLScpp/EcKey.hpp>
#include <mbedTLScpp/DefaultRbg.hpp>

#include <sgx_tcrypto.h>


namespace End2End
{


DECENTENCLAVE_KEYRING_KEY(
	Secp256r1,
	mbedTLScpp::EcKeyPair<mbedTLScpp::EcType::SECP256R1>,
	mbedTLScpp::EcPublicKeyBase<>
)
{
	using namespace mbedTLScpp;

	sgx_ecc_state_handle_t eccHlr;
	auto sgxRet = sgx_ecc256_open_context(&eccHlr);
	DECENTENCLAVE_CHECK_SGX_RUNTIME_ERROR(
		sgxRet,
		sgx_ecc256_open_context
	);

	sgx_ec256_private_t priv;
	sgx_ec256_public_t pub;
	sgxRet = sgx_ecc256_create_key_pair(&priv, &pub, eccHlr);
	sgx_ecc256_close_context(eccHlr);
	DECENTENCLAVE_CHECK_SGX_RUNTIME_ERROR(
		sgxRet,
		sgx_ecc256_create_key_pair
	);

	mbedTLScpp::EcKeyPair<mbedTLScpp::EcType::SECP256R1> keyPair(
		mbedTLScpp::EcType::SECP256R1
	);

	DecentEnclave::Common::Sgx::ExportEcKey(keyPair, pub);
	DecentEnclave::Common::Sgx::ExportEcKey(keyPair, priv);

	return keyPair;
}


DECENTENCLAVE_KEYRING_KEY(
	Secp256k1,
	mbedTLScpp::EcKeyPair<mbedTLScpp::EcType::SECP256K1>,
	mbedTLScpp::EcPublicKeyBase<>
)
{
	using namespace mbedTLScpp;

	DecentEnclave::Trusted::Sgx::RandGenerator rand;

#ifdef TESTS_END2END_TRUE_RANDOM_ETH_KEY
	// This is the code for generating a truly random key pair
	auto key = EcKeyPair<EcType::SECP256K1>::Generate(rand);

	// But in this experiment, we want to use a fixed key pair for testing
	// // so first we print out the private key
	// std::string privInt = key.BorrowSecretNum().Dec();
	// DecentEnclave::Common::Platform::Print::Str("Private key: " + privInt + "\n");

#else // TESTS_END2END_TRUE_RANDOM_ETH_KEY
	// next, we reuse the private key that have been printed out
	auto key = EcKeyPair<EcType::SECP256K1>::FromSecretNum(
		mbedTLScpp::BigNum("104161313841293763324411098699342690646687452222432671649474620618409153180280"),
		rand
	);
	// And we print out a warning message to remind the user that we are using
	// a testing key pair
	DecentEnclave::Common::Platform::Print::StrDebug("WARNING: Using a fixed key pair for testing");
#endif // TESTS_END2END_TRUE_RANDOM_ETH_KEY

	auto addr = EclipseMonitor::Eth::Transaction::AddressFromPublicKey(key);
	DecentEnclave::Common::Platform::Print::StrInfo("ETH address: " + addr.ToString());

	return key;
}


DECENTENCLAVE_KEYRING_KEY(
	Secp256k1DH,
	mbedTLScpp::EcKeyPair<mbedTLScpp::EcType::SECP256K1>,
	mbedTLScpp::EcPublicKeyBase<>
)
{
	using namespace mbedTLScpp;

	DecentEnclave::Trusted::Sgx::RandGenerator rand;

	auto key = EcKeyPair<EcType::SECP256K1>::Generate(rand);

	return key;
}


} // namespace End2End
