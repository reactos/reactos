/*
 * Internet control panel applet: security propsheet
 *
 * Copyright 2011 Detlef Riekenberg
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
#define CONST_VTABLE

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <prsht.h>
#include "commctrl.h"

#include "ole2.h"
#include "urlmon.h"
#include "initguid.h"
#include "winreg.h"
#include "shlwapi.h"

#include "inetcpl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

typedef struct secdlg_data_s {
    HWND hsec;  /* security propsheet */
    HWND hlv;   /* listview */
    HWND htb;   /* trackbar */
    IInternetSecurityManager *sec_mgr;
    IInternetZoneManager *zone_mgr;
    DWORD zone_enumerator;
    DWORD num_zones;
    ZONEATTRIBUTES *zone_attr;
    DWORD *zones;
    DWORD *levels;
    HIMAGELIST himages;
    DWORD last_lv_index;
    DWORD last_level;
} secdlg_data;

#define NUM_TRACKBAR_POS 5

static DWORD url_templates[] = {URLTEMPLATE_CUSTOM,
                                URLTEMPLATE_LOW,
                                URLTEMPLATE_MEDLOW,
                                URLTEMPLATE_MEDIUM,
                                URLTEMPLATE_MEDHIGH,
                                URLTEMPLATE_HIGH};

/*********************************************************************
 * index_from_urltemplate [internal]
 *
 */
static DWORD index_from_urltemplate(URLTEMPLATE value)
{

    DWORD index = ARRAY_SIZE(url_templates);

    while((index > 0) && (url_templates[index-1] != value))
        index--;

    index--; /* table entries are 0 based */
    if (!index && value)
        FIXME("URLTEMPLATE 0x%x not supported\n", value);

    TRACE("URLTEMPLATE 0x%08x=> Level %d\n", value, index);
    return index;
}

/*********************************************************************
 * update_security_level [internal]
 *
 */
static void update_security_level(secdlg_data *sd, DWORD lv_index, DWORD tb_index)
{
    WCHAR name[512];
    DWORD current_index;

    TRACE("(%p, lv_index: %u, tb_index: %u)\n", sd, lv_index, tb_index);

    if ((sd->levels[lv_index] != sd->last_level) || (tb_index > 0)) {
        /* show or hide the trackbar */
        if (!sd->levels[lv_index] || !sd->last_level)
            ShowWindow(sd->htb, sd->levels[lv_index] ? SW_NORMAL : SW_HIDE);

        current_index = (tb_index > 0) ? tb_index : index_from_urltemplate(sd->levels[lv_index]);

        name[0] = 0;
        LoadStringW(hcpl, IDS_SEC_LEVEL0 + current_index, name, ARRAY_SIZE(name));
        TRACE("new level #%d: %s\n", current_index, debugstr_w(name));
        SetWindowTextW(GetDlgItem(sd->hsec, IDC_SEC_LEVEL), name);

        name[0] = 0;
        LoadStringW(hcpl, IDS_SEC_LEVEL0_INFO + (current_index * 0x10), name, ARRAY_SIZE(name));
        TRACE("new level info: %s\n", debugstr_w(name));
        SetWindowTextW(GetDlgItem(sd->hsec, IDC_SEC_LEVEL_INFO), name);

        if (current_index)
            SendMessageW(sd->htb, TBM_SETPOS, TRUE, NUM_TRACKBAR_POS - current_index);

        sd->last_level = sd->levels[lv_index];

    }
}

/*********************************************************************
 * update_zone_info [internal]
 *
 */
static void update_zone_info(secdlg_data *sd, DWORD lv_index)
{
    ZONEATTRIBUTES *za = &sd->zone_attr[lv_index];
    WCHAR name[MAX_PATH];
    DWORD len;

    SetWindowTextW(GetDlgItem(sd->hsec, IDC_SEC_ZONE_INFO), za->szDescription);

    len = LoadStringW(hcpl, IDS_SEC_SETTINGS, name, ARRAY_SIZE(name));
    lstrcpynW(&name[len], za->szDisplayName, ARRAY_SIZE(name) - len - 1);

    TRACE("new title: %s\n", debugstr_w(name));
    SetWindowTextW(GetDlgItem(sd->hsec, IDC_SEC_GROUP), name);

    update_security_level(sd, lv_index, 0);
    sd->last_lv_index = lv_index;
}

/*********************************************************************
 * add_zone_to_listview [internal]
 *
 */
