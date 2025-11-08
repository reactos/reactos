#include "driver.h"
#include "nhlt.h"
#include <acpiioct.h>

DEFINE_GUID(GUID_SST_NHLT,
	0xA69F886E, 0x6CEB, 0x4594, 0xA4, 0x1F, 0x7B, 0x5D, 0xCE, 0x24, 0xC5, 0x53);

NTSTATUS
NHLTQuery(
	_In_ WDFDEVICE FxDevice,
	_In_ ULONG Arg1,
	_In_ ULONG Arg2,
	_Out_ WDFMEMORY* outputBufferMemoryArg
) {
	ULONG_PTR bytesReturned;
	NTSTATUS status = STATUS_ACPI_NOT_INITIALIZED;
	WDFMEMORY inputBufferMemory = NULL;
	PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX inputBuffer = NULL;
	PACPI_METHOD_ARGUMENT inputArgument = NULL;
	ACPI_EVAL_OUTPUT_BUFFER outputHeader;
	WDFMEMORY outputBufferMemory = NULL;
	WDF_MEMORY_DESCRIPTOR inputBufferMemoryDescriptor;
	WDF_MEMORY_DESCRIPTOR outputBufferMemoryDescriptor;

	ULONG inputBufferSize =
		(ULONG)(
			FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX, Argument) +
			ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
			ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
			ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
			ACPI_METHOD_ARGUMENT_LENGTH(0)
			);

	status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
		PagedPool,
		SKLHDAUDBUS_POOL_TAG,
		inputBufferSize,
		&inputBufferMemory,
		(PVOID*)&inputBuffer);
	if (!NT_SUCCESS(status)) {
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to create input buffer\n");
		goto end;
	}
	RtlZeroMemory(inputBuffer, inputBufferSize);

	inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_V1_EX;
	status = RtlStringCchPrintfA(
		inputBuffer->MethodName,
		sizeof(inputBuffer->MethodName),
		"_DSM"
	);
	if (!NT_SUCCESS(status)) {
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to write method name\n");
		goto end;
	}

	inputBuffer->Size = inputBufferSize;
	inputBuffer->ArgumentCount = 4;
	inputArgument = inputBuffer->Argument;

	//arg 0
	ACPI_METHOD_SET_ARGUMENT_BUFFER(inputArgument,
		&GUID_SST_NHLT,
		sizeof(GUID_SST_NHLT));

	inputArgument = ACPI_METHOD_NEXT_ARGUMENT(inputArgument);
	ACPI_METHOD_SET_ARGUMENT_INTEGER(inputArgument, Arg1);

	inputArgument = ACPI_METHOD_NEXT_ARGUMENT(inputArgument);
	ACPI_METHOD_SET_ARGUMENT_INTEGER(inputArgument, Arg2);

	inputArgument = ACPI_METHOD_NEXT_ARGUMENT(inputArgument);
	RtlZeroMemory(inputArgument, sizeof(*inputArgument));
	inputArgument->Type = ACPI_METHOD_ARGUMENT_PACKAGE_EX;
	inputArgument->DataLength = 0;

	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputBufferMemoryDescriptor,
		inputBufferMemory,
		0);
	RtlZeroMemory(&outputHeader, sizeof(outputHeader));
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputBufferMemoryDescriptor,
		&outputHeader,
		sizeof(outputHeader));

	status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(FxDevice),
		NULL,
		IOCTL_ACPI_EVAL_METHOD_EX,
		&inputBufferMemoryDescriptor,
		&outputBufferMemoryDescriptor,
		NULL,
		&bytesReturned);

	if (status == STATUS_BUFFER_OVERFLOW) {
		status = STATUS_SUCCESS;
	}
	else if (!NT_SUCCESS(status)) {
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed first ioctl\n");
		goto end;
	}

	status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
		PagedPool,
		SKLHDAUDBUS_POOL_TAG,
		outputHeader.Length,
		&outputBufferMemory,
		(PVOID*)NULL);
	if (!NT_SUCCESS(status)) {
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to create output buffer\n");
		goto end;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputBufferMemoryDescriptor,
		outputBufferMemory,
		0);
	status = WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(FxDevice),
		NULL,
		IOCTL_ACPI_EVAL_METHOD_EX,
		&inputBufferMemoryDescriptor,
		&outputBufferMemoryDescriptor,
		NULL,
		&bytesReturned);

	if (!NT_SUCCESS(status)) {
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to do 2nd ioctl\n");
		goto end;
	}

