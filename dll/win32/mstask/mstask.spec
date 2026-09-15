@ stdcall -stub ConvertAtJobsToTasks()
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -stub GetNetScheduleAccountInformation(wstr long ptr)
@ stdcall NetrJobAdd(wstr ptr ptr) NetrJobAdd_wrapper
@ stdcall NetrJobDel(wstr long long) NetrJobDel_wrapper
@ stdcall NetrJobEnum(wstr ptr long ptr ptr) NetrJobEnum_wrapper
@ stdcall NetrJobGetInfo(wstr long ptr) NetrJobGetInfo_wrapper
@ stdcall -stub SAGetAccountInformation(wstr ptr long ptr)
@ stdcall -stub SAGetNSAccountInformation(wstr long ptr)
@ stdcall -stub SASetAccountInformation(wstr wstr wstr wstr long)
@ stdcall -stub SASetNSAccountInformation(wstr wstr wstr)
@ stdcall -stub SetNetScheduleAccountInformation(wstr wstr wstr)
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
