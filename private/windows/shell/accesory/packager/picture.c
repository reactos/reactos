/* picture.c - This file contains OLE object handling routines.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"
#include "dialogs.h"


static OLECLIENTVTBL clientTbl;
static OLESTREAMVTBL streamTbl;


static VOID PicGetBounds(LPOLEOBJECT lpObject, LPRECT lprc);

/* InitClient() - Initialize the OLE client structures.
 */
BOOL
InitClient(
    VOID
    )
{
    gcfFileName  = (OLECLIPFORMAT)RegisterClipboardFormat("FileName");
    gcfLink      = (OLECLIPFORMAT)RegisterClipboardFormat("ObjectLink");
    gcfNative    = (OLECLIPFORMAT)RegisterClipboardFormat("Native");
    gcfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormat("OwnerLink");

    glpclient = PicCreateClient(&CallBack, (LPOLECLIENTVTBL)&clientTbl);

    if (!(glpStream = (LPAPPSTREAM)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(APPSTREAM))))
        goto Error;

    glpStream->lpstbl = (LPOLESTREAMVTBL)&streamTbl;
    streamTbl.Get = (DWORD (CALLBACK*)(LPOLESTREAM, void FAR*, DWORD))ReadStream;
    streamTbl.Put = (DWORD (CALLBACK*)(LPOLESTREAM, OLE_CONST void FAR*, DWORD))WriteStream;

    return TRUE;

Error:
    if (glpStream)
    {
        GlobalFree(glpStream);
    	glpStream = NULL;
    }

    if (glpclient)
    {
        GlobalFree(glpclient);
        glpclient = NULL;
    }

    return FALSE;
}



/* EndClient() - Clean up for termination.
 */
VOID
EndClient(
    VOID
    )
{
    if (glpStream)
    {
	GlobalFree(glpStream);
        glpStream = NULL;
    }

    if (glpclient)
    {
	GlobalFree(glpclient);
        glpclient = NULL;
    }
}



/* PicCreate() -
 */
LPPICT
PicCreate(
    LPOLEOBJECT lpObject,
    LPRECT lprcObject
    )
{
    HANDLE hpict = NULL;
    LPPICT lppict = NULL;
    RECT rc;

    if (lpObject)
    {
        if (!(hpict = GlobalAlloc(GMEM_MOVEABLE, sizeof(PICT)))
            || !(lppict = (LPPICT)GlobalLock(hpict)))
            goto errRtn;

        //
        // If size of window is specified, use it; otherwise, retrieve
        // the size of the item synchronously.
        //
        if (lprcObject)
            rc = *lprcObject;
        else {
            SetRectEmpty(&rc);
            PicGetBounds(lpObject, &rc);
        }

        // Store the data in the window itself
        lppict->hdata = hpict;
        lppict->lpObject = lpObject;
        lppict->rc = rc;
        lppict->fNotReady = FALSE;
    }

    return lppict;

errRtn:
    ErrorMessage(E_FAILED_TO_CREATE_CHILD_WINDOW);

    if (lppict)
        GlobalUnlock(hpict);

    if (hpict)
        GlobalFree(hpict);

    return NULL;
}



/* PicDelete() - Deletes an object (called when the item window is destroyed).
 */
VOID
PicDelete(
    LPPICT lppict
    )
{
    HANDLE hdata;
    LPOLEOBJECT lpObject;

    if (!lppict)
        return;

    if (lppict && lppict->lpObject)
    {
        lpObject = lppict->lpObject;
        lppict->lpObject = NULL;
        // Wait until the object isn't busy
        WaitForObject(lpObject);

        if (Error(OleDelete(lpObject)))
            ErrorMessage(E_FAILED_TO_DELETE_OBJECT);

        // Wait until the object deletion is complete
        WaitForObject(lpObject);
    }

    GlobalUnlock(hdata = lppict->hdata);
    GlobalFree(hdata);
}



/* PicDraw() - Draws the item associated with hwnd in the DC hDC.
 */
