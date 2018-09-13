/*
 * Initialization code for the Pbrush server
 */
#include <windows.h>
#include <port1632.h>
#include <shellapi.h>

#include "pbrush.h"
#include "pbserver.h"

extern OLESERVERVTBL vsrvrvtbl;
extern OLESERVERDOCVTBL vdocvtbl;
extern OLEOBJECTVTBL vitemvtbl;

void InitVTbls(HINSTANCE hInst) {

    /* Server virtual table */
    (FARPROC)(vsrvrvtbl.Open) = MakeProcInstance((FARPROC)SrvrOpen, hInst);
    (FARPROC)(vsrvrvtbl.Create) = MakeProcInstance((FARPROC)SrvrCreate, hInst);
    (FARPROC)(vsrvrvtbl.CreateFromTemplate) =
            MakeProcInstance((FARPROC)SrvrCreateFromTemplate, hInst);
    (FARPROC)(vsrvrvtbl.Edit) = MakeProcInstance((FARPROC)SrvrEdit, hInst);
    (FARPROC)(vsrvrvtbl.Exit) = MakeProcInstance((FARPROC)SrvrExit, hInst);
    (FARPROC)(vsrvrvtbl.Release) = MakeProcInstance((FARPROC)SrvrRelease, hInst);

    /* Document virtual table */
    (FARPROC)(vdocvtbl.Save) = MakeProcInstance((FARPROC)DocSave, hInst);
    (FARPROC)(vdocvtbl.Close) = MakeProcInstance((FARPROC)DocClose, hInst);
    (FARPROC)(vdocvtbl.GetObject) = MakeProcInstance((FARPROC)DocGetObject, hInst);
    (FARPROC)(vdocvtbl.Release) = MakeProcInstance((FARPROC)DocRelease, hInst);
    (FARPROC)(vdocvtbl.SetDocDimensions) =
            MakeProcInstance((FARPROC)DocSetDocDimensions, hInst);
    (FARPROC)(vdocvtbl.SetHostNames) =
            MakeProcInstance((FARPROC)DocSetHostNames, hInst);
    (FARPROC)(vdocvtbl.SetColorScheme) =
            MakeProcInstance((FARPROC)DocSetColorScheme, hInst);

    /* Item virtual table */
    (FARPROC)(vitemvtbl.QueryProtocol) =
            MakeProcInstance((FARPROC)ItemQueryProtocol, hInst);
    (FARPROC)(vitemvtbl.GetData) =
            MakeProcInstance((FARPROC)ItemGetData, hInst);
    (FARPROC)(vitemvtbl.SetData) =
            MakeProcInstance((FARPROC)ItemSetData, hInst);
    (FARPROC)(vitemvtbl.Release) =
            MakeProcInstance((FARPROC)ItemDelete, hInst);
    (FARPROC)(vitemvtbl.Show) = MakeProcInstance((FARPROC)ItemShow, hInst);
    (FARPROC)(vitemvtbl.SetBounds) = MakeProcInstance((FARPROC)ItemSetBounds, hInst);
    (FARPROC)(vitemvtbl.SetTargetDevice) =
            MakeProcInstance((FARPROC)ItemSetTargetDevice, hInst);
    (FARPROC)(vitemvtbl.EnumFormats) =
            MakeProcInstance((FARPROC)ItemEnumFormats, hInst);
    (FARPROC)(vitemvtbl.SetColorScheme) =
            MakeProcInstance((FARPROC)ItemSetColorScheme, hInst);
    (FARPROC)(vitemvtbl.DoVerb) = MakeProcInstance((FARPROC)ItemDoVerb, hInst);
}

extern WORD vcfLink;
extern WORD vcfOwnerLink;
extern WORD vcfNative;

extern HANDLE   hServer;
extern PPBSRVR  vpsrvr;

/************************ App Server Functions ************************/
BOOL InitServer(HINSTANCE hInst) {
    TCHAR szPBrushPicture[OBJSTRINGSMAX];
    OLESTATUS   olestat;
    int nRetries=0;
    TCHAR szEdit[20];
#ifndef OLE_20
    CHAR  szAnsi[APPNAMElen];
#endif


    /* Register the clipboard format atoms */
    /* Not in the stringtable because they're atom descriptors */
    if (!vcfLink) {
        vcfLink         = RegisterClipboardFormat(TEXT("ObjectLink"));
        vcfNative       = RegisterClipboardFormat(TEXT("Native"));
        vcfOwnerLink    = RegisterClipboardFormat(TEXT("OwnerLink"));
    }

    /* Allocate the server... */
    hServer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(PBSRVR));

    if (!hServer || !(vpsrvr = (PPBSRVR)GlobalLock(hServer)))
        goto errRtn;

    vpsrvr->hsrvr = hServer;
    vpsrvr->olesrvr.lpvtbl = &vsrvrvtbl;

    /* Register it... */
    LoadString(hInst, IDSpicture, szPBrushPicture, CharSizeOf(szPBrushPicture));
    LoadString(hInst, IDSEdit, szEdit, CharSizeOf(szEdit));
