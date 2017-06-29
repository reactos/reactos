@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall CStartMenu_CreateInstance(ptr ptr) RSHELL_CStartMenu_CreateInstance
@ stdcall CMenuDeskBar_CreateInstance(ptr ptr) RSHELL_CMenuDeskBar_CreateInstance
@ stdcall CMenuSite_CreateInstance(ptr ptr) RSHELL_CMenuSite_CreateInstance
@ stdcall CMenuBand_CreateInstance(ptr ptr) RSHELL_CMenuBand_CreateInstance
@ stdcall CMergedFolder_CreateInstance(ptr ptr) RSHELL_CMergedFolder_CreateInstance
@ stdcall CBandSite_CreateInstance(ptr ptr ptr) RSHELL_CBandSite_CreateInstance
@ stdcall CBandSiteMenu_CreateInstance(ptr ptr) RSHELL_CBandSiteMenu_CreateInstance
@ stdcall ShellDDEInit(long);
@ stdcall SHCreateDesktop(ptr);
@ stdcall SHDesktopMessageLoop(ptr);
