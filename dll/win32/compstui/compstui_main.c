/*
 * Implementation of the Common Property Sheets User Interface
 *
 * Copyright 2006 Detlef Riekenberg
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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "ddk/compstui.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(compstui);

static HMODULE compstui_hmod;

typedef struct
{
    WORD size;
    WORD flags;
    union
    {
        WCHAR *titleW;
        char *titleA;
    };
    HWND parent;
    HINSTANCE hinst;
    union
    {
        HICON       hicon;
        ULONG_PTR   icon_id;
    };
} PROPSHEETUI_INFO_HEADERAW;

struct propsheet
{
    int pages_cnt;
    HANDLE pages[100];
    struct list funcs;
};

struct propsheetpage
{
    HPROPSHEETPAGE hpsp;
    DLGPROC dlg_proc;
};

struct propsheetfunc
{
    struct list entry;
    HANDLE handle;
    PFNPROPSHEETUI func;
    LPARAM lparam;
    BOOL unicode;
    ULONG_PTR user_data;
    ULONG_PTR result;
};

#define HANDLE_FIRST 0x43440001
static struct cps_data
{
    enum
    {
        HANDLE_FREE = 0,
        HANDLE_PROPSHEET,
        HANDLE_PROPSHEETPAGE,
        HANDLE_PROPSHEETFUNC,
    } type;
    union
    {
        void *data;
        struct cps_data *next_free;
        struct propsheet *ps;
        struct propsheetpage *psp;
        struct propsheetfunc *psf;
    };
} handles[0x1000];
static struct cps_data *first_free_handle = handles;

static CRITICAL_SECTION handles_cs;
static CRITICAL_SECTION_DEBUG handles_cs_debug =
{
    0, 0, &handles_cs,
    { &handles_cs_debug.ProcessLocksList, &handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": handles_cs") }
};
static CRITICAL_SECTION handles_cs = { &handles_cs_debug, -1, 0, 0, 0, 0 };

static LONG_PTR WINAPI cps_callback(HANDLE hcps, UINT func, LPARAM lparam1, LPARAM lparam2);

static struct cps_data* get_handle_data(HANDLE handle)
{
    struct cps_data *ret;

    if ((ULONG_PTR)handle < HANDLE_FIRST || (ULONG_PTR)handle >= HANDLE_FIRST + ARRAY_SIZE(handles))
        return NULL;

    ret = &handles[(ULONG_PTR)handle - HANDLE_FIRST];
    return ret->type == HANDLE_FREE ? NULL : ret;
}

static HANDLE alloc_handle(struct cps_data **cps_data, int type)
{
    void *data = NULL;

    switch(type)
    {
    case HANDLE_PROPSHEET:
        data = calloc(1, sizeof(struct propsheet));
        break;
    case HANDLE_PROPSHEETPAGE:
        data = calloc(1, sizeof(struct propsheetpage));
        break;
    case HANDLE_PROPSHEETFUNC:
        data = calloc(1, sizeof(struct propsheetfunc));
        break;
    }

    if (!data)
        return NULL;

    EnterCriticalSection(&handles_cs);

    if (first_free_handle >= handles + ARRAY_SIZE(handles))
    {
        LeaveCriticalSection(&handles_cs);
        FIXME("out of handles\n");
        free(data);
        return NULL;
    }

    *cps_data = first_free_handle;
    if ((*cps_data)->next_free)
        first_free_handle = (*cps_data)->next_free;
    else
        first_free_handle = (*cps_data) + 1;
    LeaveCriticalSection(&handles_cs);

    (*cps_data)->type = type;
    (*cps_data)->data = data;
    return (HANDLE)(HANDLE_FIRST + ((*cps_data) - handles));
}

static void free_handle(HANDLE handle)
{
    struct cps_data *data = get_handle_data(handle);

    if (!data)
        return;

    data->type = HANDLE_FREE;
    free(data->data);

    EnterCriticalSection(&handles_cs);
    data->next_free = first_free_handle;
    first_free_handle = data;
    LeaveCriticalSection(&handles_cs);
}

static HANDLE add_hpropsheetpage(HANDLE hcps, HPROPSHEETPAGE hpsp)
{
    struct cps_data *cps_data = get_handle_data(hcps);
    struct cps_data *cpsp_data;
    HANDLE ret;

    if (!cps_data || !hpsp)
        return 0;

    if (cps_data->type != HANDLE_PROPSHEET)
    {
        FIXME("unsupported handle type %d\n", cps_data->type);
        return 0;
    }

    if (cps_data->ps->pages_cnt == ARRAY_SIZE(cps_data->ps->pages))
        return 0;

    ret = alloc_handle(&cpsp_data, HANDLE_PROPSHEETPAGE);
    if (!ret)
        return 0;
    cpsp_data->psp->hpsp = hpsp;

    cps_data->ps->pages[cps_data->ps->pages_cnt++] = ret;
    return ret;
}

static HANDLE add_propsheetfunc(HANDLE hcps, PFNPROPSHEETUI func, LPARAM lparam, BOOL unicode)
{
    struct cps_data *cps_data = get_handle_data(hcps);
    struct cps_data *cpsf_data;
    PROPSHEETUI_INFO info, callback_info;
    HANDLE ret;
    LONG lret;

    if (!cps_data || !func)
        return 0;

    if (cps_data->type != HANDLE_PROPSHEET)
    {
        FIXME("unsupported handle type %d\n", cps_data->type);
        return 0;
    }

    ret = alloc_handle(&cpsf_data, HANDLE_PROPSHEETFUNC);
    if (!ret)
        return 0;
    cpsf_data->psf->handle = ret;
    cpsf_data->psf->func = func;
    cpsf_data->psf->unicode = unicode;
    cpsf_data->psf->lparam = lparam;

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.Version = PROPSHEETUI_INFO_VERSION;
    info.Flags = unicode ? PSUIINFO_UNICODE : 0;
    info.Reason = PROPSHEETUI_REASON_INIT;
    info.hComPropSheet = hcps;
    info.pfnComPropSheet = cps_callback;
    info.lParamInit = lparam;

    callback_info = info;
    lret = func(&callback_info, lparam);
    cpsf_data->psf->user_data = callback_info.UserData;
    cpsf_data->psf->result = callback_info.Result;
    if (lret <= 0)
    {
        callback_info = info;
        callback_info.Reason = PROPSHEETUI_REASON_DESTROY;
        callback_info.UserData = cpsf_data->psf->user_data;
        callback_info.Result = cpsf_data->psf->result;
        free_handle(ret);
        func(&callback_info, 0);
        return 0;
    }

    list_add_tail(&cps_data->ps->funcs, &cpsf_data->psf->entry);
    return ret;
}

static HANDLE add_propsheetpage(HANDLE hcps, void *psp, PSPINFO *info,
        DLGPROC dlg_proc, BOOL unicode)
{
    struct cps_data *cps_data = get_handle_data(hcps);
    struct cps_data *cpsp_data;
    HPROPSHEETPAGE hpsp;
    HANDLE ret;

    if (!cps_data)
        return 0;

    if (cps_data->type != HANDLE_PROPSHEET)
    {
        FIXME("unsupported handle type %d\n", cps_data->type);
        return 0;
    }

    if (cps_data->ps->pages_cnt == ARRAY_SIZE(cps_data->ps->pages))
        return 0;

    ret = alloc_handle(&cpsp_data, HANDLE_PROPSHEETPAGE);
    if (!ret)
        return 0;

    info->cbSize = sizeof(*info);
    info->wReserved = 0;
    info->hComPropSheet = hcps;
    info->hCPSUIPage = ret;
    info->pfnComPropSheet = cps_callback;

    if (unicode)
        hpsp = CreatePropertySheetPageW((PROPSHEETPAGEW*)psp);
    else
        hpsp = CreatePropertySheetPageA((PROPSHEETPAGEA*)psp);
    if (!hpsp)
    {
        free_handle(ret);
        return 0;
    }

    cpsp_data->psp->hpsp = hpsp;
    cpsp_data->psp->dlg_proc = dlg_proc;
    cps_data->ps->pages[cps_data->ps->pages_cnt++] = ret;
    return ret;
}

static INT_PTR CALLBACK propsheetpage_dlg_procW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        PROPSHEETPAGEW *psp = (PROPSHEETPAGEW*)lparam;
        PSPINFO *info = (PSPINFO*)((BYTE*)lparam + psp->dwSize - sizeof(*info));
        struct cps_data *cpsp_data = get_handle_data(info->hCPSUIPage);

        psp->dwSize -= sizeof(*info);
        psp->pfnDlgProc = cpsp_data->psp->dlg_proc;
        SetWindowLongPtrW(hwnd, DWLP_DLGPROC, (LONG_PTR)psp->pfnDlgProc);
        return psp->pfnDlgProc(hwnd, msg, wparam, lparam);
    }

    return FALSE;
}

static HANDLE add_propsheetpageW(HANDLE hcps, PROPSHEETPAGEW *psp)
{
    PROPSHEETPAGEW *psp_copy;
    PSPINFO *info;
    HANDLE ret;

    if (!psp || psp->dwSize < PROPSHEETPAGEW_V1_SIZE)
        return 0;

    psp_copy = (PROPSHEETPAGEW*)malloc(psp->dwSize + sizeof(*info));
    if (!psp_copy)
        return 0;
    memcpy(psp_copy, psp, psp->dwSize);
    psp_copy->dwSize += sizeof(*info);
    psp_copy->pfnDlgProc = propsheetpage_dlg_procW;

    info = (PSPINFO*)((BYTE*)psp_copy + psp->dwSize);
    ret = add_propsheetpage(hcps, psp_copy, info, psp->pfnDlgProc, TRUE);
    free(psp_copy);
    return ret;
}

static INT_PTR CALLBACK propsheetpage_dlg_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        PROPSHEETPAGEA *psp = (PROPSHEETPAGEA*)lparam;
        PSPINFO *info = (PSPINFO*)((BYTE*)lparam + psp->dwSize - sizeof(*info));
        struct cps_data *cpsp_data = get_handle_data(info->hCPSUIPage);

        psp->dwSize -= sizeof(*info);
        psp->pfnDlgProc = cpsp_data->psp->dlg_proc;
        SetWindowLongPtrA(hwnd, DWLP_DLGPROC, (LONG_PTR)psp->pfnDlgProc);
        return psp->pfnDlgProc(hwnd, msg, wparam, lparam);
    }

    return FALSE;
}

static HANDLE add_propsheetpageA(HANDLE hcps, PROPSHEETPAGEA *psp)
{
    PROPSHEETPAGEA *psp_copy;
    PSPINFO *info;
    HANDLE ret;

    if (!psp || psp->dwSize < PROPSHEETPAGEW_V1_SIZE)
        return 0;

    psp_copy = (PROPSHEETPAGEA*)malloc(psp->dwSize + sizeof(*info));
    if (!psp_copy)
        return 0;
    memcpy(psp_copy, psp, psp->dwSize);
    psp_copy->dwSize += sizeof(*info);
    psp_copy->pfnDlgProc = propsheetpage_dlg_procA;

    info = (PSPINFO*)((BYTE*)psp_copy + psp->dwSize);
    ret = add_propsheetpage(hcps, psp_copy, info, psp->pfnDlgProc, FALSE);
    free(psp_copy);
    return ret;
}

static LONG_PTR WINAPI cps_callback(HANDLE hcps, UINT func, LPARAM lparam1, LPARAM lparam2)
{
    TRACE("(%p, %u, %Ix, %Ix\n", hcps, func, lparam1, lparam2);

    switch(func)
    {
    case CPSFUNC_ADD_HPROPSHEETPAGE:
        return (LONG_PTR)add_hpropsheetpage(hcps, (HPROPSHEETPAGE)lparam1);
    case CPSFUNC_ADD_PFNPROPSHEETUIA:
    case CPSFUNC_ADD_PFNPROPSHEETUIW:
        return (LONG_PTR)add_propsheetfunc(hcps, (PFNPROPSHEETUI)lparam1,
                lparam2, func == CPSFUNC_ADD_PFNPROPSHEETUIW);
    case CPSFUNC_ADD_PROPSHEETPAGEA:
        return (LONG_PTR)add_propsheetpageA(hcps, (PROPSHEETPAGEA*)lparam1);
    case CPSFUNC_ADD_PROPSHEETPAGEW:
        return (LONG_PTR)add_propsheetpageW(hcps, (PROPSHEETPAGEW*)lparam1);
    case CPSFUNC_ADD_PCOMPROPSHEETUIA:
    case CPSFUNC_ADD_PCOMPROPSHEETUIW:
    case CPSFUNC_DELETE_HCOMPROPSHEET:
    case CPSFUNC_SET_HSTARTPAGE:
    case CPSFUNC_GET_PAGECOUNT:
    case CPSFUNC_SET_RESULT:
    case CPSFUNC_GET_HPSUIPAGES:
    case CPSFUNC_LOAD_CPSUI_STRINGA:
    case CPSFUNC_LOAD_CPSUI_STRINGW:
    case CPSFUNC_LOAD_CPSUI_ICON:
    case CPSFUNC_GET_PFNPROPSHEETUI_ICON:
    case CPSFUNC_INSERT_PSUIPAGEA:
    case CPSFUNC_INSERT_PSUIPAGEW:
    case CPSFUNC_SET_PSUIPAGE_TITLEA:
    case CPSFUNC_SET_PSUIPAGE_TITLEW:
    case CPSFUNC_SET_PSUIPAGE_ICON:
    case CPSFUNC_SET_DATABLOCK:
    case CPSFUNC_QUERY_DATABLOCK:
    case CPSFUNC_SET_DMPUB_HIDEBITS:
    case CPSFUNC_IGNORE_CPSUI_PSN_APPLY:
    case CPSFUNC_DO_APPLY_CPSUI:
    case CPSFUNC_SET_FUSION_CONTEXT:
        FIXME("func not supported %d\n", func);
        return 0;
    default:
        ERR("unknown func: %d\n", func);
        return 0;
    }
}

static LONG create_property_sheetW(struct propsheet *ps, PROPSHEETUI_INFO_HEADERAW *header)
{
    HPROPSHEETPAGE hpsp[100];
    PROPSHEETHEADERW psh;
    WCHAR title[256];
    LONG_PTR ret;
    int i, len;

    if (!ps->pages_cnt)
        return ERR_CPSUI_NO_PROPSHEETPAGE;

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    if (header->flags & PSUIHDRF_NOAPPLYNOW)
        psh.dwFlags |= PSH_NOAPPLYNOW;
    psh.hwndParent = header->parent;
    psh.hInstance = header->hinst;
    psh.pszCaption = title;

    if (!(header->flags & PSUIHDRF_USEHICON) && !header->icon_id)
        header->icon_id = IDI_CPSUI_OPTION;

    if (header->flags & PSUIHDRF_USEHICON)
    {
        psh.dwFlags |= PSH_USEHICON;
        psh.hIcon = header->hicon;
    }
    else if (header->icon_id >= IDI_CPSUI_ICONID_FIRST &&
            header->icon_id <= IDI_CPSUI_ICONID_LAST)
    {
        FIXME("icon not implemented: %Id\n", header->icon_id);
    }
    else
    {
        psh.dwFlags |= PSH_USEICONID;
        psh.pszIcon = (WCHAR*)header->icon_id;
    }

    if (header->titleW)
        wcscpy_s(title, ARRAY_SIZE(title), header->titleW);
    else
        LoadStringW(compstui_hmod, IDS_CPSUI_OPTIONS, title, ARRAY_SIZE(title));

    if ((header->flags & PSUIHDRF_DEFTITLE) &&
            (!header->titleW || !(header->flags & PSUIHDRF_EXACT_PTITLE)))
    {
        len = wcslen(title);
        if (len < ARRAY_SIZE(title) - 1)
        {
            title[len++] = ' ';
            LoadStringW(compstui_hmod, IDS_CPSUI_DEFAULT, title + len, ARRAY_SIZE(title) - len);
        }
    }

    if ((header->flags & PSUIHDRF_PROPTITLE) &&
            (!header->titleW || !(header->flags & PSUIHDRF_EXACT_PTITLE)))
    {
        len = wcslen(title);
        if (len < ARRAY_SIZE(title) - 1)
        {
            title[len++] = ' ';
            LoadStringW(compstui_hmod, IDS_CPSUI_PROPERTIES, title + len, ARRAY_SIZE(title) - len);
        }
    }

    psh.nPages = ps->pages_cnt;
    psh.phpage = hpsp;
    for (i = 0; i < ps->pages_cnt; i++)
        hpsp[i] = get_handle_data(ps->pages[i])->psp->hpsp;

    ret = PropertySheetW(&psh);

    switch(ret)
    {
    case -1:
        return ERR_CPSUI_GETLASTERROR;
    case ID_PSREBOOTSYSTEM:
        return CPSUI_REBOOTSYSTEM;
    case ID_PSRESTARTWINDOWS:
        return CPSUI_RESTARTWINDOWS;
    default:
        return CPSUI_OK;
    }
}

static LONG create_prop_dlg(HWND hwnd, PFNPROPSHEETUI callback, LPARAM lparam, DWORD *res, BOOL unicode)
{
    PROPSHEETUI_INFO info, callback_info;
    PROPSHEETUI_INFO_HEADERAW header;
    struct cps_data *cps_data;
    HANDLE cps_handle;
    LONG ret;
    int i;

    if (!callback || !(cps_handle = alloc_handle(&cps_data, HANDLE_PROPSHEET)))
    {
        SetLastError(0);
        return ERR_CPSUI_GETLASTERROR;
    }
    list_init(&cps_data->ps->funcs);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.lParamInit = lparam;
    callback_info = info;
    callback_info.Reason = PROPSHEETUI_REASON_BEFORE_INIT;
    callback(&callback_info, lparam);

    info.Version = PROPSHEETUI_INFO_VERSION;
    info.Flags = unicode ? PSUIINFO_UNICODE : 0;
    info.hComPropSheet = cps_handle;
    info.pfnComPropSheet = cps_callback;
    callback_info = info;
    callback_info.Reason = PROPSHEETUI_REASON_INIT;
    ret = callback(&callback_info, lparam);
    info.UserData = callback_info.UserData;
    info.Result = callback_info.Result;
    if (ret <= 0)
    {
        ret = ERR_CPSUI_GETLASTERROR;
        goto destroy;
    }

    memset(&header, 0, sizeof(header));
    header.size = sizeof(header);
    header.parent = hwnd;
    callback_info = info;
    callback_info.Reason = PROPSHEETUI_REASON_GET_INFO_HEADER;
    ret = callback(&callback_info, (LPARAM)&header);
    info.UserData = callback_info.UserData;
    info.Result = callback_info.Result;
    if (ret <= 0)
    {
        ret = ERR_CPSUI_GETLASTERROR;
        goto destroy;
    }

    ret = create_property_sheetW(cps_data->ps, &header);

destroy:
    info.Reason = PROPSHEETUI_REASON_DESTROY;
    while (!list_empty(&cps_data->ps->funcs))
    {
        struct propsheetfunc *func = LIST_ENTRY(list_head(&cps_data->ps->funcs),
                struct propsheetfunc, entry);

        list_remove(&func->entry);

        callback_info = info;
        callback_info.Flags = func->unicode ? PSUIINFO_UNICODE : 0;
        callback_info.lParamInit = func->lparam;
        callback_info.UserData = func->user_data;
        callback_info.Result = func->result;
        func->func(&callback_info, ret <= 0 ? 0 : 0x100);
        free_handle(func->handle);
    }

    callback_info = info;
    callback(&callback_info, ret <= 0 ? 0 : 0x100);

    for (i = 0; i < cps_data->ps->pages_cnt; i++)
        free_handle(cps_data->ps->pages[i]);
    free_handle(cps_handle);

    if (res) *res = callback_info.Result;
    return ret;
}

/******************************************************************
 *      CommonPropertySheetUIA (COMPSTUI.@)
 *
 */
