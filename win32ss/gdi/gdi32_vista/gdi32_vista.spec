@ stdcall D3DKMTCreateDCFromMemory(ptr)
@ stdcall D3DKMTDestroyDCFromMemory(ptr)

; XDDM Translation
@ stdcall -version=0x502 D3DKMTCloseAdapter(ptr)
@ stdcall -version=0x502 D3DKMTCreateDevice(ptr)
@ stdcall -version=0x502 D3DKMTDestroyDevice(ptr)
@ stdcall -version=0x502-0x601 D3DKMTOpenAdapterFromLuid(ptr)
@ stdcall -version=0x502 D3DKMTOpenAdapterFromGdiDisplayName(ptr)
@ stdcall -version=0x502 D3DKMTQueryAdapterInfo(ptr) NtGdiDdDDIQueryAdapterInfo
@ stdcall -version=0x502-0x601 D3DKMTQueryVideoMemoryInfo(ptr)
@ stdcall -version=0x502 D3DKMTSetVidPnSourceOwner(ptr)
