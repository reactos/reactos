/*
 * PBSERVER.C - Code to make PaintBrush an OLE server application
 *
 * Adapted from RaoR's "Shapes" sample server
 */
#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "pbserver.h"

#include <shellapi.h>

#ifdef WIN32
#define huge
#endif

int     iExitWithSaving = IDNO;
/* Clipboard formats */
WORD    vcfLink = 0;
WORD    vcfOwnerLink = 0;
WORD    vcfNative = 0;

/* After the user chooses not to update an object while closing the server
 * this flag prevents data being sent before the server closes down */
BOOL    fSendData = TRUE;       /* return NULL data from ItemGetData()?  */

/* OLE document/object/server virtual tables */
OLESERVERDOCVTBL      vdocvtbl;
OLEOBJECTVTBL   vitemvtbl;
OLESERVERVTBL     vsrvrvtbl;

/* Are we running a Link or the non-OLE case? */
BOOL    vfIsLink = TRUE;

/* Since Paint is not an MDI application, only one document can be
 * edited per instance.  The items are just RECTs over (windows onto)
 * the document, which may overlap.
 */
HANDLE  hServer = NULL;
PPBSRVR vpsrvr = NULL;
PPBDOC  vpdoc = NULL;
PITEM   vpitem[CMAXITEMS];
int     cItems = 0;
int     nCmdShowSaved = 0;

/* What part of the image has been modified? */
RECT    vrcModified;

/* Swiped from SaveBitmapFile() */
#define DIBID           0x4D42
#define CBMENUITEMMAX   80


extern HBITMAP fileBitmap;
extern int imageWid, imageHgt, imagePlanes, imagePixels;
extern HWND pbrushWnd[];
extern TCHAR pathName[];
extern TCHAR fileName[];
extern TCHAR noFile[];
extern WORD wFileType;
extern BOOL imageFlag;
extern TCHAR *namePtr;
extern HDC fileDC;
extern TCHAR filePath[];
extern HPALETTE hPalette;
extern BOOL bZoomedOut;
extern HWND zoomOutWnd;

/* Function prototypes */
void PUBLIC NewImage(int);               /* MenuCmd.C */
static void FixMenus(void);                    /* Update, Save Copy As..., Exit and Return */
static void GetNum(LPTSTR FAR *lplpstrBuf, LPINT lpint);
static void MakeObjectVisible(
    LPRECT lpItemRect);
int PelsPerLogMeter(HDC hDC,BOOL bHoriz);
int PelsToLogHimetric(HDC hDC,BOOL bHoriz, int pels);

/* InitVTbls() is in srvrinit.c */

/* MACROS: */
/* Fix the menus for the embedded instance */
#define ChangeMenuItem(id, idnew, sz) \
    ModifyMenu(hMenu, id, MF_BYCOMMAND | MF_STRING, idnew, sz)

#ifndef OLE_20
#   define FreeOleString(s)     \
        if((s) == NULL) { /* do nothing */ ; } else LocalFree(s)

BOOL SetOleString(LPTSTR *ppstr, POLESTR postr)  {
    int cb;

    DB_OUT("In SetOleString ");

    *ppstr = NULL;

    DB_OUT2( postr == NULL, "SOS(NULL) ", "SOS(OK) ");
    if (postr == NULL)
        return TRUE;

    DB_OUT( "SOS(call MBTWC 1) " );
    cb = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, postr, -1, NULL, 0);

    DB_OUT( "SOS(call LA) " );
    *ppstr = LocalAlloc(LPTR, cb * sizeof(WCHAR));

    if (*ppstr != NULL) {
        DB_OUT( "SOS(call MBTWC 2) " );
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, postr, -1, *ppstr, cb);
    }

    DB_OUT( "Out SetOleString\n" );
    return *ppstr != NULL;
}
#else
#   define  FreeOleString(s)
#   define  SetOleString(s,o)   (*s) = o
#endif

void FreeVTbls(void) {

    /* Server virtual table */
    FreeProcInstance((FARPROC)vsrvrvtbl.Open);
    FreeProcInstance((FARPROC)vsrvrvtbl.Create);
    FreeProcInstance((FARPROC)vsrvrvtbl.CreateFromTemplate);
    FreeProcInstance((FARPROC)vsrvrvtbl.Edit);
    FreeProcInstance((FARPROC)vsrvrvtbl.Exit);
    FreeProcInstance((FARPROC)vsrvrvtbl.Release);

    /* Document virtual table */
    FreeProcInstance((FARPROC)vdocvtbl.Save);
    FreeProcInstance((FARPROC)vdocvtbl.Close);
    FreeProcInstance((FARPROC)vdocvtbl.GetObject);
    FreeProcInstance((FARPROC)vdocvtbl.Release);
    FreeProcInstance((FARPROC)vdocvtbl.SetHostNames);
    FreeProcInstance((FARPROC)vdocvtbl.SetDocDimensions);
    FreeProcInstance((FARPROC)vdocvtbl.SetColorScheme);

    /* Item virtual table */
    FreeProcInstance((FARPROC)vitemvtbl.QueryProtocol);
    FreeProcInstance((FARPROC)vitemvtbl.GetData);
    FreeProcInstance((FARPROC)vitemvtbl.SetData);
    FreeProcInstance((FARPROC)vitemvtbl.Release);
    FreeProcInstance((FARPROC)vitemvtbl.Show);
    FreeProcInstance((FARPROC)vitemvtbl.SetBounds);
    FreeProcInstance((FARPROC)vitemvtbl.SetTargetDevice);
    FreeProcInstance((FARPROC)vitemvtbl.EnumFormats);
    FreeProcInstance((FARPROC)vitemvtbl.SetColorScheme);
    FreeProcInstance((FARPROC)vitemvtbl.DoVerb);
}

/************************ App Server Functions ************************/
/* InitServer() is in srvrinit.c */

/* If fServerRevoked is TRUE then Server has been revoked,so
 * we can free server structures and close Pbrush. */
BOOL fServerRevoked = FALSE;

void DeleteServer(PPBSRVR psrvr) {
    OLESTATUS olestat;

    if (!fServerRevoked) {    /* if server is not revoked yet */
        fServerRevoked = TRUE;
        olestat = OleRevokeServer(psrvr->lhsrvr);
        if (!(olestat == OLE_OK || olestat == OLE_WAIT_FOR_RELEASE)) {
#ifdef DEBUG
            PbrushOkError(W_FAILED_TO_REVOKE_SERVER, MB_ICONHAND);
#endif
            fServerRevoked = FALSE;
        }
    }
}

/************************* SERVER VTBL FUNCTIONS **************************/
/* Open a server document.
 * Load the given file and register a server DOC.
 */
OLESTATUS FAR PASCAL SrvrOpen(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
                        POLESTR lpOLEdocname, LPOLESERVERDOC FAR *lplpoledoc) {
    LPTSTR lpDocName;

    DB_OUT("In SrvrOpen ");

    if (!SetOleString(&lpDocName, lpOLEdocname)) {
        DB_OUT("ERR SrvrOpen\n");
        return OLE_ERROR_GENERIC;
    }

    if (!(*lplpoledoc =
            (LPOLESERVERDOC)CreateDocFromFile((PPBSRVR)lpolesrvr,
            lpDocName, lhdoc, lpDocName))) {
        FreeOleString(lpDocName);
        DB_OUT("ERR SrvrOpen\n");
        return OLE_ERROR_BLANK;
    }

    SetTitle(*fileName ? fileName : noFile);
    FreeOleString(lpDocName);
    DB_OUT("Out SrvrOpen\n");
    return OLE_OK;
}

