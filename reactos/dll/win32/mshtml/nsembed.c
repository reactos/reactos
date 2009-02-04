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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);
WINE_DECLARE_DEBUG_CHANNEL(gecko);

#define NS_APPSTARTUPNOTIFIER_CONTRACTID "@mozilla.org/embedcomp/appstartup-notifier;1"
#define NS_WEBBROWSER_CONTRACTID "@mozilla.org/embedding/browser/nsWebBrowser;1"
#define NS_PROFILE_CONTRACTID "@mozilla.org/profile/manager;1"
#define NS_MEMORY_CONTRACTID "@mozilla.org/xpcom/memory-service;1"
#define NS_STRINGSTREAM_CONTRACTID "@mozilla.org/io/string-input-stream;1"
#define NS_COMMANDPARAMS_CONTRACTID "@mozilla.org/embedcomp/command-params;1"
#define NS_HTMLSERIALIZER_CONTRACTID "@mozilla.org/layout/contentserializer;1?mimetype=text/html"
#define NS_EDITORCONTROLLER_CONTRACTID "@mozilla.org/editor/editorcontroller;1"
#define NS_ARRAY_CONTRACTID "@mozilla.org/array;1"
#define NS_VARIANT_CONTRACTID "@mozilla.org/variant;1"
#define NS_PREFERENCES_CONTRACTID "@mozilla.org/preferences;1"

#define APPSTARTUP_TOPIC "app-startup"

#define PR_UINT32_MAX 0xffffffff

struct nsCStringContainer {
    void *v;
    void *d1;
    PRUint32 d2;
    void *d3;
};

static nsresult (*NS_InitXPCOM2)(nsIServiceManager**,void*,void*);
static nsresult (*NS_ShutdownXPCOM)(nsIServiceManager*);
static nsresult (*NS_GetComponentRegistrar)(nsIComponentRegistrar**);
static nsresult (*NS_StringContainerInit)(nsStringContainer*);
static nsresult (*NS_CStringContainerInit)(nsCStringContainer*);
static nsresult (*NS_StringContainerFinish)(nsStringContainer*);
static nsresult (*NS_CStringContainerFinish)(nsCStringContainer*);
static nsresult (*NS_StringSetData)(nsAString*,const PRUnichar*,PRUint32);
static nsresult (*NS_CStringSetData)(nsACString*,const char*,PRUint32);
static nsresult (*NS_NewLocalFile)(const nsAString*,PRBool,nsIFile**);
static PRUint32 (*NS_StringGetData)(const nsAString*,const PRUnichar **,PRBool*);
static PRUint32 (*NS_CStringGetData)(const nsACString*,const char**,PRBool*);

static HINSTANCE hXPCOM = NULL;

static nsIServiceManager *pServMgr = NULL;
static nsIComponentManager *pCompMgr = NULL;
static nsIMemory *nsmem = NULL;

static const WCHAR wszNsContainer[] = {'N','s','C','o','n','t','a','i','n','e','r',0};

static ATOM nscontainer_class;

#define WM_RESETFOCUS_HACK WM_USER+600

static LRESULT WINAPI nsembed_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NSContainer *This;
    nsresult nsres;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(NSContainer**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
    case WM_SIZE:
        TRACE("(%p)->(WM_SIZE)\n", This);

        nsres = nsIBaseWindow_SetSize(This->window,
                LOWORD(lParam), HIWORD(lParam), TRUE);
        if(NS_FAILED(nsres))
            WARN("SetSize failed: %08x\n", nsres);
        break;

    case WM_RESETFOCUS_HACK:
        /*
         * FIXME
         * Gecko grabs focus in edit mode and some apps don't like it.
         * We should somehow prevent grabbing focus.
         */

        TRACE("WM_RESETFOCUS_HACK\n");

        if(This->reset_focus) {
            SetFocus(This->reset_focus);
            This->reset_focus = NULL;
            if(This->doc)
                This->doc->focus = FALSE;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}


static void register_nscontainer_class(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        CS_DBLCLKS,
        nsembed_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszNsContainer,
        NULL,
    };
    wndclass.hInstance = hInst;
    nscontainer_class = RegisterClassExW(&wndclass);
}

static void set_environment(LPCWSTR gre_path)
{
    WCHAR path_env[MAX_PATH], buf[20];
    int len, debug_level = 0;

    static const WCHAR pathW[] = {'P','A','T','H',0};
    static const WCHAR warnW[] = {'w','a','r','n',0};
    static const WCHAR xpcom_debug_breakW[] =
        {'X','P','C','O','M','_','D','E','B','U','G','_','B','R','E','A','K',0};
    static const WCHAR nspr_log_modulesW[] =
        {'N','S','P','R','_','L','O','G','_','M','O','D','U','L','E','S',0};
    static const WCHAR debug_formatW[] = {'a','l','l',':','%','d',0};

    /* We have to modify PATH as XPCOM loads other DLLs from this directory. */
    GetEnvironmentVariableW(pathW, path_env, sizeof(path_env)/sizeof(WCHAR));
    len = strlenW(path_env);
    path_env[len++] = ';';
    strcpyW(path_env+len, gre_path);
    SetEnvironmentVariableW(pathW, path_env);

    SetEnvironmentVariableW(xpcom_debug_breakW, warnW);

    if(TRACE_ON(gecko))
        debug_level = 5;
    else if(WARN_ON(gecko))
        debug_level = 3;
    else if(ERR_ON(gecko))
        debug_level = 2;

    sprintfW(buf, debug_formatW, debug_level);
    SetEnvironmentVariableW(nspr_log_modulesW, buf);
}

