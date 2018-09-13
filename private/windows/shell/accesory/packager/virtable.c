/* virtable.c - This module contains the OLE virtual table/private routines.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"
#include "dialogs.h"


//#define  OLESVR_SUPPORT     /* enable support for OLE server files */


static CHAR szLink[] = "/Link";             // Appended to end of link packages



/**************************** Server functions *****************************/
/* SrvrOpen() - Wraps a filename that is passed into a command line.
 */
OLESTATUS
SrvrOpen(
    LPOLESERVER lpolesrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpdocname,
    LPOLESERVERDOC *lplpoledoc
    )
{
    LPSAMPDOC lpdoc;
    LPSTR lpstrLink = NULL;
    OLESTATUS retval = OLE_OK;
    LPOLEOBJECT lpObject = NULL;

    DPRINT("pkg: SrvrOpen");

    if (lpstrLink = Contains(lpdocname, szLink))
        *lpstrLink = '\0';

    if (!(lpdoc = (LPSAMPDOC)CreateDocFromFile(
        (LPSAMPSRVR)lpolesrvr, lhdoc, lpdocname)))
        return OLE_ERROR_GENERIC;

    // Generate a command line
    BringWindowToTop(ghwndPane[CONTENT]);

    if (gpty[CONTENT])
        DeletePane(CONTENT, TRUE);

#ifdef OLESVR_SUPPORT
    if (IsOleServerDoc (lpdocname))
    {
        gpty[CONTENT] = PICTURE;

        if (lpstrLink)
        {
            if (Error(OleCreateLinkFromFile(gszProtocol, glpclient, NULL,
                lpdocname, NULL, glhcdoc, gszCaption[CONTENT], &lpObject,
                olerender_draw, 0)))
                retval = OLE_ERROR_OPEN;
        }
        else
        {
            if (Error(OleCreateFromFile(gszProtocol, glpclient, NULL, lpdocname,
                glhcdoc, gszCaption[CONTENT], &lpObject, olerender_draw, 0)))
                retval = OLE_ERROR_OPEN;
        }

        if (retval == OLE_OK)
        {
            glpobj[CONTENT] = PicCreate(lpObject, NULL);
            ((LPPICT)glpobj[CONTENT])->fNotReady = TRUE;
            OleBlockServer(((LPSAMPSRVR)lpolesrvr)->lhsrvr);
            gfBlocked = TRUE;
        }
        else
        {
            DeregisterDoc();
            return retval;
        }
    }
    else
    {
#endif
        if (lpstrLink)
        {
            if (glpobj[CONTENT] = CmlCreateFromFilename(lpdocname, TRUE))
                gpty[CONTENT] = CMDLINK;
        }
        else
        {
            if (glpobj[CONTENT] = (LPVOID)EmbCreate(lpdocname))
                gpty[CONTENT] = PEMBED;
        }

        if (glpobj[CONTENT] == NULL)
            retval = OLE_ERROR_OPEN;

#ifdef OLESVR_SUPPORT
    }
#endif

    // If no appearance pane (which should be always), try to make one
    if (!gpty[APPEARANCE])
    {
        if (glpobj[APPEARANCE] = IconCreateFromFile(lpdocname))
        {
            gpty[APPEARANCE] = ICON;
            InvalidateRect(ghwndPane[APPEARANCE], NULL, TRUE);
        }
    }

    // Restore the character we so rudely mashed
    if (lpstrLink)
        *lpstrLink = szLink[0];

    // Save the document and change the menus
    InitEmbedded(FALSE);
    *lplpoledoc = (LPOLESERVERDOC)lpdoc;

    return retval;
}



/* SrvrCreate() - Create a new (embedded) object.
 */
OLESTATUS
SrvrCreate(
    LPOLESERVER lpolesrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpclassname,
    LPSTR lpdocname,
    LPOLESERVERDOC *lplpoledoc
    )
{

    DPRINT("pkg: SrvrCreate");

    // Initialize the new image
    InitFile();

    if (!(*lplpoledoc = (LPOLESERVERDOC)CreateNewDoc((LPSAMPSRVR)lpolesrvr,
        lhdoc, lpdocname)))
        return OLE_ERROR_GENERIC;

    InitEmbedded(TRUE);

    return OLE_OK;
}



/* SrvrCreateFromTemplate() - Create a new (embedded) object from a file.
 */
