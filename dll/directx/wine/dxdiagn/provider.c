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


#define COBJMACROS
#include "dxdiag_private.h"
#include "winver.h"
#include "objidl.h"
#include "uuids.h"
#include "vfw.h"
#include "mmddk.h"
#include "d3d9.h"
#include "strmif.h"
#include "initguid.h"
#include "mmdeviceapi.h"
#include "wine/fil_data.h"
#include "psapi.h"
#include "wbemcli.h"
#include "dsound.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static HRESULT build_information_tree(IDxDiagContainerImpl_Container **pinfo_root);
static void free_information_tree(IDxDiagContainerImpl_Container *node);

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
        *ppobj = &This->IDxDiagProvider_iface;
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

    TRACE("(%p)->(ref before=%lu)\n", This, refCount - 1);

    DXDIAGN_LockModule();

    return refCount;
}

static ULONG WINAPI IDxDiagProviderImpl_Release(IDxDiagProvider *iface)
{
    IDxDiagProviderImpl *This = impl_from_IDxDiagProvider(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n", This, refCount + 1);

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
        ret->contName = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name) + 1) * sizeof(*name));
        if (!ret->contName)
        {
            HeapFree(GetProcessHeap(), 0, ret);
            return NULL;
        }
        lstrcpyW(ret->contName, name);
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

    ret->propName = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name) + 1) * sizeof(*name));
    if (!ret->propName)
    {
        HeapFree(GetProcessHeap(), 0, ret);
        return NULL;
    }
    lstrcpyW(ret->propName, name);

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

