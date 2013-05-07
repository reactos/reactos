1 stdcall KdD0Transition()
2 stdcall KdD3Transition()
3 stdcall KdDebuggerInitialize0(ptr)
4 stdcall KdDebuggerInitialize1(ptr)
5 stdcall KdReceivePacket(long ptr ptr ptr ptr)
6 stdcall KdRestore(long)
7 stdcall KdSave(long)
8 stdcall KdSendPacket(long ptr ptr ptr)

; Legacy KD
@ stdcall KdPortInitializeEx(ptr long)
@ stdcall KdPortGetByteEx(ptr ptr)
@ stdcall KdPortPutByteEx(ptr long)
