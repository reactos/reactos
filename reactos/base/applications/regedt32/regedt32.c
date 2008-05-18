#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>

int WINAPI _tWinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst,
                     LPTSTR lpsCmdLine, int nCmdShow)
{
    TCHAR szPath[MAX_PATH];

    if(GetWindowsDirectory(szPath, MAX_PATH))
    {
        PathAppend(szPath, _T("regedit.exe"));
        ShellExecute(NULL, NULL, szPath, lpsCmdLine, NULL, nCmdShow);
    }

    return 0;
}
