#ifndef _PERFLIB_UTILS_H_
#define _PERFLIB_UTILS_H_

// query types

#define QUERY_GLOBAL       1
#define QUERY_ITEMS        2
#define QUERY_FOREIGN      3
#define QUERY_COSTLY       4
#define QUERY_COUNTER      5
#define QUERY_HELP         6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

// structure for passing to extensible counter open procedure wait thread

typedef struct _OPEN_PROC_WAIT_INFO {
    struct _OPEN_PROC_WAIT_INFO *pNext;
    LPWSTR  szLibraryName;
    LPWSTR  szServiceName;
    DWORD   dwWaitTime;
    DWORD   dwEventMsg;
    LPVOID  pData;
} OPEN_PROC_WAIT_INFO, FAR * LPOPEN_PROC_WAIT_INFO;

//#define PERFLIB_TIMING_THREAD_TIMEOUT  120000  // 2 min (in milliseconds)
#define PERFLIB_TIMING_THREAD_TIMEOUT   30000  // 30 sec (for debugging)

extern const   WCHAR GLOBAL_STRING[];
extern const   WCHAR COSTLY_STRING[];

extern const   DWORD VALUE_NAME_LENGTH;
extern const   WCHAR DisablePerformanceCounters[];


//
// function prototypes
//

LONG
GetPerflibKeyValue (
    LPCWSTR szItem,
    DWORD   dwRegType,
    DWORD   dwMaxSize,      // ... of pReturnBuffer in bytes
    LPVOID  pReturnBuffer,
    DWORD   dwDefaultSize,  // ... of pDefault in bytes
    LPVOID  pDefault
);

BOOL
MatchString (
    IN LPCWSTR lpValueArg,
    IN LPCWSTR lpNameArg
);


DWORD
GetQueryType (
    IN LPWSTR lpValue
);

DWORD
GetNextNumberFromList (
    IN LPWSTR   szStartChar,
    IN LPWSTR   *szNextChar
);

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
);

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
);

//
// Timer functions
//
HANDLE
StartPerflibFunctionTimer (
    IN  LPOPEN_PROC_WAIT_INFO pInfo
);

DWORD
KillPerflibFunctionTimer (
    IN  HANDLE  hPerflibTimer
);


DWORD
DestroyPerflibFunctionTimer (
);

LONG
PrivateRegQueryValueExT (
    HKEY    hKey,
    LPVOID  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    bUnicode
);

LONG
GetPerfDllFileInfo (
    LPCWSTR             szFileName,
    pDllValidationData  pDllData
);

DWORD
DisablePerfLibrary (
    pExtObject  pObj
);

#define PrivateRegQueryValueExW(a,b,c,d,e,f)    \
        PrivateRegQueryValueExT(a,(LPVOID)b,c,d,e,f,TRUE)

#define PrivateRegQueryValueExA(a,b,c,d,e,f)    \
        PrivateRegQueryValueExT(a,(LPVOID)b,c,d,e,f,FALSE)

#endif //_PERFLIB_UTILS_H_
