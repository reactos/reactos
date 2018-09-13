/* embed.c - Contains the routines for pseudo-embedding objects.
 *
 * Copyright (c) Microsoft Corporation, 1991-
 *
 * Why is it called pseudo-embedding?  Because it is not embedding
 * the object in the OLE sense; rather, it just sucks the entire
 * file into memory.
 */

#include "packager.h"
#include <shellapi.h>
#include "dialogs.h"
// #include <shell.h>        // For RealShellExecute() call.

#define OLEVERB_EDIT 1


static CHAR szDefTempFile[] = "PKG";
static OLECLIENTVTBL embClivtbl;
static BOOL bOleReleaseError = OLE_OK;
static HWND hTaskWnd;
static INT cEmbWait = 0;


DWORD MainWaitOnChild(LPVOID lpv);
static VOID ReplaceExtension(LPSTR lpstrTempFile, LPSTR lpstrOrigFile);
static DWORD GetFileLength(INT fh);
BOOL CALLBACK GetTaskWndProc(HWND hwnd, LPARAM lParam);
static BOOL EmbError(OLESTATUS retval);



/* EmbActivate() - Performs activation of a pseudo-embedded file.
 *
 * Notes:  Assumes that lpstrFile is in the OEM character set.
 */
BOOL
EmbActivate(
    LPEMBED lpembed,
    UINT wVerb
    )
{
    LPSTR lpFileData = NULL;
    CHAR szAnsi[CBPATHMAX];
    CHAR szFileName[CBPATHMAX];
    CHAR szDefPath[CBPATHMAX];
    CHAR szTemp[CBPATHMAX];
    INT fh;
    BOOL fError = TRUE;
    DWORD id;
    SHELLEXECUTEINFO sheinf = { sizeof(SHELLEXECUTEINFO) };

    //
    // If no hContents, we launched the server at some point...
    // so use the same temporary file name.
    //
    if (!lpembed->hContents)
    {
        if (lpembed->bOleSvrFile)
            return EmbDoVerb(lpembed, wVerb);

        if (lpembed->hTask)
        {
            hTaskWnd = NULL;
            EnumTaskWindows(lpembed->hTask, GetTaskWndProc, 0);

            if (hTaskWnd)
                BringWindowToTop(hTaskWnd);

            return TRUE;
        }

        if (!GetAtomName(lpembed->aTempName, szAnsi, CBPATHMAX))
            goto errRtn;
    }
    else
    {
        if (!GetAtomName(lpembed->aFileName, szFileName, CBPATHMAX)
            || !(lpFileData = GlobalLock(lpembed->hContents)))
            goto errRtn;

        GlobalUnlock(lpembed->hContents);

        // Unfortunately, GetTempFileName() creates a file on the disk
        GetTempPath(MAX_PATH, szDefPath);
        GetTempFileName(szDefPath, szDefTempFile, 0, szTemp);
        OemToAnsi(szTemp, szAnsi);
        DeleteFile(szAnsi);

        // Get the >real< new temp file name
        ReplaceExtension(szAnsi, szFileName);

        if ((fh = _lcreat(szAnsi, 0)) < 0)
            goto errRtn;

        if (_lwrite(fh, lpFileData, lpembed->dwSize) < lpembed->dwSize)
        {
            _lclose(fh);
            DeleteFile(szAnsi);
            goto errRtn;
        }

        _lclose(fh);
    }

    if (lpembed->bOleSvrFile)
    {
        if (!(fError = !EmbActivateThroughOle(lpembed, szAnsi, wVerb)))
        {
            GlobalFree(lpembed->hContents);
            lpembed->aTempName = AddAtom(szAnsi);
            lpembed->dwSize    = 0;
            lpembed->hContents = NULL;
        }

        goto errRtn;
    }

    // Try to execute the file
    lpembed->hTask = NULL;

    // hShellExec = RealShellExecute(NULL, NULL, szAnsi, NULL, NULL, NULL, "",
    //    NULL, SW_SHOWNORMAL, &hProcess);
    sheinf.fMask = SEE_MASK_NOCLOSEPROCESS;
    sheinf.lpFile = szAnsi;
    sheinf.nShow = SW_SHOWNORMAL;

    if (ShellExecuteEx(&sheinf))
    {
        if (lpembed->hContents)
        {
            GlobalFree(lpembed->hContents);
            lpembed->aTempName = AddAtom(szAnsi);
            lpembed->dwSize    = 0;
            lpembed->hContents = NULL;
        }

        if (sheinf.hProcess)
        {
            HANDLE hThd;
            //
            // Now create a thread to wait for the child process to exit.
            //
            if ((hThd = CreateThread(NULL, 0, MainWaitOnChild, (LPVOID)sheinf.hProcess, 0, &id)) != NULL)
            {
                fError = FALSE;
                CloseHandle(hThd);
            }
            else
            {
                CloseHandle(sheinf.hProcess);
                DeleteFile(szAnsi);
                ErrorMessage(E_FAILED_TO_EXECUTE_COMMAND);
            }
        }
        else
        {
            if (gfInvisible)
                PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
        }
    }
    else
    {
        DWORD err = GetLastError();
        DeleteFile(szAnsi);
        ErrorMessage((err == ERROR_NO_ASSOCIATION) ? E_FAILED_TO_FIND_ASSOCIATION
             : E_FAILED_TO_EXECUTE_COMMAND);
    }

errRtn:
    if (fError)
    {
        if (gfInvisible)
            PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
    }
    else
    {
        Dirty();
    }

    return !fError;
}



