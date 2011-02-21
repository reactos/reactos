@ stdcall AddDelBackupEntryA(str str str long)
@ stdcall AddDelBackupEntryW(wstr wstr wstr long)
@ stdcall AddDelBackupEntry(str str str long) AddDelBackupEntryA
@ stdcall AdvInstallFileA(long str str str str long long)
@ stdcall AdvInstallFileW(long wstr wstr wstr wstr long long)
@ stdcall AdvInstallFile(long str str str str long long) AdvInstallFileA
@ stdcall CloseINFEngine(long)
@ stdcall DelNodeA(str long)
@ stdcall DelNodeW(wstr long)
@ stdcall DelNode(str long) DelNodeA
@ stdcall DelNodeRunDLL32A(ptr ptr str long)
@ stdcall DelNodeRunDLL32W(ptr ptr wstr long)
@ stdcall DelNodeRunDLL32(ptr ptr str long) DelNodeRunDLL32A
@ stdcall -private DllMain(long long ptr)
@ stdcall DoInfInstall(ptr)
@ stdcall ExecuteCabA(ptr ptr ptr)
@ stdcall ExecuteCabW(ptr ptr ptr)
@ stdcall ExecuteCab(ptr ptr ptr) ExecuteCabA
@ stdcall ExtractFilesA(str str long ptr ptr long)
@ stdcall ExtractFilesW(wstr wstr long ptr ptr long)
@ stdcall ExtractFiles(str str long ptr ptr long) ExtractFilesA
@ stdcall FileSaveMarkNotExistA(str str str)
@ stdcall FileSaveMarkNotExistW(wstr wstr wstr)
@ stdcall FileSaveMarkNotExist(str str str) FileSaveMarkNotExistA
@ stdcall FileSaveRestoreA(ptr str str str long)
@ stdcall FileSaveRestoreW(ptr wstr wstr wstr long)
@ stdcall FileSaveRestore(ptr str str str long) FileSaveRestoreA
@ stdcall FileSaveRestoreOnINFA(ptr str str str str str long)
@ stdcall FileSaveRestoreOnINFW(ptr wstr wstr wstr wstr wstr long)
@ stdcall FileSaveRestoreOnINF(ptr str str str str str long) FileSaveRestoreOnINFA
@ stdcall GetVersionFromFileA(str ptr ptr long)
@ stdcall GetVersionFromFileW(wstr ptr ptr long)
@ stdcall GetVersionFromFile(str ptr ptr long) GetVersionFromFileA
@ stdcall GetVersionFromFileExA(str ptr ptr long)
@ stdcall GetVersionFromFileExW(wstr ptr ptr long)
@ stdcall GetVersionFromFileEx(str ptr ptr long) GetVersionFromFileExA
@ stdcall IsNTAdmin(long ptr)
@ stdcall LaunchINFSectionA(ptr ptr str long)
@ stdcall LaunchINFSectionW(ptr ptr wstr long)
@ stdcall LaunchINFSection(ptr ptr str long) LaunchINFSectionA
@ stdcall LaunchINFSectionExA(ptr ptr str long)
@ stdcall LaunchINFSectionExW(ptr ptr wstr long)
@ stdcall LaunchINFSectionEx(ptr ptr str long) LaunchINFSectionExA
@ stdcall NeedReboot(long)
@ stdcall NeedRebootInit()
@ stdcall OpenINFEngineA(str str long ptr ptr)
@ stdcall OpenINFEngineW(wstr wstr long ptr ptr)
@ stdcall OpenINFEngine(str str long ptr ptr) OpenINFEngineA
@ stdcall RebootCheckOnInstallA(long str str long)
@ stdcall RebootCheckOnInstallW(long wstr wstr long)
@ stdcall RebootCheckOnInstall(long str str long) RebootCheckOnInstallA
@ stdcall RegInstallA(ptr str ptr)
@ stdcall RegInstallW(ptr wstr ptr)
@ stdcall RegInstall(ptr str ptr) RegInstallA
@ stdcall RegRestoreAllA(ptr str long)
@ stdcall RegRestoreAllW(ptr wstr long)
@ stdcall RegRestoreAll(ptr str long) RegRestoreAllA
@ stdcall RegSaveRestoreA(ptr str long str str str long)
@ stdcall RegSaveRestoreW(ptr wstr long wstr wstr wstr long)
@ stdcall RegSaveRestore(ptr str long str str str long) RegSaveRestoreA
@ stdcall RegSaveRestoreOnINFA(ptr str str str long long long)
@ stdcall RegSaveRestoreOnINFW(ptr wstr wstr wstr long long long)
@ stdcall RegSaveRestoreOnINF(ptr str str str long long long) RegSaveRestoreOnINFA
@ stdcall RegisterOCX(ptr ptr str long)
@ stdcall RunSetupCommandA(long str str str str ptr long ptr)
@ stdcall RunSetupCommandW(long wstr wstr wstr wstr ptr long ptr)
@ stdcall RunSetupCommand(long str str str str ptr long ptr) RunSetupCommandA
@ stdcall SetPerUserSecValuesA(ptr)
@ stdcall SetPerUserSecValuesW(ptr)
@ stdcall SetPerUserSecValues(ptr) SetPerUserSecValuesA
@ stdcall TranslateInfStringA(str str str str ptr long ptr ptr)
@ stdcall TranslateInfStringW(wstr wstr wstr wstr ptr long ptr ptr)
@ stdcall TranslateInfString(str str str str ptr long ptr ptr) TranslateInfStringA
@ stdcall TranslateInfStringExA(long str str str str long ptr ptr)
@ stdcall TranslateInfStringExW(long wstr wstr wstr wstr long ptr ptr)
@ stdcall TranslateInfStringEx(long str str str str long ptr ptr) TranslateInfStringExA
@ stdcall UserInstStubWrapperA(long long str long)
@ stdcall UserInstStubWrapperW(long long wstr long)
@ stdcall UserInstStubWrapper(long long str long) UserInstStubWrapperA
@ stdcall UserUnInstStubWrapperA(long long str long)
@ stdcall UserUnInstStubWrapperW(long long wstr long)
@ stdcall UserUnInstStubWrapper(long long str long) UserUnInstStubWrapperA
