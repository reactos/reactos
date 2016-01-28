/*
 * Copyright 2012 Stefan Leichter
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include <precomp.h>

#include <wine/atlcom.h>
#include <wingdi.h>

#define ATLVer1Size FIELD_OFFSET(_ATL_MODULEW, dwAtlBuildVer)

HINSTANCE atl_instance;

typedef unsigned char cpp_bool;

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static ICatRegister *catreg;

/***********************************************************************
 *           AtlAdvise         [atl100.@]
 */
HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID *iid, DWORD *pdw)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    HRESULT hres;

    TRACE("%p %p %p %p\n", pUnkCP, pUnk, iid, pdw);

    if(!pUnkCP)
        return E_INVALIDARG;

    hres = IUnknown_QueryInterface(pUnkCP, &IID_IConnectionPointContainer, (void**)&container);
    if(FAILED(hres))
        return hres;

    hres = IConnectionPointContainer_FindConnectionPoint(container, iid, &cp);
    IConnectionPointContainer_Release(container);
    if(FAILED(hres))
        return hres;

    hres = IConnectionPoint_Advise(cp, pUnk, pdw);
    IConnectionPoint_Release(cp);
    return hres;
}

/***********************************************************************
 *           AtlUnadvise         [atl100.@]
 */
HRESULT WINAPI AtlUnadvise(IUnknown *pUnkCP, const IID *iid, DWORD dw)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    HRESULT hres;

    TRACE("%p %p %d\n", pUnkCP, iid, dw);

    if(!pUnkCP)
        return E_INVALIDARG;

    hres = IUnknown_QueryInterface(pUnkCP, &IID_IConnectionPointContainer, (void**)&container);
    if(FAILED(hres))
        return hres;

    hres = IConnectionPointContainer_FindConnectionPoint(container, iid, &cp);
    IConnectionPointContainer_Release(container);
    if(FAILED(hres))
        return hres;

    hres = IConnectionPoint_Unadvise(cp, dw);
    IConnectionPoint_Release(cp);
    return hres;
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
 *           AtlIPersistPropertyBag_Load      [atl100.@]
 */
HRESULT WINAPI AtlIPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog,
                                           ATL_PROPMAP_ENTRY *pMap, void *pThis,
                                           IUnknown *pUnk)
{
    FIXME("(%p, %p, %p, %p, %p)\n", pPropBag, pErrorLog, pMap, pThis, pUnk);

    return S_OK;
}

/***********************************************************************
 *           AtlIPersistPropertyBag_Save     [atl100.@]
 */
HRESULT WINAPI AtlIPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                                           BOOL fSaveAll, ATL_PROPMAP_ENTRY *pMap,
                                           void *pThis, IUnknown *pUnk)
{
    FIXME("(%p, %d, %d, %p, %p, %p)\n", pPropBag, fClearDirty, fSaveAll, pMap, pThis, pUnk);

    return S_OK;
}

/***********************************************************************
 *           AtlModuleAddTermFunc            [atl100.@]
 */
HRESULT WINAPI AtlModuleAddTermFunc(_ATL_MODULE *pM, _ATL_TERMFUNC *pFunc, DWORD_PTR dw)
{
    _ATL_TERMFUNC_ELEM *termfunc_elem;

    TRACE("version %04x (%p %p %ld)\n", _ATL_VER, pM, pFunc, dw);

    if (_ATL_VER > _ATL_VER_30 || pM->cbSize > ATLVer1Size) {
        termfunc_elem = HeapAlloc(GetProcessHeap(), 0, sizeof(_ATL_TERMFUNC_ELEM));
        termfunc_elem->pFunc = pFunc;
        termfunc_elem->dw = dw;
        termfunc_elem->pNext = pM->m_pTermFuncs;

        pM->m_pTermFuncs = termfunc_elem;
    }

    return S_OK;
}

#if _ATL_VER > _ATL_VER_30

/***********************************************************************
 *           AtlCallTermFunc              [atl100.@]
 */
