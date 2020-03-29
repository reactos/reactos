/*
* PROJECT:     Safely Remove Hardware Applet
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/cpl/hotplug/hotplug.c
* PURPOSE:     applet initialization
* PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "hotplug.h"

#define NDEBUG
#include <debug.h>

// globals
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_HOTPLUG, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};


INT_PTR
CALLBACK
SafeRemovalDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCLOSE:
                    EndDialog(hwndDlg, TRUE);
                    break;

            }
            break;
    }

    return FALSE;
}


LONG
APIENTRY
InitApplet(
    HWND hwnd,
    UINT uMsg,
    LPARAM wParam,
    LPARAM lParam)
{
    DPRINT1("InitApplet()\n");

    DialogBox(hApplet,
              MAKEINTRESOURCE(IDD_SAFE_REMOVE_HARDWARE_DIALOG),
              hwnd,
              SafeRemovalDlgProc);

    // TODO
    return TRUE;
}


LONG
CALLBACK
CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    UINT i = (UINT)lParam1;

    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            break;

        case CPL_DBLCLK:
            Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;

        case CPL_STARTWPARMSW:
            return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
    }
    return FALSE;
}


INT
WINAPI
DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            hApplet = hinstDLL;
            break;
    }
    return TRUE;
}
