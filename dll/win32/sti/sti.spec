@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall StiCreateInstance(ptr long ptr ptr) StiCreateInstanceW
@ stdcall StiCreateInstanceA(ptr long ptr ptr)
@ stdcall StiCreateInstanceW(ptr long ptr ptr)
