@ stdcall ConvertAtJobsToTasks()
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall GetNetScheduleAccountInformation(wstr long ptr)
@ stdcall NetrJobAdd(wstr ptr ptr)
@ stdcall NetrJobDel(wstr long long)
@ stdcall NetrJobEnum(wstr ptr long ptr ptr)
@ stdcall NetrJobGetInfo(wstr long ptr)
@ stdcall SAGetAccountInformation(wstr ptr long ptr)
@ stdcall SAGetNSAccountInformation(wstr long ptr)
@ stdcall SASetAccountInformation(wstr wstr wstr wstr long)
@ stdcall SASetNSAccountInformation(wstr wstr wstr)
@ stdcall SetNetScheduleAccountInformation(wstr wstr wstr)
#@ stdcall _ConvertAtJobsToTasks@0() mstask.ConvertAtJobsToTasks
#@ stdcall _DllCanUnloadNow@0() DllCanUnloadNow
#@ stdcall _DllGetClassObject@12(ptr ptr ptr) DllGetClassObject
#@ stdcall _GetNetScheduleAccountInformation@12(wstr ptr long ptr) GetNetScheduleAccountInformation
#@ stdcall _NetrJobAdd@12(wstr ptr ptr) NetrJobAdd
#@ stdcall _NetrJobDel@12(wstr long long) NetrJobDel
#@ stdcall _NetrJobEnum@20(wstr ptr long ptr ptr) NetrJobEnum
#@ stdcall _NetrJobGetInfo@12(wstr long ptr) NetrJobGetInfo
#@ stdcall _SAGetAccountInformation@16(wstr ptr long ptr) SAGetAccountInformation
#@ stdcall _SAGetNSAccountInformation@12(wstr long ptr) SAGetNSAccountInformation
#@ stdcall _SASetAccountInformation@20(wstr wstr wstr wstr long) SASetAccountInformation
#@ stdcall _SASetNSAccountInformation@12(wstr wstr wstr) SASetNSAccountInformation
#@ stdcall _SetNetScheduleAccountInformation@12(wstr wstr wstr) SetNetScheduleAccountInformation
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
