/****************************Module*Header******************************\
* Module Name: OBJECT.C
*
* Module Descripton: Object management functions.
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  18 March 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"

//
// Number of required and optional functions for CMMs to export
//

#define NUM_REQ_FNS    10
#define NUM_OPT_FNS    6
#define NUM_PS_FNS     3


/******************************************************************************
 *
 *                            AllocateHeapObject
 *
 *  Function:
 *       This functions allocates requested object on the process's heap,
 *       and returns a handle to it.
 *
 *  Arguments:
 *       objType  - type of object to allocate
 *
 *  Returns:
 *       Handle to object if successful, NULL otherwise
 *
 ******************************************************************************/

HANDLE
AllocateHeapObject(
    OBJECTTYPE  objType
    )
{
    DWORD    dwSize;
    POBJHEAD pObject;

    switch (objType)
    {
    case OBJ_PROFILE:
        dwSize = sizeof(PROFOBJ);
        break;

    case OBJ_TRANSFORM:
        dwSize = sizeof(TRANSFORMOBJ);
        break;

    case OBJ_CMM:
        dwSize = sizeof(CMMOBJ);
        break;

    default:
        RIP((__TEXT("Allocating invalid object\n")));
        dwSize = 0;
        break;
    }

    pObject = (POBJHEAD)MemAlloc(dwSize);

    if (!pObject)
    {
        return NULL;
    }

    pObject->objType = objType;

    return(PTRTOHDL(pObject));
}


/******************************************************************************
 *
 *                            FreeHeapObject
 *
 *  Function:
 *       This functions free an object on the process's heap
 *
 *  Arguments:
 *       hObject  - handle to object to free
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
FreeHeapObject(
    HANDLE hObject
    )
{
    POBJHEAD pObject;

    ASSERT(hObject != NULL);

    pObject = (POBJHEAD)HDLTOPTR(hObject);

    ASSERT(pObject->dwUseCount == 0);

    pObject->objType = 0;       // in case the handle gets reused

    MemFree((PVOID)pObject);
}


/******************************************************************************
 *
 *                            ValidHandle
 *
 *  Function:
 *       This functions checks if a given handle is a valid handle to
 *       an object of the specified type
 *
 *  Arguments:
 *       hObject  - handle to an object
 *       objType  - type of object to the handle refers to
 *
 *  Returns:
 *       TRUE is the handle is valid, FALSE otherwise.
 *
 ******************************************************************************/

BOOL
ValidHandle(
    HANDLE  hObject,
    OBJECTTYPE objType
    )
{
    POBJHEAD pObject;
    BOOL     rc;

    if (!hObject)
    {
        return FALSE;
    }

    pObject = (POBJHEAD)HDLTOPTR(hObject);

    rc = !IsBadReadPtr(pObject, sizeof(DWORD)) &&
         (pObject->objType == objType);

    return rc;
}


/******************************************************************************
 *
 *                           ValidProfile
 *
 *  Function:
 *       This function checks if a given profile is valid by doing some
 *       sanity checks on it. It is not a fool prof check.
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *
 *  Returns:
 *       TRUE if it is a valid profile, FALSE otherwise
 *
 ******************************************************************************/

BOOL ValidProfile(
    PPROFOBJ pProfObj
    )
{
    DWORD dwSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);

    return ((dwSize <= pProfObj->dwMapSize) &&
            (HEADER(pProfObj)->phSignature == PROFILE_SIGNATURE) &&
            (dwSize >= (sizeof(PROFILEHEADER) + sizeof(DWORD))));
}


/******************************************************************************
 *
 *                            MemAlloc
 *
 *  Function:
 *       This functions allocates requested amount of zero initialized memory
 *       from the process's heap and returns a pointer to it
 *
 *  Arguments:
 *       dwSize  - amount of memory to allocate in bytes
 *
 *  Returns:
 *       Pointer to memory if successful, NULL otherwise
 *
 ******************************************************************************/

PVOID
MemAlloc(
    DWORD dwSize
    )
{
    if (dwSize > 0)
        return (PVOID)GlobalAllocPtr(GHND | GMEM_ZEROINIT, dwSize);
    else
        return NULL;
}


/******************************************************************************
 *
 *                            MemReAlloc
 *
 *  Function:
 *       This functions reallocates a block of memory from the process's
 *       heap and returns a pointer to it
 *
 *  Arguments:
 *       pMemory    - pointer to original memory
 *       dwNewSize  - new size to reallocate
 *
 *  Returns:
 *       Pointer to memory if successful, NULL otherwise
 *
 ******************************************************************************/