static BOOL load_xpcom(const PRUnichar *gre_path)
{
    static const WCHAR strXPCOM[] = {'x','p','c','o','m','.','d','l','l',0};

    TRACE("(%s)\n", debugstr_w(gre_path));

    set_environment(gre_path);

    hXPCOM = LoadLibraryW(strXPCOM);
    if(!hXPCOM) {
        WARN("Could not load XPCOM: %d\n", GetLastError());
        return FALSE;
    }

#define NS_DLSYM(func) \
    func = (void *)GetProcAddress(hXPCOM, #func); \
    if(!func) \
        ERR("Could not GetProcAddress(" #func ") failed\n")

    NS_DLSYM(NS_InitXPCOM2);
    NS_DLSYM(NS_ShutdownXPCOM);
    NS_DLSYM(NS_GetComponentRegistrar);
    NS_DLSYM(NS_StringContainerInit);
    NS_DLSYM(NS_CStringContainerInit);
    NS_DLSYM(NS_StringContainerFinish);
    NS_DLSYM(NS_CStringContainerFinish);
    NS_DLSYM(NS_StringSetData);
    NS_DLSYM(NS_CStringSetData);
    NS_DLSYM(NS_NewLocalFile);
    NS_DLSYM(NS_StringGetData);
    NS_DLSYM(NS_CStringGetData);

#undef NS_DLSYM

    return TRUE;
}

static BOOL check_version(LPCWSTR gre_path, const char *version_string)
{
    WCHAR file_name[MAX_PATH];
    char version[128];
    DWORD read=0;
    HANDLE hfile;

    static const WCHAR wszVersion[] = {'\\','V','E','R','S','I','O','N',0};

    strcpyW(file_name, gre_path);
    strcatW(file_name, wszVersion);

    hfile = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hfile == INVALID_HANDLE_VALUE) {
        ERR("Could not open VERSION file\n");
        return FALSE;
    }

    ReadFile(hfile, version, sizeof(version), &read, NULL);
    version[read] = 0;
    CloseHandle(hfile);

    TRACE("%s\n", debugstr_a(version));

    if(strcmp(version, version_string)) {
        ERR("Unexpected version %s, expected %s\n", debugstr_a(version),
            debugstr_a(version_string));
        return FALSE;
    }

    return TRUE;
}

static BOOL load_wine_gecko_v(PRUnichar *gre_path, HKEY mshtml_key,
        const char *version, const char *version_string)
{
    DWORD res, type, size = MAX_PATH;
    HKEY hkey = mshtml_key;

    static const WCHAR wszGeckoPath[] =
        {'G','e','c','k','o','P','a','t','h',0};

    if(version) {
        /* @@ Wine registry key: HKCU\Software\Wine\MSHTML\<version> */
        res = RegOpenKeyA(mshtml_key, version, &hkey);
        if(res != ERROR_SUCCESS)
            return FALSE;
    }

    res = RegQueryValueExW(hkey, wszGeckoPath, NULL, &type, (LPBYTE)gre_path, &size);
    if(hkey != mshtml_key)
        RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    if(!check_version(gre_path, version_string))
        return FALSE;

    return load_xpcom(gre_path);
}

static BOOL load_wine_gecko(PRUnichar *gre_path)
{
    HKEY hkey;
    DWORD res;
    BOOL ret;

    static const WCHAR wszMshtmlKey[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e',
        '\\','M','S','H','T','M','L',0};

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML */
    res = RegOpenKeyW(HKEY_CURRENT_USER, wszMshtmlKey, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    ret = load_wine_gecko_v(gre_path, hkey, GECKO_VERSION, GECKO_VERSION_STRING);

    RegCloseKey(hkey);
    return ret;
}

static void set_lang(nsIPrefBranch *pref)
{
    char langs[100];
    DWORD res, size, type;
    HKEY hkey;
    nsresult nsres;

    static const WCHAR international_keyW[] =
        {'S','o','f','t','w','a','r','e',
         '\\','M','i','c','r','o','s','o','f','t',
         '\\','I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',
         '\\','I','n','t','e','r','n','a','t','i','o','n','a','l',0};

    res = RegOpenKeyW(HKEY_CURRENT_USER, international_keyW, &hkey);
    if(res != ERROR_SUCCESS)
        return;

    size = sizeof(langs);
    res = RegQueryValueExA(hkey, "AcceptLanguage", 0, &type, (LPBYTE)langs, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return;

    TRACE("Setting lang %s\n", debugstr_a(langs));

    nsres = nsIPrefBranch_SetCharPref(pref, "intl.accept_languages", langs);
    if(NS_FAILED(nsres))
        ERR("SetCharPref failed: %08x\n", nsres);
}

static void set_proxy(nsIPrefBranch *pref)
{
    char proxy[512];
    char * proxy_port;
    int proxy_port_num;
    DWORD enabled = 0, res, size, type;
    HKEY hkey;
    nsresult nsres;

    static const WCHAR proxy_keyW[] =
        {'S','o','f','t','w','a','r','e',
         '\\','M','i','c','r','o','s','o','f','t',
         '\\','W','i','n','d','o','w','s',
         '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
         '\\','I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0};

    res = RegOpenKeyW(HKEY_CURRENT_USER, proxy_keyW, &hkey);
    if(res != ERROR_SUCCESS)
        return;

    size = sizeof(enabled);
    res = RegQueryValueExA(hkey, "ProxyEnable", 0, &type, (LPBYTE)&enabled, &size);
    if(res != ERROR_SUCCESS || type != REG_DWORD || enabled == 0)
    {
        RegCloseKey(hkey);
        return;
    }

    size = sizeof(proxy);
    res = RegQueryValueExA(hkey, "ProxyServer", 0, &type, (LPBYTE)proxy, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return;

    proxy_port = strchr(proxy, ':');
    if (!proxy_port)
        return;

    *proxy_port = 0;
    proxy_port_num = atoi(proxy_port + 1);
    TRACE("Setting proxy to %s, port %d\n", debugstr_a(proxy), proxy_port_num);

    nsres = nsIPrefBranch_SetIntPref(pref, "network.proxy.type", 1);
    if(NS_FAILED(nsres))
        ERR("SetIntPref network.proxy.type failed: %08x\n", nsres);
    nsres = nsIPrefBranch_SetCharPref(pref, "network.proxy.http", proxy);
    if(NS_FAILED(nsres))
        ERR("SetCharPref network.proxy.http failed: %08x\n", nsres);
    nsres = nsIPrefBranch_SetIntPref(pref, "network.proxy.http_port", proxy_port_num);
    if(NS_FAILED(nsres))
        ERR("SetIntPref network.proxy.http_port failed: %08x\n", nsres);
    nsres = nsIPrefBranch_SetCharPref(pref, "network.proxy.ssl", proxy);
    if(NS_FAILED(nsres))
        ERR("SetCharPref network.proxy.ssl failed: %08x\n", nsres);
    nsres = nsIPrefBranch_SetIntPref(pref, "network.proxy.ssl_port", proxy_port_num);
    if(NS_FAILED(nsres))
        ERR("SetIntPref network.proxy.ssl_port failed: %08x\n", nsres);
}

static void set_bool_pref(nsIPrefBranch *pref, const char *pref_name, BOOL val)
{
    nsresult nsres;

    nsres = nsIPrefBranch_SetBoolPref(pref, pref_name, val);
    if(NS_FAILED(nsres))
        ERR("Could not set pref %s\n", debugstr_a(pref_name));
}

static void set_preferences(void)
{
    nsIPrefBranch *pref;
    nsresult nsres;

    nsres = nsIServiceManager_GetServiceByContractID(pServMgr, NS_PREFERENCES_CONTRACTID,
            &IID_nsIPrefBranch, (void**)&pref);
    if(NS_FAILED(nsres)) {
        ERR("Could not get preference service: %08x\n", nsres);
        return;
    }

    set_lang(pref);
    set_proxy(pref);
    set_bool_pref(pref, "security.warn_entering_secure", FALSE);
    set_bool_pref(pref, "security.warn_submit_insecure", FALSE);

    nsIPrefBranch_Release(pref);
}

static BOOL init_xpcom(const PRUnichar *gre_path)
{
    nsresult nsres;
    nsIObserver *pStartNotif;
    nsIComponentRegistrar *registrar = NULL;
    nsAString path;
    nsIFile *gre_dir;

    nsAString_Init(&path, gre_path);
    nsres = NS_NewLocalFile(&path, FALSE, &gre_dir);
    nsAString_Finish(&path);
    if(NS_FAILED(nsres)) {
        ERR("NS_NewLocalFile failed: %08x\n", nsres);
        FreeLibrary(hXPCOM);
        return FALSE;
    }

    nsres = NS_InitXPCOM2(&pServMgr, gre_dir, NULL);
    if(NS_FAILED(nsres)) {
        ERR("NS_InitXPCOM2 failed: %08x\n", nsres);
        FreeLibrary(hXPCOM);
        return FALSE;
    }

    nsres = nsIServiceManager_QueryInterface(pServMgr, &IID_nsIComponentManager, (void**)&pCompMgr);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIComponentManager: %08x\n", nsres);

    nsres = NS_GetComponentRegistrar(&registrar);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIComponentRegistrar_AutoRegister(registrar, NULL);
        if(NS_FAILED(nsres))
            ERR("AutoRegister(NULL) failed: %08x\n", nsres);

        nsres = nsIComponentRegistrar_AutoRegister(registrar, gre_dir);
        if(NS_FAILED(nsres))
            ERR("AutoRegister(gre_dir) failed: %08x\n", nsres);

        init_nsio(pCompMgr, registrar);
    }else {
        ERR("NS_GetComponentRegistrar failed: %08x\n", nsres);
    }

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_APPSTARTUPNOTIFIER_CONTRACTID,
            NULL, &IID_nsIObserver, (void**)&pStartNotif);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIObserver_Observe(pStartNotif, NULL, APPSTARTUP_TOPIC, NULL);
        if(NS_FAILED(nsres))
            ERR("Observe failed: %08x\n", nsres);

        nsIObserver_Release(pStartNotif);
    }else {
        ERR("could not get appstartup-notifier: %08x\n", nsres);
    }

    set_preferences();

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_MEMORY_CONTRACTID,
            NULL, &IID_nsIMemory, (void**)&nsmem);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIMemory: %08x\n", nsres);

    if(registrar) {
        register_nsservice(registrar, pServMgr);
        nsIComponentRegistrar_Release(registrar);
    }

    return TRUE;
}

static CRITICAL_SECTION cs_load_gecko;
static CRITICAL_SECTION_DEBUG cs_load_gecko_dbg =
{
    0, 0, &cs_load_gecko,
    { &cs_load_gecko_dbg.ProcessLocksList, &cs_load_gecko_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": load_gecko") }
};
static CRITICAL_SECTION cs_load_gecko = { &cs_load_gecko_dbg, -1, 0, 0, 0, 0 };

