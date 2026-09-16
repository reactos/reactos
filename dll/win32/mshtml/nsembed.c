/*
 * Copyright 2005-2007 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "shlobj.h"
#include "shlwapi.h"

#include "wine/asm.h"
#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);
WINE_DECLARE_DEBUG_CHANNEL(gecko);

#define NS_APPSTARTUPNOTIFIER_CONTRACTID "@mozilla.org/embedcomp/appstartup-notifier;1"
#define NS_WEBBROWSER_CONTRACTID "@mozilla.org/embedding/browser/nsWebBrowser;1"
#define NS_COMMANDPARAMS_CONTRACTID "@mozilla.org/embedcomp/command-params;1"
#define NS_HTMLSERIALIZER_CONTRACTID "@mozilla.org/layout/contentserializer;1?mimetype=text/html"
#define NS_EDITORCONTROLLER_CONTRACTID "@mozilla.org/editor/editorcontroller;1"
#define NS_PREFERENCES_CONTRACTID "@mozilla.org/preferences;1"
#define NS_VARIANT_CONTRACTID "@mozilla.org/variant;1"
#define NS_CATEGORYMANAGER_CONTRACTID "@mozilla.org/categorymanager;1"
#define NS_XMLHTTPREQUEST_CONTRACTID "@mozilla.org/xmlextras/xmlhttprequest;1"

#define PR_UINT32_MAX 0xffffffff

#define NS_STRING_CONTAINER_INIT_DEPEND  0x0002
#define NS_CSTRING_CONTAINER_INIT_DEPEND 0x0002

#ifdef __i386__
#define GECKO_ARCH_STRING "x86"
#elif defined(__x86_64__)
#define GECKO_ARCH_STRING "x86_64"
#else
#define GECKO_ARCH_STRING ""
#endif

#define GECKO_DIR_NAME "wine-gecko-" GECKO_VERSION "-" GECKO_ARCH_STRING

typedef UINT32 PRUint32;

static nsresult (CDECL *NS_InitXPCOM2)(nsIServiceManager**,void*,void*);
static nsresult (CDECL *NS_ShutdownXPCOM)(nsIServiceManager*);
static nsresult (CDECL *NS_GetComponentRegistrar)(nsIComponentRegistrar**);
static nsresult (CDECL *NS_StringContainerInit2)(nsStringContainer*,const PRUnichar*,PRUint32,PRUint32);
static nsresult (CDECL *NS_CStringContainerInit2)(nsCStringContainer*,const char*,PRUint32,PRUint32);
static nsresult (CDECL *NS_StringContainerFinish)(nsStringContainer*);
static nsresult (CDECL *NS_CStringContainerFinish)(nsCStringContainer*);
static nsresult (CDECL *NS_StringSetData)(nsAString*,const PRUnichar*,PRUint32);
static nsresult (CDECL *NS_CStringSetData)(nsACString*,const char*,PRUint32);
static nsresult (CDECL *NS_NewLocalFile)(const nsAString*,cpp_bool,nsIFile**);
static PRUint32 (CDECL *NS_StringGetData)(const nsAString*,const PRUnichar **,cpp_bool*);
static PRUint32 (CDECL *NS_CStringGetData)(const nsACString*,const char**,cpp_bool*);
static cpp_bool (CDECL *NS_StringGetIsVoid)(const nsAString*);
static void* (CDECL *NS_Alloc)(SIZE_T);
static void (CDECL *NS_Free)(void*);

static HINSTANCE xul_handle = NULL;

static nsIServiceManager *pServMgr = NULL;
static nsIComponentManager *pCompMgr = NULL;
static nsICategoryManager *cat_mgr;
static nsIFile *profile_directory, *plugin_directory;

static ATOM browser_class;
static WCHAR gecko_path[MAX_PATH];
static unsigned gecko_path_len;

nsresult create_nsfile(const PRUnichar *path, nsIFile **ret)
{
    nsAString str;
    nsresult nsres;

    nsAString_InitDepend(&str, path);
    nsres = NS_NewLocalFile(&str, FALSE, ret);
    nsAString_Finish(&str);

    if(NS_FAILED(nsres))
        WARN("NS_NewLocalFile failed: %08lx\n", nsres);
    return nsres;
}

typedef struct {
    nsISimpleEnumerator nsISimpleEnumerator_iface;
    LONG ref;
    nsISupports *value;
} nsSingletonEnumerator;

static inline nsSingletonEnumerator *impl_from_nsISimpleEnumerator(nsISimpleEnumerator *iface)
{
    return CONTAINING_RECORD(iface, nsSingletonEnumerator, nsISimpleEnumerator_iface);
}

static nsresult NSAPI nsSingletonEnumerator_QueryInterface(nsISimpleEnumerator *iface, nsIIDRef riid, void **ppv)
{
    nsSingletonEnumerator *This = impl_from_nsISimpleEnumerator(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, ppv);
        *ppv = &This->nsISimpleEnumerator_iface;
    }else if(IsEqualGUID(&IID_nsISimpleEnumerator, riid)) {
        TRACE("(%p)->(IID_nsISimpleEnumerator %p)\n", This, ppv);
        *ppv = &This->nsISimpleEnumerator_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*ppv);
    return NS_OK;
}

static nsrefcnt NSAPI nsSingletonEnumerator_AddRef(nsISimpleEnumerator *iface)
{
    nsSingletonEnumerator *This = impl_from_nsISimpleEnumerator(iface);
    nsrefcnt ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsSingletonEnumerator_Release(nsISimpleEnumerator *iface)
{
    nsSingletonEnumerator *This = impl_from_nsISimpleEnumerator(iface);
    nsrefcnt ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->value)
            nsISupports_Release(This->value);
        free(This);
    }

    return ref;
}

static nsresult NSAPI nsSingletonEnumerator_HasMoreElements(nsISimpleEnumerator *iface, cpp_bool *_retval)
{
    nsSingletonEnumerator *This = impl_from_nsISimpleEnumerator(iface);

    TRACE("(%p)->()\n", This);

    *_retval = This->value != NULL;
    return NS_OK;
}

static nsresult NSAPI nsSingletonEnumerator_GetNext(nsISimpleEnumerator *iface, nsISupports **_retval)
{
    nsSingletonEnumerator *This = impl_from_nsISimpleEnumerator(iface);

    TRACE("(%p)->()\n", This);

    if(!This->value)
        return NS_ERROR_UNEXPECTED;

    *_retval = This->value;
    This->value = NULL;
    return NS_OK;
}

static const nsISimpleEnumeratorVtbl nsSingletonEnumeratorVtbl = {
    nsSingletonEnumerator_QueryInterface,
    nsSingletonEnumerator_AddRef,
    nsSingletonEnumerator_Release,
    nsSingletonEnumerator_HasMoreElements,
    nsSingletonEnumerator_GetNext
};

static nsISimpleEnumerator *create_singleton_enumerator(nsISupports *value)
{
    nsSingletonEnumerator *ret;

    ret = malloc(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->nsISimpleEnumerator_iface.lpVtbl = &nsSingletonEnumeratorVtbl;
    ret->ref = 1;

    if(value)
        nsISupports_AddRef(value);
    ret->value = value;
    return &ret->nsISimpleEnumerator_iface;
}

static nsresult NSAPI nsDirectoryServiceProvider2_QueryInterface(nsIDirectoryServiceProvider2 *iface,
        nsIIDRef riid, void **result)
{
    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIDirectoryServiceProvider, riid)) {
        TRACE("(IID_nsIDirectoryServiceProvider %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIDirectoryServiceProvider2, riid)) {
        TRACE("(IID_nsIDirectoryServiceProvider2 %p)\n", result);
        *result = iface;
    }else {
        WARN("(%s %p)\n", debugstr_guid(riid), result);
        *result = NULL;
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsDirectoryServiceProvider2_AddRef(nsIDirectoryServiceProvider2 *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsDirectoryServiceProvider2_Release(nsIDirectoryServiceProvider2 *iface)
{
    return 1;
}

static nsresult create_profile_directory(void)
{
    WCHAR path[MAX_PATH + ARRAY_SIZE(L"\\wine_gecko")];
    cpp_bool exists;
    nsresult nsres;
    HRESULT hres;

    hres = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
    if(FAILED(hres)) {
        ERR("SHGetFolderPath failed: %08lx\n", hres);
        return NS_ERROR_FAILURE;
    }

    lstrcatW(path, L"\\wine_gecko");
    nsres = create_nsfile(path, &profile_directory);
    if(NS_FAILED(nsres))
        return nsres;

    nsres = nsIFile_Exists(profile_directory, &exists);
    if(NS_FAILED(nsres)) {
        ERR("Exists failed: %08lx\n", nsres);
        return nsres;
    }

    if(!exists) {
        nsres = nsIFile_Create(profile_directory, 1, 0700);
        if(NS_FAILED(nsres))
            ERR("Create failed: %08lx\n", nsres);
    }

    return nsres;
}

static nsresult NSAPI nsDirectoryServiceProvider2_GetFile(nsIDirectoryServiceProvider2 *iface,
        const char *prop, cpp_bool *persistent, nsIFile **_retval)
{
    TRACE("(%s %p %p)\n", debugstr_a(prop), persistent, _retval);

    if(!strcmp(prop, "ProfD")) {
        if(!profile_directory) {
            nsresult nsres;

            nsres = create_profile_directory();
            if(NS_FAILED(nsres))
                return nsres;
        }

        assert(profile_directory != NULL);
        return nsIFile_Clone(profile_directory, _retval);
    }

    *_retval = NULL;
    return NS_ERROR_FAILURE;
}

static nsresult NSAPI nsDirectoryServiceProvider2_GetFiles(nsIDirectoryServiceProvider2 *iface,
        const char *prop, nsISimpleEnumerator **_retval)
{
    TRACE("(%s %p)\n", debugstr_a(prop), _retval);

    if(!strcmp(prop, "APluginsDL")) {
        WCHAR plugin_path[MAX_PATH];
        nsIFile *file;
        int len;
        nsresult nsres;

        if(!plugin_directory) {
            len = GetSystemDirectoryW(plugin_path, ARRAY_SIZE(plugin_path)-ARRAY_SIZE(L"\\gecko\\plugin")+1);
            if(!len)
                return NS_ERROR_UNEXPECTED;

            lstrcpyW(plugin_path+len, L"\\gecko\\plugin");
            nsres = create_nsfile(plugin_path, &plugin_directory);
            if(NS_FAILED(nsres)) {
                *_retval = NULL;
                return nsres;
            }
        }

        nsres = nsIFile_Clone(plugin_directory, &file);
        if(NS_FAILED(nsres))
            return nsres;

        *_retval = create_singleton_enumerator((nsISupports*)file);
        nsIFile_Release(file);
        if(!*_retval)
            return NS_ERROR_OUT_OF_MEMORY;

        return NS_OK;
    }

    *_retval = NULL;
    return NS_ERROR_FAILURE;
}

static const nsIDirectoryServiceProvider2Vtbl nsDirectoryServiceProvider2Vtbl = {
    nsDirectoryServiceProvider2_QueryInterface,
    nsDirectoryServiceProvider2_AddRef,
    nsDirectoryServiceProvider2_Release,
    nsDirectoryServiceProvider2_GetFile,
    nsDirectoryServiceProvider2_GetFiles
};

static nsIDirectoryServiceProvider2 nsDirectoryServiceProvider2 =
    { &nsDirectoryServiceProvider2Vtbl };

static LRESULT WINAPI nsembed_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GeckoBrowser *This;
    nsresult nsres;

    if(msg == WM_CREATE) {
        This = *(GeckoBrowser**)lParam;
        SetPropW(hwnd, L"THIS", This);
    }else {
        This = GetPropW(hwnd, L"THIS");
    }

    switch(msg) {
    case WM_SIZE:
        TRACE("(%p)->(WM_SIZE)\n", This);

        nsres = nsIBaseWindow_SetSize(This->window,
                LOWORD(lParam), HIWORD(lParam), TRUE);
        if(NS_FAILED(nsres))
            WARN("SetSize failed: %08lx\n", nsres);
        break;

    case WM_PARENTNOTIFY:
        TRACE("WM_PARENTNOTIFY %x\n", (unsigned)wParam);

        switch(wParam) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            nsIWebBrowserFocus_Activate(This->focus);
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}


static void register_browser_class(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        CS_DBLCLKS,
        nsembed_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        L"NsContainer",
        NULL,
    };
    wndclass.hInstance = hInst;
    browser_class = RegisterClassExW(&wndclass);
}

#ifndef __REACTOS__
static BOOL install_wine_gecko(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    WCHAR app[MAX_PATH];
    WCHAR *args;
    LONG len;
    BOOL ret;

    static const WCHAR controlW[] = L"\\control.exe";
    static const WCHAR argsW[] = L" appwiz.cpl install_gecko";

    len = GetSystemDirectoryW(app, MAX_PATH-ARRAY_SIZE(controlW));
    memcpy(app+len, controlW, sizeof(controlW));

    args = malloc(len * sizeof(WCHAR) + sizeof(controlW) + sizeof(argsW));
    if(!args)
        return FALSE;

    memcpy(args, app, len*sizeof(WCHAR) + sizeof(controlW));
    memcpy(args + len + ARRAY_SIZE(controlW)-1, argsW, sizeof(argsW));

    TRACE("starting %s\n", debugstr_w(args));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessW(app, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(args);
    if (ret) {
        CloseHandle(pi.hThread);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
    }else {
        WARN("installation failed\n");
    }

    return ret;
}
#endif /* !__REACTOS__ */

