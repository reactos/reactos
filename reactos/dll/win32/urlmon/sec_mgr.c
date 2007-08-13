/*
 * Internet Security and Zone Manager
 *
 * Copyright (c) 2004 Huw D M Davies
 * Copyright 2004 Jacek Caban
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/debug.h"
#include "ole2.h"
#include "wine/unicode.h"
#include "urlmon.h"
#include "urlmon_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

/***********************************************************************
 *           InternetSecurityManager implementation
 *
 */
typedef struct {
    const IInternetSecurityManagerVtbl* lpInternetSecurityManagerVtbl;

    LONG ref;

    IInternetSecurityMgrSite *mgrsite;
    IInternetSecurityManager *custom_manager;
} SecManagerImpl;

#define SECMGR_THIS(iface) DEFINE_THIS(SecManagerImpl, InternetSecurityManager, iface)

static HRESULT map_url_to_zone(LPCWSTR url, DWORD *zone)
{
    WCHAR schema[64];
    DWORD res, size=0;
    HKEY hkey;
    HRESULT hres;

    static const WCHAR wszZoneMapProtocolKey[] =
        {'S','o','f','t','w','a','r','e','\\',
                    'M','i','c','r','o','s','o','f','t','\\',
                    'W','i','n','d','o','w','s','\\',
                    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
                    'Z','o','n','e','M','a','p','\\',
                    'P','r','o','t','o','c','o','l','D','e','f','a','u','l','t','s',0};
    static const WCHAR wszFile[] = {'f','i','l','e',0};

    *zone = -1;

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, schema, sizeof(schema)/sizeof(WCHAR), &size, 0);
    if(FAILED(hres))
        return hres;
    if(!*schema)
        return 0x80041001;

    /* file protocol is a special case */
    if(!strcmpW(schema, wszFile)) {
        WCHAR path[MAX_PATH];

        hres = CoInternetParseUrl(url, PARSE_PATH_FROM_URL, 0, path,
                sizeof(path)/sizeof(WCHAR), &size, 0);

        if(SUCCEEDED(hres) && strchrW(path, '\\')) {
            *zone = 0;
            return S_OK;
        }
    }

    WARN("domains are not yet implemented\n");

    res = RegOpenKeyW(HKEY_CURRENT_USER, wszZoneMapProtocolKey, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Could not open key %s\n", debugstr_w(wszZoneMapProtocolKey));
        return E_UNEXPECTED;
    }

    size = sizeof(DWORD);
    res = RegQueryValueExW(hkey, schema, NULL, NULL, (PBYTE)zone, &size);
    RegCloseKey(hkey);
    if(res == ERROR_SUCCESS)
        return S_OK;

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, wszZoneMapProtocolKey, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Could not open key %s\n", debugstr_w(wszZoneMapProtocolKey));
        return E_UNEXPECTED;
    }

    size = sizeof(DWORD);
    res = RegQueryValueExW(hkey, schema, NULL, NULL, (PBYTE)zone, &size);
    RegCloseKey(hkey);
    if(res == ERROR_SUCCESS)
        return S_OK;

    *zone = 3;
    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_QueryInterface(IInternetSecurityManager* iface,REFIID riid,void** ppvObject)
{
    SecManagerImpl *This = SECMGR_THIS(iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IInternetSecurityManager, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if (!*ppvObject) {
        WARN("not supported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    /* Query Interface always increases the reference count by one when it is successful */
    IInternetSecurityManager_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI SecManagerImpl_AddRef(IInternetSecurityManager* iface)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, refCount);

    return refCount;
}

static ULONG WINAPI SecManagerImpl_Release(IInternetSecurityManager* iface)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, refCount);

    /* destroy the object if there's no more reference on it */
    if (!refCount){
        if(This->mgrsite)
            IInternetSecurityMgrSite_Release(This->mgrsite);
        if(This->custom_manager)
            IInternetSecurityManager_Release(This->custom_manager);

        HeapFree(GetProcessHeap(),0,This);

        URLMON_UnlockModule();
    }

    return refCount;
}