PVOID
MemReAlloc(
    PVOID pMemory,
    DWORD dwNewSize
    )
{
    return (PVOID)GlobalReAllocPtr(pMemory, dwNewSize, GMEM_ZEROINIT);
}


/******************************************************************************
 *
 *                            MemFree
 *
 *  Function:
 *       This functions frees memory from the process's heap
 *       and returns a handle to it.
 *
 *  Arguments:
 *       pMemory  - pointer to memory to free
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
MemFree(
    PVOID pMemory
    )
{
    DWORD dwErr;

    //
    // GlobalFree() resets last error, we get and set around it so we don't
    // lose anything we have set.
    //

    dwErr = GetLastError();
    GlobalFreePtr(pMemory);
    if (dwErr)
    {
        SetLastError(dwErr);
    }
}


/******************************************************************************
 *
 *                            MyCopyMemory
 *
 *  Function:
 *       This functions copies data from one place to another. It takes care
 *       of overlapping cases. The reason we have our own function and not use
 *       MoveMemory is that MoveMemory uses memmove which pulls in msvcrt.dll
 *
 *  Arguments:
 *       pDest    - pointer to destination of copy
 *       pSrc     - pointer to source
 *       dwCount  - number of bytes to copy
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
MyCopyMemory(
    PBYTE pDest,
    PBYTE pSrc,
    DWORD dwCount
    )
{
    //
    // Make sure overlapping cases are handled
    //

    if ((pSrc < pDest) && ((pSrc + dwCount) >= pDest))
    {
        //
        // Overlapping case, copy in reverse
        //

        pSrc += dwCount - 1;
        pDest += dwCount - 1;

        while (dwCount--)
        {
            *pDest-- = *pSrc--;
        }

    }
    else
    {
        while (dwCount--)
        {
            *pDest++ = *pSrc++;
        }
    }

    return;
}


/******************************************************************************
 *
 *                              ConvertToUnicode
 *
 *  Function:
 *       This function converts a given Ansi string to Unicode. It optionally
 *       allocates memory for the Unicode string which the calling program
 *       needs to free.
 *
 *  Arguments:
 *       pszAnsiStr      - pointer to Ansi string to convert
 *       ppwszUnicodeStr - pointer to pointer to Unicode string
 *       bAllocate       - If TRUE, allocate memory for Unicode string
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
ConvertToUnicode(
    PCSTR  pszAnsiStr,
    PWSTR *ppwszUnicodeStr,
    BOOL   bAllocate
    )
{
    DWORD dwLen;                    // length of Unicode string

    dwLen = (lstrlenA(pszAnsiStr) + 1) * sizeof(WCHAR);

    //
    // Allocate memory for Unicode string
    //

    if (bAllocate)
    {
        *ppwszUnicodeStr =  (PWSTR)MemAlloc(dwLen);
        if (! (*ppwszUnicodeStr))
        {
            WARNING((__TEXT("Error allocating memory for Unicode name\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    //
    // Convert Ansi string to Unicode
    //

    if (! MultiByteToWideChar(CP_ACP, 0, pszAnsiStr, -1,
            *ppwszUnicodeStr, dwLen))
    {
        WARNING((__TEXT("Error converting to Unicode name\n")));
        MemFree(*ppwszUnicodeStr);
        *ppwszUnicodeStr = NULL;
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 *
 *                              ConvertToAnsi
 *
 *  Function:
 *       This function converts a given Unicode string to Ansi. It optionally
 *       allocates memory for the Ansi string which the calling program needs
 *       to free.
 *
 *  Arguments:
 *       pwszUnicodeStr  - pointer to Unicode string to convert
 *       ppszAnsiStr     - pointer to pointer to Ansi string.
 *       bAllocate       - If TRUE, allocate memory for Ansi string
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL
ConvertToAnsi(
    PCWSTR  pwszUnicodeStr,
    PSTR   *ppszAnsiStr,
    BOOL    bAllocate
    )
{
    DWORD dwLen;                    // length of Ansi string
    BOOL  bUsedDefaultChar;         // if default characters were used in
                                    // converting Unicode to Ansi

    dwLen = (lstrlenW(pwszUnicodeStr) + 1) * sizeof(char);

    //
    // Allocate memory for Ansi string
    //

    if (bAllocate)
    {
        *ppszAnsiStr = (PSTR)MemAlloc(dwLen);
        if (! (*ppszAnsiStr))
        {
            WARNING((__TEXT("Error allocating memory for ANSI name\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    //
    // Convert Unicode string to Ansi
    //

    if (! WideCharToMultiByte(CP_ACP, 0, pwszUnicodeStr, -1, *ppszAnsiStr,
            dwLen, NULL, &bUsedDefaultChar) || bUsedDefaultChar)
    {
        WARNING((__TEXT("Error converting to Ansi name\n")));
        MemFree(*ppszAnsiStr);
        *ppszAnsiStr = NULL;
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 *
 *                         ValidColorMatchingModule
 *
 *  Function:
 *
 *  Arguments:
 *       cmmID   - ID identifing the CMM
 *       pCMMDll - pointer to CMM module path and file name
 *
 *  Returns:
 *
 ******************************************************************************/

