/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Storage device properties
 * COPYRIGHT:   2021 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>


static
INT_PTR
CALLBACK
DiskPropPageDialog(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    DPRINT1("DiskPropPageDialog()\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE; //OnInitDialog(hwnd, wParam, lParam);

        case WM_COMMAND:
            EndDialog(hwnd, 0); //OnCommand(hwnd, wParam, lParam);
            break;
/*
        case WM_NOTIFY:
            OnNotify(hwnd, wParam, lParam);
            break;

        case WM_DESTROY:
            OnDestroy(hwnd);
            break;
*/
    }

    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
DiskClassInstaller(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = {0};
    PROPSHEETPAGE Page;
    HPROPSHEETPAGE PageHandle;

    DPRINT1("DiskClassInstaller(%u %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData);

    if (InstallFunction == DIF_ADDPROPERTYPAGE_ADVANCED)
    {
        if (DeviceInfoData == NULL)
            return ERROR_DI_DO_DEFAULT;

        AddPropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        if (SetupDiGetClassInstallParamsW(DeviceInfoSet,
                                          DeviceInfoData,
                                          (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                                          sizeof(SP_ADDPROPERTYPAGE_DATA),
                                          NULL))
        {
            DPRINT1("\n");
            if (AddPropertyPageData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES)
                return ERROR_SUCCESS;

            ZeroMemory(&Page, sizeof(PROPSHEETPAGE));
            Page.dwSize = sizeof(PROPSHEETPAGE);
            Page.dwFlags = PSP_USECALLBACK;
            Page.hInstance = hInstance;
            Page.pszTemplate = MAKEINTRESOURCE(IDD_DISK_POLICIES);
            Page.pfnDlgProc = DiskPropPageDialog;
            Page.pfnCallback = NULL; //DiskPropPageCallback;
            Page.lParam = (LPARAM)NULL;

            PageHandle = CreatePropertySheetPage(&Page);
            if (PageHandle == NULL)
            {
                DPRINT1("CreatePropertySheetPage() failed!\n");
                return ERROR_SUCCESS;
            }

            AddPropertyPageData.DynamicPages[AddPropertyPageData.NumDynamicPages] = PageHandle;
            AddPropertyPageData.NumDynamicPages++;
            DPRINT1("Pages: %ld\n", AddPropertyPageData.NumDynamicPages);

            if (!SetupDiSetClassInstallParamsW(DeviceInfoSet,
                                               DeviceInfoData,
                                               (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                                               sizeof(SP_ADDPROPERTYPAGE_DATA)))
            {
                DPRINT1("SetupDiSetClassInstallParamsW() failed (Error %lu)\n", GetLastError());
            }
        }

        return ERROR_SUCCESS;
    }

    return ERROR_DI_DO_DEFAULT;
}

/* EOF */
