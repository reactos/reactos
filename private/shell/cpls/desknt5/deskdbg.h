#ifdef __cplusplus
extern "C" {
#endif

#if DEBUG

#define LocalAlloc(flags, dwBytes)   DeskAllocPrivate(TEXT(__FILE__), __LINE__, flags, (dwBytes))
#define LocalReAlloc(hMem, dwBytes, flags)  DeskReAllocPrivate(TEXT(__FILE__), __LINE__, (hMem), (dwBytes), flags)
#define LocalFree(hMem)              DeskFreePrivate((hMem))
#define DeskCheckForLeaks()         DeskCheckForLeaksPrivate()
#define DirectLocalFree(hMem)       DeskFreeDirect((hMem))

#define ODS(sz) (OutputDebugStringA(sz), OutputDebugStringA("\r\n"))

#else

#define DirectLocalFree(hMem)       LocalFree((hMem))

#define ODS(sz)

#endif

HLOCAL
DeskAllocPrivate(const TCHAR *File, ULONG Line, ULONG Flags, DWORD dwBytes);

HLOCAL
DeskReAllocPrivate(const TCHAR *File, ULONG Line, HLOCAL hMem, DWORD dwBytes, ULONG Flags);

HLOCAL
DeskFreePrivate(HLOCAL hMem);

HLOCAL
DeskFreeDirect(HLOCAL hMem);

VOID
DeskCheckForLeaksPrivate(VOID);

#ifdef __cplusplus
}
#endif

