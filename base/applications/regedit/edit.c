/*
 * Registry editing UI functions.
 *
 * Copyright (C) 2003 Dimitrie O. Paun
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

#define NTOS_MODE_USER
#include <ndk/cmtypes.h>

typedef enum _EDIT_MODE
{
    EDIT_MODE_DEC,
    EDIT_MODE_HEX
} EDIT_MODE;


static const WCHAR* editValueName;
static WCHAR* stringValueData;
static PVOID binValueData;
static DWORD dwordValueData;
static PCM_RESOURCE_LIST resourceValueData;
static INT fullResourceIndex = -1;
static DWORD valueDataLen;
static PIO_RESOURCE_REQUIREMENTS_LIST requirementsValueData;
static INT requirementsIndex = -1;
static EDIT_MODE dwordEditMode = EDIT_MODE_HEX;

void error(HWND hwnd, INT resId, ...)
{
    va_list ap;
    WCHAR title[256];
    WCHAR errfmt[1024];
    WCHAR errstr[1024];
    HINSTANCE hInstance;

    hInstance = GetModuleHandle(0);

    if (!LoadStringW(hInstance, IDS_ERROR, title, ARRAY_SIZE(title)))
        StringCbCopyW(title, sizeof(title), L"Error");

    if (!LoadStringW(hInstance, resId, errfmt, ARRAY_SIZE(errfmt)))
        StringCbCopyW(errfmt, sizeof(errfmt), L"Unknown error string!");

    va_start(ap, resId);
    _vsnwprintf(errstr, ARRAY_SIZE(errstr), errfmt, ap);
    va_end(ap);

    MessageBoxW(hwnd, errstr, title, MB_OK | MB_ICONERROR);
}

static void error_code_messagebox(HWND hwnd, DWORD error_code)
{
    WCHAR title[256];
    if (!LoadStringW(hInst, IDS_ERROR, title, ARRAY_SIZE(title)))
        StringCbCopyW(title, sizeof(title), L"Error");
    ErrorMessageBox(hwnd, title, error_code);
}

void warning(HWND hwnd, INT resId, ...)
{
    va_list ap;
    WCHAR title[256];
    WCHAR errfmt[1024];
    WCHAR errstr[1024];
    HINSTANCE hInstance;

    hInstance = GetModuleHandle(0);

    if (!LoadStringW(hInstance, IDS_WARNING, title, ARRAY_SIZE(title)))
        StringCbCopyW(title, sizeof(title), L"Warning");

    if (!LoadStringW(hInstance, resId, errfmt, ARRAY_SIZE(errfmt)))
        StringCbCopyW(errfmt, sizeof(errfmt), L"Unknown error string!");

    va_start(ap, resId);
    StringCbVPrintfW(errstr, sizeof(errstr), errfmt, ap);
    va_end(ap);

    MessageBoxW(hwnd, errstr, title, MB_OK | MB_ICONSTOP);
}

INT_PTR CALLBACK modify_string_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR* valueData;
    HWND hwndValue;
    int len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        if (editValueName && wcscmp(editValueName, L""))
        {
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
            WCHAR buffer[255];
            LoadStringW(hInst, IDS_DEFAULT_VALUE_NAME, buffer, ARRAY_SIZE(buffer));
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, stringValueData);
        SendMessage(GetDlgItem(hwndDlg, IDC_VALUE_DATA), EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if ((len = GetWindowTextLength(hwndValue)))
                {
                    if (stringValueData)
                    {
                        if ((valueData = HeapReAlloc(GetProcessHeap(), 0, stringValueData, (len + 1) * sizeof(WCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowTextW(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                    else
                    {
                        if ((valueData = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowTextW(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                }
                else
                {
                    if (stringValueData)
                        *stringValueData = 0;
                }
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK modify_multi_string_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR* valueData;
    HWND hwndValue;
    int len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        if (editValueName && wcscmp(editValueName, L""))
        {
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
            WCHAR buffer[255];
            LoadStringW(hInst, IDS_DEFAULT_VALUE_NAME, buffer, ARRAY_SIZE(buffer));
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, stringValueData);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if ((len = GetWindowTextLength(hwndValue)))
                {
                    if (stringValueData)
                    {
                        if ((valueData = HeapReAlloc(GetProcessHeap(), 0, stringValueData, (len + 1) * sizeof(WCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowTextW(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                    else
                    {
                        if ((valueData = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowTextW(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                }
                else
                {
                    if (stringValueData)
                        *stringValueData = 0;
                }
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

LRESULT CALLBACK DwordEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldwndproc;

    oldwndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_CHAR:
        if (dwordEditMode == EDIT_MODE_DEC)
        {
            if (isdigit((int)wParam & 0xff) || iscntrl((int)wParam & 0xff))
            {
                break;
            }
            else
            {
                return 0;
            }
        }
        else if (dwordEditMode == EDIT_MODE_HEX)
        {
            if (isxdigit((int)wParam & 0xff) || iscntrl((int)wParam & 0xff))
            {
                break;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            break;
        }
    }

    return CallWindowProcW(oldwndproc, hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK modify_dword_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc;
    HWND hwndValue;
    WCHAR ValueString[32];
    LPWSTR Remainder;
    DWORD Base;
    DWORD Value = 0;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        dwordEditMode = EDIT_MODE_HEX;

        /* subclass the edit control */
        hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
        oldproc = (WNDPROC)GetWindowLongPtr(hwndValue, GWLP_WNDPROC);
        SetWindowLongPtr(hwndValue, GWLP_USERDATA, (DWORD_PTR)oldproc);
        SetWindowLongPtr(hwndValue, GWLP_WNDPROC, (DWORD_PTR)DwordEditSubclassProc);

        if (editValueName && wcscmp(editValueName, L""))
        {
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
            WCHAR buffer[255];
            LoadStringW(hInst, IDS_DEFAULT_VALUE_NAME, buffer, ARRAY_SIZE(buffer));
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        CheckRadioButton (hwndDlg, IDC_FORMAT_HEX, IDC_FORMAT_DEC, IDC_FORMAT_HEX);
        StringCbPrintfW(ValueString, sizeof(ValueString), L"%lx", dwordValueData);
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, ValueString);
        SendMessage(GetDlgItem(hwndDlg, IDC_VALUE_DATA), EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_FORMAT_HEX:
            if (HIWORD(wParam) == BN_CLICKED && dwordEditMode == EDIT_MODE_DEC)
            {
                dwordEditMode = EDIT_MODE_HEX;
                if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
                {
                    if (GetWindowTextLength(hwndValue))
                    {
                        if (GetWindowTextW(hwndValue, ValueString, 32))
                        {
                            Value = wcstoul (ValueString, &Remainder, 10);
                        }
                    }
                }
                StringCbPrintfW(ValueString, sizeof(ValueString), L"%lx", Value);
                SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, ValueString);
                return TRUE;
            }
            break;

        case IDC_FORMAT_DEC:
            if (HIWORD(wParam) == BN_CLICKED && dwordEditMode == EDIT_MODE_HEX)
            {
                dwordEditMode = EDIT_MODE_DEC;
                if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
                {
                    if (GetWindowTextLength(hwndValue))
                    {
                        if (GetWindowTextW(hwndValue, ValueString, 32))
                        {
                            Value = wcstoul (ValueString, &Remainder, 16);
                        }
                    }
                }
                StringCbPrintfW(ValueString, sizeof(ValueString), L"%lu", Value);
                SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, ValueString);
                return TRUE;
            }
            break;

        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if (GetWindowTextLength(hwndValue))
                {
                    if (!GetWindowTextW(hwndValue, ValueString, 32))
                    {
                        EndDialog(hwndDlg, IDCANCEL);
                        return TRUE;
                    }

                    Base = (dwordEditMode == EDIT_MODE_HEX) ? 16 : 10;
                    dwordValueData = wcstoul (ValueString, &Remainder, Base);
                }
                else
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                }
            }
            EndDialog(hwndDlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK modify_binary_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndValue;
    UINT len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        if (editValueName && wcscmp(editValueName, L""))
        {
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
            WCHAR buffer[255];
            LoadStringW(hInst, IDS_DEFAULT_VALUE_NAME, buffer, ARRAY_SIZE(buffer));
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
        HexEdit_LoadBuffer(hwndValue, binValueData, valueDataLen);
        /* reset the hex edit control's font */
        SendMessageW(hwndValue, WM_SETFONT, 0, 0);
        SetFocus(hwndValue);
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                len = (UINT)HexEdit_GetBufferSize(hwndValue);
                if (len > 0 && binValueData)
                    binValueData = HeapReAlloc(GetProcessHeap(), 0, binValueData, len);
                else
                    binValueData = HeapAlloc(GetProcessHeap(), 0, len + 1);
                HexEdit_CopyBuffer(hwndValue, binValueData, len);
                valueDataLen = len;
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL CreateResourceColumns(HWND hwnd)
{
    WCHAR szText[80];
    RECT rc;
    LVCOLUMN lvC;
    HWND hwndLV;
    INT width;

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;
    lvC.fmt = LVCFMT_LEFT;

    hwndLV = GetDlgItem(hwnd, IDC_DMA_LIST);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
    GetClientRect(hwndLV, &rc);

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = (rc.right - rc.left) / 2;
    LoadStringW(hInst, IDS_DMA_CHANNEL, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    lvC.cx = (rc.right - rc.left) - lvC.cx;
    LoadStringW(hInst, IDS_DMA_PORT, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 1, &lvC) == -1)
        return FALSE;


    /* Interrupt list */
    hwndLV = GetDlgItem(hwnd, IDC_IRQ_LIST);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
    GetClientRect(hwndLV, &rc);
    width = (rc.right - rc.left) / 4;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = width;
    LoadStringW(hInst, IDS_INTERRUPT_VECTOR, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    LoadStringW(hInst, IDS_INTERRUPT_LEVEL, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 1, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 2;
    LoadStringW(hInst, IDS_INTERRUPT_AFFINITY, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 2, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 3;
    lvC.cx = (rc.right - rc.left) - 3 * width;
    LoadStringW(hInst, IDS_INTERRUPT_TYPE, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 3, &lvC) == -1)
        return FALSE;


    /* Memory list */
    hwndLV = GetDlgItem(hwnd, IDC_MEMORY_LIST);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
    GetClientRect(hwndLV, &rc);
    width = (rc.right - rc.left) / 3;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = width;
    LoadStringW(hInst, IDS_MEMORY_ADDRESS, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    LoadStringW(hInst, IDS_MEMORY_LENGTH, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 1, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 2;
    lvC.cx = (rc.right - rc.left) - 2 * width;
    LoadStringW(hInst, IDS_MEMORY_ACCESS, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 2, &lvC) == -1)
        return FALSE;


    /* Port list */
    hwndLV = GetDlgItem(hwnd, IDC_PORT_LIST);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
    GetClientRect(hwndLV, &rc);
    width = (rc.right - rc.left) / 3;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = width;
    LoadStringW(hInst, IDS_PORT_ADDRESS, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    LoadStringW(hInst, IDS_PORT_LENGTH, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 1, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 2;
    lvC.cx = (rc.right - rc.left) - 2 * width;
    LoadStringW(hInst, IDS_PORT_ACCESS, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 2, &lvC) == -1)
        return FALSE;

    /* Device specific list */
    hwndLV = GetDlgItem(hwnd, IDC_DEVICE_LIST);
    ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);
    GetClientRect(hwndLV, &rc);
    width = (rc.right - rc.left) / 3;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = width;
    LoadStringW(hInst, IDS_SPECIFIC_RESERVED1, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    LoadStringW(hInst, IDS_SPECIFIC_RESERVED2, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 1, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 2;
    lvC.cx = (rc.right - rc.left) - 2 * width;
    LoadStringW(hInst, IDS_SPECIFIC_DATASIZE, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hwndLV, 2, &lvC) == -1)
        return FALSE;

    return TRUE;
}

static VOID
GetInterfaceType(INTERFACE_TYPE InterfaceType,
                 LPWSTR pBuffer,
                 DWORD dwLength)
{
//    LPWSTR lpInterfaceType;

    switch (InterfaceType)
    {
        case InterfaceTypeUndefined:
            LoadStringW(hInst, IDS_BUS_UNDEFINED, pBuffer, dwLength);
//            lpInterfaceType = L"Undefined";
            break;
        case Internal:
            LoadStringW(hInst, IDS_BUS_INTERNAL, pBuffer, dwLength);
//            lpInterfaceType = L"Internal";
            break;
        case Isa:
            LoadStringW(hInst, IDS_BUS_ISA, pBuffer, dwLength);
//            lpInterfaceType = L"Isa";
            break;
        case Eisa:
            LoadStringW(hInst, IDS_BUS_EISA, pBuffer, dwLength);
//            lpInterfaceType = L"Eisa";
            break;
        case MicroChannel:
            LoadStringW(hInst, IDS_BUS_MICROCHANNEL, pBuffer, dwLength);
//            lpInterfaceType = L"MicroChannel";
            break;
        case TurboChannel:
            LoadStringW(hInst, IDS_BUS_TURBOCHANNEL, pBuffer, dwLength);
//            lpInterfaceType = L"TurboChannel";
            break;
        case PCIBus:
            LoadStringW(hInst, IDS_BUS_PCIBUS, pBuffer, dwLength);
//            lpInterfaceType = L"PCIBus";
            break;
        case VMEBus:
            LoadStringW(hInst, IDS_BUS_VMEBUS, pBuffer, dwLength);
//            lpInterfaceType = L"VMEBus";
            break;
        case NuBus:
            LoadStringW(hInst, IDS_BUS_NUBUS, pBuffer, dwLength);
//            lpInterfaceType = L"NuBus";
            break;
        case PCMCIABus:
            LoadStringW(hInst, IDS_BUS_PCMCIABUS, pBuffer, dwLength);
//            lpInterfaceType = L"PCMCIABus";
            break;
        case CBus:
            LoadStringW(hInst, IDS_BUS_CBUS, pBuffer, dwLength);
//            lpInterfaceType = L"CBus";
            break;
        case MPIBus:
            LoadStringW(hInst, IDS_BUS_MPIBUS, pBuffer, dwLength);
//            lpInterfaceType = L"MPIBus";
            break;
        case MPSABus:
            LoadStringW(hInst, IDS_BUS_MPSABUS, pBuffer, dwLength);
//            lpInterfaceType = L"MPSABus";
            break;
        case ProcessorInternal:
            LoadStringW(hInst, IDS_BUS_PROCESSORINTERNAL, pBuffer, dwLength);
//            lpInterfaceType = L"ProcessorInternal";
            break;
        case InternalPowerBus:
            LoadStringW(hInst, IDS_BUS_INTERNALPOWERBUS, pBuffer, dwLength);
//            lpInterfaceType = L"InternalPowerBus";
            break;
        case PNPISABus:
            LoadStringW(hInst, IDS_BUS_PNPISABUS, pBuffer, dwLength);
//            lpInterfaceType = L"PNPISABus";
            break;
        case PNPBus:
            LoadStringW(hInst, IDS_BUS_PNPBUS, pBuffer, dwLength);
//            lpInterfaceType = L"PNPBus";
            break;
        default:
            LoadStringW(hInst, IDS_BUS_UNKNOWNTYPE, pBuffer, dwLength);
//            lpInterfaceType = L"Unknown interface type";
            break;
    }

//    wcscpy(pBuffer, lpInterfaceType);
}

static VOID
ParseResources(HWND hwnd)
{
    PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor;
    PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
    ULONG i;
    HWND hwndLV;

    WCHAR buffer[80];
    LVITEMW item;
    INT iItem;

    pFullDescriptor = &resourceValueData->List[0];
    for (i = 0; i < fullResourceIndex; i++)
    {
        pFullDescriptor = (PVOID)(pFullDescriptor->PartialResourceList.PartialDescriptors +
                                  pFullDescriptor->PartialResourceList.Count);
    }
    pPartialResourceList = &pFullDescriptor->PartialResourceList;

    /* Interface type */
    GetInterfaceType(pFullDescriptor->InterfaceType, buffer, 80);
    SetDlgItemTextW(hwnd, IDC_INTERFACETYPE, buffer);

    /* Busnumber */
    SetDlgItemInt(hwnd, IDC_BUSNUMBER, (UINT)pFullDescriptor->BusNumber, TRUE);

    /* Version */
    SetDlgItemInt(hwnd, IDC_VERSION, (UINT)pPartialResourceList->Version, FALSE);

    /* Revision */
    SetDlgItemInt(hwnd, IDC_REVISION, (UINT)pPartialResourceList->Revision, FALSE);

    for (i = 0; i < pPartialResourceList->Count; i++)
    {
        pDescriptor = &pPartialResourceList->PartialDescriptors[i];

        switch (pDescriptor->Type)
        {
            case CmResourceTypePort:
                hwndLV = GetDlgItem(hwnd, IDC_PORT_LIST);

#ifdef _M_AMD64
                wsprintf(buffer, L"0x%016I64x", pDescriptor->u.Port.Start.QuadPart);
#else
                wsprintf(buffer, L"0x%08lx", pDescriptor->u.Port.Start.u.LowPart);
#endif

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = 1000;
                item.iSubItem = 0;
                item.state = 0;
                item.stateMask = 0;
                item.pszText = buffer;
                item.cchTextMax = (int)wcslen(item.pszText);
                item.lParam = (LPARAM)pDescriptor;

                iItem = ListView_InsertItem(hwndLV, &item);
                if (iItem != -1)
                {
                    wsprintf(buffer, L"0x%lx", pDescriptor->u.Port.Length);
                    ListView_SetItemText(hwndLV, iItem, 1, buffer);

                    if (pDescriptor->Flags & CM_RESOURCE_PORT_IO)
                        LoadStringW(hInst, IDS_PORT_PORT_IO, buffer, ARRAY_SIZE(buffer));
                    else
                        LoadStringW(hInst, IDS_PORT_MEMORY_IO, buffer, ARRAY_SIZE(buffer));
                    ListView_SetItemText(hwndLV, iItem, 2, buffer);
                }
                break;

            case CmResourceTypeInterrupt:
                hwndLV = GetDlgItem(hwnd, IDC_IRQ_LIST);

                wsprintf(buffer, L"%lu", pDescriptor->u.Interrupt.Vector);

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = 1000;
                item.iSubItem = 0;
                item.state = 0;
                item.stateMask = 0;
                item.pszText = buffer;
                item.cchTextMax = (int)wcslen(item.pszText);
                item.lParam = (LPARAM)pDescriptor;

                iItem = ListView_InsertItem(hwndLV, &item);
                if (iItem != -1)
                {
                    wsprintf(buffer, L"%lu", pDescriptor->u.Interrupt.Level);
                    ListView_SetItemText(hwndLV, iItem, 1, buffer);

                    wsprintf(buffer, L"0x%08lx", pDescriptor->u.Interrupt.Affinity);
                    ListView_SetItemText(hwndLV, iItem, 2, buffer);

                    if (pDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
                        LoadStringW(hInst, IDS_INTERRUPT_EDGE_SENSITIVE, buffer, ARRAY_SIZE(buffer));
                    else
                        LoadStringW(hInst, IDS_INTERRUPT_LEVEL_SENSITIVE, buffer, ARRAY_SIZE(buffer));

                    ListView_SetItemText(hwndLV, iItem, 3, buffer);
                }
                break;

            case CmResourceTypeMemory:
                hwndLV = GetDlgItem(hwnd, IDC_MEMORY_LIST);

#ifdef _M_AMD64
                wsprintf(buffer, L"0x%016I64x", pDescriptor->u.Memory.Start.QuadPart);
#else
                wsprintf(buffer, L"0x%08lx", pDescriptor->u.Memory.Start.u.LowPart);
#endif

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = 1000;
                item.iSubItem = 0;
                item.state = 0;
                item.stateMask = 0;
                item.pszText = buffer;
                item.cchTextMax = (int)wcslen(item.pszText);
                item.lParam = (LPARAM)pDescriptor;

                iItem = ListView_InsertItem(hwndLV, &item);
                if (iItem != -1)
                {
                    wsprintf(buffer, L"0x%lx", pDescriptor->u.Memory.Length);
                    ListView_SetItemText(hwndLV, iItem, 1, buffer);

                    switch (pDescriptor->Flags & (CM_RESOURCE_MEMORY_READ_ONLY | CM_RESOURCE_MEMORY_WRITE_ONLY))
                    {
                        case CM_RESOURCE_MEMORY_READ_ONLY:
                            LoadStringW(hInst, IDS_MEMORY_READ_ONLY, buffer, ARRAY_SIZE(buffer));
                            break;

                        case CM_RESOURCE_MEMORY_WRITE_ONLY:
                            LoadStringW(hInst, IDS_MEMORY_WRITE_ONLY, buffer, ARRAY_SIZE(buffer));
                            break;

                        default:
                            LoadStringW(hInst, IDS_MEMORY_READ_WRITE, buffer, ARRAY_SIZE(buffer));
                            break;
                    }

                    ListView_SetItemText(hwndLV, iItem, 2, buffer);
                }
                break;

            case CmResourceTypeDma:
                hwndLV = GetDlgItem(hwnd, IDC_DMA_LIST);

                wsprintf(buffer, L"%lu", pDescriptor->u.Dma.Channel);

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = 1000;
                item.iSubItem = 0;
                item.state = 0;
                item.stateMask = 0;
                item.pszText = buffer;
                item.cchTextMax = (int)wcslen(item.pszText);
                item.lParam = (LPARAM)pDescriptor;

                iItem = ListView_InsertItem(hwndLV, &item);
                if (iItem != -1)
                {
                    wsprintf(buffer, L"%lu", pDescriptor->u.Dma.Port);
                    ListView_SetItemText(hwndLV, iItem, 1, buffer);
                }
                break;

            case CmResourceTypeDeviceSpecific:
                hwndLV = GetDlgItem(hwnd, IDC_DEVICE_LIST);

                wsprintf(buffer, L"0x%08lx", pDescriptor->u.DeviceSpecificData.Reserved1);

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = 1000;
                item.iSubItem = 0;
                item.state = 0;
                item.stateMask = 0;
                item.pszText = buffer;
                item.cchTextMax = (int)wcslen(item.pszText);
                item.lParam = (LPARAM)pDescriptor;

                iItem = ListView_InsertItem(hwndLV, &item);
                if (iItem != -1)
                {
                    wsprintf(buffer, L"0x%08lx", pDescriptor->u.DeviceSpecificData.Reserved2);
                    ListView_SetItemText(hwndLV, iItem, 1, buffer);

                    wsprintf(buffer, L"0x%lx", pDescriptor->u.DeviceSpecificData.DataSize);
                    ListView_SetItemText(hwndLV, iItem, 2, buffer);
                }
                break;
        }
    }
}

static BOOL
OnResourceNotify(HWND hwndDlg, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_PORT_LIST:
        case IDC_MEMORY_LIST:
        case IDC_DMA_LIST:
        case IDC_IRQ_LIST:
        case IDC_DEVICE_LIST:
            switch(phdr->code)
            {
                case NM_CLICK:
                    if (lpnmlv->iItem != -1)
                    {
                        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
                        LVITEMW item;

                        item.mask = LVIF_PARAM;
                        item.iItem = lpnmlv->iItem;
                        item.iSubItem = 0;

                        if (ListView_GetItem(phdr->hwndFrom, &item))
                        {
                            pDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)item.lParam;

                            EnableWindow(GetDlgItem(hwndDlg, IDC_UNDETERMINED),
                                         (pDescriptor->ShareDisposition == CmResourceShareUndetermined));

                            EnableWindow(GetDlgItem(hwndDlg, IDC_SHARED),
                                         (pDescriptor->ShareDisposition == CmResourceShareShared));

                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_EXCLUSIVE),
                                         (pDescriptor->ShareDisposition == CmResourceShareDeviceExclusive));

                            EnableWindow(GetDlgItem(hwndDlg, IDC_DRIVER_EXCLUSIVE),
                                         (pDescriptor->ShareDisposition == CmResourceShareDriverExclusive));
                        }
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_UNDETERMINED), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SHARED), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_EXCLUSIVE), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DRIVER_EXCLUSIVE), FALSE);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK modify_resource_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        CreateResourceColumns(hwndDlg);
        ParseResources(hwndDlg);
        return FALSE;

    case WM_NOTIFY:
        return OnResourceNotify(hwndDlg, (NMHDR *)lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL CreateResourceListColumns(HWND hWndListView)
{
    WCHAR szText[80];
    RECT rc;
    LVCOLUMN lvC;

    ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);

    GetClientRect(hWndListView, &rc);

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;
    lvC.fmt = LVCFMT_LEFT;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = (rc.right - rc.left) / 2;
    LoadStringW(hInst, IDS_BUSNUMBER, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    lvC.cx = (rc.right - rc.left) - lvC.cx;
    LoadStringW(hInst, IDS_INTERFACE, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 1, &lvC) == -1)
        return FALSE;

    return TRUE;
}

static VOID AddFullResourcesToList(HWND hwnd)
{
    PCM_FULL_RESOURCE_DESCRIPTOR pFullDescriptor;
    WCHAR buffer[80];
    LVITEMW item;
    ULONG i;
    INT iItem;

    pFullDescriptor = &resourceValueData->List[0];
    for (i = 0; i < resourceValueData->Count; i++)
    {
        wsprintf(buffer, L"%lu", pFullDescriptor->BusNumber);

        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 0;
        item.state = 0;
        item.stateMask = 0;
        item.pszText = buffer;
        item.cchTextMax = (int)wcslen(item.pszText);

        iItem = ListView_InsertItem(hwnd, &item);
        if (iItem != -1)
        {
            GetInterfaceType(pFullDescriptor->InterfaceType, buffer, 80);
            ListView_SetItemText(hwnd, iItem, 1, buffer);
        }
        pFullDescriptor = (PVOID)(pFullDescriptor->PartialResourceList.PartialDescriptors +
                                  pFullDescriptor->PartialResourceList.Count);
    }
}

static BOOL
OnResourceListNotify(HWND hwndDlg, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_RESOURCE_LIST:
            switch(phdr->code)
            {
                case NM_CLICK:
                    fullResourceIndex = lpnmlv->iItem;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SHOW_RESOURCE), (lpnmlv->iItem != -1));
                    break;

                case NM_DBLCLK:
                    if (lpnmlv->iItem != -1)
                    {
                        fullResourceIndex = lpnmlv->iItem;
                        DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_RESOURCE), hwndDlg, modify_resource_dlgproc);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK modify_resource_list_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        CreateResourceListColumns(GetDlgItem(hwndDlg, IDC_RESOURCE_LIST));
        AddFullResourcesToList(GetDlgItem(hwndDlg, IDC_RESOURCE_LIST));
        return FALSE;

    case WM_NOTIFY:
        return OnResourceListNotify(hwndDlg, (NMHDR *)lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SHOW_RESOURCE:
            if (fullResourceIndex != -1)
                DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_RESOURCE), hwndDlg, modify_resource_dlgproc);
            break;
        case IDOK:
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
CreateRequirementsListColumns(HWND hWndListView)
{
    WCHAR szText[80];
    RECT rc;
    LVCOLUMN lvC;

    ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);

    GetClientRect(hWndListView, &rc);

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;
    lvC.fmt = LVCFMT_LEFT;

    /* Load the column labels from the resource file. */
    lvC.iSubItem = 0;
    lvC.cx = (rc.right - rc.left) / 4;
    LoadStringW(hInst, IDS_REQALTERNATIVELIST, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 0, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 1;
    lvC.cx = (rc.right - rc.left) / 4;
    LoadStringW(hInst, IDS_REQRESOURCELIST, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 1, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 2;
    lvC.cx = (rc.right - rc.left) / 4;
    LoadStringW(hInst, IDS_REQDESCRIPTOR, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 2, &lvC) == -1)
        return FALSE;

    lvC.iSubItem = 3;
    lvC.cx = (rc.right - rc.left) - (3 * ((rc.right - rc.left) / 4));
    LoadStringW(hInst, IDS_REQDEVICETYPE, szText, ARRAY_SIZE(szText));
    if (ListView_InsertColumn(hWndListView, 3, &lvC) == -1)
        return FALSE;

    return TRUE;
}