/*****************************************************************************\
* MainWaitOnChild
*
* Waits for the specified child process to exit, then posts a message
* back to the main window.
*
* Arguments:
*
*   LPVOID lpv - Handle to the child process to wait on.
*
* Returns:
*   0
*
\*****************************************************************************/

DWORD
MainWaitOnChild(
    LPVOID lpv
    )
{
    if (WaitForSingleObject((HANDLE)lpv, INFINITE) == 0)
    {
        PostMessage(ghwndFrame, WM_READEMBEDDED, 0, 0);
    }

    CloseHandle((HANDLE)lpv);

    GetLastError(); //BUGBUG

    return 0;
}



/* EmbCreate() - Performs the pseudo-embedding of a file.
 *
 * Notes:  Assumes that lpstrFile is in the OEM character set.
 *
 *         This function is used by File Import..., is called
 *         when the Embed modifier is used on Drag&Drop, and
 *         is also used when Paste-ing a File manager file.
 */
LPEMBED
EmbCreate(
    LPSTR lpstrFile
    )
{
    ATOM aFileName = 0;
    BOOL fError = TRUE;
    DWORD dwSize = 0;
    HANDLE hdata = NULL;
    HANDLE hFileData = NULL;
    LPEMBED lpembed = NULL;
    LPSTR lpFileData = NULL;
    INT fh = 0;

    if (lpstrFile)
    {
        if ((fh = _lopen(lpstrFile, OF_READ | OF_SHARE_DENY_WRITE)) == HFILE_ERROR)
        {
            ErrorMessage(IDS_ACCESSDENIED);
            goto errRtn;
        }

        // Seek to EOF, then to the top of the file.
        dwSize = GetFileLength(fh);
        if (0 == dwSize)
        {
            ErrorMessage(IDS_NOZEROSIZEFILES);
            goto errRtn;
        }

        if (!(aFileName = AddAtom(lpstrFile))
            || !(hFileData = GlobalAlloc(GMEM_MOVEABLE, dwSize))
            || !(lpFileData = GlobalLock(hFileData)))
        {
            ErrorMessage(IDS_LOWMEM);
            goto errRtn;
        }

        if (_lread(fh, lpFileData, dwSize) != dwSize)
        {
            ErrorMessage(E_FAILED_TO_READ_FILE);
            goto errRtn;
        }
    }

    if (!(hdata = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(EMBED)))
        || !(lpembed = (LPEMBED)GlobalLock(hdata)))
    {
        ErrorMessage(IDS_LOWMEM);
        goto errRtn;
    }

    lpembed->aFileName = aFileName;
    lpembed->dwSize = dwSize;
    lpembed->hContents = hFileData;
    lpembed->hdata = hdata;
    lpembed->bOleSvrFile = IsOleServerDoc(lpstrFile);
    fError = FALSE;

errRtn:
    if (fh)
        _lclose(fh);

    if (lpFileData)
        GlobalUnlock(hFileData);

    if (fError)
    {
        if (hdata)
            GlobalFree(hdata);

        if (aFileName)
            DeleteAtom(aFileName);

        if (hFileData)
            GlobalFree(hFileData);
    }

    return lpembed;
}



/* EmbDelete() - Deallocate the pseudo-embedded file.
 */
