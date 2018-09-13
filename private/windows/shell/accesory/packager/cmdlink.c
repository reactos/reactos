/* cmdlink.c - Handles command line/pseudo-link objects.
 */

#include "packager.h"
#include <shellapi.h>

DWORD CmlWaitForChildProc( LPVOID lpv ) {
    if (WaitForSingleObject((HANDLE)lpv, INFINITE) == 0) {

        if (gfInvisible) {
            PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
        }
    }

    CloseHandle((HANDLE)lpv);

    return 0;
}

/* CmlActivate() - Activate the command line/pseudo-linked file.
 */
VOID
CmlActivate(
    LPCML lpcml
    )
{
    LPSTR lpstrTemp = (LPSTR)lpcml->szCommand;
    CHAR chSave = 0;
    DWORD err;
    BOOL fInQuote = FALSE;
    SHELLEXECUTEINFO sexi;
    HANDLE hThd;
    DWORD id;

    /* skip leading spaces */
    while (*lpstrTemp && *lpstrTemp == CHAR_SPACE)
        lpstrTemp = CharNext(lpstrTemp);


    /* find first non-quoted space */
    for (; *lpstrTemp && (*lpstrTemp != CHAR_SPACE || fInQuote); lpstrTemp = CharNext(lpstrTemp)){
        if (*lpstrTemp == CHAR_QUOTE) {
            fInQuote = !fInQuote;
        }
    }

    if (*lpstrTemp)
    {
        chSave = *lpstrTemp;
        *lpstrTemp++ = 0;
    }

    ZeroMemory(&sexi, sizeof(sexi));
    sexi.cbSize = sizeof(sexi);
    sexi.fMask = SEE_MASK_NOCLOSEPROCESS;
    sexi.hwnd = NULL;
    sexi.lpVerb = NULL;
    sexi.lpFile = lpcml->szCommand;
    sexi.lpParameters = lpstrTemp;
    sexi.lpDirectory = NULL;
    sexi.nShow = SW_SHOWNORMAL;

    DPRINT("packager: Calling ShellExecute");

    //
    //if ((err = ShellExecute(NULL, NULL, lpcml->szCommand, lpstrTemp, NULL, SW_SHOWNORMAL)) <= (HANDLE)32)
    //
    err = NO_ERROR;

    if (ShellExecuteEx(&sexi) && sexi.hProcess != NULL) {
        // Start a thread to wait on the app and close packager once it has ended
        if ((hThd = CreateThread( NULL, 0, CmlWaitForChildProc, (LPVOID)(sexi.hProcess), 0, &id )) == NULL) {
            CloseHandle(sexi.hProcess);
            err = GetLastError();
        } else {
            CloseHandle(hThd);
        }
    } else
        err = GetLastError();

    if (err != NO_ERROR)
        ErrorMessage((err == ERROR_NO_ASSOCIATION) ? E_FAILED_TO_FIND_ASSOCIATION : E_FAILED_TO_EXECUTE_COMMAND);

    DPRINT("packager: Back from ShellExecute");

    if (chSave)
        *(--lpstrTemp) = chSave;
}



/* CmlClone() -
 */
LPCML
CmlClone(
    LPCML lpcml
    )
{
    return CmlCreate(lpcml->szCommand, lpcml->fCmdIsLink);
}



/* CmlCreate() -
 */
LPCML
CmlCreateWorker(
    LPSTR lpstrCmd,
    BOOL fCmdIsLink,
    BOOL fFileName)
{
    HANDLE hdata = NULL;
    LPCML lpcml = NULL;

    if (!(hdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(CML))) ||
        !(lpcml = (LPCML)GlobalLock(hdata)))
        goto errRtn;

    // Store the data in the window itself
    lpcml->hdata = hdata;
    lpcml->fCmdIsLink = fCmdIsLink;

    /*
     * If it is not a single filename,
     *      or the filename does not have a space in it,
     *      or the 'filename' has double qoute characters in it, then
     * just copy it without quoting.
     */
    if (!fFileName || strchr( lpstrCmd, CHAR_SPACE ) == NULL ||
            strchr( lpstrCmd, CHAR_QUOTE ) != NULL)

        lstrcpy(lpcml->szCommand, lpstrCmd);

    else {
        LPSTR psz = lpcml->szCommand;
        *psz++ = CHAR_QUOTE;
        lstrcpy(psz, lpstrCmd);
        lstrcat(lpcml->szCommand, SZ_QUOTE);
    }
    CmlFixBounds(lpcml);

    return lpcml;