static HRESULT WINAPI SecManagerImpl_SetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite *pSite)
{
    SecManagerImpl *This = SECMGR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pSite);

    if(This->mgrsite)
        IInternetSecurityMgrSite_Release(This->mgrsite);

    if(This->custom_manager) {
        IInternetSecurityManager_Release(This->custom_manager);
        This->custom_manager = NULL;
    }

    This->mgrsite = pSite;

    if(pSite) {
        IServiceProvider *servprov;
        HRESULT hres;

        IInternetSecurityMgrSite_AddRef(pSite);

        hres = IInternetSecurityMgrSite_QueryInterface(pSite, &IID_IServiceProvider,
                (void**)&servprov);
        if(SUCCEEDED(hres)) {
            IServiceProvider_QueryService(servprov, &SID_SInternetSecurityManager,
                    &IID_IInternetSecurityManager, (void**)&This->custom_manager);
            IServiceProvider_Release(servprov);
        }
    }

    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_GetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite **ppSite)
{
    SecManagerImpl *This = SECMGR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppSite);

    if(!ppSite)
        return E_INVALIDARG;

    if(This->mgrsite)
        IInternetSecurityMgrSite_AddRef(This->mgrsite);

    *ppSite = This->mgrsite;
    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_MapUrlToZone(IInternetSecurityManager *iface,
                                                  LPCWSTR pwszUrl, DWORD *pdwZone,
                                                  DWORD dwFlags)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    LPWSTR url;
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%s %p %08x)\n", iface, debugstr_w(pwszUrl), pdwZone, dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_MapUrlToZone(This->custom_manager,
                pwszUrl, pdwZone, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(!pwszUrl)
        return E_INVALIDARG;

    if(dwFlags)
        FIXME("not supported flags: %08x\n", dwFlags);

    size = (strlenW(pwszUrl)+16) * sizeof(WCHAR);
    url = HeapAlloc(GetProcessHeap(), 0, size);

    hres = CoInternetParseUrl(pwszUrl, PARSE_SECURITY_URL, 0, url, size/sizeof(WCHAR), &size, 0);
    if(FAILED(hres))
        memcpy(url, pwszUrl, size);

    hres = map_url_to_zone(url, pdwZone);

    HeapFree(GetProcessHeap(), 0, url);

    return hres;
}

static HRESULT WINAPI SecManagerImpl_GetSecurityId(IInternetSecurityManager *iface, 
        LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    LPWSTR buf, ptr, ptr2;
    DWORD size, zone, len;
    HRESULT hres;

    static const WCHAR wszFile[] = {'f','i','l','e',':'};

    TRACE("(%p)->(%s %p %p %08lx)\n", iface, debugstr_w(pwszUrl), pbSecurityId,
          pcbSecurityId, dwReserved);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_GetSecurityId(This->custom_manager,
                pwszUrl, pbSecurityId, pcbSecurityId, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(!pwszUrl || !pbSecurityId || !pcbSecurityId)
        return E_INVALIDARG;

    if(dwReserved)
        FIXME("dwReserved is not supported\n");

    len = strlenW(pwszUrl)+1;
    buf = HeapAlloc(GetProcessHeap(), 0, (len+16)*sizeof(WCHAR));

    hres = CoInternetParseUrl(pwszUrl, PARSE_SECURITY_URL, 0, buf, len, &size, 0);
    if(FAILED(hres))
        memcpy(buf, pwszUrl, len*sizeof(WCHAR));

    hres = map_url_to_zone(buf, &zone);
    if(FAILED(hres)) {
        HeapFree(GetProcessHeap(), 0, buf);
        return hres == 0x80041001 ? E_INVALIDARG : hres;
    }

    /* file protocol is a special case */
    if(strlenW(pwszUrl) >= sizeof(wszFile)/sizeof(WCHAR)
            && !memcmp(buf, wszFile, sizeof(wszFile))) {

        static const BYTE secidFile[] = {'f','i','l','e',':'};

        if(*pcbSecurityId < sizeof(secidFile)+sizeof(zone))
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        memcpy(pbSecurityId, secidFile, sizeof(secidFile));
        *(DWORD*)(pbSecurityId+sizeof(secidFile)) = zone;

        *pcbSecurityId = sizeof(secidFile)+sizeof(zone);
        return S_OK;
    }

    ptr = strchrW(buf, ':');
    ptr2 = ++ptr;
    while(*ptr2 == '/')
        ptr2++;
    if(ptr2 != ptr)
        memmove(ptr, ptr2, (strlenW(ptr2)+1)*sizeof(WCHAR));

    ptr = strchrW(ptr, '/');
    if(ptr)
        *ptr = 0;

    len = WideCharToMultiByte(CP_ACP, 0, buf, -1, NULL, 0, NULL, NULL)-1;

    if(len+sizeof(DWORD) > *pcbSecurityId)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    WideCharToMultiByte(CP_ACP, 0, buf, -1, (LPSTR)pbSecurityId, -1, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, buf);

    *(DWORD*)(pbSecurityId+len) = zone;

    *pcbSecurityId = len+sizeof(DWORD);

    return S_OK;
}


static HRESULT WINAPI SecManagerImpl_ProcessUrlAction(IInternetSecurityManager *iface,
                                                      LPCWSTR pwszUrl, DWORD dwAction,
                                                      BYTE *pPolicy, DWORD cbPolicy,
                                                      BYTE *pContext, DWORD cbContext,
                                                      DWORD dwFlags, DWORD dwReserved)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %08x %p %08x %p %08x %08x %08x)\n", iface, debugstr_w(pwszUrl), dwAction,
          pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_ProcessUrlAction(This->custom_manager, pwszUrl, dwAction,
                pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}
                                               