static VOID
GetResourceType(UCHAR ResourceType,
                LPWSTR pBuffer,
                DWORD dwLength)
{
    switch (ResourceType)
    {
        case CmResourceTypePort:
            LoadStringW(hInst, IDS_TYPE_PORT, pBuffer, dwLength);
            break;

        case CmResourceTypeInterrupt:
            LoadStringW(hInst, IDS_TYPE_INTERRUPT, pBuffer, dwLength);
            break;

        case CmResourceTypeMemory:
            LoadStringW(hInst, IDS_TYPE_MEMORY, pBuffer, dwLength);
            break;

        case CmResourceTypeDma:
            LoadStringW(hInst, IDS_TYPE_DMA, pBuffer, dwLength);
            break;

        default:
            wsprintf(pBuffer, L"Unknown %u", ResourceType);
            break;
    }
}

static VOID
GetShareDisposition(
    UCHAR ShareDisposition,
    LPWSTR pBuffer,
    DWORD dwLength)
{
    switch (ShareDisposition)
    {
        case CmResourceShareUndetermined:
            LoadStringW(hInst, IDS_SHARE_UNDETERMINED, pBuffer, dwLength);
            break;

        case CmResourceShareDeviceExclusive:
            LoadStringW(hInst, IDS_SHARE_DEVICE_EXCLUSIVE, pBuffer, dwLength);
            break;

        case CmResourceShareDriverExclusive:
            LoadStringW(hInst, IDS_SHARE_DRIVER_EXCLUSIVE, pBuffer, dwLength);
            break;

        case CmResourceShareShared:
            LoadStringW(hInst, IDS_SHARE_SHARED, pBuffer, dwLength);
            break;
    }
}

