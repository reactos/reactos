/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
 * Copyright 2009 Detlef Riekenberg
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
#define NONAMELESSUNION

/* needed for IInternetZoneManagerEx2 */
#define _WIN32_IE 0x0700

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"

#include "initguid.h"

static const WCHAR url1[] = {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',
        '/','b','l','a','n','k','.','h','t','m',0};
static const WCHAR url2[] = {'i','n','d','e','x','.','h','t','m',0};
static const WCHAR url3[] = {'f','i','l','e',':','/','/','c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url4[] = {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e',
        '%','2','e','j','p','g',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g',0};
static const WCHAR url6[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR url7[] = {'f','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',
        'f','i','l','e','.','t','e','s','t',0};
static const WCHAR url8[] = {'t','e','s','t',':','1','2','3','a','b','c',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',
     '/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url10[] = {'f','i','l','e',':','/','/','s','o','m','e','%','2','0','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR url4e[] = {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e',
        '.','j','p','g',0};


static const BYTE secid1[] = {'f','i','l','e',':',0,0,0,0};
static const BYTE secid5[] = {'h','t','t','p',':','w','w','w','.','w','i','n','e','h','q',
    '.','o','r','g',3,0,0,0};
static const BYTE secid6[] = {'a','b','o','u','t',':','b','l','a','n','k',3,0,0,0};
static const BYTE secid7[] = {'f','t','p',':','w','i','n','e','h','q','.','o','r','g',
                              3,0,0,0};
static const BYTE secid10[] =
    {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e','.','j','p','g',3,0,0,0};
static const BYTE secid10_2[] =
    {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e','.','j','p','g',3,0,0,0};

static const GUID CLSID_TestActiveX =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

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
    {url7, 3,   S_OK, sizeof(secid7), secid7, S_OK}
};

static int strcmp_w(const WCHAR *str1, const WCHAR *str2)
{
    DWORD len1 = lstrlenW(str1);
    DWORD len2 = lstrlenW(str2);

    if(len1!=len2) return 1;
    return memcmp(str1, str2, len1*sizeof(WCHAR));
}

static void test_SecurityManager(void)
{
    int i;
    IInternetSecurityManager *secmgr = NULL;
    BYTE buf[512];
    DWORD zone, size, policy;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i < sizeof(secmgr_tests)/sizeof(secmgr_tests[0]); i++) {
        zone = 100;
        hres = IInternetSecurityManager_MapUrlToZone(secmgr, secmgr_tests[i].url,
                                                     &zone, 0);
        ok(hres == secmgr_tests[i].zone_hres /* IE <=6 */
           || (FAILED(secmgr_tests[i].zone_hres) && hres == E_INVALIDARG), /* IE7 */
           "[%d] MapUrlToZone failed: %08x, expected %08x\n",
                i, hres, secmgr_tests[i].zone_hres);
        if(SUCCEEDED(hres))
            ok(zone == secmgr_tests[i].zone, "[%d] zone=%d, expected %d\n", i, zone,
               secmgr_tests[i].zone);
        else
            ok(zone == secmgr_tests[i].zone || zone == -1, "[%d] zone=%d\n", i, zone);

        size = sizeof(buf);
        memset(buf, 0xf0, sizeof(buf));
        hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[i].url,
                buf, &size, 0);
        ok(hres == secmgr_tests[i].secid_hres,
           "[%d] GetSecurityId failed: %08x, expected %08x\n",
           i, hres, secmgr_tests[i].secid_hres);
        if(secmgr_tests[i].secid) {
            ok(size == secmgr_tests[i].secid_size, "[%d] size=%d, expected %d\n",
                    i, size, secmgr_tests[i].secid_size);
            ok(!memcmp(buf, secmgr_tests[i].secid, size), "[%d] wrong secid\n", i);
        }
    }

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, url10, &zone, 0);
    ok(hres == S_OK, "MapUrlToZone failed: %08x, expected S_OK\n", hres);
    ok(zone == 3, "zone=%d, expected 3\n", zone);

    /* win2k3 translates %20 into a space */
    size = sizeof(buf);
    memset(buf, 0xf0, sizeof(buf));
    hres = IInternetSecurityManager_GetSecurityId(secmgr, url10, buf, &size, 0);
    ok(hres == S_OK, "GetSecurityId failed: %08x, expected S_OK\n", hres);
    ok(size == sizeof(secid10) ||
       size == sizeof(secid10_2), /* win2k3 */
       "size=%d\n", size);
    ok(!memcmp(buf, secid10, size) ||
       !memcmp(buf, secid10_2, size), /* win2k3 */
       "wrong secid\n");

    zone = 100;
    hres = IInternetSecurityManager_MapUrlToZone(secmgr, NULL, &zone, 0);
    ok(hres == E_INVALIDARG, "MapUrlToZone failed: %08x, expected E_INVALIDARG\n", hres);
    ok(zone == 100 || zone == -1, "zone=%d\n", zone);

    size = sizeof(buf);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, NULL, buf, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  NULL, &size, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetSecurityManager_GetSecurityId(secmgr, secmgr_tests[1].url,
                                                  buf, NULL, 0);
    ok(hres == E_INVALIDARG,
       "GetSecurityId failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, NULL, URLACTION_SCRIPT_RUN, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == E_INVALIDARG, "ProcessUrlAction failed: %08x, expected E_INVALIDARG\n", hres);

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
    if(res != ERROR_SUCCESS) {
        ok(0, "Could not open zone key\n");
        return;
    }

    wsprintf(buf, "%X", action);
    size = sizeof(DWORD);
    res = RegQueryValueExA(hkey, buf, NULL, NULL, (BYTE*)&reg_policy, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || size != sizeof(DWORD)) {
        policy = 0xdeadbeef;
        hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                sizeof(WCHAR), NULL, 0, 0, 0);
        ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%x) got 0x%x (expected E_FAIL)\n", action, hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);

        policy = 0xdeadbeef;
        hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
                sizeof(DWORD), URLZONEREG_DEFAULT);
        ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%x) got 0x%x (expected E_FAIL)\n", action, hres);
        ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
        return;
    }

    policy = 0xdeadbeef;
    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, (BYTE*)&policy,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == reg_policy, "(%x) policy=%x, expected %x\n", action, policy, reg_policy);

    if(policy != URLPOLICY_QUERY) {
        if(winetest_interactive || ! is_ie_hardened()) {
            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    2, NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(DWORD), NULL, 0, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == reg_policy, "(%x) policy=%x\n", action, policy);

            policy = 0xdeadbeef;
            hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url9, action, (BYTE*)&policy,
                    sizeof(WCHAR), (BYTE*)0xdeadbeef, 16, 0, 0);
            if(reg_policy == URLPOLICY_DISALLOW)
                ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
            else
                ok(hres == S_OK, "ProcessUrlAction(%x) failed: %08x\n", action, hres);
            ok(policy == 0xdeadbeef, "(%x) policy=%x\n", action, policy);
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
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(policy == URLPOLICY_DISALLOW, "(%x) policy=%x, expected URLPOLICY_DISALLOW\n", action, policy);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(WCHAR), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);

    policy = 0xdeadbeef;
    hres = IInternetSecurityManager_ProcessUrlAction(secmgr, url1, action, (BYTE*)&policy,
            sizeof(DWORD), NULL, 0, 0, 0);
    ok(hres == S_FALSE, "ProcessUrlAction(%x) failed: %08x, expected S_FALSE\n", action, hres);
    ok(policy == URLPOLICY_DISALLOW, "policy = %x\n", policy);
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
    ok(hres == S_OK, "ProcessUrlAction(URLACTION_ACTIVEX_RUN) failed: %08x\n", hres);
    ok(policy == URLPOLICY_ALLOW || policy == URLPOLICY_DISALLOW, "policy = %x\n", policy);

    cs.clsid = CLSID_TestActiveX;
    cs.pUnk = (IUnknown*)0xdeadbeef;
    cs.dwFlags = 0;
    hres = IInternetSecurityManager_QueryCustomPolicy(secmgr, url1, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    ok(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "QueryCusromPolicy failed: %08x\n", hres);
}

