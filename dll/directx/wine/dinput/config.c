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


#include "wine/unicode.h"
#include "objbase.h"
#include "dinput_private.h"
#include "device_private.h"
#include "resource.h"

#include "wine/heap.h"

typedef struct {
    int nobjects;
    IDirectInputDevice8W *lpdid;
    DIDEVICEINSTANCEW ddi;
    DIDEVICEOBJECTINSTANCEW ddo[256];
    /* ActionFormat for every user.
     * In same order as ConfigureDevicesData usernames */
    DIACTIONFORMATW *user_afs;
} DeviceData;

typedef struct {
    int ndevices;
    DeviceData *devices;
} DIDevicesData;

typedef struct {
    IDirectInput8W *lpDI;
    LPDIACTIONFORMATW original_lpdiaf;
    DIDevicesData devices_data;
    int display_only;
    int nusernames;
    WCHAR **usernames;
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

static BOOL CALLBACK collect_devices(LPCDIDEVICEINSTANCEW lpddi, IDirectInputDevice8W *lpdid, DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) pvRef;
    DeviceData *device;
    int i, j;

    IDirectInputDevice_AddRef(lpdid);

    /* alloc array for devices if this is our first device */
    if (!data->devices_data.ndevices)
        data->devices_data.devices = HeapAlloc(GetProcessHeap(), 0, sizeof(DeviceData) * (dwRemaining + 1));
    device = &data->devices_data.devices[data->devices_data.ndevices];
    device->lpdid = lpdid;
    device->ddi = *lpddi;

    device->nobjects = 0;
    IDirectInputDevice_EnumObjects(lpdid, collect_objects, (LPVOID) device, DIDFT_ALL);

    device->user_afs = heap_alloc(sizeof(*device->user_afs) * data->nusernames);
    memset(device->user_afs, 0, sizeof(*device->user_afs) * data->nusernames);
    for (i = 0; i < data->nusernames; i++)
    {
        DIACTIONFORMATW *user_af = &device->user_afs[i];
        user_af->dwNumActions = data->original_lpdiaf->dwNumActions;
        user_af->guidActionMap = data->original_lpdiaf->guidActionMap;
        user_af->rgoAction = heap_alloc(sizeof(DIACTIONW) * data->original_lpdiaf->dwNumActions);
        memset(user_af->rgoAction, 0, sizeof(DIACTIONW) * data->original_lpdiaf->dwNumActions);
        for (j = 0; j < user_af->dwNumActions; j++)
        {
            user_af->rgoAction[j].dwSemantic = data->original_lpdiaf->rgoAction[j].dwSemantic;
            user_af->rgoAction[j].dwFlags = data->original_lpdiaf->rgoAction[j].dwFlags;
            user_af->rgoAction[j].u.lptszActionName = data->original_lpdiaf->rgoAction[j].u.lptszActionName;
        }
        IDirectInputDevice8_BuildActionMap(lpdid, user_af, data->usernames[i], 0);
    }

    data->devices_data.ndevices++;
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
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessageW (dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTCOLUMNW, 0, (LPARAM) &listColumn);

    LoadStringW(DINPUT_instance, IDS_ACTIONCOLUMN, column, ARRAY_SIZE(column));
    listColumn.cx = width;
    listColumn.pszText = column;
    listColumn.cchTextMax = lstrlenW(listColumn.pszText);

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
    static const WCHAR no_action[] = {'-','\0'};
    const WCHAR *action_text = no_action;
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
    lvItem.cchTextMax = lstrlenW(lvItem.pszText);

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

static DIACTIONFORMATW *get_cur_lpdiaf(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    int controller_sel = SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_GETCURSEL, 0, 0);
    int player_sel = SendDlgItemMessageW(dialog, IDC_PLAYERCOMBO, CB_GETCURSEL, 0, 0);
    return &data->devices_data.devices[controller_sel].user_afs[player_sel];
}

static DIACTIONFORMATW *get_original_lpdiaf(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    return data->original_lpdiaf;
}

static int dialog_display_only(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    return data->display_only;
}

static void init_devices(HWND dialog, ConfigureDevicesData *data)
{
    int i;

    /* Collect and insert */
    data->devices_data.ndevices = 0;
    IDirectInput8_EnumDevicesBySemantics(data->lpDI, NULL, data->original_lpdiaf, collect_devices, (LPVOID) data, 0);

    for (i = 0; i < data->devices_data.ndevices; i++)
        SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_ADDSTRING, 0, (LPARAM) data->devices_data.devices[i].ddi.tszProductName );
    for (i = 0; i < data->nusernames; i++)
        SendDlgItemMessageW(dialog, IDC_PLAYERCOMBO, CB_ADDSTRING, 0, (LPARAM) data->usernames[i]);
}

