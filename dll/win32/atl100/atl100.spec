10 stdcall AtlAdvise(ptr ptr ptr ptr)
11 stdcall AtlUnadvise(ptr ptr long)
12 stdcall AtlFreeMarshalStream(ptr)
13 stdcall AtlMarshalPtrInProc(ptr ptr ptr)
14 stdcall AtlUnmarshalPtr(ptr ptr ptr)
15 stub AtlComModuleGetClassObject
17 stub AtlComModuleRegisterClassObjects
20 stub AtlComModuleRevokeClassObjects
22 stub AtlComModuleUnregisterServer
23 stub AtlUpdateRegistryFromResourceD
24 stub AtlWaitWithMessageLoop
25 stub AtlSetErrorInfo
26 stdcall AtlCreateTargetDC(long ptr)
27 stdcall AtlHiMetricToPixel(ptr ptr)
28 stdcall AtlPixelToHiMetric(ptr ptr)
29 stub AtlDevModeW2A
30 stdcall AtlComPtrAssign(ptr ptr)
31 stdcall AtlComQIPtrAssign(ptr ptr ptr)
32 stdcall AtlInternalQueryInterface(ptr ptr ptr ptr)
34 stdcall AtlGetVersion(ptr)
35 stub AtlAxDialogBoxW
36 stub AtlAxDialogBoxA
37 stdcall AtlAxCreateDialogW(long wstr long ptr long)
38 stdcall AtlAxCreateDialogA(long str long ptr long)
39 stdcall AtlAxCreateControl(ptr ptr ptr ptr)
40 stdcall AtlAxCreateControlEx(ptr ptr ptr ptr ptr ptr ptr)
41 stdcall AtlAxAttachControl(ptr ptr ptr)
42 stdcall AtlAxWinInit()
43 stub AtlWinModuleAddCreateWndData
44 stub AtlWinModuleExtractCreateWndData
45 stub AtlWinModuleRegisterWndClassInfoW
46 stub AtlWinModuleRegisterWndClassInfoA
47 stdcall AtlAxGetControl(long ptr)
48 stdcall AtlAxGetHost(long ptr)
49 stub AtlRegisterClassCategoriesHelper
50 stdcall AtlIPersistStreamInit_Load(ptr ptr ptr ptr)
51 stdcall AtlIPersistStreamInit_Save(ptr long ptr ptr ptr)
52 stub AtlIPersistPropertyBag_Load
53 stub AtlIPersistPropertyBag_Save
54 stub AtlGetObjectSourceInterface
56 stub AtlLoadTypeLib
58 stub AtlModuleAddTermFunc
59 stub AtlAxCreateControlLic
60 stub AtlAxCreateControlLicEx
61 stdcall AtlCreateRegistrar(ptr)
62 stub AtlWinModuleRegisterClassExW
63 stub AtlWinModuleRegisterClassExA
64 stub AtlCallTermFunc
65 stub AtlWinModuleInit
66 stub AtlWinModuleTerm
67 stub AtlSetPerUserRegistration
68 stub AtlGetPerUserRegistration