BOOL
PicDraw(
    LPPICT lppict,
    HDC hDC,
    LPRECT lprc,
    INT xHSB,
    INT yVSB,
    BOOL fPicture,
    BOOL fFocus
    )
{
    BOOL fSuccess = FALSE;
    DWORD ot;
    HANDLE hdata;
    HFONT hfont;
    LPOLEOBJECT lpObjectUndo;
    LPSTR lpdata;
    RECT rc;
    RECT rcFocus;
    CHAR szDesc[CBMESSAGEMAX];
    CHAR szFileName[CBPATHMAX];
    CHAR szMessage[CBMESSAGEMAX + CBPATHMAX];
    INT iDelta;
    INT iPane;

    iPane = (lppict == glpobj[CONTENT]);
    lpObjectUndo = (gptyUndo[iPane] == PICTURE)
        ? ((LPPICT)glpobjUndo[iPane])->lpObject : NULL;

    // If drawing the Picture, offset by scroll bars and draw
    if (fPicture)
    {
        if (IsRectEmpty(&(lppict->rc)))
            PicGetBounds(lppict->lpObject, &(lppict->rc));

        rc = lppict->rc;

        // If image is smaller than pane, center horizontally
        if ((iDelta = lprc->right - lppict->rc.right) > 0)
            OffsetRect(&rc, iDelta >> 1, 0);
        else /* else, use the scroll bar value */
            OffsetRect(&rc, -xHSB, 0);

        // If image is smaller than pane, center vertically
        if ((iDelta = lprc->bottom - lppict->rc.bottom) > 0)
            OffsetRect(&rc, 0, iDelta >> 1);
        else /* else, use the scroll bar value */
            OffsetRect(&rc, 0, -yVSB);

        // If we have an object, call OleDraw()
        fSuccess = !Error(OleDraw(lppict->lpObject, hDC, &rc, NULL, NULL));

        if (fFocus)
            DrawFocusRect(hDC, &rc);

        return fSuccess;
    }

    // Otherwise, draw the description string
    OleQueryType(lppict->lpObject, &ot);

    if ((ot == OT_LINK
        && Error(OleGetData(lppict->lpObject, gcfLink, &hdata)))
        || (ot == OT_EMBEDDED
        && Error(OleGetData(lppict->lpObject, gcfOwnerLink, &hdata)))
        || (ot == OT_STATIC
        && (!lpObjectUndo || Error(OleGetData(lpObjectUndo, gcfOwnerLink,
        &hdata)))))
    {
        LoadString(ghInst, IDS_OBJECT, szFileName, CBMESSAGEMAX);
        LoadString(ghInst, IDS_FROZEN, szDesc, CBMESSAGEMAX);
        goto DrawString;
    }

    if (hdata && (lpdata = GlobalLock(hdata)))
    {
        switch (ot)
        {
            case OT_LINK:
                while (*lpdata++)
                    ;

                lstrcpy(szFileName, lpdata);
                Normalize(szFileName);
                LoadString(ghInst, IDS_LINKTOFILE, szDesc, CBMESSAGEMAX);
                break;

            case OT_EMBEDDED:
                RegGetClassId(szFileName, lpdata);
                LoadString(ghInst, IDS_EMBEDFILE, szDesc, CBMESSAGEMAX);
                break;

            case OT_STATIC:
                RegGetClassId(szFileName, lpdata);
                LoadString(ghInst, IDS_FROZEN, szDesc, CBMESSAGEMAX);
                break;
        }

        GlobalUnlock(hdata);

DrawString:
        wsprintf(szMessage, szDesc, szFileName);

        hfont = SelectObject(hDC, ghfontChild);
        DrawText(hDC, szMessage, -1, lprc,
            DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (fFocus)
        {
            rcFocus = *lprc;
            DrawText(hDC, szMessage, -1, &rcFocus,
                DT_CALCRECT | DT_NOPREFIX | DT_LEFT | DT_TOP | DT_SINGLELINE);
            OffsetRect(&rcFocus, (lprc->left + lprc->right - rcFocus.right) / 2,
                (lprc->top + lprc->bottom - rcFocus.bottom) / 2);
            DrawFocusRect(hDC, &rcFocus);
        }

        if (hfont)
            SelectObject(hDC, hfont);

        fSuccess = TRUE;
    }

    return fSuccess;
}



/* PicPaste() - Retrieves an object from the clipboard.
 */
LPPICT
PicPaste(
    BOOL fPaste,
    LPSTR lpstrName
    )
{
    LPOLEOBJECT lpObject;

    if (!OpenClipboard(ghwndFrame))
        return NULL;                    /* Couldn't open the clipboard */

    Hourglass(TRUE);

    // Don't replace the current object unless we're successful
    if (fPaste)
    {
        if (Error(OleCreateFromClip(gszProtocol, glpclient, glhcdoc, lpstrName,
            &lpObject, olerender_draw, 0)))
        {
            if (Error(OleCreateFromClip(gszSProtocol, glpclient, glhcdoc,
                lpstrName, &lpObject, olerender_draw, 0)))
                lpObject = NULL;

        }
    }
    else if (Error(OleCreateLinkFromClip(
        gszProtocol, glpclient, glhcdoc, lpstrName, &lpObject,
        olerender_draw, 0)))
    {
        lpObject = NULL;
    }

    CloseClipboard();
    Hourglass(FALSE);

    if (!lpObject)
        return NULL;

    return PicCreate(lpObject, NULL);
}



/* Error() - check for OLE function error conditions
 *
 * This function increments gcOleWait as appropriate.
 *
 * Pre:      Initialize ghwndError to where the focus should return.
 *
 * Returns:  TRUE  if an immediate error occurred.
 */
BOOL
Error(
    OLESTATUS olestat
    )
{
    DWORD ot;
    INT iPane;

    switch (olestat)
    {
        case OLE_WAIT_FOR_RELEASE:
            gcOleWait++;
            return FALSE;

        case OLE_OK:
            return FALSE;

        case OLE_ERROR_STATIC:              /* Only happens w/ dbl click */
            ErrorMessage(W_STATIC_OBJECT);
            break;

        case OLE_ERROR_ADVISE_PICT:
        case OLE_ERROR_OPEN:                /* Invalid link? */
        case OLE_ERROR_NAME:
            iPane = (GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]);
            if ((LPPICT)glpobj[iPane] == NULL)
            {
                ErrorMessage(E_FAILED_TO_CREATE_OBJECT);
                return FALSE;
            }
            else
            {
                OleQueryType(((LPPICT)glpobj[iPane])->lpObject, &ot);
                if (ot == OT_LINK)
                {
                    if (ghwndError == ghwndFrame)
                    {
                        if (DialogBoxAfterBlock (
                            MAKEINTRESOURCE(DTINVALIDLINK), ghwndError,
                            fnInvalidLink) == IDD_CHANGE)
                            PostMessage(ghwndFrame, WM_COMMAND, IDM_LINKS, 0L);
                    }
                    else
                    {
                        // Failed, but already in Link Properties!!
                        ErrorMessage(E_FAILED_TO_UPDATE_LINK);
                    }

                    return FALSE;
                }
            }

            break;

        default:
            break;
    }

    return TRUE;
}



