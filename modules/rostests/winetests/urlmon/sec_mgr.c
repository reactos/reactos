/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
 * Copyright 2009-2011 Detlef Riekenberg
 * Copyright 2011 Thomas Mullaly for CodeWeavers
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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"

#include "initguid.h"

#define URLZONE_CUSTOM  URLZONE_USER_MIN+1
#define URLZONE_CUSTOM2 URLZONE_CUSTOM+1

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define SET_CALLED(func) \
    do { \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(ParseUrl_SECURITY_URL_input);
DEFINE_EXPECT(ParseUrl_SECURITY_URL_input2);
DEFINE_EXPECT(ParseUrl_SECURITY_URL_expected);
DEFINE_EXPECT(ParseUrl_SECURITY_URL_http);
DEFINE_EXPECT(ParseUrl_SECURITY_DOMAIN_expected);
DEFINE_EXPECT(ProcessUrlAction);

static HRESULT (WINAPI *pCoInternetCreateSecurityManager)(IServiceProvider *, IInternetSecurityManager**, DWORD);
static HRESULT (WINAPI *pCoInternetCreateZoneManager)(IServiceProvider *, IInternetZoneManager**, DWORD);
static HRESULT (WINAPI *pCoInternetGetSecurityUrl)(LPCWSTR, LPWSTR*, PSUACTION, DWORD);
static HRESULT (WINAPI *pCoInternetGetSecurityUrlEx)(IUri*, IUri**, PSUACTION, DWORD_PTR);
static HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, IUri**);
static HRESULT (WINAPI *pCoInternetGetSession)(DWORD, IInternetSession**, DWORD);
static HRESULT (WINAPI *pCoInternetIsFeatureEnabled)(INTERNETFEATURELIST, DWORD);
static HRESULT (WINAPI *pCoInternetIsFeatureEnabledForUrl)(INTERNETFEATURELIST, DWORD, LPCWSTR, IInternetSecurityManager*);
static HRESULT (WINAPI *pCoInternetIsFeatureZoneElevationEnabled)(LPCWSTR, LPCWSTR, IInternetSecurityManager*, DWORD);

static const WCHAR url1[] = {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',
        '/','b','l','a','n','k','.','h','t','m',0};
static const WCHAR url2[] = {'i','n','d','e','x','.','h','t','m',0};
static const WCHAR url3[] = {'f','i','l','e',':','/','/','c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url4[] = {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e',
        '%','2','e','j','p','g',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/','w','w','w','.','z','o','n','e','3',
        '.','w','i','n','e','t','e','s','t',0};
