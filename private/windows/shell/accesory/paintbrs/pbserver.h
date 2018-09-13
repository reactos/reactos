/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
#define SERVERONLY

#ifdef STRICT
#undef STRICT
#define PB_STRICT
#endif

#include "ole.h"

#ifdef PB_STRICT
#define STRICT
#undef  PB_STRICT
#endif


#ifndef OLE_20
typedef CHAR    OLECHAR;
typedef LPSTR   POLESTR;

#else
typedef TCHAR    OLECHAR;
typedef LPTSTR   POLESTR;
#endif

/* Server definitions */
typedef  struct _PBSRVR {
    OLESERVER   olesrvr;
    HANDLE      hsrvr;          // handle to our server
    LHSERVER    lhsrvr;         // registration handle
} PBSRVR, FAR *PPBSRVR;

/* Document definitions */
typedef  struct _PBDOC {
    OLESERVERDOC oledoc;
    HANDLE      hdoc;          // handle to our document
    LHSERVERDOC  lhdoc;         // registration handle
    ATOM        aName;         // document name
    /* Handle to server? */
} PBDOC, FAR *PPBDOC;

typedef struct _ITEM {
    OLEOBJECT   oleobject;
    HANDLE      hitem;
    LPOLECLIENT lpoleclient;
    RECT        rc;             /* Subrectangle */
    int         ref;            /* # of references to document */
    ATOM        aName;
} ITEM, FAR *PITEM;

/* Clipboard formats */
extern WORD    vcfLink;
extern WORD    vcfOwnerLink;
extern WORD    vcfNative;

/* OLE document/object/server virtual tables */
extern OLESERVERDOCVTBL      vdocvtbl;
extern OLEOBJECTVTBL   vitemvtbl;
extern OLESERVERVTBL     vsrvrvtbl;

/* Are we running on an embedding or not? */
extern  BOOL    fServer;
extern  BOOL    vfIsLink;
extern  BOOL    fSendData;

/* Since Paint has only once instance, only one server/doc */
/* The items are just RECTs over the document, possibly overlapping */
#define CMAXITEMS       80
extern PPBSRVR  vpsrvr;
extern PPBDOC   vpdoc;
extern PITEM    vpitem[];
extern int      cItems;
extern BOOL     fServer;
extern BOOL     fOLE;
extern BOOL     fLoading;
extern  int     iExitWithSaving;
extern BOOL     fInvisible;                /* Are we /embedding, the first time? */
extern int      nCmdShowSaved;

/* What part of the image has been modified? */
extern RECT     vrcModified;

/* Function prototypes */
void FreeVTbls(void);
void InitVTbls(HINSTANCE hInst);
/************************ App Server Functions ************************/
BOOL InitServer(HINSTANCE hInst);
void DeleteServer(PPBSRVR psrvr);

/************************* SERVER VTBL FUNCTIONS **************************/
OLESTATUS FAR PASCAL SrvrOpen(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
                              POLESTR lpdocname, LPOLESERVERDOC FAR *lplpoledoc);
OLESTATUS FAR PASCAL SrvrCreate(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
                POLESTR lpclassname, POLESTR lpdocname, LPOLESERVERDOC FAR *lplpoledoc);
OLESTATUS FAR PASCAL SrvrCreateFromTemplate(LPOLESERVER lpolesrvr,
                LHSERVERDOC lhdoc, POLESTR lpclassname, POLESTR lpOLEdocname,
                POLESTR lpOLEtemplatename, LPOLESERVERDOC FAR *lplpoledoc);
OLESTATUS FAR PASCAL SrvrEdit(LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
        POLESTR lpclassname, POLESTR lpOLEdocname, LPOLESERVERDOC FAR *lplpoledoc);
OLESTATUS FAR PASCAL SrvrExit(LPOLESERVER lpolesrvr);
OLESTATUS FAR PASCAL SrvrRelease(LPOLESERVER lpolesrvr);

/********************* DOCUMENT FUNCTIONS ********************/
PPBDOC InitDoc(PPBSRVR psrvr, LHSERVERDOC lhdoc, LPTSTR lptitle);
void DeleteDoc(PPBDOC pdoc);
void ChangeDocName(PPBDOC FAR *ppdoc, LPTSTR lpname);

