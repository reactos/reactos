;; DLL installation
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()

;; Winlogon notification event handlers
@ stdcall WLEventStartup(ptr)
@ stdcall WLEventShutdown(ptr)
@ stdcall WLEventLogon(ptr)
@ stdcall WLEventLogoff(ptr)
@ stdcall WLEventStartShell(ptr)
@ stdcall WLEventPostShell(ptr)
@ stdcall WLEventLock(ptr)
@ stdcall WLEventUnlock(ptr)
@ stdcall WLEventStartScreenSaver(ptr)
@ stdcall WLEventStopScreenSaver(ptr)
@ stdcall WLEventDisconnect(ptr)
@ stdcall WLEventReconnect(ptr)
