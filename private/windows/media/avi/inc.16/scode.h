//+---------------------------------------------------------------------------
//
// File:        SCode.h
//
// Contents:    Defines standard status code services.
//
//
//----------------------------------------------------------------------------

#ifndef __SCODE_H__
#define __SCODE_H__

//
// SCODE
//

typedef long SCODE;
typedef SCODE *PSCODE;
typedef void FAR * HRESULT;
#define NOERROR 0

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+---------------------+-------+-------------------------------+
//  |S|       Context       | Facil |               Code            |
//  +-+---------------------+-------+-------------------------------+
//
//  where
//
//      S - is the severity code
//
//          0 - Success
//          1 - Error
//
//      Context - context info
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//
// Severity values
//

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1



#define SUCCEEDED(Status) ((SCODE)(Status) >= 0)

#define FAILED(Status) ((SCODE)(Status)<0)


//
// Return the code
//

#define SCODE_CODE(sc)      ((sc) & 0xFFFF)

//
//  Return the facility
//

#define SCODE_FACILITY(sc)  (((sc) >> 16) & 0x1fff)

//
//  Return the severity
//

#define SCODE_SEVERITY(sc)  (((sc) >> 31) & 0x1)

//
// Create an SCODE value from component pieces
//

#define MAKE_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )



// --------------------- Functions ---------------------------------------

#define GetScode(hr)        ((SCODE)(hr) & 0x800FFFFF)
#define ResultFromScode(sc) ((HRESULT)((SCODE)(sc) & 0x800FFFFF))

STDAPI PropagateResult(HRESULT hrPrev, SCODE scNew);


// -------------------------- Facility definitions -------------------------

#define FACILITY_NULL       0x0000 // generally useful errors ([SE]_*)
#define FACILITY_RPC            0x0001 // remote procedure call errors (RPC_E_*)
#define FACILITY_DISPATCH   0x0002 // late binding dispatch errors
#define FACILITY_STORAGE   0x0003 // storage errors (STG_E_*)
#define FACILITY_ITF            0x0004 // interface-specific errors



#define S_OK                0L
#define S_FALSE             MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 1)



// --------------------- FACILITY_NULL errors ------------------------------

#define E_UNEXPECTED        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 0xffff)
                            // relatively catastrophic failure

#define E_NOTIMPL           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 1)
                            // not implemented

#define E_OUTOFMEMORY       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 2)
                            // ran out of memory

#define E_INVALIDARG        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 3)
                            // one or more arguments are invalid

#define E_NOINTERFACE       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 4)
                            // no such interface supported


#define E_POINTER           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 5)
                            // invalid pointer

#define E_HANDLE            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 6)
                            // invalid handle

#define E_ABORT             MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 7)
                            // operation aborted

#define E_FAIL              MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 8)
                            // unspecified error


#define E_ACCESSDENIED      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_NULL, 9)
                            // general access denied error


// ----------------- FACILITY_ITF errors used by OLE ---------------------
//
// By convention, OLE interfaces divide the FACILITY_ITF range of errors
// into nonoverlapping subranges.  If an OLE interface returns a FACILITY_ITF 
// scode, it must be from the range associated with that interface or from
// the shared range: OLE_E_FIRST...OLE_E_LAST.
//
// The ranges, their associated interfaces, and the header file that defines
// the actual scodes are given below.
// 

// Generic OLE errors that may be returned by many interfaces
#define OLE_E_FIRST MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0000)
#define OLE_E_LAST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x00FF)
#define OLE_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0000)
#define OLE_S_LAST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x00FF)
// interfaces: all
// file: ole2.h


#define DRAGDROP_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0100)
#define DRAGDROP_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x010F)
#define DRAGDROP_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0100)
#define DRAGDROP_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x010F)
// interfaces: IDropSource, IDropTarget
// file: ole2.h

#define CLASSFACTORY_E_FIRST MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0110)
#define CLASSFACTORY_E_LAST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x011F)
#define CLASSFACTORY_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0110)
#define CLASSFACTORY_S_LAST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x011F)
// interfaces: IClassFactory
// file:

#define MARSHAL_E_FIRST MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0120)
#define MARSHAL_E_LAST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x012F)
#define MARSHAL_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0120)
#define MARSHAL_S_LAST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x012F)
// interfaces: IMarshal, IStdMarshalInfo, marshal APIs
// file:

#define DATA_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0130)
#define DATA_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x013F)
#define DATA_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0130)
#define DATA_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x013F)
// interfaces: IDataObject
// file: dvobj.h

#define VIEW_E_FIRST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0140)
#define VIEW_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x014F)
#define VIEW_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0140)
#define VIEW_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x014F)
// interfaces: IViewObject
// file: dvobj.h

#define REGDB_E_FIRST   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0150)
#define REGDB_E_LAST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x015F)
#define REGDB_S_FIRST   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0150)
#define REGDB_S_LAST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x015F)
// API: reg.dat manipulation
// file: 


// range 160 - 16F reserved

#define CACHE_E_FIRST   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0170) 
#define CACHE_E_LAST    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x017F)
#define CACHE_S_FIRST   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0170)
#define CACHE_S_LAST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x017F)
// interfaces: IOleCache
// file:

#define OLEOBJ_E_FIRST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0180)
#define OLEOBJ_E_LAST   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x018F)
#define OLEOBJ_S_FIRST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0180)
#define OLEOBJ_S_LAST   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x018F)
// interfaces: IOleObject
// file:

#define CLIENTSITE_E_FIRST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0190) 
#define CLIENTSITE_E_LAST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x019F)
#define CLIENTSITE_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0190)
#define CLIENTSITE_S_LAST   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x019F)
// interfaces: IOleClientSite
// file:

#define INPLACE_E_FIRST MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01A0)
#define INPLACE_E_LAST  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01AF)
#define INPLACE_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01A0)
#define INPLACE_S_LAST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01AF)
// interfaces: IOleWindow, IOleInPlaceObject, IOleInPlaceActiveObject,
//                 IOleInPlaceUIWindow, IOleInPlaceFrame, IOleInPlaceSite
// file:

#define ENUM_E_FIRST        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01B0)
#define ENUM_E_LAST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01BF)
#define ENUM_S_FIRST    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01B0)
#define ENUM_S_LAST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01BF)
// interfaces: IEnum*
// file:

#define CONVERT10_E_FIRST   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01C0)
#define CONVERT10_E_LAST   MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01CF)
#define CONVERT10_S_FIRST  MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01C0)
#define CONVERT10_S_LAST   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01CF)
// API: OleConvertOLESTREAMToIStorage, OleConvertIStorageToOLESTREAM
// file:


#define CLIPBRD_E_FIRST     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01D0)
#define CLIPBRD_E_LAST      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01DF)
#define CLIPBRD_S_FIRST     MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01D0)
#define CLIPBRD_S_LAST      MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01DF)
// interfaces: OleSetClipboard, OleGetClipboard, OleFlushClipboard
// file: ole2.h

#define MK_E_FIRST      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01E0)
#define MK_E_LAST           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01EF)
#define MK_S_FIRST          MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01E0)
#define MK_S_LAST           MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01EF)
// interfaces: IMoniker, IBindCtx, IRunningObjectTable, IParseDisplayName,
//             IOleContainer, IOleItemContainer, IOleLink
// file: moniker.h


#define CO_E_FIRST      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01F0)
#define CO_E_LAST           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x01FF)
#define CO_S_FIRST          MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01F0)
#define CO_S_LAST           MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x01FF)
// all Co* API
// file: compobj.h


// range 200 - ffff for new error codes



#endif      // ifndef __SCODE_H__
