/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
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

#define NONAMELESSUNION

#include "objbase.h"

#include "dinput_private.h"
#include "device_private.h"
#include "resource.h"

typedef struct {
    int nobjects;
    IDirectInputDevice8W *lpdid;
    DIDEVICEINSTANCEW ddi;
    DIDEVICEOBJECTINSTANCEW ddo[256];
} DeviceData;

typedef struct {
    int ndevices;
    DeviceData *devices;
} DIDevicesData;

typedef struct {
    IDirectInput8W *lpDI;
    LPDIACTIONFORMATW lpdiaf;
    LPDIACTIONFORMATW original_lpdiaf;
    DIDevicesData devices_data;
    int display_only;
} ConfigureDevicesData;

/*
 * Enumeration callback functions
 */
static BOOL CALLBACK collect_objects(LPCDIDEVICEOBJECTINSTANCEW lpddo, LPVOID pvRef)
{
    DeviceData *data = (DeviceData*) pvRef;

    data->ddo[data->nobjects] = *lpddo;

    data->nobjects++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK count_devices(LPCDIDEVICEINSTANCEW lpddi, IDirectInputDevice8W *lpdid, DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    DIDevicesData *data = (DIDevicesData*) pvRef;

    data->ndevices++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK collect_devices(LPCDIDEVICEINSTANCEW lpddi, IDirectInputDevice8W *lpdid, DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    DIDevicesData *data = (DIDevicesData*) pvRef;
    DeviceData *device = &data->devices[data->ndevices];
    device->lpdid = lpdid;
    device->ddi = *lpddi;

    IDirectInputDevice_AddRef(lpdid);

    device->nobjects = 0;
    IDirectInputDevice_EnumObjects(lpdid, collect_objects, (LPVOID) device, DIDFT_ALL);

    data->ndevices++;
    return DIENUM_CONTINUE;
}

/*
 * Listview utility functions
 */
static void init_listview_columns(HWND dialog)
{
    LVCOLUMNW listColumn;
    RECT viewRect;
    int width;
    WCHAR column[MAX_PATH];

    GetClientRect(GetDlgItem(dialog, IDC_DEVICEOBJECTSLIST), &viewRect);
    width = (viewRect.right - viewRect.left)/2;

    LoadStringW(DINPUT_instance, IDS_OBJECTCOLUMN, column, ARRAY_SIZE(column));
    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = column;
    listColumn.cchTextMax = wcslen( listColumn.pszText );
    listColumn.cx = width;

    SendDlgItemMessageW (dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    LoadStringW(DINPUT_instance, IDS_ACTIONCOLUMN, column, ARRAY_SIZE(column));
    listColumn.cx = width;
    listColumn.pszText = column;
    listColumn.cchTextMax = wcslen( listColumn.pszText );

    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTCOLUMNW, 1, (LPARAM) &listColumn);
}

static int lv_get_cur_item(HWND dialog)
{
    return SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
}

static int lv_get_item_data(HWND dialog, int index)
{
    LVITEMW item;

    if (index < 0) return -1;

    item.mask = LVIF_PARAM;
    item.iItem = index;
    item.iSubItem = 0;

    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_GETITEMW , 0, (LPARAM)&item);

    return item.lParam;
}

static void lv_set_action(HWND dialog, int item, int action, LPDIACTIONFORMATW lpdiaf)
{
    const WCHAR *action_text = L"-";
    LVITEMW lvItem;

    if (item < 0) return;

    if (action != -1)
        action_text = lpdiaf->rgoAction[action].u.lptszActionName;

    /* Keep the action and text in the listview item */
    lvItem.iItem = item;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM) action;

    /* Action index */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_SETITEMW, 0, (LPARAM) &lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = 1;
    lvItem.pszText = (WCHAR *)action_text;
    lvItem.cchTextMax = wcslen( lvItem.pszText );

    /* Text */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_SETITEMW, 0, (LPARAM) &lvItem);
}

/*
 * Utility functions
 */
static DeviceData* get_cur_device(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    int sel = SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_GETCURSEL, 0, 0);
    return &data->devices_data.devices[sel];
}

static LPDIACTIONFORMATW get_cur_lpdiaf(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    return data->lpdiaf;
}

static int dialog_display_only(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    return data->display_only;
}