static const WCHAR url6[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR url7[] = {'f','t','p',':','/','/','z','o','n','e','3',
        '.','w','i','n','e','t','e','s','t','/','f','i','l','e','.','t','e','s','t',0};
static const WCHAR url8[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
static const WCHAR url9[] = {'h','t','t','p',':','/','/','w','w','w','.','z','o','n','e','3',
        '.','w','i','n','e','t','e','s','t', '/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url10[] = {'f','i','l','e',':','/','/','s','o','m','e','%','2','0','f','i','l','e',
        '.','j','p','g',0};
static const WCHAR url11[] = {'f','i','l','e',':','/','/','c',':','/','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url12[] = {'f','i','l','e',':','/','/','/','c',':','/','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url13[] = {'h','t','t','p',':','g','o','o','g','l','e','.','c','o','m',0};
static const WCHAR url14[] = {'z','i','p',':','t','e','s','t','i','n','g','.','c','o','m','/','t','e','s','t','i','n','g',0};
static const WCHAR url15[] = {'h','t','t','p',':','/','/','g','o','o','g','l','e','.','c','o','m','.','u','k',0};
static const WCHAR url16[] = {'f','i','l','e',':','/','/','/','c',':',0};
static const WCHAR url17[] = {'f','i','l','e',':','/','/','/','c',':','c','\\',0};
static const WCHAR url18[] = {'c',':','\\','t','e','s','t','.','h','t','m',0};

static const WCHAR winetestW[] = {'w','i','n','e','t','e','s','t',0};
static const WCHAR security_urlW[] = {'w','i','n','e','t','e','s','t',':','t','e','s','t','i','n','g',0};
static const WCHAR security_url2W[] = {'w','i','n','e','t','e','s','t',':','t','e','s','t','i','n','g','2',0};
static const WCHAR security_expectedW[] = {'w','i','n','e','t','e','s','t',':','z','i','p',0};
static const WCHAR winetest_to_httpW[] = {'w','i','n','e','t','e','s','t',':','h',0};

static const char *szZoneMapDomainsKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains";
static const char *szInternetSettingsKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";

static const BYTE secid1[] = {'f','i','l','e',':',0,0,0,0};
static const BYTE secid2[] = {'*',':','i','n','d','e','x','.','h','t','m',3,0,0,0};
static const BYTE secid5[] = {'h','t','t','p',':','w','w','w','.','z','o','n','e','3',
        '.','w','i','n','e','t','e','s','t',3,0,0,0};
static const BYTE secid6[] = {'a','b','o','u','t',':','b','l','a','n','k',3,0,0,0};
static const BYTE secid7[] = {'f','t','p',':','z','o','n','e','3',
        '.','w','i','n','e','t','e','s','t',3,0,0,0};
static const BYTE secid10[] =
    {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e','.','j','p','g',3,0,0,0};
static const BYTE secid14[] =
    {'z','i','p',':','t','e','s','t','i','n','g','.','c','o','m','/','t','e','s','t','i','n','g',3,0,0,0};
static const BYTE secid10_2[] =
    {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e','.','j','p','g',3,0,0,0};
static const BYTE secid13[] = {'h','t','t','p',':','c','o','m','.','u','k',3,0,0,0};
static const BYTE secid13_2[] = {'h','t','t','p',':','g','o','o','g','l','e','.','c','o','m','.','u','k',3,0,0,0};

static const GUID CLSID_TestActiveX =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

static int called_securl_http;
static DWORD ProcessUrlAction_policy;

static struct secmgr_test {
    LPCWSTR url;
    DWORD zone;
    HRESULT zone_hres;
    DWORD secid_size;
    const BYTE *secid;
    HRESULT secid_hres;
} secmgr_tests[] = {
    {url1, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url2, 100, 0x80041001, 0, NULL, E_INVALIDARG},
    {url3, 0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url5, 3,   S_OK, sizeof(secid5), secid5, S_OK},
    {url6, 3,   S_OK, sizeof(secid6), secid6, S_OK},
    {url7, 3,   S_OK, sizeof(secid7), secid7, S_OK},
    {url11,0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url12,0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url16,0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url17,0,   S_OK, sizeof(secid1), secid1, S_OK},
    {url18,0,   S_OK, sizeof(secid1), secid1, S_OK}
};

static int strcmp_w(const WCHAR *str1, const WCHAR *str2)
{
    DWORD len1 = lstrlenW(str1);
    DWORD len2 = lstrlenW(str2);

    if(len1!=len2) return 1;
    return memcmp(str1, str2, len1*sizeof(WCHAR));
}


/* Based on RegDeleteTreeW from dlls/advapi32/registry.c */
static LONG myRegDeleteTreeA(HKEY hKey, LPCSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > ARRAY_SIZE(szNameBuf))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = malloc(dwMaxLen)))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExA(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = myRegDeleteTreeA(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyA(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueA(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueA(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        free(lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static HRESULT WINAPI SecurityManager_QueryInterface(IInternetSecurityManager* This,
        REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI SecurityManager_AddRef(IInternetSecurityManager* This)
{
    return 2;
}

static ULONG WINAPI SecurityManager_Release(IInternetSecurityManager* This)
{
    return 1;
}

static HRESULT WINAPI SecurityManager_SetSecuritySite(IInternetSecurityManager* This,
        IInternetSecurityMgrSite *pSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_GetSecuritySite(IInternetSecurityManager* This,
        IInternetSecurityMgrSite **ppSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_MapUrlToZone(IInternetSecurityManager* This,
        LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_GetSecurityId(IInternetSecurityManager* This,
        LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_ProcessUrlAction(IInternetSecurityManager* This,
        LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
        BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    CHECK_EXPECT(ProcessUrlAction);
    ok(dwAction == URLACTION_FEATURE_ZONE_ELEVATION, "dwAction = %lx\n", dwAction);
    ok(cbPolicy == sizeof(DWORD), "cbPolicy = %ld\n", cbPolicy);
    ok(!pContext, "pContext != NULL\n");
    ok(dwFlags == PUAF_NOUI, "dwFlags = %lx\n", dwFlags);
    ok(dwReserved == 0, "dwReserved = %lx\n", dwReserved);

    *pPolicy = ProcessUrlAction_policy;
    return ProcessUrlAction_policy==URLPOLICY_ALLOW ? S_OK : S_FALSE;
}

static HRESULT WINAPI SecurityManager_QueryCustomPolicy(IInternetSecurityManager* This,
        LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy,
        BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_SetZoneMapping(IInternetSecurityManager* This,
        DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecurityManager_GetZoneMappings(IInternetSecurityManager* This,
        DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IInternetSecurityManagerVtbl SecurityManagerVtbl = {
    SecurityManager_QueryInterface,
    SecurityManager_AddRef,
    SecurityManager_Release,
    SecurityManager_SetSecuritySite,
    SecurityManager_GetSecuritySite,
    SecurityManager_MapUrlToZone,
    SecurityManager_GetSecurityId,
    SecurityManager_ProcessUrlAction,
    SecurityManager_QueryCustomPolicy,
    SecurityManager_SetZoneMapping,
    SecurityManager_GetZoneMappings
};

static IInternetSecurityManager security_manager = { &SecurityManagerVtbl };

static void test_SecurityManager(void)
{
    int i;
    IInternetSecurityManager *secmgr = NULL;
    BYTE buf[512];
    DWORD zone, size, policy;
    HRESULT hres;

    if(!pCoInternetCreateSecurityManager) {
        return;
    }

    trace("Testing security manager...\n");

    hres = pCoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    for(i = 0; i < ARRAY_SIZE(secmgr_tests); i++) {
        zone = 100;
        hres = IInternetSecurityManager_MapUrlToZone(secmgr, secmgr_tests[i].url,
                                                     &zone, 0);
        ok(hres == secmgr_tests[i].zone_hres /* IE <=6 */
           || (FAILED(secmgr_tests[i].zone_hres) && hres == E_INVALIDARG), /* IE7 */
           "[%d] MapUrlToZone failed: %08lx, expected %08lx\n",
                i, hres, secmgr_tests[i].zone_hres);
        if(SUCCEEDED(hres))
            ok(zone == secmgr_tests[i].zone, "[%d] zone=%ld, expected %ld\n", i, zone,
               secmgr_tests[i].zone);
        else
            ok(zone == secmgr_tests[i].zone || zone == -1, "[%d] zone=%ld\n", i, zone);

        size = sizeof(buf);
        memset(buf, 0xf0, sizeof(buf));
        hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[i].url,
                buf, &size, 0);
        ok(hres == secmgr_tests[i].secid_hres,
           "[%d] GetSecurityId failed: %08lx, expected %08lx\n",
           i, hres, secmgr_tests[i].secid_hres);
        if(secmgr_tests[i].secid) {
            ok(size == secmgr_tests[i].secid_size, "[%d] size=%ld, expected %ld\n",
                    i, size, secmgr_tests[i].secid_size);
            ok(!memcmp(buf, secmgr_tests[i].secid, size), "[%d] wrong secid\n", i);
        }
    }

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url10, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08lx, expected S_OK\n", hres);
    ok(zone == 3, "zone=%ld, expected 3\n", zone);

    /* win2k3 translates %20 into a space */
    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url10, buf, &size, 0);
    ok(hres == S_OK, "GetSecurityId failed: %08lx, expected S_OK\n", hres);
    ok(size == sizeof(secid10) ||
       size == sizeof(secid10_2), /* win2k3 */
       "size=%ld\n", size);
    ok(!memcmp(buf, secid10, sizeof(secid10)) ||
       !memcmp(buf, secid10_2, sizeof(secid10_2)), /* win2k3 */
       "wrong secid\n");

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url13, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08lx\n", hres);
    ok(zone == URLZONE_INVALID || broken(zone == URLZONE_INTERNET), "zone=%ld\n", zone);

    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url13, buf, &size, 0);
    ok(hres == E_INVALIDARG || broken(hres == S_OK), "GetSecurityId failed: %08lx\n", hres);

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url14, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08lx, expected S_OK\n", hres);
    ok(zone == URLZONE_INTERNET, "zone=%ld\n", zone);

    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url14, buf, &size, 0);
    ok(hres == S_OK, "GetSecurityId failed: %08lx, expected S_OK\n", hres);
    ok(size == sizeof(secid14), "size=%ld\n", size);
    ok(!memcmp(buf, secid14, size), "wrong secid\n");

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, NULL, &zone, 0);
    ok(hres == E_INVALIDARG, "MapUrlToZone failed: %08lx, expected E_INVALIDARG\n", hres);
    ok(zone == 100 || zone == -1, "zone=%ld\n", zone);

    size = sizeof(buf);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, NULL, buf, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  NULL, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  buf, NULL, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, NULL, URLACTION_SCRIPT_RUN, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == E_INVALIDARG, "ProcessUrlAction failed: %08lx, expected E_INVALIDARG\n", hres);

    IInternetSecurityManager_Release(secmgr);
}

/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/mshtml_test.h and dlls/urlmon/tests/sec_mgr.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExA(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
        RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

static void test_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD res, size, policy, reg_policy;
    char buf[10];
    HKEY hkey;
    HRESULT hres;

    /* FIXME: HKEY_CURRENT_USER is most of the time the default but this can be changed on a system.
     * The test should be changed to cope with that, if need be.
     */
    res = RegOpenKeyA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3", &hkey);
    ok(res == ERROR_SUCCESS, "Could not open zone key\n");
    if(res != ERROR_SUCCESS)
        return;

    wsprintfA(buf, "%X", action);
    size = sizeof(DWORD);
    res = RegQueryValueExA(hkey, buf, NULL, NULL, (BYTE*)&reg_policy, &size);
    RegCloseKey(hkey);

    /* Try settings from HKEY_LOCAL_MACHINE. */
    if(res != ERROR_SUCCESS || size != sizeof(DWORD)) {
        res = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3", &hkey);
        ok(res == ERROR_SUCCESS, "Could not open zone key\n");

        size = sizeof(DWORD);
        res = RegQueryValueExA(hkey, buf, NULL, NULL, (BYTE*)&reg_policy, &size);
        RegCloseKey(hkey);
    }

    if(res != ERROR_SUCCESS || size != sizeof(DWORD)) {
        policy = 0xdeadbeef;
        hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                sizeof(WCHAR), NULL, 0, 0, 0);
        ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%lx) got 0x%lx (expected E_FAIL)\n", action, hres);
        ok(policy == 0xdeadbeef, "(%lx) policy=%lx\n", action, policy);

        policy = 0xdeadbeef;
        hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
                sizeof(DWORD), URLZONEREG_DEFAULT);
        ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%lx) got 0x%lx (expected E_FAIL)\n", action, hres);
        ok(policy == 0xdeadbeef, "(%lx) policy=%lx\n", action, policy);
        return;
    }

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08lx\n", hres);
    ok(policy == reg_policy, "(%lx) policy=%lx, expected %lx\n", action, policy, reg_policy);

    if(policy != URLPOLICY_QUERY) {
        if(winetest_interactive || ! is_ie_hardened()) {
            BOOL expect_parse_call = !called_securl_http;

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%lx) failed: %08lx\n", action, hres);
            ok(policy == 0xdeadbeef, "(%lx) policy=%lx\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    2, NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%lx) failed: %08lx\n", action, hres);
            ok(policy == 0xdeadbeef, "(%lx) policy=%lx\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(DWORD), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%lx) failed: %08lx\n", action, hres);
            ok(policy == reg_policy, "(%lx) policy=%lx\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), (BYTE*)0xdeadbeef, 16, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%lx) failed: %08lx\n", action, hres);
            ok(policy == 0xdeadbeef, "(%lx) policy=%lx\n", action, policy);

            policy = 0xdeadbeef;
            if(expect_parse_call)
                SET_EXPECT(ParseUrl_SECURITY_URL_http);
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, winetest_to_httpW, action, (BYTE*)&policy,
                    sizeof(DWORD), NULL, 0, 0, 0);
            if(expect_parse_call)
                CHECK_CALLED(ParseUrl_SECURITY_URL_http);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%lx) failed: %08lx\n", action, hres);
            ok(policy == reg_policy, "(%lx) policy=%lx\n", action, policy);
        }else {
            skip("IE running in Enhanced Security Configuration\n");
        }
    }
}

static void test_special_url_action(IInternetSecurityManager *secmgr, IInternetZoneManager *zonemgr, DWORD action)
{
    DWORD policy;
    HRESULT hres;

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08lx\n", hres);
    ok(policy == URLPOLICY_DISALLOW, "(%lx) policy=%lx, expected URLPOLICY_DISALLOW\n", action, policy);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(DWORD), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%lx) failed: %08lx, expected S_FALSE\n", action, hres);
    ok(policy == URLPOLICY_DISALLOW, "policy = %lx\n", policy);
}

static void test_activex(IInternetSecurityManager *secmgr)
{
    DWORD policy, policy_size;
    struct CONFIRMSAFETY cs;
    BYTE *ppolicy;
    HRESULT hres;

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, URLACTION_ACTIVEX_RUN, (BYTE*)&policy,
            sizeof(DWORD), (BYTE*)&CLSID_TestActiveX, sizeof(CLSID), 0, 0);
    ok(hres == S_OK, "ProcessUrlAction(URLACTION_ACTIVEX_RUN) failed: %08lx\n", hres);
    ok(policy == URLPOLICY_ALLOW || policy == URLPOLICY_DISALLOW, "policy = %lx\n", policy);

    cs.clsid = CLSID_TestActiveX;
    cs.pUnk = (IUnknown*)0xdeadbeef;
    cs.dwFlags = 0;
    hres = IInternetSecurityManager_QueryCustomPolicy(secmgr, url1, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    ok(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "QueryCusromPolicy failed: %08lx\n", hres);
}

static void test_polices(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IInternetSecurityManager *secmgr = NULL;
    HRESULT hres;

    trace("testing polices...\n");

    hres = pCoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);
    hres = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08lx\n", hres);

    test_url_action(secmgr, zonemgr, URLACTION_SCRIPT_RUN);
    test_url_action(secmgr, zonemgr, URLACTION_ACTIVEX_RUN);
    test_url_action(secmgr, zonemgr, URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY);
    test_url_action(secmgr, zonemgr, URLACTION_CHANNEL_SOFTDIST_PERMISSIONS);
    test_url_action(secmgr, zonemgr, 0xdeadbeef);

    test_special_url_action(secmgr, zonemgr, URLACTION_SCRIPT_OVERRIDE_SAFETY);
    test_special_url_action(secmgr, zonemgr, URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY);

    test_activex(secmgr);

    IInternetSecurityManager_Release(secmgr);
    IInternetZoneManager_Release(zonemgr);
}

/* IE (or at least newer versions of it) seem to cache the keys in ZoneMap
 * when urlmon.dll is loaded and it doesn't seem to update its cache, unless
 * SetZoneMapping is used.
 */
static void test_zone_domain_cache(void)
{
    HRESULT hres;
    DWORD res, zone;
    IInternetSecurityManager *secmgr = NULL;
    HKEY domains, domain;

    static const WCHAR testing_domain_urlW[] = {'h','t','t','p',':','/','/','t','e','s','t','i','n','g','.',
            'd','o','m','a','i','n','/',0};

    res = RegOpenKeyA(HKEY_CURRENT_USER, szZoneMapDomainsKey, &domains);
    ok(res == ERROR_SUCCESS, "RegOpenKey failed: %ld\n", res);
    if(res != ERROR_SUCCESS)
        return;

    res = RegCreateKeyA(domains, "testing.domain", &domain);
    ok(res == ERROR_SUCCESS, "RegCreateKey failed: %ld\n", res);
    if(res != ERROR_SUCCESS) {
        RegCloseKey(domains);
        return;
    }

    zone = URLZONE_CUSTOM;
    res = RegSetValueExA(domain, "http", 0, REG_DWORD, (BYTE*)&zone, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "RegSetValueEx failed: %ld\n", res);

    RegCloseKey(domain);

    hres = pCoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);

    zone = URLZONE_INVALID;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, testing_domain_urlW, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08lx\n", hres);
    todo_wine ok(zone == URLZONE_INTERNET, "Got %ld, expected URLZONE_INTERNET\n", zone);

    /* FIXME: Play nice with ZoneMaps that existed before the test is run. */
    res = RegDeleteKeyA(domains, "testing.domain");
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed: %ld\n", res);

    RegCloseKey(domains);
    IInternetSecurityManager_Release(secmgr);
}

typedef struct {
    const char  *domain;
    const char  *subdomain;
    const char  *scheme;
    DWORD       zone;
} zone_domain_mapping;

/* FIXME: Move these into SetZoneMapping tests when the day comes... */
static const zone_domain_mapping zone_domain_mappings[] = {
    /* Implicitly means "*.yabadaba.do". */
    {"yabadaba.do",NULL,"http",URLZONE_CUSTOM},
    /* The '*' doesn't count as a wildcard, since it's not the first component of the subdomain. */
    {"super.cool","testing.*","ftp",URLZONE_CUSTOM2},
    /* The '*' counts since it's the first component of the subdomain. */
    {"super.cool","*.testing","ftp",URLZONE_CUSTOM2},
    /* All known scheme types apply to wildcard schemes. */
    {"tests.test",NULL,"*",URLZONE_CUSTOM},
    /* Due to a defect with how windows checks the mappings, unknown scheme types
     * never seem to get mapped properly. */
    {"tests.test",NULL,"zip",URLZONE_CUSTOM},
    {"www.testing.com",NULL,"http",URLZONE_CUSTOM},
    {"www.testing.com","testing","http",URLZONE_CUSTOM2},
    {"org",NULL,"http",URLZONE_CUSTOM},
    {"org","testing","http",URLZONE_CUSTOM2},
    {"wine.testing",NULL,"*",URLZONE_CUSTOM2}
};

static BOOL register_zone_domains(void)
{
    HKEY domains;
    DWORD res, i;

    /* Some Windows versions don't seem to have a "Domains" key in their HKLM. */
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, szZoneMapDomainsKey, &domains);
    ok(res == ERROR_SUCCESS || broken(res == ERROR_FILE_NOT_FOUND), "RegOpenKey failed: %ld\n", res);
    if(res == ERROR_SUCCESS) {
        HKEY domain;
        DWORD zone = URLZONE_CUSTOM;

        res = RegCreateKeyA(domains, "local.machine", &domain);
        if (res == ERROR_ACCESS_DENIED)
        {
            skip("need admin rights\n");
            RegCloseKey(domains);
            return FALSE;
        }
        ok(res == ERROR_SUCCESS, "RegCreateKey failed: %ld\n", res);

        res = RegSetValueExA(domain, "http", 0, REG_DWORD, (BYTE*)&zone, sizeof(DWORD));
        ok(res == ERROR_SUCCESS, "RegSetValueEx failed: %ld\n", res);

        RegCloseKey(domain);
        RegCloseKey(domains);
    }

    res = RegOpenKeyA(HKEY_CURRENT_USER, szZoneMapDomainsKey, &domains);
    ok(res == ERROR_SUCCESS, "RegOpenKey failed: %ld\n", res);

    for(i = 0; i < ARRAY_SIZE(zone_domain_mappings); ++i) {
        const zone_domain_mapping *test = zone_domain_mappings+i;
        HKEY domain;

        res = RegCreateKeyA(domains, test->domain, &domain);
        ok(res == ERROR_SUCCESS, "RegCreateKey failed with %ld on test %ld\n", res, i);

        /* Only set the value if there's no subdomain. */
        if(!test->subdomain) {
            res = RegSetValueExA(domain, test->scheme, 0, REG_DWORD, (BYTE*)&test->zone, sizeof(DWORD));
            ok(res == ERROR_SUCCESS, "RegSetValueEx failed with %ld on test %ld\n", res, i);
        } else {
            HKEY subdomain;

            res = RegCreateKeyA(domain, test->subdomain, &subdomain);
            ok(res == ERROR_SUCCESS, "RegCreateKey failed with %ld on test %ld\n", res, i);

            res = RegSetValueExA(subdomain, test->scheme, 0, REG_DWORD, (BYTE*)&test->zone, sizeof(DWORD));
            ok(res == ERROR_SUCCESS, "RegSetValueEx failed with %ld on test %ld\n", res, i);

            RegCloseKey(subdomain);
        }

        RegCloseKey(domain);
    }

    RegCloseKey(domains);
    return TRUE;
}

