/* 
 * IDxDiagProvider Implementation
 * 
 * Copyright 2004-2005 Raphael Junqueira
 * Copyright 2010 Andrew Nguyen
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
 *
 */

#include "dxdiag_private.h"

#include <winver.h>
#include <uuids.h>
#include <mmddk.h>
#include <d3d9.h>
#include <fil_data.h>
#include <psapi.h>
#include <wbemcli.h>
#include <dsound.h>

static const WCHAR szEmpty[] = {0};

static HRESULT build_information_tree(IDxDiagContainerImpl_Container **pinfo_root);
static void free_information_tree(IDxDiagContainerImpl_Container *node);

static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR szDeviceName[] = {'s','z','D','e','v','i','c','e','N','a','m','e',0};
static const WCHAR szKeyDeviceID[] = {'s','z','K','e','y','D','e','v','i','c','e','I','D',0};
static const WCHAR szKeyDeviceKey[] = {'s','z','K','e','y','D','e','v','i','c','e','K','e','y',0};
static const WCHAR szVendorId[] = {'s','z','V','e','n','d','o','r','I','d',0};
static const WCHAR szDeviceId[] = {'s','z','D','e','v','i','c','e','I','d',0};
static const WCHAR szDeviceIdentifier[] = {'s','z','D','e','v','i','c','e','I','d','e','n','t','i','f','i','e','r',0};
static const WCHAR dwWidth[] = {'d','w','W','i','d','t','h',0};
static const WCHAR dwHeight[] = {'d','w','H','e','i','g','h','t',0};
static const WCHAR dwBpp[] = {'d','w','B','p','p',0};
static const WCHAR szDisplayMemoryLocalized[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','L','o','c','a','l','i','z','e','d',0};
static const WCHAR szDisplayMemoryEnglish[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','E','n','g','l','i','s','h',0};
static const WCHAR szDriverName[] = {'s','z','D','r','i','v','e','r','N','a','m','e',0};
static const WCHAR szDriverVersion[] = {'s','z','D','r','i','v','e','r','V','e','r','s','i','o','n',0};
static const WCHAR szSubSysId[] = {'s','z','S','u','b','S','y','s','I','d',0};
static const WCHAR szRevisionId[] = {'s','z','R','e','v','i','s','i','o','n','I','d',0};
static const WCHAR dwRefreshRate[] = {'d','w','R','e','f','r','e','s','h','R','a','t','e',0};
static const WCHAR szManufacturer[] = {'s','z','M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR szChipType[] = {'s','z','C','h','i','p','T','y','p','e',0};
static const WCHAR szDACType[] = {'s','z','D','A','C','T','y','p','e',0};
static const WCHAR szRevision[] = {'s','z','R','e','v','i','s','i','o','n',0};

struct IDxDiagProviderImpl
{
  IDxDiagProvider IDxDiagProvider_iface;
  LONG ref;
  BOOL init;
  DXDIAG_INIT_PARAMS params;
  IDxDiagContainerImpl_Container *info_root;
};

static inline IDxDiagProviderImpl *impl_from_IDxDiagProvider(IDxDiagProvider *iface)
{
     return CONTAINING_RECORD(iface, IDxDiagProviderImpl, IDxDiagProvider_iface);
}

/* IDxDiagProvider IUnknown parts follow: */
static HRESULT WINAPI IDxDiagProviderImpl_QueryInterface(IDxDiagProvider *iface, REFIID riid,
        void **ppobj)
{
    IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);

    if (!ppobj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDxDiagProvider)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDxDiagProviderImpl_AddRef(IDxDiagProvider *iface)
{
    IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    DXDIAGN_LockModule();

    return refCount;
}

static ULONG WINAPI IDxDiagProviderImpl_Release(IDxDiagProvider *iface)
{
    IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        free_information_tree(This->info_root);
        HeapFree(GetProcessHeap(), 0, This);
    }

    DXDIAGN_UnlockModule();
    
    return refCount;
}

/* IDxDiagProvider Interface follow: */
static HRESULT WINAPI IDxDiagProviderImpl_Initialize(IDxDiagProvider *iface,
        DXDIAG_INIT_PARAMS *pParams)
{
    IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pParams);

    if (NULL == pParams) {
      return E_POINTER;
    }
    if (pParams->dwSize != sizeof(DXDIAG_INIT_PARAMS) ||
        pParams->dwDxDiagHeaderVersion != DXDIAG_DX9_SDK_VERSION) {
      return E_INVALIDARG;
    }

    if (!This->info_root)
    {
        hr = build_information_tree(&This->info_root);
        if (FAILED(hr))
            return hr;
    }

    This->init = TRUE;
    memcpy(&This->params, pParams, pParams->dwSize);
    return S_OK;
}

static HRESULT WINAPI IDxDiagProviderImpl_GetRootContainer(IDxDiagProvider *iface,
        IDxDiagContainer **ppInstance)
{
  IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);

  TRACE("(%p,%p)\n", iface, ppInstance);

  if (FALSE == This->init) {
    return CO_E_NOTINITIALIZED;
  }

  return DXDiag_CreateDXDiagContainer(&IID_IDxDiagContainer, This->info_root,
          &This->IDxDiagProvider_iface, (void **)ppInstance);
}

static const IDxDiagProviderVtbl DxDiagProvider_Vtbl =
{
    IDxDiagProviderImpl_QueryInterface,
    IDxDiagProviderImpl_AddRef,
    IDxDiagProviderImpl_Release,
    IDxDiagProviderImpl_Initialize,
    IDxDiagProviderImpl_GetRootContainer
};

HRESULT DXDiag_CreateDXDiagProvider(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDxDiagProviderImpl* provider;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);

  *ppobj = NULL;
  if (punkOuter) return CLASS_E_NOAGGREGATION;

  provider = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDxDiagProviderImpl));
  if (NULL == provider) return E_OUTOFMEMORY;
  provider->IDxDiagProvider_iface.lpVtbl = &DxDiagProvider_Vtbl;
  provider->ref = 0; /* will be inited with QueryInterface */
  return IDxDiagProviderImpl_QueryInterface(&provider->IDxDiagProvider_iface, riid, ppobj);
}

static void free_property_information(IDxDiagContainerImpl_Property *prop)
{
    VariantClear(&prop->vProp);
    HeapFree(GetProcessHeap(), 0, prop->propName);
    HeapFree(GetProcessHeap(), 0, prop);
}

static void free_information_tree(IDxDiagContainerImpl_Container *node)
{
    IDxDiagContainerImpl_Container *ptr, *cursor2;

    if (!node)
        return;

    HeapFree(GetProcessHeap(), 0, node->contName);

    LIST_FOR_EACH_ENTRY_SAFE(ptr, cursor2, &node->subContainers, IDxDiagContainerImpl_Container, entry)
    {
        IDxDiagContainerImpl_Property *prop, *prop_cursor2;

        LIST_FOR_EACH_ENTRY_SAFE(prop, prop_cursor2, &ptr->properties, IDxDiagContainerImpl_Property, entry)
        {
            list_remove(&prop->entry);
            free_property_information(prop);
        }

        list_remove(&ptr->entry);
        free_information_tree(ptr);
    }

    HeapFree(GetProcessHeap(), 0, node);
}

static IDxDiagContainerImpl_Container *allocate_information_node(const WCHAR *name)
{
    IDxDiagContainerImpl_Container *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    if (name)
    {
        ret->contName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1) * sizeof(*name));
        if (!ret->contName)
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        strcpyW(ret->contName, name);
    }

    list_init(&ret->subContainers);
    list_init(&ret->properties);

    return ret;
}

