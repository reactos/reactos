@ stdcall CM_Connect_MachineW(wstr ptr)
@ stdcall CM_Disconnect_Machine(long)
@ stub CM_Free_Log_Conf_Handle
@ stub CM_Free_Res_Des_Handle
@ stub CM_Get_DevNode_Status_Ex
@ stub CM_Get_Device_ID_ExW
@ stub CM_Get_First_Log_Conf_Ex
@ stub CM_Get_Next_Res_Des_Ex
@ stub CM_Get_Res_Des_Data_Ex
@ stub CM_Get_Res_Des_Data_Size_Ex
@ stub CM_Locate_DevNode_ExW
@ stub CM_Reenumerate_DevNode_Ex
@ stub CaptureAndConvertAnsiArg
@ stub CaptureStringArg
@ stub CenterWindowRelativeToParent
@ stub ConcatenatePaths
@ stub DelayedMove
@ stub DelimStringToMultiSz
@ stub DestroyTextFileReadBuffer
@ stub DoesUserHavePrivilege
@ stub DuplicateString
@ stub EnablePrivilege
@ stub ExtensionPropSheetPageProc
@ stub FileExists
@ stub FreeStringArray
@ stub GetNewInfName
@ stub GetSetFileTimestamp
@ stub GetVersionInfoFromImage
@ stub InfIsFromOemLocation
@ stdcall InstallHinfSection(long long str long)
@ stub InstallHinfSectionA
@ stub InstallHinfSectionW
@ stub InstallStop
@ stub IsUserAdmin
@ stub LookUpStringInTable
@ stub MemoryInitialize
@ stub MultiByteToUnicode
@ stub MultiSzFromSearchControl
@ stub MyFree
@ stub MyGetFileTitle
@ stub MyMalloc
@ stub MyRealloc
@ stub OpenAndMapFileForRead
@ stub OutOfMemory
@ stub QueryMultiSzValueToArray
@ stub QueryRegistryValue
@ stub ReadAsciiOrUnicodeTextFile
@ stub RegistryDelnode
@ stub RetreiveFileSecurity
@ stub RetrieveServiceConfig
@ stub SearchForInfFile
@ stub SetArrayToMultiSzValue
@ stub SetupAddInstallSectionToDiskSpaceListA
@ stub SetupAddInstallSectionToDiskSpaceListW
@ stub SetupAddSectionToDiskSpaceListA
@ stub SetupAddSectionToDiskSpaceListW
@ stub SetupAddToDiskSpaceListA
@ stub SetupAddToDiskSpaceListW
@ stub SetupAddToSourceListA
@ stub SetupAddToSourceListW
@ stub SetupAdjustDiskSpaceListA
@ stub SetupAdjustDiskSpaceListW
@ stub SetupCancelTemporarySourceList
@ stdcall SetupCloseFileQueue(ptr)
@ stdcall SetupCloseInfFile(long)
@ stub SetupCommitFileQueue
@ stdcall SetupCommitFileQueueA(long long ptr ptr)
@ stdcall SetupCommitFileQueueW(long long ptr ptr)
@ stub SetupCopyErrorA
@ stub SetupCopyErrorW
@ stdcall SetupCopyOEMInfA(str str long long ptr long ptr ptr)
@ stub SetupCopyOEMInfW
@ stub SetupCreateDiskSpaceListA
@ stub SetupCreateDiskSpaceListW
@ stub SetupDecompressOrCopyFileA
@ stub SetupDecompressOrCopyFileW
@ stub SetupDefaultQueueCallback
@ stdcall SetupDefaultQueueCallbackA(ptr long long long)
@ stdcall SetupDefaultQueueCallbackW(ptr long long long)
@ stub SetupDeleteErrorA
@ stub SetupDeleteErrorW
@ stub SetupDestroyDiskSpaceList
@ stub SetupDiAskForOEMDisk
@ stub SetupDiBuildClassInfoList
@ stdcall SetupDiBuildClassInfoListExW(long ptr long ptr wstr ptr)
@ stub SetupDiBuildDriverInfoList
@ stub SetupDiCallClassInstaller
@ stub SetupDiCancelDriverInfoSearch
@ stub SetupDiChangeState
@ stub SetupDiClassGuidsFromNameA
@ stdcall SetupDiClassGuidsFromNameExW(wstr ptr long ptr wstr ptr)
@ stub SetupDiClassGuidsFromNameW
@ stub SetupDiClassNameFromGuidA
@ stdcall SetupDiClassNameFromGuidExW(ptr ptr long ptr wstr ptr)
@ stub SetupDiClassNameFromGuidW
@ stub SetupDiCreateDevRegKeyA
@ stub SetupDiCreateDevRegKeyW
@ stub SetupDiCreateDeviceInfoA
@ stdcall SetupDiCreateDeviceInfoList(ptr ptr)
@ stdcall SetupDiCreateDeviceInfoListExW(ptr long str ptr)
@ stub SetupDiCreateDeviceInfoW
@ stub SetupDiDeleteDevRegKey
@ stub SetupDiDeleteDeviceInfo
@ stub SetupDiDestroyClassImageList
@ stdcall SetupDiDestroyDeviceInfoList(long)
@ stub SetupDiDestroyDriverInfoList
@ stub SetupDiDrawMiniIcon
@ stdcall SetupDiEnumDeviceInfo(long long ptr)
@ stdcall SetupDiEnumDeviceInterfaces(long ptr ptr long ptr)
@ stub SetupDiEnumDriverInfoA
@ stub SetupDiEnumDriverInfoW
@ stub SetupDiGetActualSectionToInstallA
@ stub SetupDiGetActualSectionToInstallW
@ stub SetupDiGetClassBitmapIndex
@ stub SetupDiGetClassDescriptionA
@ stdcall SetupDiGetClassDescriptionExW(ptr ptr long ptr wstr ptr)
@ stub SetupDiGetClassDescriptionW
@ stub SetupDiGetClassDevPropertySheetsA
@ stub SetupDiGetClassDevPropertySheetsW
@ stdcall SetupDiGetClassDevsA(ptr ptr long long)
@ stdcall SetupDiGetClassDevsExA(ptr str ptr long ptr str ptr)
@ stdcall SetupDiGetClassDevsExW(ptr wstr ptr long ptr wstr ptr)
@ stdcall SetupDiGetClassDevsW(ptr ptr long long)
@ stub SetupDiGetClassImageIndex
@ stub SetupDiGetClassImageList
@ stub SetupDiGetClassImageListExW
@ stub SetupDiGetClassInstallParamsA
@ stub SetupDiGetClassInstallParamsW
@ stub SetupDiGetDeviceInfoListClass
@ stdcall SetupDiGetDeviceInfoListDetailA(ptr ptr)
@ stdcall SetupDiGetDeviceInfoListDetailW(ptr ptr)
@ stub SetupDiGetDeviceInstallParamsA
@ stub SetupDiGetDeviceInstallParamsW
@ stub SetupDiGetDeviceInstanceIdA
@ stub SetupDiGetDeviceInstanceIdW
@ stdcall SetupDiGetDeviceRegistryPropertyA(long ptr long ptr ptr long ptr)
@ stub SetupDiGetDeviceRegistryPropertyW
@ stub SetupDiGetDriverInfoDetailA
@ stub SetupDiGetDriverInfoDetailW
@ stub SetupDiGetDriverInstallParamsA
@ stub SetupDiGetDriverInstallParamsW
@ stub SetupDiGetDeviceInterfaceAlias
@ stdcall SetupDiGetDeviceInterfaceDetailA(long ptr ptr long ptr ptr)
@ stdcall SetupDiGetDeviceInterfaceDetailW(long ptr ptr long ptr ptr)
@ stub SetupDiGetHwProfileFriendlyNameA
@ stub SetupDiGetHwProfileFriendlyNameW
@ stub SetupDiGetHwProfileList
@ stub SetupDiGetINFClassA
@ stub SetupDiGetINFClassW
@ stub SetupDiGetSelectedDevice
@ stub SetupDiGetSelectedDriverA
@ stub SetupDiGetSelectedDriverW
@ stub SetupDiGetWizardPage
@ stub SetupDiInstallClassA
@ stub SetupDiInstallClassW
@ stub SetupDiInstallDevice
@ stub SetupDiInstallDriverFiles
@ stub SetupDiLoadClassIcon
@ stub SetupDiMoveDuplicateDevice
@ stub SetupDiOpenClassRegKey
@ stdcall SetupDiOpenClassRegKeyExW(ptr long long wstr ptr)
@ stub SetupDiOpenDevRegKey
@ stub SetupDiOpenDeviceInfoA
@ stub SetupDiOpenDeviceInfoW
@ stub SetupDiOpenDeviceInterfaceRegKey
@ stub SetupDiRegisterDeviceInfo
@ stub SetupDiRemoveDevice
@ stub SetupDiSelectDevice
@ stub SetupDiSelectOEMDrv
@ stub SetupDiSetClassInstallParamsA
@ stub SetupDiSetClassInstallParamsW
@ stub SetupDiSetDeviceInstallParamsA
@ stub SetupDiSetDeviceInstallParamsW
@ stub SetupDiSetDeviceRegistryPropertyA
@ stub SetupDiSetDeviceRegistryPropertyW
@ stub SetupDiSetDriverInstallParamsA
@ stub SetupDiSetDriverInstallParamsW
@ stub SetupDiSetSelectedDevice
@ stub SetupDiSetSelectedDriverA
@ stub SetupDiSetSelectedDriverW
@ stub SetupDuplicateDiskSpaceListA
@ stub SetupDuplicateDiskSpaceListW
@ stdcall SetupFindFirstLineA(long str str ptr)
@ stdcall SetupFindFirstLineW(long wstr wstr ptr)
@ stdcall SetupFindNextLine(ptr ptr)
@ stdcall SetupFindNextMatchLineA(ptr str ptr)
@ stdcall SetupFindNextMatchLineW(ptr wstr ptr)
@ stub SetupFreeSourceListA
@ stub SetupFreeSourceListW
@ stdcall SetupGetBinaryField(ptr long ptr long ptr)
@ stdcall SetupGetFieldCount(ptr)
@ stub SetupGetFileCompressionInfoA
@ stub SetupGetFileCompressionInfoW
@ stdcall SetupGetFileQueueCount(long long ptr)
@ stdcall SetupGetFileQueueFlags(long ptr)
@ stub SetupGetInfFileListA
@ stub SetupGetInfFileListW
@ stub SetupGetInfInformationA
@ stub SetupGetInfInformationW
@ stdcall SetupGetIntField(ptr long ptr)
@ stdcall SetupGetLineByIndexA(long str long ptr)
@ stdcall SetupGetLineByIndexW(long wstr long ptr)
@ stdcall SetupGetLineCountA(long str)
@ stdcall SetupGetLineCountW(long wstr)
@ stdcall SetupGetLineTextA(ptr long str str ptr long ptr)
@ stdcall SetupGetLineTextW(ptr long wstr wstr ptr long ptr)
@ stdcall SetupGetMultiSzFieldA(ptr long ptr long ptr)
@ stdcall SetupGetMultiSzFieldW(ptr long ptr long ptr)
@ stub SetupGetSourceFileLocationA
@ stub SetupGetSourceFileLocationW
@ stub SetupGetSourceFileSizeA
@ stub SetupGetSourceFileSizeW
@ stub SetupGetSourceInfoA
@ stub SetupGetSourceInfoW
@ stdcall SetupGetStringFieldA(ptr long ptr long ptr)
@ stdcall SetupGetStringFieldW(ptr long ptr long ptr)
@ stub SetupGetTargetPathA
@ stub SetupGetTargetPathW
@ stdcall SetupInitDefaultQueueCallback(long)
@ stdcall SetupInitDefaultQueueCallbackEx(long long long long ptr)
@ stub SetupInitializeFileLogA
@ stub SetupInitializeFileLogW
@ stub SetupInstallFileA
@ stub SetupInstallFileExA
@ stub SetupInstallFileExW
@ stub SetupInstallFileW
@ stdcall SetupInstallFilesFromInfSectionA(long long long str str long)
@ stdcall SetupInstallFilesFromInfSectionW(long long long wstr wstr long)
@ stdcall SetupInstallFromInfSectionA(long long str long long str long ptr ptr long ptr)
@ stdcall SetupInstallFromInfSectionW(long long wstr long long wstr long ptr ptr long ptr)
@ stub SetupInstallServicesFromInfSectionA
@ stub SetupInstallServicesFromInfSectionW
@ stdcall SetupIterateCabinetA(str long ptr ptr)
@ stdcall SetupIterateCabinetW(wstr long ptr ptr)
@ stub SetupLogFileA
@ stub SetupLogFileW
@ stdcall SetupOpenAppendInfFileA(str long ptr)
@ stdcall SetupOpenAppendInfFileW(wstr long ptr)
@ stdcall SetupOpenFileQueue()
@ stdcall SetupOpenInfFileA(str str long ptr)
@ stdcall SetupOpenInfFileW(wstr wstr long ptr)
@ stub SetupOpenMasterInf
@ stub SetupPromptForDiskA
@ stub SetupPromptForDiskW
@ stub SetupPromptReboot
@ stub SetupQueryDrivesInDiskSpaceListA
@ stub SetupQueryDrivesInDiskSpaceListW
@ stub SetupQueryFileLogA
@ stub SetupQueryFileLogW
@ stub SetupQueryInfFileInformationA
@ stub SetupQueryInfFileInformationW
@ stub SetupQueryInfVersionInformationA
@ stub SetupQueryInfVersionInformationW
@ stub SetupQueryInfOriginalFileInformationW
@ stub SetupQuerySourceListA
@ stub SetupQuerySourceListW
@ stub SetupQuerySpaceRequiredOnDriveA
@ stub SetupQuerySpaceRequiredOnDriveW
@ stdcall SetupQueueCopyA(long str str str str str str str long)
@ stdcall SetupQueueCopyIndirectA(ptr)
@ stdcall SetupQueueCopyIndirectW(ptr)
@ stdcall SetupQueueCopySectionA(long str long long str long)
@ stdcall SetupQueueCopySectionW(long wstr long long wstr long)
@ stdcall SetupQueueCopyW(long wstr wstr wstr wstr wstr wstr wstr long)
@ stdcall SetupQueueDefaultCopyA(long long str str str long)
@ stdcall SetupQueueDefaultCopyW(long long wstr wstr wstr long)
@ stdcall SetupQueueDeleteA(long str str)
@ stdcall SetupQueueDeleteSectionA(long long long str)
@ stdcall SetupQueueDeleteSectionW(long long long wstr)
@ stdcall SetupQueueDeleteW(long wstr wstr)
@ stdcall SetupQueueRenameA(long str str str str)
@ stdcall SetupQueueRenameSectionA(long long long str)
@ stdcall SetupQueueRenameSectionW(long long long wstr)
@ stdcall SetupQueueRenameW(long wstr wstr wstr wstr)
@ stub SetupRemoveFileLogEntryA
@ stub SetupRemoveFileLogEntryW
@ stub SetupRemoveFromDiskSpaceListA
@ stub SetupRemoveFromDiskSpaceListW
@ stub SetupRemoveFromSourceListA
@ stub SetupRemoveFromSourceListW
@ stub SetupRemoveInstallSectionFromDiskSpaceListA
@ stub SetupRemoveInstallSectionFromDiskSpaceListW
@ stub SetupRemoveSectionFromDiskSpaceListA
@ stub SetupRemoveSectionFromDiskSpaceListW
@ stub SetupRenameErrorA
@ stub SetupRenameErrorW
@ stub SetupScanFileQueue
@ stdcall SetupScanFileQueueA(long long long ptr ptr ptr)
@ stdcall SetupScanFileQueueW(long long long ptr ptr ptr)
@ stdcall SetupSetDirectoryIdA(long long str)
@ stub SetupSetDirectoryIdExA
@ stub SetupSetDirectoryIdExW
@ stdcall SetupSetDirectoryIdW(long long wstr)
@ stdcall SetupSetFileQueueFlags(long long long)
@ stub SetupSetPlatformPathOverrideA
@ stub SetupSetPlatformPathOverrideW
@ stub SetupSetSourceListA
@ stub SetupSetSourceListW
@ stdcall SetupTermDefaultQueueCallback(ptr)
@ stub SetupTerminateFileLog
@ stub ShouldDeviceBeExcluded
@ stub StampFileSecurity
@ stub StringTableAddString
@ stub StringTableAddStringEx
@ stub StringTableDestroy
@ stub StringTableDuplicate
@ stub StringTableEnum
@ stub StringTableGetExtraData
@ stub StringTableInitialize
@ stub StringTableInitializeEx
@ stub StringTableLookUpString
@ stub StringTableLookUpStringEx
@ stub StringTableSetExtraData
@ stub StringTableStringFromId
@ stub StringTableTrim
@ stub TakeOwnershipOfFile
@ stub UnicodeToMultiByte
@ stub UnmapAndCloseFile
@ stub pSetupAddMiniIconToList
@ stub pSetupAddTagToGroupOrderListEntry
@ stub pSetupAppendStringToMultiSz
@ stub pSetupDirectoryIdToPath
@ stub pSetupGetField
@ stub pSetupGetOsLoaderDriveAndPath
@ stub pSetupGetVersionDatum
@ stub pSetupGuidFromString
@ stub pSetupIsGuidNull
@ stub pSetupMakeSurePathExists
@ stub pSetupStringFromGuid
