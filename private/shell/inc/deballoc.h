#if 0
#pragma message("### Building FULL_DEBUG version ###")
#undef LocalAlloc
#undef LocalReAlloc
#undef LocalFree
#define LocalAlloc      DebugLocalAlloc
#define LocalReAlloc    DebugLocalReAlloc
#define LocalFree       DebugLocalFree

#ifdef __cplusplus
extern "C" {
#endif

HLOCAL WINAPI DebugLocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL WINAPI DebugLocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags);
HLOCAL WINAPI DebugLocalFree( HLOCAL hMem );

#ifdef __cplusplus
};
#endif

#endif
