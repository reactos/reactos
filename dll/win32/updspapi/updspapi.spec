@ stdcall UpdSpCloseFileQueue(ptr) setupapi.SetupCloseFileQueue
@ stdcall UpdSpCloseInfFile(long) setupapi.SetupCloseInfFile
@ stdcall UpdSpCommitFileQueueA(long long ptr ptr) setupapi.SetupCommitFileQueueA
@ stdcall UpdSpCommitFileQueueW(long long ptr ptr) setupapi.SetupCommitFileQueueW
@ stdcall UpdSpCopyErrorA(long str str str str str long long str long ptr) setupapi.SetupCopyErrorA
@ stdcall UpdSpCopyErrorW(long wstr wstr wstr wstr wstr long long wstr long ptr) setupapi.SetupCopyErrorW
@ stdcall UpdSpDecompressOrCopyFileA(str str ptr) setupapi.SetupDecompressOrCopyFileA
@ stdcall UpdSpDecompressOrCopyFileW(wstr wstr ptr) setupapi.SetupDecompressOrCopyFileW
@ stdcall UpdSpDefaultQueueCallbackA(ptr long long long) setupapi.SetupDefaultQueueCallbackA
@ stdcall UpdSpDefaultQueueCallbackW(ptr long long long) setupapi.SetupDefaultQueueCallbackW
@ stdcall UpdSpDeleteErrorA(long str str long long) setupapi.SetupDeleteErrorA
@ stdcall UpdSpDeleteErrorW(long wstr wstr long long) setupapi.SetupDeleteErrorW
@ stdcall UpdSpEnumInfSectionsA(long long ptr long ptr) setupapi.SetupEnumInfSectionsA
@ stdcall UpdSpEnumInfSectionsW(long long ptr long ptr) setupapi.SetupEnumInfSectionsW
@ stdcall UpdSpFindFirstLineA(long str str ptr) setupapi.SetupFindFirstLineA
@ stdcall UpdSpFindFirstLineW(long wstr wstr ptr) setupapi.SetupFindFirstLineW
@ stdcall UpdSpFindNextLine(ptr ptr) setupapi.SetupFindNextLine
@ stdcall UpdSpFindNextMatchLineA(ptr str ptr) setupapi.SetupFindNextMatchLineA
@ stdcall UpdSpFindNextMatchLineW(ptr wstr ptr) setupapi.SetupFindNextMatchLineW
@ stdcall UpdSpGetBinaryField(ptr long ptr long ptr) setupapi.SetupGetBinaryField
@ stdcall UpdSpGetFieldCount(ptr) setupapi.SetupGetFieldCount
@ stdcall UpdSpGetIntField(ptr long ptr) setupapi.SetupGetIntField
@ stdcall UpdSpGetLineByIndexA(long str long ptr) setupapi.SetupGetLineByIndexA
@ stdcall UpdSpGetLineByIndexW(long wstr long ptr) setupapi.SetupGetLineByIndexW
@ stdcall UpdSpGetLineCountA(long str) setupapi.SetupGetLineCountA
@ stdcall UpdSpGetLineCountW(long wstr) setupapi.SetupGetLineCountW
@ stdcall UpdSpGetLineTextA(ptr long str str ptr long ptr) setupapi.SetupGetLineTextA
@ stdcall UpdSpGetLineTextW(ptr long wstr wstr ptr long ptr) setupapi.SetupGetLineTextW
@ stdcall UpdSpGetMultiSzFieldA(ptr long ptr long ptr) setupapi.SetupGetMultiSzFieldA
@ stdcall UpdSpGetMultiSzFieldW(ptr long ptr long ptr) setupapi.SetupGetMultiSzFieldW
@ stdcall UpdSpGetSourceFileLocationA(ptr ptr str ptr ptr long ptr) setupapi.SetupGetSourceFileLocationA
@ stdcall UpdSpGetSourceFileLocationW(ptr ptr wstr ptr ptr long ptr) setupapi.SetupGetSourceFileLocationW
@ stdcall UpdSpGetSourceInfoA(ptr long long ptr long ptr) setupapi.SetupGetSourceInfoA
@ stdcall UpdSpGetSourceInfoW(ptr long long ptr long ptr) setupapi.SetupGetSourceInfoW
@ stdcall UpdSpGetStringFieldA(ptr long ptr long ptr) setupapi.SetupGetStringFieldA
@ stdcall UpdSpGetStringFieldW(ptr long ptr long ptr) setupapi.SetupGetStringFieldW
@ stdcall UpdSpGetTargetPathA(ptr ptr str ptr long ptr) setupapi.SetupGetTargetPathA
@ stdcall UpdSpGetTargetPathW(ptr ptr wstr ptr long ptr) setupapi.SetupGetTargetPathW
@ stdcall UpdSpInitDefaultQueueCallback(long) setupapi.SetupInitDefaultQueueCallback
@ stdcall UpdSpInitDefaultQueueCallbackEx(long long long long ptr) setupapi.SetupInitDefaultQueueCallbackEx
@ stdcall UpdSpInstallFilesFromInfSectionA(long long long str str long) setupapi.SetupInstallFilesFromInfSectionA
@ stdcall UpdSpInstallFilesFromInfSectionW(long long long wstr wstr long) setupapi.SetupInstallFilesFromInfSectionW
@ stdcall UpdSpInstallFromInfSectionA(long long str long long str long ptr ptr long ptr) setupapi.SetupInstallFromInfSectionA
@ stdcall UpdSpInstallFromInfSectionW(long long wstr long long wstr long ptr ptr long ptr) setupapi.SetupInstallFromInfSectionW
@ stdcall UpdSpIterateCabinetA(str long ptr ptr) setupapi.SetupIterateCabinetA
@ stdcall UpdSpIterateCabinetW(wstr long ptr ptr) setupapi.SetupIterateCabinetW
@ stdcall UpdSpOpenAppendInfFileA(str long ptr) setupapi.SetupOpenAppendInfFileA
@ stdcall UpdSpOpenAppendInfFileW(wstr long ptr) setupapi.SetupOpenAppendInfFileW
@ stdcall UpdSpOpenFileQueue() setupapi.SetupOpenFileQueue
@ stdcall UpdSpOpenInfFileA(str str long ptr) setupapi.SetupOpenInfFileA
@ stdcall UpdSpOpenInfFileW(wstr wstr long ptr) setupapi.SetupOpenInfFileW
@ stdcall UpdSpPromptForDiskA(ptr str str str str str long ptr long ptr) setupapi.SetupPromptForDiskA
@ stdcall UpdSpPromptForDiskW(ptr wstr wstr wstr wstr wstr long ptr long ptr) setupapi.SetupPromptForDiskW
@ stdcall UpdSpQueueCopyA(long str str str str str str str long) setupapi.SetupQueueCopyA
@ stdcall UpdSpQueueCopySectionA(long str long long str long) setupapi.SetupQueueCopySectionA
@ stdcall UpdSpQueueCopySectionW(long wstr long long wstr long) setupapi.SetupQueueCopySectionW
@ stdcall UpdSpQueueCopyW(long wstr wstr wstr wstr wstr wstr wstr long) setupapi.SetupQueueCopyW
@ stdcall UpdSpQueueDeleteA(long str str) setupapi.SetupQueueDeleteA
@ stdcall UpdSpQueueDeleteSectionA(long long long str) setupapi.SetupQueueDeleteSectionA
@ stdcall UpdSpQueueDeleteSectionW(long long long wstr) setupapi.SetupQueueDeleteSectionW
@ stdcall UpdSpQueueDeleteW(long wstr wstr) setupapi.SetupQueueDeleteW
@ stdcall UpdSpScanFileQueueA(long long long ptr ptr ptr) setupapi.SetupScanFileQueueA
@ stdcall UpdSpScanFileQueueW(long long long ptr ptr ptr) setupapi.SetupScanFileQueueW
@ stdcall UpdSpSetDirectoryIdA(long long str) setupapi.SetupSetDirectoryIdA
@ stdcall UpdSpSetDirectoryIdW(long long wstr) setupapi.SetupSetDirectoryIdW
@ stdcall UpdSpSetDynamicStringA(ptr str str)
@ stub UpdSpSetDynamicStringExA
@ stub UpdSpSetDynamicStringExW
@ stub UpdSpSetDynamicStringW
@ stdcall UpdSpStringTableAddString(ptr wstr long) setupapi.StringTableAddString
@ stdcall UpdSpStringTableAddStringEx(ptr wstr long ptr long) setupapi.StringTableAddStringEx
@ stdcall UpdSpStringTableDestroy(ptr) setupapi.StringTableDestroy
@ stub UpdSpStringTableEnum
@ stdcall UpdSpStringTableInitialize() setupapi.StringTableInitialize
@ stdcall UpdSpStringTableInitializeEx(long long) setupapi.StringTableInitializeEx
@ stdcall UpdSpStringTableLookUpString(ptr wstr long) setupapi.StringTableLookUpString
@ stdcall UpdSpStringTableLookUpStringEx(ptr wstr long ptr long) setupapi.StringTableLookUpStringEx
@ stdcall UpdSpTermDefaultQueueCallback(ptr) setupapi.SetupTermDefaultQueueCallback