static VOID
GetPortType(
    USHORT Flags,
    LPWSTR pBuffer,
    DWORD dwLength)
{
    if ((Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO)
    {
        LoadStringW(hInst, IDS_PORT_PORT_IO, pBuffer, dwLength);
    }
    else if ((Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_MEMORY)
    {
        LoadStringW(hInst, IDS_PORT_MEMORY_IO, pBuffer, dwLength);
    }
}

static VOID
GetMemoryAccess(
    USHORT Flags,
    LPWSTR pBuffer,
    DWORD dwLength)
{
    if ((Flags & (CM_RESOURCE_MEMORY_READ_ONLY | CM_RESOURCE_MEMORY_WRITE_ONLY)) == CM_RESOURCE_MEMORY_READ_WRITE)
    {
        LoadStringW(hInst, IDS_MEMORY_READ_WRITE, pBuffer, dwLength);
    }
    else if ((Flags & (CM_RESOURCE_MEMORY_READ_ONLY | CM_RESOURCE_MEMORY_WRITE_ONLY)) == CM_RESOURCE_MEMORY_READ_ONLY)
    {
        LoadStringW(hInst, IDS_MEMORY_READ_ONLY, pBuffer, dwLength);
    }
    else if ((Flags & (CM_RESOURCE_MEMORY_READ_ONLY | CM_RESOURCE_MEMORY_WRITE_ONLY)) == CM_RESOURCE_MEMORY_WRITE_ONLY)
    {
        LoadStringW(hInst, IDS_MEMORY_WRITE_ONLY, pBuffer, dwLength);
    }
}

static VOID
GetInterruptType(
    USHORT Flags,
    LPWSTR pBuffer,
    DWORD dwLength)
{
    if ((Flags & CM_RESOURCE_INTERRUPT_LEVEL_LATCHED_BITS) == CM_RESOURCE_INTERRUPT_LATCHED)
    {
        LoadStringW(hInst, IDS_INTERRUPT_EDGE_SENSITIVE, pBuffer, dwLength);
    }
    else
    {
        LoadStringW(hInst, IDS_INTERRUPT_LEVEL_SENSITIVE, pBuffer, dwLength);
    }
}

static VOID
AddRequirementsToList(HWND hwndDlg, HWND hwnd)
{
    PIO_RESOURCE_LIST pResourceList;
    PIO_RESOURCE_DESCRIPTOR pDescriptor; 
    WCHAR buffer[80];
    LVITEMW item;
    ULONG i, j, index;
    INT iItem;

    index = 0;
    pResourceList = &requirementsValueData->List[0];
    for (i = 0; i < requirementsValueData->AlternativeLists; i++)
    {
        for (j = 0; j < pResourceList->Count; j++)
        {
            pDescriptor = &pResourceList->Descriptors[j];

            wsprintf(buffer, L"%lu", i + 1);

            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = index;
            item.iSubItem = 0;
            item.state = 0;
            item.stateMask = 0;
            item.pszText = buffer;
            item.cchTextMax = (int)wcslen(item.pszText);
            item.lParam = (LPARAM)pDescriptor;

            iItem = ListView_InsertItem(hwnd, &item);
            if (iItem != -1)
            {
                wsprintf(buffer, L"%lu", j + 1);
                ListView_SetItemText(hwnd, iItem, 1, buffer);
                wsprintf(buffer, L"%lu", 1);
                ListView_SetItemText(hwnd, iItem, 2, buffer);

                GetResourceType(pDescriptor->Type, buffer, 80);
                ListView_SetItemText(hwnd, iItem, 3, buffer);
            }

            index++;
        }


        pResourceList = (PIO_RESOURCE_LIST)(pResourceList->Descriptors + pResourceList->Count);
    }

    GetInterfaceType(requirementsValueData->InterfaceType, buffer, 80);
    SetDlgItemTextW(hwndDlg, IDC_REQINTERFACETYPE, buffer);
    SetDlgItemInt(hwndDlg, IDC_REQBUSNUMBER, (UINT)requirementsValueData->BusNumber, TRUE);
    SetDlgItemInt(hwndDlg, IDC_REQSLOTNUMBER, (UINT)requirementsValueData->SlotNumber, FALSE);
}

static INT_PTR CALLBACK show_requirements_port_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PIO_RESOURCE_DESCRIPTOR pDescriptor;
    WCHAR Buffer[80];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pDescriptor = (PIO_RESOURCE_DESCRIPTOR)lParam;

        GetPortType(pDescriptor->Flags, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_TYPE, Buffer);

        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Port.Length);
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_LENGTH, Buffer);
        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Port.Alignment);
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_ALIGN, Buffer);
#ifdef _M_AMD64
        wsprintf(Buffer, L"0x%016I64x", pDescriptor->u.Port.MinimumAddress.QuadPart);