BOOL
ValidColorMatchingModule(
    DWORD cmmID,
    PTSTR pCMMDll
    )
{
    HINSTANCE hInstance = NULL;
    DWORD    (WINAPI *pfnCMGetInfo)(DWORD);
    FARPROC   pfnCMRequired;
    DWORD     i;
    BOOL      rc = FALSE;       // Assume failure

    //
    // Load the CMM
    //

    hInstance = LoadLibrary(pCMMDll);

    if (!hInstance)
    {
        WARNING((__TEXT("Could not load CMM %s\n"), pCMMDll));
        goto EndValidColorMatchingModule;
    }

    (PVOID) pfnCMGetInfo = (PVOID) GetProcAddress(hInstance, gszCMMReqFns[0]);

    if (!pfnCMGetInfo)
    {
        ERR((__TEXT("CMM does not export CMGetInfo\n")));
        goto EndValidColorMatchingModule;
    }

    //
    // Check if the CMM is the right version and reports the same ID
    //

    if ((pfnCMGetInfo(CMM_VERSION) < 0x00050000) ||
        (pfnCMGetInfo(CMM_IDENT) != cmmID))
    {
        ERR((__TEXT("CMM %s not correct version or reports incorrect ID\n"), pCMMDll));
        goto EndValidColorMatchingModule;
    }

    //
    // Check the remaining required functions is presented
    //

    for (i=1; i<NUM_REQ_FNS; i++)
    {
        pfnCMRequired = GetProcAddress(hInstance, gszCMMReqFns[i]);
        if (!pfnCMRequired)
        {
            ERR((__TEXT("CMM %s does not export %s\n"), pCMMDll, gszCMMReqFns[i]));
            goto EndValidColorMatchingModule;
        }
    }

    rc = TRUE;

EndValidColorMatchingModule:

    if (hInstance)
    {
        FreeLibrary(hInstance);
    }

    return rc;
}


/******************************************************************************
 *
 *                         GetColorMatchingModule
 *
 *  Function:
 *       This functions returns a pointer to a CMMObject corresponding to
 *       the ID given. It first looks a the list of CMM objects loaded
 *       into memory, and if it doesn't find the right one, loads it.
 *
 *  Arguments:
 *       cmmID   - ID identifing the CMM
 *
 *  Returns:
 *       Pointer to the CMM object if successful, NULL otherwise
 *
 ******************************************************************************/

