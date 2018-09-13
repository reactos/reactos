/*****************************************************************************\
*                                                                             *
* ole2.h  - main ole2 header; includes all subcomponents		      *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

#if !defined( _OLE2_H_ )
#define _OLE2_H_

#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif  /* RC_INVOKED */

#include <string.h>

/****** Standard Object Definitions *****************************************/

#include <compobj.h>


// *************** FACILITY_ITF scodes common to all interfaces ************
//
// By convention, OLE interfaces divide the FACILITY_ITF range of errors
// into nonoverlapping subranges.  If an interface returns a FACILITY_ITF
// scode, it must be from the range associated with that interface or from
// the shared range: OLE_E_FIRST...OLE_E_LAST.
//

// error codes

#define OLE_E_OLEVERB               (OLE_E_FIRST)
// invalid OLEVERB structure

#define OLE_E_ADVF                  (OLE_E_FIRST+1)
// invalid advise flags

#define OLE_E_ENUM_NOMORE           (OLE_E_FIRST+2)
// you can't enuemrate any more, because the associated data is missing

#define OLE_E_ADVISENOTSUPPORTED    (OLE_E_FIRST+3)
// this implementation doesn't take advises

#define OLE_E_NOCONNECTION          (OLE_E_FIRST+4)
// there is no connection for this connection id

#define OLE_E_NOTRUNNING            (OLE_E_FIRST+5)
// need run the object to perform this operation

#define OLE_E_NOCACHE               (OLE_E_FIRST+6)
// there is no cache to operate on

#define OLE_E_BLANK                 (OLE_E_FIRST+7)
// Uninitialized object

#define OLE_E_CLASSDIFF             (OLE_E_FIRST+8)
// linked object's source class has changed

#define OLE_E_CANT_GETMONIKER       (OLE_E_FIRST+9)
// not able to get the moniker of the object

#define OLE_E_CANT_BINDTOSOURCE     (OLE_E_FIRST+10)
// not able to bind to the source

#define OLE_E_STATIC                (OLE_E_FIRST+11)
// object is static, operation not allowed

#define OLE_E_PROMPTSAVECANCELLED   (OLE_E_FIRST+12)
// user cancelled out of save dialog

#define OLE_E_INVALIDRECT           (OLE_E_FIRST+13)
// invalid rectangle

#define OLE_E_WRONGCOMPOBJ          (OLE_E_FIRST+14)
// compobj.dll is too old for the ole2.dll initialized

#define OLE_E_INVALIDHWND           (OLE_E_FIRST+15)
// invalid window handle

#define OLE_E_NOT_INPLACEACTIVE     (OLE_E_FIRST+16)
// object is not in any of the inplace active states

#define DVGEN_E_FIRST               (OLE_E_FIRST+100)

#define DV_E_FORMATETC              (DVGEN_E_FIRST)
// invalid FORMATETC structure

#define DV_E_DVTARGETDEVICE         (DVGEN_E_FIRST+1)
// invalid DVTARGETDEVICE structure

#define DV_E_STGMEDIUM              (DVGEN_E_FIRST+2)
// invalid STDGMEDIUM structure

#define DV_E_STATDATA               (DVGEN_E_FIRST+3)
// invalid STATDATA structure

#define DV_E_LINDEX                 (DVGEN_E_FIRST+4)
// invalid lindex

#define DV_E_TYMED                  (DVGEN_E_FIRST+5)
// invalid tymed

#define DV_E_CLIPFORMAT             (DVGEN_E_FIRST+6)
// invalid clipboard format

#define DV_E_DVASPECT               (DVGEN_E_FIRST+7)
// invalid aspect(s)

#define DV_E_DVTARGETDEVICE_SIZE    (DVGEN_E_FIRST+8)
// tdSize paramter of the DVTARGETDEVICE structure is invalid

#define DV_E_NOIVIEWOBJECT          (DVGEN_E_FIRST+9)
// object doesn't support IViewObject interface


// Success codes

#define OLE_S_USEREG                (OLE_S_FIRST)
// use the reg database to provide the requested info

#define OLE_S_STATIC                (OLE_S_FIRST+1)
// success, but static

#define OLE_S_MAC_CLIPFORMAT        (OLE_S_FIRST+2)
// macintosh clipboard format

//*************************** Interface or API specific scodes *************

// Errors for OleConvertOLESTREAMToIStorage and OleConvertIStorageToOLESTREAM

// OLESTREAM Get method failed
#define CONVERT10_E_OLESTREAM_GET       (CONVERT10_E_FIRST + 0)

// OLESTREAM Put method failed
#define CONVERT10_E_OLESTREAM_PUT       (CONVERT10_E_FIRST + 1)

// Contents of the OLESTREAM not in correct format
#define CONVERT10_E_OLESTREAM_FMT       (CONVERT10_E_FIRST + 2)