#else
        wsprintf(Buffer, L"0x%08lx", pDescriptor->u.Port.MinimumAddress.u.LowPart);
#endif
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_MIN, Buffer);
#ifdef _M_AMD64
        wsprintf(Buffer, L"0x%016I64x", pDescriptor->u.Port.MaximumAddress.QuadPart);
#else
        wsprintf(Buffer, L"0x%08lx", pDescriptor->u.Port.MaximumAddress.u.LowPart);
#endif
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_MAX, Buffer);

        GetShareDisposition(pDescriptor->ShareDisposition, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_PORT_SHARE, Buffer);

        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_PORT_ALTERNATIVE), (pDescriptor->Option & IO_RESOURCE_ALTERNATIVE));
        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_PORT_PREFERRED), (pDescriptor->Option & IO_RESOURCE_PREFERRED));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            break;
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK show_requirements_memory_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PIO_RESOURCE_DESCRIPTOR pDescriptor;
    WCHAR Buffer[80];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pDescriptor = (PIO_RESOURCE_DESCRIPTOR)lParam;

        GetMemoryAccess(pDescriptor->Flags, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_ACCESS, Buffer);

        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Memory.Length);
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_LENGTH, Buffer);
        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Memory.Alignment);
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_ALIGN, Buffer);
#ifdef _M_AMD64
        wsprintf(Buffer, L"0x%016I64x", pDescriptor->u.Memory.MinimumAddress.QuadPart);