static IDxDiagContainerImpl_Property *allocate_property_information(const WCHAR *name)
{
    IDxDiagContainerImpl_Property *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->propName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1) * sizeof(*name));
    if (!ret->propName)
    {
        HeapFree(GetProcessHeap(), 0, ret);
        return NULL;
    }
    strcpyW(ret->propName, name);

    return ret;
}

static inline void add_subcontainer(IDxDiagContainerImpl_Container *node, IDxDiagContainerImpl_Container *subCont)
{
    list_add_tail(&node->subContainers, &subCont->entry);
    ++node->nSubContainers;
}

static inline HRESULT add_bstr_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, const WCHAR *str)
{
    IDxDiagContainerImpl_Property *prop;
    BSTR bstr;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    bstr = SysAllocString(str);
    if (!bstr)
    {
        free_property_information(prop);
        return E_OUTOFMEMORY;
    }

    V_VT(&prop->vProp) = VT_BSTR;
    V_BSTR(&prop->vProp) = bstr;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_ui4_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, DWORD data)
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_UI4;
    V_UI4(&prop->vProp) = data;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_bool_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, BOOL data)
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_BOOL;
    V_BOOL(&prop->vProp) = data ? VARIANT_TRUE : VARIANT_FALSE;

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

static inline HRESULT add_ull_as_bstr_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, ULONGLONG data )
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_UI8;
    V_UI8(&prop->vProp) = data;

    VariantChangeType(&prop->vProp, &prop->vProp, 0, VT_BSTR);

    list_add_tail(&node->properties, &prop->entry);
    ++node->nProperties;

    return S_OK;
}

/* Copied from programs/taskkill/taskkill.c. */
static DWORD *enumerate_processes(DWORD *list_count)
{
    DWORD *pid_list, alloc_bytes = 1024 * sizeof(*pid_list), needed_bytes;

    pid_list = HeapAlloc(GetProcessHeap(), 0, alloc_bytes);
    if (!pid_list)
        return NULL;

    for (;;)
    {
        DWORD *realloc_list;

        if (!EnumProcesses(pid_list, alloc_bytes, &needed_bytes))
        {
            HeapFree(GetProcessHeap(), 0, pid_list);
            return NULL;
        }

        /* EnumProcesses can't signal an insufficient buffer condition, so the
         * only way to possibly determine whether a larger buffer is required
         * is to see whether the written number of bytes is the same as the
         * buffer size. If so, the buffer will be reallocated to twice the
         * size. */
        if (alloc_bytes != needed_bytes)
            break;

        alloc_bytes *= 2;
        realloc_list = HeapReAlloc(GetProcessHeap(), 0, pid_list, alloc_bytes);
        if (!realloc_list)
        {
            HeapFree(GetProcessHeap(), 0, pid_list);
            return NULL;
        }
        pid_list = realloc_list;
    }

    *list_count = needed_bytes / sizeof(*pid_list);
    return pid_list;
}

/* Copied from programs/taskkill/taskkill.c. */
static BOOL get_process_name_from_pid(DWORD pid, WCHAR *buf, DWORD chars)
{
    HANDLE process;
    HMODULE module;
    DWORD required_size;

    process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process)
        return FALSE;

    if (!EnumProcessModules(process, &module, sizeof(module), &required_size))
    {
        CloseHandle(process);
        return FALSE;
    }

    if (!GetModuleBaseNameW(process, module, buf, chars))
    {
        CloseHandle(process);
        return FALSE;
    }

    CloseHandle(process);
    return TRUE;
}

/* dxdiagn's detection scheme is simply to look for a process called conf.exe. */
static BOOL is_netmeeting_running(void)
{
    static const WCHAR conf_exe[] = {'c','o','n','f','.','e','x','e',0};

    DWORD list_count;
    DWORD *pid_list = enumerate_processes(&list_count);

    if (pid_list)
    {
        DWORD i;
        WCHAR process_name[MAX_PATH];

        for (i = 0; i < list_count; i++)
        {
            if (get_process_name_from_pid(pid_list[i], process_name, sizeof(process_name)/sizeof(WCHAR)) &&
                !lstrcmpW(conf_exe, process_name))
            {
                HeapFree(GetProcessHeap(), 0, pid_list);
                return TRUE;
            }
        }
        HeapFree(GetProcessHeap(), 0, pid_list);
    }

    return FALSE;
}

