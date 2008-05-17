#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

int WINAPI _tWinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst,
                     LPTSTR lpsCmdLine, int nCmdShow)
{
    TCHAR szPath[MAX_PATH];

    GetWindowsDirectory(szPath, MAX_PATH);
    _tcscat(szPath, _T("\\regedit.exe"));

    ShellExecute(NULL, NULL, szPath, lpsCmdLine, NULL, nCmdShow);

    return 0;
}
