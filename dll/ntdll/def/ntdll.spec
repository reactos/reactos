@ stdcall -version=0x502 PropertyLengthAsVariant(ptr long long long)
@ stdcall -version=0x502 RtlConvertPropertyToVariant(ptr long ptr ptr)
@ stdcall -version=0x502 RtlConvertVariantToProperty(ptr long ptr ptr ptr long ptr)
@ fastcall -arch=i386 RtlActivateActivationContextUnsafeFast(ptr ptr)
@ fastcall -arch=i386 RtlDeactivateActivationContextUnsafeFast(ptr)
@ fastcall -arch=i386 RtlInterlockedPushListSList(ptr ptr ptr long)
@ fastcall -arch=i386 RtlUlongByteSwap(long)
@ fastcall -arch=i386 -ret64 RtlUlonglongByteSwap(double)
@ fastcall -arch=i386 RtlUshortByteSwap(long)
@ stdcall -arch=i386 ExpInterlockedPopEntrySListEnd()
@ stdcall -arch=i386 ExpInterlockedPopEntrySListFault()
@ stdcall -arch=i386 ExpInterlockedPopEntrySListResume()
@ stdcall -stub -version=0x600+ A_SHAFinal(ptr ptr)
@ stdcall -stub -version=0x600+ A_SHAInit(ptr)
@ stdcall -stub -version=0x600+ A_SHAUpdate(ptr ptr long)
@ stdcall -stub -version=0x600+ AlpcAdjustCompletionListConcurrencyCount(ptr long)
@ stdcall -stub -version=0x600+ AlpcFreeCompletionListMessage(ptr ptr)
@ stdcall -stub -version=0x600+ AlpcGetCompletionListLastMessageInformation(ptr ptr ptr)
@ stdcall -stub -version=0x600+ AlpcGetCompletionListMessageAttributes(ptr ptr)
@ stdcall -stub -version=0x600+ AlpcGetHeaderSize(long)
@ stdcall -stub -version=0x600+ AlpcGetMessageAttribute(ptr long)
@ stdcall -stub -version=0x600+ AlpcGetMessageFromCompletionList(ptr ptr)
@ stdcall -stub -version=0x600+ AlpcGetOutstandingCompletionListMessageCount(ptr)
@ stdcall -stub -version=0x600+ AlpcInitializeMessageAttribute(long ptr long ptr)
@ stdcall -stub -version=0x600+ AlpcMaxAllowedMessageLength()
@ stdcall -stub -version=0x600+ AlpcRegisterCompletionList(ptr ptr long long long)
@ stdcall -stub -version=0x600+ AlpcRegisterCompletionListWorkerThread(ptr)
@ stdcall -stub -version=0x600+ AlpcUnregisterCompletionList(ptr)
@ stdcall -stub -version=0x600+ AlpcUnregisterCompletionListWorkerThread(ptr)
@ stdcall CsrAllocateCaptureBuffer(long long)
@ stdcall CsrAllocateMessagePointer(ptr long ptr)
@ stdcall CsrCaptureMessageBuffer(ptr ptr long ptr)
@ stdcall CsrCaptureMessageMultiUnicodeStringsInPlace(ptr long ptr)
@ stdcall CsrCaptureMessageString(ptr str long long ptr)
@ stdcall CsrCaptureTimeout(long ptr)
@ stdcall CsrClientCallServer(ptr ptr long long)
@ stdcall CsrClientConnectToServer(str long ptr ptr ptr)
@ stdcall CsrFreeCaptureBuffer(ptr)
@ stdcall CsrGetProcessId()
@ stdcall CsrIdentifyAlertableThread()
@ stdcall -version=0x502 CsrNewThread()
@ stdcall -version=0x502 CsrProbeForRead(ptr long long)
@ stdcall -version=0x502 CsrProbeForWrite(ptr long long)
@ stdcall CsrSetPriorityClass(ptr ptr)
@ stdcall -stub -version=0x600+ CsrVerifyRegion(ptr long)
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
@ stdcall -version=0x502 EtwControlTraceA(double str ptr long)
@ stdcall -version=0x502 EtwControlTraceW(double wstr ptr long)
@ stdcall -stub EtwCreateTraceInstanceId(ptr ptr)
@ stub -version=0x600+ EtwDeliverDataBlock
@ stdcall -version=0x502 EtwEnableTrace(long long long ptr double)
@ stub -version=0x600+ EtwEnumerateProcessRegGuids
@ stdcall -stub -version=0x502 EtwEnumerateTraceGuids(ptr long ptr)
@ stub -version=0x600+ EtwEventActivityIdControl
@ stub -version=0x600+ EtwEventEnabled
@ stub -version=0x600+ EtwEventProviderEnabled
@ stub -version=0x600+ EtwEventRegister
@ stub -version=0x600+ EtwEventUnregister
@ stub -version=0x600+ EtwEventWrite
@ stub -version=0x600+ EtwEventWriteEndScenario
@ stub -version=0x600+ EtwEventWriteFull
@ stub -version=0x600+ EtwEventWriteStartScenario
@ stub -version=0x600+ EtwEventWriteString
@ stub -version=0x600+ EtwEventWriteTransfer
@ stdcall -version=0x502 EtwFlushTraceA(double str ptr)
@ stdcall -version=0x502 EtwFlushTraceW(double wstr ptr)
@ stdcall EtwGetTraceEnableFlags(double)
@ stdcall EtwGetTraceEnableLevel(double)
@ stdcall EtwGetTraceLoggerHandle(ptr)
@ stub -version=0x600+ EtwLogTraceEvent
@ stub -version=0x600+ EtwNotificationRegister
@ stdcall -stub -version=0x502 EtwNotificationRegistrationA(ptr long ptr long long)
@ stdcall -stub -version=0x502 EtwNotificationRegistrationW(ptr long ptr long long)
@ stub -version=0x600+ EtwNotificationUnregister
@ stub -version=0x600+ EtwProcessPrivateLoggerRequest
@ stdcall -version=0x502 EtwQueryAllTracesA(ptr long ptr)
@ stdcall -version=0x502 EtwQueryAllTracesW(ptr long ptr)
@ stdcall -version=0x502 EtwQueryTraceA(double str ptr)
@ stdcall -version=0x502 EtwQueryTraceW(double wstr ptr)
@ stdcall -stub -version=0x502 EtwReceiveNotificationsA(long long long long)
@ stdcall -stub -version=0x502 EtwReceiveNotificationsW(long long long long)
@ stub -version=0x600+ EtwRegisterSecurityProvider
@ stdcall EtwRegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr)
@ stdcall EtwRegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr)
@ stub -version=0x600+ EtwReplyNotification
@ stub -version=0x600+ EtwSendNotification
@ stub -version=0x600+ EtwSetMark
@ stdcall -version=0x502 EtwStartTraceA(ptr str ptr)
@ stdcall -version=0x502 EtwStartTraceW(ptr wstr ptr)
@ stdcall -version=0x502 EtwStopTraceA(double str ptr)
@ stdcall -version=0x502 EtwStopTraceW(double wstr ptr)
@ stdcall -version=0x502 EtwTraceEvent(double ptr)
@ stdcall -stub EtwTraceEventInstance(double ptr ptr ptr)
@ varargs EtwTraceMessage(int64 long ptr long)
@ stdcall -stub EtwTraceMessageVa(int64 long ptr long ptr)
@ stdcall EtwUnregisterTraceGuids(double)
@ stdcall -version=0x502 EtwUpdateTraceA(double str ptr)
@ stdcall -version=0x502 EtwUpdateTraceW(double wstr ptr)
@ stub -version=0x600+ EtwWriteUMSecurityEvent
@ stub -version=0x600+ EtwpCreateEtwThread
@ stub -version=0x600+ EtwpGetCpuSpeed
@ stdcall -stub -version=0x502 EtwpGetTraceBuffer(long long long long)
@ stub -version=0x600+ EtwpNotificationThread
@ stdcall -stub -version=0x502 EtwpSetHWConfigFunction(ptr long)
@ stdcall -arch=x86_64 ExpInterlockedPopEntrySListEnd()
@ stub -version=0x600+ -arch=x86_64 ExpInterlockedPopEntrySListEnd8
@ stdcall -arch=x86_64 ExpInterlockedPopEntrySListFault()
@ stub -version=0x600+ -arch=x86_64 ExpInterlockedPopEntrySListFault8
@ stdcall -arch=x86_64 ExpInterlockedPopEntrySListResume()
@ stub -version=0x600+ -arch=x86_64 ExpInterlockedPopEntrySListResume8
@ stdcall -arch=i386 KiFastSystemCall()
@ stdcall -arch=i386 KiFastSystemCallRet()
@ stdcall -arch=i386 KiIntSystemCall()
@ stdcall KiRaiseUserExceptionDispatcher()
@ stdcall KiUserApcDispatcher(ptr ptr ptr ptr)
@ stdcall KiUserCallbackDispatcher(ptr ptr long) ; CHECKME
@ stdcall KiUserExceptionDispatcher(ptr ptr)
@ stdcall -version=0x502 LdrAccessOutOfProcessResource(ptr ptr ptr ptr ptr)
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stdcall -stub -version=0x600+ LdrAddLoadAsDataTable(ptr wstr long ptr)
@ stdcall LdrAddRefDll(long ptr)
@ stdcall -version=0x502 LdrAlternateResourcesEnabled()
@ stdcall -version=0x502 LdrCreateOutOfProcessImage(long ptr ptr ptr)
@ stdcall -version=0x502 LdrDestroyOutOfProcessImage(ptr)
@ stdcall LdrDisableThreadCalloutsForDll(long)
@ stdcall LdrEnumResources(ptr ptr long ptr ptr)
@ stdcall LdrEnumerateLoadedModules(long ptr long)
@ stdcall -version=0x501-0x502 LdrFindCreateProcessManifest(long ptr ptr long ptr)
@ stdcall LdrFindEntryForAddress(ptr ptr)
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
@ stdcall -stub LdrFindResourceEx_U(ptr ptr ptr ptr ptr) ; 5.1 and higher
@ stdcall LdrFindResource_U(long ptr long ptr)
@ stdcall LdrFlushAlternateResourceModules()
@ stdcall LdrGetDllHandle(wstr long ptr ptr)
@ stdcall LdrGetDllHandleEx(long wstr long ptr ptr)
@ stub -version=0x600+ LdrGetFailureData
@ stdcall -stub -version=0x600+ LdrGetFileNameFromLoadAsDataTable(ptr ptr)
@ stdcall -stub -version=0x600+ -arch=x86_64 LdrGetKnownDllSectionHandle(wstr long ptr)
@ stdcall LdrGetProcedureAddress(ptr ptr long ptr)
@ stdcall -stub -version=0x600+ LdrGetProcedureAddressEx(ptr ptr long ptr long)
@ stdcall -stub LdrHotPatchRoutine(ptr)
@ stdcall -stub LdrInitShimEngineDynamic(ptr)
@ stdcall LdrInitializeThunk(long long long long)
@ stdcall LdrLoadAlternateResourceModule(ptr ptr)
@ stub -version=0x600+ LdrLoadAlternateResourceModuleEx
@ stdcall LdrLoadDll(wstr long ptr ptr)
@ stdcall LdrLockLoaderLock(long ptr ptr)
@ stdcall LdrOpenImageFileOptionsKey(ptr long ptr) ; 5.2 SP1 and higher
@ stub -version=0x600+ -arch=x86_64 LdrProcessInitializationComplete
@ stdcall LdrProcessRelocationBlock(ptr long ptr long)
@ stdcall LdrQueryImageFileExecutionOptions(ptr str long ptr long ptr)
@ stdcall LdrQueryImageFileExecutionOptionsEx(ptr ptr long ptr long ptr long)
@ stdcall LdrQueryImageFileKeyOption(ptr ptr long ptr long ptr)
@ stdcall -stub -version=0x600+ LdrQueryModuleServiceTags(ptr ptr ptr)
@ stdcall LdrQueryProcessModuleInformation(ptr long ptr)
@ stdcall -stub -version=0x600+ LdrRegisterDllNotification(long ptr ptr ptr)
@ stdcall -stub -version=0x600+ LdrRemoveLoadAsDataTable(ptr ptr ptr long)
@ stub -version=0x600+ LdrResFindResource
@ stub -version=0x600+ LdrResFindResourceDirectory
@ stub -version=0x600+ LdrResRelease
@ stub -version=0x600+ LdrResSearchResource
@ stdcall LdrSetAppCompatDllRedirectionCallback(long ptr ptr)
@ stdcall LdrSetDllManifestProber(ptr)
@ stub -version=0x600+ LdrSetMUICacheType
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ stdcall LdrUnloadAlternateResourceModule(ptr)
@ stub -version=0x600+ LdrUnloadAlternateResourceModuleEx
@ stdcall LdrUnloadDll(ptr)
@ stdcall LdrUnlockLoaderLock(long long)
@ stdcall -stub -version=0x600+ LdrUnregisterDllNotification(ptr)
@ stdcall LdrVerifyImageMatchesChecksum(ptr long long long)
@ stdcall -stub -version=0x600+ LdrVerifyImageMatchesChecksumEx(ptr ptr)
@ stub -version=0x600+ LdrpResGetMappingSize
@ stub -version=0x600+ LdrpResGetRCConfig
@ stub -version=0x600+ LdrpResGetResourceDirectory
@ stdcall -stub -version=0x600+ MD4Final(ptr)
@ stdcall -stub -version=0x600+ MD4Init(ptr)
@ stdcall -stub -version=0x600+ MD4Update(ptr ptr long)
@ stdcall -stub -version=0x600+ MD5Final(ptr)
@ stdcall -stub -version=0x600+ MD5Init(ptr)
@ stdcall -stub -version=0x600+ MD5Update(ptr ptr long)
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
@ stub -version=0x600+ NtAcquireCMFViewOwnership
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
@ stub -version=0x600+ NtAlpcAcceptConnectPort
@ stub -version=0x600+ NtAlpcCancelMessage
@ stub -version=0x600+ NtAlpcConnectPort
@ stub -version=0x600+ NtAlpcCreatePort
@ stub -version=0x600+ NtAlpcCreatePortSection
@ stub -version=0x600+ NtAlpcCreateResourceReserve
@ stub -version=0x600+ NtAlpcCreateSectionView
@ stub -version=0x600+ NtAlpcCreateSecurityContext
@ stub -version=0x600+ NtAlpcDeletePortSection
@ stub -version=0x600+ NtAlpcDeleteResourceReserve
@ stub -version=0x600+ NtAlpcDeleteSectionView
@ stub -version=0x600+ NtAlpcDeleteSecurityContext
@ stub -version=0x600+ NtAlpcDisconnectPort
@ stub -version=0x600+ NtAlpcImpersonateClientOfPort
@ stub -version=0x600+ NtAlpcOpenSenderProcess
@ stub -version=0x600+ NtAlpcOpenSenderThread
@ stub -version=0x600+ NtAlpcQueryInformation
@ stub -version=0x600+ NtAlpcQueryInformationMessage
@ stub -version=0x600+ NtAlpcRevokeSecurityContext
@ stub -version=0x600+ NtAlpcSendWaitReceivePort
@ stub -version=0x600+ NtAlpcSetInformation
@ stdcall NtApphelpCacheControl(long ptr)
@ stdcall NtAreMappedFilesTheSame(ptr ptr)
@ stdcall NtAssignProcessToJobObject(long long)
@ stdcall NtCallbackReturn(ptr long long)
@ stdcall NtCancelDeviceWakeupRequest(ptr)
@ stdcall NtCancelIoFile(long ptr)
@ stub -version=0x600+ NtCancelIoFileEx
@ stub -version=0x600+ NtCancelSynchronousIoFile
@ stdcall NtCancelTimer(long ptr)
@ stdcall NtClearEvent(long)
@ stdcall NtClose(long)
@ stdcall NtCloseObjectAuditAlarm(ptr ptr long)
@ stub -version=0x600+ NtCommitComplete
@ stub -version=0x600+ NtCommitEnlistment
@ stub -version=0x600+ NtCommitTransaction
@ stdcall NtCompactKeys(long ptr)
@ stdcall NtCompareTokens(ptr ptr ptr)
@ stdcall NtCompleteConnectPort(ptr)
@ stdcall NtCompressKey(ptr)
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtContinue(ptr long)
@ stdcall NtCreateDebugObject(ptr long ptr long)
@ stdcall NtCreateDirectoryObject(long long long)
@ stub -version=0x600+ NtCreateEnlistment
@ stdcall NtCreateEvent(long long long long long)
@ stdcall NtCreateEventPair(ptr long ptr)
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr)
@ stdcall NtCreateIoCompletion(ptr long ptr long)
@ stdcall NtCreateJobObject(ptr long ptr)
@ stdcall NtCreateJobSet(long ptr long)
@ stdcall NtCreateKey(ptr long ptr long ptr long long)
@ stub -version=0x600+ NtCreateKeyTransacted
@ stdcall NtCreateKeyedEvent(ptr long ptr long)
@ stdcall NtCreateMailslotFile(long long long long long long long long)
@ stdcall NtCreateMutant(ptr long ptr long)
@ stdcall NtCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall NtCreatePagingFile(long long long long)
@ stdcall NtCreatePort(ptr ptr long long ptr)
@ stub -version=0x600+ NtCreatePrivateNamespace
@ stdcall NtCreateProcess(ptr long ptr ptr long ptr ptr ptr)
@ stdcall NtCreateProcessEx(ptr long ptr ptr long ptr ptr ptr long)
@ stdcall NtCreateProfile(ptr ptr ptr long long ptr long long long) ; CHECKME
@ stub -version=0x600+ NtCreateResourceManager
@ stdcall NtCreateSection(ptr long ptr ptr long long ptr)
@ stdcall NtCreateSemaphore(ptr long ptr long long)
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stdcall NtCreateThread(ptr long ptr ptr ptr ptr ptr long)
@ stub -version=0x600+ NtCreateThreadEx
@ stdcall NtCreateTimer(ptr long ptr long)
@ stdcall NtCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub -version=0x600+ NtCreateTransaction
@ stub -version=0x600+ NtCreateTransactionManager
@ stub -version=0x600+ NtCreateUserProcess
@ stdcall NtCreateWaitablePort(ptr ptr long long long)
@ stub -version=0x600+ NtCreateWorkerFactory
@ stdcall -arch=win32 NtCurrentTeb() _NtCurrentTeb
@ stdcall NtDebugActiveProcess(ptr ptr)
@ stdcall NtDebugContinue(ptr ptr long)
@ stdcall NtDelayExecution(long ptr)
@ stdcall NtDeleteAtom(long)
@ stdcall NtDeleteBootEntry(long)
@ stdcall NtDeleteDriverEntry(long)
@ stdcall NtDeleteFile(ptr)
@ stdcall NtDeleteKey(long)
@ stdcall NtDeleteObjectAuditAlarm(ptr ptr long)
@ stub -version=0x600+ NtDeletePrivateNamespace
@ stdcall NtDeleteValueKey(long ptr)
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long)
@ stdcall NtDisplayString(ptr)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long long long long long)
@ stdcall NtEnumerateBootEntries(ptr ptr)
@ stdcall NtEnumerateDriverEntries(ptr ptr)
@ stdcall NtEnumerateKey (long long long long long long)
@ stdcall NtEnumerateSystemEnvironmentValuesEx(long ptr long)
@ stub -version=0x600+ NtEnumerateTransactionObject
@ stdcall NtEnumerateValueKey(long long long long long long)
@ stdcall NtExtendSection(ptr ptr)
@ stdcall NtFilterToken(ptr long ptr ptr ptr ptr)
@ stdcall NtFindAtom(ptr long ptr)
@ stdcall NtFlushBuffersFile(long ptr)
@ stub -version=0x600+ NtFlushInstallUILanguage
@ stdcall NtFlushInstructionCache(long ptr long)
@ stdcall NtFlushKey(long)
@ stub -version=0x600+ NtFlushProcessWriteBuffers
@ stdcall NtFlushVirtualMemory(long ptr ptr long)
@ stdcall NtFlushWriteBuffer()
@ stdcall NtFreeUserPhysicalPages(ptr ptr ptr)
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stub -version=0x600+ NtFreezeRegistry
@ stub -version=0x600+ NtFreezeTransactions
@ stdcall NtFsControlFile(long long long long long long long long long long)
@ stdcall NtGetContextThread(long ptr)
@ stdcall NtGetCurrentProcessorNumber() ; 5.2 and higher
@ stdcall NtGetDevicePowerState(ptr ptr)
@ stub -version=0x600+ NtGetMUIRegistryInfo
@ stub -version=0x600+ NtGetNextProcess
@ stub -version=0x600+ NtGetNextThread
@ stub -version=0x600+ NtGetNlsSectionPtr
@ stub -version=0x600+ NtGetNotificationResourceManager
@ stdcall NtGetPlugPlayEvent(long long ptr long)
@ stdcall NtGetTickCount() RtlGetTickCount
@ stdcall NtGetWriteWatch(long long ptr long ptr ptr ptr)
@ stdcall NtImpersonateAnonymousToken(ptr)
@ stdcall NtImpersonateClientOfPort(ptr ptr)
@ stdcall NtImpersonateThread(ptr ptr ptr)
@ stub -version=0x600+ NtInitializeNlsFiles
@ stdcall NtInitializeRegistry(long)
@ stdcall NtInitiatePowerAction (long long long long)
@ stdcall NtIsProcessInJob(long long)
@ stdcall NtIsSystemResumeAutomatic()
@ stub -version=0x600+ NtIsUILanguageComitted
@ stdcall NtListenPort(ptr ptr)
@ stdcall NtLoadDriver(ptr)
@ stdcall NtLoadKey2(ptr ptr long)
@ stdcall NtLoadKey(ptr ptr)
@ stdcall NtLoadKeyEx(ptr ptr long ptr)
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stdcall NtLockProductActivationKeys(ptr ptr)
@ stdcall NtLockRegistryKey(ptr)
@ stdcall NtLockVirtualMemory(long ptr ptr long)
@ stdcall NtMakePermanentObject(ptr)
@ stdcall NtMakeTemporaryObject(long)
@ stub -version=0x600+ NtMapCMFModule
@ stdcall NtMapUserPhysicalPages(ptr ptr ptr)
@ stdcall NtMapUserPhysicalPagesScatter(ptr ptr ptr)
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall NtModifyBootEntry(ptr)
@ stdcall NtModifyDriverEntry(ptr)
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
@ stdcall NtNotifyChangeMultipleKeys(ptr long ptr ptr ptr ptr ptr long long ptr long long)
@ stdcall NtOpenDirectoryObject(long long long)
@ stub -version=0x600+ NtOpenEnlistment
@ stdcall NtOpenEvent(long long long)
@ stdcall NtOpenEventPair(ptr long ptr)
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenIoCompletion(ptr long ptr)
@ stdcall NtOpenJobObject(ptr long ptr)
@ stdcall NtOpenKey(ptr long ptr)
@ stub -version=0x600+ NtOpenKeyTransacted
@ stdcall NtOpenKeyedEvent(ptr long ptr)
@ stdcall NtOpenMutant(ptr long ptr)
@ stdcall NtOpenObjectAuditAlarm(ptr ptr ptr ptr ptr ptr long long ptr long long ptr)
@ stub -version=0x600+ NtOpenPrivateNamespace
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stub -version=0x600+ NtOpenResourceManager
@ stdcall NtOpenSection(ptr long ptr)
@ stdcall NtOpenSemaphore(long long ptr)
@ stub -version=0x600+ NtOpenSession
@ stdcall NtOpenSymbolicLinkObject (ptr long ptr)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long ptr)
@ stdcall NtOpenThreadTokenEx(long long long long ptr)
@ stdcall NtOpenTimer(ptr long ptr)
@ stub -version=0x600+ NtOpenTransaction
@ stub -version=0x600+ NtOpenTransactionManager
@ stdcall NtPlugPlayControl(ptr ptr long)
@ stdcall NtPowerInformation(long ptr long ptr long)
@ stub -version=0x600+ NtPrePrepareComplete
@ stub -version=0x600+ NtPrePrepareEnlistment
@ stub -version=0x600+ NtPrepareComplete
@ stub -version=0x600+ NtPrepareEnlistment
@ stdcall NtPrivilegeCheck(ptr ptr ptr)
@ stdcall NtPrivilegeObjectAuditAlarm(ptr ptr ptr long ptr long)
@ stdcall NtPrivilegedServiceAuditAlarm(ptr ptr ptr ptr long)
@ stub -version=0x600+ NtPropagationComplete
@ stub -version=0x600+ NtPropagationFailed
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
@ stdcall NtQueryDriverEntryOrder(ptr ptr)
@ stdcall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall NtQueryEvent(long long ptr long ptr)
@ stdcall NtQueryFullAttributesFile(ptr ptr)
@ stdcall NtQueryInformationAtom(long long ptr long ptr)
@ stub -version=0x600+ NtQueryInformationEnlistment
@ stdcall NtQueryInformationFile(ptr ptr ptr long long)
@ stdcall NtQueryInformationJobObject(ptr long ptr long ptr)
@ stdcall NtQueryInformationPort(ptr long ptr long ptr)
@ stdcall NtQueryInformationProcess(ptr long ptr long ptr)
@ stub -version=0x600+ NtQueryInformationResourceManager
@ stdcall NtQueryInformationThread(ptr long ptr long ptr)
@ stdcall NtQueryInformationToken(ptr long ptr long ptr)
@ stub -version=0x600+ NtQueryInformationTransaction
@ stub -version=0x600+ NtQueryInformationTransactionManager
@ stub -version=0x600+ NtQueryInformationWorkerFactory
@ stdcall NtQueryInstallUILanguage(ptr)
@ stdcall NtQueryIntervalProfile(long ptr)
@ stdcall NtQueryIoCompletion(long long ptr long ptr)
@ stdcall NtQueryKey (long long ptr long ptr)
@ stub -version=0x600+ NtQueryLicenseValue
@ stdcall NtQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall NtQueryMutant(long long ptr long ptr)
@ stdcall NtQueryObject(long long long long long)
@ stdcall NtQueryOpenSubKeys(ptr ptr)
@ stdcall NtQueryOpenSubKeysEx(ptr long ptr ptr)
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
@ stub -version=0x600+ NtReadOnlyEnlistment
@ stdcall NtReadRequestData(ptr ptr long ptr long ptr)
@ stdcall NtReadVirtualMemory(long ptr ptr long ptr)
@ stub -version=0x600+ NtRecoverEnlistment
@ stub -version=0x600+ NtRecoverResourceManager
@ stub -version=0x600+ NtRecoverTransactionManager
@ stub -version=0x600+ NtRegisterProtocolAddressInformation
@ stdcall NtRegisterThreadTerminatePort(ptr)
@ stub -version=0x600+ NtReleaseCMFViewOwnership
@ stdcall NtReleaseKeyedEvent(ptr ptr long ptr)
@ stdcall NtReleaseMutant(long ptr)
@ stdcall NtReleaseSemaphore(long long ptr)
@ stub -version=0x600+ NtReleaseWorkerFactoryWorker
@ stdcall NtRemoveIoCompletion(ptr ptr ptr ptr ptr)
@ stub -version=0x600+ NtRemoveIoCompletionEx
@ stdcall NtRemoveProcessDebug(ptr ptr)
@ stdcall NtRenameKey(ptr ptr)
@ stub -version=0x600+ NtRenameTransactionManager
@ stdcall NtReplaceKey(ptr long ptr)
@ stub -version=0x600+ NtReplacePartitionUnit
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
@ stub -version=0x600+ NtRollbackComplete
@ stub -version=0x600+ NtRollbackEnlistment
@ stub -version=0x600+ NtRollbackTransaction
@ stub -version=0x600+ NtRollforwardTransactionManager
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
@ stdcall NtSetDriverEntryOrder(ptr ptr)
@ stdcall NtSetEaFile(long ptr ptr long)
@ stdcall NtSetEvent(long long)
@ stdcall NtSetEventBoostPriority(ptr)
@ stdcall NtSetHighEventPair(ptr)
@ stdcall NtSetHighWaitLowEventPair(ptr)
@ stdcall NtSetInformationDebugObject(ptr long ptr long ptr)
@ stub -version=0x600+ NtSetInformationEnlistment
@ stdcall NtSetInformationFile(ptr ptr ptr long long)
@ stdcall NtSetInformationJobObject(ptr long ptr long)
@ stdcall NtSetInformationKey(ptr long ptr long)
@ stdcall NtSetInformationObject(ptr long ptr long)
@ stdcall NtSetInformationProcess(ptr long ptr long)
@ stub -version=0x600+ NtSetInformationResourceManager
@ stdcall NtSetInformationThread(ptr long ptr long)
@ stdcall NtSetInformationToken(ptr long ptr long)
@ stub -version=0x600+ NtSetInformationTransaction
@ stub -version=0x600+ NtSetInformationTransactionManager
@ stub -version=0x600+ NtSetInformationWorkerFactory
@ stdcall NtSetIntervalProfile(long long)
@ stdcall NtSetIoCompletion(ptr long ptr long long)
@ stdcall NtSetLdtEntries(long int64 long int64)
@ stdcall NtSetLowEventPair(ptr)
@ stdcall NtSetLowWaitHighEventPair(ptr)
@ stdcall NtSetQuotaInformationFile(ptr ptr ptr long)
@ stdcall NtSetSecurityObject(long long ptr)
@ stdcall NtSetSystemEnvironmentValue(ptr ptr)
@ stdcall NtSetSystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
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
@ stub -version=0x600+ NtShutdownWorkerFactory
@ stdcall NtSignalAndWaitForSingleObject(long long long ptr)
@ stub -version=0x600+ NtSinglePhaseReject
@ stdcall NtStartProfile(ptr)
@ stdcall NtStopProfile(ptr)
@ stdcall NtSuspendProcess(ptr)
@ stdcall NtSuspendThread(long ptr)
@ stdcall NtSystemDebugControl(long ptr long ptr long ptr)
@ stdcall NtTerminateJobObject(ptr long)
@ stdcall NtTerminateProcess(ptr long)
@ stdcall NtTerminateThread(ptr long)
@ stdcall NtTestAlert()
@ stub -version=0x600+ NtThawRegistry
@ stub -version=0x600+ NtThawTransactions
@ stub -version=0x600+ NtTraceControl
@ stdcall NtTraceEvent(long long long ptr)
@ stdcall NtTranslateFilePath(ptr long ptr long)
@ stdcall NtUnloadDriver(ptr)
@ stdcall NtUnloadKey2(ptr long)
@ stdcall NtUnloadKey(long)
@ stdcall NtUnloadKeyEx(ptr ptr)
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stdcall NtUnlockVirtualMemory(long ptr ptr long)
@ stdcall NtUnmapViewOfSection(long ptr)
@ stdcall NtVdmControl(long ptr)
@ stdcall NtWaitForDebugEvent(ptr long ptr ptr)
@ stdcall NtWaitForKeyedEvent(ptr ptr long ptr)
@ stdcall NtWaitForMultipleObjects32(long ptr long long ptr)
@ stdcall NtWaitForMultipleObjects(long ptr long long ptr)
@ stdcall NtWaitForSingleObject(long long long)
@ stub -version=0x600+ NtWaitForWorkViaWorkerFactory
@ stdcall NtWaitHighEventPair(ptr)
@ stdcall NtWaitLowEventPair(ptr)
@ stub -version=0x600+ NtWorkerFactoryWorkerReady
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtWriteRequestData(ptr ptr long ptr long ptr)
@ stdcall NtWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall NtYieldExecution()
@ stub -version=0x600+ NtdllDefWindowProc_A
@ stub -version=0x600+ NtdllDefWindowProc_W
@ stub -version=0x600+ NtdllDialogWndProc_A
@ stub -version=0x600+ NtdllDialogWndProc_W
@ stdcall PfxFindPrefix(ptr ptr)
@ stdcall PfxInitialize(ptr)
@ stdcall PfxInsertPrefix(ptr ptr ptr)
@ stdcall PfxRemovePrefix(ptr ptr)
@ stdcall RtlAbortRXact(ptr)
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAcquirePebLock()
@ stdcall RtlAcquirePrivilege(ptr long long ptr)
@ stdcall RtlAcquireResourceExclusive(ptr long)
@ stdcall RtlAcquireResourceShared(ptr long)
@ stdcall -stub -version=0x600+ RtlAcquireSRWLockExclusive(ptr)
@ stdcall -stub -version=0x600+ RtlAcquireSRWLockShared(ptr)
@ stdcall RtlActivateActivationContext(long ptr ptr)
@ stdcall RtlActivateActivationContextEx(long ptr ptr ptr)
@ stdcall -arch=x86_64,arm RtlActivateActivationContextUnsafeFast(ptr ptr)
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAccessDeniedAce(ptr long long ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
@ stdcall RtlAddActionToRXact(ptr long ptr long ptr long)
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
@ stdcall RtlAddAttributeActionToRXact(ptr long ptr ptr ptr long ptr long)
@ stdcall RtlAddAuditAccessAce(ptr long long ptr long long)
@ stdcall RtlAddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall RtlAddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
@ stdcall -stub RtlAddCompoundAce(ptr long long long ptr ptr)
@ stdcall -arch=x86_64 RtlAddFunctionTable(ptr long long)
@ stub -version=0x600+ RtlAddMandatoryAce
@ stdcall RtlAddRefActivationContext(ptr)
@ stdcall RtlAddRefMemoryStream(ptr)
@ stub -version=0x600+ RtlAddSIDToBoundaryDescriptor
@ stdcall RtlAddVectoredContinueHandler(long ptr)
@ stdcall RtlAddVectoredExceptionHandler(long ptr)
@ stdcall -stub RtlAddressInSectionTable(ptr ptr long)
@ stdcall RtlAdjustPrivilege(long long long ptr)
@ stdcall RtlAllocateActivationContextStack(ptr) ; CHECKME
@ stdcall RtlAllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall RtlAllocateHandle(ptr ptr)
@ stdcall RtlAllocateHeap(ptr long ptr)
@ stub -version=0x600+ RtlAllocateMemoryBlockLookaside
@ stub -version=0x600+ RtlAllocateMemoryZone
@ stdcall RtlAnsiCharToUnicodeChar(ptr)
@ stdcall RtlAnsiStringToUnicodeSize(ptr) RtlxAnsiStringToUnicodeSize
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
@ stdcall -stub RtlAppendPathElement(ptr ptr ptr)
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
@ stdcall RtlApplicationVerifierStop(ptr str ptr str ptr str ptr str ptr str)
@ stdcall RtlApplyRXact(ptr)
@ stdcall RtlApplyRXactNoFlush(ptr)
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
@ stdcall RtlAssert(ptr ptr long ptr)
@ stub -version=0x600+ RtlBarrier
@ stub -version=0x600+ RtlBarrierForDelete
@ stdcall RtlCancelTimer(ptr ptr)
@ stdcall -register RtlCaptureContext(ptr)
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr)
@ stdcall -stub -arch=i386 RtlCaptureStackContext(ptr ptr ptr)
@ stdcall RtlCharToInteger(ptr long ptr)
@ stdcall RtlCheckForOrphanedCriticalSections(ptr)
@ stdcall -stub -version=0x502 RtlCheckProcessParameters(ptr ptr ptr ptr)
@ stdcall RtlCheckRegistryKey(long ptr)
@ stub -version=0x600+ RtlCleanUpTEBLangLists
@ stdcall RtlClearAllBits(ptr)
@ stdcall RtlClearBits(ptr long long)
@ stdcall RtlCloneMemoryStream(ptr ptr)
@ stub -version=0x600+ RtlCloneUserProcess
@ stub -version=0x600+ RtlCmDecodeMemIoResource
@ stub -version=0x600+ RtlCmEncodeMemIoResource
@ stub -version=0x600+ RtlCommitDebugInfo
@ stdcall RtlCommitMemoryStream(ptr long)
@ stdcall RtlCompactHeap(long long)
@ stub -version=0x600+ RtlCompareAltitudes
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString (ptr ptr long)
@ stub -version=0x600+ RtlCompareUnicodeStrings
@ stub -version=0x600+ -arch=x86_64 RtlCompleteProcessCloning
@ stdcall RtlCompressBuffer(long ptr long ptr long long ptr ptr)
@ stdcall RtlComputeCrc32(long ptr long)
@ stdcall RtlComputeImportTableHash(ptr ptr long)
@ stdcall RtlComputePrivatizedDllName_U(ptr ptr ptr)
@ stub -version=0x600+ RtlConnectToSm
@ stdcall RtlConsoleMultiByteToUnicodeN(ptr long ptr ptr long ptr)
@ stdcall RtlConvertExclusiveToShared(ptr)
@ stub -version=0x600+ RtlConvertLCIDToString
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
@ stdcall RtlConvertSharedToExclusive(ptr)
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stdcall RtlConvertToAutoInheritSecurityObject(ptr ptr ptr ptr long ptr)
@ stdcall RtlConvertUiListToApiList(ptr ptr long)
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
@ stdcall RtlCopyMappedMemory(ptr ptr long)
@ cdecl -version=0x600+ -arch=x86_64 RtlCopyMemory(ptr ptr long) memmove
@ stub -version=0x600+ -arch=x86_64 RtlCopyMemoryNonTemporal
@ stdcall RtlCopyMemoryStreamTo(ptr ptr int64 ptr ptr)
@ stdcall RtlCopyOutOfProcessMemoryStreamTo(ptr ptr int64 ptr ptr) RtlCopyMemoryStreamTo
@ stdcall RtlCopySecurityDescriptor(ptr ptr)
@ stdcall RtlCopySid(long ptr ptr)
@ stdcall RtlCopySidAndAttributesArray(long ptr long ptr ptr ptr ptr)
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stdcall RtlCreateActivationContext(long ptr long ptr ptr ptr)
@ stdcall RtlCreateAndSetSD(ptr long ptr ptr ptr)
@ stdcall RtlCreateAtomTable(long ptr)
@ stdcall RtlCreateBootStatusDataFile()
@ stub -version=0x600+ RtlCreateBoundaryDescriptor
@ stdcall RtlCreateEnvironment(long ptr)
@ stub -version=0x600+ RtlCreateEnvironmentEx
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stub -version=0x600+ RtlCreateMemoryBlockLookaside
@ stub -version=0x600+ RtlCreateMemoryZone
@ stdcall RtlCreateProcessParameters(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub -version=0x600+ RtlCreateProcessParametersEx
@ stdcall RtlCreateQueryDebugBuffer(long long)
@ stdcall RtlCreateRegistryKey(long wstr)
@ stdcall RtlCreateSecurityDescriptor(ptr long)
@ stdcall RtlCreateServiceSid(ptr ptr ptr) # Exists in Windows 2003 SP 2
@ stdcall RtlCreateSystemVolumeInformationFolder(ptr)
@ stdcall RtlCreateTagHeap(ptr long str str)
@ stdcall RtlCreateTimer(ptr ptr ptr ptr long long long)
@ stdcall RtlCreateTimerQueue(ptr)
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUnicodeStringFromAsciiz(ptr str)
@ stdcall RtlCreateUserProcess(ptr long ptr ptr ptr ptr long ptr ptr ptr)
@ stdcall RtlCreateUserSecurityObject(ptr long ptr ptr long ptr ptr)
@ stub -version=0x600+ RtlCreateUserStack
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stub -version=0x600+ RtlCultureNameToLCID
@ stdcall RtlCustomCPToUnicodeN(ptr wstr long ptr str long)
@ stdcall RtlCutoverTimeToSystemTime(ptr ptr ptr long)
@ stub -version=0x600+ RtlDeCommitDebugInfo
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stdcall RtlDeactivateActivationContext(long long)
@ stdcall -arch=x86_64,arm RtlDeactivateActivationContextUnsafeFast(ptr)
@ stdcall -stub RtlDebugPrintTimes()
@ stdcall RtlDecodePointer(ptr)
@ stdcall RtlDecodeSystemPointer(ptr)
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
@ stdcall RtlDefaultNpAcl(ptr)
@ stdcall RtlDelete(ptr)
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stub -version=0x600+ RtlDeleteBarrier
@ stub -version=0x600+ RtlDeleteBoundaryDescriptor
@ stdcall RtlDeleteCriticalSection(ptr)
@ stdcall RtlDeleteElementGenericTable(ptr ptr)
@ stdcall RtlDeleteElementGenericTableAvl(ptr ptr)
@ cdecl -arch=x86_64 RtlDeleteFunctionTable(ptr)
@ stdcall RtlDeleteNoSplay(ptr ptr)
@ stdcall RtlDeleteRegistryValue(long ptr ptr)
@ stdcall RtlDeleteResource(ptr)
@ stdcall RtlDeleteSecurityObject(ptr)
@ stdcall RtlDeleteTimer(ptr ptr ptr)
@ stdcall RtlDeleteTimerQueue(ptr)
@ stdcall RtlDeleteTimerQueueEx(ptr ptr)
@ stub -version=0x600+ RtlDeregisterSecureMemoryCacheCallback
@ stdcall RtlDeregisterWait(ptr)
@ stdcall RtlDeregisterWaitEx(ptr ptr)
@ stdcall RtlDestroyAtomTable(ptr)
@ stdcall RtlDestroyEnvironment(ptr)
@ stdcall RtlDestroyHandleTable(ptr)
@ stdcall RtlDestroyHeap(long)
@ stub -version=0x600+ RtlDestroyMemoryBlockLookaside
@ stub -version=0x600+ RtlDestroyMemoryZone
@ stdcall RtlDestroyProcessParameters(ptr)
@ stdcall RtlDestroyQueryDebugBuffer(ptr)
@ stdcall RtlDetermineDosPathNameType_U(wstr)
@ stdcall RtlDllShutdownInProgress()
@ stdcall RtlDnsHostNameToComputerName(ptr ptr long)
@ stdcall RtlDoesFileExists_U(wstr)
@ stdcall RtlDosApplyFileIsolationRedirection_Ustr(long ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlDosPathNameToNtPathName_U(wstr ptr ptr ptr)
@ stdcall RtlDosPathNameToNtPathName_U_WithStatus(wstr ptr ptr ptr) ; 5.2 SP1, and higher
@ stdcall RtlDosPathNameToRelativeNtPathName_U(ptr ptr ptr ptr)
@ stdcall RtlDosPathNameToRelativeNtPathName_U_WithStatus(wstr ptr ptr ptr)
@ stdcall RtlDosSearchPath_U(wstr wstr wstr long ptr ptr)
@ stdcall RtlDosSearchPath_Ustr(long ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDumpResource(ptr)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall RtlEmptyAtomTable(ptr long)
@ stdcall -stub RtlEnableEarlyCriticalSectionEventCreation()
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
@ stdcall RtlEqualComputerName(ptr ptr)
@ stdcall RtlEqualDomainName(ptr ptr)
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualPrefixSid(ptr ptr)
@ stdcall RtlEqualSid(long long)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall RtlEraseUnicodeString(ptr)
@ stub -version=0x600+ RtlExitUserProcess
@ stdcall RtlExitUserThread(long)
@ stub -version=0x600+ RtlExpandEnvironmentStrings
@ stdcall RtlExpandEnvironmentStrings_U(ptr ptr ptr ptr)
@ stdcall -version=0x502 RtlExtendHeap(ptr long ptr ptr)
@ stub -version=0x600+ RtlExtendMemoryBlockLookaside
@ stub -version=0x600+ RtlExtendMemoryZone
@ stdcall -arch=win32 -ret64 RtlExtendedIntegerMultiply(double long)
@ stdcall -arch=win32 -ret64 RtlExtendedLargeIntegerDivide(double long ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedMagicDivide(double double long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall -arch=i386 RtlFillMemoryUlong(ptr long long)
@ stdcall RtlFinalReleaseOutOfProcessMemoryStream(ptr)
@ stub -version=0x600+ RtlFindAceByType
@ stdcall RtlFindActivationContextSectionGuid(long ptr long ptr ptr)
@ stdcall RtlFindActivationContextSectionString(long ptr long ptr ptr)
@ stdcall RtlFindCharInUnicodeString(long ptr ptr ptr)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stub -version=0x600+ RtlFindClosestEncodableLength
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(double)
@ stdcall RtlFindLongestRunClear(ptr long)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(double)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
@ stdcall RtlFirstEntrySList(ptr)
@ stdcall RtlFirstFreeAce(ptr ptr)
@ stub -version=0x600+ RtlFlsAlloc
@ stub -version=0x600+ RtlFlsFree
@ stdcall RtlFlushSecureMemoryCache(ptr ptr)
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFormatMessage(ptr long long long long ptr ptr long ptr)
@ stdcall RtlFormatMessageEx(ptr long long long long ptr ptr long ptr long)
@ stdcall RtlFreeActivationContextStack(ptr)
@ stdcall RtlFreeAnsiString(long)
@ stdcall RtlFreeHandle(ptr ptr)
@ stdcall RtlFreeHeap(long long long)
@ stub -version=0x600+ RtlFreeMemoryBlockLookaside
@ stdcall RtlFreeOemString(ptr)
@ stdcall RtlFreeSid(long)
@ stdcall RtlFreeThreadActivationContextStack()
@ stdcall RtlFreeUnicodeString(ptr)
@ stub -version=0x600+ RtlFreeUserStack
@ stdcall -version=0x502 RtlFreeUserThreadStack(ptr ptr)
@ stdcall RtlGUIDFromString(ptr ptr)
@ stdcall RtlGenerate8dot3Name(ptr ptr long ptr)
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetActiveActivationContext(ptr)
@ stdcall RtlGetCallersAddress(ptr ptr)
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetCriticalSectionRecursionCount(ptr)
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetCurrentPeb()
@ stdcall RtlGetCurrentProcessorNumber() ; 5.2 SP1 and higher
@ stub -version=0x600+ RtlGetCurrentTransaction
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetElementGenericTable(ptr long)
@ stdcall RtlGetElementGenericTableAvl(ptr long)
@ stub -version=0x600+ RtlGetFileMUIPath
@ stdcall RtlGetFrame()
@ stdcall RtlGetFullPathName_U(wstr long ptr ptr)
@ stdcall RtlGetFullPathName_UstrEx(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub -version=0x600+ -arch=x86_64 RtlGetFunctionTableListHead
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stub -version=0x600+ RtlGetIntegerAtom
@ stdcall RtlGetLastNtStatus()
@ stdcall RtlGetLastWin32Error()
@ stdcall RtlGetLengthWithoutLastFullDosOrNtPathElement(long ptr ptr)
; Yes, Microsoft really misspelled this one!
@ stdcall RtlGetLengthWithoutTrailingPathSeperators(long ptr ptr) RtlGetLengthWithoutTrailingPathSeparators
@ stdcall RtlGetLongestNtPathLength()
@ stdcall RtlGetNativeSystemInformation(long long long long) NtQuerySystemInformation
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetNtVersionNumbers(ptr ptr ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stub -version=0x600+ RtlGetParentLocaleName
@ stdcall RtlGetProcessHeaps(long ptr)
@ stub -version=0x600+ RtlGetProductInfo
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetSecurityDescriptorRMControl(ptr ptr)
@ stdcall RtlGetSetBootStatusData(ptr long long ptr long long)
@ stub -version=0x600+ RtlGetSystemPreferredUILanguages
@ stdcall RtlGetThreadErrorMode()
@ stub -version=0x600+ RtlGetThreadLangIdByIndex
@ stub -version=0x600+ RtlGetThreadPreferredUILanguages
@ stub -version=0x600+ RtlGetUILanguageInfo
@ stdcall RtlGetUnloadEventTrace()
@ stub -version=0x600+ RtlGetUnloadEventTraceEx
@ stdcall RtlGetUserInfoHeap(ptr long ptr ptr ptr)
@ stub -version=0x600+ RtlGetUserPreferredUILanguages
@ stdcall RtlGetVersion(ptr)
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stub -version=0x600+ RtlHeapTrkInitialize
@ stdcall RtlIdentifierAuthoritySid(ptr)
@ stub -version=0x600+ RtlIdnToAscii
@ stub -version=0x600+ RtlIdnToNameprepUnicode
@ stub -version=0x600+ RtlIdnToUnicode
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlImageNtHeaderEx(long ptr double ptr)
@ stdcall RtlImageRvaToSection(ptr long long)
@ stdcall RtlImageRvaToVa(ptr long long ptr)
@ stdcall RtlImpersonateSelf(long)
@ stub -version=0x600+ RtlImpersonateSelfEx
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stub -version=0x600+ RtlInitBarrier
@ stdcall RtlInitCodePageTable(ptr ptr)
@ stdcall RtlInitMemoryStream(ptr)
@ stdcall RtlInitNlsTables(ptr ptr ptr ptr)
@ stdcall RtlInitOutOfProcessMemoryStream(ptr)
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
@ stdcall -stub RtlInitializeAtomPackage(ptr)
@ stdcall RtlInitializeBitMap(ptr long long)
@ stdcall -stub -version=0x600+ RtlInitializeConditionVariable(ptr)
@ stdcall RtlInitializeContext(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeCriticalSection(ptr)
@ stdcall RtlInitializeCriticalSectionAndSpinCount(ptr long)
@ stub -version=0x600+ RtlInitializeCriticalSectionEx
@ stub -version=0x600+ -arch=i386 RtlInitializeExceptionChain
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeHandleTable(long long ptr)
@ stub -version=0x600+ RtlInitializeNtUserPfn
@ stdcall RtlInitializeRXact(ptr long ptr)
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall -stub -version=0x600+ RtlInitializeSRWLock(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
@ stdcall RtlInsertElementGenericTable(ptr ptr long ptr)
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ stdcall RtlInsertElementGenericTableFull(ptr ptr long ptr ptr long)
@ stdcall RtlInsertElementGenericTableFullAvl(ptr ptr long ptr ptr long)
@ stdcall -arch=x86_64 RtlInstallFunctionTableCallback(double double long ptr ptr ptr)
@ stdcall RtlInt64ToUnicodeString(double long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stdcall -arch=win32 -ret64 RtlInterlockedCompareExchange64(ptr double double)
@ stdcall RtlInterlockedFlushSList(ptr)
@ stdcall RtlInterlockedPopEntrySList(ptr)
@ stdcall RtlInterlockedPushEntrySList(ptr ptr)
@ stdcall -arch=x86_64 RtlInterlockedPushListSList(ptr ptr ptr long)
@ stub -version=0x600+ RtlIoDecodeMemIoResource
@ stub -version=0x600+ RtlIoEncodeMemIoResource
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
@ stdcall RtlIsCriticalSectionLocked(ptr)
@ stdcall RtlIsCriticalSectionLockedByThread(ptr)
@ stub -version=0x600+ RtlIsCurrentThreadAttachExempt
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stdcall RtlIsGenericTableEmpty(ptr)
@ stdcall RtlIsGenericTableEmptyAvl(ptr)
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stub -version=0x600+ RtlIsNormalizedString
@ stdcall RtlIsTextUnicode(ptr long ptr)
@ stdcall RtlIsThreadWithinLoaderCallout()
@ stdcall RtlIsValidHandle(ptr ptr)
@ stdcall RtlIsValidIndexHandle(ptr long ptr)
@ stub -version=0x600+ RtlIsValidLocaleName
@ stub -version=0x600+ RtlLCIDToCultureName
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(double double)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(double double ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(double)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(double long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(double double)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stub -version=0x600+ RtlLcidToLocaleName
@ stdcall RtlLeaveCriticalSection(ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
@ stub -version=0x600+ RtlLocaleNameToLcid
@ stdcall RtlLockBootStatusData(ptr)
@ stub -version=0x600+ RtlLockCurrentThread
@ stdcall RtlLockHeap(long)
@ stub -version=0x600+ RtlLockMemoryBlockLookaside
@ stdcall RtlLockMemoryStreamRegion(ptr int64 int64 long)
@ stub -version=0x600+ RtlLockMemoryZone
@ stub -version=0x600+ RtlLockModuleSection
@ stdcall -stub RtlLogStackBackTrace()
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stdcall RtlLookupElementGenericTable(ptr ptr)
@ stdcall RtlLookupElementGenericTableAvl(ptr ptr)
@ stdcall RtlLookupElementGenericTableFull(ptr ptr ptr long)
@ stdcall RtlLookupElementGenericTableFullAvl(ptr ptr ptr long)
@ stdcall -arch=x86_64 RtlLookupFunctionEntry(long ptr ptr)
@ stdcall -arch=x86_64 RtlLookupFunctionTable(int64 ptr ptr)
@ stdcall RtlMakeSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlMapGenericMask(long ptr)
@ stdcall RtlMapSecurityErrorToNtStatus(long)
@ stdcall RtlMoveMemory(ptr ptr long)
@ stdcall RtlMultiAppendUnicodeStringBuffer(ptr long ptr)
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
@ stdcall RtlMultipleAllocateHeap(ptr long ptr long ptr)
@ stdcall RtlMultipleFreeHeap(ptr long long ptr)
@ stdcall RtlNewInstanceSecurityObject(long long ptr ptr ptr ptr ptr long ptr ptr)
@ stdcall RtlNewSecurityGrantedAccess(long ptr ptr ptr ptr ptr)
@ stdcall RtlNewSecurityObject(ptr ptr ptr long ptr ptr)
@ stdcall RtlNewSecurityObjectEx(ptr ptr ptr ptr long long ptr ptr)
@ stdcall RtlNewSecurityObjectWithMultipleInheritance(ptr ptr ptr ptr long long long ptr ptr)
@ stdcall RtlNormalizeProcessParams(ptr)
@ stub -version=0x600+ RtlNormalizeString
@ stdcall RtlNtPathNameToDosPathName(long ptr ptr ptr) ; CHECKME (last arg)
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stub -version=0x600+ -arch=x86_64 RtlNtdllName
@ stdcall RtlNumberGenericTableElements(ptr)
@ stdcall RtlNumberGenericTableElementsAvl(ptr)
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stub -version=0x600+ RtlNumberOfSetBitsUlongPtr
@ stdcall RtlOemStringToUnicodeSize(ptr) RtlxOemStringToUnicodeSize
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stub -version=0x600+ RtlOwnerAcesPresent
@ stdcall RtlPcToFileHeader(ptr ptr)
@ stdcall RtlPinAtomInAtomTable(ptr long)
@ stdcall RtlPopFrame(ptr)
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stub -version=0x600+ -arch=x86_64 RtlPrepareForProcessCloning
@ stub -version=0x600+ RtlProcessFlsData
@ stdcall RtlProtectHeap(ptr long)
@ stdcall RtlPushFrame(ptr)
@ stub -version=0x600+ RtlQueryActivationContextApplicationSettings
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stub -version=0x600+ RtlQueryCriticalSectionOwner
@ stdcall RtlQueryDepthSList(ptr)
@ stub -version=0x600+ RtlQueryDynamicTimeZoneInformation
@ stub -version=0x600+ RtlQueryElevationFlags
@ stub -version=0x600+ RtlQueryEnvironmentVariable
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryHeapInformation(long long ptr long ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryInformationActivationContext(long long ptr long ptr long ptr)
@ stdcall RtlQueryInformationActiveActivationContext(long ptr long ptr)
@ stdcall RtlQueryInterfaceMemoryStream(ptr ptr ptr)
@ stub -version=0x600+ RtlQueryModuleInformation
@ stdcall -stub RtlQueryProcessBackTraceInformation(ptr)
@ stdcall RtlQueryProcessDebugInformation(long long ptr)
@ stdcall RtlQueryProcessHeapInformation(ptr)
@ stdcall -stub RtlQueryProcessLockInformation(ptr)
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stdcall RtlQuerySecurityObject(ptr long ptr long ptr)
@ stdcall RtlQueryTagHeap(ptr long long long ptr)
@ stdcall RtlQueryTimeZoneInformation(ptr)
@ stdcall -arch=i386,x86_64 RtlQueueApcWow64Thread(ptr ptr ptr ptr ptr)
@ stdcall RtlQueueWorkItem(ptr ptr long)
@ stdcall -register RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stdcall RtlRandomEx(ptr)
@ stdcall RtlReAllocateHeap(long long ptr long)
@ stdcall RtlReadMemoryStream(ptr ptr long ptr)
@ stdcall RtlReadOutOfProcessMemoryStream(ptr ptr long ptr)
@ stdcall RtlRealPredecessor(ptr)
@ stdcall RtlRealSuccessor(ptr)
@ stdcall RtlRegisterSecureMemoryCacheCallback(ptr)
@ stub -version=0x600+ RtlRegisterThreadWithCsrss
@ stdcall RtlRegisterWait(ptr ptr ptr ptr long long)
@ stdcall RtlReleaseActivationContext(ptr)
@ stdcall RtlReleaseMemoryStream(ptr)
@ stdcall RtlReleasePebLock()
@ stdcall RtlReleasePrivilege(ptr)
@ stdcall RtlReleaseRelativeName(ptr)
@ stdcall RtlReleaseResource(ptr)
@ stdcall -stub -version=0x600+ RtlReleaseSRWLockExclusive(ptr)
@ stdcall -stub -version=0x600+ RtlReleaseSRWLockShared(ptr)
@ stdcall RtlRemoteCall(ptr ptr ptr long ptr long long)
@ stub -version=0x600+ RtlRemovePrivileges
@ stdcall RtlRemoveVectoredContinueHandler(ptr)
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stub -version=0x600+ RtlReportException
@ stub -version=0x600+ RtlResetMemoryBlockLookaside
@ stub -version=0x600+ RtlResetMemoryZone
@ stdcall RtlResetRtlTranslations(ptr)
@ stdcall -arch=x86_64 RtlRestoreContext(ptr ptr)
@ stdcall RtlRestoreLastWin32Error(long) RtlSetLastWin32Error
@ stub -version=0x600+ RtlRetrieveNtUserPfn
@ stdcall RtlRevertMemoryStream(ptr)
@ stdcall RtlRunDecodeUnicodeString(long ptr)
@ stdcall RtlRunEncodeUnicodeString(long ptr)
@ stdcall -stub -version=0x600+ RtlRunOnceBeginInitialize(ptr long ptr)
@ stdcall -stub -version=0x600+ RtlRunOnceComplete(ptr long ptr)
@ stdcall -stub -version=0x600+ RtlRunOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ RtlRunOnceInitialize(ptr)
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
@ stdcall RtlSeekMemoryStream(ptr int64 long ptr)
@ stdcall RtlSelfRelativeToAbsoluteSD2(ptr ptr)
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub -version=0x600+ RtlSendMsgToSm
@ stdcall RtlSetAllBits(ptr)
@ stdcall RtlSetAttributesSecurityDescriptor(ptr long ptr)
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetControlSecurityDescriptor(ptr long long)
@ stdcall RtlSetCriticalSectionSpinCount(ptr long)
@ stdcall RtlSetCurrentDirectory_U(ptr)
@ stdcall RtlSetCurrentEnvironment(wstr ptr)
@ stub -version=0x600+ RtlSetCurrentTransaction
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
@ stub -version=0x600+ RtlSetDynamicTimeZoneInformation
@ stdcall RtlSetEnvironmentStrings(wstr long)
@ stub -version=0x600+ RtlSetEnvironmentVar
@ stdcall RtlSetEnvironmentVariable(ptr ptr ptr)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetHeapInformation(ptr long ptr ptr)
@ stdcall RtlSetInformationAcl(ptr ptr long long)
@ stdcall RtlSetIoCompletionCallback(long ptr long)
@ stdcall RtlSetLastWin32Error(long)
@ stdcall RtlSetLastWin32ErrorAndNtStatusFromNtStatus(long)
@ stdcall RtlSetMemoryStreamSize(ptr int64)
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
@ stub -version=0x600+ RtlSetProcessDebugInformation
@ cdecl RtlSetProcessIsCritical(long ptr long)
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetSecurityDescriptorRMControl(ptr ptr)
@ stdcall RtlSetSecurityObject(long ptr ptr ptr ptr)
@ stdcall RtlSetSecurityObjectEx(long ptr ptr long ptr ptr)
@ stdcall RtlSetThreadErrorMode(long ptr)
@ cdecl RtlSetThreadIsCritical(long ptr long)
@ stdcall RtlSetThreadPoolStartFunc(ptr ptr)
@ stub -version=0x600+ RtlSetThreadPreferredUILanguages
@ stdcall RtlSetTimeZoneInformation(ptr)
@ stdcall RtlSetTimer(ptr ptr ptr ptr long long long)
@ stdcall RtlSetUnhandledExceptionFilter(ptr)
@ stdcall -stub -version=0x502 RtlSetUnicodeCallouts(ptr)
@ stdcall RtlSetUserFlagsHeap(ptr long ptr long long)
@ stdcall RtlSetUserValueHeap(ptr long ptr ptr)
@ stub -version=0x600+ RtlSidDominates
@ stub -version=0x600+ RtlSidEqualLevel
@ stub -version=0x600+ RtlSidHashInitialize
@ stub -version=0x600+ RtlSidHashLookup
@ stub -version=0x600+ RtlSidIsHigherLevel
@ stdcall RtlSizeHeap(long long ptr)
@ stdcall -stub -version=0x600+ RtlSleepConditionVariableCS(ptr ptr ptr)
@ stdcall -stub -version=0x600+ RtlSleepConditionVariableSRW(ptr ptr ptr long)
@ stdcall RtlSplay(ptr)
@ stdcall RtlStartRXact(ptr)
@ stdcall RtlStatMemoryStream(ptr ptr long)
@ stdcall RtlStringFromGUID(ptr ptr)
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stdcall RtlSubtreePredecessor(ptr)
@ stdcall RtlSubtreeSuccessor(ptr)
@ stdcall RtlSystemTimeToLocalTime(ptr ptr)
@ stdcall -version=0x600+ RtlTestBit(ptr long)
@ stdcall RtlTimeFieldsToTime(ptr ptr)
@ stdcall RtlTimeToElapsedTimeFields(long long)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields (long long)
@ stdcall RtlTraceDatabaseAdd(ptr long ptr ptr)
@ stdcall RtlTraceDatabaseCreate(long ptr long long ptr)
@ stdcall RtlTraceDatabaseDestroy(ptr)
@ stdcall RtlTraceDatabaseEnumerate(ptr ptr ptr)
@ stdcall RtlTraceDatabaseFind(ptr long ptr ptr)
@ stdcall RtlTraceDatabaseLock(ptr)
@ stdcall RtlTraceDatabaseUnlock(ptr)
@ stdcall RtlTraceDatabaseValidate(ptr)
@ stub -version=0x600+ RtlTryAcquirePebLock
@ stdcall RtlTryEnterCriticalSection(ptr)
@ stdcall RtlUnhandledExceptionFilter2(ptr long)
@ stdcall RtlUnhandledExceptionFilter(ptr)
@ stdcall RtlUnicodeStringToAnsiSize(ptr) RtlxUnicodeStringToAnsiSize
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUnicodeStringToInteger(ptr long ptr)
@ stdcall RtlUnicodeStringToOemSize(ptr) RtlxUnicodeStringToOemSize
@ stdcall RtlUnicodeStringToOemString(ptr ptr long)
@ stdcall RtlUnicodeToCustomCPN(ptr ptr long ptr wstr long)
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long)
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUniform(ptr)
@ stdcall RtlUnlockBootStatusData(ptr)
@ stub -version=0x600+ RtlUnlockCurrentThread
@ stdcall RtlUnlockHeap(long)
@ stub -version=0x600+ RtlUnlockMemoryBlockLookaside
@ stdcall RtlUnlockMemoryStreamRegion(ptr int64 int64 long)
@ stub -version=0x600+ RtlUnlockMemoryZone
@ stub -version=0x600+ RtlUnlockModuleSection
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
@ stub -version=0x600+ RtlUpdateClonedCriticalSection
@ stub -version=0x600+ RtlUpdateClonedSRWLock
@ stdcall RtlUpdateTimer(ptr ptr long long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stdcall -version=0x502 RtlUsageHeap(ptr long ptr)
@ stub -version=0x600+ RtlUserThreadStart
@ stdcall RtlValidAcl(ptr)
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stdcall RtlValidateProcessHeaps()
@ stdcall RtlValidateUnicodeString(long ptr)
@ stdcall RtlVerifyVersionInfo(ptr long double)
@ stdcall -arch=x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stdcall -stub -version=0x600+ RtlWakeAllConditionVariable(ptr)
@ stdcall -stub -version=0x600+ RtlWakeConditionVariable(ptr)
@ stdcall RtlWalkFrameChain(ptr long long)
@ stdcall RtlWalkHeap(long ptr)
@ stub -version=0x600+ RtlWerpReportException
@ stub -version=0x600+ RtlWow64CallFunction64
@ stdcall RtlWow64EnableFsRedirection(long)
@ stdcall RtlWow64EnableFsRedirectionEx(long ptr)
@ stub -version=0x600+ -arch=x86_64 RtlWow64GetThreadContext
@ stub -version=0x600+ -arch=x86_64 RtlWow64LogMessageInEventLogger
@ stub -version=0x600+ -arch=x86_64 RtlWow64SetThreadContext
@ stub -version=0x600+ -arch=x86_64 RtlWow64SuspendThread
@ stdcall RtlWriteMemoryStream(ptr ptr long ptr)
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stdcall RtlZeroHeap(ptr long)
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlZombifyActivationContext(ptr)
@ stdcall RtlpApplyLengthFunction(long long ptr ptr)
@ stub -version=0x600+ RtlpCheckDynamicTimeZoneInformation
@ stub -version=0x600+ RtlpCleanupRegistryKeys
@ stub -version=0x600+ RtlpConvertCultureNamesToLCIDs
@ stub -version=0x600+ RtlpConvertLCIDsToCultureNames
@ stub -version=0x600+ RtlpCreateProcessRegistryInfo
@ stdcall RtlpEnsureBufferSize(long ptr long)
@ stub -version=0x600+ RtlpGetLCIDFromLangInfoNode
@ stub -version=0x600+ RtlpGetNameFromLangInfoNode
@ stub -version=0x600+ RtlpGetSystemDefaultUILanguage
@ stub -version=0x600+ RtlpGetUserOrMachineUILanguage4NLS
@ stub -version=0x600+ RtlpInitializeLangRegistryInfo
@ stub -version=0x600+ RtlpIsQualifiedLanguage
@ stub -version=0x600+ RtlpLoadMachineUIByPolicy
@ stub -version=0x600+ RtlpLoadUserUIByPolicy
@ stub -version=0x600+ RtlpMuiFreeLangRegistryInfo
@ stub -version=0x600+ RtlpMuiRegCreateRegistryInfo
@ stub -version=0x600+ RtlpMuiRegFreeRegistryInfo
@ stub -version=0x600+ RtlpMuiRegLoadRegistryInfo
@ stdcall RtlpNotOwnerCriticalSection(ptr)
@ stdcall RtlpNtCreateKey(ptr long ptr long ptr ptr)
@ stdcall RtlpNtEnumerateSubKey(ptr ptr long long)
@ stdcall RtlpNtMakeTemporaryKey(ptr)
@ stdcall RtlpNtOpenKey(ptr long ptr long)
@ stdcall RtlpNtQueryValueKey(ptr ptr ptr ptr long)
@ stdcall RtlpNtSetValueKey(ptr long ptr long)
@ stub -version=0x600+ RtlpQueryDefaultUILanguage
@ stub -version=0x600+ -arch=x86_64 RtlpQueryProcessDebugInformationFromWow64
@ stub -version=0x600+ RtlpRefreshCachedUILanguage
@ stub -version=0x600+ RtlpSetInstallLanguage
@ stub -version=0x600+ RtlpSetPreferredUILanguages
@ stub -version=0x600+ RtlpSetUserPreferredUILanguages
@ stdcall RtlpUnWaitCriticalSection(ptr)
@ stub -version=0x600+ RtlpVerifyAndCommitUILanguageSettings
@ stdcall RtlpWaitForCriticalSection(ptr)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr)
@ stdcall RtlxOemStringToUnicodeSize(ptr)
@ stdcall RtlxUnicodeStringToAnsiSize(ptr)
@ stdcall RtlxUnicodeStringToOemSize(ptr)
@ stub -version=0x600+ ShipAssert
@ stub -version=0x600+ ShipAssertGetBufferInfo
@ stub -version=0x600+ ShipAssertMsgA
@ stub -version=0x600+ ShipAssertMsgW
@ stub -version=0x600+ TpAllocAlpcCompletion
@ stub -version=0x600+ TpAllocCleanupGroup
@ stub -version=0x600+ TpAllocIoCompletion
@ stub -version=0x600+ TpAllocPool
@ stub -version=0x600+ TpAllocTimer
@ stub -version=0x600+ TpAllocWait
@ stub -version=0x600+ TpAllocWork
@ stub -version=0x600+ TpCallbackLeaveCriticalSectionOnCompletion
@ stub -version=0x600+ TpCallbackMayRunLong
@ stub -version=0x600+ TpCallbackReleaseMutexOnCompletion
@ stub -version=0x600+ TpCallbackReleaseSemaphoreOnCompletion
@ stub -version=0x600+ TpCallbackSetEventOnCompletion
@ stub -version=0x600+ TpCallbackUnloadDllOnCompletion
@ stub -version=0x600+ TpCancelAsyncIoOperation
@ stub -version=0x600+ TpCaptureCaller
@ stub -version=0x600+ TpCheckTerminateWorker
@ stub -version=0x600+ TpDbgDumpHeapUsage
@ stub -version=0x600+ TpDbgSetLogRoutine
@ stub -version=0x600+ TpDisassociateCallback
@ stub -version=0x600+ TpIsTimerSet
@ stub -version=0x600+ TpPostWork
@ stub -version=0x600+ TpReleaseAlpcCompletion
@ stub -version=0x600+ TpReleaseCleanupGroup
@ stub -version=0x600+ TpReleaseCleanupGroupMembers
@ stub -version=0x600+ TpReleaseIoCompletion
@ stub -version=0x600+ TpReleasePool
@ stub -version=0x600+ TpReleaseTimer
@ stub -version=0x600+ TpReleaseWait
@ stub -version=0x600+ TpReleaseWork
@ stub -version=0x600+ TpSetPoolMaxThreads
@ stub -version=0x600+ TpSetPoolMinThreads
@ stub -version=0x600+ TpSetTimer
@ stub -version=0x600+ TpSetWait
@ stub -version=0x600+ TpSimpleTryPost
@ stub -version=0x600+ TpStartAsyncIoOperation
@ stub -version=0x600+ TpWaitForAlpcCompletion
@ stub -version=0x600+ TpWaitForIoCompletion
@ stub -version=0x600+ TpWaitForTimer
@ stub -version=0x600+ TpWaitForWait
@ stub -version=0x600+ TpWaitForWork
@ stdcall -ret64 VerSetConditionMask(double long long)
@ stub -version=0x600+ WerCheckEventEscalation
@ stub -version=0x600+ WerReportSQMEvent
@ stub -version=0x600+ WerReportWatsonEvent
@ stub -version=0x600+ WinSqmAddToStream
@ stub -version=0x600+ WinSqmEndSession
@ stub -version=0x600+ WinSqmEventEnabled
@ stub -version=0x600+ WinSqmEventWrite
@ stub -version=0x600+ WinSqmIsOptedIn
@ stub -version=0x600+ WinSqmSetString
@ stub -version=0x600+ WinSqmStartSession
@ stdcall ZwAcceptConnectPort(ptr long ptr long long ptr)
@ stdcall ZwAccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr)
@ stdcall ZwAccessCheckByType(ptr ptr ptr long ptr long ptr ptr long ptr ptr)
@ stdcall ZwAccessCheckByTypeAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall ZwAccessCheckByTypeResultList(ptr ptr ptr long ptr long ptr ptr long ptr ptr)
@ stdcall ZwAccessCheckByTypeResultListAndAuditAlarm(ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(ptr ptr ptr ptr ptr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stub -version=0x600+ ZwAcquireCMFViewOwnership
@ stdcall ZwAddAtom(ptr long ptr)
@ stdcall ZwAddBootEntry(ptr long)
@ stdcall ZwAddDriverEntry(ptr long)
@ stdcall ZwAdjustGroupsToken(long long long long long long)
@ stdcall ZwAdjustPrivilegesToken(long long long long long long)
@ stdcall ZwAlertResumeThread(long ptr)
@ stdcall ZwAlertThread(long)
@ stdcall ZwAllocateLocallyUniqueId(ptr)
@ stdcall ZwAllocateUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwAllocateUuids(ptr ptr ptr ptr)
@ stdcall ZwAllocateVirtualMemory(long ptr ptr ptr long long)
@ stub -version=0x600+ ZwAlpcAcceptConnectPort
@ stub -version=0x600+ ZwAlpcCancelMessage
@ stub -version=0x600+ ZwAlpcConnectPort
@ stub -version=0x600+ ZwAlpcCreatePort
@ stub -version=0x600+ ZwAlpcCreatePortSection
@ stub -version=0x600+ ZwAlpcCreateResourceReserve
@ stub -version=0x600+ ZwAlpcCreateSectionView
@ stub -version=0x600+ ZwAlpcCreateSecurityContext
@ stub -version=0x600+ ZwAlpcDeletePortSection
@ stub -version=0x600+ ZwAlpcDeleteResourceReserve
@ stub -version=0x600+ ZwAlpcDeleteSectionView
@ stub -version=0x600+ ZwAlpcDeleteSecurityContext
@ stub -version=0x600+ ZwAlpcDisconnectPort
@ stub -version=0x600+ ZwAlpcImpersonateClientOfPort
@ stub -version=0x600+ ZwAlpcOpenSenderProcess
@ stub -version=0x600+ ZwAlpcOpenSenderThread
@ stub -version=0x600+ ZwAlpcQueryInformation
@ stub -version=0x600+ ZwAlpcQueryInformationMessage
@ stub -version=0x600+ ZwAlpcRevokeSecurityContext
@ stub -version=0x600+ ZwAlpcSendWaitReceivePort
@ stub -version=0x600+ ZwAlpcSetInformation
@ stdcall ZwApphelpCacheControl(long ptr)
@ stdcall ZwAreMappedFilesTheSame(ptr ptr)
@ stdcall ZwAssignProcessToJobObject(long long)
@ stdcall ZwCallbackReturn(ptr long long)
@ stdcall ZwCancelDeviceWakeupRequest(ptr)
@ stdcall ZwCancelIoFile(long ptr)
@ stub -version=0x600+ ZwCancelIoFileEx
@ stub -version=0x600+ ZwCancelSynchronousIoFile
@ stdcall ZwCancelTimer(long ptr)
@ stdcall ZwClearEvent(long)
@ stdcall ZwClose(long)
@ stdcall ZwCloseObjectAuditAlarm(ptr ptr long)
@ stub -version=0x600+ ZwCommitComplete
@ stub -version=0x600+ ZwCommitEnlistment
@ stub -version=0x600+ ZwCommitTransaction
@ stdcall ZwCompactKeys(long ptr)
@ stdcall ZwCompareTokens(ptr ptr ptr)
@ stdcall ZwCompleteConnectPort(ptr)
@ stdcall ZwCompressKey(ptr)
@ stdcall ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall ZwContinue(ptr long)
@ stdcall ZwCreateDebugObject(ptr long ptr long)
@ stdcall ZwCreateDirectoryObject(long long long)
@ stub -version=0x600+ ZwCreateEnlistment
@ stdcall ZwCreateEvent(long long long long long)
@ stdcall ZwCreateEventPair(ptr long ptr)
@ stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr)
@ stdcall ZwCreateIoCompletion(ptr long ptr long)
@ stdcall ZwCreateJobObject(ptr long ptr)
@ stdcall ZwCreateJobSet(long ptr long)
@ stdcall ZwCreateKey(ptr long ptr long ptr long long)
@ stub -version=0x600+ ZwCreateKeyTransacted
@ stdcall ZwCreateKeyedEvent(ptr long ptr long)
@ stdcall ZwCreateMailslotFile(long long long long long long long long)
@ stdcall ZwCreateMutant(ptr long ptr long)
@ stdcall ZwCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall ZwCreatePagingFile(long long long long)
@ stdcall ZwCreatePort(ptr ptr long long long)
@ stdcall ZwCreateProcess(ptr long ptr ptr long ptr ptr ptr)
@ stdcall ZwCreateProcessEx(ptr long ptr ptr long ptr ptr ptr long)
@ stdcall ZwCreateProfile(ptr ptr ptr long long ptr long long long) ; CHECKME
@ stub -version=0x600+ ZwCreateResourceManager
@ stdcall ZwCreateSection(ptr long ptr ptr long long long)
@ stdcall ZwCreateSemaphore(ptr long ptr long long)
@ stdcall ZwCreateSymbolicLinkObject(ptr long ptr ptr)
@ stdcall ZwCreateThread(ptr long ptr ptr ptr ptr ptr long)
@ stub -version=0x600+ ZwCreateThreadEx
@ stdcall ZwCreateTimer(ptr long ptr long)
@ stdcall ZwCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub -version=0x600+ ZwCreateTransaction
@ stub -version=0x600+ ZwCreateTransactionManager
@ stub -version=0x600+ ZwCreateUserProcess
@ stdcall ZwCreateWaitablePort(ptr ptr long long long)
@ stub -version=0x600+ ZwCreateWorkerFactory
@ stdcall ZwDebugActiveProcess(ptr ptr)
@ stdcall ZwDebugContinue(ptr ptr long)
@ stdcall ZwDelayExecution(long ptr)
@ stdcall ZwDeleteAtom(long)
@ stdcall ZwDeleteBootEntry(long)
@ stdcall ZwDeleteDriverEntry(long)
@ stdcall ZwDeleteFile(ptr)
@ stdcall ZwDeleteKey(long)
@ stdcall ZwDeleteObjectAuditAlarm(ptr ptr long)
@ stub -version=0x600+ ZwDeletePrivateNamespace
@ stdcall ZwDeleteValueKey(long ptr)
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long)
@ stdcall ZwDisplayString(ptr)
@ stdcall ZwDuplicateObject(long long long ptr long long long)
@ stdcall ZwDuplicateToken(long long long long long long)
@ stdcall ZwEnumerateBootEntries(ptr ptr)
@ stdcall ZwEnumerateDriverEntries(ptr ptr)
@ stdcall ZwEnumerateKey(long long long ptr long ptr)
@ stdcall ZwEnumerateSystemEnvironmentValuesEx(long ptr long)
@ stub -version=0x600+ ZwEnumerateTransactionObject
@ stdcall ZwEnumerateValueKey(long long long ptr long ptr)
@ stdcall ZwExtendSection(ptr ptr)
@ stdcall ZwFilterToken(ptr long ptr ptr ptr ptr)
@ stdcall ZwFindAtom(ptr long ptr)
@ stdcall ZwFlushBuffersFile(long ptr)
@ stub -version=0x600+ ZwFlushInstallUILanguage
@ stdcall ZwFlushInstructionCache(long ptr long)
@ stdcall ZwFlushKey(long)
@ stub -version=0x600+ ZwFlushProcessWriteBuffers
@ stdcall ZwFlushVirtualMemory(long ptr ptr long)
@ stdcall ZwFlushWriteBuffer()
@ stdcall ZwFreeUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwFreeVirtualMemory(long ptr ptr long)
@ stub -version=0x600+ ZwFreezeRegistry
@ stub -version=0x600+ ZwFreezeTransactions
@ stdcall ZwFsControlFile(long long long long long long long long long long)
@ stdcall ZwGetContextThread(long ptr)
@ stdcall ZwGetCurrentProcessorNumber()
@ stdcall ZwGetDevicePowerState(ptr ptr)
@ stub -version=0x600+ ZwGetMUIRegistryInfo
@ stub -version=0x600+ ZwGetNextProcess
@ stub -version=0x600+ ZwGetNextThread
@ stub -version=0x600+ ZwGetNlsSectionPtr
@ stub -version=0x600+ ZwGetNotificationResourceManager
@ stdcall ZwGetPlugPlayEvent(long long ptr long)
@ stdcall ZwGetWriteWatch(long long ptr long ptr ptr ptr)
@ stdcall ZwImpersonateAnonymousToken(ptr)
@ stdcall ZwImpersonateClientOfPort(ptr ptr)
@ stdcall ZwImpersonateThread(ptr ptr ptr)
@ stub -version=0x600+ ZwInitializeNlsFiles
@ stdcall ZwInitializeRegistry(long)
@ stdcall ZwInitiatePowerAction(long long long long)
@ stdcall ZwIsProcessInJob(long long)
@ stdcall ZwIsSystemResumeAutomatic()
@ stub -version=0x600+ ZwIsUILanguageComitted
@ stdcall ZwListenPort(ptr ptr)
@ stdcall ZwLoadDriver(ptr)
@ stdcall ZwLoadKey2(ptr ptr long)
@ stdcall ZwLoadKey(ptr ptr)
@ stdcall ZwLoadKeyEx(ptr ptr long ptr)
@ stdcall ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stdcall ZwLockProductActivationKeys(ptr ptr)
@ stdcall ZwLockRegistryKey(ptr)
@ stdcall ZwLockVirtualMemory(long ptr ptr long)
@ stdcall ZwMakePermanentObject(ptr)
@ stdcall ZwMakeTemporaryObject(long)
@ stub -version=0x600+ ZwMapCMFModule
@ stdcall ZwMapUserPhysicalPages(ptr ptr ptr)
@ stdcall ZwMapUserPhysicalPagesScatter(ptr ptr ptr)
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall ZwModifyBootEntry(ptr)
@ stdcall ZwModifyDriverEntry(ptr)
@ stdcall ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
@ stdcall ZwNotifyChangeMultipleKeys(ptr long ptr ptr ptr ptr ptr long long ptr long long)
@ stdcall ZwOpenDirectoryObject(long long long)
@ stub -version=0x600+ ZwOpenEnlistment
@ stdcall ZwOpenEvent(long long long)
@ stdcall ZwOpenEventPair(ptr long ptr)
@ stdcall ZwOpenFile(ptr long ptr ptr long long)
@ stdcall ZwOpenIoCompletion(ptr long ptr)
@ stdcall ZwOpenJobObject(ptr long ptr)
@ stdcall ZwOpenKey(ptr long ptr)
@ stub -version=0x600+ ZwOpenKeyTransacted
@ stdcall ZwOpenKeyedEvent(ptr long ptr)
@ stdcall ZwOpenMutant(ptr long ptr)
@ stdcall ZwOpenObjectAuditAlarm(ptr ptr ptr ptr ptr ptr long long ptr long long ptr)
@ stub -version=0x600+ ZwOpenPrivateNamespace
@ stdcall ZwOpenProcess(ptr long ptr ptr)
@ stdcall ZwOpenProcessToken(long long ptr)
@ stdcall ZwOpenProcessTokenEx(long long long ptr)
@ stub -version=0x600+ ZwOpenResourceManager
@ stdcall ZwOpenSection(ptr long ptr)
@ stdcall ZwOpenSemaphore(long long ptr)
@ stub -version=0x600+ ZwOpenSession
@ stdcall ZwOpenSymbolicLinkObject (ptr long ptr)
@ stdcall ZwOpenThread(ptr long ptr ptr)
@ stdcall ZwOpenThreadToken(long long long ptr)
@ stdcall ZwOpenThreadTokenEx(long long long long ptr)
@ stdcall ZwOpenTimer(ptr long ptr)
@ stub -version=0x600+ ZwOpenTransaction
@ stub -version=0x600+ ZwOpenTransactionManager
@ stdcall ZwPlugPlayControl(ptr ptr long)
@ stdcall ZwPowerInformation(long ptr long ptr long)
@ stub -version=0x600+ ZwPrePrepareComplete
@ stub -version=0x600+ ZwPrePrepareEnlistment
@ stub -version=0x600+ ZwPrepareComplete
@ stub -version=0x600+ ZwPrepareEnlistment
@ stdcall ZwPrivilegeCheck(ptr ptr ptr)
@ stdcall ZwPrivilegeObjectAuditAlarm(ptr ptr ptr long ptr long)
@ stdcall ZwPrivilegedServiceAuditAlarm(ptr ptr ptr ptr long)
@ stub -version=0x600+ ZwPropagationComplete
@ stub -version=0x600+ ZwPropagationFailed
@ stdcall ZwProtectVirtualMemory(long ptr ptr long ptr)
@ stdcall ZwPulseEvent(long ptr)
@ stdcall ZwQueryAttributesFile(ptr ptr)
@ stdcall ZwQueryBootEntryOrder(ptr ptr)
@ stdcall ZwQueryBootOptions(ptr ptr)
@ stdcall ZwQueryDebugFilterState(long long)
@ stdcall ZwQueryDefaultLocale(long ptr)
@ stdcall ZwQueryDefaultUILanguage(ptr)
@ stdcall ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall ZwQueryDirectoryObject(long ptr long long long ptr ptr)
@ stdcall ZwQueryDriverEntryOrder(ptr ptr)
@ stdcall ZwQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall ZwQueryEvent(long long ptr long ptr)
@ stdcall ZwQueryFullAttributesFile(ptr ptr)
@ stdcall ZwQueryInformationAtom(long long ptr long ptr)
@ stub -version=0x600+ ZwQueryInformationEnlistment
@ stdcall ZwQueryInformationFile(long ptr ptr long long)
@ stdcall ZwQueryInformationJobObject(long long ptr long ptr)
@ stdcall ZwQueryInformationPort(ptr long ptr long ptr)
@ stdcall ZwQueryInformationProcess(long long ptr long ptr)
@ stub -version=0x600+ ZwQueryInformationResourceManager
@ stdcall ZwQueryInformationThread(long long ptr long ptr)
@ stdcall ZwQueryInformationToken(long long ptr long ptr)
@ stub -version=0x600+ ZwQueryInformationTransaction
@ stub -version=0x600+ ZwQueryInformationTransactionManager
@ stub -version=0x600+ ZwQueryInformationWorkerFactory
@ stdcall ZwQueryInstallUILanguage(ptr)
@ stdcall ZwQueryIntervalProfile(long ptr)
@ stdcall ZwQueryIoCompletion(long long ptr long ptr)
@ stdcall ZwQueryKey(long long ptr long ptr)
@ stub -version=0x600+ ZwQueryLicenseValue
@ stdcall ZwQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall ZwQueryMutant(long long ptr long ptr)
@ stdcall ZwQueryObject(long long long long long)
@ stdcall ZwQueryOpenSubKeys(ptr ptr)
@ stdcall ZwQueryOpenSubKeysEx(ptr long ptr ptr)
@ stdcall ZwQueryPerformanceCounter (long long)
@ stdcall ZwQueryPortInformationProcess()
@ stdcall ZwQueryQuotaInformationFile(ptr ptr ptr long long ptr long ptr long)
@ stdcall ZwQuerySection (long long long long long)
@ stdcall ZwQuerySecurityObject (long long long long long)
@ stdcall ZwQuerySemaphore (long long long long long)
@ stdcall ZwQuerySymbolicLinkObject(long ptr ptr)
@ stdcall ZwQuerySystemEnvironmentValue(ptr ptr long ptr)
@ stdcall ZwQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
@ stdcall ZwQuerySystemInformation(long long long long)
@ stdcall ZwQuerySystemTime(ptr)
@ stdcall ZwQueryTimer(ptr long ptr long ptr)
@ stdcall ZwQueryTimerResolution(long long long)
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr)
@ stdcall ZwQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall ZwQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall ZwQueueApcThread(long ptr long long long)
@ stdcall ZwRaiseException(ptr ptr long)
@ stdcall ZwRaiseHardError(long long long ptr long ptr)
@ stdcall ZwReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall ZwReadFileScatter(long long ptr ptr ptr ptr long ptr ptr)
@ stub -version=0x600+ ZwReadOnlyEnlistment
@ stdcall ZwReadRequestData(ptr ptr long ptr long ptr)
@ stdcall ZwReadVirtualMemory(long ptr ptr long ptr)
@ stub -version=0x600+ ZwRecoverEnlistment
@ stub -version=0x600+ ZwRecoverResourceManager
@ stub -version=0x600+ ZwRecoverTransactionManager
@ stub -version=0x600+ ZwRegisterProtocolAddressInformation
@ stdcall ZwRegisterThreadTerminatePort(ptr)
@ stub -version=0x600+ ZwReleaseCMFViewOwnership
@ stdcall ZwReleaseKeyedEvent(ptr ptr long ptr)
@ stdcall ZwReleaseMutant(long ptr)
@ stdcall ZwReleaseSemaphore(long long ptr)
@ stub -version=0x600+ ZwReleaseWorkerFactoryWorker
@ stdcall ZwRemoveIoCompletion(ptr ptr ptr ptr ptr)
@ stub -version=0x600+ ZwRemoveIoCompletionEx
@ stdcall ZwRemoveProcessDebug(ptr ptr)
@ stdcall ZwRenameKey(ptr ptr)
@ stub -version=0x600+ ZwRenameTransactionManager
@ stdcall ZwReplaceKey(ptr long ptr)
@ stub -version=0x600+ ZwReplacePartitionUnit
@ stdcall ZwReplyPort(ptr ptr)
@ stdcall ZwReplyWaitReceivePort(ptr ptr ptr ptr)
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
@ stub -version=0x600+ ZwRollbackComplete
@ stub -version=0x600+ ZwRollbackEnlistment
@ stub -version=0x600+ ZwRollbackTransaction
@ stub -version=0x600+ ZwRollforwardTransactionManager
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
@ stdcall ZwSetDriverEntryOrder(ptr ptr)
@ stdcall ZwSetEaFile(long ptr ptr long)
@ stdcall ZwSetEvent(long long)
@ stdcall ZwSetEventBoostPriority(ptr)
@ stdcall ZwSetHighEventPair(ptr)
@ stdcall ZwSetHighWaitLowEventPair(ptr)
@ stdcall ZwSetInformationDebugObject(ptr long ptr long ptr)
@ stub -version=0x600+ ZwSetInformationEnlistment
@ stdcall ZwSetInformationFile(long long long long long)
@ stdcall ZwSetInformationJobObject(long long ptr long)
@ stdcall ZwSetInformationKey(long long ptr long)
@ stdcall ZwSetInformationObject(long long ptr long)
@ stdcall ZwSetInformationProcess(long long long long)
@ stub -version=0x600+ ZwSetInformationResourceManager
@ stdcall ZwSetInformationThread(long long ptr long)
@ stdcall ZwSetInformationToken(long long ptr long)
@ stub -version=0x600+ ZwSetInformationTransaction
@ stub -version=0x600+ ZwSetInformationTransactionManager
@ stub -version=0x600+ ZwSetInformationWorkerFactory
@ stdcall ZwSetIntervalProfile(long long)
@ stdcall ZwSetIoCompletion(ptr long ptr long long)
@ stdcall ZwSetLdtEntries(long int64 long int64)
@ stdcall ZwSetLowEventPair(ptr)
@ stdcall ZwSetLowWaitHighEventPair(ptr)
@ stdcall ZwSetQuotaInformationFile(ptr ptr ptr long)
@ stdcall ZwSetSecurityObject(long long ptr)
@ stdcall ZwSetSystemEnvironmentValue(ptr ptr)
@ stdcall ZwSetSystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
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
@ stub -version=0x600+ ZwShutdownWorkerFactory
@ stdcall ZwSignalAndWaitForSingleObject(long long long ptr)
@ stub -version=0x600+ ZwSinglePhaseReject
@ stdcall ZwStartProfile(ptr)
@ stdcall ZwStopProfile(ptr)
@ stdcall ZwSuspendProcess(ptr)
@ stdcall ZwSuspendThread(long ptr)
@ stdcall ZwSystemDebugControl(long ptr long ptr long ptr)
@ stdcall ZwTerminateJobObject(ptr long)
@ stdcall ZwTerminateProcess(ptr long)
@ stdcall ZwTerminateThread(ptr long)
@ stdcall ZwTestAlert()
@ stub -version=0x600+ ZwThawRegistry
@ stub -version=0x600+ ZwThawTransactions
@ stub -version=0x600+ ZwTraceControl
@ stdcall ZwTraceEvent(long long long ptr)
@ stdcall ZwTranslateFilePath(ptr long ptr long)
@ stdcall ZwUnloadDriver(ptr)
@ stdcall ZwUnloadKey2(ptr long)
@ stdcall ZwUnloadKey(long)
@ stdcall ZwUnloadKeyEx(ptr ptr)
@ stdcall ZwUnlockFile(long ptr ptr ptr ptr)
@ stdcall ZwUnlockVirtualMemory(long ptr ptr long)
@ stdcall ZwUnmapViewOfSection(long ptr)
@ stdcall ZwVdmControl(long ptr)
@ stdcall ZwWaitForDebugEvent(ptr long ptr ptr)
@ stdcall ZwWaitForKeyedEvent(ptr ptr long ptr)
@ stdcall ZwWaitForMultipleObjects32(long ptr long long ptr)
@ stdcall ZwWaitForMultipleObjects(long ptr long long ptr)
@ stdcall ZwWaitForSingleObject(long long long)
@ stub -version=0x600+ ZwWaitForWorkViaWorkerFactory
@ stdcall ZwWaitHighEventPair(ptr)
@ stdcall ZwWaitLowEventPair(ptr)
@ stub -version=0x600+ ZwWorkerFactoryWorkerReady
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall ZwWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall ZwWriteRequestData(ptr ptr long ptr long ptr)
@ stdcall ZwWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall ZwYieldExecution()
@ cdecl -arch=i386 _CIcos()
@ cdecl -arch=i386 _CIlog()
@ cdecl -arch=i386 _CIpow()
@ cdecl -arch=i386 _CIsin()
@ cdecl -arch=i386 _CIsqrt()
@ cdecl -arch=x86_64,arm __C_specific_handler(ptr long ptr ptr)
@ cdecl -arch=x86_64,arm __chkstk()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ cdecl -arch=arm __jump_unwind()
@ cdecl -stub -version=0x600+ -arch=x86_64 __misaligned_access()
@ cdecl __toascii(long)
@ cdecl -arch=i386 -ret64 _alldiv(double double)
@ cdecl -arch=i386 _alldvrm()
@ cdecl -arch=i386 -ret64 _allmul(double double)
@ cdecl -arch=i386 -norelay _alloca_probe()
@ cdecl -version=0x600+ -arch=i386 _alloca_probe_16()
@ stub -version=0x600+ -arch=i386 _alloca_probe_8
@ cdecl -arch=i386 -ret64 _allrem(double double)
@ cdecl -arch=i386 _allshl()
@ cdecl -arch=i386 _allshr()
@ cdecl -ret64 _atoi64(str)
@ cdecl -arch=i386 -ret64 _aulldiv(double double)
@ cdecl -arch=i386 _aulldvrm()
@ cdecl -arch=i386 -ret64 _aullrem(double double)
@ cdecl -arch=i386 _aullshr()
@ extern -arch=i386 _chkstk
@ cdecl -arch=i386,x86_64,arm _fltused()
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
@ cdecl -arch=x86_64,arm _setjmp(ptr ptr)
@ cdecl -arch=x86_64,arm _setjmpex(ptr ptr)
@ varargs _snprintf(ptr long str)
@ varargs _snwprintf(ptr long wstr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _stricmp(str str)
@ cdecl _strlwr(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strupr(str)
@ stub -version=0x600+ _swprintf
@ cdecl -version=0x502 _tolower(long)
@ cdecl -version=0x502 _toupper(long)
@ cdecl _ui64toa(double ptr long)
@ cdecl _ui64tow(double ptr long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
@ cdecl _vscwprintf(wstr ptr)
@ cdecl _vsnprintf(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ stub -version=0x600+ _vswprintf
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcstoui64(wstr ptr long)
@ cdecl _wcsupr(wstr)
@ cdecl _wtoi(wstr)
@ cdecl _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl abs(long)
@ cdecl -arch=i386,x86_64 atan(double)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl ceil(double)
@ cdecl cos(double)
@ cdecl fabs(double)
@ cdecl floor(double)
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
@ cdecl -arch=i386,x86_64,arm pow(double double)
@ cdecl qsort(ptr long long ptr)
@ cdecl sin(double)
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
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)

# FIXME: check if this is correct
@ stdcall -arch=arm __dtoi64()
@ stdcall -arch=arm __dtou64()
@ stdcall -arch=arm __i64tod()
@ stdcall -arch=arm __u64tod()
@ stdcall -arch=arm __rt_sdiv()
@ stdcall -arch=arm __rt_sdiv64()
@ stdcall -arch=arm __rt_udiv()
@ stdcall -arch=arm __rt_udiv64()
@ stdcall -arch=arm __rt_srsh()