BOOL load_gecko(BOOL silent)
{
    PRUnichar gre_path[MAX_PATH];
    BOOL ret = FALSE;

    static DWORD loading_thread;

    TRACE("()\n");

    /* load_gecko may be called recursively */
    if(loading_thread == GetCurrentThreadId())
        return pCompMgr != NULL;

    EnterCriticalSection(&cs_load_gecko);

    if(!loading_thread) {
        loading_thread = GetCurrentThreadId();

        if(load_wine_gecko(gre_path)
           || (install_wine_gecko(silent) && load_wine_gecko(gre_path)))
            ret = init_xpcom(gre_path);
        else
           MESSAGE("Could not load Mozilla. HTML rendering will be disabled.\n");
    }else {
        ret = pCompMgr != NULL;
    }

    LeaveCriticalSection(&cs_load_gecko);

    return ret;
}

void *nsalloc(size_t size)
{
    return nsIMemory_Alloc(nsmem, size);
}

void nsfree(void *mem)
{
    nsIMemory_Free(nsmem, mem);
}

void nsACString_Init(nsACString *str, const char *data)
{
    NS_CStringContainerInit(str);
    if(data)
        nsACString_SetData(str, data);
}

void nsACString_SetData(nsACString *str, const char *data)
{
    NS_CStringSetData(str, data, PR_UINT32_MAX);
}

PRUint32 nsACString_GetData(const nsACString *str, const char **data)
{
    return NS_CStringGetData(str, data, NULL);
}

void nsACString_Finish(nsACString *str)
{
    NS_CStringContainerFinish(str);
}

void nsAString_Init(nsAString *str, const PRUnichar *data)
{
    NS_StringContainerInit(str);
    if(data)
        nsAString_SetData(str, data);
}

