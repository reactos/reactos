/*
* PROJECT:     Safely Remove Hardware Applet
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/cpl/hotplug/hotplug.c
* PURPOSE:     applet initialization
* PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "hotplug.h"

// globals
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};


LONG
APIENTRY
InitApplet(
    HWND hwnd, 
    UINT uMsg,
    LPARAM wParam,
    LPARAM lParam)
{
    // TODO
    return FALSE;
}


LONG
CALLBACK
CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    switch(uMsg)
    {
        case CPL_INIT:
        {
            return TRUE;
        }
        case CPL_GETCOUNT:
        {
            return NUM_APPLETS;
        }
        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->idIcon = Applets[0].idIcon;
            CPlInfo->idName = Applets[0].idName;
            CPlInfo->idInfo = Applets[0].idDescription;
            break;
        }
        case CPL_DBLCLK:
        {
            InitApplet(hwndCPl, uMsg, lParam1, lParam2);
            break;
        }
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

    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        hApplet = hinstDLL;
        break;
    }
    return TRUE;
}
