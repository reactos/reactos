//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1996.
//
//  File:       OLE2.h
//  Contents:   Main OLE2 header; Defines Linking and Emmebbeding interfaces, and API's.
//              Also includes .h files for the compobj, and oleauto  subcomponents.
//
//----------------------------------------------------------------------------
#if !defined( _OLE2_H_ )
#define _OLE2_H_

// Set packing to 8
#include <pshpack8.h>

// Make 100% sure WIN32 is defined
#ifndef WIN32
#define WIN32    100  // 100 == NT version 1.0
#endif


// SET to remove _export from interface definitions


#include <winerror.h>

#include <objbase.h>
#include <oleauto.h>

// View OBJECT Error Codes

#define E_DRAW                  VIEW_E_DRAW

// IDataObject Error Codes
#define DATA_E_FORMATETC        DV_E_FORMATETC





// Common stuff gleamed from OLE.2,

/* verbs */
#define OLEIVERB_PRIMARY            (0L)
#define OLEIVERB_SHOW               (-1L)
#define OLEIVERB_OPEN               (-2L)
#define OLEIVERB_HIDE               (-3L)
#define OLEIVERB_UIACTIVATE         (-4L)
#define OLEIVERB_INPLACEACTIVATE    (-5L)
#define OLEIVERB_DISCARDUNDOSTATE   (-6L)

// for OleCreateEmbeddingHelper flags; roles in low word; options in high word
#define EMBDHLP_INPROC_HANDLER   0x0000L
#define EMBDHLP_INPROC_SERVER    0x0001L
#define EMBDHLP_CREATENOW    0x00000000L
#define EMBDHLP_DELAYCREATE  0x00010000L

/* extended create function flags */
#define OLECREATE_LEAVERUNNING	0x00000001

/* pull in the MIDL generated header */

#include <oleidl.h>






/****** DV APIs ***********************************************************/


WINOLEAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder);







/****** OLE API Prototypes ************************************************/

WINOLEAPI_(DWORD) OleBuildVersion( VOID );

/* helper functions */
WINOLEAPI ReadClassStg(LPSTORAGE pStg, CLSID FAR* pclsid);
WINOLEAPI WriteClassStg(LPSTORAGE pStg, REFCLSID rclsid);
WINOLEAPI ReadClassStm(LPSTREAM pStm, CLSID FAR* pclsid);
WINOLEAPI WriteClassStm(LPSTREAM pStm, REFCLSID rclsid);
WINOLEAPI WriteFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType);
WINOLEAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT FAR* pcf, LPOLESTR FAR* lplpszUserType);


/* init/term */

WINOLEAPI OleInitialize(LPVOID pvReserved);
WINOLEAPI_(void) OleUninitialize(void);


/* APIs to query whether (Embedded/Linked) object can be created from
   the data object */

WINOLEAPI  OleQueryLinkFromData(LPDATAOBJECT pSrcDataObject);
WINOLEAPI  OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject);


/* Object creation APIs */

WINOLEAPI  OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateEx(REFCLSID rclsid, REFIID riid, DWORD dwFlags,
                DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
                LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
                DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromDataEx(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
                LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
                DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkFromDataEx(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
                LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
                DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);


WINOLEAPI  OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkEx(LPMONIKER pmkLinkSrc, REFIID riid,
            DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
            LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
            DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
            LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkToFileEx(LPCOLESTR lpszFileName, REFIID riid,
            DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
            LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
            DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
            LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromFileEx(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD* rgAdvf,
            LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink,
            DWORD FAR* rgdwConnection, LPOLECLIENTSITE pClientSite,
            LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite,
            LPVOID FAR* ppvObj);

WINOLEAPI  OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad);

WINOLEAPI  OleLoadFromStream( LPSTREAM pStm, REFIID iidInterface, LPVOID FAR* ppvObj);
WINOLEAPI  OleSaveToStream( LPPERSISTSTREAM pPStm, LPSTREAM pStm );


WINOLEAPI  OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained);
WINOLEAPI  OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible);


/* Drag/Drop APIs */

WINOLEAPI  RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget);
WINOLEAPI  RevokeDragDrop(HWND hwnd);
WINOLEAPI  DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource,
            DWORD dwOKEffects, LPDWORD pdwEffect);

/* Clipboard APIs */

WINOLEAPI  OleSetClipboard(LPDATAOBJECT pDataObj);
WINOLEAPI  OleGetClipboard(LPDATAOBJECT FAR* ppDataObj);
WINOLEAPI  OleFlushClipboard(void);
WINOLEAPI  OleIsCurrentClipboard(LPDATAOBJECT pDataObj);


/* InPlace Editing APIs */

WINOLEAPI_(HOLEMENU)   OleCreateMenuDescriptor (HMENU hmenuCombined,
                                LPOLEMENUGROUPWIDTHS lpMenuWidths);
WINOLEAPI              OleSetMenuDescriptor (HOLEMENU holemenu, HWND hwndFrame,
                                HWND hwndActiveObject,
                                LPOLEINPLACEFRAME lpFrame,
                                LPOLEINPLACEACTIVEOBJECT lpActiveObj);
