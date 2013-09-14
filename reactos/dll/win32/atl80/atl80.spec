10 stdcall AtlAdvise(ptr ptr ptr ptr) atl100.AtlAdvise
11 stdcall AtlUnadvise(ptr ptr long) atl100.AtlUnadvise
12 stdcall AtlFreeMarshalStream(ptr) atl100.AtlFreeMarshalStream
13 stdcall AtlMarshalPtrInProc(ptr ptr ptr) atl100.AtlMarshalPtrInProc
14 stdcall AtlUnmarshalPtr(ptr ptr ptr) atl100.AtlUnmarshalPtr
15 stdcall AtlComModuleGetClassObject(ptr ptr ptr ptr) atl100.AtlComModuleGetClassObject
17 stdcall AtlComModuleRegisterClassObjects(ptr long long) atl100.AtlComModuleRegisterClassObjects
18 stdcall AtlComModuleRegisterServer(ptr long ptr)
19 stdcall AtlRegisterTypeLib(ptr wstr)
20 stub AtlComModuleRevokeClassObjects
22 stdcall AtlComModuleUnregisterServer(ptr long ptr) atl100.AtlComModuleUnregisterServer
23 stdcall AtlUpdateRegistryFromResourceD(long wstr long ptr ptr) atl100.AtlUpdateRegistryFromResourceD
24 stdcall AtlWaitWithMessageLoop(long) atl100.AtlWaitWithMessageLoop
25 stub AtlSetErrorInfo
26 stdcall AtlCreateTargetDC(long ptr) atl100.AtlCreateTargetDC
27 stdcall AtlHiMetricToPixel(ptr ptr) atl100.AtlHiMetricToPixel
28 stdcall AtlPixelToHiMetric(ptr ptr) atl100.AtlPixelToHiMetric
29 stub AtlDevModeW2A
30 stdcall AtlComPtrAssign(ptr ptr) atl100.AtlComPtrAssign
31 stdcall AtlComQIPtrAssign(ptr ptr ptr) atl100.AtlComQIPtrAssign
32 stdcall AtlInternalQueryInterface(ptr ptr ptr ptr) atl100.AtlInternalQueryInterface
34 stdcall AtlGetVersion(ptr)
35 stdcall AtlAxDialogBoxW(long wstr long ptr long) atl100.AtlAxDialogBoxW
36 stdcall AtlAxDialogBoxA(long str long ptr long) atl100.AtlAxDialogBoxA
37 stdcall AtlAxCreateDialogW(long wstr long ptr long) atl100.AtlAxCreateDialogW
38 stdcall AtlAxCreateDialogA(long str long ptr long) atl100.AtlAxCreateDialogA
39 stdcall AtlAxCreateControl(ptr ptr ptr ptr) atl100.AtlAxCreateControl
40 stdcall AtlAxCreateControlEx(ptr ptr ptr ptr ptr ptr ptr) atl100.AtlAxCreateControlEx
41 stdcall AtlAxAttachControl(ptr ptr ptr) atl100.AtlAxAttachControl
42 stdcall AtlAxWinInit()
43 stdcall AtlWinModuleAddCreateWndData(ptr ptr ptr) atl100.AtlWinModuleAddCreateWndData
44 stdcall AtlWinModuleExtractCreateWndData(ptr) atl100.AtlWinModuleExtractCreateWndData
45 stub AtlWinModuleRegisterWndClassInfoW
46 stub AtlWinModuleRegisterWndClassInfoA
47 stdcall AtlAxGetControl(long ptr) atl100.AtlAxGetControl
48 stdcall AtlAxGetHost(long ptr) atl100.AtlAxGetHost
49 stdcall AtlRegisterClassCategoriesHelper(ptr ptr long) atl100.AtlRegisterClassCategoriesHelper
50 stdcall AtlIPersistStreamInit_Load(ptr ptr ptr ptr) atl100.AtlIPersistStreamInit_Load
51 stdcall AtlIPersistStreamInit_Save(ptr long ptr ptr ptr) atl100.AtlIPersistStreamInit_Save
52 stdcall AtlIPersistPropertyBag_Load(ptr ptr ptr ptr ptr) atl100.AtlIPersistPropertyBag_Load
53 stub AtlIPersistPropertyBag_Save
54 stdcall AtlGetObjectSourceInterface(ptr ptr ptr ptr ptr) atl100.AtlGetObjectSourceInterface
55 stub AtlUnRegisterTypeLib
56 stdcall AtlLoadTypeLib(long wstr ptr ptr) atl100.AtlLoadTypeLib
58 stdcall AtlModuleAddTermFunc(ptr ptr long) atl100.AtlModuleAddTermFunc
59 stub AtlAxCreateControlLic
60 stub AtlAxCreateControlLicEx
61 stdcall AtlCreateRegistrar(ptr) atl100.AtlCreateRegistrar
62 stub AtlWinModuleRegisterClassExW
63 stub AtlWinModuleRegisterClassExA
64 stdcall AtlCallTermFunc(ptr) atl100.AtlCallTermFunc
65 stdcall AtlWinModuleInit(ptr) atl100.AtlWinModuleInit
66 stub AtlWinModuleTerm