#else
        wsprintf(Buffer, L"0x%08lx", pDescriptor->u.Memory.MinimumAddress.u.LowPart);
#endif
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_MIN, Buffer);
#ifdef _M_AMD64
        wsprintf(Buffer, L"0x%016I64x", pDescriptor->u.Memory.MaximumAddress.QuadPart);
#else
        wsprintf(Buffer, L"0x%08lx", pDescriptor->u.Memory.MaximumAddress.u.LowPart);
#endif
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_MAX, Buffer);

        GetShareDisposition(pDescriptor->ShareDisposition, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_MEMORY_SHARE, Buffer);

        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_MEMORY_ALTERNATIVE), (pDescriptor->Option & IO_RESOURCE_ALTERNATIVE));
        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_MEMORY_PREFERRED), (pDescriptor->Option & IO_RESOURCE_PREFERRED));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            break;
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK show_requirements_interrupt_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PIO_RESOURCE_DESCRIPTOR pDescriptor;
    WCHAR Buffer[80];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pDescriptor = (PIO_RESOURCE_DESCRIPTOR)lParam;

        GetInterruptType(pDescriptor->Flags, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_INT_TYPE, Buffer);

        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Interrupt.MinimumVector);
        SetDlgItemTextW(hwndDlg, IDC_REQ_INT_MIN, Buffer);
        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Interrupt.MaximumVector);
        SetDlgItemTextW(hwndDlg, IDC_REQ_INT_MAX, Buffer);

        GetShareDisposition(pDescriptor->ShareDisposition, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_INT_SHARE, Buffer);

        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_INT_ALTERNATIVE), (pDescriptor->Option & IO_RESOURCE_ALTERNATIVE));
        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_INT_PREFERRED), (pDescriptor->Option & IO_RESOURCE_PREFERRED));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            break;
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK show_requirements_dma_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PIO_RESOURCE_DESCRIPTOR pDescriptor;
    WCHAR Buffer[80];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pDescriptor = (PIO_RESOURCE_DESCRIPTOR)lParam;
        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Dma.MinimumChannel);
        SetDlgItemTextW(hwndDlg, IDC_REQ_DMA_MIN, Buffer);
        wsprintf(Buffer, L"0x%lx", pDescriptor->u.Dma.MaximumChannel);
        SetDlgItemTextW(hwndDlg, IDC_REQ_DMA_MAX, Buffer);

        GetShareDisposition(pDescriptor->ShareDisposition, Buffer, 80);
        SetDlgItemTextW(hwndDlg, IDC_REQ_DMA_SHARE, Buffer);

        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_DMA_ALTERNATIVE), (pDescriptor->Option & IO_RESOURCE_ALTERNATIVE));
        EnableWindow(GetDlgItem(hwndDlg, IDC_REQ_DMA_PREFERRED), (pDescriptor->Option & IO_RESOURCE_PREFERRED));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            break;
        }
    }
    return FALSE;
}

