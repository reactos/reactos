#include "driver.h"

NTSTATUS HDA_TransferCodecVerbs(
	_In_ PVOID _context,
	_In_ ULONG Count,
	_Inout_updates_(Count)
	PHDAUDIO_CODEC_TRANSFER CodecTransfer,
	_In_opt_ PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback,
	_In_opt_ PVOID Context
) {
	SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s called (Count: %d)!\n", __func__, Count);

	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	NTSTATUS status = STATUS_SUCCESS;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoCtx = devData->FdoContext;

	status = WdfDeviceStopIdle(devData->FdoContext->WdfDevice, TRUE);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = SendHDACmds(fdoCtx, Count, CodecTransfer);
	if (!NT_SUCCESS(status)) {
		goto out;
	}

	UINT16 codecAddr = (UINT16)devData->CodecIds.CodecAddress;

	int timeout_ms = 1000;
	LARGE_INTEGER StartTime;
	KeQuerySystemTime(&StartTime);
	for (ULONG loopcounter = 0; ; loopcounter++) {
		ULONG TransferredCount = 0;
		for (ULONG i = 0; i < Count; i++) {
			if (CodecTransfer[i].Input.IsValid) {
				TransferredCount++;
			}
		}
		if (TransferredCount >= Count) {
			break;
		}

		LARGE_INTEGER CurrentTime;
		KeQuerySystemTime(&CurrentTime);

		if (((CurrentTime.QuadPart - StartTime.QuadPart) / (10 * 1000)) >= timeout_ms) {
			InterlockedAdd(&fdoCtx->rirb.cmds[codecAddr], TransferredCount - Count);

			SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s timeout (Count: %d, transferred %d)!\n", __func__, Count, TransferredCount);
			status = STATUS_IO_TIMEOUT;
			goto out;
		}

		LARGE_INTEGER Timeout;
		Timeout.QuadPart = -10 * 100;
		KeWaitForSingleObject(&fdoCtx->rirb.xferEvent[codecAddr], Executive, KernelMode, TRUE, &Timeout);
	}

	SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s exit (Count: %d)!\n", __func__, Count);

	if (Callback) { //TODO: Do this async
		DbgPrint("Got Callback\n");
		Callback(CodecTransfer, Context);
	}

	status = STATUS_SUCCESS;

out:
	WdfDeviceResumeIdle(fdoCtx->WdfDevice);
	return status;
}