/* Create a new server doc. */
OLESTATUS FAR PASCAL SrvrCreate(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
                POLESTR lpclassname, POLESTR lpdocname, LPOLESERVERDOC FAR *lplpoledoc) {
    LPTSTR lpDocName;
    DB_OUT("In SrvrCreate ");

    if (!SetOleString(&lpDocName, lpdocname)) {
        DB_OUT("ERR SrvrCreate\n");
        return OLE_ERROR_GENERIC;
    }

    /* Initialize the new image */
    SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEnew, 0L);

    if (!(*lplpoledoc =
           (LPOLESERVERDOC)CreateNewDoc((PPBSRVR)lpolesrvr, lhdoc, lpDocName))){
        FreeOleString(lpDocName);
        DB_OUT("ERR SrvrCreate\n");
        return OLE_ERROR_MEMORY;
    }

    FixMenus(); /* Change Save to Update */
    imageFlag = TRUE;
    FreeOleString(lpDocName);
    DB_OUT("Out SrvrCreate\n");
    return OLE_OK;
}

/* Create a server doc from template. Similar to SrvrOpen except the
 * title is different */

OLESTATUS FAR PASCAL SrvrCreateFromTemplate(LPOLESERVER lpolesrvr,
                LHSERVERDOC lhdoc, POLESTR lpclassname, POLESTR lpOLEdocname,
                POLESTR lpOLEtemplatename, LPOLESERVERDOC FAR *lplpoledoc) {

    PPBDOC    pdoc;
    LPTSTR lptemplatename;
    LPTSTR lpdocname;
    OLESTATUS ret;

    DB_OUT("In SrvrCreateFromTemplate ");

    if (!SetOleString(&lptemplatename, lpOLEtemplatename)) {
        DB_OUT("ERR SrvrCreateFromTemplate\n");
        return OLE_ERROR_GENERIC;
    }

    if (!SetOleString(&lpdocname, lpOLEdocname)) {
        FreeOleString(lptemplatename);
        DB_OUT("ERR SrvrCreateFromTemplate\n");
        return OLE_ERROR_GENERIC;
    }

    if (!(pdoc = (PPBDOC)CreateDocFromFile((PPBSRVR)lpolesrvr, lptemplatename, lhdoc, lpdocname)))
        ret = OLE_ERROR_BLANK;
    else {

        /* Open the template and give it the correct name */
        lstrcpy(noFile, lpdocname);
        SetTitle(noFile);

        *lplpoledoc = (LPOLESERVERDOC)pdoc;
        FixMenus(); /* Change Save to Update */
        imageFlag = TRUE;

        ret = OLE_OK;
    }

    FreeOleString(lpdocname);
    FreeOleString(lptemplatename);
    DB_OUT("Out SrvrCreateFromTemplate\n");
    return ret;
}

/* Create a new server doc. It will be initialized later on by a call
 * to ItemSetData() */
OLESTATUS FAR PASCAL SrvrEdit(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
    POLESTR lpOLEclassname, POLESTR lpOLEdocname, LPOLESERVERDOC FAR *lplpoledoc) {

    LPTSTR lpdocname;

    DB_OUT("In SrvrEdit ");

    if (!SetOleString(&lpdocname, lpOLEdocname)) {
        DB_OUT("ERR SrvrEdit\n");
        return OLE_ERROR_GENERIC;
    }

    DB_OUT("SE1 ");

    if (!(*lplpoledoc =
        (LPOLESERVERDOC)CreateNewDoc((PPBSRVR)lpolesrvr, lhdoc, lpdocname))) {
        DB_OUT("SE2 ");
        FreeOleString(lpdocname);
        DB_OUT("ERR SrvrEdit\n");
        return OLE_ERROR_MEMORY;
    }

    DB_OUT("SE3 ");

    FixMenus(); /* Change Save to Update */
    DB_OUT("SE4 ");

    FreeOleString(lpdocname);
    DB_OUT("Out SrvrEdit\n");
    return OLE_OK;
}

/* Server should exit */
OLESTATUS FAR PASCAL SrvrExit(LPOLESERVER lpolesrvr) {
    PPBSRVR     psrvr = (PPBSRVR) lpolesrvr;

    DB_OUT("In SrvrExit ");
    DeleteServer(psrvr);
    DB_OUT("Out SrvrExit\n");
    return OLE_OK;
}

/* Server should exit only if it is invisible OR
 * if it was launched as a server and does not have any open doc */
OLESTATUS FAR PASCAL SrvrRelease(LPOLESERVER lpolesrvr) {
    PPBSRVR   psrvr = (PPBSRVR)lpolesrvr;

    DB_OUT( "In SrvrRelease ");

    /* If Pbrush is still invisible then it is OK to exit, OR
     * If Pbrush was launched as a server and there are no docs, then exit. */
    if (fInvisible || (fServer && !vpdoc))
        DeleteServer(psrvr);

    if (fServerRevoked && hServer) {
        /* Release the server virtual table and info */
        GlobalUnlock(hServer);
        GlobalFree(hServer);
        hServer = NULL;

        /* Destroy the window only when we're all through */
        if (pbrushWnd[PARENTid])
            DestroyWindow(pbrushWnd[PARENTid]);
    }

    DB_OUT( "Out SrvrRelease\n");
    return OLE_OK;
}

/********************* DOCUMENT FUNCTIONS ********************/
/* InitDoc() is in srvrinit.c */
void DeleteDoc(PPBDOC pdoc) {
    OLESTATUS olestat;

    if (pdoc) {
        if (iExitWithSaving != IDNO)
            SendDocChangeMsg(pdoc, OLE_CLOSED);

        olestat = OleRevokeServerDoc(pdoc->lhdoc);

#ifdef DEBUG
        if (olestat != OLE_OK && olestat != OLE_WAIT_FOR_RELEASE)
            PbrushOkError(W_FAILED_TO_REVOKE_DOCUMENT, MB_ICONHAND);
#endif
        vpdoc = NULL;
    }
}

void ChangeDocName(PPBDOC FAR *ppdoc, LPTSTR lpname)
{
#ifndef OLE_20
    CHAR  szAnsi[FILENAMElen + PATHlen];
#endif

    if (*ppdoc)
    {
        if ((*ppdoc)->aName)
            GlobalDeleteAtom((*ppdoc)->aName);
        (*ppdoc)->aName = GlobalAddAtom(lpname);

#ifndef OLE_20
        WideCharToMultiByte (CP_ACP, 0, lpname, -1, szAnsi, FILENAMElen + PATHlen, NULL, NULL);
        OleRenameServerDoc((*ppdoc)->lhdoc, szAnsi);
#else
        OleRenameServerDoc((*ppdoc)->lhdoc, lpname);
#endif
        if (cItems)
            return;

        /* This won't be necessary when Rename works */
        DeleteDoc(*ppdoc);
    }
    *ppdoc = InitDoc(vpsrvr, 0, lpname);
}

/********************** Document VTable Functions *********************/
OLESTATUS FAR PASCAL DocSave(LPOLESERVERDOC lpoledoc) {
    /*
     * The document will only have one client
     * area, so there is only one item to save.
     */
    DB_OUT("In DocSave ");


    /* Call paintbrush save document routine */
    SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEsave, 0L);
    DB_OUT("Out DocSave\n");
    return OLE_OK;
}

/* If the OLE library is requesting the doc delete, and not the user,
 * the client app is going away and we should kill ourselves.
 */
OLESTATUS FAR PASCAL DocClose(LPOLESERVERDOC lpoledoc) {
    PPBDOC    pdoc = (PPBDOC)lpoledoc;

    DB_OUT("In DocClose ");
    DeleteDoc(pdoc);
    DeleteServer(vpsrvr);
    DB_OUT("Out DocClose\n");
    return OLE_OK;
}

