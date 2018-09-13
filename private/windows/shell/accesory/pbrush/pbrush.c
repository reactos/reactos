
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "pbrush.h"

extern int _cdecl ShellMessageBox(HINSTANCE hAppInst, HWND hWnd, LPCSTR
        lpcText, LPCSTR lpcTitle, UINT fuStyle, ...);
// stolen from the CRT, used to shirink our code

int _stdcall ModuleEntry(void)
{
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    return WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

}


const char szFile[] = "mspaint.exe";

//---------------------------------------------------------------------------
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ShellExecute(HWND_DESKTOP, NULL, szFile, lpCmdLine, NULL, nCmdShow);
    ExitProcess(0);
    return 0;
}
