3 stdcall ADsGetObject(wstr ptr ptr)
4 stdcall ADsBuildEnumerator(ptr ptr)
5 stdcall ADsFreeEnumerator(ptr)
6 stdcall ADsEnumerateNext(ptr long ptr ptr)
7 stdcall ADsBuildVarArrayStr(ptr long ptr)
8 stdcall ADsBuildVarArrayInt(ptr long ptr)
9 stdcall ADsOpenObject(wstr wstr wstr long ptr ptr)
12 stdcall ADsSetLastError(long ptr ptr) # adsldpc.ADsSetLastError
13 stdcall ADsGetLastError(ptr ptr long ptr long) # adsldpc.ADsGetLastError
14 stdcall AllocADsMem(long) # adsldpc.AllocADsMem
15 stdcall FreeADsMem(ptr) # adsldpc.FreeADsMem
16 stdcall ReallocADsMem(ptr long long) # adsldpc.ReallocADsMem
17 stdcall AllocADsStr(ptr) # adsldpc.AllocADsStr
18 stdcall FreeADsStr(ptr) # adsldpc.FreeADsStr
19 stdcall ReallocADsStr(ptr ptr) # adsldpc.ReallocADsStr
20 stdcall ADsEncodeBinaryData(ptr long ptr) # adsldpc.ADsEncodeBinaryData
21 stdcall PropVariantToAdsType(ptr long ptr ptr)
22 stdcall AdsTypeToPropVariant(ptr long ptr)
23 stdcall AdsFreeAdsValues(ptr long)
24 stdcall ADsDecodeBinaryData(wstr ptr long) # adsldpc.ADsDecodeBinaryData
25 cdecl AdsTypeToPropVariant2() # unknown prototype
26 cdecl PropVariantToAdsType2() # unknown prototype
27 cdecl ConvertSecDescriptorToVariant() # unknown prototype
28 cdecl ConvertSecurityDescriptorToSecDes() # unknown prototype
29 stdcall BinarySDToSecurityDescriptor(ptr ptr wstr wstr wstr long)
30 stdcall SecurityDescriptorToBinarySD(long long long long ptr ptr wstr wstr wstr long)
31 cdecl -version=0x600+ ConvertTrusteeToSid() # unknown prototype

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