void nsAString_SetData(nsAString *str, const PRUnichar *data)
{
    NS_StringSetData(str, data, PR_UINT32_MAX);
}

PRUint32 nsAString_GetData(const nsAString *str, const PRUnichar **data)
{
    return NS_StringGetData(str, data, NULL);
}

void nsAString_Finish(nsAString *str)
{
    NS_StringContainerFinish(str);
}

nsIInputStream *create_nsstream(const char *data, PRInt32 data_len)
{
    nsIStringInputStream *ret;
    nsresult nsres;

    if(!pCompMgr)
        return NULL;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_STRINGSTREAM_CONTRACTID, NULL, &IID_nsIStringInputStream,
            (void**)&ret);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIStringInputStream\n");
        return NULL;
    }

    nsres = nsIStringInputStream_SetData(ret, data, data_len);
    if(NS_FAILED(nsres)) {
        ERR("AdoptData failed: %08x\n", nsres);
        nsIStringInputStream_Release(ret);
        return NULL;
    }

    return (nsIInputStream*)ret;
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

static void nsnode_to_nsstring_rec(nsIContentSerializer *serializer, nsIDOMNode *nsnode, nsAString *str)
{
    nsIDOMNodeList *node_list = NULL;
    PRBool has_children = FALSE;
    PRUint16 type;
    nsresult nsres;

    nsIDOMNode_HasChildNodes(nsnode, &has_children);

    nsres = nsIDOMNode_GetNodeType(nsnode, &type);
    if(NS_FAILED(nsres)) {
        ERR("GetType failed: %08x\n", nsres);
        return;
    }

    switch(type) {
    case ELEMENT_NODE: {
        nsIDOMElement *nselem;
        nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
        nsIContentSerializer_AppendElementStart(serializer, nselem, nselem, str);
        nsIDOMElement_Release(nselem);
        break;
    }
    case TEXT_NODE: {
        nsIDOMText *nstext;
        nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMText, (void**)&nstext);
        nsIContentSerializer_AppendText(serializer, nstext, 0, -1, str);
        nsIDOMText_Release(nstext);
        break;
    }
    case COMMENT_NODE: {
        nsIDOMComment *nscomment;
        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMComment, (void**)&nscomment);
        nsres = nsIContentSerializer_AppendComment(serializer, nscomment, 0, -1, str);
        break;
    }
    case DOCUMENT_NODE: {
        nsIDOMDocument *nsdoc;
        nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMDocument, (void**)&nsdoc);
        nsIContentSerializer_AppendDocumentStart(serializer, nsdoc, str);
        nsIDOMDocument_Release(nsdoc);
        break;
    }
    case DOCUMENT_FRAGMENT_NODE:
        break;
    default:
        FIXME("Unhandled type %u\n", type);
    }

    if(has_children) {
        PRUint32 child_cnt, i;
        nsIDOMNode *child_node;

        nsIDOMNode_GetChildNodes(nsnode, &node_list);
        nsIDOMNodeList_GetLength(node_list, &child_cnt);

        for(i=0; i<child_cnt; i++) {
            nsres = nsIDOMNodeList_Item(node_list, i, &child_node);
            if(NS_SUCCEEDED(nsres)) {
                nsnode_to_nsstring_rec(serializer, child_node, str);
                nsIDOMNode_Release(child_node);
            }else {
                ERR("Item failed: %08x\n", nsres);
            }
        }

        nsIDOMNodeList_Release(node_list);
    }

    if(type == ELEMENT_NODE) {
        nsIDOMElement *nselem;
        nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
        nsIContentSerializer_AppendElementEnd(serializer, nselem, str);
        nsIDOMElement_Release(nselem);
    }
}

void nsnode_to_nsstring(nsIDOMNode *nsdoc, nsAString *str)
{
    nsIContentSerializer *serializer;
    nsIDOMNode *nsnode;
    nsresult nsres;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_HTMLSERIALIZER_CONTRACTID, NULL, &IID_nsIContentSerializer,
            (void**)&serializer);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIContentSerializer: %08x\n", nsres);
        return;
    }

    nsres = nsIContentSerializer_Init(serializer, 0, 100, NULL, FALSE, FALSE /* FIXME */);
    if(NS_FAILED(nsres))
        ERR("Init failed: %08x\n", nsres);

    nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMNode, (void**)&nsnode);
    nsnode_to_nsstring_rec(serializer, nsnode, str);
    nsIDOMNode_Release(nsnode);

    nsres = nsIContentSerializer_Flush(serializer, str);
    if(NS_FAILED(nsres))
        ERR("Flush failed: %08x\n", nsres);

    nsIContentSerializer_Release(serializer);
}

void get_editor_controller(NSContainer *This)
{
    nsIEditingSession *editing_session = NULL;
    nsIControllerContext *ctrlctx;
    nsresult nsres;

    if(This->editor) {
        nsIEditor_Release(This->editor);
        This->editor = NULL;
    }

    if(This->editor_controller) {
        nsIController_Release(This->editor_controller);
        This->editor_controller = NULL;
    }

    nsres = get_nsinterface((nsISupports*)This->webbrowser, &IID_nsIEditingSession,
            (void**)&editing_session);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIEditingSession: %08x\n", nsres);
        return;
    }

    nsres = nsIEditingSession_GetEditorForWindow(editing_session,
            This->doc->window->nswindow, &This->editor);
    nsIEditingSession_Release(editing_session);
    if(NS_FAILED(nsres)) {
        ERR("Could not get editor: %08x\n", nsres);
        return;
    }

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr,
            NS_EDITORCONTROLLER_CONTRACTID, NULL, &IID_nsIControllerContext, (void**)&ctrlctx);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIControllerContext_SetCommandContext(ctrlctx, (nsISupports *)This->editor);
        if(NS_FAILED(nsres))
            ERR("SetCommandContext failed: %08x\n", nsres);
        nsres = nsIControllerContext_QueryInterface(ctrlctx, &IID_nsIController,
                (void**)&This->editor_controller);
        nsIControllerContext_Release(ctrlctx);
        if(NS_FAILED(nsres))
            ERR("Could not get nsIController interface: %08x\n", nsres);
    }else {
        ERR("Could not create edit controller: %08x\n", nsres);
    }
}