OLESTATUS FAR PASCAL DocRelease(LPOLESERVERDOC lpoledoc) {
    PPBDOC  pdoc = (PPBDOC)lpoledoc;
    HANDLE  hdoc;

    DB_OUT("In DocRelease ");
    if (pdoc->aName)
        GlobalDeleteAtom(pdoc->aName);
    GlobalUnlock(hdoc = pdoc->hdoc);
    GlobalFree(hdoc);

    DB_OUT("Out DocRelease\n");
    return OLE_OK;
}

/* Retrieve an item/object in a document.
 * An item is a rectangle in a picture.
 * For linked objects, it could be part of an image.
 */
OLESTATUS FAR PASCAL DocGetObject(LPOLESERVERDOC lpoledoc, POLESTR pitemname,
        LPOLEOBJECT FAR *lplpoleobject, LPOLECLIENT lpoleclient) {

    PITEM   pitem;
    RECT    rcImage;
    LPTSTR  pItemName;
    OLESTATUS oleStat = OLE_ERROR_GENERIC;

    DB_OUT("In DocGetObject ");

    if (!SetOleString(&pItemName, pitemname))
        goto errRtn;

    /*
     * Always create a new item in this case, it's much easier than
     * worrying about the sub-rectangle bitmap.  AddItem will remove
     * duplicates, and remove the item when its reference count is 0.
     */
    *lplpoleobject = NULL;
    if (!(pitem = CreateNewItem((PPBDOC)lpoledoc)))
        goto errRtn;

    pitem->lpoleclient = lpoleclient;

    /* The first time around, imageWid and imageHgt will only have
     * values if we are a linked object.
     */
    SetRect(&rcImage, 0, 0, imageWid, imageHgt);

    if (*pItemName) {
        RECT    rcIntersect;

        ScanRect(pItemName, &(pitem->rc));

        /* Perform item name validation checks in the linked case */
        if (vfIsLink
         && (!IntersectRect(&rcIntersect, &pitem->rc, &rcImage)
          || !EqualRect(&rcIntersect, &pitem->rc)))
            goto errRtn;

        pitem->aName = AddAtom(pItemName);
    } else {
            /* NULL item:  No rectangle to scan, use the image size */
        pitem->rc = rcImage;
        pitem->aName = 0;
    }

    *lplpoleobject = (LPOLEOBJECT)AddItem(pitem);

    oleStat = OLE_OK;

errRtn:
    if (oleStat != OLE_OK) {
        PbrushOkError(E_INVALID_ITEM_NAME, MB_ICONHAND);

        if (pitem) {
            HANDLE hitem;

            GlobalUnlock(hitem = pitem->hitem);
            GlobalFree(hitem);
        }
    }

    FreeOleString(pItemName);

    DB_OUT("Out DocGetObject\n");
    return oleStat;
}

TCHAR szClientName[MAXCLIENTNAME];

/*
 * Called for Embedded objects only.
 * Set the title bar to "Paintbrush - Paintbrush Picture in <lpdocName>".
 * Also change the File.Exit to "Exit & Return to <lpdocName>"
 */
OLESTATUS FAR PASCAL DocSetHostNames(LPOLESERVERDOC lpoledoc,
                            POLESTR lpOLEclientName, POLESTR lpOLEdocName)
{
    DWORD dwSize = KEYNAMESIZE;
    TCHAR szLoader[OBJSTRINGSMAX];
    TCHAR szBuffer[KEYNAMESIZE + 100];
    TCHAR szClass[KEYNAMESIZE];
    TCHAR szMenuStrFormat[CBMENUITEMMAX];
    HMENU hMenu;
    TCHAR lpFile[MAX_PATH];
    LPTSTR lpdocName, lpclientName;

    DB_OUT("In DocSetHostNames ");

    if (!SetOleString(&lpdocName, lpOLEdocName)) {
        DB_OUT("ERR DocSetHostNames\n");
        return OLE_ERROR_GENERIC;
    }

    if (!SetOleString(&lpclientName, lpOLEclientName)) {
        FreeOleString(lpdocName);
        DB_OUT("ERR DocSetHostNames\n");
        return OLE_ERROR_GENERIC;
    }

    // IDSPicture = "Paintbrush Picture"
    lstrcpy(szClientName, lpclientName);
    if (RegQueryValue(HKEY_CLASSES_ROOT, TEXT("PBrush"), szClass, (PLONG)&dwSize))
    {
         LoadString(hInst, IDSpicture, szClass, CharSizeOf(szClass));
    }

    lstrcpy (lpFile, PFileInPath(lpdocName));
    if (lpFile[0] == (TCHAR) 0)
        LoadString(hInst, IDSuntitled, lpFile, CharSizeOf(lpFile));

    LoadString (hInst, IDSxiny, szLoader, CharSizeOf(szLoader));
#ifdef JAPAN // added by Hiraisi (BUG#3989/WIN31)
    wsprintf(szBuffer, szLoader, lpFile);
#else
    wsprintf (szBuffer, szLoader, szClass, lpFile);
#endif
    lstrcpy (fileName, lpFile);
    SetTitle (szBuffer);

    LoadString (hInst, IDS_EXITANDRETURN, szMenuStrFormat, CharSizeOf(szMenuStrFormat));
    wsprintf (szBuffer, szMenuStrFormat, lpFile);
    hMenu = GetMenu (pbrushWnd[PARENTid]);
    ModifyMenu (hMenu, FILEexit, MF_BYCOMMAND | MF_STRING, FILEexit, szBuffer);

    FreeOleString(lpdocName);
    FreeOleString(lpclientName);

    DB_OUT("Out DocSetHostNames\n");
    return OLE_OK;
}

OLESTATUS FAR PASCAL DocSetDocDimensions(LPOLESERVERDOC lpoledoc, LPRECT lprc) {
    /* Links should never call this function, but we will never
     * succeed in resizing the BITMAP (no plans to re-BitBlt()).
     */
#ifdef DEBUG
    if (vfIsLink)
        PbrushOkError(E_SET_DIMENSIONS_UNSUPPORTED, MB_ICONEXCLAMATION);
    else
        PbrushOkError(W_SET_DIMENSIONS_UNSUPPORTED, MB_ICONINFORMATION);
#endif

    return OLE_OK;
}

OLESTATUS FAR PASCAL DocSetColorScheme(LPOLESERVERDOC lpoledoc, LPLOGPALETTE lppal) {

    HPALETTE    hNewPal;

    DB_OUT("In DocSetColorScheme ");

    if (!(hNewPal = CreatePalette(lppal))) {
        DB_OUT("ERR DocSetColorScheme\n");
        return OLE_ERROR_GENERIC;
    }

    if (hNewPal != hPalette) {
        SelectPalette(hdcWork, hNewPal, 0);
        RealizePalette(hdcWork);

        SelectPalette(hdcImage, hNewPal, 0);
        RealizePalette(hdcImage);

#ifdef WANT_RIPS
        DeleteObject(hPalette);
#endif
        hPalette = hNewPal;
    }

    DB_OUT("Out DocSetColorScheme\n");
    return OLE_OK;
}

/**************************** ITEM SUBROUTINES ***************************/
void FAR CutCopyObjectFormats(HDC hDC, HBITMAP hBitmap, RECT rc, WORD msg) {
    /* This function is called when the clipboard is already opened */

    HANDLE      hdata = NULL;
    RECT        rc2;

    /* Set the Clipboard */
    /* Native data... (in our case, the item rectangle sub-BITMAP) */
    /* Return a handle to native data (DIB file format) */
    if (hdata = GetNative(hDC, hBitmap, rc))
        SetClipboardData(vcfNative, hdata);

    /* Link data... (PBrush\0<Doc name>\0<Item name>\0\0) */
    /* Don't copy a link if the doc has no name, or if it's a CUT operation */
    if (vfIsLink && (*fileName) && msg != WM_CUT && theTool != SCISSORStool
        && (hdata = GetLink(rc)))
        SetClipboardData(vcfLink, hdata);

    SetRect(&rc2, 0, 0, rc.right - rc.left, rc.bottom - rc.top);
    /* Reconstruct this, for the time being */
    if (hdata = GetLink(rc2))
        SetClipboardData(vcfOwnerLink, hdata);

    /* Metafile data */
    /* Skip metafile copy if we are in 3.0 compatibility mode */
    if (!fOmitPictureFormat && (hdata = GetMF(hDC, hBitmap, rc)))
        SetClipboardData(CF_METAFILEPICT, hdata);
}

