# ordinal exports
101 stdcall -noname IEWinMain(wstr long)

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall IEGetWriteableHKCU(ptr)
@ stdcall IERefreshElevationPolicy()
@ stdcall OpenURL(long long str long)
@ stdcall SetQueryNetSessionCount(long)