static HRESULT fill_language_information(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR regional_setting_engW[] = {'R','e','g','i','o','n','a','l',' ','S','e','t','t','i','n','g',0};
    static const WCHAR languages_fmtW[] = {'%','s',' ','(','%','s',':',' ','%','s',')',0};
    static const WCHAR szLanguagesLocalized[] = {'s','z','L','a','n','g','u','a','g','e','s','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szLanguagesEnglish[] = {'s','z','L','a','n','g','u','a','g','e','s','E','n','g','l','i','s','h',0};

    WCHAR system_lang[80], regional_setting[100], user_lang[80], language_str[300];
    HRESULT hr;

    /* szLanguagesLocalized */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SNATIVELANGNAME, system_lang, sizeof(system_lang)/sizeof(WCHAR));
    LoadStringW(dxdiagn_instance, IDS_REGIONAL_SETTING, regional_setting, sizeof(regional_setting)/sizeof(WCHAR));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNATIVELANGNAME, user_lang, sizeof(user_lang)/sizeof(WCHAR));

    snprintfW(language_str, sizeof(language_str)/sizeof(WCHAR), languages_fmtW, system_lang, regional_setting, user_lang);

    hr = add_bstr_property(node, szLanguagesLocalized, language_str);
    if (FAILED(hr))
        return hr;

    /* szLanguagesEnglish */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE, system_lang, sizeof(system_lang)/sizeof(WCHAR));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, user_lang, sizeof(user_lang)/sizeof(WCHAR));

    snprintfW(language_str, sizeof(language_str)/sizeof(WCHAR), languages_fmtW, system_lang, regional_setting_engW, user_lang);

    hr = add_bstr_property(node, szLanguagesEnglish, language_str);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT fill_datetime_information(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR date_fmtW[] = {'M','\'','/','\'','d','\'','/','\'','y','y','y','y',0};
    static const WCHAR time_fmtW[] = {'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
    static const WCHAR datetime_fmtW[] = {'%','s',',',' ','%','s',0};
    static const WCHAR szTimeLocalized[] = {'s','z','T','i','m','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szTimeEnglish[] = {'s','z','T','i','m','e','E','n','g','l','i','s','h',0};

    SYSTEMTIME curtime;
    WCHAR date_str[80], time_str[80], datetime_str[200];
    HRESULT hr;

    GetLocalTime(&curtime);

    GetTimeFormatW(LOCALE_NEUTRAL, 0, &curtime, time_fmtW, time_str, sizeof(time_str)/sizeof(WCHAR));

    /* szTimeLocalized */
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &curtime, NULL, date_str, sizeof(date_str)/sizeof(WCHAR));

    snprintfW(datetime_str, sizeof(datetime_str)/sizeof(WCHAR), datetime_fmtW, date_str, time_str);

    hr = add_bstr_property(node, szTimeLocalized, datetime_str);
    if (FAILED(hr))
        return hr;

    /* szTimeEnglish */
    GetDateFormatW(LOCALE_NEUTRAL, 0, &curtime, date_fmtW, date_str, sizeof(date_str)/sizeof(WCHAR));

    snprintfW(datetime_str, sizeof(datetime_str)/sizeof(WCHAR), datetime_fmtW, date_str, time_str);

    hr = add_bstr_property(node, szTimeEnglish, datetime_str);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT fill_os_string_information(IDxDiagContainerImpl_Container *node, OSVERSIONINFOW *info)
{
    static const WCHAR winxpW[] = {'W','i','n','d','o','w','s',' ','X','P',' ','P','r','o','f','e','s','s','i','o','n','a','l',0};
    static const WCHAR szOSLocalized[] = {'s','z','O','S','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSExLocalized[] = {'s','z','O','S','E','x','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSExLongLocalized[] = {'s','z','O','S','E','x','L','o','n','g','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSEnglish[] = {'s','z','O','S','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExEnglish[] = {'s','z','O','S','E','x','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExLongEnglish[] = {'s','z','O','S','E','x','L','o','n','g','E','n','g','l','i','s','h',0};

    static const WCHAR *prop_list[] = {szOSLocalized, szOSExLocalized, szOSExLongLocalized,
                                       szOSEnglish, szOSExEnglish, szOSExLongEnglish};

    size_t i;
    HRESULT hr;

    /* FIXME: OS detection should be performed, and localized OS strings
     * should contain translated versions of the "build" phrase. */
    for (i = 0; i < sizeof(prop_list)/sizeof(prop_list[0]); i++)
    {
        hr = add_bstr_property(node, prop_list[i], winxpW);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static HRESULT fill_processor_information(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR szProcessorEnglish[] = {'s','z','P','r','o','c','e','s','s','o','r','E','n','g','l','i','s','h',0};

    static const WCHAR cimv2W[] = {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',0};
    static const WCHAR proc_classW[] = {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR max_clock_speedW[] = {'M','a','x','C','l','o','c','k','S','p','e','e','d',0};
    static const WCHAR cpu_noW[] = {'N','u','m','b','e','r','O','f','L','o','g','i','c','a','l','P','r','o','c','e','s','s','o','r','s',0};

    static const WCHAR processor_fmtW[] = {'%','s','(','%','d',' ','C','P','U','s',')',',',' ','~','%','d','M','H','z',0};

    IWbemLocator *wbem_locator;
    IWbemServices *wbem_service;
    IWbemClassObject *wbem_class;
    IEnumWbemClassObject *wbem_enum;
    VARIANT cpu_name, cpu_no, clock_speed;
    WCHAR print_buf[200];
    BSTR bstr;
    ULONG no;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void**)&wbem_locator);
    if(FAILED(hr))
        return hr;

    bstr = SysAllocString(cimv2W);
    if(!bstr) {
        IWbemLocator_Release(wbem_locator);
        return E_OUTOFMEMORY;
    }
    hr = IWbemLocator_ConnectServer(wbem_locator, bstr, NULL, NULL, NULL, 0, NULL, NULL, &wbem_service);
    IWbemLocator_Release(wbem_locator);
    SysFreeString(bstr);
    if(FAILED(hr))
        return hr;

    bstr = SysAllocString(proc_classW);
    if(!bstr) {
        IWbemServices_Release(wbem_service);
        return E_OUTOFMEMORY;
    }
    hr = IWbemServices_CreateInstanceEnum(wbem_service, bstr, WBEM_FLAG_SYSTEM_ONLY, NULL, &wbem_enum);
    IWbemServices_Release(wbem_service);
    SysFreeString(bstr);
    if(FAILED(hr))
        return hr;

    hr = IEnumWbemClassObject_Next(wbem_enum, 1000, 1, &wbem_class, &no);
    IEnumWbemClassObject_Release(wbem_enum);
    if(FAILED(hr))
        return hr;

    hr = IWbemClassObject_Get(wbem_class, cpu_noW, 0, &cpu_no, NULL, NULL);
    if(FAILED(hr)) {
        IWbemClassObject_Release(wbem_class);
        return hr;
    }
    hr = IWbemClassObject_Get(wbem_class, max_clock_speedW, 0, &clock_speed, NULL, NULL);
    if(FAILED(hr)) {
        IWbemClassObject_Release(wbem_class);
        return hr;
    }
    hr = IWbemClassObject_Get(wbem_class, nameW, 0, &cpu_name, NULL, NULL);
    IWbemClassObject_Release(wbem_class);
    if(FAILED(hr))
        return hr;

    sprintfW(print_buf, processor_fmtW, V_BSTR(&cpu_name), V_I4(&cpu_no), V_I4(&clock_speed));
    VariantClear(&cpu_name);
    VariantClear(&cpu_no);
    VariantClear(&clock_speed);

    return add_bstr_property(node, szProcessorEnglish, print_buf);
}

static HRESULT build_systeminfo_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR dwDirectXVersionMajor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','a','j','o','r',0};
    static const WCHAR dwDirectXVersionMinor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','i','n','o','r',0};
    static const WCHAR szDirectXVersionLetter[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','e','t','t','e','r',0};
    static const WCHAR szDirectXVersionLetter_v[] = {'c',0};
    static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
    static const WCHAR bNECPC98[] = {'b','N','E','C','P','C','9','8',0};
    static const WCHAR szDirectXVersionEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','E','n','g','l','i','s','h',0};
    static const WCHAR szDirectXVersionEnglish_v[] = {'4','.','0','9','.','0','0','0','0','.','0','9','0','4',0};
    static const WCHAR szDirectXVersionLongEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR szDirectXVersionLongEnglish_v[] = {'=',' ','"','D','i','r','e','c','t','X',' ','9','.','0','c',' ','(','4','.','0','9','.','0','0','0','0','.','0','9','0','4',')',0};
    static const WCHAR ullPhysicalMemory[] = {'u','l','l','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
    static const WCHAR ullUsedPageFile[]   = {'u','l','l','U','s','e','d','P','a','g','e','F','i','l','e',0};
    static const WCHAR ullAvailPageFile[]  = {'u','l','l','A','v','a','i','l','P','a','g','e','F','i','l','e',0};
    static const WCHAR bNetMeetingRunning[] = {'b','N','e','t','M','e','e','t','i','n','g','R','u','n','n','i','n','g',0};
    static const WCHAR szWindowsDir[] = {'s','z','W','i','n','d','o','w','s','D','i','r',0};
    static const WCHAR dwOSMajorVersion[] = {'d','w','O','S','M','a','j','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSMinorVersion[] = {'d','w','O','S','M','i','n','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSBuildNumber[] = {'d','w','O','S','B','u','i','l','d','N','u','m','b','e','r',0};
    static const WCHAR dwOSPlatformID[] = {'d','w','O','S','P','l','a','t','f','o','r','m','I','D',0};
    static const WCHAR szCSDVersion[] = {'s','z','C','S','D','V','e','r','s','i','o','n',0};
    static const WCHAR szPhysicalMemoryEnglish[] = {'s','z','P','h','y','s','i','c','a','l','M','e','m','o','r','y','E','n','g','l','i','s','h',0};
    static const WCHAR szPageFileLocalized[] = {'s','z','P','a','g','e','F','i','l','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szPageFileEnglish[] = {'s','z','P','a','g','e','F','i','l','e','E','n','g','l','i','s','h',0};
    static const WCHAR szMachineNameLocalized[] = {'s','z','M','a','c','h','i','n','e','N','a','m','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szMachineNameEnglish[] = {'s','z','M','a','c','h','i','n','e','N','a','m','e','E','n','g','l','i','s','h',0};
    static const WCHAR szSystemManufacturerEnglish[] = {'s','z','S','y','s','t','e','m','M','a','n','u','f','a','c','t','u','r','e','r','E','n','g','l','i','s','h',0};
    static const WCHAR szSystemModelEnglish[] = {'s','z','S','y','s','t','e','m','M','o','d','e','l','E','n','g','l','i','s','h',0};
    static const WCHAR szBIOSEnglish[] = {'s','z','B','I','O','S','E','n','g','l','i','s','h',0};
    static const WCHAR szSetupParamEnglish[] = {'s','z','S','e','t','u','p','P','a','r','a','m','E','n','g','l','i','s','h',0};
    static const WCHAR szDxDiagVersion[] = {'s','z','D','x','D','i','a','g','V','e','r','s','i','o','n',0};

    static const WCHAR notpresentW[] = {'N','o','t',' ','p','r','e','s','e','n','t',0};

    static const WCHAR pagefile_fmtW[] = {'%','u','M','B',' ','u','s','e','d',',',' ','%','u','M','B',' ','a','v','a','i','l','a','b','l','e',0};
    static const WCHAR physmem_fmtW[] = {'%','u','M','B',' ','R','A','M',0};

    HRESULT hr;
    MEMORYSTATUSEX msex;
    OSVERSIONINFOW info;
    DWORD count, usedpage_mb, availpage_mb;
    WCHAR buffer[MAX_PATH], computer_name[MAX_COMPUTERNAME_LENGTH + 1], print_buf[200], localized_pagefile_fmt[200];
    DWORD_PTR args[2];

    hr = add_ui4_property(node, dwDirectXVersionMajor, 9);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwDirectXVersionMinor, 0);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionLetter, szDirectXVersionLetter_v);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionEnglish, szDirectXVersionEnglish_v);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDirectXVersionLongEnglish, szDirectXVersionLongEnglish_v);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, bDebug, FALSE);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, bNECPC98, FALSE);
    if (FAILED(hr))
        return hr;

    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx(&msex);

    hr = add_ull_as_bstr_property(node, ullPhysicalMemory, msex.ullTotalPhys);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, ullUsedPageFile, msex.ullTotalPageFile - msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, ullAvailPageFile, msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, bNetMeetingRunning, is_netmeeting_running());
    if (FAILED(hr))
        return hr;

    info.dwOSVersionInfoSize = sizeof(info);
    GetVersionExW(&info);

    hr = add_ui4_property(node, dwOSMajorVersion, info.dwMajorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSMinorVersion, info.dwMinorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSBuildNumber, info.dwBuildNumber);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, dwOSPlatformID, info.dwPlatformId);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szCSDVersion, info.szCSDVersion);
    if (FAILED(hr))
        return hr;

    /* FIXME: Roundoff should not be done with truncated division. */
    snprintfW(print_buf, sizeof(print_buf)/sizeof(WCHAR), physmem_fmtW, (DWORD)(msex.ullTotalPhys / (1024 * 1024)));
    hr = add_bstr_property(node, szPhysicalMemoryEnglish, print_buf);
    if (FAILED(hr))
        return hr;

    usedpage_mb = (DWORD)((msex.ullTotalPageFile - msex.ullAvailPageFile) / (1024 * 1024));
    availpage_mb = (DWORD)(msex.ullAvailPageFile / (1024 * 1024));
    LoadStringW(dxdiagn_instance, IDS_PAGE_FILE_FORMAT, localized_pagefile_fmt, sizeof(localized_pagefile_fmt)/sizeof(WCHAR));
    args[0] = usedpage_mb;
    args[1] = availpage_mb;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   localized_pagefile_fmt, 0, 0, print_buf,
                   sizeof(print_buf)/sizeof(*print_buf), (__ms_va_list*)args);

    hr = add_bstr_property(node, szPageFileLocalized, print_buf);
    if (FAILED(hr))
        return hr;

    snprintfW(print_buf, sizeof(print_buf)/sizeof(WCHAR), pagefile_fmtW, usedpage_mb, availpage_mb);

    hr = add_bstr_property(node, szPageFileEnglish, print_buf);
    if (FAILED(hr))
        return hr;

    GetWindowsDirectoryW(buffer, MAX_PATH);

    hr = add_bstr_property(node, szWindowsDir, buffer);
    if (FAILED(hr))
        return hr;

    count = sizeof(computer_name)/sizeof(WCHAR);
    if (!GetComputerNameW(computer_name, &count))
        return E_FAIL;

    hr = add_bstr_property(node, szMachineNameLocalized, computer_name);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szMachineNameEnglish, computer_name);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szSystemManufacturerEnglish, szEmpty);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szSystemModelEnglish, szEmpty);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szBIOSEnglish, szEmpty);
    if (FAILED(hr))
        return hr;

    hr = fill_processor_information(node);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szSetupParamEnglish, notpresentW);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, szDxDiagVersion, szEmpty);
    if (FAILED(hr))
        return hr;

    hr = fill_language_information(node);
    if (FAILED(hr))
        return hr;

    hr = fill_datetime_information(node);
    if (FAILED(hr))
        return hr;

    hr = fill_os_string_information(node, &info);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