PITEM CreateNewItem(PPBDOC pdoc) {
    HANDLE      hitem = NULL;
    PITEM       pitem = NULL;

    /* Now create the item */
    hitem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(ITEM));

    if (!hitem || (!(pitem = (PITEM) GlobalLock(hitem))))
        goto errRtn;

    pitem->hitem = hitem;
    pitem->oleobject.lpvtbl = &vitemvtbl;
    /* pitem->rc will be filled out by the caller */

    return pitem;

 errRtn:
    if (pitem)
        GlobalUnlock(hitem);

    if (hitem)
        GlobalFree(hitem);

    return NULL;
}

BOOL SendDocChangeMsg(PPBDOC pdoc, WORD options) {
    BOOL fSuccess = FALSE;
    int i;
#ifdef LATER
    RECT rc;
#endif

    for (i = 0; i < cItems; i++) {
#ifdef LATER
        /* If item has been updated, ... */
        if (IntersectRect(&rc, &vrcModified, &vpitem[i].rc))
#endif
            fSuccess = SendItemChangeMsg(vpitem[i], options) || fSuccess;
    }
    return fSuccess;
}

BOOL SendItemChangeMsg(PITEM pitem, WORD options) {
    if (pitem->lpoleclient) {
       (*pitem->lpoleclient->lpvtbl->CallBack)
           (pitem->lpoleclient, options, (LPOLEOBJECT)pitem);
        return TRUE;
     }
     return FALSE;
}

/********************* Item VTable Subroutines **************************/
OLESTATUS FAR PASCAL ItemDelete(LPOLEOBJECT lpoleobject) {
    DB_OUT("In ItemDelete ");
    DeleteItem((PITEM)lpoleobject);
    DB_OUT("Out ItemDelete\n");
    return OLE_OK;              /* Add error checking later */
}

OLESTATUS FAR PASCAL ItemGetData(LPOLEOBJECT lpoleobject,
                                 OLECLIPFORMAT cfFormat, LPHANDLE lphandle) {
    HCURSOR hcurOld;
    PITEM pitem = (PITEM)lpoleobject;

    DB_OUT("In ItemGetData ");
    /* Suppress data updates? */
    if (!fSendData) {
        *lphandle = NULL;
        return OLE_OK;
    }

    hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    if (cfFormat == vcfNative) {
        *lphandle = GetNative(NULL, NULL, pitem->rc);
    } else if (cfFormat == vcfLink) {
        *lphandle = GetLink(pitem->rc);
    } else if (cfFormat == CF_METAFILEPICT) {
        *lphandle = GetMF(NULL, NULL, pitem->rc);
    } else if ((cfFormat == CF_BITMAP) || (cfFormat == CF_DIB)) {
        *lphandle = GetBitmap(pitem);
    } else if (cfFormat == vcfOwnerLink) {
        RECT rc2;

        SetRect(&rc2, 0, 0, pitem->rc.right - pitem->rc.left,
                            pitem->rc.bottom - pitem->rc.top);

        *lphandle = GetLink(rc2);
    }
    SetCursor(hcurOld);
    DB_OUT("Out ItemGetData\n");
    return (*lphandle) ? OLE_OK : OLE_ERROR_MEMORY;
}

OLESTATUS FAR PASCAL ItemSetData(LPOLEOBJECT lpoleobject,
                                 OLECLIPFORMAT cfFormat, HANDLE hdata) {
/*
 * Read in the embedded object data in Native format.
 * (or Link format).  Presumably this will not be called
 * unless we are editing the correct document.
 */
    BOOL        fError = FALSE;
    PITEM       pitem = (PITEM)lpoleobject;
    HCURSOR     hcurOld;

    DB_OUT("In ItemSetData ");

    hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (cfFormat == vcfNative)
        fError = PutNative(pitem, pbrushWnd[PAINTid], hdata);

    /* We should also be able to accept an HBITMAP here */
    SetCursor(hcurOld);
    GlobalFree(hdata);
    DB_OUT("Out ItemSetData\n");
    return fError ? OLE_ERROR_GENERIC : OLE_OK;
}

OLESTATUS FAR PASCAL ItemShow(LPOLEOBJECT lpoleobject, BOOL fActivate) {
    PITEM       pitem = (PITEM)lpoleobject;
    RECT        rcTemp;

    DB_OUT("In ItemShow ");

    if (fActivate) {
        fInvisible = FALSE;
        DB_OUT("is: fInvs=FALSE ");

        /* Show the item.  This routine is called when the user
         * double clicks in the client application, and the PBrush
         * server is already active.  If iconic, restore the window;
         * then give it the focus.
         */
        if (IsIconic(pbrushWnd[PARENTid]))
            SendMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_RESTORE, 0L);
        SetForegroundWindow(pbrushWnd[PARENTid]);
        BringWindowToTop(pbrushWnd[PARENTid]);

        /* If it's a link, be sure to show the item */
        if (vfIsLink) {
            // int itemHeight, itemWidth;

            DB_OUT("is: is link " );

            if (bZoomedOut)
                SendMessage(zoomOutWnd, WM_KEYDOWN, VK_ESCAPE, 0L);
            else if (inMagnify)
                SendMessage(pbrushWnd[PARENTid], WM_COMMAND,
                      GET_WM_COMMAND_MPS(MISCzoomOut, NULL, 0));

#if 0
            /* if item is larger than the paint Window, MAXIMIZE to show the
             * entire image */
            itemWidth = pitem->rc.right - pitem->rc.left;
            itemHeight = pitem->rc.bottom - pitem->rc.top;
            if (itemWidth >= (imageView.right - imageView.left) ||
                itemHeight >= (imageView.bottom - imageView.top))
                SendMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_MAXIMIZE, 0L);
#endif

            /* check if the image has to be scrolled into view */
            IntersectRect(&rcTemp, &(pitem->rc), &imageView);
            if (!EqualRect(&rcTemp, &(pitem->rc)))
                MakeObjectVisible((LPRECT)&(pitem->rc));

            /* Set the tool to "rectangle" selection */
            SendMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, PICKtool, 0L);

            /* Now, draw the selection rectangle */
            //InvalidateRect(pbrushWnd[PARENTid], NULL, FALSE);
            RedrawWindow(pbrushWnd[PARENTid], NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASENOW);
            UpdateWindow(pbrushWnd[PARENTid]);
            SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
            pickRect = pitem->rc;
            --pickRect.bottom;
            --pickRect.right;
            OffsetRect(&pickRect, -imageView.left, -imageView.top);
            /* If more than one instance of pbrush is launched while editing
             * multiple links, focus changes back and forth and we don't want
             * the image with outline to be retrieved.
             * Use TerminateKill to avoid saving this intermediate form of the image.
             */
            TerminateKill = FALSE;
            SendMessage(pbrushWnd[PAINTid], WM_OUTLINE, 0, 0L);
            TerminateKill = TRUE;
        } else  /* Force a full frame redraw */ {
            DB_OUT("is: InvRect(FALSE) ");
            //InvalidateRect(pbrushWnd[PARENTid], NULL, FALSE);
            RedrawWindow(pbrushWnd[PARENTid], NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASENOW);

        }
    }

    DB_OUT("Out ItemShow\n");
    return OLE_OK;
}

