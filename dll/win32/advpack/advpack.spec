1 stdcall DelNodeRunDLL32(ptr ptr str long) DelNodeRunDLL32A
2 stdcall DelNodeRunDLL32A(ptr ptr str long)
3 stdcall DoInfInstall(ptr)
4 stdcall DoInfInstallA(ptr) DoInfInstall
# DoInfInstallW
6 stdcall FileSaveRestore(ptr str str str long) FileSaveRestoreA
7 stdcall FileSaveRestoreA(ptr str str str long)
8 stdcall LaunchINFSectionA(ptr ptr str long)
9 stdcall LaunchINFSectionEx(ptr ptr str long) LaunchINFSectionExA
10 stdcall LaunchINFSectionExA(ptr ptr str long)
11 stdcall RegisterOCX(ptr ptr str long)
# RegisterOCXW
13 stdcall AddDelBackupEntry(str str str long) AddDelBackupEntryA
14 stdcall AddDelBackupEntryA(str str str long)
15 stdcall AddDelBackupEntryW(wstr wstr wstr long)
16 stdcall AdvInstallFile(long str str str str long long) AdvInstallFileA
17 stdcall AdvInstallFileA(long str str str str long long)
18 stdcall AdvInstallFileW(long wstr wstr wstr wstr long long)
19 stdcall CloseINFEngine(long)
20 stdcall DelNode(str long) DelNodeA
21 stdcall DelNodeA(str long)
22 stdcall DelNodeRunDLL32W(ptr ptr wstr long)
23 stdcall DelNodeW(wstr long)
24 stdcall ExecuteCab(ptr ptr ptr) ExecuteCabA
25 stdcall ExecuteCabA(ptr ptr ptr)
26 stdcall ExecuteCabW(ptr ptr ptr)
27 stdcall ExtractFiles(str str long ptr ptr long) ExtractFilesA
28 stdcall ExtractFilesA(str str long ptr ptr long)
29 stdcall ExtractFilesW(wstr wstr long ptr ptr long)
30 stdcall FileSaveMarkNotExist(str str str) FileSaveMarkNotExistA
31 stdcall FileSaveMarkNotExistA(str str str)
32 stdcall FileSaveMarkNotExistW(wstr wstr wstr)
33 stdcall FileSaveRestoreOnINF(ptr str str str str str long) FileSaveRestoreOnINFA
34 stdcall FileSaveRestoreOnINFA(ptr str str str str str long)
35 stdcall FileSaveRestoreOnINFW(ptr wstr wstr wstr wstr wstr long)
36 stdcall FileSaveRestoreW(ptr wstr wstr wstr long)
37 stdcall GetVersionFromFile(str ptr ptr long) GetVersionFromFileA
38 stdcall GetVersionFromFileA(str ptr ptr long)
39 stdcall GetVersionFromFileEx(str ptr ptr long) GetVersionFromFileExA
40 stdcall GetVersionFromFileExA(str ptr ptr long)
41 stdcall GetVersionFromFileExW(wstr ptr ptr long)
42 stdcall GetVersionFromFileW(wstr ptr ptr long)
43 stdcall IsNTAdmin(long ptr)
44 stdcall LaunchINFSection(ptr ptr str long) LaunchINFSectionA
45 stdcall LaunchINFSectionExW(ptr ptr wstr long)
46 stdcall LaunchINFSectionW(ptr ptr wstr long)
47 stdcall NeedReboot(long)
48 stdcall NeedRebootInit()
49 stdcall OpenINFEngine(str str long ptr ptr) OpenINFEngineA
50 stdcall OpenINFEngineA(str str long ptr ptr)
51 stdcall OpenINFEngineW(wstr wstr long ptr ptr)
52 stdcall RebootCheckOnInstall(long str str long) RebootCheckOnInstallA
53 stdcall RebootCheckOnInstallA(long str str long)
54 stdcall RebootCheckOnInstallW(long wstr wstr long)
55 stdcall RegInstall(ptr str ptr) RegInstallA
56 stdcall RegInstallA(ptr str ptr)
57 stdcall RegInstallW(ptr wstr ptr)
58 stdcall RegRestoreAll(ptr str long) RegRestoreAllA
59 stdcall RegRestoreAllA(ptr str long)
60 stdcall RegRestoreAllW(ptr wstr long)
61 stdcall RegSaveRestore(ptr str long str str str long) RegSaveRestoreA
62 stdcall RegSaveRestoreA(ptr str long str str str long)
63 stdcall RegSaveRestoreOnINF(ptr str str str long long long) RegSaveRestoreOnINFA
64 stdcall RegSaveRestoreOnINFA(ptr str str str long long long)
65 stdcall RegSaveRestoreOnINFW(ptr wstr wstr wstr long long long)
66 stdcall RegSaveRestoreW(ptr wstr long wstr wstr wstr long)
67 stdcall RunSetupCommand(long str str str str ptr long ptr) RunSetupCommandA
68 stdcall RunSetupCommandA(long str str str str ptr long ptr)
69 stdcall RunSetupCommandW(long wstr wstr wstr wstr ptr long ptr)
70 stdcall SetPerUserSecValues(ptr) SetPerUserSecValuesA
71 stdcall SetPerUserSecValuesA(ptr)
72 stdcall SetPerUserSecValuesW(ptr)
73 stdcall TranslateInfString(str str str str ptr long ptr ptr) TranslateInfStringA
74 stdcall TranslateInfStringA(str str str str ptr long ptr ptr)
75 stdcall TranslateInfStringEx(long str str str str long ptr ptr) TranslateInfStringExA
76 stdcall TranslateInfStringExA(long str str str str long ptr ptr)
77 stdcall TranslateInfStringExW(long wstr wstr wstr wstr long ptr ptr)
78 stdcall TranslateInfStringW(wstr wstr wstr wstr ptr long ptr ptr)
79 stdcall UserInstStubWrapper(long long str long) UserInstStubWrapperA
80 stdcall UserInstStubWrapperA(long long str long)
81 stdcall UserInstStubWrapperW(long long wstr long)
82 stdcall UserUnInstStubWrapper(long long str long) UserUnInstStubWrapperA
83 stdcall UserUnInstStubWrapperA(long long str long)
84 stdcall UserUnInstStubWrapperW(long long wstr long)
