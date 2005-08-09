@ stub AcquireSCMLock
@ stub AddMiniIconToList
@ stub AddTagToGroupOrderListEntry
@ stub AppendStringToMultiSz
@ stdcall AssertFail(str long str)
@ stub CMP_Init_Detection
@ stub CMP_RegisterNotification
@ stub CMP_Report_LogOn
@ stub CMP_UnregisterNotification
@ stub CMP_WaitNoPendingInstallEvents
@ stub CMP_WaitServices
@ stub CM_Add_Empty_Log_Conf
@ stub CM_Add_Empty_Log_Conf_Ex
@ stub CM_Add_IDA
@ stub CM_Add_IDW
@ stub CM_Add_ID_ExA
@ stub CM_Add_ID_ExW
@ stub CM_Add_Range
@ stub CM_Add_Res_Des
@ stub CM_Add_Res_Des_Ex
@ stdcall CM_Connect_MachineA(str ptr)
@ stdcall CM_Connect_MachineW(wstr ptr)
@ stub CM_Create_DevNodeA
@ stub CM_Create_DevNodeW
@ stub CM_Create_DevNode_ExA
@ stub CM_Create_DevNode_ExW
@ stub CM_Create_Range_List
@ stub CM_Delete_Class_Key
@ stub CM_Delete_Class_Key_Ex
@ stub CM_Delete_DevNode_Key
@ stub CM_Delete_DevNode_Key_Ex
@ stub CM_Delete_Range
@ stub CM_Detect_Resource_Conflict
@ stub CM_Detect_Resource_Conflict_Ex
@ stub CM_Disable_DevNode
@ stub CM_Disable_DevNode_Ex
@ stdcall CM_Disconnect_Machine(long)
@ stub CM_Dup_Range_List
@ stub CM_Enable_DevNode
@ stub CM_Enable_DevNode_Ex
@ stdcall CM_Enumerate_Classes(long ptr long)
@ stdcall CM_Enumerate_Classes_Ex(long ptr long ptr)
@ stub CM_Enumerate_EnumeratorsA
@ stub CM_Enumerate_EnumeratorsW
@ stub CM_Enumerate_Enumerators_ExA
@ stub CM_Enumerate_Enumerators_ExW
@ stub CM_Find_Range
@ stub CM_First_Range
@ stub CM_Free_Log_Conf
@ stub CM_Free_Log_Conf_Ex
@ stub CM_Free_Log_Conf_Handle
@ stub CM_Free_Range_List
@ stub CM_Free_Res_Des
@ stub CM_Free_Res_Des_Ex
@ stub CM_Free_Res_Des_Handle
@ stdcall CM_Get_Child(ptr long long)
@ stdcall CM_Get_Child_Ex(ptr long long long)
@ stub CM_Get_Class_Key_NameA
@ stub CM_Get_Class_Key_NameW
@ stub CM_Get_Class_Key_Name_ExA
@ stub CM_Get_Class_Key_Name_ExW
@ stub CM_Get_Class_NameA
@ stub CM_Get_Class_NameW
@ stub CM_Get_Class_Name_ExA
@ stub CM_Get_Class_Name_ExW
@ stdcall CM_Get_Depth(ptr long long)
@ stdcall CM_Get_Depth_Ex(ptr long long long)
@ stdcall CM_Get_DevNode_Registry_PropertyA(long long ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Registry_PropertyW(long long ptr ptr ptr long)
@ stdcall CM_Get_DevNode_Registry_Property_ExA(long long ptr ptr ptr long long)
@ stdcall CM_Get_DevNode_Registry_Property_ExW(long long ptr ptr ptr long long)
@ stdcall CM_Get_DevNode_Status(ptr ptr long long)
@ stdcall CM_Get_DevNode_Status_Ex(ptr ptr long long long)
@ stdcall CM_Get_Device_IDA(long str long long)
@ stdcall CM_Get_Device_IDW(long wstr long long)
@ stdcall CM_Get_Device_ID_ExA(long str long long long)
@ stdcall CM_Get_Device_ID_ExW(long wstr long long long)
@ stdcall CM_Get_Device_ID_ListA(str str long long)
@ stdcall CM_Get_Device_ID_ListW(wstr wstr long long)
@ stdcall CM_Get_Device_ID_List_ExA(str str long long long)
@ stdcall CM_Get_Device_ID_List_ExW(wstr wstr long long long)
@ stdcall CM_Get_Device_ID_List_SizeA(ptr str long)
@ stdcall CM_Get_Device_ID_List_SizeW(ptr wstr long)
@ stdcall CM_Get_Device_ID_List_Size_ExA(ptr str long long)
@ stdcall CM_Get_Device_ID_List_Size_ExW(ptr wstr long long)
@ stdcall CM_Get_Device_ID_Size(ptr long long)
@ stdcall CM_Get_Device_ID_Size_Ex(ptr long long long)
@ stub CM_Get_Device_Interface_AliasA
@ stub CM_Get_Device_Interface_AliasW
@ stub CM_Get_Device_Interface_Alias_ExA
@ stub CM_Get_Device_Interface_Alias_ExW
@ stub CM_Get_Device_Interface_ListA
@ stub CM_Get_Device_Interface_ListW
@ stub CM_Get_Device_Interface_List_ExA
@ stub CM_Get_Device_Interface_List_ExW
@ stub CM_Get_Device_Interface_List_SizeA
@ stub CM_Get_Device_Interface_List_SizeW
@ stub CM_Get_Device_Interface_List_Size_ExA
@ stub CM_Get_Device_Interface_List_Size_ExW
@ stub CM_Get_First_Log_Conf
@ stub CM_Get_First_Log_Conf_Ex
@ stdcall CM_Get_Global_State(ptr long)
@ stdcall CM_Get_Global_State_Ex(ptr long long)
@ stub CM_Get_HW_Prof_FlagsA
@ stub CM_Get_HW_Prof_FlagsW
@ stub CM_Get_HW_Prof_Flags_ExA
@ stub CM_Get_HW_Prof_Flags_ExW
@ stub CM_Get_Hardware_Profile_InfoA
@ stub CM_Get_Hardware_Profile_InfoW
@ stub CM_Get_Hardware_Profile_Info_ExA
@ stub CM_Get_Hardware_Profile_Info_ExW
@ stub CM_Get_Log_Conf_Priority
@ stub CM_Get_Log_Conf_Priority_Ex
@ stub CM_Get_Next_Log_Conf
@ stub CM_Get_Next_Log_Conf_Ex
@ stub CM_Get_Next_Res_Des
@ stub CM_Get_Next_Res_Des_Ex
@ stdcall CM_Get_Parent(ptr long long)
@ stdcall CM_Get_Parent_Ex(ptr long long long)
@ stub CM_Get_Res_Des_Data
@ stub CM_Get_Res_Des_Data_Ex
@ stub CM_Get_Res_Des_Data_Size
@ stub CM_Get_Res_Des_Data_Size_Ex
@ stdcall CM_Get_Sibling(ptr long long)
@ stdcall CM_Get_Sibling_Ex(ptr long long long)
@ stdcall CM_Get_Version()
@ stdcall CM_Get_Version_Ex(long)
@ stub CM_Intersect_Range_List
@ stub CM_Invert_Range_List
@ stub CM_Is_Dock_Station_Present
@ stub CM_Is_Dock_Station_Present_Ex
@ stdcall CM_Locate_DevNodeA(ptr str long)
@ stdcall CM_Locate_DevNodeW(ptr wstr long)
@ stdcall CM_Locate_DevNode_ExA(ptr str long long)
@ stdcall CM_Locate_DevNode_ExW(ptr wstr long long)
@ stub CM_Merge_Range_List
@ stub CM_Modify_Res_Des
@ stub CM_Modify_Res_Des_Ex
@ stub CM_Move_DevNode
@ stub CM_Move_DevNode_Ex
@ stub CM_Next_Range
@ stub CM_Open_Class_KeyA
@ stub CM_Open_Class_KeyW
@ stub CM_Open_Class_Key_ExA
@ stub CM_Open_Class_Key_ExW
@ stub CM_Open_DevNode_Key
@ stub CM_Open_DevNode_Key_Ex
@ stub CM_Query_Arbitrator_Free_Data
@ stub CM_Query_Arbitrator_Free_Data_Ex
@ stub CM_Query_Arbitrator_Free_Size
@ stub CM_Query_Arbitrator_Free_Size_Ex
@ stub CM_Query_Remove_SubTree
@ stub CM_Query_Remove_SubTree_Ex
@ stub CM_Reenumerate_DevNode
@ stub CM_Reenumerate_DevNode_Ex
@ stub CM_Register_Device_Driver
@ stub CM_Register_Device_Driver_Ex
@ stub CM_Register_Device_InterfaceA
@ stub CM_Register_Device_InterfaceW
@ stub CM_Register_Device_Interface_ExA
@ stub CM_Register_Device_Interface_ExW
@ stub CM_Remove_SubTree
@ stub CM_Remove_SubTree_Ex
@ stub CM_Remove_Unmarked_Children
@ stub CM_Remove_Unmarked_Children_Ex
@ stub CM_Request_Device_EjectA
@ stub CM_Request_Device_EjectW
@ stub CM_Request_Eject_PC
@ stub CM_Reset_Children_Marks
@ stub CM_Reset_Children_Marks_Ex
@ stub CM_Run_Detection
@ stub CM_Run_Detection_Ex
@ stdcall CM_Set_DevNode_Problem(long long long)
@ stdcall CM_Set_DevNode_Problem_Ex(long long long long)
@ stdcall CM_Set_DevNode_Registry_PropertyA(long long ptr long long)
@ stdcall CM_Set_DevNode_Registry_PropertyW(long long ptr long long)
@ stdcall CM_Set_DevNode_Registry_Property_ExA(long long ptr long long long)
@ stdcall CM_Set_DevNode_Registry_Property_ExW(long long ptr long long long)
@ stub CM_Set_HW_Prof
@ stub CM_Set_HW_Prof_Ex
@ stub CM_Set_HW_Prof_FlagsA
@ stub CM_Set_HW_Prof_FlagsW
@ stub CM_Set_HW_Prof_Flags_ExA
@ stub CM_Set_HW_Prof_Flags_ExW
@ stub CM_Setup_DevNode
@ stub CM_Setup_DevNode_Ex
@ stub CM_Test_Range_Available
@ stub CM_Uninstall_DevNode
@ stub CM_Uninstall_DevNode_Ex
@ stub CM_Unregister_Device_InterfaceA
@ stub CM_Unregister_Device_InterfaceW
@ stub CM_Unregister_Device_Interface_ExA
@ stub CM_Unregister_Device_Interface_ExW
@ stdcall CaptureAndConvertAnsiArg(str ptr)
@ stdcall CaptureStringArg(wstr ptr)
@ stdcall CenterWindowRelativeToParent(long)
@ stdcall ConcatenatePaths(wstr wstr long ptr)
@ stdcall DelayedMove(wstr wstr)
@ stub DelimStringToMultiSz
@ stub DestroyTextFileReadBuffer
@ stdcall DoesUserHavePrivilege(wstr)
@ stdcall DuplicateString(wstr)
@ stdcall EnablePrivilege(wstr long)
@ stub ExtensionPropSheetPageProc
@ stdcall FileExists(wstr ptr)
@ stub FreeStringArray
@ stub GetCurrentDriverSigningPolicy
@ stub GetNewInfName
@ stdcall GetSetFileTimestamp(wstr ptr ptr ptr long)
@ stdcall GetVersionInfoFromImage(wstr ptr ptr)
@ stub InfIsFromOemLocation
@ stub InstallCatalog
@ stdcall InstallHinfSection(long long str long) InstallHinfSectionA
@ stdcall InstallHinfSectionA(long long str long)
@ stdcall InstallHinfSectionW(long long wstr long)
@ stub InstallStop
@ stub InstallStopEx
@ stdcall IsUserAdmin()
@ stub LookUpStringInTable
@ stub MemoryInitialize
@ stdcall MultiByteToUnicode(str long)
@ stub MultiSzFromSearchControl
@ stdcall MyFree(ptr)
@ stdcall MyGetFileTitle(wstr)
@ stdcall MyMalloc(long)
@ stdcall MyRealloc(ptr long)
@ stdcall OpenAndMapFileForRead(wstr ptr ptr ptr ptr)
@ stub OutOfMemory
@ stub QueryMultiSzValueToArray
@ stdcall QueryRegistryValue(long wstr ptr ptr ptr)
@ stub ReadAsciiOrUnicodeTextFile
@ stub RegistryDelnode
@ stdcall RetreiveFileSecurity(wstr ptr)
@ stub RetrieveServiceConfig
@ stub SearchForInfFile
@ stub SetArrayToMultiSzValue
@ stdcall SetupAddInstallSectionToDiskSpaceListA(long long long str ptr long)
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
@ stub SetupCloseLog
@ stdcall SetupCommitFileQueueA(long long ptr ptr)
@ stdcall SetupCommitFileQueueW(long long ptr ptr)
@ stdcall SetupCopyErrorA(long str str str str str long long str long ptr)
@ stdcall SetupCopyErrorW(long wstr wstr wstr wstr wstr long long wstr long ptr)
@ stdcall SetupCopyOEMInfA(str str long long ptr long ptr ptr)
@ stdcall SetupCopyOEMInfW(wstr wstr long long ptr long ptr ptr)
@ stdcall SetupCreateDiskSpaceListA(ptr long long)
@ stdcall SetupCreateDiskSpaceListW(ptr long long)
@ stub SetupDecompressOrCopyFileA
@ stub SetupDecompressOrCopyFileW
@ stub SetupDefaultQueueCallback
@ stdcall SetupDefaultQueueCallbackA(ptr long long long)
@ stdcall SetupDefaultQueueCallbackW(ptr long long long)
@ stdcall SetupDeleteErrorA(long str str long long)
@ stdcall SetupDeleteErrorW(long wstr wstr long long)
@ stdcall SetupDestroyDiskSpaceList(long)
@ stub SetupDiAskForOEMDisk
@ stdcall SetupDiBuildClassInfoList(long ptr long ptr)
@ stdcall SetupDiBuildClassInfoListExA(long ptr long ptr str ptr)
@ stdcall SetupDiBuildClassInfoListExW(long ptr long ptr wstr ptr)
@ stdcall SetupDiBuildDriverInfoList(long ptr long)
@ stdcall SetupDiCallClassInstaller(long ptr ptr)
@ stub SetupDiCancelDriverInfoSearch
@ stub SetupDiChangeState
@ stdcall SetupDiClassGuidsFromNameA(str ptr long ptr)
@ stdcall SetupDiClassGuidsFromNameExA(str ptr long ptr str ptr)
@ stdcall SetupDiClassGuidsFromNameExW(wstr ptr long ptr wstr ptr)
@ stdcall SetupDiClassGuidsFromNameW(wstr ptr long ptr)
@ stdcall SetupDiClassNameFromGuidA(ptr str long ptr)
@ stdcall SetupDiClassNameFromGuidExA(ptr str long ptr wstr ptr)
@ stdcall SetupDiClassNameFromGuidExW(ptr wstr long ptr wstr ptr)
@ stdcall SetupDiClassNameFromGuidW(ptr wstr long ptr)
@ stub SetupDiCreateDevRegKeyA
@ stub SetupDiCreateDevRegKeyW
@ stdcall SetupDiCreateDeviceInfoA(ptr str ptr str ptr long ptr)
@ stdcall SetupDiCreateDeviceInfoW(ptr wstr ptr wstr ptr long ptr)
@ stdcall SetupDiCreateDeviceInfoList(ptr ptr)
@ stdcall SetupDiCreateDeviceInfoListExA(ptr long str ptr)
@ stdcall SetupDiCreateDeviceInfoListExW(ptr long wstr ptr)
@ stub SetupDiDeleteDevRegKey
@ stdcall SetupDiDeleteDeviceInfo(long ptr)
@ stub SetupDiDeleteDeviceInterfaceData
@ stub SetupDiDestroyClassImageList
@ stdcall SetupDiDestroyDeviceInfoList(long)
@ stdcall SetupDiDestroyDriverInfoList(long ptr long)
@ stub SetupDiDrawMiniIcon
@ stdcall SetupDiEnumDeviceInfo(long long ptr)
@ stdcall SetupDiEnumDeviceInterfaces(long ptr ptr long ptr)
@ stdcall SetupDiEnumDriverInfoA(long ptr long long ptr)
@ stdcall SetupDiEnumDriverInfoW(long ptr long long ptr)
@ stdcall SetupDiGetActualSectionToInstallA(long str str long ptr ptr)
@ stdcall SetupDiGetActualSectionToInstallW(long wstr wstr long ptr ptr)
@ stub SetupDiGetClassBitmapIndex
@ stdcall SetupDiGetClassDescriptionA(ptr str long ptr)
@ stdcall SetupDiGetClassDescriptionExA(ptr str long ptr str ptr)
@ stdcall SetupDiGetClassDescriptionExW(ptr wstr long ptr wstr ptr)
@ stdcall SetupDiGetClassDescriptionW(ptr wstr long ptr)
@ stub SetupDiGetClassDevPropertySheetsA
@ stub SetupDiGetClassDevPropertySheetsW
@ stdcall SetupDiGetClassDevsA(ptr ptr long long)
@ stdcall SetupDiGetClassDevsExA(ptr str ptr long ptr str ptr)
@ stdcall SetupDiGetClassDevsExW(ptr wstr ptr long ptr wstr ptr)
@ stdcall SetupDiGetClassDevsW(ptr ptr long long)
@ stub SetupDiGetClassImageIndex
@ stub SetupDiGetClassImageList
@ stub SetupDiGetClassImageListExA
@ stub SetupDiGetClassImageListExW
@ stub SetupDiGetClassInstallParamsA
@ stub SetupDiGetClassInstallParamsW
@ stub SetupDiGetDeviceInfoListClass
@ stdcall SetupDiGetDeviceInfoListDetailA(ptr ptr)
@ stdcall SetupDiGetDeviceInfoListDetailW(ptr ptr)
@ stdcall SetupDiGetDeviceInstallParamsA(ptr ptr ptr)
@ stub SetupDiGetDeviceInstallParamsW
@ stub SetupDiGetDeviceInstanceIdA
@ stub SetupDiGetDeviceInstanceIdW
@ stdcall SetupDiGetDeviceRegistryPropertyA(long ptr long ptr ptr long ptr)
@ stdcall SetupDiGetDeviceRegistryPropertyW(long ptr long ptr ptr long ptr)
@ stub SetupDiGetDriverInfoDetailA
@ stub SetupDiGetDriverInfoDetailW
@ stub SetupDiGetDriverInstallParamsA
@ stub SetupDiGetDriverInstallParamsW
@ stub SetupDiGetDeviceInterfaceAlias
@ stdcall SetupDiGetDeviceInterfaceDetailA(long ptr ptr long ptr ptr)
@ stdcall SetupDiGetDeviceInterfaceDetailW(long ptr ptr long ptr ptr)
@ stub SetupDiGetHwProfileFriendlyNameA
@ stub SetupDiGetHwProfileFriendlyNameExA
@ stub SetupDiGetHwProfileFriendlyNameExW
@ stub SetupDiGetHwProfileFriendlyNameW
@ stub SetupDiGetHwProfileList
@ stub SetupDiGetHwProfileListExA
@ stub SetupDiGetHwProfileListExW
@ stub SetupDiGetINFClassA
@ stub SetupDiGetINFClassW
@ stub SetupDiGetSelectedDevice
@ stub SetupDiGetSelectedDriverA
@ stdcall SetupDiGetSelectedDriverW(ptr ptr ptr)
@ stub SetupDiGetWizardPage
@ stdcall SetupDiInstallClassA(long str long ptr)
@ stub SetupDiInstallClassExA
@ stub SetupDiInstallClassExW
@ stdcall SetupDiInstallClassW(long wstr long ptr)
@ stub SetupDiInstallDevice
@ stub SetupDiInstallDriverFiles
@ stub SetupDiLoadClassIcon
@ stub SetupDiMoveDuplicateDevice
@ stdcall SetupDiOpenClassRegKey(ptr long)
@ stdcall SetupDiOpenClassRegKeyExA(ptr long long str ptr)
@ stdcall SetupDiOpenClassRegKeyExW(ptr long long wstr ptr)
@ stdcall SetupDiOpenDevRegKey(ptr ptr long long long long)
@ stdcall SetupDiOpenDeviceInfoA(ptr str long long ptr)
@ stdcall SetupDiOpenDeviceInfoW(ptr wstr long long ptr)
@ stdcall SetupDiOpenDeviceInterfaceA(ptr str long ptr)
@ stub SetupDiOpenDeviceInterfaceRegKey
@ stdcall SetupDiOpenDeviceInterfaceW(ptr wstr long ptr)
@ stub SetupDiRegisterDeviceInfo
@ stub SetupDiRemoveDevice
@ stub SetupDiRemoveDeviceInterface
@ stub SetupDiSelectDevice
@ stub SetupDiSelectOEMDrv
@ stdcall SetupDiSetClassInstallParamsA(ptr ptr ptr long)
@ stub SetupDiSetClassInstallParamsW
@ stub SetupDiSetDeviceInstallParamsA
@ stub SetupDiSetDeviceInstallParamsW
@ stub SetupDiSetDeviceRegistryPropertyA
@ stub SetupDiSetDeviceRegistryPropertyW
@ stub SetupDiSetDriverInstallParamsA
@ stub SetupDiSetDriverInstallParamsW
@ stub SetupDiSetSelectedDevice
@ stub SetupDiSetSelectedDriverA
@ stdcall SetupDiSetSelectedDriverW(ptr ptr ptr)
@ stub SetupDiUnremoveDevice
@ stub SetupDuplicateDiskSpaceListA
@ stub SetupDuplicateDiskSpaceListW
@ stdcall SetupFindFirstLineA(long str str ptr)
@ stdcall SetupFindFirstLineW(long wstr wstr ptr)
@ stdcall SetupFindNextLine(ptr ptr)
@ stdcall SetupFindNextMatchLineA(ptr str ptr)
@ stdcall SetupFindNextMatchLineW(ptr wstr ptr)
@ stub SetupFreeSourceListA
@ stub SetupFreeSourceListW
@ stub SetupGetBackupInformationA
@ stub SetupGetBackupInformationW
@ stdcall SetupGetBinaryField(ptr long ptr long ptr)
@ stdcall SetupGetFieldCount(ptr)
@ stub SetupGetFileCompressionInfoA
@ stub SetupGetFileCompressionInfoW
@ stdcall SetupGetFileQueueCount(long long ptr)
@ stdcall SetupGetFileQueueFlags(long ptr)
@ stub SetupGetInfFileListA
@ stdcall SetupGetInfFileListW(wstr long wstr long ptr)
@ stdcall SetupGetInfInformationA(ptr long ptr long ptr)
@ stdcall SetupGetInfInformationW(ptr long ptr long ptr)
@ stub SetupGetInfSections
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
@ stdcall SetupInitializeFileLogA(str long)
@ stdcall SetupInitializeFileLogW(wstr long)
@ stub SetupInstallFileA
@ stub SetupInstallFileExA
@ stub SetupInstallFileExW
@ stub SetupInstallFileW
@ stdcall SetupInstallFilesFromInfSectionA(long long long str str long)
@ stdcall SetupInstallFilesFromInfSectionW(long long long wstr wstr long)
@ stdcall SetupInstallFromInfSectionA(long long str long long str long ptr ptr long ptr)
@ stdcall SetupInstallFromInfSectionW(long long wstr long long wstr long ptr ptr long ptr)
@ stub SetupInstallServicesFromInfSectionA
@ stub SetupInstallServicesFromInfSectionExA
@ stub SetupInstallServicesFromInfSectionExW
@ stub SetupInstallServicesFromInfSectionW
@ stdcall SetupIterateCabinetA(str long ptr ptr)
@ stdcall SetupIterateCabinetW(wstr long ptr ptr)
@ stub SetupLogErrorA
@ stub SetupLogErrorW
@ stub SetupLogFileA
@ stub SetupLogFileW
@ stdcall SetupOpenAppendInfFileA(str long ptr)
@ stdcall SetupOpenAppendInfFileW(wstr long ptr)
@ stdcall SetupOpenFileQueue()
@ stdcall SetupOpenInfFileA(str str long ptr)
@ stdcall SetupOpenInfFileW(wstr wstr long ptr)
@ stdcall SetupOpenMasterInf()
@ stub SetupPromptForDiskA
@ stub SetupPromptForDiskW
@ stub SetupPromptReboot
@ stub SetupQueryDrivesInDiskSpaceListA
@ stub SetupQueryDrivesInDiskSpaceListW
@ stub SetupQueryFileLogA
@ stub SetupQueryFileLogW
@ stub SetupQueryInfFileInformationA
@ stub SetupQueryInfFileInformationW
@ stub SetupQueryInfOriginalFileInformationA
@ stub SetupQueryInfOriginalFileInformationW
@ stub SetupQueryInfVersionInformationA
@ stub SetupQueryInfVersionInformationW
@ stub SetupQuerySourceListA
@ stub SetupQuerySourceListW
@ stdcall SetupQuerySpaceRequiredOnDriveA(long str ptr ptr long)
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
@ stdcall SetupRenameErrorA(long str str str long long)
@ stdcall SetupRenameErrorW(long wstr wstr wstr long long)
@ stub SetupScanFileQueue
@ stdcall SetupScanFileQueueA(long long long ptr ptr ptr)
@ stdcall SetupScanFileQueueW(long long long ptr ptr ptr)
@ stdcall SetupSetDirectoryIdA(long long str)
@ stub SetupSetDirectoryIdExA
@ stub SetupSetDirectoryIdExW
@ stdcall SetupSetDirectoryIdW(long long wstr)
@ stdcall SetupSetFileQueueAlternatePlatformA(ptr ptr str)
@ stdcall SetupSetFileQueueAlternatePlatformW(ptr ptr wstr)
@ stdcall SetupSetFileQueueFlags(long long long)
@ stub SetupSetPlatformPathOverrideA
@ stub SetupSetPlatformPathOverrideW
@ stub SetupSetSourceListA
@ stub SetupSetSourceListW
@ stdcall SetupTermDefaultQueueCallback(ptr)
@ stdcall SetupTerminateFileLog(long)
@ stub ShouldDeviceBeExcluded
@ stdcall StampFileSecurity(wstr ptr)
@ stdcall StringTableAddString(ptr wstr long)
@ stub StringTableAddStringEx
@ stdcall StringTableDestroy(ptr)
@ stdcall StringTableDuplicate(ptr)
@ stub StringTableEnum
@ stub StringTableGetExtraData
@ stdcall StringTableInitialize()
@ stub StringTableInitializeEx
@ stdcall StringTableLookUpString(ptr wstr long)
@ stub StringTableLookUpStringEx
@ stub StringTableSetExtraData
@ stdcall StringTableStringFromId(ptr long)
@ stdcall StringTableStringFromIdEx(ptr long ptr ptr)
@ stdcall StringTableTrim(ptr)
@ stdcall TakeOwnershipOfFile(wstr)
@ stdcall UnicodeToMultiByte(wstr long)
@ stdcall UnmapAndCloseFile(long long ptr)
@ stub VerifyCatalogFile
@ stub VerifyFile
@ stub pSetupAccessRunOnceNodeList
@ stub pSetupAddMiniIconToList
@ stub pSetupAddTagToGroupOrderListEntry
@ stub pSetupAppendStringToMultiSz
@ stub pSetupDestroyRunOnceNodeList
@ stub pSetupDirectoryIdToPath
@ stub pSetupGetField
@ stub pSetupGetGlobalFlags
@ stub pSetupGetOsLoaderDriveAndPath
@ stub pSetupGetQueueFlags
@ stub pSetupGetVersionDatum
@ stub pSetupGuidFromString
@ stub pSetupIsGuidNull
@ stub pSetupMakeSurePathExists
@ stub pSetupSetGlobalFlags
@ stub pSetupSetQueueFlags
@ stub pSetupSetSystemSourceFlags
@ stub pSetupStringFromGuid
@ stub pSetupVerifyQueuedCatalogs
