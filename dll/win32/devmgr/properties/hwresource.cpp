/*
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            dll/win32/devmgr/hwresource.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Johannes Anderwald <johannes.anderwald@reactos.org>
 * UPDATE HISTORY:
 *      2005/11/24  Created
 */

#include "precomp.h"
#include "restypes.h"
#include "properties.h"

#include "resource.h"


typedef struct
{
    HWND hWnd;
    HWND hWndDevList;


}HARDWARE_RESOURCE_DATA, *PHARDWARE_RESOURCE_DATA;


#define CX_TYPECOLUMN_WIDTH 120

static VOID
InitializeDevicesList(
    IN HWND hWndDevList)
{
    LVCOLUMN lvc;
    RECT rcClient;
    WCHAR szColName[255];
    int iCol = 0;

    /* set the list view style */
    (void)ListView_SetExtendedListViewStyle(hWndDevList,
                                            LVS_EX_FULLROWSELECT);

    GetClientRect(hWndDevList,
                  &rcClient);

    /* add the list view columns */
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = szColName;

    if (LoadString(hDllInstance,
                   IDS_RESOURCE_COLUMN,
                   szColName,
                   sizeof(szColName) / sizeof(szColName[0])))
    {
        lvc.cx = CX_TYPECOLUMN_WIDTH;
        (void)ListView_InsertColumn(hWndDevList,
                                    iCol++,
                                    &lvc);
    }
    if (LoadString(hDllInstance,
                   IDS_SETTING_COLUMN,
                   szColName,
                   sizeof(szColName) / sizeof(szColName[0])))
    {
        lvc.cx = rcClient.right - CX_TYPECOLUMN_WIDTH -
                 GetSystemMetrics(SM_CXVSCROLL);

        (void)ListView_InsertColumn(hWndDevList,
                                    iCol++,
                                    &lvc);
    }
}

VOID
InsertListItem(
    IN HWND hWndDevList,
    IN INT ItemCount,
    IN LPWSTR ResourceType,
    IN LPWSTR ResourceDescription)
{
    INT iItem;
    LVITEM li = {0};

    li.mask = LVIF_STATE | LVIF_TEXT;
    li.iItem = ItemCount;
    li.pszText = ResourceType;
    //li.iImage = ClassDevInfo->ImageIndex;
    iItem = ListView_InsertItem(hWndDevList, &li);

    if (iItem != -1)
    {
        li.mask = LVIF_TEXT;
        li.iItem = iItem;
        li.iSubItem = 1;
        li.pszText = ResourceDescription;
        (void)ListView_SetItem(hWndDevList, &li);
    }
}

