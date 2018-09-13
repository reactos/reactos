#include "syncapp.h"

#ifndef WIN32
#include <w32sys.h>             // for IsPEFormat definition
#endif

static TCHAR const g_szAppName [] = TEXT("SYNCAPP") ;
static TCHAR const c_szDLL[]      = TEXT("SYNCUI.DLL");
#ifdef UNICODE
#    define BRIEFCASE_CREATE_ENTRY  "Briefcase_CreateW"
#else
#    define BRIEFCASE_CREATE_ENTRY  "Briefcase_Create"
#endif

static CHAR  const c_szFunction[] = BRIEFCASE_CREATE_ENTRY; // Lib entry point (never UNICODE)

static HINSTANCE hInst;
static HICON g_hIcon;

static HINSTANCE g_hModule;
static RUNDLLPROC g_lpfnCommand;
static HWND g_hwndStub;

static TCHAR s_szRunDLL32[] = TEXT("SYNCAPP.EXE ");

static BOOL   ParseCommand(void)
{
        // Load the library and get the procedure address
        // Note that we try to get a module handle first, so we don't need
        // to pass full file names around
        //

        g_hModule = GetModuleHandle(c_szDLL);
        if (g_hModule)
        {
                TCHAR szName[MAXPATHLEN];

                GetModuleFileName(g_hModule, szName, ARRAYSIZE(szName));
                LoadLibrary(szName);
        }
        else
        {
                g_hModule = LoadLibrary(c_szDLL);
                if ((UINT_PTR)g_hModule <= 32)
                {
                        return(FALSE);
                }
        }

        g_lpfnCommand = (RUNDLLPROC)GetProcAddress(g_hModule, c_szFunction);
        if (!g_lpfnCommand)
        {
                FreeLibrary(g_hModule);
                return(FALSE);
        }

        return(TRUE);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
        switch(iMessage)
        {
        case WM_CREATE:
                g_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT));
                break;

        case WM_DESTROY:
                break;

        default:
                return DefWindowProc(hWnd, iMessage, wParam, lParam) ;
                break;
        }

        return 0L;
}


static BOOL   InitStubWindow(HINSTANCE hInst, HINSTANCE hPrevInstance)
{
        WNDCLASS wndclass;

        if (!hPrevInstance)
        {
                wndclass.style         = 0 ;
                wndclass.lpfnWndProc   = (WNDPROC)WndProc ;
                wndclass.cbClsExtra    = 0 ;
                wndclass.cbWndExtra    = 0 ;
                wndclass.hInstance     = hInst ;
                wndclass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_DEFAULT)) ;
                wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
                wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
                wndclass.lpszMenuName  = NULL ;
                wndclass.lpszClassName = g_szAppName ;

                if (!RegisterClass(&wndclass))
                {
                        return(FALSE);
                }
        }

        g_hwndStub = CreateWindow(g_szAppName, TEXT(""), 0,
                0, 0, 0, 0, NULL, NULL, hInst, NULL);

        return(g_hwndStub != NULL);
}


static void   CleanUp(void)
{
        DestroyWindow(g_hwndStub);

        FreeLibrary(g_hModule);
}


int  WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszWinMainCmdLine, int nCmdShow)
{
        LPTSTR lpszCmdLine;
        hInst = hInstance;

        //
        // The command line passed to WinMain is always ANSI, so for UNICODE
        // builds we need to ask for the command line in UNICODE
        //

#ifdef UNICODE

        //
        // Since the command line returned from GetCommandLine includes
        // argv[0], but the one passed to Winmain does not, we have
        // to strip argv[0] in order to be equivalent
        //

        lpszCmdLine = GetCommandLine();
        
        //
        // Skip past program name (first token in command line).
        // Check for and handle quoted program name.
        //
        
        if ( *lpszCmdLine == '\"' ) 
        {
    
            //
            // Scan, and skip over, subsequent characters until
            // another double-quote or a null is encountered.
            //
    
            while ( *++lpszCmdLine && (*lpszCmdLine
                 != '\"') );
            //
            // If we stopped on a double-quote (usual case), skip
            // over it.
            //
    
            if ( *lpszCmdLine == '\"' )
                lpszCmdLine++;
        }
        else 
        {
            while (*lpszCmdLine > ' ')
                lpszCmdLine++;
        }

        //
        // Skip past any white space preceeding the second token.
        //
    
        while (*lpszCmdLine && (*lpszCmdLine <= ' ')) 
        {
            lpszCmdLine++;
        }

#else
        lpszCmdLine = lpszWinMainCmdLine;
#endif

        // turn off critical error stuff
        SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

        if (!ParseCommand())
        {
                goto Error0;
        }

        if (!InitStubWindow(hInstance, hPrevInstance))
        {
                goto Error1;
        }

        (*g_lpfnCommand)(g_hwndStub, hInstance, lpszCmdLine, nCmdShow);

Error1:
        CleanUp();
Error0:
        return(FALSE);
}
