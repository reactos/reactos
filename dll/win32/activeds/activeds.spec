3 stdcall ADsGetObject(wstr ptr ptr)
4 stdcall ADsBuildEnumerator(ptr ptr)
5 stub ADsFreeEnumerator
6 stdcall ADsEnumerateNext(ptr long ptr ptr)
7 stub ADsBuildVarArrayStr
8 stub ADsBuildVarArrayInt
9 stdcall ADsOpenObject(wstr wstr wstr long ptr ptr)
12 stub ADsSetLastError
13 stub ADsGetLastError
14 stub AllocADsMem
15 stdcall FreeADsMem(ptr)
16 stub ReallocADsMem
17 stub AllocADsStr
18 stub FreeADsStr
19 stub ReallocADsStr
20 stub ADsEncodeBinaryData
21 stub PropVariantToAdsType
22 stub AdsTypeToPropVariant
23 stub AdsFreeAdsValues
24 stub ADsDecodeBinaryData
25 stub AdsTypeToPropVariant2
26 stub PropVariantToAdsType2
27 stub ConvertSecDescriptorToVariant
28 stub ConvertSecurityDescriptorToSecDes
#@ stub DllCanUnloadNow
#@ stub DllGetClassObject
#@ stub DllRegisterServer
#@ stub DllUnregisterServer