static inline HRESULT add_i4_property(IDxDiagContainerImpl_Container *node, const WCHAR *propName, LONG data)
{
    IDxDiagContainerImpl_Property *prop;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_I4;
    V_I4(&prop->vProp) = data;

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
    HRESULT hr;

    prop = allocate_property_information(propName);
    if (!prop)
        return E_OUTOFMEMORY;

    V_VT(&prop->vProp) = VT_UI8;
    V_UI8(&prop->vProp) = data;

    hr = VariantChangeType(&prop->vProp, &prop->vProp, 0, VT_BSTR);
    if (FAILED(hr))
    {
        free_property_information(prop);
        return hr;
    }

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
    DWORD list_count;
    DWORD *pid_list = enumerate_processes(&list_count);

    if (pid_list)
    {
        DWORD i;
        WCHAR process_name[MAX_PATH];

        for (i = 0; i < list_count; i++)
        {
            if (get_process_name_from_pid(pid_list[i], process_name, ARRAY_SIZE(process_name)) &&
                    !lstrcmpW(L"conf.exe", process_name))
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
    WCHAR system_lang[80], regional_setting[100], user_lang[80], language_str[300];
    HRESULT hr;

    /* szLanguagesLocalized */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SNATIVELANGNAME, system_lang, ARRAY_SIZE(system_lang));
    LoadStringW(dxdiagn_instance, IDS_REGIONAL_SETTING, regional_setting, ARRAY_SIZE(regional_setting));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNATIVELANGNAME, user_lang, ARRAY_SIZE(user_lang));

    swprintf(language_str, ARRAY_SIZE(language_str), L"%s (%s: %s)", system_lang, regional_setting,
            user_lang);

    hr = add_bstr_property(node, L"szLanguagesLocalized", language_str);
    if (FAILED(hr))
        return hr;

    /* szLanguagesEnglish */
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE, system_lang, ARRAY_SIZE(system_lang));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, user_lang, ARRAY_SIZE(user_lang));

    swprintf(language_str, ARRAY_SIZE(language_str), L"%s (%s: %s)", system_lang,
            L"Regional Setting", user_lang);

    hr = add_bstr_property(node, L"szLanguagesEnglish", language_str);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT fill_datetime_information(IDxDiagContainerImpl_Container *node)
{
    SYSTEMTIME curtime;
    WCHAR date_str[80], time_str[80], datetime_str[200];
    HRESULT hr;

    GetLocalTime(&curtime);

    GetTimeFormatW(LOCALE_NEUTRAL, 0, &curtime, L"HH':'mm':'ss", time_str, ARRAY_SIZE(time_str));

    /* szTimeLocalized */
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &curtime, NULL, date_str, ARRAY_SIZE(date_str));

    swprintf(datetime_str, ARRAY_SIZE(datetime_str), L"%s, %s", date_str, time_str);

    hr = add_bstr_property(node, L"szTimeLocalized", datetime_str);
    if (FAILED(hr))
        return hr;

    /* szTimeEnglish */
    GetDateFormatW(LOCALE_NEUTRAL, 0, &curtime, L"M'/'d'/'yyyy", date_str, ARRAY_SIZE(date_str));

    swprintf(datetime_str, ARRAY_SIZE(datetime_str), L"%s, %s", date_str, time_str);

    hr = add_bstr_property(node, L"szTimeEnglish", datetime_str);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT fill_os_string_information(IDxDiagContainerImpl_Container *node, OSVERSIONINFOW *info)
{
    static const WCHAR *prop_list[] =
    {
        L"szOSLocalized", L"szOSExLocalized", L"szOSExLongLocalized",
        L"szOSEnglish", L"szOSExEnglish", L"szOSExLongEnglish"
    };
    size_t i;
    HRESULT hr;

    /* FIXME: OS detection should be performed, and localized OS strings
     * should contain translated versions of the "build" phrase. */
    for (i = 0; i < ARRAY_SIZE(prop_list); i++)
    {
        hr = add_bstr_property(node, prop_list[i], L"Windows XP Professional");
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static HRESULT fill_processor_information(IDxDiagContainerImpl_Container *node)
{
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

    bstr = SysAllocString(L"\\\\.\\root\\cimv2");
    if(!bstr) {
        IWbemLocator_Release(wbem_locator);
        return E_OUTOFMEMORY;
    }
    hr = IWbemLocator_ConnectServer(wbem_locator, bstr, NULL, NULL, NULL, 0, NULL, NULL, &wbem_service);
    IWbemLocator_Release(wbem_locator);
    SysFreeString(bstr);
    if(FAILED(hr))
        return hr;

    bstr = SysAllocString(L"Win32_Processor");
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

    hr = IWbemClassObject_Get(wbem_class, L"NumberOfLogicalProcessors", 0, &cpu_no, NULL, NULL);
    if(FAILED(hr)) {
        IWbemClassObject_Release(wbem_class);
        return hr;
    }
    hr = IWbemClassObject_Get(wbem_class, L"MaxClockSpeed", 0, &clock_speed, NULL, NULL);
    if(FAILED(hr)) {
        IWbemClassObject_Release(wbem_class);
        return hr;
    }
    hr = IWbemClassObject_Get(wbem_class, L"Name", 0, &cpu_name, NULL, NULL);
    IWbemClassObject_Release(wbem_class);
    if(FAILED(hr))
        return hr;

    swprintf(print_buf, ARRAY_SIZE(print_buf), L"%s(%d CPUs), ~%dMHz",
             V_BSTR(&cpu_name), V_I4(&cpu_no), V_I4(&clock_speed));
    VariantClear(&cpu_name);
    VariantClear(&cpu_no);
    VariantClear(&clock_speed);

    return add_bstr_property(node, L"szProcessorEnglish", print_buf);
}

static HRESULT build_systeminfo_tree(IDxDiagContainerImpl_Container *node)
{
    HRESULT hr;
    MEMORYSTATUSEX msex;
    OSVERSIONINFOW info;
    DWORD count, usedpage_mb, availpage_mb;
    WCHAR buffer[MAX_PATH], computer_name[MAX_COMPUTERNAME_LENGTH + 1], print_buf[200], localized_pagefile_fmt[200];
    DWORD_PTR args[2];

    hr = add_ui4_property(node, L"dwDirectXVersionMajor", 9);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, L"dwDirectXVersionMinor", 0);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szDirectXVersionLetter", L"c");
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szDirectXVersionEnglish", L"4.09.0000.0904");
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szDirectXVersionLongEnglish", L"= \"DirectX 9.0c (4.09.0000.0904)");
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, L"bDebug", FALSE);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, L"bIsD3DDebugRuntime", FALSE);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, L"bNECPC98", FALSE);
    if (FAILED(hr))
        return hr;

    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx(&msex);

    hr = add_ull_as_bstr_property(node, L"ullPhysicalMemory", msex.ullTotalPhys);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, L"ullUsedPageFile", msex.ullTotalPageFile - msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    hr = add_ull_as_bstr_property(node, L"ullAvailPageFile", msex.ullAvailPageFile);
    if (FAILED(hr))
        return hr;

    hr = add_bool_property(node, L"bNetMeetingRunning", is_netmeeting_running());
    if (FAILED(hr))
        return hr;

    info.dwOSVersionInfoSize = sizeof(info);
    GetVersionExW(&info);

    hr = add_ui4_property(node, L"dwOSMajorVersion", info.dwMajorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, L"dwOSMinorVersion", info.dwMinorVersion);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, L"dwOSBuildNumber", info.dwBuildNumber);
    if (FAILED(hr))
        return hr;

    hr = add_ui4_property(node, L"dwOSPlatformID", info.dwPlatformId);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szCSDVersion", info.szCSDVersion);
    if (FAILED(hr))
        return hr;

    /* FIXME: Roundoff should not be done with truncated division. */
    swprintf(print_buf, ARRAY_SIZE(print_buf), L"%I64uMB RAM", msex.ullTotalPhys / (1024 * 1024));
    hr = add_bstr_property(node, L"szPhysicalMemoryEnglish", print_buf);
    if (FAILED(hr))
        return hr;

    usedpage_mb = (DWORD)((msex.ullTotalPageFile - msex.ullAvailPageFile) / (1024 * 1024));
    availpage_mb = (DWORD)(msex.ullAvailPageFile / (1024 * 1024));
    LoadStringW(dxdiagn_instance, IDS_PAGE_FILE_FORMAT, localized_pagefile_fmt,
                ARRAY_SIZE(localized_pagefile_fmt));
    args[0] = usedpage_mb;
    args[1] = availpage_mb;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, localized_pagefile_fmt,
                   0, 0, print_buf, ARRAY_SIZE(print_buf), (va_list *)args);

    hr = add_bstr_property(node, L"szPageFileLocalized", print_buf);
    if (FAILED(hr))
        return hr;

    swprintf(print_buf, ARRAY_SIZE(print_buf), L"%uMB used, %uMB available", usedpage_mb, availpage_mb);

    hr = add_bstr_property(node, L"szPageFileEnglish", print_buf);
    if (FAILED(hr))
        return hr;

    GetWindowsDirectoryW(buffer, MAX_PATH);

    hr = add_bstr_property(node, L"szWindowsDir", buffer);
    if (FAILED(hr))
        return hr;

    count = ARRAY_SIZE(computer_name);
    if (!GetComputerNameW(computer_name, &count))
        return E_FAIL;

    hr = add_bstr_property(node, L"szMachineNameLocalized", computer_name);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szMachineNameEnglish", computer_name);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szSystemManufacturerEnglish", L"");
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szSystemModelEnglish", L"");
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szBIOSEnglish", L"");
    if (FAILED(hr))
        return hr;

    hr = fill_processor_information(node);
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szSetupParamEnglish", L"Not present");
    if (FAILED(hr))
        return hr;

    hr = add_bstr_property(node, L"szDxDiagVersion", L"");
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
        dd_caps.dwCaps2 = dd_caps.dwCaps3 = dd_caps.dwCaps4 = 0;
        hr = IDirectDraw7_GetAvailableVidMem(pDirectDraw, &dd_caps, available_mem, NULL);
        IDirectDraw7_Release(pDirectDraw);
        if (SUCCEEDED(hr))
            return TRUE;
    }

    return FALSE;
}