static void set_environment(LPCWSTR gre_path)
{
    size_t len, gre_path_len;
    int debug_level = 0;
    WCHAR *path, buf[20];
    const WCHAR *ptr;

    SetEnvironmentVariableW(L"XPCOM_DEBUG_BREAK", L"warn");

    if(TRACE_ON(gecko))
        debug_level = 5;
    else if(WARN_ON(gecko))
        debug_level = 2;
    else if(ERR_ON(gecko))
        debug_level = 1;

    swprintf(buf, ARRAY_SIZE(buf), L"all:%d", debug_level);
    SetEnvironmentVariableW(L"NSPR_LOG_MODULES", buf);

    len = GetEnvironmentVariableW(L"PATH", NULL, 0);
    gre_path_len = lstrlenW(gre_path);
    path = malloc((len + gre_path_len + 1) * sizeof(WCHAR));
    if(!path)
        return;
    GetEnvironmentVariableW(L"PATH", path, len);

    /* We have to modify PATH as xul.dll loads other DLLs from this directory. */
    if(!(ptr = wcsstr(path, gre_path))
       || (ptr > path && *(ptr-1) != ';')
       || (ptr[gre_path_len] && ptr[gre_path_len] != ';')) {
        if(len)
            path[len-1] = ';';
        lstrcpyW(path+len, gre_path);
        SetEnvironmentVariableW(L"PATH", path);
    }
    free(path);
}

static void set_bool_pref(nsIPrefBranch *pref, const char *pref_name, BOOL val)
{
    nsresult nsres;

    nsres = nsIPrefBranch_SetBoolPref(pref, pref_name, val);
    if(NS_FAILED(nsres))
        ERR("Could not set pref %s\n", debugstr_a(pref_name));
}

static void set_int_pref(nsIPrefBranch *pref, const char *pref_name, int val)
{
    nsresult nsres;

    nsres = nsIPrefBranch_SetIntPref(pref, pref_name, val);
    if(NS_FAILED(nsres))
        ERR("Could not set pref %s\n", debugstr_a(pref_name));
}

static void set_string_pref(nsIPrefBranch *pref, const char *pref_name, const char *val)
{
    nsresult nsres;

    nsres = nsIPrefBranch_SetCharPref(pref, pref_name, val);
    if(NS_FAILED(nsres))
        ERR("Could not set pref %s\n", debugstr_a(pref_name));
}