void set_ns_editmode(NSContainer *This)
{
    nsIEditingSession *editing_session = NULL;
    nsIURIContentListener *listener = NULL;
    nsIDOMWindow *dom_window = NULL;
    nsresult nsres;

    nsres = get_nsinterface((nsISupports*)This->webbrowser, &IID_nsIEditingSession,
            (void**)&editing_session);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIEditingSession: %08x\n", nsres);
        return;
    }

    nsres = nsIWebBrowser_GetContentDOMWindow(This->webbrowser, &dom_window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get content DOM window: %08x\n", nsres);
        nsIEditingSession_Release(editing_session);
        return;
    }

    nsres = nsIEditingSession_MakeWindowEditable(editing_session, dom_window,
            NULL, FALSE, TRUE, TRUE);
    nsIEditingSession_Release(editing_session);
    nsIDOMWindow_Release(dom_window);
    if(NS_FAILED(nsres)) {
        ERR("MakeWindowEditable failed: %08x\n", nsres);
        return;
    }

    /* MakeWindowEditable changes WebBrowser's parent URI content listener.
     * It seams to be a bug in Gecko. To workaround it we set our content
     * listener again and Gecko's one as its parent.
     */
    nsIWebBrowser_GetParentURIContentListener(This->webbrowser, &listener);
    nsIURIContentListener_SetParentContentListener(NSURICL(This), listener);
    nsIURIContentListener_Release(listener);
    nsIWebBrowser_SetParentURIContentListener(This->webbrowser, NSURICL(This));
}

void update_nsdocument(HTMLDocument *doc)
{
    nsIDOMHTMLDocument *nsdoc;
    nsIDOMDocument *nsdomdoc;
    nsresult nsres;

    if(!doc->nscontainer || !doc->nscontainer->navigation)
        return;

    nsres = nsIWebNavigation_GetDocument(doc->nscontainer->navigation, &nsdomdoc);
    if(NS_FAILED(nsres) || !nsdomdoc) {
        ERR("GetDocument failed: %08x\n", nsres);
        return;
    }

    nsres = nsIDOMDocument_QueryInterface(nsdomdoc, &IID_nsIDOMHTMLDocument, (void**)&nsdoc);
    nsIDOMDocument_Release(nsdomdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLDocument iface: %08x\n", nsres);
        return;
    }

    if(nsdoc == doc->nsdoc) {
        nsIDOMHTMLDocument_Release(nsdoc);
        return;
    }

    if(doc->nsdoc)
        nsIDOMHTMLDocument_Release(doc->nsdoc);

    doc->nsdoc = nsdoc;

    if(nsdoc)
        set_mutation_observer(doc->nscontainer, nsdoc);
}

void close_gecko(void)
{
    TRACE("()\n");

    release_nsio();

    if(pCompMgr)
        nsIComponentManager_Release(pCompMgr);

    if(pServMgr)
        nsIServiceManager_Release(pServMgr);

    if(nsmem)
        nsIMemory_Release(nsmem);

    if(hXPCOM)
        FreeLibrary(hXPCOM);
}

/**********************************************************
 *      nsIWebBrowserChrome interface
 */

#define NSWBCHROME_THIS(iface) DEFINE_THIS(NSContainer, WebBrowserChrome, iface)

static nsresult NSAPI nsWebBrowserChrome_QueryInterface(nsIWebBrowserChrome *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

    *result = NULL;
    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = NSWBCHROME(This);
    }else if(IsEqualGUID(&IID_nsIWebBrowserChrome, riid)) {
        TRACE("(%p)->(IID_nsIWebBrowserChrome, %p)\n", This, result);
        *result = NSWBCHROME(This);
    }else if(IsEqualGUID(&IID_nsIContextMenuListener, riid)) {
        TRACE("(%p)->(IID_nsIContextMenuListener, %p)\n", This, result);
        *result = NSCML(This);
    }else if(IsEqualGUID(&IID_nsIURIContentListener, riid)) {
        TRACE("(%p)->(IID_nsIURIContentListener %p)\n", This, result);
        *result = NSURICL(This);
    }else if(IsEqualGUID(&IID_nsIEmbeddingSiteWindow, riid)) {
        TRACE("(%p)->(IID_nsIEmbeddingSiteWindow %p)\n", This, result);
        *result = NSEMBWNDS(This);
    }else if(IsEqualGUID(&IID_nsITooltipListener, riid)) {
        TRACE("(%p)->(IID_nsITooltipListener %p)\n", This, result);
        *result = NSTOOLTIP(This);
    }else if(IsEqualGUID(&IID_nsIInterfaceRequestor, riid)) {
        TRACE("(%p)->(IID_nsIInterfaceRequestor %p)\n", This, result);
        *result = NSIFACEREQ(This);
    }else if(IsEqualGUID(&IID_nsIWeakReference, riid)) {
        TRACE("(%p)->(IID_nsIWeakReference %p)\n", This, result);
        *result = NSWEAKREF(This);
    }else if(IsEqualGUID(&IID_nsISupportsWeakReference, riid)) {
        TRACE("(%p)->(IID_nsISupportsWeakReference %p)\n", This, result);
        *result = NSSUPWEAKREF(This);
    }

    if(*result) {
        nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsWebBrowserChrome_AddRef(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsWebBrowserChrome_Release(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This->event_vector);
        if(This->parent)
            nsIWebBrowserChrome_Release(NSWBCHROME(This->parent));
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsWebBrowserChrome_SetStatus(nsIWebBrowserChrome *iface,
        PRUint32 statusType, const PRUnichar *status)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

    TRACE("(%p)->(%d %s)\n", This, statusType, debugstr_w(status));

    /* FIXME: This hack should be removed when we'll load all pages by URLMoniker */
    if(This->doc)
        update_nsdocument(This->doc);

    return NS_OK;
}

static nsresult NSAPI nsWebBrowserChrome_GetWebBrowser(nsIWebBrowserChrome *iface,
        nsIWebBrowser **aWebBrowser)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

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
    NSContainer *This = NSWBCHROME_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aWebBrowser);

    if(aWebBrowser != This->webbrowser)
        ERR("Wrong nsWebBrowser!\n");

    return NS_OK;
}

