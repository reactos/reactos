
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */



/* DDK/NDK/SDK Headers */
/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/ntifs.h>
#include <ddk/tvout.h>
#include <ndk/ntndk.h>



#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>

#include <ddk/ddkmapi.h>
#include "dxapi_driver.h"




NTSTATUS
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    /* 
     * NOTE this driver will never be load, it only contain export list
     * to win32k eng functions
     */
    return STATUS_SUCCESS;
}

NTSTATUS
GsDriverEntry(IN PVOID Context1,
              IN PVOID Context2)
{
    return DriverEntry(Context1, Context2);
}



ULONG
DxApiGetVersion()
{
    /* MSDN say this always return Direct Sound version 4.02 */
    return 0x402;
}


/* protype from dxapi.h and ddkmapi.h from ddk, MSDN does not provide protype for this api, only which
 * functions it support, if u search in msdn you found full documations for each function 
 * for each functions. 
 */

DWORD
DxApi(ULONG dwFunctionNum,
      PVOID lpvInBuffer,
      ULONG cbInBuffer,
      PVOID lpvOutBuffer,
      ULONG cbOutBuffer)
{

    dwFunctionNum -= DD_FIRST_DXAPI;

    if ((lpvOutBuffer == NULL) || 
       (dwFunctionNum < (DD_FIRST_DXAPI - DD_FIRST_DXAPI)) ||
       (dwFunctionNum > (DD_DXAPI_FLUSHVPCAPTUREBUFFERS - DD_FIRST_DXAPI)) ||
       (gDxApiEntryPoint[dwFunctionNum].pfn == NULL) ||
       (cbInBuffer != tblCheckInBuffer[dwFunctionNum]) ||
       (cbOutBuffer != tblCheckOutBuffer[dwFunctionNum]))

    {
        return 0;
    }

    gDxApiEntryPoint[dwFunctionNum].pfn(lpvInBuffer, lpvOutBuffer);

    return 0;
}

VOID
DxGetVersionNumber(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxCloseHandle(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxOpenDirectDraw(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxOpenSurface(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxOpenVideoPort(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetKernelCaps(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetFieldNumber(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxSetFieldNumber(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxSetSkipPattern(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetSurfaceState(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxSetSurfaceState(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxLock(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxFlipOverlay(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxFlipVideoPort(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetCurrentAutoflip(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetPreviousAutoflip(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxRegisterEvent(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxUnregisterEvent(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxGetPolarity(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxOpenVpCatureDevice(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxAddVpCaptureBuffer(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}

VOID
DxFlushVpCaptureBuffs(PVOID lpvInBuffer, PVOID lpvOutBuffer)
{
    /* FIXME Unimplement */
}



