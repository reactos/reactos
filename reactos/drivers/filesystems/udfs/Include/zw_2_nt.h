////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __Zw_to_Nt__NameConvert__H__
#define __Zw_to_Nt__NameConvert__H__

#ifdef NT_NATIVE_MODE

#define ZwClose          NtClose
#define ZwOpenKey        NtOpenKey
#define ZwQueryValueKey  NtQueryValueKey

//#define ExAllocatePool(hernya,size) MyGlobalAlloc(size)
//#define ExFreePool(addr)            MyGlobalFree((PVOID)(addr))

#endif //NT_NATIVE_MODE

#endif //__Zw_to_Nt__NameConvert__H__
