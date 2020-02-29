
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr) kernel32.InitOnceExecuteOnce
@ stdcall GetFileInformationByHandleEx(long long ptr long) kernel32.GetFileInformationByHandleEx
@ stdcall -ret64 GetTickCount64() kernel32.GetTickCount64

@ stdcall InitializeSRWLock(ptr) kernel32.InitializeSRWLock
@ stdcall AcquireSRWLockExclusive(ptr) kernel32.AcquireSRWLockExclusive
@ stdcall AcquireSRWLockShared(ptr) kernel32.AcquireSRWLockShared
@ stdcall ReleaseSRWLockExclusive(ptr) kernel32.ReleaseSRWLockExclusive
@ stdcall ReleaseSRWLockShared(ptr) kernel32.ReleaseSRWLockShared

@ stdcall InitializeConditionVariable(ptr) kernel32.InitializeConditionVariable
@ stdcall SleepConditionVariableCS(ptr ptr long) kernel32.SleepConditionVariableCS
@ stdcall SleepConditionVariableSRW(ptr ptr long long) kernel32.SleepConditionVariableSRW
@ stdcall WakeAllConditionVariable(ptr) kernel32.WakeAllConditionVariable
@ stdcall WakeConditionVariable(ptr) kernel32.WakeConditionVariable

@ stdcall InitializeCriticalSectionEx(ptr long long) kernel32.InitializeCriticalSectionEx