static void unregister_zone_domains(void)
{
    HKEY domains;
    DWORD res, i;

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, szZoneMapDomainsKey, &domains);
    ok(res == ERROR_SUCCESS || broken(res == ERROR_FILE_NOT_FOUND), "RegOpenKey failed: %ld\n", res);
    if(res == ERROR_SUCCESS) {
        RegDeleteKeyA(domains, "local.machine");
        RegCloseKey(domains);
    }

    res = RegOpenKeyA(HKEY_CURRENT_USER, szZoneMapDomainsKey, &domains);
    ok(res == ERROR_SUCCESS, "RegOpenKey failed: %ld\n", res);

    for(i = 0; i < ARRAY_SIZE(zone_domain_mappings); ++i) {
        const zone_domain_mapping *test = zone_domain_mappings+i;

        /* FIXME: Uses the "cludge" approach to remove the test data from the registry!
         *        Although, if domain names are... unique, this shouldn't cause any harm
         *        to keys (if any) that existed before the tests.
         */
        if(test->subdomain) {
            HKEY domain;

            res = RegOpenKeyA(domains, test->domain, &domain);
            if(res == ERROR_SUCCESS) {
                RegDeleteKeyA(domain, test->subdomain);
                RegCloseKey(domain);
            }
        }
        RegDeleteKeyA(domains, test->domain);
    }

    RegCloseKey(domains);
}

