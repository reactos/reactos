/*
 *  M A P I W I N . H
 *
 *  Definitions used by the MAPI Development Team to aid in
 *  developing single-source service providers that run on
 *  both WIN32 and WIN16 platforms.
 *  There are three sections.
 *
 *  The first section defines how to call something that
 *  is available by different methods in WIN16 vs. WIN32.
 *  As such, they are totally new mechanisms.
 *
 *  The second section establishes things that are available
 *  AS-IS in one environment but we have to define for the
 *  other environment.
 *
 *  The third section simply defines a few conventions
 *  (simplifications) for common operations.
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

/*
 *  Routines are included in the first section to manage per-instance
 *  global variables for DLLs. They assume that all of the DLL's
 *  per-instance global variables live in a single block of memory.
 *  Functions are provided to install and retrieve the correct block of
 *  memory for the current instance.
 *
 *  There are only two functions:
 *
 *      PvGetInstanceGlobals    Call this to get the address of the
 *                              per-instance globals structure.
 *      ScSetinstanceGlobals    Call this to install the
 *                              per-instance globals structure. It
 *                              may fail if the number of instances
 *                              exceeds a certain limit.
 *
 *  The caller is free to choose the name, size, and allocation
 *  method of the per-instance global variables structure.
 *
 *  The WIN32 implementation uses a pointer in the DLL's data
 *  segment. This assumes that the DLL gets a separate instance
 *  of the default data segment per calling process.
 *
 *  The WIN16 implementation uses a fixed array of pointers and a
 *  matching fixed array of keys unique to the calling process.
 */

/*
 *  The second section consists largely of Win32 file I/O functions
 *  that are not supported under Win16. These functions are
 *  implemented in mapiwin.c, using DOS calls. Most have limitations
 *  relative to their Win32 counterparts, which are spelled out in
 *  the comments to the source code.
 */

#ifndef __MAPIWIN_H__
#define __MAPIWIN_H__

#if defined (WIN32) && !defined (_WIN32)
#define _WIN32
#endif

#include "mapinls.h"