/* The logic from pixelformat_for_depth() in dlls/wined3d/utils.c is reversed. */
static DWORD depth_for_pixelformat(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_P8: return 8;
    case D3DFMT_X1R5G5B5: return 15;
    case D3DFMT_R5G6B5: return 16;
    /* This case will fail to distinguish an original bpp of 24. */
    case D3DFMT_X8R8G8B8: return 32;
    default:
        FIXME("Unknown D3DFORMAT %d, returning 32 bpp\n", format);
        return 32;
    }
}

static BOOL get_texture_memory(GUID *adapter, DWORD *available_mem)
{
    IDirectDraw7 *pDirectDraw;
    HRESULT hr;
    DDSCAPS2 dd_caps;

    hr = DirectDrawCreateEx(adapter, (void **)&pDirectDraw, &IID_IDirectDraw7, NULL);
    if (SUCCEEDED(hr))
    {
        dd_caps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
        dd_caps.dwCaps2 = dd_caps.dwCaps3 = dd_caps.u1.dwCaps4 = 0;
        hr = IDirectDraw7_GetAvailableVidMem(pDirectDraw, &dd_caps, available_mem, NULL);
        IDirectDraw7_Release(pDirectDraw);
        if (SUCCEEDED(hr))
            return TRUE;
    }

    return FALSE;
}

static const WCHAR *vendor_id_to_manufacturer_string(DWORD vendor_id)
{
    static const WCHAR atiW[] = {'A','T','I',' ','T','e','c','h','n','o','l','o','g','i','e','s',' ','I','n','c','.',0};
    static const WCHAR nvidiaW[] = {'N','V','I','D','I','A',0};
    static const WCHAR intelW[] = {'I','n','t','e','l',' ','C','o','r','p','o','r','a','t','i','o','n',0};
    static const WCHAR unknownW[] = {'U','n','k','n','o','w','n',0};

    /* Enumeration copied from dlls/wined3d/wined3d_private.h and slightly modified. */
    enum pci_vendor
    {
        HW_VENDOR_AMD = 0x1002,
        HW_VENDOR_NVIDIA = 0x10de,
        HW_VENDOR_INTEL = 0x8086,
    };

    switch (vendor_id)
    {
    case HW_VENDOR_AMD:
        return atiW;
    case HW_VENDOR_NVIDIA:
        return nvidiaW;
    case HW_VENDOR_INTEL:
        return intelW;
    default:
        FIXME("Unknown PCI vendor ID 0x%04x\n", vendor_id);
        return unknownW;
    }
}