static void run_child_process(void)
{
    char cmdline[MAX_PATH];
    char path[MAX_PATH];
    char **argv;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    BOOL ret;

    GetModuleFileNameA(NULL, path, MAX_PATH);

    si.cb = sizeof(si);
    winetest_get_mainargs(&argv);
    sprintf(cmdline, "\"%s\" %s domain_tests", argv[0], argv[1]);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Failed to spawn child process: %lu\n", GetLastError());
    wait_child_process(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

typedef struct {
    const WCHAR  *url;
    DWORD       zone;
    BOOL        todo;
    DWORD       broken_zone;
} zone_mapping_test;

static const zone_mapping_test zone_mapping_tests[] = {
    /* Tests for "yabadaba.do" zone mappings. */
    {L"http://yabadaba.do/", URLZONE_CUSTOM},
    {L"http://google.yabadaba.do/", URLZONE_CUSTOM},
    {L"zip://yabadaba.do/", URLZONE_INTERNET},
    /* Tests for "super.cool" zone mappings. */
    {L"ftp://testing.google.super.cool/", URLZONE_INTERNET},
    {L"ftp://testing.*.super.cool/", URLZONE_CUSTOM2},
    {L"ftp://google.testing.super.cool/", URLZONE_CUSTOM2},
    /* Tests for "tests.test" zone mappings. */
    {L"http://tests.test/", URLZONE_CUSTOM},
    {L"http://www.tests.test/", URLZONE_CUSTOM},
    {L"ftp://tests.test/", URLZONE_CUSTOM},
    {L"ftp://www.tests.test/", URLZONE_CUSTOM},
    {L"test://www.tests.test/", URLZONE_INTERNET},
    {L"test://tests.test/", URLZONE_INTERNET},
    {L"zip://www.tests.test/", URLZONE_INTERNET},
    {L"zip://tests.test/", URLZONE_INTERNET},
    /* Tests for "www.testing.com" zone mappings. */
    {L"http://google.www.testing.com/", URLZONE_INTERNET},
    {L"http://www.testing.com/", URLZONE_CUSTOM, FALSE, URLZONE_INTERNET},
    {L"http://testing.www.testing.com/", URLZONE_CUSTOM2, FALSE, URLZONE_INTERNET},
    /* Tests for "org" zone mappings. */
    {L"http://google.org/", URLZONE_INTERNET, FALSE, URLZONE_CUSTOM},
    {L"http://org/", URLZONE_CUSTOM},
    {L"http://testing.org/", URLZONE_CUSTOM2},
    /* Tests for "wine.testing" mapping */
    {L"*:wine.testing/test", URLZONE_CUSTOM2},
    {L"http://wine.testing/testing", URLZONE_CUSTOM2}
};

static void test_zone_domain_mappings(void)
{
    HRESULT hres;
    DWORD i, res;
    IInternetSecurityManager *secmgr = NULL;
    HKEY domains;
    DWORD zone = URLZONE_INVALID;

    trace("testing zone domain mappings...\n");

    hres = pCoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, szZoneMapDomainsKey, &domains);
    if(res == ERROR_SUCCESS) {
        static const WCHAR local_machineW[] = {'h','t','t','p',':','/','/','t','e','s','t','.','l','o','c','a','l',
                '.','m','a','c','h','i','n','e','/',0};

        hres = IInternetSecurityManager_MapUrlToZone(secmgr, local_machineW, &zone, 0);
        ok(hres == S_OK, "MapUrlToZone failed: %08lx\n", hres);
        ok(zone == URLZONE_CUSTOM, "Expected URLZONE_CUSTOM, but got %ld\n", zone);

        RegCloseKey(domains);
    }

    for(i = 0; i < ARRAY_SIZE(zone_mapping_tests); ++i) {
        const zone_mapping_test *test = zone_mapping_tests+i;
        zone = URLZONE_INVALID;

        hres = IInternetSecurityManager_MapUrlToZone(secmgr, test->url, &zone, 0);
        ok(hres == S_OK, "MapUrlToZone failed: %08lx\n", hres);
        todo_wine_if (test->todo)
            ok(zone == test->zone || broken(test->broken_zone == zone),
                "Expected %ld, but got %ld on test %ld\n", test->zone, zone, i);
    }

    IInternetSecurityManager_Release(secmgr);
}

static void test_zone_domains(void)
{
    if(is_ie_hardened()) {
        skip("IE running in Enhanced Security Configuration\n");
        return;
    } else if(!pCreateUri) {
        win_skip("Skipping zone domain tests, IE too old\n");
        return;
    }

    trace("testing zone domains...\n");

    test_zone_domain_cache();

    if (!register_zone_domains()) return;
    run_child_process();
    unregister_zone_domains();
}

static void test_CoInternetCreateZoneManager(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IUnknown *punk = NULL;
    HRESULT hr;

    trace("simple zone manager tests...\n");

    hr = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%lx\n", hr);
    if (FAILED(hr))
        return;

    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IUnknown, (void **) &punk);
    ok(SUCCEEDED(hr), "got 0x%lx with %p (expected Success)\n", hr, punk);
    if (punk)
        IUnknown_Release(punk);

    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManager, (void **) &punk);
    ok(SUCCEEDED(hr), "got 0x%lx with %p (expected Success)\n", hr, punk);
    if (punk)
        IUnknown_Release(punk);


    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManagerEx, (void **) &punk);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(punk);

        hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManagerEx2, (void **) &punk);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE /* some W2K3 */),
           "got 0x%lx (expected S_OK)\n", hr);
        if (punk)
            IUnknown_Release(punk);
        else
            win_skip("InternetZoneManagerEx2 not supported\n");

    }
    else
        win_skip("InternetZoneManagerEx not supported\n");

    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

}

static void test_CreateZoneEnumerator(void)
{
    IInternetZoneManager *zonemgr = NULL;
    HRESULT hr;
    DWORD dwEnum;
    DWORD dwEnum2;
    DWORD dwCount;
    DWORD dwCount2;

    trace("testing zone enumerator...\n");

    hr = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%lx\n", hr);
    if (FAILED(hr))
        return;

    dwEnum=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, NULL, 0);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef),
        "got 0x%lx with 0x%lx (expected E_INVALIDARG with 0xdeadbeef)\n", hr, dwEnum);

    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, NULL, &dwCount, 0);
    ok((hr == E_INVALIDARG) && (dwCount == 0xdeadbeef),
        "got 0x%lx and 0x%lx (expected E_INVALIDARG and 0xdeadbeef)\n", hr, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0xffffffff);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef) && (dwCount == 0xdeadbeef),
        "got 0x%lx with 0x%lx and 0x%lx (expected E_INVALIDARG with 0xdeadbeef and 0xdeadbeef)\n",
        hr, dwEnum, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 1);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef) && (dwCount == 0xdeadbeef),
        "got 0x%lx with 0x%lx and 0x%lx (expected E_INVALIDARG with 0xdeadbeef and 0xdeadbeef)\n",
        hr, dwEnum, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    /* Normal use */
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    if (SUCCEEDED(hr)) {
        dwEnum2=0xdeadbeef;
        dwCount2=0xdeadbeef;
        hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum2, &dwCount2, 0);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        if (SUCCEEDED(hr)) {
            /* native urlmon has an incrementing counter for dwEnum */
            hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum2);
            ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        }

        hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

        /* Destroy the Enumerator twice is detected and handled in native urlmon */
        hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
        ok((hr == E_INVALIDARG), "got 0x%lx (expected E_INVALIDARG)\n", hr);
    }

    /* ::Release succeed also, when a ::DestroyZoneEnumerator is missing */
    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
}

static void test_GetZoneActionPolicy(void)
{
    IInternetZoneManager *zonemgr = NULL;
    BYTE buf[32];
    HRESULT hres;
    DWORD action = URLACTION_CREDENTIALS_USE; /* Implemented on all IE versions */

    trace("testing GetZoneActionPolixy...\n");

    hres = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08lx\n", hres);
    ok(*(DWORD*)buf == URLPOLICY_CREDENTIALS_SILENT_LOGON_OK ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_MUST_PROMPT_USER ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_ANONYMOUS_ONLY,
            "unexpected policy=%ld\n", *(DWORD*)buf);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, NULL,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            2, URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1fff, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%lx) got 0x%lx (expected E_FAIL)\n", action, hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 13, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08lx, expected E_INVALIDARG\n", hres);

    IInternetZoneManager_Release(zonemgr);
}

static void test_GetZoneAt(void)
{
    IInternetZoneManager *zonemgr = NULL;
    HRESULT hr;
    DWORD dwEnum;
    DWORD dwCount;
    DWORD dwZone;
    DWORD i;

    trace("testing GetZoneAt...\n");

    hr = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%lx\n", hr);
    if (FAILED(hr))
        return;

    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0);
    if (FAILED(hr))
        goto cleanup;

    if (0) {
        /* this crashes with native urlmon */
        IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, 0, NULL);
    }

    dwZone = 0xdeadbeef;
    hr = IInternetZoneManager_GetZoneAt(zonemgr, 0xdeadbeef, 0, &dwZone);
    ok(hr == E_INVALIDARG,
        "got 0x%lx with 0x%lx (expected E_INVALIDARG)\n", hr, dwZone);

    for (i = 0; i < dwCount; i++)
    {
        dwZone = 0xdeadbeef;
        hr = IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, i, &dwZone);
        ok(hr == S_OK, "#%ld: got x%lx with %ld (expected S_OK)\n", i, hr, dwZone);
    }

    dwZone = 0xdeadbeef;
    /* MSDN (index .. must be .. less than or equal to) is wrong */
    hr = IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, dwCount, &dwZone);
    ok(hr == E_INVALIDARG,
        "got 0x%lx with 0x%lx (expected E_INVALIDARG)\n", hr, dwZone);

    hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

