
;@ stdcall A_SHAFinal ; 6.0 and higher
;@ stdcall A_SHAInit ; 6.0 and higher
;@ stdcall A_SHAUpdate ; 6.0 and higher
;@ stdcall AitFireParentUsageEvent ; 6.1 and higher
;@ stdcall AitLogFeatureUsageByApp ; 6.1 and higher
;@ stdcall AlpcAdjustCompletionListConcurrencyCount ; 6.0 and higher
;@ stdcall AlpcFreeCompletionListMessage ; 6.0 and higher
;@ stdcall AlpcGetCompletionListLastMessageInformation ; 6.0 and higher
;@ stdcall AlpcGetCompletionListMessageAttributes ; 6.0 and higher
;@ stdcall AlpcGetHeaderSize ; 6.0 and higher
;@ stdcall AlpcGetMessageAttribute ; 6.0 and higher
;@ stdcall AlpcGetMessageFromCompletionList ; 6.0 and higher
;@ stdcall AlpcGetOutstandingCompletionListMessageCount ; 6.0 and higher
;@ stdcall AlpcInitializeMessageAttribute ; 6.0 and higher
;@ stdcall AlpcMaxAllowedMessageLength ; 6.0 and higher
;@ stdcall AlpcRegisterCompletionList ; 6.0 and higher
;@ stdcall AlpcRegisterCompletionListWorkerThread ; 6.0 and higher
;@ stdcall AlpcRundownCompletionList ; 6.1 and higher
;@ stdcall AlpcUnregisterCompletionList ; 6.0 and higher
;@ stdcall AlpcUnregisterCompletionListWorkerThread ; 6.0 and higher
@ stdcall CsrAllocateCaptureBuffer(long long)
;@ stdcall CsrAllocateCapturePointer ; NT3, NT4 only
@ stdcall CsrAllocateMessagePointer(ptr long ptr)
@ stdcall CsrCaptureMessageBuffer(ptr ptr long ptr)
@ stdcall CsrCaptureMessageMultiUnicodeStringsInPlace(ptr long ptr)
@ stdcall CsrCaptureMessageString(ptr str long long ptr)
@ stdcall CsrCaptureTimeout(long ptr)
@ stdcall CsrClientCallServer(ptr ptr long long)
@ stdcall CsrClientConnectToServer(str long ptr ptr ptr)
;@ stdcall CsrClientMaxMessage ; NT3 only
;@ stdcall CsrClientSendMessage ; NT3 only
;@ stdcall CsrClientThreadConnect ; NT3 only
@ stdcall CsrFreeCaptureBuffer(ptr)
@ stdcall CsrGetProcessId()
@ stdcall CsrIdentifyAlertableThread()
@ stdcall CsrNewThread()
@ stdcall CsrProbeForRead(ptr long long)
@ stdcall CsrProbeForWrite(ptr long long)
@ stdcall CsrSetPriorityClass(ptr ptr)
;@ stdcall CsrpProcessCallbackRequest ; 3.51 only
@ stdcall DbgBreakPoint()
@ varargs DbgPrint(str)
@ varargs DbgPrintEx(long long str)
@ varargs DbgPrintReturnControlC(str)
@ stdcall DbgPrompt(ptr ptr long)
@ stdcall DbgQueryDebugFilterState(long long)
@ stdcall DbgSetDebugFilterState(long long long)
@ stdcall DbgUiConnectToDbg()
@ stdcall DbgUiContinue(ptr long)
@ stdcall DbgUiConvertStateChangeStructure(ptr ptr)
@ stdcall DbgUiDebugActiveProcess(ptr)
@ stdcall DbgUiGetThreadDebugObject()
@ stdcall DbgUiIssueRemoteBreakin(ptr)
@ stdcall DbgUiRemoteBreakin()
@ stdcall DbgUiSetThreadDebugObject(ptr)
@ stdcall DbgUiStopDebugging(ptr)
@ stdcall DbgUiWaitStateChange(ptr ptr)
@ stdcall DbgUserBreakPoint()
@ stdcall -arch=i386 KiFastSystemCall()
@ stdcall -arch=i386 KiFastSystemCallRet()
@ stdcall -arch=i386 KiIntSystemCall()
@ stdcall -arch=i386,x86_64 ExpInterlockedPopEntrySListEnd()
@ stdcall -arch=i386,x86_64 ExpInterlockedPopEntrySListFault()
@ stdcall -arch=i386,x86_64 ExpInterlockedPopEntrySListResume()
@ stdcall KiRaiseUserExceptionDispatcher()
@ stdcall KiUserApcDispatcher(ptr ptr ptr ptr)
@ stdcall KiUserCallbackDispatcher(ptr ptr long) ; CHECKME
@ stdcall KiUserExceptionDispatcher(ptr ptr)
;@ stdcall LdrAccessOutOfProcessResource
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stdcall LdrAddRefDll(long ptr)
;@ stdcall LdrAlternateResourcesEnabled
;@ stdcall LdrCreateOutOfProcessImage
;@ stdcall LdrDestroyOutOfProcessImage
@ stdcall LdrDisableThreadCalloutsForDll(long)
@ stdcall LdrEnumResources(ptr ptr long ptr ptr)
@ stdcall LdrEnumerateLoadedModules(long ptr long)
;@ stdcall LdrFindCreateProcessManifest ; 5.1 and 5.2 only
@ stdcall LdrFindEntryForAddress(ptr ptr)
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
;@ stdcall LdrFindResourceEx_U ; 5.1 and higher
@ stdcall LdrFindResource_U(long ptr long ptr)
;@ stdcall LdrFlushAlternateResourceModules
@ stdcall LdrGetDllHandle(wstr long ptr ptr)
@ stdcall LdrGetDllHandleEx(long wstr long ptr ptr)
@ stdcall LdrGetProcedureAddress(ptr ptr long ptr)
;@ stdcall LdrHotPatchRoutine
;@ stdcall LdrInitShimEngineDynamic
@ stdcall LdrInitializeThunk(long long long long)
@ stdcall LdrLoadAlternateResourceModule(ptr ptr)
@ stdcall LdrLoadDll(wstr long ptr ptr)
@ stdcall LdrLockLoaderLock(long ptr ptr)
;@ stdcall LdrOpenImageFileOptionsKey ; 5.2 SP1 and higher
@ stdcall LdrProcessRelocationBlock(ptr long ptr long)
@ stdcall LdrQueryImageFileExecutionOptions(ptr str long ptr long ptr)
@ stdcall LdrQueryProcessModuleInformation(ptr long ptr)
;@ stdcall LdrSetAppCompatDllRedirectionCallback
;@ stdcall LdrSetDllManifestProber
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ stdcall LdrUnloadAlternateResourceModule(ptr)
@ stdcall LdrUnloadDll(ptr)
@ stdcall LdrUnlockLoaderLock(long long)
@ stdcall LdrVerifyImageMatchesChecksum(ptr long long long)
@ extern NlsAnsiCodePage
@ extern NlsMbCodePageTag
@ extern NlsMbOemCodePageTag
@ stdcall NtAcceptConnectPort(ptr long ptr long long ptr)
@ stdcall NtAccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall NtAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr)
@ stdcall NtAccessCheckByType(ptr ptr ptr long ptr long ptr ptr long ptr ptr)
@ stdcall NtAccessCheckByTypeAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall NtAccessCheckByTypeResultList(ptr ptr ptr long ptr long ptr ptr long ptr ptr)
@ stdcall NtAccessCheckByTypeResultListAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall NtAccessCheckByTypeResultListAndAuditAlarmByHandle(ptr ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall NtAddAtom(ptr long ptr)
@ stdcall NtAddBootEntry(ptr long)
@ stdcall NtAddDriverEntry(ptr long) ; 5.2 and higher
@ stdcall NtAdjustGroupsToken(long long ptr long ptr ptr)
@ stdcall NtAdjustPrivilegesToken(long long long long long long)
@ stdcall NtAlertResumeThread(long ptr)
@ stdcall NtAlertThread(long)
@ stdcall NtAllocateLocallyUniqueId(ptr)
@ stdcall NtAllocateUserPhysicalPages(ptr ptr ptr)
@ stdcall NtAllocateUuids(ptr ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr ptr ptr long long)
@ stdcall NtApphelpCacheControl(long ptr)
@ stdcall NtAreMappedFilesTheSame(ptr ptr)
@ stdcall NtAssignProcessToJobObject(long long)
@ stdcall NtCallbackReturn(ptr long long)
@ stdcall NtCancelDeviceWakeupRequest(ptr)
@ stdcall NtCancelIoFile(long ptr)
;@ stdcall NtCancelIoFileEx(long ptr ptr) ; 6.0 and higher
@ stdcall NtCancelTimer(long ptr)
@ stdcall NtClearEvent(long)
@ stdcall NtClose(long)
@ stdcall NtCloseObjectAuditAlarm(ptr ptr long)
@ stdcall NtCompactKeys(long ptr)
@ stdcall NtCompareTokens(ptr ptr ptr)
@ stdcall NtCompleteConnectPort(ptr)
@ stdcall NtCompressKey(ptr)
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtContinue(ptr long)
@ stdcall NtCreateDebugObject(ptr long ptr long)
@ stdcall NtCreateDirectoryObject(long long long)
@ stdcall NtCreateEvent(long long long long long)
@ stdcall NtCreateEventPair(ptr long ptr)
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr)
@ stdcall NtCreateIoCompletion(ptr long ptr long)
@ stdcall NtCreateJobObject(ptr long ptr)
@ stdcall NtCreateJobSet(long ptr long)
@ stdcall NtCreateKey(ptr long ptr long ptr long long)
@ stdcall NtCreateKeyedEvent(ptr long ptr long)
@ stdcall NtCreateMailslotFile(long long long long long long long long)
@ stdcall NtCreateMutant(ptr long ptr long)
@ stdcall NtCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall NtCreatePagingFile(long long long long)
@ stdcall NtCreatePort(ptr ptr long long ptr)
@ stdcall NtCreateProcess(ptr long ptr ptr long ptr ptr ptr)
@ stdcall NtCreateProcessEx(ptr long ptr ptr long ptr ptr ptr long)
@ stdcall NtCreateProfile(ptr ptr ptr long long ptr long long long) ; CHECKME
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtCreateSemaphore(ptr long ptr long long)
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stdcall NtCreateThread(ptr long ptr ptr ptr ptr ptr long)
@ stdcall NtCreateTimer(ptr long ptr long)
@ stdcall NtCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtCreateWaitablePort(ptr ptr long long long)
@ stdcall -arch=win32 NtCurrentTeb() _NtCurrentTeb
@ stdcall NtDebugActiveProcess(ptr ptr)
@ stdcall NtDebugContinue(ptr ptr long)
@ stdcall NtDelayExecution(long ptr)
@ stdcall NtDeleteAtom(long)
@ stdcall NtDeleteBootEntry(long)
@ stdcall NtDeleteFile(ptr)
@ stdcall NtDeleteKey(long)
@ stdcall NtDeleteObjectAuditAlarm(ptr ptr long)
@ stdcall NtDeleteValueKey(long ptr)
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long)
@ stdcall NtDisplayString(ptr)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long long long long long)
@ stdcall NtEnumerateBootEntries(ptr ptr)
;@ stdcall NtEnumerateBus ; 3.51 only
@ stdcall NtEnumerateKey (long long long long long long)
@ stdcall NtEnumerateSystemEnvironmentValuesEx(long ptr long)
@ stdcall NtEnumerateValueKey(long long long long long long)
@ stdcall NtExtendSection(ptr ptr)
@ stdcall NtFilterToken(ptr long ptr ptr ptr ptr)
@ stdcall NtFindAtom(ptr long ptr)
@ stdcall NtFlushBuffersFile(long ptr)
@ stdcall NtFlushInstructionCache(long ptr long)
@ stdcall NtFlushKey(long)
@ stdcall NtFlushVirtualMemory(long ptr ptr long)
@ stdcall NtFlushWriteBuffer()
@ stdcall NtFreeUserPhysicalPages(ptr ptr ptr)
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stdcall NtFsControlFile(long long long long long long long long long long)
@ stdcall NtGetContextThread(long ptr)
@ stdcall NtGetCurrentProcessorNumber() ; 5.2 and higher
@ stdcall NtGetDevicePowerState(ptr ptr)
@ stdcall NtGetPlugPlayEvent(long long ptr long)
;@ stdcall NtGetTickCount()
@ stdcall NtGetWriteWatch(long long ptr long ptr ptr ptr)
@ stdcall NtImpersonateAnonymousToken(ptr)
@ stdcall NtImpersonateClientOfPort(ptr ptr)
@ stdcall NtImpersonateThread(ptr ptr ptr)
@ stdcall NtInitializeRegistry(long)
@ stdcall NtInitiatePowerAction (long long long long)
@ stdcall NtIsProcessInJob(long long)
@ stdcall NtIsSystemResumeAutomatic()
@ stdcall NtListenPort(ptr ptr)
@ stdcall NtLoadDriver(ptr)
@ stdcall NtLoadKey2(ptr ptr long)
@ stdcall NtLoadKey(ptr ptr)
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stdcall NtLockProductActivationKeys(ptr ptr)
@ stdcall NtLockRegistryKey(ptr)
@ stdcall NtLockVirtualMemory(long ptr ptr long)
@ stdcall NtMakePermanentObject(ptr)
@ stdcall NtMakeTemporaryObject(long)
@ stdcall NtMapUserPhysicalPages(ptr ptr ptr)
@ stdcall NtMapUserPhysicalPagesScatter(ptr ptr ptr)
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall NtModifyBootEntry(ptr)
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
@ stdcall NtNotifyChangeMultipleKeys(ptr long ptr ptr ptr ptr ptr long long ptr long long)
@ stdcall NtOpenDirectoryObject(long long long)
@ stdcall NtOpenEvent(long long long)
@ stdcall NtOpenEventPair(ptr long ptr)
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenIoCompletion(ptr long ptr)
@ stdcall NtOpenJobObject(ptr long ptr)
@ stdcall NtOpenKey(ptr long ptr)
@ stdcall NtOpenKeyedEvent(ptr long ptr)
@ stdcall NtOpenMutant(ptr long ptr)
@ stdcall NtOpenObjectAuditAlarm(ptr ptr ptr ptr ptr ptr long long ptr long long ptr)
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stdcall NtOpenSection(ptr long ptr)
@ stdcall NtOpenSemaphore(long long ptr)
@ stdcall NtOpenSymbolicLinkObject (ptr long ptr)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long ptr)
@ stdcall NtOpenThreadTokenEx(long long long long ptr)
@ stdcall NtOpenTimer(ptr long ptr)
@ stdcall NtPlugPlayControl(ptr ptr long)
@ stdcall NtPowerInformation(long ptr long ptr long)
@ stdcall NtPrivilegeCheck(ptr ptr ptr)
@ stdcall NtPrivilegeObjectAuditAlarm(ptr ptr ptr long ptr long)
@ stdcall NtPrivilegedServiceAuditAlarm(ptr ptr ptr ptr long)
@ stdcall NtProtectVirtualMemory(long ptr ptr long ptr)
@ stdcall NtPulseEvent(long ptr)
@ stdcall NtQueryAttributesFile(ptr ptr)
@ stdcall NtQueryBootEntryOrder(ptr ptr)
@ stdcall NtQueryBootOptions(ptr ptr)
@ stdcall NtQueryDebugFilterState(long long)
@ stdcall NtQueryDefaultLocale(long ptr)
@ stdcall NtQueryDefaultUILanguage(ptr)
@ stdcall NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall NtQueryDirectoryObject(long ptr long long long ptr ptr)
@ stdcall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall NtQueryEvent(long long ptr long ptr)
@ stdcall NtQueryFullAttributesFile(ptr ptr)
@ stdcall NtQueryInformationAtom(long long ptr long ptr)
@ stdcall NtQueryInformationFile(long ptr ptr long long)
@ stdcall NtQueryInformationJobObject(long long ptr long ptr)
@ stdcall NtQueryInformationPort(ptr long ptr long ptr)
@ stdcall NtQueryInformationProcess(long long ptr long ptr)
@ stdcall NtQueryInformationThread(long long ptr long ptr)
@ stdcall NtQueryInformationToken(long long ptr long ptr)
@ stdcall NtQueryInstallUILanguage(ptr)
@ stdcall NtQueryIntervalProfile(long ptr)
@ stdcall NtQueryIoCompletion(long long ptr long ptr)
@ stdcall NtQueryKey (long long ptr long ptr)
@ stdcall NtQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall NtQueryMutant(long long ptr long ptr)
@ stdcall NtQueryObject(long long long long long)
@ stdcall NtQueryOpenSubKeys(ptr ptr)
@ stdcall NtQueryPerformanceCounter(ptr ptr)
@ stdcall NtQueryPortInformationProcess()
@ stdcall NtQueryQuotaInformationFile(ptr ptr ptr long long ptr long ptr long)
@ stdcall NtQuerySection (long long long long long)
@ stdcall NtQuerySecurityObject (long long long long long)
@ stdcall NtQuerySemaphore (long long ptr long ptr)
@ stdcall NtQuerySymbolicLinkObject(long ptr ptr)
@ stdcall NtQuerySystemEnvironmentValue(ptr ptr long ptr)
@ stdcall NtQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
@ stdcall NtQuerySystemInformation(long long long long)
@ stdcall NtQuerySystemTime(ptr)
@ stdcall NtQueryTimer(ptr long ptr long ptr)
@ stdcall NtQueryTimerResolution(long long long)
@ stdcall NtQueryValueKey(long long long long long long)
@ stdcall NtQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtQueueApcThread(long ptr long long long)
@ stdcall NtRaiseException(ptr ptr long)
@ stdcall NtRaiseHardError(long long long ptr long ptr)
@ stdcall NtReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtReadFileScatter(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtReadRequestData(ptr ptr long ptr long ptr)
@ stdcall NtReadVirtualMemory(long ptr ptr long ptr)
;@ stdcall NtRegisterNewDevice ; 3.51 only
@ stdcall NtRegisterThreadTerminatePort(ptr)
@ stdcall NtReleaseKeyedEvent(ptr ptr long ptr)
@ stdcall NtReleaseMutant(long ptr)
;@ stdcall NtReleaseProcessMutant ; 3.51 only
@ stdcall NtReleaseSemaphore(long long ptr)
@ stdcall NtRemoveIoCompletion(ptr ptr ptr ptr ptr)
@ stdcall NtRemoveProcessDebug(ptr ptr)
@ stdcall NtRenameKey(ptr ptr)
@ stdcall NtReplaceKey(ptr long ptr)
@ stdcall NtReplyPort(ptr ptr)
@ stdcall NtReplyWaitReceivePort(ptr ptr ptr ptr)
@ stdcall NtReplyWaitReceivePortEx(ptr ptr ptr ptr ptr)
@ stdcall NtReplyWaitReplyPort(ptr ptr)
@ stdcall NtRequestDeviceWakeup(ptr)
@ stdcall NtRequestPort(ptr ptr)
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr)
@ stdcall NtRequestWakeupLatency(long)
@ stdcall NtResetEvent(long ptr)
@ stdcall NtResetWriteWatch(long ptr long)
@ stdcall NtRestoreKey(long long long)
@ stdcall NtResumeProcess(ptr)
@ stdcall NtResumeThread(long long)
@ stdcall NtSaveKey(long long)
@ stdcall NtSaveKeyEx(ptr ptr long)
@ stdcall NtSaveMergedKeys(ptr ptr ptr)
@ stdcall NtSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtSetBootEntryOrder(ptr ptr)
@ stdcall NtSetBootOptions(ptr long)
@ stdcall NtSetContextThread(long ptr)
@ stdcall NtSetDebugFilterState(long long long)
@ stdcall NtSetDefaultHardErrorPort(ptr)
@ stdcall NtSetDefaultLocale(long long)
@ stdcall NtSetDefaultUILanguage(long)
@ stdcall NtSetEaFile(long ptr ptr long)
@ stdcall NtSetEvent(long long)
@ stdcall NtSetEventBoostPriority(ptr)
@ stdcall NtSetHighEventPair(ptr)
@ stdcall NtSetHighWaitLowEventPair(ptr)
;@ stdcall NtSetHighWaitLowThread ; 3.51 and 4.0 only
@ stdcall NtSetInformationDebugObject(ptr long ptr long ptr)
@ stdcall NtSetInformationFile(long long long long long)
@ stdcall NtSetInformationJobObject(long long ptr long)
@ stdcall NtSetInformationKey(long long ptr long)
@ stdcall NtSetInformationObject(long long ptr long)
@ stdcall NtSetInformationProcess(long long long long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stdcall NtSetInformationToken(long long ptr long)
@ stdcall NtSetIntervalProfile(long long)
@ stdcall NtSetIoCompletion(ptr long ptr long long)
@ stdcall NtSetLdtEntries(long double long double) ; CHECKME
@ stdcall NtSetLowEventPair(ptr)
@ stdcall NtSetLowWaitHighEventPair(ptr)
;@ stdcall NtSetLowWaitHighThread ; 3.51 and 4.0 only
@ stdcall NtSetQuotaInformationFile(ptr ptr ptr long)
@ stdcall NtSetSecurityObject(long long ptr)
@ stdcall NtSetSystemEnvironmentValue(ptr ptr)
@ stdcall NtSetSystemEnvironmentValueEx(ptr ptr)
@ stdcall NtSetSystemInformation(long ptr long)
@ stdcall NtSetSystemPowerState(long long long)
@ stdcall NtSetSystemTime(ptr ptr)
@ stdcall NtSetThreadExecutionState(long ptr)
@ stdcall NtSetTimer(long ptr ptr ptr long long ptr)
@ stdcall NtSetTimerResolution(long long ptr)
@ stdcall NtSetUuidSeed(ptr)
@ stdcall NtSetValueKey(long long long long long long)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall NtShutdownSystem(long)
@ stdcall NtSignalAndWaitForSingleObject(long long long ptr)
@ stdcall NtStartProfile(ptr)
@ stdcall NtStopProfile(ptr)
@ stdcall NtSuspendProcess(ptr)
@ stdcall NtSuspendThread(long ptr)
@ stdcall NtSystemDebugControl(long ptr long ptr long ptr)
@ stdcall NtTerminateJobObject(long long)
@ stdcall NtTerminateProcess(long long)
@ stdcall NtTerminateThread(long long)
@ stdcall NtTestAlert()
@ stdcall NtTraceEvent(long long long ptr)
@ stdcall NtTranslateFilePath(ptr long ptr long)
@ stdcall NtUnloadDriver(ptr)
@ stdcall NtUnloadKey(long)
@ stdcall NtUnloadKeyEx(ptr ptr)
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stdcall NtUnlockVirtualMemory(long ptr ptr long)
@ stdcall NtUnmapViewOfSection(long ptr)
@ stdcall NtVdmControl(long ptr)
;@ stdcall NtW32Call(long ptr long ptr ptr)
@ stdcall NtWaitForDebugEvent(ptr long ptr ptr)
@ stdcall NtWaitForKeyedEvent(ptr ptr long ptr)
@ stdcall NtWaitForMultipleObjects(long ptr long long ptr)
;@ stdcall NtWaitForProcessMutant ; 3.51 only
@ stdcall NtWaitForSingleObject(long long long)
@ stdcall NtWaitHighEventPair(ptr)
@ stdcall NtWaitLowEventPair(ptr)
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtWriteRequestData(ptr ptr long ptr long ptr)
@ stdcall NtWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall NtYieldExecution()
;@ stdcall PfxFindPrefix
;@ stdcall PfxInitialize
;@ stdcall PfxInsertPrefix
;@ stdcall PfxRemovePrefix
;@ stdcall PropertyLengthAsVariant
;@ stdcall RtlAbortRXact
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAcquirePebLock()
@ stdcall RtlAcquirePrivilege(ptr long long ptr)
@ stdcall RtlAcquireResourceExclusive(ptr long)
@ stdcall RtlAcquireResourceShared(ptr long)
@ stdcall RtlAcquireSRWLockExclusive(ptr)
@ stdcall RtlAcquireSRWLockShared(ptr)
@ stdcall RtlActivateActivationContext(long ptr ptr)
;@ stdcall RtlActivateActivationContextEx
@ fastcall RtlActivateActivationContextUnsafeFast(ptr ptr)
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAccessDeniedAce(ptr long long ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
;@ stdcall RtlAddActionToRXact
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
;@ stdcall RtlAddAttributeActionToRXact
@ stdcall RtlAddAuditAccessAce(ptr long long ptr long long)
@ stdcall RtlAddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall RtlAddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
;@ stdcall RtlAddCompoundAce
;@ stdcall RtlAddRange ; 5.0 and 5.1 only
@ stdcall -arch=x86_64 RtlAddFunctionTable(ptr long long)
@ stdcall RtlAddMandatoryAce(ptr long long long long ptr)
@ stdcall RtlAddRefActivationContext(ptr)
;@ stdcall RtlAddRefMemoryStream
@ stdcall RtlAddVectoredContinueHandler(long ptr)
@ stdcall RtlAddVectoredExceptionHandler(long ptr)
;@ stdcall RtlAddressInSectionTable
@ stdcall RtlAdjustPrivilege(long long long ptr)
@ stdcall RtlAllocateActivationContextStack(ptr) ; CHEKME
@ stdcall RtlAllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall RtlAllocateHandle(ptr ptr)
@ stdcall RtlAllocateHeap(ptr long ptr)
@ stdcall RtlAnsiCharToUnicodeChar(ptr)
@ stdcall RtlAnsiStringToUnicodeSize(ptr) RtlxAnsiStringToUnicodeSize
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
;@ stdcall RtlAppendPathElement
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
;@ stdcall RtlApplicationVerifierStop
;@ stdcall RtlApplyRXact
;@ stdcall RtlApplyRXactNoFlush
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
;@ stdcall RtlAssert2
@ stdcall RtlAssert(ptr ptr long ptr)
;@ stdcall RtlCancelTimer
@ stdcall -register RtlCaptureContext(ptr)
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr)
;@ stdcall RtlCaptureStackContext
@ stdcall RtlCharToInteger(ptr long ptr)
;@ stdcall RtlCheckForOrphanedCriticalSections
;@ stdcall RtlCheckProcessParameters
@ stdcall RtlCheckRegistryKey(long ptr)
@ stdcall RtlClearAllBits(ptr)
@ stdcall RtlClearBits(ptr long long)
;@ stdcall RtlCloneMemoryStream
;@ stdcall RtlClosePropertySet ; NT 4.0 only
;@ stdcall RtlCommitMemoryStream
@ stdcall RtlCompactHeap(long long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString (ptr ptr long)
@ stdcall RtlCompressBuffer(long ptr long ptr long long ptr ptr)
@ stdcall RtlComputeCrc32(long ptr long)
@ stdcall RtlComputeImportTableHash(ptr ptr long)
;@ stdcall RtlComputePrivatizedDllName_U
;@ stdcall RtlConsoleMultiByteToUnicodeN
@ stdcall RtlConvertExclusiveToShared(ptr)
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
;@ stdcall RtlConvertPropertyToVariant
@ stdcall RtlConvertSharedToExclusive(ptr)
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
;@ stdcall RtlConvertToAutoInheritSecurityObject
;@ stdcall RtlConvertUiListToApiList
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
;@ stdcall RtlConvertVariantToProperty
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
;@ stdcall RtlCopyMappedMemory
;@ stdcall RtlCopyMemoryStreamTo
;@ stdcall RtlCopyOutOfProcessMemoryStreamTo
;@ stdcall RtlCopyRangeList ; 5.0 and 5.1 only
@ stdcall RtlCopySecurityDescriptor(ptr ptr)
@ stdcall RtlCopySid(long ptr ptr)
@ stdcall RtlCopySidAndAttributesArray(long ptr long ptr ptr ptr ptr)
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stdcall RtlCreateActivationContext(ptr ptr)
;@ stdcall RtlCreateAndSetSD
@ stdcall RtlCreateAtomTable(long ptr)
@ stdcall RtlCreateBootStatusDataFile()
@ stdcall RtlCreateEnvironment(long ptr)
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stdcall RtlCreateProcessParameters(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
;@ stdcall RtlCreatePropertySet ; 4.0 only
@ stdcall RtlCreateQueryDebugBuffer(long long)
@ stdcall RtlCreateRegistryKey(long wstr)
@ stdcall RtlCreateSecurityDescriptor(ptr long)
@ stdcall RtlCreateSystemVolumeInformationFolder(ptr)
@ stdcall RtlCreateTagHeap(ptr long str str)
@ stdcall RtlCreateTimer(ptr ptr ptr ptr long long long)
@ stdcall RtlCreateTimerQueue(ptr)
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUnicodeStringFromAsciiz(ptr str)
@ stdcall RtlCreateUserProcess(ptr long ptr ptr ptr ptr long ptr ptr ptr)
;@ stdcall RtlCreateUserSecurityObject
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stdcall RtlCustomCPToUnicodeN(ptr wstr long ptr str long)
@ stdcall RtlCutoverTimeToSystemTime(ptr ptr ptr long)
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stdcall RtlDeactivateActivationContext(long long)
@ fastcall RtlDeactivateActivationContextUnsafeFast(ptr)
;@ stdcall RtlDebugPrintTimes
@ stdcall RtlDecodePointer(ptr)
@ stdcall RtlDecodeSystemPointer(ptr) RtlEncodeSystemPointer
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
;@ stdcall RtlDefaultNpAcl
@ stdcall RtlDelete(ptr)
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stdcall RtlDeleteCriticalSection(ptr)
@ stdcall RtlDeleteElementGenericTable(ptr ptr)
@ stdcall RtlDeleteElementGenericTableAvl(ptr ptr)
@ cdecl -arch=x86_64 RtlDeleteFunctionTable(ptr)
@ stdcall RtlDeleteNoSplay(ptr ptr)
@ stdcall RtlDeleteOwnersRanges(ptr ptr)
@ stdcall RtlDeleteRange(ptr long long long long ptr)
@ stdcall RtlDeleteRegistryValue(long ptr ptr)
@ stdcall RtlDeleteResource(ptr)
@ stdcall RtlDeleteSecurityObject(ptr)
@ stdcall RtlDeleteTimer(ptr ptr ptr)
@ stdcall RtlDeleteTimerQueue(ptr)
@ stdcall RtlDeleteTimerQueueEx(ptr ptr)
@ stdcall RtlDeregisterWait(ptr)
@ stdcall RtlDeregisterWaitEx(ptr ptr)
@ stdcall RtlDestroyAtomTable(ptr)
@ stdcall RtlDestroyEnvironment(ptr)
@ stdcall RtlDestroyHandleTable(ptr)
@ stdcall RtlDestroyHeap(long)
@ stdcall RtlDestroyProcessParameters(ptr)
@ stdcall RtlDestroyQueryDebugBuffer(ptr)
@ stdcall RtlDetermineDosPathNameType_U(wstr)
@ stdcall RtlDllShutdownInProgress()
@ stdcall RtlDnsHostNameToComputerName(ptr ptr long)
@ stdcall RtlDoesFileExists_U(wstr)
@ stdcall RtlDosApplyFileIsolationRedirection_Ustr(long ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlDosPathNameToNtPathName_U(wstr ptr ptr ptr)
;@ stdcall RtlDosPathNameToNtPathName_U_WithStatus ; 5.2 SP1, and higher
@ stdcall RtlDosPathNameToRelativeNtPathName_U(ptr ptr ptr ptr) ; CHECKME
;@ stdcall RtlDosPathNameToRelativeNtPathName_U_WithStatus
@ stdcall RtlDosSearchPath_U(wstr wstr wstr long ptr ptr)
@ stdcall RtlDosSearchPath_Ustr(long ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDumpResource(ptr)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall RtlEmptyAtomTable(ptr long)
;@ stdcall RtlEnableEarlyCriticalSectionEventCreation
@ stdcall RtlEncodePointer(ptr)
@ stdcall RtlEncodeSystemPointer(ptr)
@ stdcall -arch=win32 -ret64 RtlEnlargedIntegerMultiply(long long)
@ stdcall -arch=win32 RtlEnlargedUnsignedDivide(double long ptr)
@ stdcall -arch=win32 -ret64 RtlEnlargedUnsignedMultiply(long long)
@ stdcall RtlEnterCriticalSection(ptr)
@ stdcall RtlEnumProcessHeaps(ptr ptr)
@ stdcall RtlEnumerateGenericTable(ptr long)
@ stdcall RtlEnumerateGenericTableAvl(ptr long)
@ stdcall RtlEnumerateGenericTableLikeADirectory(ptr ptr ptr long ptr ptr ptr)
@ stdcall RtlEnumerateGenericTableWithoutSplaying(ptr ptr)
@ stdcall RtlEnumerateGenericTableWithoutSplayingAvl(ptr ptr)
;@ stdcall RtlEnumerateProperties ; 4.0 only
@ stdcall RtlEqualComputerName(ptr ptr)
@ stdcall RtlEqualDomainName(ptr ptr)
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualPrefixSid(ptr ptr)
@ stdcall RtlEqualSid(long long)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall RtlEraseUnicodeString(ptr)
@ stdcall RtlExitUserThread(long)
@ stdcall RtlExpandEnvironmentStrings_U(ptr ptr ptr ptr)
@ stdcall RtlExtendHeap(ptr long ptr ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedIntegerMultiply(double long)
@ stdcall -arch=win32 -ret64 RtlExtendedLargeIntegerDivide(double long ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedMagicDivide(double double long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall RtlFillMemoryUlong(ptr long long)
;@ stdcall RtlFinalReleaseOutOfProcessMemoryStream
;@ stdcall RtlFindActivationContextSectionGuid
@ stdcall RtlFindActivationContextSectionString(long ptr long ptr ptr)
@ stdcall RtlFindCharInUnicodeString(long ptr ptr ptr)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
;@ stdcall RtlFindLastBackwardRunSet(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(double)
@ stdcall RtlFindLongestRunClear(ptr long)
@ stdcall RtlFindLongestRunSet(ptr long)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(double)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
;@ stdcall RtlFindNextForwardRunSet(ptr long ptr)
@ stdcall RtlFindRange(ptr long long long long long long long long ptr ptr ptr)
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
;@ stdcall RtlFindSetRuns(ptr ptr long long)
@ stdcall RtlFirstEntrySList(ptr)
@ stdcall RtlFirstFreeAce(ptr ptr)
;@ stdcall RtlFlushPropertySet ; 4.0 only
@ stdcall RtlFlushSecureMemoryCache(ptr ptr)
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFormatMessage(ptr long long long long ptr ptr long)
;@ stdcall RtlFormatMessageEx
;@ stdcall RtlFreeActivationContextStack
@ stdcall RtlFreeAnsiString(long)
@ stdcall RtlFreeHandle(ptr ptr)
@ stdcall RtlFreeHeap(long long long)
@ stdcall RtlFreeOemString(ptr)
@ stdcall RtlFreeRangeList(ptr)
@ stdcall RtlFreeSid(long)
@ stdcall RtlFreeThreadActivationContextStack()
@ stdcall RtlFreeUnicodeString(ptr)
@ stdcall RtlFreeUserThreadStack(ptr ptr) ; 4.0 to 5.2 only
@ stdcall RtlGUIDFromString(ptr ptr)
@ stdcall RtlGenerate8dot3Name(ptr ptr long ptr)
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetActiveActivationContext(ptr)
@ stdcall RtlGetCallersAddress(ptr ptr)
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
;@ stdcall RtlGetCriticalSectionRecursionCount
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetCurrentPeb()
@ stdcall RtlGetCurrentProcessorNumber() ; 5.2 SP1 and higher
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetElementGenericTable(ptr long)
@ stdcall RtlGetElementGenericTableAvl(ptr long)
@ stdcall RtlGetFirstRange(ptr ptr ptr)
;@ stdcall RtlGetFrame
@ stdcall RtlGetFullPathName_U(wstr long ptr ptr)
@ stdcall RtlGetFullPathName_UstrEx(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetLastNtStatus()
@ stdcall RtlGetLastWin32Error()
;@ stdcall RtlGetLengthWithoutLastFullDosOrNtPathElement
; Yes, Microsoft really misspelled this one!
;@ stdcall RtlGetLengthWithoutTrailingPathSeperators
@ stdcall RtlGetLongestNtPathLength()
;@ stdcall RtlGetNativeSystemInformation
@ stdcall RtlGetNextRange(ptr ptr long)
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetNtVersionNumbers(ptr ptr ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
;@ stdcall RtlGetProductInfo(long long long long ptr)
@ stdcall RtlGetProcessHeaps(long ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetSecurityDescriptorRMControl(ptr ptr)
@ stdcall RtlGetSetBootStatusData(ptr long long ptr long long)
@ stdcall RtlGetThreadErrorMode()
;@ stdcall RtlGetUnloadEventTrace
@ stdcall RtlGetUserInfoHeap(ptr long ptr ptr ptr)
@ stdcall RtlGetVersion(ptr)
;@ stdcall RtlGuidToPropertySetName ; 4.0 only
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stdcall RtlIdentifierAuthoritySid(ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlImageNtHeaderEx(long ptr double ptr)
@ stdcall RtlImageRvaToSection(ptr long long)
@ stdcall RtlImageRvaToVa(ptr long long ptr)
@ stdcall RtlImpersonateSelf(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stdcall RtlInitCodePageTable(ptr ptr)
;@ stdcall RtlInitMemoryStream
@ stdcall RtlInitNlsTables(ptr ptr ptr ptr)
;@ stdcall RtlInitOutOfProcessMemoryStream
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
;@ stdcall RtlInitializeAtomPackage
@ stdcall RtlInitializeBitMap(ptr long long)
@ stdcall RtlInitializeContext(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeCriticalSection(ptr)
@ stdcall RtlInitializeCriticalSectionAndSpinCount(ptr long)
;@ stdcall RtlInitializeCriticalSectionEx(ptr long long)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeHandleTable(long long ptr)
;@ stdcall RtlInitializeRXact
@ stdcall RtlInitializeRangeList(ptr)
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
@ stdcall RtlInitializeSRWLock(ptr)
;@ stdcall RtlInitializeStackTraceDataBase ; 5.1 SP2 and SP3, and 5.2 only
@ stdcall RtlInsertElementGenericTable(ptr ptr long ptr)
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ stdcall -arch=x86_64 RtlInstallFunctionTableCallback(double double long ptr ptr ptr)
@ stdcall RtlInt64ToUnicodeString(double long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stdcall -arch=win32 -ret64 RtlInterlockedCompareExchange64(ptr double double)
@ stdcall -arch=i386,x86_64 RtlInterlockedFlushSList(ptr)
@ stdcall -arch=i386,x86_64 RtlInterlockedPopEntrySList(ptr)
@ stdcall -arch=i386,x86_64 RtlInterlockedPushEntrySList(ptr ptr)
@ stdcall RtlInterlockedPushListSList(ptr ptr ptr long)
@ stdcall RtlInvertRangeList(ptr ptr)
@ stdcall RtlIpv4AddressToStringA(ptr ptr)
@ stdcall RtlIpv4AddressToStringExA(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringExW(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringW(ptr ptr)
@ stdcall RtlIpv4StringToAddressA(str long ptr ptr)
@ stdcall RtlIpv4StringToAddressExA(str long ptr ptr)
@ stdcall RtlIpv4StringToAddressExW(wstr long ptr ptr)
@ stdcall RtlIpv4StringToAddressW(wstr long ptr ptr)
@ stdcall RtlIpv6AddressToStringA(ptr ptr)
@ stdcall RtlIpv6AddressToStringExA(ptr long long ptr ptr)
@ stdcall RtlIpv6AddressToStringExW(ptr long long ptr ptr)
@ stdcall RtlIpv6AddressToStringW(ptr ptr)
@ stdcall RtlIpv6StringToAddressA(str ptr ptr)
@ stdcall RtlIpv6StringToAddressExA(str ptr ptr ptr)
@ stdcall RtlIpv6StringToAddressExW(wstr ptr ptr ptr)
@ stdcall RtlIpv6StringToAddressW(wstr ptr ptr)
@ stdcall RtlIsActivationContextActive(ptr)
;@ stdcall RtlIsCriticalSectionLocked
;@ stdcall RtlIsCriticalSectionLockedByThread
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stdcall RtlIsGenericTableEmpty(ptr)
@ stdcall RtlIsGenericTableEmptyAvl(ptr)
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stdcall RtlIsRangeAvailable(ptr long long long long long long ptr ptr ptr)
@ stdcall RtlIsTextUnicode(ptr long ptr)
@ stdcall RtlIsThreadWithinLoaderCallout()
@ stdcall RtlIsValidHandle(ptr ptr)
@ stdcall RtlIsValidIndexHandle(ptr long ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(double double)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(double double ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(double)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(double double)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stdcall RtlLeaveCriticalSection(ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
@ stdcall RtlLockBootStatusData(ptr)
@ stdcall RtlLockHeap(long)
;@ stdcall RtlLockMemoryStreamRegion
;@ stdcall RtlLogStackBackTrace
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stdcall RtlLookupElementGenericTable(ptr ptr)
@ stdcall RtlLookupElementGenericTableAvl(ptr ptr)
@ stdcall -arch=x86_64 RtlLookupFunctionEntry(long ptr ptr)
@ stdcall RtlMakeSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlMapGenericMask(long ptr)
;@ stdcall RtlMapSecurityErrorToNtStatus
@ stdcall RtlMergeRangeLists(ptr ptr ptr long)
@ stdcall RtlMoveMemory(ptr ptr long)
;@ stdcall RtlMultiAppendUnicodeStringBuffer
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
;@ stdcall RtlNewInstanceSecurityObject
;@ stdcall RtlNewSecurityGrantedAccess
@ stdcall RtlNewSecurityObject(ptr ptr ptr long ptr ptr)
;@ stdcall RtlNewSecurityObjectEx
;@ stdcall RtlNewSecurityObjectWithMultipleInheritance
@ stdcall RtlNormalizeProcessParams(ptr)
@ stdcall RtlNtPathNameToDosPathName(ptr ptr ptr ptr) ; CHECKME
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlNumberGenericTableElements(ptr)
@ stdcall RtlNumberGenericTableElementsAvl(ptr)
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
;@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stdcall RtlPcToFileHeader(ptr ptr)
@ stdcall RtlPinAtomInAtomTable(ptr long)
;@ stdcall RtlPopFrame
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
;@ stdcall RtlPropertySetNameToGuid ; 4.0 only
@ stdcall RtlProtectHeap(ptr long)
;@ stdcall RtlPushFrame
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryDepthSList(ptr)
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryHeapInformation(long long ptr long ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryInformationActivationContext(long long ptr long ptr long ptr)
;@ stdcall RtlQueryInformationActiveActivationContext
;@ stdcall RtlQueryInterfaceMemoryStream
;@ stdcall RtlQueryProcessBackTraceInformation
@ stdcall RtlQueryProcessDebugInformation(long long ptr)
;@ stdcall RtlQueryProcessHeapInformation
;@ stdcall RtlQueryProcessLockInformation
;@ stdcall RtlQueryProperties ; 4.0 only
;@ stdcall RtlQueryPropertyNames ; 4.0 only
;@ stdcall RtlQueryPropertySet ; 4.0 only
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stdcall RtlQuerySecurityObject(ptr long ptr long ptr)
@ stdcall RtlQueryTagHeap(ptr long long long ptr)
@ stdcall RtlQueryTimeZoneInformation(ptr)
;@ stdcall RtlQueueApcWow64Thread
@ stdcall RtlQueueWorkItem(ptr ptr long)
@ stdcall -register RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stdcall RtlRandomEx(ptr)
@ stdcall RtlReAllocateHeap(long long ptr long)
;@ stdcall RtlReadMemoryStream
;@ stdcall RtlReadOutOfProcessMemoryStream
@ stdcall RtlRealPredecessor(ptr)
@ stdcall RtlRealSuccessor(ptr)
@ stdcall RtlRegisterSecureMemoryCacheCallback(ptr)
@ stdcall RtlRegisterWait(ptr ptr ptr ptr long long)
@ stdcall RtlReleaseActivationContext(ptr)
;@ stdcall RtlReleaseMemoryStream
@ stdcall RtlReleasePebLock()
@ stdcall RtlReleasePrivilege(ptr)
@ stdcall RtlReleaseRelativeName(ptr)
@ stdcall RtlReleaseResource(ptr)
@ stdcall RtlReleaseSRWLockExclusive(ptr)
@ stdcall RtlReleaseSRWLockShared(ptr)
@ stdcall RtlRemoteCall(ptr ptr ptr long ptr long long)
@ stdcall RtlRemoveVectoredContinueHandler(ptr)
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stdcall RtlResetRtlTranslations(ptr)
@ stdcall -arch=x86_64 RtlRestoreContext(ptr ptr)
@ stdcall RtlRestoreLastWin32Error(long) RtlSetLastWin32Error
;@ stdcall RtlRevertMemoryStream
@ stdcall RtlRunDecodeUnicodeString(long ptr)
@ stdcall RtlRunEncodeUnicodeString(long ptr)
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
;@ stdcall RtlSeekMemoryStream
@ stdcall RtlSelfRelativeToAbsoluteSD2(ptr ptr)
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlSetAllBits(ptr)
@ stdcall RtlSetAttributesSecurityDescriptor(ptr long ptr)
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetControlSecurityDescriptor(ptr long long)
@ stdcall RtlSetCriticalSectionSpinCount(ptr long)
@ stdcall RtlSetCurrentDirectory_U(ptr)
@ stdcall RtlSetCurrentEnvironment(wstr ptr)
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
;@ stdcall RtlSetEnvironmentStrings
@ stdcall RtlSetEnvironmentVariable(ptr ptr ptr)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetHeapInformation(ptr long ptr ptr)
@ stdcall RtlSetInformationAcl(ptr ptr long long)
@ stdcall RtlSetIoCompletionCallback(long ptr long)
@ stdcall RtlSetLastWin32Error(long)
@ stdcall RtlSetLastWin32ErrorAndNtStatusFromNtStatus(long)
;@ stdcall RtlSetMemoryStreamSize
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetProcessIsCritical(long ptr long)
;@ stdcall RtlSetProperties ; RtlSetProperties
;@ stdcall RtlSetPropertyClassId ; 4.0 only
;@ stdcall RtlSetPropertyNames ; 4.0 only
;@ stdcall RtlSetPropertySetClassId ; 4.0 only
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetSecurityDescriptorRMControl(ptr ptr)
@ stdcall RtlSetSecurityObject(long ptr ptr ptr ptr)
;@ stdcall RtlSetSecurityObjectEx
@ stdcall RtlSetThreadErrorMode(long ptr)
@ stdcall RtlSetThreadIsCritical(long ptr long)
@ stdcall RtlSetThreadPoolStartFunc(ptr ptr)
@ stdcall RtlSetTimeZoneInformation(ptr)
;@ stdcall RtlSetTimer
@ stdcall RtlSetUnhandledExceptionFilter(ptr)
;@ stdcall RtlSetUnicodeCallouts
@ stdcall RtlSetUserFlagsHeap(ptr long ptr long long)
@ stdcall RtlSetUserValueHeap(ptr long ptr ptr)
@ stdcall RtlSizeHeap(long long ptr)
@ stdcall RtlSleepConditionVariableCS(ptr ptr ptr)
@ stdcall RtlSleepConditionVariableSRW(ptr ptr ptr long)
@ stdcall RtlSplay(ptr)
;@ stdcall RtlStartRXact
;@ stdcall RtlStatMemoryStream
@ stdcall RtlStringFromGUID(ptr ptr)
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stdcall RtlSubtreePredecessor(ptr)
@ stdcall RtlSubtreeSuccessor(ptr)
@ stdcall RtlSystemTimeToLocalTime(ptr ptr)
@ stdcall RtlTimeFieldsToTime(ptr ptr)
@ stdcall RtlTimeToElapsedTimeFields(long long)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields (long long)
;@ stdcall RtlTraceDatabaseAdd
;@ stdcall RtlTraceDatabaseCreate
;@ stdcall RtlTraceDatabaseDestroy
;@ stdcall RtlTraceDatabaseEnumerate
;@ stdcall RtlTraceDatabaseFind
;@ stdcall RtlTraceDatabaseLock
;@ stdcall RtlTraceDatabaseUnlock
;@ stdcall RtlTraceDatabaseValidate
@ stdcall RtlTryEnterCriticalSection(ptr)
@ fastcall -arch=i386 RtlUlongByteSwap(long)
@ fastcall -ret64 RtlUlonglongByteSwap(double)
;@ stdcall RtlUnhandledExceptionFilter2
@ stdcall RtlUnhandledExceptionFilter(ptr)
;@ stdcall RtlUnicodeStringToAnsiSize(ptr)
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUnicodeStringToInteger(ptr long ptr)
;@ stdcall RtlUnicodeStringToOemSize(ptr)
@ stdcall RtlUnicodeStringToOemString(ptr ptr long)
@ stdcall RtlUnicodeToCustomCPN(ptr ptr long ptr wstr long)
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long)
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUniform(ptr)
@ stdcall RtlUnlockBootStatusData(ptr)
@ stdcall RtlUnlockHeap(long)
;@ stdcall RtlUnlockMemoryStreamRegion
@ stdcall -register RtlUnwind(ptr ptr ptr ptr)
@ stdcall -arch=x86_64 RtlUnwindEx(long long ptr long ptr)
@ stdcall RtlUpcaseUnicodeChar(long)
@ stdcall RtlUpcaseUnicodeString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeToCustomCPN(ptr ptr long ptr wstr long)
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUpdateTimer(ptr ptr long long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stdcall RtlUsageHeap(ptr long ptr)
@ fastcall -arch=i386 RtlUshortByteSwap(long)
@ stdcall RtlValidAcl(ptr)
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stdcall RtlValidateProcessHeaps()
@ stdcall RtlValidateUnicodeString(long ptr)
@ stdcall RtlVerifyVersionInfo(ptr long double)
@ stdcall -arch=x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stdcall RtlWalkFrameChain(ptr long long)
@ stdcall RtlWalkHeap(long ptr)
@ stdcall RtlWow64EnableFsRedirection(long)
@ stdcall RtlWow64EnableFsRedirectionEx(long ptr)
@ stdcall RtlWakeAllConditionVariable(ptr)
@ stdcall RtlWakeConditionVariable(ptr)
;@ stdcall RtlWriteMemoryStream
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stdcall RtlZeroHeap(ptr long)
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlZombifyActivationContext(ptr)
;@ stdcall RtlpApplyLengthFunction
@ stdcall RtlpEnsureBufferSize(ptr ptr ptr) ; CHECKME
;@ stdcall RtlpNotOwnerCriticalSection
@ stdcall RtlpNtCreateKey(ptr long ptr long ptr ptr)
@ stdcall RtlpNtEnumerateSubKey(ptr ptr long long)
@ stdcall RtlpNtMakeTemporaryKey(ptr)
@ stdcall RtlpNtOpenKey(ptr long ptr long)
@ stdcall RtlpNtQueryValueKey(ptr ptr ptr ptr long)
@ stdcall RtlpNtSetValueKey(ptr long ptr long)
@ stdcall RtlpUnWaitCriticalSection(ptr)
@ stdcall RtlpWaitForCriticalSection(ptr)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr)
@ stdcall RtlxOemStringToUnicodeSize(ptr)
@ stdcall RtlxUnicodeStringToAnsiSize(ptr)
@ stdcall RtlxUnicodeStringToOemSize(ptr) ; RtlUnicodeStringToOemSize
@ stdcall -ret64 VerSetConditionMask(double long long)
@ stdcall ZwAcceptConnectPort(ptr long ptr long long ptr) NtAcceptConnectPort
@ stdcall ZwAccessCheck(ptr long long ptr ptr ptr ptr ptr) NtAccessCheck
@ stdcall ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) NtAccessCheckAndAuditAlarm
@ stdcall ZwAccessCheckByType(ptr ptr ptr long ptr long ptr ptr long ptr ptr) NtAccessCheckByType
@ stdcall ZwAccessCheckByTypeAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr) NtAccessCheckByTypeAndAuditAlarm
@ stdcall ZwAccessCheckByTypeResultList(ptr ptr ptr long ptr long ptr ptr long ptr ptr) NtAccessCheckByTypeResultList
@ stdcall ZwAccessCheckByTypeResultListAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr) NtAccessCheckByTypeResultListAndAuditAlarm
@ stdcall ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(ptr ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr) NtAccessCheckByTypeResultListAndAuditAlarmByHandle
@ stdcall ZwAddAtom(ptr long ptr) NtAddAtom
@ stdcall ZwAddBootEntry(ptr long)
@ stdcall ZwAdjustGroupsToken(long long long long long long) NtAdjustGroupsToken
@ stdcall ZwAdjustPrivilegesToken(long long long long long long) NtAdjustPrivilegesToken
@ stdcall ZwAlertResumeThread(long ptr) NtAlertResumeThread
@ stdcall ZwAlertThread(long) NtAlertThread
@ stdcall ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
@ stdcall ZwAllocateUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwAllocateUuids(ptr ptr ptr ptr) NtAllocateUuids
@ stdcall ZwAllocateVirtualMemory(long ptr ptr ptr long long) NtAllocateVirtualMemory
@ stdcall ZwAreMappedFilesTheSame(ptr ptr) NtAreMappedFilesTheSame
@ stdcall ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stdcall ZwCallbackReturn(ptr long long)
@ stdcall ZwCancelDeviceWakeupRequest(ptr)
@ stdcall ZwCancelIoFile(long ptr) NtCancelIoFile
;@ stdcall ZwCancelIoFileEx(long ptr ptr) NtCancelIoFileEx
@ stdcall ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall ZwClearEvent(long) NtClearEvent
@ stdcall ZwClose(long) NtClose
@ stdcall ZwCloseObjectAuditAlarm(ptr ptr long)
@ stdcall ZwCompactKeys(long ptr) NtCompactKeys
@ stdcall ZwCompareTokens(ptr ptr ptr) NtCompareTokens
@ stdcall ZwCompleteConnectPort(ptr) NtCompleteConnectPort
@ stdcall ZwCompressKey(ptr) NtCompressKey
@ stdcall ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stdcall ZwContinue(ptr long) NtContinue
@ stdcall ZwCreateDebugObject(ptr long ptr long) NtCreateDebugObject
@ stdcall ZwCreateDirectoryObject(long long long) NtCreateDirectoryObject
@ stdcall ZwCreateEvent(long long long long long) NtCreateEvent
@ stdcall ZwCreateEventPair(ptr long ptr) NtCreateEventPair
@ stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
@ stdcall ZwCreateIoCompletion(ptr long ptr long) NtCreateIoCompletion
@ stdcall ZwCreateJobObject(ptr long ptr) NtCreateJobObject
@ stdcall ZwCreateJobSet(long ptr long) NtCreateJobSet
@ stdcall ZwCreateKey(ptr long ptr long ptr long long) NtCreateKey
@ stdcall ZwCreateKeyedEvent(ptr long ptr long) NtCreateKeyedEvent
@ stdcall ZwCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
@ stdcall ZwCreateMutant(ptr long ptr long) NtCreateMutant
@ stdcall ZwCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr) NtCreateNamedPipeFile
@ stdcall ZwCreatePagingFile(long long long long) NtCreatePagingFile
@ stdcall ZwCreatePort(ptr ptr long long long) NtCreatePort
@ stdcall ZwCreateProcess(ptr long ptr ptr long ptr ptr ptr)
@ stdcall ZwCreateProcessEx(ptr long ptr ptr long ptr ptr ptr long) NtCreateProcessEx
@ stdcall ZwCreateProfile(ptr ptr ptr long long ptr long long long) NtCreateProfile ; CHECKME
@ stdcall ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall ZwCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stdcall ZwCreateThread(ptr long ptr ptr ptr ptr ptr long)
@ stdcall ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stdcall ZwCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall ZwCreateWaitablePort(ptr ptr long long long) NtCreateWaitablePort
@ stdcall ZwDebugActiveProcess(ptr ptr) NtDebugActiveProcess
@ stdcall ZwDebugContinue(ptr ptr long) NtDebugContinue
@ stdcall ZwDelayExecution(long ptr) NtDelayExecution
@ stdcall ZwDeleteAtom(long) NtDeleteAtom
@ stdcall ZwDeleteBootEntry(long) NtDeleteBootEntry
@ stdcall ZwDeleteFile(ptr) NtDeleteFile
@ stdcall ZwDeleteKey(long) NtDeleteKey
@ stdcall ZwDeleteObjectAuditAlarm(ptr ptr long)
@ stdcall ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
@ stdcall ZwDisplayString(ptr) NtDisplayString
@ stdcall ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall ZwDuplicateToken(long long long long long long) NtDuplicateToken
@ stdcall ZwEnumerateBootEntries(ptr ptr)
;@ stdcall ZwEnumerateBus ; 3.51 only
@ stdcall ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
@ stdcall ZwEnumerateSystemEnvironmentValuesEx(long ptr long) NtEnumerateSystemEnvironmentValuesEx
@ stdcall ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stdcall ZwExtendSection(ptr ptr) NtExtendSection
@ stdcall ZwFilterToken(ptr long ptr ptr ptr ptr) NtFilterToken
@ stdcall ZwFindAtom(ptr long ptr) NtFindAtom
@ stdcall ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stdcall ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall ZwFlushKey(long) NtFlushKey
@ stdcall ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stdcall ZwFlushWriteBuffer()
@ stdcall ZwFreeUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall ZwFsControlFile(long long long long long long long long long long) NtFsControlFile
@ stdcall ZwGetContextThread(long ptr) NtGetContextThread
@ stdcall ZwGetCurrentProcessorNumber()
@ stdcall ZwGetDevicePowerState(ptr ptr)
@ stdcall ZwGetPlugPlayEvent(long long ptr long)
;@ stdcall ZwGetTickCount() NtGetTickCount
@ stdcall ZwGetWriteWatch(long long ptr long ptr ptr ptr) NtGetWriteWatch
@ stdcall ZwImpersonateAnonymousToken(ptr)
@ stdcall ZwImpersonateClientOfPort(ptr ptr) NtImpersonateClientOfPort
@ stdcall ZwImpersonateThread(ptr ptr ptr) NtImpersonateThread
@ stdcall ZwInitializeRegistry(long)
@ stdcall ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall ZwIsProcessInJob(long long) NtIsProcessInJob
@ stdcall ZwIsSystemResumeAutomatic()
@ stdcall ZwListenPort(ptr ptr) NtListenPort
@ stdcall ZwLoadDriver(ptr) NtLoadDriver
@ stdcall ZwLoadKey2(ptr ptr long) NtLoadKey2
@ stdcall ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
@ stdcall ZwLockProductActivationKeys(ptr ptr) NtLockProductActivationKeys
@ stdcall ZwLockRegistryKey(ptr) NtLockRegistryKey
@ stdcall ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
@ stdcall ZwMakePermanentObject(ptr) NtMakePermanentObject
@ stdcall ZwMakeTemporaryObject(long) NtMakeTemporaryObject
@ stdcall ZwMapUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwMapUserPhysicalPagesScatter(ptr ptr ptr)
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
@ stdcall ZwModifyBootEntry(ptr) NtModifyBootEntry
@ stdcall ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) NtNotifyChangeDirectoryFile
@ stdcall ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall ZwNotifyChangeMultipleKeys(ptr long ptr ptr ptr ptr ptr long long ptr long long) NtNotifyChangeMultipleKeys
@ stdcall ZwOpenDirectoryObject(long long long) NtOpenDirectoryObject
@ stdcall ZwOpenEvent(long long long) NtOpenEvent
@ stdcall ZwOpenEventPair(ptr long ptr) NtOpenEventPair
@ stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall ZwOpenIoCompletion(ptr long ptr) NtOpenIoCompletion
@ stdcall ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall ZwOpenKeyedEvent(ptr long ptr) NtOpenKeyedEvent
@ stdcall ZwOpenMutant(ptr long ptr) NtOpenMutant
@ stdcall ZwOpenObjectAuditAlarm(ptr ptr ptr ptr ptr ptr long long ptr long long ptr)
@ stdcall ZwOpenProcess(ptr long ptr ptr) NtOpenProcess
@ stdcall ZwOpenProcessToken(long long ptr) NtOpenProcessToken
@ stdcall ZwOpenProcessTokenEx(long long long ptr) NtOpenProcessTokenEx
@ stdcall ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall ZwOpenSemaphore(long long ptr) NtOpenSemaphore
@ stdcall ZwOpenSymbolicLinkObject (ptr long ptr) NtOpenSymbolicLinkObject
@ stdcall ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall ZwOpenThreadToken(long long long ptr) NtOpenThreadToken
@ stdcall ZwOpenThreadTokenEx(long long long long ptr) NtOpenThreadTokenEx
@ stdcall ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stdcall ZwPlugPlayControl(ptr ptr long)
@ stdcall ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall ZwPrivilegeCheck(ptr ptr ptr) NtPrivilegeCheck
@ stdcall ZwPrivilegeObjectAuditAlarm(ptr ptr ptr long ptr long)
@ stdcall ZwPrivilegedServiceAuditAlarm(ptr ptr ptr ptr long)
@ stdcall ZwProtectVirtualMemory(long ptr ptr long ptr) NtProtectVirtualMemory
@ stdcall ZwPulseEvent(long ptr) NtPulseEvent
@ stdcall ZwQueryAttributesFile(ptr ptr) NtQueryAttributesFile
@ stdcall ZwQueryBootEntryOrder(ptr ptr) NtQueryBootEntryOrder
@ stdcall ZwQueryBootOptions(ptr ptr) NtQueryBootOptions
@ stdcall ZwQueryDebugFilterState(long long) NtQueryDebugFilterState
@ stdcall ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall ZwQueryDefaultUILanguage(ptr) NtQueryDefaultUILanguage
@ stdcall ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) NtQueryDirectoryFile
@ stdcall ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stdcall ZwQueryEaFile(long ptr ptr long long ptr long ptr long) NtQueryEaFile
@ stdcall ZwQueryEvent(long long ptr long ptr) NtQueryEvent
@ stdcall ZwQueryFullAttributesFile(ptr ptr) NtQueryFullAttributesFile
@ stdcall ZwQueryInformationAtom(long long ptr long ptr) NtQueryInformationAtom
@ stdcall ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stdcall ZwQueryInformationJobObject(long long ptr long ptr) NtQueryInformationJobObject
@ stdcall ZwQueryInformationPort(ptr long ptr long ptr) NtQueryInformationPort
@ stdcall ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
@ stdcall ZwQueryIntervalProfile(long ptr) NtQueryIntervalProfile
@ stdcall ZwQueryIoCompletion(long long ptr long ptr) NtQueryIoCompletion
@ stdcall ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall ZwQueryMultipleValueKey(long ptr long ptr long ptr) NtQueryMultipleValueKey
@ stdcall ZwQueryMutant(long long ptr long ptr) NtQueryMutant
@ stdcall ZwQueryObject(long long long long long) NtQueryObject
@ stdcall ZwQueryOpenSubKeys(ptr ptr) NtQueryOpenSubKeys
@ stdcall ZwQueryPerformanceCounter (long long) NtQueryPerformanceCounter
@ stdcall ZwQueryPortInformationProcess() NtQueryPortInformationProcess
@ stdcall ZwQueryQuotaInformationFile(ptr ptr ptr long long ptr long ptr long) NtQueryQuotaInformationFile
@ stdcall ZwQuerySection (long long long long long) NtQuerySection
@ stdcall ZwQuerySecurityObject (long long long long long) NtQuerySecurityObject
@ stdcall ZwQuerySemaphore (long long long long long) NtQuerySemaphore
@ stdcall ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stdcall ZwQuerySystemEnvironmentValue(ptr ptr long ptr) NtQuerySystemEnvironmentValue
@ stdcall ZwQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr) NtQuerySystemEnvironmentValueEx
@ stdcall ZwQuerySystemInformation(long long long long) NtQuerySystemInformation
@ stdcall ZwQuerySystemTime(ptr) NtQuerySystemTime
@ stdcall ZwQueryTimer(ptr long ptr long ptr) NtQueryTimer
@ stdcall ZwQueryTimerResolution(long long long) NtQueryTimerResolution
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall ZwQueryVirtualMemory(long ptr long ptr long ptr) NtQueryVirtualMemory
@ stdcall ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall ZwQueueApcThread(long ptr long long long) NtQueueApcThread
@ stdcall ZwRaiseException(ptr ptr long) NtRaiseException
@ stdcall ZwRaiseHardError(long long long ptr long ptr) NtRaiseHardError
@ stdcall ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall ZwReadFileScatter(long long ptr ptr ptr ptr long ptr ptr) NtReadFileScatter
@ stdcall ZwReadRequestData(ptr ptr long ptr long ptr) NtReadRequestData
@ stdcall ZwReadVirtualMemory(long ptr ptr long ptr) NtReadVirtualMemory
;@ stdcall ZwRegisterNewDevice ; 3.51 only
@ stdcall ZwRegisterThreadTerminatePort(ptr) NtRegisterThreadTerminatePort
@ stdcall ZwReleaseKeyedEvent(ptr ptr long ptr) NtReleaseKeyedEvent
@ stdcall ZwReleaseMutant(long ptr) NtReleaseMutant
;@ stdcall ZwReleaseProcessMutant ; 3.51 only
@ stdcall ZwReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stdcall ZwRemoveIoCompletion(ptr ptr ptr ptr ptr) NtRemoveIoCompletion
@ stdcall ZwRemoveProcessDebug(ptr ptr) NtRemoveProcessDebug
@ stdcall ZwRenameKey(ptr ptr) NtRenameKey
@ stdcall ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stdcall ZwReplyPort(ptr ptr) NtReplyPort
@ stdcall ZwReplyWaitReceivePort(ptr ptr ptr ptr) NtReplyWaitReceivePort
@ stdcall ZwReplyWaitReceivePortEx(ptr ptr ptr ptr ptr)
@ stdcall ZwReplyWaitReplyPort(ptr ptr)
@ stdcall ZwRequestDeviceWakeup(ptr)
@ stdcall ZwRequestPort(ptr ptr)
@ stdcall ZwRequestWaitReplyPort(ptr ptr ptr)
@ stdcall ZwRequestWakeupLatency(long)
@ stdcall ZwResetEvent(long ptr)
@ stdcall ZwResetWriteWatch(long ptr long)
@ stdcall ZwRestoreKey(long long long)
@ stdcall ZwResumeProcess(ptr)
@ stdcall ZwResumeThread(long long)
@ stdcall ZwSaveKey(long long)
@ stdcall ZwSaveKeyEx(ptr ptr long)
@ stdcall ZwSaveMergedKeys(ptr ptr ptr)
@ stdcall ZwSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall ZwSetBootEntryOrder(ptr ptr)
@ stdcall ZwSetBootOptions(ptr long)
@ stdcall ZwSetContextThread(long ptr)
@ stdcall ZwSetDebugFilterState(long long long)
@ stdcall ZwSetDefaultHardErrorPort(ptr)
@ stdcall ZwSetDefaultLocale(long long)
@ stdcall ZwSetDefaultUILanguage(long)
@ stdcall ZwSetEaFile(long ptr ptr long)
@ stdcall ZwSetEvent(long long)
@ stdcall ZwSetEventBoostPriority(ptr)
@ stdcall ZwSetHighEventPair(ptr)
@ stdcall ZwSetHighWaitLowEventPair(ptr)
;@ stdcall ZwSetHighWaitLowThread ; 3.51 and 4.0 only
@ stdcall ZwSetInformationDebugObject(ptr long ptr long ptr)
@ stdcall ZwSetInformationFile(long long long long long)
@ stdcall ZwSetInformationJobObject(long long ptr long)
@ stdcall ZwSetInformationKey(long long ptr long)
@ stdcall ZwSetInformationObject(long long ptr long)
@ stdcall ZwSetInformationProcess(long long long long)
@ stdcall ZwSetInformationThread(long long ptr long)
@ stdcall ZwSetInformationToken(long long ptr long)
@ stdcall ZwSetIntervalProfile(long long)
@ stdcall ZwSetIoCompletion(ptr long ptr long long)
@ stdcall ZwSetLdtEntries(long double long double) ; CHECKME
@ stdcall ZwSetLowEventPair(ptr)
@ stdcall ZwSetLowWaitHighEventPair(ptr)
;@ stdcall ZwSetLowWaitHighThread ; 3.51 and 4.0 only
@ stdcall ZwSetQuotaInformationFile(ptr ptr ptr long)
@ stdcall ZwSetSecurityObject(long long ptr)
@ stdcall ZwSetSystemEnvironmentValue(ptr ptr)
@ stdcall ZwSetSystemEnvironmentValueEx(ptr ptr)
@ stdcall ZwSetSystemInformation(long ptr long)
@ stdcall ZwSetSystemPowerState(long long long)
@ stdcall ZwSetSystemTime(ptr ptr)
@ stdcall ZwSetThreadExecutionState(long ptr)
@ stdcall ZwSetTimer(long ptr ptr ptr long long ptr)
@ stdcall ZwSetTimerResolution(long long ptr)
@ stdcall ZwSetUuidSeed(ptr)
@ stdcall ZwSetValueKey(long long long long long long)
@ stdcall ZwSetVolumeInformationFile(long ptr ptr long long)
@ stdcall ZwShutdownSystem(long)
@ stdcall ZwSignalAndWaitForSingleObject(long long long ptr)
@ stdcall ZwStartProfile(ptr)
@ stdcall ZwStopProfile(ptr)
@ stdcall ZwSuspendProcess(ptr)
@ stdcall ZwSuspendThread(long ptr)
@ stdcall ZwSystemDebugControl(long ptr long ptr long ptr)
@ stdcall ZwTerminateJobObject(long long)
@ stdcall ZwTerminateProcess(long long)
@ stdcall ZwTerminateThread(long long)
@ stdcall ZwTestAlert()
@ stdcall ZwTraceEvent(long long long ptr)
@ stdcall ZwTranslateFilePath(ptr long ptr long)
@ stdcall ZwUnloadDriver(ptr)
@ stdcall ZwUnloadKey(long)
@ stdcall ZwUnloadKeyEx(ptr ptr)
@ stdcall ZwUnlockFile(long ptr ptr ptr ptr)
@ stdcall ZwUnlockVirtualMemory(long ptr ptr long)
@ stdcall ZwUnmapViewOfSection(long ptr)
@ stdcall ZwVdmControl(long ptr)
;@ stdcall ZwW32Call(long ptr long ptr ptr)
@ stdcall ZwWaitForDebugEvent(ptr long ptr ptr)
@ stdcall ZwWaitForKeyedEvent(ptr ptr long ptr)
@ stdcall ZwWaitForMultipleObjects(long ptr long long ptr)
;@ stdcall ZwWaitForProcessMutant ; 3.51 only
@ stdcall ZwWaitForSingleObject(long long long)
@ stdcall ZwWaitHighEventPair(ptr)
@ stdcall ZwWaitLowEventPair(ptr)
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall ZwWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall ZwWriteRequestData(ptr ptr long ptr long ptr)
@ stdcall ZwWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall ZwYieldExecution()
;@ cdecl _CIcos
;@ cdecl _CIlog
;@ cdecl -private -arch=i386 _CIpow()
;@ cdecl _CIsin
;@ cdecl _CIsqrt
@ cdecl -arch=x86_64 __C_specific_handler(ptr long ptr ptr)
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ cdecl __toascii(long)
@ cdecl -arch=i386 -ret64 _alldiv(double double)
@ cdecl -arch=i386 _alldvrm()
@ cdecl -arch=i386 -ret64 _allmul(double double)
@ cdecl -arch=i386 -norelay _alloca_probe()
@ cdecl -arch=i386 -ret64 _allrem(double double)
@ cdecl -arch=i386 _allshl()
@ cdecl -arch=i386 _allshr()
@ cdecl -ret64 _atoi64(str)
@ cdecl -arch=i386 -ret64 _aulldiv(double double)
@ cdecl -arch=i386 _aulldvrm()
@ cdecl -arch=i386 -ret64 _aullrem(double double)
@ cdecl -arch=i386 _aullshr()
@ extern -arch=i386 _chkstk
@ cdecl -arch=i386,x86_64 _fltused()
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl _i64toa(double ptr long)
@ cdecl _i64tow(double ptr long)
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long)
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl -arch=x86_64 _local_unwind()
@ cdecl _ltoa(long ptr long)
@ cdecl _ltow(long ptr long)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl -arch=x86_64 _setjmp(ptr ptr)
@ cdecl -arch=x86_64 _setjmpex(ptr ptr)
@ varargs _snprintf(ptr long str)
@ varargs _snwprintf(ptr long wstr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _stricmp(str str)
@ cdecl _strlwr(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strupr(str)
@ cdecl _tolower(long)
@ cdecl _toupper(long)
@ cdecl _ui64toa(double ptr long)
@ cdecl _ui64tow(double ptr long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
;@ cdecl _vscwprintf
@ cdecl _vsnprintf(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsupr(wstr)
@ cdecl _wtoi(wstr)
@ cdecl _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl abs(long)
@ cdecl -arch=i386,x86_64 atan(double)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl -arch=i386,x86_64 ceil(double)
@ cdecl -arch=i386,x86_64 cos(double)
@ cdecl -arch=i386,x86_64 fabs(double)
@ cdecl -arch=i386,x86_64 floor(double)
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalpha(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswlower(long)
@ cdecl iswspace(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl -arch=i386,x86_64 log(double)
@ cdecl -arch=x86_64 longjmp(ptr)
@ cdecl mbstowcs(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long) memmove
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl -arch=i386,x86_64 pow(double double)
@ cdecl qsort(ptr long long ptr)
@ cdecl -arch=i386,x86_64 sin(double)
@ varargs sprintf(ptr str)
@ cdecl -arch=i386,x86_64 sqrt(double)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ varargs swprintf(ptr wstr)
@ cdecl -arch=i386,x86_64 tan(double)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ stdcall vDbgPrintEx(long long str ptr)
@ stdcall vDbgPrintExWithPrefix(str long long str ptr)
@ cdecl vsprintf(ptr str ptr)
@ cdecl wcscat(wstr wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
;@ cdecl wcstok(wstr wstr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)
