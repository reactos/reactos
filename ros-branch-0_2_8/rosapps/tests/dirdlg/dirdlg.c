#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include "resource.h"

static char selected[MAX_PATH + 1];

INT_PTR
CALLBACK
DlgMainProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  char dir[MAX_PATH + 1];

  switch(uMsg)
  {
    case WM_COMMAND:
    {
      switch(HIWORD(wParam))
      {
        case LBN_DBLCLK:
        {
          switch(LOWORD(wParam))
          {
            case IDC_DIRS:
            {
              if(DlgDirSelectEx(hwndDlg, dir, MAX_PATH, IDC_DIRS))
              {
                chdir(dir);
                GetCurrentDirectory(MAX_PATH, dir);
                DlgDirList(hwndDlg, dir, IDC_DIRS, IDC_DIREDIT, DDL_DIRECTORY | DDL_DRIVES);
              }
              else
              {
                SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_OK, 0), 0);
              }
              break;
            }
          }
          break;
        }
        default:
        {
          switch(LOWORD(wParam))
          {
            case IDC_OK:
            {
              char file[MAX_PATH + 1];
              int len;

              if(!DlgDirSelectEx(hwndDlg, file, MAX_PATH, IDC_DIRS))
              {
                GetCurrentDirectory(MAX_PATH, selected);
                len = strlen(selected);
                if(strlen(file))
                {
                  if(selected[len - 1] != '\\')
                  {
                    lstrcat(selected, "\\");
                  }
                  lstrcat(selected, file);
                  EndDialog(hwndDlg, IDC_OK);
                }
              }
              break;
            }
            case IDC_CANCEL:
            {
              EndDialog(hwndDlg, IDC_CANCEL);
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case WM_INITDIALOG:
    {
      SendDlgItemMessage(hwndDlg, IDC_DIRS, LB_SETCOLUMNWIDTH, 150, 0);
      GetCurrentDirectory(MAX_PATH, dir);
      DlgDirList(hwndDlg, dir, IDC_DIRS, IDC_DIREDIT, DDL_DIRECTORY | DDL_DRIVES);
      SetFocus(GetDlgItem(hwndDlg, IDC_DIRS));
      break;
    }
    case WM_CLOSE:
    {
      EndDialog(hwndDlg, IDC_CANCEL);
      return TRUE;
    }
  }
  return FALSE;
}

int WINAPI
WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpszCmdLine,
  int nCmdShow)
{
  char str[MAX_PATH + 32];
  if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DlgMainProc) == IDC_OK)
  {
    sprintf(str, "You selected \"%s\"", selected);
    MessageBox(0, str, "Selected file", MB_ICONINFORMATION);
  }
  return 0;
}