Retry:
#ifndef OLE_20
    WideCharToMultiByte (CP_ACP, 0, pgmName, -1, szAnsi, APPNAMElen, NULL, NULL);
    olestat = OleRegisterServer(szAnsi, (LPOLESERVER)vpsrvr,
                                (LONG FAR *)&vpsrvr->lhsrvr, hInst, OLE_SERVER_MULTI);
#else
    olestat = OleRegisterServer(pgmName, (LPOLESERVER)vpsrvr,
                                (LONG FAR *)&vpsrvr->lhsrvr, hInst, OLE_SERVER_MULTI);
#endif

    if (olestat != OLE_OK && olestat != OLE_WAIT_FOR_RELEASE)
      {
        HKEY hKeyPBrush, hKeyEdit;

        /* if we have already retried, error out. */
        if (nRetries)
            goto errRtn;
        nRetries++;

        RegSetValue(HKEY_CLASSES_ROOT, TEXT(".bmp"), REG_SZ, TEXT("PBrush"),6);

        if (RegCreateKey(HKEY_CLASSES_ROOT, TEXT("PBrush"), &hKeyPBrush)
              == ERROR_SUCCESS)
          {
            RegSetValue(hKeyPBrush, NULL, REG_SZ, szPBrushPicture, 0);

            if (RegCreateKey(hKeyPBrush, TEXT("protocol\\StdFileEditing"), &hKeyEdit)
                  == ERROR_SUCCESS)
              {
                RegSetValue(hKeyEdit, TEXT("server"), REG_SZ, TEXT("pbrush.exe"), 0);
                RegSetValue(hKeyEdit, TEXT("verb\\0"), REG_SZ, szEdit, 0);
                RegCloseKey(hKeyEdit);
              }
            RegCloseKey(hKeyPBrush);
          }

        goto Retry;
      }

    return TRUE;

errRtn:
//    PbrushOkError(E_FAILED_TO_REGISTER_SERVER, MB_ICONHAND); BUGBUGBUG

    /* If we failed, clean up */
    if (vpsrvr) {
        GlobalUnlock(hServer);
        vpsrvr = NULL;
    }

    if (hServer) {
        GlobalFree(hServer);
        hServer = NULL;
    }

    return FALSE;
}

/********************* DOCUMENT FUNCTIONS ********************/
PPBDOC InitDoc(PPBSRVR psrvr, LHSERVERDOC lhdoc, LPTSTR lptitle) {
    HANDLE      hdoc = NULL;
    PPBDOC      pdoc = NULL;
#ifndef OLE_20
    CHAR        szAnsi[FILENAMElen + PATHlen];
#endif

    hdoc = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(PBDOC));

    if (!hdoc || !(pdoc = (PPBDOC)GlobalLock(hdoc)))
        goto errRtn;

    pdoc->hdoc = hdoc;
    pdoc->oledoc.lpvtbl = &vdocvtbl;

    if (!lhdoc) {
#ifndef OLE_20
        WideCharToMultiByte (CP_ACP, 0, lptitle, -1, szAnsi, FILENAMElen + PATHlen, NULL, NULL);
        if (OleRegisterServerDoc(psrvr->lhsrvr, szAnsi, (LPOLESERVERDOC)pdoc,
            (LHSERVERDOC FAR *)&pdoc->lhdoc) != OLE_OK)
#else
        if (OleRegisterServerDoc(psrvr->lhsrvr, lptitle, (LPOLESERVERDOC)pdoc,
            (LHSERVERDOC FAR *)&pdoc->lhdoc) != OLE_OK)
#endif
            goto errRtn;
    } else
        pdoc->lhdoc = lhdoc;

    pdoc->aName = GlobalAddAtom(lptitle);

    return pdoc;

errRtn:
    PbrushOkError(E_FAILED_TO_REGISTER_DOCUMENT, MB_ICONHAND);

    /* Clean up */
    if (pdoc)
        GlobalUnlock(hdoc);

    if (hdoc)
        GlobalFree(hdoc);

    return NULL;
}