PCMMOBJ
GetColorMatchingModule(
    DWORD cmmID
    )
{
    HANDLE    hCMMObj;
    PCMMOBJ   pCMMObj = NULL;
    FARPROC   *ppTemp;
    HINSTANCE hInstance = NULL;
    HKEY      hkCMM = NULL;
    DWORD     dwTaskID;
    TCHAR     szCMMID[5];
    DWORD     dwType, bufSize, i;
    TCHAR     szBuffer[MAX_PATH];
    BOOL      rc = FALSE;       // Assume failure

    //
    // Check if we have already loaded this CMM
    //

    dwTaskID = GetCurrentProcessId();

    EnterCriticalSection(&critsec);     // Critical section
    pCMMObj = gpCMMChain;

    while (pCMMObj)
    {
        if ((pCMMObj->dwCMMID == cmmID) && (pCMMObj->dwTaskID == dwTaskID))
        {
            pCMMObj->objHdr.dwUseCount++;
            break;
        }
        pCMMObj = pCMMObj->pNext;
    }
    LeaveCriticalSection(&critsec);     // Critical section

    if (pCMMObj)
        return pCMMObj;

    //
    // CMM not already loaded - check to see if it is default CMM before
    // looking in the registry
    //

    if (cmmID == CMM_WINDOWS_DEFAULT)
    {
        hInstance = LoadLibrary(gszDefaultCMM);
        goto AttemptedLoadingCMM;
    }

    //
    // Not default CMM, look in the registry
    //

    if (RegOpenKey(HKEY_LOCAL_MACHINE, gszICMatcher, &hkCMM) != ERROR_SUCCESS)
    {
        return NULL;
    }

    //
    // Make a string with the CMM ID
    //

#ifdef UNICODE
    {
        DWORD temp = FIX_ENDIAN(cmmID);

        if (!MultiByteToWideChar(CP_ACP, 0, (PSTR)&temp, 4, szCMMID, 5))
        {
            WARNING((__TEXT("Could not convert cmmID %x to Unicode\n"), temp));
            goto EndGetColorMatchingModule;
        }
    }
#else
    for (i=0; i<4; i++)
    {
        szCMMID[i] = ((PSTR)&cmmID)[3-i];
    }
#endif
    szCMMID[4] = '\0';

    //
    // Get the file name of the CMM dll if registered
    //

    bufSize = MAX_PATH;
    if (RegQueryValueEx(hkCMM, (PTSTR)szCMMID, 0, &dwType, (BYTE *)szBuffer, &bufSize) !=
        ERROR_SUCCESS)
    {
        WARNING((__TEXT("CMM %s not registered\n"), szCMMID));
        goto EndGetColorMatchingModule;
    }

    //
    // Load the CMM
    //

    hInstance = LoadLibrary(szBuffer);

AttemptedLoadingCMM:

    if (!hInstance)
    {
        WARNING((__TEXT("Could not load CMM %x\n"), cmmID));
        goto EndGetColorMatchingModule;
    }

    //
    // Allocate a CMM object
    //

    hCMMObj = AllocateHeapObject(OBJ_CMM);
    if (!hCMMObj)
    {
        ERR((__TEXT("Could not allocate CMM object\n")));
        goto EndGetColorMatchingModule;
    }

    pCMMObj = (PCMMOBJ)HDLTOPTR(hCMMObj);

    ASSERT(pCMMObj != NULL);

    //
    // Fill in the CMM object
    //

    pCMMObj->objHdr.dwUseCount = 1;
    pCMMObj->dwCMMID = cmmID;
    pCMMObj->dwTaskID = dwTaskID;
    pCMMObj->hCMM = hInstance;

    ppTemp = (FARPROC *)&pCMMObj->fns.pCMGetInfo;
    *ppTemp = GetProcAddress(hInstance, gszCMMReqFns[0]);
    ppTemp++;

    if (!pCMMObj->fns.pCMGetInfo)
    {
        ERR((__TEXT("CMM does not export CMGetInfo\n")));
        goto EndGetColorMatchingModule;
    }

    //
    // Check if the CMM is the right version and reports the same ID
    //

    if (pCMMObj->fns.pCMGetInfo(CMM_VERSION) < 0x00050000 ||
        pCMMObj->fns.pCMGetInfo(CMM_IDENT) != cmmID)
    {
        ERR((__TEXT("CMM not correct version or reports incorrect ID\n")));
        goto EndGetColorMatchingModule;
    }

    //
    // Load the remaining required functions
    //

    for (i=1; i<NUM_REQ_FNS; i++)
    {
        *ppTemp = GetProcAddress(hInstance, gszCMMReqFns[i]);
        if (!*ppTemp)
        {
            ERR((__TEXT("CMM %s does not export %s\n"), szCMMID, gszCMMReqFns[i]));
            goto EndGetColorMatchingModule;
        }
        ppTemp++;
    }

    //
    // Load the optional functions
    //

    for (i=0; i<NUM_OPT_FNS; i++)
    {
        *ppTemp = GetProcAddress(hInstance, gszCMMOptFns[i]);

        //
        // Even these functions are required for Windows default CMM
        //

        if (cmmID == CMM_WINDOWS_DEFAULT && !*ppTemp)
        {
            ERR((__TEXT("Windows default CMM does not export %s\n"), gszCMMOptFns[i]));
            goto EndGetColorMatchingModule;
        }
        ppTemp++;
    }

    //
    // Load the PS functions - these are optional even for the default CMM
    //

    for (i=0; i<NUM_PS_FNS; i++)
    {
        *ppTemp = GetProcAddress(hInstance, gszPSFns[i]);
        ppTemp++;
    }

    //
    // If any of the PS Level2 fns is not exported, do not use this CMM
    // for any of the PS Level 2 functionality
    //

    if (!pCMMObj->fns.pCMGetPS2ColorSpaceArray ||
        !pCMMObj->fns.pCMGetPS2ColorRenderingIntent ||
        !pCMMObj->fns.pCMGetPS2ColorRenderingDictionary)
    {
        pCMMObj->fns.pCMGetPS2ColorSpaceArray = NULL;
        pCMMObj->fns.pCMGetPS2ColorRenderingIntent = NULL;
        pCMMObj->fns.pCMGetPS2ColorRenderingDictionary = NULL;
        pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
    }

    //
    // Add the CMM object to the chain at the beginning
    //

    EnterCriticalSection(&critsec);     // Critical section
    pCMMObj->pNext = gpCMMChain;
    gpCMMChain = pCMMObj;
    LeaveCriticalSection(&critsec);     // Critical section

    rc = TRUE;                          // Success!

EndGetColorMatchingModule:

    if (!rc)
    {
        if (pCMMObj)
        {
            pCMMObj->objHdr.dwUseCount--;   // decrement before freeing
            FreeHeapObject(hCMMObj);
            pCMMObj = NULL;
        }
        if (hInstance)
        {
            FreeLibrary(hInstance);
        }
    }

    if (hkCMM)
    {
        RegCloseKey(hkCMM);
    }

    return pCMMObj;
}


