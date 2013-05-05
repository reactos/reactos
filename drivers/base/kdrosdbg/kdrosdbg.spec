; Old KD
@ stdcall KdPortGetByte(ptr)
@ stdcall KdPortGetByteEx(ptr ptr)
@ stdcall KdPortInitialize(ptr long long)
@ stdcall KdPortInitializeEx(ptr long long)
@ stdcall KdPortPollByte(ptr)
@ stdcall KdPortPollByteEx(ptr ptr)
@ stdcall KdPortPutByte(long)
@ stdcall KdPortPutByteEx(ptr long)

; New KD
@ stdcall KdD0Transition()
@ stdcall KdD3Transition()
@ stdcall KdDebuggerInitialize0(ptr)
@ stdcall KdDebuggerInitialize1(ptr)
@ stdcall KdReceivePacket(long ptr ptr ptr ptr)
@ stdcall KdRestore(long)
@ stdcall KdSave(long)
@ stdcall KdSendPacket(long ptr ptr ptr)
