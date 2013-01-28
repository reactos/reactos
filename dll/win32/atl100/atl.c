/*
 * Copyright 2012 Stefan Leichter
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <objbase.h>

#include <atlbase.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/***********************************************************************
 *           AtlAdvise         [atl100.@]
 */
HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID *iid, LPDWORD pdw)
{
    FIXME("%p %p %p %p\n", pUnkCP, pUnk, iid, pdw);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnadvise         [atl100.@]
 */
HRESULT WINAPI AtlUnadvise(IUnknown *pUnkCP, const IID *iid, DWORD dw)
{
    FIXME("%p %p %d\n", pUnkCP, iid, dw);
    return S_OK;
}

/***********************************************************************
 *           AtlFreeMarshalStream         [atl100.@]
 */
HRESULT WINAPI AtlFreeMarshalStream(IStream *stm)
{
    FIXME("%p\n", stm);
    return S_OK;
}

/***********************************************************************
 *           AtlMarshalPtrInProc         [atl100.@]
 */
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown *pUnk, const IID *iid, IStream **pstm)
{
    FIXME("%p %p %p\n", pUnk, iid, pstm);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnmarshalPtr              [atl100.@]
 */
HRESULT WINAPI AtlUnmarshalPtr(IStream *stm, const IID *iid, IUnknown **ppUnk)
{
    FIXME("%p %p %p\n", stm, iid, ppUnk);
    return E_FAIL;
}

/***********************************************************************
 *           AtlCreateTargetDC         [atl100.@]
 */
HDC WINAPI AtlCreateTargetDC( HDC hdc, DVTARGETDEVICE *dv )
{
    static const WCHAR displayW[] = {'d','i','s','p','l','a','y',0};
    const WCHAR *driver = NULL, *device = NULL, *port = NULL;
    DEVMODEW *devmode = NULL;

    TRACE( "(%p, %p)\n", hdc, dv );

    if (dv)
    {
        if (dv->tdDriverNameOffset) driver  = (WCHAR *)((char *)dv + dv->tdDriverNameOffset);
        if (dv->tdDeviceNameOffset) device  = (WCHAR *)((char *)dv + dv->tdDeviceNameOffset);
        if (dv->tdPortNameOffset)   port    = (WCHAR *)((char *)dv + dv->tdPortNameOffset);
        if (dv->tdExtDevmodeOffset) devmode = (DEVMODEW *)((char *)dv + dv->tdExtDevmodeOffset);
    }
    else
    {
        if (hdc) return hdc;
        driver = displayW;
    }
    return CreateDCW( driver, device, port, devmode );
}

/***********************************************************************
 *           AtlHiMetricToPixel              [atl100.@]
 */
void WINAPI AtlHiMetricToPixel(const SIZEL* lpHiMetric, SIZEL* lpPix)
{
    HDC dc = GetDC(NULL);
    lpPix->cx = lpHiMetric->cx * GetDeviceCaps( dc, LOGPIXELSX ) / 100;
    lpPix->cy = lpHiMetric->cy * GetDeviceCaps( dc, LOGPIXELSY ) / 100;
    ReleaseDC( NULL, dc );
}

/***********************************************************************
 *           AtlPixelToHiMetric              [atl100.@]
 */
void WINAPI AtlPixelToHiMetric(const SIZEL* lpPix, SIZEL* lpHiMetric)
{
    HDC dc = GetDC(NULL);
    lpHiMetric->cx = 100 * lpPix->cx / GetDeviceCaps( dc, LOGPIXELSX );
    lpHiMetric->cy = 100 * lpPix->cy / GetDeviceCaps( dc, LOGPIXELSY );
    ReleaseDC( NULL, dc );
}

/***********************************************************************
 *           AtlComPtrAssign              [atl100.@]
 */
IUnknown* WINAPI AtlComPtrAssign(IUnknown** pp, IUnknown *p)
{
    TRACE("(%p %p)\n", pp, p);

    if (p) IUnknown_AddRef(p);
    if (*pp) IUnknown_Release(*pp);
    *pp = p;
    return p;
}

/***********************************************************************
 *           AtlComQIPtrAssign              [atl100.@]
 */
IUnknown* WINAPI AtlComQIPtrAssign(IUnknown** pp, IUnknown *p, REFIID riid)
{
    IUnknown *new_p = NULL;

    TRACE("(%p %p %s)\n", pp, p, debugstr_guid(riid));

    if (p) IUnknown_QueryInterface(p, riid, (void **)&new_p);
    if (*pp) IUnknown_Release(*pp);
    *pp = new_p;
    return new_p;
}

/***********************************************************************
 *           AtlInternalQueryInterface     [atl100.@]
 */
HRESULT WINAPI AtlInternalQueryInterface(void* this, const _ATL_INTMAP_ENTRY* pEntries,  REFIID iid, void** ppvObject)
{
    int i = 0;
    HRESULT rc = E_NOINTERFACE;
    TRACE("(%p, %p, %s, %p)\n",this, pEntries, debugstr_guid(iid), ppvObject);

    if (IsEqualGUID(iid,&IID_IUnknown))
    {
        TRACE("Returning IUnknown\n");
        *ppvObject = ((LPSTR)this+pEntries[0].dw);
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    while (pEntries[i].pFunc != 0)
    {
        TRACE("Trying entry %i (%s %i %p)\n",i,debugstr_guid(pEntries[i].piid),
              pEntries[i].dw, pEntries[i].pFunc);

        if (!pEntries[i].piid || IsEqualGUID(iid,pEntries[i].piid))
        {
            TRACE("MATCH\n");
            if (pEntries[i].pFunc == (_ATL_CREATORARGFUNC*)1)
            {
                TRACE("Offset\n");
                *ppvObject = ((LPSTR)this+pEntries[i].dw);
                IUnknown_AddRef((IUnknown*)*ppvObject);
                return S_OK;
            }
            else
            {
                TRACE("Function\n");
                rc = pEntries[i].pFunc(this, iid, ppvObject, pEntries[i].dw);
                if(rc==S_OK || pEntries[i].piid)
                    return rc;
            }
        }
        i++;
    }
    TRACE("Done returning (0x%x)\n",rc);
    return rc;
}

/* FIXME: should be in a header file */
typedef struct ATL_PROPMAP_ENTRY
{
    LPCOLESTR szDesc;
    DISPID dispid;
    const CLSID* pclsidPropPage;
    const IID* piidDispatch;
    DWORD dwOffsetData;
    DWORD dwSizeData;
    VARTYPE vt;
} ATL_PROPMAP_ENTRY;

/***********************************************************************
 *           AtlIPersistStreamInit_Load      [atl100.@]
 */
HRESULT WINAPI AtlIPersistStreamInit_Load( LPSTREAM pStm, ATL_PROPMAP_ENTRY *pMap,
                                           void *pThis, IUnknown *pUnk)
{
    FIXME("(%p, %p, %p, %p)\n", pStm, pMap, pThis, pUnk);

    return S_OK;
}

/***********************************************************************
 *           AtlIPersistStreamInit_Save      [atl100.@]
 */
HRESULT WINAPI AtlIPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty,
                                          ATL_PROPMAP_ENTRY *pMap, void *pThis,
                                          IUnknown *pUnk)
{
    FIXME("(%p, %d, %p, %p, %p)\n", pStm, fClearDirty, pMap, pThis, pUnk);

    return S_OK;
}

/***********************************************************************
 *           AtlGetVersion              [atl100.@]
 */
DWORD WINAPI AtlGetVersion(void *pReserved)
{
   return 0x0a00;
}
