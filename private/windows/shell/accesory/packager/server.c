/* server.c - This module contains the OLE server worker/public routines.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"


#define CBLINKMAX           260


/*
 * This server only supports one document per instance.  The items are
 * just rectangles over the document, possibly overlapping.
 */

static LHCLIENTDOC lhClipDoc = 0;
static OLESERVERDOCVTBL vdocvtbl;           // Document virtual table
static OLEOBJECTVTBL vitemvtbl;             // Item virtual table
static OLESERVERVTBL vsrvrvtbl;             // Server virtual table
static LPSAMPITEM vlpitem[CITEMSMAX];       // Pointers to active OLE items
static INT cItems = 0;                      // Number of active OLE items
static CHAR szClip[] = "Clipboard";


static VOID DeleteDoc(LPSAMPDOC lpdoc);
static BOOL SendItemChangeMsg(LPSAMPITEM lpitem, UINT options);
static INT FindItem(LPSAMPITEM lpitem);



/************ Server initialization and termination routines **********/
/* InitServer() - Initializes the OLE server
 */
BOOL
InitServer(
    VOID
    )
{
    // Allocate the server block
    if (!(ghServer = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof(PBSRVR)))
        || !(glpsrvr = (LPSAMPSRVR)LocalLock(ghServer)))
        goto errRtn;

    // Initialize the server, document, and item virtual tables
    vsrvrvtbl.Open                  = SrvrOpen;
    vsrvrvtbl.Create                = SrvrCreate;
    vsrvrvtbl.CreateFromTemplate    = SrvrCreateFromTemplate;
    vsrvrvtbl.Edit                  = SrvrEdit;
    vsrvrvtbl.Exit                  = SrvrExit;
    vsrvrvtbl.Release               = SrvrRelease;
    vsrvrvtbl.Execute               = SrvrExecute;

    vdocvtbl.Save                   = DocSave;
    vdocvtbl.Close                  = DocClose;
    vdocvtbl.SetHostNames           = DocSetHostNames;
    vdocvtbl.SetDocDimensions       = DocSetDocDimensions;
    vdocvtbl.GetObject              = DocGetObject;
    vdocvtbl.Release                = DocRelease;
    vdocvtbl.SetColorScheme         = DocSetColorScheme;
    vdocvtbl.Execute                = DocExecute;

    vitemvtbl.QueryProtocol         = ItemQueryProtocol;
    vitemvtbl.Release               = ItemDelete;
    vitemvtbl.Show                  = ItemShow;
    vitemvtbl.DoVerb                = ItemDoVerb;
    vitemvtbl.GetData               = ItemGetData;
    vitemvtbl.SetData               = ItemSetData;
    vitemvtbl.SetTargetDevice       = ItemSetTargetDevice;
    vitemvtbl.SetBounds             = ItemSetBounds;
    vitemvtbl.EnumFormats           = ItemEnumFormats;
    vitemvtbl.SetColorScheme        = ItemSetColorScheme;


    // Try to register the server
    glpsrvr->olesrvr.lpvtbl = &vsrvrvtbl;
    if (Error(OleRegisterServer(gszAppClassName, (LPOLESERVER)glpsrvr,
        (LONG_PTR * )&glpsrvr->lhsrvr, ghInst, OLE_SERVER_MULTI)))
        goto errRtn;

    // Initialize the client name
    lstrcpy(gszClientName, "");

    return TRUE;

errRtn:
    ErrorMessage(E_FAILED_TO_REGISTER_SERVER);

    // If we failed, clean up
    if (glpsrvr)
    {
        LocalUnlock(ghServer);
        glpsrvr = NULL;
    }

    if (ghServer)
        LocalFree(ghServer);

    ghServer = NULL;

    return FALSE;
}



/* DeleteServer() - Revokes the OLE server.
 */
VOID
DeleteServer(
    LPSAMPSRVR lpsrvr
    )
{
    if (gfServer)
    {
        gfServer = FALSE;
        OleRevokeServer(lpsrvr->lhsrvr);
    }
}