// There was in an error in a Windows GDI call while converting the bitmap
// to a DIB.
#define CONVERT10_E_OLESTREAM_BITMAP_TO_DIB (CONVERT10_E_FIRST + 3)

// Contents of the IStorage not in correct format
#define CONVERT10_E_STG_FMT             (CONVERT10_E_FIRST + 4)

// Contents of IStorage is missing one of the standard streams ("\1CompObj",
// "\1Ole", "\2OlePres000").  This may be the storage for a DLL object, or a
// class that does not use the def handler.
#define CONVERT10_E_STG_NO_STD_STREAM   (CONVERT10_E_FIRST + 5)

// There was in an error in a Windows GDI call while converting the DIB
// to a bitmap.
#define CONVERT10_E_STG_DIB_TO_BITMAP   (CONVERT10_E_FIRST + 6)


// Returned by either API, this scode indicates that the original object
//  had no presentation, therefore the converted object does not either.
#define CONVERT10_S_NO_PRESENTATION     (CONVERT10_S_FIRST + 0)


// Errors for Clipboard functions

// OpenClipboard Failed
#define CLIPBRD_E_CANT_OPEN     (CLIPBRD_E_FIRST + 0)

// EmptyClipboard Failed
#define CLIPBRD_E_CANT_EMPTY        (CLIPBRD_E_FIRST + 1)

// SetClipboard Failed
#define CLIPBRD_E_CANT_SET          (CLIPBRD_E_FIRST + 2)

// Data on clipboard is invalid
#define CLIPBRD_E_BAD_DATA          (CLIPBRD_E_FIRST + 3)

// CloseClipboard Failed
#define CLIPBRD_E_CANT_CLOSE        (CLIPBRD_E_FIRST + 4)


/****** OLE value types *****************************************************/

/* rendering options */
typedef enum tagOLERENDER
{
    OLERENDER_NONE   = 0,
    OLERENDER_DRAW   = 1,
    OLERENDER_FORMAT = 2,
    OLERENDER_ASIS   = 3
} OLERENDER;
typedef  OLERENDER FAR* LPOLERENDER;

// OLE verb; returned by IEnumOLEVERB
typedef struct FARSTRUCT tagOLEVERB
{
    LONG    lVerb;
    LPSTR   lpszVerbName;
    DWORD   fuFlags;
    DWORD grfAttribs;
} OLEVERB, FAR* LPOLEVERB;


// Bitwise verb attributes used in OLEVERB.grfAttribs
typedef enum tagOLEVERBATTRIB // bitwise
{
    OLEVERBATTRIB_NEVERDIRTIES = 1,
    OLEVERBATTRIB_ONCONTAINERMENU = 2
} OLEVERBATTRIB;


// IOleObject::GetUserType optons; determines which form of the string to use
typedef enum tagUSERCLASSTYPE
{
    USERCLASSTYPE_FULL = 1,
    USERCLASSTYPE_SHORT= 2,
    USERCLASSTYPE_APPNAME= 3,
} USERCLASSTYPE;


// bits returned from IOleObject::GetMistStatus
typedef enum tagOLEMISC // bitwise
{
    OLEMISC_RECOMPOSEONRESIZE   = 1,
    OLEMISC_ONLYICONIC          = 2,
    OLEMISC_INSERTNOTREPLACE    = 4,
    OLEMISC_STATIC              = 8,
    OLEMISC_CANTLINKINSIDE      = 16,
    OLEMISC_CANLINKBYOLE1       = 32,
    OLEMISC_ISLINKOBJECT        = 64,
    OLEMISC_INSIDEOUT           = 128,
    OLEMISC_ACTIVATEWHENVISIBLE = 256
} OLEMISC;


// IOleObject::Close options
typedef enum tagOLECLOSE
{
    OLECLOSE_SAVEIFDIRTY = 0,
    OLECLOSE_NOSAVE      = 1,
    OLECLOSE_PROMPTSAVE  = 2
} OLECLOSE;


// IOleObject::GetMoniker and IOleClientSite::GetMoniker options; determines
// if and how monikers should be assigned.
typedef enum tagOLEGETMONIKER
{
    OLEGETMONIKER_ONLYIFTHERE=1,
    OLEGETMONIKER_FORCEASSIGN=2,
    OLEGETMONIKER_UNASSIGN=3,
    OLEGETMONIKER_TEMPFORUSER=4
} OLEGETMONIKER;


// IOleObject::GetMoniker, IOleObject::SetMoniker and
// IOleClientSite::GetMoniker options; determines which moniker to use
typedef enum tagOLEWHICHMK
{
    OLEWHICHMK_CONTAINER=1,
    OLEWHICHMK_OBJREL=2,
    OLEWHICHMK_OBJFULL=3
} OLEWHICHMK;