/* CallBack() - Routine that OLE client DLL calls when events occur.
 *
 * This routine is called when the object has been updated and may
 * need to be redisplayed; if asynchronous operations have completed;
 * and if the application allows the user to cancel long operations
 * (like Painting, or other asynchronous operations).
 */
INT CALLBACK
CallBack(
    LPOLECLIENT lpclient,
    OLE_NOTIFICATION flags,
    LPOLEOBJECT lpObject
    )
{
    INT iPane;

    switch (flags)
    {
        case OLE_CLOSED:
            if (gfInvisible)
                PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
            else
                SetFocus(ghwndError);

            break;

        case OLE_SAVED:
        case OLE_CHANGED:
            {
                //
                // The OLE libraries make sure that we only receive
                // update messages according to the Auto/Manual flags.
                //
                iPane = (gpty[CONTENT] == PICTURE
                    && ((LPPICT)glpobj[CONTENT])->lpObject == lpObject);

                if (gpty[iPane] == PICTURE)
                {
                    ((LPPICT)glpobj[iPane])->fNotReady = FALSE;
                    InvalidateRect(ghwndPane[iPane], NULL, TRUE);
                    SetRect(&(((LPPICT)glpobj[iPane])->rc), 0, 0, 0, 0);
                    Dirty();
                }

                break;
            }

        case OLE_RELEASE:
            {
                if (gcOleWait)
                    gcOleWait--;
                else
                    ErrorMessage(E_UNEXPECTED_RELEASE);

                switch (Error(OleQueryReleaseError(lpObject)))
                {
                    case FALSE:
                        switch (OleQueryReleaseMethod(lpObject))
                        {
                            case OLE_SETUPDATEOPTIONS:
                                iPane = (gpty[CONTENT] == PICTURE
                                    && ((LPPICT)glpobj[CONTENT])->lpObject ==
                                    lpObject);

                                PostMessage(ghwndPane[iPane], WM_COMMAND,
                                    IDM_LINKDONE, 0L);

                            default:
                                break;
                        }

                        break;

                    case TRUE:
                        switch (OleQueryReleaseMethod(lpObject))
                        {
                            case OLE_DELETE:
                                ErrorMessage(E_FAILED_TO_DELETE_OBJECT);
                                break;

                            case OLE_LOADFROMSTREAM:
                                ErrorMessage(E_FAILED_TO_READ_OBJECT);
                                break;

                            case OLE_LNKPASTE:
                                ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                                break;

                            case OLE_ACTIVATE:
                                ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
                                break;

                            case OLE_UPDATE:
                                ErrorMessage(E_FAILED_TO_UPDATE);
                                break;

                            case OLE_RECONNECT:
                                ErrorMessage(E_FAILED_TO_RECONNECT_OBJECT);
                                break;
                        }

                        break;
                }

                break;
            }

        case OLE_QUERY_RETRY:
            // if lpObject doesn't match any one of these 4 objects, it means
            // that PicDelete() has been called on lpObject, so there is no
            // point in continueing the RETRIES.
            // See PicDelete() code for more info.
            if ((glpobj[CONTENT]
                && lpObject == ((LPPICT)glpobj[CONTENT])->lpObject)
                || (glpobj[APPEARANCE]
                && lpObject == ((LPPICT) glpobj[APPEARANCE])->lpObject)
                || (glpobjUndo[CONTENT]
                && lpObject == ((LPPICT) glpobjUndo[CONTENT])->lpObject)
                || (glpobjUndo[APPEARANCE]
                && lpObject == ((LPPICT) glpobjUndo[APPEARANCE])->lpObject))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }

        case OLE_QUERY_PAINT:
            return TRUE;

        default:
            break;
    }

    return 0;
}