static void test_polices(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IInternetSecurityManager *secmgr = NULL;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    ok(hres == S_OK, "CoInternetCreateSecurityManager failed: %08x\n", hres);
    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);

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

static void test_CoInternetCreateZoneManager(void)
{
    IInternetZoneManager *zonemgr = NULL;
    IUnknown *punk = NULL;
    HRESULT hr;

    hr = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%x\n", hr);
    if (FAILED(hr))
        return;

    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IUnknown, (void **) &punk);
    ok(SUCCEEDED(hr), "got 0x%x with %p (expected Success)\n", hr, punk);
    if (punk)
        IUnknown_Release(punk);

    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManager, (void **) &punk);
    ok(SUCCEEDED(hr), "got 0x%x with %p (expected Success)\n", hr, punk);
    if (punk)
        IUnknown_Release(punk);


    hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManagerEx, (void **) &punk);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(punk);

        hr = IInternetZoneManager_QueryInterface(zonemgr, &IID_IInternetZoneManagerEx2, (void **) &punk);
        if (punk)
            IUnknown_Release(punk);
        else
            win_skip("InternetZoneManagerEx2 not supported\n");

    }
    else
        win_skip("InternetZoneManagerEx not supported\n");

    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

}

static void test_CreateZoneEnumerator(void)
{
    IInternetZoneManager *zonemgr = NULL;
    HRESULT hr;
    DWORD dwEnum;
    DWORD dwEnum2;
    DWORD dwCount;
    DWORD dwCount2;

    hr = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%x\n", hr);
    if (FAILED(hr))
        return;

    dwEnum=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, NULL, 0);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef),
        "got 0x%x with 0x%x (expected E_INVALIDARG with 0xdeadbeef)\n", hr, dwEnum);

    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, NULL, &dwCount, 0);
    ok((hr == E_INVALIDARG) && (dwCount == 0xdeadbeef),
        "got 0x%x and 0x%x (expected E_INVALIDARG and 0xdeadbeef)\n", hr, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0xffffffff);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef) && (dwCount == 0xdeadbeef),
        "got 0x%x with 0x%x and 0x%x (expected E_INVALIDARG with 0xdeadbeef and 0xdeadbeef)\n",
        hr, dwEnum, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 1);
    ok((hr == E_INVALIDARG) && (dwEnum == 0xdeadbeef) && (dwCount == 0xdeadbeef),
        "got 0x%x with 0x%x and 0x%x (expected E_INVALIDARG with 0xdeadbeef and 0xdeadbeef)\n",
        hr, dwEnum, dwCount);

    dwEnum=0xdeadbeef;
    dwCount=0xdeadbeef;
    /* Normal use */
    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    if (SUCCEEDED(hr)) {
        dwEnum2=0xdeadbeef;
        dwCount2=0xdeadbeef;
        hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum2, &dwCount2, 0);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        if (SUCCEEDED(hr)) {
            /* native urlmon has an incrementing counter for dwEnum */
            hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum2);
            ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        }

        hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

        /* Destroy the Enumerator twice is detected and handled in native urlmon */
        hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
        ok((hr == E_INVALIDARG), "got 0x%x (expected E_INVALIDARG)\n", hr);
    }

    /* ::Release succeed also, when a ::DestroyZoneEnumerator is missing */
    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
}