static nsresult NSAPI nsWebBrowserChrome_GetChromeFlags(nsIWebBrowserChrome *iface,
        PRUint32 *aChromeFlags)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%p)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SetChromeFlags(nsIWebBrowserChrome *iface,
        PRUint32 aChromeFlags)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%08x)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_DestroyBrowserWindow(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    TRACE("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SizeBrowserTo(nsIWebBrowserChrome *iface,
        PRInt32 aCX, PRInt32 aCY)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%d %d)\n", This, aCX, aCY);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ShowAsModal(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_IsWindowModal(nsIWebBrowserChrome *iface, PRBool *_retval)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ExitModalEventLoop(nsIWebBrowserChrome *iface,
        nsresult aStatus)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%08x)\n", This, aStatus);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSWBCHROME_THIS

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

#define NSCML_THIS(iface) DEFINE_THIS(NSContainer, ContextMenuListener, iface)

static nsresult NSAPI nsContextMenuListener_QueryInterface(nsIContextMenuListener *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsContextMenuListener_AddRef(nsIContextMenuListener *iface)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsContextMenuListener_Release(nsIContextMenuListener *iface)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsContextMenuListener_OnShowContextMenu(nsIContextMenuListener *iface,
        PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    NSContainer *This = NSCML_THIS(iface);
    nsIDOMMouseEvent *event;
    POINT pt;
    DWORD dwID = CONTEXT_MENU_DEFAULT;
    nsresult nsres;

    TRACE("(%p)->(%08x %p %p)\n", This, aContextFlags, aEvent, aNode);

    nsres = nsIDOMEvent_QueryInterface(aEvent, &IID_nsIDOMMouseEvent, (void**)&event);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMMouseEvent interface: %08x\n", nsres);
        return nsres;
    }

    nsIDOMMouseEvent_GetScreenX(event, &pt.x);
    nsIDOMMouseEvent_GetScreenY(event, &pt.y);
    nsIDOMMouseEvent_Release(event);

    switch(aContextFlags) {
    case CONTEXT_NONE:
    case CONTEXT_DOCUMENT:
    case CONTEXT_TEXT:
        dwID = CONTEXT_MENU_DEFAULT;
        break;
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

    show_context_menu(This->doc, dwID, &pt, (IDispatch*)HTMLDOMNODE(get_node(This->doc, aNode, TRUE)));

    return NS_OK;
}

#undef NSCML_THIS

static const nsIContextMenuListenerVtbl nsContextMenuListenerVtbl = {
    nsContextMenuListener_QueryInterface,
    nsContextMenuListener_AddRef,
    nsContextMenuListener_Release,
    nsContextMenuListener_OnShowContextMenu
};

/**********************************************************
 *      nsIURIContentListener interface
 */

#define NSURICL_THIS(iface) DEFINE_THIS(NSContainer, URIContentListener, iface)

static nsresult NSAPI nsURIContentListener_QueryInterface(nsIURIContentListener *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsURIContentListener_AddRef(nsIURIContentListener *iface)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsURIContentListener_Release(nsIURIContentListener *iface)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsURIContentListener_OnStartURIOpen(nsIURIContentListener *iface,
                                                          nsIURI *aURI, PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);
    nsIWineURI *wine_uri;
    nsACString spec_str;
    const char *spec;
    nsresult nsres;

    nsACString_Init(&spec_str, NULL);
    nsIURI_GetSpec(aURI, &spec_str);
    nsACString_GetData(&spec_str, &spec);

    TRACE("(%p)->(%p(%s) %p)\n", This, aURI, debugstr_a(spec), _retval);

    nsACString_Finish(&spec_str);

    nsres = nsIURI_QueryInterface(aURI, &IID_nsIWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("Could not get nsIWineURI interface: %08x\n", nsres);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    nsIWineURI_SetNSContainer(wine_uri, This);
    nsIWineURI_SetIsDocumentURI(wine_uri, TRUE);

    if(This->bscallback) {
        IMoniker *mon = get_channelbsc_mon(This->bscallback);

        if(mon) {
            LPWSTR wine_url;
            HRESULT hres;

            hres = IMoniker_GetDisplayName(mon, NULL, 0, &wine_url);
            if(SUCCEEDED(hres)) {
                nsIWineURI_SetWineURL(wine_uri, wine_url);
                CoTaskMemFree(wine_url);
            }else {
                WARN("GetDisplayName failed: %08x\n", hres);
            }

            IMoniker_Release(mon);
        }
    }

    nsIWineURI_Release(wine_uri);

    *_retval = FALSE;
    return This->content_listener
        ? nsIURIContentListener_OnStartURIOpen(This->content_listener, aURI, _retval)
        : NS_OK;
}

static nsresult NSAPI nsURIContentListener_DoContent(nsIURIContentListener *iface,
        const char *aContentType, PRBool aIsContentPreferred, nsIRequest *aRequest,
        nsIStreamListener **aContentHandler, PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);

    TRACE("(%p)->(%s %x %p %p %p)\n", This, debugstr_a(aContentType), aIsContentPreferred,
            aRequest, aContentHandler, _retval);

    return This->content_listener
        ? nsIURIContentListener_DoContent(This->content_listener, aContentType,
                  aIsContentPreferred, aRequest, aContentHandler, _retval)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_IsPreferred(nsIURIContentListener *iface,
        const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_a(aContentType), aDesiredContentType, _retval);

    /* FIXME: Should we do something here? */
    *_retval = TRUE; 

    return This->content_listener
        ? nsIURIContentListener_IsPreferred(This->content_listener, aContentType,
                  aDesiredContentType, _retval)
        : NS_OK;
}

static nsresult NSAPI nsURIContentListener_CanHandleContent(nsIURIContentListener *iface,
        const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType,
        PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);

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
    NSContainer *This = NSURICL_THIS(iface);

    WARN("(%p)->(%p)\n", This, aLoadCookie);

    return This->content_listener
        ? nsIURIContentListener_GetLoadCookie(This->content_listener, aLoadCookie)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_SetLoadCookie(nsIURIContentListener *iface,
        nsISupports *aLoadCookie)
{
    NSContainer *This = NSURICL_THIS(iface);

    WARN("(%p)->(%p)\n", This, aLoadCookie);

    return This->content_listener
        ? nsIURIContentListener_SetLoadCookie(This->content_listener, aLoadCookie)
        : NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_GetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener **aParentContentListener)
{
    NSContainer *This = NSURICL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aParentContentListener);

    if(This->content_listener)
        nsIURIContentListener_AddRef(This->content_listener);

    *aParentContentListener = This->content_listener;
    return NS_OK;
}

