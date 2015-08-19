@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall CStartMenu_Constructor(ptr ptr)
@ stdcall CMenuDeskBar_Constructor(ptr ptr);
@ stdcall CMenuSite_Constructor(ptr ptr);
@ stdcall CMenuBand_Constructor(ptr ptr);
@ stdcall CMergedFolder_Constructor(ptr ptr);
@ stdcall ShellDDEInit(long);
@ stdcall SHCreateDesktop(ptr);
@ stdcall SHDesktopMessageLoop(ptr);