static void add_zone_to_listview(secdlg_data *sd, DWORD *pindex, DWORD zone)
{
    DWORD lv_index = *pindex;
    ZONEATTRIBUTES *za = &sd->zone_attr[lv_index];
    LVITEMW lvitem;
    HRESULT hr;
    INT iconid = 0;
    HMODULE hdll = NULL;
    WCHAR * ptr;
    HICON icon;

    TRACE("item %d (zone %d)\n", lv_index, zone);

    sd->zones[lv_index] = zone;

    memset(&lvitem, 0, sizeof(LVITEMW));
    memset(za, 0, sizeof(ZONEATTRIBUTES));
    za->cbSize = sizeof(ZONEATTRIBUTES);
    hr = IInternetZoneManager_GetZoneAttributes(sd->zone_mgr, zone, za);
    if (SUCCEEDED(hr)) {
        TRACE("displayname: %s\n", debugstr_w(za->szDisplayName));
        TRACE("description: %s\n", debugstr_w(za->szDescription));
        TRACE("minlevel: 0x%x, recommended: 0x%x, current: 0x%x (flags: 0x%x)\n", za->dwTemplateMinLevel,
             za->dwTemplateRecommended, za->dwTemplateCurrentLevel, za->dwFlags);

        if (za->dwFlags & ZAFLAGS_NO_UI ) {
            TRACE("item %d (zone %d): UI disabled for %s\n", lv_index, zone, debugstr_w(za->szDisplayName));
            return;
        }

        sd->levels[lv_index] = za->dwTemplateCurrentLevel;

        lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lvitem.iItem = lv_index;
        lvitem.iSubItem = 0;
        lvitem.pszText = za->szDisplayName;
        lvitem.lParam = (LPARAM) zone;

        /* format is "filename.ext#iconid" */
        ptr = StrChrW(za->szIconPath, '#');
        if (ptr) {
            *ptr = 0;
            ptr++;
            iconid = StrToIntW(ptr);
            hdll = LoadLibraryExW(za->szIconPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
            TRACE("%p: icon #%d from %s\n", hdll, iconid, debugstr_w(za->szIconPath));

            icon = LoadImageW(hdll, MAKEINTRESOURCEW(iconid), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                              GetSystemMetrics(SM_CYICON), LR_SHARED);

            if (!icon) {
                FIXME("item %d (zone %d): missing icon #%d in %s\n", lv_index, zone, iconid, debugstr_w(za->szIconPath));
            }

            /* the failure result (NULL) from LoadImageW let ImageList_AddIcon fail
               with -1, which is reused in ListView_InsertItemW to disable the image */
            lvitem.iImage = ImageList_AddIcon(sd->himages, icon);
        }
        else
            FIXME("item %d (zone %d): malformed szIconPath %s\n", lv_index, zone, debugstr_w(za->szIconPath));

        if (ListView_InsertItemW(sd->hlv, &lvitem) >= 0) {
            /* activate first item in the listview */
            if (! lv_index) {
                lvitem.state = LVIS_FOCUSED | LVIS_SELECTED;
                lvitem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                SendMessageW(sd->hlv, LVM_SETITEMSTATE, 0, (LPARAM) &lvitem);
                sd->last_level = ~0;
                update_zone_info(sd, lv_index);
            }
            (*pindex)++;
        }
        FreeLibrary(hdll);
    }
    else
        FIXME("item %d (zone %d): GetZoneAttributes failed with 0x%x\n", lv_index, zone, hr);
}

/*********************************************************************
 * security_cleanup_zones [internal]
 *
 */
static void security_cleanup_zones(secdlg_data *sd)
{
    if (sd->zone_enumerator) {
        IInternetZoneManager_DestroyZoneEnumerator(sd->zone_mgr, sd->zone_enumerator);
    }

    if (sd->zone_mgr) {
        IInternetZoneManager_Release(sd->zone_mgr);
    }

    if (sd->sec_mgr) {
        IInternetSecurityManager_Release(sd->sec_mgr);
    }
}

/*********************************************************************
 * security_enum_zones [internal]
 *
 */
static HRESULT security_enum_zones(secdlg_data * sd)
{
    HRESULT hr;

    hr = CoInternetCreateSecurityManager(NULL, &sd->sec_mgr, 0);
    if (SUCCEEDED(hr)) {
        hr = CoInternetCreateZoneManager(NULL, &sd->zone_mgr, 0);
        if (SUCCEEDED(hr)) {
            hr = IInternetZoneManager_CreateZoneEnumerator(sd->zone_mgr, &sd->zone_enumerator, &sd->num_zones, 0);
        }
    }
    return hr;
}

/*********************************************************************
 * security_on_destroy [internal]
 *
 * handle WM_NCDESTROY
 *
 */
static INT_PTR security_on_destroy(secdlg_data * sd)
{
    TRACE("(%p)\n", sd);

    heap_free(sd->zone_attr);
    heap_free(sd->zones);
    if (sd->himages) {
        SendMessageW(sd->hlv, LVM_SETIMAGELIST, LVSIL_NORMAL, 0);
        ImageList_Destroy(sd->himages);
    }

    security_cleanup_zones(sd);
    SetWindowLongPtrW(sd->hsec, DWLP_USER, 0);
    heap_free(sd);
    return TRUE;
}

/*********************************************************************
 * security_on_initdialog [internal]
 *
 * handle WM_INITDIALOG
 *
 */
static INT_PTR security_on_initdialog(HWND hsec)
{
    secdlg_data *sd;
    HRESULT hr;
    DWORD current_zone;
    DWORD lv_index = 0;
    DWORD i;

    sd = heap_alloc_zero(sizeof(secdlg_data));
    SetWindowLongPtrW(hsec, DWLP_USER, (LONG_PTR) sd);
    if (!sd) {
        return FALSE;
    }

    sd->hsec = hsec;
    sd->hlv = GetDlgItem(hsec, IDC_SEC_LISTVIEW);
    sd->htb = GetDlgItem(hsec, IDC_SEC_TRACKBAR);

    EnableWindow(sd->htb, FALSE); /* not changeable yet */

    TRACE("(%p)   (data: %p, listview: %p, trackbar: %p)\n", hsec, sd, sd->hlv, sd->htb);

    SendMessageW(sd->htb, TBM_SETRANGE, FALSE, MAKELONG(0, NUM_TRACKBAR_POS - 1));
    SendMessageW(sd->htb, TBM_SETTICFREQ, 1, 0 );

    /* Create the image lists for the listview */
    sd->himages = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_COLOR32 | ILC_MASK, 1, 1);

    TRACE("using imagelist: %p\n", sd->himages);
    if (!sd->himages) {
        ERR("ImageList_Create failed!\n");
        return FALSE;
    }
    SendMessageW(sd->hlv, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)sd->himages);

    hr = security_enum_zones(sd);
    if (FAILED(hr)) {
        ERR("got 0x%x\n", hr);
        security_on_destroy(sd);
        return FALSE;
    }

    TRACE("found %d zones\n", sd->num_zones);

    /* remember ZONEATTRIBUTES for a listview entry */
    sd->zone_attr = heap_alloc(sizeof(ZONEATTRIBUTES) * sd->num_zones);
    if (!sd->zone_attr) {
        security_on_destroy(sd);
        return FALSE;
    }

    /* remember zone number and current security level for a listview entry */
    sd->zones = heap_alloc((sizeof(DWORD) + sizeof(DWORD)) * sd->num_zones);
    if (!sd->zones) {
        security_on_destroy(sd);
        return FALSE;
    }
    sd->levels = &sd->zones[sd->num_zones];

    /* use the same order as visible with native inetcpl.cpl */
    add_zone_to_listview(sd, &lv_index, URLZONE_INTERNET);
    add_zone_to_listview(sd, &lv_index, URLZONE_INTRANET);
    add_zone_to_listview(sd, &lv_index, URLZONE_TRUSTED);
    add_zone_to_listview(sd, &lv_index, URLZONE_UNTRUSTED);

    for (i = 0; i < sd->num_zones; i++)
    {
        hr = IInternetZoneManager_GetZoneAt(sd->zone_mgr, sd->zone_enumerator, i, &current_zone);
        if (SUCCEEDED(hr) && (current_zone != (DWORD)URLZONE_INVALID)) {
            if (!current_zone || (current_zone > URLZONE_UNTRUSTED)) {
                add_zone_to_listview(sd, &lv_index, current_zone);
            }
        }
    }
    return TRUE;
}