static HRESULT fill_display_information_d3d(IDxDiagContainerImpl_Container *node)
{
    IDxDiagContainerImpl_Container *display_adapter;
    HRESULT hr;
    IDirect3D9 *pDirect3D9;
    WCHAR buffer[256];
    UINT index, count;

    pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pDirect3D9)
        return E_FAIL;

    count = IDirect3D9_GetAdapterCount(pDirect3D9);
    for (index = 0; index < count; index++)
    {
        static const WCHAR adapterid_fmtW[] = {'%','u',0};
        static const WCHAR driverversion_fmtW[] = {'%','u','.','%','u','.','%','0','4','u','.','%','0','4','u',0};
        static const WCHAR id_fmtW[] = {'0','x','%','0','4','x',0};
        static const WCHAR subsysid_fmtW[] = {'0','x','%','0','8','x',0};
        static const WCHAR mem_fmt[] = {'%','.','1','f',' ','M','B',0};
        static const WCHAR b3DAccelerationExists[] = {'b','3','D','A','c','c','e','l','e','r','a','t','i','o','n','E','x','i','s','t','s',0};
        static const WCHAR b3DAccelerationEnabled[] = {'b','3','D','A','c','c','e','l','e','r','a','t','i','o','n','E','n','a','b','l','e','d',0};
        static const WCHAR bDDAccelerationEnabled[] = {'b','D','D','A','c','c','e','l','e','r','a','t','i','o','n','E','n','a','b','l','e','d',0};
        static const WCHAR bNoHardware[] = {'b','N','o','H','a','r','d','w','a','r','e',0};

        D3DADAPTER_IDENTIFIER9 adapter_info;
        D3DDISPLAYMODE adapter_mode;
        D3DCAPS9 device_caps;
        DWORD available_mem = 0;
        BOOL hardware_accel;

        snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), adapterid_fmtW, index);
        display_adapter = allocate_information_node(buffer);
        if (!display_adapter)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        add_subcontainer(node, display_adapter);

        hr = IDirect3D9_GetAdapterIdentifier(pDirect3D9, index, 0, &adapter_info);
        if (SUCCEEDED(hr))
        {
            WCHAR driverW[sizeof(adapter_info.Driver)];
            WCHAR descriptionW[sizeof(adapter_info.Description)];
            WCHAR devicenameW[sizeof(adapter_info.DeviceName)];

            MultiByteToWideChar(CP_ACP, 0, adapter_info.Driver, -1, driverW, sizeof(driverW)/sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, adapter_info.Description, -1, descriptionW, sizeof(descriptionW)/sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, adapter_info.DeviceName, -1, devicenameW, sizeof(devicenameW)/sizeof(WCHAR));

            hr = add_bstr_property(display_adapter, szDriverName, driverW);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, szDescription, descriptionW);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, szDeviceName, devicenameW);
            if (FAILED(hr))
                goto cleanup;

            snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), driverversion_fmtW,
                      HIWORD(adapter_info.DriverVersion.u.HighPart), LOWORD(adapter_info.DriverVersion.u.HighPart),
                      HIWORD(adapter_info.DriverVersion.u.LowPart), LOWORD(adapter_info.DriverVersion.u.LowPart));

            hr = add_bstr_property(display_adapter, szDriverVersion, buffer);
            if (FAILED(hr))
                goto cleanup;

            snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), id_fmtW, adapter_info.VendorId);
            hr = add_bstr_property(display_adapter, szVendorId, buffer);
            if (FAILED(hr))
                goto cleanup;

            snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), id_fmtW, adapter_info.DeviceId);
            hr = add_bstr_property(display_adapter, szDeviceId, buffer);
            if (FAILED(hr))
                goto cleanup;

            snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), subsysid_fmtW, adapter_info.SubSysId);
            hr = add_bstr_property(display_adapter, szSubSysId, buffer);
            if (FAILED(hr))
                goto cleanup;

            snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), id_fmtW, adapter_info.Revision);
            hr = add_bstr_property(display_adapter, szRevisionId, buffer);
            if (FAILED(hr))
                goto cleanup;

            StringFromGUID2(&adapter_info.DeviceIdentifier, buffer, 39);
            hr = add_bstr_property(display_adapter, szDeviceIdentifier, buffer);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, szManufacturer, vendor_id_to_manufacturer_string(adapter_info.VendorId));
            if (FAILED(hr))
                goto cleanup;
        }

        hr = IDirect3D9_GetAdapterDisplayMode(pDirect3D9, index, &adapter_mode);
        if (SUCCEEDED(hr))
        {
            hr = add_ui4_property(display_adapter, dwWidth, adapter_mode.Width);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, dwHeight, adapter_mode.Height);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, dwRefreshRate, adapter_mode.RefreshRate);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, dwBpp, depth_for_pixelformat(adapter_mode.Format));
            if (FAILED(hr))
                goto cleanup;
        }

        hr = add_bstr_property(display_adapter, szKeyDeviceKey, szEmpty);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szKeyDeviceID, szEmpty);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szChipType, szEmpty);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szDACType, szEmpty);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szRevision, szEmpty);
        if (FAILED(hr))
            goto cleanup;

        if (!get_texture_memory(&adapter_info.DeviceIdentifier, &available_mem))
            WARN("get_texture_memory helper failed\n");

        snprintfW(buffer, sizeof(buffer)/sizeof(buffer[0]), mem_fmt, available_mem / 1000000.0f);

        hr = add_bstr_property(display_adapter, szDisplayMemoryLocalized, buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szDisplayMemoryEnglish, buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = IDirect3D9_GetDeviceCaps(pDirect3D9, index, D3DDEVTYPE_HAL, &device_caps);
        hardware_accel = SUCCEEDED(hr);

        hr = add_bool_property(display_adapter, b3DAccelerationEnabled, hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, b3DAccelerationExists, hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, bDDAccelerationEnabled, hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, bNoHardware, FALSE);
        if (FAILED(hr))
            goto cleanup;
    }

    hr = S_OK;
cleanup:
    IDirect3D9_Release(pDirect3D9);
    return hr;
}

static HRESULT fill_display_information_fallback(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR szAdapterID[] = {'0',0};
    static const WCHAR *empty_properties[] = {szDeviceIdentifier, szVendorId, szDeviceId,
                                              szKeyDeviceKey, szKeyDeviceID, szDriverName,
                                              szDriverVersion, szSubSysId, szRevisionId,
                                              szManufacturer, szChipType, szDACType, szRevision};

    IDxDiagContainerImpl_Container *display_adapter;
    HRESULT hr;
    IDirectDraw7 *pDirectDraw;
    DDSCAPS2 dd_caps;
    DISPLAY_DEVICEW disp_dev;
    DDSURFACEDESC2 surface_descr;
    DWORD tmp;
    WCHAR buffer[256];

    display_adapter = allocate_information_node(szAdapterID);
    if (!display_adapter)
        return E_OUTOFMEMORY;

    add_subcontainer(node, display_adapter);

    disp_dev.cb = sizeof(disp_dev);
    if (EnumDisplayDevicesW( NULL, 0, &disp_dev, 0 ))
    {
        hr = add_bstr_property(display_adapter, szDeviceName, disp_dev.DeviceName);
        if (FAILED(hr))
            return hr;

        hr = add_bstr_property(display_adapter, szDescription, disp_dev.DeviceString);
        if (FAILED(hr))
            return hr;
    }

    /* Silently ignore a failure from DirectDrawCreateEx. */
    hr = DirectDrawCreateEx(NULL, (void **)&pDirectDraw, &IID_IDirectDraw7, NULL);
    if (FAILED(hr))
        return S_OK;

    dd_caps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    dd_caps.dwCaps2 = dd_caps.dwCaps3 = dd_caps.u1.dwCaps4 = 0;
    hr = IDirectDraw7_GetAvailableVidMem(pDirectDraw, &dd_caps, &tmp, NULL);
    if (SUCCEEDED(hr))
    {
        static const WCHAR mem_fmt[] = {'%','.','1','f',' ','M','B',0};

        snprintfW(buffer, sizeof(buffer)/sizeof(buffer[0]), mem_fmt, tmp / 1000000.0f);

        hr = add_bstr_property(display_adapter, szDisplayMemoryLocalized, buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, szDisplayMemoryEnglish, buffer);
        if (FAILED(hr))
            goto cleanup;
    }

    surface_descr.dwSize = sizeof(surface_descr);
    hr = IDirectDraw7_GetDisplayMode(pDirectDraw, &surface_descr);
    if (SUCCEEDED(hr))
    {
        if (surface_descr.dwFlags & DDSD_WIDTH)
        {
            hr = add_ui4_property(display_adapter, dwWidth, surface_descr.dwWidth);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_HEIGHT)
        {
            hr = add_ui4_property(display_adapter, dwHeight, surface_descr.dwHeight);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_PIXELFORMAT)
        {
            hr = add_ui4_property(display_adapter, dwBpp, surface_descr.u4.ddpfPixelFormat.u1.dwRGBBitCount);
            if (FAILED(hr))
                goto cleanup;
        }
    }

    hr = add_ui4_property(display_adapter, dwRefreshRate, 60);
    if (FAILED(hr))
        goto cleanup;

    for (tmp = 0; tmp < sizeof(empty_properties)/sizeof(empty_properties[0]); tmp++)
    {
        hr = add_bstr_property(display_adapter, empty_properties[tmp], szEmpty);
        if (FAILED(hr))
            goto cleanup;
    }

    hr = S_OK;
cleanup:
    IDirectDraw7_Release(pDirectDraw);
    return hr;
}