/******************************************************************************
 *
 *                         GetPreferredCMM
 *
 *  Function:
 *       This functions returns a pointer to the app specified CMM to use
 *
 *  Arguments:
 *       None
 *
 *  Returns:
 *       Pointer to app specified CMM object on success, NULL otherwise
 *
 ******************************************************************************/

PCMMOBJ GetPreferredCMM(
    )
{
    PCMMOBJ pCMMObj;

    EnterCriticalSection(&critsec);     // Critical section
    pCMMObj = gpPreferredCMM;

    if (pCMMObj)
    {
        //
        // Increment use count
        //

        pCMMObj->objHdr.dwUseCount++;
    }
    LeaveCriticalSection(&critsec);     // Critical section

    return pCMMObj;
}


/******************************************************************************
 *
 *                         ReleaseColorMatchingModule
 *
 *  Function:
 *       This functions releases a CMM object. If the ref count goes to
 *       zero, it unloads the CMM and frees all memory associated with it.
 *
 *  Arguments:
 *       pCMMObj  - pointer to CMM object to release
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
ReleaseColorMatchingModule(
    PCMMOBJ pCMMObj
    )
{
    EnterCriticalSection(&critsec);     // Critical section

    ASSERT(pCMMObj->objHdr.dwUseCount > 0);

    pCMMObj->objHdr.dwUseCount--;

    if (pCMMObj->objHdr.dwUseCount == 0)
    {
        //
        // Unloading the CMM everytime a transform is freed might not be
        // very efficient. So for now, I am not going to unload it. When
        // the app terminates, kernel should unload all dll's loaded by
        // this app
        //
    }
    LeaveCriticalSection(&critsec);     // Critical section

    return;
}


#ifdef DBG

/******************************************************************************
 *
 *                              MyDebugPrint
 *
 *  Function:
 *       This function takes a format string and paramters, composes a string
 *       and sends it out to the debug port. Available only in debug build.
 *
 *  Arguments:
 *       pFormat  - pointer to format string
 *       .......  - parameters based on the format string like printf()
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
MyDebugPrintA(
    PSTR pFormat,
    ...
    )
{
    char     szBuffer[256];
    va_list  arglist;

    va_start(arglist, pFormat);
    wvsprintfA(szBuffer, pFormat, arglist);
    va_end(arglist);

    OutputDebugStringA(szBuffer);

    return;
}


VOID
MyDebugPrintW(
    PWSTR pFormat,
    ...
    )
{
    WCHAR    szBuffer[256];
    va_list  arglist;

    va_start(arglist, pFormat);
    wvsprintfW(szBuffer, pFormat, arglist);
    va_end(arglist);

    OutputDebugStringW(szBuffer);

    return;
}

/******************************************************************************
 *
 *                              StripDirPrefixA
 *
 *  Function:
 *       This function takes a path name and returns a pointer to the filename
 *       part. This is availabel only for the debug build.
 *
 *  Arguments:
 *       pszPathName - path name of file (can be file name alone)
 *
 *  Returns:
 *       A pointer to the file name
 *
 ******************************************************************************/

PSTR
StripDirPrefixA(
    PSTR pszPathName
    )
{
    DWORD dwLen = lstrlenA(pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

#endif

