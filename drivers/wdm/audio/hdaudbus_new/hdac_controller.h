#if !defined(_HDA_CONTROLLER_H_)
#define _HDA_CONTROLLER_H_

//New
NTSTATUS GetHDACapabilities(PFDO_CONTEXT fdoCtx);
NTSTATUS StartHDAController(PFDO_CONTEXT fdoCtx);
NTSTATUS StopHDAController(PFDO_CONTEXT fdoCtx);
NTSTATUS SendHDACmds(PFDO_CONTEXT fdoCtx, ULONG count, PHDAUDIO_CODEC_TRANSFER CodecTransfer);
NTSTATUS RunSingleHDACmd(PFDO_CONTEXT fdoCtx, ULONG val, ULONG* res);

//Old
BOOLEAN NTAPI hda_interrupt(WDFINTERRUPT Interrupt, ULONG MessageID);
void NTAPI hda_dpc(WDFINTERRUPT Interrupt, WDFOBJECT AssociatedObject);

#endif