static VOID
ShowRequirement(HWND hwndDlg)
{
    PIO_RESOURCE_DESCRIPTOR pDescriptor; 
    LVITEMW item;

    if (requirementsIndex == -1)
        return;

    item.mask = LVIF_PARAM;
    item.iItem = requirementsIndex;
    item.iSubItem = 0;
    ListView_GetItem(GetDlgItem(hwndDlg, IDC_REQUIREMENTS_LIST), &item);

    pDescriptor = (PIO_RESOURCE_DESCRIPTOR)item.lParam; 
    if (pDescriptor)
    {
        switch (pDescriptor->Type)
        {
        case CmResourceTypePort:
            DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_EDIT_REQUIREMENTS_PORT), hwndDlg, show_requirements_port_dlgproc, (LPARAM)pDescriptor);
            break;
        case CmResourceTypeMemory:
            DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_EDIT_REQUIREMENTS_MEMORY), hwndDlg, show_requirements_memory_dlgproc, (LPARAM)pDescriptor);
            break;
        case CmResourceTypeInterrupt:
            DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_EDIT_REQUIREMENTS_INT), hwndDlg, show_requirements_interrupt_dlgproc, (LPARAM)pDescriptor);
            break;
        case CmResourceTypeDma:
            DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_EDIT_REQUIREMENTS_DMA), hwndDlg, show_requirements_dma_dlgproc, (LPARAM)pDescriptor);
            break;
        default:
            break;
        }
    }
}

