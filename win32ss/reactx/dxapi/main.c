
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */


#include "dxapi_driver.h"

#define NDEBU /* debug prints are enabled, add a G at the end to disable it ;-) */
#include <debug.h>

NTSTATUS NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    /*
     * NOTE this driver will never be load, it only contain export list
     * to win32k eng functions
     */
    return STATUS_SUCCESS;
}

/*++
* @name DxApiGetVersion
* @implemented
*
* The function DxApiGetVersion return the dsound version, and it always return 4.02
*
* @return
* Always return 4.02
*
* @remarks.
* none
*
*--*/
ULONG
NTAPI
DxApiGetVersion()
{
    /* MSDN say this always return Direct Sound version 4.02 */
    return 0x402;
}



/*++
* @name DxApi
* @implemented
*
* The function DxApi calls to diffent functions, follow functions
* are supported
* DxGetVersionNumber, DxCloseHandle, DxOpenDirectDraw, DxOpenSurface,
* DxOpenVideoPort, DxGetKernelCaps, DxGetFieldNumber, DxSetFieldNumber,
* DxSetSkipPattern, DxGetSurfaceState, DxSetSurfaceState, DxLock,
* DxFlipOverlay, DxFlipVideoPort, DxGetCurrentAutoflip, DxGetPreviousAutoflip,
* DxRegisterEvent, DxUnregisterEvent, DxGetPolarity, DxOpenVpCatureDevice,
* DxAddVpCaptureBuffer, DxFlushVpCaptureBuffs
*
* See ddkmapi.h as well

*
* @param ULONG dwFunctionNum
* The function id we want call on in the dxapi.sys see ddkmapi.h for the id
*
* @param PVOID lpvInBuffer
* Our input buffer to the functions we call to, This param can be NULL
*
* @param ULONG cbInBuffer
* Our size in bytes of the input buffer, rember wrong size will result in the function
* does not being call.
*
* @param PVOID lpvOutBuffer
* Our Output buffer, there the function fill in the info, this param can not
* be null. if it null the functions we trying call on will not be call
*
* @param ULONG cbOutBuffer
* Our size in bytes of the output buffer, rember wrong size will result in the function
* does not being call.
*
* @return
* Return Always 0.
*
* @remarks.
* before call to any of this functions, do not forget set lpvOutBuffer->ddRVal = DDERR_GEN*,
* if that member exists in the outbuffer ;
*
*--*/

DWORD
NTAPI
DxApi(IN DWORD dwFunctionNum,
      IN LPVOID lpvInBuffer,
      IN DWORD cbInBuffer,
      OUT LPVOID lpvOutBuffer,
      OUT DWORD cbOutBuffer)
{

    dwFunctionNum -= DD_FIRST_DXAPI;

    if ((lpvOutBuffer == NULL) ||
       /*(dwFunctionNum < (DD_FIRST_DXAPI - DD_FIRST_DXAPI)) ||*/
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
NTAPI
DxApiInitialize (
    PVOID p1,
    PVOID p2,
    PVOID p3,
    PVOID p4,
    PVOID p5,
    PVOID p6,
    PVOID p7,
    PVOID p8)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
DxAutoflipUpdate (
    PVOID p1,
    PVOID p2,
    PVOID p3,
    PVOID p4,
    PVOID p5)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
DxEnableIRQ (
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
DxLoseObject (
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
DxUpdateCapture (
    PVOID p1,
    PVOID p2,
    PVOID p3)
{
    UNIMPLEMENTED;
}


/*++
* @name DxGetVersionNumber
* @implemented
*
* The function DxGetVersionNumber return dxapi interface version, that is 1.0
*
* @return
* Always return 1.0
*
* @remarks.
* none
*
*--*/
VOID
DxGetVersionNumber(PVOID lpvInBuffer, LPDDGETVERSIONNUMBER lpvOutBuffer)
{
    lpvOutBuffer->ddRVal = DD_OK;
    lpvOutBuffer->dwMajorVersion = 1;
    lpvOutBuffer->dwMinorVersion = 0;
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



