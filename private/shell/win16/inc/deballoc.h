#pragma message("### Building FULL_DEBUG version ###")
#define LocalAlloc	DebugLocalAlloc
#define LocalReAlloc	DebugLocalReAlloc
#define LocalFree	DebugLocalFree
HLOCAL WINAPI DebugLocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL WINAPI DebugLocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags);
HLOCAL WINAPI DebugLocalFree( HLOCAL hMem );