static void MakeObjectVisible(
    LPRECT lpItemRect)
{
    int xPos;
    int yPos;

    /* Fix the x coordinate */
    if (lpItemRect->left < imageView.left)
        xPos = lpItemRect->left;
    else if (lpItemRect->right > imageView.right)
        xPos = imageView.left + (lpItemRect->right - imageView.right);
    else
        xPos = imageView.left;      /* unchanged */

    /* Fix the y coordinate */
    if (lpItemRect->top < imageView.top)
        yPos = lpItemRect->top;
    else if (lpItemRect->bottom > imageView.bottom)
        yPos = imageView.top + (lpItemRect->bottom - imageView.bottom);
    else
        yPos = imageView.top;       /* unchanged */

    /* Scroll the image */
    if (xPos != imageView.left)
        SendMessage(pbrushWnd[PAINTid], WM_HSCROLL,
                    SB_THUMBPOSITION, MAKELONG(xPos, 0));

    if (yPos != imageView.top)
        SendMessage(pbrushWnd[PAINTid], WM_VSCROLL,
                    SB_THUMBPOSITION, MAKELONG(yPos, 0));
}

OLESTATUS FAR PASCAL ItemSetBounds(LPOLEOBJECT lpoleobject, LPRECT lprc) {
    /* Again, we won't allow the BITMAP dimensions to change */
#ifdef DEBUG
    if (vfIsLink)
        PbrushOkError(E_SET_DIMENSIONS_UNSUPPORTED, MB_ICONEXCLAMATION);
    else
        PbrushOkError(W_SET_DIMENSIONS_UNSUPPORTED, MB_ICONINFORMATION);
#endif

    return OLE_ERROR_GENERIC;
}

OLESTATUS FAR PASCAL ItemSetTargetDevice(LPOLEOBJECT lpoleobject, HANDLE h) {
#ifdef DEBUG
    PbrushOkError(W_SET_TARGET_DEVICE_UNSUPPORTED, MB_ICONINFORMATION);
#endif
    if (h)
        GlobalFree(h);

    return OLE_ERROR_GENERIC;
}

OLECLIPFORMAT FAR PASCAL ItemEnumFormats(LPOLEOBJECT lpobject,
                                         OLECLIPFORMAT cfFormat) {

    /* This is called by the OLE libraries to get a format for
     * display on the screen.
     */
    DB_OUT("In ItemEnumFormats ");

    if (!cfFormat) {
        DB_OUT("Out1 ItemEnumFormats\n");
        return CF_BITMAP;
    }

    if ((cfFormat == CF_BITMAP) || (cfFormat == CF_DIB)) {
        DB_OUT("Out2 ItemEnumFormats\n");
        return CF_METAFILEPICT;
    }

    if (cfFormat == CF_METAFILEPICT) {
        DB_OUT("Out3 ItemEnumFormats\n");
        return vcfNative;
    }

    return 0;
}

LPVOID FAR PASCAL ItemQueryProtocol(LPOLEOBJECT lpoleobject, POLESTR lpprotocol) {
    LPVOID lpv;

    DB_OUT("In ItemQueryProtocol ");

#ifndef OLE_20
    lpv =  (!lstrcmpiA(lpprotocol, "StdFileEditing") ? lpoleobject : NULL);
#else
    lpv =  (!lstrcmpi(lpprotocol, TEXT("StdFileEditing")) ? lpoleobject : NULL);
#endif

    DB_OUT2(lpv == NULL, "ret NULL ", "ret obj ");
    DB_OUT("Out ItemQueryProtocol\n");

    return lpv;
}

OLESTATUS FAR PASCAL ItemSetColorScheme(LPOLEOBJECT lpoleobject, LPLOGPALETTE lppal) {
    DB_OUT("InERROut ItemSetColorScheme\n");
    return OLE_ERROR_GENERIC;
}

OLESTATUS CALLBACK ItemDoVerb(LPOLEOBJECT lpoleobject,
                                UINT wVerb, BOOL fShow, BOOL fActivate) {

    OLESTATUS ols = OLE_OK;

    DB_OUT("In ItemDoVerb ");
    DB_OUT2(fShow, "fShow ", "No Show ");

    if (fShow)
        ols = (*lpoleobject->lpvtbl->Show)(lpoleobject, fActivate);

    DB_OUT("Out ItemDoVerb\n" );
    return ols;
}

/******************** APPLICATION SUPPLIED FUNCTIONS *********************/
PPBDOC CreateNewDoc(PPBSRVR psrvr, LHSERVERDOC lhdoc, LPTSTR lpstr) {
    lstrcpy(noFile, lpstr);
    SetTitle(noFile);
    return (vpdoc = InitDoc(psrvr, lhdoc, lpstr));
}

PPBDOC CreateDocFromFile(PPBSRVR psrvr, LPTSTR lpstr, LHSERVERDOC lhdoc, LPTSTR lpstrDocName) {
    /* Return TRUE iff document created.  Call paintbrush read from file here */

    LPTSTR   lpTmp;
    LPTSTR   lpTmp2 = NULL;

    /* Parse the path and the file name */
    lpTmp = lpstr;
    while (*lpTmp) {
        if (*lpTmp == TEXT('\\'))
            lpTmp2 = lpTmp;
#ifdef DBCS
        lpTmp = CharNext(lpTmp);
#else
        lpTmp++;
#endif
    }
    if (lpTmp2) {           /* There was a backslash! */
        *lpTmp2++ = 0;
        lstrcpy(filePath, lpstr);
        changeDiskDir(filePath);
        lstrcpy(fileName, lpTmp2);
        *(--lpTmp2) = TEXT('\\'); /* restore it */
    } else {
        *filePath = (TCHAR) 0;
        lstrcpy(fileName, lpstr);
    }

    /* Read in the document */
    if (LoadBitmapFile(pbrushWnd[PAINTid], fileName, NULL))
        return NULL;

    NewImage(0);

    /* Set the initial modified bounding rect to NULL */
    SetRect(&vrcModified, -1, -1, -1, -1);

    /* Initialize document */
    return (vpdoc = InitDoc(psrvr, lhdoc, lpstrDocName));
}

