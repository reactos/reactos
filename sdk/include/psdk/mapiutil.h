/*
 * Copyright 2004 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef MAPIUTIL_H_
#define MAPIUTIL_H_

#include <mapix.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAD_ALL_ROWS 1 /* Delete all rows */

LPMALLOC WINAPI MAPIGetDefaultMalloc(void);

#define SOF_UNIQUEFILENAME 0x80000000U /* Create a unique (temporary) filename */

#if defined (UNICODE) || defined (__WINESRC__)
typedef HRESULT (WINAPI * LPOPENSTREAMONFILE)(LPALLOCATEBUFFER,LPFREEBUFFER,
                                              ULONG,LPWSTR,LPWSTR,LPSTREAM*);
HRESULT WINAPI OpenStreamOnFile(LPALLOCATEBUFFER,LPFREEBUFFER,
                                ULONG,LPWSTR,LPWSTR,LPSTREAM*);
#else
typedef HRESULT (WINAPI * LPOPENSTREAMONFILE)(LPALLOCATEBUFFER,LPFREEBUFFER,
                                              ULONG,LPSTR,LPSTR,LPSTREAM*);
HRESULT WINAPI OpenStreamOnFile(LPALLOCATEBUFFER,LPFREEBUFFER,
                                ULONG,LPSTR,LPSTR,LPSTREAM*);
#endif
#define OPENSTREAMONFILE "OpenStreamOnFile"

BOOL WINAPI FEqualNames(LPMAPINAMEID,LPMAPINAMEID);

typedef struct IPropData *LPPROPDATA;

#define IPROP_READONLY  0x00001U
#define IPROP_READWRITE 0x00002U
#define IPROP_CLEAN     0x10000U
#define IPROP_DIRTY     0x20000U

SCODE WINAPI CreateIProp(LPCIID,ALLOCATEBUFFER*,ALLOCATEMORE*,FREEBUFFER*,
                         LPVOID,LPPROPDATA*);
SCODE WINAPI PropCopyMore(LPSPropValue,LPSPropValue,ALLOCATEMORE*,LPVOID);
ULONG WINAPI UlPropSize(LPSPropValue);
VOID  WINAPI GetInstance(LPSPropValue,LPSPropValue,ULONG);
BOOL  WINAPI FPropContainsProp(LPSPropValue,LPSPropValue,ULONG);
BOOL  WINAPI FPropCompareProp(LPSPropValue,ULONG,LPSPropValue);
LONG  WINAPI LPropCompareProp(LPSPropValue,LPSPropValue);

HRESULT WINAPI HrAddColumns(LPMAPITABLE,LPSPropTagArray,LPALLOCATEBUFFER,LPFREEBUFFER);
HRESULT WINAPI HrAddColumnsEx(LPMAPITABLE,LPSPropTagArray,LPALLOCATEBUFFER,
                              LPFREEBUFFER,void (*)(LPSPropTagArray));
HRESULT WINAPI HrAllocAdviseSink(LPNOTIFCALLBACK,LPVOID,LPMAPIADVISESINK*);
HRESULT WINAPI HrThisThreadAdviseSink(LPMAPIADVISESINK,LPMAPIADVISESINK*);
HRESULT WINAPI HrDispatchNotifications (ULONG);

ULONG WINAPI UlAddRef(void*);
ULONG WINAPI UlRelease(void*);

HRESULT WINAPI HrGetOneProp(LPMAPIPROP,ULONG,LPSPropValue*);
HRESULT WINAPI HrSetOneProp(LPMAPIPROP,LPSPropValue);
BOOL    WINAPI FPropExists(LPMAPIPROP,ULONG);
void    WINAPI FreePadrlist(LPADRLIST);
void    WINAPI FreeProws(LPSRowSet);
HRESULT WINAPI HrQueryAllRows(LPMAPITABLE,LPSPropTagArray,LPSRestriction,
                              LPSSortOrderSet,LONG,LPSRowSet*);
LPSPropValue WINAPI PpropFindProp(LPSPropValue,ULONG,ULONG);

