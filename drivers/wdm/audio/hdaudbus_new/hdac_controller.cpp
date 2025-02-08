/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/hdac_controller.cpp
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */
#include "driver.h"

//New
NTSTATUS ResetHDAController(PFDO_CONTEXT fdoCtx, BOOLEAN wakeup) {
	UINT32 gctl;

	//Clear STATESTS
	hda_write16(fdoCtx, STATESTS, STATESTS_INT_MASK);

	//Stop all Streams DMA Engine
	for (UINT32 i = 0; i < fdoCtx->numStreams; i++) {
		hdac_stream_stop(&fdoCtx->streams[i]);
	}

	//Stop CORB and RIRB
	hda_write8(fdoCtx, CORBCTL, 0);
	hda_write8(fdoCtx, RIRBCTL, 0);

	//Reset DMA position buffer
	hda_write32(fdoCtx, DPLBASE, 0);
    if (fdoCtx->is64BitOK)
    {
        hda_write32(fdoCtx, DPUBASE, 0);
    }

	//Reset the controller for at least 100 us
	gctl = hda_read32(fdoCtx, GCTL);
	hda_write32(fdoCtx, GCTL, gctl & ~HDA_GCTL_RESET);

	for (int count = 0; count < 1000; count++) {
		gctl = hda_read32(fdoCtx, GCTL);
		if (!(gctl & HDA_GCTL_RESET)) {
			break;
		}
		udelay(10);
	}

	if (gctl & HDA_GCTL_RESET) {
		DbgPrint("Error: unable to put controller in reset\n");
		return STATUS_DEVICE_POWER_FAILURE;
	}

	//If wakeup not requested, leave in reset state
	if (!wakeup)
		return STATUS_SUCCESS;

	udelay(100);
	gctl = hda_read32(fdoCtx, GCTL);
	hda_write32(fdoCtx, GCTL, gctl | HDA_GCTL_RESET);

	for (int count = 0; count < 1000; count++) {
		gctl = hda_read32(fdoCtx, GCTL);
		if (gctl & HDA_GCTL_RESET) {
			break;
		}
		udelay(10);
	}
	if (!(gctl & HDA_GCTL_RESET)) {
		DbgPrint("Error: controller stuck in reset\n");
		return STATUS_DEVICE_POWER_FAILURE;
	}

	//Wait for codecs to finish their own reset sequence. Delay from VoodooHDA so it resets properly
	udelay(1000);

	if (!fdoCtx->codecMask) {
		fdoCtx->codecMask = hda_read16(fdoCtx, STATESTS);
		SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
			"codec mask = 0x%lx\n", fdoCtx->codecMask);
	}

	return STATUS_SUCCESS;
}

NTSTATUS GetHDACapabilities(PFDO_CONTEXT fdoCtx) {
	UINT16 gcap = hda_read16(fdoCtx, GCAP);
	SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"chipset global capabilities = 0x%x\n", gcap);

	fdoCtx->is64BitOK = (gcap & 0x1);
    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT, "64 bit OK? %d\n", fdoCtx->is64BitOK);
    fdoCtx->is64BitOK = FALSE; // TODO 64bit support

	fdoCtx->hwVersion = (hda_read8(fdoCtx, VMAJ) << 8) | hda_read8(fdoCtx, VMIN);

	fdoCtx->captureStreams = (gcap >> 8) & 0x0f;
	fdoCtx->playbackStreams = (gcap >> 12) & 0x0f;

	SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"streams (cap %d, playback %d)\n", fdoCtx->captureStreams, fdoCtx->playbackStreams);

	fdoCtx->captureIndexOff = 0;
	fdoCtx->playbackIndexOff = fdoCtx->captureStreams;
	fdoCtx->numStreams = fdoCtx->captureStreams + fdoCtx->playbackStreams;

	UINT8 corbSize = hda_read8(fdoCtx, CORBSIZE);
	if (!(corbSize & 0x40)) {
		return STATUS_DEVICE_FEATURE_NOT_SUPPORTED; //CORB must support 256
	}

	UINT8 rirbSize = hda_read8(fdoCtx, RIRBSIZE);
	if (!(rirbSize & 0x40)) {
		return STATUS_DEVICE_FEATURE_NOT_SUPPORTED; //RIRB must support 256
	}

	return STATUS_SUCCESS;
}