static void set_lang(nsIPrefBranch *pref)
{
    char langs[100];
    DWORD res, size, type;
    HKEY hkey;

    res = RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Internet Explorer\\International", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    size = sizeof(langs);
    res = RegQueryValueExA(hkey, "AcceptLanguage", 0, &type, (LPBYTE)langs, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return;

    TRACE("Setting lang %s\n", debugstr_a(langs));

    set_string_pref(pref, "intl.accept_languages", langs);
}

static void set_preferences(void)
{
    nsIPrefBranch *pref;
    nsresult nsres;

    nsres = nsIServiceManager_GetServiceByContractID(pServMgr, NS_PREFERENCES_CONTRACTID,
            &IID_nsIPrefBranch, (void**)&pref);
    if(NS_FAILED(nsres)) {
        ERR("Could not get preference service: %08lx\n", nsres);
        return;
    }

    set_lang(pref);
    set_bool_pref(pref, "security.warn_entering_secure", FALSE);
    set_bool_pref(pref, "security.warn_submit_insecure", FALSE);
    set_bool_pref(pref, "dom.ipc.plugins.enabled", FALSE);
    set_bool_pref(pref, "layout.css.grid.enabled", TRUE);
    set_int_pref(pref, "layout.spellcheckDefault", 0);

    nsIPrefBranch_Release(pref);
}

static BOOL init_xpcom(const PRUnichar *gre_path)
{
    nsIComponentRegistrar *registrar = NULL;
    nsIFile *gre_dir;
    WCHAR *ptr;
    nsresult nsres;

    nsres = create_nsfile(gre_path, &gre_dir);
    if(NS_FAILED(nsres)) {
        FreeLibrary(xul_handle);
        return FALSE;
    }

    nsres = NS_InitXPCOM2(&pServMgr, gre_dir, (nsIDirectoryServiceProvider*)&nsDirectoryServiceProvider2);
    nsIFile_Release(gre_dir);
    if(NS_FAILED(nsres)) {
        ERR("NS_InitXPCOM2 failed: %08lx\n", nsres);
        FreeLibrary(xul_handle);
        return FALSE;
    }

    lstrcpyW(gecko_path, gre_path);
    for(ptr = gecko_path; *ptr; ptr++) {
        if(*ptr == '\\')
            *ptr = '/';
    }
    gecko_path_len = ptr-gecko_path;

    nsres = nsIServiceManager_QueryInterface(pServMgr, &IID_nsIComponentManager, (void**)&pCompMgr);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIComponentManager: %08lx\n", nsres);

    init_nsio(pCompMgr);
    init_mutation(pCompMgr);
    set_preferences();

    nsres = nsIServiceManager_GetServiceByContractID(pServMgr, NS_CATEGORYMANAGER_CONTRACTID,
            &IID_nsICategoryManager, (void**)&cat_mgr);
    if(NS_FAILED(nsres))
        ERR("Could not get category manager service: %08lx\n", nsres);

    nsres = NS_GetComponentRegistrar(&registrar);
    if(NS_SUCCEEDED(nsres)) {
        register_nsservice(registrar, pServMgr);
        nsIComponentRegistrar_Release(registrar);
    }else {
        ERR("NS_GetComponentRegistrar failed: %08lx\n", nsres);
    }

    init_dispex_cc();
    init_window_cc();

    return TRUE;
}

static BOOL load_xul(WCHAR *gecko_path)
{
    size_t len;

    set_environment(gecko_path);

    TRACE("(%s)\n", debugstr_w(gecko_path));

    len = wcslen(gecko_path);
    wcscpy(gecko_path + len, L"\\xul.dll");
    xul_handle = LoadLibraryExW(gecko_path, 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    gecko_path[len] = 0;
    if(!xul_handle) {
        WARN("Could not load XUL: %ld\n", GetLastError());
        return FALSE;
    }

#define NS_DLSYM(func) \
    func = (void *)GetProcAddress(xul_handle, #func); \
    if(!func) \
        ERR("Could not GetProcAddress(" #func ") failed\n")

    NS_DLSYM(NS_InitXPCOM2);
    NS_DLSYM(NS_ShutdownXPCOM);
    NS_DLSYM(NS_GetComponentRegistrar);
    NS_DLSYM(NS_StringContainerInit2);
    NS_DLSYM(NS_CStringContainerInit2);
    NS_DLSYM(NS_StringContainerFinish);
    NS_DLSYM(NS_CStringContainerFinish);
    NS_DLSYM(NS_StringSetData);
    NS_DLSYM(NS_CStringSetData);
    NS_DLSYM(NS_NewLocalFile);
    NS_DLSYM(NS_StringGetData);
    NS_DLSYM(NS_CStringGetData);
    NS_DLSYM(NS_StringGetIsVoid);
    NS_DLSYM(NS_Alloc);
    NS_DLSYM(NS_Free);
    NS_DLSYM(ccref_incr);
    NS_DLSYM(ccref_decr);
    NS_DLSYM(ccref_init);
    NS_DLSYM(ccp_init);
    NS_DLSYM(describe_cc_node);
    NS_DLSYM(note_cc_edge);

#undef NS_DLSYM

    return init_xpcom(gecko_path);
}

static WCHAR *check_version(const WCHAR *path)
{
    WCHAR *file_name;
    char version[128];
    DWORD read=0;
    size_t len;
    HANDLE hfile;

    if(!wcsncmp(path, L"\\??\\", 4))
        path += 4;
    if(path[1] != ':') {
        TRACE("Skipping %s\n", debugstr_w(path));
        return FALSE; /* Gecko needs to be accessible via dos path */
    }

    len = wcslen(path);
    file_name = malloc((len + 12) * sizeof(WCHAR));
    if(!file_name)
        return NULL;

    PathCanonicalizeW(file_name, path);
    len = lstrlenW(file_name);
    wcscpy(file_name + len, L"\\VERSION");

    hfile = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    file_name[len] = 0;
    if(hfile == INVALID_HANDLE_VALUE) {
        TRACE("%s not found\n", debugstr_w(file_name));
        free(file_name);
        return NULL;
    }

    ReadFile(hfile, version, sizeof(version), &read, NULL);
    version[read] = 0;
    CloseHandle(hfile);

    TRACE("%s: %s\n", debugstr_w(file_name), debugstr_a(version));

    if(strcmp(version, GECKO_VERSION_STRING)) {
        ERR("Unexpected version %s, expected \"%s\"\n", debugstr_a(version),
            GECKO_VERSION_STRING);
        free(file_name);
        return NULL;
    }

    return file_name;
}

static WCHAR *find_wine_gecko_reg(void)
{
    WCHAR buffer[MAX_PATH];
    DWORD res, type, size;
    HKEY hkey;

    /* @@ Wine registry key: HKLM\Software\Wine\MSHTML\<version> */
    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Wine\\MSHTML\\" GECKO_VERSION, &hkey);
    if(res != ERROR_SUCCESS)
        return NULL;

    size = sizeof(buffer);
    res = RegQueryValueExW(hkey, L"GeckoPath", NULL, &type, (LPBYTE)buffer, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    return check_version(buffer);
}

#ifndef __REACTOS__
static WCHAR *strdupWW(const WCHAR *str1, const WCHAR *str2)
{
    size_t len1 = lstrlenW(str1);
    size_t len2 = lstrlenW(str2);
    WCHAR *ret = malloc((len1 + len2 + 1) * sizeof(WCHAR));
    if(!ret) return NULL;
    memcpy(ret, str1, len1 * sizeof(WCHAR));
    memcpy(ret + len1, str2, len2 * sizeof(WCHAR));
    ret[len1 + len2] = 0;
    return ret;
}

static WCHAR *find_wine_gecko_datadir(void)
{
    const WCHAR *data_dir;
    WCHAR *path = NULL, *ret;

    if((data_dir = _wgetenv(L"WINEDATADIR")))
        path = strdupWW(data_dir, L"\\gecko\\" GECKO_DIR_NAME);
    else if((data_dir = _wgetenv(L"WINEBUILDDIR")))
        path = strdupWW(data_dir, L"\\..\\gecko\\" GECKO_DIR_NAME);
    if(!path)
        return NULL;

    ret = check_version(path);
    free(path);
    return ret;
}

static WCHAR *find_wine_gecko_unix(const char *unix_path)
{
    static WCHAR * (CDECL *p_wine_get_dos_file_name)(const char*);
    WCHAR *dos_dir, *ret;

    if(!p_wine_get_dos_file_name) {
        p_wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleA("kernel32"), "wine_get_dos_file_name");
        if(!p_wine_get_dos_file_name)
            return FALSE;
    }

    dos_dir = p_wine_get_dos_file_name(unix_path);
    if(!dos_dir)
        return FALSE;

    ret = check_version(dos_dir);

    HeapFree(GetProcessHeap(), 0, dos_dir);
    return ret;
}
#endif /* !__REACTOS__ */

static CRITICAL_SECTION cs_load_gecko;
static CRITICAL_SECTION_DEBUG cs_load_gecko_dbg =
{
    0, 0, &cs_load_gecko,
    { &cs_load_gecko_dbg.ProcessLocksList, &cs_load_gecko_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": load_gecko") }
};
static CRITICAL_SECTION cs_load_gecko = { &cs_load_gecko_dbg, -1, 0, 0, 0, 0 };

BOOL load_gecko(void)
{
    BOOL ret = FALSE;

    static DWORD loading_thread;

    TRACE("()\n");

    if(!GECKO_ARCH_STRING[0]) {
        FIXME("Wine Gecko is not supported on this architecture.\n");
        return FALSE;
    }

    /* load_gecko may be called recursively */
    if(loading_thread == GetCurrentThreadId())
        return pCompMgr != NULL;

    EnterCriticalSection(&cs_load_gecko);

    if(!loading_thread) {
        WCHAR *gecko_path;

        loading_thread = GetCurrentThreadId();
#ifndef __REACTOS__
        if(!(gecko_path = find_wine_gecko_reg())
           && !(gecko_path = find_wine_gecko_datadir())
           && !(gecko_path = find_wine_gecko_unix(INSTALL_DATADIR "/wine/gecko/" GECKO_DIR_NAME))
           && (!strcmp(INSTALL_DATADIR, "/usr/share") ||
               !(gecko_path = find_wine_gecko_unix("/usr/share/wine/gecko/" GECKO_DIR_NAME)))
           && !(gecko_path = find_wine_gecko_unix("/opt/wine/gecko/" GECKO_DIR_NAME))
           && install_wine_gecko())
#endif
            gecko_path = find_wine_gecko_reg();

        if(gecko_path) {
            ret = load_xul(gecko_path);
            free(gecko_path);
        }else {
           MESSAGE("Could not find Wine Gecko. HTML rendering will be disabled.\n");
        }
    }else {
        FIXME("Gecko can only be used from one thread.\n");
        ret = FALSE;
    }

    LeaveCriticalSection(&cs_load_gecko);

    return ret;
}

void *nsalloc(size_t size)
{
    return NS_Alloc(size);
}

void nsfree(void *mem)
{
    NS_Free(mem);
}

BOOL nsACString_Init(nsACString *str, const char *data)
{
    return NS_SUCCEEDED(NS_CStringContainerInit2(str, data, PR_UINT32_MAX, 0));
}

/*
 * Initializes nsACString with data owned by caller.
 * Caller must ensure that data is valid during lifetime of string object.
 */
void nsACString_InitDepend(nsACString *str, const char *data)
{
    NS_CStringContainerInit2(str, data, PR_UINT32_MAX, NS_CSTRING_CONTAINER_INIT_DEPEND);
}

void nsACString_SetData(nsACString *str, const char *data)
{
    NS_CStringSetData(str, data, PR_UINT32_MAX);
}

UINT32 nsACString_GetData(const nsACString *str, const char **data)
{
    return NS_CStringGetData(str, data, NULL);
}

void nsACString_Finish(nsACString *str)
{
    NS_CStringContainerFinish(str);
}

BOOL nsAString_Init(nsAString *str, const PRUnichar *data)
{
    return NS_SUCCEEDED(NS_StringContainerInit2(str, data, PR_UINT32_MAX, 0));
}

/*
 * Initializes nsAString with data owned by caller.
 * Caller must ensure that data is valid during lifetime of string object.
 */
void nsAString_InitDepend(nsAString *str, const PRUnichar *data)
{
    NS_StringContainerInit2(str, data, PR_UINT32_MAX, NS_STRING_CONTAINER_INIT_DEPEND);
}

UINT32 nsAString_GetData(const nsAString *str, const PRUnichar **data)
{
    return NS_StringGetData(str, data, NULL);
}

void nsAString_SetData(nsAString *str, const PRUnichar *data)
{
    NS_StringSetData(str, data, PR_UINT32_MAX);
}

void nsAString_Finish(nsAString *str)
{
    NS_StringContainerFinish(str);
}

HRESULT map_nsresult(nsresult nsres)
{
    switch(nsres) {
    case NS_OK:
        return S_OK;
    case NS_ERROR_OUT_OF_MEMORY:
        return E_OUTOFMEMORY;
    case NS_ERROR_NOT_IMPLEMENTED:
        return E_NOTIMPL;
    case NS_NOINTERFACE:
        return E_NOINTERFACE;
    case NS_ERROR_INVALID_POINTER:
        return E_POINTER;
    case NS_ERROR_INVALID_ARG:
        return E_INVALIDARG;
    case NS_ERROR_UNEXPECTED:
        return E_UNEXPECTED;
    case NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR:
        return 0x80700007; /* according to tests */
    case NS_ERROR_DOM_SYNTAX_ERR:
        return E_INVALIDARG; /* FIXME: Throw SyntaxError for IE9+ modes */
    case NS_BINDING_ABORTED:
        return E_ABORT;
    }
    return E_FAIL;
}

HRESULT return_nsstr(nsresult nsres, nsAString *nsstr, BSTR *p)
{
    const PRUnichar *str;
    HRESULT hres = S_OK;

    if(NS_FAILED(nsres)) {
        WARN("failed: %08lx\n", nsres);
        nsAString_Finish(nsstr);
        return map_nsresult(nsres);
    }

    nsAString_GetData(nsstr, &str);
    TRACE("ret %s\n", debugstr_w(str));
    if(*str) {
        *p = SysAllocString(str);
        if(!*p)
            hres = E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    nsAString_Finish(nsstr);
    return hres;
}

HRESULT return_nsstr_variant(nsresult nsres, nsAString *nsstr, unsigned flags, VARIANT *p)
{
    HRESULT hres = S_OK;

    if(NS_FAILED(nsres)) {
        ERR("failed: %08lx\n", nsres);
        nsAString_Finish(nsstr);
        return map_nsresult(nsres);
    }

    if(NS_StringGetIsVoid(nsstr)) {
        V_VT(p) = VT_NULL;
    }else {
        const WCHAR *str;
        size_t len;
        nsAString_GetData(nsstr, &str);
        len = wcslen(str);
        if(flags & NSSTR_IMPLICIT_PX) {
            const WCHAR *iter;
            if(len > 2 && !wcscmp(str + len - 2, L"px"))
                len -= 2;
            for(iter = str; iter < str + len && is_digit(*iter); iter++);
            if(*iter == '.') {
                const WCHAR *dot = iter++;
                while(iter < str + len && is_digit(*iter)) iter++;
                if(iter == str + len && dot) len = dot - str;
            }
        }
        if(flags & NSSTR_COLOR) {
            hres = nscolor_to_str(str, &V_BSTR(p));
        }else if(*str) {
            V_BSTR(p) = SysAllocStringLen(str, len);
            if(!V_BSTR(p))
                hres = E_OUTOFMEMORY;
        }else {
            V_BSTR(p) = NULL;
        }
        if(SUCCEEDED(hres))
            V_VT(p) = VT_BSTR;
    }

    nsAString_Finish(nsstr);
    if(FAILED(hres))
        return hres;
    TRACE("ret %s\n", debugstr_variant(p));
    return S_OK;
}

/*
 * Converts VARIANT to string and stores the result in nsAString. To avoid useless
 * allocations, the function uses an existing string if available, so caller must
 * ensure that passed VARIANT is unchanged as long as its string representation is used
 */
HRESULT variant_to_nsstr(VARIANT *v, BOOL hex_int, nsAString *nsstr)
{
    WCHAR buf[32];

    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
        nsAString_InitDepend(nsstr, NULL);
        return S_OK;

    case VT_BSTR:
        nsAString_InitDepend(nsstr, V_BSTR(v));
        break;

    case VT_BSTR|VT_BYREF:
        nsAString_InitDepend(nsstr, *V_BSTRREF(v));
        break;

    case VT_I4:
        wsprintfW(buf, hex_int ? L"#%06x" : L"%d", V_I4(v));
        nsAString_Init(nsstr, buf);
        break;

    case VT_R8:
    case VT_DISPATCH: {
        VARIANT strv;
        HRESULT hres;

        V_VT(&strv) = VT_EMPTY;
        hres = VariantChangeTypeEx(&strv, v, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT),
                                   0, VT_BSTR);
        if(FAILED(hres))
            return hres;

        nsAString_Init(nsstr, V_BSTR(&strv));
        SysFreeString(V_BSTR(&strv));
        break;
    }

    default:
        FIXME("not implemented for %s\n", debugstr_variant(v));
        return E_NOTIMPL;

    }
    return S_OK;
}

nsICommandParams *create_nscommand_params(void)
{
    nsICommandParams *ret = NULL;
    nsresult nsres;

    if(!pCompMgr)
        return NULL;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_COMMANDPARAMS_CONTRACTID, NULL, &IID_nsICommandParams,
            (void**)&ret);
    if(NS_FAILED(nsres))
        ERR("Could not get nsICommandParams\n");

    return ret;
}

nsIWritableVariant *create_nsvariant(void)
{
    nsIWritableVariant *ret = NULL;
    nsresult nsres;

    if(!pCompMgr)
        return NULL;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_VARIANT_CONTRACTID, NULL, &IID_nsIWritableVariant, (void**)&ret);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIVariant\n");

    return ret;
}