static const WCHAR *vendor_id_to_manufacturer_string(DWORD vendor_id)
{
    unsigned int i;
    static const struct
    {
        DWORD id;
        const WCHAR *name;
    }
    vendors[] =
    {
        {0x1002, L"ATI Technologies Inc."},
        {0x10de, L"NVIDIA"},
        {0x15ad, L"VMware"},
        {0x1af4, L"Red Hat"},
        {0x8086, L"Intel Corporation"},
    };

    for (i = 0; i < ARRAY_SIZE(vendors); ++i)
    {
        if (vendors[i].id == vendor_id)
            return vendors[i].name;
    }

    FIXME("Unknown PCI vendor ID 0x%04lx.\n", vendor_id);

    return L"Unknown";
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
        D3DADAPTER_IDENTIFIER9 adapter_info;
        D3DDISPLAYMODE adapter_mode;
        D3DCAPS9 device_caps;
        DWORD available_mem = 0;
        BOOL hardware_accel;

        swprintf(buffer, ARRAY_SIZE(buffer), L"%u", index);
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

            MultiByteToWideChar(CP_ACP, 0, adapter_info.Driver, -1, driverW, ARRAY_SIZE(driverW));
            MultiByteToWideChar(CP_ACP, 0, adapter_info.Description, -1, descriptionW,
                                ARRAY_SIZE(descriptionW));
            MultiByteToWideChar(CP_ACP, 0, adapter_info.DeviceName, -1, devicenameW,
                                ARRAY_SIZE(devicenameW));

            hr = add_bstr_property(display_adapter, L"szDriverName", driverW);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, L"szDescription", descriptionW);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, L"szDeviceName", devicenameW);
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"%u.%u.%04u.%04u",
                    HIWORD(adapter_info.DriverVersion.HighPart), LOWORD(adapter_info.DriverVersion.HighPart),
                    HIWORD(adapter_info.DriverVersion.LowPart), LOWORD(adapter_info.DriverVersion.LowPart));

            hr = add_bstr_property(display_adapter, L"szDriverVersion", buffer);
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"0x%04x", adapter_info.VendorId);
            hr = add_bstr_property(display_adapter, L"szVendorId", buffer);
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"0x%04x", adapter_info.DeviceId);
            hr = add_bstr_property(display_adapter, L"szDeviceId", buffer);
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"0x%08x", adapter_info.SubSysId);
            hr = add_bstr_property(display_adapter, L"szSubSysId", buffer);
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"0x%04x", adapter_info.Revision);
            hr = add_bstr_property(display_adapter, L"szRevisionId", buffer);
            if (FAILED(hr))
                goto cleanup;

            StringFromGUID2(&adapter_info.DeviceIdentifier, buffer, 39);
            hr = add_bstr_property(display_adapter, L"szDeviceIdentifier", buffer);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, L"szManufacturer",
                    vendor_id_to_manufacturer_string(adapter_info.VendorId));
            if (FAILED(hr))
                goto cleanup;
        }

        hr = IDirect3D9_GetAdapterDisplayMode(pDirect3D9, index, &adapter_mode);
        if (SUCCEEDED(hr))
        {
            hr = add_ui4_property(display_adapter, L"dwWidth", adapter_mode.Width);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, L"dwHeight", adapter_mode.Height);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, L"dwRefreshRate", adapter_mode.RefreshRate);
            if (FAILED(hr))
                goto cleanup;

            hr = add_ui4_property(display_adapter, L"dwBpp", depth_for_pixelformat(adapter_mode.Format));
            if (FAILED(hr))
                goto cleanup;

            swprintf(buffer, ARRAY_SIZE(buffer), L"%d x %d (%d bit) (%dHz)", adapter_mode.Width,
                    adapter_mode.Height, depth_for_pixelformat(adapter_mode.Format),
                    adapter_mode.RefreshRate);

            hr = add_bstr_property(display_adapter, L"szDisplayModeLocalized", buffer);
            if (FAILED(hr))
                goto cleanup;

            hr = add_bstr_property(display_adapter, L"szDisplayModeEnglish", buffer);
            if (FAILED(hr))
                goto cleanup;
        }

        hr = add_bstr_property(display_adapter, L"szKeyDeviceKey", L"");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szKeyDeviceID", L"");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szChipType", L"");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDACType", L"");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szRevision", L"");
        if (FAILED(hr))
            goto cleanup;

        if (!get_texture_memory(&adapter_info.DeviceIdentifier, &available_mem))
            WARN("get_texture_memory helper failed\n");

        swprintf(buffer, ARRAY_SIZE(buffer), L"%.1f MB", available_mem / 1000000.0f);

        hr = add_bstr_property(display_adapter, L"szDisplayMemoryLocalized", buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDisplayMemoryEnglish", buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = IDirect3D9_GetDeviceCaps(pDirect3D9, index, D3DDEVTYPE_HAL, &device_caps);
        hardware_accel = SUCCEEDED(hr);

        hr = add_bool_property(display_adapter, L"b3DAccelerationEnabled", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"b3DAccelerationExists", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bAGPEnabled", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bAGPExistenceValid", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bAGPExists", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bDDAccelerationEnabled", hardware_accel);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bNoHardware", FALSE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bCanRenderWindow", TRUE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szMonitorName", L"Generic PnP Monitor");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szMonitorMaxRes", L"Failed to get parameter");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverAttributes", L"Final Retail");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverLanguageEnglish", L"English");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverLanguageLocalized", L"English");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverDateEnglish", L"1/1/2016 10:00:00");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverDateLocalized", L"1/1/2016 10:00:00 AM");
        if (FAILED(hr))
            goto cleanup;

        hr = add_i4_property(display_adapter, L"lDriverSize", 10 * 1024 * 1024);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szMiniVdd", L"n/a");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szMiniVddDateLocalized", L"n/a");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szMiniVddDateEnglish", L"n/a");
        if (FAILED(hr))
            goto cleanup;

        hr = add_i4_property(display_adapter, L"lMiniVddSize", 0);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szVdd", L"n/a");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bDriverBeta", FALSE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bDriverDebug", FALSE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bDriverSigned", TRUE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(display_adapter, L"bDriverSignedValid", TRUE);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDriverSignDate", L"n/a");
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(display_adapter, L"dwDDIVersion", 11);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDDIVersionEnglish", L"11");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDDIVersionLocalized", L"11");
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(display_adapter, L"iAdapter", index);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(display_adapter, L"dwWHQLLevel", 0);
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
    static const WCHAR *empty_properties[] =
    {
        L"szDeviceIdentifier", L"szVendorId", L"szDeviceId", L"szKeyDeviceKey",
        L"szKeyDeviceID", L"szDriverName", L"szDriverVersion", L"szSubSysId",
        L"szRevisionId", L"szManufacturer", L"szChipType", L"szDACType", L"szRevision"
    };

    IDxDiagContainerImpl_Container *display_adapter;
    HRESULT hr;
    IDirectDraw7 *pDirectDraw;
    DDSCAPS2 dd_caps;
    DISPLAY_DEVICEW disp_dev;
    DDSURFACEDESC2 surface_descr;
    DWORD tmp;
    WCHAR buffer[256];

    display_adapter = allocate_information_node(L"0");
    if (!display_adapter)
        return E_OUTOFMEMORY;

    add_subcontainer(node, display_adapter);

    disp_dev.cb = sizeof(disp_dev);
    if (EnumDisplayDevicesW( NULL, 0, &disp_dev, 0 ))
    {
        hr = add_bstr_property(display_adapter, L"szDeviceName", disp_dev.DeviceName);
        if (FAILED(hr))
            return hr;

        hr = add_bstr_property(display_adapter, L"szDescription", disp_dev.DeviceString);
        if (FAILED(hr))
            return hr;
    }

    /* Silently ignore a failure from DirectDrawCreateEx. */
    hr = DirectDrawCreateEx(NULL, (void **)&pDirectDraw, &IID_IDirectDraw7, NULL);
    if (FAILED(hr))
        return S_OK;

    dd_caps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    dd_caps.dwCaps2 = dd_caps.dwCaps3 = dd_caps.dwCaps4 = 0;
    hr = IDirectDraw7_GetAvailableVidMem(pDirectDraw, &dd_caps, &tmp, NULL);
    if (SUCCEEDED(hr))
    {
        swprintf(buffer, ARRAY_SIZE(buffer), L"%.1f MB", tmp / 1000000.0f);

        hr = add_bstr_property(display_adapter, L"szDisplayMemoryLocalized", buffer);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(display_adapter, L"szDisplayMemoryEnglish", buffer);
        if (FAILED(hr))
            goto cleanup;
    }

    surface_descr.dwSize = sizeof(surface_descr);
    hr = IDirectDraw7_GetDisplayMode(pDirectDraw, &surface_descr);
    if (SUCCEEDED(hr))
    {
        if (surface_descr.dwFlags & DDSD_WIDTH)
        {
            hr = add_ui4_property(display_adapter, L"dwWidth", surface_descr.dwWidth);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_HEIGHT)
        {
            hr = add_ui4_property(display_adapter, L"dwHeight", surface_descr.dwHeight);
            if (FAILED(hr))
                goto cleanup;
        }

        if (surface_descr.dwFlags & DDSD_PIXELFORMAT)
        {
            hr = add_ui4_property(display_adapter, L"dwBpp",
                    surface_descr.ddpfPixelFormat.dwRGBBitCount);
            if (FAILED(hr))
                goto cleanup;
        }
    }

    hr = add_ui4_property(display_adapter, L"dwRefreshRate", 60);
    if (FAILED(hr))
        goto cleanup;

    for (tmp = 0; tmp < ARRAY_SIZE(empty_properties); tmp++)
    {
        hr = add_bstr_property(display_adapter, empty_properties[tmp], L"");
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
    IMMDeviceCollection *devices;
    HRESULT hr;
    int index;
};

