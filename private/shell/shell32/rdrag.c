#include "shellprv.h"
#pragma  hdrstop

// warning: this will fail given a UNICODE hDrop on an ANSI build and
// the DRAGINFO is esentially a TCHAR struct with no A/W versions exported
//
// in:
//      hDrop   drop handle
//
// out:
//      a bunch of info about the hdrop
//      (mostly the pointer to the double NULL file name list)
//
// returns:
//      TRUE    the DRAGINFO struct was filled in
//      FALSE   the hDrop was bad
//

STDAPI_(BOOL) DragQueryInfo(HDROP hDrop, DRAGINFO *pdi)
{
    if (hDrop && (pdi->uSize == SIZEOF(DRAGINFO))) 
    {
        LPDROPFILES lpdfx = (LPDROPFILES)GlobalLock((HGLOBAL)hDrop);
        
        pdi->lpFileList = NULL;
        
        if (lpdfx)
        {
            LPTSTR lpOldFileList;
            if (LOWORD(lpdfx->pFiles) == SIZEOF(DROPFILES16))
            {
                //
                // This is Win31-stye HDROP
                //
                LPDROPFILES16 pdf16 = (LPDROPFILES16)lpdfx;
                pdi->pt.x  = pdf16->pt.x;
                pdi->pt.y  = pdf16->pt.y;
                pdi->fNC   = pdf16->fNC;
                pdi->grfKeyState = 0;
                lpOldFileList = (LPTSTR)((LPBYTE)pdf16 + pdf16->pFiles);
            }
            else
            {
                //
                // This is a new (NT-compatible) HDROP.
                //
                pdi->pt.x  = lpdfx->pt.x;
                pdi->pt.y  = lpdfx->pt.y;
                pdi->fNC   = lpdfx->fNC;
                pdi->grfKeyState = 0;
                lpOldFileList = (LPTSTR)((LPBYTE)lpdfx + lpdfx->pFiles);
                
                // there could be other data in there, but all
                // the HDROPs we build should be this size
                ASSERT(lpdfx->pFiles == SIZEOF(DROPFILES));
            }
            
            {
                BOOL fListMatchesBuild;
                
#ifdef UNICODE
                if ((LOWORD(lpdfx->pFiles) == SIZEOF(DROPFILES16)) || lpdfx->fWide == FALSE)
                {
                    fListMatchesBuild = FALSE;
                }
                else
                {
                    fListMatchesBuild = TRUE;
                }
#else
                if ((LOWORD(lpdfx->pFiles) != SIZEOF(DROPFILES16)) && lpdfx->fWide == TRUE)
                {
                    ASSERT(0 && "Unicode drop to Ansi explorer not supported");
                    GlobalUnlock((HGLOBAL)hDrop);
                    return FALSE;
                }
                else
                {
                    fListMatchesBuild = TRUE;
                }
#endif
                if (fListMatchesBuild)
                {
                    LPTSTR pTStr = (LPTSTR) lpOldFileList;
                    LPTSTR pNewFileList;
                    UINT   cChar;
                    
                    // Look for the end of the file list
                    
                    while (*pTStr || *(pTStr + 1))
                    {
                        pTStr++;
                    }
                    pTStr++;    // Advance to last NUL of double terminator
                    
                    cChar = (UINT)(pTStr - lpOldFileList);
                    
                    pNewFileList = (LPTSTR) SHAlloc((cChar + 1) * SIZEOF(TCHAR));
                    if (NULL == pNewFileList)
                    {
                        GlobalUnlock((HGLOBAL)hDrop);
                        return FALSE;
                    }
                    
                    // Copy strings to new buffer and set LPDROPINFO filelist
                    // pointer to point to this new buffer
                    
                    CopyMemory(pNewFileList, lpOldFileList, ((cChar + 1) * SIZEOF(TCHAR)));
                    pdi->lpFileList = pNewFileList;
                }
                else
                {
                    LPXSTR pXStr = (LPXSTR) lpOldFileList;
                    LPTSTR pNewFileList;
                    LPTSTR pSaveFileList;
                    UINT   cChar;
                    UINT   cchConverted;
                    
                    // Look for the end of the file list
                    
                    while (*pXStr || (*(pXStr + 1)))
                    {
                        pXStr++;
                    }
                    pXStr++;   // Advance to the last NUL of the double terminator
                    
                    cChar = (UINT)(pXStr - ((LPXSTR) lpOldFileList));
                    
                    pNewFileList = (LPTSTR) SHAlloc((cChar + 1) * SIZEOF(TCHAR));
                    if (NULL == pNewFileList)
                    {
                        GlobalUnlock((HGLOBAL)hDrop);
                        return FALSE;
                    }
                    pSaveFileList = pNewFileList;
                    
                    pXStr = (LPXSTR) lpOldFileList;
                    
                    do
                    {
#ifdef UNICODE
                        cchConverted = MultiByteToWideChar(CP_ACP, 0, pXStr, -1, 
                            pNewFileList, ((cChar + 1) * SIZEOF(TCHAR)));        // Not really, but... "trust me"
                        
#else
                        ASSERT(0 && "Unicode drop to Ansi explorer not supported");
                        cchConverted = 0;
#endif
                        
                        if (0 == cchConverted)
                        {
                            ASSERT(0 && "Unable to convert HDROP filename ANSI -> UNICODE");
                            GlobalUnlock((HGLOBAL)hDrop);
                            SHFree(pSaveFileList);
                            return FALSE;
                        }
                        
                        pNewFileList += cchConverted;
                        pXStr += lstrlenX(pXStr) + 1;
                    } while (*pXStr);
                    
                    // Add the double-null-terminator to the output list
                    
                    *pNewFileList = 0;
                    pdi->lpFileList = pSaveFileList;
                }
            }
            
            GlobalUnlock((HGLOBAL)hDrop);
            
            return TRUE;
        }
    }
    return FALSE;
}