/* DestroyServer() - Deallocates the OLE server.
 */
VOID
DestroyServer(
    VOID
    )
{
    if (ghServer)
    {
        // Release the server virtual table and info
        LocalUnlock(ghServer);
        LocalFree(ghServer);
        ghServer = NULL;

        // Destroy the window only when we're all through
        DestroyWindow(ghwndFrame);
        gfServer = FALSE;
    }
}



/********************* Document support functions ********************/
/* InitDoc() - Initialize and register the document with the OLE library.
 */
LPSAMPDOC
InitDoc(
    LPSAMPSRVR lpsrvr,
    LHSERVERDOC lhdoc,
    LPSTR lptitle
    )
{
    HANDLE hdoc = NULL;
    LPSAMPDOC lpdoc = NULL;

    if (!(hdoc = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof(PBDOC)))
        || !(lpdoc = (LPSAMPDOC)LocalLock(hdoc)))
        goto errRtn;

    lpdoc->hdoc = hdoc;
    lpdoc->aName = GlobalAddAtom(lptitle);
    lpdoc->oledoc.lpvtbl = &vdocvtbl;

    if (!lhdoc)
    {
        if (Error(OleRegisterServerDoc(lpsrvr->lhsrvr, lptitle,
            (LPOLESERVERDOC)lpdoc, (LHSERVERDOC * ) & lpdoc->lhdoc)))
            goto errRtn;
    }
    else
    {
        lpdoc->lhdoc = lhdoc;
    }

    gfDocExists  = TRUE;
    gfDocCleared = FALSE;
    return lpdoc;

errRtn:
    ErrorMessage(E_FAILED_TO_REGISTER_DOCUMENT);

    // Clean up
    if (lpdoc)
        LocalUnlock(hdoc);

    if (hdoc)
        LocalFree(hdoc);

    return NULL;
}



/* DeleteDoc() - Notify the OLE library that the document is to be deleted.
 */
static VOID
DeleteDoc(
    LPSAMPDOC lpdoc
    )
{
    if (gfOleClosed)
        SendDocChangeMsg(lpdoc, OLE_CLOSED);

    OleRevokeServerDoc(lpdoc->lhdoc);
}



/* ChangeDocName() - Notify the OLE library that the document name is changing.
 */
VOID
ChangeDocName(
    LPSAMPDOC *lplpdoc,
    LPSTR lpname
    )
{
    // If the document exists, delete and re-register.
    if (*lplpdoc)
    {
        GlobalDeleteAtom((*lplpdoc)->aName);
        (*lplpdoc)->aName = GlobalAddAtom(lpname);

        //
        // If the document contains items, just notify the children.
        // If we aren't embedded, always delete and re-register.
        //
        OleRenameServerDoc((*lplpdoc)->lhdoc, lpname);
        if (gfEmbedded && cItems)
            return;

        DeleteDoc(*lplpdoc);
    }

    *lplpdoc = InitDoc(glpsrvr, 0, lpname);
}



/* SendDocChangeMsg() - Notify the client that the document has changed.
 */
BOOL
SendDocChangeMsg(
    LPSAMPDOC lpdoc,
    UINT options
    )
{
    BOOL fSuccess = FALSE;
    INT i;

    for (i = 0; i < cItems; i++)
    {
        if (SendItemChangeMsg(vlpitem[i], options))
            fSuccess = TRUE;
    }

    return fSuccess;
}



/* CreateNewDoc() - Called when a document is newly created.
 *
 * Returns: hDocument if document successfully created, NULL otherwise.
 * Note:    This function is only called when the document is being
 *          created through OLE actions.
 */
LPSAMPDOC
CreateNewDoc(
    LPSAMPSRVR lpsrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpstr
    )
{
    glpdoc = InitDoc(lpsrvr, lhdoc, lpstr);
    lstrcpy(szUntitled, lpstr);
    SetTitle(TRUE);

    return glpdoc;
}