errRtn:
    ErrorMessage(E_FAILED_TO_CREATE_CHILD_WINDOW);

    if (lpcml)
        GlobalUnlock(hdata);

    if (hdata)
        GlobalFree(hdata);

    return NULL;
}



/* CmlDelete() - Wipe out the command line.
 */
VOID
CmlDelete(
    LPCML lpcml
    )
{
    HANDLE hdata;

    if (lpcml)
    {
        GlobalUnlock(hdata = lpcml->hdata);
        GlobalFree(hdata);
    }
}



/* CmlDraw() - Draw the command line, centered nicely.
 */
VOID
CmlDraw(
    LPCML lpcml,
    HDC hdc,
    LPRECT lprc,
    INT xHSB,
    BOOL fFocus
    )
{
    HFONT hfont;
    RECT rcFocus;
    CHAR szDesc[CBSTRINGMAX];
    CHAR szFile[CBPATHMAX];
    CHAR szMessage[CBSTRINGMAX + CBPATHMAX];
    RECT rc;

    hfont = SelectObject(hdc, ghfontChild);

    if (lpcml->fCmdIsLink)
    {
        LoadString(ghInst, IDS_LINKTOFILE, szDesc, CharCountOf(szDesc));
        lstrcpy(szFile, lpcml->szCommand);
        Normalize(szFile);
        wsprintf(szMessage, szDesc, (LPSTR)szFile);

        DrawText(hdc, szMessage, -1, lprc, DT_SINGLELINE | DT_NOPREFIX |
            DT_CENTER | DT_VCENTER);

        if (fFocus)
        {
            rcFocus = *lprc;
            DrawText(hdc, szMessage, -1, &rcFocus, DT_CALCRECT | DT_SINGLELINE |
                DT_NOPREFIX | DT_LEFT | DT_TOP);
            OffsetRect(&rcFocus, (lprc->left + lprc->right - rcFocus.right) /
                2, (lprc->top + lprc->bottom - rcFocus.bottom) / 2);
            DrawFocusRect(hdc, &rcFocus);
        }
    }
    else
    {
        rc = *lprc;

        // We should have scroll bars, the text is wider than the window
        if (rc.right < lpcml->rc.right)
        {
            rc.right = lpcml->rc.right;
            OffsetRect(&rc, -xHSB, 0);
        }

        DrawText(hdc, lpcml->szCommand, -1, &rc, DT_SINGLELINE | DT_NOPREFIX |
            DT_CENTER | DT_VCENTER);

        if (fFocus)
            DrawFocusRect(hdc, &rc);
    }

    if (hfont)
        SelectObject(hdc, hfont);
}



/* CmlFixBounds() -
 */
VOID
CmlFixBounds(
    LPCML lpcml
    )
{
    HDC hdc;
    HFONT hfont;

    // Figure out how large the text region will be
    if (*lpcml->szCommand)
    {
        if (hdc = GetWindowDC(ghwndFrame))
        {
            hfont = SelectObject(hdc, ghfontChild);

            SetRect(&(lpcml->rc), 0, 0, 20000, 100);
            DrawText(hdc, lpcml->szCommand, -1, &(lpcml->rc), DT_CALCRECT |
                DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

            if (hfont)
                SelectObject(hdc, hfont);

            ReleaseDC(ghwndFrame, hdc);
        }
    }
    else
    {
        SetRect(&(lpcml->rc), 0, 0, 0, 0);
    }

    PostMessage(ghwndPane[CONTENT], WM_FIXSCROLL, 0, 0L);
}



/* CmlReadFromNative() - Read a command line object from the native data.
 */
LPCML
CmlReadFromNative(
    LPSTR *lplpstr
    )
{
    BOOL fCmdIsLink;
    WORD w;
    CHAR szCmd[CBCMDLINKMAX];

    MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
    fCmdIsLink = (BOOL)w;
    lstrcpy(szCmd, *lplpstr);
    *lplpstr += lstrlen(szCmd) + 1;

    return CmlCreate(szCmd, fCmdIsLink);
}



/* CmlWriteToNative() - Write a command line object to the native data.
 */
DWORD
CmlWriteToNative(
    LPCML lpcml,
    LPSTR *lplpstr
    )
{
    WORD w;

    if (lplpstr)
    {
        w = (WORD)lpcml->fCmdIsLink;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
        MemWrite(lplpstr, (LPSTR)lpcml->szCommand,
            lstrlen(lpcml->szCommand) + 1);
    }

    return sizeof(WORD) + lstrlen(lpcml->szCommand) + 1;
}