static void destroy_data(HWND dialog)
{
    int i, j;
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    DIDevicesData *devices_data = &data->devices_data;

    /* Free the devices */
    for (i=0; i < devices_data->ndevices; i++)
    {
        IDirectInputDevice8_Release(devices_data->devices[i].lpdid);
        for (j=0; j < data->nusernames; j++)
            heap_free(devices_data->devices[i].user_afs[j].rgoAction);
        heap_free(devices_data->devices[i].user_afs);
    }

    HeapFree(GetProcessHeap(), 0, devices_data->devices);
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
        DWORD ddo_inst, ddo_type;
        int action = -1;

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = device->ddo[i].tszName;
        item.cchTextMax = lstrlenW(item.pszText);

        /* Add the item */
        SendDlgItemMessageW(dialog, IDC_DEVICEOBJECTSLIST, LVM_INSERTITEMW, 0, (LPARAM) &item);
        ddo_inst = DIDFT_GETINSTANCE(device->ddo[i].dwType);
        ddo_type = DIDFT_GETTYPE(device->ddo[i].dwType);

        /* Search for an assigned action for this device */
        for (j=0; j < lpdiaf->dwNumActions; j++)
        {
            DWORD af_inst = DIDFT_GETINSTANCE(lpdiaf->rgoAction[j].dwObjID);
            DWORD af_type = DIDFT_GETTYPE(lpdiaf->rgoAction[j].dwObjID);
            if (af_type == DIDFT_PSHBUTTON) af_type = DIDFT_BUTTON;
            if (af_type == DIDFT_RELAXIS) af_type = DIDFT_AXIS;
            /* NOTE previously compared dwType == dwObjId but default buildActionMap actions
             * were PSHBUTTON and RELAXS and didnt show up on config */
            if (IsEqualGUID(&lpdiaf->rgoAction[j].guidInstance, &device->ddi.guidInstance) &&
                ddo_inst == af_inst && ddo_type & af_type)
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
    LPDIACTIONFORMATW lpdiaf = get_original_lpdiaf(dialog);
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
    if (lpdiaf->rgoAction[old_action].dwFlags & DIA_APPFIXED) return;

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

static void reset_actions(HWND dialog)
{
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    DIDevicesData *ddata = (DIDevicesData*) &data->devices_data;
    unsigned i, j;

    for (i = 0; i < data->devices_data.ndevices; i++)
    {
        DeviceData *device = &ddata->devices[i];
        for (j = 0; j < data->nusernames; j++)
            IDirectInputDevice8_BuildActionMap(device->lpdid, &device->user_afs[j], data->usernames[j], DIDBAM_HWDEFAULTS);
    }
}

static void save_actions(HWND dialog) {
    ConfigureDevicesData *data = (ConfigureDevicesData*) GetWindowLongPtrW(dialog, DWLP_USER);
    DIDevicesData *ddata = (DIDevicesData*) &data->devices_data;
    unsigned i, j;
    if (!data->display_only) {
        for (i = 0; i < ddata->ndevices; i++)
        {
            DeviceData *device = &ddata->devices[i];
            for (j = 0; j < data->nusernames; j++)
            {
                if (save_mapping_settings(device->lpdid, &device->user_afs[j], data->usernames[j]) != DI_OK)
                    MessageBoxA(dialog, "Could not save settings", 0, MB_ICONERROR);
            }
        }
    }
}

static INT_PTR CALLBACK ConfigureDevicesDlgProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            ConfigureDevicesData *data = (ConfigureDevicesData*) lParam;

            /* Initialize action format and enumerate devices */
            init_devices(dialog, data);

            /* Store information in the window */
            SetWindowLongPtrW(dialog, DWLP_USER, (LONG_PTR) data);

            init_listview_columns(dialog);

            /* Select the first device and show its actions */
            SendDlgItemMessageW(dialog, IDC_CONTROLLERCOMBO, CB_SETCURSEL, 0, 0);
            SendDlgItemMessageW(dialog, IDC_PLAYERCOMBO, CB_SETCURSEL, 0, 0);
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
                case IDC_PLAYERCOMBO:

                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            fill_device_object_list(dialog);
                            break;
                    }
                    break;

                case IDOK:
                    save_actions(dialog);
                    EndDialog(dialog, 0);
                    destroy_data(dialog);
                    break;

                case IDCANCEL:
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
    int i;
    DWORD size;
    WCHAR *username = NULL;
    ConfigureDevicesData data;
    data.lpDI = iface;
    data.original_lpdiaf = lpdiCDParams->lprgFormats;
    data.display_only = !(dwFlags & DICD_EDIT);
    data.nusernames = lpdiCDParams->dwcUsers;
    if (lpdiCDParams->lptszUserNames == NULL)
    {
        /* Get default user name */
        GetUserNameW(NULL, &size);
        username = heap_alloc(size * sizeof(WCHAR) );
        GetUserNameW(username, &size);
        data.nusernames = 1;
        data.usernames = heap_alloc(sizeof(WCHAR *));
        data.usernames[0] = username;
    }
    else
    {
        WCHAR *p = lpdiCDParams->lptszUserNames;
        data.usernames = heap_alloc(sizeof(WCHAR *) * data.nusernames);
        for (i = 0; i < data.nusernames; i++)
        {
            if (*p)
            {
                data.usernames[i] = p;
                while (*(p++));
            }
            else
                /* Return if there is an empty string */
                return DIERR_INVALIDPARAM;
        }
    }

    InitCommonControls();

    DialogBoxParamW(DINPUT_instance, (const WCHAR *)MAKEINTRESOURCE(IDD_CONFIGUREDEVICES),
            lpdiCDParams->hwnd, ConfigureDevicesDlgProc, (LPARAM)&data);

    heap_free(username);
    heap_free(data.usernames);

    return DI_OK;
}
