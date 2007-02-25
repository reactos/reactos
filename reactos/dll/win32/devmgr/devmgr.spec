# devmgr.dll exports

 5 stdcall DeviceProperties_RunDLLA(ptr ptr str long)
 6 stdcall DeviceProperties_RunDLLW(ptr ptr wstr long)
 7 stdcall DevicePropertiesA(ptr ptr str str long)
 8 stdcall DevicePropertiesW(ptr ptr wstr wstr long)
 9 stdcall DeviceManager_ExecuteA(ptr ptr str long)
10 stdcall DeviceManager_ExecuteW(ptr ptr wstr long)
11 stdcall DeviceProblemTextA(ptr long long str long)
12 stdcall DeviceProblemTextW(ptr long long wstr long)
13 stdcall DeviceProblemWizardA(ptr str str)
14 stdcall DeviceProblemWizardW(ptr wstr wstr)
15 stdcall DeviceManagerPrintA(str str long long ptr)
16 stdcall DeviceManagerPrintW(wstr wstr long long ptr)
17 stdcall DeviceAdvancedPropertiesA(ptr str str)
18 stdcall DeviceAdvancedPropertiesW(ptr wstr wstr)
19 stdcall DeviceCreateHardwarePage(ptr ptr)
20 stdcall DeviceCreateHardwarePageEx(ptr ptr long long)
21 stdcall DevicePropertiesExA(ptr str str long long)
22 stdcall DevicePropertiesExW(ptr wstr wstr long long)
23 stdcall DeviceProblenWizard_RunDLLA(ptr ptr str long) DeviceProblemWizard_RunDLLA
24 stdcall DeviceProblenWizard_RunDLLW(ptr ptr wstr long) DeviceProblemWizard_RunDLLW

25 stub DllCanUnloadNow
26 stub DllGetClassObject
27 stub DllRegisterServer
28 stub DllUnregisterServer