#ifdef WIN32
#define LPSIZEL PSIZEL
#elif (WINVER < 0x0400)
typedef struct FARSTRUCT tagSIZEL
{
    long cx;
    long cy;
} SIZEL, FAR* LPSIZEL;
#endif


#ifdef WIN32
#define LPRECTL PRECTL
#elif (WINVER < 0x0400)
typedef struct FARSTRUCT tagRECTL
{
    long    left;
    long    top;
    long    right;
    long    bottom;
} RECTL, FAR* LPRECTL;

typedef struct FARSTRUCT tagPOINTL {
    LONG x;
    LONG y;
} POINTL;

#endif


#ifndef LPCRECT
typedef const RECT FAR* LPCRECT;
#endif

#ifndef LPCRECTL
typedef const RECTL FAR* LPCRECTL;
#endif


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


/****** Clipboard Data structures *****************************************/

typedef struct tagOBJECTDESCRIPTOR
{
   ULONG    cbSize;              // Size of structure in bytes
   CLSID    clsid;               // CLSID of data being transferred
   DWORD    dwDrawAspect;        // Display aspect of the object
                                 //     normally DVASPECT_CONTENT or ICON.
                                 //     dwDrawAspect will be 0 (which is NOT
                                 //     DVASPECT_CONTENT) if the copier or
                                 //     dragsource didn't draw the object to
                                 //     begin with.
   SIZEL    sizel;               // size of the object in HIMETRIC
                                 //    sizel is opt.: will be (0,0) for apps
                                 //    which don't draw the object being
                                 //    transferred
   POINTL   pointl;              // Offset in HIMETRIC units from the
                                 //    upper-left corner of the obj where the
                                 //    mouse went down for the drag.
                                 //    NOTE: y coordinates increase downward.
                                 //          x coordinates increase to right
                                 //    pointl is opt.; it is only meaningful
                                 //    if object is transfered via drag/drop.
                                 //    (0, 0) if mouse position is unspecified
                                 //    (eg. when obj transfered via clipboard)
   DWORD    dwStatus;            // Misc. status flags for object. Flags are
                                 //    defined by OLEMISC enum. these flags
                                 //    are as would be returned
                                 //    by IOleObject::GetMiscStatus.
   DWORD    dwFullUserTypeName;  // Offset from beginning of structure to
                                 //    null-terminated string that specifies
                                 //    Full User Type Name of the object.
                                 //    0 indicates string not present.
   DWORD    dwSrcOfCopy;         // Offset from beginning of structure to
                                 //    null-terminated string that specifies
                                 //    source of the transfer.
                                 //    dwSrcOfCOpy is normally implemented as
                                 //    the display name of the temp-for-user
                                 //    moniker which identifies the source of
                                 //    the data.
                                 //    0 indicates string not present.
                                 //    NOTE: moniker assignment is NOT forced.
                                 //    see IOleObject::GetMoniker(
                                 //                OLEGETMONIKER_TEMPFORUSER)

 /* variable sized string data may appear here */

} OBJECTDESCRIPTOR,  *POBJECTDESCRIPTOR,  FAR *LPOBJECTDESCRIPTOR,
  LINKSRCDESCRIPTOR, *PLINKSRCDESCRIPTOR, FAR *LPLINKSRCDESCRIPTOR;



/* verbs */
#define OLEIVERB_PRIMARY            (0L)
#define OLEIVERB_SHOW               (-1L)
#define OLEIVERB_OPEN               (-2L)
#define OLEIVERB_HIDE               (-3L)
#define OLEIVERB_UIACTIVATE         (-4L)
#define OLEIVERB_INPLACEACTIVATE    (-5L)
#define OLEIVERB_DISCARDUNDOSTATE   (-6L)


//      forward type declarations
#if defined(__cplusplus)
interface IOleClientSite;
interface IOleContainer;
interface IOleObject;
#else
typedef interface IOleClientSite IOleClientSite;
typedef interface IOleContainer IOleContainer;
typedef interface IOleObject IOleObject;
#endif

typedef         IOleObject FAR* LPOLEOBJECT;
typedef     IOleClientSite FAR* LPOLECLIENTSITE;
typedef       IOleContainer FAR* LPOLECONTAINER;


/****** OLE GUIDs *********************************************************/

#include "oleguid.h"


/****** Other Major Interfaces ********************************************/

#include <dvobj.h>

#include <storage.h>



/****** IDrop??? Interfaces ********************************************/

#define MK_ALT 0x0020


#define DROPEFFECT_NONE     0
#define DROPEFFECT_COPY     1
#define DROPEFFECT_MOVE     2
#define DROPEFFECT_LINK     4
#define DROPEFFECT_SCROLL   0x80000000

// default inset-width of the hot zone, in pixels
#define DD_DEFSCROLLINSET 11

// default delay before scrolling, in milliseconds
#define DD_DEFSCROLLDELAY 50


/* Dragdrop specific error codes */

