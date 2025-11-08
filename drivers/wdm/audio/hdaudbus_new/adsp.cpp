#include "driver.h"
#define ADSP_DECL 1
#include "adsp.h"

NTSTATUS ADSPGetResources(_In_ PVOID _context, _PCI_BAR* hdaBar, _PCI_BAR* adspBar, PVOID *ppcap, PNHLT_INFO nhltInfo, BUS_INTERFACE_STANDARD* pciConfig) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	if (hdaBar) {
		*hdaBar = devData->FdoContext->m_BAR0;
	}

	if (adspBar) {
		*adspBar = devData->FdoContext->m_BAR4;
	}

	if (ppcap) {
		*ppcap = devData->FdoContext->ppcap;
	}

	if (nhltInfo) {
		if (devData->FdoContext->nhlt) {
			nhltInfo->nhlt = devData->FdoContext->nhlt;
			nhltInfo->nhltSz = devData->FdoContext->nhltSz;
		}
		else if (devData->FdoContext->sofTplg) {
			nhltInfo->nhlt = devData->FdoContext->sofTplg;
			nhltInfo->nhltSz = devData->FdoContext->sofTplgSz;
		}
	}

	if (pciConfig) {
		*pciConfig = devData->FdoContext->BusInterface;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ADSPSetPowerState(_In_ PVOID _context, _In_ DEVICE_POWER_STATE powerState) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	NTSTATUS status = STATUS_SUCCESS;
	if (powerState == PowerDeviceD3) {
		WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);
	} else if (powerState == PowerDeviceD0) {
		status = WdfDeviceStopIdle(devData->FdoContext->WdfDevice, TRUE);
	}
	return status;
}

NTSTATUS ADSPRegisterInterrupt(_In_ PVOID _context, _In_ PADSP_INTERRUPT_CALLBACK callback, _In_ PVOID callbackContext) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	devData->FdoContext->dspInterruptCallback = callback;
	devData->FdoContext->dspInterruptContext = callbackContext;
	return STATUS_SUCCESS;
}

NTSTATUS ADSPUnregisterInterrupt(_In_ PVOID _context) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	devData->FdoContext->dspInterruptCallback = NULL;
	devData->FdoContext->dspInterruptContext = NULL;
	return STATUS_SUCCESS;
}

NTSTATUS ADSPGetRenderStream(_In_ PVOID _context, HDAUDIO_STREAM_FORMAT StreamFormat, PHANDLE Handle, _Out_ UINT8* streamTag) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoContext = devData->FdoContext;

	NTSTATUS status = WdfDeviceStopIdle(devData->FdoContext->WdfDevice, TRUE);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);
	for (UINT32 i = 0; i < fdoContext->playbackStreams; i++) {
		int tag = fdoContext->playbackIndexOff + i;
		PHDAC_STREAM stream = &fdoContext->streams[tag];
		if (stream->PdoContext != NULL) {
			continue;
		}

		stream->stripe = FALSE;
		stream->PdoContext = devData;
		stream->running = FALSE;
		stream->streamFormat = StreamFormat;

		int mask = HDA_PPCTL_PROCEN(stream->idx);
		UINT32 val = 0;
		val = read16(fdoContext->ppcap + HDA_REG_PP_PPCTL) & mask;

		if (!val) {
			hdac_update32(fdoContext->ppcap, HDA_REG_PP_PPCTL, mask, mask);
		}

		if (Handle)
			*Handle = (HANDLE)stream;
		if (streamTag)
			*streamTag = stream->streamTag;

		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_SUCCESS;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS ADSPGetCaptureStream(_In_ PVOID _context, HDAUDIO_STREAM_FORMAT StreamFormat, PHANDLE Handle, _Out_ UINT8* streamTag) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoContext = devData->FdoContext;

	NTSTATUS status = WdfDeviceStopIdle(devData->FdoContext->WdfDevice, TRUE);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);
	for (UINT32 i = 0; i < fdoContext->captureStreams; i++) {
		int tag = fdoContext->captureIndexOff + i;
		PHDAC_STREAM stream = &fdoContext->streams[tag];
		if (stream->PdoContext != NULL) {
			continue;
		}

		stream->stripe = FALSE;
		stream->PdoContext = devData;
		stream->running = FALSE;
		stream->streamFormat = StreamFormat;

		int mask = HDA_PPCTL_PROCEN(stream->idx);
		UINT32 val = 0;
		val = read16(fdoContext->ppcap + HDA_REG_PP_PPCTL) & mask;

		if (!val) {
			hdac_update32(fdoContext->ppcap, HDA_REG_PP_PPCTL, mask, mask);
		}

		if (Handle)
			*Handle = (HANDLE)stream;
		if (streamTag)
			*streamTag = stream->streamTag;

		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_SUCCESS;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS ADSPFreeStream(
	_In_ PVOID _context,
	_In_ HANDLE Handle
) {
	SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s called!\n", __func__);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	if (stream->running) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	PFDO_CONTEXT fdoContext = devData->FdoContext;
	int mask = HDA_PPCTL_PROCEN(stream->idx);
	UINT32 val = 0;
	val = read16(fdoContext->ppcap + HDA_REG_PP_PPCTL) & mask;

	if (val) {
		hdac_update32(fdoContext->ppcap, HDA_REG_PP_PPCTL, mask, 0);
	}

	stream->PdoContext = NULL;
	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);

	return STATUS_SUCCESS;
}