static HRESULT build_displaydevices_tree(IDxDiagContainerImpl_Container *node)
{
    HRESULT hr;

    /* Try to use Direct3D to obtain the required information first. */
    hr = fill_display_information_d3d(node);
    if (hr != E_FAIL)
        return hr;

    return fill_display_information_fallback(node);
}

struct enum_context
{
    IDxDiagContainerImpl_Container *cont;
    HRESULT hr;
    int index;
};

static const WCHAR szGUIDFmt[] =
{
    '%','0','8','x','-','%','0','4','x','-','%','0','4','x','-','%','0',
    '2','x','%','0','2','x','-','%','0','2','x','%','0','2','x','%','0','2',
    'x','%','0','2','x','%','0','2','x','%','0','2','x',0
};

static LPWSTR guid_to_string(LPWSTR lpwstr, REFGUID lpcguid)
{
    wsprintfW(lpwstr, szGUIDFmt, lpcguid->Data1, lpcguid->Data2,
        lpcguid->Data3, lpcguid->Data4[0], lpcguid->Data4[1],
        lpcguid->Data4[2], lpcguid->Data4[3], lpcguid->Data4[4],
        lpcguid->Data4[5], lpcguid->Data4[6], lpcguid->Data4[7]);

    return lpwstr;
}

BOOL CALLBACK dsound_enum(LPGUID guid, LPCWSTR desc, LPCWSTR module, LPVOID context)
{
    static const WCHAR deviceid_fmtW[] = {'%','u',0};
    static const WCHAR szGuidDeviceID[] = {'s','z','G','u','i','d','D','e','v','i','c','e','I','D',0};
    static const WCHAR szDriverPath[] = {'s','z','D','r','i','v','e','r','P','a','t','h',0};

    struct enum_context *enum_ctx = context;
    IDxDiagContainerImpl_Container *device;
    WCHAR buffer[256];
    const WCHAR *p, *name;

    /* the default device is enumerated twice, one time without GUID */
    if (!guid) return TRUE;

    snprintfW(buffer, sizeof(buffer)/sizeof(WCHAR), deviceid_fmtW, enum_ctx->index);
    device = allocate_information_node(buffer);
    if (!device)
    {
        enum_ctx->hr = E_OUTOFMEMORY;
        return FALSE;
    }

    add_subcontainer(enum_ctx->cont, device);

    guid_to_string(buffer, guid);
    enum_ctx->hr = add_bstr_property(device, szGuidDeviceID, buffer);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    enum_ctx->hr = add_bstr_property(device, szDescription, desc);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    enum_ctx->hr = add_bstr_property(device, szDriverPath, module);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    name = module;
    if ((p = strrchrW(name, '\\'))) name = p + 1;
    if ((p = strrchrW(name, '/'))) name = p + 1;

    enum_ctx->hr = add_bstr_property(device, szDriverName, name);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    enum_ctx->index++;
    return TRUE;
}

static HRESULT build_directsound_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR DxDiag_SoundDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_SoundCaptureDevices[] = {'D','x','D','i','a','g','_','S','o','u','n','d','C','a','p','t','u','r','e','D','e','v','i','c','e','s',0};

    struct enum_context enum_ctx;
    IDxDiagContainerImpl_Container *cont;

    cont = allocate_information_node(DxDiag_SoundDevices);
    if (!cont)
        return E_OUTOFMEMORY;

    add_subcontainer(node, cont);

    enum_ctx.cont = cont;
    enum_ctx.hr = S_OK;
    enum_ctx.index = 0;

    DirectSoundEnumerateW(dsound_enum, &enum_ctx);
    if (FAILED(enum_ctx.hr))
        return enum_ctx.hr;

    cont = allocate_information_node(DxDiag_SoundCaptureDevices);
    if (!cont)
        return E_OUTOFMEMORY;

    add_subcontainer(node, cont);

    enum_ctx.cont = cont;
    enum_ctx.hr = S_OK;
    enum_ctx.index = 0;

    DirectSoundCaptureEnumerateW(dsound_enum, &enum_ctx);
    if (FAILED(enum_ctx.hr))
        return enum_ctx.hr;

    return S_OK;
}

static HRESULT build_directmusic_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_directinput_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_directplay_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_systemdevices_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT fill_file_description(IDxDiagContainerImpl_Container *node, const WCHAR *szFilePath, const WCHAR *szFileName)
{
    static const WCHAR szSlashSep[] = {'\\',0};
    static const WCHAR szPath[] = {'s','z','P','a','t','h',0};
    static const WCHAR szName[] = {'s','z','N','a','m','e',0};
    static const WCHAR szVersion[] = {'s','z','V','e','r','s','i','o','n',0};
    static const WCHAR szAttributes[] = {'s','z','A','t','t','r','i','b','u','t','e','s',0};
    static const WCHAR szLanguageEnglish[] = {'s','z','L','a','n','g','u','a','g','e','E','n','g','l','i','s','h',0};
    static const WCHAR dwFileTimeHigh[] = {'d','w','F','i','l','e','T','i','m','e','H','i','g','h',0};
    static const WCHAR dwFileTimeLow[] = {'d','w','F','i','l','e','T','i','m','e','L','o','w',0};
    static const WCHAR bBeta[] = {'b','B','e','t','a',0};
    static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
    static const WCHAR bExists[] = {'b','E','x','i','s','t','s',0};

    /* Values */
    static const WCHAR szFinal_Retail_v[] = {'F','i','n','a','l',' ','R','e','t','a','i','l',0};
    static const WCHAR szEnglish_v[] = {'E','n','g','l','i','s','h',0};
    static const WCHAR szVersionFormat[] = {'%','u','.','%','0','2','u','.','%','0','4','u','.','%','0','4','u',0};

    HRESULT hr;
    WCHAR *szFile;
    WCHAR szVersion_v[1024];
    DWORD retval, hdl;
    void *pVersionInfo = NULL;
    BOOL boolret = FALSE;
    UINT uiLength;
    VS_FIXEDFILEINFO *pFileInfo;

    TRACE("Filling container %p for %s in %s\n", node,
          debugstr_w(szFileName), debugstr_w(szFilePath));

    szFile = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (lstrlenW(szFilePath) +
                                            lstrlenW(szFileName) + 2 /* slash + terminator */));
    if (!szFile)
        return E_OUTOFMEMORY;

    lstrcpyW(szFile, szFilePath);
    lstrcatW(szFile, szSlashSep);
    lstrcatW(szFile, szFileName);

    retval = GetFileVersionInfoSizeW(szFile, &hdl);
    if (retval)
    {
        pVersionInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retval);
        if (!pVersionInfo)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        if (GetFileVersionInfoW(szFile, 0, retval, pVersionInfo) &&
            VerQueryValueW(pVersionInfo, szSlashSep, (void **)&pFileInfo, &uiLength))
            boolret = TRUE;
    }

    hr = add_bstr_property(node, szPath, szFile);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(node, szName, szFileName);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bool_property(node, bExists, boolret);
    if (FAILED(hr))
        goto cleanup;

    if (boolret)
    {
        snprintfW(szVersion_v, sizeof(szVersion_v)/sizeof(szVersion_v[0]),
                  szVersionFormat,
                  HIWORD(pFileInfo->dwFileVersionMS),
                  LOWORD(pFileInfo->dwFileVersionMS),
                  HIWORD(pFileInfo->dwFileVersionLS),
                  LOWORD(pFileInfo->dwFileVersionLS));

        TRACE("Found version as (%s)\n", debugstr_w(szVersion_v));

        hr = add_bstr_property(node, szVersion, szVersion_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, szAttributes, szFinal_Retail_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, szLanguageEnglish, szEnglish_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, dwFileTimeHigh, pFileInfo->dwFileDateMS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, dwFileTimeLow, pFileInfo->dwFileDateLS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, bBeta, ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_PRERELEASE) != 0);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, bDebug, ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_DEBUG) != 0);
        if (FAILED(hr))
            goto cleanup;
    }

    hr = S_OK;
