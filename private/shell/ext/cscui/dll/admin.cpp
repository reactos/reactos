//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       admin.cpp
//
//  Authors;
//    Jeff Saathoff (jeffreys)
//
//  Notes;
//    Support for Administratively pinned folders
//--------------------------------------------------------------------------
#include "pch.h"


DWORD WINAPI _PinAdminFoldersThread(LPVOID);
LONG _RegEnumValueExp(HKEY, DWORD, LPTSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);


//*************************************************************
//
//  ApplyAdminFolderPolicy
//
//  Purpose:    Pin the admin folder list
//
//  Parameters: none
//
//  Return:     none
//
//  Notes:      
//
//*************************************************************
void
ApplyAdminFolderPolicy(void)
{
    DWORD dwThreadId;
    HANDLE hThread = CreateThread(NULL, 0, _PinAdminFoldersThread, NULL, 0, &dwThreadId);   
    if (hThread)
    {
        CloseHandle(hThread);
    }
}

//
// Does a particular path exist in the DPA of path strings?
//
BOOL
ExistsAPF(
    HDPA hdpa, 
    LPCTSTR pszPath
    )
{
    const int cItems = DPA_GetPtrCount(hdpa);
    for (int i = 0; i < cItems; i++)
    {
        LPCTSTR pszItem = (LPCTSTR)DPA_GetPtr(hdpa, i);
        if (pszItem && (0 == lstrcmpi(pszItem, pszPath)))
            return TRUE;
    }
    return FALSE;
}



BOOL
ReadAPFFromRegistry(HDPA hdpaFiles)
{
    const HKEY rghkeyRoots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };

    for (int i = 0; i < ARRAYSIZE(rghkeyRoots); i++)
    {
        HKEY hKey;

        // Read in the Administratively pinned folder list.
        if (ERROR_SUCCESS == RegOpenKey(rghkeyRoots[i], c_szRegKeyAPF, &hKey))
        {
            TCHAR szName[MAX_PATH];
            DWORD dwIndex = 0, dwSize = ARRAYSIZE(szName);

            while (ERROR_SUCCESS == _RegEnumValueExp(hKey, dwIndex, szName, &dwSize, NULL, NULL, NULL, NULL))
            {
                if (!ExistsAPF(hdpaFiles, szName))
                {
                    LPTSTR pszDup;
                    if (LocalAllocString(&pszDup, szName))
                    {
                        if (-1 == DPA_AppendPtr(hdpaFiles, pszDup))
                        {
                            LocalFreeString(&pszDup);
                        }
                    }
                }
                dwSize = ARRAYSIZE(szName);
                dwIndex++;
            }    
            RegCloseKey(hKey);
        }
    }
    return TRUE;
}


void
_DoAdminPin(LPCTSTR pszItem)
{
    BOOL bNewItem = FALSE;
    DWORD dwPinCount = 0;
    DWORD dwHintFlags = 0;

    TraceEnter(TRACE_ADMINPIN, "_DoAdminPin");

    if (!pszItem || !*pszItem)
        TraceLeaveVoid();

    TraceAssert(PathIsUNC(pszItem));

    // This may fail, for example if the file is not in the cache
    CSCQueryFileStatus(pszItem, NULL, &dwPinCount, &dwHintFlags);

    // Is the admin flag already turned on?
    bNewItem = !(dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN);

    // Turn on the admin flag
    dwHintFlags |= FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT | FLAG_CSC_HINT_PIN_ADMIN;

    //
    // Pin the item
    //
    CSCPinFile(pszItem,
               dwHintFlags,
               NULL,
               &dwPinCount,
               &dwHintFlags);

    //
    // If this item was not previously admin pinned, increment the
    // pin count a second time.  This lets us eliminate items that
    // are no longer in the admin pin list when we decrement pin
    // counts later.
    //
    if (bNewItem)
    {
        CSCPinFile(pszItem,
                   dwHintFlags,
                   NULL,
                   &dwPinCount,
                   &dwHintFlags);
    }
    Trace((TEXT("AdminPin (%d) %s"), dwPinCount, pszItem));

    TraceLeaveVoid();
}


void
_PinLinkTarget(LPCTSTR pszLink)
{
    LPTSTR pszTarget = NULL;

    TraceEnter(TRACE_ADMINPIN, "_PinLinkTarget");
    TraceAssert(pszLink);

    // We only want to pin a link target if it's a file (not a directory).
    // GetLinkTarget does this check and only returns files.
    GetLinkTarget(pszLink, NULL, &pszTarget);

    if (pszTarget)
    {
        // Pin the target
        _DoAdminPin(pszTarget);

        LocalFree(pszTarget);
    }

    TraceLeaveVoid();
}