#if defined (UNICODE) || defined (__WINESRC__)
BOOL   WINAPI FBinFromHex(LPWSTR,LPBYTE);
SCODE  WINAPI ScBinFromHexBounded(LPWSTR,LPBYTE,ULONG);
void   WINAPI HexFromBin(LPBYTE,int,LPWSTR);
ULONG  WINAPI UlFromSzHex(LPCWSTR);
LPWSTR WINAPI SzFindCh(LPCWSTR,USHORT);
LPWSTR WINAPI SzFindLastCh(LPCWSTR,USHORT);
LPWSTR WINAPI SzFindSz(LPCWSTR,LPCWSTR);
UINT   WINAPI UFromSz(LPCSTR);
#else
BOOL  WINAPI FBinFromHex(LPSTR,LPBYTE);
SCODE WINAPI ScBinFromHexBounded(LPSTR,LPBYTE,ULONG);
void  WINAPI HexFromBin(LPBYTE,int,LPSTR);
ULONG WINAPI UlFromSzHex(LPCSTR);
LPSTR WINAPI SzFindCh(LPCSTR,USHORT);
LPSTR WINAPI SzFindLastCh(LPCSTR,USHORT);
LPSTR WINAPI SzFindSz(LPCSTR,LPCSTR);
UINT  WINAPI UFromSz(LPCSTR);
#endif

SCODE WINAPI ScInitMapiUtil(ULONG);
void  WINAPI DeinitMapiUtil(void);

#define szHrDispatchNotifications "_HrDispatchNotifications@4"
#define szScCreateConversationIndex "_ScCreateConversationIndex@16"

typedef HRESULT (WINAPI DISPATCHNOTIFICATIONS)(ULONG);
typedef DISPATCHNOTIFICATIONS *LPDISPATCHNOTIFICATIONS;
typedef SCODE (WINAPI CREATECONVERSATIONINDEX)(ULONG,LPBYTE,ULONG*,LPBYTE*);
typedef CREATECONVERSATIONINDEX *LPCREATECONVERSATIONINDEX;

typedef struct ITableData *LPTABLEDATA;

typedef void (WINAPI CALLERRELEASE)(ULONG,LPTABLEDATA,LPMAPITABLE);

/*****************************************************************************
 * ITableData interface
 *
 * The underlying table data structure for IMAPITable.
 */