static nsresult NSAPI nsURIContentListener_SetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener *aParentContentListener)
{
    NSContainer *This = NSURICL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aParentContentListener);

    if(aParentContentListener == NSURICL(This))
        return NS_OK;

    if(This->content_listener)
        nsIURIContentListener_Release(This->content_listener);

    This->content_listener = aParentContentListener;
    if(This->content_listener)
        nsIURIContentListener_AddRef(This->content_listener);

    return NS_OK;
}

#undef NSURICL_THIS

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

#define NSEMBWNDS_THIS(iface) DEFINE_THIS(NSContainer, EmbeddingSiteWindow, iface)

static nsresult NSAPI nsEmbeddingSiteWindow_QueryInterface(nsIEmbeddingSiteWindow *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_AddRef(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_Release(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetDimensions(nsIEmbeddingSiteWindow *iface,
        PRUint32 flags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%08x %d %d %d %d)\n", This, flags, x, y, cx, cy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetDimensions(nsIEmbeddingSiteWindow *iface,
        PRUint32 flags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%08x %p %p %p %p)\n", This, flags, x, y, cx, cy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetFocus(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);

    TRACE("(%p)\n", This);

    if(This->reset_focus)
        PostMessageW(This->hwnd, WM_RESETFOCUS_HACK, 0, 0);

    return nsIBaseWindow_SetFocus(This->window);
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetVisibility(nsIEmbeddingSiteWindow *iface,
        PRBool *aVisibility)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aVisibility);

    *aVisibility = This->doc && This->doc->hwnd && IsWindowVisible(This->doc->hwnd);
    return NS_OK;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetVisibility(nsIEmbeddingSiteWindow *iface,
        PRBool aVisibility)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);

    TRACE("(%p)->(%x)\n", This, aVisibility);

    return NS_OK;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetTitle(nsIEmbeddingSiteWindow *iface,
        PRUnichar **aTitle)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%p)\n", This, aTitle);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetTitle(nsIEmbeddingSiteWindow *iface,
        const PRUnichar *aTitle)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%s)\n", This, debugstr_w(aTitle));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetSiteWindow(nsIEmbeddingSiteWindow *iface,
        void **aSiteWindow)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);

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

#define NSTOOLTIP_THIS(iface) DEFINE_THIS(NSContainer, TooltipListener, iface)