OLESTATUS
SrvrCreateFromTemplate(
    LPOLESERVER lpolesrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpclassname,
    LPSTR lpdocname,
    LPSTR lptemplatename,
    LPOLESERVERDOC *lplpoledoc
    )
{
    LPSAMPDOC lpdoc;

    DPRINT("pkg: SrvrCreateFromTemplate");

    if (!(lpdoc = (LPSAMPDOC)CreateDocFromFile((LPSAMPSRVR)lpolesrvr, lhdoc,
        lptemplatename)))
        return OLE_ERROR_GENERIC;

    // Save the document and change the menus
    *lplpoledoc = (LPOLESERVERDOC)lpdoc;
    InitEmbedded(FALSE);
    lstrcpy(szUntitled, lpdocname);
    SetTitle(TRUE);

    return OLE_OK;
}



/* SrvrEdit() - Open an (embedded) object for editing.
 */
OLESTATUS
SrvrEdit(
    LPOLESERVER lpolesrvr,
    LHSERVERDOC lhdoc,
    LPSTR lpclassname,
    LPSTR lpdocname,
    LPOLESERVERDOC *lplpoledoc
    )
{
    DPRINT("pkg: SrvrEdit");

    if (!(*lplpoledoc = (LPOLESERVERDOC)CreateNewDoc((LPSAMPSRVR)lpolesrvr,
        lhdoc, lpdocname)))
        return OLE_ERROR_MEMORY;

    InitEmbedded(FALSE);

    return OLE_OK;
}



/* SrvrExit() - Called to cause the OLE server to be revoked.
 */
OLESTATUS
SrvrExit(
    LPOLESERVER lpolesrvr
    )
{
    DPRINT("pkg: SrvrExit");
    DeleteServer((LPSAMPSRVR)lpolesrvr);
    return OLE_OK;

}



/* SrvrRelease() - Called so that the server memory can be freed.
 *
 * Note:    This call may occur in isolation without a SrvrExit()
 *          call.  If this occurs, we still revoke the server.
 */
OLESTATUS
SrvrRelease(
    LPOLESERVER lpolesrvr
    )
{
    DPRINT("pkg: SrvrRelease");
    if (gvlptempdoc)
        return OLE_OK;

    if (gfInvisible || (gfEmbeddedFlag && !gfDocExists))
        DeleteServer((LPSAMPSRVR)lpolesrvr);

    if (ghServer)
        DestroyServer();

    return OLE_OK;
}



/* SrvrExecute() - Called to execute DDE commands
 */
OLESTATUS
SrvrExecute(
    LPOLESERVER lpolesrvr,
    HANDLE hCmds
    )
{
    DPRINT("pkg: SrvrExecute");
    return OLE_ERROR_PROTOCOL;
}



/************************** Document functions *************************/
/* DocSave() - OLE callback to save the document.
 */
OLESTATUS
DocSave(
    LPOLESERVERDOC lpoledoc
    )
{
    DPRINT("pkg: DocSave");
    return OLE_OK;
}



/* DocClose() - OLE callback when the document is to be closed.
 *
 * This command has no additional effects; since we are not an MDI application
 * we don't close the child window.  The window is destroyed when the server
 * function "Release" is called.
 */
OLESTATUS
DocClose(
    LPOLESERVERDOC lpoledoc
    )
{
    DPRINT("pkg: DocClose");
    DeregisterDoc();
    return OLE_OK;
}



/* DocRelease() - Deallocate document memory.
 */
OLESTATUS
DocRelease(
    LPOLESERVERDOC lpoledoc
    )
{
    LPSAMPDOC lpdoc = (LPSAMPDOC)lpoledoc;
    HANDLE hdoc;

    DPRINT("pkg: DocRelase");
    if (lpdoc)
    {
        if (!gfDocCleared)
        {
            glpdoc = NULL;
            DeregisterDoc();
        }

        GlobalDeleteAtom(lpdoc->aName);
        LocalUnlock(hdoc = lpdoc->hdoc);
        LocalFree(hdoc);
        gfDocExists = FALSE;
    }

    return OLE_OK;
}



/* DocGetObject() - Create a new object within the current document
 */
OLESTATUS
DocGetObject(
    LPOLESERVERDOC lpoledoc,
    LPSTR lpitemname,
    LPOLEOBJECT *lplpoleobject,
    LPOLECLIENT lpoleclient
    )
{
    LPSAMPITEM lpitem;

    DPRINT("pkg: DocGetObject");

    //
    // Always create a new item in this case, it's much easier than
    // worrying about the sub-rectangle bitmap.
    //
    lpitem = CreateNewItem((LPSAMPDOC)lpoledoc);
    lpitem->lpoleclient = lpoleclient;
    if (*lpitemname)
    {
        lpitem->aName = AddAtom(lpitemname);
    }
    else
    {
        lpitem->aName = 0;
    }

    if (!(*lplpoleobject = (LPOLEOBJECT)AddItem(lpitem)))
        return OLE_ERROR_GENERIC;

    return OLE_OK;
}



