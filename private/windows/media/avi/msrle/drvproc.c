/****************************************************************************
 *
 *   drvproc.c
 *
 ***************************************************************************/
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1991 - 1995 Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#ifndef _INC_COMPDDK
#define _INC_COMPDDK    50      /* version number */
#endif

#include <vfw.h>
#include "msrle.h"

HMODULE ghModule;

/***************************************************************************
 * @doc INTERNAL
 *
 * @api LRESULT | DriverProc | The entry point for an installable driver.
 *
 * @parm DWORD | dwDriverId | For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm HDRVR | hDriver | This is the handle returned to the
 *     application by the driver interface.
 *
 * @parm UINT | uiMessage | The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols. Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 * @parm LPARAM | lParam1 | Data for this message.  Defined separately for
 *     each message
 *
 * @parm LPARAM | lParam2 | Data for this message.  Defined separately for
 *     each message
 *
 * @rdesc Defined separately for each message.
 ***************************************************************************/

LRESULT FAR PASCAL _loadds DriverProc(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    PRLEINST pri = dwDriverID != -1 ? (PRLEINST)dwDriverID : NULL;

    switch (uiMessage)
    {
        case DRV_LOAD:
#ifdef _WIN32
            if (ghModule) {
                // AVI explicitly loads us as well, but does not pass the
                // correct (as known by WINMM) driver handle.
            } else {
                ghModule = (HANDLE) GetDriverModuleHandle(hDriver);
            }
#endif
            RleLoad();
	    return (LRESULT)1L;

	case DRV_FREE:
            RleFree();
	    return (LRESULT)1L;

	case DRV_OPEN:
	    // if being opened with no open struct, then return a non-zero
            // value without actually opening

	    if (lParam2 == 0L)
                return -1l;

            return (LRESULT)(DWORD_PTR)(UINT_PTR)RleOpen();

	case DRV_CLOSE:
            if (pri)
                RleClose(pri);

	    return (LRESULT)1L;

	/*********************************************************************

	    state messages

	*********************************************************************/

	case ICM_GETSTATE:
            return RleGetState(pri, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_SETSTATE:
            return RleSetState(pri, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_GETINFO:
            return RleGetInfo(pri, (ICINFO FAR *)lParam1, (DWORD)lParam2);

        case ICM_GETDEFAULTQUALITY:
            if (lParam1)
            {
                *((LPDWORD)lParam1) = QUALITY_DEFAULT;
                return ICERR_OK;
            }
            break;
	
	/*********************************************************************

	    compression messages

	*********************************************************************/

	case ICM_COMPRESS_QUERY:
            return RleCompressQuery(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_BEGIN:
            return RleCompressBegin(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_FORMAT:
            return RleCompressGetFormat(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_SIZE:
            return RleCompressGetSize(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);
	
	case ICM_COMPRESS:
            return RleCompress(pri,
			    (ICCOMPRESS FAR *)lParam1, (DWORD)lParam2);

	case ICM_COMPRESS_END:
            return RleCompressEnd(pri);
	
	/*********************************************************************

	    decompress messages

	*********************************************************************/

	case ICM_DECOMPRESS_QUERY:
            return RleDecompressQuery(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS_BEGIN:
            return RleDecompressBegin(pri,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS_GET_FORMAT:
            return RleDecompressGetFormat(pri,
			 (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

	case ICM_DECOMPRESS:
            return RleDecompress(pri,
			 (ICDECOMPRESS FAR *)lParam1, (DWORD)lParam2);

	case ICM_DECOMPRESS_END:
            return RleDecompressEnd(pri);

	/*********************************************************************

	    standard driver messages

	*********************************************************************/

	case DRV_DISABLE:
	case DRV_ENABLE:
	    return (LRESULT)1L;

	case DRV_INSTALL:
	case DRV_REMOVE:
	    return (LRESULT)DRV_OK;
    }

    if (uiMessage < DRV_USER)
        return DefDriverProc(dwDriverID, hDriver, uiMessage,lParam1,lParam2);
    else
	return ICERR_UNSUPPORTED;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api int | LibMain | Library initialization code.
 *
 * @parm HANDLE | hModule | Our module handle.
 *
 * @parm WORD | wHeapSize | The heap size from the .def file.
 *
 * @parm LPSTR | lpCmdLine | The command line.
 *
 * @rdesc Returns 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
#ifndef _WIN32
int NEAR PASCAL LibMain(HMODULE hModule, WORD wHeapSize, LPSTR lpCmdLine)
{
    ghModule = hModule;

    return 1;
}
#endif

#if 0 // NO DLL load proc needed
    // NOTE: ghModule Will be set up on DRV_LOAD call
#ifdef _WIN32

BOOL WINAPI DLLEntryPoint(HINSTANCE hModule, ULONG Reason, LPVOID pv)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
	    ghModule = hModule;
            DisableThreadLibraryCalls(hModule);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif
#endif  // if 0