static nsresult NSAPI nsTooltipListener_QueryInterface(nsITooltipListener *iface, nsIIDRef riid,
                                                       nsQIResult result)
{
    NSContainer *This = NSTOOLTIP_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsTooltipListener_AddRef(nsITooltipListener *iface)
{
    NSContainer *This = NSTOOLTIP_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsTooltipListener_Release(nsITooltipListener *iface)
{
    NSContainer *This = NSTOOLTIP_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsTooltipListener_OnShowTooltip(nsITooltipListener *iface,
        PRInt32 aXCoord, PRInt32 aYCoord, const PRUnichar *aTipText)
{
    NSContainer *This = NSTOOLTIP_THIS(iface);

    if (This->doc)
        show_tooltip(This->doc, aXCoord, aYCoord, aTipText);

    return NS_OK;
}

static nsresult NSAPI nsTooltipListener_OnHideTooltip(nsITooltipListener *iface)
{
    NSContainer *This = NSTOOLTIP_THIS(iface);

    if (This->doc)
        hide_tooltip(This->doc);

    return NS_OK;
}

#undef NSTOOLTIM_THIS

static const nsITooltipListenerVtbl nsTooltipListenerVtbl = {
    nsTooltipListener_QueryInterface,
    nsTooltipListener_AddRef,
    nsTooltipListener_Release,
    nsTooltipListener_OnShowTooltip,
    nsTooltipListener_OnHideTooltip
};

#define NSIFACEREQ_THIS(iface) DEFINE_THIS(NSContainer, InterfaceRequestor, iface)

static nsresult NSAPI nsInterfaceRequestor_QueryInterface(nsIInterfaceRequestor *iface,
                                                          nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSIFACEREQ_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsInterfaceRequestor_AddRef(nsIInterfaceRequestor *iface)
{
    NSContainer *This = NSIFACEREQ_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsInterfaceRequestor_Release(nsIInterfaceRequestor *iface)
{
    NSContainer *This = NSIFACEREQ_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsInterfaceRequestor_GetInterface(nsIInterfaceRequestor *iface,
                                                        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSIFACEREQ_THIS(iface);

    if(IsEqualGUID(&IID_nsIDOMWindow, riid)) {
        TRACE("(%p)->(IID_nsIDOMWindow %p)\n", This, result);
        return nsIWebBrowser_GetContentDOMWindow(This->webbrowser, (nsIDOMWindow**)result);
    }

    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

#undef NSIFACEREQ_THIS

static const nsIInterfaceRequestorVtbl nsInterfaceRequestorVtbl = {
    nsInterfaceRequestor_QueryInterface,
    nsInterfaceRequestor_AddRef,
    nsInterfaceRequestor_Release,
    nsInterfaceRequestor_GetInterface
};

#define NSWEAKREF_THIS(iface) DEFINE_THIS(NSContainer, WeakReference, iface)

static nsresult NSAPI nsWeakReference_QueryInterface(nsIWeakReference *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsWeakReference_AddRef(nsIWeakReference *iface)
{
    NSContainer *This = NSWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsWeakReference_Release(nsIWeakReference *iface)
{
    NSContainer *This = NSWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsWeakReference_QueryReferent(nsIWeakReference *iface,
        const nsIID *riid, void **result)
{
    NSContainer *This = NSWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

#undef NSWEAKREF_THIS

static const nsIWeakReferenceVtbl nsWeakReferenceVtbl = {
    nsWeakReference_QueryInterface,
    nsWeakReference_AddRef,
    nsWeakReference_Release,
    nsWeakReference_QueryReferent
};

#define NSSUPWEAKREF_THIS(iface) DEFINE_THIS(NSContainer, SupportsWeakReference, iface)

static nsresult NSAPI nsSupportsWeakReference_QueryInterface(nsISupportsWeakReference *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSSUPWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsSupportsWeakReference_AddRef(nsISupportsWeakReference *iface)
{
    NSContainer *This = NSSUPWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsSupportsWeakReference_Release(nsISupportsWeakReference *iface)
{
    NSContainer *This = NSSUPWEAKREF_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsSupportsWeakReference_GetWeakReference(nsISupportsWeakReference *iface,
        nsIWeakReference **_retval)
{
    NSContainer *This = NSSUPWEAKREF_THIS(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    nsIWeakReference_AddRef(NSWEAKREF(This));
    *_retval = NSWEAKREF(This);
    return NS_OK;
}

#undef NSWEAKREF_THIS

static const nsISupportsWeakReferenceVtbl nsSupportsWeakReferenceVtbl = {
    nsSupportsWeakReference_QueryInterface,
    nsSupportsWeakReference_AddRef,
    nsSupportsWeakReference_Release,
    nsSupportsWeakReference_GetWeakReference
};


NSContainer *NSContainer_Create(HTMLDocument *doc, NSContainer *parent)
{
    nsIWebBrowserSetup *wbsetup;
    nsIScrollable *scrollable;
    NSContainer *ret;
    nsresult nsres;

    if(!load_gecko(FALSE))
        return NULL;

    ret = heap_alloc_zero(sizeof(NSContainer));

    ret->lpWebBrowserChromeVtbl      = &nsWebBrowserChromeVtbl;
    ret->lpContextMenuListenerVtbl   = &nsContextMenuListenerVtbl;
    ret->lpURIContentListenerVtbl    = &nsURIContentListenerVtbl;
    ret->lpEmbeddingSiteWindowVtbl   = &nsEmbeddingSiteWindowVtbl;
    ret->lpTooltipListenerVtbl       = &nsTooltipListenerVtbl;
    ret->lpInterfaceRequestorVtbl    = &nsInterfaceRequestorVtbl;
    ret->lpWeakReferenceVtbl         = &nsWeakReferenceVtbl;
    ret->lpSupportsWeakReferenceVtbl = &nsSupportsWeakReferenceVtbl;

    ret->doc = doc;
    ret->ref = 1;
    init_mutation(ret);

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_WEBBROWSER_CONTRACTID,
            NULL, &IID_nsIWebBrowser, (void**)&ret->webbrowser);
    if(NS_FAILED(nsres)) {
        ERR("Creating WebBrowser failed: %08x\n", nsres);
        heap_free(ret);
        return NULL;
    }

    if(parent)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(parent));
    ret->parent = parent;

    nsres = nsIWebBrowser_SetContainerWindow(ret->webbrowser, NSWBCHROME(ret));
    if(NS_FAILED(nsres))
        ERR("SetContainerWindow failed: %08x\n", nsres);

    nsres = nsIWebBrowser_QueryInterface(ret->webbrowser, &IID_nsIBaseWindow,
            (void**)&ret->window);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIBaseWindow interface: %08x\n", nsres);

    nsres = nsIWebBrowser_QueryInterface(ret->webbrowser, &IID_nsIWebBrowserSetup,
                                         (void**)&wbsetup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(wbsetup, SETUP_IS_CHROME_WRAPPER, FALSE);
        nsIWebBrowserSetup_Release(wbsetup);
        if(NS_FAILED(nsres))
            ERR("SetProperty(SETUP_IS_CHROME_WRAPPER) failed: %08x\n", nsres);
    }else {
        ERR("Could not get nsIWebBrowserSetup interface\n");
    }

    nsres = nsIWebBrowser_QueryInterface(ret->webbrowser, &IID_nsIWebNavigation,
            (void**)&ret->navigation);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIWebNavigation interface: %08x\n", nsres);

    nsres = nsIWebBrowserFocus_QueryInterface(ret->webbrowser, &IID_nsIWebBrowserFocus,
            (void**)&ret->focus);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIWebBrowserFocus interface: %08x\n", nsres);

    if(!nscontainer_class)
        register_nscontainer_class();

    ret->hwnd = CreateWindowExW(0, wszNsContainer, NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 100, 100,
            GetDesktopWindow(), NULL, hInst, ret);

    nsres = nsIBaseWindow_InitWindow(ret->window, ret->hwnd, NULL, 0, 0, 100, 100);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIBaseWindow_Create(ret->window);
        if(NS_FAILED(nsres))
            WARN("Creating window failed: %08x\n", nsres);

        nsIBaseWindow_SetVisibility(ret->window, FALSE);
        nsIBaseWindow_SetEnabled(ret->window, FALSE);
    }else {
        ERR("InitWindow failed: %08x\n", nsres);
    }

    nsres = nsIWebBrowser_SetParentURIContentListener(ret->webbrowser, NSURICL(ret));
    if(NS_FAILED(nsres))
        ERR("SetParentURIContentListener failed: %08x\n", nsres);

    init_nsevents(ret);

    nsres = nsIWebBrowser_QueryInterface(ret->webbrowser, &IID_nsIScrollable, (void**)&scrollable);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable,
                ScrollOrientation_Y, Scrollbar_Always);
        if(NS_FAILED(nsres))
            ERR("Could not set default Y scrollbar prefs: %08x\n", nsres);

        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable,
                ScrollOrientation_X, Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default X scrollbar prefs: %08x\n", nsres);

        nsIScrollable_Release(scrollable);
    }else {
        ERR("Could not get nsIScrollable: %08x\n", nsres);
    }

    return ret;
}

void NSContainer_Release(NSContainer *This)
{
    TRACE("(%p)\n", This);

    This->doc = NULL;

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

    nsIWebBrowserChrome_Release(NSWBCHROME(This));
}
