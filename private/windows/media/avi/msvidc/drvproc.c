/*----------------------------------------------------------------------+
|									|
| drvproc.c - driver procedure						|
|									|
| Copyright (c) 1990-1994 Microsoft Corporation.			|
| Portions Copyright Media Vision Inc.					|
| All Rights Reserved.							|
|									|
| You have a non-exclusive, worldwide, royalty-free, and perpetual	|
| license to use this source code in developing hardware, software	|
| (limited to drivers and other software required for hardware		|
| functionality), and firmware for video display and/or processing	|
| boards.   Microsoft makes no warranties, express or implied, with	|
| respect to the Video 1 codec, including without limitation warranties	|
| of merchantability or fitness for a particular purpose.  Microsoft	|
| shall not be liable for any damages whatsoever, including without	|
| limitation consequential damages arising from your use of the Video 1	|
| codec.								|
|									|
+----------------------------------------------------------------------*/
#include <windows.h>
#include <win32.h>
#include <mmsystem.h>

#ifndef _INC_COMPDDK
#define _INC_COMPDDK    50      /* version number */
#endif

#include <vfw.h>
#include "msvidc.h"
#ifdef _WIN32
//#include <mmddk.h>
//LONG   FAR PASCAL DefDriverProc(DWORD dwDriverIdentifier, HANDLE driverID, UINT message, LONG lParam1, LONG lParam2);
#endif

HMODULE ghModule = NULL;

/***************************************************************************
 * DriverProc  -  The entry point for an installable driver.
 *
 * PARAMETERS
 * dwDriverId:  For most messages, <dwDriverId> is the DWORD
 *     value that the driver returns in response to a <DRV_OPEN> message.
 *     Each time that the driver is opened, through the <DrvOpen> API,
 *     the driver receives a <DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <dwDriverId>.
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
 * hDriver: This is the handle returned to the application by the
 *    driver interface.
 *
 * uiMessage: The requested action to be performed. Message
 *     values below <DRV_RESERVED> are used for globally defined messages.
 *     Message values from <DRV_RESERVED> to <DRV_USER> are used for
 *     defined driver protocols. Messages above <DRV_USER> are used
 *     for driver specific messages.
 *
 * lParam1: Data for this message.  Defined separately for
 *     each message
 *
 * lParam2: Data for this message.  Defined separately for
 *     each message
 *
 * RETURNS
 *   Defined separately for each message.
 *
 ***************************************************************************/

#ifdef _WIN32
// rely on whoever is loading us to synchronize load/free
UINT LoadCount = 0;
#endif