/* DocSetHostNames() - Sets the title bar to the correct document name.
 *
 * Note:    The format is "<lpclientName> <app name> - <lpdocName>".
 */
OLESTATUS
DocSetHostNames(
    LPOLESERVERDOC lpoledoc,
    LPSTR lpclientName,
    LPSTR lpdocName
    )
{
    DPRINT("pkg: DocSetHostnames");
    lstrcpy(szUntitled, lpdocName);
    lstrcpy(gszClientName, lpclientName);
    SetTitle(TRUE);

    return OLE_OK;
}



/* DocSetDocDimensions() - OLE callback to change the document dimensions.
 *
 * Note:    This command is unsupported.  It is the client application's
 *          responsibility to report errors (as needed).
 */
OLESTATUS
DocSetDocDimensions(
    LPOLESERVERDOC lpoledoc,
    LPRECT lprc
    )
{
    DPRINT("pkg: DocSetDocDimensions");
    return OLE_ERROR_GENERIC;
}



/* DocSetColorScheme() - OLE callback to change the document colors.
 *
 * Note:    This command is unsupported.  It is the client application's
 *          responsibility to report errors (as needed).
 */
OLESTATUS
DocSetColorScheme(
    LPOLESERVERDOC lpoledoc,
    LPLOGPALETTE lppal
    )
{
    DPRINT("pkg: DocSetColorScheme");
    return OLE_ERROR_GENERIC;
}



/* DocExecute() - Called to execute DDE commands
 */
OLESTATUS
DocExecute(
    LPOLESERVERDOC lpoledoc,
    HANDLE hCmds
    )
{
    DPRINT("pkg: DocExecute");
    return OLE_ERROR_PROTOCOL;
}



/**************************** Item functions ***************************/
/* ItemDelete() - Free memory associated with the current item.
 */
OLESTATUS
ItemDelete(
    LPOLEOBJECT lpoleobject
    )
{
    DPRINT("pkg: ItemDelete");
    DeleteItem((LPSAMPITEM)lpoleobject);

    return OLE_OK;              /* Add error checking later */
}



/* ItemGetData() - Used by the client to obtain the item data.
 */
OLESTATUS
ItemGetData(
    LPOLEOBJECT lpoleobject,
    OLECLIPFORMAT cfFormat,
    LPHANDLE lphandle
    )
{

    DPRINT("pkg: ItemGetData");
    if ((gpty[CONTENT] == PICTURE) && ((LPPICT)glpobj[CONTENT])->fNotReady)
        return OLE_BUSY;

    if (cfFormat == gcfNative)
    {
        if (*lphandle = GetNative(FALSE))
            return OLE_OK;

    }
    else if (cfFormat == CF_METAFILEPICT)
    {
        if (*lphandle = GetMF())
            return OLE_OK;

    }
    else if (cfFormat == gcfOwnerLink)
    {
        if (*lphandle = GetLink())
            return OLE_OK;
    }

    // Clipboard format not supported
    return OLE_ERROR_GENERIC;
}



/* ItemSetData() - Used by the client to paste data into a server.
 *
 * Read in the embedded object data in Native format.  This will
 * not be called unless we are editing the correct document.
 */
OLESTATUS
ItemSetData(
    LPOLEOBJECT lpoleobject,
    OLECLIPFORMAT cfFormat,
    HANDLE hdata
    )
{
    LPSAMPITEM lpitem = (LPSAMPITEM)lpoleobject;

    DPRINT("pkg: ItemSetData");
    if (cfFormat == gcfNative && !PutNative(hdata))
    {
        SendMessage(ghwndFrame, WM_COMMAND, IDM_NEW, 0L);
        GlobalFree(hdata);

        return OLE_ERROR_GENERIC;
    }

    GlobalFree(hdata);

    return OLE_OK;
}



/* ItemDoVerb() - Play/Edit the object.
 *
 * This routine is called when the user tries to run an object that
 * is wrapped by the packager.
 */
