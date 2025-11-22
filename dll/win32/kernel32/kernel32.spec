@ stdcall -version=0x600+ AcquireSRWLockExclusive(ptr) NTDLL.RtlAcquireSRWLockExclusive
@ stdcall -version=0x600+ AcquireSRWLockShared(ptr) NTDLL.RtlAcquireSRWLockShared
@ stdcall ActivateActCtx(ptr ptr) kernel32_win7.ActivateActCtx
@ stdcall AddAtomA(str) kernel32_win7.AddAtomA
@ stdcall AddAtomW(wstr) kernel32_win7.AddAtomW
@ stdcall AddConsoleAliasA(str str str) kernel32_win7.AddConsoleAliasA ;check
@ stdcall AddConsoleAliasW(wstr wstr wstr) kernel32_win7.AddConsoleAliasW ;check
@ stdcall AddLocalAlternateComputerNameA(str ptr) kernel32_win7.AddLocalAlternateComputerNameA
@ stdcall AddLocalAlternateComputerNameW(wstr ptr) kernel32_win7.AddLocalAlternateComputerNameW
@ stdcall AddRefActCtx(ptr) kernel32_win7.AddRefActCtx
@ stdcall -stub -version=0x600+ AddSIDToBoundaryDescriptor(ptr ptr)
@ stdcall -stub -version=0x600+ AddSecureMemoryCacheCallback(ptr)
@ stdcall AddVectoredContinueHandler(long ptr) ntdll.RtlAddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) ntdll.RtlAddVectoredExceptionHandler
@ stdcall -stub -version=0x600+ AdjustCalendarDate(ptr long long)
@ stdcall AllocConsole() kernel32_win7.AllocConsole
@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernel32_win7.AllocateUserPhysicalPages
@ stdcall -stub -version=0x600+ AllocateUserPhysicalPagesNuma(ptr ptr ptr long)
@ stdcall -version=0x600+ ApplicationRecoveryFinished(long)
@ stdcall -version=0x600+ ApplicationRecoveryInProgress(ptr)
@ stdcall AreFileApisANSI() kernel32_win7.AreFileApisANSI
@ stdcall AssignProcessToJobObject(ptr ptr)
@ stdcall AttachConsole(long) kernel32_win7.AttachConsole
@ stdcall BackupRead(ptr ptr long ptr long long ptr) kernel32_win7.BackupRead
@ stdcall BackupSeek(ptr long long ptr ptr ptr) kernel32_win7.BackupSeek
@ stdcall BackupWrite(ptr ptr long ptr long long ptr) kernel32_win7.BackupWrite
@ stdcall BaseCheckAppcompatCache(wstr ptr wstr ptr) kernel32_win7.BaseCheckAppcompatCache
@ stdcall BaseCheckRunApp(long ptr long long long long long long long long) kernel32_win7.BaseCheckRunApp
@ stdcall BaseCleanupAppcompatCacheSupport(ptr) kernel32_win7.BaseCleanupAppcompatCacheSupport
@ stdcall BaseDumpAppcompatCache() kernel32_win7.BaseDumpAppcompatCache
@ stdcall BaseFlushAppcompatCache() kernel32_win7.BaseFlushAppcompatCache
@ stub -version=0x600+ BaseGenerateAppCompatData
@ stdcall BaseInitAppcompatCacheSupport() kernel32_win7.BaseInitAppcompatCacheSupport
@ stdcall BaseIsAppcompatInfrastructureDisabled() kernel32_win7.BaseIsAppcompatInfrastructureDisabled
@ stdcall -version=0x501-0x502 BaseProcessInitPostImport() kernel32_win7.BaseProcessInitPostImport
@ stdcall -version=0x600+ BaseProcessInitPostImport() # HACK: This export is dynamicaly imported by ntdll kernel32_win7.BaseProcessInitPostImport
;@ stdcall -version=0x502 -arch=x86_64 BaseProcessStart()
@ stdcall BaseQueryModuleData(str str ptr ptr ptr) ;check kernel32_win7.BaseQueryModuleData
@ stub -version=0x600+ BaseThreadInitThunk
;@ stdcall -version=0x502 -arch=x86_64 BaseThreadStart()
@ stdcall BaseUpdateAppcompatCache(long long long) kernel32_win7.BaseUpdateAppcompatCache
@ stdcall BasepCheckBadapp(long ptr long long long long long long long) kernel32_win7.BasepCheckBadapp
@ stdcall BasepCheckWinSaferRestrictions(long long long long long long) kernel32_win7.BasepCheckWinSaferRestrictions
@ stdcall BasepFreeAppCompatData(ptr ptr) kernel32_win7.BasepFreeAppCompatData
@ stdcall Beep(long long) kernel32_win7.Beep
@ stdcall BeginUpdateResourceA(str long)
@ stdcall BeginUpdateResourceW(wstr long)
@ stdcall BindIoCompletionCallback(long ptr long) kernel32_win7.BindIoCompletionCallback
@ stdcall BuildCommDCBA(str ptr) kernel32_win7.BuildCommDCBA
@ stdcall BuildCommDCBAndTimeoutsA(str ptr ptr) kernel32_win7.BuildCommDCBAndTimeoutsA
@ stdcall BuildCommDCBAndTimeoutsW(wstr ptr ptr) kernel32_win7.BuildCommDCBAndTimeoutsW
@ stdcall BuildCommDCBW(wstr ptr) kernel32_win7.BuildCommDCBW
@ stdcall CallNamedPipeA(str ptr long ptr long ptr long) kernel32_win7.CallNamedPipeA
@ stdcall CallNamedPipeW(wstr ptr long ptr long ptr long) kernel32_win7.CallNamedPipeW
@ stdcall -stub -version=0x600+ CallbackMayRunLong(ptr)
@ stdcall CancelDeviceWakeupRequest(long) kernel32_win7.CancelDeviceWakeupRequest
@ stdcall CancelIo(long) kernel32_win7.CancelIo
@ stdcall -stub -version=0x600+ CancelIoEx(ptr ptr)
@ stdcall -stub -version=0x600+ CancelSynchronousIo(ptr)
@ stdcall -stub -version=0x600+ CancelThreadpoolIo(ptr)
@ stdcall CancelTimerQueueTimer(long long) kernel32_win7.CancelTimerQueueTimer
@ stdcall CancelWaitableTimer(long) kernel32_win7.CancelWaitableTimer
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernel32_win7.ChangeTimerQueueTimer
@ stdcall -stub -version=0x600+ CheckElevation(ptr ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ CheckElevationEnabled(ptr)
@ stub -version=0x600+ CheckForReadOnlyResource
@ stdcall CheckNameLegalDOS8Dot3A(str str long long long) kernel32_win7.CheckNameLegalDOS8Dot3A
@ stdcall CheckNameLegalDOS8Dot3W(wstr str long long long) kernel32_win7.CheckNameLegalDOS8Dot3W
@ stdcall CheckRemoteDebuggerPresent(long ptr) kernel32_win7.CheckRemoteDebuggerPresent
@ stdcall ClearCommBreak(long) kernel32_win7.ClearCommBreak
@ stdcall ClearCommError(long ptr ptr) kernel32_win7.ClearCommError
@ stdcall CloseConsoleHandle(long) kernel32_win7.CloseConsoleHandle
@ stdcall CloseHandle(long) kernel32_win7.CloseHandle
@ stdcall -stub -version=0x600+ ClosePrivateNamespace(ptr long)
@ stdcall CloseProfileUserMapping() kernel32_win7.CloseProfileUserMapping
@ stdcall -stub -version=0x600+ CloseThreadpool(ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolCleanupGroup(ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolCleanupGroupMembers(ptr long ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolIo(ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolTimer(ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolWait(ptr)
@ stdcall -stub -version=0x600+ CloseThreadpoolWork(ptr)
@ stdcall CmdBatNotification(long) kernel32_win7.CmdBatNotification
@ stdcall CommConfigDialogA(str long ptr) kernel32_win7.CommConfigDialogA
@ stdcall CommConfigDialogW(wstr long ptr) kernel32_win7.CommConfigDialogW
@ stdcall -stub -version=0x600+ CompareCalendarDates(ptr ptr ptr)
@ stdcall CompareFileTime(ptr ptr) kernel32_win7.CompareFileTime
@ stdcall CompareStringA(long long str long str long) kernel32_win7.CompareStringA
@ stdcall -version=0x600+ CompareStringEx(wstr long wstr long wstr long ptr ptr ptr)
@ stdcall -version=0x600+ CompareStringOrdinal(wstr long wstr long long)
@ stdcall CompareStringW(long long wstr long wstr long) kernel32_win7.CompareStringW
@ stdcall ConnectNamedPipe(long ptr) kernel32_win7.ConnectNamedPipe
;@ stdcall -arch=x86_64 ConsoleIMERoutine()
@ stdcall ConsoleMenuControl(long long long) kernel32_win7.ConsoleMenuControl
@ stdcall ContinueDebugEvent(long long long) kernel32_win7.ContinueDebugEvent
@ stdcall -stub -version=0x600+ ConvertCalDateTimeToSystemTime(ptr ptr)
@ stdcall ConvertDefaultLocale(long) kernel32_win7.ConvertDefaultLocale
@ stdcall ConvertFiberToThread() kernel32_win7.ConvertFiberToThread
@ stdcall -stub -version=0x600+ ConvertNLSDayOfWeekToWin32DayOfWeek(long)
@ stdcall -stub -version=0x600+ ConvertSystemTimeToCalDateTime(ptr long ptr)
@ stdcall ConvertThreadToFiber(ptr) kernel32_win7.ConvertThreadToFiber
@ stdcall ConvertThreadToFiberEx(ptr long) kernel32_win7.ConvertThreadToFiberEx
@ stdcall CopyFileA(str str long) kernel32_win7.CopyFileA
@ stdcall CopyFileExA(str str ptr ptr ptr long) kernel32_win7.CopyFileExA
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernel32_win7.CopyFileExW
@ stdcall -stub -version=0x600+ CopyFileTransactedA(str str ptr ptr ptr long ptr)
@ stdcall -stub -version=0x600+ CopyFileTransactedW(wstr wstr ptr ptr ptr long ptr)
@ stdcall CopyFileW(wstr wstr long) kernel32_win7.CopyFileW
@ stdcall CopyLZFile(long long) LZCopy
@ stdcall CreateActCtxA(ptr) kernel32_win7.CreateActCtxA
@ stdcall CreateActCtxW(ptr) kernel32_win7.CreateActCtxW
@ stdcall -stub -version=0x600+ CreateBoundaryDescriptorA(str long)
@ stdcall -stub -version=0x600+ CreateBoundaryDescriptorW(wstr long)
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr) kernel32_win7.CreateConsoleScreenBuffer
@ stdcall CreateDirectoryA(str ptr) kernel32_win7.CreateDirectoryA
@ stdcall CreateDirectoryExA(str str ptr) kernel32_win7.CreateDirectoryExA
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernel32_win7.CreateDirectoryExW
@ stdcall -stub -version=0x600+ CreateDirectoryTransactedA(str str ptr ptr)
@ stdcall -stub -version=0x600+ CreateDirectoryTransactedW(wstr wstr ptr ptr)
@ stdcall CreateDirectoryW(wstr ptr) kernel32_win7.CreateDirectoryW
@ stdcall CreateEventA(ptr long long str) kernel32_win7.CreateEventA
@ stdcall -stub -version=0x600+ CreateEventExA(ptr str long long)
@ stdcall -stub -version=0x600+ CreateEventExW(ptr wstr long long)
@ stdcall CreateEventW(ptr long long wstr) kernel32_win7.CreateEventW
@ stdcall CreateFiber(long ptr ptr) kernel32_win7.CreateFiber
@ stdcall CreateFiberEx(long long long ptr ptr) kernel32_win7.CreateFiberEx
@ stdcall CreateFileA(str long long ptr long long long) kernel32_win7.CreateFileA
@ stdcall CreateFileMappingA(long ptr long long long str) kernel32_win7.CreateFileMappingA
@ stdcall -stub -version=0x600+ CreateFileMappingNumaA(ptr ptr long long long str long)
@ stdcall -stub -version=0x600+ CreateFileMappingNumaW(ptr ptr long long long wstr long)
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernel32_win7.CreateFileMappingW
@ stdcall -stub -version=0x600+ CreateFileTransactedA(str long long ptr long long ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ CreateFileTransactedW(wstr long long ptr long long ptr ptr ptr ptr)
@ stdcall CreateFileW(wstr long long ptr long long long) kernel32_win7.CreateFileW
@ stdcall CreateHardLinkA(str str ptr) kernel32_win7.CreateHardLinkA
@ stdcall -stub -version=0x600+ CreateHardLinkTransactedA(str str ptr ptr)
@ stdcall -stub -version=0x600+ CreateHardLinkTransactedW(wstr wstr ptr ptr)
@ stdcall CreateHardLinkW(wstr wstr ptr) kernel32_win7.CreateHardLinkW
@ stdcall CreateIoCompletionPort(long long long long) kernel32_win7.CreateIoCompletionPort
@ stdcall CreateJobObjectA(ptr str)
@ stdcall CreateJobObjectW(ptr wstr)
@ stdcall CreateJobSet(long ptr long)
@ stdcall CreateMailslotA(ptr long long ptr)
@ stdcall CreateMailslotW(ptr long long ptr)
@ stdcall CreateMemoryResourceNotification(long) kernel32_win7.CreateMemoryResourceNotification
@ stdcall CreateMutexA(ptr long str) kernel32_win7.CreateMutexA
@ stdcall -stub -version=0x600+ CreateMutexExA(ptr str long long)
@ stdcall -stub -version=0x600+ CreateMutexExW(ptr wstr long long)
@ stdcall CreateMutexW(ptr long wstr) kernel32_win7.CreateMutexW
@ stdcall CreateNamedPipeA(str long long long long long long ptr) kernel32_win7.CreateNamedPipeA
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernel32_win7.CreateNamedPipeW
@ stdcall -version=0x501-0x502 CreateNlsSecurityDescriptor(ptr long long) kernel32_win7.CreateNlsSecurityDescriptor
@ stdcall CreatePipe(ptr ptr ptr long) kernel32_win7.CreatePipe
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceA(ptr ptr str)
@ stdcall -stub -version=0x600+ CreatePrivateNamespaceW(ptr ptr wstr)
@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernel32_win7.CreateProcessA
@ stdcall CreateProcessInternalA(ptr str str ptr ptr long long ptr str ptr ptr long) kernel32_win7.CreateProcessInternalA
@ stdcall CreateProcessInternalW(ptr wstr wstr ptr ptr long long ptr wstr ptr ptr long) kernel32_win7.CreateProcessInternalW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32_win7.CreateProcessW
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernel32_win7.CreateRemoteThread
@ stdcall CreateSemaphoreA(ptr long long str) kernel32_win7.CreateSemaphoreA
@ stdcall -version=0x600+ CreateSemaphoreExA(ptr long long str long long)
@ stdcall -version=0x600+ CreateSemaphoreExW(ptr long long wstr long long)
@ stdcall CreateSemaphoreW(ptr long long wstr) kernel32_win7.CreateSemaphoreW
@ stdcall -i386 CreateSocketHandle() kernel32_win7.CreateSocketHandle
@ stdcall -version=0x600+ CreateSymbolicLinkA(str str long)
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedA(str str long ptr)
@ stdcall -stub -version=0x600+ CreateSymbolicLinkTransactedW(wstr wstr long ptr)
@ stdcall -version=0x600+ CreateSymbolicLinkW(wstr wstr long)
@ stdcall CreateTapePartition(long long long long)
@ stdcall CreateThread(ptr long ptr long long ptr) kernel32_win7.CreateThread
@ stdcall -stub -version=0x600+ CreateThreadpool(ptr)
@ stdcall -stub -version=0x600+ CreateThreadpoolCleanupGroup()
@ stdcall -stub -version=0x600+ CreateThreadpoolIo(ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ CreateThreadpoolTimer(ptr ptr ptr)
@ stdcall -stub -version=0x600+ CreateThreadpoolWait(ptr ptr ptr)
@ stdcall -stub -version=0x600+ CreateThreadpoolWork(ptr ptr ptr)
@ stdcall CreateTimerQueue() kernel32_win7.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernel32_win7.CreateTimerQueueTimer
@ stdcall CreateToolhelp32Snapshot(long long) kernel32_win7.CreateToolhelp32Snapshot
@ stdcall CreateWaitableTimerA(ptr long str) kernel32_win7.CreateWaitableTimerA
@ stub -version=0x600+ CreateWaitableTimerExA
@ stub -version=0x600+ CreateWaitableTimerExW
@ stdcall CreateWaitableTimerW(ptr long wstr) kernel32_win7.CreateWaitableTimerW
;@ stdcall -arch=x86_64 CtrlRoutine()
@ stdcall DeactivateActCtx(long ptr) kernel32_win7.DeactivateActCtx
@ stdcall DebugActiveProcess(long) kernel32_win7.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernel32_win7.DebugActiveProcessStop
@ stdcall DebugBreak() ntdll.DbgBreakPoint
@ stdcall DebugBreakProcess(long) kernel32_win7.DebugBreakProcess
@ stdcall DebugSetProcessKillOnExit(long) kernel32_win7.DebugSetProcessKillOnExit
@ stdcall DecodePointer(ptr) ntdll.RtlDecodePointer
@ stdcall DecodeSystemPointer(ptr) ntdll.RtlDecodeSystemPointer
@ stdcall DefineDosDeviceA(long str str) kernel32_win7.DefineDosDeviceA
@ stdcall DefineDosDeviceW(long wstr wstr) kernel32_win7.DefineDosDeviceW
@ stdcall DelayLoadFailureHook(str str) kernel32_win7.DelayLoadFailureHook
@ stdcall DeleteAtom(long) kernel32_win7.DeleteAtom
@ stub -version=0x600+ DeleteBoundaryDescriptor
@ stdcall DeleteCriticalSection(ptr) ntdll.RtlDeleteCriticalSection
@ stdcall DeleteFiber(ptr) kernel32_win7.DeleteFiber
@ stdcall DeleteFileA(str) kernel32_win7.DeleteFileA
@ stub -version=0x600+ DeleteFileTransactedA
@ stub -version=0x600+ DeleteFileTransactedW
@ stdcall DeleteFileW(wstr) kernel32_win7.DeleteFileW
@ stdcall -stub -version=0x600+ DeleteProcThreadAttributeList(ptr)
@ stdcall DeleteTimerQueue(long) kernel32_win7.DeleteTimerQueue
@ stdcall DeleteTimerQueueEx(long long) kernel32_win7.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernel32_win7.DeleteTimerQueueTimer
@ stdcall DeleteVolumeMountPointA(str) kernel32_win7.DeleteVolumeMountPointA ;check
@ stdcall DeleteVolumeMountPointW(wstr) kernel32_win7.DeleteVolumeMountPointW ;check
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernel32_win7.DeviceIoControl
@ stdcall DisableThreadLibraryCalls(ptr) kernel32_win7.DisableThreadLibraryCalls
@ stub -version=0x600+ DisassociateCurrentThreadFromCallback
@ stdcall DisconnectNamedPipe(long) kernel32_win7.DisconnectNamedPipe
@ stdcall DnsHostnameToComputerNameA(str ptr ptr) kernel32_win7.DnsHostnameToComputerNameA
@ stdcall DnsHostnameToComputerNameW(wstr ptr ptr) kernel32_win7.DnsHostnameToComputerNameW
@ stdcall DosDateTimeToFileTime(long long ptr) kernel32_win7.DosDateTimeToFileTime
@ stdcall DosPathToSessionPathA(long str str) kernel32_win7.DosPathToSessionPathA
@ stdcall DosPathToSessionPathW(long wstr wstr) kernel32_win7.DosPathToSessionPathW
@ stdcall DuplicateConsoleHandle(long long long long) kernel32_win7.DuplicateConsoleHandle
@ stdcall DuplicateHandle(long long long ptr long long long) kernel32_win7.DuplicateHandle
@ stdcall EncodePointer(ptr) ntdll.RtlEncodePointer
@ stdcall EncodeSystemPointer(ptr) ntdll.RtlEncodeSystemPointer
@ stdcall EndUpdateResourceA(long long)
@ stdcall EndUpdateResourceW(long long)
@ stdcall EnterCriticalSection(ptr) ntdll.RtlEnterCriticalSection
@ stdcall EnumCalendarInfoA(ptr long long long) kernel32_win7.EnumCalendarInfoA
@ stdcall EnumCalendarInfoExA(ptr long long long) kernel32_win7.EnumCalendarInfoExA
@ stdcall -version=0x600+ EnumCalendarInfoExEx(ptr wstr long wstr long long)
@ stdcall EnumCalendarInfoExW(ptr long long long) kernel32_win7.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernel32_win7.EnumCalendarInfoW
@ stdcall EnumDateFormatsA(ptr long long) kernel32_win7.EnumDateFormatsA
@ stdcall EnumDateFormatsExA(ptr long long) kernel32_win7.EnumDateFormatsExA
@ stdcall -version=0x600+ EnumDateFormatsExEx(ptr wstr long long)
@ stdcall EnumDateFormatsExW(ptr long long) kernel32_win7.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernel32_win7.EnumDateFormatsW
@ stdcall EnumLanguageGroupLocalesA(ptr long long ptr) kernel32_win7.EnumLanguageGroupLocalesA
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr) kernel32_win7.EnumLanguageGroupLocalesW
@ stdcall EnumResourceLanguagesA(long str str ptr long)
@ stdcall -version=0x600+ EnumResourceLanguagesExA(long str str ptr long long long) kernel32_win7.EnumResourceLanguagesExA
@ stdcall -version=0x600+ EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernel32_win7.EnumResourceLanguagesExW
@ stdcall EnumResourceLanguagesW(long wstr wstr ptr long)
@ stdcall EnumResourceNamesA(long str ptr long)
@ stdcall -version=0x600+ EnumResourceNamesExA(long str ptr long long long) kernel32_win7.EnumResourceNamesExA
@ stdcall -version=0x600+ EnumResourceNamesExW(long wstr ptr long long long) kernel32_win7.EnumResourceNamesExW
@ stdcall EnumResourceNamesW(long wstr ptr long) kernel32_win7.EnumResourceNamesW
@ stdcall EnumResourceTypesA(long ptr long)
@ stdcall -version=0x600+ EnumResourceTypesExA(long ptr long long long) kernel32_win7.EnumResourceTypesExA
@ stdcall -version=0x600+ EnumResourceTypesExW(long ptr long long long) kernel32_win7.EnumResourceTypesExW
@ stdcall EnumResourceTypesW(long ptr long)
@ stdcall EnumSystemCodePagesA(ptr long) kernel32_win7.EnumSystemCodePagesA
@ stdcall EnumSystemCodePagesW(ptr long) kernel32_win7.EnumSystemCodePagesW
@ stdcall EnumSystemFirmwareTables(long ptr long) kernel32_win7.EnumSystemFirmwareTables
@ stdcall EnumSystemGeoID(long long ptr) kernel32_win7.EnumSystemGeoID
@ stdcall EnumSystemLanguageGroupsA(ptr long ptr) kernel32_win7.EnumSystemLanguageGroupsA
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr) kernel32_win7.EnumSystemLanguageGroupsW
@ stdcall EnumSystemLocalesA(ptr long) kernel32_win7.EnumSystemLocalesA
@ stdcall -version=0x600+ EnumSystemLocalesEx(ptr long long ptr)
@ stdcall EnumSystemLocalesW(ptr long) kernel32_win7.EnumSystemLocalesW
@ stdcall EnumTimeFormatsA(ptr long long) kernel32_win7.EnumTimeFormatsA
@ stdcall -version=0x600+ EnumTimeFormatsEx(ptr wstr long long)
@ stdcall EnumTimeFormatsW(ptr long long) kernel32_win7.EnumTimeFormatsW
@ stdcall EnumUILanguagesA(ptr long long) kernel32_win7.EnumUILanguagesA
@ stdcall EnumUILanguagesW(ptr long long) kernel32_win7.EnumUILanguagesW
@ stdcall EnumerateLocalComputerNamesA(ptr long str ptr) kernel32_win7.EnumerateLocalComputerNamesA
@ stdcall EnumerateLocalComputerNamesW(ptr long wstr ptr) kernel32_win7.EnumerateLocalComputerNamesW
@ stdcall EraseTape(ptr long long)
@ stdcall EscapeCommFunction(long long) kernel32_win7.EscapeCommFunction
@ stdcall ExitProcess(long) kernel32_win7.ExitProcess
@ stdcall ExitThread(long) kernel32_win7.ExitThread
@ stdcall ExitVDM(long long) kernel32_win7.ExitVDM
@ stdcall ExpandEnvironmentStringsA(str ptr long) kernel32_win7.ExpandEnvironmentStringsA
@ stdcall ExpandEnvironmentStringsW(wstr ptr long) kernel32_win7.ExpandEnvironmentStringsW
@ stdcall ExpungeConsoleCommandHistoryA(long) kernel32_win7.ExpungeConsoleCommandHistoryA
@ stdcall ExpungeConsoleCommandHistoryW(long) kernel32_win7.ExpungeConsoleCommandHistoryW
@ stdcall FatalAppExitA(long str) kernel32_win7.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32_win7.FatalAppExitW
@ stdcall FatalExit(long) kernel32_win7.FatalExit
@ stdcall FileTimeToDosDateTime(ptr ptr ptr) kernel32_win7.FileTimeToDosDateTime
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernel32_win7.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernel32_win7.FileTimeToSystemTime
@ stdcall FillConsoleOutputAttribute(long long long long ptr) kernel32_win7.FillConsoleOutputAttribute
@ stdcall FillConsoleOutputCharacterA(long long long long ptr) kernel32_win7.FillConsoleOutputCharacterA
@ stdcall FillConsoleOutputCharacterW(long long long long ptr) kernel32_win7.FillConsoleOutputCharacterW
@ stdcall FindActCtxSectionGuid(long ptr long ptr ptr) kernel32_win7.FindActCtxSectionGuid
@ stdcall FindActCtxSectionStringA(long ptr long str ptr) kernel32_win7.FindActCtxSectionStringA
@ stdcall FindActCtxSectionStringW(long ptr long wstr ptr) kernel32_win7.FindActCtxSectionStringW
@ stdcall FindAtomA(str) kernel32_win7.FindAtomA
@ stdcall FindAtomW(wstr) kernel32_win7.FindAtomW
@ stdcall FindClose(long) kernel32_win7.FindClose
@ stdcall FindCloseChangeNotification(long) kernel32_win7.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernel32_win7.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernel32_win7.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernel32_win7.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernel32_win7.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernel32_win7.FindFirstFileExW
@ stub -version=0x600+ FindFirstFileNameTransactedW
@ stub -version=0x600+ FindFirstFileNameW
@ stub -version=0x600+ FindFirstFileTransactedA
@ stub -version=0x600+ FindFirstFileTransactedW
@ stdcall FindFirstFileW(wstr ptr) kernel32_win7.FindFirstFileW
@ stub -version=0x600+ FindFirstStreamTransactedW
@ stdcall FindFirstStreamW(wstr ptr ptr long) kernel32_win7.FindFirstStreamW
@ stdcall FindFirstVolumeA(ptr long) kernel32_win7.FindFirstVolumeA
@ stdcall FindFirstVolumeMountPointA(str ptr long) kernel32_win7.FindFirstVolumeMountPointA
@ stdcall FindFirstVolumeMountPointW(wstr ptr long) kernel32_win7.FindFirstVolumeMountPointW
@ stdcall FindFirstVolumeW(ptr long) kernel32_win7.FindFirstVolumeW
@ stub -version=0x600+ FindNLSString
@ stub -version=0x600+ FindNLSStringEx
@ stdcall FindNextChangeNotification(long) kernel32_win7.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernel32_win7.FindNextFileA
@ stub -version=0x600+ FindNextFileNameW
@ stdcall FindNextFileW(long ptr) kernel32_win7.FindNextFileW
@ stdcall FindNextStreamW(ptr ptr) kernel32_win7.FindNextStreamW
@ stdcall FindNextVolumeA(long ptr long) kernel32_win7.FindNextVolumeA
@ stdcall FindNextVolumeMountPointA(long str long) kernel32_win7.FindNextVolumeMountPointA
@ stdcall FindNextVolumeMountPointW(long wstr long) kernel32_win7.FindNextVolumeMountPointW
@ stdcall FindNextVolumeW(long ptr long) kernel32_win7.FindNextVolumeW
@ stdcall FindResourceA(long str str)
@ stdcall FindResourceExA(long str str long)
@ stdcall FindResourceExW(long wstr wstr long) kernel32_win7.FindResourceExW
@ stdcall FindResourceW(long wstr wstr) kernel32_win7.FindResourceW
@ stdcall FindVolumeClose(ptr) kernel32_win7.FindVolumeClose
@ stdcall FindVolumeMountPointClose(ptr) kernel32_win7.FindVolumeMountPointClose
@ stdcall FlsAlloc(ptr) kernel32_win7.FlsAlloc
@ stdcall FlsFree(long) kernel32_win7.FlsFree
@ stdcall FlsGetValue(long) kernel32_win7.FlsGetValue
@ stdcall FlsSetValue(long ptr) kernel32_win7.FlsSetValue
@ stdcall FlushConsoleInputBuffer(long) kernel32_win7.FlushConsoleInputBuffer
@ stdcall FlushFileBuffers(long) kernel32_win7.FlushFileBuffers
@ stdcall FlushInstructionCache(long long long) kernel32_win7.FlushInstructionCache
@ stub -version=0x600+ FlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long) kernel32_win7.FlushViewOfFile
@ stdcall FoldStringA(long str long ptr long) kernel32_win7.FoldStringA
@ stdcall FoldStringW(long wstr long ptr long) kernel32_win7.FoldStringW
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernel32_win7.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernel32_win7.FormatMessageW
@ stdcall FreeConsole() kernel32_win7.FreeConsole
@ stdcall FreeEnvironmentStringsA(ptr) kernel32_win7.FreeEnvironmentStringsA
@ stdcall FreeEnvironmentStringsW(ptr) kernel32_win7.FreeEnvironmentStringsW
@ stdcall FreeLibrary(long) kernel32_win7.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernel32_win7.FreeLibraryAndExitThread
@ stub -version=0x600+ FreeLibraryWhenCallbackReturns
@ stdcall FreeResource(long) kernel32_win7.FreeResource
@ stdcall FreeUserPhysicalPages(long long long) kernel32_win7.FreeUserPhysicalPages
@ stdcall GenerateConsoleCtrlEvent(long long) kernel32_win7.GenerateConsoleCtrlEvent
@ stdcall GetACP() kernel32_win7.GetACP
@ stdcall -version=0x600+ GetApplicationRecoveryCallback(ptr ptr ptr ptr ptr)
@ stub -version=0x600+ GetApplicationRestartSettings
@ stdcall GetAtomNameA(long ptr long) kernel32_win7.GetAtomNameA
@ stdcall GetAtomNameW(long ptr long) kernel32_win7.GetAtomNameW
@ stdcall GetBinaryType(str ptr) kernel32_win7.GetBinaryTypeA
@ stdcall GetBinaryTypeA(str ptr) kernel32_win7.GetBinaryTypeA
@ stdcall GetBinaryTypeW(wstr ptr) kernel32_win7.GetBinaryTypeW
@ stdcall -version=0x501-0x600 GetCPFileNameFromRegistry(long wstr long) kernel32_win7.GetCPFileNameFromRegistry
@ stdcall GetCPInfo(long ptr) kernel32_win7.GetCPInfo
@ stdcall GetCPInfoExA(long long ptr) kernel32_win7.GetCPInfoExA
@ stdcall GetCPInfoExW(long long ptr) kernel32_win7.GetCPInfoExW
@ stub -version=0x600+ GetCalendarDateFormat
@ stub -version=0x600+ GetCalendarDateFormatEx
@ stub -version=0x600+ GetCalendarDaysInMonth
@ stub -version=0x600+ GetCalendarDifferenceInDays
@ stdcall GetCalendarInfoA(long long long ptr long ptr) kernel32_win7.GetCalendarInfoA
@ stdcall -version=0x600+ GetCalendarInfoEx(wstr long wstr long wstr long ptr)
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernel32_win7.GetCalendarInfoW
@ stub -version=0x600+ GetCalendarMonthsInYear
@ stub -version=0x600+ GetCalendarSupportedDateRange
@ stub -version=0x600+ GetCalendarWeekNumber
@ stdcall GetComPlusPackageInstallStatus() kernel32_win7.GetComPlusPackageInstallStatus
@ stdcall GetCommConfig(long ptr long) kernel32_win7.GetCommConfig
@ stdcall GetCommMask(long ptr) kernel32_win7.GetCommMask
@ stdcall GetCommModemStatus(long ptr) kernel32_win7.GetCommModemStatus
@ stdcall GetCommProperties(long ptr) kernel32_win7.GetCommProperties
@ stdcall GetCommState(long ptr) kernel32_win7.GetCommState
@ stdcall GetCommTimeouts(long ptr) kernel32_win7.GetCommTimeouts
@ stdcall GetCommandLineA() kernel32_win7.GetCommandLineA
@ stdcall GetCommandLineW() kernel32_win7.GetCommandLineW
@ stdcall GetCompressedFileSizeA(long ptr) kernel32_win7.GetCompressedFileSizeA
@ stub -version=0x600+ GetCompressedFileSizeTransactedA
@ stub -version=0x600+ GetCompressedFileSizeTransactedW
@ stdcall GetCompressedFileSizeW(long ptr) kernel32_win7.GetCompressedFileSizeW
@ stdcall GetComputerNameA(ptr ptr) kernel32_win7.GetComputerNameA
@ stdcall GetComputerNameExA(long ptr ptr) kernel32_win7.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernel32_win7.GetComputerNameExW
@ stdcall GetComputerNameW(ptr ptr) kernel32_win7.GetComputerNameW
@ stdcall GetConsoleAliasA(str str long str) kernel32_win7.GetConsoleAliasA
@ stdcall GetConsoleAliasExesA(str long) kernel32_win7.GetConsoleAliasExesA
@ stdcall GetConsoleAliasExesLengthA() kernel32_win7.GetConsoleAliasExesLengthA
@ stdcall GetConsoleAliasExesLengthW() kernel32_win7.GetConsoleAliasExesLengthW
@ stdcall GetConsoleAliasExesW(wstr long) kernel32_win7.GetConsoleAliasExesW
@ stdcall GetConsoleAliasW(wstr ptr long wstr) kernel32_win7.GetConsoleAliasW
@ stdcall GetConsoleAliasesA(str long str) kernel32_win7.GetConsoleAliasesA
@ stdcall GetConsoleAliasesLengthA(str) kernel32_win7.GetConsoleAliasesLengthA
@ stdcall GetConsoleAliasesLengthW(wstr) kernel32_win7.GetConsoleAliasesLengthW
@ stdcall GetConsoleAliasesW(wstr long wstr) kernel32_win7.GetConsoleAliasesW
@ stdcall GetConsoleCP() kernel32_win7.GetConsoleCP
@ stdcall GetConsoleCharType(long long ptr) kernel32_win7.GetConsoleCharType
@ stdcall GetConsoleCommandHistoryA(long long long) kernel32_win7.GetConsoleCommandHistoryA
@ stdcall GetConsoleCommandHistoryLengthA(long) kernel32_win7.GetConsoleCommandHistoryLengthA
@ stdcall GetConsoleCommandHistoryLengthW(long) kernel32_win7.GetConsoleCommandHistoryLengthW
@ stdcall GetConsoleCommandHistoryW(long long long) kernel32_win7.GetConsoleCommandHistoryW
@ stdcall GetConsoleCursorInfo(long ptr) kernel32_win7.GetConsoleCursorInfo
@ stdcall GetConsoleCursorMode(long ptr ptr) kernel32_win7.GetConsoleCursorMode
@ stdcall GetConsoleDisplayMode(ptr) kernel32_win7.GetConsoleDisplayMode
@ stdcall GetConsoleFontInfo(long long long ptr) kernel32_win7.GetConsoleFontInfo
@ stdcall GetConsoleFontSize(long long) kernel32_win7.GetConsoleFontSize
@ stdcall GetConsoleHardwareState(long long ptr) kernel32_win7.GetConsoleHardwareState
@ stdcall -version=0x600+ GetConsoleHistoryInfo(ptr)
@ stdcall GetConsoleInputExeNameA(long ptr) kernel32_win7.GetConsoleInputExeNameA
@ stdcall GetConsoleInputExeNameW(long ptr) kernel32_win7.GetConsoleInputExeNameW
@ stdcall GetConsoleInputWaitHandle() kernel32_win7.GetConsoleInputWaitHandle
@ stdcall GetConsoleKeyboardLayoutNameA(ptr) kernel32_win7.GetConsoleKeyboardLayoutNameA
@ stdcall GetConsoleKeyboardLayoutNameW(ptr) kernel32_win7.GetConsoleKeyboardLayoutNameW
@ stdcall GetConsoleMode(long ptr) kernel32_win7.GetConsoleMode
@ stdcall GetConsoleNlsMode(long ptr) kernel32_win7.GetConsoleNlsMode
@ stdcall -version=0x600+ GetConsoleOriginalTitleA(ptr long)
@ stdcall -version=0x600+ GetConsoleOriginalTitleW(ptr long)
@ stdcall GetConsoleOutputCP() kernel32_win7.GetConsoleOutputCP
@ stdcall GetConsoleProcessList(ptr long) kernel32_win7.GetConsoleProcessList
@ stdcall GetConsoleScreenBufferInfo(long ptr) kernel32_win7.GetConsoleScreenBufferInfo
@ stdcall -version=0x600+ GetConsoleScreenBufferInfoEx(ptr ptr)
@ stdcall GetConsoleSelectionInfo(ptr) kernel32_win7.GetConsoleSelectionInfo
@ stdcall GetConsoleTitleA(ptr long) kernel32_win7.GetConsoleTitleA
@ stdcall GetConsoleTitleW(ptr long) kernel32_win7.GetConsoleTitleW
@ stdcall GetConsoleWindow() kernel32_win7.GetConsoleWindow
@ stdcall GetCurrencyFormatA(long long str ptr str long) kernel32_win7.GetCurrencyFormatA
@ stdcall -version=0x600+ GetCurrencyFormatEx(wstr long wstr ptr wstr long)
@ stdcall GetCurrencyFormatW(long long wstr ptr wstr long) kernel32_win7.GetCurrencyFormatW
@ stdcall GetCurrentActCtx(ptr) kernel32_win7.GetCurrentActCtx
@ stdcall GetCurrentConsoleFont(long long ptr) kernel32_win7.GetCurrentConsoleFont
@ stdcall -version=0x600+ GetCurrentConsoleFontEx(ptr long ptr)
@ stdcall GetCurrentDirectoryA(long ptr) kernel32_win7.GetCurrentDirectoryA
@ stdcall GetCurrentDirectoryW(long ptr) kernel32_win7.GetCurrentDirectoryW
@ stdcall -version=0x602+ GetCurrentPackageId(ptr ptr)
@ stdcall -norelay GetCurrentProcess() kernel32_win7.GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernel32_win7.GetCurrentProcessId
@ stdcall GetCurrentProcessorNumber() kernel32_win7.GetCurrentProcessorNumber
@ stdcall -norelay GetCurrentThread() kernel32_win7.GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernel32_win7.GetCurrentThreadId
@ stdcall GetDateFormatA(long long ptr str ptr long) kernel32_win7.GetDateFormatA
@ stdcall -version=0x600+ GetDateFormatEx(wstr long ptr wstr wstr long wstr)
@ stdcall GetDateFormatW(long long ptr wstr ptr long) kernel32_win7.GetDateFormatW
@ stdcall GetDefaultCommConfigA(str ptr long) kernel32_win7.GetDefaultCommConfigA
@ stdcall GetDefaultCommConfigW(wstr ptr long) kernel32_win7.GetDefaultCommConfigW
@ stdcall -version=0x501-0x502 GetDefaultSortkeySize(ptr) kernel32_win7.GetDefaultSortkeySize
@ stdcall GetDevicePowerState(long ptr) kernel32_win7.GetDevicePowerState
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernel32_win7.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernel32_win7.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32_win7.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernel32_win7.GetDiskFreeSpaceW
@ stdcall GetDllDirectoryA(long ptr) kernel32_win7.GetDllDirectoryA
@ stdcall GetDllDirectoryW(long ptr) kernel32_win7.GetDllDirectoryW
@ stdcall GetDriveTypeA(str) kernel32_win7.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernel32_win7.GetDriveTypeW
@ stub -version=0x600+ GetDurationFormat
@ stub -version=0x600+ GetDurationFormatEx
@ stub -version=0x600+ GetDynamicTimeZoneInformation
@ stdcall GetEnvironmentStrings() kernel32_win7.GetEnvironmentStrings
@ stdcall GetEnvironmentStringsA() kernel32_win7.GetEnvironmentStringsA
@ stdcall GetEnvironmentStringsW() kernel32_win7.GetEnvironmentStringsW
@ stdcall GetEnvironmentVariableA(str ptr long) kernel32_win7.GetEnvironmentVariableA
@ stdcall GetEnvironmentVariableW(wstr ptr long) kernel32_win7.GetEnvironmentVariableW
@ stdcall -version=0x600+ GetErrorMode() kernel32_win7.GetErrorMode
@ stdcall GetExitCodeProcess(long ptr) kernel32_win7.GetExitCodeProcess
@ stdcall GetExitCodeThread(long ptr) kernel32_win7.GetExitCodeThread
@ stdcall GetExpandedNameA(str ptr) kernel32_win7.GetExpandedNameA
@ stdcall GetExpandedNameW(wstr ptr) kernel32_win7.GetExpandedNameW
@ stdcall GetFileAttributesA(str) kernel32_win7.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernel32_win7.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernel32_win7.GetFileAttributesExW
@ stub -version=0x600+ GetFileAttributesTransactedA
@ stub -version=0x600+ GetFileAttributesTransactedW
@ stdcall GetFileAttributesW(wstr) kernel32_win7.GetFileAttributesW
@ stdcall -version=0x600+ GetFileBandwidthReservation(ptr ptr ptr ptr ptr ptr) kernel32_win7.GetFileBandwidthReservation
@ stdcall GetFileInformationByHandle(long ptr) kernel32_win7.GetFileInformationByHandle
@ stdcall -version=0x600+ GetFileInformationByHandleEx(ptr long ptr long) kernel32_win7.GetFileInformationByHandleEx
@ stdcall -version=0x600+ GetFileMUIInfo(long wstr ptr ptr) kernel32_win7.GetFileMUIInfo
@ stdcall -version=0x600+ GetFileMUIPath(long wstr wstr ptr wstr ptr ptr) kernel32_win7.GetFileMUIPath
@ stdcall GetFileSize(long ptr) kernel32_win7.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernel32_win7.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernel32_win7.GetFileTime
@ stdcall GetFileType(long) kernel32_win7.GetFileType
@ stdcall -version=0x600+ GetFinalPathNameByHandleA(ptr str long long) kernel32_win7.GetFinalPathNameByHandleA
@ stdcall -version=0x600+ GetFinalPathNameByHandleW(ptr wstr long long) kernel32_win7.GetFinalPathNameByHandleW
@ stdcall GetFirmwareEnvironmentVariableA(str str ptr long) kernel32_win7.GetFirmwareEnvironmentVariableA
@ stdcall -version=0x602+ GetFirmwareEnvironmentVariableExA(str str ptr long long) kernel32_win7.GetFirmwareEnvironmentVariableExA
@ stdcall -version=0x602+ GetFirmwareEnvironmentVariableExW(wstr wstr ptr long long) kernel32_win7.GetFirmwareEnvironmentVariableExW
@ stdcall GetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernel32_win7.GetFirmwareEnvironmentVariableW
@ stdcall -version=0x602+ GetFirmwareType(ptr) kernel32_win7.GetFirmwareType
@ stdcall GetFullPathNameA(str long ptr ptr) kernel32_win7.GetFullPathNameA
@ stub -version=0x600+ GetFullPathNameTransactedA
@ stub -version=0x600+ GetFullPathNameTransactedW
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernel32_win7.GetFullPathNameW
@ stdcall GetGeoInfoA(long long ptr long long) kernel32_win7.GetGeoInfoA
@ stdcall GetGeoInfoW(long long ptr long long) kernel32_win7.GetGeoInfoW
@ stdcall -i386 GetHandleContext(long) kernel32_win7.GetHandleContext
@ stdcall GetHandleInformation(long ptr) kernel32_win7.GetHandleInformation
@ stdcall GetLargePageMinimum() kernel32_win7.GetLargePageMinimum
@ stdcall GetLargestConsoleWindowSize(long) kernel32_win7.GetLargestConsoleWindowSize
@ stdcall GetLastError() kernel32_win7.GetLastError
@ stdcall -version=0x500-0x502 GetLinguistLangSize(ptr) kernel32_win7.GetLinguistLangSize
@ stdcall GetLocalTime(ptr) kernel32_win7.GetLocalTime
@ stdcall GetLocaleInfoA(long long ptr long) kernel32_win7.GetLocaleInfoA
@ stdcall -version=0x600+ GetLocaleInfoEx(wstr long ptr long) kernel32_win7.GetLocaleInfoEx
@ stdcall GetLocaleInfoW(long long ptr long) kernel32_win7.GetLocaleInfoW
@ stdcall -version=0x600+ IsValidLocaleName(wstr) kernel32_win7.IsValidLocaleName
@ stdcall GetLogicalDriveStringsA(long ptr) kernel32_win7.GetLogicalDriveStringsA
@ stdcall GetLogicalDriveStringsW(long ptr) kernel32_win7.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernel32_win7.GetLogicalDrives
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernel32_win7.GetLogicalProcessorInformation
@ stdcall GetLongPathNameA(str long long) kernel32_win7.GetLongPathNameA
@ stub -version=0x600+ GetLongPathNameTransactedA
@ stub -version=0x600+ GetLongPathNameTransactedW
@ stdcall GetLongPathNameW(wstr long long) kernel32_win7.GetLongPathNameW
@ stdcall GetMailslotInfo(long ptr ptr ptr ptr) kernel32_win7.GetMailslotInfo
@ stdcall GetModuleFileNameA(long ptr long) kernel32_win7.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernel32_win7.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernel32_win7.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernel32_win7.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernel32_win7.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernel32_win7.GetModuleHandleW
@ stdcall GetNLSVersion(long long ptr) kernel32_win7.GetNLSVersion
@ stdcall GetNLSVersionEx(long wstr ptr) kernel32_win7.GetNLSVersionEx
@ stub -version=0x600+ GetNamedPipeAttribute
@ stub -version=0x600+ GetNamedPipeClientComputerNameA
@ stub -version=0x600+ GetNamedPipeClientComputerNameW
@ stdcall -version=0x600+ GetNamedPipeClientProcessId(ptr ptr)
@ stub -version=0x600+ GetNamedPipeClientSessionId
@ stdcall GetNamedPipeHandleStateA(long ptr ptr ptr ptr str long) kernel32_win7.GetNamedPipeHandleStateA
@ stdcall GetNamedPipeHandleStateW(long ptr ptr ptr ptr wstr long) kernel32_win7.GetNamedPipeHandleStateW
@ stdcall GetNamedPipeInfo(long ptr ptr ptr ptr) kernel32_win7.GetNamedPipeInfo
@ stub -version=0x600+ GetNamedPipeServerProcessId
@ stub -version=0x600+ GetNamedPipeServerSessionId
@ stdcall GetNativeSystemInfo(ptr) kernel32_win7.GetNativeSystemInfo
@ stdcall GetNextVDMCommand(long) kernel32_win7.GetNextVDMCommand
@ stdcall -version=0x500-0x502 GetNlsSectionName(long long long str str long) kernel32_win7.GetNlsSectionName
@ stdcall GetNumaAvailableMemoryNode(long ptr) kernel32_win7.GetNumaAvailableMemoryNode
@ stdcall GetNumaHighestNodeNumber(ptr) kernel32_win7.GetNumaHighestNodeNumber
@ stdcall GetNumaNodeProcessorMask(long ptr) kernel32_win7.GetNumaNodeProcessorMask
@ stdcall GetNumaProcessorNode(long ptr) kernel32_win7.GetNumaProcessorNode
@ stub -version=0x600+ GetNumaProximityNode
@ stdcall GetNumberFormatA(long long str ptr ptr long) kernel32_win7.GetNumberFormatA
@ stdcall -version=0x600+ GetNumberFormatEx(wstr long wstr ptr wstr long) kernel32_win7.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernel32_win7.GetNumberFormatW
@ stdcall GetNumberOfConsoleFonts() kernel32_win7.GetNumberOfConsoleFonts
@ stdcall GetNumberOfConsoleInputEvents(long ptr) kernel32_win7.GetNumberOfConsoleInputEvents
@ stdcall GetNumberOfConsoleMouseButtons(ptr) kernel32_win7.GetNumberOfConsoleMouseButtons
@ stdcall GetOEMCP() kernel32_win7.GetOEMCP
@ stdcall GetOverlappedResult(long ptr ptr long) kernel32_win7.GetOverlappedResult
@ stdcall -stub -version=0x600+ GetPhysicallyInstalledSystemMemory(ptr) kernel32_win7.GetPhysicallyInstalledSystemMemory
@ stdcall GetPriorityClass(long) kernel32_win7.GetPriorityClass
@ stdcall GetPrivateProfileIntA(str str long str) kernel32_win7.GetPrivateProfileIntA
@ stdcall GetPrivateProfileIntW(wstr wstr long wstr) kernel32_win7.GetPrivateProfileIntW
@ stdcall GetPrivateProfileSectionA(str ptr long str) kernel32_win7.GetPrivateProfileSectionA
@ stdcall GetPrivateProfileSectionNamesA(ptr long str) kernel32_win7.GetPrivateProfileSectionNamesA
@ stdcall GetPrivateProfileSectionNamesW(ptr long wstr) kernel32_win7.GetPrivateProfileSectionNamesW
@ stdcall GetPrivateProfileSectionW(wstr ptr long wstr) kernel32_win7.GetPrivateProfileSectionW
@ stdcall GetPrivateProfileStringA(str str str ptr long str) kernel32_win7.GetPrivateProfileStringA
@ stdcall GetPrivateProfileStringW(wstr wstr wstr ptr long wstr) kernel32_win7.GetPrivateProfileStringW
@ stdcall GetPrivateProfileStructA(str str ptr long str) kernel32_win7.GetPrivateProfileStructA
@ stdcall GetPrivateProfileStructW(wstr wstr ptr long wstr) kernel32_win7.GetPrivateProfileStructW
@ stdcall GetProcAddress(long str) kernel32_win7.GetProcAddress
@ stdcall GetProcessAffinityMask(long ptr ptr) kernel32_win7.GetProcessAffinityMask
@ stub -version=0x600+ GetProcessDEPPolicy
@ stdcall GetProcessHandleCount(long ptr) kernel32_win7.GetProcessHandleCount
@ stdcall -norelay GetProcessHeap() kernel32_win7.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) kernel32_win7.GetProcessHeaps
@ stdcall GetProcessId(long) kernel32_win7.GetProcessId
@ stdcall GetProcessIdOfThread(ptr) kernel32_win7.GetProcessIdOfThread
@ stdcall GetProcessIoCounters(long ptr) kernel32_win7.GetProcessIoCounters
@ stdcall GetProcessPriorityBoost(long ptr) kernel32_win7.GetProcessPriorityBoost
@ stdcall GetProcessShutdownParameters(ptr ptr) kernel32_win7.GetProcessShutdownParameters
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernel32_win7.GetProcessTimes
@ stdcall GetProcessVersion(long) kernel32_win7.GetProcessVersion
@ stdcall GetProcessWorkingSetSize(long ptr ptr) kernel32_win7.GetProcessWorkingSetSize
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr long) kernel32_win7.GetProcessWorkingSetSizeEx
@ stub -version=0x600+ GetProductInfo
@ stdcall GetProfileIntA(str str long) kernel32_win7.GetProfileIntA
@ stdcall GetProfileIntW(wstr wstr long) kernel32_win7.GetProfileIntW
@ stdcall GetProfileSectionA(str ptr long) kernel32_win7.GetProfileSectionA
@ stdcall GetProfileSectionW(wstr ptr long) kernel32_win7.GetProfileSectionW
@ stdcall GetProfileStringA(str str str ptr long) kernel32_win7.GetProfileStringA
@ stdcall GetProfileStringW(wstr wstr wstr ptr long) kernel32_win7.GetProfileStringW
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernel32_win7.GetQueuedCompletionStatus
@ stub -version=0x600+ GetQueuedCompletionStatusEx
@ stdcall GetShortPathNameA(str ptr long)
@ stdcall GetShortPathNameW(wstr ptr long) kernel32_win7.GetShortPathNameW
@ stdcall GetStartupInfoA(ptr)
@ stdcall GetStartupInfoW(ptr) kernel32_win7.GetStartupInfoW
@ stdcall GetStdHandle(long) kernel32_win7.GetStdHandle
@ stub -version=0x600+ GetStringScripts
@ stdcall GetStringTypeA(long long str long ptr) kernel32_win7.GetStringTypeA
@ stdcall GetStringTypeExA(long long str long ptr) kernel32_win7.GetStringTypeExA
@ stdcall GetStringTypeExW(long long wstr long ptr) kernel32_win7.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernel32_win7.GetStringTypeW
@ stub -version=0x600+ GetSystemDEPPolicy
@ stdcall GetSystemDefaultLCID() kernel32_win7.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLangID() kernel32_win7.GetSystemDefaultLangID
@ stdcall -stub -version=0x600+ GetSystemDefaultLocaleName(ptr long) kernel32_win7.GetSystemDefaultLocaleName
@ stdcall GetSystemDefaultUILanguage() kernel32_win7.GetSystemDefaultUILanguage
@ stdcall GetSystemDirectoryA(ptr long) kernel32_win7.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernel32_win7.GetSystemDirectoryW
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernel32_win7.GetSystemFileCacheSize
@ stdcall GetSystemFirmwareTable(long long ptr long) kernel32_win7.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernel32_win7.GetSystemInfo
@ stdcall GetSystemPowerStatus(ptr) kernel32_win7.GetSystemPowerStatus
@ stdcall -version=0x600+ GetSystemPreferredUILanguages(long ptr wstr ptr) kernel32_win7.GetSystemPreferredUILanguages
@ stdcall GetSystemRegistryQuota(ptr ptr) kernel32_win7.GetSystemRegistryQuota
@ stdcall GetSystemTime(ptr) kernel32_win7.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernel32_win7.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernel32_win7.GetSystemTimeAsFileTime
@ stdcall -version=0x602+ GetSystemTimePreciseAsFileTime(ptr) kernel32_win7.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemTimes(ptr ptr ptr) kernel32_win7.GetSystemTimes
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernel32_win7.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernel32_win7.GetSystemWindowsDirectoryW
@ stdcall GetSystemWow64DirectoryA(ptr long) kernel32_win7.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernel32_win7.GetSystemWow64DirectoryW
@ stdcall GetTapeParameters(ptr long ptr ptr) kernel32_win7.GetTapeParameters
@ stdcall GetTapePosition(ptr long ptr ptr ptr) kernel32_win7.GetTapePosition
@ stdcall GetTapeStatus(ptr) kernel32_win7.GetTapeStatus
@ stdcall GetTempFileNameA(str str long ptr) kernel32_win7.GetTempFileNameA
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernel32_win7.GetTempFileNameW
@ stdcall GetTempPathA(long ptr) kernel32_win7.GetTempPathA
@ stdcall GetTempPathW(long ptr) kernel32_win7.GetTempPathW
@ stdcall GetThreadContext(long ptr) kernel32_win7.GetThreadContext
@ stdcall -stub -version=0x600+ GetThreadErrorMode() kernel32_win7.GetThreadErrorMode
@ stdcall GetThreadIOPendingFlag(long ptr) kernel32_win7.GetThreadIOPendingFlag
@ stdcall GetThreadId(ptr) kernel32_win7.GetThreadId
@ stdcall GetThreadLocale() kernel32_win7.GetThreadLocale
@ stdcall -version=0x600+ GetThreadPreferredUILanguages(long ptr wstr ptr) kernel32_win7.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long) kernel32_win7.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernel32_win7.GetThreadPriorityBoost
@ stdcall GetThreadSelectorEntry(long long ptr) kernel32_win7.GetThreadSelectorEntry
@ stdcall GetThreadTimes(long ptr ptr ptr ptr) kernel32_win7.GetThreadTimes
@ stdcall -version=0x600+ GetThreadUILanguage() kernel32_win7.GetThreadUILanguage
@ stdcall GetTickCount() kernel32_win7.GetTickCount
@ stdcall -version=0x600+ -ret64 GetTickCount64() kernel32_win7.GetTickCount64
@ stdcall GetTimeFormatA(long long ptr str ptr long) kernel32_win7.GetTimeFormatA
@ stdcall -version=0x600+ GetTimeFormatEx(wstr long ptr wstr wstr long) kernel32_win7.GetTimeFormatEx
@ stdcall GetTimeFormatW(long long ptr wstr ptr long) kernel32_win7.GetTimeFormatW
@ stdcall GetTimeZoneInformation(ptr) kernel32_win7.GetTimeZoneInformation
@ stdcall -stub -version=0x600+ GetTimeZoneInformationForYear(long ptr ptr) kernel32_win7.GetTimeZoneInformationForYear
@ stdcall -version=0x600+ GetUILanguageInfo(long wstr wstr ptr ptr) kernel32_win7.GetUILanguageInfo
@ stdcall GetUserDefaultLCID() kernel32_win7.GetUserDefaultLCID
@ stdcall GetUserDefaultLangID() kernel32_win7.GetUserDefaultLangID
@ stdcall -version=0x600+ GetUserDefaultLocaleName(wstr long) kernel32_win7.GetUserDefaultLocaleName
@ stdcall GetUserDefaultUILanguage() kernel32_win7.GetUserDefaultUILanguage
@ stdcall GetUserGeoID(long) kernel32_win7.GetUserGeoID
@ stdcall -version=0x600+ GetUserPreferredUILanguages(long ptr wstr ptr) kernel32_win7.GetUserPreferredUILanguages
@ stdcall GetVDMCurrentDirectories(long long) kernel32_win7.GetVDMCurrentDirectories
@ stdcall GetVersion() kernel32_win7.GetVersion
@ stdcall GetVersionExA(ptr) kernel32_win7.GetVersionExA
@ stdcall GetVersionExW(ptr) kernel32_win7.GetVersionExW
@ stdcall GetVolumeInformationA(str ptr long ptr ptr ptr ptr long) kernel32_win7.GetVolumeInformationA
@ stub -version=0x600+ GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernel32_win7.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointA(str ptr long) kernel32_win7.GetVolumeNameForVolumeMountPointA
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernel32_win7.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameA(str ptr long) kernel32_win7.GetVolumePathNameA
@ stdcall GetVolumePathNameW(wstr ptr long) kernel32_win7.GetVolumePathNameW
@ stdcall GetVolumePathNamesForVolumeNameA(str str long ptr) kernel32_win7.GetVolumePathNamesForVolumeNameA
@ stdcall GetVolumePathNamesForVolumeNameW(wstr wstr long ptr) kernel32_win7.GetVolumePathNamesForVolumeNameW
@ stdcall GetWindowsDirectoryA(ptr long) kernel32_win7.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernel32_win7.GetWindowsDirectoryW
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernel32_win7.GetWriteWatch
@ stdcall GlobalAddAtomA(str) kernel32_win7.GlobalAddAtomA
@ stdcall GlobalAddAtomW(wstr) kernel32_win7.GlobalAddAtomW
@ stdcall GlobalAlloc(long long) kernel32_win7.GlobalAlloc
@ stdcall GlobalCompact(long) kernel32_win7.GlobalCompact
@ stdcall GlobalDeleteAtom(long) kernel32_win7.GlobalDeleteAtom
@ stdcall GlobalFindAtomA(str) kernel32_win7.GlobalFindAtomA
@ stdcall GlobalFindAtomW(wstr) kernel32_win7.GlobalFindAtomW
@ stdcall GlobalFix(long) kernel32_win7.GlobalFix
@ stdcall GlobalFlags(long) kernel32_win7.GlobalFlags
@ stdcall GlobalFree(long) kernel32_win7.GlobalFree
@ stdcall GlobalGetAtomNameA(long ptr long) kernel32_win7.GlobalGetAtomNameA
@ stdcall GlobalGetAtomNameW(long ptr long) kernel32_win7.GlobalGetAtomNameW
@ stdcall GlobalHandle(ptr) kernel32_win7.GlobalHandle
@ stdcall GlobalLock(long) kernel32_win7.GlobalLock
@ stdcall GlobalMemoryStatus(ptr) kernel32_win7.GlobalMemoryStatus
@ stdcall GlobalMemoryStatusEx(ptr) kernel32_win7.GlobalMemoryStatusEx
@ stdcall GlobalReAlloc(long long long) kernel32_win7.GlobalReAlloc
@ stdcall GlobalSize(long) kernel32_win7.GlobalSize
@ stdcall GlobalUnWire(long) kernel32_win7.GlobalUnWire
@ stdcall GlobalUnfix(long) kernel32_win7.GlobalUnfix
@ stdcall GlobalUnlock(long) kernel32_win7.GlobalUnlock
@ stdcall GlobalWire(long) kernel32_win7.GlobalWire
@ stdcall Heap32First(ptr long long) kernel32_win7.Heap32First
@ stdcall Heap32ListFirst(long ptr) kernel32_win7.Heap32ListFirst
@ stdcall Heap32ListNext(long ptr) kernel32_win7.Heap32ListNext
@ stdcall Heap32Next(ptr) kernel32_win7.Heap32Next
@ stdcall HeapAlloc(long long long) kernel32_win7.HeapAlloc
@ stdcall HeapCompact(long long) kernel32_win7.HeapCompact
@ stdcall HeapCreate(long long long) kernel32_win7.HeapCreate
@ stdcall -version=0x351-0x502 HeapCreateTagsW(ptr long wstr wstr) kernel32_win7.HeapCreateTagsW
@ stdcall HeapDestroy(long) kernel32_win7.HeapDestroy
@ stdcall -version=0x351-0x502 HeapExtend(long long ptr long) kernel32_win7.HeapExtend
@ stdcall HeapFree(long long long) kernel32_win7.HeapFree
@ stdcall HeapLock(long) kernel32_win7.HeapLock
@ stdcall HeapQueryInformation(long long ptr long ptr) kernel32_win7.HeapQueryInformation
@ stdcall -version=0x351-0x502 HeapQueryTagW(long long long long ptr) kernel32_win7.HeapQueryTagW
@ stdcall HeapReAlloc(long long ptr long) kernel32_win7.HeapReAlloc
@ stdcall HeapSetInformation(ptr long ptr long) kernel32_win7.HeapSetInformation
@ stdcall HeapSize(long long ptr) kernel32_win7.HeapSize
@ stdcall HeapSummary(long long ptr) kernel32_win7.HeapSummary
@ stdcall HeapUnlock(long) kernel32_win7.HeapUnlock
@ stdcall -version=0x351-0x502 HeapUsage(long long long long ptr) kernel32_win7.HeapUsage
@ stdcall HeapValidate(long long ptr) kernel32_win7.HeapValidate
@ stdcall HeapWalk(long ptr) kernel32_win7.HeapWalk
@ stdcall -stub -version=0x600+ IdnToAscii(long wstr long ptr long) kernel32_win7.IdnToAscii
@ stdcall -stub -version=0x600+ IdnToNameprepUnicode(long wstr long ptr long) kernel32_win7.IdnToNameprepUnicode
@ stdcall -stub -version=0x600+ IdnToUnicode(long wstr long ptr long) kernel32_win7.IdnToUnicode
@ stdcall InitAtomTable(long) kernel32_win7.InitAtomTable
@ stdcall -version=0x600+ InitOnceBeginInitialize(ptr long ptr ptr) kernel32_win7.InitOnceBeginInitialize
@ stdcall -version=0x600+ InitOnceComplete(ptr long ptr) kernel32_win7.InitOnceComplete
@ stdcall -version=0x600+ InitOnceExecuteOnce(ptr ptr ptr ptr) kernel32_win7.InitOnceExecuteOnce
@ stdcall -version=0x600+ InitOnceInitialize(ptr) kernel32_win7.InitOnceInitialize
@ stdcall -version=0x600+ InitializeConditionVariable(ptr) kernel32_win7.InitializeConditionVariable
@ stdcall InitializeCriticalSection(ptr) kernel32_win7.InitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernel32_win7.InitializeCriticalSectionAndSpinCount
@ stdcall -version=0x600+ InitializeCriticalSectionEx(ptr long long) kernel32_win7.InitializeCriticalSectionEx
@ stdcall -stub -version=0x600+ InitializeProcThreadAttributeList(ptr long long ptr) kernel32_win7.InitializeProcThreadAttributeList
@ stdcall InitializeSListHead(ptr) kernel32_win7.InitializeSListHead
@ stdcall -version=0x600+ InitializeSRWLock(ptr) kernel32_win7.InitializeSRWLock
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr double double) kernel32_win7.InterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernel32_win7.InterlockedCompareExchange
@ stdcall -arch=i386 InterlockedDecrement(ptr) kernel32_win7.InterlockedDecrement
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernel32_win7.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long) kernel32_win7.InterlockedExchangeAdd
@ stdcall InterlockedFlushSList(ptr) kernel32_win7.InterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernel32_win7.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernel32_win7.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32_win7.InterlockedPushEntrySList
@ fastcall -version=0x600+ InterlockedPushListSList(ptr ptr ptr long) kernel32_win7.InterlockedPushListSList
@ stdcall InvalidateConsoleDIBits(long long) kernel32_win7.InvalidateConsoleDIBits
@ stdcall IsBadCodePtr(ptr) kernel32_win7.IsBadCodePtr
@ stdcall IsBadHugeReadPtr(ptr long) kernel32_win7.IsBadHugeReadPtr
@ stdcall IsBadHugeWritePtr(ptr long) kernel32_win7.IsBadHugeWritePtr
@ stdcall IsBadReadPtr(ptr long) kernel32_win7.IsBadReadPtr
@ stdcall IsBadStringPtrA(ptr long) kernel32_win7.IsBadStringPtrA
@ stdcall IsBadStringPtrW(ptr long) kernel32_win7.IsBadStringPtrW
@ stdcall IsBadWritePtr(ptr long) kernel32_win7.IsBadWritePtr
@ stub -version=0x600+ IsCalendarLeapDay
@ stub -version=0x600+ IsCalendarLeapMonth
@ stub -version=0x600+ IsCalendarLeapYear
@ stdcall IsDBCSLeadByte(long) kernel32_win7.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernel32_win7.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent() kernel32_win7.IsDebuggerPresent
@ stdcall IsNLSDefinedString(long long ptr long long) kernel32_win7.IsNLSDefinedString
@ stdcall -stub -version=0x600+ IsNormalizedString(long wstr long) kernel32_win7.IsNormalizedString
@ stdcall IsProcessInJob(long long ptr) kernel32_win7.IsProcessInJob
@ stdcall IsProcessorFeaturePresent(long) kernel32_win7.IsProcessorFeaturePresent
@ stdcall IsSystemResumeAutomatic() kernel32_win7.IsSystemResumeAutomatic
@ stdcall -version=0x600+ IsThreadAFiber() kernel32_win7.IsThreadAFiber
@ stub -version=0x600+ IsThreadpoolTimerSet
@ stdcall IsTimeZoneRedirectionEnabled() kernel32_win7.IsTimeZoneRedirectionEnabled
@ stub -version=0x600+ IsValidCalDateTime
@ stdcall IsValidCodePage(long) kernel32_win7.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernel32_win7.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernel32_win7.IsValidLocale
@ stdcall -version=0x501-0x502 IsValidUILanguage(long) kernel32_win7.IsValidUILanguage
@ stdcall IsWow64Process(ptr ptr) kernel32_win7.IsWow64Process
@ stdcall -version=0x601+ K32EmptyWorkingSet(long) kernel32_win7.K32EmptyWorkingSet
@ stdcall -version=0x601+ K32EnumDeviceDrivers(ptr long ptr) kernel32_win7.K32EnumDeviceDrivers
@ stdcall -version=0x601+ K32EnumPageFilesA(ptr ptr) kernel32_win7.K32EnumPageFilesA
@ stdcall -version=0x601+ K32EnumPageFilesW(ptr ptr) kernel32_win7.K32EnumPageFilesW
@ stdcall -version=0x601+ K32EnumProcessModules(long ptr long ptr) kernel32_win7.K32EnumProcessModules
@ stdcall -stub -version=0x601+ K32EnumProcessModulesEx(long ptr long ptr long) kernel32_win7.K32EnumProcessModulesEx
@ stdcall -version=0x601+ K32EnumProcesses(ptr long ptr) kernel32_win7.K32EnumProcesses
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameA(ptr ptr long) kernel32_win7.K32GetDeviceDriverBaseNameA
@ stdcall -version=0x601+ K32GetDeviceDriverBaseNameW(ptr ptr long) kernel32_win7.K32GetDeviceDriverBaseNameW
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameA(ptr ptr long) kernel32_win7.K32GetDeviceDriverFileNameA
@ stdcall -version=0x601+ K32GetDeviceDriverFileNameW(ptr ptr long) kernel32_win7.K32GetDeviceDriverFileNameW
@ stdcall -version=0x601+ K32GetMappedFileNameA(long ptr ptr long) kernel32_win7.K32GetMappedFileNameA
@ stdcall -version=0x601+ K32GetMappedFileNameW(long ptr ptr long) kernel32_win7.K32GetMappedFileNameW
@ stdcall -version=0x601+ K32GetModuleBaseNameA(long long ptr long) kernel32_win7.K32GetModuleBaseNameA
@ stdcall -version=0x601+ K32GetModuleBaseNameW(long long ptr long) kernel32_win7.K32GetModuleBaseNameW
@ stdcall -version=0x601+ K32GetModuleFileNameExA(long long ptr long) kernel32_win7.K32GetModuleFileNameExA
@ stdcall -version=0x601+ K32GetModuleFileNameExW(long long ptr long) kernel32_win7.K32GetModuleFileNameExW
@ stdcall -version=0x601+ K32GetModuleInformation(long long ptr long) kernel32_win7.K32GetModuleInformation
@ stdcall -version=0x601+ K32GetPerformanceInfo(ptr long) kernel32_win7.K32GetPerformanceInfo
@ stdcall -version=0x601+ K32GetProcessImageFileNameA(long ptr long) kernel32_win7.K32GetProcessImageFileNameA
@ stdcall -version=0x601+ K32GetProcessImageFileNameW(long ptr long) kernel32_win7.K32GetProcessImageFileNameW
@ stdcall -version=0x601+ K32GetProcessMemoryInfo(long ptr long) kernel32_win7.K32GetProcessMemoryInfo
@ stdcall -version=0x601+ K32GetWsChanges(long ptr long) kernel32_win7.K32GetWsChanges
@ stdcall -stub -version=0x601+ K32GetWsChangesEx(long ptr ptr) kernel32_win7.K32GetWsChangesEx
@ stdcall -version=0x601+ K32InitializeProcessForWsWatch(long) kernel32_win7.K32InitializeProcessForWsWatch
@ stdcall -version=0x601+ K32QueryWorkingSet(long ptr long) kernel32_win7.K32QueryWorkingSet
@ stdcall -version=0x601+ K32QueryWorkingSetEx(long ptr long) kernel32_win7.K32QueryWorkingSetEx
@ stdcall -version=0x600+ LCIDToLocaleName(long wstr long long) kernel32_win7.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernel32_win7.LCMapStringA
@ stdcall -version=0x600+ LCMapStringEx(long long wstr long ptr long ptr ptr long) kernel32_win7.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernel32_win7.LCMapStringW
@ stdcall LZClose(long)
@ stdcall LZCloseFile(long)
@ stdcall LZCopy(long long)
@ stdcall LZCreateFileW(ptr long long long ptr)
@ stdcall LZDone()
@ stdcall LZInit(long)
@ stdcall LZOpenFileA(str ptr long) 
@ stdcall LZOpenFileW(wstr ptr long)
@ stdcall LZRead(long str long)
@ stdcall LZSeek(long long long)
@ stdcall LZStart() 
@ stdcall LeaveCriticalSection(ptr) ntdll.RtlLeaveCriticalSection
@ stub -version=0x600+ LeaveCriticalSectionWhenCallbackReturns
@ stdcall LoadLibraryA(str) kernel32_win7.LoadLibraryA
@ stdcall LoadLibraryExA(str long long) kernel32_win7.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernel32_win7.LoadLibraryExW
@ stdcall LoadLibraryW(wstr) kernel32_win7.LoadLibraryW
@ stdcall LoadModule(str ptr) kernel32_win7.LoadModule
@ stdcall LoadResource(long long) kernel32_win7.LoadResource
@ stub -version=0x600+ LoadStringBaseExW
@ stub -version=0x600+ LoadStringBaseW
@ stdcall LocalAlloc(long long) kernel32_win7.LocalAlloc
@ stdcall LocalCompact(long) kernel32_win7.LocalCompact
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernel32_win7.LocalFileTimeToFileTime
@ stdcall LocalFlags(long) kernel32_win7.LocalFlags
@ stdcall LocalFree(long) kernel32_win7.LocalFree
@ stdcall LocalHandle(ptr) kernel32_win7.LocalHandle
@ stdcall LocalLock(long) kernel32_win7.LocalLock
@ stdcall LocalReAlloc(long long long) kernel32_win7.LocalReAlloc
@ stdcall LocalShrink(long long) kernel32_win7.LocalShrink
@ stdcall LocalSize(long) kernel32_win7.LocalSize
@ stdcall LocalUnlock(long) kernel32_win7.LocalUnlock
@ stdcall -version=0x600+ LocaleNameToLCID(wstr long) kernel32_win7.LocaleNameToLCID
@ stdcall LockFile(long long long long long) kernel32_win7.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernel32_win7.LockFileEx
@ stdcall LockResource(long) kernel32_win7.LockResource
@ stdcall MapUserPhysicalPages(ptr long ptr) kernel32_win7.MapUserPhysicalPages
@ stdcall MapUserPhysicalPagesScatter(ptr long ptr) kernel32_win7.MapUserPhysicalPagesScatter
@ stdcall MapViewOfFile(long long long long long) kernel32_win7.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernel32_win7.MapViewOfFileEx
@ stub -version=0x600+ MapViewOfFileExNuma
@ stdcall Module32First(long ptr) kernel32_win7.Module32First
@ stdcall Module32FirstW(long ptr) kernel32_win7.Module32FirstW
@ stdcall Module32Next(long ptr) kernel32_win7.Module32Next
@ stdcall Module32NextW(long ptr) kernel32_win7.Module32NextW
@ stdcall MoveFileA(str str) kernel32_win7.MoveFileA
@ stdcall MoveFileExA(str str long) kernel32_win7.MoveFileExA
@ stdcall MoveFileExW(wstr wstr long) kernel32_win7.MoveFileExW
@ stub -version=0x600+ MoveFileTransactedA
@ stub -version=0x600+ MoveFileTransactedW
@ stdcall MoveFileW(wstr wstr) kernel32_win7.MoveFileW
@ stdcall MoveFileWithProgressA(str str ptr ptr long) kernel32_win7.MoveFileWithProgressA
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernel32_win7.MoveFileWithProgressW
@ stdcall MulDiv(long long long) kernel32_win7.MulDiv
@ stdcall MultiByteToWideChar(long long str long ptr long) kernel32_win7.MultiByteToWideChar
@ stdcall NeedCurrentDirectoryForExePathA(str) kernel32_win7.NeedCurrentDirectoryForExePathA
@ stdcall NeedCurrentDirectoryForExePathW(wstr) kernel32_win7.NeedCurrentDirectoryForExePathW
@ stub -version=0x600+ NlsCheckPolicy
@ stdcall -version=0x500-0x600 NlsConvertIntegerToString(long long long wstr long) kernel32_win7.NlsConvertIntegerToString
@ stub -version=0x600+ NlsEventDataDescCreate
@ stdcall NlsGetCacheUpdateCount() kernel32_win7.NlsGetCacheUpdateCount
@ stdcall -version=0x500-0x502 NlsResetProcessLocale() kernel32_win7.NlsResetProcessLocale
@ stub -version=0x600+ NlsUpdateLocale
@ stub -version=0x600+ NlsUpdateSystemLocale
@ stub -version=0x600+ NlsWriteEtwEvent
@ stdcall -stub -version=0x600+ NormalizeString(long wstr long ptr long) kernel32_win7.NormalizeString
@ stdcall -stub -version=0x600+ NotifyUILanguageChange(long wstr wstr long ptr) kernel32_win7.NotifyUILanguageChange
@ stdcall OpenConsoleW(wstr long long long) kernel32_win7.OpenConsoleW
@ stdcall -version=0x500-0x502 OpenDataFile(long long) kernel32_win7.OpenDataFile
@ stdcall OpenEventA(long long str) kernel32_win7.OpenEventA
@ stdcall OpenEventW(long long wstr) kernel32_win7.OpenEventW
@ stdcall OpenFile(str ptr long) kernel32_win7.OpenFile
@ stdcall -version=0x600+ OpenFileById(ptr ptr long long ptr long) kernel32_win7.OpenFileById
@ stdcall OpenFileMappingA(long long str) kernel32_win7.OpenFileMappingA
@ stdcall OpenFileMappingW(long long wstr) kernel32_win7.OpenFileMappingW
@ stdcall OpenJobObjectA(long long str) kernel32_win7.OpenJobObjectA
@ stdcall OpenJobObjectW(long long wstr) kernel32_win7.OpenJobObjectW
@ stdcall OpenMutexA(long long str) kernel32_win7.OpenMutexA
@ stdcall OpenMutexW(long long wstr) kernel32_win7.OpenMutexW
@ stub -version=0x600+ OpenPrivateNamespaceA
@ stub -version=0x600+ OpenPrivateNamespaceW
@ stdcall OpenProcess(long long long) kernel32_win7.OpenProcess
@ stdcall OpenProfileUserMapping() kernel32_win7.OpenProfileUserMapping
@ stdcall OpenSemaphoreA(long long str) kernel32_win7.OpenSemaphoreA
@ stdcall OpenSemaphoreW(long long wstr) kernel32_win7.OpenSemaphoreW
@ stdcall OpenThread(long long long) kernel32_win7.OpenThread
@ stdcall OpenWaitableTimerA(long long str) kernel32_win7.OpenWaitableTimerA
@ stdcall OpenWaitableTimerW(long long wstr) kernel32_win7.OpenWaitableTimerW
@ stdcall OutputDebugStringA(str) kernel32_win7.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernel32_win7.OutputDebugStringW
@ stdcall PeekConsoleInputA(ptr ptr long ptr) kernel32_win7.PeekConsoleInputA
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernel32_win7.PeekConsoleInputW
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernel32_win7.PeekNamedPipe
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernel32_win7.PostQueuedCompletionStatus
@ stdcall PrepareTape(ptr long long) kernel32_win7.PrepareTape
@ stdcall PrivCopyFileExW(wstr wstr ptr ptr long long) kernel32_win7.PrivCopyFileExW
@ stdcall PrivMoveFileIdentityW(long long long) kernel32_win7.PrivMoveFileIdentityW
@ stdcall Process32First(ptr ptr) kernel32_win7.Process32First
@ stdcall Process32FirstW(ptr ptr) kernel32_win7.Process32FirstW
@ stdcall Process32Next(ptr ptr) kernel32_win7.Process32Next
@ stdcall Process32NextW(ptr ptr) kernel32_win7.Process32NextW
@ stdcall ProcessIdToSessionId(long ptr) kernel32_win7.ProcessIdToSessionId
@ stdcall PulseEvent(long) kernel32_win7.PulseEvent
@ stdcall PurgeComm(long long) kernel32_win7.PurgeComm
@ stub -version=0x600+ QueryActCtxSettingsW
@ stdcall QueryActCtxW(long ptr ptr long ptr long ptr) kernel32_win7.QueryActCtxW
@ stdcall QueryDepthSList(ptr) ntdll.RtlQueryDepthSList
@ stdcall QueryDosDeviceA(str ptr long) kernel32_win7.QueryDosDeviceA
@ stdcall QueryDosDeviceW(wstr ptr long) kernel32_win7.QueryDosDeviceW
@ stdcall -version=0x600+ QueryFullProcessImageNameA(ptr long str ptr) kernel32_win7.QueryFullProcessImageNameA
@ stdcall -version=0x600+ QueryFullProcessImageNameW(ptr long wstr ptr) kernel32_win7.QueryFullProcessImageNameW
@ stub -version=0x600+ QueryIdleProcessorCycleTime
@ stdcall QueryInformationJobObject(long long ptr long ptr) kernel32_win7.QueryInformationJobObject
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernel32_win7.QueryMemoryResourceNotification
@ stdcall QueryPerformanceCounter(ptr) kernel32_win7.QueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) kernel32_win7.QueryPerformanceFrequency
@ stub -version=0x600+ QueryProcessAffinityUpdateMode
@ stub -version=0x600+ QueryProcessCycleTime
@ stub -version=0x600+ QueryThreadCycleTime
@ stdcall QueueUserAPC(ptr long long) kernel32_win7.QueueUserAPC
@ stdcall QueueUserWorkItem(ptr ptr long) kernel32_win7.QueueUserWorkItem
@ stdcall -norelay RaiseException(long long long ptr) kernel32_win7.RaiseException
@ stdcall ReOpenFile(ptr long long long) kernel32_win7.ReOpenFile
@ stdcall ReadConsoleA(long ptr long ptr ptr) kernel32_win7.ReadConsoleA
@ stdcall ReadConsoleInputA(long ptr long ptr) kernel32_win7.ReadConsoleInputA
@ stdcall ReadConsoleInputExA(long ptr long ptr long) kernel32_win7.ReadConsoleInputExA
@ stdcall ReadConsoleInputExW(long ptr long ptr long) kernel32_win7.ReadConsoleInputExW
@ stdcall ReadConsoleInputW(long ptr long ptr) kernel32_win7.ReadConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr) kernel32_win7.ReadConsoleOutputA
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr) kernel32_win7.ReadConsoleOutputAttribute
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr) kernel32_win7.ReadConsoleOutputCharacterA
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr) kernel32_win7.ReadConsoleOutputCharacterW
@ stdcall ReadConsoleOutputW(long ptr long long ptr) kernel32_win7.ReadConsoleOutputW
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernel32_win7.ReadConsoleW
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernel32_win7.ReadDirectoryChangesW
@ stdcall ReadFile(long ptr long ptr ptr) kernel32_win7.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernel32_win7.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernel32_win7.ReadFileScatter
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernel32_win7.ReadProcessMemory
@ stdcall -version=0x600+ RegisterApplicationRecoveryCallback(ptr ptr long long)
@ stdcall -version=0x600+ RegisterApplicationRestart(wstr long)
@ stdcall RegisterConsoleIME(ptr ptr) kernel32_win7.RegisterConsoleIME
@ stdcall RegisterConsoleOS2(long) kernel32_win7.RegisterConsoleOS2
@ stdcall RegisterConsoleVDM(long long long long long long long long long long long) kernel32_win7.RegisterConsoleVDM
@ stdcall RegisterWaitForInputIdle(ptr) kernel32_win7.RegisterWaitForInputIdle
@ stdcall RegisterWaitForSingleObject(ptr long ptr ptr long long) kernel32_win7.RegisterWaitForSingleObject
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernel32_win7.RegisterWaitForSingleObjectEx
@ stdcall RegisterWowBaseHandlers(long) kernel32_win7.RegisterWowBaseHandlers
@ stdcall RegisterWowExec(long) kernel32_win7.RegisterWowExec
@ stdcall ReleaseActCtx(ptr) kernel32_win7.ReleaseActCtx
@ stdcall ReleaseMutex(long) kernel32_win7.ReleaseMutex
@ stub -version=0x600+ ReleaseMutexWhenCallbackReturns
@ stdcall -version=0x600+ ReleaseSRWLockExclusive(ptr) ntdll.RtlReleaseSRWLockExclusive
@ stdcall -version=0x600+ ReleaseSRWLockShared(ptr) ntdll.RtlReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr) kernel32_win7.ReleaseSemaphore
@ stub -version=0x600+ ReleaseSemaphoreWhenCallbackReturns
@ stdcall RemoveDirectoryA(str) kernel32_win7.RemoveDirectoryA
@ stub -version=0x600+ RemoveDirectoryTransactedA
@ stub -version=0x600+ RemoveDirectoryTransactedW
@ stdcall RemoveDirectoryW(wstr) kernel32_win7.RemoveDirectoryW
@ stdcall RemoveLocalAlternateComputerNameA(str long) kernel32_win7.RemoveLocalAlternateComputerNameA
@ stdcall RemoveLocalAlternateComputerNameW(wstr long) kernel32_win7.RemoveLocalAlternateComputerNameW
@ stub -version=0x600+ RemoveSecureMemoryCacheCallback
@ stdcall RemoveVectoredContinueHandler(ptr) ntdll.RtlRemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) ntdll.RtlRemoveVectoredExceptionHandler
@ stdcall ReplaceFile(wstr wstr wstr long ptr ptr) kernel32_win7.ReplaceFileW
@ stdcall ReplaceFileA(str str str long ptr ptr) kernel32_win7.ReplaceFileA
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernel32_win7.ReplaceFileW
@ stub -version=0x600+ ReplacePartitionUnit
@ stdcall RequestDeviceWakeup(long) kernel32_win7.RequestDeviceWakeup
@ stdcall RequestWakeupLatency(long) kernel32_win7.RequestWakeupLatency
@ stdcall ResetEvent(long) kernel32_win7.ResetEvent
@ stdcall ResetWriteWatch(ptr long) kernel32_win7.ResetWriteWatch
@ stdcall RestoreLastError(long) ntdll.RtlRestoreLastWin32Error
@ stdcall ResumeThread(long) kernel32_win7.ResumeThread
@ stdcall -arch=x86_64 RtlAddFunctionTable(ptr long long) ntdll.RtlAddFunctionTable
@ stdcall -register RtlCaptureContext(ptr) ntdll.RtlCaptureContext
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr) ntdll.RtlCaptureStackBackTrace
@ stdcall -arch=x86_64 RtlCompareMemory(ptr ptr ptr) ntdll.RtlCompareMemory
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr ptr) ntdll.memcpy
@ stdcall -arch=x86_64 RtlDeleteFunctionTable(ptr) ntdll.RtlDeleteFunctionTable
@ stdcall RtlFillMemory(ptr long long) ntdll.RtlFillMemory
@ stdcall -arch=x86_64 RtlInstallFunctionTableCallback(double double long ptr ptr ptr) ntdll.RtlInstallFunctionTableCallback
@ stdcall -arch=x86_64 RtlLookupFunctionEntry(ptr ptr ptr) ntdll.RtlLookupFunctionEntry
@ stdcall RtlMoveMemory(ptr ptr long) ntdll.RtlMoveMemory
@ stdcall -arch=x86_64 RtlPcToFileHeader(ptr ptr) ntdll.RtlPcToFileHeader
@ stdcall -arch=x86_64 RtlRaiseException(ptr) ntdll.RtlRaiseException
@ stdcall -arch=x86_64 RtlRestoreContext(ptr ptr) ntdll.RtlRestoreContext
@ stdcall RtlUnwind(ptr ptr ptr ptr) ntdll.RtlUnwind
@ stdcall -arch=x86_64 RtlUnwindEx(ptr ptr ptr ptr ptr ptr) ntdll.RtlUnwindEx
@ stdcall -arch=x86_64 RtlVirtualUnwind(long int64 int64 ptr ptr ptr ptr ptr) ntdll.RtlVirtualUnwind
@ stdcall RtlZeroMemory(ptr long) ntdll.RtlZeroMemory
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr) kernel32_win7.ScrollConsoleScreenBufferA
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr) kernel32_win7.ScrollConsoleScreenBufferW
@ stdcall SearchPathA(str str str long ptr ptr) kernel32_win7.SearchPathA
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr) kernel32_win7.SearchPathW
@ stdcall -version=0x500-0x502 SetCPGlobal(long) kernel32_win7.SetCPGlobal
@ stdcall SetCalendarInfoA(long long long str) kernel32_win7.SetCalendarInfoA
@ stdcall SetCalendarInfoW(long long long wstr) kernel32_win7.SetCalendarInfoW
@ stdcall SetClientTimeZoneInformation(ptr) kernel32_win7.SetClientTimeZoneInformation
@ stdcall SetComPlusPackageInstallStatus(ptr) kernel32_win7.SetComPlusPackageInstallStatus
@ stdcall SetCommBreak(long) kernel32_win7.SetCommBreak
@ stdcall SetCommConfig(long ptr long) kernel32_win7.SetCommConfig
@ stdcall SetCommMask(long ptr) kernel32_win7.SetCommMask
@ stdcall SetCommState(long ptr) kernel32_win7.SetCommState
@ stdcall SetCommTimeouts(long ptr) kernel32_win7.SetCommTimeouts
@ stdcall SetComputerNameA(str) kernel32_win7.SetComputerNameA
@ stdcall SetComputerNameExA(long str) kernel32_win7.SetComputerNameExA
@ stdcall SetComputerNameExW(long wstr) kernel32_win7.SetComputerNameExW
@ stdcall SetComputerNameW(wstr) kernel32_win7.SetComputerNameW
@ stdcall SetConsoleActiveScreenBuffer(long) kernel32_win7.SetConsoleActiveScreenBuffer
@ stdcall SetConsoleCP(long) kernel32_win7.SetConsoleCP
@ stdcall -version=0x351-0x502 SetConsoleCommandHistoryMode(long) kernel32_win7.SetConsoleCommandHistoryMode
@ stdcall SetConsoleCtrlHandler(ptr long) kernel32_win7.SetConsoleCtrlHandler
@ stdcall SetConsoleCursor(long long) kernel32_win7.SetConsoleCursor
@ stdcall SetConsoleCursorInfo(long ptr) kernel32_win7.SetConsoleCursorInfo
@ stdcall SetConsoleCursorMode(long long long) kernel32_win7.SetConsoleCursorMode
@ stdcall SetConsoleCursorPosition(long long) kernel32_win7.SetConsoleCursorPosition
@ stdcall SetConsoleDisplayMode(long long ptr) kernel32_win7.SetConsoleDisplayMode
@ stdcall SetConsoleFont(long long) kernel32_win7.SetConsoleFont
@ stdcall SetConsoleHardwareState(long long long) kernel32_win7.SetConsoleHardwareState
@ stdcall -version=0x600+ SetConsoleHistoryInfo(ptr) kernel32_win7.SetConsoleHistoryInfo
@ stdcall SetConsoleIcon(ptr) kernel32_win7.SetConsoleIcon
@ stdcall SetConsoleInputExeNameA(ptr) kernel32_win7.SetConsoleInputExeNameA
@ stdcall SetConsoleInputExeNameW(ptr) kernel32_win7.SetConsoleInputExeNameW
@ stdcall SetConsoleKeyShortcuts(long long long long) kernel32_win7.SetConsoleKeyShortcuts
@ stdcall SetConsoleLocalEUDC(long long long long) kernel32_win7.SetConsoleLocalEUDC
@ stdcall SetConsoleMaximumWindowSize(long long) kernel32_win7.SetConsoleMaximumWindowSize
@ stdcall SetConsoleMenuClose(long) kernel32_win7.SetConsoleMenuClose
@ stdcall SetConsoleMode(long long) kernel32_win7.SetConsoleMode
@ stdcall SetConsoleNlsMode(long long) kernel32_win7.SetConsoleNlsMode
@ stdcall SetConsoleNumberOfCommandsA(long long) kernel32_win7.SetConsoleNumberOfCommandsA
@ stdcall SetConsoleNumberOfCommandsW(long long) kernel32_win7.SetConsoleNumberOfCommandsW
@ stdcall SetConsoleOS2OemFormat(long) kernel32_win7.SetConsoleOS2OemFormat
@ stdcall SetConsoleOutputCP(long) kernel32_win7.SetConsoleOutputCP
@ stdcall SetConsolePalette(long long long) kernel32_win7.SetConsolePalette
@ stdcall -version=0x600+ SetConsoleScreenBufferInfoEx(ptr ptr) kernel32_win7.SetConsoleScreenBufferInfoEx
@ stdcall SetConsoleScreenBufferSize(long long) kernel32_win7.SetConsoleScreenBufferSize
@ stdcall SetConsoleTextAttribute(long long) kernel32_win7.SetConsoleTextAttribute
@ stdcall SetConsoleTitleA(str) kernel32_win7.SetConsoleTitleA
@ stdcall SetConsoleTitleW(wstr) kernel32_win7.SetConsoleTitleW
@ stdcall SetConsoleWindowInfo(long long ptr) kernel32_win7.SetConsoleWindowInfo
@ stdcall SetCriticalSectionSpinCount(ptr long) ntdll.RtlSetCriticalSectionSpinCount
@ stub -version=0x600+ SetCurrentConsoleFontEx
@ stdcall SetCurrentDirectoryA(str) kernel32_win7.SetCurrentDirectoryA
@ stdcall SetCurrentDirectoryW(wstr) kernel32_win7.SetCurrentDirectoryW
@ stdcall SetDefaultCommConfigA(str ptr long) kernel32_win7.SetDefaultCommConfigA
@ stdcall SetDefaultCommConfigW(wstr ptr long) kernel32_win7.SetDefaultCommConfigW
@ stdcall SetDllDirectoryA(str) kernel32_win7.SetDllDirectoryA
@ stdcall SetDllDirectoryW(wstr) kernel32_win7.SetDllDirectoryW
@ stub -version=0x600+ SetDynamicTimeZoneInformation
@ stdcall SetEndOfFile(long) kernel32_win7.SetEndOfFile
@ stdcall SetEnvironmentStringsA(ptr) kernel32_win7.SetEnvironmentStringsA
@ stdcall SetEnvironmentStringsW(ptr) kernel32_win7.SetEnvironmentStringsW
@ stdcall SetEnvironmentVariableA(str str) kernel32_win7.SetEnvironmentVariableA
@ stdcall SetEnvironmentVariableW(wstr wstr) kernel32_win7.SetEnvironmentVariableW
@ stdcall SetErrorMode(long) kernel32_win7.SetErrorMode
@ stdcall SetEvent(long) kernel32_win7.SetEvent
@ stub -version=0x600+ SetEventWhenCallbackReturns
@ stdcall SetFileApisToANSI() kernel32_win7.SetFileApisToANSI
@ stdcall SetFileApisToOEM() kernel32_win7.SetFileApisToOEM
@ stdcall SetFileAttributesA(str long) kernel32_win7.SetFileAttributesA
@ stub -version=0x600+ SetFileAttributesTransactedA
@ stub -version=0x600+ SetFileAttributesTransactedW
@ stdcall SetFileAttributesW(wstr long) kernel32_win7.SetFileAttributesW
@ stdcall -version=0x600+ SetFileBandwidthReservation(ptr long long long ptr ptr) kernel32_win7.SetFileBandwidthReservation
@ stdcall SetFileCompletionNotificationModes(ptr long) kernel32_win7.SetFileCompletionNotificationModes
@ stub -version=0x600+ SetFileInformationByHandle
@ stub -version=0x600+ SetFileIoOverlappedRange
@ stdcall SetFilePointer(long long ptr long) kernel32_win7.SetFilePointer
@ stdcall SetFilePointerEx(long double ptr long) kernel32_win7.SetFilePointerEx
@ stdcall SetFileShortNameA(long str) kernel32_win7.SetFileShortNameA
@ stdcall SetFileShortNameW(long wstr) kernel32_win7.SetFileShortNameW
@ stdcall SetFileTime(long ptr ptr ptr) kernel32_win7.SetFileTime
@ stdcall SetFileValidData(long double) kernel32_win7.SetFileValidData
@ stdcall SetFirmwareEnvironmentVariableA(str str ptr long) kernel32_win7.SetFirmwareEnvironmentVariableA
@ stdcall -version=0x602+ SetFirmwareEnvironmentVariableExA(str str ptr long long) kernel32_win7.SetFirmwareEnvironmentVariableExA
@ stdcall -version=0x602+ SetFirmwareEnvironmentVariableExW(str str ptr long long) kernel32_win7.SetFirmwareEnvironmentVariableExW
@ stdcall SetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernel32_win7.SetFirmwareEnvironmentVariableW
@ stdcall -i386 SetHandleContext(long long) kernel32_win7.SetHandleContext
@ stdcall SetHandleCount(long) kernel32_win7.SetHandleCount
@ stdcall SetHandleInformation(long long long) kernel32_win7.SetHandleInformation
@ stdcall SetInformationJobObject(long long ptr long) kernel32_win7.SetInformationJobObject
@ stdcall SetLastConsoleEventActive() kernel32_win7.SetLastConsoleEventActive ; missing in XP SP3
@ stdcall SetLastError(long) ntdll.RtlSetLastWin32Error
@ stdcall SetLocalPrimaryComputerNameA(long long) kernel32_win7.SetLocalPrimaryComputerNameA ; missing in XP SP3
@ stdcall SetLocalPrimaryComputerNameW(long long) kernel32_win7.SetLocalPrimaryComputerNameW ; missing in XP SP3
@ stdcall SetLocalTime(ptr) kernel32_win7.SetLocalTime
@ stdcall SetLocaleInfoA(long long str) kernel32_win7.SetLocaleInfoA
@ stdcall SetLocaleInfoW(long long wstr) kernel32_win7.SetLocaleInfoW
@ stdcall SetMailslotInfo(long long) kernel32_win7.SetMailslotInfo
@ stdcall SetMessageWaitingIndicator(ptr long) kernel32_win7.SetMessageWaitingIndicator
@ stub -version=0x600+ SetNamedPipeAttribute
@ stdcall SetNamedPipeHandleState(long long long long) kernel32_win7.SetNamedPipeHandleState
@ stdcall SetPriorityClass(long long) kernel32_win7.SetPriorityClass
@ stdcall SetProcessAffinityMask(long long) kernel32_win7.SetProcessAffinityMask
@ stub -version=0x600+ SetProcessAffinityUpdateMode
@ stub -version=0x600+ SetProcessDEPPolicy
@ stdcall SetProcessPriorityBoost(long long) kernel32_win7.SetProcessPriorityBoost
@ stdcall SetProcessShutdownParameters(long long) kernel32_win7.SetProcessShutdownParameters
@ stdcall SetProcessWorkingSetSize(long long long) kernel32_win7.SetProcessWorkingSetSize
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernel32_win7.SetProcessWorkingSetSizeEx
@ stdcall SetStdHandle(long long) kernel32_win7.SetStdHandle
@ stub -version=0x600+ SetStdHandleEx
@ stdcall SetSystemFileCacheSize(long long long) kernel32_win7.SetSystemFileCacheSize
@ stdcall SetSystemPowerState(long long) kernel32_win7.SetSystemPowerState
@ stdcall SetSystemTime(ptr) kernel32_win7.SetSystemTime
@ stdcall SetSystemTimeAdjustment(long long) kernel32_win7.SetSystemTimeAdjustment
@ stdcall SetTapeParameters(ptr long ptr) kernel32_win7.SetTapeParameters
@ stdcall SetTapePosition(ptr long long long long long) kernel32_win7.SetTapePosition
@ stdcall SetTermsrvAppInstallMode(long) kernel32_win7.SetTermsrvAppInstallMode
@ stdcall SetThreadAffinityMask(long long) kernel32_win7.SetThreadAffinityMask
@ stdcall SetThreadContext(long ptr) kernel32_win7.SetThreadContext
@ stdcall -stub -version=0x600+ SetThreadErrorMode(long ptr)
@ stdcall SetThreadExecutionState(long) kernel32_win7.SetThreadExecutionState
@ stdcall SetThreadIdealProcessor(long long) kernel32_win7.SetThreadIdealProcessor
@ stdcall SetThreadLocale(long) kernel32_win7.SetThreadLocale
@ stdcall -version=0x600+ SetThreadPreferredUILanguages(long wstr ptr) kernel32_win7.SetThreadPreferredUILanguages
@ stdcall SetThreadPriority(long long) kernel32_win7.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernel32_win7.SetThreadPriorityBoost
@ stdcall SetThreadStackGuarantee(ptr) kernel32_win7.SetThreadStackGuarantee
@ stdcall SetThreadUILanguage(long) kernel32_win7.SetThreadUILanguage
@ stub -version=0x600+ SetThreadpoolThreadMaximum
@ stub -version=0x600+ SetThreadpoolThreadMinimum
@ stub -version=0x600+ SetThreadpoolTimer
@ stub -version=0x600+ SetThreadpoolWait
@ stdcall SetTimeZoneInformation(ptr) kernel32_win7.SetTimeZoneInformation
@ stdcall SetTimerQueueTimer(long ptr ptr long long long) kernel32_win7.SetTimerQueueTimer
@ stdcall SetUnhandledExceptionFilter(ptr) kernel32_win7.SetUnhandledExceptionFilter
@ stdcall SetUserGeoID(long) kernel32_win7.SetUserGeoID
@ stdcall SetVDMCurrentDirectories(long long) kernel32_win7.SetVDMCurrentDirectories
@ stdcall SetVolumeLabelA(str str) kernel32_win7.SetVolumeLabelA
@ stdcall SetVolumeLabelW(wstr wstr) kernel32_win7.SetVolumeLabelW
@ stdcall SetVolumeMountPointA(str str) kernel32_win7.SetVolumeMountPointA
@ stdcall SetVolumeMountPointW(wstr wstr) kernel32_win7.SetVolumeMountPointW
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernel32_win7.SetWaitableTimer
@ stdcall SetupComm(long long long) kernel32_win7.SetupComm
@ stdcall ShowConsoleCursor(long long) kernel32_win7.ShowConsoleCursor
@ stdcall SignalObjectAndWait(long long long long) kernel32_win7.SignalObjectAndWait
@ stdcall SizeofResource(long long) kernel32_win7.SizeofResource
@ stdcall Sleep(long) kernel32_win7.Sleep
@ stdcall -version=0x600+ SleepConditionVariableCS(ptr ptr long) kernel32_win7.SleepConditionVariableCS
@ stdcall -version=0x600+ SleepConditionVariableSRW(ptr ptr long long) kernel32_win7.SleepConditionVariableSRW
@ stdcall SleepEx(long long) kernel32_win7.SleepEx
@ stub -version=0x600+ StartThreadpoolIo
@ stdcall -stub -version=0x600+ SubmitThreadpoolWork(ptr) kernel32_win7.SubmitThreadpoolWork
@ stdcall SuspendThread(long) kernel32_win7.SuspendThread
@ stdcall SwitchToFiber(ptr) kernel32_win7.SwitchToFiber
@ stdcall SwitchToThread() kernel32_win7.SwitchToThread
@ stdcall SystemTimeToFileTime(ptr ptr) kernel32_win7.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernel32_win7.SystemTimeToTzSpecificLocalTime
@ stdcall TerminateJobObject(ptr long) kernel32_win7.TerminateJobObject
@ stdcall TerminateProcess(ptr long) kernel32_win7.TerminateProcess
@ stdcall TerminateThread(ptr long) kernel32_win7.TerminateThread
@ stdcall TermsrvAppInstallMode() kernel32_win7.TermsrvAppInstallMode
@ stdcall Thread32First(long ptr) kernel32_win7.Thread32First
@ stdcall Thread32Next(long ptr) kernel32_win7.Thread32Next
@ stdcall TlsAlloc() kernel32_win7.TlsAlloc
@ stdcall TlsFree(long) kernel32_win7.TlsFree
@ stdcall -norelay TlsGetValue(long) kernel32_win7.TlsGetValue
@ stdcall -norelay TlsSetValue(long ptr) kernel32_win7.TlsSetValue
@ stdcall Toolhelp32ReadProcessMemory(long ptr ptr long ptr) kernel32_win7.Toolhelp32ReadProcessMemory
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernel32_win7.TransactNamedPipe
@ stdcall TransmitCommChar(long long) kernel32_win7.TransmitCommChar
@ stdcall -version=0x601+ TryAcquireSRWLockExclusive(ptr) ntdll.RtlTryAcquireSRWLockExclusive
@ stdcall -version=0x601+ TryAcquireSRWLockShared(ptr) ntdll.RtlTryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) ntdll.RtlTryEnterCriticalSection
@ stub -version=0x600+ TrySubmitThreadpoolCallback
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernel32_win7.TzSpecificLocalTimeToSystemTime
@ stdcall UTRegister(long str str str ptr ptr ptr) kernel32_win7.UTRegister
@ stdcall UTUnRegister(long) kernel32_win7.UTUnRegister
@ stdcall UnhandledExceptionFilter(ptr) kernel32_win7.UnhandledExceptionFilter
@ stdcall UnlockFile(long long long long long) kernel32_win7.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernel32_win7.UnlockFileEx
@ stdcall UnmapViewOfFile(ptr) kernel32_win7.UnmapViewOfFile
@ stub -version=0x600+ UnregisterApplicationRecoveryCallback 
@ stub -version=0x600+ UnregisterApplicationRestart
@ stdcall UnregisterConsoleIME() kernel32_win7.UnregisterConsoleIME
@ stdcall UnregisterWait(long) kernel32_win7.UnregisterWait
@ stdcall UnregisterWaitEx(long long) kernel32_win7.UnregisterWaitEx
@ stub -version=0x600+ UpdateCalendarDayOfWeek
@ stdcall -stub -version=0x600+ UpdateProcThreadAttribute(ptr long ptr ptr ptr ptr ptr) kernel32_win7.UpdateProcThreadAttribute
@ stdcall UpdateResourceA(long str str long ptr long) kernel32_win7.UpdateResourceA
@ stdcall UpdateResourceW(long wstr wstr long ptr long) kernel32_win7.UpdateResourceW
@ stdcall VDMConsoleOperation(long long) kernel32_win7.VDMConsoleOperation
@ stdcall VDMOperationStarted(long) kernel32_win7.VDMOperationStarted
@ stdcall -version=0x500-0x502 ValidateLCType(long long ptr ptr) kernel32_win7.ValidateLCType
@ stdcall -version=0x500-0x502 ValidateLocale(long) kernel32_win7.ValidateLocale
@ stdcall VerLanguageNameA(long str long) kernel32_win7.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernel32_win7.VerLanguageNameW
@ stdcall -ret64 VerSetConditionMask(long long long long) ntdll.VerSetConditionMask
@ stdcall VerifyConsoleIoHandle(long) kernel32_win7.VerifyConsoleIoHandle
@ stub -version=0x600+ VerifyScripts
@ stdcall VerifyVersionInfoA(long long double)
@ stdcall VerifyVersionInfoW(long long double) kernel32_win7.VerifyVersionInfoW
@ stdcall VirtualAlloc(ptr long long long) kernel32_win7.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernel32_win7.VirtualAllocEx
@ stub -version=0x600+ VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernel32_win7.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernel32_win7.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernel32_win7.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernel32_win7.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernel32_win7.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernel32_win7.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernel32_win7.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernel32_win7.VirtualUnlock
@ stdcall WTSGetActiveConsoleSessionId() kernel32_win7.WTSGetActiveConsoleSessionId
@ stdcall WaitCommEvent(long ptr ptr) kernel32_win7.WaitCommEvent
@ stdcall WaitForDebugEvent(ptr long) kernel32_win7.WaitForDebugEvent
@ stdcall WaitForMultipleObjects(long ptr long long) kernel32_win7.WaitForMultipleObjects
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernel32_win7.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernel32_win7.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernel32_win7.WaitForSingleObjectEx
@ stub -version=0x600+ WaitForThreadpoolIoCallbacks
@ stub -version=0x600+ WaitForThreadpoolTimerCallbacks
@ stub -version=0x600+ WaitForThreadpoolWaitCallback
@ stdcall -stub -version=0x600+ WaitForThreadpoolWorkCallbacks(ptr long) kernel32_win7.WaitForThreadpoolWorkCallbacks
@ stdcall WaitNamedPipeA(str long) kernel32_win7.WaitNamedPipeA
@ stdcall WaitNamedPipeW(wstr long) kernel32_win7.WaitNamedPipeW
@ stdcall -version=0x600+ WakeAllConditionVariable(ptr) ntdll.RtlWakeAllConditionVariable
@ stdcall -version=0x600+ WakeConditionVariable(ptr) ntdll.RtlWakeConditionVariable
@ stub -version=0x600+ WerGetFlags
@ stub -version=0x600+ WerRegisterFile
@ stub -version=0x600+ WerRegisterMemoryBlock
@ stub -version=0x600+ WerSetFlags
@ stub -version=0x600+ WerUnregisterFile
@ stub -version=0x600+ WerUnregisterMemoryBlock 
@ stub -version=0x600+ WerpCleanupMessageMapping 
@ stub -version=0x600+ WerpInitiateRemoteRecovery
@ stub -version=0x600+ WerpNotifyLoadStringResource
@ stub -version=0x600+ WerpNotifyLoadStringResourceEx
@ stub -version=0x600+ WerpNotifyUseStringResource
@ stub -version=0x600+ WerpStringLookup 
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernel32_win7.WideCharToMultiByte
@ stdcall WinExec(str long) kernel32_win7.WinExec
@ stdcall Wow64DisableWow64FsRedirection(ptr) kernel32_win7.Wow64DisableWow64FsRedirection
@ stdcall Wow64EnableWow64FsRedirection(long) kernel32_win7.Wow64EnableWow64FsRedirection
@ stub -version=0x600+ Wow64GetThreadContext
@ stdcall Wow64RevertWow64FsRedirection(ptr) kernel32_win7.Wow64RevertWow64FsRedirection
@ stub -version=0x600+ Wow64SetThreadContext
@ stub -version=0x600+ Wow64SuspendThread
@ stdcall WriteConsoleA(long ptr long ptr ptr) kernel32_win7.WriteConsoleA
@ stdcall WriteConsoleInputA(long ptr long ptr) kernel32_win7.WriteConsoleInputA
@ stdcall WriteConsoleInputVDMA(long long long long) kernel32_win7.WriteConsoleInputVDMA
@ stdcall WriteConsoleInputVDMW(long long long long) kernel32_win7.WriteConsoleInputVDMW
@ stdcall WriteConsoleInputW(long ptr long ptr) kernel32_win7.WriteConsoleInputW
@ stdcall WriteConsoleOutputA(long ptr long long ptr) kernel32_win7.WriteConsoleOutputA
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr) kernel32_win7.WriteConsoleOutputAttribute
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr) kernel32_win7.WriteConsoleOutputCharacterA
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr) kernel32_win7.WriteConsoleOutputCharacterW
@ stdcall WriteConsoleOutputW(long ptr long long ptr) kernel32_win7.WriteConsoleOutputW
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernel32_win7.WriteConsoleW
@ stdcall WriteFile(long ptr long ptr ptr) kernel32_win7.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernel32_win7.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernel32_win7.WriteFileGather
@ stdcall WritePrivateProfileSectionA(str str str) kernel32_win7.WritePrivateProfileSectionA
@ stdcall WritePrivateProfileSectionW(wstr wstr wstr) kernel32_win7.WritePrivateProfileSectionW
@ stdcall WritePrivateProfileStringA(str str str str) kernel32_win7.WritePrivateProfileStringA
@ stdcall WritePrivateProfileStringW(wstr wstr wstr wstr) kernel32_win7.WritePrivateProfileStringW
@ stdcall WritePrivateProfileStructA(str str ptr long str) kernel32_win7.WritePrivateProfileStructA
@ stdcall WritePrivateProfileStructW(wstr wstr ptr long wstr) kernel32_win7.WritePrivateProfileStructW
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernel32_win7.WriteProcessMemory
@ stdcall WriteProfileSectionA(str str) kernel32_win7.WriteProfileSectionA
@ stdcall WriteProfileSectionW(str str) kernel32_win7.WriteProfileSectionW
@ stdcall WriteProfileStringA(str str str) kernel32_win7.WriteProfileStringA
@ stdcall WriteProfileStringW(wstr wstr wstr) kernel32_win7.WriteProfileStringW
@ stdcall WriteTapemark(ptr long long long) kernel32_win7.WriteTapemark
@ stdcall ZombifyActCtx(ptr) kernel32_win7.ZombifyActCtx
@ stdcall -arch=x86_64,arm64 __C_specific_handler() ntdll.__C_specific_handler
@ stdcall -arch=x86_64,arm64 __chkstk() ntdll.__chkstk
;@ stdcall -arch=x86_64 __misaligned_access() ntdll.__misaligned_access
@ stdcall _hread(long ptr long) kernel32_win7._hread
@ stdcall _hwrite(long ptr long) kernel32_win7._hwrite
@ stdcall _lclose(long) kernel32_win7._lclose
@ stdcall _lcreat(str long) kernel32_win7._lcreat
@ stdcall _llseek(long long long) kernel32_win7._llseek
@ stdcall -arch=x86_64,arm64 _local_unwind() ntdll._local_unwind
@ stdcall _lopen(str long) kernel32_win7._lopen
@ stdcall _lread(long ptr long) kernel32_win7._hread
@ stdcall _lwrite(long ptr long) kernel32_win7._hwrite
@ stdcall lstrcat(str str) kernel32_win7.lstrcatA
@ stdcall lstrcatA(str str) kernel32_win7.lstrcatA
@ stdcall lstrcatW(wstr wstr) kernel32_win7.lstrcatW
@ stdcall lstrcmp(str str) kernel32_win7.lstrcmpA
@ stdcall lstrcmpA(str str) kernel32_win7.lstrcmpA
@ stdcall lstrcmpW(wstr wstr) kernel32_win7.lstrcmpW
@ stdcall lstrcmpi(str str) kernel32_win7.lstrcmpiA
@ stdcall lstrcmpiA(str str) kernel32_win7.lstrcmpiA
@ stdcall lstrcmpiW(wstr wstr) kernel32_win7.lstrcmpiW
@ stdcall lstrcpy(ptr str) kernel32_win7.lstrcpyA
@ stdcall lstrcpyA(ptr str) kernel32_win7.lstrcpyA
@ stdcall lstrcpyW(ptr wstr) kernel32_win7.lstrcpyW
@ stdcall lstrcpyn(ptr str long) lstrcpynA
@ stdcall lstrcpynA(ptr str long) kernel32_win7.lstrcpynA
@ stdcall lstrcpynW(ptr wstr long) kernel32_win7.lstrcpynW
@ stdcall lstrlen(str) kernel32_win7.lstrlen
@ stdcall lstrlenA(str) kernel32_win7.lstrlenA
@ stdcall lstrlenW(wstr) kernel32_win7.lstrlenW
;@ stdcall -arch=x86_64 uaw_lstrcmpW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrcmpiW(wstr wstr)
;@ stdcall -arch=x86_64 uaw_lstrlenW(wstr)
;@ stdcall -arch=x86_64 uaw_wcschr(wstr long)
;@ stdcall -arch=x86_64 uaw_wcscpy(ptr wstr)
;@ stdcall -arch=x86_64 uaw_wcsicmp(wstr wstr)
;@ stdcall -arch=x86_64 uaw_wcslen(wstr)
;@ stdcall -arch=x86_64 uaw_wcsrchr(wstr long)

