
1 stub DoInitialCleanup
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
6 stub HrCreateDesktopIcon
7 stub HrGetAnswerFileParametersForNetCard
8 stub HrGetExtendedStatusFromNCS
9 stub HrGetIconFromMediaType
10 stub HrGetInstanceGuidOfPreNT5NetCardInstance
11 stub HrGetNetConExtendedStatusFromGuid
12 stub HrGetNetConExtendedStatusFromINetConnection
13 stub HrGetStatusStringFromNetConExtendedStatus
14 stub HrIsIpStateCheckingEnabled
15 stub HrLaunchConnection
16 stub HrLaunchConnectionEx
17 stub HrLaunchNetworkOptionalComponents
18 stub HrOemUpgrade
19 stub HrRenameConnection
20 stub HrRunWizard
21 stub InvokeDunFile
22 stdcall NcFreeNetconProperties(ptr)
23 stub NcIsValidConnectionName
24 stub NetSetupAddRasConnection
25 stub NetSetupFinishInstall
26 stub NetSetupInstallSoftware
27 stub NetSetupPrepareSysPrep
28 stub NetSetupRequestWizardPages
29 stub NetSetupSetProgressCallback
30 stub NormalizeExtendedStatus
31 stub RaiseSupportDialog
32 stub RepairConnection
33 stub StartNCW
