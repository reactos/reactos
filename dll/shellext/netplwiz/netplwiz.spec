1 stub AddNetPlaceRunDll
2 stub PassportWizardRunDll
3 stub PublishRunDll
4 stub UsersRunDll
5 stub ClearAutoLogon
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
8 stub -private DllInstall
9 stdcall DllMain(ptr long ptr)
@ stub -private DllRegisterServer
@ stub -private DllUnregisterServer
12 stub NetAccessWizard
13 stub NetPlacesWizardDoModal
14 stdcall SHDisconnectNetDrives(ptr)
