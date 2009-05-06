/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "mciqtz_private.h"
#include "digitalv.h"
#include "wownt32.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciqtz);

static DWORD MCIQTZ_mciClose(UINT, DWORD, LPMCI_GENERIC_PARMS);
static DWORD MCIQTZ_mciStop(UINT, DWORD, LPMCI_GENERIC_PARMS);

/*======================================================================*
 *                          MCI QTZ implementation                      *
 *======================================================================*/

HINSTANCE MCIQTZ_hInstance = 0;

/***********************************************************************
 *              DllMain (MCIQTZ.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        MCIQTZ_hInstance = hInstDLL;
        break;
    }
    return TRUE;
}

/**************************************************************************
 *                              MCIQTZ_mciGetOpenDev            [internal]
 */
static WINE_MCIQTZ* MCIQTZ_mciGetOpenDev(UINT wDevID)
{
    WINE_MCIQTZ* wma = (WINE_MCIQTZ*)mciGetDriverData(wDevID);

    if (!wma) {
        WARN("Invalid wDevID=%u\n", wDevID);
        return NULL;
    }
    return wma;
}

/**************************************************************************
 *                              MCIQTZ_drvOpen                  [internal]
 */
static DWORD MCIQTZ_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIQTZ* wma;

    TRACE("(%s, %p)\n", debugstr_w(str), modp);

    /* session instance */
    if (!modp)
        return 0xFFFFFFFF;

    wma = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIQTZ));
    if (!wma)
        return 0;

    wma->wDevID = modp->wDeviceID;
    mciSetDriverData(wma->wDevID, (DWORD_PTR)wma);

    return modp->wDeviceID;
}

/**************************************************************************
 *                              MCIQTZ_drvClose         [internal]
 */
static DWORD MCIQTZ_drvClose(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x)\n", dwDevID);

    wma = MCIQTZ_mciGetOpenDev(dwDevID);

    if (wma) {
        /* finish all outstanding things */
        MCIQTZ_mciClose(dwDevID, MCI_WAIT, NULL);

        HeapFree(GetProcessHeap(), 0, wma);
        return 1;
    }

    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 *                              MCIQTZ_drvConfigure             [internal]
 */
static DWORD MCIQTZ_drvConfigure(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x)\n", dwDevID);

    wma = MCIQTZ_mciGetOpenDev(dwDevID);
    if (!wma)
        return 0;

    MCIQTZ_mciStop(dwDevID, MCI_WAIT, NULL);

    MessageBoxA(0, "Sample QTZ Wine Driver !", "MM-Wine Driver", MB_OK);

    return 1;
}

/***************************************************************************
 *                              MCIQTZ_mciOpen                  [internal]
 */
static DWORD MCIQTZ_mciOpen(UINT wDevID, DWORD dwFlags,
                            LPMCI_DGV_OPEN_PARMSW lpOpenParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpOpenParms);

    if (!lpOpenParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&wma->pgraph);
    if (FAILED(hr)) {
        TRACE("Cannot create filtergraph (hr = %x)\n", hr);
        goto err;
    }

    hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaControl, (LPVOID*)&wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot get IMediaControl interface (hr = %x)\n", hr);
        goto err;
    }

    if (!((dwFlags & MCI_OPEN_ELEMENT) && (dwFlags & MCI_OPEN_ELEMENT))) {
        TRACE("Wrong dwFlags %x\n", dwFlags);
        goto err;
    }

    if (!lpOpenParms->lpstrElementName || !lpOpenParms->lpstrElementName[0]) {
        TRACE("Invalid filename specified\n");
        goto err;
    }

    TRACE("Open file %s\n", debugstr_w(lpOpenParms->lpstrElementName));

    hr = IGraphBuilder_RenderFile(wma->pgraph, lpOpenParms->lpstrElementName, NULL);
    if (FAILED(hr)) {
        TRACE("Cannot render file (hr = %x)\n", hr);
        goto err;
    }

    wma->opened = TRUE;

    return 0;

err:
    if (wma->pgraph)
        IGraphBuilder_Release(wma->pgraph);
    wma->pgraph = NULL;
    if (wma->pmctrl)
        IMediaControl_Release(wma->pmctrl);
    wma->pmctrl = NULL;

    CoUninitialize();

    return MCIERR_INTERNAL;
}

/***************************************************************************
 *                              MCIQTZ_mciClose                 [internal]
 */
