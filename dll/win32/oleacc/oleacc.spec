@ stdcall AccessibleChildren(ptr long long ptr ptr)
@ stdcall AccessibleObjectFromEvent(ptr long long ptr ptr)
@ stdcall AccessibleObjectFromPoint(int64 ptr ptr)
@ stdcall AccessibleObjectFromWindow(ptr long ptr ptr)
@ stdcall CreateStdAccessibleObject(ptr long ptr ptr)
@ stub CreateStdAccessibleProxyA
@ stub CreateStdAccessibleProxyW
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall GetOleaccVersionInfo(ptr ptr)
@ stdcall GetProcessHandleFromHwnd(ptr)
@ stdcall GetRoleTextA(long ptr long)
@ stdcall GetRoleTextW(long ptr long)
@ stdcall GetStateTextA(long ptr long)
@ stdcall GetStateTextW(long ptr long)
@ extern IID_IAccessible
@ extern IID_IAccessibleHandler
@ extern LIBID_Accessibility
@ stdcall LresultFromObject(ptr long ptr)
@ stdcall ObjectFromLresult(long ptr long ptr)
@ stdcall WindowFromAccessibleObject(ptr ptr)
