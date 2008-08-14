/*
 *
 * PROJECT:         ReactOS WDM Audio driver mapper
 * FILE:            dll/win32/wdmaud.drv/wdmaud.c
 * PURPOSE:         wdmaud.drv
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *
 * UPDATE HISTORY:
 *      25/05/2008  Created
 */

#include <stdarg.h>

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <debug.h>

DWORD APIENTRY
mxdMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    DPRINT1("mxdMessage(%04X, %04X, %08X, %08X, %08X);\n", uDevice, uMsg, dwUser, dwParam1, dwParam2);

    switch (uMsg)
    {
        case MXDM_INIT:
        break;

        case MXDM_GETNUMDEVS:
        break;

        case MXDM_GETDEVCAPS:
        break;

        case MXDM_OPEN:
        break;

        case MXDM_CLOSE:
        break;

        case MXDM_GETLINEINFO:
        break;

        case MXDM_GETLINECONTROLS:
        break;

        case MXDM_GETCONTROLDETAILS:
        break;

        case MXDM_SETCONTROLDETAILS:
        break;
    }

    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
auxMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    DPRINT1("auxMessage(%04X, %04X, %08X, %08X, %08X);\n", uDevice, uMsg, dwUser, dwParam1, dwParam2);

    switch (uMsg)
    {
        case AUXDM_GETDEVCAPS:

        break;

        case AUXDM_GETNUMDEVS:

        break;

        case AUXDM_GETVOLUME:

        break;

        case AUXDM_SETVOLUME:

        break;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
wodMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    DPRINT1("wodMessage(%04X, %04X, %08X, %08X, %08X);\n", uDevice, uMsg, dwUser, dwParam1, dwParam2);

    switch (uMsg)
    {
        case WODM_GETNUMDEVS:
        break;

        case WODM_GETDEVCAPS:
        break;

        case WODM_OPEN:
        break;

        case WODM_CLOSE:
        break;

        case WODM_WRITE:
        break;

        case WODM_PAUSE:
        break;

        case WODM_RESTART:
        break;

        case WODM_RESET:
        break;

        case WODM_BREAKLOOP:
        break;

        case WODM_GETPOS:
        break;

        case WODM_SETPITCH:
        break;

        case WODM_SETVOLUME:
        break;

        case WODM_SETPLAYBACKRATE:
        break;

        case WODM_GETPITCH:
        break;

        case WODM_GETVOLUME:
        break;

        case WODM_GETPLAYBACKRATE:
        break;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
widMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    DPRINT1("widMessage(%04X, %04X, %08X, %08X, %08X);\n", uDevice, uMsg, dwUser, dwParam1, dwParam2);

    switch (uMsg)
    {
        case WIDM_GETNUMDEVS:
        break;

        case WIDM_GETDEVCAPS:
        break;

        case WIDM_OPEN:
        break;

        case WIDM_CLOSE:
        break;

        case WIDM_ADDBUFFER:
        break;

        case WIDM_STOP:
        break;

        case WIDM_START:
        break;

        case WIDM_RESET:
        break;

        case WIDM_GETPOS:
        break;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    return MMSYSERR_NOTSUPPORTED;
}

DWORD APIENTRY
modMessage(UINT uDevice,
           UINT uMsg,
           DWORD dwUser,
           DWORD dwParam1,
           DWORD dwParam2)
{
    DPRINT1("modMessage(%04X, %04X, %08X, %08X, %08X);\n", uDevice, uMsg, dwUser, dwParam1, dwParam2);

    return MMSYSERR_NOTSUPPORTED;
}

LRESULT APIENTRY
DriverProc(DWORD dwDriverID,
           HDRVR hDriver,
           UINT uiMessage,
           LPARAM lParam1,
           LPARAM lParam2)
{
    return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

