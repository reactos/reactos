SVC_(MapUserPhysicalPagesScatter, 3)
SVC_(WaitForSingleObject, 4)
SVC_(CallbackReturn, 3)
SVC_(ReadFile, 9)
SVC_(DeviceIoControlFile, 10)
SVC_(WriteFile, 9)
SVC_(RemoveIoCompletion, 5)
SVC_(ReleaseSemaphore, 3)
SVC_(ReplyWaitReceivePort, 4)
SVC_(ReplyPort, 2)
SVC_(SetInformationThread, 4)
SVC_(SetEvent, 2)
SVC_(Close, 1)
SVC_(QueryObject, 5)
SVC_(QueryInformationFile, 5)
SVC_(OpenKey, 3)
SVC_(EnumerateValueKey, 6)
SVC_(FindAtom, 3)
SVC_(QueryDefaultLocale, 2)
SVC_(QueryKey, 5)
SVC_(QueryValueKey, 6)
SVC_(AllocateVirtualMemory, 6)
SVC_(QueryInformationProcess, 5)
SVC_(WaitForMultipleObjects32, 5)
SVC_(WriteFileGather, 9)
SVC_(SetInformationProcess, 4)
SVC_(CreateKey, 7)
SVC_(FreeVirtualMemory, 4)
SVC_(ImpersonateClientOfPort, 2)
SVC_(ReleaseMutant, 2)
SVC_(QueryInformationToken, 5)
SVC_(RequestWaitReplyPort, 3)
SVC_(QueryVirtualMemory, 6)
SVC_(OpenThreadToken, 4)
SVC_(QueryInformationThread
SVC_(OpenProcess, 5)
SVC_(SetInformationFile, 5)
SVC_(MapViewOfSection, 10)
SVC_(AccessCheckAndAuditAlarm, 11)
SVC_(UnmapViewOfSection, 2)
SVC_(ReplyWaitReceivePortEx, 5)
SVC_(TerminateProcess, 2)
SVC_(SetEventBoostPriority, 1)
SVC_(ReadFileScatter, 9)
SVC_(OpenThreadTokenEx, 5)
SVC_(OpenProcessTokenEx, 4)
SVC_(QueryPerformanceCounter
SVC_(EnumerateKey, 2)
SVC_(OpenFile, 6)
SVC_(DelayExecution, 2)
SVC_(QueryDirectoryFile, 11)
SVC_(QuerySystemInformation, 4)
SVC_(OpenSection, 3)
SVC_(QueryTimer, 5)
SVC_(FsControlFile, 10)
SVC_(WriteVirtualMemory, 5)
SVC_(CloseObjectAuditAlarm, 3)
SVC_(DuplicateObject, 7)
SVC_(QueryAttributesFile, 2)
SVC_(ClearEvent, 1)
SVC_(ReadVirtualMemory, 5)
SVC_(OpenEvent, 3)
SVC_(AdjustPrivilegesToken, 6)
SVC_(DuplicateToken, 6)
SVC_(Continue, 2)
SVC_(QueryDefaultUILanguage, 1)
SVC_(QueueApcThread, 5)
SVC_(YieldExecution, 0)
SVC_(AddAtom, 3)
SVC_(CreateEvent, 5)
SVC_(QueryVolumeInformationFile, 5)
SVC_(CreateSection, 7)
SVC_(FlushBuffersFile, 2)
SVC2_(ApphelpCacheControl, 2)
SVC_(CreateProcessEx, 9)
SVC_(CreateThread, 8)
SVC_(IsProcessInJob, 2)
SVC_(ProtectVirtualMemory, 5)
SVC_(QuerySection, 5)
SVC_(ResumeThread, 2)
SVC_(TerminateThread, 2)
SVC_(ReadRequestData, 6)
SVC_(CreateFile, 11)
SVC_(QueryEvent, 5)
SVC_(WriteRequestData, 6)
SVC_(OpenDirectoryObject, 3)
SVC_(AccessCheckByTypeAndAuditAlarm, 16)
SVC_(QuerySystemTime, 1)
SVC_(WaitForMultipleObjects, 5)
SVC_(SetInformationObject, 4)
SVC_(CancelIoFile, 2)
SVC_(TraceEvent, 4)
SVC_(PowerInformation, 5)
SVC_(SetValueKey, 6)
SVC_(CancelTimer, 2)
SVC_(SetTimer, 7)
SVC_(AcceptConnectPort, 6)
SVC_(AccessCheck, 8)
SVC_(AccessCheckByType, 11)
SVC_(AccessCheckByTypeResultList, 11)
SVC_(AccessCheckByTypeResultListAndAuditAlarm, 16)
SVC_(AccessCheckByTypeResultListAndAuditAlarmByHandle, 17)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(AddAtomEx, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
#if (NTDDI_VERSION < NTDDI_WIN7)
SVC_(AcquireCMFViewOwnership, 3)
#endif
SVC_(AddBootEntry, 2)
SVC_(AddDriverEntry, 2)
#endif
SVC_(AdjustGroupsToken, 6)
SVC_(AlertResumeThread, 2)
SVC_(AlertThread, 1)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(AlertThreadByThreadId, 0) // FIXME
#endif
SVC_(AllocateLocallyUniqueId, 1)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(AllocateReserveObject, 3)
#endif
SVC_(AllocateUserPhysicalPages, 3)
SVC_(AllocateUuids, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(AlpcAcceptConnectPort, 9)
SVC_(AlpcCancelMessage, 3)
SVC_(AlpcConnectPort, 11)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(AlpcConnectPortEx, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(AlpcCreatePort, 3)
SVC_(AlpcCreatePortSection, 6)
SVC_(AlpcCreateResourceReserve, 4)
SVC_(AlpcCreateSectionView, 3)
SVC_(AlpcCreateSecurityContext, 3)
SVC_(AlpcDeletePortSection,3)
SVC_(AlpcDeleteResourceReserve, 3)
SVC_(AlpcDeleteSectionView, 3)
SVC_(AlpcDeleteSecurityContext, 3)
SVC_(AlpcDisconnectPort, 2)
SVC_(AlpcImpersonateClientOfPort, 3)
SVC_(AlpcOpenSenderProcess, 6)
SVC_(AlpcOpenSenderThread, 6)
SVC_(AlpcQueryInformation, 5)
SVC_(AlpcQueryInformationMessage, 6)
SVC_(AlpcRevokeSecurityContext, 3)
SVC_(AlpcSendWaitReceivePort, 8)
SVC_(AlpcSetInformation, 4)
#endif // (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(AreMappedFilesTheSame, 2)
SVC_(AssignProcessToJobObject, 2)
#if (NTDDI_VERSION >= NTDDI_SERVER08 && NTDDI_VERSION < NTDDI_WIN7)
SVC_(RequestDeviceWakeup, 0) // FIXME
#endif
#if (NTDDI_VERSION == NTDDI_VISTA)
SVC_(SavepointTransaction, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(AssociateWaitCompletionPacket, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_VISTASP1 && NTDDI_VERSION <= NTDDI_VISTASP2)
SVC2_(xHalPostMicrocodeUpdate, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CancelIoFileEx, 3)
SVC_(CancelSynchronousIoFile, 3)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CancelWaitCompletionPacket, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CommitComplete, 2)
SVC_(CommitEnlistment, 2)
SVC_(CommitTransaction, 2)
#endif // (NTDDI_VERSION == NTDDI_VISTA)
SVC_(CompactKeys, 2)
SVC_(CompareTokens, 3)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC2_(xHalGetInterruptTranslator, 7)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTASP1 && NTDDI_VERSION <= NTDDI_VISTASP2)
SVC2_(ArbPreprocessEntry, 0) // FIXME
#elif (NTDDI_VERSION < NTDDI_WIN7)
SVC_(CompleteConnectPort, 1)
#endif
SVC_(CompressKey, 1)
SVC_(ConnectPort, 8)
SVC_(CreateDebugObject, 4)
SVC_(CreateDirectoryObject, 3)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateDirectoryObjectEx, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateEnlistment
#endif // (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateEventPair, 3)
SVC_(CreateIoCompletion, 4)
SVC_(CreateJobObject, 3)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC2_(xHalAllocatePmcCounterSet, 0) // FIXME
#else
SVC_(CreateJobSet, 3)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateKeyTransacted, 8)
#endif
SVC_(CreateKeyedEvent, 4)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateLowBoxToken, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateMailslotFile, 8)
SVC_(CreateMutant, 4)
SVC_(CreateNamedPipeFile, 14)
SVC_(CreatePagingFile 4)
SVC_(CreatePort, 5)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreatePrivateNamespace, 4)
#endif // (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateProcess, 8)
SVC_(CreateProfile, 9)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(CreateProfileEx, 10)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateResourceManager, 7)
#endif
SVC_(CreateSemaphore, 5)
SVC_(CreateSymbolicLinkObject, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateThreadEx, 11)
#endif
SVC_(CreateTimer, 4)
SVC_(CreateToken, 13)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateTokenEx, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateTransaction, 10)
SVC_(CreateTransactionManager, 6)
SVC_(CreateUserProcess, 11)
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateWaitCompletionPacket, 0) // FIXME
#endif
SVC_(CreateWaitablePort, 5)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(CreateWnfStateName, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(CreateWorkerFactory, 10)
#endif
SVC_(DebugActiveProcess, 2)
SVC_(DebugContinue, 3)
SVC_(DeleteAtom, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(DeleteBootEntry, 1)
SVC_(DeleteDriverEntry, 1)
#endif
SVC_(DeleteFile, 1)
SVC_(DeleteKey, 1)
SVC_(DeleteObjectAuditAlarm, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(DeletePrivateNamespace, 1)
#endif
SVC_(DeleteValueKey, 2)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(DeleteWnfStateData, 0) // FIXME
SVC_(DeleteWnfStateName, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(DisableLastKnownGood, 0)
#endif
SVC_(DisplayString, 1)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(DrawText, 1)
SVC_(EnableLastKnownGood, 0)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(EnumerateBootEntries, 2)
SVC_(EnumerateDriverEntries, 2)
SVC_(EnumerateSystemEnvironmentValuesEx, 3)
SVC_(EnumerateTransactionObject, 5)
#endif
SVC_(ExtendSection, 2)
SVC_(FilterToken, 6)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(FlushBuffersFileEx, 3)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(FlushInstallUILanguage, 2)
#endif
#if (NTDDI_VERSION < NTDDI_WIN8)
SVC_(FlushInstructionCache, 3)
#endif
SVC_(FlushKey, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(FlushProcessWriteBuffers, 0)
#endif
SVC_(FlushVirtualMemory, 4)
SVC_(FlushWriteBuffer, 0)
SVC_(FreeUserPhysicalPages, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(FreezeRegistry
SVC_(FreezeTransactions, 2)
#endif
SVC_(GetContextThread, 2)
SVC_(GetCurrentProcessorNumber, 0)
SVC_(GetDevicePowerState
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(GetMUIRegistryInfo, 3)
SVC_(GetNextProcess, 5)
SVC_(GetNextThread, 6)
SVC_(GetNlsSectionPtr, 5)
SVC_(GetNotificationResourceManager, 7)
#endif
#if (NTDDI_VERSION < NTDDI_WIN8)
SVC_(GetPlugPlayEvent, 4)
#endif
SVC_(GetWriteWatch, 7)
SVC_(ImpersonateAnonymousToken, 1)
SVC_(ImpersonateThread, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(InitializeNlsFiles
#endif
SVC_(InitializeRegistry, 1)
SVC_(InitiatePowerAction, 4)
SVC_(IsSystemResumeAutomatic, 0)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(IsUILanguageComitted, 0)
#endif
SVC_(ListenPort, 2)
SVC_(LoadDriver, 1)
SVC_(LoadKey, 2)
SVC_(LoadKey2, 3)
SVC_(LoadKeyEx, 4)
SVC_(LockFile, 10)
SVC_(LockProductActivationKeys, 2)
SVC_(LockRegistryKey, 1)
SVC_(LockVirtualMemory, 4)
SVC_(MakePermanentObject, 1)
SVC_(MakeTemporaryObject, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(MapCMFModule
#endif
SVC_(MapUserPhysicalPages, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(ModifyBootEntry, 1)
SVC_(ModifyDriverEntry, 1)
#endif
SVC_(NotifyChangeDirectoryFile, 9)
SVC_(NotifyChangeKey, 10)
SVC_(NotifyChangeMultipleKeys, 12)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(NotifyChangeSession, 8)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenEnlistment, 5)
#endif
SVC_(OpenEventPair, 3)
SVC_(OpenIoCompletion, 3)
SVC_(OpenJobObject, 3)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(OpenKeyEx, 4)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenKeyTransacted, 4)
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(OpenKeyTransactedEx, 5)
#endif
SVC_(OpenKeyedEvent, 3)
SVC_(OpenMutant, 3)
SVC_(OpenObjectAuditAlarm, 12)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenPrivateNamespace, 4)
#endif
SVC_(OpenProcessToken, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenResourceManager, 5)
#endif
SVC_(OpenSemaphore, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenSession, 3)
#endif
SVC_(OpenSymbolicLinkObject, 3)
SVC_(OpenThread, 4)
SVC_(OpenTimer, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(OpenTransaction, 5)
SVC_(OpenTransactionManager, 6)
#endif
SVC_(PlugPlayControl, 3)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(PrePrepareComplete, 2)
SVC_(PrePrepareEnlistment, 2)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(PrefetchVirtualMemory, 0) // FIXME
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(PrepareComplete, 2)
SVC_(PrepareEnlistment, 2)
#endif // (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(PrivilegeCheck, 3)
SVC_(PrivilegeObjectAuditAlarm, 6)
SVC_(PrivilegedServiceAuditAlarm, 5)
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
SVC_(PropagationComplete, 4)
SVC_(PropagationFailed, 3)
#endif
SVC_(PulseEvent, 2)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryBootEntryOrder, 2)
#endif
SVC_(QueryBootOptions, 2)
SVC_(QueryDebugFilterState, 2)
SVC_(QueryDirectoryObject, 7)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryDriverEntryOrder, 2)
#endif
SVC_(QueryEaFile, 9)
SVC_(QueryFullAttributesFile, 2)
SVC_(QueryInformationAtom, 5)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryInformationEnlistment, 5)
#endif
SVC_(QueryInformationJobObject, 5)
SVC_(QueryInformationPort, 5)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryInformationResourceManager, 5)
SVC_(QueryInformationTransaction, 5)
SVC_(QueryInformationTransactionManager, 5)
SVC_(QueryInformationWorkerFactory, 5)
#endif
SVC_(QueryInstallUILanguage, 1)
SVC_(QueryIntervalProfile, 2)
SVC_(QueryIoCompletion, 5)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryLicenseValue, 5)
#endif
SVC_(QueryMultipleValueKey, 6)
SVC_(QueryMutant, 5)
SVC_(QueryOpenSubKeys, 2)
SVC_(QueryOpenSubKeysEx, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QueryPortInformationProcess, 0)
#endif
#if (NTDDI_VERSION < NTDDI_VISTA)
SVC2_(ShimExceptionHandler, 0) // FIXME
#endif
SVC_(QueryQuotaInformationFile, 9)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(QuerySecurityAttributesToken, 6)
#endif
SVC_(QuerySecurityObject, 5)
SVC_(QuerySemaphore, 5)
SVC_(QuerySymbolicLinkObject, 3)
SVC_(QuerySystemEnvironmentValue, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(QuerySystemEnvironmentValueEx, 5)
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(QuerySystemInformationEx, 6)
#endif
SVC_(QueryTimerResolution, 3)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(QueryWnfStateData, 0) // FIXME
SVC_(QueryWnfStateNameInformation, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(QueueApcThreadEx, 6)
#endif
SVC_(RaiseException, 3)
SVC_(RaiseHardError, 6)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(ReadOnlyEnlistment, 2)
SVC_(RecoverEnlistment, 2)
SVC_(RecoverResourceManager, 1)
SVC_(RecoverTransactionManager, 1)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
SVC_(RegisterProtocolAddressInformation, 5)
#endif
#if (NTDDI_VERSION == NTDDI_VISTA)
SVC_(KeRestoreFloatingPointState, 0) // FIXME
#endif
SVC_(RegisterThreadTerminatePort, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(ReleaseCMFViewOwnership, 0)
#endif
SVC_(ReleaseKeyedEvent, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(ReleaseWorkerFactoryWorker, 1)
SVC_(RemoveIoCompletionEx, 6)
#endif
SVC_(RemoveProcessDebug, 2)
SVC_(RenameKey, 2)
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
SVC_(RenameTransactionManager, 2)
#endif
SVC_(ReplaceKey, 3)
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
SVC_(ReplacePartitionUnit, 3)
#endif
SVC_(ReplyWaitReplyPort, 2)
SVC_(RequestPort, 2)
#if (NTDDI_VERSION < NTDDI_WIN7)
SVC_(RequestWakeupLatency, 1)
#endif
SVC_(ResetEvent, 2)
SVC_(ResetWriteWatch, 3)
SVC_(RestoreKey, 3)
SVC_(ResumeProcess, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(RollbackComplete, 2)
SVC_(RollbackEnlistment, 2)
SVC_(RollbackTransaction, 2)
SVC_(RollforwardTransactionManager, 2)
#endif
SVC_(SaveKey, 2)
SVC_(SaveKeyEx, 3)
SVC_(SaveMergedKeys, 3)
SVC_(SecureConnectPort, 9)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(SerializeBoot, 0)
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetBootEntryOrder, 2)
SVC_(SetBootOptions, 2)
#endif
SVC_(SetContextThread, 2)
SVC_(SetDebugFilterState, 3)
SVC_(SetDefaultHardErrorPort, 1)
SVC_(SetDefaultLocale, 2)
SVC_(SetDefaultUILanguage, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetDriverEntryOrder, 2)
#endif
SVC_(SetEaFile, 4)
SVC_(SetHighEventPair, 1)
SVC_(SetHighWaitLowEventPair, 1)
SVC_(SetInformationDebugObject, 5)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetInformationEnlistment, 4)
#endif
SVC_(SetInformationJobObject, 4)
SVC_(SetInformationKey, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetInformationResourceManager, 4)
#endif
SVC_(SetInformationToken, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetInformationTransaction, 4)
SVC_(SetInformationTransactionManager, 4)
SVC_(SetInformationWorkerFactory, 4)
#endif
SVC_(SetIntervalProfile, 2)
SVC_(SetIoCompletion, 5)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(SetIoCompletionEx, 6)
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC2_(xKdReleaseIntegratedDeviceForDebugging, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_SERVER08 && NTDDI_VERSION < NTDDI_WIN7)
SVC_(SetLdtEntries, 6)
#elif (NTDDI_VERSION < NTDDI_WIN8)
SVC2_(xKdSetupPciDeviceForDebugging, 2)
#endif
SVC_(SetLowEventPair, 1)
SVC_(SetLowWaitHighEventPair, 1)
SVC_(SetQuotaInformationFile, 4)
SVC_(SetSecurityObject, 3)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(SetSystemCodeIntegrityRoots, 0) // FIXME
#endif
SVC_(SetSystemEnvironmentValue, 2)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SetSystemEnvironmentValueEx, 5)
#endif
SVC_(SetSystemInformation, 3)
SVC_(SetSystemPowerState, 3)
SVC_(SetSystemTime, 2)
SVC_(SetThreadExecutionState, 2)
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(SetTimerEx
#endif
SVC_(SetTimerResolution, 3)
SVC_(SetUuidSeed, 1)
SVC_(SetVolumeInformationFile, 5)
SVC_(ShutdownSystem, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(ShutdownWorkerFactory, 2)
#endif
SVC_(SignalAndWaitForSingleObject, 4)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(SinglePhaseReject, 2)
#endif
SVC_(StartProfile, 1)
#if (NTDDI_VERSION == NTDDI_VISTA)
SVC2_(xHalGetInterruptTranslator, 0) // FIXME
#endif
SVC_(StopProfile, 1)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(SubscribeWnfStateChange, 0) // FIXME
#endif
SVC_(SuspendProcess, 1)
SVC_(SuspendThread, 2)
SVC_(SystemDebugControl, 6)
SVC_(TerminateJobObject, 2)
SVC_(TestAlert, 0)
#if (NTDDI_VERSION == NTDDI_VISTA)
SVC_(ThawRegistry, 0)
SVC_(ThawTransactions, 0)
SVC_(TraceControl, 6)
SVC_(TranslateFilePath, 4)
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
SVC_(UmsThreadYield, 1)
#endif
SVC_(UnloadDriver, 1)
SVC_(UnloadKey, 1)
SVC_(UnloadKey2, 2)
SVC_(UnloadKeyEx, 2)
SVC_(UnlockFile, 5)
SVC_(UnlockVirtualMemory, 4)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(UnmapViewOfSectionEx, 0) // FIXME
SVC_(UnsubscribeWnfStateChange, 0) // FIXME
SVC_(UpdateWnfStateData, 0) // FIXME
SVC_(WaitForAlertByThreadId, 0) // FIXME
#endif
SVC_(VdmControl, 2)
SVC_(WaitForDebugEvent, 4)
SVC_(WaitForKeyedEvent, 4)
#if (NTDDI_VERSION >= NTDDI_WIN8)
SVC_(WaitForWnfNotifications, 0) // FIXME
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(WaitForWorkViaWorkerFactory, 2)
#endif
SVC_(WaitHighEventPair, 1)
SVC_(WaitLowEventPair, 1)
#if (NTDDI_VERSION >= NTDDI_VISTA)
SVC_(WorkerFactoryWorkerReady, 1)
#endif