static LPWSTR guid_to_string(LPWSTR lpwstr, REFGUID lpcguid)
{
    wsprintfW(lpwstr, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", lpcguid->Data1, lpcguid->Data2,
        lpcguid->Data3, lpcguid->Data4[0], lpcguid->Data4[1], lpcguid->Data4[2], lpcguid->Data4[3], lpcguid->Data4[4],
        lpcguid->Data4[5], lpcguid->Data4[6], lpcguid->Data4[7]);

    return lpwstr;
}

BOOL CALLBACK dsound_enum(LPGUID guid, LPCWSTR desc, LPCWSTR module, LPVOID context)
{
    struct enum_context *enum_ctx = context;
    IDxDiagContainerImpl_Container *device;
    WCHAR buffer[256];
    const WCHAR *p, *name;
    UINT32 i, count;

    /* the default device is enumerated twice, one time without GUID */
    if (!guid) return TRUE;

    swprintf(buffer, ARRAY_SIZE(buffer), L"%u", enum_ctx->index);
    device = allocate_information_node(buffer);
    if (!device)
    {
        enum_ctx->hr = E_OUTOFMEMORY;
        return FALSE;
    }

    add_subcontainer(enum_ctx->cont, device);

    guid_to_string(buffer, guid);
    enum_ctx->hr = add_bstr_property(device, L"szGuidDeviceID", buffer);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    enum_ctx->hr = add_bstr_property(device, L"szDescription", desc);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    enum_ctx->hr = add_bstr_property(device, L"szDriverPath", module);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    name = module;
    if ((p = wcsrchr(name, '\\'))) name = p + 1;
    if ((p = wcsrchr(name, '/'))) name = p + 1;

    enum_ctx->hr = add_bstr_property(device, L"szDriverName", name);
    if (FAILED(enum_ctx->hr))
        return FALSE;

    if (enum_ctx->devices && SUCCEEDED(IMMDeviceCollection_GetCount(enum_ctx->devices, &count)))
    {
        static const PROPERTYKEY devicepath_key =
        {
            {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
        };
        IPropertyStore *ps;
        WCHAR *start, *end;
        IMMDevice *mmdev;
        PROPVARIANT pv;
        HRESULT hr;

        for (i = 0; i < count; ++i)
        {
            mmdev = NULL;
            ps = NULL;
            hr = IMMDeviceCollection_Item(enum_ctx->devices, i, &mmdev);
            if (SUCCEEDED(hr))
                hr = IMMDevice_OpenPropertyStore(mmdev, STGM_READ, &ps);
            PropVariantInit(&pv);
            if (SUCCEEDED(hr))
                hr = IPropertyStore_GetValue(ps, (const PROPERTYKEY*)&PKEY_AudioEndpoint_GUID, &pv);
            if (SUCCEEDED(hr))
            {
                StringFromGUID2(guid, buffer, ARRAY_SIZE(buffer));
                hr = pv.vt == VT_LPWSTR && !wcsicmp(buffer, pv.pwszVal) ? S_OK : E_FAIL;
                PropVariantClear(&pv);
            }
            PropVariantInit(&pv);
            if (SUCCEEDED(hr))
                hr = IPropertyStore_GetValue(ps, &devicepath_key, &pv);
            if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR)
            {
                if ((start = wcsstr(pv.pwszVal, L"}.")))
                    start += 2;
                else
                    start = pv.pwszVal;
                if (wcsnicmp(start, L"ROOT", 4) && (end = wcschr(start, '\\')) && (end = wcschr(end + 1, '\\'))
                        && end - start < ARRAY_SIZE(buffer))
                {
                    memcpy(buffer, start, (end - start) * sizeof(WCHAR));
                    buffer[end - start] = 0;
                    start = buffer;
                }
                add_bstr_property(device, L"szHardwareID", start);
                PropVariantClear(&pv);
            }
            if (ps)
                IPropertyStore_Release(ps);
            if (mmdev)
                IMMDevice_Release(mmdev);
            if (SUCCEEDED(hr))
                break;
        }
    }

    enum_ctx->index++;
    return TRUE;
}

