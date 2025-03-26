/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Utility Functions related to Print Processors
* COPYRIGHT:   Copyright 2020 Doug Lyons (douglyons@douglyons.com)
*/

#include "precomp.h"
#include <shlobj.h>
#include <undocshell.h>

#include <pseh/pseh2.h>

#define MAX_GETPRINTER_SIZE 4096 - MAX_PATH
typedef void (WINAPI *PPfpSHChangeNotify)(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);

static HMODULE hShell32 = (HMODULE)-1;


/*
 * Converts an incoming Unicode string to an ANSI string.
 * It is only useful for "in-place" conversions where the ANSI string goes
 * back into the same place where the Unicode string came into this function.
 *
 * It returns an error code.
 */
// TODO: It seems that many of the functions involving printing could use this.
DWORD UnicodeToAnsiInPlace(PWSTR pwszField)
{
    PSTR pszTemp;
    DWORD cch;

    /*
     * Map the incoming Unicode pwszField string to an ANSI one here so that we can do
     * in-place conversion. We read the Unicode input and then we write back the ANSI
     * conversion into the same buffer for use with our GetPrinterDriverA function
     */
    PSTR pszField = (PSTR)pwszField;

    if (!pwszField)
    {
        return ERROR_SUCCESS;
    }

    cch = wcslen(pwszField);
    if (cch == 0)
    {
        return ERROR_SUCCESS;
    }

    pszTemp = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
    if (!pszTemp)
    {
        ERR("HeapAlloc failed!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    WideCharToMultiByte(CP_ACP, 0, pwszField, -1, pszTemp, cch + 1, NULL, NULL);
    StringCchCopyA(pszField, cch + 1, pszTemp);

    HeapFree(hProcessHeap, 0, pszTemp);

    return ERROR_SUCCESS;
}

static int multi_sz_lenW(const WCHAR *str)
{
    const WCHAR *ptr = str;
    if (!str) return 0;
    do
    {
        ptr += lstrlenW(ptr) + 1;
    } while (*ptr);

    return (ptr - str + 1);// * sizeof(WCHAR); wine does this.
}

DWORD UnicodeToAnsiZZInPlace(PWSTR pwszzField)
{
    PSTR pszTemp;
    INT len, lenW;
    PSTR pszField = (PSTR)pwszzField;

    lenW = multi_sz_lenW(pwszzField);
    if (lenW == 0)
    {
        return ERROR_SUCCESS;
    }

    len = WideCharToMultiByte(CP_ACP, 0, pwszzField, lenW, NULL, 0, NULL, NULL);

    pszTemp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    WideCharToMultiByte(CP_ACP, 0, pwszzField, lenW, pszTemp, len, NULL, NULL);

    StringCchCopyA(pszField, len, pszTemp);

    HeapFree(hProcessHeap, 0, pszTemp);

    return ERROR_SUCCESS;
}

//
//  Implement and simplify later.
//
LONG WINAPI
IntProtectHandle( HANDLE hSpooler, BOOL Close )
{
    BOOL Bad = TRUE;
    LONG Ret;
    PSPOOLER_HANDLE pHandle;

    EnterCriticalSection(&rtlCritSec);

    _SEH2_TRY
    {
        pHandle = (PSPOOLER_HANDLE)hSpooler;
        if ( pHandle && pHandle->Sig == SPOOLER_HANDLE_SIG )
        {
            Bad = FALSE; // Not bad.
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    Ret = Bad; // Set return Level to 1 if we are BAD.

    if ( Bad )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        ERR("IPH : Printer Handle failed!\n");
    }
    else
    {
        if ( Close )
        {
            if ( pHandle->bShared || pHandle->cCount != 0 )
            {
                pHandle->bShared = TRUE;
                Ret = 2; // Return a high level and we are shared.
                FIXME("IPH Close : We are shared\n");
            }
            else
            {
                pHandle->bClosed = TRUE;
                FIXME("IPH Close : closing.\n");
            }
        }
    }

    if ( !Ret ) // Need to be Level 0.
    {
        pHandle->cCount++;
        FIXME("IPH : Count %d\n",pHandle->cCount);
    }

    LeaveCriticalSection(&rtlCritSec);

    // Return Level:
    // 2 : Close and/or shared
    // 1 : Failed Handle
    // 0 : In use.
    return Ret;
}
//
// This one too.
//
BOOL WINAPI
IntUnprotectHandle( HANDLE hSpooler )
{
    BOOL Ret = FALSE;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hSpooler;
    EnterCriticalSection(&rtlCritSec);
    if ( pHandle->bShared && --pHandle->cCount == 0 )
    {
        pHandle->bClosed = TRUE;
        pHandle->bShared = FALSE;
        Ret = TRUE;
    }
    LeaveCriticalSection(&rtlCritSec);
    FIXME("IUH : Count %d\n",pHandle->cCount);
    if ( Ret )
    {
//        ClosePrinterWorker( pHandle );
    }
    return Ret;
}

/**
 * @name AllocSplStr
 *
 * Allocates memory for a Unicode string and copies the input string into it.
 * Equivalent of wcsdup, but the returned buffer is allocated from the spooler heap and must be freed with DllFreeSplStr.
 *
 * @param pwszInput
 * The input string to copy
 *
 * @return
 * Pointer to the copied string or NULL if no memory could be allocated.
 */
PWSTR WINAPI
AllocSplStr(PCWSTR pwszInput)
{
    DWORD cbInput;
    PWSTR pwszOutput;

    // Sanity check
    if (!pwszInput)
        return NULL;

    // Get the length of the input string.
    cbInput = (wcslen(pwszInput) + 1) * sizeof(WCHAR);

    // Allocate it. We don't use DllAllocSplMem here, because it unnecessarily zeroes the memory.
    pwszOutput = HeapAlloc(hProcessHeap, 0, cbInput);
    if (!pwszOutput)
    {
        ERR("HeapAlloc failed!\n");
        return NULL;
    }

    // Copy the string and return it.
    CopyMemory(pwszOutput, pwszInput, cbInput);
    return pwszOutput;
}

/**
 * @name DllAllocSplMem
 *
 * Allocate a block of zeroed memory.
 * Windows allocates from a separate spooler heap here while we just use the process heap.
 *
 * @param dwBytes
 * Number of bytes to allocate.
 *
 * @return
 * A pointer to the allocated memory or NULL in case of an error.
 * You have to free this memory using DllFreeSplMem.
 */
PVOID WINAPI
DllAllocSplMem(DWORD dwBytes)
{
    return HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, dwBytes);
}

/**
 * @name DllFreeSplMem
 *
 * Frees the memory allocated with DllAllocSplMem.
 *
 * @param pMem
 * Pointer to the allocated memory.
 *
 * @return
 * TRUE in case of success, FALSE otherwise.
 */
BOOL WINAPI
DllFreeSplMem(PVOID pMem)
{
    if ( !pMem ) return TRUE;
    return HeapFree(hProcessHeap, 0, pMem);
}

/**
 * @name DllFreeSplStr
 *
 * Frees the string allocated with AllocSplStr.
 *
 * @param pwszString
 * Pointer to the allocated string.
 *
 * @return
 * TRUE in case of success, FALSE otherwise.
 */
BOOL WINAPI
DllFreeSplStr(PWSTR pwszString)
{
    if ( pwszString )
       return HeapFree(hProcessHeap, 0, pwszString);
    return FALSE;
}

SECURITY_DESCRIPTOR * get_sd( SECURITY_DESCRIPTOR *sd, DWORD *size )
{
    PSID sid_group, sid_owner;
    ACL *sacl, *dacl;
    BOOL bSet = FALSE, bSetd = FALSE, bSets = FALSE;
    PSECURITY_DESCRIPTOR absolute_sd, retsd;

    if ( !IsValidSecurityDescriptor( sd ) )
    {
        return NULL;
    }

    InitializeSecurityDescriptor( &absolute_sd, SECURITY_DESCRIPTOR_REVISION );

    if ( !GetSecurityDescriptorOwner( sd, &sid_owner, &bSet ) )
    {
        return NULL;
    }

    SetSecurityDescriptorOwner( &absolute_sd, sid_owner, bSet );

    if ( !GetSecurityDescriptorGroup( sd, &sid_group, &bSet ) )
    {
        return NULL;
    }

    SetSecurityDescriptorGroup( &absolute_sd, sid_group, bSet );

    if ( !GetSecurityDescriptorDacl( sd, &bSetd, &dacl, &bSet ) )
    {
        return NULL;
    }

    SetSecurityDescriptorDacl( &absolute_sd, bSetd, dacl, bSet );

    if ( !GetSecurityDescriptorSacl( sd, &bSets, &sacl, &bSet ) )
    {
        return(NULL);
    }

    SetSecurityDescriptorSacl( &absolute_sd, bSets, sacl, bSet );

    *size = GetSecurityDescriptorLength( &absolute_sd );

    retsd = HeapAlloc( GetProcessHeap(), 0, *size );

    if ( retsd )
    {
        if ( !MakeSelfRelativeSD( &absolute_sd, retsd, size ) )
        {
            HeapFree( GetProcessHeap(), 0, retsd );
            retsd = NULL;
        }
    }

    return retsd;
}

VOID
UpdateTrayIcon( HANDLE hPrinter, DWORD JobId )
{
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;
    SHCNF_PRINTJOB_INFO spji;
    PRINTER_INFO_1W pi1w[MAX_GETPRINTER_SIZE] = {0};
    DWORD cbNeeded;
    PPfpSHChangeNotify fpFunction;

    pHandle->bTrayIcon = TRUE;

    spji.JobId = JobId;

    if (!GetPrinterW( hPrinter, 1, (PBYTE)&pi1w, MAX_GETPRINTER_SIZE, &cbNeeded) )
    {
        ERR("UpdateTrayIcon : GetPrinterW cbNeeded %d\n");
        return;
    }

    if ( hShell32 == (HMODULE)-1 )
    {
        hShell32 = LoadLibraryW(L"shell32.dll");
    }

    if ( hShell32 )
    {
        fpFunction = (PPfpSHChangeNotify)GetProcAddress( hShell32, "SHChangeNotify" );

        if ( fpFunction )
        {
            fpFunction( SHCNE_CREATE, (SHCNF_FLUSHNOWAIT|SHCNF_FLUSH|SHCNF_PRINTJOBW), pi1w->pName , &spji );
        }
    }
    else
    {
        ERR("UpdateTrayIcon : No Shell32!\n");
    }
}
