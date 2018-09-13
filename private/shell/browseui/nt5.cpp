#include "priv.h"
#include "shbrows2.h"
#include "commonsb.h"
#include "resource.h"

//
//
//  If you want NT5 defines in the above headerfiles, you gotta set _WIN32_WINNT
//  to 0x0500, which the standard browseui build does not do...
//
//  So we do it here in nt5.cpp.
//
//


// WM_APPCOMMAND handling
//
typedef struct tagAppCmd
{
    UINT idAppCmd;
    UINT idCmd;
} APPCMD;

BOOL CShellBrowser2::_OnAppCommand(WPARAM wParam, LPARAM lParam)
{
    static APPCMD rgcmd[] =
    {
        { APPCOMMAND_BROWSER_BACKWARD, FCIDM_NAVIGATEBACK },
        { APPCOMMAND_BROWSER_FORWARD, FCIDM_NAVIGATEFORWARD },
        { APPCOMMAND_BROWSER_REFRESH, FCIDM_REFRESH },
        { APPCOMMAND_BROWSER_STOP, FCIDM_STOP },
        { APPCOMMAND_BROWSER_SEARCH, FCIDM_VBBSEARCHBAND }, // FCIDM_SEARCHPAGE ?
        { APPCOMMAND_BROWSER_FAVORITES, FCIDM_VBBFAVORITESBAND },
        { APPCOMMAND_BROWSER_HOME, FCIDM_STARTPAGE },
        { APPCOMMAND_LAUNCH_MAIL, FCIDM_MAIL }
    };

    BOOL bRet = FALSE;
    if (!IsMinimized(_pbbd->_hwnd))
    {
        UINT idAppCmd = GET_APPCOMMAND_LPARAM(lParam);

        for (int i = 0 ; i < ARRAYSIZE(rgcmd) ; i++)
        {
            if (rgcmd[i].idAppCmd == idAppCmd)
            {
                OnCommand(GET_WM_COMMAND_MPS(rgcmd[i].idCmd,
                                             GET_WM_COMMAND_HWND(wParam, lParam),
                                             GET_WM_COMMAND_CMD(wParam, lParam)));
                bRet = TRUE;
                break;
            }
        }
    }
    return bRet;
}

// Our NT5 version of the WndProc
//
LPARAM CShellBrowser2::_WndProcBSNT5(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPARAM lRet = 0;
    switch(uMsg)
    {
       case WM_APPCOMMAND:
       {
          if (_OnAppCommand(wParam, lParam))
             lRet = 1;
          break;
       }
       default:
          break;
    }

    return lRet;
}