cleanup:
    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
}

static void test_GetZoneAttributes(void)
{
    IInternetZoneManager *zonemgr = NULL;
    CHAR buffer [sizeof(ZONEATTRIBUTES) + 32];
    ZONEATTRIBUTES* pZA = (ZONEATTRIBUTES*) buffer;
    HRESULT hr;
    DWORD i;

    trace("testing GetZoneAttributes...\n");

    hr = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%lx\n", hr);
    if (FAILED(hr))
        return;

    /* native urlmon has Zone "0" up to Zone "4" since IE4 */
    for (i = 0; i < 5; i++) {
        memset(buffer, -1, sizeof(buffer));
        hr = IInternetZoneManager_GetZoneAttributes(zonemgr, i, pZA);
        ok(hr == S_OK, "#%ld: got 0x%lx (expected S_OK)\n", i, hr);
    }

    /* IE8 no longer set cbSize */
    memset(buffer, -1, sizeof(buffer));
    pZA->cbSize = 0;
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    ok((pZA->cbSize == 0) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = %ld (expected 0)\n", pZA->cbSize);

    memset(buffer, -1, sizeof(buffer));
    pZA->cbSize = 64;
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    ok((pZA->cbSize == 64) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = %ld (expected 64)\n", pZA->cbSize);

    memset(buffer, -1, sizeof(buffer));
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    ok((pZA->cbSize == 0xffffffff) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = 0x%lx (expected 0xffffffff)\n", pZA->cbSize);

    /* IE8 up to IE10 don't fail on invalid zones */
    memset(buffer, -1, sizeof(buffer));
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0xdeadbeef, pZA);
    ok(hr == S_OK || hr == E_FAIL || hr == E_POINTER,
        "got 0x%lx (expected S_OK or E_FAIL)\n", hr);

    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
}

static void test_SetZoneAttributes(void)
{
    IInternetZoneManager *zonemgr = NULL;
    CHAR buffer [sizeof(ZONEATTRIBUTES) + 16];
    ZONEATTRIBUTES* pZA = (ZONEATTRIBUTES*) buffer;
    CHAR regpath[MAX_PATH];
    HKEY hkey;
    HRESULT hr;
    DWORD res;

    trace("testing SetZoneAttributes...\n");
    hr = pCoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%lx\n", hr);
    if (FAILED(hr))
        return;

    memset(buffer, -1, sizeof(buffer));
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, URLZONE_LOCAL_MACHINE, pZA);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    sprintf(regpath, "%s\\Zones\\%d", szInternetSettingsKey, URLZONE_CUSTOM);
    res = RegCreateKeyA(HKEY_CURRENT_USER, regpath, &hkey);
    RegCloseKey(hkey);

    ok(res == ERROR_SUCCESS, "got %ld (expected ERROR_SUCCESS)\n", res);
    if (res != ERROR_SUCCESS)
        goto cleanup;

    pZA->cbSize = sizeof(ZONEATTRIBUTES);
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, NULL);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    /* normal use */
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, pZA);
    if (hr == E_FAIL) {
        win_skip("SetZoneAttributes not supported: IE too old\n");
        goto cleanup;
    }
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* native urlmon ignores cbSize */
    pZA->cbSize = sizeof(ZONEATTRIBUTES) + sizeof(DWORD);
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, pZA);
    ok(hr == S_OK, "got 0x%lx for sizeof(ZONEATTRIBUTES) + sizeof(DWORD) (expected S_OK)\n", hr);

    pZA->cbSize = sizeof(ZONEATTRIBUTES) - sizeof(DWORD);
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, pZA);
    ok(hr == S_OK, "got 0x%lx for sizeof(ZONEATTRIBUTES) - sizeof(DWORD) (expected S_OK)\n", hr);

    pZA->cbSize = 0;
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, pZA);
    ok(hr == S_OK, "got 0x%lx for size 0 (expected S_OK)\n", hr);

    /* The key for the zone must be present, when calling SetZoneAttributes */
    myRegDeleteTreeA(HKEY_CURRENT_USER, regpath);
    /* E_FAIL is returned from IE6 here, which is reasonable.
       All newer IE return S_OK without saving the zone attributes to the registry.
       This is a Windows bug, but we have to accept that as standard */
    hr = IInternetZoneManager_SetZoneAttributes(zonemgr, URLZONE_CUSTOM, pZA);
    ok((hr == S_OK) || broken(hr == E_FAIL), "got 0x%lx (expected S_OK)\n", hr);

    /* SetZoneAttributes did not create the directory */
    res = RegOpenKeyA(HKEY_CURRENT_USER, regpath, &hkey);
    ok((res == ERROR_FILE_NOT_FOUND) && (hkey == NULL),
        "got %lu with %p (expected ERROR_FILE_NOT_FOUND with NULL)\n", res, hkey);

    if (hkey) RegCloseKey(hkey);

cleanup:
    /* delete zone settings in the registry */
    myRegDeleteTreeA(HKEY_CURRENT_USER, regpath);

    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
}


static void test_InternetSecurityMarshalling(void)
{
    IInternetSecurityManager *secmgr = NULL;
    IUnknown *unk;
    IStream *stream;
    HRESULT hres;

    trace("testing marshalling...\n");

    hres = pCoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSecurityManager_QueryInterface(secmgr, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "QueryInterface returned: %08lx\n", hres);

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal returned: %08lx\n", hres);

    hres = CoMarshalInterface(stream, &IID_IInternetSecurityManager, unk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    /* Not supported in W98 */
    ok(hres == S_OK || broken(hres == REGDB_E_IIDNOTREG),
        "CoMarshalInterface returned: %08lx\n", hres);

    IStream_Release(stream);
    IUnknown_Release(unk);
    IInternetSecurityManager_Release(secmgr);
}

static void test_InternetGetSecurityUrl(void)
{
    const WCHAR url5_out[] = {'h','t','t','p',':','w','w','w','.','z','o','n','e','3',
                              '.','w','i','n','e','t','e','s','t',0};
    const WCHAR url7_out[] = {'f','t','p',':','z','o','n','e','3','.','w','i','n','e','t','e','s','t',0};

    const WCHAR *in[] = {url2, url3, url4, url5, url7, url8, url9, url10};
    const WCHAR *out_default[] = {url2, url3, url4, url5_out, url7_out, url8, url5_out, url10};
    const WCHAR *out_securl[] = {url2, url3, url4, url5, url7, url8, url9, url10};

    WCHAR *sec;
    DWORD i;
    HRESULT hres;

    trace("testing CoInternetGetSecurityUrl...\n");

    for(i = 0; i < ARRAY_SIZE(in); i++) {
        hres = pCoInternetGetSecurityUrl(in[i], &sec, PSU_DEFAULT, 0);
        ok(hres == S_OK, "(%ld) CoInternetGetSecurityUrl returned: %08lx\n", i, hres);
        if(hres == S_OK) {
            ok(!strcmp_w(sec, out_default[i]), "(%ld) Got %s, expected %s\n",
                    i, wine_dbgstr_w(sec), wine_dbgstr_w(out_default[i]));
            CoTaskMemFree(sec);
        }

        hres = pCoInternetGetSecurityUrl(in[i], &sec, PSU_SECURITY_URL_ONLY, 0);
        ok(hres == S_OK, "(%ld) CoInternetGetSecurityUrl returned: %08lx\n", i, hres);
        if(hres == S_OK) {
            ok(!strcmp_w(sec, out_securl[i]), "(%ld) Got %s, expected %s\n",
                    i, wine_dbgstr_w(sec), wine_dbgstr_w(out_securl[i]));
            CoTaskMemFree(sec);
        }
    }

    SET_EXPECT(ParseUrl_SECURITY_URL_input2);
    SET_EXPECT(ParseUrl_SECURITY_URL_expected);
    SET_EXPECT(ParseUrl_SECURITY_DOMAIN_expected);

    hres = pCoInternetGetSecurityUrl(security_url2W, &sec, PSU_DEFAULT, 0);
    ok(hres == S_OK, "CoInternetGetSecurityUrl returned 0x%08lx, expected S_OK\n", hres);

    CHECK_CALLED(ParseUrl_SECURITY_URL_input2);
    CHECK_CALLED(ParseUrl_SECURITY_URL_expected);
    CHECK_CALLED(ParseUrl_SECURITY_DOMAIN_expected);

    ok(!lstrcmpW(security_expectedW, sec), "Expected %s but got %s\n",
       wine_dbgstr_w(security_expectedW), wine_dbgstr_w(sec));
    CoTaskMemFree(sec);
}

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface,
                                                          REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    return 1;
}