static HRESULT build_directsound_tree(IDxDiagContainerImpl_Container *node)
{
    struct enum_context enum_ctx;
    IDxDiagContainerImpl_Container *cont;
    IMMDeviceEnumerator *mmdevenum;

    cont = allocate_information_node(L"DxDiag_SoundDevices");
    if (!cont)
        return E_OUTOFMEMORY;

    add_subcontainer(node, cont);

    enum_ctx.cont = cont;
    enum_ctx.hr = S_OK;
    enum_ctx.index = 0;

    enum_ctx.devices = NULL;
    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
            (void **)&mmdevenum)))
    {
        IMMDeviceEnumerator_EnumAudioEndpoints(mmdevenum, eAll, DEVICE_STATE_ACTIVE, &enum_ctx.devices);
        IMMDeviceEnumerator_Release(mmdevenum);
    }

    DirectSoundEnumerateW(dsound_enum, &enum_ctx);
    if (enum_ctx.devices)
    {
        IMMDeviceCollection_Release(enum_ctx.devices);
        enum_ctx.devices = NULL;
    }
    if (FAILED(enum_ctx.hr))
        return enum_ctx.hr;

    cont = allocate_information_node(L"DxDiag_SoundCaptureDevices");
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
    lstrcatW(szFile, L"\\");
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
            VerQueryValueW(pVersionInfo, L"\\", (void **)&pFileInfo, &uiLength))
            boolret = TRUE;
    }

    hr = add_bstr_property(node, L"szPath", szFile);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(node, L"szName", szFileName);
    if (FAILED(hr))
        goto cleanup;

    hr = add_bool_property(node, L"bExists", boolret);
    if (FAILED(hr))
        goto cleanup;

    if (boolret)
    {
        swprintf(szVersion_v, ARRAY_SIZE(szVersion_v), L"%u.%02u.%04u.%04u",
                HIWORD(pFileInfo->dwFileVersionMS), LOWORD(pFileInfo->dwFileVersionMS),
                HIWORD(pFileInfo->dwFileVersionLS), LOWORD(pFileInfo->dwFileVersionLS));

        TRACE("Found version as (%s)\n", debugstr_w(szVersion_v));

        hr = add_bstr_property(node, L"szVersion", szVersion_v);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, L"szAttributes", L"Final Retail");
        if (FAILED(hr))
            goto cleanup;

        hr = add_bstr_property(node, L"szLanguageEnglish", L"English");
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, L"dwFileTimeHigh", pFileInfo->dwFileDateMS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_ui4_property(node, L"dwFileTimeLow", pFileInfo->dwFileDateLS);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, L"bBeta",
                ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_PRERELEASE) != 0);
        if (FAILED(hr))
            goto cleanup;

        hr = add_bool_property(node, L"bDebug",
                ((pFileInfo->dwFileFlags & pFileInfo->dwFileFlagsMask) & VS_FF_DEBUG) != 0);
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
        L"d3d8.dll",
        L"d3d9.dll",
        L"ddraw.dll",
        L"devenum.dll",
        L"dinput8.dll",
        L"dinput.dll",
        L"dmband.dll",
        L"dmcompos.dll",
        L"dmime.dll",
        L"dmloader.dll",
        L"dmscript.dll",
        L"dmstyle.dll",
        L"dmsynth.dll",
        L"dmusic.dll",
        L"dplayx.dll",
        L"dpnet.dll",
        L"dsound.dll",
        L"dswave.dll",
        L"dxdiagn.dll",
        L"quartz.dll"
    };

    HRESULT hr;
    WCHAR szFilePath[MAX_PATH];
    INT i;

    GetSystemDirectoryW(szFilePath, MAX_PATH);

    for (i = 0; i < ARRAY_SIZE(dlls); i++)
    {
        WCHAR szFileID[5];
        IDxDiagContainerImpl_Container *file_container;

        swprintf(szFileID, ARRAY_SIZE(szFileID), L"%d", i);

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
    HRESULT hr;

    VariantInit(friendly_name);
    VariantInit(clsid_name);

    hr = IPropertyBag_Read(pPropBag, L"FriendlyName", friendly_name, 0);
    if (FAILED(hr))
        return hr;

    hr = IPropertyBag_Read(pPropBag, L"CLSID", clsid_name, 0);
    if (FAILED(hr))
    {
        VariantClear(friendly_name);
        return hr;
    }

    return S_OK;
}