NTSTATUS HDA_AllocateCaptureDmaEngine(
	_In_ PVOID _context,
	_In_ UCHAR CodecAddress,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_Out_ PHANDLE Handle,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
	UNREFERENCED_PARAMETER(CodecAddress);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

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

		stream->PdoContext = devData;
		stream->running = FALSE;
		stream->streamFormat = *StreamFormat;

		ConverterFormat->ConverterFormat = hdac_format(stream);

		if (Handle)
			*Handle = (HANDLE)stream;

		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_SUCCESS;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS HDA_AllocateRenderDmaEngine(
	_In_ PVOID _context,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_In_ BOOLEAN Stripe,
	_Out_ PHANDLE Handle,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

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

		stream->stripe = Stripe;
		stream->PdoContext = devData;
		stream->running = FALSE;
		stream->streamFormat = *StreamFormat;

		ConverterFormat->ConverterFormat = hdac_format(stream);

		if (Handle)
			*Handle = (HANDLE)stream;

		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_SUCCESS;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS HDA_ChangeBandwidthAllocation(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	if (stream->running) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	stream->streamFormat = *StreamFormat;
	ConverterFormat->ConverterFormat = hdac_format(stream);

	hdac_stream_setup(stream);

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s called!\n", __func__);

	return STATUS_SUCCESS;
}

NTSTATUS HDA_FreeDmaEngine(
	_In_ PVOID _context,
	_In_ HANDLE Handle
) {
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	if (stream->running) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	stream->PdoContext = NULL;
	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	WdfDeviceResumeIdle(devData->FdoContext->WdfDevice);

	return STATUS_SUCCESS;
}

NTSTATUS HDA_SetDmaEngineState(
	_In_ PVOID _context,
	_In_ HDAUDIO_STREAM_STATE StreamState,
	_In_ ULONG NumberOfHandles,
	_In_reads_(NumberOfHandles) PHANDLE Handles
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	for (ULONG i = 0; i < NumberOfHandles; i++) {
		PHDAC_STREAM stream = (PHDAC_STREAM)Handles[i];
		if (stream->PdoContext != devData) {
			return STATUS_INVALID_HANDLE;
		}

		WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

		if (StreamState == RunState && !stream->running) {
			hdac_stream_start(stream);
			stream->running = TRUE;
		}
		else if ((StreamState == PauseState || StreamState == StopState) && stream->running) {
			hdac_stream_stop(stream);
			stream->running = FALSE;
		}
		else if (StreamState == ResetState) {
			if (!stream->running) {
				hdac_stream_reset(stream);
			}
			else {
				return STATUS_INVALID_PARAMETER;
			}
		}

		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	}

	return STATUS_SUCCESS;
}

VOID HDA_GetWallClockRegister(
	_In_ PVOID _context,
	_Out_ PULONG* Wallclock
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		*Wallclock = NULL;
		return;
	}
	*Wallclock = (ULONG *)((devData->FdoContext)->m_BAR0.Base.baseptr + HDA_REG_WALLCLKA);
}

NTSTATUS HDA_GetLinkPositionRegister(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_Out_ PULONG* Position
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	if (devData->CodecIds.CtlrVenId != VEN_INTEL) //Experimental Non-Intel support
		*Position = (ULONG*)(stream->sdAddr + HDA_REG_SD_LPIBA);
	else
		*Position = (ULONG *)stream->posbuf; //Use Posbuf for all Intel

	return STATUS_SUCCESS;
}

NTSTATUS HDA_RegisterEventCallback(
	_In_ PVOID _context,
	_In_ PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine,
	_In_opt_ PVOID Context,
	_Out_ PUCHAR Tag
) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;


	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (devData->FdoContext) {
		WdfInterruptAcquireLock(devData->FdoContext->Interrupt);
	}

	for (UINT8 i = 0; i < MAX_UNSOLICIT_CALLBACKS; i++) {
		if (devData->unsolitCallbacks[i].inUse)
			continue;

		if (Tag)
			*Tag = i;

		devData->unsolitCallbacks[i].inUse = TRUE;
		devData->unsolitCallbacks[i].Context = Context;
		devData->unsolitCallbacks[i].Routine = Routine;

		if (devData->FdoContext) {
			WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
		}
		return STATUS_SUCCESS;
	}

	if (devData->FdoContext) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	}
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS HDA_UnregisterEventCallback(
	_In_ PVOID _context,
	_In_ UCHAR Tag
) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->unsolitCallbacks[Tag].inUse) {
		return STATUS_NOT_FOUND;
	}

	if (devData->FdoContext) {
		WdfInterruptAcquireLock(devData->FdoContext->Interrupt);
	}

	devData->unsolitCallbacks[Tag].Routine = NULL;
	devData->unsolitCallbacks[Tag].Context = NULL;
	devData->unsolitCallbacks[Tag].inUse = FALSE;

	if (devData->FdoContext) {
		WdfInterruptReleaseLock(devData->FdoContext->Interrupt);
	}

	return STATUS_SUCCESS;
}

NTSTATUS HDA_GetDeviceInformation(
	_In_ PVOID _context,
	_Inout_ PHDAUDIO_DEVICE_INFORMATION DeviceInformation
) {
	if (!_context)
		return STATUS_NO_SUCH_DEVICE;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	if (DeviceInformation->Size >= sizeof(HDAUDIO_DEVICE_INFORMATION)) {
		DeviceInformation->CodecsDetected = devData->FdoContext->numCodecs;
		DeviceInformation->DeviceVersion = devData->FdoContext->hwVersion;
		DeviceInformation->DriverVersion = 0x100;
		DeviceInformation->IsStripingSupported = TRUE;
	}
	if (DeviceInformation->Size >= sizeof(HDAUDIO_DEVICE_INFORMATION_V2)) {
		PHDAUDIO_DEVICE_INFORMATION_V2 DeviceInformationV2 = (PHDAUDIO_DEVICE_INFORMATION_V2)DeviceInformation;
		DeviceInformationV2->CtrlRevision = devData->FdoContext->revId;
		DeviceInformationV2->CtrlVendorId = devData->FdoContext->venId;
		DeviceInformationV2->CtrlDeviceId = devData->FdoContext->devId;
	}

	return STATUS_SUCCESS;
}

void HDA_GetResourceInformation(
	_In_ PVOID _context,
	_Out_ PUCHAR CodecAddress,
	_Out_ PUCHAR FunctionGroupStartNode
) {
	if (!_context)
		return;
	
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (CodecAddress)
		*CodecAddress = (UINT8)devData->CodecIds.CodecAddress;
	if (FunctionGroupStartNode)
		*FunctionGroupStartNode = devData->CodecIds.FunctionGroupStartNode;
}

