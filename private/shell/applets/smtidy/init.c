//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

#include "smtidy.h"
#include "smwiz.h"
#include "resource.h"
#include "util.h"

//----------------------------------------------------------------------------
HINSTANCE g_hinstApp = NULL;

//----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lr = 0;

    switch (uMsg) 
    {
        case WM_CREATE:
            lr = 0;
            break;
        default:
            lr = DefWindowProc(hwnd, uMsg, wparam, lparam);
    }

    return lr;
}

//----------------------------------------------------------------------------
BOOL Class_Register(LPCTSTR pszClass, WNDPROC pfnWndProc)
{
    long lRet = FALSE;
    WNDCLASSEX wc;

    if (GetClassInfoEx(g_hinstApp, pszClass, &wc))
    {
        lRet = TRUE;
    }
    else
    {
        wc.cbSize           = sizeof(WNDCLASSEX);
        wc.style            = CS_DBLCLKS;
        wc.lpfnWndProc      = pfnWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = sizeof(PVOID);
        wc.hInstance        = g_hinstApp;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = pszClass;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;

        if (RegisterClassEx(&wc))
        	lRet = TRUE;
    	else
            lRet = GetClassInfoEx(g_hinstApp, pszClass, &wc);
	}

    return lRet;
}

const TCHAR c_szSMWizardClass[] = TEXT("Start Menu Wizard");

typedef struct
{
    HWND hwndOwner;
    HWND hwndOwnee;
} FindOwneeInfo, *PFindOwneeInfo;

//----------------------------------------------------------------------------
BOOL CALLBACK FindOwneeCallback(HWND hwnd, LPARAM lp)
{
    BOOL fRet = TRUE;
    PFindOwneeInfo pfii = (PFindOwneeInfo)lp;
    
    if (GetWindow(hwnd, GW_OWNER) == pfii->hwndOwner)
    {
        pfii->hwndOwnee = hwnd;
        fRet = FALSE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FindOwnee(HWND hwnd, HWND *phwndOwnee)
{
    BOOL fRet = FALSE;
    FindOwneeInfo fii = {hwnd , 0};
    
    *phwndOwnee = NULL;
    EnumWindows(FindOwneeCallback, (LPARAM)&fii);
    if (fii.hwndOwnee)
    {
        *phwndOwnee = fii.hwndOwnee;
        fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL App_Create(PSMTIDYINFO psmti)
{
    BOOL fRet = FALSE;
    
    psmti->hdsaSMI = DSA_Create(sizeof(SMITEM), 0);
    if (psmti->hdsaSMI)
    {
        psmti->hbmp = LoadBitmap(g_hinstApp, MAKEINTRESOURCE(IDB_SMTIDY));
        if (psmti->hbmp)
        {
            HWND hwnd = FindWindow(c_szSMWizardClass, NULL);
            if (hwnd)
            {
                HWND hwndOwnee;
                if (FindOwnee(hwnd, &hwndOwnee))
                    SetForegroundWindow(hwndOwnee);
            }
            else
            {
                if (Class_Register(c_szSMWizardClass, WndProc))
                {
                    psmti->hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, c_szSMWizardClass, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, g_hinstApp, 0);
                    if (psmti->hwnd)
                    {
                        GetWindowRect(psmti->hwnd, &(psmti->rc));
                        // Default options if the user hit's 'Finish' early.
                        psmti->dwFlags = SMTIF_FIX_BROKEN_SHORTCUTS | SMTIF_GROUP_UNUSED_SHORTCUTS | SMTIF_GROUP_READMES | SMTIF_FIX_SINGLE_ITEM_FOLDERS | SMTIF_REMOVE_EMPTY_FOLDERS;
                        Dbg_OpenLog();
                        fRet = TidyStartMenuWizard(psmti);
                    }
                }
            }
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL App_Destroy(PSMTIDYINFO psmti)
{
    if (psmti->hdsaSMI)
    {
        int i;
        int cItems = DSA_GetItemCount(psmti->hdsaSMI);;

        // Free all the strings first.
        for (i=0; i<cItems; i++)
        {
            PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
            if (psmi->pidlItem)
                ILFree(psmi->pidlItem);
            if (psmi->pszTarget)
                LFree(psmi->pszTarget);
            if (psmi->pszNewTarget)
                LFree(psmi->pszNewTarget);
        }

        // Then the dsa.
        DSA_Destroy(psmti->hdsaSMI);
        psmti->hdsaSMI = NULL;
    }

    if (psmti->hbmp)
    {
    	DeleteObject(psmti->hbmp);
    	psmti->hbmp = NULL;
    }

    if (psmti->pszLostTargetGroup)
        LFree(psmti->pszLostTargetGroup);
        
    if (psmti->pszReadMeGroup)
        LFree(psmti->pszReadMeGroup);

    if (psmti->pszUnusedShortcutGroup)
        LFree(psmti->pszUnusedShortcutGroup);

    if (psmti->hdpa)
        DPA_Destroy(psmti->hdpa);

    if (psmti->hfontLarge)
        DeleteObject(psmti->hfontLarge);
        
    // NB pszSearchOrigin isn't allocated, it's on the stack.

    Dbg_CloseLog();            
    return TRUE;
}

//----------------------------------------------------------------------------
void MessageLoop()
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

//----------------------------------------------------------------------------
int WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    SMTIDYINFO smti;

    USE(hPrevInstance);
    USE(nCmdShow);
    USE(lpszCmdLine);
    
    g_hinstApp = hInstance;

#ifdef DEBUG
    if (GetAsyncKeyState(VK_MENU) < 0)
        DebugBreak();
#endif
         
    memset(&smti, 0, sizeof(smti));

    if (App_Create(&smti))
    {
        // MessageLoop();
        App_Destroy(&smti);
    }
    
    Dbg(TEXT("smt.wm: App exit."));
    return TRUE;
}

//----------------------------------------------------------------------------
// stolen from the CRT, used to shrink our code by 10K.
void _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    if (*pszCmdLine == TEXT('\"'))
    {
        // Scan, and skip over, subsequent characters until
        // another double-quote or a null is encountered.
        while (*++pszCmdLine && (*pszCmdLine != '\"'));
        // If we stopped on a double-quote (usual case), skip
        // over it.
        if (*pszCmdLine == TEXT('\"'))
    	    pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > TEXT(' '))
    	    pszCmdLine++;
    }
    
    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
        si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);

//  Some compilers get mad if you put code after ExitProcess() because
//  they know that ExitProcess does not return.
//
//  Dbg(TEXT("smt.me: Main thread exiting without ExitProcess."));
}