LONG WINAPI CommonPropertySheetUIA(HWND hWnd, PFNPROPSHEETUI pfnPropSheetUI, LPARAM lparam, LPDWORD pResult)
{
    FIXME("(%p, %p, 0x%Ix, %p)\n", hWnd, pfnPropSheetUI, lparam, pResult);
    return CPSUI_CANCEL;
}

/******************************************************************
 *      CommonPropertySheetUIW (COMPSTUI.@)
 *
 */
LONG WINAPI CommonPropertySheetUIW(HWND hwnd, PFNPROPSHEETUI callback, LPARAM lparam, LPDWORD res)
{
    TRACE("(%p, %p, 0x%Ix, %p)\n", hwnd, callback, lparam, res);
    return create_prop_dlg(hwnd, callback, lparam, res, TRUE);
}

/******************************************************************
 *      GetCPSUIUserData (COMPSTUI.@)
 *
 */
ULONG_PTR WINAPI GetCPSUIUserData(HWND hDlg)
{
    FIXME("(%p): stub\n", hDlg);
    return 0;
}

/******************************************************************
 *      SetCPSUIUserData (COMPSTUI.@)
 *
 */
BOOL WINAPI SetCPSUIUserData(HWND hDlg, ULONG_PTR UserData )
{
    FIXME("(%p, %08Ix): stub\n", hDlg, UserData);
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        compstui_hmod = hinst;
    return TRUE;
}