LRESULT FAR PASCAL _LOADDS DriverProc(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2)
{
    INSTINFO *pi = (INSTINFO *)dwDriverID;

    LPBITMAPINFOHEADER lpbiIn;
    LPBITMAPINFOHEADER lpbiOut;
    ICDECOMPRESSEX FAR *px;
#ifdef _WIN32
    LRESULT lres;
#endif
    
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
	    lres = VideoLoad();
            if (lres) {
                ++LoadCount;
            }
	    return lres;
#else
	    return (LRESULT) VideoLoad();
#endif

	case DRV_FREE:
	    VideoFree();
#ifdef _WIN32
            if (--LoadCount) {
            } else {
                ghModule = NULL;
            }
#endif
	    return (LRESULT)1L;

        case DRV_OPEN:
	    // if being opened with no open struct, then return a non-zero
	    // value without actually opening
	    if (lParam2 == 0L)
                return 0xFFFF0000;

	    return (LRESULT)(DWORD_PTR)(UINT_PTR)VideoOpen((ICOPEN FAR *) lParam2);

	case DRV_CLOSE:
#ifdef _WIN32
	    if (dwDriverID != 0xFFFF0000)
#else
	    if (pi)
#endif
		VideoClose(pi);

	    return (LRESULT)1L;

	/*********************************************************************

	    state messages

	*********************************************************************/

        case DRV_QUERYCONFIGURE:    // configuration from drivers applet
            return (LRESULT)0L;

        case DRV_CONFIGURE:
            return DRV_OK;

        case ICM_CONFIGURE:
            //
            //  return ICERR_OK if you will do a configure box, error otherwise
            //
            if (lParam1 == -1)
		return QueryConfigure(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
	    else
		return Configure(pi, (HWND)lParam1);

        case ICM_ABOUT:
            //
            //  return ICERR_OK if you will do a about box, error otherwise
            //
            if (lParam1 == -1)
		return QueryAbout(pi) ? ICERR_OK : ICERR_UNSUPPORTED;
	    else
		return About(pi, (HWND)lParam1);

	case ICM_GETSTATE:
	    return GetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_SETSTATE:
	    return SetState(pi, (LPVOID)lParam1, (DWORD)lParam2);

	case ICM_GETINFO:
            return GetInfo(pi, (ICINFO FAR *)lParam1, (DWORD)lParam2);

        case ICM_GETDEFAULTQUALITY:
            if (lParam1)
            {
                *((LPDWORD)lParam1) = 7500;
                return ICERR_OK;
            }
            break;

	/*********************************************************************

            get/set messages

	*********************************************************************/

        case ICM_GET:
            break;
	
	/*********************************************************************

	    compression messages

	*********************************************************************/

	case ICM_COMPRESS_QUERY:
	    return CompressQuery(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_BEGIN:
	    return CompressBegin(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_FORMAT:
	    return CompressGetFormat(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

	case ICM_COMPRESS_GET_SIZE:
	    return CompressGetSize(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);
	
	case ICM_COMPRESS:
	    return Compress(pi,
			    (ICCOMPRESS FAR *)lParam1, (DWORD)lParam2);

	case ICM_COMPRESS_END:
	    return CompressEnd(pi);

	case ICM_SET_STATUS_PROC:
	    // DPF(("ICM_SET_STATUS_PROC\n"));
	    pi->Status = ((ICSETSTATUSPROC FAR *) lParam1)->Status;
	    pi->lParam = ((ICSETSTATUSPROC FAR *) lParam1)->lParam;
	    return 0;
	
        /*********************************************************************

            decompress format query messages

        *********************************************************************/

        case ICM_DECOMPRESS_GET_FORMAT:
	    return DecompressGetFormat(pi,
                         (LPBITMAPINFOHEADER)lParam1,
                         (LPBITMAPINFOHEADER)lParam2);

        case ICM_DECOMPRESS_GET_PALETTE:
            return DecompressGetPalette(pi,
			 (LPBITMAPINFOHEADER)lParam1,
			 (LPBITMAPINFOHEADER)lParam2);

        /*********************************************************************

            decompress (old) messages, map these to the new (ex) messages

        *********************************************************************/

        case ICM_DECOMPRESS_QUERY:
            lpbiIn  = (LPBITMAPINFOHEADER)lParam1;
            lpbiOut = (LPBITMAPINFOHEADER)lParam2;

            return DecompressQuery(pi,0,
                    lpbiIn,NULL,
                    0,0,-1,-1,
                    lpbiOut,NULL,
                    0,0,-1,-1);

        case ICM_DECOMPRESS_BEGIN:
            lpbiIn  = (LPBITMAPINFOHEADER)lParam1;
            lpbiOut = (LPBITMAPINFOHEADER)lParam2;

            return DecompressBegin(pi,0,
                    lpbiIn,NULL,
                    0,0,-1,-1,
                    lpbiOut,NULL,
                    0,0,-1,-1);

        case ICM_DECOMPRESS:
            px = (ICDECOMPRESSEX FAR *)lParam1;

            return Decompress(pi,0,
                    px->lpbiSrc,px->lpSrc,
                    0, 0, -1, -1,
                    px->lpbiDst,px->lpDst,
                    0, 0, -1, -1);

	case ICM_DECOMPRESS_END:
            return DecompressEnd(pi);

        /*********************************************************************

            decompress (ex) messages

        *********************************************************************/

        case ICM_DECOMPRESSEX_QUERY:
            px = (ICDECOMPRESSEX FAR *)lParam1;

            return DecompressQuery(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX_BEGIN:
            px = (ICDECOMPRESSEX FAR *)lParam1;

            return DecompressBegin(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX:
            px = (ICDECOMPRESSEX FAR *)lParam1;

            return Decompress(pi,
                    px->dwFlags,
                    px->lpbiSrc,px->lpSrc,
                    px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
                    px->lpbiDst,px->lpDst,
                    px->xDst,px->yDst,px->dxDst,px->dyDst);

        case ICM_DECOMPRESSEX_END:
	    return DecompressEnd(pi);

	/*********************************************************************

	    draw messages

	*********************************************************************/

	case ICM_DRAW_BEGIN:
            return DrawBegin(pi,(ICDRAWBEGIN FAR *)lParam1, (DWORD)lParam2);

	case ICM_DRAW:
            return Draw(pi,(ICDRAW FAR *)lParam1, (DWORD)lParam2);

	case ICM_DRAW_END:
	    return DrawEnd(pi);
	
	/*********************************************************************

	    standard driver messages

	*********************************************************************/

	case DRV_DISABLE:
	    return (LRESULT)1L;
	case DRV_ENABLE:
	    return (LRESULT)1L;

	case DRV_INSTALL:
	    return (LRESULT)DRV_OK;
	case DRV_REMOVE:
	    return (LRESULT)DRV_OK;
    }

    if (uiMessage < DRV_USER)
        return DefDriverProc(dwDriverID, hDriver, uiMessage,lParam1,lParam2);
    else
	return ICERR_UNSUPPORTED;
}

#ifdef _WIN32

#if 0
BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    if (Reason == DLL_PROCESS_ATTACH) {
        ghModule = (HANDLE) hModule;
    }
    return TRUE;
}
#endif

#else

/****************************************************************************
 * LibMain - Library initialization code.
 *
 * PARAMETERS
 * hModule: Our module handle.
 *
 * wHeapSize: The heap size from the .def file.
 *
 * lpCmdLine: The command line.
 *
 * Returns 1 if the initialization was successful and 0 otherwise.
 ***************************************************************************/
int NEAR PASCAL LibMain(HMODULE hModule, WORD wHeapSize, LPSTR lpCmdLine)
{
    ghModule = hModule;

    return 1;
}

#endif