NTSTATUS HDA_AllocateDmaBufferWithNotification(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ ULONG NotificationCount,
	_In_ SIZE_T RequestedBufferSize,
	_Out_ PMDL* BufferMdl,
	_Out_ PSIZE_T AllocatedBufferSize,
	_Out_ PSIZE_T OffsetFromFirstPage,
	_Out_ PUCHAR StreamId,
	_Out_ PULONG FifoSize
) {
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	if (stream->running) {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	if (stream->mdlBuf) {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	PHYSICAL_ADDRESS zeroAddr;
	zeroAddr.QuadPart = 0;
	PHYSICAL_ADDRESS maxAddr;
	maxAddr.QuadPart = devData->FdoContext->is64BitOK ? MAXULONG64 : MAXULONG32;

	UINT32 allocSize = (UINT32)RequestedBufferSize;
	UINT32 allocOffset = 0;
	UINT32 halfSize = 0;
	if (NotificationCount == 2) {
		halfSize = (UINT32)RequestedBufferSize / 2;
		allocOffset = PAGE_SIZE - (halfSize % PAGE_SIZE);
		allocSize = (UINT32)RequestedBufferSize + allocOffset;
	}

	PMDL mdl = MmAllocatePagesForMdl(zeroAddr, maxAddr, zeroAddr, allocSize);
	if (!mdl) {
		return STATUS_NO_MEMORY;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);
	stream->mdlBuf = mdl;
	stream->bufSz = (UINT32)RequestedBufferSize;

	*BufferMdl = mdl;
	*AllocatedBufferSize = RequestedBufferSize;
	*OffsetFromFirstPage = allocOffset;
	*StreamId = stream->streamTag;

	{
		UINT16 numBlocks = 0;
		UINT16 pageNum = 0;

		//Set up the BDL
		PHDAC_BDLENTRY bdl = stream->bdl;
		UINT32 size = (UINT32)RequestedBufferSize;

		PPFN_NUMBER pfnArray = MmGetMdlPfnArray(mdl);
		UINT32 offset = allocOffset;
		while (halfSize > 0) {
			if (numBlocks > HDA_MAX_BDL_ENTRIES) {
				DbgPrint("Too many BDL entries!\n");
				numBlocks = HDA_MAX_BDL_ENTRIES;
				break;
			}

			UINT32 pageOff = offset % PAGE_SIZE;
			UINT32 chunk = (pageOff == 0) ? PAGE_SIZE : (PAGE_SIZE - pageOff);
			if (halfSize < chunk)
				chunk = halfSize;

			PFN_NUMBER pfn = pfnArray[pageNum];
			PHYSICAL_ADDRESS addr = { 0 };
			addr.QuadPart = pfn << PAGE_SHIFT;

			bdl->lowAddr = addr.LowPart + pageOff;
			bdl->highAddr = addr.HighPart;
			bdl->len = chunk;

			halfSize -= chunk;
			size -= chunk;

			//Program interrupt for when buffer is halfway
			bdl->ioc = (halfSize > 0) ? 0 : 1;
			bdl++;
			numBlocks++;
			offset += chunk;
			if ((offset % PAGE_SIZE) == 0)
				pageNum++; //Only increment page num if we go past page boundary
		}

		while (size > 0) {
			if (numBlocks > HDA_MAX_BDL_ENTRIES) {
				DbgPrint("Too many BDL entries!\n");
				numBlocks = HDA_MAX_BDL_ENTRIES;
				break;
			}

			UINT32 pageOff = offset % PAGE_SIZE;
			UINT32 chunk = (pageOff == 0) ? PAGE_SIZE : (PAGE_SIZE - pageOff);
			if (size < chunk)
				chunk = size;

			PFN_NUMBER pfn = pfnArray[pageNum];
			PHYSICAL_ADDRESS addr = { 0 };
			addr.QuadPart = pfn << PAGE_SHIFT;
			bdl->lowAddr = addr.LowPart + pageOff;
			bdl->highAddr = addr.HighPart;
			bdl->len = chunk;

			size -= chunk;
			//Program interrupt for when buffer ends
			bdl->ioc = (size > 0) ? 0 : 1;
			bdl++;
			numBlocks++;
			offset += chunk;
			if ((offset % PAGE_SIZE) == 0)
				pageNum++; //Only increment page num if we go past page boundary
		}
		stream->numBlocks = numBlocks;
	}

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	hdac_stream_reset(stream);
	hdac_stream_setup(stream);

	*FifoSize = stream->fifoSize;
	return STATUS_SUCCESS;
}

NTSTATUS HDA_FreeDmaBufferWithNotification(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PMDL BufferMdl,
	_In_ SIZE_T BufferSize
) {
	UNREFERENCED_PARAMETER(BufferMdl);
	UNREFERENCED_PARAMETER(BufferSize);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	if (stream->running) {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	if (!stream->mdlBuf) {
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	WdfInterruptAcquireLock(devData->FdoContext->Interrupt);

	stream_write32(stream, SD_BDLPL, 0);
	stream_write32(stream, SD_BDLPU, 0);
	stream_write32(stream, SD_CTL, 0);

	PMDL mdl = stream->mdlBuf;
	stream->mdlBuf = NULL;

	WdfInterruptReleaseLock(devData->FdoContext->Interrupt);

	MmFreePagesFromMdl(mdl);
	ExFreePool(mdl);

	return STATUS_SUCCESS;
}

NTSTATUS HDA_AllocateDmaBuffer(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ SIZE_T RequestedBufferSize,
	_Out_ PMDL* BufferMdl,
	_Out_ PSIZE_T AllocatedBufferSize,
	_Out_ PUCHAR StreamId,
	_Out_ PULONG FifoSize
) {
	SIZE_T OffsetFromFirstPage;
	return HDA_AllocateDmaBufferWithNotification(_context, Handle, 1, RequestedBufferSize, BufferMdl, AllocatedBufferSize, &OffsetFromFirstPage, StreamId, FifoSize);
}

NTSTATUS HDA_FreeDmaBuffer(
	_In_ PVOID _context,
	_In_ HANDLE Handle
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	return HDA_FreeDmaBufferWithNotification(_context, Handle, stream->mdlBuf, stream->bufSz);
}

NTSTATUS HDA_RegisterNotificationEvent(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PKEVENT NotificationEvent
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	BOOL registered = FALSE;

	for (int i = 0; i < MAX_NOTIF_EVENTS; i++) {
		if (stream->registeredEvents[i])
			continue;
		stream->registeredEvents[i] = NotificationEvent;
		registered = true;
		break;
	}

	return registered ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS HDA_UnregisterNotificationEvent(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PKEVENT NotificationEvent
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	BOOL registered = FALSE;

	for (int i = 0; i < MAX_NOTIF_EVENTS; i++) {
		if (stream->registeredEvents[i] != NotificationEvent)
			continue;
		stream->registeredEvents[i] = NULL;
		registered = true;
		break;
	}

	return registered ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

NTSTATUS HDA_RegisterNotificationCallback(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	PDEVICE_OBJECT Fdo,
	PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
	PVOID CallbackContext
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	BOOL registered = FALSE;

	for (int i = 0; i < MAX_NOTIF_EVENTS; i++) {
		if (stream->registeredCallbacks[i].InUse)
			continue;
		stream->registeredCallbacks[i].InUse = TRUE;
		stream->registeredCallbacks[i].Fdo = Fdo;
		stream->registeredCallbacks[i].NotificationCallback = NotificationCallback;
		stream->registeredCallbacks[i].CallbackContext = CallbackContext;

		InterlockedIncrement(&Fdo->ReferenceCount);

		registered = true;
		break;
	}

	return registered ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS HDA_UnregisterNotificationCallback(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
	PVOID CallbackContext
) {
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

	BOOL registered = FALSE;

	for (int i = 0; i < MAX_NOTIF_EVENTS; i++) {
		if (stream->registeredCallbacks[i].InUse &&
			stream->registeredCallbacks[i].NotificationCallback != NotificationCallback &&
			stream->registeredCallbacks[i].CallbackContext != CallbackContext)
			continue;

		InterlockedDecrement(&stream->registeredCallbacks[i].Fdo->ReferenceCount);

		RtlZeroMemory(&stream->registeredCallbacks[i], sizeof(stream->registeredCallbacks[i]));
		registered = true;
		break;
	}

	return registered ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

HDAUDIO_BUS_INTERFACE_V2 HDA_BusInterfaceV2(PVOID Context) {
	HDAUDIO_BUS_INTERFACE_V2 busInterface;
	RtlZeroMemory(&busInterface, sizeof(HDAUDIO_BUS_INTERFACE_V2));

	busInterface.Size = sizeof(HDAUDIO_BUS_INTERFACE_V2);
	busInterface.Version = 0x0100;
	busInterface.Context = Context;
	busInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	busInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
	busInterface.TransferCodecVerbs = HDA_TransferCodecVerbs;
	busInterface.AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
	busInterface.AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
	busInterface.ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
	busInterface.AllocateDmaBuffer = HDA_AllocateDmaBuffer;
	busInterface.FreeDmaBuffer = HDA_FreeDmaBuffer;
	busInterface.FreeDmaEngine = HDA_FreeDmaEngine;
	busInterface.SetDmaEngineState = HDA_SetDmaEngineState;
	busInterface.GetWallClockRegister = HDA_GetWallClockRegister;
	busInterface.GetLinkPositionRegister = HDA_GetLinkPositionRegister;
	busInterface.RegisterEventCallback = HDA_RegisterEventCallback;
	busInterface.UnregisterEventCallback = HDA_UnregisterEventCallback;
	busInterface.GetDeviceInformation = HDA_GetDeviceInformation;
	busInterface.GetResourceInformation = HDA_GetResourceInformation;
	busInterface.AllocateDmaBufferWithNotification = HDA_AllocateDmaBufferWithNotification;
	busInterface.FreeDmaBufferWithNotification = HDA_FreeDmaBufferWithNotification;
	busInterface.RegisterNotificationEvent = HDA_RegisterNotificationEvent;
	busInterface.UnregisterNotificationEvent = HDA_UnregisterNotificationEvent;

	return busInterface;
}

HDAUDIO_BUS_INTERFACE_V3 HDA_BusInterfaceV3(PVOID Context) {
	HDAUDIO_BUS_INTERFACE_V3 busInterface;
	RtlZeroMemory(&busInterface, sizeof(HDAUDIO_BUS_INTERFACE_V3));

	busInterface.Size = sizeof(HDAUDIO_BUS_INTERFACE_V3);
	busInterface.Version = 0x0100;
	busInterface.Context = Context;
	busInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	busInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
	busInterface.TransferCodecVerbs = HDA_TransferCodecVerbs;
	busInterface.AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
	busInterface.AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
	busInterface.ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
	busInterface.AllocateDmaBuffer = HDA_AllocateDmaBuffer;
	busInterface.FreeDmaBuffer = HDA_FreeDmaBuffer;
	busInterface.FreeDmaEngine = HDA_FreeDmaEngine;
	busInterface.SetDmaEngineState = HDA_SetDmaEngineState;
	busInterface.GetWallClockRegister = HDA_GetWallClockRegister;
	busInterface.GetLinkPositionRegister = HDA_GetLinkPositionRegister;
	busInterface.RegisterEventCallback = HDA_RegisterEventCallback;
	busInterface.UnregisterEventCallback = HDA_UnregisterEventCallback;
	busInterface.GetDeviceInformation = HDA_GetDeviceInformation;
	busInterface.GetResourceInformation = HDA_GetResourceInformation;
	busInterface.AllocateDmaBufferWithNotification = HDA_AllocateDmaBufferWithNotification;
	busInterface.FreeDmaBufferWithNotification = HDA_FreeDmaBufferWithNotification;
	busInterface.RegisterNotificationEvent = HDA_RegisterNotificationEvent;
	busInterface.UnregisterNotificationEvent = HDA_UnregisterNotificationEvent;
	busInterface.RegisterNotificationCallback = HDA_RegisterNotificationCallback;
	busInterface.UnregisterNotificationCallback = HDA_UnregisterNotificationCallback;

	return busInterface;
}

HDAUDIO_BUS_INTERFACE HDA_BusInterface(PVOID Context) {
	HDAUDIO_BUS_INTERFACE busInterface;
	RtlZeroMemory(&busInterface, sizeof(HDAUDIO_BUS_INTERFACE));

	busInterface.Size = sizeof(HDAUDIO_BUS_INTERFACE);
	busInterface.Version = 0x0100;
	busInterface.Context = Context;
	busInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
	busInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
	busInterface.TransferCodecVerbs = HDA_TransferCodecVerbs;
	busInterface.AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
	busInterface.AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
	busInterface.ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
	busInterface.AllocateDmaBuffer = HDA_AllocateDmaBuffer;
	busInterface.FreeDmaBuffer = HDA_FreeDmaBuffer;
	busInterface.FreeDmaEngine = HDA_FreeDmaEngine;
	busInterface.SetDmaEngineState = HDA_SetDmaEngineState;
	busInterface.GetWallClockRegister = HDA_GetWallClockRegister;
	busInterface.GetLinkPositionRegister = HDA_GetLinkPositionRegister;
	busInterface.RegisterEventCallback = HDA_RegisterEventCallback;
	busInterface.UnregisterEventCallback = HDA_UnregisterEventCallback;
	busInterface.GetDeviceInformation = HDA_GetDeviceInformation;
	busInterface.GetResourceInformation = HDA_GetResourceInformation;

	return busInterface;
}