DWORD WINAPI
_PinAdminFolderCallback(LPCTSTR             pszItem,
                        ENUM_REASON         eReason,
                        LPWIN32_FIND_DATA   /*pFind32*/,
                        LPARAM              /*lpContext*/)
{
    TraceEnter(TRACE_ADMINPIN, "_PinAdminFolderCallback");
    TraceAssert(pszItem);

    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    if (!pszItem || !*pszItem)
        TraceLeaveValue(CSCPROC_RETURN_SKIP);

    if (eReason == ENUM_REASON_FILE || eReason == ENUM_REASON_FOLDER_BEGIN)
    {
        SHFILEINFO sfi;
        sfi.dwAttributes = SFGAO_FILESYSTEM | SFGAO_LINK;

        // Is the path valid?  If so, get attributes.
        if (!SHGetFileInfo(pszItem, 0, &sfi, SIZEOF(sfi), SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED))
            TraceLeaveValue(CSCPROC_RETURN_SKIP);

        // If it's not a file system object, bail.
        if (!(sfi.dwAttributes & SFGAO_FILESYSTEM))
            TraceLeaveValue(CSCPROC_RETURN_SKIP);

        // If it's a link, pin the target
        if (sfi.dwAttributes & SFGAO_LINK)
            _PinLinkTarget(pszItem);

        // Pin the item
        if (PathIsUNC(pszItem))
            _DoAdminPin(pszItem);
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


DWORD WINAPI
_UnpinAdminFolderCallback(LPCTSTR             pszItem,
                          ENUM_REASON         eReason,
                          DWORD               dwStatus,
                          DWORD               dwHintFlags,
                          DWORD               dwPinCount,
                          LPWIN32_FIND_DATA   /*pFind32*/,
                          LPARAM              /*dwContext*/)
{
    BOOL bDeleteItem = FALSE;
    TraceEnter(TRACE_ADMINPIN, "_UnpinAdminFolderCallback");


    if (WAIT_OBJECT_0 == WaitForSingleObject(g_heventTerminate, 0))
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    if (!pszItem || !*pszItem)
        TraceLeaveValue(CSCPROC_RETURN_SKIP);

    TraceAssert(PathIsUNC(pszItem));

    //
    // Reduce the pin count by 1.  This effectively unpins
    // items that were admin pinned but are no longer part
    // of the admin pin set.
    //
    if ((eReason == ENUM_REASON_FILE || eReason == ENUM_REASON_FOLDER_BEGIN)
        && (dwHintFlags & FLAG_CSC_HINT_PIN_ADMIN))
    {
        dwHintFlags = 0;
        
        // If the pin count is going to zero (or is already zero)
        // turn off the admin hint flag.
        if (dwPinCount < 2)
            dwHintFlags = FLAG_CSC_HINT_PIN_ADMIN;

        // If the pin count is non-zero, reduce it by 1.
        if (dwPinCount > 0)
            dwHintFlags |= FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT;

        CSCUnpinFile(pszItem,
                     dwHintFlags,
                     &dwStatus,
                     &dwPinCount,
                     &dwHintFlags);
                     
        //
        // If it's a file, delete it below on this pass
        //
        bDeleteItem = (ENUM_REASON_FILE == eReason);

        Trace((TEXT("AdminUnpin (%d) %s"), dwPinCount, pszItem));
    }
    else if (ENUM_REASON_FOLDER_END == eReason)
    {
        //
        // Delete any unused folders in the post-order part of the traversal.
        //
        // Note that dwPinCount and dwHintFlags are always 0 in the
        // post-order part of the traversal, so fetch them here.
        //
        bDeleteItem = CSCQueryFileStatus(pszItem, &dwStatus, &dwPinCount, &dwHintFlags);
    }            

    //
    // Delete items that are no longer pinned and have no offline changes
    //
    if (bDeleteItem
        && 0 == dwPinCount && 0 == dwHintFlags
        && !(dwStatus & FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY))
    {
        CscDelete(pszItem);
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


DWORD WINAPI
_PinAdminFoldersThread(LPVOID)
{
    HRESULT hrComInit;

    // Make sure the DLL stays loaded while this thread is running
    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);

    TraceEnter(TRACE_ADMINPIN, "_PinAdminFoldersThread");
    TraceAssert(IsCSCEnabled());

    HANDLE rghSyncObj[] = { g_heventTerminate,
                            g_hmutexAdminPin };

    //
    // Wait until we either own the "admin pin" mutex OR the
    // "terminate" event is set.
    //
    TraceMsg("Waiting for 'admin-pin' mutex or 'terminate' event...");
    DWORD dwWait = WaitForMultipleObjects(ARRAYSIZE(rghSyncObj),
                                          rghSyncObj,
                                          FALSE,
                                          INFINITE);
    if (1 == (dwWait - WAIT_OBJECT_0))
    {
        TraceMsg("Thread now owns 'admin-pin' mutex.");
        //
        // We own the "admin pin" mutex.  OK to perform admin pin.
        //
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
        hrComInit = CoInitialize(NULL);

        //
        // Get the Admin Folders list from the registry
        //
        HDPA hdpaFiles = DPA_Create(10);
        if (NULL != hdpaFiles)
        {
            ReadAPFFromRegistry(hdpaFiles);

            //
            // Iterate through the list and pin the items
            //
            const int cFiles = DPA_GetPtrCount(hdpaFiles);
            for (int i = 0; i < cFiles; i++)
            {
                LPTSTR pszUNC  = NULL;
                LPTSTR pszItem = (LPTSTR)DPA_GetPtr(hdpaFiles, i);
                DWORD dwResult;

                if (!PathIsUNC(pszItem))
                    GetRemotePath(pszItem, &pszUNC);
                    
                if (pszUNC)
                {
                    LocalFreeString(&pszItem);
                    pszItem = pszUNC;
                }
                // else we only try to pin link targets

                // Pin this item
                dwResult = _PinAdminFolderCallback(pszItem, ENUM_REASON_FILE, NULL, 0);

                // Pin everything under this folder (if it's a folder)
                if (CSCPROC_RETURN_CONTINUE == dwResult
                    && PathIsUNC(pszItem))
                {
                    _Win32EnumFolder(pszItem, TRUE, _PinAdminFolderCallback, 0);
                }
                LocalFreeString(&pszUNC);
                LocalFreeString(&pszItem);
            }
            DPA_Destroy(hdpaFiles);
        }        

        //
        // Enumerate the entire CSC database looking for Admin pinned items. For
        // each, reduce the pin count by one. This unpins items that are no longer
        // in the Admin list and balances the pin count for those that remain.
        //
        _CSCEnumDatabase(NULL, TRUE, _UnpinAdminFolderCallback, 0);

        if (SUCCEEDED(hrComInit))
            CoUninitialize();
            
        TraceMsg("Thread releasing 'admin-pin' mutex.");
        ReleaseMutex(g_hmutexAdminPin);            
    }            

    TraceMsg("_PinAdminFoldersThread exiting");
    TraceLeave();
    FreeLibraryAndExitThread(hInstThisDll, 0);
    return 0;
}


//
// Expand all environment strings in a text string.
//
HRESULT
_ExpandStringInPlace(
    LPTSTR psz,
    DWORD cch
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    LPTSTR pszCopy;
    if (LocalAllocString(&pszCopy, psz))
    {
        DWORD cchExpanded = ExpandEnvironmentStrings(pszCopy, psz, cch);
        if (0 == cchExpanded)
            hr = HRESULT_FROM_WIN32(GetLastError());
        else if (cchExpanded > cch)
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else
            hr = S_OK;

        LocalFreeString(&pszCopy);
    }
    if (FAILED(hr) && 0 < cch)
    {
        *psz = TEXT('\0');
    }
    return hr;
}


//
// Version of RegEnumValue that expands environment variables 
// in all string values.
//
LONG
_RegEnumValueExp(
    HKEY hKey,
    DWORD dwIndex,
    LPTSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    DWORD cchNameDest = lpcbValueName ? *lpcbValueName / sizeof(TCHAR) : 0;
    DWORD cchDataDest = lpcbData ? *lpcbData / sizeof(TCHAR) : 0;
    DWORD dwType;
    if (NULL == lpType)
        lpType = &dwType;
        
    LONG lResult = RegEnumValue(hKey,
                                dwIndex,
                                lpValueName,
                                lpcbValueName,
                                lpReserved,
                                lpType,
                                lpData,
                                lpcbData);

    if (ERROR_SUCCESS == lResult)
    {
        HRESULT hr = _ExpandStringInPlace(lpValueName, cchNameDest);
        
        if ((NULL != lpData) && (REG_SZ == *lpType || REG_EXPAND_SZ == *lpType))
        {
            hr = _ExpandStringInPlace((LPTSTR)lpData, cchDataDest);
        }
        lResult = HRESULT_CODE(hr);
    }
    return lResult;
}