/* WaitForObject() - Waits, dispatching messages, until the object is free.
 *
 * If the object is busy, spin in a dispatch loop.
 */
VOID
WaitForObject(
    LPOLEOBJECT lpObject
    )
{
    while (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
        ProcessMessage();
}



/* PicSetUpdateOptions() - Sets the update options of the object.
 *
 * Returns:  TRUE if the command completed synchronously.
 */
BOOL
PicSetUpdateOptions(
    LPPICT lppict,
    UINT idCmd
    )
{
    OLESTATUS olestat = OLE_ERROR_GENERIC;

    olestat = OleSetLinkUpdateOptions(
        lppict->lpObject,
        (idCmd == IDD_AUTO) ? oleupdate_always : oleupdate_oncall);

    if (Error(olestat))
        ErrorMessage(E_FAILED_TO_UPDATE_LINK);

    return (olestat == OLE_OK);
}



/* PicReadFromNative() - Reads an object from the pointer lpstr.
 *
 * SIDE EFFECT:  Advances the pointer past the object.
 */
LPPICT
PicReadFromNative(
    LPSTR *lplpstr,
    LPSTR lpstrName
    )
{
    LPOLEOBJECT lpObject;
    LPSTR lpstrStart;
    RECT rcObject;
    WORD w;

    // Save current position of file pointer
    lpstrStart = *lplpstr;
    SetFile(SOP_MEMORY, 0, lplpstr);

    // Load the new object
    if (Error(OleLoadFromStream((LPOLESTREAM)glpStream, gszProtocol, glpclient,
        glhcdoc, lpstrName, &lpObject)))
    {
        // Reset file pointer, and try again
        *lplpstr = lpstrStart;
        SetFile(SOP_MEMORY, 0, lplpstr);

        // Read it with the "Static" protocol
        if (Error(OleLoadFromStream((LPOLESTREAM)glpStream, gszSProtocol,
            glpclient, glhcdoc, lpstrName, &lpObject)))
            return NULL;
    }

    MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
    rcObject.left = (INT)w;
    MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
    rcObject.top = (INT)w;
    MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
    rcObject.right = (INT)w;
    MemRead(lplpstr, (LPSTR)&w, sizeof(WORD));
    rcObject.bottom = (INT)w;

    // Create a window at the right place, and display the object
    return PicCreate(lpObject, &rcObject);
}



/* PicWriteToNative() - Writes an object to memory.
 *
 * SIDE EFFECT:  Moves pointer to end of written object
 */
DWORD
PicWriteToNative(
    LPPICT lppict,
    LPOLEOBJECT lpObject,
    LPSTR *lplpstr
    )
{
    DWORD cb = 0L;
    WORD w;

    // Save the object
    SetFile(SOP_MEMORY, 0, lplpstr);

    if (Error(OleSaveToStream(lpObject, (LPOLESTREAM)glpStream)))
        goto Done;

    cb += gcbObject;

    if (lplpstr)
    {
        w = (WORD)lppict->rc.left;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
        w = (WORD)lppict->rc.top;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
        w = (WORD)lppict->rc.right;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
        w = (WORD)lppict->rc.bottom;
        MemWrite(lplpstr, (LPSTR)&w, sizeof(WORD));
    }

    cb += (DWORD)(4 * sizeof(WORD));

Done:
    return cb;
}



/* Hourglass() - Puts up the hourglass as needed.
 */
VOID
Hourglass(
    BOOL fOn
    )
{
    static HCURSOR hcurSaved = NULL;    // Cursor saved when hourglass is up
    static UINT cWait = 0;              // Number of "Hourglass"es up

    if (fOn)
    {
        if (!(cWait++))
            hcurSaved = SetCursor(ghcurWait);
    }
    else
    {
        if (!(--cWait) && hcurSaved)
        {
            SetCursor(hcurSaved);
            hcurSaved = NULL;
        }
    }
}



VOID
PicActivate(
    LPPICT lppict,
    UINT idCmd
    )
{
    DWORD ot;
    DWORD ot2;
    RECT rc;
    INT iPane;
    BOOL bAlreadyOpen = FALSE;

    iPane = (lppict == glpobj[CONTENT]);
    OleQueryType(lppict->lpObject, &ot);
    if (ot != OT_STATIC)
    {
        // Compute the window dimensions
        GetClientRect(ghwndPane[iPane], &rc);
        bAlreadyOpen = (OleQueryOpen(lppict->lpObject) == OLE_OK);

        // Open the object
        if (Error(OleActivate(lppict->lpObject,
            (idCmd == IDD_PLAY ? OLE_PLAY : OLE_EDIT),
            TRUE, TRUE, ghwndPane[iPane], &rc)))
        {
            ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
            goto errRtn;
        }
        else
        {
            WaitForObject(lppict->lpObject);
            if (!glpobj[iPane])
                goto errRtn;

            OleQueryType(lppict->lpObject, &ot2);
            if (ot2 == OT_EMBEDDED)
                Error(OleSetHostNames(lppict->lpObject, gszAppClassName,
                    (iPane == CONTENT) ? szContent : szAppearance));
        }

        return;
    }
    else
    {
        ErrorMessage(W_STATIC_OBJECT);
    }

errRtn:
    if (gfInvisible && !bAlreadyOpen)
        PostMessage(ghwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
}



VOID
PicUpdate(
    LPPICT lppict
    )
{
    DWORD ot;

    OleQueryType(lppict->lpObject, &ot);
    if (ot == OT_LINK)
    {
        if (Error(OleUpdate(lppict->lpObject)))
            ErrorMessage(E_FAILED_TO_UPDATE);
    }
}



VOID
PicFreeze(
    LPPICT lppict
    )
{
    DWORD ot;
    LPOLEOBJECT lpObject;
    INT iPane;

    iPane = (lppict == glpobj[CONTENT]);
    OleQueryType(lppict->lpObject, &ot);
    if (ot != OT_STATIC)
    {
        if (Error(OleObjectConvert(lppict->lpObject, gszSProtocol, glpclient,
             glhcdoc, gszCaption[iPane], &lpObject)))
        {
            ErrorMessage(E_FAILED_TO_FREEZE);
            return;
        }

        if (Error(OleDelete(lppict->lpObject)))
            ErrorMessage(E_FAILED_TO_DELETE_OBJECT);

        lppict->lpObject = lpObject;

        // Redraw the list box contents
        PostMessage(ghwndError, WM_REDRAW, 0, 0L);
    }
}



VOID
PicChangeLink(
    LPPICT lppict
    )
{
    HANDLE hData;
    OLESTATUS olestat;

    // Change the link information
    olestat = OleGetData(lppict->lpObject, gcfLink, &hData);
    if (!Error(olestat) && hData)
    {
        hData = OfnGetNewLinkName(ghwndError, hData);
        if (hData && !Error(OleSetData(lppict->lpObject, gcfLink, hData)))
            PostMessage(ghwndError, WM_REDRAW, 0, 0L);
    }
}



/* PicCopy() - Puts an object onto the clipboard.
 *
 * Returns:  TRUE iff successful.
 */
BOOL
PicCopy(
    LPPICT lppict
    )
{
    BOOL fSuccess = FALSE;

    // If we can't open the clipboard, fail
    if (!lppict->lpObject || !OpenClipboard(ghwndFrame))
        return FALSE;

    // Empty the clipboard
    EmptyClipboard();

    // Successful if we managed to copy to the clipboard
    fSuccess = !Error(OleCopyToClipboard(lppict->lpObject));

    CloseClipboard();
    return fSuccess;
}



/* PicGetBounds() -
 */
static VOID
PicGetBounds(
    LPOLEOBJECT lpObject,
    LPRECT lprc
    )
{
    if (IsRectEmpty(lprc))
    {
        switch (OleQueryBounds(lpObject, lprc))
        {
            case OLE_WAIT_FOR_RELEASE:
                Hourglass(TRUE);
                gcOleWait++;
                WaitForObject(lpObject);
                Hourglass(FALSE);

            case OLE_OK:
                // Map from HIMETRIC into screen coordinates
                lprc->right = MulDiv(giXppli,
                    lprc->right - lprc->left, HIMETRIC_PER_INCH);
                lprc->bottom = MulDiv (giYppli,
                    lprc->top - lprc->bottom, HIMETRIC_PER_INCH);
                lprc->left = 0;
                lprc->top = 0;

            default:
                break;
        }
    }
}


/* PicSaveUndo() - Saves a copy of the object for Undo.
 */
VOID
PicSaveUndo(
    LPPICT lppict
    )
{
    INT iPane = (lppict == glpobj[CONTENT]);
    LPOLEOBJECT lpObject;

    // Clone the object
    if (Error(OleClone(lppict->lpObject, glpclient, glhcdoc, gszTemp, &lpObject))
        || !lpObject)
    {
        ErrorMessage(W_FAILED_TO_CLONE_UNDO);
    }
    else
    {
        // Save the undo, delete the prior Undo
        DeletePane(iPane, FALSE);
        OleRename(lpObject, gszCaption[iPane]);
        glpobj[iPane] = PicCreate(lpObject, &(lppict->rc));
        gpty[iPane] = PICTURE;

        if (iPane == CONTENT)
            EnableWindow(ghwndPict, TRUE);
    }
}



/* PicPaste() - Creates object from a file
 */
LPPICT
PicFromFile(
    BOOL fEmbedded,
    LPSTR szFile
    )
{
    LPOLEOBJECT lpObject;

    Hourglass(TRUE);

    // Don't replace the current object unless we're successful
    if (fEmbedded)
    {
        if (Error(OleCreateFromFile(gszProtocol, glpclient, NULL, szFile,
            glhcdoc, gszCaption[CONTENT], &lpObject, olerender_draw, 0)))
            lpObject = NULL;
    }
    else
    {
        if (Error(OleCreateLinkFromFile(gszProtocol, glpclient, NULL, szFile,
            NULL, glhcdoc, gszCaption[CONTENT], &lpObject, olerender_draw, 0)))
            lpObject = NULL;
    }

    Hourglass(FALSE);

    if (!lpObject)
        return NULL;

    WaitForObject(lpObject);

    return PicCreate(lpObject, NULL);
}



LPOLECLIENT
PicCreateClient(
    PCALL_BACK fnCallBack,
    LPOLECLIENTVTBL lpclivtbl
    )
{
    LPOLECLIENT pclient;
    if (!(pclient = (LPOLECLIENT)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(OLECLIENT))))
        return NULL;

    pclient->lpvtbl = lpclivtbl;
    pclient->lpvtbl->CallBack = fnCallBack;

    return pclient;
}