void HDAInitCorb(PFDO_CONTEXT fdoCtx) {
	//Set the corb size to 256 entries
	hda_write8(fdoCtx, CORBSIZE, 0x02);

	//Setup CORB address
	fdoCtx->corb.buf = (UINT32*)fdoCtx->rb;
	fdoCtx->corb.addr = MmGetPhysicalAddress(fdoCtx->corb.buf);
	hda_write32(fdoCtx, CORBLBASE, fdoCtx->corb.addr.LowPart);
	hda_write32(fdoCtx, CORBUBASE, fdoCtx->corb.addr.HighPart);

	//Set WP and RP
	fdoCtx->corb.wp = 0;
	hda_write16(fdoCtx, CORBWP, fdoCtx->corb.wp);
	hda_write16(fdoCtx, CORBRP, HDA_CORBRP_RST);

	udelay(10); //Delay for 10 us to reset

    fdoCtx->corb.rp = 0;
	hda_write16(fdoCtx, CORBRP, 0);
}

void HDAInitRirb(PFDO_CONTEXT fdoCtx) {
	//Set the rirb size to 256 entries
	hda_write8(fdoCtx, RIRBSIZE, 0x02);

	//Setup CORB address
	fdoCtx->rirb.buf = (UINT32*)(fdoCtx->rb + 0x800);
	fdoCtx->rirb.addr = MmGetPhysicalAddress(fdoCtx->rirb.buf);
	RtlZeroMemory(fdoCtx->rirb.cmds, sizeof(fdoCtx->rirb.cmds));
	hda_write32(fdoCtx, RIRBLBASE, fdoCtx->rirb.addr.LowPart);
	hda_write32(fdoCtx, RIRBUBASE, fdoCtx->rirb.addr.HighPart);

	//Set WP and RP
	fdoCtx->rirb.rp = 0;
	hda_write16(fdoCtx, RIRBWP, HDA_RIRBWP_RST);

	//Set interrupt threshold
	hda_write16(fdoCtx, RINTCNT, 1);

	//Enable Received response reporting
	hda_write8(fdoCtx, RIRBCTL, HDA_RBCTL_IRQ_EN);
}

void HDAStartCorb(PFDO_CONTEXT fdoCtx) {
	UINT8 corbCTL;
	corbCTL = hda_read8(fdoCtx, CORBCTL);
	corbCTL |= HDA_CORBCTL_RUN;
	hda_write8(fdoCtx, CORBCTL, corbCTL);
}

void HDAStartRirb(PFDO_CONTEXT fdoCtx) {
	UINT8 rirbCTL;
	rirbCTL = hda_read8(fdoCtx, RIRBCTL);
	rirbCTL |= HDA_RBCTL_DMA_EN;
	hda_write8(fdoCtx, RIRBCTL, rirbCTL);
}

NTSTATUS StartHDAController(PFDO_CONTEXT fdoCtx) {
	NTSTATUS status;
	status = ResetHDAController(fdoCtx, TRUE);
	if (!NT_SUCCESS(status)) {
        return status;
	}

	//Clear STATESTS
	hda_write16(fdoCtx, STATESTS, STATESTS_INT_MASK);

	HDAInitCorb(fdoCtx);
	HDAInitRirb(fdoCtx);

	HDAStartCorb(fdoCtx);
	HDAStartRirb(fdoCtx);

	//Enabling Controller Interrupt
	hda_write32(fdoCtx, GCTL, hda_read32(fdoCtx, GCTL) | HDA_GCTL_UNSOL);
	hda_write32(fdoCtx, INTCTL, hda_read32(fdoCtx, INTCTL) | HDA_INT_CTRL_EN | HDA_INT_GLOBAL_EN);

	//Program position buffer
	PHYSICAL_ADDRESS posbufAddr = MmGetPhysicalAddress(fdoCtx->posbuf);
    hda_write32(fdoCtx, DPLBASE, posbufAddr.LowPart | HDA_DPLBASE_ENABLE);
    if (fdoCtx->is64BitOK)
    {
        hda_write32(fdoCtx, DPUBASE, posbufAddr.HighPart);
    }

	udelay(1000);

	fdoCtx->ControllerEnabled = TRUE;
	return status;
}

