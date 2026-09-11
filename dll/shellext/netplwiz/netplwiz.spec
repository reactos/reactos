1 stub AddNetPlaceRunDll
2 stub PassportWizardRunDll
3 stub PublishRunDll
4 stdcall UsersRunDll(ptr ptr ptr long) # NT5
@ stdcall UsersRunDllW(ptr ptr ptr long) # NT6
5 stdcall ClearAutoLogon()
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stub -private DllInstall
9 stdcall DllMain(ptr long ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
12 stub NetAccessWizard
13 stub NetPlacesWizardDoModal
14 stdcall SHDisconnectNetDrives(ptr)