char *get_nscategory_entry(const char *category, const char *entry)
{
    char *ret = NULL;
    nsresult nsres;

    nsres = nsICategoryManager_GetCategoryEntry(cat_mgr, category, entry, &ret);
    return NS_SUCCEEDED(nsres) ? ret : NULL;
}

nsresult get_nsinterface(nsISupports *iface, REFIID riid, void **ppv)
{
    nsIInterfaceRequestor *iface_req;
    nsresult nsres;

    nsres = nsISupports_QueryInterface(iface, &IID_nsIInterfaceRequestor, (void**)&iface_req);
    if(NS_FAILED(nsres))
        return nsres;

    nsres = nsIInterfaceRequestor_GetInterface(iface_req, riid, ppv);
    nsIInterfaceRequestor_Release(iface_req);

    return nsres;
}

static HRESULT nsnode_to_nsstring_rec(nsIContentSerializer *serializer, nsIDOMNode *nsnode, nsAString *str)
{
    nsIDOMNodeList *node_list = NULL;
    cpp_bool has_children = FALSE;
    nsIContent *nscontent;
    UINT16 type;
    nsresult nsres;

    nsIDOMNode_HasChildNodes(nsnode, &has_children);

    nsres = nsIDOMNode_GetNodeType(nsnode, &type);
    if(NS_FAILED(nsres)) {
        ERR("GetType failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(type != DOCUMENT_NODE) {
        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIContent, (void**)&nscontent);
        if(NS_FAILED(nsres)) {
            ERR("Could not get nsIContent interface: %08lx\n", nsres);
            return E_FAIL;
        }
    }

    switch(type) {
    case ELEMENT_NODE:
        nsIContentSerializer_AppendElementStart(serializer, nscontent, nscontent, str);
        break;
    case TEXT_NODE:
        nsIContentSerializer_AppendText(serializer, nscontent, 0, -1, str);
        break;
    case COMMENT_NODE:
        nsIContentSerializer_AppendComment(serializer, nscontent, 0, -1, str);
        break;
    case DOCUMENT_NODE: {
        nsIDocument *nsdoc;
        nsIDOMNode_QueryInterface(nsnode, &IID_nsIDocument, (void**)&nsdoc);
        nsIContentSerializer_AppendDocumentStart(serializer, nsdoc, str);
        nsIDocument_Release(nsdoc);
        break;
    }
    case DOCUMENT_TYPE_NODE:
        nsIContentSerializer_AppendDoctype(serializer, nscontent, str);
        break;
    case DOCUMENT_FRAGMENT_NODE:
        break;
    default:
        FIXME("Unhandled type %u\n", type);
    }

    if(has_children) {
        UINT32 child_cnt, i;
        nsIDOMNode *child_node;

        nsIDOMNode_GetChildNodes(nsnode, &node_list);
        nsIDOMNodeList_GetLength(node_list, &child_cnt);

        for(i=0; i<child_cnt; i++) {
            nsres = nsIDOMNodeList_Item(node_list, i, &child_node);
            if(NS_SUCCEEDED(nsres)) {
                nsnode_to_nsstring_rec(serializer, child_node, str);
                nsIDOMNode_Release(child_node);
            }else {
                ERR("Item failed: %08lx\n", nsres);
            }
        }

        nsIDOMNodeList_Release(node_list);
    }

    if(type == ELEMENT_NODE)
        nsIContentSerializer_AppendElementEnd(serializer, nscontent, str);

    if(type != DOCUMENT_NODE)
        nsIContent_Release(nscontent);
    return S_OK;
}

HRESULT nsnode_to_nsstring(nsIDOMNode *nsnode, nsAString *str)
{
    nsIContentSerializer *serializer;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_HTMLSERIALIZER_CONTRACTID, NULL, &IID_nsIContentSerializer,
            (void**)&serializer);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIContentSerializer: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIContentSerializer_Init(serializer, 0, 100, NULL, FALSE, FALSE /* FIXME */);
    if(NS_FAILED(nsres))
        ERR("Init failed: %08lx\n", nsres);

    hres = nsnode_to_nsstring_rec(serializer, nsnode, str);
    if(SUCCEEDED(hres)) {
        nsres = nsIContentSerializer_Flush(serializer, str);
        if(NS_FAILED(nsres))
            ERR("Flush failed: %08lx\n", nsres);
    }

    nsIContentSerializer_Release(serializer);
    return hres;
}

