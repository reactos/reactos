@ stdcall SmConnectToSm(ptr ptr long ptr)
@ stub SmCreateForeignSession
@ stdcall SmSessionComplete(ptr long long)
@ stub SmTerminateForeignSession
@ stdcall SmExecPgm(ptr ptr long)
@ stdcall SmLoadDeferedSubsystem(ptr ptr)
@ stdcall SmStartCsr(ptr ptr ptr ptr ptr)
@ stdcall SmStopCsr(ptr long)

@ stdcall -version=0x600+ RtlConnectToSm(ptr ptr long ptr) SmConnectToSm
@ stdcall -version=0x600+ RtlSendMsgToSm(ptr ptr) SmSendMsgToSm

## Utilities
@ stdcall SmExecuteProgram(ptr ptr)
@ stdcall SmLookupSubsystem(ptr ptr ptr ptr ptr)
@ stdcall SmQueryInformation(ptr long ptr long ptr)