/* CreateDocFromFile() - Called when a document is to be created from a file.
 *
 * Returns: hDocument if document successfully created, NULL otherwise.
 * Note:    This function is only called when the document is being
 *          created through OLE actions.  The file name is temporarily
 *          set to load the file, then it is reset to "".  This is so
 *          that we won't save back to the template if we exit.
 */
LPSAMPDOC
CreateDocFromFile(
    LPSAMPSRVR lpsrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpstr
    )
{
    // Initialize document
    if (!(glpdoc = InitDoc(lpsrvr, lhdoc, lpstr)) || !(*lpstr))
        return NULL;

    lstrcpy(szUntitled, lpstr);
    SetTitle(TRUE);

    return glpdoc;
}



/********************** Item support functions ************************/
/* CopyObjects() - Copies selection to the clipboard.
 */
BOOL
CopyObjects(
    VOID
    )
{
    HANDLE hdata;

    // If we can't open the clipboard, fail
    if (!OpenClipboard(ghwndFrame))
        return FALSE;

    Hourglass(TRUE);

    // Empty the clipboard
    EmptyClipboard();

    //
    // Copy the clipboard contents.
    //
    // Start with Native Data - which will just contain all the objects
    // which intersect with the selection rectangle.
    //
    if (hdata = GetNative(TRUE))
    {
        SetClipboardData(gcfNative, hdata);
        OleSavedClientDoc(lhClipDoc);
    }

    if (lhClipDoc)
    {
        OleRevokeClientDoc(lhClipDoc);
    lhClipDoc = 0;
    }

    if (hdata = GetLink())
        SetClipboardData(gcfOwnerLink, hdata);

    //
    // Metafile data:  Re-invert the image before putting
    // it onto the clipboard.
    //
    if (hdata = GetMF())
        SetClipboardData(CF_METAFILEPICT, hdata);

    CloseClipboard();
    Hourglass(FALSE);

    return TRUE;
}



/* CreateNewItem() - Allocate a new item.
 *
 * Note:    lpitem->rc will be filled out by the caller.
 */
LPSAMPITEM
CreateNewItem(
    LPSAMPDOC lpdoc
    )
{
    HANDLE hitem = NULL;
    LPSAMPITEM lpitem = NULL;

    // Now create the item
    if (!(hitem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(ITEM)))
        || !(lpitem = (LPSAMPITEM)GlobalLock(hitem)))
        goto errRtn;

    lpitem->hitem = hitem;
    lpitem->oleobject.lpvtbl = &vitemvtbl;
    return lpitem;

errRtn:
    if (lpitem)
        GlobalUnlock(hitem);

    if (hitem)
        GlobalFree(hitem);

    return NULL;
}



/* SendItemChangeMsg() - Notify the client that the item has changed.
 */
static BOOL
SendItemChangeMsg(
    LPSAMPITEM lpitem,
    UINT options
    )
{
    if (lpitem->lpoleclient)
    {
        (*lpitem->lpoleclient->lpvtbl->CallBack)
            (lpitem->lpoleclient, options, (LPOLEOBJECT)lpitem);

        return TRUE;
    }

    return FALSE;
}



/******************** Data reading/writing functions *********************/
/* GetNative(fClip) - Write the item native format to a memory block.
 *
 * This function will just write the objects which intersect
 * with the selection rectangle into a memory block.  If we
 * are running as an embedded instance, return ALL items, even
 * those which are not in the selection area.  Then we will
 * never lose objects that we move out of the selection area
 * when editing an embedded object.
 *
 * Args: fClip - TRUE means native data is for copying to clipboard
 *
 * Returns: A handle containing the native format, or NULL.
 */