void WINAPI AtlCallTermFunc(_ATL_MODULE *pM)
{
    _ATL_TERMFUNC_ELEM *iter = pM->m_pTermFuncs, *tmp;

    TRACE("(%p)\n", pM);

    while(iter) {
        iter->pFunc(iter->dw);
        tmp = iter;
        iter = iter->pNext;
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    pM->m_pTermFuncs = NULL;
}

#endif

/***********************************************************************
 *           AtlLoadTypeLib             [atl100.56]
 */
HRESULT WINAPI AtlLoadTypeLib(HINSTANCE inst, LPCOLESTR lpszIndex,
        BSTR *pbstrPath, ITypeLib **ppTypeLib)
{
    size_t path_len, index_len;
    ITypeLib *typelib = NULL;
    WCHAR *path;
    HRESULT hres;

    static const WCHAR tlb_extW[] = {'.','t','l','b',0};

    TRACE("(%p %s %p %p)\n", inst, debugstr_w(lpszIndex), pbstrPath, ppTypeLib);

    index_len = lpszIndex ? strlenW(lpszIndex) : 0;
    path = heap_alloc((MAX_PATH+index_len)*sizeof(WCHAR) + sizeof(tlb_extW));
    if(!path)
        return E_OUTOFMEMORY;

    path_len = GetModuleFileNameW(inst, path, MAX_PATH);
    if(!path_len) {
        heap_free(path);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if(index_len)
        memcpy(path+path_len, lpszIndex, (index_len+1)*sizeof(WCHAR));

    hres = LoadTypeLib(path, &typelib);
    if(FAILED(hres)) {
        WCHAR *ptr;

        for(ptr = path+path_len-1; ptr > path && *ptr != '\\' && *ptr != '.'; ptr--);
        if(*ptr != '.')
            ptr = path+path_len;
        memcpy(ptr, tlb_extW, sizeof(tlb_extW));
        hres = LoadTypeLib(path, &typelib);
    }

    if(SUCCEEDED(hres)) {
        *pbstrPath = SysAllocString(path);
        if(!*pbstrPath) {
            ITypeLib_Release(typelib);
            hres = E_OUTOFMEMORY;
        }
    }

    heap_free(path);
    if(FAILED(hres))
        return hres;

    *ppTypeLib = typelib;
    return S_OK;
}

#if _ATL_VER <= _ATL_VER_80

/***********************************************************************
 *           AtlRegisterTypeLib         [atl80.19]
 */
HRESULT WINAPI AtlRegisterTypeLib(HINSTANCE inst, const WCHAR *index)
{
    ITypeLib *typelib;
    BSTR path;
    HRESULT hres;

    TRACE("(%p %s)\n", inst, debugstr_w(index));

    hres = AtlLoadTypeLib(inst, index, &path, &typelib);
    if(FAILED(hres))
        return hres;

    hres = RegisterTypeLib(typelib, path, NULL); /* FIXME: pass help directory */
    ITypeLib_Release(typelib);
    SysFreeString(path);
    return hres;
}

#endif

#if _ATL_VER > _ATL_VER_30

/***********************************************************************
 *           AtlWinModuleInit                          [atl100.65]
 */
HRESULT WINAPI AtlWinModuleInit(_ATL_WIN_MODULE *winmod)
{
    TRACE("(%p)\n", winmod);

    if(winmod->cbSize != sizeof(*winmod))
        return E_INVALIDARG;

    InitializeCriticalSection(&winmod->m_csWindowCreate);
    winmod->m_pCreateWndList = NULL;
    return S_OK;
}

/***********************************************************************
 *           AtlWinModuleAddCreateWndData              [atl100.43]
 */
void WINAPI AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE *pM, _AtlCreateWndData *pData, void *pvObject)
{
    TRACE("(%p, %p, %p)\n", pM, pData, pvObject);

    pData->m_pThis = pvObject;
    pData->m_dwThreadID = GetCurrentThreadId();

    EnterCriticalSection(&pM->m_csWindowCreate);
    pData->m_pNext = pM->m_pCreateWndList;
    pM->m_pCreateWndList = pData;
    LeaveCriticalSection(&pM->m_csWindowCreate);
}

/***********************************************************************
 *           AtlWinModuleExtractCreateWndData          [atl100.44]
 */
void* WINAPI AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE *winmod)
{
    _AtlCreateWndData *iter, *prev = NULL;
    DWORD thread_id;

    TRACE("(%p)\n", winmod);

    thread_id = GetCurrentThreadId();

    EnterCriticalSection(&winmod->m_csWindowCreate);

    for(iter = winmod->m_pCreateWndList; iter && iter->m_dwThreadID != thread_id; iter = iter->m_pNext)
        prev = iter;
    if(iter) {
        if(prev)
            prev->m_pNext = iter->m_pNext;
        else
            winmod->m_pCreateWndList = iter->m_pNext;
    }

    LeaveCriticalSection(&winmod->m_csWindowCreate);

    return iter ? iter->m_pThis : NULL;
}