cleanup:
    HeapFree(GetProcessHeap(), 0, pVersionInfo);
    HeapFree(GetProcessHeap(), 0, szFile);

    return hr;
}
static HRESULT build_directxfiles_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR dlls[][15] =
    {
        {'d','3','d','8','.','d','l','l',0},
        {'d','3','d','9','.','d','l','l',0},
        {'d','d','r','a','w','.','d','l','l',0},
        {'d','e','v','e','n','u','m','.','d','l','l',0},
        {'d','i','n','p','u','t','8','.','d','l','l',0},
        {'d','i','n','p','u','t','.','d','l','l',0},
        {'d','m','b','a','n','d','.','d','l','l',0},
        {'d','m','c','o','m','p','o','s','.','d','l','l',0},
        {'d','m','i','m','e','.','d','l','l',0},
        {'d','m','l','o','a','d','e','r','.','d','l','l',0},
        {'d','m','s','c','r','i','p','t','.','d','l','l',0},
        {'d','m','s','t','y','l','e','.','d','l','l',0},
        {'d','m','s','y','n','t','h','.','d','l','l',0},
        {'d','m','u','s','i','c','.','d','l','l',0},
        {'d','p','l','a','y','x','.','d','l','l',0},
        {'d','p','n','e','t','.','d','l','l',0},
        {'d','s','o','u','n','d','.','d','l','l',0},
        {'d','s','w','a','v','e','.','d','l','l',0},
        {'d','x','d','i','a','g','n','.','d','l','l',0},
        {'q','u','a','r','t','z','.','d','l','l',0}
    };

    HRESULT hr;
    WCHAR szFilePath[MAX_PATH];
    INT i;

    GetSystemDirectoryW(szFilePath, MAX_PATH);

    for (i = 0; i < sizeof(dlls) / sizeof(dlls[0]); i++)
    {
        static const WCHAR szFormat[] = {'%','d',0};

        WCHAR szFileID[5];
        IDxDiagContainerImpl_Container *file_container;

        snprintfW(szFileID, sizeof(szFileID)/sizeof(szFileID[0]), szFormat, i);

        file_container = allocate_information_node(szFileID);
        if (!file_container)
            return E_OUTOFMEMORY;

        hr = fill_file_description(file_container, szFilePath, dlls[i]);
        if (FAILED(hr))
        {
            free_information_tree(file_container);
            continue;
        }

        add_subcontainer(node, file_container);
    }

    return S_OK;
}

static HRESULT read_property_names(IPropertyBag *pPropBag, VARIANT *friendly_name, VARIANT *clsid_name)
{
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};

    HRESULT hr;

    VariantInit(friendly_name);
    VariantInit(clsid_name);

    hr = IPropertyBag_Read(pPropBag, wszFriendlyName, friendly_name, 0);
    if (FAILED(hr))
        return hr;

    hr = IPropertyBag_Read(pPropBag, wszClsidName, clsid_name, 0);
    if (FAILED(hr))
    {
        VariantClear(friendly_name);
        return hr;
    }

    return S_OK;
}

static HRESULT fill_filter_data_information(IDxDiagContainerImpl_Container *subcont, BYTE *pData, ULONG cb)
{
    static const WCHAR szVersionW[] = {'s','z','V','e','r','s','i','o','n',0};
    static const WCHAR dwInputs[] = {'d','w','I','n','p','u','t','s',0};
    static const WCHAR dwOutputs[] = {'d','w','O','u','t','p','u','t','s',0};
    static const WCHAR dwMeritW[] = {'d','w','M','e','r','i','t',0};
    static const WCHAR szVersionFormat[] = {'v','%','d',0};

    HRESULT hr;
    IFilterMapper2 *pFileMapper = NULL;
    IAMFilterData *pFilterData = NULL;
    BYTE *ppRF = NULL;
    REGFILTER2 *pRF = NULL;
    WCHAR bufferW[10];
    ULONG j;
    DWORD dwNOutputs = 0;
    DWORD dwNInputs = 0;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper2,
                          (void **)&pFileMapper);
    if (FAILED(hr))
        return hr;

    hr = IFilterMapper2_QueryInterface(pFileMapper, &IID_IAMFilterData, (void **)&pFilterData);
    if (FAILED(hr))
        goto cleanup;

    hr = IAMFilterData_ParseFilterData(pFilterData, pData, cb, (BYTE **)&ppRF);
    if (FAILED(hr))
        goto cleanup;
    pRF = ((REGFILTER2**)ppRF)[0];

    snprintfW(bufferW, sizeof(bufferW)/sizeof(bufferW[0]), szVersionFormat, pRF->dwVersion);
    hr = add_bstr_property(subcont, szVersionW, bufferW);
    if (FAILED(hr))
        goto cleanup;

    if (pRF->dwVersion == 1)
    {
        for (j = 0; j < pRF->u.s1.cPins; j++)
            if (pRF->u.s1.rgPins[j].bOutput)
                dwNOutputs++;
            else
                dwNInputs++;
    }
    else if (pRF->dwVersion == 2)
    {
        for (j = 0; j < pRF->u.s2.cPins2; j++)
            if (pRF->u.s2.rgPins2[j].dwFlags & REG_PINFLAG_B_OUTPUT)
                dwNOutputs++;
            else
                dwNInputs++;
    }

    hr = add_ui4_property(subcont, dwInputs, dwNInputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, dwOutputs, dwNOutputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, dwMeritW, pRF->dwMerit);
    if (FAILED(hr))
        goto cleanup;

    hr = S_OK;
cleanup:
    CoTaskMemFree(pRF);
    if (pFilterData) IAMFilterData_Release(pFilterData);
    if (pFileMapper) IFilterMapper2_Release(pFileMapper);

    return hr;
}