static HRESULT fill_filter_data_information(IDxDiagContainerImpl_Container *subcont, BYTE *pData, ULONG cb)
{
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

    hr = IAMFilterData_ParseFilterData(pFilterData, pData, cb, &ppRF);
    if (FAILED(hr))
        goto cleanup;
    pRF = ((REGFILTER2**)ppRF)[0];

    swprintf(bufferW, ARRAY_SIZE(bufferW), L"v%d", pRF->dwVersion);
    hr = add_bstr_property(subcont, L"szVersion", bufferW);
    if (FAILED(hr))
        goto cleanup;

    if (pRF->dwVersion == 1)
    {
        for (j = 0; j < pRF->cPins; j++)
            if (pRF->rgPins[j].bOutput)
                dwNOutputs++;
            else
                dwNInputs++;
    }
    else if (pRF->dwVersion == 2)
    {
        for (j = 0; j < pRF->cPins2; j++)
            if (pRF->rgPins2[j].dwFlags & REG_PINFLAG_B_OUTPUT)
                dwNOutputs++;
            else
                dwNInputs++;
    }

    hr = add_ui4_property(subcont, L"dwInputs", dwNInputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, L"dwOutputs", dwNOutputs);
    if (FAILED(hr))
        goto cleanup;

    hr = add_ui4_property(subcont, L"dwMerit", pRF->dwMerit);
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

    hr = add_bstr_property(subcont, L"szName", V_BSTR(&friendly_name));
    if (FAILED(hr))
        goto cleanup;

    hr = add_bstr_property(subcont, L"ClsidFilter", V_BSTR(&clsid_name));
    if (FAILED(hr))
        goto cleanup;

    hr = IPropertyBag_Read(pPropFilterBag, L"FilterData", &v, NULL);
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
    if (hr != S_OK)
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

            swprintf(bufferW, ARRAY_SIZE(bufferW), L"%d", i);
            subcont = allocate_information_node(bufferW);
            if (!subcont)
            {
                hr = E_OUTOFMEMORY;
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, L"szCatName", V_BSTR(&vCatName));
            if (FAILED(hr))
            {
                free_information_tree(subcont);
                IMoniker_Release(pMoniker);
                break;
            }

            hr = add_bstr_property(subcont, L"ClsidCat", V_BSTR(&vCatClsid));
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
    static const struct
    {
        const WCHAR *name;
        HRESULT (*initfunc)(IDxDiagContainerImpl_Container *);
    } root_children[] =
    {
        {L"DxDiag_SystemInfo", build_systeminfo_tree},
        {L"DxDiag_DisplayDevices", build_displaydevices_tree},
        {L"DxDiag_DirectSound", build_directsound_tree},
        {L"DxDiag_DirectMusic", build_directmusic_tree},
        {L"DxDiag_DirectInput", build_directinput_tree},
        {L"DxDiag_DirectPlay", build_directplay_tree},
        {L"DxDiag_SystemDevices", build_systemdevices_tree},
        {L"DxDiag_DirectXFiles", build_directxfiles_tree},
        {L"DxDiag_DirectShowFilters", build_directshowfilters_tree},
        {L"DxDiag_LogicalDisks", build_logicaldisks_tree},
    };

    IDxDiagContainerImpl_Container *info_root;
    size_t index;

    info_root = allocate_information_node(NULL);
    if (!info_root)
        return E_OUTOFMEMORY;

    for (index = 0; index < ARRAY_SIZE(root_children); index++)
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
