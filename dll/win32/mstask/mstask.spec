@ stdcall ConvertAtJobsToTasks()
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall GetNetScheduleAccountInformation(ptr long ptr)
@ stdcall NetrJobAdd(ptr ptr ptr)
@ stdcall NetrJobDel(ptr long long)
@ stdcall NetrJobEnum(ptr ptr long ptr ptr)
@ stdcall NetrJobGetInfo(ptr long ptr)
@ stdcall SAGetAccountInformation(ptr ptr long ptr)
@ stdcall SAGetNSAccountInformation(ptr long ptr)
@ stdcall SASetAccountInformation(ptr wstr wstr wstr long)
@ stdcall SASetNSAccountInformation(ptr wstr wstr)
@ stdcall SetNetScheduleAccountInformation(wstr wstr wstr)
#@ stdcall _ConvertAtJobsToTasks@0() mstask.ConvertAtJobsToTasks
#@ stdcall _DllCanUnloadNow@0() DllCanUnloadNow
#@ stdcall _DllGetClassObject@12(ptr ptr ptr) DllGetClassObject
#@ stdcall _GetNetScheduleAccountInformation@12(ptr ptr long ptr) GetNetScheduleAccountInformation
#@ stdcall _NetrJobAdd@12(ptr ptr ptr) NetrJobAdd
#@ stdcall _NetrJobDel@12(ptr long long) NetrJobDel
#@ stdcall _NetrJobEnum@20(ptr ptr long ptr ptr) NetrJobEnum
#@ stdcall _NetrJobGetInfo@12(ptr long ptr) NetrJobGetInfo
#@ stdcall _SAGetAccountInformation@16(ptr ptr long ptr) SAGetAccountInformation
#@ stdcall _SAGetNSAccountInformation@12(ptr long ptr) SAGetNSAccountInformation
#@ stdcall _SASetAccountInformation@20(ptr wstr wstr wstr long) SASetAccountInformation
#@ stdcall _SASetNSAccountInformation@12(ptr wstr wstr) SASetNSAccountInformation
#@ stdcall _SetNetScheduleAccountInformation@12(wstr wstr wstr) SetNetScheduleAccountInformation
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
