# ordinal exports
101 stdcall -noname IEWinMain(str long)

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall IEGetWriteableHKCU(ptr)
@ stdcall OpenURL(long long str long)