static void test_GetZoneActionPolicy(void)
{
    IInternetZoneManager *zonemgr = NULL;
    BYTE buf[32];
    HRESULT hres;
    DWORD action = URLACTION_CREDENTIALS_USE; /* Implemented on all IE versions */

    hres = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hres == S_OK, "CoInternetCreateZoneManager failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == S_OK, "GetZoneActionPolicy failed: %08x\n", hres);
    ok(*(DWORD*)buf == URLPOLICY_CREDENTIALS_SILENT_LOGON_OK ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_MUST_PROMPT_USER ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT ||
            *(DWORD*)buf == URLPOLICY_CREDENTIALS_ANONYMOUS_ONLY,
            "unexpected policy=%d\n", *(DWORD*)buf);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, NULL,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, action, buf,
            2, URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 3, 0x1fff, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_FAIL || broken(hres == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)),
            "(0x%x) got 0x%x (expected E_FAIL)\n", action, hres);

    hres = IInternetZoneManager_GetZoneActionPolicy(zonemgr, 13, action, buf,
            sizeof(DWORD), URLZONEREG_DEFAULT);
    ok(hres == E_INVALIDARG, "GetZoneActionPolicy failed: %08x, expected E_INVALIDARG\n", hres);

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

    hr = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%x\n", hr);
    if (FAILED(hr))
        return;

    hr = IInternetZoneManager_CreateZoneEnumerator(zonemgr, &dwEnum, &dwCount, 0);
    if (FAILED(hr))
        goto cleanup;

    if (0) {
        /* this crashes with native urlmon */
        hr = IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, 0, NULL);
    }

    dwZone = 0xdeadbeef;
    hr = IInternetZoneManager_GetZoneAt(zonemgr, 0xdeadbeef, 0, &dwZone);
    ok(hr == E_INVALIDARG,
        "got 0x%x with 0x%x (expected E_INVALIDARG)\n", hr, dwZone);

    for (i = 0; i < dwCount; i++)
    {
        dwZone = 0xdeadbeef;
        hr = IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, i, &dwZone);
        ok(hr == S_OK, "#%d: got x%x with %d (expected S_OK)\n", i, hr, dwZone);
    }

    dwZone = 0xdeadbeef;
    /* MSDN (index .. must be .. less than or equal to) is wrong */
    hr = IInternetZoneManager_GetZoneAt(zonemgr, dwEnum, dwCount, &dwZone);
    ok(hr == E_INVALIDARG,
        "got 0x%x with 0x%x (expected E_INVALIDARG)\n", hr, dwZone);

    hr = IInternetZoneManager_DestroyZoneEnumerator(zonemgr, dwEnum);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

cleanup:
    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
}