static HRESULT WINAPI InternetProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD *pcchResult, DWORD dwReserved)
{
    const WCHAR *ret = NULL;

    ok(pwzResult != NULL, "pwzResult == NULL\n");
    ok(pcchResult != NULL, "pcchResult == NULL\n");
    ok(!dwParseFlags, "Expected 0, but got 0x%08lx\n", dwParseFlags);

    switch(ParseAction) {
    case PARSE_SECURITY_URL:
        if(!strcmp_w(pwzUrl, security_urlW)) {
            CHECK_EXPECT(ParseUrl_SECURITY_URL_input);
            ok(cchResult == lstrlenW(pwzUrl)+1, "Got %ld\n", cchResult);
            ret = security_expectedW;
        } else if(!strcmp_w(pwzUrl, security_url2W)) {
            CHECK_EXPECT(ParseUrl_SECURITY_URL_input2);
            ok(cchResult == lstrlenW(pwzUrl)+1, "Got %ld\n", cchResult);
            ret = security_expectedW;
        } else if(!strcmp_w(pwzUrl, security_expectedW)) {
            CHECK_EXPECT(ParseUrl_SECURITY_URL_expected);
            ok(cchResult == lstrlenW(pwzUrl)+1, "Got %ld\n", cchResult);
            ret = security_expectedW;
        } else if(!strcmp_w(pwzUrl, winetest_to_httpW)) {
            switch(++called_securl_http) {
            case 1:
                ok(cchResult == lstrlenW(pwzUrl)+1, "Got %ld\n", cchResult);
                break;
            case 2:
                CHECK_EXPECT(ParseUrl_SECURITY_URL_http);
                ok(cchResult == lstrlenW(url9)+1, "Got %ld\n", cchResult);
                break;
            default:
                todo_wine CHECK_EXPECT(ParseUrl_SECURITY_URL_http);
            }
            ret = url9;
        } else
            ok(0, "Unexpected call, pwzUrl=%s\n", wine_dbgstr_w(pwzUrl));

        break;
    case PARSE_SECURITY_DOMAIN:

        CHECK_EXPECT(ParseUrl_SECURITY_DOMAIN_expected);

        ok(!strcmp_w(pwzUrl, security_expectedW), "Expected %s but got %s\n",
            wine_dbgstr_w(security_expectedW), wine_dbgstr_w(pwzUrl));
        ok(cchResult == lstrlenW(pwzUrl)+1, "Got %ld\n", cchResult);
        ret = security_expectedW;
        break;
    default:
        ok(0, "Unexpected call, ParseAction=%d pwzUrl=%s\n", ParseAction,
            wine_dbgstr_w(pwzUrl));
    }

    if(!ret)
        return E_FAIL;

    *pcchResult = lstrlenW(ret)+1;
    if(*pcchResult > cchResult)
        return S_FALSE;
    memcpy(pwzResult, ret, (*pcchResult)*sizeof(WCHAR));
    return S_OK;
}

static HRESULT WINAPI InternetProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
        LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_QueryInfo(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer,
        DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl InternetProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    InternetProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    InternetProtocolInfo_QueryInfo
};