HANDLE
GetNative(
    BOOL fClip
    )
{
    BOOL fSuccess = FALSE;
    DWORD cBytes = 0L;
    HANDLE hdata = NULL;
    LPSTR lpdata = NULL;
    DWORD cEmbWrite;
    LPOLEOBJECT lpobjapp = NULL;
    LPOLEOBJECT lpobjcon = NULL;
    LPPICT lpcPict;
    LPPICT lpaPict;
    WORD w;

    // Compute the size of the appearance
    lpaPict = glpobj[APPEARANCE];
    lpcPict = glpobj[CONTENT];

    switch (gpty[APPEARANCE])
    {
        case ICON:
            cBytes += IconWriteToNative(glpobj[APPEARANCE], NULL);
            break;

        case PICTURE:
            if (fClip)
            {
                if (Error(OleRegisterClientDoc(
                    gszAppClassName, szClip, 0L, &lhClipDoc)))
                    goto Error;

                if (Error(OleClone(
                    lpaPict->lpObject, glpclient, lhClipDoc,
                    szAppearance, &lpobjapp)))
                    goto Error;

                cBytes += PicWriteToNative(lpaPict, lpobjapp, NULL);
            }
            else
            {
                cBytes += PicWriteToNative(lpaPict, lpaPict->lpObject, NULL);
            }

            break;

        default:
            break;
    }

    // Compute the content size
    switch (gpty[CONTENT])
    {
        case CMDLINK:
            cBytes += CmlWriteToNative(glpobj[CONTENT], NULL);
            break;

        case PEMBED:    /* EmbWrite returns -1L if the user cancels */
            cEmbWrite = EmbWriteToNative(glpobj[CONTENT], NULL);

            if (cEmbWrite == (DWORD) - 1L)
                return FALSE;

            cBytes += cEmbWrite;
            break;

        case PICTURE:
            if (fClip)
            {
                if (!lhClipDoc && (Error(OleRegisterClientDoc(
                    gszAppClassName, szClip, 0L, &lhClipDoc))))
                    goto Error;

                if (Error(OleClone(lpcPict->lpObject, glpclient,
                    lhClipDoc, szContent, &lpobjcon)))
                    goto Error;

                cBytes += PicWriteToNative(lpcPict, lpobjcon, NULL);
            }
            else
            {
                cBytes += PicWriteToNative(lpcPict, lpcPict->lpObject, NULL);
            }

            break;

        default:
            break;
    }

    if (cBytes == 0L) // then no data
        goto Error;

    cBytes += (DWORD)(2 * sizeof(WORD));

    // Allocate a memory block for the data
    if (!(hdata = GlobalAlloc(GMEM_ZEROINIT, cBytes)) ||
        !(lpdata = (LPSTR)GlobalLock(hdata)))
        goto Error;

    // Write out the appearance
    w = (WORD)gpty[APPEARANCE];
    MemWrite(&lpdata, (LPSTR)&w, sizeof(WORD));
    switch (gpty[APPEARANCE])
    {
        case ICON:
            IconWriteToNative(glpobj[APPEARANCE], &lpdata);
            break;

        case PICTURE:
            if (fClip)
                PicWriteToNative(lpaPict, lpobjapp, &lpdata);
            else
                PicWriteToNative(lpaPict, lpaPict->lpObject, &lpdata);

            break;

        default:
            break;
    }

    // Write out the content
    w = (WORD)gpty[CONTENT];
    MemWrite(&lpdata, (LPSTR)&w, sizeof(WORD));

    switch (gpty[CONTENT])
    {
        case CMDLINK:
            CmlWriteToNative(glpobj[CONTENT], &lpdata);
            break;

        case PEMBED:
            EmbWriteToNative(glpobj[CONTENT], &lpdata);
            break;

        case PICTURE:
            if (fClip)
                PicWriteToNative(lpcPict, lpobjcon, &lpdata);
            else
                PicWriteToNative(lpcPict, lpcPict->lpObject, &lpdata);

            break;

        default:
            break;
    }

    fSuccess = TRUE;

Error:
    if (lpobjcon)
        OleRelease (lpobjcon);

    if (lpobjapp)
        OleRelease (lpobjapp);

    if (lpdata)
        GlobalUnlock(hdata);

    if (!fSuccess && hdata)
    {
        GlobalFree(hdata);
        hdata = NULL;
    }

    return hdata;
}



