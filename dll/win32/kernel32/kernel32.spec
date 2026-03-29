@ stdcall -version=0x600+ AcquireSRWLockExclusive(ptr) ntdll.RtlAcquireSRWLockExclusive
@ stdcall -version=0x600+ AcquireSRWLockShared(ptr) ntdll.RtlAcquireSRWLockShared
@ stdcall ActivateActCtx(ptr ptr) kernelbase.ActivateActCtx
@ stdcall AddAtomA(str)
@ stdcall AddAtomW(wstr)
@ stdcall AddConsoleAliasA(str str str) kernelbase.AddConsoleAliasA
@ stdcall AddConsoleAliasW(wstr wstr wstr) kernelbase.AddConsoleAliasW
@ stdcall AddDllDirectory(wstr) kernelbase.AddDllDirectory
# @ stub -version=0x601+ AddIntegrityLabelToBoundaryDescriptor
@ stdcall AddLocalAlternateComputerNameA(str ptr)
@ stdcall AddLocalAlternateComputerNameW(wstr ptr)
@ stdcall AddRefActCtx(ptr) kernelbase.AddRefActCtx
@ stdcall -stub -version=0x600+ AddSecureMemoryCacheCallback(ptr)
@ stdcall -stub -version=0x600+ AddSIDToBoundaryDescriptor(ptr ptr)
@ stdcall AddVectoredContinueHandler(long ptr) ntdll.RtlAddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) ntdll.RtlAddVectoredExceptionHandler
@ stdcall -stub -version=0x600+ AdjustCalendarDate(ptr long long)
@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernelbase.AllocateUserPhysicalPages
@ stdcall -version=0x600+ AllocateUserPhysicalPagesNuma(long ptr ptr long) kernelbase.AllocateUserPhysicalPagesNuma
@ stdcall AllocConsole() kernelbase.AllocConsole
@ stdcall -version=0x600+ ApplicationRecoveryFinished(long)
@ stdcall -version=0x600+ ApplicationRecoveryInProgress(ptr)
@ stdcall -version=0xA00+ AppPolicyGetMediaFoundationCodecLoading(ptr ptr) kernelbase.AppPolicyGetMediaFoundationCodecLoading
@ stdcall -version=0xA00+ AppPolicyGetWindowingModel(ptr ptr) kernelbase.AppPolicyGetWindowingModel
@ stdcall AreFileApisANSI() kernelbase.AreFileApisANSI
@ stdcall AssignProcessToJobObject(ptr ptr)
@ stdcall AttachConsole(long) kernelbase.AttachConsole
@ stdcall BackupRead(ptr ptr long ptr long long ptr)
@ stdcall BackupSeek(ptr long long ptr ptr ptr)
@ stdcall BackupWrite(ptr ptr long ptr long long ptr)
@ stdcall BaseCheckAppcompatCache(wstr ptr wstr ptr)
@ stdcall BaseCheckRunApp(long ptr long long long long long long long long)
@ stdcall BaseCleanupAppcompatCacheSupport(ptr)
@ stdcall BaseDumpAppcompatCache()
@ stdcall BaseFlushAppcompatCache()
@ stub -version=0x600+ BaseGenerateAppCompatData
@ stdcall BaseInitAppcompatCacheSupport()
@ stdcall BaseIsAppcompatInfrastructureDisabled() IsShimInfrastructureDisabled
@ stdcall -version=0x501-0x502 BaseProcessInitPostImport()
@ stdcall -version=0x600+ BaseProcessInitPostImport() # HACK: This export is dynamicaly imported by ntdll
;@ stdcall -version=0x502 -arch=x86_64 BaseProcessStart()
@ stdcall BaseQueryModuleData(str str ptr ptr ptr) ;check
@ stub -version=0x600+ BaseThreadInitThunk
;@ stdcall -version=0x502 -arch=x86_64 BaseThreadStart()
@ stdcall BaseUpdateAppcompatCache(long long long)
@ stdcall BasepCheckBadapp(long ptr long long long long long long long)
@ stdcall BasepCheckWinSaferRestrictions(long long long long long long)
@ stdcall BasepFreeAppCompatData(ptr ptr)
@ stdcall Beep(long long)
@ stdcall BeginUpdateResourceA(str long)
@ stdcall BeginUpdateResourceW(wstr long)
@ stdcall BindIoCompletionCallback(long ptr long)
@ stdcall BuildCommDCBA(str ptr)
@ stdcall BuildCommDCBAndTimeoutsA(str ptr ptr)
@ stdcall BuildCommDCBAndTimeoutsW(wstr ptr ptr)
@ stdcall BuildCommDCBW(wstr ptr)
@ stdcall -version=0x600+ CallbackMayRunLong(ptr) kernelbase.CallbackMayRunLong
@ stdcall CallNamedPipeA(str ptr long ptr long ptr long)
@ stdcall CallNamedPipeW(wstr ptr long ptr long ptr long) kernelbase.CallNamedPipeW
@ stdcall CancelDeviceWakeupRequest(long)
@ stdcall CancelIo(long) kernelbase.CancelIo
@ stdcall -version=0x600+ CancelIoEx(long ptr) kernelbase.CancelIoEx
@ stdcall -version=0x600+ CancelSynchronousIo(long) kernelbase.CancelSynchronousIo
@ stdcall -version=0x600+ CancelThreadpoolIo(ptr) ntdll.TpCancelAsyncIoOperation
@ stdcall CancelTimerQueueTimer(ptr ptr)
@ stdcall CancelWaitableTimer(long) kernelbase.CancelWaitableTimer
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernelbase.ChangeTimerQueueTimer
@ stdcall -stub -version=0x600+ CheckElevation(ptr ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ CheckElevationEnabled(ptr)
# @ stub CheckForReadOnlyResource
@ stdcall CheckNameLegalDOS8Dot3A(str ptr long ptr ptr)
@ stdcall CheckNameLegalDOS8Dot3W(wstr ptr long ptr ptr)
@ stdcall CheckRemoteDebuggerPresent(long ptr) kernelbase.CheckRemoteDebuggerPresent
@ stdcall ClearCommBreak(long) kernelbase.ClearCommBreak
@ stdcall ClearCommError(long ptr ptr) kernelbase.ClearCommError
@ stdcall CloseConsoleHandle(long)
@ stdcall CloseHandle(long) kernelbase.CloseHandle
@ stdcall -stub -version=0x600+ ClosePrivateNamespace(ptr long)
@ stdcall CloseProfileUserMapping()
@ stdcall -version=0xA00+ ClosePseudoConsole(ptr) kernelbase.ClosePseudoConsole
@ stdcall -version=0x600+ CloseThreadpool(ptr) ntdll.TpReleasePool
@ stdcall -version=0x600+ CloseThreadpoolCleanupGroup(ptr) ntdll.TpReleaseCleanupGroup
@ stdcall -version=0x600+ CloseThreadpoolCleanupGroupMembers(ptr long ptr) ntdll.TpReleaseCleanupGroupMembers
@ stdcall -version=0x600+ CloseThreadpoolIo(ptr) ntdll.TpReleaseIoCompletion
@ stdcall -version=0x600+ CloseThreadpoolTimer(ptr) ntdll.TpReleaseTimer
@ stdcall -version=0x600+ CloseThreadpoolWait(ptr) ntdll.TpReleaseWait
@ stdcall -version=0x600+ CloseThreadpoolWork(ptr) ntdll.TpReleaseWork
@ stdcall CmdBatNotification(long)
@ stdcall CommConfigDialogA(str long ptr)
@ stdcall CommConfigDialogW(wstr long ptr)
@ stdcall -stub -version=0x600+ CompareCalendarDates(ptr ptr ptr)
@ stdcall CompareFileTime(ptr ptr) kernelbase.CompareFileTime
@ stdcall CompareStringA(long long str long str long) kernelbase.CompareStringA
@ stdcall -version=0x600+ CompareStringEx(wstr long wstr long wstr long ptr ptr long) kernelbase.CompareStringEx
@ stdcall -version=0x600+ CompareStringOrdinal(wstr long wstr long long) kernelbase.CompareStringOrdinal
@ stdcall CompareStringW(long long wstr long wstr long) kernelbase.CompareStringW
@ stdcall ConnectNamedPipe(long ptr) kernelbase.ConnectNamedPipe
@ stdcall ConsoleMenuControl(long long long)
@ stdcall ContinueDebugEvent(long long long) kernelbase.ContinueDebugEvent
@ stdcall -stub -version=0x600+ ConvertCalDateTimeToSystemTime(ptr ptr)
@ stdcall ConvertDefaultLocale(long)
@ stdcall ConvertFiberToThread() kernelbase.ConvertFiberToThread
@ stdcall -stub -version=0x600+ ConvertNLSDayOfWeekToWin32DayOfWeek(long)
@ stdcall -stub -version=0x600+ ConvertSystemTimeToCalDateTime(ptr long ptr)
@ stdcall ConvertThreadToFiber(ptr) kernelbase.ConvertThreadToFiber
@ stdcall ConvertThreadToFiberEx(ptr long) kernelbase.ConvertThreadToFiberEx
@ stdcall ConvertToGlobalHandle(long)
@ stdcall CopyContext(ptr long ptr) kernelbase.CopyContext
@ stdcall -version=0x602+ CopyFile2(wstr wstr ptr) kernelbase.CopyFile2
@ stdcall CopyFileA(str str long)
@ stdcall CopyFileExA(str str ptr ptr ptr long)
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernelbase.CopyFileExW
@ stdcall -stub -version=0x600+ CopyFileTransactedA(str str ptr ptr ptr long ptr)
@ stdcall -stub -version=0x600+ CopyFileTransactedW(wstr wstr ptr ptr ptr long ptr)
@ stdcall CopyFileW(wstr wstr long) kernelbase.CopyFileW
@ stdcall CopyLZFile(long long) LZCopy
@ stdcall CreateActCtxA(ptr)
@ stdcall CreateActCtxW(ptr) kernelbase.CreateActCtxW
@ stdcall -stub -version=0x600+ CreateBoundaryDescriptorA(str long)
@ stdcall -version=0x600+ CreateBoundaryDescriptorW(wstr long) kernelbase.CreateBoundaryDescriptorW
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr) kernelbase.CreateConsoleScreenBuffer
@ stdcall CreateDirectoryA(str ptr) kernelbase.CreateDirectoryA
@ stdcall CreateDirectoryExA(str str ptr)
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernelbase.CreateDirectoryExW
@ stdcall -version=0x600+ CreateDirectoryTransactedA(str str ptr ptr)
@ stdcall -version=0x600+ CreateDirectoryTransactedW(wstr wstr ptr ptr)
@ stdcall CreateDirectoryW(wstr ptr) kernelbase.CreateDirectoryW
@ stdcall CreateEventA(ptr long long str) kernelbase.CreateEventA
@ stdcall -version=0x600+ CreateEventExA(ptr str long long) kernelbase.CreateEventExA
@ stdcall -version=0x600+ CreateEventExW(ptr wstr long long) kernelbase.CreateEventExW
@ stdcall CreateEventW(ptr long long wstr) kernelbase.CreateEventW
@ stdcall CreateFiber(long ptr ptr) kernelbase.CreateFiber
@ stdcall CreateFiberEx(long long long ptr ptr) kernelbase.CreateFiberEx
@ stdcall -version=0x602+ CreateFile2(wstr long long long ptr) kernelbase.CreateFile2
@ stdcall CreateFileA(str long long ptr long long long) kernelbase.CreateFileA
@ stdcall CreateFileMappingA(long ptr long long long str)
@ stdcall -version=0x602+ CreateFileMappingFromApp(long ptr long int64 wstr) kernelbase.CreateFileMappingFromApp
@ stdcall -stub -version=0x600+ CreateFileMappingNumaA(ptr ptr long long long str long)
@ stdcall -version=0x600+ CreateFileMappingNumaW(long ptr long long long wstr long) kernelbase.CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stdcall -version=0x600+ CreateFileTransactedA(str long long ptr long long long ptr ptr ptr)
@ stdcall -version=0x600+ CreateFileTransactedW(wstr long long ptr long long long ptr ptr ptr)
@ stdcall CreateFileW(wstr long long ptr long long long) kernelbase.CreateFileW
@ stdcall CreateHardLinkA(str str ptr) kernelbase.CreateHardLinkA
@ stdcall -version=0x600+ CreateHardLinkTransactedA(str str ptr ptr)
@ stdcall -version=0x600+ CreateHardLinkTransactedW(wstr wstr ptr ptr)
@ stdcall CreateHardLinkW(wstr wstr ptr) kernelbase.CreateHardLinkW
@ stdcall CreateIoCompletionPort(long long long long) kernelbase.CreateIoCompletionPort
@ stdcall CreateJobObjectA(ptr str)
@ stdcall CreateJobObjectW(ptr wstr)
@ stdcall CreateJobSet(long ptr long)
@ stdcall CreateMailslotA(str long long ptr)
@ stdcall CreateMailslotW(wstr long long ptr)
@ stdcall CreateMemoryResourceNotification(long) kernelbase.CreateMemoryResourceNotification
@ stdcall CreateMutexA(ptr long str) kernelbase.CreateMutexA
@ stdcall -version=0x600+ CreateMutexExA(ptr str long long) kernelbase.CreateMutexExA
@ stdcall -version=0x600+ CreateMutexExW(ptr wstr long long) kernelbase.CreateMutexExW
@ stdcall CreateMutexW(ptr long wstr) kernelbase.CreateMutexW
@ stdcall CreateNamedPipeA(str long long long long long long ptr)
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernelbase.CreateNamedPipeW
@ stdcall -version=0x501-0x502 CreateNlsSecurityDescriptor(ptr long long)
@ stdcall CreatePipe(ptr ptr ptr long) kernelbase.CreatePipe
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceA(ptr ptr str)
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceW(ptr ptr wstr)
@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernelbase.CreateProcessA
@ stdcall -version=0xA00+ CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr) kernelbase.CreateProcessAsUserA
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernelbase.CreateProcessAsUserW
@ stdcall CreateProcessInternalA(long str str ptr ptr long long ptr str ptr ptr ptr) kernelbase.CreateProcessInternalA
@ stdcall CreateProcessInternalW(long wstr wstr ptr ptr long long ptr wstr ptr ptr ptr) kernelbase.CreateProcessInternalW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernelbase.CreateProcessW
@ stdcall -version=0xA00+ CreatePseudoConsole(long long long long ptr) kernelbase.CreatePseudoConsole
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernelbase.CreateRemoteThread
@ stdcall CreateRemoteThreadEx(long ptr long ptr ptr long ptr ptr) kernelbase.CreateRemoteThreadEx
@ stdcall CreateSemaphoreA(ptr long long str)
@ stdcall -version=0x600+ CreateSemaphoreExA(ptr long long str long long)
@ stdcall -version=0x600+ CreateSemaphoreExW(ptr long long wstr long long) kernelbase.CreateSemaphoreExW
@ stdcall CreateSemaphoreW(ptr long long wstr) kernelbase.CreateSemaphoreW
@ stdcall CreateSocketHandle()
@ stdcall -version=0x600+ CreateSymbolicLinkA(str str long)
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedA(str str long ptr)
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedW(wstr wstr long ptr)
@ stdcall -version=0x600+ CreateSymbolicLinkW(wstr wstr long) kernelbase.CreateSymbolicLinkW
@ stdcall CreateTapePartition(long long long long)
@ stdcall CreateThread(ptr long ptr long long ptr) kernelbase.CreateThread
@ stdcall -version=0x600+ CreateThreadpool(ptr) kernelbase.CreateThreadpool
@ stdcall -version=0x600+ CreateThreadpoolCleanupGroup() kernelbase.CreateThreadpoolCleanupGroup
@ stdcall -version=0x600+ CreateThreadpoolIo(ptr ptr ptr ptr) kernelbase.CreateThreadpoolIo
@ stdcall -version=0x600+ CreateThreadpoolTimer(ptr ptr ptr) kernelbase.CreateThreadpoolTimer
@ stdcall -version=0x600+ CreateThreadpoolWait(ptr ptr ptr) kernelbase.CreateThreadpoolWait
@ stdcall -version=0x600+ CreateThreadpoolWork(ptr ptr ptr) kernelbase.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernelbase.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernelbase.CreateTimerQueueTimer
@ stdcall CreateToolhelp32Snapshot(long long)
@ stdcall -version=0x601+ -arch=win64 CreateUmsCompletionList(ptr)
@ stdcall -version=0x601+ -arch=win64 CreateUmsThreadContext(ptr)
@ stdcall CreateWaitableTimerA(ptr long str)
@ stdcall -version=0x600+ CreateWaitableTimerExA(ptr str long long)
@ stdcall -version=0x600+ CreateWaitableTimerExW(ptr wstr long long) kernelbase.CreateWaitableTimerExW
@ stdcall CreateWaitableTimerW(ptr long wstr) kernelbase.CreateWaitableTimerW
@ stdcall CtrlRoutine(ptr) kernelbase.CtrlRoutine
@ stdcall DeactivateActCtx(long long) kernelbase.DeactivateActCtx
@ stdcall DebugActiveProcess(long) kernelbase.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernelbase.DebugActiveProcessStop
@ stdcall DebugBreak()
@ stdcall DebugBreakProcess(long)
@ stdcall DebugSetProcessKillOnExit(long)
@ stdcall DecodePointer(ptr) ntdll.RtlDecodePointer
@ stdcall DecodeSystemPointer(ptr) ntdll.RtlDecodeSystemPointer
@ stdcall DefineDosDeviceA(long str str)
@ stdcall DefineDosDeviceW(long wstr wstr) kernelbase.DefineDosDeviceW
@ stdcall DelayLoadFailureHook(str str) kernelbase.DelayLoadFailureHook
@ stdcall DeleteAtom(long)
# @ stub DeleteBoundaryDescriptor
@ stdcall DeleteCriticalSection(ptr) ntdll.RtlDeleteCriticalSection
@ stdcall DeleteFiber(ptr) kernelbase.DeleteFiber
@ stdcall DeleteFileA(str) kernelbase.DeleteFileA
@ stdcall -version=0x600+ DeleteFileTransactedA(str ptr)
@ stdcall -version=0x600+ DeleteFileTransactedW(wstr ptr)
@ stdcall DeleteFileW(wstr) kernelbase.DeleteFileW
@ stdcall -version=0x600+ DeleteProcThreadAttributeList(ptr) kernelbase.DeleteProcThreadAttributeList
@ stdcall DeleteTimerQueue(long)
@ stdcall DeleteTimerQueueEx(long long) kernelbase.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernelbase.DeleteTimerQueueTimer
@ stdcall -version=0x601+ -arch=win64 DeleteUmsCompletionList(ptr)
@ stdcall -version=0x601+ -arch=win64 DeleteUmsThreadContext(ptr)
@ stdcall DeleteVolumeMountPointA(str)
@ stdcall DeleteVolumeMountPointW(wstr) kernelbase.DeleteVolumeMountPointW
@ stdcall -version=0x601+ -arch=win64 DequeueUmsCompletionListItems(ptr long ptr)
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) KERNEL32_DeviceIoControl
@ stdcall DisableThreadLibraryCalls(long) kernelbase.DisableThreadLibraryCalls
# @ stub DisableThreadProfiling
@ stdcall -version=0x600+ DisassociateCurrentThreadFromCallback(ptr) ntdll.TpDisassociateCallback
@ stdcall -version=0xA00+ DiscardVirtualMemory(ptr long) kernelbase.DiscardVirtualMemory
@ stdcall DisconnectNamedPipe(long) kernelbase.DisconnectNamedPipe
@ stdcall DnsHostnameToComputerNameA(str ptr ptr)
@ stdcall DnsHostnameToComputerNameW(wstr ptr ptr)
@ stdcall DosDateTimeToFileTime(long long ptr)
@ stdcall DosPathToSessionPathA(long str str)
@ stdcall DosPathToSessionPathW(long wstr wstr)
@ stdcall DuplicateConsoleHandle(long long long long)
@ stdcall DuplicateHandle(long long long ptr long long long) kernelbase.DuplicateHandle
# @ stub EnableThreadProfiling
@ stdcall EncodePointer(ptr) ntdll.RtlEncodePointer
@ stdcall EncodeSystemPointer(ptr) ntdll.RtlEncodeSystemPointer
@ stdcall EndUpdateResourceA(long long)
@ stdcall EndUpdateResourceW(long long)
@ stdcall EnterCriticalSection(ptr) ntdll.RtlEnterCriticalSection
@ stdcall -version=0x601+ -arch=win64 EnterUmsSchedulingMode(ptr)
@ stdcall EnumCalendarInfoA(ptr long long long)
@ stdcall EnumCalendarInfoExA(ptr long long long)
@ stdcall -version=0x600+ EnumCalendarInfoExEx(ptr wstr long wstr long long) kernelbase.EnumCalendarInfoExEx
@ stdcall EnumCalendarInfoExW(ptr long long long) kernelbase.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernelbase.EnumCalendarInfoW
@ stdcall EnumDateFormatsA(ptr long long)
@ stdcall EnumDateFormatsExA(ptr long long)
@ stdcall -version=0x600+ EnumDateFormatsExEx(ptr wstr long long) kernelbase.EnumDateFormatsExEx
@ stdcall EnumDateFormatsExW(ptr long long) kernelbase.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernelbase.EnumDateFormatsW
@ stdcall EnumerateLocalComputerNamesA(ptr long str ptr)
@ stdcall EnumerateLocalComputerNamesW(ptr long wstr ptr)
@ stdcall EnumLanguageGroupLocalesA(ptr long long ptr)
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr) kernelbase.EnumLanguageGroupLocalesW
@ stdcall EnumResourceLanguagesA(long str str ptr long)
@ stdcall -version=0x600+ EnumResourceLanguagesExA(long str str ptr long long long) kernelbase.EnumResourceLanguagesExA
@ stdcall -version=0x600+ EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernelbase.EnumResourceLanguagesExW
@ stdcall EnumResourceLanguagesW(long wstr wstr ptr long)
@ stdcall EnumResourceNamesA(long str ptr long)
@ stdcall -version=0x600+ EnumResourceNamesExA(long str ptr long long long) kernelbase.EnumResourceNamesExA
@ stdcall -version=0x600+ EnumResourceNamesExW(long wstr ptr long long long) kernelbase.EnumResourceNamesExW
@ stdcall EnumResourceNamesW(long wstr ptr long) kernelbase.EnumResourceNamesW
@ stdcall EnumResourceTypesA(long ptr long)
@ stdcall -version=0x600+ EnumResourceTypesExA(long ptr long long long) kernelbase.EnumResourceTypesExA
@ stdcall -version=0x600+ EnumResourceTypesExW(long ptr long long long) kernelbase.EnumResourceTypesExW
@ stdcall EnumResourceTypesW(long ptr long)
@ stdcall EnumSystemCodePagesA(ptr long)
@ stdcall EnumSystemCodePagesW(ptr long) kernelbase.EnumSystemCodePagesW
@ stdcall EnumSystemFirmwareTables(long ptr long) kernelbase.EnumSystemFirmwareTables
@ stdcall EnumSystemGeoID(long long ptr) kernelbase.EnumSystemGeoID
@ stdcall EnumSystemLanguageGroupsA(ptr long ptr)
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr) kernelbase.EnumSystemLanguageGroupsW
@ stdcall EnumSystemLocalesA(ptr long) kernelbase.EnumSystemLocalesA
@ stdcall -version=0x600+ EnumSystemLocalesEx(ptr long long ptr) kernelbase.EnumSystemLocalesEx
@ stdcall EnumSystemLocalesW(ptr long) kernelbase.EnumSystemLocalesW
@ stdcall EnumTimeFormatsA(ptr long long)
@ stdcall -version=0x600+ EnumTimeFormatsEx(ptr wstr long long) kernelbase.EnumTimeFormatsEx
@ stdcall EnumTimeFormatsW(ptr long long) kernelbase.EnumTimeFormatsW
@ stdcall EnumUILanguagesA(ptr long long)
@ stdcall EnumUILanguagesW(ptr long long) kernelbase.EnumUILanguagesW
@ stdcall EraseTape(ptr long long)
@ stdcall EscapeCommFunction(long long) kernelbase.EscapeCommFunction
@ stdcall -version=0x601+ -arch=win64 ExecuteUmsThread(ptr)
@ stdcall ExitProcess(long)
@ stdcall ExitThread(long) ntdll.RtlExitUserThread
@ stdcall ExitVDM(long long)
@ stdcall ExpandEnvironmentStringsA(str ptr long) kernelbase.ExpandEnvironmentStringsA
@ stdcall ExpandEnvironmentStringsW(wstr ptr long) kernelbase.ExpandEnvironmentStringsW
@ stdcall ExpungeConsoleCommandHistoryA(str) kernelbase.ExpungeConsoleCommandHistoryA
@ stdcall ExpungeConsoleCommandHistoryW(wstr) kernelbase.ExpungeConsoleCommandHistoryW
@ stdcall FatalAppExitA(long str) kernelbase.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernelbase.FatalAppExitW
@ stdcall FatalExit(long)
@ stdcall FileTimeToDosDateTime(ptr ptr ptr)
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernelbase.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernelbase.FileTimeToSystemTime
@ stdcall FillConsoleOutputAttribute(long long long long ptr) kernelbase.FillConsoleOutputAttribute
@ stdcall FillConsoleOutputCharacterA(long long long long ptr) kernelbase.FillConsoleOutputCharacterA
@ stdcall FillConsoleOutputCharacterW(long long long long ptr) kernelbase.FillConsoleOutputCharacterW
@ stdcall FindActCtxSectionGuid(long ptr long ptr ptr) kernelbase.FindActCtxSectionGuid
@ stdcall FindActCtxSectionStringA(long ptr long str ptr)
@ stdcall FindActCtxSectionStringW(long ptr long wstr ptr) kernelbase.FindActCtxSectionStringW
@ stdcall FindAtomA(str)
@ stdcall FindAtomW(wstr)
@ stdcall FindClose(long) kernelbase.FindClose
@ stdcall FindCloseChangeNotification(long) kernelbase.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernelbase.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernelbase.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernelbase.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernelbase.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernelbase.FindFirstFileExW
# @ stub FindFirstFileNameTransactedW
@ stdcall -version=0x600+ FindFirstFileNameW(wstr long ptr ptr) kernelbase.FindFirstFileNameW
@ stdcall -version=0x600+ FindFirstFileTransactedA(str long ptr long ptr long ptr)
@ stdcall -version=0x600+ FindFirstFileTransactedW(wstr long ptr long ptr long ptr)
@ stdcall FindFirstFileW(wstr ptr) kernelbase.FindFirstFileW
# @ stub FindFirstStreamTransactedW
@ stdcall FindFirstStreamW(wstr long ptr long) kernelbase.FindFirstStreamW
@ stdcall FindFirstVolumeA(ptr long)
@ stdcall FindFirstVolumeMountPointA(str ptr long)
@ stdcall FindFirstVolumeMountPointW(wstr ptr long)
@ stdcall FindFirstVolumeW(ptr long) kernelbase.FindFirstVolumeW
@ stdcall FindNextChangeNotification(long) kernelbase.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernelbase.FindNextFileA
# @ stub FindNextFileNameW
@ stdcall FindNextFileW(long ptr) kernelbase.FindNextFileW
@ stdcall FindNextStreamW(long ptr) kernelbase.FindNextStreamW
@ stdcall FindNextVolumeA(long ptr long)
@ stdcall FindNextVolumeMountPointA(long str long)
@ stdcall FindNextVolumeMountPointW(long wstr long)
@ stdcall FindNextVolumeW(long ptr long) kernelbase.FindNextVolumeW
@ stdcall -version=0x600+ FindNLSString(long long wstr long wstr long ptr) kernelbase.FindNLSString
@ stdcall -version=0x600+ FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernelbase.FindNLSStringEx
@ stdcall FindResourceA(long str str)
@ stdcall FindResourceExA(long str str long)
@ stdcall FindResourceExW(long wstr wstr long) kernelbase.FindResourceExW
@ stdcall FindResourceW(long wstr wstr) kernelbase.FindResourceW
@ stdcall FindStringOrdinal(long wstr long wstr long long) kernelbase.FindStringOrdinal
@ stdcall FindVolumeClose(ptr) kernelbase.FindVolumeClose
@ stdcall FindVolumeMountPointClose(ptr)
@ stdcall FlsAlloc(ptr) kernelbase.FlsAlloc
@ stdcall FlsFree(long) kernelbase.FlsFree
@ stdcall FlsGetValue(long) kernelbase.FlsGetValue
@ stdcall FlsSetValue(long ptr) kernelbase.FlsSetValue
@ stdcall FlushConsoleInputBuffer(long) kernelbase.FlushConsoleInputBuffer
@ stdcall FlushFileBuffers(long) kernelbase.FlushFileBuffers
@ stdcall FlushInstructionCache(long long long) kernelbase.FlushInstructionCache
@ stdcall -version=0x600+ FlushProcessWriteBuffers() ntdll.NtFlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long) kernelbase.FlushViewOfFile
@ stdcall FoldStringA(long str long ptr long)
@ stdcall FoldStringW(long wstr long ptr long) kernelbase.FoldStringW
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernelbase.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernelbase.FormatMessageW
@ stdcall FreeConsole() kernelbase.FreeConsole
@ stdcall FreeEnvironmentStringsA(ptr) kernelbase.FreeEnvironmentStringsA
@ stdcall FreeEnvironmentStringsW(ptr) kernelbase.FreeEnvironmentStringsW
@ stdcall FreeLibrary(long) kernelbase.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long)
@ stdcall -version=0x600+ FreeLibraryWhenCallbackReturns(ptr ptr) ntdll.TpCallbackUnloadDllOnCompletion
@ stdcall FreeResource(long) kernelbase.FreeResource
@ stdcall FreeUserPhysicalPages(long ptr ptr) kernelbase.FreeUserPhysicalPages
@ stdcall GenerateConsoleCtrlEvent(long long) kernelbase.GenerateConsoleCtrlEvent
@ stdcall GetACP() kernelbase.GetACP
@ stdcall GetActiveProcessorCount(long)
@ stdcall GetActiveProcessorGroupCount()
@ stdcall -version=0x600+ GetApplicationRecoveryCallback(ptr ptr ptr ptr ptr)
@ stdcall -version=0x600+ GetApplicationRestartSettings(long ptr ptr ptr) kernelbase.GetApplicationRestartSettings
@ stdcall GetAtomNameA(long ptr long)
@ stdcall GetAtomNameW(long ptr long)
@ stdcall GetBinaryType(str ptr) GetBinaryTypeA
@ stdcall GetBinaryTypeA(str ptr)
@ stdcall GetBinaryTypeW(wstr ptr)
# @ stub GetCalendarDateFormat
# @ stub GetCalendarDateFormatEx
# @ stub GetCalendarDaysInMonth
# @ stub GetCalendarDifferenceInDays
@ stdcall GetCalendarInfoA(long long long ptr long ptr)
@ stdcall -version=0x600+ GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernelbase.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernelbase.GetCalendarInfoW
# @ stub GetCalendarMonthsInYear
# @ stub GetCalendarSupportedDateRange
# @ stub GetCalendarWeekNumber
@ stdcall GetCommandLineA() kernelbase.GetCommandLineA
@ stdcall GetCommandLineW() kernelbase.GetCommandLineW
@ stdcall GetCommConfig(long ptr ptr) kernelbase.GetCommConfig
@ stdcall GetCommMask(long ptr) kernelbase.GetCommMask
@ stdcall GetCommModemStatus(long ptr) kernelbase.GetCommModemStatus
@ stdcall GetCommProperties(long ptr) kernelbase.GetCommProperties
@ stdcall GetCommState(long ptr) kernelbase.GetCommState
@ stdcall GetCommTimeouts(long ptr) kernelbase.GetCommTimeouts
@ stdcall GetComPlusPackageInstallStatus()
@ stdcall GetCompressedFileSizeA(str ptr) kernelbase.GetCompressedFileSizeA
# @ stub GetCompressedFileSizeTransactedA
# @ stub GetCompressedFileSizeTransactedW
@ stdcall GetCompressedFileSizeW(wstr ptr) kernelbase.GetCompressedFileSizeW
@ stdcall GetComputerNameA(ptr ptr)
@ stdcall GetComputerNameExA(long ptr ptr)
@ stdcall GetComputerNameExW(long ptr ptr) kernelbase.GetComputerNameExW
@ stdcall GetComputerNameW(ptr ptr)
@ stdcall GetConsoleAliasA(str ptr long str) kernelbase.GetConsoleAliasA
@ stdcall GetConsoleAliasesA(str long str)
@ stdcall GetConsoleAliasesLengthA(str) kernelbase.GetConsoleAliasesLengthA
@ stdcall GetConsoleAliasesLengthW(wstr) kernelbase.GetConsoleAliasesLengthW
@ stdcall GetConsoleAliasesW(wstr long wstr)
@ stdcall GetConsoleAliasExesA(str long)
@ stdcall GetConsoleAliasExesLengthA() kernelbase.GetConsoleAliasExesLengthA
@ stdcall GetConsoleAliasExesLengthW() kernelbase.GetConsoleAliasExesLengthW
@ stdcall GetConsoleAliasExesW(wstr long)
@ stdcall GetConsoleAliasW(wstr ptr long wstr) kernelbase.GetConsoleAliasW
@ stdcall GetConsoleCharType(long long ptr)
@ stdcall GetConsoleCommandHistoryA(ptr long str) kernelbase.GetConsoleCommandHistoryA
@ stdcall GetConsoleCommandHistoryLengthA(str) kernelbase.GetConsoleCommandHistoryLengthA
@ stdcall GetConsoleCommandHistoryLengthW(wstr) kernelbase.GetConsoleCommandHistoryLengthW
@ stdcall GetConsoleCommandHistoryW(ptr long wstr) kernelbase.GetConsoleCommandHistoryW
@ stdcall GetConsoleCP() kernelbase.GetConsoleCP
@ stdcall GetConsoleCursorInfo(long ptr) kernelbase.GetConsoleCursorInfo
@ stdcall GetConsoleCursorMode(long ptr ptr)
@ stdcall GetConsoleDisplayMode(ptr) kernelbase.GetConsoleDisplayMode
@ stdcall GetConsoleFontInfo(ptr long long ptr)
@ stdcall GetConsoleFontSize(long long) kernelbase.GetConsoleFontSize
@ stdcall GetConsoleHardwareState(long long ptr)
@ stdcall -version=0x600+ GetConsoleHistoryInfo(ptr)
@ stdcall GetConsoleInputExeNameA(long ptr) kernelbase.GetConsoleInputExeNameA
@ stdcall GetConsoleInputExeNameW(long ptr) kernelbase.GetConsoleInputExeNameW
@ stdcall GetConsoleInputWaitHandle()
@ stdcall GetConsoleKeyboardLayoutNameA(ptr)
@ stdcall GetConsoleKeyboardLayoutNameW(ptr)
@ stdcall GetConsoleMode(long ptr) kernelbase.GetConsoleMode
@ stdcall GetConsoleNlsMode(long ptr)
@ stdcall -version=0x600+ GetConsoleOriginalTitleA(ptr long) kernelbase.GetConsoleOriginalTitleA
@ stdcall -version=0x600+ GetConsoleOriginalTitleW(ptr long) kernelbase.GetConsoleOriginalTitleW
@ stdcall GetConsoleOutputCP() kernelbase.GetConsoleOutputCP
@ stdcall GetConsoleProcessList(ptr long) kernelbase.GetConsoleProcessList
@ stdcall GetConsoleScreenBufferInfo(long ptr) kernelbase.GetConsoleScreenBufferInfo
@ stdcall -version=0x600+ GetConsoleScreenBufferInfoEx(long ptr) kernelbase.GetConsoleScreenBufferInfoEx
@ stdcall GetConsoleSelectionInfo(ptr)
@ stdcall GetConsoleTitleA(ptr long) kernelbase.GetConsoleTitleA
@ stdcall GetConsoleTitleW(ptr long) kernelbase.GetConsoleTitleW
@ stdcall GetConsoleWindow() kernelbase.GetConsoleWindow
@ stdcall -version=0x501-0x600 GetCPFileNameFromRegistry(long wstr long)
@ stdcall GetCPInfo(long ptr) kernelbase.GetCPInfo
@ stdcall GetCPInfoExA(long long ptr)
@ stdcall GetCPInfoExW(long long ptr) kernelbase.GetCPInfoExW
@ stdcall GetCurrencyFormatA(long long str ptr ptr long)
@ stdcall -version=0x600+ GetCurrencyFormatEx(wstr long wstr ptr ptr long) kernelbase.GetCurrencyFormatEx
@ stdcall GetCurrencyFormatW(long long wstr ptr ptr long) kernelbase.GetCurrencyFormatW
@ stdcall GetCurrentActCtx(ptr) kernelbase.GetCurrentActCtx
@ stdcall GetCurrentConsoleFont(long long ptr) kernelbase.GetCurrentConsoleFont
@ stdcall -version=0x600+ GetCurrentConsoleFontEx(long long ptr) kernelbase.GetCurrentConsoleFontEx
@ stdcall GetCurrentDirectoryA(long ptr) kernelbase.GetCurrentDirectoryA
@ stdcall GetCurrentDirectoryW(long ptr) kernelbase.GetCurrentDirectoryW
@ stdcall -version=0x602+ GetCurrentPackageFamilyName(ptr ptr) kernelbase.GetCurrentPackageFamilyName
@ stdcall -version=0x602+ GetCurrentPackageFullName(ptr ptr) kernelbase.GetCurrentPackageFullName
@ stdcall -version=0x602+ GetCurrentPackageId(ptr ptr) kernelbase.GetCurrentPackageId
@ stdcall -version=0x602+ GetCurrentPackageInfo(long ptr ptr ptr) kernelbase.GetCurrentPackageInfo
@ stdcall -version=0x602+ GetCurrentPackagePath(ptr ptr) kernelbase.GetCurrentPackagePath
@ stdcall -norelay GetCurrentProcess() KERNEL32_GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() KERNEL32_GetCurrentProcessId
@ stdcall GetCurrentProcessorNumber() ntdll.NtGetCurrentProcessorNumber
@ stdcall GetCurrentProcessorNumberEx(ptr) ntdll.RtlGetCurrentProcessorNumberEx
@ stdcall -norelay GetCurrentThread() KERNEL32_GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() KERNEL32_GetCurrentThreadId
@ stdcall -version=0x602+ GetCurrentThreadStackLimits(ptr ptr) kernelbase.GetCurrentThreadStackLimits
@ stdcall -version=0x601+ -arch=win64 GetCurrentUmsThread()
@ stdcall GetDateFormatA(long long ptr str ptr long) kernelbase.GetDateFormatA
@ stdcall -version=0x600+ GetDateFormatEx(wstr long ptr wstr ptr long wstr) kernelbase.GetDateFormatEx
@ stdcall GetDateFormatW(long long ptr wstr ptr long) kernelbase.GetDateFormatW
@ stdcall GetDefaultCommConfigA(str ptr ptr)
@ stdcall GetDefaultCommConfigW(wstr ptr ptr)
@ stdcall -version=0x501-0x502 GetDefaultSortkeySize(ptr)
@ stdcall GetDevicePowerState(long ptr)
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr)
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr)
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernelbase.GetDiskFreeSpaceW
@ stdcall GetDllDirectoryA(long ptr)
@ stdcall GetDllDirectoryW(long ptr)
@ stdcall GetDriveTypeA(str) kernelbase.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernelbase.GetDriveTypeW
# @ stub GetDurationFormat
# @ stub GetDurationFormatEx
@ stdcall -version=0x600+ GetDynamicTimeZoneInformation(ptr) kernelbase.GetDynamicTimeZoneInformation
@ stdcall -version=0x602+ GetDynamicTimeZoneInformationEffectiveYears(ptr ptr ptr) kernelbase.GetDynamicTimeZoneInformationEffectiveYears
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernelbase.GetEnabledXStateFeatures
@ stdcall GetEnvironmentStrings() kernelbase.GetEnvironmentStrings
@ stdcall GetEnvironmentStringsA() kernelbase.GetEnvironmentStringsA
@ stdcall GetEnvironmentStringsW() kernelbase.GetEnvironmentStringsW
@ stdcall GetEnvironmentVariableA(str ptr long) kernelbase.GetEnvironmentVariableA
@ stdcall GetEnvironmentVariableW(wstr ptr long) kernelbase.GetEnvironmentVariableW
# @ stub GetEraNameCountedString
@ stdcall -version=0x600+ GetErrorMode() kernelbase.GetErrorMode
@ stdcall GetExitCodeProcess(long ptr)
@ stdcall GetExitCodeThread(long ptr) kernelbase.GetExitCodeThread
@ stdcall GetExpandedNameA(str ptr)
@ stdcall GetExpandedNameW(wstr ptr)
@ stdcall GetFileAttributesA(str) kernelbase.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernelbase.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernelbase.GetFileAttributesExW
@ stdcall -version=0x600+ GetFileAttributesTransactedA(str long ptr ptr)
@ stdcall -version=0x600+ GetFileAttributesTransactedW(wstr long ptr ptr)
@ stdcall GetFileAttributesW(wstr) kernelbase.GetFileAttributesW
@ stdcall -version=0x600+ GetFileBandwidthReservation(ptr ptr ptr ptr ptr ptr)
@ stdcall GetFileInformationByHandle(long ptr) kernelbase.GetFileInformationByHandle
@ stdcall -version=0x600+ GetFileInformationByHandleEx(long long ptr long) kernelbase.GetFileInformationByHandleEx
@ stdcall -version=0x600+ GetFileMUIInfo(long wstr ptr ptr) kernelbase.GetFileMUIInfo
@ stdcall -version=0x600+ GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernelbase.GetFileMUIPath
@ stdcall GetFileSize(long ptr) kernelbase.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernelbase.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernelbase.GetFileTime
@ stdcall GetFileType(long) kernelbase.GetFileType
@ stdcall -version=0x600+ GetFinalPathNameByHandleA(long ptr long long) kernelbase.GetFinalPathNameByHandleA
@ stdcall -version=0x600+ GetFinalPathNameByHandleW(long ptr long long) kernelbase.GetFinalPathNameByHandleW
@ stdcall GetFirmwareEnvironmentVariableA(str str ptr long)
@ stdcall GetFirmwareEnvironmentVariableW(wstr wstr ptr long)
@ stdcall -version=0x602+ GetFirmwareType(ptr)
@ stdcall GetFullPathNameA(str long ptr ptr) kernelbase.GetFullPathNameA
# @ stub GetFullPathNameTransactedA
# @ stub GetFullPathNameTransactedW
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernelbase.GetFullPathNameW
@ stdcall GetGeoInfoA(long long ptr long long)
@ stdcall -version=0xA00+ GetGeoInfoEx(ptr long ptr long) kernelbase.GetGeoInfoEx
@ stdcall GetGeoInfoW(long long ptr long long) kernelbase.GetGeoInfoW
@ stdcall GetHandleContext(long)
@ stdcall GetHandleInformation(long ptr) kernelbase.GetHandleInformation
@ stdcall GetLargePageMinimum() kernelbase.GetLargePageMinimum
@ stdcall GetLargestConsoleWindowSize(long) kernelbase.GetLargestConsoleWindowSize
@ stdcall GetLastError() kernelbase.GetLastError
@ stdcall -version=0x500-0x502 GetLinguistLangSize(ptr)
@ stdcall GetLocaleInfoA(long long ptr long) kernelbase.GetLocaleInfoA
@ stdcall -version=0x600+ GetLocaleInfoEx(wstr long ptr long) kernelbase.GetLocaleInfoEx
@ stdcall GetLocaleInfoW(long long ptr long) kernelbase.GetLocaleInfoW
@ stdcall GetLocalTime(ptr) kernelbase.GetLocalTime
@ stdcall GetLogicalDrives() kernelbase.GetLogicalDrives
@ stdcall GetLogicalDriveStringsA(long ptr)
@ stdcall GetLogicalDriveStringsW(long ptr) kernelbase.GetLogicalDriveStringsW
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernelbase.GetLogicalProcessorInformation
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr) kernelbase.GetLogicalProcessorInformationEx
@ stdcall GetLongPathNameA(str ptr long) kernelbase.GetLongPathNameA
# @ stub GetLongPathNameTransactedA
# @ stub GetLongPathNameTransactedW
@ stdcall GetLongPathNameW(wstr ptr long) kernelbase.GetLongPathNameW
@ stdcall GetMailslotInfo(long ptr ptr ptr ptr)
@ stdcall GetMaximumProcessorCount(long)
@ stdcall GetMaximumProcessorGroupCount()
@ stdcall GetModuleFileNameA(long ptr long) kernelbase.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernelbase.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernelbase.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernelbase.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernelbase.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernelbase.GetModuleHandleW
# @ stub GetNamedPipeAttribute
# @ stub GetNamedPipeClientComputerNameA
# @ stub GetNamedPipeClientComputerNameW
@ stdcall -version=0x600+ GetNamedPipeClientProcessId(long ptr)
@ stdcall -version=0x600+ GetNamedPipeClientSessionId(long ptr)
@ stdcall GetNamedPipeHandleStateA(long ptr ptr ptr ptr ptr long)
@ stdcall GetNamedPipeHandleStateW(long ptr ptr ptr ptr ptr long) kernelbase.GetNamedPipeHandleStateW
@ stdcall GetNamedPipeInfo(long ptr ptr ptr ptr) kernelbase.GetNamedPipeInfo
@ stdcall -version=0x600+ GetNamedPipeServerProcessId(long ptr)
@ stdcall -version=0x600+ GetNamedPipeServerSessionId(long ptr)
@ stdcall GetNativeSystemInfo(ptr) kernelbase.GetNativeSystemInfo
@ stdcall -version=0x601+ -arch=win64 GetNextUmsListItem(ptr)
@ stdcall GetNextVDMCommand(long)
@ stdcall -version=0x500-0x502 GetNlsSectionName(long long long str str long)
@ stdcall GetNLSVersion(long long ptr) kernelbase.GetNLSVersion
@ stdcall GetNLSVersionEx(long wstr ptr) kernelbase.GetNLSVersionEx
@ stdcall GetNumaAvailableMemoryNode(long ptr)
@ stdcall GetNumaAvailableMemoryNodeEx(long ptr)
@ stdcall GetNumaHighestNodeNumber(ptr) kernelbase.GetNumaHighestNodeNumber
# @ stub GetNumaNodeNumberFromHandle
@ stdcall GetNumaNodeProcessorMask(long ptr)
@ stdcall GetNumaNodeProcessorMaskEx(long ptr) kernelbase.GetNumaNodeProcessorMaskEx
@ stdcall GetNumaProcessorNode(long ptr)
@ stdcall GetNumaProcessorNodeEx(ptr ptr)
@ stdcall -version=0x600+ GetNumaProximityNode(long ptr)
@ stdcall GetNumaProximityNodeEx(long ptr) kernelbase.GetNumaProximityNodeEx
@ stdcall GetNumberFormatA(long long str ptr ptr long)
@ stdcall -version=0x600+ GetNumberFormatEx(wstr long wstr ptr ptr long) kernelbase.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernelbase.GetNumberFormatW
@ stdcall GetNumberOfConsoleFonts()
@ stdcall GetNumberOfConsoleInputEvents(long ptr) kernelbase.GetNumberOfConsoleInputEvents
@ stdcall GetNumberOfConsoleMouseButtons(ptr) kernelbase.GetNumberOfConsoleMouseButtons
@ stdcall GetOEMCP() kernelbase.GetOEMCP
@ stdcall GetOverlappedResult(long ptr ptr long) kernelbase.GetOverlappedResult
@ stdcall -version=0x602+ GetOverlappedResultEx(long ptr ptr long long) kernelbase.GetOverlappedResultEx
@ stdcall -version=0x602+ GetPackageFamilyName(long ptr ptr) kernelbase.GetPackageFamilyName
@ stdcall -version=0x602+ GetPackageFullName(long ptr ptr) kernelbase.GetPackageFullName
@ stdcall -version=0x603+ GetPackagePathByFullName(wstr ptr wstr) kernelbase.GetPackagePathByFullName
@ stdcall -version=0x602+ GetPackagesByPackageFamily(wstr ptr ptr ptr ptr) kernelbase.GetPackagesByPackageFamily
@ stdcall -version=0x600+ GetPhysicallyInstalledSystemMemory(ptr) kernelbase.GetPhysicallyInstalledSystemMemory
@ stdcall GetPriorityClass(long) kernelbase.GetPriorityClass
@ stdcall GetPrivateProfileIntA(str str long str)
@ stdcall GetPrivateProfileIntW(wstr wstr long wstr)
@ stdcall GetPrivateProfileSectionA(str ptr long str)
@ stdcall GetPrivateProfileSectionNamesA(ptr long str)
@ stdcall GetPrivateProfileSectionNamesW(ptr long wstr)
@ stdcall GetPrivateProfileSectionW(wstr ptr long wstr)
@ stdcall GetPrivateProfileStringA(str str str ptr long str)
@ stdcall GetPrivateProfileStringW(wstr wstr wstr ptr long wstr)
@ stdcall GetPrivateProfileStructA(str str ptr long str)
@ stdcall GetPrivateProfileStructW(wstr wstr ptr long wstr)
@ stdcall GetProcAddress(long str)
@ stdcall GetProcessAffinityMask(long ptr ptr)
@ stdcall -version=0x600+ GetProcessDEPPolicy(long ptr ptr)
@ stdcall GetProcessGroupAffinity(long ptr ptr) kernelbase.GetProcessGroupAffinity
@ stdcall GetProcessHandleCount(long ptr) kernelbase.GetProcessHandleCount
@ stdcall GetProcessHeap() kernelbase.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) RtlGetProcessHeaps
@ stdcall GetProcessId(long) kernelbase.GetProcessId
@ stdcall GetProcessIdOfThread(long) kernelbase.GetProcessIdOfThread
@ stdcall -version=0x602+ GetProcessInformation(long long ptr long)
@ stdcall GetProcessIoCounters(long ptr)
@ stdcall -version=0x602+ GetProcessMitigationPolicy(long long ptr long) kernelbase.GetProcessMitigationPolicy
# @ stub GetProcessorSystemCycleTime
@ stdcall GetProcessPreferredUILanguages(long ptr ptr ptr) kernelbase.GetProcessPreferredUILanguages
@ stdcall GetProcessPriorityBoost(long ptr) kernelbase.GetProcessPriorityBoost
@ stdcall GetProcessShutdownParameters(ptr ptr) kernelbase.GetProcessShutdownParameters
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernelbase.GetProcessTimes
# @ stub GetProcessUserModeExceptionPolicy
@ stdcall GetProcessVersion(long) kernelbase.GetProcessVersion
@ stdcall GetProcessWorkingSetSize(long ptr ptr)
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr) kernelbase.GetProcessWorkingSetSizeEx
@ stdcall -version=0x600+ GetProductInfo(long long long long ptr) kernelbase.GetProductInfo
@ stdcall GetProfileIntA(str str long)
@ stdcall GetProfileIntW(wstr wstr long)
@ stdcall GetProfileSectionA(str ptr long)
@ stdcall GetProfileSectionW(wstr ptr long)
@ stdcall GetProfileStringA(str str str ptr long)
@ stdcall GetProfileStringW(wstr wstr wstr ptr long)
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernelbase.GetQueuedCompletionStatus
@ stdcall -version=0x600+ GetQueuedCompletionStatusEx(ptr ptr long ptr long long) kernelbase.GetQueuedCompletionStatusEx
@ stdcall GetShortPathNameA(str ptr long)
@ stdcall GetShortPathNameW(wstr ptr long) kernelbase.GetShortPathNameW
@ stdcall GetStartupInfoA(ptr)
@ stdcall GetStartupInfoW(ptr) kernelbase.GetStartupInfoW
@ stdcall GetStdHandle(long) kernelbase.GetStdHandle
# @ stub GetStringScripts
@ stdcall GetStringTypeA(long long str long ptr) kernelbase.GetStringTypeA
@ stdcall GetStringTypeExA(long long str long ptr)
@ stdcall GetStringTypeExW(long long wstr long ptr) kernelbase.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernelbase.GetStringTypeW
@ stdcall -version=0xA00+ GetSystemCpuSetInformation(ptr long ptr ptr long) kernelbase.GetSystemCpuSetInformation
@ stdcall GetSystemDefaultLangID() kernelbase.GetSystemDefaultLangID
@ stdcall GetSystemDefaultLCID() kernelbase.GetSystemDefaultLCID
@ stdcall -version=0x600+ GetSystemDefaultLocaleName(ptr long) kernelbase.GetSystemDefaultLocaleName
@ stdcall GetSystemDefaultUILanguage() kernelbase.GetSystemDefaultUILanguage
@ stdcall -version=0x600+ GetSystemDEPPolicy()
@ stdcall GetSystemDirectoryA(ptr long)
@ stdcall GetSystemDirectoryW(ptr long)
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernelbase.GetSystemFileCacheSize
@ stdcall GetSystemFirmwareTable(long long ptr long) kernelbase.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernelbase.GetSystemInfo
@ stdcall GetSystemPowerStatus(ptr)
@ stdcall -version=0x600+ GetSystemPreferredUILanguages(long ptr ptr ptr) kernelbase.GetSystemPreferredUILanguages
@ stdcall GetSystemRegistryQuota(ptr ptr)
@ stdcall GetSystemTime(ptr) kernelbase.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernelbase.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernelbase.GetSystemTimeAsFileTime
@ stdcall -version=0x602+ GetSystemTimePreciseAsFileTime(ptr) kernelbase.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemTimes(ptr ptr ptr) kernelbase.GetSystemTimes
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernelbase.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernelbase.GetSystemWindowsDirectoryW
@ stdcall GetSystemWow64DirectoryA(ptr long) kernelbase.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernelbase.GetSystemWow64DirectoryW
@ stdcall GetTapeParameters(ptr long ptr ptr)
@ stdcall GetTapePosition(ptr long ptr ptr ptr)
@ stdcall GetTapeStatus(ptr)
@ stdcall GetTempFileNameA(str str long ptr) kernelbase.GetTempFileNameA
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernelbase.GetTempFileNameW
@ stdcall -version=0xA00+ GetTempPath2A(long ptr) kernelbase.GetTempPath2A
@ stdcall -version=0xA00+ GetTempPath2W(long ptr) kernelbase.GetTempPath2W
@ stdcall GetTempPathA(long ptr) kernelbase.GetTempPathA
@ stdcall GetTempPathW(long ptr) kernelbase.GetTempPathW
@ stdcall GetThreadContext(long ptr) kernelbase.GetThreadContext
@ stdcall -version=0xA00+ GetThreadDescription(long ptr) kernelbase.GetThreadDescription
@ stdcall -version=0x600+ GetThreadErrorMode() kernelbase.GetThreadErrorMode
@ stdcall GetThreadGroupAffinity(long ptr) kernelbase.GetThreadGroupAffinity
@ stdcall GetThreadId(ptr) kernelbase.GetThreadId
@ stdcall GetThreadIdealProcessorEx(long ptr) kernelbase.GetThreadIdealProcessorEx
@ stdcall GetThreadIOPendingFlag(long ptr) kernelbase.GetThreadIOPendingFlag
@ stdcall GetThreadLocale() kernelbase.GetThreadLocale
@ stdcall -version=0x600+ GetThreadPreferredUILanguages(long ptr ptr ptr) kernelbase.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long) kernelbase.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernelbase.GetThreadPriorityBoost
@ stdcall GetThreadSelectorEntry(long long ptr)
@ stdcall GetThreadTimes(long ptr ptr ptr ptr) kernelbase.GetThreadTimes
@ stdcall -version=0x600+ GetThreadUILanguage() kernelbase.GetThreadUILanguage
@ stdcall GetTickCount()
@ stdcall -ret64 -version=0x600+ GetTickCount64()
@ stdcall GetTimeFormatA(long long ptr str ptr long) kernelbase.GetTimeFormatA
@ stdcall -version=0x600+ GetTimeFormatEx(wstr long ptr wstr ptr long) kernelbase.GetTimeFormatEx
@ stdcall GetTimeFormatW(long long ptr wstr ptr long) kernelbase.GetTimeFormatW
@ stdcall GetTimeZoneInformation(ptr) kernelbase.GetTimeZoneInformation
@ stdcall -version=0x600+ GetTimeZoneInformationForYear(long ptr ptr) kernelbase.GetTimeZoneInformationForYear
@ stdcall -version=0x600+ GetUILanguageInfo(long wstr wstr ptr ptr)
@ stdcall -version=0x601+ -arch=win64 GetUmsCompletionListEvent(ptr ptr)
# @ stub -version=0x601+ -arch=win64 GetUmsSystemThreadInformation
@ stdcall -version=0xA00+ GetUserDefaultGeoName(ptr long) kernelbase.GetUserDefaultGeoName
@ stdcall GetUserDefaultLangID() kernelbase.GetUserDefaultLangID
@ stdcall GetUserDefaultLCID() kernelbase.GetUserDefaultLCID
@ stdcall -version=0x600+ GetUserDefaultLocaleName(ptr long) kernelbase.GetUserDefaultLocaleName
@ stdcall GetUserDefaultUILanguage() kernelbase.GetUserDefaultUILanguage
@ stdcall GetUserGeoID(long) kernelbase.GetUserGeoID
@ stdcall -version=0x600+ GetUserPreferredUILanguages(long ptr ptr ptr) kernelbase.GetUserPreferredUILanguages
@ stdcall GetVDMCurrentDirectories(long long)
@ stdcall GetVersion() kernelbase.GetVersion
@ stdcall GetVersionExA(ptr) kernelbase.GetVersionExA
@ stdcall GetVersionExW(ptr) kernelbase.GetVersionExW
@ stdcall GetVolumeInformationA(str ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationA
@ stdcall -version=0x600+ GetVolumeInformationByHandleW(ptr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernelbase.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointA(str ptr long)
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernelbase.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameA(str ptr long)
@ stdcall GetVolumePathNamesForVolumeNameA(str ptr long ptr)
@ stdcall GetVolumePathNamesForVolumeNameW(wstr ptr long ptr) kernelbase.GetVolumePathNamesForVolumeNameW
@ stdcall GetVolumePathNameW(wstr ptr long) kernelbase.GetVolumePathNameW
@ stdcall GetWindowsDirectoryA(ptr long) kernelbase.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernelbase.GetWindowsDirectoryW
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernelbase.GetWriteWatch
@ stdcall -arch=i386,x86_64 GetXStateFeaturesMask(ptr ptr) kernelbase.GetXStateFeaturesMask
@ stdcall GlobalAddAtomA(str)
@ stdcall GlobalAddAtomW(wstr)
@ stdcall GlobalAlloc(long long) kernelbase.GlobalAlloc
@ stdcall GlobalCompact(long)
@ stdcall GlobalDeleteAtom(long)
@ stdcall GlobalFindAtomA(str)
@ stdcall GlobalFindAtomW(wstr)
@ stdcall GlobalFix(long)
@ stdcall GlobalFlags(long)
@ stdcall GlobalFree(long) kernelbase.GlobalFree
@ stdcall GlobalGetAtomNameA(long ptr long)
@ stdcall GlobalGetAtomNameW(long ptr long)
@ stdcall GlobalHandle(ptr)
@ stdcall GlobalLock(long)
@ stdcall GlobalMemoryStatus(ptr)
@ stdcall GlobalMemoryStatusEx(ptr) kernelbase.GlobalMemoryStatusEx
@ stdcall GlobalReAlloc(long long long)
@ stdcall GlobalSize(long)
@ stdcall GlobalUnfix(long)
@ stdcall GlobalUnlock(long)
@ stdcall GlobalUnWire(long)
@ stdcall GlobalWire(long)
@ stdcall Heap32First(ptr long long)
@ stdcall Heap32ListFirst(long ptr)
@ stdcall Heap32ListNext(long ptr)
@ stdcall Heap32Next(ptr)
@ stdcall HeapAlloc(long long long) ntdll.RtlAllocateHeap
@ stdcall HeapCompact(long long) kernelbase.HeapCompact
@ stdcall HeapCreate(long long long)
@ stdcall -version=0x351-0x502 HeapCreateTagsW(ptr long wstr wstr)
@ stdcall HeapDestroy(long)
@ stdcall -version=0x351-0x502 HeapExtend(long long ptr long)
@ stdcall HeapFree(long long ptr) kernelbase.HeapFree
@ stdcall HeapLock(long) kernelbase.HeapLock
@ stdcall HeapQueryInformation(long long ptr long ptr) kernelbase.HeapQueryInformation
@ stdcall -version=0x351-0x502 HeapQueryTagW(long long long long ptr)
@ stdcall HeapReAlloc(long long ptr long) ntdll.RtlReAllocateHeap
@ stub -version=0x351-0x502 HeapSetFlags
@ stdcall HeapSetInformation(ptr long ptr long) kernelbase.HeapSetInformation
@ stdcall HeapSize(long long ptr) ntdll.RtlSizeHeap
@ stdcall HeapSummary(long long ptr)
@ stdcall HeapUnlock(long) kernelbase.HeapUnlock
@ stdcall -version=0x351-0x502 HeapUsage(long long long long ptr)
@ stdcall HeapValidate(long long ptr) kernelbase.HeapValidate
@ stdcall HeapWalk(long ptr) kernelbase.HeapWalk
@ stdcall -version=0x600+ IdnToAscii(long wstr long ptr long) kernelbase.IdnToAscii
@ stdcall -version=0x600+ IdnToNameprepUnicode(long wstr long ptr long) kernelbase.IdnToNameprepUnicode
@ stdcall -version=0x600+ IdnToUnicode(long wstr long ptr long) kernelbase.IdnToUnicode
@ stdcall InitAtomTable(long)
@ stdcall -version=0x600+ InitializeConditionVariable(ptr) ntdll.RtlInitializeConditionVariable
@ stdcall InitializeContext(ptr long ptr ptr) kernelbase.InitializeContext
@ stdcall -version=0xA00+ InitializeContext2(ptr long ptr ptr int64) kernelbase.InitializeContext2
@ stdcall InitializeCriticalSection(ptr) ntdll.RtlInitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernelbase.InitializeCriticalSectionAndSpinCount
@ stdcall -version=0x600+ InitializeCriticalSectionEx(ptr long long) kernelbase.InitializeCriticalSectionEx
@ stdcall -version=0x600+ InitializeProcThreadAttributeList(ptr long long ptr) kernelbase.InitializeProcThreadAttributeList
@ stdcall InitializeSListHead(ptr) ntdll.RtlInitializeSListHead
@ stdcall -version=0x600+ InitializeSRWLock(ptr) ntdll.RtlInitializeSRWLock
@ stdcall -version=0x600+ InitOnceBeginInitialize(ptr long ptr ptr) kernelbase.InitOnceBeginInitialize
@ stdcall -version=0x600+ InitOnceComplete(ptr long ptr) kernelbase.InitOnceComplete
@ stdcall -version=0x600+ InitOnceExecuteOnce(ptr ptr ptr ptr) kernelbase.InitOnceExecuteOnce
@ stdcall -version=0x600+ InitOnceInitialize(ptr) ntdll.RtlRunOnceInitialize
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long)
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr int64 int64) ntdll.RtlInterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedDecrement(ptr)
@ stdcall -arch=i386 InterlockedExchange(ptr long)
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long)
@ stdcall InterlockedFlushSList(ptr) ntdll.RtlInterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr)
@ stdcall InterlockedPopEntrySList(ptr) ntdll.RtlInterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) ntdll.RtlInterlockedPushEntrySList
@ stdcall -fastcall InterlockedPushListSList(ptr ptr ptr long) ntdll.RtlInterlockedPushListSList
@ stdcall -version=0x602+ InterlockedPushListSListEx(ptr ptr ptr long) ntdll.RtlInterlockedPushListSListEx
@ stdcall InvalidateConsoleDIBits(long long)
@ stdcall IsBadCodePtr(ptr)
@ stdcall IsBadHugeReadPtr(ptr long)
@ stdcall IsBadHugeWritePtr(ptr long)
@ stdcall IsBadReadPtr(ptr long)
@ stdcall -norelay IsBadStringPtrA(ptr long)
@ stdcall IsBadStringPtrW(ptr long)
@ stdcall IsBadWritePtr(ptr long)
# @ stub IsCalendarLeapDay
# @ stub IsCalendarLeapMonth
# @ stub IsCalendarLeapYear
@ stdcall IsDBCSLeadByte(long) kernelbase.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernelbase.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent() kernelbase.IsDebuggerPresent
@ stdcall IsNLSDefinedString(long long ptr wstr long) kernelbase.IsNLSDefinedString
@ stdcall -version=0x600+ IsNormalizedString(long wstr long) kernelbase.IsNormalizedString
@ stdcall IsProcessInJob(long long ptr) kernelbase.IsProcessInJob
@ stdcall IsProcessorFeaturePresent(long) kernelbase.IsProcessorFeaturePresent
@ stdcall IsSystemResumeAutomatic()
@ stdcall -version=0x600+ IsThreadAFiber() kernelbase.IsThreadAFiber
@ stdcall -version=0x600+ IsThreadpoolTimerSet(ptr) ntdll.TpIsTimerSet
@ stdcall IsTimeZoneRedirectionEnabled()
# @ stub IsValidCalDateTime
@ stdcall IsValidCodePage(long) kernelbase.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernelbase.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernelbase.IsValidLocale
@ stdcall -version=0x600+ IsValidLocaleName(wstr) kernelbase.IsValidLocaleName
@ stdcall -version=0x602+ IsValidNLSVersion(long wstr ptr) kernelbase.IsValidNLSVersion
@ stdcall -version=0x501-0x502 IsValidUILanguage(long)
@ stdcall IsWow64Process(ptr ptr) kernelbase.IsWow64Process
@ stdcall -version=0xA00+ IsWow64Process2(ptr ptr ptr) kernelbase.IsWow64Process2
@ stdcall -version=0x601+ K32EmptyWorkingSet(long) kernelbase.K32EmptyWorkingSet
@ stdcall -version=0x601+ K32EnumDeviceDrivers(ptr long ptr) kernelbase.K32EnumDeviceDrivers
@ stdcall -version=0x601+ K32EnumPageFilesA(ptr ptr) kernelbase.K32EnumPageFilesA
@ stdcall -version=0x601+ K32EnumPageFilesW(ptr ptr) kernelbase.K32EnumPageFilesW
@ stdcall -version=0x601+ K32EnumProcesses(ptr long ptr) kernelbase.K32EnumProcesses
@ stdcall -version=0x601+ K32EnumProcessModules(long ptr long ptr) kernelbase.K32EnumProcessModules
@ stdcall -version=0x601+ K32EnumProcessModulesEx(long ptr long ptr long) kernelbase.K32EnumProcessModulesEx
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameA(ptr ptr long) kernelbase.K32GetDeviceDriverBaseNameA
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameW(ptr ptr long) kernelbase.K32GetDeviceDriverBaseNameW
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameA(ptr ptr long) kernelbase.K32GetDeviceDriverFileNameA
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameW(ptr ptr long) kernelbase.K32GetDeviceDriverFileNameW
@ stdcall -version=0x601+ K32GetMappedFileNameA(long ptr ptr long) kernelbase.K32GetMappedFileNameA
@ stdcall -version=0x601+ K32GetMappedFileNameW(long ptr ptr long) kernelbase.K32GetMappedFileNameW
@ stdcall -version=0x601+ K32GetModuleBaseNameA(long long ptr long) kernelbase.K32GetModuleBaseNameA
@ stdcall -version=0x601+ K32GetModuleBaseNameW(long long ptr long) kernelbase.K32GetModuleBaseNameW
@ stdcall -version=0x601+ K32GetModuleFileNameExA(long long ptr long) kernelbase.K32GetModuleFileNameExA
@ stdcall -version=0x601+ K32GetModuleFileNameExW(long long ptr long) kernelbase.K32GetModuleFileNameExW
@ stdcall -version=0x601+ K32GetModuleInformation(long long ptr long) kernelbase.K32GetModuleInformation
@ stdcall -version=0x601+ K32GetPerformanceInfo(ptr long) kernelbase.K32GetPerformanceInfo
@ stdcall -version=0x601+ K32GetProcessImageFileNameA(long ptr long) kernelbase.K32GetProcessImageFileNameA
@ stdcall -version=0x601+ K32GetProcessImageFileNameW(long ptr long) kernelbase.K32GetProcessImageFileNameW
@ stdcall -version=0x601+ K32GetProcessMemoryInfo(long ptr long) kernelbase.K32GetProcessMemoryInfo
@ stdcall -version=0x601+ K32GetWsChanges(long ptr long) kernelbase.K32GetWsChanges
@ stdcall -version=0x601+ K32GetWsChangesEx(long ptr ptr) kernelbase.K32GetWsChangesEx
@ stdcall -version=0x601+ K32InitializeProcessForWsWatch(long) kernelbase.K32InitializeProcessForWsWatch
@ stdcall -version=0x601+ K32QueryWorkingSet(long ptr long) kernelbase.K32QueryWorkingSet
@ stdcall -version=0x601+ K32QueryWorkingSetEx(long ptr long) kernelbase.K32QueryWorkingSetEx
@ stdcall -version=0x600+ LCIDToLocaleName(long ptr long long) kernelbase.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernelbase.LCMapStringA
@ stdcall -version=0x600+ LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernelbase.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernelbase.LCMapStringW
@ stdcall LeaveCriticalSection(ptr) ntdll.RtlLeaveCriticalSection
@ stdcall -version=0x600+ LeaveCriticalSectionWhenCallbackReturns(ptr ptr) ntdll.TpCallbackLeaveCriticalSectionOnCompletion
@ stdcall LoadAppInitDlls() kernelbase.LoadAppInitDlls
@ stdcall LoadLibraryA(str) kernelbase.LoadLibraryA
@ stdcall LoadLibraryExA( str long long) kernelbase.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernelbase.LoadLibraryExW
@ stdcall LoadLibraryW(wstr) kernelbase.LoadLibraryW
@ stdcall LoadModule(str ptr)
@ stdcall -version=0x602+ LoadPackagedLibrary(wstr long) kernelbase.LoadPackagedLibrary
@ stdcall LoadResource(long long) kernelbase.LoadResource
# @ stub LoadStringBaseExW
# @ stub LoadStringBaseW
@ stdcall LocalAlloc(long long) kernelbase.LocalAlloc
@ stdcall LocalCompact(long)
@ stdcall -version=0x600+ LocaleNameToLCID(wstr long) kernelbase.LocaleNameToLCID
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernelbase.LocalFileTimeToFileTime
@ stdcall LocalFlags(long)
@ stdcall LocalFree(long) kernelbase.LocalFree
@ stdcall LocalHandle(ptr)
@ stdcall LocalLock(long) kernelbase.LocalLock
@ stdcall LocalReAlloc(long long long) kernelbase.LocalReAlloc
@ stdcall LocalShrink(long long)
@ stdcall LocalSize(long)
@ stdcall LocalUnlock(long) kernelbase.LocalUnlock
@ stdcall -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernelbase.LocateXStateFeature
@ stdcall LockFile(long long long long long) kernelbase.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernelbase.LockFileEx
@ stdcall LockResource(long) kernelbase.LockResource
@ stdcall LZClose(long)
@ stdcall LZCloseFile(long)
@ stdcall LZCopy(long long)
@ stdcall LZCreateFileW(ptr long long long ptr)
@ stdcall LZDone()
@ stdcall LZInit(long)
@ stdcall LZOpenFileA(str ptr long)
@ stdcall LZOpenFileW(wstr ptr long)
@ stdcall LZRead(long ptr long)
@ stdcall LZSeek(long long long)
@ stdcall LZStart()
@ stdcall MapUserPhysicalPages(ptr long ptr) kernelbase.MapUserPhysicalPages
@ stdcall MapUserPhysicalPagesScatter(ptr long ptr)
@ stdcall MapViewOfFile(long long long long long) kernelbase.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernelbase.MapViewOfFileEx
@ stdcall -version=0x600+ MapViewOfFileExNuma(long long long long long ptr long) kernelbase.MapViewOfFileExNuma
@ stdcall -version=0x602+ MapViewOfFileFromApp(long long int64 long) kernelbase.MapViewOfFileFromApp
@ stdcall Module32First(long ptr)
@ stdcall Module32FirstW(long ptr)
@ stdcall Module32Next(long ptr)
@ stdcall Module32NextW(long ptr)
@ stdcall MoveFileA(str str)
@ stdcall MoveFileExA(str str long)
@ stdcall MoveFileExW(wstr wstr long) kernelbase.MoveFileExW
@ stdcall -version=0x600+ MoveFileTransactedA(str str ptr ptr long ptr)
@ stdcall -version=0x600+ MoveFileTransactedW(wstr wstr ptr ptr long ptr)
@ stdcall MoveFileW(wstr wstr)
@ stdcall MoveFileWithProgressA(str str ptr ptr long)
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernelbase.MoveFileWithProgressW
@ stdcall MulDiv(long long long)
@ stdcall MultiByteToWideChar(long long str long ptr long) kernelbase.MultiByteToWideChar
@ stdcall NeedCurrentDirectoryForExePathA(str) kernelbase.NeedCurrentDirectoryForExePathA
@ stdcall NeedCurrentDirectoryForExePathW(wstr) kernelbase.NeedCurrentDirectoryForExePathW
# @ stub NlsCheckPolicy
@ stdcall -version=0x500-0x600 NlsConvertIntegerToString(long long long wstr long)
# @ stub NlsEventDataDescCreate
@ stdcall NlsGetCacheUpdateCount()
@ stdcall -version=0x500-0x502 NlsResetProcessLocale()
# @ stub NlsUpdateLocale
# @ stub NlsUpdateSystemLocale
# @ stub NlsWriteEtwEvent
@ stdcall -version=0x600+ NormalizeString(long wstr long ptr long) kernelbase.NormalizeString
# @ stub NotifyMountMgr
@ stdcall -stub -version=0x600+ NotifyUILanguageChange(long wstr wstr long ptr)
@ stdcall OpenConsoleW(wstr long long long)
@ stdcall -version=0x500-0x502 OpenDataFile(long long)
@ stdcall OpenEventA(long long str) kernelbase.OpenEventA
@ stdcall OpenEventW(long long wstr) kernelbase.OpenEventW
@ stdcall OpenFile(str ptr long)
@ stdcall -version=0x600+ OpenFileById(long ptr long long ptr long) kernelbase.OpenFileById
@ stdcall OpenFileMappingA(long long str)
@ stdcall OpenFileMappingW(long long wstr) kernelbase.OpenFileMappingW
@ stdcall OpenJobObjectA(long long str)
@ stdcall OpenJobObjectW(long long wstr)
@ stdcall OpenMutexA(long long str)
@ stdcall OpenMutexW(long long wstr) kernelbase.OpenMutexW
# @ stub OpenPrivateNamespaceA
# @ stub OpenPrivateNamespaceW
@ stdcall OpenProcess(long long long) kernelbase.OpenProcess
@ stdcall OpenProcessToken(long long ptr) kernelbase.OpenProcessToken
@ stdcall OpenProfileUserMapping()
@ stdcall OpenSemaphoreA(long long str)
@ stdcall OpenSemaphoreW(long long wstr) kernelbase.OpenSemaphoreW
@ stdcall OpenThread(long long long) kernelbase.OpenThread
@ stdcall OpenThreadToken(long long long ptr) kernelbase.OpenThreadToken
@ stdcall OpenWaitableTimerA(long long str)
@ stdcall OpenWaitableTimerW(long long wstr) kernelbase.OpenWaitableTimerW
@ stdcall OutputDebugStringA(str)
@ stdcall OutputDebugStringW(wstr) kernelbase.OutputDebugStringW
@ stdcall -version=0x602+ PackageIdFromFullName(wstr long ptr ptr) kernelbase.PackageIdFromFullName
@ stdcall PeekConsoleInputA(ptr ptr long ptr) kernelbase.PeekConsoleInputA
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernelbase.PeekConsoleInputW
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernelbase.PeekNamedPipe
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernelbase.PostQueuedCompletionStatus
@ stdcall PowerClearRequest(long long)
@ stdcall PowerCreateRequest(ptr)
@ stdcall PowerSetRequest(long long)
@ stdcall -version=0x602+ PrefetchVirtualMemory(ptr ptr ptr long) kernelbase.PrefetchVirtualMemory
@ stdcall PrepareTape(ptr long long)
@ stdcall PrivCopyFileExW(wstr wstr ptr ptr long long)
@ stdcall PrivMoveFileIdentityW(long long long)
@ stdcall Process32First(ptr ptr)
@ stdcall Process32FirstW(ptr ptr)
@ stdcall Process32Next(ptr ptr)
@ stdcall Process32NextW(ptr ptr)
@ stdcall ProcessIdToSessionId(long ptr) kernelbase.ProcessIdToSessionId
@ stdcall PulseEvent(long) kernelbase.PulseEvent
@ stdcall PurgeComm(long long) kernelbase.PurgeComm
@ stdcall -version=0x600+ QueryActCtxSettingsW(long ptr wstr wstr ptr long ptr) kernelbase.QueryActCtxSettingsW
@ stdcall QueryActCtxW(long ptr ptr long ptr long ptr) kernelbase.QueryActCtxW
@ stdcall QueryDepthSList(ptr) ntdll.RtlQueryDepthSList
@ stdcall QueryDosDeviceA(str ptr long)
@ stdcall QueryDosDeviceW(wstr ptr long) kernelbase.QueryDosDeviceW
@ stdcall -version=0x600+ QueryFullProcessImageNameA(ptr long ptr ptr) kernelbase.QueryFullProcessImageNameA
@ stdcall -version=0x600+ QueryFullProcessImageNameW(ptr long ptr ptr) kernelbase.QueryFullProcessImageNameW
@ stdcall -version=0x600+ QueryIdleProcessorCycleTime(ptr ptr) kernelbase.QueryIdleProcessorCycleTime
@ stdcall QueryIdleProcessorCycleTimeEx(long ptr ptr) kernelbase.QueryIdleProcessorCycleTimeEx
@ stdcall QueryInformationJobObject(long long ptr long ptr)
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernelbase.QueryMemoryResourceNotification
@ stdcall QueryPerformanceCounter(ptr) kernelbase.QueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) kernelbase.QueryPerformanceFrequency
# @ stub QueryProcessAffinityUpdateMode
@ stdcall -version=0x600+ QueryProcessCycleTime(long ptr) kernelbase.QueryProcessCycleTime
@ stdcall -version=0x600+ QueryThreadCycleTime(long ptr) kernelbase.QueryThreadCycleTime
@ stdcall QueryThreadpoolStackInformation(ptr ptr) kernelbase.QueryThreadpoolStackInformation
# @ stub QueryThreadProfiling
@ stdcall -version=0x601+ -arch=win64 QueryUmsThreadInformation(ptr long ptr long ptr)
@ stdcall QueryUnbiasedInterruptTime(ptr) kernelbase.QueryUnbiasedInterruptTime
@ stdcall QueueUserAPC(ptr long long) kernelbase.QueueUserAPC
@ stdcall QueueUserWorkItem(ptr ptr long) kernelbase.QueueUserWorkItem
@ stdcall RaiseException(long long long ptr) kernelbase.RaiseException
@ stdcall RaiseFailFastException(ptr ptr long) kernelbase.RaiseFailFastException
@ stdcall ReadConsoleA(long ptr long ptr ptr) kernelbase.ReadConsoleA
@ stdcall ReadConsoleInputA(long ptr long ptr) kernelbase.ReadConsoleInputA
@ stdcall ReadConsoleInputExA(long ptr long ptr long)
@ stdcall ReadConsoleInputExW(long ptr long ptr long)
@ stdcall ReadConsoleInputW(long ptr long ptr) kernelbase.ReadConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr) kernelbase.ReadConsoleOutputA
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr) kernelbase.ReadConsoleOutputAttribute
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterA
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr) kernelbase.ReadConsoleOutputCharacterW
@ stdcall ReadConsoleOutputW(long ptr long long ptr) kernelbase.ReadConsoleOutputW
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernelbase.ReadConsoleW
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernelbase.ReadDirectoryChangesW
@ stdcall ReadFile(long ptr long ptr ptr) kernelbase.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernelbase.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernelbase.ReadFileScatter
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernelbase.ReadProcessMemory
# @ stub ReadThreadProfilingData
@ stdcall -private RegCloseKey(long) kernelbase.RegCloseKey
@ stdcall -private RegCreateKeyExA(long str long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExA
@ stdcall -private RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) kernelbase.RegCreateKeyExW
@ stdcall -private RegDeleteKeyExA(long str long long) kernelbase.RegDeleteKeyExA
@ stdcall -private RegDeleteKeyExW(long wstr long long) kernelbase.RegDeleteKeyExW
@ stdcall -private RegDeleteTreeA(long str) kernelbase.RegDeleteTreeA
@ stdcall -private RegDeleteTreeW(long wstr) kernelbase.RegDeleteTreeW
@ stdcall -private RegDeleteValueA(long str) kernelbase.RegDeleteValueA
@ stdcall -private RegDeleteValueW(long wstr) kernelbase.RegDeleteValueW
@ stdcall -private RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExA
@ stdcall -private RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumKeyExW
@ stdcall -private RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueA
@ stdcall -private RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) kernelbase.RegEnumValueW
@ stdcall -private RegFlushKey(long) kernelbase.RegFlushKey
@ stdcall -private RegGetKeySecurity(long long ptr ptr) kernelbase.RegGetKeySecurity
@ stdcall -private RegGetValueA(long str str long ptr ptr ptr) kernelbase.RegGetValueA
@ stdcall -private RegGetValueW(long wstr wstr long ptr ptr ptr) kernelbase.RegGetValueW
@ stdcall -version=0x600+ RegisterApplicationRecoveryCallback(ptr ptr long long)
@ stdcall -version=0x600+ RegisterApplicationRestart(wstr long)
@ stdcall RegisterConsoleIME(ptr ptr)
@ stdcall RegisterConsoleOS2(long)
@ stdcall RegisterConsoleVDM(long long long long long long long long long long long)
@ stdcall RegisterWaitForInputIdle(ptr)
@ stdcall RegisterWaitForSingleObject(ptr long ptr ptr long long)
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernelbase.RegisterWaitForSingleObjectEx
@ stdcall RegisterWowBaseHandlers(long)
@ stdcall RegisterWowExec(long)
@ stdcall -private RegLoadKeyA(long str str) kernelbase.RegLoadKeyA
@ stdcall -private RegLoadKeyW(long wstr wstr) kernelbase.RegLoadKeyW
@ stdcall -private RegLoadMUIStringA(long str str long ptr long str) kernelbase.RegLoadMUIStringA
@ stdcall -private RegLoadMUIStringW(long wstr wstr long ptr long wstr) kernelbase.RegLoadMUIStringW
@ stdcall -private RegNotifyChangeKeyValue(long long long long long) kernelbase.RegNotifyChangeKeyValue
@ stdcall -private RegOpenCurrentUser(long ptr) kernelbase.RegOpenCurrentUser
@ stdcall -private RegOpenKeyExA(long str long long ptr) kernelbase.RegOpenKeyExA
@ stdcall -private RegOpenKeyExW(long wstr long long ptr) kernelbase.RegOpenKeyExW
@ stdcall -private RegOpenUserClassesRoot(ptr long long ptr) kernelbase.RegOpenUserClassesRoot
@ stdcall -private RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyA
@ stdcall -private RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernelbase.RegQueryInfoKeyW
@ stdcall -private RegQueryValueExA(long str ptr ptr ptr ptr) kernelbase.RegQueryValueExA
@ stdcall -private RegQueryValueExW(long wstr ptr ptr ptr ptr) kernelbase.RegQueryValueExW
@ stdcall -private RegRestoreKeyA(long str long) kernelbase.RegRestoreKeyA
@ stdcall -private RegRestoreKeyW(long wstr long) kernelbase.RegRestoreKeyW
@ stdcall -private RegSaveKeyExA(long str ptr long) kernelbase.RegSaveKeyExA
@ stdcall -private RegSaveKeyExW(long wstr ptr long) kernelbase.RegSaveKeyExW
@ stdcall -private RegSetKeySecurity(long long ptr) kernelbase.RegSetKeySecurity
@ stdcall -private RegSetValueExA(long str long long ptr long) kernelbase.RegSetValueExA
@ stdcall -private RegSetValueExW(long wstr long long ptr long) kernelbase.RegSetValueExW
@ stdcall -private RegUnLoadKeyA(long str) kernelbase.RegUnLoadKeyA
@ stdcall -private RegUnLoadKeyW(long wstr) kernelbase.RegUnLoadKeyW
@ stdcall ReleaseActCtx(ptr) kernelbase.ReleaseActCtx
@ stdcall ReleaseMutex(long) kernelbase.ReleaseMutex
@ stdcall -version=0x600+ ReleaseMutexWhenCallbackReturns(ptr long) ntdll.TpCallbackReleaseMutexOnCompletion
@ stdcall ReleaseSemaphore(long long ptr) kernelbase.ReleaseSemaphore
@ stdcall -version=0x600+ ReleaseSemaphoreWhenCallbackReturns(ptr long long) ntdll.TpCallbackReleaseSemaphoreOnCompletion
@ stdcall -version=0x600+ ReleaseSRWLockExclusive(ptr) ntdll.RtlReleaseSRWLockExclusive
@ stdcall -version=0x600+ ReleaseSRWLockShared(ptr) ntdll.RtlReleaseSRWLockShared
@ stdcall RemoveDirectoryA(str) kernelbase.RemoveDirectoryA
@ stdcall -version=0x600+ RemoveDirectoryTransactedA(str ptr)
@ stdcall -version=0x600+ RemoveDirectoryTransactedW(wstr ptr)
@ stdcall RemoveDirectoryW(wstr) kernelbase.RemoveDirectoryW
@ stdcall RemoveDllDirectory(ptr) kernelbase.RemoveDllDirectory
@ stdcall RemoveLocalAlternateComputerNameA(str long)
@ stdcall RemoveLocalAlternateComputerNameW(wstr long)
# @ stub RemoveSecureMemoryCacheCallback
@ stdcall RemoveVectoredContinueHandler(ptr) ntdll.RtlRemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) ntdll.RtlRemoveVectoredExceptionHandler
@ stdcall ReOpenFile(ptr long long long) ReOpenFile
@ stdcall ReplaceFile(wstr wstr wstr long ptr ptr) ReplaceFileW
@ stdcall ReplaceFileA(str str str long ptr ptr)
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernelbase.ReplaceFileW
# @ stub ReplacePartitionUnit
@ stdcall RequestDeviceWakeup(long)
@ stdcall RequestWakeupLatency(long)
@ stdcall ResetEvent(long) kernelbase.ResetEvent
@ stdcall ResetWriteWatch(ptr long) kernelbase.ResetWriteWatch
@ stdcall -version=0xA00+ ResizePseudoConsole(ptr long) kernelbase.ResizePseudoConsole
@ stdcall -version=0x602+ ResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long) ntdll.LdrResolveDelayLoadedAPI
@ stdcall ResolveLocaleName(wstr ptr long) kernelbase.ResolveLocaleName
@ stdcall RestoreLastError(long) ntdll.RtlRestoreLastWin32Error
@ stdcall ResumeThread(long) kernelbase.ResumeThread
@ cdecl -arch=x86_64 RtlAddFunctionTable(ptr long long) ntdll.RtlAddFunctionTable
@ stdcall -norelay RtlCaptureContext(ptr) kernelbase.RtlCaptureContext
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr) ntdll.RtlCaptureStackBackTrace
@ stdcall -arch=x86_64 RtlCompareMemory(ptr ptr long) ntdll.RtlCompareMemory
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr long) ntdll.RtlCopyMemory
@ cdecl -arch=x86_64 RtlDeleteFunctionTable(ptr) ntdll.RtlDeleteFunctionTable
@ stdcall RtlFillMemory(ptr long long) ntdll.RtlFillMemory
@ cdecl -arch=x86_64 RtlInstallFunctionTableCallback(long long long ptr ptr ptr) ntdll.RtlInstallFunctionTableCallback
@ stdcall -version=a -arch=x86_64 -norelay RtlIsEcCode(ptr) ntdll.RtlIsEcCode
@ stdcall -arch=x86_64 RtlLookupFunctionEntry(long ptr ptr) ntdll.RtlLookupFunctionEntry
@ stdcall RtlMoveMemory(ptr ptr long) ntdll.RtlMoveMemory
@ stdcall RtlPcToFileHeader(ptr ptr) ntdll.RtlPcToFileHeader
@ stdcall -arch=arm,x86_64 -norelay RtlRaiseException(ptr) ntdll.RtlRaiseException
@ cdecl RtlRestoreContext(ptr ptr) kernelbase.RtlRestoreContext
@ stdcall RtlUnwind(ptr ptr ptr long) ntdll.RtlUnwind
@ stdcall -arch=x86_64 RtlUnwindEx(long long ptr long ptr) ntdll.RtlUnwindEx
@ stdcall -arch=x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr) ntdll.RtlVirtualUnwind
@ stdcall RtlZeroMemory(ptr long) ntdll.RtlZeroMemory
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferA
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr) kernelbase.ScrollConsoleScreenBufferW
@ stdcall SearchPathA(str str str long ptr ptr) kernelbase.SearchPathA
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr) kernelbase.SearchPathW
@ stdcall -version=0x602+ SetCachedSigningLevel(ptr long long long) kernelbase.SetCachedSigningLevel
@ stdcall SetCalendarInfoA(long long long str)
@ stdcall SetCalendarInfoW(long long long wstr) kernelbase.SetCalendarInfoW
@ stdcall SetClientTimeZoneInformation(ptr)
@ stdcall SetCommBreak(long) kernelbase.SetCommBreak
@ stdcall SetCommConfig(long ptr long) kernelbase.SetCommConfig
@ stdcall SetCommMask(long long) kernelbase.SetCommMask
@ stdcall SetCommState(long ptr) kernelbase.SetCommState
@ stdcall SetCommTimeouts(long ptr) kernelbase.SetCommTimeouts
@ stdcall SetComPlusPackageInstallStatus(ptr)
@ stdcall SetComputerNameA(str) kernelbase.SetComputerNameA
@ stdcall SetComputerNameExA(long str) kernelbase.SetComputerNameExA
@ stdcall SetComputerNameExW(long wstr) kernelbase.SetComputerNameExW
@ stdcall SetComputerNameW(wstr) kernelbase.SetComputerNameW
@ stdcall SetConsoleActiveScreenBuffer(long) kernelbase.SetConsoleActiveScreenBuffer
@ stdcall -version=0x351-0x502 SetConsoleCommandHistoryMode(long)
@ stdcall SetConsoleCP(long) kernelbase.SetConsoleCP
@ stdcall SetConsoleCtrlHandler(ptr long) kernelbase.SetConsoleCtrlHandler
@ stdcall SetConsoleCursor(long long)
@ stdcall SetConsoleCursorInfo(long ptr) kernelbase.SetConsoleCursorInfo
@ stdcall SetConsoleCursorMode(long long long)
@ stdcall SetConsoleCursorPosition(long long) kernelbase.SetConsoleCursorPosition
@ stdcall SetConsoleDisplayMode(long long ptr) kernelbase.SetConsoleDisplayMode
@ stdcall SetConsoleFont(long long)
@ stdcall SetConsoleHardwareState(long long long)
@ stdcall -version=0x600+ SetConsoleHistoryInfo(ptr)
@ stdcall SetConsoleIcon(ptr)
@ stdcall SetConsoleInputExeNameA(str) kernelbase.SetConsoleInputExeNameA
@ stdcall SetConsoleInputExeNameW(wstr) kernelbase.SetConsoleInputExeNameW
@ stdcall SetConsoleKeyShortcuts(long long ptr long)
@ stdcall SetConsoleLocalEUDC(long long long long)
@ stdcall SetConsoleMaximumWindowSize(long long)
@ stdcall SetConsoleMenuClose(long)
@ stdcall SetConsoleMode(long long) kernelbase.SetConsoleMode
@ stdcall SetConsoleNlsMode(long long)
@ stdcall SetConsoleNumberOfCommandsA(long long)
@ stdcall SetConsoleNumberOfCommandsW(long long)
@ stdcall SetConsoleOS2OemFormat(long)
@ stdcall SetConsoleOutputCP(long) kernelbase.SetConsoleOutputCP
@ stdcall SetConsolePalette(long long long)
@ stdcall -version=0x600+ SetConsoleScreenBufferInfoEx(long ptr) kernelbase.SetConsoleScreenBufferInfoEx
@ stdcall SetConsoleScreenBufferSize(long long) kernelbase.SetConsoleScreenBufferSize
@ stdcall SetConsoleTextAttribute(long long) kernelbase.SetConsoleTextAttribute
@ stdcall SetConsoleTitleA(str) kernelbase.SetConsoleTitleA
@ stdcall SetConsoleTitleW(wstr) kernelbase.SetConsoleTitleW
@ stdcall SetConsoleWindowInfo(long long ptr) kernelbase.SetConsoleWindowInfo
@ stdcall -version=0x500-0x502 SetCPGlobal(long)
@ stdcall SetCriticalSectionSpinCount(ptr long) ntdll.RtlSetCriticalSectionSpinCount
@ stdcall -version=0x600+ SetCurrentConsoleFontEx(long long ptr) kernelbase.SetCurrentConsoleFontEx
@ stdcall SetCurrentDirectoryA(str) kernelbase.SetCurrentDirectoryA
@ stdcall SetCurrentDirectoryW(wstr) kernelbase.SetCurrentDirectoryW
@ stdcall SetDefaultCommConfigA(str ptr long)
@ stdcall SetDefaultCommConfigW(wstr ptr long)
@ stdcall SetDefaultDllDirectories(long) kernelbase.SetDefaultDllDirectories
@ stdcall SetDllDirectoryA(str)
@ stdcall SetDllDirectoryW(wstr)
# @ stub SetDynamicTimeZoneInformation
@ stdcall SetEndOfFile(long) kernelbase.SetEndOfFile
@ stdcall SetEnvironmentStringsA(str) kernelbase.SetEnvironmentStringsA
@ stdcall SetEnvironmentStringsW(wstr) kernelbase.SetEnvironmentStringsW
@ stdcall SetEnvironmentVariableA(str str) kernelbase.SetEnvironmentVariableA
@ stdcall SetEnvironmentVariableW(wstr wstr) kernelbase.SetEnvironmentVariableW
@ stdcall SetErrorMode(long) kernelbase.SetErrorMode
@ stdcall SetEvent(long) kernelbase.SetEvent
@ stdcall -version=0x600+ SetEventWhenCallbackReturns(ptr long) ntdll.TpCallbackSetEventOnCompletion
@ stdcall SetFileApisToANSI() kernelbase.SetFileApisToANSI
@ stdcall SetFileApisToOEM() kernelbase.SetFileApisToOEM
@ stdcall SetFileAttributesA(str long) kernelbase.SetFileAttributesA
# @ stub SetFileAttributesTransactedA
# @ stub SetFileAttributesTransactedW
@ stdcall SetFileAttributesW(wstr long) kernelbase.SetFileAttributesW
@ stdcall -version=0x600+ SetFileBandwidthReservation(ptr long long long ptr ptr)
@ stdcall SetFileCompletionNotificationModes(long long)
@ stdcall -version=0x600+ SetFileInformationByHandle(long long ptr long) kernelbase.SetFileInformationByHandle
# @ stub SetFileIoOverlappedRange
@ stdcall SetFilePointer(long long ptr long) kernelbase.SetFilePointer
@ stdcall SetFilePointerEx(long int64 ptr long) kernelbase.SetFilePointerEx
@ stdcall SetFileShortNameA(long str)
@ stdcall SetFileShortNameW(long wstr)
@ stdcall SetFileTime(long ptr ptr ptr) kernelbase.SetFileTime
@ stdcall SetFileValidData(ptr int64) kernelbase.SetFileValidData
@ stdcall SetFirmwareEnvironmentVariableA(str str ptr long)
@ stdcall SetFirmwareEnvironmentVariableW(wstr wstr ptr long)
@ stdcall SetHandleContext(long long)
@ stdcall SetHandleCount(long)
@ stdcall SetHandleInformation(long long long) kernelbase.SetHandleInformation
@ stdcall SetInformationJobObject(long long ptr long)
@ stdcall SetLastConsoleEventActive() ; missing in XP SP3
@ stdcall SetLastError(long) RtlSetLastWin32Error
@ stdcall SetLocaleInfoA(long long str)
@ stdcall SetLocaleInfoW(long long wstr) kernelbase.SetLocaleInfoW
@ stdcall SetLocalPrimaryComputerNameA(long long) ; missing in XP SP3
@ stdcall SetLocalPrimaryComputerNameW(long long) ; missing in XP SP3
@ stdcall SetLocalTime(ptr) kernelbase.SetLocalTime
@ stdcall SetMailslotInfo(long long)
@ stdcall SetMessageWaitingIndicator(ptr long)
# @ stub SetNamedPipeAttribute
@ stdcall SetNamedPipeHandleState(long ptr ptr ptr) kernelbase.SetNamedPipeHandleState
@ stdcall SetPriorityClass(long long) kernelbase.SetPriorityClass
@ stdcall SetProcessAffinityMask(long long)
@ stdcall -version=0x600+ SetProcessAffinityUpdateMode(long long) kernelbase.SetProcessAffinityUpdateMode
@ stdcall -version=0xA00+ SetProcessDefaultCpuSets(ptr ptr long) kernelbase.SetProcessDefaultCpuSets
@ stdcall -version=0x600+ SetProcessDEPPolicy(long)
@ stdcall -version=0x602+ SetProcessInformation(long long ptr long) kernelbase.SetProcessInformation
@ stdcall -version=0x602+ SetProcessMitigationPolicy(long ptr long) kernelbase.SetProcessMitigationPolicy
@ stdcall SetProcessPreferredUILanguages(long ptr ptr) kernelbase.SetProcessPreferredUILanguages
@ stdcall SetProcessPriorityBoost(long long) kernelbase.SetProcessPriorityBoost
@ stdcall SetProcessShutdownParameters(long long) kernelbase.SetProcessShutdownParameters
# @ stub SetProcessUserModeExceptionPolicy
@ stdcall SetProcessWorkingSetSize(long long long)
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernelbase.SetProcessWorkingSetSizeEx
@ stdcall SetSearchPathMode(long)
@ stdcall SetStdHandle(long long) kernelbase.SetStdHandle
@ stdcall -version=0x600+ SetStdHandleEx(long long ptr) kernelbase.SetStdHandleEx
@ stdcall SetSystemFileCacheSize(long long long) kernelbase.SetSystemFileCacheSize
@ stdcall SetSystemPowerState(long long)
@ stdcall SetSystemTime(ptr) kernelbase.SetSystemTime
@ stdcall SetSystemTimeAdjustment(long long) kernelbase.SetSystemTimeAdjustment
@ stdcall SetTapeParameters(ptr long ptr)
@ stdcall SetTapePosition(ptr long long long long long)
@ stdcall SetTermsrvAppInstallMode(long)
@ stdcall SetThreadAffinityMask(long long)
@ stdcall SetThreadContext(long ptr) kernelbase.SetThreadContext
@ stdcall -version=0xA00+ SetThreadDescription(ptr wstr) kernelbase.SetThreadDescription
@ stdcall -version=0x600+ SetThreadErrorMode(long ptr) kernelbase.SetThreadErrorMode
@ stdcall SetThreadExecutionState(long)
@ stdcall SetThreadGroupAffinity(long ptr ptr) kernelbase.SetThreadGroupAffinity
@ stdcall SetThreadIdealProcessor(long long) kernelbase.SetThreadIdealProcessor
@ stdcall SetThreadIdealProcessorEx(long ptr ptr) kernelbase.SetThreadIdealProcessorEx
@ stdcall -version=0xA00+ SetThreadInformation(long long ptr long) kernelbase.SetThreadInformation
@ stdcall SetThreadLocale(long) kernelbase.SetThreadLocale
@ stdcall SetThreadpoolStackInformation(ptr ptr) kernelbase.SetThreadpoolStackInformation
@ stdcall -version=0x600+ SetThreadpoolThreadMaximum(ptr long) ntdll.TpSetPoolMaxThreads
@ stdcall -version=0x600+ SetThreadpoolThreadMinimum(ptr long) ntdll.TpSetPoolMinThreads
@ stdcall -version=0x600+ SetThreadpoolTimer(ptr ptr long long) ntdll.TpSetTimer
@ stdcall -version=0x600+ SetThreadpoolWait(ptr long ptr) ntdll.TpSetWait
@ stdcall -version=0x600+ SetThreadPreferredUILanguages(long ptr ptr) kernelbase.SetThreadPreferredUILanguages
@ stdcall SetThreadPriority(long long) kernelbase.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernelbase.SetThreadPriorityBoost
@ stdcall -version=0xA00+ SetThreadSelectedCpuSets(ptr ptr long) kernelbase.SetThreadSelectedCpuSets
@ stdcall SetThreadStackGuarantee(ptr) kernelbase.SetThreadStackGuarantee
@ stdcall SetThreadToken(ptr ptr) kernelbase.SetThreadToken
@ stdcall SetThreadUILanguage(long) kernelbase.SetThreadUILanguage
@ stdcall SetTimerQueueTimer(long ptr ptr long long long)
@ stdcall SetTimeZoneInformation(ptr) kernelbase.SetTimeZoneInformation
@ stdcall -version=0x601+ -arch=win64 SetUmsThreadInformation(ptr long ptr long)
@ stdcall SetUnhandledExceptionFilter(ptr) kernelbase.SetUnhandledExceptionFilter
@ stdcall SetupComm(long long long) kernelbase.SetupComm
@ stdcall SetUserGeoID(long) kernelbase.SetUserGeoID
@ stdcall -version=0xA00+ SetUserGeoName(wstr) kernelbase.SetUserGeoName
@ stdcall SetVDMCurrentDirectories(long long)
@ stdcall SetVolumeLabelA(str str)
@ stdcall SetVolumeLabelW(wstr wstr)
@ stdcall SetVolumeMountPointA(str str)
@ stdcall SetVolumeMountPointW(wstr wstr)
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernelbase.SetWaitableTimer
@ stdcall SetWaitableTimerEx(long ptr long ptr ptr ptr long) kernelbase.SetWaitableTimerEx
@ stdcall -arch=i386,x86_64 SetXStateFeaturesMask(ptr int64) kernelbase.SetXStateFeaturesMask
@ stdcall ShowConsoleCursor(long long)
@ stdcall SignalObjectAndWait(long long long long) kernelbase.SignalObjectAndWait
@ stdcall SizeofResource(long long) kernelbase.SizeofResource
@ stdcall Sleep(long) kernelbase.Sleep
@ stdcall -version=0x600+ SleepConditionVariableCS(ptr ptr long) kernelbase.SleepConditionVariableCS
@ stdcall -version=0x600+ SleepConditionVariableSRW(ptr ptr long long) kernelbase.SleepConditionVariableSRW
@ stdcall SleepEx(long long) kernelbase.SleepEx
# @ stub SortCloseHandle
# @ stub SortGetHandle
@ stdcall -version=0x600+ StartThreadpoolIo(ptr) ntdll.TpStartAsyncIoOperation
@ stdcall -version=0x600+ SubmitThreadpoolWork(ptr) ntdll.TpPostWork
@ stdcall SuspendThread(long) kernelbase.SuspendThread
@ stdcall SwitchToFiber(ptr) kernelbase.SwitchToFiber
@ stdcall SwitchToThread() kernelbase.SwitchToThread
@ stdcall SystemTimeToFileTime(ptr ptr) kernelbase.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr)
# @ stub SystemTimeToTzSpecificLocalTimeEx
@ stdcall TerminateJobObject(long long)
@ stdcall TerminateProcess(long long) kernelbase.TerminateProcess
@ stdcall TerminateThread(long long) kernelbase.TerminateThread
@ stdcall TermsrvAppInstallMode()
@ stdcall Thread32First(long ptr)
@ stdcall Thread32Next(long ptr)
@ stdcall TlsAlloc() kernelbase.TlsAlloc
@ stdcall TlsFree(long) kernelbase.TlsFree
@ stdcall TlsGetValue(long) kernelbase.TlsGetValue
@ stdcall TlsSetValue(long ptr) kernelbase.TlsSetValue
@ stdcall Toolhelp32ReadProcessMemory(long ptr ptr long ptr)
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernelbase.TransactNamedPipe
@ stdcall TransmitCommChar(long long) kernelbase.TransmitCommChar
@ stdcall -version=0x601+ TryAcquireSRWLockExclusive(ptr) ntdll.RtlTryAcquireSRWLockExclusive
@ stdcall -version=0x601+ TryAcquireSRWLockShared(ptr) ntdll.RtlTryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) ntdll.RtlTryEnterCriticalSection
@ stdcall -version=0x600+ TrySubmitThreadpoolCallback(ptr ptr ptr) kernelbase.TrySubmitThreadpoolCallback
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernelbase.TzSpecificLocalTimeToSystemTime
# @ stub TzSpecificLocalTimeToSystemTimeEx
@ stdcall -version=0x601+ -arch=win64 UmsThreadYield(ptr)
@ stdcall UnhandledExceptionFilter(ptr) kernelbase.UnhandledExceptionFilter
@ stdcall UnlockFile(long long long long long) kernelbase.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernelbase.UnlockFileEx
@ stdcall UnmapViewOfFile(ptr) kernelbase.UnmapViewOfFile
@ stdcall -version=0x602+ UnmapViewOfFileEx(ptr long) kernelbase.UnmapViewOfFileEx
# @ stub UnregisterApplicationRecoveryCallback
@ stdcall -version=0x600+ UnregisterApplicationRestart()
@ stdcall UnregisterConsoleIME()
@ stdcall UnregisterWait(long)
@ stdcall UnregisterWaitEx(long long) kernelbase.UnregisterWaitEx
# @ stub UpdateCalendarDayOfWeek
@ stdcall -version=0x600+ UpdateProcThreadAttribute(ptr long long ptr long ptr ptr) kernelbase.UpdateProcThreadAttribute
@ stdcall UpdateResourceA(long str str long ptr long)
@ stdcall UpdateResourceW(long wstr wstr long ptr long)
@ stdcall UTRegister(long str str str ptr ptr ptr)
@ stdcall UTUnRegister(long)
@ stdcall -version=0x500-0x502 ValidateLCType(long long ptr ptr)
@ stdcall -version=0x500-0x502 ValidateLocale(long)
@ stdcall VDMConsoleOperation(long long)
@ stdcall VDMOperationStarted(long)
@ stdcall VerifyConsoleIoHandle(long)
# @ stub VerifyScripts
@ stdcall VerifyVersionInfoA(ptr long int64)
@ stdcall VerifyVersionInfoW(ptr long int64)
@ stdcall VerLanguageNameA(long str long) kernelbase.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernelbase.VerLanguageNameW
@ stdcall -ret64 VerSetConditionMask(long long long long) ntdll.VerSetConditionMask
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernelbase.VirtualAllocEx
@ stdcall -version=0x600+ VirtualAllocExNuma(long ptr long long long long) kernelbase.VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernelbase.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernelbase.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernelbase.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernelbase.VirtualUnlock
@ stdcall WaitCommEvent(long ptr ptr) kernelbase.WaitCommEvent
@ stdcall WaitForDebugEvent(ptr long) kernelbase.WaitForDebugEvent
@ stdcall -version=0xA00+ WaitForDebugEventEx(ptr long) kernelbase.WaitForDebugEventEx
@ stdcall WaitForMultipleObjects(long ptr long long) kernelbase.WaitForMultipleObjects
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernelbase.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernelbase.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernelbase.WaitForSingleObjectEx
@ stdcall -version=0x600+ WaitForThreadpoolIoCallbacks(ptr long) ntdll.TpWaitForIoCompletion
@ stdcall -version=0x600+ WaitForThreadpoolTimerCallbacks(ptr long) ntdll.TpWaitForTimer
@ stdcall -version=0x600+ WaitForThreadpoolWaitCallbacks(ptr long) ntdll.TpWaitForWait
@ stdcall -version=0x600+ WaitForThreadpoolWorkCallbacks(ptr long) ntdll.TpWaitForWork
@ stdcall WaitNamedPipeA(str long)
@ stdcall WaitNamedPipeW(wstr long)
@ stdcall -version=0x600+ WakeAllConditionVariable(ptr) ntdll.RtlWakeAllConditionVariable
@ stdcall -version=0x600+ WakeConditionVariable(ptr) ntdll.RtlWakeConditionVariable
@ stdcall -version=0x600+ WerGetFlags(ptr ptr) kernelbase.WerGetFlags
# @ stub WerpCleanupMessageMapping
# @ stub WerpInitiateRemoteRecovery
# @ stub WerpNotifyLoadStringResource
# @ stub WerpNotifyLoadStringResourceEx
# @ stub WerpNotifyUseStringResource
# @ stub WerpStringLookup
@ stdcall -version=0x600+ WerRegisterFile(wstr long long) kernelbase.WerRegisterFile
@ stdcall -version=0x600+ WerRegisterMemoryBlock(ptr long) kernelbase.WerRegisterMemoryBlock
@ stdcall WerRegisterRuntimeExceptionModule(wstr ptr) kernelbase.WerRegisterRuntimeExceptionModule
@ stdcall -version=0x600+ WerSetFlags(long) kernelbase.WerSetFlags
@ stdcall -version=0x600+ WerUnregisterFile(wstr) kernelbase.WerUnregisterFile
@ stdcall -version=0x600+ WerUnregisterMemoryBlock(ptr) kernelbase.WerUnregisterMemoryBlock
@ stdcall WerUnregisterRuntimeExceptionModule(wstr ptr) kernelbase.WerUnregisterRuntimeExceptionModule
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernelbase.WideCharToMultiByte
@ stdcall WinExec(str long)
@ stdcall Wow64DisableWow64FsRedirection(ptr) kernelbase.Wow64DisableWow64FsRedirection
@ stdcall Wow64EnableWow64FsRedirection(long) kernelbase.Wow64EnableWow64FsRedirection
@ stdcall -version=0x600+ Wow64GetThreadContext(long ptr) kernelbase.Wow64GetThreadContext
@ stdcall Wow64GetThreadSelectorEntry(long long ptr)
@ stdcall Wow64RevertWow64FsRedirection(ptr) kernelbase.Wow64RevertWow64FsRedirection
@ stdcall -version=0x600+ Wow64SetThreadContext(long ptr) kernelbase.Wow64SetThreadContext
# @ stub Wow64SuspendThread
@ stdcall WriteConsoleA(long ptr long ptr ptr) kernelbase.WriteConsoleA
@ stdcall WriteConsoleInputA(long ptr long ptr) kernelbase.WriteConsoleInputA
@ stdcall WriteConsoleInputVDMA(long long long long)
@ stdcall WriteConsoleInputVDMW(long long long long)
@ stdcall WriteConsoleInputW(long ptr long ptr) kernelbase.WriteConsoleInputW
@ stdcall WriteConsoleOutputA(long ptr long long ptr) kernelbase.WriteConsoleOutputA
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr) kernelbase.WriteConsoleOutputAttribute
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterA
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr) kernelbase.WriteConsoleOutputCharacterW
@ stdcall WriteConsoleOutputW(long ptr long long ptr) kernelbase.WriteConsoleOutputW
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernelbase.WriteConsoleW
@ stdcall WriteFile(long ptr long ptr ptr) kernelbase.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernelbase.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernelbase.WriteFileGather
@ stdcall WritePrivateProfileSectionA(str str str)
@ stdcall WritePrivateProfileSectionW(wstr wstr wstr)
@ stdcall WritePrivateProfileStringA(str str str str)
@ stdcall WritePrivateProfileStringW(wstr wstr wstr wstr)
@ stdcall WritePrivateProfileStructA(str str ptr long str)
@ stdcall WritePrivateProfileStructW(wstr wstr ptr long wstr)
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernelbase.WriteProcessMemory
@ stdcall WriteProfileSectionA(str str)
@ stdcall WriteProfileSectionW(wstr wstr)
@ stdcall WriteProfileStringA(str str str)
@ stdcall WriteProfileStringW(wstr wstr wstr)
@ stdcall WriteTapemark(ptr long long long)
@ stdcall WTSGetActiveConsoleSessionId()
@ stdcall ZombifyActCtx(ptr) kernelbase.ZombifyActCtx
@ stdcall -arch=x86_64,arm64 __C_specific_handler() ntdll.__C_specific_handler
@ stdcall -arch=x86_64,arm64 __chkstk() ntdll.__chkstk
;@ stdcall -arch=x86_64 __misaligned_access() ntdll.__misaligned_access
@ stdcall _hread(long ptr long)
@ stdcall _hwrite(long ptr long)
@ stdcall _lclose(long)
@ stdcall _lcreat(str long)
@ stdcall _llseek(long long long)
@ stdcall -arch=x86_64,arm64 _local_unwind() ntdll._local_unwind
@ stdcall _lopen(str long)
@ stdcall _lread(long ptr long) _hread
@ stdcall _lwrite(long ptr long) _hwrite
@ stdcall lstrcat(str str) lstrcatA
@ stdcall lstrcatA(str str)
@ stdcall lstrcatW(wstr wstr)
@ stdcall lstrcmp(str str) lstrcmpA
@ stdcall lstrcmpA(str str)
@ stdcall lstrcmpW(wstr wstr)
@ stdcall lstrcmpi(str str) lstrcmpiA
@ stdcall lstrcmpiA(str str)
@ stdcall lstrcmpiW(wstr wstr)
@ stdcall lstrcpy(ptr str) lstrcpyA
@ stdcall lstrcpyA(ptr str)
@ stdcall lstrcpyW(ptr wstr)
@ stdcall lstrcpyn(ptr str long) lstrcpynA
@ stdcall lstrcpynA(ptr str long)
@ stdcall lstrcpynW(ptr wstr long)
@ stdcall lstrlen(str) lstrlenA
@ stdcall lstrlenA(str)
@ stdcall lstrlenW(wstr)
;@ stdcall -arch=x86_64 uaw_lstrcmpW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrcmpiW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrlenW(wstr)
;@ stdcall -arch=x86_64 uaw_wcschr(wstr long)
;@ stdcall -arch=x86_64 uaw_wcscpy(ptr wstr)
;@ stdcall -arch=x86_64 uaw_wcsicmp(wstr wstr)
;@ stdcall -arch=x86_64 uaw_wcslen(wstr)
;@ stdcall -arch=x86_64 uaw_wcsrchr(wstr long)
