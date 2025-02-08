/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/hdac_stream.cpp
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

#define NDEBUG
#include "driver.h"

VOID
NTAPI
HDA_InterfaceReference(
PVOID BusContext)
{
    DPRINT1("HDA_InterfaceReference\n");
}

VOID
NTAPI
HDA_InterfaceDereference(
PVOID BusContext)
{
    DPRINT1("HDA_InterfaceDereference\n");
}

typedef struct
{
    PIO_WORKITEM WorkItem;
    PPDO_DEVICE_DATA devData;
    PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback;
    PVOID Context;
    PFDO_CONTEXT fdoCtx; 
    ULONG Count;
    PHDAUDIO_CODEC_TRANSFER CodecTransfer;
} TRANSFER_CODEC_CONTEXT, *PTRANSFER_CODEC_CONTEXT;

VOID
NTAPI
WorkerStreamRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PTRANSFER_CODEC_CONTEXT StreamContext = (PTRANSFER_CODEC_CONTEXT)Context;

    UINT16 codecAddr = (UINT16)StreamContext->devData->CodecIds.CodecAddress;

    int timeout_ms = 100 * StreamContext->Count;
    LARGE_INTEGER StartTime;
    KeQuerySystemTime(&StartTime);
    for (ULONG loopcounter = 0;; loopcounter++)
    {
        ULONG TransferredCount = 0;
        for (ULONG i = 0; i < StreamContext->Count; i++)
        {
            if (StreamContext->CodecTransfer[i].Input.IsValid)
            {
                TransferredCount++;
            }
        }
        if (TransferredCount >= StreamContext->Count)
        {
            break;
        }

        LARGE_INTEGER CurrentTime;
        KeQuerySystemTime(&CurrentTime);

        if (((CurrentTime.QuadPart - StartTime.QuadPart) / (10 * 1000)) >= timeout_ms)
        {
            InterlockedExchangeAdd(&StreamContext->fdoCtx->rirb.cmds[codecAddr], (LONG)TransferredCount - StreamContext->Count);
            SklHdAudBusPrint(
                DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s timeout (Count: %d, transferred %d)!\n", __func__, StreamContext->Count,
                TransferredCount);
            IoFreeWorkItem(StreamContext->WorkItem);
            ExFreePoolWithTag(StreamContext, SKLHDAUDBUS_POOL_TAG);
            return;
        }

        LARGE_INTEGER Timeout;
        Timeout.QuadPart = -10 * 100;
        KeWaitForSingleObject(&StreamContext->fdoCtx->rirb.xferEvent[codecAddr], Executive, KernelMode, TRUE, &Timeout);
    }

    // SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s exit (Count: %d)!\n", __func__, Count);
    StreamContext->Callback(StreamContext->CodecTransfer, StreamContext->Context);
    IoFreeWorkItem(StreamContext->WorkItem);
    ExFreePoolWithTag(StreamContext, SKLHDAUDBUS_POOL_TAG);

}