#define DRAGDROP_E_NOTREGISTERED        (DRAGDROP_E_FIRST)
// trying to revoke a drop target that has not been registered

#define DRAGDROP_E_ALREADYREGISTERED    (DRAGDROP_E_FIRST+1)
// this window has already been registered as a drop target

#define DRAGDROP_E_INVALIDHWND          (DRAGDROP_E_FIRST+2)
// invalid HWND


#define DRAGDROP_S_DROP                 (DRAGDROP_S_FIRST + 0)
// successful drop took place

#define DRAGDROP_S_CANCEL               (DRAGDROP_S_FIRST + 1)
// drag-drop operation canceled

#define DRAGDROP_S_USEDEFAULTCURSORS    (DRAGDROP_S_FIRST + 2)
// use the default cursor


#undef INTERFACE
#define INTERFACE   IDropTarget

DECLARE_INTERFACE_(IDropTarget, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter) (THIS_ LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) PURE;
    STDMETHOD(DragOver) (THIS_ DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) PURE;
    STDMETHOD(DragLeave) (THIS) PURE;
    STDMETHOD(Drop) (THIS_ LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) PURE;
};
typedef         IDropTarget FAR* LPDROPTARGET;



#undef INTERFACE
#define INTERFACE   IDropSource

DECLARE_INTERFACE_(IDropSource, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDropSource methods ***
    STDMETHOD(QueryContinueDrag) (THIS_ BOOL fEscapePressed, DWORD grfKeyState) PURE;
    STDMETHOD(GiveFeedback) (THIS_ DWORD dwEffect) PURE;
};
typedef         IDropSource FAR* LPDROPSOURCE;



/****** IPersist??? Interfaces ********************************************/


#undef INTERFACE
#define INTERFACE   IPersist

DECLARE_INTERFACE_(IPersist, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;
};
typedef            IPersist FAR* LPPERSIST;



#undef INTERFACE
#define INTERFACE   IPersistStorage

DECLARE_INTERFACE_(IPersistStorage, IPersist)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;

    // *** IPersistStorage methods ***
    STDMETHOD(IsDirty) (THIS) PURE;
    STDMETHOD(InitNew) (THIS_ LPSTORAGE pStg) PURE;
    STDMETHOD(Load) (THIS_ LPSTORAGE pStg) PURE;
    STDMETHOD(Save) (THIS_ LPSTORAGE pStgSave, BOOL fSameAsLoad) PURE;
    STDMETHOD(SaveCompleted) (THIS_ LPSTORAGE pStgNew) PURE;
    STDMETHOD(HandsOffStorage) (THIS) PURE;
};
typedef         IPersistStorage FAR* LPPERSISTSTORAGE;



#undef INTERFACE
#define INTERFACE   IPersistStream

DECLARE_INTERFACE_(IPersistStream, IPersist)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty) (THIS) PURE;
    STDMETHOD(Load) (THIS_ LPSTREAM pStm) PURE;
    STDMETHOD(Save) (THIS_ LPSTREAM pStm,
                    BOOL fClearDirty) PURE;
    STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize) PURE;
};
typedef          IPersistStream FAR* LPPERSISTSTREAM;



#undef INTERFACE
#define INTERFACE   IPersistFile

DECLARE_INTERFACE_(IPersistFile, IPersist)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;

    // *** IPersistFile methods ***
    STDMETHOD(IsDirty) (THIS) PURE;
    STDMETHOD(Load) (THIS_ LPCSTR lpszFileName, DWORD grfMode) PURE;
    STDMETHOD(Save) (THIS_ LPCSTR lpszFileName, BOOL fRemember) PURE;
    STDMETHOD(SaveCompleted) (THIS_ LPCSTR lpszFileName) PURE;
    STDMETHOD(GetCurFile) (THIS_ LPSTR FAR * lplpszFileName) PURE;
};
typedef            IPersistFile FAR* LPPERSISTFILE;


/****** Moniker Object Interfaces ******************************************/

#include <moniker.h>


/****** OLE Object Interfaces ******************************************/


#undef  INTERFACE
#define INTERFACE   IEnumOLEVERB

DECLARE_INTERFACE_(IEnumOLEVERB, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumOLEVERB methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, LPOLEVERB rgelt, ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumOLEVERB FAR* FAR* ppenm) PURE;
};
typedef         IEnumOLEVERB FAR* LPENUMOLEVERB;




#undef  INTERFACE
#define INTERFACE   IOleObject

#define OLEOBJ_E_NOVERBS                (OLEOBJ_E_FIRST + 0)

#define OLEOBJ_E_INVALIDVERB            (OLEOBJ_E_FIRST + 1)

#define OLEOBJ_S_INVALIDVERB            (OLEOBJ_S_FIRST + 0)

#define OLEOBJ_S_CANNOT_DOVERB_NOW      (OLEOBJ_S_FIRST + 1)
// verb number is valid but verb cannot be done now, for instance
// hiding a link or hiding a visible OLE 1.0 server