static BOOL
OnRequirementsListNotify(HWND hwndDlg, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_REQUIREMENTS_LIST:
            switch(phdr->code)
            {
                case NM_CLICK:
                    requirementsIndex = lpnmlv->iItem;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SHOW_REQUIREMENT), (lpnmlv->iItem != -1));
                    break;

                case NM_DBLCLK:
                    if (lpnmlv->iItem != -1)
                    {
                        requirementsIndex = lpnmlv->iItem;
                        ShowRequirement(hwndDlg);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK modify_requirements_list_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        CreateRequirementsListColumns(GetDlgItem(hwndDlg, IDC_REQUIREMENTS_LIST));
        AddRequirementsToList(hwndDlg, GetDlgItem(hwndDlg, IDC_REQUIREMENTS_LIST));
        return FALSE;

    case WM_NOTIFY:
        return OnRequirementsListNotify(hwndDlg, (NMHDR *)lParam);

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SHOW_REQUIREMENT:
            if (requirementsIndex != -1)
                ShowRequirement(hwndDlg);
            break;
        case IDOK:
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCWSTR valueName, BOOL EditBin)
{
    DWORD type;
    LONG lRet;
    BOOL result = FALSE;

    if (!hKey)
        return FALSE;

    editValueName = valueName;

    lRet = RegQueryValueExW(hKey, valueName, 0, &type, 0, &valueDataLen);
    if (lRet != ERROR_SUCCESS && (valueName == NULL || !valueName[0]))
    {
        lRet = ERROR_SUCCESS; /* Allow editing of (Default) values which don't exist */
        type = REG_SZ;
        valueDataLen = 0;
        stringValueData = NULL;
        binValueData = NULL;
    }

    if (lRet != ERROR_SUCCESS)
    {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }

    if (EditBin == FALSE && ((type == REG_SZ) || (type == REG_EXPAND_SZ)))
    {
        if (valueDataLen > 0)
        {
            if (!(stringValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen)))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }
            lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)stringValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }
        }
        else
        {
            stringValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_STRING), hwnd, modify_string_dlgproc) == IDOK)
        {
            if (stringValueData)
            {
                lRet = RegSetValueExW(hKey, valueName, 0, type, (LPBYTE)stringValueData, (DWORD)(wcslen(stringValueData) + 1) * sizeof(WCHAR));
            }
            else
            {
                lRet = RegSetValueExW(hKey, valueName, 0, type, NULL, 0);
            }
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == FALSE && type == REG_MULTI_SZ)
    {
        if (valueDataLen > 0)
        {
            size_t llen, listlen, nl_len;
            LPWSTR src, lines = NULL;

            if (!(stringValueData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, valueDataLen + sizeof(WCHAR))))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }
            lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)stringValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }

            /* convert \0 to \r\n */
            src = stringValueData;
            nl_len = wcslen(L"\r\n") * sizeof(WCHAR);
            listlen = sizeof(WCHAR);
            lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, listlen + sizeof(WCHAR));
            while(*src != L'\0')
            {
                llen = wcslen(src);
                if(llen == 0)
                    break;
                listlen += (llen * sizeof(WCHAR)) + nl_len;
                lines = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lines, listlen);
                wcscat(lines, src);
                wcscat(lines, L"\r\n");
                src += llen + 1;
            }
            HeapFree(GetProcessHeap(), 0, stringValueData);
            stringValueData = lines;
        }
        else
        {
            stringValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_MULTI_STRING), hwnd, modify_multi_string_dlgproc) == IDOK)
        {
            if (stringValueData)
            {
                /* convert \r\n to \0 */
                BOOL EmptyLines = FALSE;
                LPWSTR src, lines, nl;
                size_t linechars, buflen, c_nl, dest;

                src = stringValueData;
                buflen = sizeof(WCHAR);
                lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen + sizeof(WCHAR));
                c_nl = wcslen(L"\r\n");
                dest = 0;
                while(*src != L'\0')
                {
                    if((nl = wcsstr(src, L"\r\n")))
                    {
                        linechars = nl - src;
                        if(nl == src)
                        {
                            EmptyLines = TRUE;
                            src = nl + c_nl;
                            continue;
                        }
                    }
                    else
                    {
                        linechars = wcslen(src);
                    }
                    if(linechars > 0)
                    {
                        buflen += ((linechars + 1) * sizeof(WCHAR));
                        lines = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lines, buflen);
                        memcpy((lines + dest), src, linechars * sizeof(WCHAR));
                        dest += linechars;
                        lines[dest++] = L'\0';
                    }
                    else
                    {
                        EmptyLines = TRUE;
                    }
                    src += linechars + (nl != NULL ? c_nl : 0);
                }
                lines[++dest] = L'\0';

                if(EmptyLines)
                {
                    warning(hwnd, IDS_MULTI_SZ_EMPTY_STRING);
                }

                lRet = RegSetValueExW(hKey, valueName, 0, type, (LPBYTE)lines, (DWORD)buflen);
                HeapFree(GetProcessHeap(), 0, lines);
            }
            else
            {
                lRet = RegSetValueExW(hKey, valueName, 0, type, NULL, 0);
            }
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == FALSE && type == REG_DWORD)
    {
        lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)&dwordValueData, &valueDataLen);
        if (lRet != ERROR_SUCCESS)
        {
            error(hwnd, IDS_BAD_VALUE, valueName);
            goto done;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_DWORD), hwnd, modify_dword_dlgproc) == IDOK)
        {
            lRet = RegSetValueExW(hKey, valueName, 0, type, (LPBYTE)&dwordValueData, sizeof(DWORD));
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == FALSE && type == REG_RESOURCE_LIST)
    {
        if (valueDataLen > 0)
        {
            resourceValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen);
            if (resourceValueData == NULL)
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }

            lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)resourceValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }
        }
        else
        {
            resourceValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_RESOURCE_LIST), hwnd, modify_resource_list_dlgproc) == IDOK)
        {
        }
    }
    else if (EditBin == FALSE && type == REG_FULL_RESOURCE_DESCRIPTOR)
    {
        if (valueDataLen > 0)
        {
            resourceValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen + sizeof(ULONG));
            if (resourceValueData == NULL)
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }

            lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)&resourceValueData->List[0], &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }

            resourceValueData->Count = 1;
            fullResourceIndex = 0;
        }
        else
        {
            resourceValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_RESOURCE), hwnd, modify_resource_dlgproc) == IDOK)
        {
        }
    }
    else if (EditBin == FALSE && type == REG_RESOURCE_REQUIREMENTS_LIST)
    {
        if (valueDataLen > 0)
        {
            requirementsValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen + sizeof(ULONG));
            if (requirementsValueData == NULL)
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }

            lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)requirementsValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }

        }
        else
        {
            requirementsValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_REQUIREMENTS_LIST), hwnd, modify_requirements_list_dlgproc) == IDOK)
        {
        }
    }
    else if ((EditBin != FALSE) || (type == REG_NONE) || (type == REG_BINARY))
    {
        if(valueDataLen > 0)
        {
            if(!(binValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen + 1)))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }

            /* Use the unicode version, so editing strings in binary mode is correct */
            lRet = RegQueryValueExW(hKey, valueName,
                                    0, 0, (LPBYTE)binValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, binValueData);
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }
        }
        else
        {
            binValueData = NULL;
        }

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_BIN_DATA), hwnd, modify_binary_dlgproc) == IDOK)
        {
            /* Use the unicode version, so editing strings in binary mode is correct */
            lRet = RegSetValueExW(hKey, valueName,
                                  0, type, (LPBYTE)binValueData, valueDataLen);
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
        if(binValueData != NULL)
            HeapFree(GetProcessHeap(), 0, binValueData);
    }
    else
    {
        error(hwnd, IDS_UNSUPPORTED_TYPE, type);
    }

done:
    if (resourceValueData)
        HeapFree(GetProcessHeap(), 0, resourceValueData);
    resourceValueData = NULL;

    if (stringValueData)
        HeapFree(GetProcessHeap(), 0, stringValueData);
    stringValueData = NULL;

    if (requirementsValueData)
        HeapFree(GetProcessHeap(), 0, requirementsValueData);
    requirementsValueData = NULL;

    return result;
}