void setup_editor_controller(GeckoBrowser *This)
{
    nsIEditingSession *editing_session = NULL;
    nsIControllerContext *ctrlctx;
    nsresult nsres;

    unlink_ref(&This->editor);
    unlink_ref(&This->editor_controller);

    nsres = get_nsinterface((nsISupports*)This->webbrowser, &IID_nsIEditingSession,
            (void**)&editing_session);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIEditingSession: %08lx\n", nsres);
        return;
    }

    nsres = nsIEditingSession_GetEditorForWindow(editing_session,
            This->doc->window->window_proxy, &This->editor);
    nsIEditingSession_Release(editing_session);
    if(NS_FAILED(nsres)) {
        ERR("Could not get editor: %08lx\n", nsres);
        return;
    }

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_EDITORCONTROLLER_CONTRACTID, NULL, &IID_nsIControllerContext, (void**)&ctrlctx);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIControllerContext_SetCommandContext(ctrlctx, (nsISupports *)This->editor);
        if(NS_FAILED(nsres))
            ERR("SetCommandContext failed: %08lx\n", nsres);
        nsres = nsIControllerContext_QueryInterface(ctrlctx, &IID_nsIController,
                (void**)&This->editor_controller);
        nsIControllerContext_Release(ctrlctx);
        if(NS_FAILED(nsres))
            ERR("Could not get nsIController interface: %08lx\n", nsres);
    }else {
        ERR("Could not create edit controller: %08lx\n", nsres);
    }
}

void close_gecko(void)
{
    TRACE("()\n");

    release_nsio();
    init_mutation(NULL);

    if(profile_directory) {
        nsIFile_Release(profile_directory);
        profile_directory = NULL;
    }

    if(plugin_directory) {
        nsIFile_Release(plugin_directory);
        plugin_directory = NULL;
    }

    if(pCompMgr)
        nsIComponentManager_Release(pCompMgr);

    if(pServMgr)
        nsIServiceManager_Release(pServMgr);

    if(cat_mgr)
        nsICategoryManager_Release(cat_mgr);

    /* Gecko doesn't really support being unloaded */
    /* if (hXPCOM) FreeLibrary(hXPCOM); */

    DeleteCriticalSection(&cs_load_gecko);
}

BOOL is_gecko_path(const char *path)
{
    WCHAR *buf, *ptr;
    BOOL ret;

    buf = strdupUtoW(path);
    if(!buf || lstrlenW(buf) < gecko_path_len)
        return FALSE;

    for(ptr = buf; *ptr; ptr++) {
        if(*ptr == '\\')
            *ptr = '/';
    }

    UrlUnescapeW(buf, NULL, NULL, URL_UNESCAPE_INPLACE | URL_UNESCAPE_AS_UTF8);
    buf[gecko_path_len] = 0;

    ret = !wcsicmp(buf, gecko_path);
    free(buf);
    return ret;
}

void set_viewer_zoom(GeckoBrowser *browser, float factor)
{
    nsIContentViewer *content_viewer;
    nsIDocShell *doc_shell;
    nsresult nsres;

    TRACE("Setting to %f\n", factor);

    nsres = get_nsinterface((nsISupports*)browser->navigation, &IID_nsIDocShell, (void**)&doc_shell);
    assert(nsres == NS_OK);

    nsres = nsIDocShell_GetContentViewer(doc_shell, &content_viewer);
    assert(nsres == NS_OK && content_viewer);
    nsIDocShell_Release(doc_shell);

    nsres = nsIContentViewer_SetFullZoom(content_viewer, factor);
    if(NS_FAILED(nsres))
        ERR("SetFullZoom failed: %08lx\n", nsres);

    nsIContentViewer_Release(content_viewer);
}

float get_viewer_zoom(GeckoBrowser *browser)
{
    nsIContentViewer *content_viewer;
    nsIDocShell *doc_shell;
    nsresult nsres;
    float factor;

    nsres = get_nsinterface((nsISupports*)browser->navigation, &IID_nsIDocShell, (void**)&doc_shell);
    assert(nsres == NS_OK);

    nsres = nsIDocShell_GetContentViewer(doc_shell, &content_viewer);
    assert(nsres == NS_OK && content_viewer);
    nsIDocShell_Release(doc_shell);

    nsres = nsIContentViewer_GetFullZoom(content_viewer, &factor);
    if(NS_FAILED(nsres))
        ERR("GetFullZoom failed: %08lx\n", nsres);
    TRACE("Got %f\n", factor);

    nsIContentViewer_Release(content_viewer);
    return factor;
}

struct nsWeakReference {
    nsIWeakReference nsIWeakReference_iface;

    LONG ref;

    GeckoBrowser *browser;
};

static inline nsWeakReference *impl_from_nsIWeakReference(nsIWeakReference *iface)
{
    return CONTAINING_RECORD(iface, nsWeakReference, nsIWeakReference_iface);
}

static nsresult NSAPI nsWeakReference_QueryInterface(nsIWeakReference *iface,
        nsIIDRef riid, void **result)
{
    nsWeakReference *This = impl_from_nsIWeakReference(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIWeakReference_iface;
    }else if(IsEqualGUID(&IID_nsIWeakReference, riid)) {
        TRACE("(%p)->(IID_nsIWeakReference %p)\n", This, result);
        *result = &This->nsIWeakReference_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        *result = NULL;
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsWeakReference_AddRef(nsIWeakReference *iface)
{
    nsWeakReference *This = impl_from_nsIWeakReference(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsWeakReference_Release(nsIWeakReference *iface)
{
    nsWeakReference *This = impl_from_nsIWeakReference(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        assert(!This->browser);
        free(This);
    }

    return ref;
}

static nsresult NSAPI nsWeakReference_QueryReferent(nsIWeakReference *iface,
        const nsIID *riid, void **result)
{
    nsWeakReference *This = impl_from_nsIWeakReference(iface);

    if(!This->browser)
        return NS_ERROR_NULL_POINTER;

    return nsIWebBrowserChrome_QueryInterface(&This->browser->nsIWebBrowserChrome_iface, riid, result);
}

static const nsIWeakReferenceVtbl nsWeakReferenceVtbl = {
    nsWeakReference_QueryInterface,
    nsWeakReference_AddRef,
    nsWeakReference_Release,
    nsWeakReference_QueryReferent
};

/**********************************************************
 *      nsIWebBrowserChrome interface
 */

static inline GeckoBrowser *impl_from_nsIWebBrowserChrome(nsIWebBrowserChrome *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsWebBrowserChrome_QueryInterface(nsIWebBrowserChrome *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);

    *result = NULL;
    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = &This->nsIWebBrowserChrome_iface;
    }else if(IsEqualGUID(&IID_nsIWebBrowserChrome, riid)) {
        TRACE("(%p)->(IID_nsIWebBrowserChrome, %p)\n", This, result);
        *result = &This->nsIWebBrowserChrome_iface;
    }else if(IsEqualGUID(&IID_nsIContextMenuListener, riid)) {
        TRACE("(%p)->(IID_nsIContextMenuListener, %p)\n", This, result);
        *result = &This->nsIContextMenuListener_iface;
    }else if(IsEqualGUID(&IID_nsIURIContentListener, riid)) {
        TRACE("(%p)->(IID_nsIURIContentListener %p)\n", This, result);
        *result = &This->nsIURIContentListener_iface;
    }else if(IsEqualGUID(&IID_nsIEmbeddingSiteWindow, riid)) {
        TRACE("(%p)->(IID_nsIEmbeddingSiteWindow %p)\n", This, result);
        *result = &This->nsIEmbeddingSiteWindow_iface;
    }else if(IsEqualGUID(&IID_nsITooltipListener, riid)) {
        TRACE("(%p)->(IID_nsITooltipListener %p)\n", This, result);
        *result = &This->nsITooltipListener_iface;
    }else if(IsEqualGUID(&IID_nsIInterfaceRequestor, riid)) {
        TRACE("(%p)->(IID_nsIInterfaceRequestor %p)\n", This, result);
        *result = &This->nsIInterfaceRequestor_iface;
    }else if(IsEqualGUID(&IID_nsISupportsWeakReference, riid)) {
        TRACE("(%p)->(IID_nsISupportsWeakReference %p)\n", This, result);
        *result = &This->nsISupportsWeakReference_iface;
    }

    if(*result) {
        nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsWebBrowserChrome_AddRef(nsIWebBrowserChrome *iface)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsWebBrowserChrome_Release(nsIWebBrowserChrome *iface)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->doc)
            detach_gecko_browser(This);
        if(This->weak_reference) {
            This->weak_reference->browser = NULL;
            nsIWeakReference_Release(&This->weak_reference->nsIWeakReference_iface);
        }
        free(This);
    }

    return ref;
}

static nsresult NSAPI nsWebBrowserChrome_SetStatus(nsIWebBrowserChrome *iface,
        UINT32 statusType, const PRUnichar *status)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    TRACE("(%p)->(%d %s)\n", This, statusType, debugstr_w(status));
    return NS_OK;
}

static nsresult NSAPI nsWebBrowserChrome_GetWebBrowser(nsIWebBrowserChrome *iface,
        nsIWebBrowser **aWebBrowser)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);

    TRACE("(%p)->(%p)\n", This, aWebBrowser);

    if(!aWebBrowser)
        return NS_ERROR_INVALID_ARG;

    if(This->webbrowser)
        nsIWebBrowser_AddRef(This->webbrowser);
    *aWebBrowser = This->webbrowser;
    return S_OK;
}

