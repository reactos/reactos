#include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include "resource.h"

typedef BOOL (WINAPI * SH_GIL_PROC)(HIMAGELIST *phLarge, HIMAGELIST *phSmall);
typedef BOOL (WINAPI * FII_PROC)(BOOL fFullInit);

/*** Shell32 undoc'd functions ***/
  /* Shell_GetImageLists  @71  */
  /* FileIconInit         @660 */

BOOL
DisplayImageList(HWND hwnd,
                 UINT uID)
{
    HWND hLV;
    SP_CLASSIMAGELIST_DATA ImageListData;
    LV_ITEM lvItem;
    TCHAR Buf[6];
    INT ImageListCount = -1;
    INT i = 0;

    hLV = GetDlgItem(hwnd, IDC_LSTVIEW);
    (void)ListView_DeleteAllItems(hLV);

    if (uID == IDC_SYSTEM)
    {
        HIMAGELIST hLarge, hSmall;
        HMODULE      hShell32;
        SH_GIL_PROC  Shell_GetImageLists;
        FII_PROC     FileIconInit;

        hShell32 = LoadLibrary(_T("shell32.dll"));
        if(hShell32 == NULL)
            return FALSE;

        Shell_GetImageLists = (SH_GIL_PROC)GetProcAddress(hShell32, (LPCSTR)71);
        FileIconInit = (FII_PROC)GetProcAddress(hShell32, (LPCSTR)660);

        if(Shell_GetImageLists == NULL || FileIconInit == NULL)
        {
            FreeLibrary(hShell32);
            return FALSE;
        }

        FileIconInit(TRUE);

        Shell_GetImageLists(&hLarge, &hSmall);

        ImageListCount = ImageList_GetImageCount(hSmall);

        (void)ListView_SetImageList(hLV,
                                    hSmall,
                                    LVSIL_SMALL);

        FreeLibrary(hShell32);
    }
    else if (uID == IDC_DEVICE)
    {
        ImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
        SetupDiGetClassImageList(&ImageListData);

        ImageListCount = ImageList_GetImageCount(ImageListData.ImageList);

        (void)ListView_SetImageList(hLV,
                                    ImageListData.ImageList,
                                    LVSIL_SMALL);
    }
    else
        return FALSE;

    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;

    while (i <= ImageListCount)
    {
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = _itot(i, Buf, 10);
        lvItem.iImage = i;

        (void)ListView_InsertItem(hLV, &lvItem);

        i++;
    }

    return TRUE;
}


BOOL CALLBACK
DlgProc(HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            DisplayImageList(hwnd, IDC_SYSTEM);
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            return TRUE;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd, 0);
                    return TRUE;

                case IDC_SYSTEM:
                    DisplayImageList(hwnd, IDC_SYSTEM);
                    return TRUE;

                case IDC_DEVICE:
                    DisplayImageList(hwnd, IDC_DEVICE);
                    return TRUE;
            }
        }
    }

    return FALSE;
}

int WINAPI
WinMain(HINSTANCE hThisInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpszArgument,
        int nCmdShow)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    return DialogBox(hThisInstance,
                     MAKEINTRESOURCE(IDD_IMGLST),
                     NULL,
                     (DLGPROC)DlgProc);
}
