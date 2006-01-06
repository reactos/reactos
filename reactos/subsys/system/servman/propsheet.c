#include "servman.h"

extern HINSTANCE hInstance;


/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
		        WPARAM wParam,
		        LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:

            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START:
                    break;

                case IDC_STOP:

                    break;
            }
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)

                default:
                    break;
            }
            break;
    }

    return FALSE;
}



static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hInstance;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
PropSheets(HWND hwnd)
{
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[1];
  TCHAR Caption[256];

  LoadString(hInstance, IDS_PROP_SHEET, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hInstance;
  psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_DLG_GENERAL, GeneralPageProc);
  //logon
  //recovery
  //dependancies

  return (LONG)(PropertySheet(&psh) != -1);
}
