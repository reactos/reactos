#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include "resource.h"
#include "sysdm.h"


/* Environment dialog procedure */
INT_PTR CALLBACK
VirtMemDlgProc(HWND hwndDlg,
		   UINT uMsg,
		   WPARAM wParam,
		   LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDCANCEL:
          EndDialog(hwndDlg, 0);
          return TRUE;
      }
      break;

    case WM_NOTIFY:
      break;
  }

  return FALSE;
}

/* EOF */