VOID
EmbDelete(
    LPEMBED lpembed
    )
{
    HANDLE  hdata;

    if (!lpembed)
        return;

    if (lpembed->lpLinkObj)
    {
        EmbRead(glpobj[CONTENT]);
        EmbDeleteLinkObject(lpembed);
    }
    else {
        /* If the task is active, there's nothing we can do */
#if 0
        if (lpembed->hSvrInst)
            TermToolHelp(lpembed);
#endif  //BUGBUG Does anything need to be done for this case? Like terminating the waiting thread, perhaps?
    }

    if (lpembed->aFileName)
    {
        DeleteAtom(lpembed->aFileName);
        lpembed->aFileName = 0;
    }

    if (lpembed->aTempName)
    {
        DeleteAtom(lpembed->aTempName);
        lpembed->aTempName = 0;
    }

    if (lpembed->hContents)
    {
        GlobalFree(lpembed->hContents);
        lpembed->dwSize = 0;
        lpembed->hContents = NULL;
    }

    GlobalUnlock(hdata = lpembed->hdata);
    GlobalFree(hdata);
}



/* EmbDraw() - Draw the pseudo-embedded object.
 *
 * Note:  This drawing is DESCRIPTION-ONLY.
 */
VOID
EmbDraw(
    LPEMBED lpembed,
    HDC hdc,
    LPRECT lprc,
    BOOL fFocus
    )
{
    RECT rcFocus;
    CHAR szEmbedFile[CBMESSAGEMAX];
    CHAR szFileName[CBPATHMAX];
    CHAR szMessage[CBMESSAGEMAX + CBPATHMAX];

    if (GetAtomName(lpembed->aFileName, szFileName, CBPATHMAX)
        && LoadString(ghInst, IDS_EMBEDFILE, szEmbedFile, CBMESSAGEMAX))
    {
        Normalize(szFileName);
        wsprintf(szMessage, szEmbedFile, (LPSTR)szFileName);

        DrawText(hdc, szMessage, -1, lprc,
            DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (fFocus)
        {
            rcFocus = *lprc;
            DrawText(hdc, szMessage, -1, &rcFocus, DT_CALCRECT | DT_NOPREFIX |
                DT_LEFT | DT_TOP | DT_SINGLELINE);
            OffsetRect(&rcFocus, (lprc->left + lprc->right - rcFocus.right) /
                2, (lprc->top + lprc->bottom - rcFocus.bottom) / 2);
            DrawFocusRect(hdc, &rcFocus);
        }
    }
}



/* EmbReadFromNative() - Reads a pseudo-embedded object from memory.
 *
 * Notes:  This function is called by GetNative().
 */
LPEMBED
EmbReadFromNative(
    LPSTR *lplpstr
    )
{
    BOOL fError = TRUE;
    DWORD dwSize;
    HANDLE hData = NULL;
    LPEMBED lpembed = NULL;
    LPSTR lpData = NULL;
    CHAR szFileName[CBPATHMAX];

    MemRead(lplpstr, (LPSTR)&dwSize, sizeof(dwSize));
    MemRead(lplpstr, (LPSTR)szFileName, dwSize);
    MemRead(lplpstr, (LPSTR)&dwSize, sizeof(dwSize));

    if (!(lpembed = EmbCreate(NULL))
        || !(hData = GlobalAlloc(GMEM_MOVEABLE, dwSize))
        || !(lpData = GlobalLock(hData)))
        goto errRtn;

    MemRead(lplpstr, (LPSTR)lpData, dwSize);

    lpembed->aFileName = AddAtom(szFileName);
    lpembed->dwSize = dwSize;
    lpembed->hContents = hData;
    lpembed->bOleSvrFile = IsOleServerDoc(szFileName);
    fError = FALSE;

errRtn:
    if (lpData)
        GlobalUnlock(hData);

    if (fError)
    {
        if (hData)
            GlobalFree(hData);

        if (lpembed)
        {
            EmbDelete(lpembed);
            lpembed = NULL;
        }
    }

    return lpembed;
}



/* EmbWriteToNative() - Used to save pseudo-embed to memory.
 *
 * Note:  This function is called by GetNative().
 */
DWORD
EmbWriteToNative(
    LPEMBED lpembed,
    LPSTR *lplpstr
    )
{
    BOOL fError = TRUE;
    DWORD cBytes = 0;
    DWORD dwSize;
    HANDLE hData = NULL;
    LPSTR lpData = NULL;
    LPSTR lpFileData = NULL;
    CHAR szFileName[CBPATHMAX];
    INT fhTemp = -1;
    CHAR * hplpstr;

    if (!GetAtomName(lpembed->aFileName, szFileName, CBPATHMAX))
        goto errRtn;

    if (!lplpstr)
    {
        cBytes = lstrlen(szFileName) + 1 + sizeof(dwSize);
    }
    else
    {
        dwSize = lstrlen(szFileName) + 1;
        MemWrite(lplpstr, (LPSTR)&dwSize, sizeof(dwSize));
        MemWrite(lplpstr, (LPSTR)szFileName, dwSize);
    }

    // Read from memory if it's there; otherwise, it's executing
    if (lpembed->hContents)
    {
        cBytes += sizeof(lpembed->dwSize) + lpembed->dwSize;

        if (lplpstr)
        {
            if (!(lpFileData = GlobalLock(lpembed->hContents)))
                goto errRtn;

            MemWrite(lplpstr, (LPSTR)&(lpembed->dwSize), sizeof(lpembed->dwSize));
            MemWrite(lplpstr, (LPSTR)lpFileData, lpembed->dwSize);
        }
    } else {
        int i;

        if (!GetAtomName(lpembed->aTempName, szFileName, CBPATHMAX))
            goto errRtn;

        for (i = 0; i < 5; i++ ) {
            int j;

            fhTemp = _lopen(szFileName, OF_READ | OF_SHARE_DENY_WRITE);

            if (fhTemp != HFILE_ERROR) {
                break;
            }

            /*
             * We could not open the file.  It is probably still open by the
             * server.  Wait 5 seconds for the server to finish closing the
             * file and then try again.
             */
            for (j=0; j<25; j++) {
                MSG msg;
                PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
                Sleep(200);
            }
        }

        /*
         * If after 25 seconds we still could not open the file, then it
         * must be screwed
         */
        if (fhTemp == HFILE_ERROR)
            goto errRtn;

        dwSize = GetFileLength(fhTemp);

        if (!lplpstr)
            cBytes += sizeof(dwSize) + dwSize;
        else
        {
            MemWrite(lplpstr, (LPSTR)&dwSize, sizeof(dwSize));
            _lread(fhTemp, *lplpstr, dwSize);

            // Increment the pointer being read into
            hplpstr = *lplpstr;
            hplpstr += dwSize;
            *lplpstr = hplpstr;
        }
    }

    fError = FALSE;

errRtn:
    if (fhTemp >= 0)
    {
        _lclose(fhTemp);
        if (!fError && lplpstr && !(lpembed->hTask || lpembed->lpLinkObj))
            DeleteFile(szFileName);
    }

    if (lpData)
        GlobalUnlock(hData);

    if (hData)
        GlobalFree(hData);

    if (lpFileData)
        GlobalUnlock(lpembed->hContents);

    return (fError ? ((DWORD)(-1L)) : cBytes);
}



/* EmbWriteToFile() - Used to save pseudo-embed to a file.
 *
 * Note:  This function is called by File Export...
 */
VOID
EmbWriteToFile(
    LPEMBED lpembed,
    INT fh
    )
{
    BOOL fError = TRUE;
    DWORD dwSize;
    HANDLE hData = NULL;
    LPSTR lpData = NULL;
    LPSTR lpFileData = NULL;
    CHAR szTempName[CBPATHMAX];
    INT fhTemp = -1;
    CHAR szMessage[CBMESSAGEMAX];

    // Read from memory if it's there
    if (lpembed->hContents)
    {
        if (!(lpFileData = GlobalLock(lpembed->hContents)))
            goto errRtn;

        if (_lwrite(fh, lpFileData, lpembed->dwSize) != lpembed->dwSize)
            goto errRtn;
    }
    else
    {
        // otherwise, it is/was executing
        if (lpembed->hTask && !gfInvisible)
        {
            // Object being edited, snapshot current contents?
            LoadString(ghInst, IDS_ASKCLOSETASK, szMessage, CBMESSAGEMAX);
            BringWindowToTop(ghwndFrame);
            switch (MessageBoxAfterBlock(ghwndError, szMessage, szAppName,
                 MB_OKCANCEL))
            {
                case IDOK:
                    break;

                case IDCANCEL:
                    return;
            }
        }

        if (!GetAtomName(lpembed->aTempName, szTempName, CBPATHMAX)
            || (fhTemp = _lopen(szTempName, OF_READ | OF_SHARE_DENY_WRITE)) == HFILE_ERROR)
            goto errRtn;

        dwSize = GetFileLength(fhTemp);
        while (dwSize && !(hData = GlobalAlloc(GMEM_MOVEABLE, dwSize)))
            dwSize = dwSize >> 1;

        if (!dwSize || !(lpData = GlobalLock(hData)))
            goto errRtn;

        while (dwSize)
        {
            dwSize = _lread(fhTemp, lpData, dwSize);

            if (dwSize)
                _lwrite(fh, lpData, dwSize);
        }
    }

    fError = FALSE;

errRtn:
    if (fhTemp >= 0)
    {
        _lclose(fhTemp);

        if (!fError && !lpembed->hTask)
            DeleteFile(gszFileName);
    }

    if (lpData)
        GlobalUnlock(hData);

    if (hData)
        GlobalFree(hData);

    if (lpFileData)
        GlobalUnlock(lpembed->hContents);
}



/* ReplaceExtension() - Replaces the extension of the temp file.
 *
 * This routine ensures that the temp file has the same extension as the
 * original file, so that the ShellExecute() will load the same file.
 */
static VOID
ReplaceExtension(
    LPSTR lpstrTempFile,
    LPSTR lpstrOrigFile
    )
{
    LPSTR lpstrBack = NULL;

    // Get temp file extension
    while (*lpstrTempFile)
    {
        if (*lpstrTempFile == '\\')
            lpstrBack = lpstrTempFile;

        if (gbDBCS)
        {
            lpstrTempFile = CharNext(lpstrTempFile);
        }
        else
        {
            lpstrTempFile++;
        }
    }

    while (lpstrBack && *lpstrBack && *lpstrBack != '.')
        lpstrBack++;

    if (lpstrBack && *lpstrBack)
        lpstrTempFile = lpstrBack + 1;

    // Get original file extension
    while (*lpstrOrigFile)
    {
        if (*lpstrOrigFile == '\\')
            lpstrBack = lpstrOrigFile;

        if (gbDBCS)
        {
            lpstrOrigFile = CharNext(lpstrOrigFile);
        }
        else
        {
            lpstrOrigFile++;
        }
    }

    while (lpstrBack && *lpstrBack && *lpstrBack != '.')
        lpstrBack++;

    if (lpstrBack && *lpstrBack)
    {
        lpstrOrigFile = lpstrBack + 1;

        // Move the extension on over
        lstrcpy(lpstrTempFile, lpstrOrigFile);
    }
    else
    {
         /* Wipe out the extension altogether */
        *lpstrTempFile = 0;
    }
}



/* GetFileLength() - Obtain the size of the temporary file used.
 *
 * Returns:         The length of the file in bytes.
 * Side effects:    Resets fh to the beginning of the file.
 */
static DWORD
GetFileLength(
    INT fh
    )
{
    DWORD dwSize;

    dwSize = _llseek(fh, 0L, 2);
    _llseek(fh, 0L, 0);

    return dwSize;
}



/* EmbRead() - Reads the contents back when the task has terminated.
 */
VOID
EmbRead(
    LPEMBED lpembed
    )
{
    BOOL fError = TRUE;
    DWORD dwSize;
    HANDLE hFileData = NULL;
    LPSTR lpFileData = NULL;
    CHAR szTempFileName[CBPATHMAX];
    INT fhTemp = -1;

    if (!lpembed || !lpembed->aTempName)
        return;

    if (!GetAtomName(lpembed->aTempName, szTempFileName, CBPATHMAX))
        return;

    if ((fhTemp = _lopen(szTempFileName, OF_READ | OF_SHARE_DENY_WRITE)) == HFILE_ERROR)
        goto errRtn;

    dwSize = GetFileLength(fhTemp);

    if (!(hFileData = GlobalAlloc(GMEM_MOVEABLE, dwSize))
        || !(lpFileData = GlobalLock(hFileData))
        || (_lread(fhTemp, lpFileData, dwSize) != dwSize))
        goto errRtn;

    DeleteAtom(lpembed->aTempName);
    lpembed->aTempName  = 0;
    lpembed->dwSize     = dwSize;
    lpembed->hContents  = hFileData;
    lpembed->hTask      = NULL;

    fError = FALSE;

errRtn:
    if (fhTemp >= 0)
    {
        _lclose(fhTemp);

        if (!fError)
            DeleteFile(szTempFileName);
    }

    if (lpFileData)
        GlobalUnlock(hFileData);

    if (fError && hFileData)
        GlobalFree(hFileData);
}



BOOL CALLBACK
GetTaskWndProc(
    HWND hwnd,
    LPARAM lParam
    )
{
    if (IsWindowVisible(hwnd))
    {
        hTaskWnd = hwnd;
        return FALSE;
    }

    return TRUE;
}



BOOL
EmbDoVerb(
    LPEMBED lpembed,
    UINT wVerb
    )
{
    if (wVerb == IDD_PLAY)
    {
        if (EmbError(OleActivate(lpembed->lpLinkObj, OLEVERB_PRIMARY, TRUE,
            TRUE, NULL, NULL)))
            return FALSE;
    }
    else
    {
        // it must be verb IDD_EDIT
        if (EmbError(OleActivate(lpembed->lpLinkObj, OLEVERB_EDIT, TRUE,
            TRUE, NULL, NULL)))
            return FALSE;
    }

    WaitForObject(lpembed->lpLinkObj);

    if (bOleReleaseError != OLE_OK)
    {
        bOleReleaseError = OLE_OK;
        return FALSE;
    }

    // if the verb is IDD_PLAY then we need not do any more
    if (wVerb == IDD_PLAY)
        return TRUE;

    // If the verb is IDD_EDIT, then we must show the server, and we will do
    // that by calling server's show method
    if (EmbError((*(lpembed->lpLinkObj)->lpvtbl->Show)(lpembed->lpLinkObj,
        TRUE)))
        return FALSE;

    WaitForObject(lpembed->lpLinkObj);

    if (bOleReleaseError != OLE_OK)
    {
        bOleReleaseError = OLE_OK;
        return FALSE;
    }

    return TRUE;
}



BOOL
EmbActivateThroughOle(
    LPEMBED lpembed,
    LPSTR lpdocname,
    UINT wVerb
    )
{
    bOleReleaseError = OLE_OK;

    if (!(lpembed->lpclient = PicCreateClient(&EmbCallBack, &embClivtbl)))
        return FALSE;

    if (EmbError(OleCreateLinkFromFile(gszProtocol, lpembed->lpclient, NULL,
        lpdocname, NULL, glhcdoc, gszCaption[CONTENT], &lpembed->lpLinkObj,
        olerender_none, 0)))
        return  FALSE;

    WaitForObject(lpembed->lpLinkObj);

    if (bOleReleaseError == OLE_OK)
    {
        if (gfEmbObjectOpen = EmbDoVerb(lpembed, wVerb))
            return TRUE;
    }

    EmbDeleteLinkObject(lpembed);

    return FALSE;
}



/* EmbCallBack() - Routine that OLE client DLL calls when events occur.
 */
INT CALLBACK
EmbCallBack(
    LPOLECLIENT lpclient,
    OLE_NOTIFICATION flags,
    LPOLEOBJECT lpObject
    )
{
    switch (flags)
    {
    case OLE_CLOSED:
    case OLE_SAVED:
    case OLE_CHANGED:
        break;

    case OLE_RELEASE:
        if (cEmbWait)
            --cEmbWait;

        bOleReleaseError = OleQueryReleaseError(lpObject);
        break;

    default:
        break;
    }

    return 0;
}



VOID
EmbDeleteLinkObject(
    LPEMBED lpembed
    )
{
    HGLOBAL hg;

    bOleReleaseError = OLE_OK;

    if (!lpembed->lpLinkObj)
        return;

    WaitForObject(lpembed->lpLinkObj);

    if (!EmbError(OleDelete(lpembed->lpLinkObj)))
        WaitForObject (lpembed->lpLinkObj);

    lpembed->lpLinkObj = NULL;

    if (lpembed->lpclient)
    {
        if (hg = GlobalHandle(lpembed->lpclient))
        {
            GlobalUnlock(hg);
            GlobalFree(hg);
        }

        lpembed->lpclient = NULL;
    }

    gfEmbObjectOpen = FALSE;
    bOleReleaseError = OLE_OK;
}



static BOOL
EmbError(
    OLESTATUS retval
    )
{
    switch (retval)
    {
    case OLE_WAIT_FOR_RELEASE:
        cEmbWait++;
        return FALSE;

    case OLE_OK:
        return FALSE;

    default:
        return TRUE;
    }
}


