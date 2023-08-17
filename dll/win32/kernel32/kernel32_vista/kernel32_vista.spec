
; InitOnce.c
@ stdcall InitOnceBeginInitialize(ptr long ptr ptr)
@ stdcall InitOnceComplete(ptr long ptr)
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall InitOnceInitialize(ptr) ntdll.RtlRunOnceInitialize

; Individual files
@ stdcall GetFileInformationByHandleEx(ptr long ptr long)
@ stdcall -ret64 GetTickCount64()
@ stdcall SetFileInformationByHandle(ptr long ptr long)

; sync.c SRWLock
@ stdcall InitializeSRWLock(ptr)
@ stdcall AcquireSRWLockExclusive(ptr)
@ stdcall AcquireSRWLockShared(ptr)
@ stdcall ReleaseSRWLockExclusive(ptr)
@ stdcall ReleaseSRWLockShared(ptr)
; sync.c ConditionVariable
@ stdcall InitializeConditionVariable(ptr)
@ stdcall SleepConditionVariableCS(ptr ptr long)
@ stdcall SleepConditionVariableSRW(ptr ptr long long)
@ stdcall WakeAllConditionVariable(ptr)
@ stdcall WakeConditionVariable(ptr)
; sync.c CriticalSection
@ stdcall InitializeCriticalSectionEx(ptr long long)

; vista.c
@ stdcall ApplicationRecoveryFinished(long)
@ stdcall ApplicationRecoveryInProgress(ptr)
@ stdcall CreateSymbolicLinkA(str str long)
@ stdcall CreateSymbolicLinkW(wstr wstr long)
@ stdcall GetApplicationRecoveryCallback(ptr ptr ptr ptr ptr)
@ stdcall GetApplicationRestart(ptr wstr ptr ptr)
@ stdcall GetFileBandwidthReservation(ptr ptr ptr ptr ptr ptr)
@ stdcall GetFileMUIInfo(long wstr ptr ptr)
@ stdcall GetFileMUIPath(long wstr wstr ptr wstr ptr ptr)
@ stdcall GetFinalPathNameByHandleA(ptr str long long)
@ stdcall GetFinalPathNameByHandleW(ptr wstr long long)
# @ stdcall -version=0x601+ GetProcessPreferredUILanguages(long ptr wstr ptr)
@ stdcall GetSystemPreferredUILanguages(long ptr wstr ptr)
@ stdcall GetThreadPreferredUILanguages(long ptr wstr ptr)
@ stdcall GetThreadUILanguage()
@ stdcall GetUILanguageInfo(long wstr wstr ptr ptr)
@ stdcall GetUserPreferredUILanguages(long ptr wstr ptr)
@ stdcall OpenFileById(ptr ptr long long ptr long)
@ stdcall QueryFullProcessImageNameA(ptr long str ptr)
@ stdcall QueryFullProcessImageNameW(ptr long wstr ptr)
@ stdcall RegisterApplicationRecoveryCallback(ptr ptr long long)
@ stdcall RegisterApplicationRestart(wstr long)
@ stdcall SetFileBandwidthReservation(ptr long long long ptr ptr)
# @ stdcall -version=0x601+ SetProcessPreferredUILanguages(long wstr ptr)
@ stdcall SetThreadPreferredUILanguages(long wstr ptr)