static void test_GetZoneAttributes(void)
{
    IInternetZoneManager *zonemgr = NULL;
    CHAR buffer [sizeof(ZONEATTRIBUTES) + 32];
    ZONEATTRIBUTES* pZA = (ZONEATTRIBUTES*) buffer;
    HRESULT hr;
    DWORD i;

    hr = CoInternetCreateZoneManager(NULL, &zonemgr, 0);
    ok(hr == S_OK, "CoInternetCreateZoneManager result: 0x%x\n", hr);
    if (FAILED(hr))
        return;

    /* native urlmon has Zone "0" up to Zone "4" since IE4 */
    for (i = 0; i < 5; i++) {
        memset(buffer, -1, sizeof(buffer));
        hr = IInternetZoneManager_GetZoneAttributes(zonemgr, i, pZA);
        ok(hr == S_OK, "#%d: got 0x%x (expected S_OK)\n", i, hr);
    }

    /* IE8 no longer set cbSize */
    memset(buffer, -1, sizeof(buffer));
    pZA->cbSize = 0;
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    ok((pZA->cbSize == 0) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = %d (expected 0)\n", pZA->cbSize);

    memset(buffer, -1, sizeof(buffer));
    pZA->cbSize = 64;
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    ok((pZA->cbSize == 64) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = %d (expected 64)\n", pZA->cbSize);

    memset(buffer, -1, sizeof(buffer));
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, pZA);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    ok((pZA->cbSize == 0xffffffff) || (pZA->cbSize == sizeof(ZONEATTRIBUTES)),
        "got cbSize = 0x%x (expected 0xffffffff)\n", pZA->cbSize);

    /* IE8 no longer fail on invalid zones */
    memset(buffer, -1, sizeof(buffer));
    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0xdeadbeef, pZA);
    ok(hr == S_OK || (hr == E_FAIL),
        "got 0x%x (expected S_OK or E_FAIL)\n", hr);

    hr = IInternetZoneManager_GetZoneAttributes(zonemgr, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    hr = IInternetZoneManager_Release(zonemgr);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
}

static void test_InternetSecurityMarshalling(void)
{
    IInternetSecurityManager *secmgr = NULL;
    IUnknown *unk;
    IStream *stream;
    HRESULT hres;

    hres = CoInternetCreateSecurityManager(NULL, &secmgr, 0);
    if(FAILED(hres))
        return;

    hres = IInternetSecurityManager_QueryInterface(secmgr, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "QueryInterface returned: %08x\n", hres);

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal returned: %08x\n", hres);

    hres = CoMarshalInterface(stream, &IID_IInternetSecurityManager, unk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hres == S_OK, "CoMarshalInterface returned: %08x\n", hres);

    IStream_Release(stream);
    IUnknown_Release(unk);
    IInternetSecurityManager_Release(secmgr);
}

static void test_InternetGetSecurityUrl(void)
{
    const WCHAR url5_out[] = {'h','t','t','p',':','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
    const WCHAR url7_out[] = {'f','t','p',':','w','i','n','e','h','q','.','o','r','g',0};

    const WCHAR *in[] = {url2, url3, url4, url5, url7, url8, url9, url10};
    const WCHAR *out_default[] = {url2, url3, url4, url5_out, url7_out, url8, url5_out, url10};
    const WCHAR *out_securl[] = {url2, url3, url4, url5, url7, url8, url9, url10};

    WCHAR *sec;
    DWORD i;
    HRESULT hres;

    for(i=0; i<sizeof(in)/sizeof(WCHAR*); i++) {
        hres = CoInternetGetSecurityUrl(in[i], &sec, PSU_DEFAULT, 0);
        ok(hres == S_OK, "(%d) CoInternetGetSecurityUrl returned: %08x\n", i, hres);
        if(hres == S_OK) {
            ok(!strcmp_w(sec, out_default[i]), "(%d) Got %s, expected %s\n",
                    i, wine_dbgstr_w(sec), wine_dbgstr_w(out_default[i]));
            CoTaskMemFree(sec);
        }

        hres = CoInternetGetSecurityUrl(in[i], &sec, PSU_SECURITY_URL_ONLY, 0);
        ok(hres == S_OK, "(%d) CoInternetGetSecurityUrl returned: %08x\n", i, hres);
        if(hres == S_OK) {
            ok(!strcmp_w(sec, out_securl[i]), "(%d) Got %s, expected %s\n",
                    i, wine_dbgstr_w(sec), wine_dbgstr_w(out_securl[i]));
            CoTaskMemFree(sec);
        }
    }
}


START_TEST(sec_mgr)
{
    OleInitialize(NULL);

    test_InternetGetSecurityUrl();
    test_SecurityManager();
    test_polices();
    test_CoInternetCreateZoneManager();
    test_CreateZoneEnumerator();
    test_GetZoneActionPolicy();
    test_GetZoneAt();
    test_GetZoneAttributes();
    test_InternetSecurityMarshalling();

    OleUninitialize();
}
