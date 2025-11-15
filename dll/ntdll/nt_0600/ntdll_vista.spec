@ stdcall LdrRegisterDllNotification(long ptr ptr ptr)
@ stdcall LdrUnregisterDllNotification(ptr)

@ stdcall RtlInitializeConditionVariable(ptr)
@ stdcall RtlWakeConditionVariable(ptr)
@ stdcall RtlWakeAllConditionVariable(ptr)
@ stdcall RtlSleepConditionVariableCS(ptr ptr ptr)
@ stdcall RtlSleepConditionVariableSRW(ptr ptr ptr long)
@ stdcall RtlInitializeSRWLock(ptr)
@ stdcall RtlAcquireSRWLockShared(ptr)
@ stdcall RtlReleaseSRWLockShared(ptr)
@ stdcall RtlAcquireSRWLockExclusive(ptr)
@ stdcall RtlReleaseSRWLockExclusive(ptr)
@ stdcall RtlRunOnceInitialize(ptr)
@ stdcall RtlRunOnceBeginInitialize(ptr long ptr)
@ stdcall RtlRunOnceComplete(ptr long ptr)
@ stdcall RtlRunOnceExecuteOnce(ptr ptr ptr ptr)

@ stdcall RtlConnectToSm(ptr ptr long ptr) SmConnectToSm
@ stdcall RtlSendMsgToSm(ptr ptr) SmSendMsgToSm
@ stdcall RtlLcidToLocaleName(long ptr long long)
@ stdcall RtlLocaleNameToLcid(wstr ptr long)
@ stdcall RtlCompareUnicodeStrings(wstr long wstr long long)
