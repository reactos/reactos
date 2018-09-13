#include "stdafx.h"
#include "systray.h"
#include <cscuiext.h>
///////////////////////////////////////////////////////////////////////////////
// CSC_CheckEnable

typedef BOOL (WINAPI* PFNCSCMSGPROCESS)(LPMSG);

static HWND g_hWndCSC = NULL;
static HMODULE g_hCSCUIDll = NULL;
static PFNCSCMSGPROCESS g_pfnMsgProcess = NULL;

BOOL CSC_CheckEnable(HWND hWnd, BOOL bSvcEnabled)
{

    if ((g_hWndCSC && IsWindow(g_hWndCSC) && !bSvcEnabled) ||
        (((!g_hWndCSC) || !IsWindow(g_hWndCSC)) && bSvcEnabled))
    
    {
        if (!g_hCSCUIDll)
            g_hCSCUIDll = LoadLibrary(TEXT("cscui.dll"));
            
        if (g_hCSCUIDll)
        {
            PFNCSCUIINITIALIZE pfn = (PFNCSCUIINITIALIZE)GetProcAddress(g_hCSCUIDll, "CSCUIInitialize");
            g_pfnMsgProcess = (PFNCSCMSGPROCESS)GetProcAddress(g_hCSCUIDll, "CSCUIMsgProcess");
            if (pfn)
            {
                DWORD dwFlags = CI_INITIALIZE | CI_CREATEWINDOW;

                if (!bSvcEnabled)
                    dwFlags = CI_TERMINATE | CI_DESTROYWINDOW;
                    
                g_hWndCSC = (*pfn)(NULL, dwFlags);
            }    
        }

        if (g_hCSCUIDll && !bSvcEnabled)
        {
            FreeLibrary(g_hCSCUIDll);
            g_hCSCUIDll = NULL;
            g_pfnMsgProcess = NULL;
        }
    }        
    return(TRUE);
}

BOOL CSC_MsgProcess(LPMSG pMsg)
{
    if (g_pfnMsgProcess)
        return (*g_pfnMsgProcess)(pMsg);

    return FALSE;
}
