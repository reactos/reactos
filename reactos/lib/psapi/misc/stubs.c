/* $Id: stubs.c,v 1.3 2002/08/29 23:57:54 hyperion Exp $ */
#include <windows.h>
#include <psapi.h>

#if 0
BOOL STDCALL EnumPageFiles(
  PENUM_PAGE_CALLBACKW pCallbackRoutine,
  LPVOID lpContext
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}
#endif
DWORD STDCALL GetDeviceDriverBaseNameA(
  LPVOID ImageBase,  // driver load address
  LPSTR lpBaseName,  // driver base name buffer
  DWORD nSize        // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetDeviceDriverBaseNameW(
  LPVOID ImageBase,  // driver load address
  LPWSTR lpBaseName, // driver base name buffer
  DWORD nSize        // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetDeviceDriverFileNameA(
  LPVOID ImageBase,  // driver load address
  LPSTR lpFilename,  // path buffer
  DWORD nSize        // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetDeviceDriverFileNameW(
  LPVOID ImageBase,  // driver load address
  LPWSTR lpFilename, // path buffer
  DWORD nSize        // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetMappedFileNameA(
  HANDLE hProcess,    // handle to process
  LPVOID lpv,         // address to verify
  LPSTR lpFilename,   // file name buffer
  DWORD nSize         // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetMappedFileNameW(
  HANDLE hProcess,    // handle to process
  LPVOID lpv,         // address to verify
  LPWSTR lpFilename,  // file name buffer
  DWORD nSize         // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetModuleBaseNameA(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPSTR lpBaseName,   // base name buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetModuleBaseNameW(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPWSTR lpBaseName,  // base name buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetModuleFileNameExA(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPSTR lpFilename,   // path buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

DWORD STDCALL GetModuleFileNameExW(
  HANDLE hProcess,    // handle to process
  HMODULE hModule,    // handle to module
  LPWSTR lpFilename,  // path buffer
  DWORD nSize         // maximum characters to retrieve
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL GetModuleInformation(
  HANDLE hProcess,         // handle to process
  HMODULE hModule,         // handle to module
  LPMODULEINFO lpmodinfo,  // information buffer
  DWORD cb                 // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}
#if 0
BOOL STDCALL GetPerformanceInfo(
  PPERFORMANCE_INFORMATION pPerformanceInformation, 
  DWORD cb 

)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}
#endif
BOOL STDCALL GetProcessMemoryInfo(
  HANDLE Process,                          // handle to process
  PPROCESS_MEMORY_COUNTERS ppsmemCounters, // buffer
  DWORD cb                                 // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL GetWsChanges(
  HANDLE hProcess,                         // handle to process
  PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, // buffer
  DWORD cb                                 // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL InitializeProcessForWsWatch(
  HANDLE hProcess  // handle to process
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

BOOL STDCALL QueryWorkingSet(
  HANDLE hProcess,  // handle to process
  PVOID pv,         // information buffer
  DWORD cb          // size of buffer
)
{
 SetLastError(ERROR_INVALID_FUNCTION);
 return FALSE;
}

/* EOF */