static IInternetProtocolInfo protocol_info = { &InternetProtocolInfoVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        *ppv = &protocol_info;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory protocol_cf = { &ClassFactoryVtbl };

static void register_protocols(void)
{
    IInternetSession *session;
    HRESULT hres;

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterNameSpace(session, &protocol_cf, &IID_NULL,
            winetestW, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    IInternetSession_Release(session);
}

static void unregister_protocols(void) {
    IInternetSession *session;
    HRESULT hr;

    hr = pCoInternetGetSession(0, &session, 0);
    ok(hr == S_OK, "CoInternetGetSession failed: 0x%08lx\n", hr);
    if(FAILED(hr))
        return;

    hr = IInternetSession_UnregisterNameSpace(session, &protocol_cf, winetestW);
    ok(hr == S_OK, "UnregisterNameSpace failed: 0x%08lx\n", hr);

    IInternetSession_Release(session);
}

static const struct {
    const WCHAR *uri;
    DWORD       create_flags;
    const char  *security_uri;
    HRESULT     security_hres;
    const WCHAR *default_uri;
    HRESULT     default_hres;
    BOOL        todo;
} sec_url_ex_tests[] = {
    {L"index.htm", Uri_CREATE_ALLOW_RELATIVE, "*:index.html", S_OK, L"*:index.htm", S_OK},
    {L"file://c:\\Index.htm", Uri_CREATE_FILE_USE_DOS_PATH, "file:///c:/Index.htm", S_OK, L"file:///c:/Index.htm", S_OK},
    {L"file:some%20file%2ejpg", 0, NULL, E_INVALIDARG, NULL, E_INVALIDARG},
    {L"file:some file.jpg", 0, NULL, E_INVALIDARG, NULL, E_INVALIDARG},
    {L"http://www.zone3.winetest/", 0, "http://www.zone3.winetest/", S_OK, L"http://www.zone3.winetest/", S_OK},
    {L"about:blank", 0, "about:blank", S_OK, L"about:blank", S_OK},
    {L"ftp://zone3.winetest/file.test", 0, "ftp://zone3.winetest/file.test", S_OK, L"ftp://zone3.winetest/file.test", S_OK},
    {L"test:123abc", 0, "test:123abc", S_OK, L"test:123abc", S_OK},
    {L"http:google.com/test.file", 0, "http:google.com/test.file", S_OK, L"http:google.com/test.file", S_OK},
    {L"ftp://test@ftp.winehq.org/", 0, "ftp://ftp.winehq.org/", S_OK, L"ftp://ftp.winehq.org/", S_OK},
    {L"test://google@ftp.winehq.org/", 0, "test://google@ftp.winehq.org/", S_OK, L"test://google@ftp.winehq.org/", S_OK}
};

static void test_InternetGetSecurityUrlEx(void)
{
    HRESULT hr;
    DWORD i;
    IUri *uri = NULL, *result = NULL;

    trace("testing CoInternetGetSecurityUrlEx...\n");

    hr = pCoInternetGetSecurityUrlEx(NULL, NULL, PSU_DEFAULT, 0);
    ok(hr == E_INVALIDARG, "CoInternetGetSecurityUrlEx returned 0x%08lx, expected E_INVALIDARG\n", hr);

    result = (void*) 0xdeadbeef;
    hr = pCoInternetGetSecurityUrlEx(NULL, &result, PSU_DEFAULT, 0);
    ok(hr == E_INVALIDARG, "CoInternetGetSecurityUrlEx returned 0x%08lx, expected E_INVALIDARG\n", hr);
    ok(result == (void*) 0xdeadbeef, "'result' was %p\n", result);

    for(i = 0; i < ARRAY_SIZE(sec_url_ex_tests); ++i) {
        uri = NULL;

        hr = pCreateUri(sec_url_ex_tests[i].uri, sec_url_ex_tests[i].create_flags, 0, &uri);
        ok(hr == S_OK, "CreateUri returned 0x%08lx on test %ld\n", hr, i);
        if(hr == S_OK) {
            result = NULL;

            hr = pCoInternetGetSecurityUrlEx(uri, &result, PSU_DEFAULT, 0);
            todo_wine_if (sec_url_ex_tests[i].todo) {
                ok(hr == sec_url_ex_tests[i].default_hres,
                    "CoInternetGetSecurityUrlEx returned 0x%08lx, expected 0x%08lx on test %ld\n",
                    hr, sec_url_ex_tests[i].default_hres, i);
            }
            if(SUCCEEDED(hr)) {
                BSTR received;

                hr = IUri_GetDisplayUri(result, &received);
                ok(hr == S_OK, "GetDisplayUri returned 0x%08lx on test %ld\n", hr, i);
                if(hr == S_OK) {
                    todo_wine_if (sec_url_ex_tests[i].todo) {
                        ok(!lstrcmpW(sec_url_ex_tests[i].default_uri, received),
                            "Expected %s but got %s on test %ld\n",
                            wine_dbgstr_w(sec_url_ex_tests[i].default_uri), wine_dbgstr_w(received), i);
                    }
                }
                SysFreeString(received);
            }
            if(result) IUri_Release(result);

            result = NULL;
            hr = pCoInternetGetSecurityUrlEx(uri, &result, PSU_SECURITY_URL_ONLY, 0);
            todo_wine_if (sec_url_ex_tests[i].todo) {
                ok(hr == sec_url_ex_tests[i].default_hres,
                    "CoInternetGetSecurityUrlEx returned 0x%08lx, expected 0x%08lx on test %ld\n",
                    hr, sec_url_ex_tests[i].default_hres, i);
            }
            if(SUCCEEDED(hr)) {
                BSTR received;

                hr = IUri_GetDisplayUri(result, &received);
                ok(hr == S_OK, "GetDisplayUri returned 0x%08lx on test %ld\n", hr, i);
                if(hr == S_OK) {
                    todo_wine_if (sec_url_ex_tests[i].todo) {
                        ok(!lstrcmpW(sec_url_ex_tests[i].default_uri, received),
                            "Expected %s but got %s on test %ld\n",
                            wine_dbgstr_w(sec_url_ex_tests[i].default_uri), wine_dbgstr_w(received), i);
                    }
                }
                SysFreeString(received);
            }
            if(result) IUri_Release(result);
        }

        if(uri) IUri_Release(uri);
    }
}

static void test_InternetGetSecurityUrlEx_Pluggable(void)
{
    HRESULT hr;
    IUri *uri = NULL, *result;

    trace("testing CoInternetGetSecurityUrlEx for pluggable protocols...\n");

    hr = pCreateUri(security_urlW, 0, 0, &uri);
    ok(hr == S_OK, "CreateUri returned 0x%08lx\n", hr);
    if(hr == S_OK) {
        SET_EXPECT(ParseUrl_SECURITY_URL_input);
        SET_EXPECT(ParseUrl_SECURITY_URL_expected);
        SET_EXPECT(ParseUrl_SECURITY_DOMAIN_expected);

        hr = pCoInternetGetSecurityUrlEx(uri, &result, PSU_DEFAULT, 0);
        ok(hr == S_OK, "CoInternetGetSecurityUrlEx returned 0x%08lx, expected S_OK\n", hr);

        CHECK_CALLED(ParseUrl_SECURITY_URL_input);
        CHECK_CALLED(ParseUrl_SECURITY_URL_expected);
        CHECK_CALLED(ParseUrl_SECURITY_DOMAIN_expected);

        if(hr == S_OK) {
            BSTR received = NULL;

            hr = IUri_GetAbsoluteUri(result, &received);
            ok(hr == S_OK, "GetAbsoluteUri returned 0x%08lx\n", hr);
            if(hr == S_OK) {
                ok(!strcmp_w(security_expectedW, received), "Expected %s but got %s\n",
                    wine_dbgstr_w(security_expectedW), wine_dbgstr_w(received));
            }
            SysFreeString(received);
        }
        if(result) IUri_Release(result);

        result = NULL;

        SET_EXPECT(ParseUrl_SECURITY_URL_input);
        SET_EXPECT(ParseUrl_SECURITY_URL_expected);

        hr = pCoInternetGetSecurityUrlEx(uri, &result, PSU_SECURITY_URL_ONLY, 0);
        ok(hr == S_OK, "CoInternetGetSecurityUrlEx returned 0x%08lx, expected S_OK\n", hr);

        CHECK_CALLED(ParseUrl_SECURITY_URL_input);
        CHECK_CALLED(ParseUrl_SECURITY_URL_expected);

        if(hr == S_OK) {
            BSTR received = NULL;

            hr = IUri_GetAbsoluteUri(result, &received);
            ok(hr == S_OK, "GetAbsoluteUri returned 0x%08lx\n", hr);
            if(hr == S_OK) {
                ok(!strcmp_w(security_expectedW, received), "Expected %s but got %s\n",
                    wine_dbgstr_w(security_expectedW), wine_dbgstr_w(received));
            }
            SysFreeString(received);
        }
        if(result) IUri_Release(result);
    }
    if(uri) IUri_Release(uri);
}

static const BYTE secidex2_1[] = {'z','i','p',':','/','/','t','e','s','t','i','n','g','.','c','o','m','/',3,0,0,0};
static const BYTE secidex2_2[] = {'z','i','p',':','t','e','s','t','i','n','g','.','c','o','m',3,0,0,0};
static const BYTE secidex2_3[] = {'*',':','t','e','s','t','i','n','g','.','c','o','m',3,0,0,0};

static const struct {
    const WCHAR  *uri;
    DWORD       create_flags;
    HRESULT     map_hres;
    DWORD       zone;
    BOOL        map_todo;
    const BYTE  *secid;
    DWORD       secid_size;
    HRESULT     secid_hres;
    BOOL        secid_todo;
} sec_mgr_ex2_tests[] = {
    {L"res://mshtml.dll/blank.htm", 0, S_OK, URLZONE_LOCAL_MACHINE, FALSE, secid1, sizeof(secid1), S_OK},
    {L"index.htm", Uri_CREATE_ALLOW_RELATIVE, 0, URLZONE_INTERNET, FALSE, secid2, sizeof(secid2), S_OK},
    {L"file://c:\\Index.html", 0, 0, URLZONE_LOCAL_MACHINE, FALSE, secid1, sizeof(secid1), S_OK},
    {L"http://www.zone3.winetest/", 0, 0, URLZONE_INTERNET, FALSE, secid5, sizeof(secid5), S_OK},
    {L"about:blank", 0, 0, URLZONE_INTERNET, FALSE, secid6, sizeof(secid6), S_OK},
    {L"ftp://zone3.winetest/file.test", 0, 0, URLZONE_INTERNET, FALSE, secid7, sizeof(secid7), S_OK},
    {L"/file/testing/test.test", Uri_CREATE_ALLOW_RELATIVE, 0, URLZONE_INTERNET, FALSE, NULL, 0, E_INVALIDARG},
    {L"zip://testing.com/", 0, 0, URLZONE_INTERNET, FALSE, secidex2_1, sizeof(secidex2_1), S_OK},
    {L"zip:testing.com", 0, 0, URLZONE_INTERNET, FALSE, secidex2_2, sizeof(secidex2_2), S_OK},
    {L"http:google.com", 0, S_OK, URLZONE_INVALID, FALSE, NULL, 0, E_INVALIDARG},
    {L"http:/google.com", 0, S_OK, URLZONE_INVALID, FALSE, NULL, 0, E_INVALIDARG},
    {L"*:/testing", 0, S_OK, URLZONE_INTERNET, FALSE, NULL, 0, E_INVALIDARG},
    {L"*://testing.com", 0, S_OK, URLZONE_INTERNET, FALSE, secidex2_3, sizeof(secidex2_3), S_OK}
};

static void test_SecurityManagerEx2(void)
{
    HRESULT hres;
    DWORD i, zone;
    BYTE buf[512];
    DWORD buf_size = sizeof(buf);
    IInternetSecurityManager *sec_mgr;
    IInternetSecurityManagerEx2 *sec_mgr2;
    IUri *uri = NULL;

    static const WCHAR domainW[] = {'c','o','m','.','u','k',0};

    if(!pCreateUri) {
        win_skip("Skipping SecurityManagerEx2, IE is too old\n");
        return;
    }

    trace("Testing SecurityManagerEx2...\n");

    hres = pCoInternetCreateSecurityManager(NULL, &sec_mgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08lx\n", hres);

    hres = IInternetSecurityManager_QueryInterface(sec_mgr, &IID_IInternetSecurityManagerEx2, (void**)&sec_mgr2);
    ok(hres == S_OK, "QueryInterface(IID_IInternetSecurityManagerEx2) failed: %08lx\n", hres);

    zone = 0xdeadbeef;

    hres = IInternetSecurityManagerEx2_MapUrlToZoneEx2(sec_mgr2, NULL, &zone, 0, NULL, NULL);
    ok(hres == E_INVALIDARG, "MapUrlToZoneEx2 returned %08lx, expected E_INVALIDARG\n", hres);
    ok(zone == URLZONE_INVALID, "zone was %ld\n", zone);

    hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, NULL, buf, &buf_size, 0);
    ok(hres == E_INVALIDARG, "GetSecurityIdEx2 returned %08lx, expected E_INVALIDARG\n", hres);
    ok(buf_size == sizeof(buf), "buf_size was %ld\n", buf_size);

    hres = pCreateUri(url5, 0, 0, &uri);
    ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);

    hres = IInternetSecurityManagerEx2_MapUrlToZoneEx2(sec_mgr2, uri, NULL, 0, NULL, NULL);
    ok(hres == E_INVALIDARG, "MapToUrlZoneEx2 returned %08lx, expected E_INVALIDARG\n", hres);

    buf_size = sizeof(buf);
    hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, uri, NULL, &buf_size, 0);
    ok(hres == E_INVALIDARG || broken(hres == S_OK), "GetSecurityIdEx2 failed: %08lx\n", hres);
    ok(buf_size == sizeof(buf), "bug_size was %ld\n", buf_size);

    hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, uri, buf, NULL, 0);
    ok(hres == E_INVALIDARG, "GetSecurityIdEx2 returned %08lx, expected E_INVALIDARG\n", hres);

    IUri_Release(uri);

    for(i = 0; i < ARRAY_SIZE(sec_mgr_ex2_tests); ++i) {
        uri = NULL;
        zone = URLZONE_INVALID;

        hres = pCreateUri(sec_mgr_ex2_tests[i].uri, sec_mgr_ex2_tests[i].create_flags, 0, &uri);
        ok(hres == S_OK, "CreateUri returned %08lx for '%s'\n", hres, wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));

        hres = IInternetSecurityManagerEx2_MapUrlToZoneEx2(sec_mgr2, uri, &zone, 0, NULL, NULL);
        todo_wine_if (sec_mgr_ex2_tests[i].map_todo) {
            ok(hres == sec_mgr_ex2_tests[i].map_hres, "MapUrlToZoneEx2 returned %08lx, expected %08lx for '%s'\n",
                hres, sec_mgr_ex2_tests[i].map_hres, wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));
            ok(zone == sec_mgr_ex2_tests[i].zone, "Expected zone %ld, but got %ld for '%s'\n", sec_mgr_ex2_tests[i].zone,
                zone, wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));
        }

        buf_size = sizeof(buf);
        memset(buf, 0xf0, buf_size);

        hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, uri, buf, &buf_size, 0);
        todo_wine_if (sec_mgr_ex2_tests[i].secid_todo) {
            ok(hres == sec_mgr_ex2_tests[i].secid_hres, "GetSecurityIdEx2 returned %08lx, expected %08lx on test '%s'\n",
                hres, sec_mgr_ex2_tests[i].secid_hres, wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));
            if(sec_mgr_ex2_tests[i].secid) {
                ok(buf_size == sec_mgr_ex2_tests[i].secid_size, "Got wrong security id size=%ld, expected %ld on test '%s'\n",
                    buf_size, sec_mgr_ex2_tests[i].secid_size, wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));
                ok(!memcmp(buf, sec_mgr_ex2_tests[i].secid, sec_mgr_ex2_tests[i].secid_size), "Got wrong security id on test '%s'\n",
                    wine_dbgstr_w(sec_mgr_ex2_tests[i].uri));
            }
        }

        IUri_Release(uri);
    }

    hres = pCreateUri(url15, 0, 0, &uri);
    ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);

    buf_size = sizeof(buf);
    memset(buf, 0xf0, buf_size);

    hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, uri, buf, &buf_size, (DWORD_PTR)domainW);
    ok(hres == S_OK, "GetSecurityIdEx2 failed: %08lx\n", hres);
    todo_wine ok(buf_size == sizeof(secid13), "buf_size was %ld\n", buf_size);
    todo_wine ok(!memcmp(buf, secid13, sizeof(secid13)), "Got wrong secid\n");

    buf_size = sizeof(buf);
    memset(buf, 0xf0, buf_size);

    hres = IInternetSecurityManagerEx2_GetSecurityIdEx2(sec_mgr2, uri, buf, &buf_size, 0);
    ok(hres == S_OK, "GetSecurityIdEx2 failed: %08lx\n", hres);
    ok(buf_size == sizeof(secid13_2), "buf_size was %ld\n", buf_size);
    ok(!memcmp(buf, secid13_2, sizeof(secid13_2)), "Got wrong secid\n");

    IUri_Release(uri);

    IInternetSecurityManagerEx2_Release(sec_mgr2);
    IInternetSecurityManager_Release(sec_mgr);
}