static HRESULT fill_filter_container(IDxDiagContainerImpl_Container *subcont, IMoniker *pMoniker)
{
    static const WCHAR szName[] = {'s','z','N','a','m','e',0};
    static const WCHAR ClsidFilterW[] = {'C','l','s','i','d','F','i','l','t','e','r',0};
    static const WCHAR wszFilterDataName[] = {'F','i','l','t','e','r','D','a','t','a',0};

    HRESULT hr;
    IPropertyBag *pPropFilterBag = NULL;
    BYTE *pData;
    VARIANT friendly_name;
    VARIANT clsid_name;
    VARIANT v;

    VariantInit(&friendly_name);
    VariantInit(&clsid_name);
    VariantInit(&v);

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (void **)&pPropFilterBag);
    if (FAILED(hr))
        return hr;

    hr = read_property_names(pPropFilterBag, &friendly_name, &clsid_name);
    if (FAILED(hr))
        goto cleanup;

    TRACE("Name = %s\n", debugstr_w(V_BSTR(&friendly_name)));
    TRACE("CLSID = %s\n", debugstr_w(V_BSTR(&clsid_name)));

    hr = add_bstr_property(subcont, szName, V_BSTR(&friendly_name));
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(subcont, ClsidFilterW, V_BSTR(&clsid_name));
    if (FAILED(hr))
        goto cleanup;

    hr = IPropertyBag_Read(pPropFilterBag, wszFilterDataName, &v, NULL);
    if (FAILED(hr))
        goto cleanup;

    hr = SafeArrayAccessData(V_ARRAY(&v), (void **)&pData);
    if (FAILED(hr))
        goto cleanup;

    hr = fill_filter_data_information(subcont, pData, V_ARRAY(&v)->rgsabound->cElements);
    SafeArrayUnaccessData(V_ARRAY(&v));
    if (FAILED(hr))
        goto cleanup;

    hr = S_OK;
cleanup:
    VariantClear(&v);
    VariantClear(&clsid_name);
    VariantClear(&friendly_name);
    if (pPropFilterBag) IPropertyBag_Release(pPropFilterBag);

    return hr;
}

static HRESULT build_directshowfilters_tree(IDxDiagContainerImpl_Container *node)
{
    static const WCHAR szCatName[] = {'s','z','C','a','t','N','a','m','e',0};
    static const WCHAR ClsidCatW[] = {'C','l','s','i','d','C','a','t',0};
    static const WCHAR szIdFormat[] = {'%','d',0};

    HRESULT hr;
    int i = 0;
    ICreateDevEnum *pCreateDevEnum;
    IEnumMoniker *pEmCat = NULL;
    IMoniker *pMCat = NULL;
	IEnumMoniker *pEnum = NULL;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ICreateDevEnum, (void **)&pCreateDevEnum);
    if (FAILED(hr))
        return hr;

    hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &CLSID_ActiveMovieCategories, &pEmCat, 0);
    if (FAILED(hr))
        goto cleanup;

    while (IEnumMoniker_Next(pEmCat, 1, &pMCat, NULL) == S_OK)
    {
        VARIANT vCatName;
        VARIANT vCatClsid;
        IPropertyBag *pPropBag;
        CLSID clsidCat;
        IMoniker *pMoniker = NULL;

        hr = IMoniker_BindToStorage(pMCat, NULL, NULL, &IID_IPropertyBag, (void **)&pPropBag);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            break;
        }

        hr = read_property_names(pPropBag, &vCatName, &vCatClsid);
        IPropertyBag_Release(pPropBag);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            break;
        }

        hr = CLSIDFromString(V_BSTR(&vCatClsid), &clsidCat);
        if (FAILED(hr))
        {
            IMoniker_Release(pMCat);
            VariantClear(&vCatClsid);
            VariantClear(&vCatName);
            break;
        }

        hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &clsidCat, &pEnum, 0);
        if (hr != S_OK)
        {
            IMoniker_Release(pMCat);
            VariantClear(&vCatClsid);
            VariantClear(&vCatName);
            continue;
        }

        TRACE("Enumerating class %s\n", debugstr_guid(&clsidCat));

        while (IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL) == S_OK)
        {
            WCHAR bufferW[10];
            IDxDiagContainerImpl_Container *subcont;

            snprintfW(bufferW, sizeof(bufferW)/sizeof(bufferW[0]), szIdFormat, i);
            subcont = allocate_information_node(bufferW);
            if (!subcont)
            {
                hr = E_OUTOFMEMORY;
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, szCatName, V_BSTR(&vCatName));
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, ClsidCatW, V_BSTR(&vCatClsid));
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            hr = fill_filter_container(subcont, pMoniker);
            IMoniker_Release(pMoniker);
            if (FAILED(hr))
            {
                WARN("Skipping invalid filter\n");
                free_information_tree(subcont);
                hr = S_OK;
                continue;
            }

            add_subcontainer(node, subcont);
            i++;
        }

        IEnumMoniker_Release(pEnum);
        IMoniker_Release(pMCat);
        VariantClear(&vCatClsid);
        VariantClear(&vCatName);

        if (FAILED(hr))
            break;
    }

cleanup:
    if (pEmCat) IEnumMoniker_Release(pEmCat);
    ICreateDevEnum_Release(pCreateDevEnum);
    return hr;
}

static HRESULT build_logicaldisks_tree(IDxDiagContainerImpl_Container *node)
{
    return S_OK;
}

static HRESULT build_information_tree(IDxDiagContainerImpl_Container **pinfo_root)
{
    static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};
    static const WCHAR DxDiag_DisplayDevices[] = {'D','x','D','i','a','g','_','D','i','s','p','l','a','y','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_DirectSound[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d',0};
    static const WCHAR DxDiag_DirectMusic[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','M','u','s','i','c',0};
    static const WCHAR DxDiag_DirectInput[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','I','n','p','u','t',0};
    static const WCHAR DxDiag_DirectPlay[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','P','l','a','y',0};
    static const WCHAR DxDiag_SystemDevices[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_DirectXFiles[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','X','F','i','l','e','s',0};
    static const WCHAR DxDiag_DirectShowFilters[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','h','o','w','F','i','l','t','e','r','s',0};
    static const WCHAR DxDiag_LogicalDisks[] = {'D','x','D','i','a','g','_','L','o','g','i','c','a','l','D','i','s','k','s',0};

    static const struct
    {
        const WCHAR *name;
        HRESULT (*initfunc)(IDxDiagContainerImpl_Container *);
    } root_children[] =
    {
        {DxDiag_SystemInfo, build_systeminfo_tree},
        {DxDiag_DisplayDevices, build_displaydevices_tree},
        {DxDiag_DirectSound, build_directsound_tree},
        {DxDiag_DirectMusic, build_directmusic_tree},
        {DxDiag_DirectInput, build_directinput_tree},
        {DxDiag_DirectPlay, build_directplay_tree},
        {DxDiag_SystemDevices, build_systemdevices_tree},
        {DxDiag_DirectXFiles, build_directxfiles_tree},
        {DxDiag_DirectShowFilters, build_directshowfilters_tree},
        {DxDiag_LogicalDisks, build_logicaldisks_tree},
    };

    IDxDiagContainerImpl_Container *info_root;
    size_t index;

    info_root = allocate_information_node(NULL);
    if (!info_root)
        return E_OUTOFMEMORY;

    for (index = 0; index < sizeof(root_children)/sizeof(root_children[0]); index++)
    {
        IDxDiagContainerImpl_Container *node;
        HRESULT hr;

        node = allocate_information_node(root_children[index].name);
        if (!node)
        {
            free_information_tree(info_root);
            return E_OUTOFMEMORY;
        }

        hr = root_children[index].initfunc(node);
        if (FAILED(hr))
        {
            free_information_tree(node);
            free_information_tree(info_root);
            return hr;
        }

        add_subcontainer(info_root, node);
    }

    *pinfo_root = info_root;
    return S_OK;
}
