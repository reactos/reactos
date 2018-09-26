@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
6 stub HrDiAddComponentToINetCfg
7 stub LanaCfgFromCommandArgs
8 stub ModemClassCoInstaller
9 stub NetCfgDiagFromCommandArgs
10 stub NetCfgDiagRepairRegistryBindings
11 stdcall NetClassInstaller(long ptr ptr)
12 stdcall NetPropPageProvider(ptr ptr long)
13 stub RasAddBindings
14 stub RasCountBindings
15 stub RasRemoveBindings
16 stub SvchostChangeSvchostGroup
17 stub UpdateLanaConfigUsingAnswerfile