static void test_CoInternetIsFeatureZoneElevationEnabled(void)
{
    struct {
        const WCHAR *url_from;
        const WCHAR *url_to;
        DWORD flags;
        HRESULT hres;
        DWORD policy_flags;
    } testcases[] = {
        /*  0 */ { L"http://www.winehq.org", L"http://www.winehq.org", 0, S_FALSE, URLPOLICY_ALLOW },
        /*  1 */ { L"http://www.winehq.org", L"http://www.winehq.org", 0, S_OK, URLPOLICY_DISALLOW },
        /*  2 */ { L"http://www.winehq.org", L"http://www.codeweavers.com", 0, S_FALSE, URLPOLICY_ALLOW },
        /*  3 */ { L"http://www.winehq.org", L"http://www.codeweavers.com", 0, S_OK, URLPOLICY_DISALLOW },
        /*  4 */ { L"http://www.winehq.org", L"http://www.winehq.org", GET_FEATURE_FROM_PROCESS, S_FALSE, -1 },
        /*  5 */ { L"http://www.winehq.org", L"http://www.winehq.org/dir", GET_FEATURE_FROM_PROCESS, S_FALSE, -1 },
        /*  6 */ { L"http://www.winehq.org", L"http://www.codeweavers.com", GET_FEATURE_FROM_PROCESS, S_FALSE, -1 },
        /*  7 */ { L"http://www.winehq.org", L"ftp://winehq.org", GET_FEATURE_FROM_PROCESS, S_FALSE, -1 },
        /*  8 */ { L"http://www.winehq.org", L"ftp://winehq.org", GET_FEATURE_FROM_PROCESS|0x100, S_FALSE, URLPOLICY_ALLOW },
        /*  9 */ { L"http://www.winehq.org", L"ftp://winehq.org", GET_FEATURE_FROM_REGISTRY, S_FALSE, URLPOLICY_ALLOW },
    };

    int i;
    HRESULT hres;

    if(!pCoInternetIsFeatureZoneElevationEnabled || !pCoInternetIsFeatureEnabled
            || !pCoInternetIsFeatureEnabledForUrl) {
        win_skip("Skipping CoInternetIsFeatureZoneElevationEnabled tests\n");
        return;
    }


    hres = pCoInternetIsFeatureEnabled(FEATURE_ZONE_ELEVATION, GET_FEATURE_FROM_PROCESS);
    ok(SUCCEEDED(hres), "CoInternetIsFeatureEnabled returned %lx\n", hres);

    trace("Testing CoInternetIsFeatureZoneElevationEnabled... (%lx)\n", hres);

    for(i = 0; i < ARRAY_SIZE(testcases); i++) {
        if(hres==S_OK && testcases[i].flags == GET_FEATURE_FROM_PROCESS)
            testcases[i].policy_flags = URLPOLICY_ALLOW;
    }

    /* IE10 does not seem to use passed ISecurityManager */
    SET_EXPECT(ProcessUrlAction);
    pCoInternetIsFeatureZoneElevationEnabled(url1, url1, &security_manager, 0);
    i = called_ProcessUrlAction;
    SET_CALLED(ProcessUrlAction);
    if(!i) {
        skip("CoInternetIsFeatureZoneElevationEnabled does not use passed ISecurityManager\n");
        return;
    }

    for(i = 0; i < ARRAY_SIZE(testcases); i++) {
        if(testcases[i].policy_flags != -1) {
            ProcessUrlAction_policy = testcases[i].policy_flags;
            SET_EXPECT(ProcessUrlAction);
        }
        hres = pCoInternetIsFeatureZoneElevationEnabled(testcases[i].url_from, testcases[i].url_to,
                &security_manager, testcases[i].flags);
        ok(hres == testcases[i].hres, "%d) CoInternetIsFeatureZoneElevationEnabled returned %lx\n", i, hres);
        if(testcases[i].policy_flags != -1)
            CHECK_CALLED(ProcessUrlAction);

        if(testcases[i].policy_flags != -1)
            SET_EXPECT(ProcessUrlAction);
        hres = pCoInternetIsFeatureEnabledForUrl(FEATURE_ZONE_ELEVATION,
                testcases[i].flags, testcases[i].url_to, &security_manager);
        ok(hres == testcases[i].hres, "%d) CoInternetIsFeatureEnabledForUrl returned %lx\n", i, hres);
        if(testcases[i].policy_flags != -1)
            CHECK_CALLED(ProcessUrlAction);
    }
}

START_TEST(sec_mgr)
{
    HMODULE hurlmon;
    int argc;
    char **argv;

    hurlmon = GetModuleHandleA("urlmon.dll");
    pCoInternetCreateSecurityManager = (void*) GetProcAddress(hurlmon, "CoInternetCreateSecurityManager");
    pCoInternetCreateZoneManager = (void*) GetProcAddress(hurlmon, "CoInternetCreateZoneManager");
    pCoInternetGetSecurityUrl = (void*) GetProcAddress(hurlmon, "CoInternetGetSecurityUrl");
    pCoInternetGetSecurityUrlEx = (void*) GetProcAddress(hurlmon, "CoInternetGetSecurityUrlEx");
    pCreateUri = (void*) GetProcAddress(hurlmon, "CreateUri");
    pCoInternetGetSession = (void*) GetProcAddress(hurlmon, "CoInternetGetSession");
    pCoInternetIsFeatureEnabled = (void*) GetProcAddress(hurlmon, "CoInternetIsFeatureEnabled");
    pCoInternetIsFeatureEnabledForUrl = (void*) GetProcAddress(hurlmon, "CoInternetIsFeatureEnabledForUrl");
    pCoInternetIsFeatureZoneElevationEnabled = (void*) GetProcAddress(hurlmon, "CoInternetIsFeatureZoneElevationEnabled");

    if (!pCoInternetCreateSecurityManager || !pCoInternetCreateZoneManager ||
        !pCoInternetGetSecurityUrl) {
        win_skip("Various CoInternet* functions not present in IE 4.0\n");
        return;
    }

    argc = winetest_get_mainargs(&argv);
    if(argc > 2 && !strcmp(argv[2], "domain_tests")) {
        test_zone_domain_mappings();
        return;
    }

    OleInitialize(NULL);
    register_protocols();

    test_InternetGetSecurityUrl();

    if(!pCoInternetGetSecurityUrlEx || !pCreateUri)
        win_skip("Skipping CoInternetGetSecurityUrlEx tests, IE too old\n");
    else {
        test_InternetGetSecurityUrlEx();
        test_InternetGetSecurityUrlEx_Pluggable();
    }

    test_SecurityManager();
    test_SecurityManagerEx2();
    test_polices();
    test_zone_domains();
    test_CoInternetCreateZoneManager();
    test_CreateZoneEnumerator();
    test_GetZoneActionPolicy();
    test_GetZoneAt();
    test_GetZoneAttributes();
    test_SetZoneAttributes();
    test_InternetSecurityMarshalling();
    test_CoInternetIsFeatureZoneElevationEnabled();

    unregister_protocols();
    OleUninitialize();
}
