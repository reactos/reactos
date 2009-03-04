/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/input.c
 * PURPOSE:     ReactX diagnosis input page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"
#include <dinput.h>

typedef struct
{
    HWND hwndDlg;
    IDirectInput8W * pObj;
    HWND hDevList;
    HWND hPortTree;
    INT Count;
}INPUT_DIALOG_CONTEXT, *PINPUT_DIALOG_CONTEXT;


BOOL CALLBACK DirectInputEnumDevCb(
  LPCDIDEVICEINSTANCEW lpddi,
  LPVOID pvRef
)
{
    HRESULT hResult;
    WCHAR szText[100];
    IDirectInputDevice8W * pDev = NULL;
    //DIPROPGUIDANDPATH GuidPath;
    //DIPROPSTRING TypeName;
    DIPROPDWORD VendorID;
    DIDEVCAPS DevCaps;
    DWORD dwProductID;
    DWORD dwManufacturerID;
    LVITEMW Item;
    LRESULT lResult;

    PINPUT_DIALOG_CONTEXT pContext = (PINPUT_DIALOG_CONTEXT)pvRef;

    if (!pContext)
        return DIENUM_STOP;

    ZeroMemory(&Item, sizeof(LVITEMW));
    Item.mask = LVIF_TEXT;
    Item.pszText = (LPWSTR)lpddi->tszProductName;
    Item.iItem = pContext->Count;
    /* insert device item */
    lResult = SendMessageW(pContext->hDevList, LVM_INSERTITEM, 0, (LPARAM)&Item);
    if (lResult == -1)
       return DIENUM_CONTINUE;

    /* is the device attached */
    szText[0] = L'\0';
    hResult = pContext->pObj->lpVtbl->GetDeviceStatus(pContext->pObj, &lpddi->guidInstance);
    if (hResult == DI_OK)
        LoadStringW(hInst, IDS_DEVICE_STATUS_ATTACHED, szText, sizeof(szText) / sizeof(WCHAR));
    else if (hResult == DI_NOTATTACHED)
        LoadStringW(hInst, IDS_DEVICE_STATUS_MISSING, szText, sizeof(szText) / sizeof(WCHAR));
    else
        LoadStringW(hInst, IDS_DEVICE_STATUS_UNKNOWN, szText, sizeof(szText) / sizeof(WCHAR));

    if (szText[0])
    {
       szText[(sizeof(szText) / sizeof(WCHAR))-1] = L'\0';
       Item.iSubItem = 1;
       Item.pszText = szText;
       SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);
    }

    hResult = pContext->pObj->lpVtbl->CreateDevice(pContext->pObj, &lpddi->guidInstance, &pDev, NULL);

    if (hResult != DI_OK)
        return DIENUM_STOP;

    ZeroMemory(&VendorID, sizeof(DIPROPDWORD));
    VendorID.diph.dwSize = sizeof(DIPROPDWORD);
    VendorID.diph.dwHeaderSize = sizeof(DIPROPHEADER);

    hResult = pDev->lpVtbl->GetProperty(pDev, DIPROP_VIDPID, (LPDIPROPHEADER)&VendorID);
    if (hResult == DI_OK)
    {
        /* set manufacturer id */
        dwManufacturerID = LOWORD(VendorID.dwData);
        wsprintfW(szText, L"0x%04X", dwManufacturerID);
        Item.iSubItem = 3;
        SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);
        /* set product id */
        dwProductID = HIWORD(VendorID.dwData);
        wsprintfW(szText, L"0x%04X", dwProductID);
        Item.iSubItem = 4;
        SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);
    }
    else
    {
        szText[0] = L'\0';
        LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText) / sizeof(WCHAR));
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        /* set manufacturer id */
        Item.iSubItem = 3;
        SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);
        /* set product id */
        Item.iSubItem = 4;
        SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);
    }

    /* check for force feedback support */
    DevCaps.dwSize = sizeof(DIDEVCAPS); // use DIDEVCAPS_DX3 for DX3 support
    hResult = pDev->lpVtbl->GetCapabilities(pDev, &DevCaps);
    szText[0] = L'\0';
    if (hResult == DI_OK)
    {
        if (DevCaps.dwFlags & DIDC_FORCEFEEDBACK)
            LoadStringW(hInst, IDS_OPTION_YES, szText, sizeof(szText)/sizeof(WCHAR));
        else
            LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText)/sizeof(WCHAR));
    }
    else
    {
        LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText)/sizeof(WCHAR));
    }

    Item.iSubItem = 5;
    SendMessageW(pContext->hDevList, LVM_SETITEM, lResult, (LPARAM)&Item);


#if 0
    ZeroMemory(&GuidPath, sizeof(DIPROPGUIDANDPATH));
    GuidPath.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    GuidPath.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	GuidPath.diph.dwHow = DIPH_DEVICE;
    hResult = pDev->lpVtbl->GetProperty(pDev, DIPROP_GUIDANDPATH, (LPDIPROPHEADER)&GuidPath);

    ZeroMemory(&TypeName, sizeof(TypeName));
    TypeName.diph.dwSize = sizeof(TypeName);
    TypeName.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	TypeName.diph.dwHow = DIPH_DEVICE;
    hResult = pDev->lpVtbl->GetProperty(pDev, DIPROP_GETPORTDISPLAYNAME, (LPDIPROPHEADER)&TypeName);


#endif


    pDev->lpVtbl->Release(pDev);
    pContext->Count++;


    return DIENUM_CONTINUE;
}

VOID
InitListViewColumns(PINPUT_DIALOG_CONTEXT pContext)
{
    WCHAR szText[256];
    LVCOLUMNW lvcolumn;
    INT Index;


    pContext->hDevList = GetDlgItem(pContext->hwndDlg, IDC_LIST_DEVICE);

    ZeroMemory(&lvcolumn, sizeof(LVCOLUMNW));
    lvcolumn.pszText = szText;
    lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvcolumn.fmt = LVCFMT_LEFT;
    lvcolumn.cx = 100;

    for(Index = 0; Index < 6; Index++)
    {
        szText[0] = L'\0';
        LoadStringW(hInst, IDS_DEVICE_NAME + Index, szText, sizeof(szText) / sizeof(WCHAR));
        szText[(sizeof(szText) / sizeof(WCHAR))-1] = L'\0';
        if (SendMessageW(pContext->hDevList, LVM_INSERTCOLUMNW, Index, (LPARAM)&lvcolumn) == -1)
            return;
    }
}

static
void
InitializeDirectInputDialog(HWND hwndDlg)
{
    INPUT_DIALOG_CONTEXT Context;
    HRESULT hResult;
    IDirectInput8W * pObj;

    hResult = DirectInput8Create(hInst, DIRECTINPUT_VERSION, &IID_IDirectInput8W, (LPVOID*)&pObj, NULL);
    if (hResult != DI_OK)
        return;

    ZeroMemory(&Context, sizeof(Context));
    Context.pObj = pObj;
    Context.hwndDlg = hwndDlg;
    InitListViewColumns(&Context);
    hResult = pObj->lpVtbl->EnumDevices(pObj, DI8DEVCLASS_ALL, DirectInputEnumDevCb, (PVOID)&Context, DIEDFL_ALLDEVICES);

    pObj->lpVtbl->Release(pObj);
}



INT_PTR CALLBACK
InputPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message) {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeDirectInputDialog(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}
