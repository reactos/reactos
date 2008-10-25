@ stdcall VDMBreakThread(ptr ptr)
@ stdcall VDMDetectWOW()
@ stdcall VDMEnumProcessWOW(ptr ptr)
@ stdcall VDMEnumTaskWOW(long ptr ptr)
@ stdcall VDMEnumTaskWOWEx(long ptr ptr)
;VDMGetAddrExpression
@ stdcall VDMGetContext(ptr ptr ptr)
@ stdcall VDMGetDbgFlags(ptr)
@ stdcall VDMGetModuleSelector(ptr ptr long str ptr)
@ stdcall VDMGetPointer(ptr ptr long long long)
@ stdcall VDMGetSegmentInfo(long long long ptr)
;VDMGetSegtablePointer
@ stdcall VDMGetSelectorModule(ptr ptr long ptr str long str long)
;VDMGetSymbol
;VDMGetThreadContext
;VDMGetThreadSelectorEntry
@ stdcall VDMGlobalFirst(ptr ptr ptr long ptr ptr)
@ stdcall VDMGlobalNext(ptr ptr ptr long ptr ptr)
@ stdcall VDMIsModuleLoaded(str)
@ stdcall VDMKillWOW()
@ stdcall VDMModuleFirst(ptr ptr ptr ptr ptr)
@ stdcall VDMModuleNext(ptr ptr ptr ptr ptr)
@ stdcall VDMProcessException(ptr)
@ stdcall VDMSetContext(ptr ptr ptr)
@ stdcall VDMSetDbgFlags(ptr long)
;VDMSetThreadContext
@ stdcall VDMStartTaskInWOW(long str long)
@ stdcall VDMTerminateTaskWOW(long long)