/*********************************************************************
 * security_on_notify [internal]
 *
 * handle WM_NOTIFY
 *
 */
static INT_PTR security_on_notify(secdlg_data *sd, WPARAM wparam, LPARAM lparam)
{
    NMLISTVIEW *nm;

    nm = (NMLISTVIEW *) lparam;
    switch (nm->hdr.code)
    {
        case LVN_ITEMCHANGED:
            TRACE("LVN_ITEMCHANGED (0x%lx, 0x%lx) from %p with code: %d (item: %d, uNewState: %u)\n",
                    wparam, lparam, nm->hdr.hwndFrom, nm->hdr.code, nm->iItem, nm->uNewState);
            if ((nm->uNewState & LVIS_SELECTED) == LVIS_SELECTED) {
                update_zone_info(sd, nm->iItem);
            }
            break;

        case PSN_APPLY:
            TRACE("PSN_APPLY (0x%lx, 0x%lx) from %p with code: %d\n", wparam, lparam,
                    nm->hdr.hwndFrom, nm->hdr.code);
            break;

        default:
            TRACE("WM_NOTIFY (0x%lx, 0x%lx) from %p with code: %d\n", wparam, lparam,
                    nm->hdr.hwndFrom, nm->hdr.code);

    }
    return FALSE;
}

/*********************************************************************
 * security_dlgproc [internal]
 *
 */
INT_PTR CALLBACK security_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    secdlg_data *sd;

    if (msg == WM_INITDIALOG) {
        return security_on_initdialog(hwnd);
    }

    sd = (secdlg_data *)GetWindowLongPtrW(hwnd, DWLP_USER);
    if (sd) {
        switch (msg)
        {
            case WM_NOTIFY:
                return security_on_notify(sd, wparam, lparam);

            case WM_NCDESTROY:
                return security_on_destroy(sd);

            default:
                /* do not flood the log */
                if ((msg == WM_SETCURSOR) || (msg == WM_NCHITTEST) ||
                    (msg == WM_MOUSEMOVE) || (msg == WM_MOUSEACTIVATE) || (msg == WM_PARENTNOTIFY))
                    return FALSE;

                TRACE("(%p, 0x%08x/%03d, 0x%08lx, 0x%08lx)\n", hwnd, msg, msg, wparam, lparam);
        }
    }
    return FALSE;
}
