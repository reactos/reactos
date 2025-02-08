/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/hdac_controller.h
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

#if !defined(_HDA_CONTROLLER_H_)
#define _HDA_CONTROLLER_H_

//New
NTSTATUS GetHDACapabilities(PFDO_CONTEXT fdoCtx);
NTSTATUS StartHDAController(PFDO_CONTEXT fdoCtx);
NTSTATUS StopHDAController(PFDO_CONTEXT fdoCtx);
NTSTATUS SendHDACmds(PFDO_CONTEXT fdoCtx, ULONG count, PHDAUDIO_CODEC_TRANSFER CodecTransfer);
NTSTATUS RunSingleHDACmd(PFDO_CONTEXT fdoCtx, ULONG val, ULONG* res);

//Old
BOOLEAN
NTAPI
hda_interrupt(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext);

VOID
NTAPI
hda_dpc_queue(
    _In_ PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

VOID NTAPI
hda_dpc_stream(_In_ PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);


VOID
NTAPI
hda_dpc_unsolicited(_In_ PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
#endif