/* PutNative() - Read the item native data from a selector.
 *
 * Reads as many objects as it can, in upwards order (better error recovery).
 * Note:  It may be worthwhile to scale the object(s) to the window here.
 *
 * Returns: TRUE iff successful.
 */
BOOL
PutNative(
    HANDLE hdata
    )
{
    BOOL fSuccess = FALSE;
    LPSTR lpdata;
    WORD w;

    if (!(lpdata = (LPSTR)GlobalLock(hdata)))
        goto Error;

    // Delete any previous panes
    DeletePane(APPEARANCE, TRUE);
    DeletePane(CONTENT, TRUE);

    // Read in the appearance
    MemRead(&lpdata, (LPSTR)&w, sizeof(WORD));
    gpty[APPEARANCE] = w;
    switch (gpty[APPEARANCE])
    {
        case ICON:
            if (!(glpobj[APPEARANCE] = IconReadFromNative(&lpdata)))
                gpty[APPEARANCE] = NOTHING;

            break;

        case PICTURE:
            if (glpobj[APPEARANCE] =
                PicReadFromNative(&lpdata, gszCaption[APPEARANCE]))
            {
                SendMessage(ghwndPane[APPEARANCE], WM_FIXSCROLL, 0, 0L);
                break;
            }

        default:
            gpty[APPEARANCE] = NOTHING;
            break;
    }

    // Read the content
    MemRead(&lpdata, (LPSTR)&w, sizeof(WORD));
    gpty[CONTENT] = w;
    switch (gpty[CONTENT])
    {
        case CMDLINK:
            if (!(glpobj[CONTENT] = CmlReadFromNative(&lpdata)))
                gpty[CONTENT] = NOTHING;

            break;

        case PEMBED:
            if (!(glpobj[CONTENT] = (LPVOID)EmbReadFromNative(&lpdata)))
                gpty[CONTENT] = NOTHING;

            break;

        case PICTURE:
            if (glpobj[CONTENT] =
                (LPVOID)PicReadFromNative(&lpdata, gszCaption[CONTENT]))
            {
                SendMessage(ghwndPane[CONTENT], WM_FIXSCROLL, 0, 0L);
                EnableWindow(ghwndPict, TRUE);
                break;
            }

        default:
            gpty[CONTENT] = NOTHING;
            break;
    }

    fSuccess = TRUE;
    InvalidateRect(ghwndFrame, NULL, TRUE);

Error:
    if (lpdata)
        GlobalUnlock(hdata);

    return fSuccess;
}



/* GetLink() - Retrieves ObjectLink/OwnerLink information.
 *
 * This function returns a string describing the selected area.
 */
HANDLE
GetLink(
    VOID
    )
{
    CHAR pchlink[CBLINKMAX];
    INT cblink;
    HANDLE hlink;
    LPSTR lplink;

    // Link data - <App name>\0<Doc name>\0<Item name>\0\0
    lstrcpy((LPSTR)pchlink, gszAppClassName);
    cblink = lstrlen((LPSTR)pchlink) + 1;

    // Copy the file name
    lstrcpy((LPSTR)(pchlink + cblink), szDummy);
    cblink += lstrlen((LPSTR)(pchlink + cblink)) + 1;

    // Copy the item name
    lstrcpy((LPSTR)(pchlink + cblink), szDummy);
    cblink += lstrlen((LPSTR)(pchlink + cblink)) + 1;
    pchlink[cblink++] = 0;       /* throw in another NULL at the end */

    // Allocate a memory block for the data
    if (!(hlink = GlobalAlloc(GMEM_ZEROINIT, cblink)) ||
        !(lplink = (LPSTR)GlobalLock(hlink)))
        goto Error;

    // Copy the data, then return the memory block
    MemWrite(&lplink, (LPSTR)pchlink, cblink);
    GlobalUnlock(hlink);
    return hlink;

Error:
    if (hlink)
        GlobalFree(hlink);

    return NULL;
}



/* GetMF() - Retrieve a metafile of the selected area.
 *
 * Note:    Originally, tried to Blt directly from the Window DC.  This
 *          doesn't work very well because when the window is obscured,
 *          the obscured portion shows up when the link is updated.
 */