static HRESULT WINAPI SecManagerImpl_QueryCustomPolicy(IInternetSecurityManager *iface,
                                                       LPCWSTR pwszUrl, REFGUID guidKey,
                                                       BYTE **ppPolicy, DWORD *pcbPolicy,
                                                       BYTE *pContext, DWORD cbContext,
                                                       DWORD dwReserved)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %p %p %08x %08x )\n", iface, debugstr_w(pwszUrl), debugstr_guid(guidKey),
          ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_QueryCustomPolicy(This->custom_manager, pwszUrl, guidKey,
                ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_SetZoneMapping(IInternetSecurityManager *iface,
                                                    DWORD dwZone, LPCWSTR pwszPattern, DWORD dwFlags)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%08x %s %08x)\n", iface, dwZone, debugstr_w(pwszPattern),dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_SetZoneMapping(This->custom_manager, dwZone,
                pwszPattern, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetZoneMappings(IInternetSecurityManager *iface,
        DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    SecManagerImpl *This = SECMGR_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%08x %p %08x)\n", iface, dwZone, ppenumString,dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_GetZoneMappings(This->custom_manager, dwZone,
                ppenumString, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}

static const IInternetSecurityManagerVtbl VT_SecManagerImpl =
{
    SecManagerImpl_QueryInterface,
    SecManagerImpl_AddRef,
    SecManagerImpl_Release,
    SecManagerImpl_SetSecuritySite,
    SecManagerImpl_GetSecuritySite,
    SecManagerImpl_MapUrlToZone,
    SecManagerImpl_GetSecurityId,
    SecManagerImpl_ProcessUrlAction,
    SecManagerImpl_QueryCustomPolicy,
    SecManagerImpl_SetZoneMapping,
    SecManagerImpl_GetZoneMappings
};

HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    SecManagerImpl *This;

    TRACE("(%p,%p)\n",pUnkOuter,ppobj);
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    /* Initialize the virtual function table. */
    This->lpInternetSecurityManagerVtbl = &VT_SecManagerImpl;

    This->ref = 1;
    This->mgrsite = NULL;
    This->custom_manager = NULL;

    *ppobj = This;

    URLMON_LockModule();

    return S_OK;
}

/***********************************************************************
 *           InternetZoneManager implementation
 *
 */
typedef struct {
    const IInternetZoneManagerVtbl* lpVtbl;
    LONG ref;
} ZoneMgrImpl;