OLESTATUS
ItemDoVerb(
    LPOLEOBJECT lpoleobject,
    UINT wVerb,
    BOOL fShow,
    BOOL fActivate
    )
{
    DPRINT("pkg: ItemDoVerb");
    switch (wVerb)
    {
        case OLE_PLAY:
            if (fShow)
                return (*(lpoleobject->lpvtbl->Show))(lpoleobject, fActivate);
            break;

        case OLE_EDIT:
            if (fShow && fActivate)
            {
                if (gfInvisible)
                {
                    ShowWindow(ghwndFrame, gnCmdShowSave);
                    gfInvisible = FALSE;
                }

                // If iconic, restore the window; then give it the focus.
                if (IsIconic(ghwndFrame))
                    SendMessage(ghwndFrame, WM_SYSCOMMAND, SC_RESTORE, 0L);

                BringWindowToTop(ghwndFrame);
            }

        default:
            break;
    }

    return OLE_OK;
}



/* ItemShow() - Show the item.
 *
 * This routine is called when the user tries to edit an object in a
 * client application, and the server is already active.
 */
OLESTATUS
ItemShow(
    LPOLEOBJECT lpoleobject,
    BOOL fActivate
    )
{
    HWND hwndItem;

    DPRINT("pkg: ItemShow");
    if (fActivate
        && (hwndItem = GetTopWindow(ghwndFrame))
        && (gpty[(hwndItem == ghwndPane[CONTENT])] == NOTHING))
    {
        //
        //  Lets assume that in this case the client has
        //  attempted an InsertObject operation with
        //  the Package class. (5.30.91) v-dougk
        //
        if (gfInvisible)
        {
            ShowWindow(ghwndFrame, SW_SHOW);
            gfInvisible = FALSE;
        }

        BringWindowToTop(ghwndFrame);
    }
    else
    {
        PostMessage(hwndItem, WM_COMMAND, IDD_PLAY, 0L);
    }

    return OLE_OK;
}



/* ItemSetBounds() - Set the item's size.
 *
 * Note:    This command is not supported.
 */
OLESTATUS
ItemSetBounds(
    LPOLEOBJECT lpoleobject,
    LPRECT lprc
    )
{
    DPRINT("pkg: ItemSetBounds");
    return OLE_ERROR_GENERIC;
}



/* ItemSetTargetDevice() - Changes the target device for item display.
 *
 * Note:    This command is not supported.
 */
OLESTATUS
ItemSetTargetDevice(
    LPOLEOBJECT lpoleobject,
    HANDLE h
    )
{
    DPRINT("pkg: ItemSetTargetDevice");
    if (h)
        GlobalFree(h);

    return OLE_ERROR_GENERIC;
}



/* ItemEnumFormats() - Enumerate formats which are renderable.
 *
 * This is called by the OLE libraries to get a format for screen display.
 * Currently, only Metafile and Native are supported.
 */
OLECLIPFORMAT
ItemEnumFormats(
    LPOLEOBJECT lpobject,
     OLECLIPFORMAT cfFormat
     )
{
    DPRINT("pkg: ItemEnumFormats");
    if (!cfFormat)
        return CF_METAFILEPICT;

    if (cfFormat == CF_METAFILEPICT)
        return gcfNative;

    return 0;
}



/* ItemQueryProtocol() - Tells whether the given protocol is supported.
 *
 * Returns:  lpoleobject iff the protocol is "StdFileEditing".
 */
LPVOID
ItemQueryProtocol(
    LPOLEOBJECT lpoleobject,
    LPSTR lpprotocol
    )
{
    DPRINT("pkg: ItemQueryProtocol");
    return (!lstrcmpi(lpprotocol, "StdFileEditing") ? lpoleobject : NULL);
}



/* ItemSetColorScheme() - Denotes the palette to be used for item display.
 *
 * Note:    This command is not supported.
 */
OLESTATUS
ItemSetColorScheme(
    LPOLEOBJECT lpoleobject,
    LPLOGPALETTE lppal
    )
{
    DPRINT("pkg: ItemSetColorScheme");
    return OLE_ERROR_GENERIC;
}



BOOL
IsOleServerDoc(
    LPSTR lpdocname
    )
{
    CHAR key[256];
    LONG cb = 256;

    if (!lpdocname)
        return FALSE;

    while (*lpdocname && *lpdocname != '.')
        lpdocname++;

    if (*(lpdocname + 1) == 0)
        return FALSE;

    if (RegQueryValue (HKEY_CLASSES_ROOT, lpdocname, key, &cb))
        return FALSE;

    //
    // Now we got the class, check whether it is a ole server class
    // which wants objects to packaged rather than file
    //
    lstrcat (key, "\\protocol");
    lstrcat (key, "\\StdFileEditing");
    lstrcat (key, "\\server");

    cb = 256;
    if (RegQueryValue (HKEY_CLASSES_ROOT, key, key, &cb))
        return FALSE;

    return TRUE;
}