static nsresult NSAPI nsWebBrowserChrome_SetWebBrowser(nsIWebBrowserChrome *iface,
        nsIWebBrowser *aWebBrowser)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);

    TRACE("(%p)->(%p)\n", This, aWebBrowser);

    if(aWebBrowser != This->webbrowser)
        ERR("Wrong nsWebBrowser!\n");

    return NS_OK;
}

static nsresult NSAPI nsWebBrowserChrome_GetChromeFlags(nsIWebBrowserChrome *iface,
        UINT32 *aChromeFlags)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)->(%p)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SetChromeFlags(nsIWebBrowserChrome *iface,
        UINT32 aChromeFlags)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)->(%08x)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_DestroyBrowserWindow(nsIWebBrowserChrome *iface)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    TRACE("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SizeBrowserTo(nsIWebBrowserChrome *iface,
        LONG aCX, LONG aCY)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)->(%ld %ld)\n", This, aCX, aCY);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ShowAsModal(nsIWebBrowserChrome *iface)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_IsWindowModal(nsIWebBrowserChrome *iface, cpp_bool *_retval)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ExitModalEventLoop(nsIWebBrowserChrome *iface,
        nsresult aStatus)
{
    GeckoBrowser *This = impl_from_nsIWebBrowserChrome(iface);
    WARN("(%p)->(%08lx)\n", This, aStatus);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIWebBrowserChromeVtbl nsWebBrowserChromeVtbl = {
    nsWebBrowserChrome_QueryInterface,
    nsWebBrowserChrome_AddRef,
    nsWebBrowserChrome_Release,
    nsWebBrowserChrome_SetStatus,
    nsWebBrowserChrome_GetWebBrowser,
    nsWebBrowserChrome_SetWebBrowser,
    nsWebBrowserChrome_GetChromeFlags,
    nsWebBrowserChrome_SetChromeFlags,
    nsWebBrowserChrome_DestroyBrowserWindow,
    nsWebBrowserChrome_SizeBrowserTo,
    nsWebBrowserChrome_ShowAsModal,
    nsWebBrowserChrome_IsWindowModal,
    nsWebBrowserChrome_ExitModalEventLoop
};

/**********************************************************
 *      nsIContextMenuListener interface
 */

static inline GeckoBrowser *impl_from_nsIContextMenuListener(nsIContextMenuListener *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsIContextMenuListener_iface);
}

static nsresult NSAPI nsContextMenuListener_QueryInterface(nsIContextMenuListener *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIContextMenuListener(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsContextMenuListener_AddRef(nsIContextMenuListener *iface)
{
    GeckoBrowser *This = impl_from_nsIContextMenuListener(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsContextMenuListener_Release(nsIContextMenuListener *iface)
{
    GeckoBrowser *This = impl_from_nsIContextMenuListener(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsContextMenuListener_OnShowContextMenu(nsIContextMenuListener *iface,
        UINT32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    GeckoBrowser *This = impl_from_nsIContextMenuListener(iface);
    nsIDOMMouseEvent *mouse_event;
    HTMLDOMNode *node;
    DOMEvent *event;
    POINT pt;
    DWORD dwID = CONTEXT_MENU_DEFAULT;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%08x %p %p)\n", This, aContextFlags, aEvent, aNode);

    if (!aNode)
        return NS_ERROR_FAILURE;

    hres = get_node(aNode, TRUE, &node);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = create_event_from_nsevent(aEvent, This->doc->doc_node->script_global,
                                     dispex_compat_mode(&node->event_target.dispex), &event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&node->event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }

    nsres = nsIDOMEvent_QueryInterface(aEvent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
    assert(NS_SUCCEEDED(nsres));

    nsIDOMMouseEvent_GetScreenX(mouse_event, &pt.x);
    nsIDOMMouseEvent_GetScreenY(mouse_event, &pt.y);
    nsIDOMMouseEvent_Release(mouse_event);

    switch(aContextFlags) {
    case CONTEXT_NONE:
    case CONTEXT_DOCUMENT:
    case CONTEXT_TEXT: {
        nsISelection *selection;

        if(!This->doc->doc_node->html_document) {
            FIXME("Not implemented for XML document\n");
            break;
        }

        nsres = nsIDOMHTMLDocument_GetSelection(This->doc->doc_node->html_document, &selection);
        if(NS_SUCCEEDED(nsres) && selection) {
            cpp_bool is_collapsed;

            /* FIXME: Check if the click was inside selection. */
            nsres = nsISelection_GetIsCollapsed(selection, &is_collapsed);
            nsISelection_Release(selection);
            if(NS_SUCCEEDED(nsres) && !is_collapsed)
                dwID = CONTEXT_MENU_TEXTSELECT;
        }
        break;
    }
    case CONTEXT_IMAGE:
    case CONTEXT_IMAGE|CONTEXT_LINK:
        dwID = CONTEXT_MENU_IMAGE;
        break;
    case CONTEXT_LINK:
        dwID = CONTEXT_MENU_ANCHOR;
        break;
    case CONTEXT_INPUT:
        dwID = CONTEXT_MENU_CONTROL;
        break;
    default:
        FIXME("aContextFlags=%08x\n", aContextFlags);
    };

    show_context_menu(This->doc, dwID, &pt, (IDispatch*)&node->IHTMLDOMNode_iface);
    node_release(node);
    return NS_OK;
}

static const nsIContextMenuListenerVtbl nsContextMenuListenerVtbl = {
    nsContextMenuListener_QueryInterface,
    nsContextMenuListener_AddRef,
    nsContextMenuListener_Release,
    nsContextMenuListener_OnShowContextMenu
};

/**********************************************************
 *      nsIURIContentListener interface
 */

static inline GeckoBrowser *impl_from_nsIURIContentListener(nsIURIContentListener *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsIURIContentListener_iface);
}

static nsresult NSAPI nsURIContentListener_QueryInterface(nsIURIContentListener *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsURIContentListener_AddRef(nsIURIContentListener *iface)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsURIContentListener_Release(nsIURIContentListener *iface)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsURIContentListener_OnStartURIOpen(nsIURIContentListener *iface,
                                                          nsIURI *aURI, cpp_bool *_retval)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);
    nsACString spec_str;
    const char *spec;

    nsACString_Init(&spec_str, NULL);
    nsIURI_GetSpec(aURI, &spec_str);
    nsACString_GetData(&spec_str, &spec);

    TRACE("(%p)->(%p(%s) %p)\n", This, aURI, debugstr_a(spec), _retval);

    nsACString_Finish(&spec_str);

    return This->content_listener
        ? nsIURIContentListener_OnStartURIOpen(This->content_listener, aURI, _retval)
        : NS_OK;
}

static nsresult NSAPI nsURIContentListener_DoContent(nsIURIContentListener *iface,
        const nsACString *aContentType, cpp_bool aIsContentPreferred, nsIRequest *aRequest,
        nsIStreamListener **aContentHandler, cpp_bool *_retval)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    TRACE("(%p)->(%p %x %p %p %p)\n", This, aContentType, aIsContentPreferred,
            aRequest, aContentHandler, _retval);

    return This->content_listener
        ? nsIURIContentListener_DoContent(This->content_listener, aContentType,
                  aIsContentPreferred, aRequest, aContentHandler, _retval)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_IsPreferred(nsIURIContentListener *iface,
        const char *aContentType, char **aDesiredContentType, cpp_bool *_retval)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_a(aContentType), aDesiredContentType, _retval);

    /* FIXME: Should we do something here? */
    *_retval = TRUE; 

    return This->content_listener
        ? nsIURIContentListener_IsPreferred(This->content_listener, aContentType,
                  aDesiredContentType, _retval)
        : NS_OK;
}

static nsresult NSAPI nsURIContentListener_CanHandleContent(nsIURIContentListener *iface,
        const char *aContentType, cpp_bool aIsContentPreferred, char **aDesiredContentType,
        cpp_bool *_retval)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    TRACE("(%p)->(%s %x %p %p)\n", This, debugstr_a(aContentType), aIsContentPreferred,
            aDesiredContentType, _retval);

    return This->content_listener
        ? nsIURIContentListener_CanHandleContent(This->content_listener, aContentType,
                aIsContentPreferred, aDesiredContentType, _retval)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_GetLoadCookie(nsIURIContentListener *iface,
        nsISupports **aLoadCookie)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    WARN("(%p)->(%p)\n", This, aLoadCookie);

    return This->content_listener
        ? nsIURIContentListener_GetLoadCookie(This->content_listener, aLoadCookie)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_SetLoadCookie(nsIURIContentListener *iface,
        nsISupports *aLoadCookie)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    WARN("(%p)->(%p)\n", This, aLoadCookie);

    return This->content_listener
        ? nsIURIContentListener_SetLoadCookie(This->content_listener, aLoadCookie)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_GetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener **aParentContentListener)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    TRACE("(%p)->(%p)\n", This, aParentContentListener);

    if(This->content_listener)
        nsIURIContentListener_AddRef(This->content_listener);

    *aParentContentListener = This->content_listener;
    return NS_OK;
}