static void init_devices(HWND dialog, IDirectInput8W *lpDI, DIDevicesData *data, LPDIACTIONFORMATW lpdiaf)
{
    int i;

    /* Count devices */
    data->ndevices = 0;
    IDirectInput8_EnumDevicesBySemantics(lpDI, NULL, lpdiaf, count_devices, (LPVOID) data, 0);

    /* Allocate devices */
    data->devices = malloc( sizeof(DeviceData) * data->ndevices );

    /* Collect and insert */
    data->ndevices = 0;
    IDirectInput8_EnumDevicesBySemantics(lpDI, NULL, lpdiaf, collect_devices, (LPVOID) data, 0);

    for (i=0; i < data->ndevices; i++)
        SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_ADDSTRING, 0, (LPARAM) data->devices[i].ddi.tszProductName );
}

static void destroy_data(HWND dialog)
{
    int i;
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    DIDevicesData *devices_data = &data->devices_data;

    /* Free the devices */
    for (i=0; i < devices_data->ndevices; i++)
        IDirectInputDevice8_Release(devices_data->devices[i].lpdid);

    free( devices_data->devices );

    /* Free the backup LPDIACTIONFORMATW  */
    free( data->original_lpdiaf->rgoAction );
    free( data->original_lpdiaf );
}

static void fill_device_object_list(HWND dialog)
{
    DeviceData *device = get_cur_device(dialog);
    LPDIACTIONFORMATW lpdiaf = get_cur_lpdiaf(dialog);
    LVITEMW item;
    int i, j;

    /* Clean the listview */
    SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_DELETEALLITEMS, 0, 0);

    /* Add each object */
    for (i=0; i < device->nobjects; i++)
    {
        int action = -1;

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = device->ddo[i].tszName;
        item.cchTextMax = wcslen( item.pszText );

        /* Add the item */
        SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTITEMW, 0, (LPARAM) &item);

        /* Search for an assigned action  for this device */
        for (j=0; j < lpdiaf->dwNumActions; j++)
        {
            if (IsEqualGUID(&lpdiaf->rgoAction[j].guidInstance, &device->ddi.guidInstance) &&
                lpdiaf->rgoAction[j].dwObjID == device->ddo[i].dwType)
            {
                action = j;
                break;
            }
        }

        lv_set_action(dialog, i, action, lpdiaf);
    }
}

static void show_suitable_actions(HWND dialog)
{
    DeviceData *device = get_cur_device(dialog);
    LPDIACTIONFORMATW lpdiaf = get_cur_lpdiaf(dialog);
    int i, added = 0;
    int obj = lv_get_cur_item(dialog);

    if (obj < 0) return;

    SendDlgItemMessageW(dialog, IDC_ACTIONLIST, LB_RESETCONTENT, 0, 0);

    for (i=0; i < lpdiaf->dwNumActions; i++)
    {
        /* Skip keyboard actions for non keyboards */
        if (GET_DIDEVICE_TYPE(device->ddi.dwDevType) != DI8DEVTYPE_KEYBOARD &&
            (lpdiaf->rgoAction[i].dwSemantic & DIKEYBOARD_MASK) == DIKEYBOARD_MASK) continue;

        /* Skip mouse actions for non mouses */
        if (GET_DIDEVICE_TYPE(device->ddi.dwDevType) != DI8DEVTYPE_MOUSE &&
            (lpdiaf->rgoAction[i].dwSemantic & DIMOUSE_MASK) == DIMOUSE_MASK) continue;

        /* Add action string and index in the action format to the list entry */
        if (DIDFT_GETINSTANCE(lpdiaf->rgoAction[i].dwSemantic) & DIDFT_GETTYPE(device->ddo[obj].dwType))
        {
            SendDlgItemMessageW(dialog, IDC_ACTIONLIST, LB_ADDSTRING, 0, (LPARAM)lpdiaf->rgoAction[i].u.lptszActionName);
            SendDlgItemMessageW(dialog, IDC_ACTIONLIST, LB_SETITEMDATA, added, (LPARAM) i);
            added++;
        }
    }
}