end:
	if (inputBufferMemory != NULL) {
		WdfObjectDelete(inputBufferMemory);
		inputBufferMemory = NULL;
	}

	if (!NT_SUCCESS(status)) {
		if (outputBufferMemory != NULL) {
			WdfObjectDelete(outputBufferMemory);
			outputBufferMemory = NULL;
		}
	}
	else {
		*outputBufferMemoryArg = outputBufferMemory;
	}
	return status;
}

NTSTATUS NHLTCheckSupported(_In_ WDFDEVICE FxDevice) {
	WDFMEMORY outputBufferMemory;
	NTSTATUS status = NHLTQuery(FxDevice, NHLTRev1, NHLTSupportQuery, &outputBufferMemory);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	if (!outputBufferMemory) {
		return STATUS_NO_MEMORY;
	}

	PACPI_EVAL_OUTPUT_BUFFER outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(outputBufferMemory, NULL);
	if (outputBuffer->Count < 1) {
		status = STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
		goto end;
	}
	PACPI_METHOD_ARGUMENT argument = outputBuffer->Argument;

	UCHAR supportedQueries = argument->Data[0];

	if ((supportedQueries & 0x3) == 0) {
		status = STATUS_NOT_SUPPORTED;
	}

end:
	if (outputBufferMemory != NULL) {
		WdfObjectDelete(outputBufferMemory);
		outputBufferMemory = NULL;
	}
	return status;
}

void parseACPI(UINT8* res, UINT32 offset, UINT32 sz, UINT64* nhltAddr, UINT64* nhltSz);

NTSTATUS NHLTQueryTableAddress(_In_ WDFDEVICE FxDevice, UINT64 *nhltAddr, UINT64 *nhltSz) {
	WDFMEMORY outputBufferMemory;
	NTSTATUS status = NHLTQuery(FxDevice, NHLTRev1, NHLTMemoryAddress, &outputBufferMemory);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	if (!outputBufferMemory) {
		return STATUS_NO_MEMORY;
	}

	PACPI_EVAL_OUTPUT_BUFFER outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(outputBufferMemory, NULL);
	if (outputBuffer->Count < 1) {
		return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
		goto end;
	}

	PACPI_METHOD_ARGUMENT argument = outputBuffer->Argument;

	UINT8* res = argument->Data;
	UINT32 sz = argument->DataLength;

	*nhltAddr = 0;
	*nhltSz = 0;
	parseACPI(res, 0, sz, nhltAddr, nhltSz);

	if (nhltAddr == 0 || nhltSz == 0) {
		status = STATUS_UNSUCCESSFUL;
	}

end:
	if (outputBufferMemory != NULL) {
		WdfObjectDelete(outputBufferMemory);
		outputBufferMemory = NULL;
	}
	return status;
}

//Begin ACPI parser

#define ACPI_RESOURCE_NAME_ADDRESS64            0x8A

/*
 * LARGE descriptors
 */
#define AML_RESOURCE_LARGE_HEADER_COMMON \
    UINT8                           DescriptorType;\
    UINT16                          ResourceLength;

#define AML_RESOURCE_ADDRESS_COMMON \
    UINT8                           ResourceType; \
    UINT8                           Flags; \
    UINT8                           SpecificFlags;

#include <pshpack1.h>
typedef struct aml_resource_address64
{
	AML_RESOURCE_LARGE_HEADER_COMMON
	AML_RESOURCE_ADDRESS_COMMON
	UINT64                          Granularity;
	UINT64                          Minimum;
	UINT64                          Maximum;
	UINT64                          TranslationOffset;
	UINT64                          AddressLength;
} AML_RESOURCE_ADDRESS64;
#include <poppack.h>

void parseACPIMemory64(UINT8* res, UINT32 offset, UINT32 sz, UINT64 *nhltAddr, UINT64 *nhltSz) {
	if (offset + 3 > sz)
		return;

	UINT8 opcode = res[offset];
	if (opcode != ACPI_RESOURCE_NAME_ADDRESS64)
		return;

	AML_RESOURCE_ADDRESS64* address64 = (AML_RESOURCE_ADDRESS64*)(res + offset);

	*nhltAddr = address64->Minimum;
	*nhltSz = address64->AddressLength;
}

void parseACPI(UINT8* res, UINT32 offset, UINT32 sz, UINT64* nhltAddr, UINT64* nhltSz) {
	if (offset + 3 > sz)
		return;

	UINT8 opcode = res[offset];

	UINT16 len;
	memcpy(&len, res + offset + 1, sizeof(UINT16));

	if (opcode == ACPI_RESOURCE_NAME_ADDRESS64)
		parseACPIMemory64(res, offset, sz, nhltAddr, nhltSz);

	offset += (len + 3);
	parseACPI(res, offset, sz, nhltAddr, nhltSz);
}