WINOLEAPI              OleDestroyMenuDescriptor (HOLEMENU holemenu);

WINOLEAPI              OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                            LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg);


/* Helper APIs */
WINOLEAPI_(HANDLE) OleDuplicateData (HANDLE hSrc, CLIPFORMAT cfFormat,
                        UINT uiFlags);

WINOLEAPI          OleDraw (LPUNKNOWN pUnknown, DWORD dwAspect, HDC hdcDraw,
                    LPCRECT lprcBounds);

WINOLEAPI          OleRun(LPUNKNOWN pUnknown);
WINOLEAPI_(BOOL)   OleIsRunning(LPOLEOBJECT pObject);
WINOLEAPI          OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses);
WINOLEAPI_(void)   ReleaseStgMedium(LPSTGMEDIUM);
WINOLEAPI          CreateOleAdviseHolder(LPOLEADVISEHOLDER FAR* ppOAHolder);

WINOLEAPI          OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                    REFIID riid, LPVOID FAR* lplpObj);

WINOLEAPI          OleCreateEmbeddingHelper(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                    DWORD flags, LPCLASSFACTORY pCF,
                                        REFIID riid, LPVOID FAR* lplpObj);

WINOLEAPI_(BOOL)   IsAccelerator(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg,
                                        WORD FAR* lpwCmd);
/* Icon extraction Helper APIs */

WINOLEAPI_(HGLOBAL) OleGetIconOfFile(LPOLESTR lpszPath, BOOL fUseFileAsLabel);

WINOLEAPI_(HGLOBAL) OleGetIconOfClass(REFCLSID rclsid,     LPOLESTR lpszLabel,
                                        BOOL fUseTypeAsLabel);

WINOLEAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel,
                                        LPOLESTR lpszSourceFile, UINT iIconIndex);



/* Registration Database Helper APIs */

WINOLEAPI                  OleRegGetUserType (REFCLSID clsid, DWORD dwFormOfType,
                                        LPOLESTR FAR* pszUserType);

WINOLEAPI                  OleRegGetMiscStatus     (REFCLSID clsid, DWORD dwAspect,
                                        DWORD FAR* pdwStatus);

WINOLEAPI                  OleRegEnumFormatEtc     (REFCLSID clsid, DWORD dwDirection,
                                        LPENUMFORMATETC FAR* ppenum);

WINOLEAPI                  OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB FAR* ppenum);





/* OLE 1.0 conversion APIS */

/***** OLE 1.0 OLESTREAM declarations *************************************/

typedef struct _OLESTREAM FAR*  LPOLESTREAM;

typedef struct _OLESTREAMVTBL
{
    DWORD (CALLBACK* Get)(LPOLESTREAM, void FAR*, DWORD);
    DWORD (CALLBACK* Put)(LPOLESTREAM, const void FAR*, DWORD);
} OLESTREAMVTBL;
typedef  OLESTREAMVTBL FAR*  LPOLESTREAMVTBL;

typedef struct _OLESTREAM
{
    LPOLESTREAMVTBL lpstbl;
} OLESTREAM;


WINOLEAPI OleConvertOLESTREAMToIStorage
    (LPOLESTREAM                lpolestream,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd);

WINOLEAPI OleConvertIStorageToOLESTREAM
    (LPSTORAGE      pstg,
    LPOLESTREAM     lpolestream);


/* Storage Utility APIs */
WINOLEAPI GetHGlobalFromILockBytes (LPLOCKBYTES plkbyt, HGLOBAL FAR* phglobal);
WINOLEAPI CreateILockBytesOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                    LPLOCKBYTES FAR* pplkbyt);

WINOLEAPI GetHGlobalFromStream (LPSTREAM pstm, HGLOBAL FAR* phglobal);
WINOLEAPI CreateStreamOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                LPSTREAM FAR* ppstm);


/* ConvertTo APIS */

WINOLEAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew);
WINOLEAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew);
WINOLEAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew);
WINOLEAPI GetConvertStg(LPSTORAGE pStg);
WINOLEAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert);


WINOLEAPI OleConvertIStorageToOLESTREAMEx
    (LPSTORAGE          pstg,
                                    // Presentation data to OLESTREAM
     CLIPFORMAT         cfFormat,   //      format
     LONG               lWidth,     //      width
     LONG               lHeight,    //      height
     DWORD              dwSize,     //      size in bytes
     LPSTGMEDIUM        pmedium,    //      bits
     LPOLESTREAM        polestm);

WINOLEAPI OleConvertOLESTREAMToIStorageEx
    (LPOLESTREAM        polestm,
     LPSTORAGE          pstg,
                                    // Presentation data from OLESTREAM
     CLIPFORMAT FAR*    pcfFormat,  //      format
     LONG FAR*          plwWidth,   //      width
     LONG FAR*          plHeight,   //      height
     DWORD FAR*         pdwSize,    //      size in bytes
     LPSTGMEDIUM        pmedium);   //      bits

#ifndef RC_INVOKED
#include <poppack.h>
#endif // RC_INVOKED

#endif     // __OLE2_H__

