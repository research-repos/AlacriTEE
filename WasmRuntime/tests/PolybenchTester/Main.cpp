#include <cstdio>

#include <chrono>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>

#include <sgx_urts.h>
#include <sgx_edger8r.h>

#include "EnclaveMain.hpp"

extern "C" {

void ocall_print(const char *str)
{
	printf("%s", str);
}

extern "C" uint64_t ocall_decent_untrusted_timestamp_us()
{
	auto now = std::chrono::system_clock::now();
	auto nowUs = std::chrono::duration_cast<std::chrono::microseconds>(
		now.time_since_epoch()
	);
	return static_cast<uint64_t>(nowUs.count());
}

extern sgx_status_t ecall_enclave_wasm_main(
	sgx_enclave_id_t eid,
	const uint8_t *wasm_file, size_t wasm_file_size,
	const uint8_t *wasm_nopt_file, size_t wasm_nopt_file_size
);

} // extern "C"

static std::vector<uint8_t> ReadFile2Buffer(const std::string& filename)
{
	FILE *file;
	size_t file_size, read_size;

	if ((file = fopen(filename.c_str(), "rb")) == nullptr)
	{
		throw std::runtime_error(
			"Read file to buffer failed: open file " + filename + " failed");
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	std::vector<uint8_t> buffer(file_size);

	read_size = fread(buffer.data(), 1, file_size, file);
	fclose(file);

	if (read_size < file_size)
	{
		throw std::runtime_error(
			"Read file " + filename + " to buffer failed: read file content failed");
	}

	return buffer;
}

static void WriteBuffer2File(
	const std::string& filename, const std::vector<uint8_t>& buffer)
{
	FILE *file;
	size_t writeSize = 0;

	if ((file = fopen(filename.c_str(), "wb")) == nullptr)
	{
		throw std::runtime_error(
			"write buffer to file failed: open file " + filename + " failed");
	}

	writeSize = fwrite(buffer.data(), 1, buffer.size(), file);
	fclose(file);

	if(writeSize < buffer.size())
	{
		throw std::runtime_error(
			"write buffer to file " + filename + " failed: write file content failed");
	}
}

static void enclave_init(sgx_enclave_id_t *p_eid)
{
	sgx_launch_token_t token = { 0 };
	sgx_status_t ret = SGX_ERROR_UNEXPECTED;
	int updated = 0;

	std::vector<uint8_t> tokenBuf;
	try
	{
		tokenBuf = ReadFile2Buffer(DECENT_ENCLAVE_PLATFORM_SGX_TOKEN);
	}
	catch(const std::runtime_error&)
	{}

	ret = sgx_create_enclave(
		DECENT_ENCLAVE_PLATFORM_SGX_IMAGE,
		1 /*SGX_DEBUG_FLAG*/,
		&token,
		&updated,
		p_eid,
		nullptr);

	if (ret != SGX_SUCCESS) {
		throw std::runtime_error("Failed to create enclave");
	}

	if (updated == 1)
	{
		tokenBuf.resize(std::distance(std::begin(token), std::end(token)));
		std::copy(std::begin(token), std::end(token), tokenBuf.begin());
		WriteBuffer2File(DECENT_ENCLAVE_PLATFORM_SGX_TOKEN, tokenBuf);
	}
}

static bool BenchmarkOnUntrusted(
	const std::vector<uint8_t>& wasmBytecode,
	const std::vector<uint8_t>& noptWasmBytecode
)
{
	return PolybenchTester::EnclaveWasmMain(
		wasmBytecode.data(), wasmBytecode.size(),
		noptWasmBytecode.data(), noptWasmBytecode.size()
	);
}

static void BenchmarkOnEnclave(
	const std::vector<uint8_t>& wasmBytecode,
	const std::vector<uint8_t>& noptWasmBytecode
)
{
	// init enclave
	sgx_enclave_id_t eid = 0;
	enclave_init(&eid);

	// iwasm main
	auto ret = ecall_enclave_wasm_main(
		eid,
		wasmBytecode.data(), wasmBytecode.size(),
		noptWasmBytecode.data(), noptWasmBytecode.size()
	);
	if(ret != SGX_SUCCESS)
	{
		std::cerr << "ERROR: "
			<< "Failed to run ecall_enclave_wasm_main." << std::endl;
	}

	// destroy enclave
	sgx_destroy_enclave(eid);
}

int main(int argc, char**argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: "
			<< argv[0] << " <wasm file> <inst. wasm file>" << std::endl;
		return -1;
	}

	const std::string wasmFilenamePath = argv[1];
	const std::string instWasmFilenamePath = argv[2];

	auto wasmBytecode = ReadFile2Buffer(wasmFilenamePath);
	auto instWasmBytecode = ReadFile2Buffer(instWasmFilenamePath);

	if (!BenchmarkOnUntrusted(wasmBytecode, instWasmBytecode))
	{
		return -1;
	}
	BenchmarkOnEnclave(wasmBytecode, instWasmBytecode);

	return 0;
}
