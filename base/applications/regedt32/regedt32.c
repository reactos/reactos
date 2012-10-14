#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>

#define REGEDIT  _T("regedit.exe")

int
WINAPI
_tWinMain(HINSTANCE hCurInst,
          HINSTANCE hPrevInst,
          LPTSTR lpsCmdLine,
          int nCmdShow)
{
    TCHAR szPath[MAX_PATH];

    if(GetWindowsDirectory(szPath, MAX_PATH))
    {
        PathAppend(szPath, REGEDIT);
        ShellExecute(NULL,
                     NULL,
                     szPath,
                     lpsCmdLine,
                     NULL,
                     nCmdShow);
    }
    else
    {
        ShellExecute(NULL,
                     NULL,
                     REGEDIT,
                     lpsCmdLine,
                     NULL,
                     nCmdShow);
    }

    return 0;
}