/********************** Document VTable Functions *********************/
OLESTATUS FAR PASCAL DocSave(LPOLESERVERDOC lpoledoc);
OLESTATUS FAR PASCAL DocClose(LPOLESERVERDOC lpoledoc);
OLESTATUS FAR PASCAL DocRelease(LPOLESERVERDOC lpoledoc);
OLESTATUS FAR PASCAL DocGetObject(LPOLESERVERDOC lpoledoc, POLESTR pitemname,
        LPOLEOBJECT FAR *lplpoleobject, LPOLECLIENT lpoleclient);
OLESTATUS FAR PASCAL DocSetHostNames(LPOLESERVERDOC lpoledoc,
                            POLESTR lpOLEclientName, POLESTR lpOLEdocName);
OLESTATUS FAR PASCAL DocSetDocDimensions(LPOLESERVERDOC lpoledoc, LPRECT lprc);
OLESTATUS FAR PASCAL DocSetColorScheme(LPOLESERVERDOC lpoledoc, LPLOGPALETTE lppal);

/**************************** ITEM SUBROUTINES ***************************/
void FAR CutCopyObjectFormats(HDC hDC, HBITMAP hBitmap, RECT rc, WORD msg);
PITEM CreateNewItem(PPBDOC pdoc);
BOOL SendDocChangeMsg(PPBDOC pdoc, WORD options);
BOOL SendItemChangeMsg(PITEM pitem, WORD options);

/********************* Item VTable Subroutines **************************/
OLESTATUS FAR PASCAL ItemOpen(LPOLEOBJECT lpoleobject);
OLESTATUS FAR PASCAL ItemDelete(LPOLEOBJECT lpoleobject);
OLESTATUS FAR PASCAL ItemGetData(LPOLEOBJECT lpoleobject,
                                 OLECLIPFORMAT cfFormat, LPHANDLE lphandle);
OLESTATUS FAR PASCAL ItemSetData(LPOLEOBJECT lpoleobject,
                                 OLECLIPFORMAT cfFormat, HANDLE hdata);
OLESTATUS FAR PASCAL ItemShow(LPOLEOBJECT lpoleobject, BOOL fActivate);
OLESTATUS FAR PASCAL ItemSetBounds(LPOLEOBJECT lpoleobject, LPRECT lprc);
OLESTATUS FAR PASCAL ItemSetTargetDevice(LPOLEOBJECT lpoleobject, HANDLE h);
OLECLIPFORMAT FAR PASCAL ItemEnumFormats(LPOLEOBJECT lpoleobject,
                                         OLECLIPFORMAT cfFormat);
LPVOID FAR PASCAL ItemQueryProtocol(LPOLEOBJECT lpoleobject, POLESTR lpprotocol);
OLESTATUS FAR PASCAL ItemSetColorScheme(LPOLEOBJECT lpoleobject, LPLOGPALETTE lppal);
OLESTATUS CALLBACK ItemDoVerb(LPOLEOBJECT lpoleobject,
                                UINT wVerb, BOOL fShow, BOOL fActivate);

/******************** APPLICATION SUPPLIED FUNCTIONS *********************/
PPBDOC CreateNewDoc(PPBSRVR psrvr, LHSERVERDOC lhdoc, LPTSTR lpstr);
PPBDOC CreateDocFromFile(PPBSRVR psrvr, LPTSTR lpstr, LHSERVERDOC lhdoc, LPTSTR lpstrDocName);
HANDLE GetNative(HDC hDC, HBITMAP hBItmap, RECT rc);
BOOL PutNative(PITEM pitem, HWND hWnd, HANDLE hdata);
int GetItemName(LPTSTR lpstr, int cbMax);

PITEM AddItem(PITEM pitem);
BOOL DeleteItem(PITEM pitem);
int FndItem(PITEM pitem);

void ScanRect(LPTSTR lpstr, LPRECT rc);
int OutRect(LPTSTR lpstr, int cb, RECT rc);
HANDLE GetLink(RECT rc);
HBITMAP GetBitmap(PITEM pitem);
HANDLE GetMF(HDC hDC, HBITMAP hBitmap, RECT rc);
void FAR UnfixMenus(void);
