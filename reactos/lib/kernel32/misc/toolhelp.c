/* $Id: toolhelp.c,v 1.3 2003/07/10 18:50:51 chorns Exp $
 *
 * KERNEL32.DLL toolhelp functions
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/toolhelp.c
 * PURPOSE:         Toolhelp functions
 * PROGRAMMER:      Robert Dickenson (robd@mok.lvcm.com)
 * UPDATE HISTORY:
 *                  Created 05 January 2003
 */

#include <windows.h>
#include <tlhelp32.h>


#define CHECK_PARAM_SIZE(ptr, siz) \
    if (!ptr || ptr->dwSize != siz) { \
        SetLastError(ERROR_INVALID_PARAMETER); \
        return FALSE; \
    }


/*
 * @unimplemented
 */
BOOL WINAPI
Heap32First(LPHEAPENTRY32 lphe, DWORD th32ProcessID, DWORD th32HeapID)
{
    CHECK_PARAM_SIZE(lphe, sizeof(HEAPENTRY32));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Heap32Next(LPHEAPENTRY32 lphe)
{
/*
typedef struct tagHEAPENTRY32 {
	DWORD dwSize;
	HANDLE hHandle;
	DWORD dwAddress;
	DWORD dwBlockSize;
	DWORD dwFlags;
	DWORD dwLockCount;
	DWORD dwResvd;
	DWORD th32ProcessID;
	DWORD th32HeapID;
} HEAPENTRY32,*PHEAPENTRY32,*LPHEAPENTRY32;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Heap32ListFirst(HANDLE hSnapshot, LPHEAPLIST32 lphl)
{
    CHECK_PARAM_SIZE(lphl, sizeof(HEAPLIST32));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Heap32ListNext(HANDLE hSnapshot, LPHEAPLIST32 lph1)
{
/*
typedef struct tagHEAPLIST32 {
	DWORD dwSize;
	DWORD th32ProcessID;
	DWORD th32HeapID;
	DWORD dwFlags;
} HEAPLIST32,*PHEAPLIST32,*LPHEAPLIST32;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
    CHECK_PARAM_SIZE(lpme, sizeof(MODULEENTRY32));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Module32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    CHECK_PARAM_SIZE(lpme, sizeof(MODULEENTRY32W));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
/*
typedef struct tagMODULEENTRY32 {
	DWORD dwSize;
	DWORD th32ModuleID;
	DWORD th32ProcessID;
	DWORD GlblcntUsage;
	DWORD ProccntUsage;
	BYTE *modBaseAddr;
	DWORD modBaseSize;
	HMODULE hModule;
	char szModule[MAX_MODULE_NAME32 + 1];
	char szExePath[MAX_PATH];
} MODULEENTRY32,*PMODULEENTRY32,*LPMODULEENTRY32;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL WINAPI
Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
/*
typedef struct tagMODULEENTRY32W {
	DWORD dwSize;
	DWORD th32ModuleID;
	DWORD th32ProcessID;
	DWORD GlblcntUsage;
	DWORD ProccntUsage;
	BYTE *modBaseAddr;
	DWORD modBaseSize;
	HMODULE hModule; 
	WCHAR szModule[MAX_MODULE_NAME32 + 1];
	WCHAR szExePath[MAX_PATH];
} MODULEENTRY32W,*PMODULEENTRY32W,*LPMODULEENTRY32W;

 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    CHECK_PARAM_SIZE(lppe, sizeof(PROCESSENTRY32));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
/*
typedef struct tagPROCESSENTRY32 {
	DWORD dwSize;
	DWORD cntUsage;
	DWORD th32ProcessID;
	DWORD th32DefaultHeapID;
	DWORD th32ModuleID;
	DWORD cntThreads;
	DWORD th32ParentProcessID;
	LONG pcPriClassBase;
	DWORD dwFlags;
	CHAR  szExeFile[MAX_PATH];
} PROCESSENTRY32,*PPROCESSENTRY32,*LPPROCESSENTRY32;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
Process32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    CHECK_PARAM_SIZE(lppe, sizeof(PROCESSENTRY32W));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
Process32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
/*
typedef struct tagPROCESSENTRY32W {
	DWORD dwSize;
	DWORD cntUsage;
	DWORD th32ProcessID;
	DWORD th32DefaultHeapID;
	DWORD th32ModuleID;
	DWORD cntThreads;
	DWORD th32ParentProcessID;
	LONG pcPriClassBase;
	DWORD dwFlags;
	WCHAR szExeFile[MAX_PATH];
} PROCESSENTRY32W,*PPROCESSENTRY32W,*LPPROCESSENTRY32W;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


BOOL WINAPI Thread32First(HANDLE hSnapshot, LPTHREADENTRY32 lpte)
{
    CHECK_PARAM_SIZE(lpte, sizeof(THREADENTRY32));

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI Thread32Next(HANDLE hSnapshot, LPTHREADENTRY32 lpte)
{
/*
typedef struct tagTHREADENTRY32 {
	DWORD dwSize;
	DWORD cntUsage;
	DWORD th32ThreadID;
	DWORD th32OwnerProcessID;
	LONG tpBasePri;
	LONG tpDeltaPri;
	DWORD dwFlags;
} THREADENTRY32,*PTHREADENTRY32,*LPTHREADENTRY32;
 */
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
Toolhelp32ReadProcessMemory(DWORD th32ProcessID,
  LPCVOID lpBaseAddress, LPVOID lpBuffer,
  DWORD cbRead, LPDWORD lpNumberOfBytesRead)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


#define TL_DEV_NAME L"\\??\\TlHelpDevice"

/*
 * @unimplemented
 */
HANDLE STDCALL
CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
{
    // return open handle to snapshot on success, -1 on failure
    // the snapshot handle behavies like an object handle
    SECURITY_ATTRIBUTES sa;
    HANDLE hSnapshot = (HANDLE)-1;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);

    if (dwFlags & TH32CS_INHERIT) {
    }
    if (dwFlags & TH32CS_SNAPHEAPLIST) {
    }
    if (dwFlags & TH32CS_SNAPMODULE) {
    }
    if (dwFlags & TH32CS_SNAPPROCESS) {
    }
    if (dwFlags & TH32CS_SNAPTHREAD) {
    }
    hSnapshot = CreateFileW(TL_DEV_NAME,
                           GENERIC_READ, FILE_SHARE_READ + FILE_SHARE_WRITE,
                           &sa, OPEN_EXISTING, 0L/*FILE_ATTRIBUTE_SYSTEM*/, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {

    }
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    // caller must use CloseHandle to destroy the returned snapshot handle
    return hSnapshot;
}


#if 0 /* extracted from mingw tlhelp32.h for easy reference while working above */
/*
#define HF32_DEFAULT	1
#define HF32_SHARED	2
#define LF32_FIXED	0x1
#define LF32_FREE	0x2
#define LF32_MOVEABLE	0x4
#define MAX_MODULE_NAME32	255
#define TH32CS_SNAPHEAPLIST	0x1
#define TH32CS_SNAPPROCESS	0x2
#define TH32CS_SNAPTHREAD	0x4
#define TH32CS_SNAPMODULE	0x8
#define TH32CS_SNAPALL	(TH32CS_SNAPHEAPLIST|TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD|TH32CS_SNAPMODULE)
#define TH32CS_INHERIT	0x80000000

BOOL WINAPI Heap32First(LPHEAPENTRY32,DWORD,DWORD);
BOOL WINAPI Heap32ListFirst(HANDLE,LPHEAPLIST32);
BOOL WINAPI Heap32ListNext(HANDLE,LPHEAPLIST32);
BOOL WINAPI Heap32Next(LPHEAPENTRY32);
BOOL WINAPI Module32First(HANDLE,LPMODULEENTRY32);
BOOL WINAPI Module32FirstW(HANDLE,LPMODULEENTRY32W);
BOOL WINAPI Module32Next(HANDLE,LPMODULEENTRY32);
BOOL WINAPI Module32NextW(HANDLE,LPMODULEENTRY32W);
BOOL WINAPI Process32First(HANDLE,LPPROCESSENTRY32);
BOOL WINAPI Process32FirstW(HANDLE,LPPROCESSENTRY32W);
BOOL WINAPI Process32Next(HANDLE,LPPROCESSENTRY32);
BOOL WINAPI Process32NextW(HANDLE,LPPROCESSENTRY32W);
BOOL WINAPI Thread32First(HANDLE,LPTHREADENTRY32);
BOOL WINAPI Thread32Next(HANDLE,LPTHREADENTRY32);
BOOL WINAPI Toolhelp32ReadProcessMemory(DWORD,LPCVOID,LPVOID,DWORD,LPDWORD);
HANDLE WINAPI CreateToolhelp32Snapshot(DWORD,DWORD);

#ifdef UNICODE
#define LPMODULEENTRY32 LPMODULEENTRY32W
#define LPPROCESSENTRY32 LPPROCESSENTRY32W
#define MODULEENTRY32 MODULEENTRY32W
#define Module32First Module32FirstW
#define Module32Next Module32NextW
#define PMODULEENTRY32 PMODULEENTRY32W
#define PPROCESSENTRY32 PPROCESSENTRY32W
#define PROCESSENTRY32 PROCESSENTRY32W
#define Process32First Process32FirstW
#define Process32Next Process32NextW
#endif // UNICODE
 */
#endif /* 0 */

/* EOF */