VOID
AddResourceItems(
    IN PDEVADVPROP_INFO dap,
    IN HWND hWndDevList)
{
    WCHAR szBuffer[100];
    WCHAR szDetail[100];
    PCM_RESOURCE_LIST ResourceList;
    ULONG ItemCount = 0, Index;

    ResourceList = (PCM_RESOURCE_LIST)dap->pResourceList;

    for (Index = 0; Index < ResourceList->List[0].PartialResourceList.Count; Index++)
    {
         PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
         if (Descriptor->Type == CmResourceTypeInterrupt)
         {
             if (LoadString(hDllInstance, IDS_RESOURCE_INTERRUPT, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
             {
                 wsprintf(szDetail, L"0x%08x (%d)", Descriptor->u.Interrupt.Level, Descriptor->u.Interrupt.Vector);
                 InsertListItem(hWndDevList, ItemCount, szBuffer, szDetail);
                 ItemCount++;
             }
         }
         else if (Descriptor->Type == CmResourceTypePort)
         {
             if (LoadString(hDllInstance, IDS_RESOURCE_PORT, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
             {
                 wsprintf(szDetail, L"%08lx - %08lx", Descriptor->u.Port.Start.LowPart, Descriptor->u.Port.Start.LowPart + Descriptor->u.Port.Length - 1);
                 InsertListItem(hWndDevList, ItemCount, szBuffer, szDetail);
                 ItemCount++;
             }
         }
         else if (Descriptor->Type == CmResourceTypeMemory)
         {
             if (LoadString(hDllInstance, IDS_RESOURCE_MEMORY_RANGE, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
             {
                 wsprintf(szDetail, L"%08I64x - %08I64x", Descriptor->u.Memory.Start.QuadPart, Descriptor->u.Memory.Start.QuadPart + Descriptor->u.Memory.Length - 1);
                 InsertListItem(hWndDevList, ItemCount, szBuffer, szDetail);
                 ItemCount++;
             }
         }
         else if (Descriptor->Type == CmResourceTypeDma)
         {
             if (LoadString(hDllInstance, IDS_RESOURCE_DMA, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
             {
                 wsprintf(szDetail, L"%08ld", Descriptor->u.Dma.Channel);
                 InsertListItem(hWndDevList, ItemCount, szBuffer, szDetail);
                 ItemCount++;
             }
         }
    }
}


static VOID
UpdateDriverResourceDlg(IN HWND hwndDlg,
                        IN PDEVADVPROP_INFO dap)
{
    /* set the device image */
    SendDlgItemMessage(hwndDlg,
                       IDC_DEVICON,
                       STM_SETICON,
                       (WPARAM)dap->hDevIcon,
                       0);

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);
}

INT_PTR
CALLBACK
ResourcesProcDriverDlgProc(IN HWND hwndDlg,
                     IN UINT uMsg,
                     IN WPARAM wParam,
                     IN LPARAM lParam)
{
    PDEVADVPROP_INFO hpd;
    HWND hWndDevList;
    INT_PTR Ret = FALSE;

    hpd = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg, DWLP_USER);

    if (hpd != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_INITDIALOG:
            {
                /* init list */
                hWndDevList = GetDlgItem(hwndDlg, IDC_DRIVERRESOURCES);
                InitializeDevicesList(hWndDevList);

                hpd = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (hpd != NULL)
                {
                    SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR)hpd);

                    UpdateDriverResourceDlg(hwndDlg, hpd);
                    AddResourceItems(hpd, hWndDevList);
                }

                Ret = TRUE;
                break;
            }
        }
    }

    return Ret;
}

static
PCM_RESOURCE_LIST
GetAllocatedResourceList(
    LPWSTR pszDeviceID)
{
    PCM_RESOURCE_LIST pResourceList = NULL;
    HKEY hKey = NULL;
    DWORD dwError, dwSize;

    CStringW keyName = L"SYSTEM\\CurrentControlSet\\Enum\\";
    keyName += pszDeviceID;
    keyName += L"\\Control";

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &hKey);
    if (dwError != ERROR_SUCCESS)
    {
        /* failed to open device instance log conf dir */
        return NULL;
    }

    dwSize = 0;
    RegQueryValueExW(hKey, L"AllocConfig", NULL, NULL, NULL, &dwSize);
    if (dwSize == 0)
        goto done;

    pResourceList = static_cast<PCM_RESOURCE_LIST>(HeapAlloc(GetProcessHeap(), 0, dwSize));
    if (pResourceList == NULL)
        goto done;

    dwError = RegQueryValueExW(hKey, L"AllocConfig", NULL, NULL, (LPBYTE)pResourceList, &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pResourceList);
        pResourceList = NULL;
    }

done:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return pResourceList;
}

static
PCM_RESOURCE_LIST
GetBootResourceList(
    LPWSTR pszDeviceID)
{
    PCM_RESOURCE_LIST pResourceList = NULL;
    HKEY hKey = NULL;
    DWORD dwError, dwSize;

    CStringW keyName = L"SYSTEM\\CurrentControlSet\\Enum\\";
    keyName += pszDeviceID;
    keyName += L"\\LogConf";

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &hKey);
    if (dwError != ERROR_SUCCESS)
    {
        /* failed to open device instance log conf dir */
        return NULL;
    }

    dwSize = 0;
    RegQueryValueExW(hKey, L"BootConfig", NULL, NULL, NULL, &dwSize);
    if (dwSize == 0)
        goto done;

    pResourceList = static_cast<PCM_RESOURCE_LIST>(HeapAlloc(GetProcessHeap(), 0, dwSize));
    if (pResourceList == NULL)
        goto done;

    dwError = RegQueryValueExW(hKey, L"BootConfig", NULL, NULL, (LPBYTE)pResourceList, &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pResourceList);
        pResourceList = NULL;
    }

done:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return pResourceList;
}

PVOID
GetResourceList(
    LPWSTR pszDeviceID)
{
    PCM_RESOURCE_LIST pResourceList = NULL;

    pResourceList = GetAllocatedResourceList(pszDeviceID);
    if (pResourceList == NULL)
        pResourceList = GetBootResourceList(pszDeviceID);

    return (PVOID)pResourceList;
}