NTSTATUS StopHDAController(PFDO_CONTEXT fdoCtx) {
	NTSTATUS status = ResetHDAController(fdoCtx, FALSE);
	fdoCtx->ControllerEnabled = FALSE;
	return status;
}

static UINT16 HDACommandAddr(UINT32 cmd) {
	return (cmd >> 28) & 0xF;
}

NTSTATUS
SendHDACmds(PFDO_CONTEXT fdoCtx, ULONG count, PHDAUDIO_CODEC_TRANSFER CodecTransfer) {

    KIRQL OldLevel = KeAcquireInterruptSpinLock(fdoCtx->Interrupt);
	for (ULONG i = 0; i < count; i++) {
		PHDAUDIO_CODEC_TRANSFER transfer = &CodecTransfer[i];
		RtlZeroMemory(&transfer->Input, sizeof(transfer->Input));

		UINT16 addr = HDACommandAddr(transfer->Output.Command);

		//Add command to corb
		UINT16 wp = hda_read16(fdoCtx, CORBWP);
		if (wp == 0xffff) {
			//Something wrong, controller likely went to sleep
			SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"%s: device not found\n", __func__);
			KeReleaseInterruptSpinLock(fdoCtx->Interrupt, OldLevel);
			return STATUS_DEVICE_DOES_NOT_EXIST;
		}

		wp++;
		wp %= HDA_MAX_CORB_ENTRIES;

		UINT16 rp = hda_read16(fdoCtx, CORBRP);
		if (wp == rp) {
			//Oops it's full
			SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"%s: device busy\n", __func__);	
            KeReleaseInterruptSpinLock (fdoCtx->Interrupt, OldLevel);
			return STATUS_RETRY;
		}

		fdoCtx->rirb.xfer[addr].xfer[fdoCtx->rirb.cmds[addr]] = transfer;
		InterlockedIncrement(&fdoCtx->rirb.cmds[addr]);

		fdoCtx->corb.buf[wp] = transfer->Output.Command;

		hda_write16(fdoCtx, CORBWP, wp);
	}
    KeReleaseInterruptSpinLock (fdoCtx->Interrupt, OldLevel);
	return STATUS_SUCCESS;
}

NTSTATUS RunSingleHDACmd(PFDO_CONTEXT fdoCtx, ULONG val, ULONG* res) {
	HDAUDIO_CODEC_TRANSFER transfer = { 0 };
	transfer.Output.Command = val;
	NTSTATUS status = SendHDACmds(fdoCtx, 1, &transfer);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	LARGE_INTEGER StartTime;
	KeQuerySystemTime(&StartTime);

	int timeout_ms = 1000;
	for (ULONG loopcounter = 0; ; loopcounter++) {
		if (transfer.Input.IsValid) {
			if (res) {
				*res = transfer.Input.Response;
			}
            //DPRINT1("done\n");
			return STATUS_SUCCESS;
		}

		LARGE_INTEGER CurrentTime;
		KeQuerySystemTime(&CurrentTime);

		UINT16 addr = HDACommandAddr(transfer.Output.Command);

		if (((CurrentTime.QuadPart - StartTime.QuadPart) / (10 * 1000)) >= timeout_ms) {
			InterlockedDecrement(&fdoCtx->rirb.cmds[addr]);
            DPRINT1("CurrentTime %u\n", CurrentTime.QuadPart);
			return STATUS_IO_TIMEOUT;
		}
        else
        {
            //DPRINT1("loopcounter %u\n", loopcounter);
        }

		LARGE_INTEGER Timeout;
		Timeout.QuadPart = -10 * 100;
		KeWaitForSingleObject(&fdoCtx->rirb.xferEvent[addr], Executive, KernelMode, TRUE, &Timeout);
	}
    return status;
}

#define HDA_RIRB_EX_UNSOL_EV	(1<<4)