static void assign_action(HWND dialog)
{
    DeviceData *device = get_cur_device(dialog);
    LPDIACTIONFORMATW lpdiaf = get_cur_lpdiaf(dialog);
    LVFINDINFOW lvFind;
    int sel = SendDlgItemMessageW(dialog, IDC_ACTIONLIST, LB_GETCURSEL, 0, 0);
    int action = SendDlgItemMessageW(dialog, IDC_ACTIONLIST, LB_GETITEMDATA, sel, 0);
    int obj = lv_get_cur_item(dialog);
    int old_action = lv_get_item_data(dialog, obj);
    int used_obj;
    DWORD type;

    if (old_action == action) return;
    if (obj < 0) return;
    type = device->ddo[obj].dwType;

    /* Clear old action */
    if (old_action != -1)
    {
        lpdiaf->rgoAction[old_action].dwObjID = 0;
        lpdiaf->rgoAction[old_action].guidInstance = GUID_NULL;
        lpdiaf->rgoAction[old_action].dwHow = DIAH_UNMAPPED;
    }

    /* Find if action text is already set for other object and unset it */
    lvFind.flags = LVFI_PARAM;
    lvFind.lParam = action;

    used_obj = SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_FINDITEMW, -1, (LPARAM) &lvFind);

    lv_set_action(dialog, used_obj, -1, lpdiaf);

    /* Set new action */
    lpdiaf->rgoAction[action].dwObjID = type;
    lpdiaf->rgoAction[action].guidInstance = device->ddi.guidInstance;
    lpdiaf->rgoAction[action].dwHow = DIAH_USERCONFIG;

    /* Set new action in the list */
    lv_set_action(dialog, obj, action, lpdiaf);
}

static void copy_actions(LPDIACTIONFORMATW to, LPDIACTIONFORMATW from)
{
    DWORD i;
    for (i=0; i < from->dwNumActions; i++)
    {
        to->rgoAction[i].guidInstance = from->rgoAction[i].guidInstance;
        to->rgoAction[i].dwObjID = from->rgoAction[i].dwObjID;
        to->rgoAction[i].dwHow = from->rgoAction[i].dwHow;
        to->rgoAction[i].u.lptszActionName = from->rgoAction[i].u.lptszActionName;
    }
}

static void reset_actions(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    LPDIACTIONFORMATW to = data->lpdiaf, from = data->original_lpdiaf;

    copy_actions(to, from);
}

static INT_PTR CALLBACK ConfigureDevicesDlgProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            ConfigureDevicesData *data = (ConfigureDevicesData*) lParam;

            /* Initialize action format and enumerate devices */
            init_devices(dialog, data->lpDI, &data->devices_data, data->lpdiaf);

            /* Store information in the window */
            SetWindowLongPtrW(dialog, DWLP_USER, (LONG_PTR) data);

            init_listview_columns(dialog);

            /* Create a backup action format for CANCEL and RESET operations */
            data->original_lpdiaf = malloc( sizeof(*data->original_lpdiaf) );
            data->original_lpdiaf->dwNumActions = data->lpdiaf->dwNumActions;
            data->original_lpdiaf->rgoAction = malloc( sizeof(DIACTIONW) * data->lpdiaf->dwNumActions );
            copy_actions(data->original_lpdiaf, data->lpdiaf);

            /* Select the first device and show its actions */
            SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_SETCURSEL, 0, 0);
            fill_device_object_list(dialog);

            ShowCursor(TRUE);

            break;
        }

        case WM_DESTROY:
            ShowCursor(FALSE);
            break;

        case WM_NOTIFY:

            switch (((LPNMHDR)lParam)->code)
            {
                case LVN_ITEMCHANGED:
                    show_suitable_actions(dialog);
                    break;
            }
            break;


        case WM_COMMAND:

            switch(LOWORD(wParam))
            {

                case IDC_ACTIONLIST:

                    switch (HIWORD(wParam))
                    {
                        case LBN_DBLCLK:
                            /* Ignore this if app did not ask for editing */
                            if (dialog_display_only(dialog)) break;

                            assign_action(dialog);
                            break;
                    }
                    break;

                case IDC_CONTROLLERCOMBO:

                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            fill_device_object_list(dialog);
                            break;
                    }
                    break;

                case IDOK:
                    EndDialog(dialog, 0);
                    destroy_data(dialog);
                    break;

                case IDCANCEL:
                    reset_actions(dialog);
                    EndDialog(dialog, 0);
                    destroy_data(dialog);
                    break;

                case IDC_RESET:
                    reset_actions(dialog);
                    fill_device_object_list(dialog);
                    break;
            }
        break;
    }

    return FALSE;
}

HRESULT _configure_devices(IDirectInput8W *iface,
                           LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
                           LPDICONFIGUREDEVICESPARAMSW lpdiCDParams,
                           DWORD dwFlags,
                           LPVOID pvRefData
)
{
    ConfigureDevicesData data;
    data.lpDI = iface;
    data.lpdiaf = lpdiCDParams->lprgFormats;
    data.display_only = !(dwFlags & DICD_EDIT);

    InitCommonControls();

    DialogBoxParamW(DINPUT_instance, (const WCHAR *)MAKEINTRESOURCE(IDD_CONFIGUREDEVICES),
            lpdiCDParams->hwnd, ConfigureDevicesDlgProc, (LPARAM)&data);

    return DI_OK;
}
