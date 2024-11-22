
@ extern IsUnattendedSetup
@ extern MbrPartitionTypes
@ extern GptPartitionTypes

;; fileqsup and infsupp function pointers to be initialized by the user of this library
;;@ extern SpFileExports
;;@ extern SpInfExports

;; infsupp
@ cdecl INF_GetDataField(ptr long ptr)  ## -private


;; filesup
@ cdecl ConcatPathsV(ptr long long ptr)
@ cdecl CombinePathsV(ptr long long ptr)
@ varargs ConcatPaths(ptr long long)
@ varargs CombinePaths(ptr long long)
@ cdecl SetupCopyFile(wstr wstr long)
@ cdecl SetupDeleteFile(wstr long)
@ cdecl SetupMoveFile(wstr wstr long)

;; genlist
@ stdcall CreateGenericList()
@ stdcall DestroyGenericList(ptr long)
@ stdcall GetCurrentListEntry(ptr)
@ stdcall GetFirstListEntry(ptr)
@ stdcall GetNextListEntry(ptr)
@ stdcall GetListEntryData(ptr)
@ stdcall GetNumberOfListEntries(ptr)
@ stdcall SetCurrentListEntry(ptr ptr)

;; partlist
@ stdcall CreatePartitionList()
@ stdcall CreatePartition(ptr ptr int64 ptr)
@ stdcall DeletePartition(ptr ptr ptr)
@ stdcall DestroyPartitionList(ptr)
@ stdcall GetNextPartition(ptr ptr)
@ stdcall GetPrevPartition(ptr ptr)
@ stdcall GetAdjUnpartitionedEntry(ptr long)
@ stdcall PartitionCreateChecks(ptr int64 ptr)
@ cdecl -ret64 RoundingDivide(int64 int64)
;;;;
@ cdecl IsPartitionActive(ptr)          ## -private
@ cdecl SelectPartition(ptr long long)

;; osdetect
@ stdcall CreateNTOSInstallationsList(ptr)
@ stdcall FindSubStrI(wstr wstr)

;; settings
@ cdecl CreateComputerTypeList(ptr)
@ cdecl CreateDisplayDriverList(ptr)
@ cdecl CreateKeyboardDriverList(ptr)
@ cdecl CreateKeyboardLayoutList(ptr wstr ptr)
@ cdecl CreateLanguageList(ptr ptr)
@ cdecl GetDefaultLanguageIndex()

;; mui
@ cdecl MUIDefaultKeyboardLayout(wstr)
@ cdecl MUIGetOEMCodePage(wstr)         ## -private

;; setuplib
@ stdcall CheckUnattendedSetup(ptr)
@ stdcall FinishSetup(ptr)
@ stdcall IsValidInstallDirectory(wstr)
@ stdcall InitDestinationPaths(ptr wstr ptr)
@ stdcall InitializeSetup(ptr ptr ptr ptr)
@ stdcall InitSystemPartition(ptr ptr ptr ptr ptr)
@ stdcall InstallSetupInfFile(ptr)
@ stdcall UpdateRegistry(ptr long ptr long wstr ptr ptr)

;; fsutil
@ stdcall FsVolCommitOpsQueue(ptr ptr ptr ptr ptr)
@ stdcall GetRegisteredFileSystems(long ptr)

;; install
@ stdcall PrepareFileCopy(ptr ptr)
@ stdcall DoFileCopy(ptr ptr ptr)

;; bootsup
@ stdcall InstallBootManagerAndBootEntries(long ptr ptr ptr ptr)
@ stdcall InstallBootcodeToRemovable(long ptr ptr ptr)
