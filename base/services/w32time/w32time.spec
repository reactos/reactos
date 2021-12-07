@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
6 stdcall SvchostEntry_W32Time(long ptr) W32TmServiceMain
18 stdcall W32TimeSyncNow(wstr long long)
21 stdcall W32TmServiceMain(long ptr)