static HRESULT open_zone_key(DWORD zone, HKEY *hkey, URLZONEREG zone_reg)
{
    static const WCHAR wszZonesKey[] =
        {'S','o','f','t','w','a','r','e','\\',
            'M','i','c','r','o','s','o','f','t','\\',
            'W','i','n','d','o','w','s','\\',
            'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
            'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s','\\',
            'Z','o','n','e','s','\\',0};
    static const WCHAR wszFormat[] = {'%','s','%','l','d',0};

    WCHAR key_name[sizeof(wszZonesKey)/sizeof(WCHAR)+8];
    HKEY parent_key;
    DWORD res;

    switch(zone_reg) {
    case URLZONEREG_DEFAULT: /* FIXME: TEST */
    case URLZONEREG_HKCU:
        parent_key = HKEY_CURRENT_USER;
        break;
    case URLZONEREG_HKLM:
        parent_key = HKEY_LOCAL_MACHINE;
        break;
    default:
        WARN("Unknown URLZONEREG: %d\n", zone_reg);
        return E_FAIL;
    };

    wsprintfW(key_name, wszFormat, wszZonesKey, zone);

    res = RegOpenKeyW(parent_key, key_name, hkey);

    if(res != ERROR_SUCCESS) {
        WARN("RegOpenKey failed\n");
        return E_INVALIDARG;
    }

    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_QueryInterface
 */
static HRESULT WINAPI ZoneMgrImpl_QueryInterface(IInternetZoneManager* iface, REFIID riid, void** ppvObject)
{
    ZoneMgrImpl* This = (ZoneMgrImpl*)iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if(!This || !ppvObject)
        return E_INVALIDARG;

    if(!IsEqualIID(&IID_IUnknown, riid) && !IsEqualIID(&IID_IInternetZoneManager, riid)) {
        FIXME("Unknown interface: %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    *ppvObject = iface;
    IInternetZoneManager_AddRef(iface);

    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_AddRef
 */
static ULONG WINAPI ZoneMgrImpl_AddRef(IInternetZoneManager* iface)
{
    ZoneMgrImpl* This = (ZoneMgrImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

/********************************************************************
 *      IInternetZoneManager_Release
 */
static ULONG WINAPI ZoneMgrImpl_Release(IInternetZoneManager* iface)
{
    ZoneMgrImpl* This = (ZoneMgrImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if(!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
        URLMON_UnlockModule();
    }
    
    return refCount;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneAttributes
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneAttributes(IInternetZoneManager* iface,
                                                    DWORD dwZone,
                                                    ZONEATTRIBUTES* pZoneAttributes)
{
    FIXME("(%p)->(%d %p) stub\n", iface, dwZone, pZoneAttributes);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_SetZoneAttributes
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneAttributes(IInternetZoneManager* iface,
                                                    DWORD dwZone,
                                                    ZONEATTRIBUTES* pZoneAttributes)
{
    FIXME("(%p)->(%08x %p) stub\n", iface, dwZone, pZoneAttributes);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneCustomPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneCustomPolicy(IInternetZoneManager* iface,
                                                      DWORD dwZone,
                                                      REFGUID guidKey,
                                                      BYTE** ppPolicy,
                                                      DWORD* pcbPolicy,
                                                      URLZONEREG ulrZoneReg)
{
    FIXME("(%p)->(%08x %s %p %p %08x) stub\n", iface, dwZone, debugstr_guid(guidKey),
                                                    ppPolicy, pcbPolicy, ulrZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_SetZoneCustomPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneCustomPolicy(IInternetZoneManager* iface,
                                                      DWORD dwZone,
                                                      REFGUID guidKey,
                                                      BYTE* ppPolicy,
                                                      DWORD cbPolicy,
                                                      URLZONEREG ulrZoneReg)
{
    FIXME("(%p)->(%08x %s %p %08x %08x) stub\n", iface, dwZone, debugstr_guid(guidKey),
                                                    ppPolicy, cbPolicy, ulrZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneActionPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneActionPolicy(IInternetZoneManager* iface,
        DWORD dwZone, DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy, URLZONEREG urlZoneReg)
{
    WCHAR action[16];
    HKEY hkey;
    LONG res;
    DWORD size = cbPolicy;
    HRESULT hres;

    static const WCHAR wszFormat[] = {'%','l','X',0};

    TRACE("(%p)->(%d %08x %p %d %d)\n", iface, dwZone, dwAction, pPolicy,
            cbPolicy, urlZoneReg);

    if(!pPolicy)
        return E_INVALIDARG;

    hres = open_zone_key(dwZone, &hkey, urlZoneReg);
    if(FAILED(hres))
        return hres;

    wsprintfW(action, wszFormat, dwAction);

    res = RegQueryValueExW(hkey, action, NULL, NULL, pPolicy, &size);
    if(res == ERROR_MORE_DATA) {
        hres = E_INVALIDARG;
    }else if(res == ERROR_FILE_NOT_FOUND) {
        hres = E_FAIL;
    }else if(res != ERROR_SUCCESS) {
        ERR("RegQueryValue failed: %d\n", res);
        hres = E_UNEXPECTED;
    }

    RegCloseKey(hkey);

    return hres;
}

/********************************************************************
 *      IInternetZoneManager_SetZoneActionPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneActionPolicy(IInternetZoneManager* iface,
                                                      DWORD dwZone,
                                                      DWORD dwAction,
                                                      BYTE* pPolicy,
                                                      DWORD cbPolicy,
                                                      URLZONEREG urlZoneReg)
{
    FIXME("(%p)->(%08x %08x %p %08x %08x) stub\n", iface, dwZone, dwAction, pPolicy,
                                                       cbPolicy, urlZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_PromptAction
 */
static HRESULT WINAPI ZoneMgrImpl_PromptAction(IInternetZoneManager* iface,
                                               DWORD dwAction,
                                               HWND hwndParent,
                                               LPCWSTR pwszUrl,
                                               LPCWSTR pwszText,
                                               DWORD dwPromptFlags)
{
    FIXME("%p %08x %p %s %s %08x\n", iface, dwAction, hwndParent,
          debugstr_w(pwszUrl), debugstr_w(pwszText), dwPromptFlags );
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_LogAction
 */
static HRESULT WINAPI ZoneMgrImpl_LogAction(IInternetZoneManager* iface,
                                            DWORD dwAction,
                                            LPCWSTR pwszUrl,
                                            LPCWSTR pwszText,
                                            DWORD dwLogFlags)
{
    FIXME("(%p)->(%08x %s %s %08x) stub\n", iface, dwAction, debugstr_w(pwszUrl),
                                              debugstr_w(pwszText), dwLogFlags);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_CreateZoneEnumerator
 */
static HRESULT WINAPI ZoneMgrImpl_CreateZoneEnumerator(IInternetZoneManager* iface,
                                                       DWORD* pdwEnum,
                                                       DWORD* pdwCount,
                                                       DWORD dwFlags)
{
    FIXME("(%p)->(%p %p %08x) stub\n", iface, pdwEnum, pdwCount, dwFlags);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneAt
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneAt(IInternetZoneManager* iface,
                                            DWORD dwEnum,
                                            DWORD dwIndex,
                                            DWORD* pdwZone)
{
    FIXME("(%p)->(%08x %08x %p) stub\n", iface, dwEnum, dwIndex, pdwZone);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_DestroyZoneEnumerator
 */
static HRESULT WINAPI ZoneMgrImpl_DestroyZoneEnumerator(IInternetZoneManager* iface,
                                                        DWORD dwEnum)
{
    FIXME("(%p)->(%08x) stub\n", iface, dwEnum);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_CopyTemplatePoliciesToZone
 */
static HRESULT WINAPI ZoneMgrImpl_CopyTemplatePoliciesToZone(IInternetZoneManager* iface,
                                                             DWORD dwTemplate,
                                                             DWORD dwZone,
                                                             DWORD dwReserved)
{
    FIXME("(%p)->(%08x %08x %08x) stub\n", iface, dwTemplate, dwZone, dwReserved);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_Construct
 */
static const IInternetZoneManagerVtbl ZoneMgrImplVtbl = {
    ZoneMgrImpl_QueryInterface,
    ZoneMgrImpl_AddRef,
    ZoneMgrImpl_Release,
    ZoneMgrImpl_GetZoneAttributes,
    ZoneMgrImpl_SetZoneAttributes,
    ZoneMgrImpl_GetZoneCustomPolicy,
    ZoneMgrImpl_SetZoneCustomPolicy,
    ZoneMgrImpl_GetZoneActionPolicy,
    ZoneMgrImpl_SetZoneActionPolicy,
    ZoneMgrImpl_PromptAction,
    ZoneMgrImpl_LogAction,
    ZoneMgrImpl_CreateZoneEnumerator,
    ZoneMgrImpl_GetZoneAt,
    ZoneMgrImpl_DestroyZoneEnumerator,
    ZoneMgrImpl_CopyTemplatePoliciesToZone,
};

HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    ZoneMgrImpl* ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ZoneMgrImpl));

    TRACE("(%p %p)\n", pUnkOuter, ppobj);
    ret->lpVtbl = &ZoneMgrImplVtbl;
    ret->ref = 1;
    *ppobj = (IInternetZoneManager*)ret;

    URLMON_LockModule();

    return S_OK;
}

/***********************************************************************
 *           CoInternetCreateSecurityManager (URLMON.@)
 *
 */
HRESULT WINAPI CoInternetCreateSecurityManager( IServiceProvider *pSP,
    IInternetSecurityManager **ppSM, DWORD dwReserved )
{
    TRACE("%p %p %d\n", pSP, ppSM, dwReserved );

    if(pSP)
        FIXME("pSP not supported\n");

    return SecManagerImpl_Construct(NULL, (void**) ppSM);
}

/********************************************************************
 *      CoInternetCreateZoneManager (URLMON.@)
 */
HRESULT WINAPI CoInternetCreateZoneManager(IServiceProvider* pSP, IInternetZoneManager** ppZM, DWORD dwReserved)
{
    TRACE("(%p %p %x)\n", pSP, ppZM, dwReserved);
    return ZoneMgrImpl_Construct(NULL, (void**)ppZM);
}