// 3.1 API

STDAPI_(BOOL) DragQueryPoint(HDROP hDrop, POINT *ppt)
{
    BOOL fRet = FALSE;
    LPDROPFILES lpdfs = (LPDROPFILES)GlobalLock((HGLOBAL)hDrop);
    if (lpdfs)
    {
        if (LOWORD(lpdfs->pFiles) == SIZEOF(DROPFILES16))
        {
            //
            // This is Win31-stye HDROP
            //
            LPDROPFILES16 pdf16 = (LPDROPFILES16)lpdfs;
            ppt->x = pdf16->pt.x;
            ppt->y = pdf16->pt.y;
            fRet = !pdf16->fNC;
        }
        else
        {
            //
            // This is a new (NT-compatible) HDROP
            //
            ppt->x = (UINT)lpdfs->pt.x;
            ppt->y = (UINT)lpdfs->pt.y;
            fRet = !lpdfs->fNC;

            // there could be other data in there, but all
            // the HDROPs we build should be this size
            ASSERT(lpdfs->pFiles == SIZEOF(DROPFILES));
        }
        GlobalUnlock((HGLOBAL)hDrop);
    }

    return fRet;
}

#ifdef WINNT
//
// Unfortunately we need it split out this way because WOW needs to
// able to call a function named DragQueryFileAorW (so it can shorten them)
// BUGBUG - BobDay - If there is time, try to change WOW and the SHELL at
// the same time so that they don't need this function
//
STDAPI_(UINT) DragQueryFileAorW(HDROP hDrop, UINT iFile, void *lpFile, UINT cb, BOOL fNeedAnsi, BOOL fShorten)
{
    UINT i;
    LPDROPFILESTRUCT lpdfs = (LPDROPFILESTRUCT)GlobalLock(hDrop);
    if (lpdfs)
    {
        // see if it is the new format
        BOOL fWide = LOWORD(lpdfs->pFiles) == SIZEOF(DROPFILES) && lpdfs->fWide;
        if (fWide)
        {
            LPWSTR lpList;
            WCHAR szPath[MAX_PATH];

            //
            // UNICODE HDROP
            //

            lpList = (LPWSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

            // find either the number of files or the start of the file
            // we're looking for
            //
            for (i = 0; (iFile == (UINT)-1 || i != iFile) && *lpList; i++)
            {
                while (*lpList++)
                    ;
            }

            if (iFile == (UINT)-1)
                goto Exit;


            iFile = i = lstrlenW(lpList);
            if (fShorten && iFile < MAX_PATH)
            {
                wcscpy(szPath, lpList);
                SheShortenPathW(szPath, TRUE);
                lpList = szPath;
                iFile = i = lstrlenW(lpList);
            }

            if (fNeedAnsi)
            {
                // Do not assume that a count of characters == a count of bytes
                i = WideCharToMultiByte(CP_ACP, 0, lpList, -1, NULL, 0, NULL, NULL);
                iFile = i ? --i : i;
            }

            if (!i || !cb || !lpFile)
                goto Exit;

            if (fNeedAnsi) 
            {
                SHUnicodeToAnsi(lpList, (LPSTR)lpFile, cb);
            } 
            else 
            {
                cb--;
                if (cb < i)
                    i = cb;
                lstrcpynW((LPWSTR)lpFile, lpList, i + 1);
            }
        }
        else
        {
            LPSTR lpList;
            CHAR szPath[MAX_PATH];

            //
            // This is Win31-style HDROP or an ANSI NT Style HDROP
            //
            lpList = (LPSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

            // find either the number of files or the start of the file
            // we're looking for
            //
            for (i = 0; (iFile == (UINT)-1 || i != iFile) && *lpList; i++)
            {
                while (*lpList++)
                    ;
            }

            if (iFile == (UINT)-1)
                goto Exit;

            iFile = i = lstrlenA(lpList);
            if (fShorten && iFile < MAX_PATH)
            {
                strcpy(szPath, lpList);
                SheShortenPathA(szPath, TRUE);
                lpList = szPath;
                iFile = i = lstrlenA(lpList);
            }

            if (!fNeedAnsi)
            {
                i = MultiByteToWideChar(CP_ACP, 0, lpList, -1, NULL, 0);
                iFile = i ? --i : i;
            }

            if (!i || !cb || !lpFile)
                goto Exit;

            if (fNeedAnsi) 
            {
                cb--;
                if (cb < i)
                    i = cb;
    
                lstrcpynA((LPSTR)lpFile, lpList, i + 1);
            } 
            else 
            {
                SHAnsiToUnicode(lpList, (LPWSTR)lpFile, cb);
            }
        }
    }

    i = iFile;

Exit:
    GlobalUnlock(hDrop);

    return i;
}

STDAPI_(UINT) DragQueryFileW(HDROP hDrop, UINT wFile, LPWSTR lpFile, UINT cb)
{
   return DragQueryFileAorW(hDrop, wFile, lpFile, cb, FALSE, FALSE);
}

STDAPI_(UINT) DragQueryFileA(HDROP hDrop, UINT wFile, LPSTR lpFile, UINT cb)
{
   return DragQueryFileAorW(hDrop, wFile, lpFile, cb, TRUE, FALSE);
}

#else // WINNT

STDAPI_(UINT) DragQueryFile(HDROP hDrop, UINT iFile, LPTSTR lpFile, UINT cb)
{
    LPTSTR lpList;
    int i;
    LPDROPFILES lpdfs = (LPDROPFILES)GlobalLock((HGLOBAL)hDrop);

    if (LOWORD(lpdfs->pFiles) == SIZEOF(DROPFILES16))
    {
        //
        // This is Win31-stye HDROP
        //
        LPDROPFILES16 pdf16 = (LPDROPFILES16)lpdfs;
        lpList = (LPTSTR)((LPBYTE)pdf16 + pdf16->pFiles);
    }
    else
    {
        //
        // This is a new (NT-compatible) HDROP
        //
        lpList = (LPTSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

        // there could be other data in there, but all
        // the HDROPs we build should be this size
        ASSERT(lpdfs->pFiles == SIZEOF(DROPFILES));
    }

    /* find either the number of files or the start of the file
     * we're looking for
     */
    for (i = 0; ((int)iFile == -1 || i != (int)iFile) && *lpList; i++)
    {
        while (*lpList++)
            ;
    }
    if (iFile == -1)
        goto Exit;

    iFile = i = lstrlen(lpList);

    if (!i || !cb || !lpFile)
        goto Exit;

    cb--;
    if (cb < (UINT)i)
        i = cb;

    lstrcpyn(lpFile, lpList, cb+1);

    // Note that we are returning the length of the string NOT INCLUDING the
    // NULL.  The DOCs are in error.
    i = iFile;

Exit:
    GlobalUnlock((HGLOBAL)hDrop);

    return i;
}

#endif // !WINNT

STDAPI_(void) DragFinish(HDROP hDrop)
{
    GlobalFree((HGLOBAL)hDrop);
}

STDAPI_(void) DragAcceptFiles(HWND hwnd, BOOL fAccept)
{
    long exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (fAccept)
        exstyle |= WS_EX_ACCEPTFILES;
    else
        exstyle &= (~WS_EX_ACCEPTFILES);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
}
