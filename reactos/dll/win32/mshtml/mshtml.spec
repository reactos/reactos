@ stub CreateHTMLPropertyPage
@ stdcall -private DllCanUnloadNow()
@ stub DllEnumClassObjects
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub MatchExactGetIDsOfNames
@ stdcall PrintHTML(ptr ptr str long)
@ stdcall RNIGetCompatibleVersion()
@ stdcall RunHTMLApplication(long long str long)
@ stdcall ShowHTMLDialog(ptr ptr ptr wstr ptr)
@ stub ShowModalDialog
@ stub ShowModelessHTMLDialog

#Wine extension for Mozilla plugin support
@ stdcall NP_GetEntryPoints(ptr)