HANDLE GetNative(HDC hDC, HBITMAP hBitmap, RECT rc) {
/*
 * Derived from SaveBitmapFile()
 *
 * The native format is the entire DIB file format
 * (it is insufficient to just provide a handle).
 */
    int       i;
    int       width, height;
    BOOL      error = TRUE;
    LPBYTE      lpBits;
    BYTE huge *hpBits;
    HDC       parentDC = NULL;
    int       ScanLineSize, InfoSize;
    DWORD     dwImgSize;
    HANDLE    hLink = NULL;
    LPBYTE     lpLink = NULL;
    HCURSOR   hOldCursor;
    HBITMAP   hOldBitmap = NULL;
    HPALETTE  hOldPalette = NULL;
    LPBITMAPFILEHEADER  lpHdr;
    LPBITMAPINFO lpInfo;
    LPBITMAPINFO lpbmInfo = NULL;           /* deal with alignment -- FGS */
    HANDLE    hbmInfo=NULL;

    /* Compute the rectangle to be selected */
    width = (rc.right - rc.left);
    height = (rc.bottom - rc.top);

    hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    if (!hDC) {
        if (!(fileDC = CreateCompatibleDC(NULL)))
        {
            PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
            goto error1;
        }
    }

    if (hPalette) {
       hOldPalette = SelectPalette(hDC? hDC: fileDC, hPalette, 0);
       RealizePalette(hDC? hDC: fileDC);
    }

    switch (wFileType) {
        case BITMAPFILE24:
            i = 24;
            break;

        case BITMAPFILE:
        case MSPFILE:
            i = 1;
            break;

        case BITMAPFILE4:
            i = 4;
            break;

        case BITMAPFILE8:
            i = 8;
            break;
        case PCXFILE:
            i = imagePlanes * imagePixels;
            break;
    }
    /* BITMAPINFOSIZE = size of header + color table. */
    /* no color table for 24 bpp bitmaps. */
    InfoSize = sizeof(BITMAPINFOHEADER) +
                ((i == 24)? 0: (sizeof(RGBQUAD) << i));
    /* Scan Line should be DWORD aligned and the length is in bytes */
    ScanLineSize = ((((WORD)width * (WORD)i) + 31)/32) * 4;

    /* Allocate memory */
    dwImgSize = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)InfoSize +
                (DWORD)ScanLineSize * (DWORD)height;

    hLink = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, dwImgSize);
    if (!hLink || (!(lpLink = (LPBYTE)GlobalLock(hLink)))) {
        SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
        goto error2;
    }
    hbmInfo = GlobalAlloc(GHND, InfoSize);
    if (!hbmInfo || (!(lpbmInfo = (LPBITMAPINFO)GlobalLock(hbmInfo)))) {
        SimpleMessage(IDSNoMemAvail, fileName, MB_OK | MB_ICONEXCLAMATION);
        goto error2;
    }
    lpHdr  = (LPBITMAPFILEHEADER)lpLink;
    lpInfo = (LPBITMAPINFO)(lpLink + sizeof(BITMAPFILEHEADER));
    lpBits = (LPBYTE)(lpLink + sizeof(BITMAPFILEHEADER) + InfoSize);

    /* fill in image header */
    lpbmInfo->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    lpbmInfo->bmiHeader.biWidth       = width;
    lpbmInfo->bmiHeader.biHeight        = height;
    lpbmInfo->bmiHeader.biPlanes              = 1;
    lpbmInfo->bmiHeader.biBitCount      = (WORD)i;
    lpbmInfo->bmiHeader.biCompression   =
    lpbmInfo->bmiHeader.biClrUsed            =
    lpbmInfo->bmiHeader.biClrImportant  = 0;
    lpbmInfo->bmiHeader.biSizeImage     = 0;
    lpHdr->bfOffBits = sizeof(BITMAPFILEHEADER) + InfoSize;
    lpbmInfo->bmiHeader.biXPelsPerMeter = PelsPerLogMeter(hDC ? hDC : fileDC,TRUE);
    lpbmInfo->bmiHeader.biYPelsPerMeter = PelsPerLogMeter(hDC ? hDC : fileDC,FALSE);

    if (!hDC) {
        int ht;

        for (ht = height, fileBitmap = NULL; ht && !fileBitmap; ) {

            if (!(fileBitmap = CreateBitmap(width, ht, (BYTE)imagePlanes,
                                            (BYTE)imagePixels, NULL)))
                ht = ht >> 1;
        }

        if (!ht) {
            PbrushOkError(IDSNotEnufMem, MB_ICONHAND);
            goto error2;
        }

        lpbmInfo->bmiHeader.biHeight      = ht;
        for (i = height; i; i -= ht) {
            if (ht > i)
                lpbmInfo->bmiHeader.biHeight = ht = i;

            if(!(hOldBitmap = SelectObject(fileDC, fileBitmap))) {
               SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
               goto error3;
            }

            BitBlt(fileDC, 0, 0, width, ht,
                    hdcWork, rc.left, rc.top + (i - ht), SRCCOPY);

            if(!SelectObject(fileDC, hOldBitmap)) {
               SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
               goto error3;
            }

            GetDIBits(fileDC, fileBitmap, 0, ht, lpBits, lpbmInfo, DIB_RGB_COLORS);
            hpBits = lpBits;
            hpBits += (DWORD)ht * (DWORD)ScanLineSize;
            lpBits = hpBits;
        }
        lpbmInfo->bmiHeader.biHeight       = height;
    } else
        GetDIBits(hDC, hBitmap, 0, height, lpBits, lpbmInfo, DIB_RGB_COLORS);

    lpHdr->bfSize = dwImgSize;
    lpHdr->bfType = DIBID;
    lpHdr->bfReserved1 = lpHdr->bfReserved2 = 0;
    lpbmInfo->bmiHeader.biSizeImage = (DWORD)height * (DWORD)ScanLineSize;

    RepeatMove(lpInfo,lpbmInfo,InfoSize);
    error = FALSE;

error3:
    if (!hDC && hOldBitmap)
        SelectObject(fileDC, hOldBitmap);

    if (!hDC && fileBitmap)
        DeleteObject(fileBitmap);

error2:
    if (lpLink)
        GlobalUnlock(hLink);

    if (lpbmInfo)
        GlobalUnlock(hbmInfo);

    if (hbmInfo) {
        GlobalFree(hbmInfo);
        hbmInfo = NULL;
    }

    if (error && hLink) {
        GlobalFree(hLink);
        hLink = NULL;
    }

    if (!hDC) {
        if (fileDC) {
            if (hPalette && hOldPalette)
                SelectPalette(hDC? hDC :fileDC, hOldPalette, 0);
            DeleteDC(fileDC);
        }
    }

error1:
    SetCursor(hOldCursor);
    return hLink;
}