static DWORD MCIQTZ_mciClose(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    if (wma->opened) {
        IGraphBuilder_Release(wma->pgraph);
        IMediaControl_Release(wma->pmctrl);
        CoUninitialize();
        wma->opened = FALSE;
    }

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciPlay                  [internal]
 */
static DWORD MCIQTZ_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    hr = IMediaControl_Run(wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot run filtergraph (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    wma->started = TRUE;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciSeek                  [internal]
 */
static DWORD MCIQTZ_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;
    IMediaPosition* pmpos;
    LONGLONG newpos;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    if (dwFlags & MCI_SEEK_TO_START) {
        newpos = 0;
    } else if (dwFlags & MCI_SEEK_TO_END) {
        FIXME("MCI_SEEK_TO_END not implemented yet\n");
        return MCIERR_INTERNAL;
    } else if (dwFlags & MCI_TO) {
        FIXME("MCI_TO not implemented yet\n");
        return MCIERR_INTERNAL;
    } else {
        WARN("dwFlag doesn't tell where to seek to...\n");
        return MCIERR_MISSING_PARAMETER;
    }

    hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaPosition, (LPVOID*)&pmpos);
    if (FAILED(hr)) {
        FIXME("Cannot get IMediaPostion interface (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    hr = IMediaPosition_put_CurrentPosition(pmpos, newpos);
    if (FAILED(hr)) {
        FIXME("Cannot set position (hr = %x)\n", hr);
        IMediaPosition_Release(pmpos);
        return MCIERR_INTERNAL;
    }

    IMediaPosition_Release(pmpos);

    if (dwFlags & MCI_NOTIFY)
        mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)), wDevID, MCI_NOTIFY_SUCCESSFUL);

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciStop                  [internal]
 */
static DWORD MCIQTZ_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (!wma->started)
        return 0;

    hr = IMediaControl_Stop(wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot stop filtergraph (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    wma->started = FALSE;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciStatus                [internal]
 */
static DWORD MCIQTZ_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_DGV_STATUS_PARMSW lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (!(dwFlags & MCI_STATUS_ITEM)) {
        WARN("No status item specified\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }

    switch (lpParms->dwItem) {
        case MCI_STATUS_LENGTH:
            FIXME("MCI_STATUS_LENGTH not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_POSITION:
        {
            HRESULT hr;
            REFTIME curpos;
            IMediaPosition* pmpos;

            hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaPosition, (LPVOID*)&pmpos);
            if (FAILED(hr)) {
                FIXME("Cannot get IMediaPostion interface (hr = %x)\n", hr);
                return MCIERR_INTERNAL;
            }

            hr = IMediaPosition_get_CurrentPosition(pmpos, &curpos);
            if (FAILED(hr)) {
                FIXME("Cannot get position (hr = %x)\n", hr);
                IMediaPosition_Release(pmpos);
                return MCIERR_INTERNAL;
            }

            IMediaPosition_Release(pmpos);
            lpParms->dwReturn = curpos / 10000;

            break;
        }
        case MCI_STATUS_NUMBER_OF_TRACKS:
            FIXME("MCI_STATUS_NUMBER_OF_TRACKS not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_MODE:
            FIXME("MCI_STATUS_MODE not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_MEDIA_PRESENT:
            FIXME("MCI_STATUS_MEDIA_PRESENT not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_TIME_FORMAT:
            FIXME("MCI_STATUS_TIME_FORMAT not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_READY:
            FIXME("MCI_STATUS_READY not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_CURRENT_TRACK:
            FIXME("MCI_STATUS_CURRENT_TRACK not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        default:
            FIXME("Unknown command %08X\n", lpParms->dwItem);
            return MCIERR_UNRECOGNIZED_COMMAND;
    }

    if (dwFlags & MCI_NOTIFY)
        mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)), wDevID, MCI_NOTIFY_SUCCESSFUL);

    return 0;
}

/*======================================================================*
 *                          MCI QTZ entry points                        *
 *======================================================================*/

/**************************************************************************
 *                              DriverProc (MCIQTZ.@)
 */
LRESULT CALLBACK MCIQTZ_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lX, %p, %08X, %08lX, %08lX)\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
        case DRV_LOAD:                  return 1;
        case DRV_FREE:                  return 1;
        case DRV_OPEN:                  return MCIQTZ_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
        case DRV_CLOSE:                 return MCIQTZ_drvClose(dwDevID);
        case DRV_ENABLE:                return 1;
        case DRV_DISABLE:               return 1;
        case DRV_QUERYCONFIGURE:        return 1;
        case DRV_CONFIGURE:             return MCIQTZ_drvConfigure(dwDevID);
        case DRV_INSTALL:               return DRVCNF_RESTART;
        case DRV_REMOVE:                return DRVCNF_RESTART;
    }

    /* session instance */
    if (dwDevID == 0xFFFFFFFF)
        return 1;

    switch (wMsg) {
        case MCI_OPEN_DRIVER:   return MCIQTZ_mciOpen      (dwDevID, dwParam1, (LPMCI_DGV_OPEN_PARMSW)     dwParam2);
        case MCI_CLOSE_DRIVER:  return MCIQTZ_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
        case MCI_PLAY:          return MCIQTZ_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)          dwParam2);
        case MCI_SEEK:          return MCIQTZ_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)          dwParam2);
        case MCI_STOP:          return MCIQTZ_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
        case MCI_STATUS:        return MCIQTZ_mciStatus    (dwDevID, dwParam1, (LPMCI_DGV_STATUS_PARMSW)   dwParam2);
        case MCI_RECORD:
        case MCI_SET:
        case MCI_PAUSE:
        case MCI_RESUME:
        case MCI_GETDEVCAPS:
        case MCI_INFO:
        case MCI_PUT:
        case MCI_WINDOW:
        case MCI_LOAD:
        case MCI_SAVE:
        case MCI_FREEZE:
        case MCI_REALIZE:
        case MCI_UNFREEZE:
        case MCI_UPDATE:
        case MCI_WHERE:
        case MCI_STEP:
        case MCI_COPY:
        case MCI_CUT:
        case MCI_DELETE:
        case MCI_PASTE:
        case MCI_CUE:
        /* Digital Video specific */
        case MCI_CAPTURE:
        case MCI_MONITOR:
        case MCI_RESERVE:
        case MCI_SETAUDIO:
        case MCI_SIGNAL:
        case MCI_SETVIDEO:
        case MCI_QUALITY:
        case MCI_LIST:
        case MCI_UNDO:
        case MCI_CONFIGURE:
        case MCI_RESTORE:
            FIXME("Unimplemented command [%08X]\n", wMsg);
            break;
        case MCI_SPIN:
        case MCI_ESCAPE:
            WARN("Unsupported command [%08X]\n", wMsg);
            break;
        case MCI_OPEN:
        case MCI_CLOSE:
            FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
            break;
        default:
            TRACE("Sending msg [%08X] to default driver proc\n", wMsg);
            return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }

    return MCIERR_UNRECOGNIZED_COMMAND;
}
