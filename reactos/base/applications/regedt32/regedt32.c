#include <windows.h>
#include <tchar.h>
#include <shellapi.h>

int WINAPI _tWinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst,
                     LPTSTR lpsCmdLine, int nCmdShow)
{
    ShellExecute(NULL, NULL, _T("regedit.exe"), lpsCmdLine, NULL, nCmdShow);

    return 0;
}