BOOL PutNative(PITEM pitem, HWND hWnd, HANDLE hdata) {
    LPBYTE               lpNative;
    LPBITMAPFILEHEADER  lphdr;
    BITMAPINFO  UNALIGNED * lphdrInfo;
    LPBITMAPINFO        lpDIBinfo = NULL;               /* needed for alignment */
    BOOL                error = TRUE;
    int                 wplanes, wbitpx, i;
    HANDLE              hDIBinfo = NULL;
    HDC                 hdc = NULL, parentdc = NULL;
    HBITMAP             hbitmap = NULL, htempbit = NULL;
    DWORD               dwSize, dwNumColors;
    HCURSOR             oldCsr;
    UINT                errmsg, wSize;
    UINT                wUsage;
    HPALETTE            hNewPal = NULL;
    LPBYTE               lpTemp;
    int                 rc;
    BYTE huge           *hpTemp;
    int                 ht;

    if (!(lpNative = GlobalLock(hdata))) {
        errmsg = IDSUnableHdr;
        return TRUE;
    }

    lphdr = (LPBITMAPFILEHEADER)lpNative;
    lphdrInfo = (BITMAPINFO UNALIGNED *)(lpNative + sizeof(BITMAPFILEHEADER));

    if (lphdrInfo->bmiHeader.biPlanes != 1 || lphdr->bfType != DIBID) {
        errmsg = IDSUnableHdr;
        goto error1;
    }

    if (lphdrInfo->bmiHeader.biCompression) {
        errmsg = IDSUnknownFmt;
        goto error1;
    }

    if (!(dwNumColors = lphdrInfo->bmiHeader.biClrUsed))
       if (lphdrInfo->bmiHeader.biBitCount != 24)
          dwNumColors = (1L << lphdrInfo->bmiHeader.biBitCount);

    if (!(parentdc = GetDisplayDC(hWnd))) {
        errmsg = IDSCantAlloc;
        goto error1;
    }

    if (lphdrInfo->bmiHeader.biBitCount != 1) {
        wplanes = GetDeviceCaps(parentdc, PLANES);
        wbitpx = GetDeviceCaps(parentdc, BITSPIXEL);
    } else {
        wplanes = 1;
        wbitpx = 1;
    }

    oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Create a new image with the new sizes, etc. */
    nNewImageWidth      = LOWORD(lphdrInfo->bmiHeader.biWidth);
    nNewImageHeight     = LOWORD(lphdrInfo->bmiHeader.biHeight);
    nNewImagePlanes     = wplanes;
    nNewImagePixels     = wbitpx;

    rc = AllocImg(nNewImageWidth, nNewImageHeight,
                      nNewImagePlanes, nNewImagePixels, FALSE);

    dwSize = sizeof(BITMAPINFOHEADER)
           + dwNumColors * sizeof(RGBQUAD);

    if (!(hDIBinfo = GlobalAlloc(GMEM_MOVEABLE, dwSize))) {
        errmsg = IDSCantAlloc;
        goto error2;
    }

    if (!(lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo))) {
        errmsg = IDSCantAlloc;
        goto error3;
    }

    /* copy header into allocated memory */
    RepeatMove((LPBYTE)lpDIBinfo, (LPBYTE)lphdrInfo, (WORD)dwSize);
    hNewPal = MakeImagePalette(hPalette, hDIBinfo, &wUsage);
    if (hNewPal && hNewPal != hPalette) {
        SelectPalette(hdcWork, hNewPal, 0);
        RealizePalette(hdcWork);
        SelectPalette(hdcImage, hNewPal, 0);
        RealizePalette(hdcImage);
#ifdef WANT_RIPS
        DeleteObject(hPalette);
#endif
        hPalette = hNewPal;
    }

    hdc = CreateCompatibleDC(parentdc);
    ReleaseDC(hWnd, parentdc);
    parentdc = NULL;

    if (!hdc) {
        errmsg = IDSNoMemAvail;
        goto error4;
    }

    for (ht = nNewImageHeight, hbitmap = NULL; ht && !hbitmap; ) {
        if (!(hbitmap = CreateBitmap(nNewImageWidth, ht,
                                     (BYTE) imagePlanes, (BYTE) imagePixels,
                                     NULL)))
            ht = ht >> 1;
    }

    if (!ht) {
        errmsg = IDSNoMemAvail;
        goto error5;
    }

    if (hPalette) {
       SelectPalette(hdc, hPalette, FALSE);
       RealizePalette(hdc);
    }

    wSize = ((nNewImageWidth
                * lphdrInfo->bmiHeader.biBitCount + 31) & (-32)) >> 3;

    lpTemp = lpNative + sizeof(BITMAPFILEHEADER) + dwSize;

    error = FALSE;

    if (!(htempbit = SelectObject(hdc, hbitmap))) {
        errmsg = IDSNoMemAvail;
        goto error6;
    }

    lpDIBinfo->bmiHeader.biHeight = ht;
    for (i = nNewImageHeight; i; i -= ht) {
        if (i < ht)
             lpDIBinfo->bmiHeader.biHeight = ht = i;

        if(!SelectObject(hdc, htempbit)) {
           errmsg = IDSNoMemAvail;
           goto error6;
        }

        if(!SetDIBits(hdc, hbitmap, 0, ht, lpTemp, lpDIBinfo, wUsage)) {
            errmsg = IDSNoMemAvail;
            error = TRUE;
            break;
        }

        if(!SelectObject(hdc, hbitmap)) {
           errmsg = IDSNoMemAvail;
           goto error6;
        }

        BitBlt(hdcWork, 0, i - ht, nNewImageWidth, ht,
                hdc, 0, 0, SRCCOPY);

        hpTemp = lpTemp;
        hpTemp += (DWORD)ht * (DWORD)wSize;
        lpTemp = hpTemp;
    }

    /* Copy the work bitmap into the undo bitmap */
    BitBlt(hdcImage, 0, 0, nNewImageWidth, nNewImageHeight,
            hdcWork, 0, 0, SRCCOPY);

    wFileType = GetImageFileType(lphdrInfo->bmiHeader.biBitCount);

    if (error)
        ClearImg();

    imageFlag = FALSE;          /* Doesn't need saving */

    NewImage(rc);

    /* Fix the item dimensions */
    SetRect(&(pitem->rc), 0, 0, nNewImageWidth, nNewImageHeight);

    if (!error)
        GetCurrentDirectory(PATHlen, filePath);

error6:
    if (htempbit)
        SelectObject(hdc, htempbit);

    DeleteObject(hbitmap);

error5:
    DeleteDC(hdc);

error4:
    GlobalUnlock(hDIBinfo);

error3:
    GlobalFree(hDIBinfo);

error2:
    SetCursor(oldCsr);
    if (parentdc)
        ReleaseDC(hWnd, parentdc);

error1:
    GlobalUnlock(hdata);
    return error;
}

PITEM AddItem(PITEM pitem) {
    int  i;
    HANDLE hitem;

    i = FndItem((PITEM)pitem);
    if (i < cItems) {
        vpitem[i]->ref++;

        /* Free the duplicate item */
        GlobalUnlock(hitem = pitem->hitem);
        GlobalFree(hitem);
    } else {
        if (i < CMAXITEMS) {
            vpitem[cItems] = (PITEM)pitem;
            vpitem[cItems++]->ref = 1;
        } else
            return NULL;
    }
    return vpitem[i];
}

BOOL DeleteItem(PITEM pitem) {
    int  i;

    i = FndItem(pitem);
    if (i < cItems && vpitem[i]->ref)
        vpitem[i]->ref--;
    else
        return FALSE;

    /* Remove the item from the item table */
    if (!vpitem[i]->ref) {
        GlobalUnlock(vpitem[i]->hitem);
        GlobalFree(vpitem[i]->hitem);
        for (i++; i < cItems; i++)
            vpitem[i - 1] = vpitem[i];
        cItems--;
    }
    return TRUE;
}

int FndItem(PITEM pitem) {
    BOOL        fFound;
    int         i;

    fFound = FALSE;
    for (i = 0; i < cItems && !fFound;) {
        if (pitem->aName == vpitem[i]->aName) {
            fFound = TRUE;
        } else i++;
    }
    return i;
}

void ScanRect(LPTSTR lpstr, LPRECT lprc) {
    TCHAR Buf[40];
    LPTSTR   lpstrBuf = Buf;

    lstrcpy(Buf, lpstr);
    GetNum(&lpstrBuf, (LPINT)&(lprc->left));
    GetNum(&lpstrBuf, (LPINT)&(lprc->top));
    GetNum(&lpstrBuf, (LPINT)&(lprc->right));
    GetNum(&lpstrBuf, (LPINT)&(lprc->bottom));
}

int OutRect(LPTSTR lpstr, int cb, RECT rc) {
    TCHAR Buf[40];
    int  cch;

    wsprintf(Buf, TEXT("%d %d %d %d"), rc.left, rc.top, rc.right, rc.bottom);
    if ((cch = lstrlen(Buf)) > cb)
        Buf[cch = cb] = 0;

    lstrcpy(lpstr, Buf);
    return cch;
}

#define CBLINKMAX   128

HANDLE GetLink(RECT rc) {
    TCHAR pchlink[CBLINKMAX];
    int  cblink;
#ifndef OLE_20
    int cchLink;
#endif
    HANDLE hlink;
    POLESTR   lplink;

    /* Link data... (PBrush\0<Doc name>\0<Item name>\0\0) */
    lstrcpy(pchlink, pgmName);
    cblink    = lstrlen(pchlink) + 1;

    /* Copy filePath\fileName */
    lstrcpy((LPTSTR)(pchlink + cblink), filePath);
    if (filePath[lstrlen(filePath)-1] != TEXT('\\'))
        lstrcat(pchlink + cblink, TEXT("\\"));
    lstrcat(pchlink + cblink, fileName);
    cblink += lstrlen(pchlink + cblink) + 1;
    cblink   += (int)OutRect(pchlink + cblink, CBLINKMAX - cblink, rc) + 1;
    pchlink[cblink++] = (TCHAR) 0;       /* throw in another NULL at the end */

#ifndef OLE_20
    cchLink = cblink;
    cblink = WideCharToMultiByte(CP_ACP, 0, pchlink, cchLink, NULL, 0,
            NULL, NULL);
#else
    cblink *= sizeof(TCHAR);
#endif

    hlink = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, cblink);

    if (hlink) {
        if (lplink = (POLESTR)GlobalLock(hlink)) {
#ifndef OLE_20
            WideCharToMultiByte(CP_ACP, 0, pchlink, cchLink, lplink, cblink,
                    NULL, NULL);
#else
            RepeatMove(lplink, pchlink, (WORD)cblink);
#endif
            GlobalUnlock(hlink);
        } else {
            GlobalFree(hlink);
            hlink = NULL;
        }
    }
    return hlink;
}