NTSTATUS ADSPPrepareDSP(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ unsigned int ByteSize,
	_In_ int NumBlocks,
	_Out_ PVOID* bdlBuf
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	if (stream->running) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_DEVICE_BUSY;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	PHYSICAL_ADDRESS maxAddr;
	maxAddr.QuadPart = devData->FdoContext->is64BitOK ? MAXULONG64 : MAXULONG32;

	stream->mdlBuf = NULL;
	stream->bufSz = ByteSize;
	stream->numBlocks = (UINT16)NumBlocks;

	hdac_stream_reset(stream);

	/* reset BDL address */
	stream_write32(stream, SD_BDLPL, 0);
	stream_write32(stream, SD_BDLPU, 0);

	hdac_stream_setup(stream);

	if (bdlBuf)
		*bdlBuf = stream->bdl;
	return STATUS_SUCCESS;
}

NTSTATUS ADSPCleanupDSP(_In_ PVOID _context, _In_ HANDLE Handle) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	stream_write32(stream, SD_BDLPL, 0);
	stream_write32(stream, SD_BDLPU, 0);
	stream_write32(stream, SD_CTL, 0);

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	return STATUS_SUCCESS;
}

void ADSPStartStopDSP(_In_ PVOID _context, _In_ HANDLE Handle, BOOL startStop) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return;
	}

	if (startStop)
		hdac_stream_start(stream);
	else
		hdac_stream_stop(stream);
}

void ADSPEnableSPIB(_In_ PVOID _context, _In_ HANDLE Handle, UINT32 value) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return;
	}

	if (!devData->FdoContext->spbcap) {
		return;
	}

	UINT32 mask = (1 << stream->idx);
	hdac_update32(devData->FdoContext->spbcap, HDA_REG_SPB_SPBFCCTL, mask, mask);

	write32(stream->spib_addr, value);
}

void ADSPDisableSPIB(_In_ PVOID _context, _In_ HANDLE Handle) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return;
	}

	if (!devData->FdoContext->spbcap) {
		return;
	}

	UINT32 mask = (1 << stream->idx);
	hdac_update32(devData->FdoContext->spbcap, HDA_REG_SPB_SPBFCCTL, mask, 0);

	write32(stream->spib_addr, 0);
}

UINT32 ADSPStreamPosition(_In_ PVOID _context, _In_ HANDLE Handle) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return 0;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (!stream || stream->PdoContext != devData) {
		return 0;
	}

	return *stream->posbuf;
}

ADSP_BUS_INTERFACE ADSP_BusInterface(PVOID Context) {
	ADSP_BUS_INTERFACE busInterface;
	RtlZeroMemory(&busInterface, sizeof(ADSP_BUS_INTERFACE));

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)Context;

	busInterface.Size = sizeof(ADSP_BUS_INTERFACE);
	busInterface.Version = 1;
	busInterface.Context = Context;
	busInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	busInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
	busInterface.CtlrDevId = devData->CodecIds.CtlrDevId;
	busInterface.GetResources = ADSPGetResources;
	busInterface.SetDSPPowerState = ADSPSetPowerState;
	busInterface.RegisterInterrupt = ADSPRegisterInterrupt;
	busInterface.UnregisterInterrupt = ADSPUnregisterInterrupt;

	busInterface.GetRenderStream = ADSPGetRenderStream;
	busInterface.GetCaptureStream = ADSPGetCaptureStream;
	busInterface.FreeStream = ADSPFreeStream;
	busInterface.PrepareDSP = ADSPPrepareDSP;
	busInterface.CleanupDSP = ADSPCleanupDSP;
	busInterface.TriggerDSP = ADSPStartStopDSP;
	busInterface.StreamPosition = ADSPStreamPosition;

	busInterface.DSPEnableSPIB = ADSPEnableSPIB;
	busInterface.DSPDisableSPIB = ADSPDisableSPIB;

	return busInterface;
}