static nsresult NSAPI nsURIContentListener_SetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener *aParentContentListener)
{
    GeckoBrowser *This = impl_from_nsIURIContentListener(iface);

    TRACE("(%p)->(%p)\n", This, aParentContentListener);

    if(aParentContentListener == &This->nsIURIContentListener_iface)
        return NS_OK;

    if(This->content_listener)
        nsIURIContentListener_Release(This->content_listener);

    This->content_listener = aParentContentListener;
    if(This->content_listener)
        nsIURIContentListener_AddRef(This->content_listener);

    return NS_OK;
}

static const nsIURIContentListenerVtbl nsURIContentListenerVtbl = {
    nsURIContentListener_QueryInterface,
    nsURIContentListener_AddRef,
    nsURIContentListener_Release,
    nsURIContentListener_OnStartURIOpen,
    nsURIContentListener_DoContent,
    nsURIContentListener_IsPreferred,
    nsURIContentListener_CanHandleContent,
    nsURIContentListener_GetLoadCookie,
    nsURIContentListener_SetLoadCookie,
    nsURIContentListener_GetParentContentListener,
    nsURIContentListener_SetParentContentListener
};

/**********************************************************
 *      nsIEmbeddinSiteWindow interface
 */

static inline GeckoBrowser *impl_from_nsIEmbeddingSiteWindow(nsIEmbeddingSiteWindow *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsIEmbeddingSiteWindow_iface);
}

static nsresult NSAPI nsEmbeddingSiteWindow_QueryInterface(nsIEmbeddingSiteWindow *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_AddRef(nsIEmbeddingSiteWindow *iface)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_Release(nsIEmbeddingSiteWindow *iface)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetDimensions(nsIEmbeddingSiteWindow *iface,
        UINT32 flags, LONG x, LONG y, LONG cx, LONG cy)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    WARN("(%p)->(%08x %ld %ld %ld %ld)\n", This, flags, x, y, cx, cy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetDimensions(nsIEmbeddingSiteWindow *iface,
        UINT32 flags, LONG *x, LONG *y, LONG *cx, LONG *cy)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    RECT r;

    TRACE("(%p)->(%x %p %p %p %p)\n", This, flags, x, y, cx, cy);

    if(!GetWindowRect(This->hwnd, &r)) {
        ERR("GetWindowRect failed\n");
        return NS_ERROR_FAILURE;
    }

    if(x)
        *x = r.left;
    if(y)
        *y = r.top;
    if(cx)
        *cx = r.right-r.left;
    if(cy)
        *cy = r.bottom-r.top;
    return NS_OK;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetFocus(nsIEmbeddingSiteWindow *iface)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);

    TRACE("(%p)\n", This);

    return nsIBaseWindow_SetFocus(This->window);
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetVisibility(nsIEmbeddingSiteWindow *iface,
        cpp_bool *aVisibility)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);

    TRACE("(%p)->(%p)\n", This, aVisibility);

    *aVisibility = This->doc && This->doc->hwnd && IsWindowVisible(This->doc->hwnd);
    return NS_OK;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetVisibility(nsIEmbeddingSiteWindow *iface,
        cpp_bool aVisibility)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);

    TRACE("(%p)->(%x)\n", This, aVisibility);

    return NS_OK;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetTitle(nsIEmbeddingSiteWindow *iface,
        PRUnichar **aTitle)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    WARN("(%p)->(%p)\n", This, aTitle);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetTitle(nsIEmbeddingSiteWindow *iface,
        const PRUnichar *aTitle)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);
    WARN("(%p)->(%s)\n", This, debugstr_w(aTitle));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetSiteWindow(nsIEmbeddingSiteWindow *iface,
        void **aSiteWindow)
{
    GeckoBrowser *This = impl_from_nsIEmbeddingSiteWindow(iface);

    TRACE("(%p)->(%p)\n", This, aSiteWindow);

    *aSiteWindow = This->hwnd;
    return NS_OK;
}

static const nsIEmbeddingSiteWindowVtbl nsEmbeddingSiteWindowVtbl = {
    nsEmbeddingSiteWindow_QueryInterface,
    nsEmbeddingSiteWindow_AddRef,
    nsEmbeddingSiteWindow_Release,
    nsEmbeddingSiteWindow_SetDimensions,
    nsEmbeddingSiteWindow_GetDimensions,
    nsEmbeddingSiteWindow_SetFocus,
    nsEmbeddingSiteWindow_GetVisibility,
    nsEmbeddingSiteWindow_SetVisibility,
    nsEmbeddingSiteWindow_GetTitle,
    nsEmbeddingSiteWindow_SetTitle,
    nsEmbeddingSiteWindow_GetSiteWindow
};

static inline GeckoBrowser *impl_from_nsITooltipListener(nsITooltipListener *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsITooltipListener_iface);
}