HBITMAP GetBitmap(PITEM pitem) {
    HDC         hdcmem;
    HBITMAP     hbitmap;
    HBITMAP     holdbitmap;
    int         cxBitmap;
    int         cyBitmap;

    cxBitmap = (pitem->rc.right - pitem->rc.left);
    cyBitmap = (pitem->rc.bottom - pitem->rc.top);

    hdcmem = CreateCompatibleDC(NULL);
    hbitmap = CreateBitmap(cxBitmap, cyBitmap,
                           (BYTE)imagePlanes, (BYTE)imagePixels, 0);
    holdbitmap = SelectObject(hdcmem, hbitmap);

    MSetWindowOrg(hdcmem, 0, 0);
    MSetWindowExt(hdcmem, cxBitmap, cyBitmap);

    /* Paint directly into the BITMAP */
    BitBlt(hdcmem, 0, 0, cxBitmap, cyBitmap,
            hdcWork, pitem->rc.left, pitem->rc.top, SRCCOPY);

    hbitmap = SelectObject(hdcmem, holdbitmap);
    DeleteDC(hdcmem);
    return hbitmap;
}

HANDLE GetMF(HDC hDC, HBITMAP hBitmap, RECT rc) {
    BOOL            fCreatedDC  = FALSE;
    BOOL            fError      = TRUE;
    HANDLE          hmfpict     = NULL;
    HBITMAP         hbm         = NULL;
    HBITMAP         hbmOld      = NULL;
    HPALETTE        hOldPalette = NULL, hDefPalette = NULL;
    HDC             hdcMF       = NULL;
    HDC             hdcWnd      = NULL;
    HWND            hwndFrame   = pbrushWnd[PARENTid];
    LPMETAFILEPICT  lpmfpict    = NULL;
    int             cxImage;
    int             cyImage;


    /* Compute the image size */
    cxImage = rc.right - rc.left;
    cyImage = rc.bottom - rc.top;

    /* Get the window DC */
    if (!(hdcWnd = GetDC(hwndFrame)))
        goto Error;

    if (!hDC) {
        fCreatedDC = TRUE;

        /* Make a DC compatible to the window DC, and initialize a bitmap. */
        if (!(hDC = CreateCompatibleDC(NULL))
         || !(hbm = CreateCompatibleBitmap(hdcWnd, cxImage, cyImage))
         || (hPalette && !(hOldPalette = SelectPalette(hDC, hPalette, 0)))
         || !(hbmOld = SelectObject(hDC, hbm)))
           goto Error;

        if (hPalette)
            RealizePalette(hDC);
        /* Draw the image... */
        BitBlt(hDC, 0, 0, cxImage, cyImage,
                hdcWork, rc.left, rc.top, SRCCOPY);
    } else
    {
        if (hPalette) {
           hOldPalette = SelectPalette(hDC, hPalette, 0);
           RealizePalette(hDC);
        }
        hbmOld = SelectObject(hDC, hBitmap);
    }

    /* Create the metafile */
    if (!(hmfpict = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, sizeof(METAFILEPICT)))
     || !(lpmfpict = (LPMETAFILEPICT)GlobalLock(hmfpict))
     || !(hdcMF = CreateMetaFile(NULL)))
        goto Error;
    /* Initialize the metafile */
    MSetWindowOrg(hdcMF, 0, 0);
    MSetWindowExt(hdcMF, cxImage, cyImage);

    if (hPalette)
    {
       SelectPalette(hdcMF, hPalette, TRUE);
       RealizePalette(hdcMF);
    }
    /* ... and then write it out to the metafile. */
    StretchBlt(hdcMF, 0, 0, cxImage, cyImage,
               hDC, 0, 0, cxImage, cyImage, SRCCOPY);

    if (hDefPalette = GetStockObject(DEFAULT_PALETTE))
    {
        SelectPalette(hdcMF, hDefPalette, TRUE);
        //RealizePalette(hdcMF);
    }

    /* Finish filling in the metafile header */
    lpmfpict->mm = MM_ANISOTROPIC;
    lpmfpict->xExt = PelsToLogHimetric(hdcWnd,TRUE,cxImage);
    lpmfpict->yExt = PelsToLogHimetric(hdcWnd,FALSE,cyImage);

    ReleaseDC(hwndFrame, hdcWnd);
    hdcWnd = NULL;

    lpmfpict->hMF  = CloseMetaFile(hdcMF);


    fError = FALSE;

Error:
    if (lpmfpict)
        GlobalUnlock(hmfpict);
    if (hbmOld)
        SelectObject(hDC, hbmOld);
    if (hbm)
        DeleteObject(hbm);
    if (hOldPalette)
    {
        SelectPalette(hDC, hOldPalette, 0);
        RealizePalette(hDC);
    }
    if (fCreatedDC && hDC)
        DeleteDC(hDC);
    if (hdcWnd)
        ReleaseDC(hwndFrame, hdcWnd);

    /* If we had an error, return NULL */
    if (fError && hmfpict) {
        GlobalFree(hmfpict);
        hmfpict = NULL;
    }

    return hmfpict;
}

/* Change menu item from Save to Update */
void FixMenus(void) {
    HMENU hMenu = GetMenu(pbrushWnd[PARENTid]);
    TCHAR szTemp[CBMENUITEMMAX];

    /* Fix the menu bar items */
    LoadString(hInst, IDS_UPDATE, szTemp, CharSizeOf(szTemp));
    ChangeMenuItem(FILEsave, FILEupdate, szTemp);

    vfIsLink = FALSE;
}

/* Change menu item from Update to Save
 * and "Exit & Return to xxx" to "Exit" */
void FAR UnfixMenus(void) {
    HMENU hMenu = GetMenu(pbrushWnd[PARENTid]);
    TCHAR szMenuStr[CBMENUITEMMAX];

    /* Fix the menu bar items */
    LoadString(hInst, IDS_SAVE, szMenuStr, CharSizeOf(szMenuStr));
    ChangeMenuItem(FILEupdate, FILEsave, szMenuStr);

    LoadString(hInst, IDSuntitled, noFile, CharSizeOf(noFile));

    LoadString(hInst, IDS_EXIT, szMenuStr, CharSizeOf(szMenuStr));
    ChangeMenuItem(FILEexit, FILEexit, szMenuStr);

    vfIsLink = TRUE;
}

void GetNum(LPTSTR FAR *lplpstrBuf, int FAR *lpint) {
    *lpint = 0;
    while (TEXT('0') <= **lplpstrBuf && **lplpstrBuf <= TEXT('9')) {
        *lpint = *lpint * 10 + (**lplpstrBuf - TEXT('0'));
        (*lplpstrBuf)++;
    }

    while (**lplpstrBuf && **lplpstrBuf == TEXT(' '))
        (*lplpstrBuf)++;
}

//int   WINAPI MultDiv(int,int,int);

#define nPelsPerLogInch GetDeviceCaps(hDC, bHoriz ? LOGPIXELSX : LOGPIXELSY)

int PelsPerLogMeter(HDC hDC,BOOL bHoriz)
{
    #define nMMPerMeterTimes10 10000
    #define nMMPerInchTimes10  254

    return MulDiv(nPelsPerLogInch, nMMPerMeterTimes10, nMMPerInchTimes10);
}

int PelsToLogHimetric(HDC hDC,BOOL bHoriz, int pels)
{
    #define nHMPerInch 2540

    return MulDiv(pels,nHMPerInch,nPelsPerLogInch);
}