static void HDAProcessUnsolEvents(PFDO_CONTEXT fdoCtx) {
	UINT rp = 0;

    DPRINT1("HDAProcessUnsolEvents entered unsol_rp %u unsol_wp %u\n", fdoCtx->unsol_rp, fdoCtx->unsol_wp);

    while (fdoCtx->unsol_rp != fdoCtx->unsol_wp)
    {
        rp = fdoCtx->unsol_rp;
        HDAC_RIRB rirb = fdoCtx->unsol_queue[rp];

        if (!(rirb.response_ex & HDA_RIRB_EX_UNSOL_EV)) { // no unsolicited event
            rp = (fdoCtx->unsol_rp + 1) % HDA_UNSOL_QUEUE_SIZE;
            fdoCtx->unsol_rp = rp;
            continue;
        }

		HDAUDIO_CODEC_RESPONSE response;
		RtlZeroMemory(&response, sizeof(HDAUDIO_CODEC_RESPONSE));

		response.Response = rirb.response;
		response.IsUnsolicitedResponse = 1;
        response.IsValid = TRUE;

		PPDO_DEVICE_DATA codec = fdoCtx->codecs[rirb.response_ex & 0x0f];
        if (!codec || codec->FdoContext != fdoCtx)
        {
            DPRINT1("No Codec %p %p %p\n", codec, codec->FdoContext, fdoCtx);
            rp = (fdoCtx->unsol_rp + 1) % HDA_UNSOL_QUEUE_SIZE;
            fdoCtx->unsol_rp = rp;
            continue;
        }

		UINT Tag = response.Unsolicited.Tag;
		CODEC_UNSOLIT_CALLBACK callback = codec->unsolitCallbacks[Tag];
		if (callback.inUse && callback.Routine) {
            DPRINT1("Performing callback\n");
			callback.Routine(response, callback.Context);
		}
        else
        {
            DPRINT1("No Callback set %p inUse %u Routine %p\n", callback.inUse, callback.Routine);
        }

        rp = (fdoCtx->unsol_rp + 1) % HDA_UNSOL_QUEUE_SIZE;
        fdoCtx->unsol_rp = rp;
	}
    DPRINT1("ProcessUnsolEvents done\n");
}

static void HDAFlushRIRB(PFDO_CONTEXT fdoCtx) {
	UINT16 wp, addr;

	wp = hda_read16(fdoCtx, RIRBWP);
	if (wp == 0xffff) {
		//Invalid WP
		return;
	}

	if (wp == fdoCtx->rirb.wp)
		return;
	fdoCtx->rirb.wp = wp;

	while (fdoCtx->rirb.rp != wp) {
		fdoCtx->rirb.rp++;
		fdoCtx->rirb.rp %= HDA_MAX_RIRB_ENTRIES;

		HDAC_RIRB rirb = fdoCtx->rirb.rirbbuf[fdoCtx->rirb.rp];

		addr = rirb.response_ex & 0xf;
		if (addr >= HDA_MAX_CODECS) {
            DPRINT1("Unexpected soliticited response\n");
			SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"Unexpected unsolicited response %x: %x\n",
				rirb.response, rirb.response_ex);
		}
		else if (rirb.response_ex & HDA_RIRB_EX_UNSOL_EV) {
            UINT old_unsol_wp = fdoCtx->unsol_wp;
            RtlCopyMemory(&fdoCtx->unsol_queue[fdoCtx->unsol_wp], &rirb, sizeof(rirb));
            UINT unsol_wp = (fdoCtx->unsol_wp + 1) % HDA_UNSOL_QUEUE_SIZE;
            fdoCtx->unsol_wp = unsol_wp;
			fdoCtx->processUnsol = TRUE;
            DPRINT1("Unsolicited response old_unsol_wp %u unsol_wp %u\n", old_unsol_wp, unsol_wp);
		}
		else if (fdoCtx->rirb.cmds[addr]) {
			PHDAC_CODEC_XFER codecXfer = &fdoCtx->rirb.xfer[addr];
			if (codecXfer->xfer[0]) {
				//SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				//	"Got response for 0x%x: 0x%x\n", codecXfer->xfer[0]->Output.Command, rirb.response);
				codecXfer->xfer[0]->Input.Response = rirb.response;
				codecXfer->xfer[0]->Input.IsValid = 1;
			}
			
			RtlMoveMemory(&codecXfer->xfer[0], &codecXfer->xfer[1], sizeof(PHDAUDIO_CODEC_TRANSFER) * (HDA_MAX_CORB_ENTRIES - 1));
			codecXfer->xfer[HDA_MAX_CORB_ENTRIES - 1] = NULL;
			InterlockedDecrement(&fdoCtx->rirb.cmds[addr]);

			KeSetEvent(&fdoCtx->rirb.xferEvent[addr], IO_NO_INCREMENT, FALSE);
		}
		else {
            DPRINT1("Unexpectes solicited response addr %x\n", addr);
			SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"Unexpected unsolicited response from address %d %x\n", addr,
				rirb.response);
		}
	}
}

