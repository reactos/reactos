@ stub DDHAL32_VidMemAlloc
@ stub DDHAL32_VidMemFree
@ stub DDInternalLock
@ stub DDInternalUnlock
@ stub DSoundHelp
@ stdcall DirectDrawCreate(ptr ptr ptr)
@ stdcall DirectDrawCreateClipper(long ptr ptr)
@ stdcall DirectDrawCreateEx(ptr ptr ptr ptr)
@ stdcall DirectDrawEnumerateA(ptr ptr)
@ stdcall DirectDrawEnumerateExA(ptr ptr long)
@ stub DirectDrawEnumerateExW
@ stub DirectDrawEnumerateW
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub GetNextMipMap
@ stub GetSurfaceFromDC
@ stub HeapVidMemAllocAligned
@ stub InternalLock
@ stub InternalUnlock
@ stub LateAllocateSurfaceMem
@ stub VidMemAlloc
@ stub VidMemAmountFree
@ stub VidMemFini
@ stub VidMemFree
@ stub VidMemInit
@ stub VidMemLargestFree
@ stdcall D3DParseUnknownCommand(ptr ptr)