HANDLE
GetMF(
    VOID
    )
{
    BOOL fError = TRUE;
    HANDLE hdata = NULL;
    HDC hdcMF = NULL;
    HDC hdcWnd = NULL;
    HFONT hfont;
    HANDLE hmfpict;
    LPMETAFILEPICT lpmfpict;
    LPIC lpic;
    LPPICT lppict;
    RECT rcTemp;
    RECT rcText;
    INT cxImage;
    INT cyImage;

    hmfpict = GlobalAlloc(GMEM_ZEROINIT, sizeof(METAFILEPICT));
    if (!hmfpict)
        goto Error;

    lpmfpict = (LPMETAFILEPICT)GlobalLock(hmfpict);

    // If the picture has a metafile, use it!
    if (gpty[APPEARANCE] == PICTURE)
    {
        LPMETAFILEPICT  lpmfpictOrg = NULL;

        if (Error(OleGetData(
            ((LPPICT)glpobj[APPEARANCE])->lpObject, CF_METAFILEPICT, &hdata))
            || !hdata
            || !(lpmfpictOrg = (LPMETAFILEPICT)GlobalLock(hdata)))
            goto NoPicture;

        // Copy the metafile
        lpmfpict->hMF = CopyMetaFile(lpmfpictOrg->hMF, NULL);
        GlobalUnlock(hdata);

        // If we failed, just draw it
        if (!lpmfpict->hMF)
            goto NoPicture;

        // Finish filling in the metafile header
        lpmfpict->mm   = lpmfpictOrg->mm;
        lpmfpict->xExt = lpmfpictOrg->xExt;
        lpmfpict->yExt = lpmfpictOrg->yExt;

        GlobalUnlock(hmfpict);
        return hmfpict;
    }

NoPicture:
    // Get the window DC, and make a DC compatible to it.
    if (!(hdcWnd = GetDC(NULL)))
        goto Error;

    switch (gpty[APPEARANCE])
    {
        case ICON:
            lpic = (LPIC)glpobj[APPEARANCE];

            // Set the icon text rectangle, and the icon font
            SetRect(&rcText, 0, 0, gcxArrange, gcyArrange);
            hfont = SelectObject(hdcWnd, ghfontTitle);

            // Figure out how large the text region will be
            // since this is going in a metafile we will not wrap
            // the icon text

            DrawText(hdcWnd, lpic->szIconText, -1, &rcText,
                DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

            if (hfont)
                SelectObject(hdcWnd, hfont);

            // Compute the image size
            rcText.right++;
            cxImage = (rcText.right > gcxIcon) ? rcText.right : gcxIcon;
            cyImage = gcyIcon + rcText.bottom + 1;
            break;

        case PICTURE:
            lppict  = (LPPICT)glpobj[APPEARANCE];
            cxImage = lppict->rc.right - lppict->rc.left + 1;
            cyImage = lppict->rc.bottom - lppict->rc.top + 1;
            break;

        default:
            cxImage = GetSystemMetrics(SM_CXICON);
            cyImage = GetSystemMetrics(SM_CYICON);
            break;
    }

    cxImage += cxImage / 4; // grow the image a bit

    cyImage += cyImage / 8;

    // Create the metafile
    if (!(hdcMF = CreateMetaFile(NULL)))
        goto Error;

    // Initialize the metafile
    SetWindowOrgEx(hdcMF, 0, 0, NULL);
    SetWindowExtEx(hdcMF, cxImage - 1, cyImage - 1, NULL);

    //
    // Fill in the background
    //
    // We displace back to (0, 0) because that's where the BITMAP resides.
    //
    SetRect(&rcTemp, 0, 0, cxImage, cyImage);
    switch (gpty[APPEARANCE])
    {
        case ICON:
            IconDraw(glpobj[APPEARANCE], hdcMF, &rcTemp, FALSE, cxImage, cyImage);
            break;

        case PICTURE:
            PicDraw(glpobj[APPEARANCE], hdcMF, &rcTemp, 0, 0, TRUE, FALSE);
            break;

        default:
            DrawIcon(hdcMF, 0, 0, LoadIcon(ghInst, MAKEINTRESOURCE(ID_APPLICATION)));
            break;
    }

    // Map to device independent coordinates
    rcTemp.right =
        MulDiv((rcTemp.right - rcTemp.left), HIMETRIC_PER_INCH, giXppli);
    rcTemp.bottom =
        MulDiv((rcTemp.bottom - rcTemp.top), HIMETRIC_PER_INCH, giYppli);

    // Finish filling in the metafile header
    lpmfpict->mm = MM_ANISOTROPIC;
    lpmfpict->xExt = rcTemp.right;
    lpmfpict->yExt = rcTemp.bottom;
    lpmfpict->hMF = CloseMetaFile(hdcMF);

    fError = FALSE;

Error:
    if (hdcWnd)
        ReleaseDC(NULL, hdcWnd);

    // If we had an error, return NULL
    if (fError && hmfpict)
    {
        GlobalUnlock(hmfpict);
        GlobalFree(hmfpict);
        hmfpict = NULL;
    }

    return hmfpict;
}



/* InitEmbedded() - Perform operations specific to editing embedded objects.
 *
 * This routine changes the menu items as appropriate.
 */
VOID
InitEmbedded(
    BOOL fCreate
    )
{
    HMENU hmenu;

    if (hmenu = GetMenu(ghwndFrame))
        EnableMenuItem(hmenu, IDM_UPDATE, fCreate ? MF_GRAYED : MF_ENABLED);

    gfEmbedded = TRUE;
}



/***************** Item circular queue/utility functions *****************/
/* AddItem() - Add an item to the global item list.
 */
LPSAMPITEM
AddItem(
    LPSAMPITEM lpitem
    )
{
    INT i;
    HANDLE hitem;

    i = FindItem((LPSAMPITEM)lpitem);
    if (i < cItems)
    {
        vlpitem[i]->ref++;

        // Free the duplicate item
        GlobalUnlock(hitem = lpitem->hitem);
        GlobalFree(hitem);
    }
    else
    {
        if (i < CITEMSMAX)
        {
            vlpitem[cItems] = (LPSAMPITEM)lpitem;
            vlpitem[cItems++]->ref = 1;
        }
        else
        {
            return NULL;
        }
    }

    return vlpitem[i];
}



/* DeleteItem() - Delete an item from the global item list.
 *
 * Returns: TRUE iff successful.
 */
BOOL
DeleteItem(
    LPSAMPITEM lpitem
    )
{
    BOOL fFound;
    HANDLE hitem;
    INT i;

    i = FindItem(lpitem);

    if ((fFound = (i < cItems && vlpitem[i]->ref))
        && !(--vlpitem[i]->ref))
    {
        // Free the item
        GlobalUnlock(hitem = vlpitem[i]->hitem);
        GlobalFree(hitem);

        // Shift everything else down
        cItems--;
        for ( ; i < cItems; i++)
            vlpitem[i] = vlpitem[i + 1];
    }

    return fFound;
}



/* FindItem() - Locate an item in the global item list.
 */
static INT
FindItem(
    LPSAMPITEM lpitem
    )
{
    BOOL fFound = FALSE;
    INT i;

    for (i = 0; i < cItems && !fFound;)
    {
        if (lpitem->aName == vlpitem[i]->aName)
        {
            fFound = TRUE;
        }
        else
        {
            i++;
        }
    }

    return i;
}



/* EndEmbedding() - Return to normal editing.
 *
 * This routine changes the menu items as appropriate.
 */
VOID
EndEmbedding(
    VOID
    )
{
    HMENU hmenu;

    // Fix the "Untitled" string
    LoadString(ghInst, IDS_UNTITLED, szUntitled, CBMESSAGEMAX);

    if (hmenu = GetMenu(ghwndFrame))
        EnableMenuItem(hmenu, IDM_UPDATE, MF_GRAYED);

    gfEmbedded = FALSE;
}