/***********************************************************************
 *           AtlComModuleGetClassObject                [atl100.15]
 */
HRESULT WINAPI AtlComModuleGetClassObject(_ATL_COM_MODULE *pm, REFCLSID rclsid, REFIID riid, void **ppv)
{
    _ATL_OBJMAP_ENTRY **iter;
    HRESULT hres;

    TRACE("(%p %s %s %p)\n", pm, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(!pm)
        return E_INVALIDARG;

    for(iter = pm->m_ppAutoObjMapFirst; iter < pm->m_ppAutoObjMapLast; iter++) {
        if(IsEqualCLSID((*iter)->pclsid, rclsid) && (*iter)->pfnGetClassObject) {
            if(!(*iter)->pCF)
                hres = (*iter)->pfnGetClassObject((*iter)->pfnCreateInstance, &IID_IUnknown, (void**)&(*iter)->pCF);
            if((*iter)->pCF)
                hres = IUnknown_QueryInterface((*iter)->pCF, riid, ppv);
            TRACE("returning %p (%08x)\n", *ppv, hres);
            return hres;
        }
    }

    WARN("Class %s not found\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *           AtlComModuleRegisterClassObjects   [atl100.17]
 */
HRESULT WINAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE *module, DWORD context, DWORD flags)
{
    _ATL_OBJMAP_ENTRY **iter;
    IUnknown *unk;
    HRESULT hres;

    TRACE("(%p %x %x)\n", module, context, flags);

    if(!module)
        return E_INVALIDARG;

    for(iter = module->m_ppAutoObjMapFirst; iter < module->m_ppAutoObjMapLast; iter++) {
        if(!(*iter)->pfnGetClassObject)
            continue;

        hres = (*iter)->pfnGetClassObject((*iter)->pfnCreateInstance, &IID_IUnknown, (void**)&unk);
        if(FAILED(hres))
            return hres;

        hres = CoRegisterClassObject((*iter)->pclsid, unk, context, flags, &(*iter)->dwRegister);
        IUnknown_Release(unk);
        if(FAILED(hres))
            return hres;
    }

   return S_OK;
}

/***********************************************************************
 *           AtlComModuleRevokeClassObjects   [atl100.20]
 */
HRESULT WINAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE *module)
{
    _ATL_OBJMAP_ENTRY **iter;
    HRESULT hres;

    TRACE("(%p)\n", module);

    if(!module)
        return E_INVALIDARG;

    for(iter = module->m_ppAutoObjMapFirst; iter < module->m_ppAutoObjMapLast; iter++) {
        hres = CoRevokeClassObject((*iter)->dwRegister);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/***********************************************************************
 *           AtlComModuleUnregisterServer       [atl100.22]
 */
HRESULT WINAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE *mod, BOOL bRegTypeLib, const CLSID *clsid)
{
    const struct _ATL_CATMAP_ENTRY *catmap;
    _ATL_OBJMAP_ENTRY **iter;
    HRESULT hres;

    TRACE("(%p %x %s)\n", mod, bRegTypeLib, debugstr_guid(clsid));

    for(iter = mod->m_ppAutoObjMapFirst; iter < mod->m_ppAutoObjMapLast; iter++) {
        if(!*iter || (clsid && !IsEqualCLSID((*iter)->pclsid, clsid)))
            continue;

        TRACE("Unregistering clsid %s\n", debugstr_guid((*iter)->pclsid));

        catmap = (*iter)->pfnGetCategoryMap();
        if(catmap) {
            hres = AtlRegisterClassCategoriesHelper((*iter)->pclsid, catmap, FALSE);
            if(FAILED(hres))
                return hres;
        }

        hres = (*iter)->pfnUpdateRegistry(FALSE);
        if(FAILED(hres))
            return hres;
    }

    if(bRegTypeLib) {
        ITypeLib *typelib;
        TLIBATTR *attr;
        BSTR path;

        hres = AtlLoadTypeLib(mod->m_hInstTypeLib, NULL, &path, &typelib);
        if(FAILED(hres))
            return hres;

        SysFreeString(path);
        hres = ITypeLib_GetLibAttr(typelib, &attr);
        if(SUCCEEDED(hres)) {
            hres = UnRegisterTypeLib(&attr->guid, attr->wMajorVerNum, attr->wMinorVerNum, attr->lcid, attr->syskind);
            ITypeLib_ReleaseTLibAttr(typelib, attr);
        }
        ITypeLib_Release(typelib);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

#endif

/***********************************************************************
 *           AtlRegisterClassCategoriesHelper          [atl100.49]
 */
HRESULT WINAPI AtlRegisterClassCategoriesHelper(REFCLSID clsid, const struct _ATL_CATMAP_ENTRY *catmap, BOOL reg)
{
    const struct _ATL_CATMAP_ENTRY *iter;
    HRESULT hres;

    TRACE("(%s %p %x)\n", debugstr_guid(clsid), catmap, reg);

    if(!catmap)
        return S_OK;

    if(!catreg) {
        ICatRegister *new_catreg;

        hres = CoCreateInstance(&CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
                &IID_ICatRegister, (void**)&new_catreg);
        if(FAILED(hres))
            return hres;

        if(InterlockedCompareExchangePointer((void**)&catreg, new_catreg, NULL))
            ICatRegister_Release(new_catreg);
    }

    for(iter = catmap; iter->iType != _ATL_CATMAP_ENTRY_END; iter++) {
        CATID catid = *iter->pcatid; /* For stupid lack of const in ICatRegister declaration. */

        if(iter->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED) {
            if(reg)
                hres = ICatRegister_RegisterClassImplCategories(catreg, clsid, 1, &catid);
            else
                hres = ICatRegister_UnRegisterClassImplCategories(catreg, clsid, 1, &catid);
        }else {
            if(reg)
                hres = ICatRegister_RegisterClassReqCategories(catreg, clsid, 1, &catid);
            else
                hres = ICatRegister_UnRegisterClassReqCategories(catreg, clsid, 1, &catid);
        }
        if(FAILED(hres))
            return hres;
    }

    if(!reg) {
        WCHAR reg_path[256] = {'C','L','S','I','D','\\'}, *ptr = reg_path+6;

        static const WCHAR implemented_catW[] =
            {'I','m','p','l','e','m','e','n','t','e','d',' ','C','a','t','e','g','o','r','i','e','s',0};
        static const WCHAR required_catW[] =
            {'R','e','q','u','i','r','e','d',' ','C','a','t','e','g','o','r','i','e','s',0};

        ptr += StringFromGUID2(clsid, ptr, 64)-1;
        *ptr++ = '\\';

        memcpy(ptr, implemented_catW, sizeof(implemented_catW));
        RegDeleteKeyW(HKEY_CLASSES_ROOT, reg_path);

        memcpy(ptr, required_catW, sizeof(required_catW));
        RegDeleteKeyW(HKEY_CLASSES_ROOT, reg_path);
    }

    return S_OK;
}

/***********************************************************************
 *           AtlWaitWithMessageLoop     [atl100.24]
 */
BOOL WINAPI AtlWaitWithMessageLoop(HANDLE handle)
{
    MSG msg;
    DWORD res;

    TRACE("(%p)\n", handle);

    while(1) {
        res = MsgWaitForMultipleObjects(1, &handle, FALSE, INFINITE, QS_ALLINPUT);
        switch(res) {
        case WAIT_OBJECT_0:
            return TRUE;
        case WAIT_OBJECT_0+1:
            if(GetMessageW(&msg, NULL, 0, 0) < 0)
                return FALSE;

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            break;
        default:
            return FALSE;
        }
    }
}

static HRESULT get_default_source(ITypeLib *typelib, const CLSID *clsid, IID *iid)
{
    ITypeInfo *typeinfo, *src_typeinfo = NULL;
    TYPEATTR *attr;
    int type_flags;
    unsigned i;
    HRESULT hres;

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, clsid, &typeinfo);
    if(FAILED(hres))
        return hres;

    hres = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    if(FAILED(hres)) {
        ITypeInfo_Release(typeinfo);
        return hres;
    }

    for(i=0; i < attr->cImplTypes; i++) {
        hres = ITypeInfo_GetImplTypeFlags(typeinfo, i, &type_flags);
        if(SUCCEEDED(hres) && type_flags == (IMPLTYPEFLAG_FSOURCE|IMPLTYPEFLAG_FDEFAULT)) {
            HREFTYPE ref;

            hres = ITypeInfo_GetRefTypeOfImplType(typeinfo, i, &ref);
            if(SUCCEEDED(hres))
                hres = ITypeInfo_GetRefTypeInfo(typeinfo, ref, &src_typeinfo);
            break;
        }
    }

    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
    ITypeInfo_Release(typeinfo);
    if(FAILED(hres))
        return hres;

    if(!src_typeinfo) {
        *iid = IID_NULL;
        return S_OK;
    }

    hres = ITypeInfo_GetTypeAttr(src_typeinfo, &attr);
    if(SUCCEEDED(hres)) {
        *iid = attr->guid;
        ITypeInfo_ReleaseTypeAttr(src_typeinfo, attr);
    }
    ITypeInfo_Release(src_typeinfo);
    return hres;
}

/***********************************************************************
 *           AtlGetObjectSourceInterface    [atl100.54]
 */
HRESULT WINAPI AtlGetObjectSourceInterface(IUnknown *unk, GUID *libid, IID *iid, unsigned short *major, unsigned short *minor)
{
    IProvideClassInfo2 *classinfo;
    ITypeInfo *typeinfo;
    ITypeLib *typelib;
    IPersist *persist;
    IDispatch *disp;
    HRESULT hres;

    TRACE("(%p %p %p %p %p)\n", unk, libid, iid, major, minor);

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres))
        return hres;

    hres = IDispatch_GetTypeInfo(disp, 0, 0, &typeinfo);
    IDispatch_Release(disp);
    if(FAILED(hres))
        return hres;

    hres = ITypeInfo_GetContainingTypeLib(typeinfo, &typelib, 0);
    ITypeInfo_Release(typeinfo);
    if(SUCCEEDED(hres)) {
        TLIBATTR *attr;

        hres = ITypeLib_GetLibAttr(typelib, &attr);
        if(SUCCEEDED(hres)) {
            *libid = attr->guid;
            *major = attr->wMajorVerNum;
            *minor = attr->wMinorVerNum;
            ITypeLib_ReleaseTLibAttr(typelib, attr);
        }else {
            ITypeLib_Release(typelib);
        }
    }
    if(FAILED(hres))
        return hres;

    hres = IUnknown_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&classinfo);
    if(SUCCEEDED(hres)) {
        hres = IProvideClassInfo2_GetGUID(classinfo, GUIDKIND_DEFAULT_SOURCE_DISP_IID, iid);
        IProvideClassInfo2_Release(classinfo);
        ITypeLib_Release(typelib);
        return hres;
    }

    hres = IUnknown_QueryInterface(unk, &IID_IPersist, (void**)&persist);
    if(SUCCEEDED(hres)) {
        CLSID clsid;

        hres = IPersist_GetClassID(persist, &clsid);
        if(SUCCEEDED(hres))
            hres = get_default_source(typelib, &clsid, iid);
        IPersist_Release(persist);
    }

    return hres;
}

#if _ATL_VER >= _ATL_VER90

/***********************************************************************
 *           AtlSetPerUserRegistration [atl100.67]
 */
HRESULT WINAPI AtlSetPerUserRegistration(cpp_bool bEnable)
{
    FIXME("stub: bEnable: %d\n", bEnable);
    return E_NOTIMPL;
}

/***********************************************************************
 *           AtlGetPerUserRegistration  [atl100.68]
 */
HRESULT WINAPI AtlGetPerUserRegistration(cpp_bool *pbEnabled)
{
    FIXME("stub: returning false\n");
    *pbEnabled = 0;
    return S_OK;
}

#endif

/***********************************************************************
 *           AtlGetVersion              [atl100.@]
 */
DWORD WINAPI AtlGetVersion(void *pReserved)
{
    TRACE("version %04x (%p)\n", _ATL_VER, pReserved);
    return _ATL_VER;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        atl_instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        if(catreg)
            ICatRegister_Release(catreg);
    }

    return TRUE;
}
