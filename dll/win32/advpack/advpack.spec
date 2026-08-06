@ stdcall AddDelBackupEntry(str str str long) AddDelBackupEntryA
@ stdcall AddDelBackupEntryA(str str str long)
@ stdcall AddDelBackupEntryW(wstr wstr wstr long)
@ stdcall AdvInstallFile(long str str str str long long) AdvInstallFileA
@ stdcall AdvInstallFileA(long str str str str long long)
@ stdcall AdvInstallFileW(long wstr wstr wstr wstr long long)
@ stdcall CloseINFEngine(long)
@ stdcall DelNode(str long) DelNodeA
@ stdcall DelNodeA(str long)
@ stdcall DelNodeRunDLL32(ptr ptr str long) DelNodeRunDLL32A
@ stdcall DelNodeRunDLL32A(ptr ptr str long)
@ stdcall DelNodeRunDLL32W(ptr ptr wstr long)
@ stdcall DelNodeW(wstr long)
@ stdcall DoInfInstall(ptr)
@ stdcall ExecuteCab(ptr ptr ptr) ExecuteCabA
@ stdcall ExecuteCabA(ptr ptr ptr)
@ stdcall ExecuteCabW(ptr ptr ptr)
@ stdcall ExtractFiles(str str long str ptr long) ExtractFilesA
@ stdcall ExtractFilesA(str str long str ptr long)
@ stdcall ExtractFilesW(wstr wstr long wstr ptr long)
@ stdcall FileSaveMarkNotExist(str str str) FileSaveMarkNotExistA
@ stdcall FileSaveMarkNotExistA(str str str)
@ stdcall FileSaveMarkNotExistW(wstr wstr wstr)
@ stdcall FileSaveRestore(ptr str str str long) FileSaveRestoreA
@ stdcall FileSaveRestoreA(ptr str str str long)
@ stdcall FileSaveRestoreOnINF(ptr str str str str str long) FileSaveRestoreOnINFA
@ stdcall FileSaveRestoreOnINFA(ptr str str str str str long)
@ stdcall FileSaveRestoreOnINFW(ptr wstr wstr wstr wstr wstr long)
@ stdcall FileSaveRestoreW(ptr wstr wstr wstr long)
@ stdcall GetVersionFromFile(str ptr ptr long) GetVersionFromFileA
@ stdcall GetVersionFromFileA(str ptr ptr long)
@ stdcall GetVersionFromFileEx(str ptr ptr long) GetVersionFromFileExA
@ stdcall GetVersionFromFileExA(str ptr ptr long)
@ stdcall GetVersionFromFileExW(wstr ptr ptr long)
@ stdcall GetVersionFromFileW(wstr ptr ptr long)
@ stdcall IsNTAdmin(long ptr)
@ stdcall LaunchINFSection(ptr ptr str long) LaunchINFSectionA
@ stdcall LaunchINFSectionA(ptr ptr str long)
@ stdcall LaunchINFSectionEx(ptr ptr str long) LaunchINFSectionExA
@ stdcall LaunchINFSectionExA(ptr ptr str long)
@ stdcall LaunchINFSectionExW(ptr ptr wstr long)
@ stdcall LaunchINFSectionW(ptr ptr wstr long)
@ stdcall NeedReboot(long)
@ stdcall NeedRebootInit()
@ stdcall OpenINFEngine(str str long ptr ptr) OpenINFEngineA
@ stdcall OpenINFEngineA(str str long ptr ptr)
@ stdcall OpenINFEngineW(wstr wstr long ptr ptr)
@ stdcall RebootCheckOnInstall(long str str long) RebootCheckOnInstallA
@ stdcall RebootCheckOnInstallA(long str str long)
@ stdcall RebootCheckOnInstallW(long wstr wstr long)
@ stdcall RegInstall(ptr str ptr) RegInstallA
@ stdcall RegInstallA(ptr str ptr)
@ stdcall RegInstallW(ptr wstr ptr)
@ stdcall RegRestoreAll(ptr str long) RegRestoreAllA
@ stdcall RegRestoreAllA(ptr str long)
@ stdcall RegRestoreAllW(ptr wstr long)
@ stdcall RegSaveRestore(ptr str long str str str long) RegSaveRestoreA
@ stdcall RegSaveRestoreA(ptr str long str str str long)
@ stdcall RegSaveRestoreOnINF(ptr str str str long long long) RegSaveRestoreOnINFA
@ stdcall RegSaveRestoreOnINFA(ptr str str str long long long)
@ stdcall RegSaveRestoreOnINFW(ptr wstr wstr wstr long long long)
@ stdcall RegSaveRestoreW(ptr wstr long wstr wstr wstr long)
@ stdcall RegisterOCX(ptr ptr str long)
@ stdcall RunSetupCommand(long str str str str ptr long ptr) RunSetupCommandA
@ stdcall RunSetupCommandA(long str str str str ptr long ptr)
@ stdcall RunSetupCommandW(long wstr wstr wstr wstr ptr long ptr)
@ stdcall SetPerUserSecValues(ptr) SetPerUserSecValuesA
@ stdcall SetPerUserSecValuesA(ptr)
@ stdcall SetPerUserSecValuesW(ptr)
@ stdcall TranslateInfString(str str str str ptr long ptr ptr) TranslateInfStringA
@ stdcall TranslateInfStringA(str str str str ptr long ptr ptr)
@ stdcall TranslateInfStringEx(long str str str str long ptr ptr) TranslateInfStringExA
@ stdcall TranslateInfStringExA(long str str str str long ptr ptr)
@ stdcall TranslateInfStringExW(long wstr wstr wstr wstr long ptr ptr)
@ stdcall TranslateInfStringW(wstr wstr wstr wstr ptr long ptr ptr)
@ stdcall UserInstStubWrapper(long long str long) UserInstStubWrapperA
@ stdcall UserInstStubWrapperA(long long str long)
@ stdcall UserInstStubWrapperW(long long wstr long)
@ stdcall UserUnInstStubWrapper(long long str long) UserUnInstStubWrapperA
@ stdcall UserUnInstStubWrapperA(long long str long)
@ stdcall UserUnInstStubWrapperW(long long wstr long)