NTSTATUS
NTAPI
HDA_TransferCodecVerbs(
	_In_ PVOID _context,
	_In_ ULONG Count,
	_Inout_updates_(Count)
	PHDAUDIO_CODEC_TRANSFER CodecTransfer,
	_In_opt_ PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback,
	_In_opt_ PVOID Context
) {
    PTRANSFER_CODEC_CONTEXT StreamContext = NULL;
	//SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s called (Count: %d)!\n", __func__, Count);

    //DPRINT("HDA_TransferCodecVerbs %p Count %u CodecTransfer %p Callback %p Context %p\n", _context, Count, CodecTransfer, Callback, Context);

	if (!_context)
    {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
    }

	NTSTATUS status = STATUS_SUCCESS;

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoCtx = devData->FdoContext;

    if (Callback)
    {
        StreamContext = (PTRANSFER_CODEC_CONTEXT)ExAllocatePoolZero(NonPagedPool, sizeof(TRANSFER_CODEC_CONTEXT), SKLHDAUDBUS_POOL_TAG);
        if (StreamContext)
        {
            StreamContext->devData = devData;
            StreamContext->Callback = Callback;
            StreamContext->Context = Context;
            StreamContext->fdoCtx = fdoCtx;
            StreamContext->Count = Count;
            StreamContext->CodecTransfer = CodecTransfer;
            StreamContext->WorkItem = IoAllocateWorkItem(devData->ChildPDO);
            if (!StreamContext->WorkItem)
            {
                ExFreePoolWithTag(StreamContext, SKLHDAUDBUS_POOL_TAG);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

	status = SendHDACmds(fdoCtx, Count, CodecTransfer);
	if (!NT_SUCCESS(status)) {
        DPRINT1("Failed to send with %x\n", status);
		return status;
	}

    if (Callback)
    {
        IoQueueWorkItem(StreamContext->WorkItem, WorkerStreamRoutine, DelayedWorkQueue, StreamContext);
        return STATUS_SUCCESS;
    }

	UINT16 codecAddr = (UINT16)devData->CodecIds.CodecAddress;

	int timeout_ms = 100 * Count;
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
            InterlockedExchangeAdd(&fdoCtx->rirb.cmds[codecAddr], (LONG)TransferredCount - Count); // FIXME
            DPRINT1("Timeout");
			SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s timeout (Count: %d, transferred %d)!\n", __func__, Count, TransferredCount);
			return STATUS_IO_TIMEOUT;
		}

		LARGE_INTEGER Timeout;
		Timeout.QuadPart = -10 * 100;
		KeWaitForSingleObject(&fdoCtx->rirb.xferEvent[codecAddr], Executive, KernelMode, TRUE, &Timeout);
	}

	//SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s exit (Count: %d)!\n", __func__, Count);
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_AllocateCaptureDmaEngine(
	_In_ PVOID _context,
	_In_ UCHAR CodecAddress,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_Out_ PHANDLE Handle,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
	UNREFERENCED_PARAMETER(CodecAddress);
    DPRINT1("HDA_AllocateCaptureDmaEngine %p\n", _context);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoContext = devData->FdoContext;

	KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);
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
        
        KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
		return STATUS_SUCCESS;
	}
    KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
    ASSERT(FALSE);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
HDA_AllocateRenderDmaEngine(
	_In_ PVOID _context,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_In_ BOOLEAN Stripe,
	_Out_ PHANDLE Handle,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
    DPRINT1("HDA_AllocateRenderDmaEngine %p Irql %u\n", _context, KeGetCurrentIrql());
    DPRINT1(
        "HDA_AllocateRenderDmaEngine SampleRate %u ValidBitsPerSample %u ContainerSize %u NumberOfChannels %u\n",
        StreamFormat->SampleRate, StreamFormat->ValidBitsPerSample, StreamFormat->ContainerSize,
        StreamFormat->NumberOfChannels);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PFDO_CONTEXT fdoContext = devData->FdoContext;

	KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

    for (UINT32 i = 0; i < fdoContext->playbackStreams; i++) {
		UINT32 tag = fdoContext->playbackIndexOff + i;
		PHDAC_STREAM stream = &fdoContext->streams[tag];
		if (stream->PdoContext != NULL) {
			continue;
		}

		stream->stripe = Stripe;
		stream->PdoContext = devData;
		stream->running = FALSE;
        RtlCopyMemory(&stream->streamFormat, StreamFormat, sizeof(HDAUDIO_STREAM_FORMAT));
		
		ConverterFormat->ConverterFormat = hdac_format(stream);

		if (Handle)
			*Handle = (HANDLE)stream;

        DPRINT1("Render StreamIndex %u Stripe %u\n", i, Stripe);
        KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
        return STATUS_SUCCESS;
	}
    
    KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
    ASSERT(FALSE);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
HDA_ChangeBandwidthAllocation(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
	_Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat
) {
    DPRINT1("HDA_ChangeBandwidthAllocation %p\n", _context);


	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
        ASSERT(FALSE);
        return STATUS_INVALID_HANDLE;
	}

    KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

	if (stream->running) {
        ASSERT(FALSE);
        KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	stream->streamFormat = *StreamFormat;
	ConverterFormat->ConverterFormat = hdac_format(stream);

	hdac_stream_setup(stream);

    KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
	SklHdAudBusPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL, "%s called!\n", __func__);

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_FreeDmaEngine(
	_In_ PVOID _context,
	_In_ HANDLE Handle)
{
    DPRINT1("HDA_FreeDmaEngine %p\n", _context);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
        ASSERT(FALSE);
        return STATUS_INVALID_HANDLE;
	}

	KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

	if (stream->running) {
        ASSERT(FALSE);
        KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	stream->PdoContext = NULL;
	KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_SetDmaEngineState(
	_In_ PVOID _context,
	_In_ HDAUDIO_STREAM_STATE StreamState,
	_In_ ULONG NumberOfHandles,
	_In_reads_(NumberOfHandles) PHANDLE Handles
) {
    DPRINT1("HDA_SetDmaEngineState %p StreamState %u NumberOfHandles %u\n", _context, StreamState, NumberOfHandles);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}
	for (ULONG i = 0; i < NumberOfHandles; i++) {
		PHDAC_STREAM stream = (PHDAC_STREAM)Handles[i];
		if (stream->PdoContext != devData) {
            ASSERT(FALSE);
			return STATUS_INVALID_HANDLE;
		}

		 KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

        DPRINT1("OldState: %u\n", stream->StreamState);
         if (StreamState == RunState && !stream->running)
         {
            hdac_stream_setup(stream);
            DPRINT1("starting stream\n");
            hdac_stream_start(stream);
            stream->running = TRUE;
            stream->StreamState = RunState;
         }
         else if ((StreamState == PauseState || StreamState == StopState) && stream->running)
         {
            DPRINT1("stopping stream\n");
            hdac_stream_stop(stream);
            stream->running = FALSE;
            stream->StreamState = PauseState;
         }
         else if (StreamState == ResetState)
         {
            if (!stream->running)
            {
                hdac_stream_reset(stream);
                hdac_stream_setup(stream);
            }
            else
            {
                DPRINT1("NO OP\n");
                //return STATUS_INVALID_PARAMETER;
            }
            stream->StreamState = ResetState;
         }
		KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
	}

	return STATUS_SUCCESS;
}

VOID
NTAPI
HDA_GetWallClockRegister(
	_In_ PVOID _context,
	_Out_ PULONG* Wallclock
) {
    DPRINT1("HDA_GetWallClockRegister %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		*Wallclock = NULL;
		return;
	}
	//*Wallclock = (ULONG *)((devData->FdoContext)->m_BAR0.Base.baseptr + HDA_REG_WALLCLK);
    *Wallclock = (ULONG *)((devData->FdoContext)->m_BAR0.Base.baseptr + HDA_REG_WALLCLKA);
}

NTSTATUS
NTAPI
HDA_GetLinkPositionRegister(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_Out_ PULONG* Position
) {
    DPRINT1("HDA_GetLinkPositionRegister %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
		return STATUS_INVALID_HANDLE;
	}

    if (devData->CodecIds.CtlrVenId == VEN_INTEL)
    {
        // Experimental Non-Intel support
        *Position = (ULONG *)(stream->sdAddr + HDA_REG_SD_LPIB);
        DPRINT1("using lpib %p\n", Position);
    }
    else
    {
        DPRINT1("using posBuf %p\n", Position);
        *Position = (ULONG *)stream->posbuf; // Use Posbuf for all Intel
    }

    //*Position = (ULONG *)stream->posbuf;

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_RegisterEventCallback(
	_In_ PVOID _context,
	_In_ PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine,
	_In_opt_ PVOID Context,
	_Out_ PUCHAR Tag
) {
    DPRINT1("HDA_RegisterEventCallback %p\n", _context);

    if (!_context) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
    }

    PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
    KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

	for (UINT8 i = 0; i < MAX_UNSOLICIT_CALLBACKS; i++) {
		if (devData->unsolitCallbacks[i].inUse)
			continue;

		if (Tag)
			*Tag = i;

		devData->unsolitCallbacks[i].inUse = TRUE;
		devData->unsolitCallbacks[i].Context = Context;
		devData->unsolitCallbacks[i].Routine = Routine;

        DPRINT1("HDA_RegisterEventCallback SUCCESS Tag %u\n", i);
		KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
        return STATUS_SUCCESS;
	}

    DPRINT1("HDA_RegisterEventCallback NoResources\n");
	KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);
	return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
HDA_UnregisterEventCallback(
	_In_ PVOID _context,
	_In_ UCHAR Tag
) {
        DPRINT1("HDA_UnregisterEventCallback %p\n", _context);

	if (!_context)
    {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
    }

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->unsolitCallbacks[Tag].inUse)
    {
        ASSERT(FALSE);
        return STATUS_NOT_FOUND;
	}

    KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

	devData->unsolitCallbacks[Tag].Routine = NULL;
	devData->unsolitCallbacks[Tag].Context = NULL;
	devData->unsolitCallbacks[Tag].inUse = FALSE;

    KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_GetDeviceInformation(
	_In_ PVOID _context,
	_Inout_ PHDAUDIO_DEVICE_INFORMATION DeviceInformation
) {
    DPRINT1("HDA_GetDeviceInformation %p\n", _context);

	if (!_context)
    {
        ASSERT(FALSE);
        DPRINT1("NoContext");
        return STATUS_NO_SUCH_DEVICE;
    }

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext)
    {
        ASSERT(FALSE);
        DPRINT1("No FdoContext\n");
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
		DeviceInformationV2->CtrlRevision = devData->CodecIds.RevId;
		DeviceInformationV2->CtrlVendorId = devData->CodecIds.VenId;
		DeviceInformationV2->CtrlDeviceId = devData->CodecIds.DevId;
	}

	return STATUS_SUCCESS;
}

void
NTAPI
HDA_GetResourceInformation(
	_In_ PVOID _context,
	_Out_ PUCHAR CodecAddress,
	_Out_ PUCHAR FunctionGroupStartNode
) {
    DPRINT1("HDA_GetResourceInformation %p\n", _context);

	if (!_context)
		return;
	
	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (CodecAddress)
		*CodecAddress = (UINT8)devData->CodecIds.CodecAddress;
	if (FunctionGroupStartNode)
		*FunctionGroupStartNode = devData->CodecIds.FunctionGroupStartNode;
}

NTSTATUS
NTAPI
HDA_AllocateDmaBufferWithNotification(
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
    DPRINT1("HDA_AllocateDmaBufferWithNotification %p handle %p\n", _context, Handle);
    DPRINT1("HDA_AllocateDmaBufferWithNotification NotificationCount %u RequestedBufferSize %u\n", NotificationCount,
        RequestedBufferSize);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
		return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
        ASSERT(FALSE);
        return STATUS_INVALID_HANDLE;
	}

	if (stream->running) {
        ASSERT(FALSE);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	if (stream->mdlBuf) {
        ASSERT(FALSE);
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
        ASSERT(FALSE);
		return STATUS_NO_MEMORY;
	}

	KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);
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
            DPRINT("bdl %x ioc %u\n", bdl, bdl->ioc);
			bdl++;
			numBlocks++;
			offset += chunk;
			if ((offset % PAGE_SIZE) == 0)
				pageNum++; //Only increment page num if we go past page boundary
		}
		stream->numBlocks = numBlocks;
	}

    KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);

	*FifoSize = stream->fifoSize;
    DPRINT1("HDA_AllocateDmaBufferWithNotification SUCCESS");
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_FreeDmaBufferWithNotification(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PMDL BufferMdl,
	_In_ SIZE_T BufferSize
) {
	UNREFERENCED_PARAMETER(BufferMdl);
	UNREFERENCED_PARAMETER(BufferSize);

    DPRINT1("HDA_FreeDmaBufferWithNotification %p\n", _context);

	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
	}

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
        ASSERT(FALSE);
        return STATUS_INVALID_HANDLE;
	}

	if (stream->running)
    {
        ASSERT(FALSE);
        return STATUS_INVALID_DEVICE_REQUEST;
	}

	if (!stream->mdlBuf)
    {
        ASSERT(FALSE);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

    KIRQL OldLevel = KeAcquireInterruptSpinLock(devData->FdoContext->Interrupt);

	stream_write32(stream, SD_BDLPL, 0);
    if (stream->FdoContext->is64BitOK)
    {
        stream_write32(stream, SD_BDLPU, 0);
    }
	stream_write32(stream, SD_CTL, 0);

	PMDL mdl = stream->mdlBuf;
	stream->mdlBuf = NULL;

	KeReleaseInterruptSpinLock(devData->FdoContext->Interrupt, OldLevel);

	MmFreePagesFromMdl(mdl);
	ExFreePool(mdl);

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_AllocateDmaBuffer(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ SIZE_T RequestedBufferSize,
	_Out_ PMDL* BufferMdl,
	_Out_ PSIZE_T AllocatedBufferSize,
	_Out_ PUCHAR StreamId,
	_Out_ PULONG FifoSize
) {
	SIZE_T OffsetFromFirstPage;
    DPRINT1("HDA_AllocateDmaBuffer %p\n", _context);

	return HDA_AllocateDmaBufferWithNotification(_context, Handle, 1, RequestedBufferSize, BufferMdl, AllocatedBufferSize, &OffsetFromFirstPage, StreamId, FifoSize);
}

NTSTATUS
NTAPI
HDA_FreeDmaBuffer(
	_In_ PVOID _context,
	_In_ HANDLE Handle
) {
    DPRINT1("HDA_FreeDmaBuffer %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData)
    {
        ASSERT(FALSE);
		return STATUS_INVALID_HANDLE;
	}

	return HDA_FreeDmaBufferWithNotification(_context, Handle, stream->mdlBuf, stream->bufSz);
}

NTSTATUS
NTAPI
HDA_RegisterNotificationEvent(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PKEVENT NotificationEvent
) {
    DPRINT1("HDA_RegisterNotificationEvent %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
        return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData)
    {
        ASSERT(FALSE);
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

NTSTATUS
NTAPI
HDA_UnregisterNotificationEvent(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	_In_ PKEVENT NotificationEvent
) {
    DPRINT1("HDA_UnregisterNotificationEvent %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext)
    {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData)
    {
        ASSERT(FALSE);
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

NTSTATUS
NTAPI
HDA_RegisterNotificationCallback(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	PDEVICE_OBJECT Fdo,
	PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
	PVOID CallbackContext
) {
    DPRINT1("HDA_RegisterNotificationCallback %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext)
    {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData)
    {
        ASSERT(FALSE);
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

NTSTATUS
NTAPI
HDA_UnregisterNotificationCallback(
	_In_ PVOID _context,
	_In_ HANDLE Handle,
	PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
	PVOID CallbackContext
) {
    DPRINT1("HDA_UnregisterNotificationCallback %p\n", _context);

	PPDO_DEVICE_DATA devData = (PPDO_DEVICE_DATA)_context;
	if (!devData->FdoContext) {
        ASSERT(FALSE);
		return STATUS_NO_SUCH_DEVICE;
	}

	PHDAC_STREAM stream = (PHDAC_STREAM)Handle;
	if (stream->PdoContext != devData) {
        ASSERT(FALSE);
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

NTSTATUS
NTAPI
HDA_SetupDmaEngineWithBdl(
    IN PVOID _context,
    IN HANDLE Handle,
    IN ULONG BufferLength,
    IN ULONG Lvi,
    IN PHDAUDIO_BDL_ISR Isr,
    IN PVOID Context,
    OUT PUCHAR StreamId,
    OUT PULONG FifoSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_FreeContiguousDmaBuffer(
    IN PVOID _context,
    IN HANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_AllocateContiguousDmaBuffer(
    IN PVOID _context,
    IN HANDLE Handle,
    IN ULONG RequestedBufferSize,
    OUT PVOID* DataBuffer,
    OUT PHDAUDIO_BUFFER_DESCRIPTOR* BdlBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
HDA_BusInterfaceStandard(
    IN PVOID Context,
    PBUS_INTERFACE_STANDARD InterfaceStandard)
{
    PPDO_DEVICE_DATA DeviceExtension;

    /* get device extension */
    DeviceExtension = (PPDO_DEVICE_DATA)Context;

    RtlCopyMemory(InterfaceStandard, &DeviceExtension->FdoContext->BusInterface, sizeof(BUS_INTERFACE_STANDARD));
}

VOID
NTAPI HDA_ReferenceDeviceObject(
    _In_ PVOID Context)
{
    DPRINT1("HDA_ReferenceDeviceObject");
}

VOID
NTAPI
HDA_DereferenceDeviceObject(_In_ PVOID Context)
{
    DPRINT1("HDA_ReferenceDeviceObject");
}

NTSTATUS
NTAPI
HDA_QueryReferenceString(
    _In_ PVOID Context,
    _Inout_ PWCHAR* String)
{
    UNIMPLEMENTED;
    ASSERT(0);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
HDA_BusInterfaceReference(
    PVOID Context,
    PBUS_INTERFACE_REFERENCE busInterface)
{
    RtlZeroMemory(busInterface, sizeof(BUS_INTERFACE_REFERENCE));
    busInterface->Interface.Size = sizeof(BUS_INTERFACE_REFERENCE);
    busInterface->Interface.Version = 0x100;
    busInterface->Interface.InterfaceReference = HDA_InterfaceReference;
    busInterface->Interface.InterfaceDereference = HDA_InterfaceDereference;
    busInterface->ReferenceDeviceObject = HDA_ReferenceDeviceObject;
    busInterface->DereferenceDeviceObject = HDA_DereferenceDeviceObject;
    busInterface->QueryReferenceString = HDA_QueryReferenceString;
}

VOID
HDA_BusInterfaceBDL(
    PVOID Context,
    PHDAUDIO_BUS_INTERFACE_BDL busInterface)
{
    RtlZeroMemory(busInterface, sizeof(HDAUDIO_BUS_INTERFACE_BDL));
    busInterface->Size = sizeof(HDAUDIO_BUS_INTERFACE_BDL);
    busInterface->Version = 0x0100;
    busInterface->Context = Context;
    busInterface->InterfaceReference = HDA_InterfaceReference;
    busInterface->InterfaceDereference = HDA_InterfaceDereference;
    busInterface->TransferCodecVerbs = HDA_TransferCodecVerbs;
    busInterface->AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
    busInterface->AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
    busInterface->ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
    busInterface->AllocateContiguousDmaBuffer = HDA_AllocateContiguousDmaBuffer;
    busInterface->SetupDmaEngineWithBdl = HDA_SetupDmaEngineWithBdl;
    busInterface->FreeContiguousDmaBuffer = HDA_FreeContiguousDmaBuffer;
    busInterface->FreeDmaEngine = HDA_FreeDmaEngine;
    busInterface->SetDmaEngineState = HDA_SetDmaEngineState;
    busInterface->GetWallClockRegister = HDA_GetWallClockRegister;
    busInterface->GetLinkPositionRegister = HDA_GetLinkPositionRegister;
    busInterface->RegisterEventCallback = HDA_RegisterEventCallback;
    busInterface->UnregisterEventCallback = HDA_UnregisterEventCallback;
    busInterface->GetDeviceInformation = HDA_GetDeviceInformation;
    busInterface->GetResourceInformation = HDA_GetResourceInformation;
}	


VOID
HDA_BusInterfaceV2(PVOID Context,PHDAUDIO_BUS_INTERFACE_V2 busInterface)
{

	RtlZeroMemory(busInterface, sizeof(HDAUDIO_BUS_INTERFACE_V2));
	busInterface->Size = sizeof(HDAUDIO_BUS_INTERFACE_V2);
	busInterface->Version = 0x0100;
	busInterface->Context = Context;
	busInterface->InterfaceReference = HDA_InterfaceReference;
	busInterface->InterfaceDereference = HDA_InterfaceDereference;
	busInterface->TransferCodecVerbs = HDA_TransferCodecVerbs;
	busInterface->AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
	busInterface->AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
	busInterface->ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
	busInterface->AllocateDmaBuffer = HDA_AllocateDmaBuffer;
	busInterface->FreeDmaBuffer = HDA_FreeDmaBuffer;
	busInterface->FreeDmaEngine = HDA_FreeDmaEngine;
	busInterface->SetDmaEngineState = HDA_SetDmaEngineState;
	busInterface->GetWallClockRegister = HDA_GetWallClockRegister;
	busInterface->GetLinkPositionRegister = HDA_GetLinkPositionRegister;
	busInterface->RegisterEventCallback = HDA_RegisterEventCallback;
	busInterface->UnregisterEventCallback = HDA_UnregisterEventCallback;
	busInterface->GetDeviceInformation = HDA_GetDeviceInformation;
	busInterface->GetResourceInformation = HDA_GetResourceInformation;
	busInterface->AllocateDmaBufferWithNotification = HDA_AllocateDmaBufferWithNotification;
	busInterface->FreeDmaBufferWithNotification = HDA_FreeDmaBufferWithNotification;
	busInterface->RegisterNotificationEvent = HDA_RegisterNotificationEvent;
	busInterface->UnregisterNotificationEvent = HDA_UnregisterNotificationEvent;
}

HDAUDIO_BUS_INTERFACE_V3 HDA_BusInterfaceV3(PVOID Context) {
	HDAUDIO_BUS_INTERFACE_V3 busInterface;
	RtlZeroMemory(&busInterface, sizeof(HDAUDIO_BUS_INTERFACE_V3));

	busInterface.Size = sizeof(HDAUDIO_BUS_INTERFACE_V3);
	busInterface.Version = 0x0100;
	busInterface.Context = Context;
	busInterface.InterfaceReference = HDA_InterfaceReference;
	busInterface.InterfaceDereference = HDA_InterfaceDereference;
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

VOID
HDA_BusInterface(
    PVOID Context,
    PHDAUDIO_BUS_INTERFACE busInterface)
{
	RtlZeroMemory(busInterface, sizeof(HDAUDIO_BUS_INTERFACE));

	busInterface->Size = sizeof(HDAUDIO_BUS_INTERFACE);
	busInterface->Version = 0x0100;
	busInterface->Context = Context;
	busInterface->InterfaceReference = HDA_InterfaceReference;
	busInterface->InterfaceDereference = HDA_InterfaceDereference;
	busInterface->TransferCodecVerbs = HDA_TransferCodecVerbs;
	busInterface->AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
	busInterface->AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
	busInterface->ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
	busInterface->AllocateDmaBuffer = HDA_AllocateDmaBuffer;
	busInterface->FreeDmaBuffer = HDA_FreeDmaBuffer;
	busInterface->FreeDmaEngine = HDA_FreeDmaEngine;
	busInterface->SetDmaEngineState = HDA_SetDmaEngineState;
	busInterface->GetWallClockRegister = HDA_GetWallClockRegister;
	busInterface->GetLinkPositionRegister = HDA_GetLinkPositionRegister;
	busInterface->RegisterEventCallback = HDA_RegisterEventCallback;
	busInterface->UnregisterEventCallback = HDA_UnregisterEventCallback;
	busInterface->GetDeviceInformation = HDA_GetDeviceInformation;
	busInterface->GetResourceInformation = HDA_GetResourceInformation;
}