static nsresult NSAPI nsTooltipListener_QueryInterface(nsITooltipListener *iface, nsIIDRef riid,
        void **result)
{
    GeckoBrowser *This = impl_from_nsITooltipListener(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsTooltipListener_AddRef(nsITooltipListener *iface)
{
    GeckoBrowser *This = impl_from_nsITooltipListener(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsTooltipListener_Release(nsITooltipListener *iface)
{
    GeckoBrowser *This = impl_from_nsITooltipListener(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsTooltipListener_OnShowTooltip(nsITooltipListener *iface,
        LONG aXCoord, LONG aYCoord, const PRUnichar *aTipText)
{
    GeckoBrowser *This = impl_from_nsITooltipListener(iface);

    if (This->doc)
        show_tooltip(This->doc, aXCoord, aYCoord, aTipText);

    return NS_OK;
}

static nsresult NSAPI nsTooltipListener_OnHideTooltip(nsITooltipListener *iface)
{
    GeckoBrowser *This = impl_from_nsITooltipListener(iface);

    if (This->doc)
        hide_tooltip(This->doc);

    return NS_OK;
}

static const nsITooltipListenerVtbl nsTooltipListenerVtbl = {
    nsTooltipListener_QueryInterface,
    nsTooltipListener_AddRef,
    nsTooltipListener_Release,
    nsTooltipListener_OnShowTooltip,
    nsTooltipListener_OnHideTooltip
};

static inline GeckoBrowser *impl_from_nsIInterfaceRequestor(nsIInterfaceRequestor *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsIInterfaceRequestor_iface);
}

static nsresult NSAPI nsInterfaceRequestor_QueryInterface(nsIInterfaceRequestor *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIInterfaceRequestor(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsInterfaceRequestor_AddRef(nsIInterfaceRequestor *iface)
{
    GeckoBrowser *This = impl_from_nsIInterfaceRequestor(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsInterfaceRequestor_Release(nsIInterfaceRequestor *iface)
{
    GeckoBrowser *This = impl_from_nsIInterfaceRequestor(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsInterfaceRequestor_GetInterface(nsIInterfaceRequestor *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsIInterfaceRequestor(iface);

    if(IsEqualGUID(&IID_mozIDOMWindowProxy, riid)) {
        TRACE("(%p)->(IID_nsIDOMWindow %p)\n", This, result);
        return nsIWebBrowser_GetContentDOMWindow(This->webbrowser, (mozIDOMWindowProxy**)result);
    }

    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static const nsIInterfaceRequestorVtbl nsInterfaceRequestorVtbl = {
    nsInterfaceRequestor_QueryInterface,
    nsInterfaceRequestor_AddRef,
    nsInterfaceRequestor_Release,
    nsInterfaceRequestor_GetInterface
};

static inline GeckoBrowser *impl_from_nsISupportsWeakReference(nsISupportsWeakReference *iface)
{
    return CONTAINING_RECORD(iface, GeckoBrowser, nsISupportsWeakReference_iface);
}

static nsresult NSAPI nsSupportsWeakReference_QueryInterface(nsISupportsWeakReference *iface,
        nsIIDRef riid, void **result)
{
    GeckoBrowser *This = impl_from_nsISupportsWeakReference(iface);
    return nsIWebBrowserChrome_QueryInterface(&This->nsIWebBrowserChrome_iface, riid, result);
}

static nsrefcnt NSAPI nsSupportsWeakReference_AddRef(nsISupportsWeakReference *iface)
{
    GeckoBrowser *This = impl_from_nsISupportsWeakReference(iface);
    return nsIWebBrowserChrome_AddRef(&This->nsIWebBrowserChrome_iface);
}

static nsrefcnt NSAPI nsSupportsWeakReference_Release(nsISupportsWeakReference *iface)
{
    GeckoBrowser *This = impl_from_nsISupportsWeakReference(iface);
    return nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);
}

static nsresult NSAPI nsSupportsWeakReference_GetWeakReference(nsISupportsWeakReference *iface,
        nsIWeakReference **_retval)
{
    GeckoBrowser *This = impl_from_nsISupportsWeakReference(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    if(!This->weak_reference) {
        This->weak_reference = malloc(sizeof(nsWeakReference));
        if(!This->weak_reference)
            return NS_ERROR_OUT_OF_MEMORY;

        This->weak_reference->nsIWeakReference_iface.lpVtbl = &nsWeakReferenceVtbl;
        This->weak_reference->ref = 1;
        This->weak_reference->browser = This;
    }

    *_retval = &This->weak_reference->nsIWeakReference_iface;
    nsIWeakReference_AddRef(*_retval);
    return NS_OK;
}

static const nsISupportsWeakReferenceVtbl nsSupportsWeakReferenceVtbl = {
    nsSupportsWeakReference_QueryInterface,
    nsSupportsWeakReference_AddRef,
    nsSupportsWeakReference_Release,
    nsSupportsWeakReference_GetWeakReference
};

static HRESULT init_browser(GeckoBrowser *browser)
{
    mozIDOMWindowProxy *mozwindow;
    nsIWebBrowserSetup *wbsetup;
    nsIScrollable *scrollable;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_WEBBROWSER_CONTRACTID,
            NULL, &IID_nsIWebBrowser, (void**)&browser->webbrowser);
    if(NS_FAILED(nsres)) {
        ERR("Creating WebBrowser failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebBrowser_SetContainerWindow(browser->webbrowser, &browser->nsIWebBrowserChrome_iface);
    if(NS_FAILED(nsres)) {
        ERR("SetContainerWindow failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIBaseWindow,
            (void**)&browser->window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIBaseWindow interface: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIWebBrowserSetup,
                                         (void**)&wbsetup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(wbsetup, SETUP_IS_CHROME_WRAPPER, FALSE);
        nsIWebBrowserSetup_Release(wbsetup);
        if(NS_FAILED(nsres)) {
            ERR("SetProperty(SETUP_IS_CHROME_WRAPPER) failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }else {
        ERR("Could not get nsIWebBrowserSetup interface\n");
        return E_FAIL;
    }

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIWebNavigation,
            (void**)&browser->navigation);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebNavigation interface: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIWebBrowserFocus,
            (void**)&browser->focus);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebBrowserFocus interface: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!browser_class) {
        register_browser_class();
        if(!browser_class)
            return E_FAIL;
    }

    browser->hwnd = CreateWindowExW(0, L"NsContainer", NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 100, 100,
            GetDesktopWindow(), NULL, hInst, browser);
    if(!browser->hwnd) {
        WARN("Could not create window\n");
        return E_FAIL;
    }

    nsres = nsIBaseWindow_InitWindow(browser->window, browser->hwnd, NULL, 0, 0, 100, 100);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIBaseWindow_Create(browser->window);
        if(NS_FAILED(nsres)) {
            WARN("Creating window failed: %08lx\n", nsres);
            return E_FAIL;
        }

        nsIBaseWindow_SetVisibility(browser->window, FALSE);
        nsIBaseWindow_SetEnabled(browser->window, FALSE);
    }else {
        ERR("InitWindow failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebBrowser_SetParentURIContentListener(browser->webbrowser,
            &browser->nsIURIContentListener_iface);
    if(NS_FAILED(nsres))
        ERR("SetParentURIContentListener failed: %08lx\n", nsres);

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIScrollable, (void**)&scrollable);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable,
                ScrollOrientation_Y, Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default Y scrollbar prefs: %08lx\n", nsres);

        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable,
                ScrollOrientation_X, Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default X scrollbar prefs: %08lx\n", nsres);

        nsIScrollable_Release(scrollable);
    }else {
        ERR("Could not get nsIScrollable: %08lx\n", nsres);
    }

    nsres = nsIWebBrowser_GetContentDOMWindow(browser->webbrowser, &mozwindow);
    if(NS_FAILED(nsres)) {
        ERR("GetContentDOMWindow failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = create_outer_window(browser, mozwindow, NULL, &browser->content_window);
    mozIDOMWindowProxy_Release(mozwindow);
    return hres;
}

HRESULT create_gecko_browser(HTMLDocumentObj *doc, GeckoBrowser **_ret)
{
    GeckoBrowser *ret;
    HRESULT hres;

    if(!load_gecko())
        return CLASS_E_CLASSNOTAVAILABLE;

    ret = calloc(1, sizeof(GeckoBrowser));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->nsIWebBrowserChrome_iface.lpVtbl = &nsWebBrowserChromeVtbl;
    ret->nsIContextMenuListener_iface.lpVtbl = &nsContextMenuListenerVtbl;
    ret->nsIURIContentListener_iface.lpVtbl = &nsURIContentListenerVtbl;
    ret->nsIEmbeddingSiteWindow_iface.lpVtbl = &nsEmbeddingSiteWindowVtbl;
    ret->nsITooltipListener_iface.lpVtbl = &nsTooltipListenerVtbl;
    ret->nsIInterfaceRequestor_iface.lpVtbl = &nsInterfaceRequestorVtbl;
    ret->nsISupportsWeakReference_iface.lpVtbl = &nsSupportsWeakReferenceVtbl;

    ret->doc = doc;
    ret->ref = 1;
    ret->script_mode = SCRIPTMODE_ACTIVESCRIPT;
    ret->usermode = UNKNOWN_USERMODE;
    list_init(&ret->document_nodes);
    list_init(&ret->outer_windows);

    hres = init_browser(ret);
    if(SUCCEEDED(hres))
        *_ret = ret;
    else
        nsIWebBrowserChrome_Release(&ret->nsIWebBrowserChrome_iface);
    return hres;
}

void detach_gecko_browser(GeckoBrowser *This)
{
    nsIDOMWindowUtils *window_utils = NULL;

    TRACE("(%p)\n", This);

    This->doc = NULL;

    if(This->content_window) {
        get_nsinterface((nsISupports*)This->content_window->nswindow, &IID_nsIDOMWindowUtils, (void**)&window_utils);

        IHTMLWindow2_Release(&This->content_window->base.IHTMLWindow2_iface);
        This->content_window = NULL;
    }

    while(!list_empty(&This->document_nodes)) {
        HTMLDocumentNode *doc = LIST_ENTRY(list_head(&This->document_nodes), HTMLDocumentNode, browser_entry);
        detach_document_node(doc);
    }

    while(!list_empty(&This->outer_windows)) {
        HTMLOuterWindow *window = LIST_ENTRY(list_head(&This->outer_windows), HTMLOuterWindow, browser_entry);
        list_remove(&window->browser_entry);
        window->browser = NULL;
    }

    ShowWindow(This->hwnd, SW_HIDE);
    SetParent(This->hwnd, NULL);

    nsIBaseWindow_SetVisibility(This->window, FALSE);
    nsIBaseWindow_Destroy(This->window);

    nsIWebBrowser_SetContainerWindow(This->webbrowser, NULL);

    nsIWebBrowser_Release(This->webbrowser);
    This->webbrowser = NULL;

    nsIWebNavigation_Release(This->navigation);
    This->navigation = NULL;

    nsIBaseWindow_Release(This->window);
    This->window = NULL;

    nsIWebBrowserFocus_Release(This->focus);
    This->focus = NULL;

    if(This->editor_controller) {
        nsIController_Release(This->editor_controller);
        This->editor_controller = NULL;
    }

    if(This->editor) {
        nsIEditor_Release(This->editor);
        This->editor = NULL;
    }

    if(This->content_listener) {
        nsIURIContentListener_Release(This->content_listener);
        This->content_listener = NULL;
    }

    if(This->hwnd) {
        DestroyWindow(This->hwnd);
        This->hwnd = NULL;
    }

    nsIWebBrowserChrome_Release(&This->nsIWebBrowserChrome_iface);

    /* Force cycle collection */
    if(window_utils) {
        nsIDOMWindowUtils_CycleCollect(window_utils, NULL, 0);
        nsIDOMWindowUtils_Release(window_utils);
    }
}

/*
 * FIXME: nsIScriptObjectPrincipal uses thiscall calling convention, so we need this hack on i386.
 * This will be removed after the next Gecko update, that will change calling convention on Gecko side.
 */
#ifdef __i386__
#if defined(__REACTOS__) && defined(_MSC_VER) /* FIXME: Inspect */
__declspec(naked) void call_thiscall_func_impl(void)
{
    __asm
    {
        pop eax
        pop edx
        pop ecx
        push eax
        jmp edx
    }
}
void *call_thiscall_func = (void *)call_thiscall_func_impl;
#else
__ASM_GLOBAL_FUNC(call_thiscall_func,
        "popl %eax\n\t"
        "popl %edx\n\t"
        "popl %ecx\n\t"
        "pushl %eax\n\t"
        "jmp *%edx\n\t")
#endif
#define nsIScriptObjectPrincipal_GetPrincipal(this) ((void* (WINAPI*)(void*,void*))&call_thiscall_func)((this)->lpVtbl->GetPrincipal,this)
#endif

nsIXMLHttpRequest *create_nsxhr(nsIDOMWindow *nswindow)
{
    nsIScriptObjectPrincipal *sop;
    mozIDOMWindow *inner_window;
    nsIPrincipal *nspri;
    nsIGlobalObject *nsglo;
    nsIXMLHttpRequest *nsxhr;
    nsresult nsres;

    nsres = nsIDOMWindow_GetInnerWindow(nswindow, &inner_window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get inner window: %08lx\n", nsres);
        return NULL;
    }

    nsres = mozIDOMWindow_QueryInterface(inner_window, &IID_nsIGlobalObject, (void **)&nsglo);
    mozIDOMWindow_Release(inner_window);
    assert(nsres == NS_OK);

    nsres = nsIGlobalObject_QueryInterface(nsglo, &IID_nsIScriptObjectPrincipal, (void**)&sop);
    assert(nsres == NS_OK);

    /* The returned principal is *not* AddRef'd */
    nspri = nsIScriptObjectPrincipal_GetPrincipal(sop);
    nsIScriptObjectPrincipal_Release(sop);

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_XMLHTTPREQUEST_CONTRACTID, NULL, &IID_nsIXMLHttpRequest,
            (void**)&nsxhr);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIXMLHttpRequest_Init(nsxhr, nspri, NULL, nsglo, NULL, NULL);
        if(NS_FAILED(nsres))
            nsIXMLHttpRequest_Release(nsxhr);
    }
    nsIGlobalObject_Release(nsglo);
    if(NS_FAILED(nsres)) {
        ERR("nsIXMLHttpRequest_Init failed: %08lx\n", nsres);
        return NULL;
    }

    return nsxhr;
}