#define OLEOBJ_S_INVALIDHWND            (OLEOBJ_S_FIRST + 2)
// invalid hwnd passed


DECLARE_INTERFACE_(IOleObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleObject methods ***
    STDMETHOD(SetClientSite) (THIS_ LPOLECLIENTSITE pClientSite) PURE;
    STDMETHOD(GetClientSite) (THIS_ LPOLECLIENTSITE FAR* ppClientSite) PURE;
    STDMETHOD(SetHostNames) (THIS_ LPCSTR szContainerApp, LPCSTR szContainerObj) PURE;
    STDMETHOD(Close) (THIS_ DWORD dwSaveOption) PURE;
    STDMETHOD(SetMoniker) (THIS_ DWORD dwWhichMoniker, LPMONIKER pmk) PURE;
    STDMETHOD(GetMoniker) (THIS_ DWORD dwAssign, DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk) PURE;
    STDMETHOD(InitFromData) (THIS_ LPDATAOBJECT pDataObject,
                BOOL fCreation,
                DWORD dwReserved) PURE;
    STDMETHOD(GetClipboardData) (THIS_ DWORD dwReserved,
                LPDATAOBJECT FAR* ppDataObject) PURE;
    STDMETHOD(DoVerb) (THIS_ LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect) PURE;
    STDMETHOD(EnumVerbs) (THIS_ LPENUMOLEVERB FAR* ppenumOleVerb) PURE;
    STDMETHOD(Update) (THIS) PURE;
    STDMETHOD(IsUpToDate) (THIS) PURE;
    STDMETHOD(GetUserClassID) (THIS_ CLSID FAR* pClsid) PURE;
    STDMETHOD(GetUserType) (THIS_ DWORD dwFormOfType, LPSTR FAR* pszUserType) PURE;
    STDMETHOD(SetExtent) (THIS_ DWORD dwDrawAspect, LPSIZEL lpsizel) PURE;
    STDMETHOD(GetExtent) (THIS_ DWORD dwDrawAspect, LPSIZEL lpsizel) PURE;

    STDMETHOD(Advise)(THIS_ LPADVISESINK pAdvSink, DWORD FAR* pdwConnection) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE;
    STDMETHOD(EnumAdvise) (THIS_ LPENUMSTATDATA FAR* ppenumAdvise) PURE;
    STDMETHOD(GetMiscStatus) (THIS_ DWORD dwAspect, DWORD FAR* pdwStatus) PURE;
    STDMETHOD(SetColorScheme) (THIS_ LPLOGPALETTE lpLogpal) PURE;
};
typedef      IOleObject FAR* LPOLEOBJECT;



#undef  INTERFACE
#define INTERFACE   IOleClientSite

DECLARE_INTERFACE_(IOleClientSite, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleClientSite methods ***
    STDMETHOD(SaveObject) (THIS) PURE;
    STDMETHOD(GetMoniker) (THIS_ DWORD dwAssign, DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk) PURE;
    STDMETHOD(GetContainer) (THIS_ LPOLECONTAINER FAR* ppContainer) PURE;
    STDMETHOD(ShowObject) (THIS) PURE;
    STDMETHOD(OnShowWindow) (THIS_ BOOL fShow) PURE;
    STDMETHOD(RequestNewObjectLayout) (THIS) PURE;
};
typedef      IOleClientSite FAR* LPOLECLIENTSITE;



#undef  INTERFACE
#define INTERFACE   IParseDisplayName

DECLARE_INTERFACE_(IParseDisplayName, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IParseDisplayName method ***
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPSTR lpszDisplayName,
        ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut) PURE;
};
typedef       IParseDisplayName FAR* LPPARSEDISPLAYNAME;


#undef  INTERFACE
#define INTERFACE   IOleContainer

DECLARE_INTERFACE_(IOleContainer, IParseDisplayName)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IParseDisplayName method ***
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPSTR lpszDisplayName,
        ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut) PURE;

    // *** IOleContainer methods ***
    STDMETHOD(EnumObjects) ( DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown) PURE;
    STDMETHOD(LockContainer) (THIS_ BOOL fLock) PURE;
};
typedef IOleContainer FAR* LPOLECONTAINER;


typedef enum tagBINDSPEED
{
    BINDSPEED_INDEFINITE    = 1,
    BINDSPEED_MODERATE      = 2,
    BINDSPEED_IMMEDIATE     = 3
} BINDSPEED;

typedef enum tagOLECONTF
{
    OLECONTF_EMBEDDINGS     =  1,
    OLECONTF_LINKS          =  2,
    OLECONTF_OTHERS         =  4,
    OLECONTF_ONLYUSER       =  8,
    OLECONTF_ONLYIFRUNNING  = 16
} OLECONTF;