#ifdef __cplusplus
extern "C" {
#endif


/********************************/
/*  Our conventions for things  */
/*  we choose to do differently */
/*  on WIN16 vs. WIN32.         */
/********************************/

#ifdef  WIN16

#define MULDIV(x,y,z)               MulDiv32(x,y,z)
#define IsBadReadPtr(lp,cb)         FBadReadPtr(lp,cb)

#define cInstMax                    50
LPVOID FAR PASCAL   PvGetInstanceGlobals(void);
LONG FAR PASCAL     ScSetInstanceGlobals(LPVOID pv);
LONG FAR PASCAL     ScSetVerifyInstanceGlobals(LPVOID pv, DWORD dwPid);
LPVOID FAR PASCAL   PvGetVerifyInstanceGlobals(DWORD dwPid);
LPVOID FAR PASCAL   PvSlowGetInstanceGlobals(DWORD dwPid);
BOOL __export FAR PASCAL FCleanupInstanceGlobals(WORD, DWORD);

#elif defined(_MAC) /* !WIN16 */

#define MULDIV(x,y,z)               MulDiv(x,y,z)

LPVOID FAR PASCAL   PvGetInstanceGlobals(WORD wDataSet);
LONG FAR PASCAL     ScSetInstanceGlobals(LPVOID pv, WORD wDataSet);
LONG FAR PASCAL     ScSetVerifyInstanceGlobals(LPVOID pv, DWORD dwPid,
                        WORD wDataSet);
LPVOID FAR PASCAL   PvGetVerifyInstanceGlobals(DWORD dwPid, DWORD wDataSet);
LPVOID FAR PASCAL   PvSlowGetInstanceGlobals(DWORD dwPid, DWORD wDataSet);
BOOL FAR PASCAL     FCleanupInstanceGlobals(WORD, DWORD);

#else   /* !WIN16 */

#define MULDIV(x,y,z)               MulDiv(x,y,z)

extern LPVOID pinstX;
#define PvGetInstanceGlobals()                  pinstX
#define ScSetInstanceGlobals(_pv)               (pinstX = _pv, 0)
#define PvGetVerifyInstanceGlobals(_pid)        pinstX
#define ScSetVerifyInstanceGlobals(_pv,_pid)    (pinstX = _pv, 0)
#define PvSlowGetInstanceGlobals(_pid)          pinstX

#endif  /* WIN16 */

#if defined(_WIN32) && !defined(_MAC)
#define szMAPIDLLSuffix     "32"
#elif defined(WIN16) || defined(DOS)
#define szMAPIDLLSuffix     ""
#elif  defined(_MAC)
#define szMAPIDLLSuffix     "M"
#else
#error "Don't know the suffix for DLLs on this platform"
#endif

/********************************/
/*  Things missing from one     */
/*  system-provided environment */
/*  or the other.               */
/********************************/

#if !defined(_WIN32) 
#define ZeroMemory(pb,cb)           memset((pb),0,(cb))
#define FillMemory(pb,cb,b)         memset((pb),(b),(cb))
#define CopyMemory(pbDst,pbSrc,cb)  do                              \
                                    {                               \
                                        size_t _cb = (size_t)(cb);  \
                                        if (_cb)                    \
                                            memcpy(pbDst,pbSrc,_cb);\
                                    } while (FALSE)
#define MoveMemory(pbDst,pbSrc,cb)  memmove((pbDst),(pbSrc),(cb))

#define UNALIGNED

#endif

#if defined(WIN16) || defined(_MAC)

#ifndef _MAC
#include <error.h>              /*  for GetLastError() */
#endif

typedef int                 INT;
typedef unsigned long       ULONG;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef double              LONGLONG;
typedef double              DWORDLONG;
typedef unsigned char       UCHAR;
typedef unsigned char FAR*  PUCHAR;
typedef int                 BOOL;


#ifndef _MAC
typedef char                BOOLEAN;

#ifndef _FILETIME_
#define _FILETIME_
typedef struct tagFILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;
#endif      /* _FILETIME */

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, FAR *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    CHAR StandardName[ 32 ];        /* was WCHAR */
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    CHAR DaylightName[ 32 ];        /* was WCHAR */
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, FAR *LPTIME_ZONE_INFORMATION;


#if defined(DOS) || defined(WIN16)
/* Simulate effect of afx header */
#define __T(x)      x
#define _T(x)       __T(x)
#define TEXT        _T
#endif

#define APIENTRY        WINAPI

#define SetForegroundWindow         SetActiveWindow

#define wsprintfA                   wsprintf
#define GetWindowsDirectoryA        GetWindowsDirectory
#define GetSystemDirectoryA         GetSystemDirectory
#define GetPrivateProfileStringA    GetPrivateProfileString
#define GetPrivateProfileIntA       GetPrivateProfileInt
#define GetProfileStringA           GetProfileString
#define GetModuleFileNameA          GetModuleFileName
#define CharUpperBuffA              CharUpperBuff
#define LoadLibraryA                LoadLibrary
#define lstrcatA                    lstrcat
#define RegisterWindowMessageA      RegisterWindowMessage
#define MAKEINTRESOURCEA            MAKEINTRESOURCE

#define WNDCLASSA                   WNDCLASS                                    

#endif  /* !_MAC */

/* Synchronization */
#define InterlockedIncrement(plong) (++(*(plong)))
#define InterlockedDecrement(plong) (--(*(plong)))

#ifndef CreateMutex
#define CreateMutexA    CreateMutex
#define CreateMutexW    CreateMutex
#define CreateMutex(pv, bool, sz)   (INVALID_HANDLE_VALUE)
#endif

#define WaitForSingleObject(hObj, dw)   ((void)0)
#define ReleaseMutex(hObj)              ((BOOL)1)
#define CloseMutexHandle(hObj)          TRUE

#define CRITICAL_SECTION            ULONG
#define InitializeCriticalSection(_pcs) ((void)0)
#define DeleteCriticalSection(_pcs)     ((void)0)
#define EnterCriticalSection(_pcs)      ((void)0)
#define LeaveCriticalSection(_pcs)      ((void)0)

#define MAX_PATH                    260

#ifndef _MAC
/*
 *  File Access Modes
 *
 *  The possible combination of file access modes as passed into
 *  the CreateFile() api map to OpenFile() as follows:
 *
 *   GENERIC_READ                       OPEN_ACCESS_READONLY
 *   GENERIC_WRITE                      OPEN_ACCESS_WRITEONLY
 *   GENERIC_READ | GENERIC_WRITE       OPEN_ACCESS_READWRITE
 *
 *   0                                  OPEN_SHARE_DENYREADWRITE
 *   FILE_SHARE_READ                    OPEN_SHARE_DENYWRITE
 *   FILE_SHARE_WRITE                   OPEN_SHARE_DENYREAD
 *   FILE_SHARE_READ | FILE_SHARE_WRITE OPEN_SHARE_DENYNONE
 *
 *  Due to the mappings we cannot pass them through directly,
 *  so we will have to use a conversion within APIs that test
 *  these bits.  It would be best to use the Win32 #defines
 *  for these flags and convert as needed in the APIs.
 */
#define GENERIC_READ                (0x80000000) /* from WINNT.H */
#define GENERIC_WRITE               (0x40000000) /* from WINNT.H */
#define FILE_SHARE_READ             (0x00000001) /* from WINNT.H */
#define FILE_SHARE_WRITE            (0x00000002) /* from WINNT.H */
#endif  /* _MAC */

#define FILE_FLAG_SEQUENTIAL_SCAN   0x08000000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#ifndef _MAC
#define INVALID_HANDLE_VALUE        ((HANDLE)(-1))
#define DELETE                      0x00010000L

#define FILE_BEGIN                  0
#define FILE_CURRENT                1
#define FILE_END                    2
#endif

#define FILE_ATTRIBUTE_READONLY         0x00000001
#define FILE_ATTRIBUTE_HIDDEN           0x00000002
#define FILE_ATTRIBUTE_SYSTEM           0x00000004
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020
#define FILE_ATTRIBUTE_NORMAL           0x00000080
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100

#define FILE_FLAG_WRITE_THROUGH     0x80000000
#define FILE_FLAG_RANDOM_ACCESS     0x10000000

#ifndef _MAC
typedef struct _WIN32_FIND_DATA {
    DWORD       dwFileAttributes;
    FILETIME    ftCreationTime;
    FILETIME    ftLastAccessTime;
    FILETIME    ftLastWriteTime;
    DWORD       nFileSizeHigh;
    DWORD       nFileSizeLow;
    DWORD       dwReserved0;
    DWORD       dwReserved1;
    CHAR        cFileName[ MAX_PATH ];
    CHAR        cAlternateFileName[ 16 ];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

#define TIME_ZONE_ID_INVALID        0xFFFFFFFF
#endif
#define TIME_ZONE_ID_UNKNOWN        0
#define TIME_ZONE_ID_STANDARD       1
#define TIME_ZONE_ID_DAYLIGHT       2



DWORD WINAPI    GetLastError(void);
DWORD WINAPI    GetFileAttributes(LPCSTR lpFileName);
DWORD WINAPI    GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL WINAPI     GetFileTime(HANDLE hFile, FILETIME FAR *lpftCreation,
                FILETIME FAR *lpftLastAccess, FILETIME FAR *lpftLastWrite);
BOOL WINAPI     SetFileTime(HANDLE hFile, const FILETIME FAR *lpftCreation,
                const FILETIME FAR *lpftLastAccess,
                const FILETIME FAR *lpftLastWrite);
#ifndef _MAC
/*  IsTask can crash - here's a safer one.  */
BOOL WINAPI     FIsTask(HTASK hTask);

HANDLE WINAPI   CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess,
                DWORD dwShareMode, LPVOID lpSecurityAttributes,
                DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                HANDLE hTemplateFile);
BOOL WINAPI     ReadFile(HANDLE hFile, LPVOID lpBuffer,
                DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                LPVOID lpOverlapped);
BOOL WINAPI     WriteFile(HANDLE hFile, LPCVOID lpBuffer,
                DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                LPVOID lpOverlapped);
#endif
DWORD WINAPI    SetFilePointer(HANDLE hFile, LONG lDistanceToMove,
                LONG FAR *lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL WINAPI     SetEndOfFile(HANDLE hFile);
BOOL WINAPI     CloseHandle(HANDLE hObject);
DWORD WINAPI    GetTempPath(DWORD nBufferLength, LPSTR lpBuffer);
UINT WINAPI     GetTempFileName32 (LPCSTR lpPathName, LPCSTR lpPrefixString,
                UINT uUnique, LPSTR lpTempFileName);
BOOL WINAPI     DeleteFile(LPCSTR lpFileName);
#ifndef _MAC
BOOL WINAPI     CreateDirectory(LPCSTR lpPathName, LPVOID lpSecurityAttributes);
#endif
BOOL WINAPI     RemoveDirectory(LPCSTR lpPathName);
BOOL WINAPI     CopyFile(LPCSTR szSrc, LPCSTR szDst, BOOL fFailIfExists);
BOOL WINAPI     MoveFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
HANDLE WINAPI   FindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
BOOL WINAPI     FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
BOOL WINAPI     FindClose(HANDLE hFindFile);
DWORD WINAPI    GetFullPathName(LPCSTR lpFileName, DWORD nBufferLength,
                LPSTR lpBuffer, LPSTR *lpFilePart);
void WINAPI     Sleep(DWORD dwMilliseconds);
LONG WINAPI     CompareFileTime(const FILETIME FAR *, const FILETIME FAR *);
BOOL WINAPI     LocalFileTimeToFileTime(const FILETIME FAR *, FILETIME FAR *);
BOOL WINAPI     FileTimeToLocalFileTime(const FILETIME FAR *, FILETIME FAR *);
BOOL WINAPI     FileTimeToSystemTime(const FILETIME FAR *, SYSTEMTIME FAR *);
BOOL WINAPI     SystemTimeToFileTime(const SYSTEMTIME FAR *, FILETIME FAR *);
void WINAPI     GetSystemTime(SYSTEMTIME FAR *);
void WINAPI     GetLocalTime(SYSTEMTIME FAR *);
BOOL WINAPI     FileTimeToDosDateTime(const FILETIME FAR * lpFileTime,
                WORD FAR *lpFatDate, WORD FAR *lpFatTime);
BOOL WINAPI     DosDateTimeToFileTime(WORD wFatDate, WORD wFatTime,
                FILETIME FAR * lpFileTime);
DWORD WINAPI    GetTimeZoneInformation(
                LPTIME_ZONE_INFORMATION lpTimeZoneInformation);
BOOL WINAPI     SetTimeZoneInformation(
                const TIME_ZONE_INFORMATION FAR *lpTimeZoneInformation);

DWORD WINAPI    GetCurrentProcessId(void);
long WINAPI     MulDiv32(long, long, long);
#ifndef _MAC
BOOL WINAPI     FBadReadPtr(const void FAR* lp, UINT cb);
#endif

#else   /* !WIN16 */

/* Remaps GetTempFileName32() to the real 32bit version */

#define GetTempFileName32(_szPath,_szPfx,_n,_lpbuf) GetTempFileName(_szPath,_szPfx,_n,_lpbuf)

#define CloseMutexHandle    CloseHandle

#endif  /* !WIN16 */


#ifdef _MAC
#define CRITICAL_SECTION            ULONG
#define InitializeCriticalSection(_pcs) ((void)0)
#define DeleteCriticalSection(_pcs)     ((void)0)
#define EnterCriticalSection(_pcs)      ((void)0)
#define LeaveCriticalSection(_pcs)      ((void)0)
#endif

/********************************/
/*  Our private conventions     */
/*  (common to WIN16/WIN32)     */
/********************************/

#define Cbtszsize(_a)   ((lstrlen(_a)+1)*sizeof(TCHAR))
#define CbtszsizeA(_a)  ((lstrlenA(_a) + 1))
#define CbtszsizeW(_a)  ((lstrlenW(_a) + 1) * sizeof(WCHAR))
#define HexCchOf(_s)    (sizeof(_s)*2+1)
#define HexSizeOf(_s)   (HexCchOf(_s)*sizeof(TCHAR))

BOOL WINAPI IsBadBoundedStringPtr(const void FAR* lpsz, UINT cchMax);

/* FUTURE - obsolete. OLE2 no longer contains these */
#define GetSCode                    GetScode
#define ReportResult(_a,_b,_c,_d)   ResultFromScode(_b)

#ifdef __cplusplus
}
#endif

#endif /* __MAPIWIN_H__ */