int
hda_stream_interrupt(PFDO_CONTEXT fdoCtx, unsigned int status) {
	int handled = 0;
	UINT8 sd_status;

	for (UINT32 i = 0; i < fdoCtx->numStreams; i++) {
		PHDAC_STREAM stream = &fdoCtx->streams[i];
		if (status & stream->int_sta_mask) {
			sd_status = stream_read8(stream, SD_STS);
			stream_write8(stream, SD_STS, SD_INT_MASK);
			handled |= 1 << stream->idx;
			if (sd_status & SD_INT_COMPLETE)
            {
                stream->irqReceived = TRUE;
            }
		}
	}
	return handled;
}

BOOLEAN
NTAPI
hda_interrupt(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    PDEVICE_OBJECT DeviceObject;
    PFDO_CONTEXT fdoCtx;
    BOOLEAN handled = FALSE;

    /* get device extension */
    DeviceObject = static_cast<PDEVICE_OBJECT>(ServiceContext);
    fdoCtx = static_cast<PFDO_CONTEXT>(DeviceObject->DeviceExtension);
    ASSERT(fdoCtx->IsFDO == TRUE);

	if (fdoCtx->dspInterruptCallback) {
		handled = (BOOLEAN)fdoCtx->dspInterruptCallback(fdoCtx->dspInterruptContext);
	}

    if (!fdoCtx->ControllerEnabled)
    {
        DPRINT1("Controllernot enabled\n");
        return handled;
    }

	UINT32 status = hda_read32(fdoCtx, INTSTS);
	if (status == 0)
		return handled;

	handled = TRUE;

	if (hda_stream_interrupt(fdoCtx, status)) {
        KeInsertQueueDpc(&fdoCtx->StreamDpc, DeviceObject, NULL);
	}

	status = hda_read8(fdoCtx, RIRBSTS);
	if (status & RIRB_INT_MASK) {
		hda_write8(fdoCtx, RIRBSTS, RIRB_INT_MASK);
		if (status & RIRB_INT_RESPONSE) {
            KeInsertQueueDpc(&fdoCtx->QueueDpc, DeviceObject, NULL);
        }
	}
	return handled;
}

VOID
NTAPI
hda_dpc_stream(PRKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PFDO_CONTEXT fdoCtx = Fdo_GetContext((PDEVICE_OBJECT)DeferredContext);
    for (UINT32 i = 0; i < fdoCtx->numStreams; i++)
    {
        PHDAC_STREAM stream = &fdoCtx->streams[i];
        if (stream->irqReceived)
        {
            stream->irqReceived = FALSE;

            for (int j = 0; j < MAX_NOTIF_EVENTS; j++)
            {
                if (stream->registeredCallbacks[j].InUse)
                {
                    LARGE_INTEGER unknownVal = {0};
                    KeQuerySystemTime(&unknownVal);
                    stream->registeredCallbacks[j].NotificationCallback(
                        stream->registeredCallbacks[j].CallbackContext, unknownVal);
                }
            }

            for (int j = 0; j < MAX_NOTIF_EVENTS; j++)
            {
                if (stream->registeredEvents[j])
                {
                    KeSetEvent(stream->registeredEvents[j], IO_NO_INCREMENT, FALSE);
                }
            }
        }
    }
}



VOID
NTAPI
hda_dpc_queue(
   PRKDPC Dpc,
   PVOID DeferredContext,
   PVOID SystemArgument1,
   PVOID SystemArgument2)
{
	PFDO_CONTEXT fdoCtx = Fdo_GetContext((PDEVICE_OBJECT)DeferredContext);

    HDAFlushRIRB(fdoCtx);

	if (fdoCtx->processUnsol)
    {
        HDAProcessUnsolEvents(fdoCtx);
        fdoCtx->processUnsol = FALSE;
    }
}