#undef  INTERFACE
#define INTERFACE   IOleItemContainer

DECLARE_INTERFACE_(IOleItemContainer, IOleContainer)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IParseDisplayName method ***
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPSTR lpszDisplayName,
        ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut) PURE;

    // *** IOleContainer methods ***
    STDMETHOD(EnumObjects) (THIS_ DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown) PURE;
    STDMETHOD(LockContainer) (THIS_ BOOL fLock) PURE;

    // *** IOleItemContainer methods ***
    STDMETHOD(GetObject) (THIS_ LPSTR lpszItem, DWORD dwSpeedNeeded,
        LPBINDCTX pbc, REFIID riid, LPVOID FAR* ppvObject) PURE;
    STDMETHOD(GetObjectStorage) (THIS_ LPSTR lpszItem, LPBINDCTX pbc,
        REFIID riid, LPVOID FAR* ppvStorage) PURE;
    STDMETHOD(IsRunning) (THIS_ LPSTR lpszItem) PURE;
};
typedef       IOleItemContainer FAR* LPOLEITEMCONTAINER;



#undef  INTERFACE
#define INTERFACE   IOleAdviseHolder

DECLARE_INTERFACE_(IOleAdviseHolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleAdviseHolder methods ***
    STDMETHOD(Advise)(THIS_ LPADVISESINK pAdvise, DWORD FAR* pdwConnection) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE;
    STDMETHOD(EnumAdvise)(THIS_ LPENUMSTATDATA FAR* ppenumAdvise) PURE;

    STDMETHOD(SendOnRename)(THIS_ LPMONIKER pmk) PURE;
    STDMETHOD(SendOnSave)(THIS) PURE;
    STDMETHOD(SendOnClose)(THIS) PURE;
};
typedef      IOleAdviseHolder FAR* LPOLEADVISEHOLDER;


/****** OLE Link Interface ************************************************/

/* Link update options */
typedef enum tagOLEUPDATE
{
    OLEUPDATE_ALWAYS=1,
    OLEUPDATE_ONCALL=3
} OLEUPDATE;
typedef  OLEUPDATE FAR* LPOLEUPDATE;


// for IOleLink::BindToSource
typedef enum tagOLELINKBIND
{
    OLELINKBIND_EVENIFCLASSDIFF = 1,
} OLELINKBIND;


#undef  INTERFACE
#define INTERFACE   IOleLink

DECLARE_INTERFACE_(IOleLink, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleLink methods ***
    STDMETHOD(SetUpdateOptions) (THIS_ DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetUpdateOptions) (THIS_ LPDWORD pdwUpdateOpt) PURE;
    STDMETHOD(SetSourceMoniker) (THIS_ LPMONIKER pmk, REFCLSID rclsid) PURE;
    STDMETHOD(GetSourceMoniker) (THIS_ LPMONIKER FAR* ppmk) PURE;
    STDMETHOD(SetSourceDisplayName) (THIS_ LPCSTR lpszDisplayName) PURE;
    STDMETHOD(GetSourceDisplayName) (THIS_ LPSTR FAR* lplpszDisplayName) PURE;
    STDMETHOD(BindToSource) (THIS_ DWORD bindflags, LPBINDCTX pbc) PURE;
    STDMETHOD(BindIfRunning) (THIS) PURE;
    STDMETHOD(GetBoundSource) (THIS_ LPUNKNOWN FAR* ppUnk) PURE;
    STDMETHOD(UnbindSource) (THIS) PURE;
    STDMETHOD(Update) (THIS_ LPBINDCTX pbc) PURE;
};
typedef         IOleLink FAR* LPOLELINK;


/****** OLE InPlace Editing Interfaces ************************************/

#ifdef _MAC
typedef Handle  HOLEMENU;
typedef long    SIZE;
typedef long    HACCEL;
#else
DECLARE_HANDLE(HOLEMENU);
#endif

typedef struct FARSTRUCT tagOIFI          // OleInPlaceFrameInfo
{
    UINT    cb;
    BOOL    fMDIApp;
    HWND    hwndFrame;
    HACCEL  haccel;
    int     cAccelEntries;
} OLEINPLACEFRAMEINFO, FAR* LPOLEINPLACEFRAMEINFO;


typedef struct FARSTRUCT tagOleMenuGroupWidths
{
    LONG    width[6];
} OLEMENUGROUPWIDTHS, FAR* LPOLEMENUGROUPWIDTHS;

typedef RECT    BORDERWIDTHS;
typedef LPRECT  LPBORDERWIDTHS;
typedef LPCRECT LPCBORDERWIDTHS;

/* Inplace editing specific error codes */

#define INPLACE_E_NOTUNDOABLE   (INPLACE_E_FIRST)
// undo is not avaiable

#define INPLACE_E_NOTOOLSPACE       (INPLACE_E_FIRST+1)
// Space for tools is not available

