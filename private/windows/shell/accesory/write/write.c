
#include <windows.h>
#include <shellapi.h>

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();

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

    ShellExecuteA(HWND_DESKTOP, NULL, "wordpad.exe", lpCmdLine, NULL,
                      si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    return 0 ;
}
