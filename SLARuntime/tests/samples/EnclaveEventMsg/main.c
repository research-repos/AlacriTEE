// Copyright (c) 2024 SLARuntime Authors
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common.h>

int32_t enclave_wasm_main(uint32_t eIdSize, uint32_t eDataSize)
{
	int32_t retVal = 0;
	char buf[2048];
	uint32_t recvSize = 0;

	// printing current info
	sprintf(buf, "Event ID size               : %d\n", eIdSize);
	enclave_wasm_print_string(buf);
	sprintf(buf, "Event data size             : %d\n", eDataSize);
	enclave_wasm_print_string(buf);

	// getting and printing length of event ID and data
	recvSize = enclave_wasm_get_event_id_len();
	if (recvSize != eIdSize)
	{
		sprintf(buf, "Event ID size mismatch      : %d != %d\n", recvSize, eIdSize);
		enclave_wasm_print_string(buf);
		retVal = 1;
	}
	else
	{
		sprintf(buf, "Event ID size match         : %d == %d\n", recvSize, eIdSize);
		enclave_wasm_print_string(buf);
	}
	recvSize = enclave_wasm_get_event_data_len();
	if (recvSize != eDataSize)
	{
		sprintf(buf, "Event data size mismatch    : %d != %d\n", recvSize, eDataSize);
		enclave_wasm_print_string(buf);
		retVal = 1;
	}
	else
	{
		sprintf(buf, "Event data size match       : %d == %d\n", recvSize, eDataSize);
		enclave_wasm_print_string(buf);
	}

	// allocating buffers for event ID and data
	uint8_t* eId = (uint8_t*)malloc(eIdSize + 1);
	uint8_t* eData = (uint8_t*)malloc(eDataSize + 1);
	sprintf(buf, "Event ID buffer allocated   : %p\n", eId);
	enclave_wasm_print_string(buf);
	sprintf(buf, "Event data buffer allocated : %p\n", eData);
	enclave_wasm_print_string(buf);

	// filling and printing initial data in buffers
	sprintf((char*)eId, "Hello");
	sprintf((char*)eData, "World");
	sprintf(buf, "Event ID buffer             : %s\n", eId);
	enclave_wasm_print_string(buf);
	sprintf(buf, "Event data buffer           : %s\n", eData);
	enclave_wasm_print_string(buf);

	// getting event ID
	recvSize = enclave_wasm_get_event_id(eId, eIdSize);
	if (recvSize != eIdSize)
	{
		sprintf(buf, "Event ID size mismatch      : %d != %d\n", recvSize, eIdSize);
		enclave_wasm_print_string(buf);
		retVal = 1;
	}
	else
	{
		sprintf(buf, "Event ID size match         : %d == %d\n", recvSize, eIdSize);
		enclave_wasm_print_string(buf);
	}

	// getting event data
	recvSize = enclave_wasm_get_event_data(eData, eDataSize);
	if (recvSize != eDataSize)
	{
		sprintf(buf, "Event data size mismatch    : %d != %d\n", recvSize, eDataSize);
		enclave_wasm_print_string(buf);
		retVal = 1;
	}
	else
	{
		sprintf(buf, "Event data size match       : %d == %d\n", recvSize, eDataSize);
		enclave_wasm_print_string(buf);
	}

	// freeing buffers
	free(eId);
	free(eData);

	return 0;
}