#define INPLACE_S_TRUNCATED     (INPLACE_S_FIRST)
// Message is too long, some of it had to be truncated before displaying


//      forward type declarations
#if defined(__cplusplus)
interface IOleInPlaceUIWindow;
#else
typedef interface IOleInPlaceUIWindow IOleInPlaceUIWindow;
#endif

typedef     IOleInPlaceUIWindow FAR* LPOLEINPLACEUIWINDOW;


#undef  INTERFACE
#define INTERFACE   IOleWindow

DECLARE_INTERFACE_(IOleWindow, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;
};

typedef         IOleWindow FAR* LPOLEWINDOW;



#undef  INTERFACE
#define INTERFACE   IOleInPlaceObject

DECLARE_INTERFACE_(IOleInPlaceObject, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IOleInPlaceObject methods ***
    STDMETHOD(InPlaceDeactivate) (THIS) PURE;
    STDMETHOD(UIDeactivate) (THIS) PURE;
    STDMETHOD(SetObjectRects) (THIS_ LPCRECT lprcPosRect,
                    LPCRECT lprcClipRect) PURE;
    STDMETHOD(ReactivateAndUndo) (THIS) PURE;
};
typedef         IOleInPlaceObject FAR* LPOLEINPLACEOBJECT;



#undef  INTERFACE
#define INTERFACE   IOleInPlaceActiveObject

DECLARE_INTERFACE_(IOleInPlaceActiveObject, IOleWindow)
{
   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IOleInPlaceActiveObject methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(OnFrameWindowActivate) (THIS_ BOOL fActivate) PURE;
    STDMETHOD(OnDocWindowActivate) (THIS_ BOOL fActivate) PURE;
    STDMETHOD(ResizeBorder) (THIS_ LPCRECT lprectBorder, LPOLEINPLACEUIWINDOW lpUIWindow, BOOL fFrameWindow) PURE;
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
};
typedef         IOleInPlaceActiveObject FAR* LPOLEINPLACEACTIVEOBJECT;



#undef  INTERFACE
#define INTERFACE   IOleInPlaceUIWindow

DECLARE_INTERFACE_(IOleInPlaceUIWindow, IOleWindow)
{
   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IOleInPlaceUIWindow methods ***
    STDMETHOD(GetBorder) (THIS_ LPRECT lprectBorder) PURE;
    STDMETHOD(RequestBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) PURE;
    STDMETHOD(SetBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) PURE;
    STDMETHOD(SetActiveObject) (THIS_ LPOLEINPLACEACTIVEOBJECT lpActiveObject,
                        LPCSTR lpszObjName) PURE;
};
typedef     IOleInPlaceUIWindow FAR* LPOLEINPLACEUIWINDOW;



#undef  INTERFACE
#define INTERFACE   IOleInPlaceFrame

DECLARE_INTERFACE_(IOleInPlaceFrame, IOleInPlaceUIWindow)
{
   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IOleInPlaceUIWindow methods ***
    STDMETHOD(GetBorder) (THIS_ LPRECT lprectBorder) PURE;
    STDMETHOD(RequestBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) PURE;
    STDMETHOD(SetBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) PURE;
    STDMETHOD(SetActiveObject) (THIS_ LPOLEINPLACEACTIVEOBJECT lpActiveObject,
                    LPCSTR lpszObjName) PURE;


    // *** IOleInPlaceFrame methods ***
    STDMETHOD(InsertMenus) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) PURE;
    STDMETHOD(SetMenu) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) PURE;
    STDMETHOD(RemoveMenus) (THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetStatusText) (THIS_ LPCSTR lpszStatusText) PURE;
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg, WORD wID) PURE;
};
typedef     IOleInPlaceFrame FAR* LPOLEINPLACEFRAME;


#undef  INTERFACE
#define INTERFACE   IOleInPlaceSite

DECLARE_INTERFACE_(IOleInPlaceSite, IOleWindow)
{
   // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IOleInPlaceSite methods ***
    STDMETHOD(CanInPlaceActivate) (THIS) PURE;
    STDMETHOD(OnInPlaceActivate) (THIS) PURE;
    STDMETHOD(OnUIActivate) (THIS) PURE;
    STDMETHOD(GetWindowContext) (THIS_ LPOLEINPLACEFRAME FAR* lplpFrame,
                        LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                        LPRECT lprcPosRect,
                        LPRECT lprcClipRect,
                        LPOLEINPLACEFRAMEINFO lpFrameInfo) PURE;
    STDMETHOD(Scroll) (THIS_ SIZE scrollExtent) PURE;
    STDMETHOD(OnUIDeactivate) (THIS_ BOOL fUndoable) PURE;
    STDMETHOD(OnInPlaceDeactivate) (THIS) PURE;
    STDMETHOD(DiscardUndoState) (THIS) PURE;
    STDMETHOD(DeactivateAndUndo) (THIS) PURE;
    STDMETHOD(OnPosRectChange) (THIS_ LPCRECT lprcPosRect) PURE;
};
typedef         IOleInPlaceSite FAR* LPOLEINPLACESITE;



