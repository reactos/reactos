/* $Id: shlwapi.h,v 1.9 2004/10/20 20:31:36 gvg Exp $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <shlwapi.h>

#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#define INTERFACE IQueryAssociations
DECLARE_INTERFACE_(IQueryAssociations,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IQueryAssociations methods ***/
    STDMETHOD(Init)(THIS_ ASSOCF  flags, LPCWSTR  pszAssoc, HKEY  hkProgid, HWND  hwnd) PURE;
    STDMETHOD(GetString)(THIS_ ASSOCF  flags, ASSOCSTR  str, LPCWSTR  pszExtra, LPWSTR  pszOut, DWORD * pcchOut) PURE;
    STDMETHOD(GetKey)(THIS_ ASSOCF  flags, ASSOCKEY  key, LPCWSTR  pszExtra, HKEY * phkeyOut) PURE;
    STDMETHOD(GetData)(THIS_ ASSOCF  flags, ASSOCDATA  data, LPCWSTR  pszExtra, LPVOID  pvOut, DWORD * pcbOut) PURE;
    STDMETHOD(GetEnum)(THIS_ ASSOCF  flags, ASSOCENUM  assocenum, LPCWSTR  pszExtra, REFIID  riid, LPVOID * ppvOut) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IQueryAssociations_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IQueryAssociations_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IQueryAssociations_Release(p)              (p)->lpVtbl->Release(p)
#define IQueryAssociations_Init(p,a,b,c,d)         (p)->lpVtbl->Init(p,a,b,c,d)
#define IQueryAssociations_GetString(p,a,b,c,d,e)  (p)->lpVtbl->GetString(p,a,b,c,d,e)
#define IQueryAssociations_GetKey(p,a,b,c,d)       (p)->lpVtbl->GetKey(p,a,b,c,d)
#define IQueryAssociations_GetData(p,a,b,c,d,e)    (p)->lpVtbl->GetData(p,a,b,c,d,e)
#define IQueryAssociations_GetEnum(p,a,b,c,d,e)    (p)->lpVtbl->GetEnum(p,a,b,c,d,e)
#endif

#endif  /* __WINE_SHLWAPI_H */