#define INTERFACE ITableData
DECLARE_INTERFACE_(ITableData,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** ITableData methods ***/
    STDMETHOD(HrGetView)(THIS_ LPSSortOrderSet lpSort, CALLERRELEASE *lpRel,
                         ULONG ulData, LPMAPITABLE *lppTable) PURE;
    STDMETHOD(HrModifyRow)(THIS_ LPSRow lpRow) PURE;
    STDMETHOD(HrDeleteRow)(THIS_ LPSPropValue lpKey) PURE;
    STDMETHOD(HrQueryRow)(THIS_ LPSPropValue lpKey, LPSRow *lppRow, ULONG *lpRowNum) PURE;
    STDMETHOD(HrEnumRow)(THIS_ ULONG ulRowNum, LPSRow *lppRow) PURE;
    STDMETHOD(HrNotify)(THIS_ ULONG ulFlags, ULONG cValues, LPSPropValue lpValues) PURE;
    STDMETHOD(HrInsertRow)(THIS_ ULONG ulRow, LPSRow lpRow) PURE;
    STDMETHOD(HrModifyRows)(THIS_ ULONG ulFlags, LPSRowSet lpRows) PURE;
    STDMETHOD(HrDeleteRows)(THIS_ ULONG ulFlags, LPSRowSet lpRows, ULONG *lpCount) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define ITableData_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITableData_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ITableData_Release(p)            (p)->lpVtbl->Release(p)
        /*** ITableData methods ***/
#define ITableData_HrGetView(p,a,b,c,d)  (p)->lpVtbl->HrGetView(p,a,b,c,d)
#define ITableData_HrModifyRow(p,a)      (p)->lpVtbl->HrModifyRow(p,a)
#define ITableData_HrDeleteRow(p,a)      (p)->lpVtbl->HrDeleteRow(p,a)
#define ITableData_HrQueryRow(p,a,b,c)   (p)->lpVtbl->HrQueryRow(p,a,b,c)
#define ITableData_HrEnumRow(p,a,b)      (p)->lpVtbl->HrEnumRow(p,a,b)
#define ITableData_HrNotify(p,a,b,c)     (p)->lpVtbl->HrNotify(p,a,b,c)
#define ITableData_HrInsertRow(p,a,b)    (p)->lpVtbl->HrInsertRow(p,a,b)
#define ITableData_HrModifyRows(p,a,b)   (p)->lpVtbl->HrModifyRows(p,a,b)
#define ITableData_HrDeleteRows(p,a,b,c) (p)->lpVtbl->HrDeleteRows(p,a,b,c)
#endif

SCODE WINAPI CreateTable(LPCIID,ALLOCATEBUFFER*,ALLOCATEMORE*,FREEBUFFER*,
                         LPVOID,ULONG,ULONG,LPSPropTagArray,LPTABLEDATA*);

SCODE WINAPI ScCountNotifications(int,LPNOTIFICATION,ULONG*);
SCODE WINAPI ScCountProps(int,LPSPropValue,ULONG*);
SCODE WINAPI ScCopyNotifications(int,LPNOTIFICATION,LPVOID,ULONG*);
SCODE WINAPI ScCopyProps(int,LPSPropValue,LPVOID,ULONG*);
SCODE WINAPI ScDupPropset(int,LPSPropValue,LPALLOCATEBUFFER,LPSPropValue*);
SCODE WINAPI ScRelocNotifications(int,LPNOTIFICATION,LPVOID,LPVOID,ULONG*);
SCODE WINAPI ScRelocProps(int,LPSPropValue,LPVOID,LPVOID,ULONG*);

LPSPropValue WINAPI LpValFindProp(ULONG,ULONG,LPSPropValue);

static inline FILETIME FtAddFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONG64 *pl = (LONG64*)&ftLeft, *pr = (LONG64*)&ftRight;
    union { FILETIME ft; LONG64 ll; } ftmap;    
    ftmap.ll = *pl + *pr;
    return ftmap.ft;
}

static inline FILETIME FtSubFt(FILETIME ftLeft, FILETIME ftRight)
{
    LONG64 *pl = (LONG64*)&ftLeft, *pr = (LONG64*)&ftRight;
    union { FILETIME ft; LONG64 ll; } ftmap;    
    ftmap.ll = *pl - *pr;
    return ftmap.ft;
}

static inline FILETIME FtNegFt(FILETIME ftLeft)
{
    LONG64 *p = (LONG64*)&ftLeft;
    union { FILETIME ft; LONG64 ll; } ftmap;    
    ftmap.ll = -*p;
    return ftmap.ft;
}

static inline FILETIME FtMulDw(DWORD dwLeft, FILETIME ftRight)
{
    LONG64 l = (LONG64)dwLeft, *pr = (LONG64*)&ftRight;
    union { FILETIME ft; LONG64 ll; } ftmap;    
    ftmap.ll = l * (*pr);
    return ftmap.ft;
}

static inline FILETIME FtMulDwDw(DWORD dwLeft, DWORD dwRight)
{
    LONG64 l = (LONG64)dwLeft, r = (LONG64)dwRight;
    union { FILETIME ft; LONG64 ll; } ftmap;    
    ftmap.ll = l * r;
    return ftmap.ft;
}

/*****************************************************************************
 * IPropData interface
 *
 */