/****** OLE API Prototypes ************************************************/

STDAPI_(DWORD) OleBuildVersion( VOID );

/* helper functions */
STDAPI ReadClassStg(LPSTORAGE pStg, CLSID FAR* pclsid);
STDAPI WriteClassStg(LPSTORAGE pStg, REFCLSID rclsid);
STDAPI ReadClassStm(LPSTREAM pStm, CLSID FAR* pclsid);
STDAPI WriteClassStm(LPSTREAM pStm, REFCLSID rclsid);
STDAPI WriteFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT cf, LPSTR lpszUserType);
STDAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT FAR* pcf, LPSTR FAR* lplpszUserType);


/* init/term */

STDAPI OleInitialize(LPMALLOC pMalloc);
STDAPI_(void) OleUninitialize(void);


/* APIs to query whether (Embedded/Linked) object can be created from
   the data object */

STDAPI  OleQueryLinkFromData(LPDATAOBJECT pSrcDataObject);
STDAPI  OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject);


/* Object creation APIs */

STDAPI  OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

STDAPI  OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

STDAPI  OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

STDAPI  OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);


STDAPI  OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

STDAPI  OleCreateLinkToFile(LPCSTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

STDAPI  OleCreateFromFile(REFCLSID rclsid, LPCSTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

STDAPI  OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite,
            LPVOID FAR* ppvObj);

STDAPI  OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad);

STDAPI  OleLoadFromStream( LPSTREAM pStm, REFIID iidInterface, LPVOID FAR* ppvObj);
STDAPI  OleSaveToStream( LPPERSISTSTREAM pPStm, LPSTREAM pStm );


STDAPI  OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained);
STDAPI  OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible);


/* Drag/Drop APIs */

STDAPI  RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget);
STDAPI  RevokeDragDrop(HWND hwnd);
STDAPI  DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource,
            DWORD dwOKEffects, LPDWORD pdwEffect);

/* Clipboard APIs */

STDAPI  OleSetClipboard(LPDATAOBJECT pDataObj);
STDAPI  OleGetClipboard(LPDATAOBJECT FAR* ppDataObj);
STDAPI  OleFlushClipboard(void);
STDAPI  OleIsCurrentClipboard(LPDATAOBJECT pDataObj);


/* InPlace Editing APIs */

STDAPI_(HOLEMENU)   OleCreateMenuDescriptor (HMENU hmenuCombined,
                                LPOLEMENUGROUPWIDTHS lpMenuWidths);
STDAPI              OleSetMenuDescriptor (HOLEMENU holemenu, HWND hwndFrame,
                                HWND hwndActiveObject,
                                LPOLEINPLACEFRAME lpFrame,
                                LPOLEINPLACEACTIVEOBJECT lpActiveObj);
STDAPI              OleDestroyMenuDescriptor (HOLEMENU holemenu);

STDAPI              OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                            LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg);


/* Helper APIs */
STDAPI_(HANDLE) OleDuplicateData (HANDLE hSrc, CLIPFORMAT cfFormat,
                        UINT uiFlags);

STDAPI          OleDraw (LPUNKNOWN pUnknown, DWORD dwAspect, HDC hdcDraw,
                    LPCRECT lprcBounds);

STDAPI          OleRun(LPUNKNOWN pUnknown);
STDAPI_(BOOL)   OleIsRunning(LPOLEOBJECT pObject);

STDAPI_(void)   ReleaseStgMedium(LPSTGMEDIUM);
STDAPI          CreateOleAdviseHolder(LPOLEADVISEHOLDER FAR* ppOAHolder);

STDAPI          OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                    REFIID riid, LPVOID FAR* lplpObj);

/* OLE 1.0 conversion APIS */

STDAPI OleConvertOLESTREAMToIStorage
    (LPOLESTREAM                lpolestream,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd);

STDAPI OleConvertIStorageToOLESTREAM
    (LPSTORAGE      pstg,
    LPOLESTREAM     lpolestream);


/* Storage Utility APIs */
STDAPI GetHGlobalFromILockBytes (LPLOCKBYTES plkbyt, HGLOBAL FAR* phglobal);
STDAPI CreateILockBytesOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                    LPLOCKBYTES FAR* pplkbyt);

STDAPI GetHGlobalFromStream (LPSTREAM pstm, HGLOBAL FAR* phglobal);
STDAPI CreateStreamOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                LPSTREAM FAR* ppstm);


/* ConvertTo APIS */

STDAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew);
STDAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew);
STDAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew);
STDAPI GetConvertStg(LPSTORAGE pStg);
STDAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert);


#endif // _OLE2_H_
