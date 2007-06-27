@ stdcall WTSCloseServer(long)
@ stdcall WTSDisconnectSession(long long long)
@ stdcall WTSEnumerateProcessesA(long long long ptr ptr)
@ stdcall WTSEnumerateProcessesW(long long long ptr ptr)
@ stub WTSEnumerateServersA
@ stub WTSEnumerateServersW
@ stdcall WTSEnumerateSessionsA(long long long ptr ptr)
@ stdcall WTSEnumerateSessionsW(long long long ptr ptr)
@ stdcall WTSFreeMemory(ptr)
@ stub WTSLogoffSession
@ stdcall WTSOpenServerA(ptr)
@ stdcall WTSOpenServerW(ptr)
@ stdcall WTSQuerySessionInformationA(long long long ptr ptr)
@ stdcall WTSQuerySessionInformationW(long long long ptr ptr)
@ stub WTSQueryUserConfigA
@ stub WTSQueryUserConfigW
@ stub WTSSendMessageA
@ stub WTSSendMessageW
@ stub WTSSetSessionInformationA
@ stub WTSSetSessionInformationW
@ stub WTSSetUserConfigA
@ stub WTSSetUserConfigW
@ stub WTSShutdownSystem
@ stub WTSTerminateProcess
@ stub WTSVirtualChannelClose
@ stub WTSVirtualChannelOpen
@ stub WTSVirtualChannelPurgeInput
@ stub WTSVirtualChannelPurgeOutput
@ stub WTSVirtualChannelQuery
@ stub WTSVirtualChannelRead
@ stub WTSVirtualChannelWrite
@ stdcall WTSWaitSystemEvent(long long ptr)