static LONG CopyKey(HKEY hDestKey, LPCWSTR lpDestSubKey, HKEY hSrcKey, LPCWSTR lpSrcSubKey)
{
    LONG lResult;
    DWORD dwDisposition;
    HKEY hDestSubKey = NULL;
    HKEY hSrcSubKey = NULL;
    DWORD dwIndex, dwType, cbName, cbData;
    WCHAR szSubKey[256];
    WCHAR szValueName[256];
    BYTE szValueData[512];

    FILETIME ft;

    /* open the source subkey, if specified */
    if (lpSrcSubKey)
    {
        lResult = RegOpenKeyExW(hSrcKey, lpSrcSubKey, 0, KEY_ALL_ACCESS, &hSrcSubKey);
        if (lResult)
            goto done;
        hSrcKey = hSrcSubKey;
    }

    /* create the destination subkey */
    lResult = RegCreateKeyExW(hDestKey, lpDestSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                             &hDestSubKey, &dwDisposition);
    if (lResult)
        goto done;

    /* copy all subkeys */
    dwIndex = 0;
    do
    {
        cbName = ARRAY_SIZE(szSubKey);
        lResult = RegEnumKeyExW(hSrcKey, dwIndex++, szSubKey, &cbName, NULL, NULL, NULL, &ft);
        if (lResult == ERROR_SUCCESS)
        {
            lResult = CopyKey(hDestSubKey, szSubKey, hSrcKey, szSubKey);
            if (lResult)
                goto done;
        }
    }
    while(lResult == ERROR_SUCCESS);

    /* copy all subvalues */
    dwIndex = 0;
    do
    {
        cbName = ARRAY_SIZE(szValueName);
        cbData = ARRAY_SIZE(szValueData);
        lResult = RegEnumValueW(hSrcKey, dwIndex++, szValueName, &cbName, NULL, &dwType, szValueData, &cbData);
        if (lResult == ERROR_SUCCESS)
        {
            lResult = RegSetValueExW(hDestSubKey, szValueName, 0, dwType, szValueData, cbData);
            if (lResult)
                goto done;
        }
    }
    while(lResult == ERROR_SUCCESS);

    lResult = ERROR_SUCCESS;

done:
    if (hSrcSubKey)
        RegCloseKey(hSrcSubKey);
    if (hDestSubKey)
        RegCloseKey(hDestSubKey);
    if (lResult != ERROR_SUCCESS)
        SHDeleteKey(hDestKey, lpDestSubKey);
    return lResult;
}

static LONG MoveKey(HKEY hDestKey, LPCWSTR lpDestSubKey, HKEY hSrcKey, LPCWSTR lpSrcSubKey)
{
    LONG lResult;

    if (!lpSrcSubKey)
        return ERROR_INVALID_FUNCTION;

    if (_wcsicmp(lpDestSubKey, lpSrcSubKey) == 0)
    {
        /* Destination name equals source name */
        return ERROR_SUCCESS;
    }

    lResult = CopyKey(hDestKey, lpDestSubKey, hSrcKey, lpSrcSubKey);
    if (lResult == ERROR_SUCCESS)
        SHDeleteKey(hSrcKey, lpSrcSubKey);

    return lResult;
}

BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath)
{
    WCHAR msg[128], caption[128];
    BOOL result = FALSE;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ|KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        error_code_messagebox(hwnd, lRet);
        return FALSE;
    }

    LoadStringW(hInst, IDS_QUERY_DELETE_KEY_CONFIRM, caption, ARRAY_SIZE(caption));
    LoadStringW(hInst, IDS_QUERY_DELETE_KEY_ONE, msg, ARRAY_SIZE(msg));

    if (MessageBoxW(g_pChildWnd->hWnd, msg, caption, MB_ICONQUESTION | MB_YESNO) != IDYES)
        goto done;

    lRet = SHDeleteKey(hKeyRoot, keyPath);
    if (lRet != ERROR_SUCCESS)
    {
        error(hwnd, IDS_BAD_KEY, keyPath);
        goto done;
    }
    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}

LONG RenameKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpNewName)
{
    LPCWSTR s;
    LPWSTR lpNewSubKey = NULL;
    LONG Ret = 0;
    SIZE_T cbNewSubKey;

    if (!lpSubKey)
        return Ret;

    s = wcsrchr(lpSubKey, L'\\');
    if (s)
    {
        s++;
        cbNewSubKey = (s - lpSubKey + wcslen(lpNewName) + 1) * sizeof(WCHAR);
        lpNewSubKey = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cbNewSubKey);
        if (lpNewSubKey != NULL)
        {
            StringCbCopyNW(lpNewSubKey, cbNewSubKey, lpSubKey, (s - lpSubKey) * sizeof(WCHAR));
            StringCbCatW(lpNewSubKey, cbNewSubKey, lpNewName);
            lpNewName = lpNewSubKey;
        }
        else
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = MoveKey(hKey, lpNewName, hKey, lpSubKey);

    if (lpNewSubKey)
    {
        HeapFree(GetProcessHeap(), 0, lpNewSubKey);
    }
    return Ret;
}

LONG RenameValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpDestValue, LPCWSTR lpSrcValue)
{
    LONG lResult;
    HKEY hSubKey = NULL;
    DWORD dwType, cbData;
    BYTE data[512];

    if (lpSubKey)
    {
        lResult = RegOpenKeyW(hKey, lpSubKey, &hSubKey);
        if (lResult != ERROR_SUCCESS)
            goto done;
        hKey = hSubKey;
    }

    cbData = sizeof(data);
    lResult = RegQueryValueExW(hKey, lpSrcValue, NULL, &dwType, data, &cbData);
    if (lResult != ERROR_SUCCESS)
        goto done;

    lResult = RegSetValueExW(hKey, lpDestValue, 0, dwType, data, cbData);
    if (lResult != ERROR_SUCCESS)
        goto done;

    RegDeleteValue(hKey, lpSrcValue);

done:
    if (hSubKey)
        RegCloseKey(hSubKey);
    return lResult;
}

LONG QueryStringValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR pszBuffer, DWORD dwBufferLen)
{
    LONG lResult;
    HKEY hSubKey = NULL;
    DWORD cbData, dwType;

    if (lpSubKey)
    {
        lResult = RegOpenKeyW(hKey, lpSubKey, &hSubKey);
        if (lResult != ERROR_SUCCESS)
            goto done;
        hKey = hSubKey;
    }

    cbData = (dwBufferLen - 1) * sizeof(*pszBuffer);
    lResult = RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE)pszBuffer, &cbData);
    if (lResult != ERROR_SUCCESS)
        goto done;
    if (dwType != REG_SZ)
    {
        lResult = -1;
        goto done;
    }

    pszBuffer[cbData / sizeof(*pszBuffer)] = L'\0';

done:
    if (lResult != ERROR_SUCCESS)
        pszBuffer[0] = L'\0';
    if (hSubKey)
        RegCloseKey(hSubKey);
    return lResult;
}

BOOL GetKeyName(LPWSTR pszDest, size_t iDestLength, HKEY hRootKey, LPCWSTR lpSubKey)
{
    LPCWSTR pszRootKey;

    if (hRootKey == HKEY_CLASSES_ROOT)
        pszRootKey = L"HKEY_CLASSES_ROOT";
    else if (hRootKey == HKEY_CURRENT_USER)
        pszRootKey = L"HKEY_CURRENT_USER";
    else if (hRootKey == HKEY_LOCAL_MACHINE)
        pszRootKey = L"HKEY_LOCAL_MACHINE";
    else if (hRootKey == HKEY_USERS)
        pszRootKey = L"HKEY_USERS";
    else if (hRootKey == HKEY_CURRENT_CONFIG)
        pszRootKey = L"HKEY_CURRENT_CONFIG";
    else if (hRootKey == HKEY_DYN_DATA)
        pszRootKey = L"HKEY_DYN_DATA";
    else
        return FALSE;

    if (lpSubKey[0])
        _snwprintf(pszDest, iDestLength, L"%s\\%s", pszRootKey, lpSubKey);
    else
        _snwprintf(pszDest, iDestLength, L"%s", pszRootKey);
    return TRUE;
}