#define INTERFACE IPropData
DECLARE_INTERFACE_(IPropData,IMAPIProp)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMAPIProp methods ***/
    STDMETHOD(GetLastError)(THIS_ HRESULT hRes, ULONG ulFlags, LPMAPIERROR *lppErr) PURE;
    STDMETHOD(SaveChanges)(THIS_ ULONG ulFlags) PURE;
    STDMETHOD(GetProps)(THIS_ LPSPropTagArray lpPropTags, ULONG ulFlags, ULONG *lpValues, LPSPropValue *lppProps) PURE;
    STDMETHOD(GetPropList)(THIS_ ULONG  ulFlags, LPSPropTagArray *lppPropTagArray) PURE;
    STDMETHOD(OpenProperty)(THIS_ ULONG ulPropTag, LPCIID lpIid, ULONG ulOpts, ULONG ulFlags, LPUNKNOWN *lppUnk) PURE;
    STDMETHOD(SetProps)(THIS_ ULONG cValues, LPSPropValue lpProps, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(DeleteProps)(THIS_ LPSPropTagArray lpPropTags, LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyTo)(THIS_ ULONG ciidExclude, LPCIID lpIid, LPSPropTagArray lpProps, ULONG ulParam,
                      LPMAPIPROGRESS lpProgress, LPCIID lpIface,LPVOID lpDest, ULONG ulFlags,
                      LPSPropProblemArray *lppProbs) PURE;
    STDMETHOD(CopyProps)(THIS_ LPSPropTagArray lpIncludeProps, ULONG ulParam, LPMAPIPROGRESS lpProgress,
                         LPCIID lpIid, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray *lppProblems) PURE;
    STDMETHOD(GetNamesFromIDs)(THIS_ LPSPropTagArray *lppPropTags, LPGUID lpIid, ULONG ulFlags, ULONG *lpCount,
                               LPMAPINAMEID **lpppNames) PURE;
    STDMETHOD(GetIDsFromNames)(THIS_ ULONG cPropNames, LPMAPINAMEID *lppNames, ULONG ulFlags, LPSPropTagArray *lppPropTags) PURE;
    /*** IPropData methods ***/
    STDMETHOD(HrSetObjAccess)(THIS_ ULONG ulAccess) PURE;
    STDMETHOD(HrSetPropAccess)(THIS_ LPSPropTagArray lpPropTags, ULONG *lpAccess) PURE;
    STDMETHOD(HrGetPropAccess)(THIS_ LPSPropTagArray *lppPropTags, ULONG **lppAccess) PURE;
    STDMETHOD(HrAddObjProps)(THIS_ LPSPropTagArray lppPropTags, LPSPropProblemArray *lppProbs) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
        /*** IUnknown methods ***/
#define IPropData_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IPropData_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IPropData_Release(p)                   (p)->lpVtbl->Release(p)
        /*** IMAPIProp methods ***/
#define IPropData_GetLastError(p,a,b,c)        (p)->lpVtbl->GetLastError(p,a,b,c)        
#define IPropData_SaveChanges(p,a)             (p)->lpVtbl->SaveChanges(p,a)             
#define IPropData_GetProps(p,a,b,c,d)          (p)->lpVtbl->GetProps(p,a,b,c,d)          
#define IPropData_GetPropList(p,a,b)           (p)->lpVtbl->GetPropList(p,a,b)           
#define IPropData_OpenProperty(p,a,b,c,d,e)    (p)->lpVtbl->OpenProperty(p,a,b,c,d,e)    
#define IPropData_SetProps(p,a,b,c)            (p)->lpVtbl->SetProps(p,a,b,c)            
#define IPropData_DeleteProps(p,a,b)           (p)->lpVtbl->DeleteProps(p,a,b)           
#define IPropData_CopyTo(p,a,b,c,d,e,f,g,h,i)  (p)->lpVtbl->CopyTo(p,a,b,c,d,e,f,g,h,i)  
#define IPropData_CopyProps(p,a,b,c,d,e,f,g)   (p)->lpVtbl->CopyProps(p,a,b,c,d,e,f,g)   
#define IPropData_GetNamesFromIDs(p,a,b,c,d,e) (p)->lpVtbl->GetNamesFromIDs(p,a,b,c,d,e) 
#define IPropData_GetIDsFromNames(p,a,b,c,d)   (p)->lpVtbl->GetIDsFromNames(p,a,b,c,d)   
#define IPropData_HrSetObjAccess(p,a)          (p)->lpVtbl->HrSetObjAccess(p,a)
#define IPropData_HrSetPropAccess(p,a,b)       (p)->lpVtbl->HrSetPropAccess(p,a,b)
#define IPropData_HrGetPropAccess(p,a,b)       (p)->lpVtbl->HrGetPropAccess(p,a,b)
#define IPropData_HrAddObjProps(p,a,b)         (p)->lpVtbl->HrAddObjProps(p,a,b)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAPIUTIL_H_ */
