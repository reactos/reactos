
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

#include <dxg_int.h>

/*++
* @name DxDxgGenericThunk
* @implemented
*
* The function DxDxgGenericThunk redirects DirectX calls to other functions.
*
* @param ULONG_PTR ulIndex
* The functions we want redirct
*
* @param ULONG_PTR ulHandle
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr1
* Unknown
*
* @param PVOID pvPtr1
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr2
* Unknown
*
* @param PVOID pvPtr2
* Unknown
*
* @return 
* Always returns DDHAL_DRIVER_NOTHANDLED
*
* @remarks.
* This function is no longer used in Windows NT 2000/XP/2003
*
*--*/
DWORD
NTAPI
DxDxgGenericThunk(ULONG_PTR ulIndex,
                  ULONG_PTR ulHandle,
                  SIZE_T *pdwSizeOfPtr1,
                  PVOID pvPtr1,
                  SIZE_T *pdwSizeOfPtr2,
                  PVOID pvPtr2)
{
    return DDHAL_DRIVER_NOTHANDLED;
}


/*++
* @name DxDdIoctl
* @implemented
*
* The function DxDdIoctl is the ioctl call to different DirectX functions 
*
* @param ULONG ulIoctl
* The ioctl code that we want call to
*
* @param PVOID pBuffer
* Our in or out buffer with data to the ioctl code we are using
*
* @param ULONG ulBufferSize
* The buffer size in bytes
*
* @return 
* Always returns DDERR_UNSUPPORTED
*
* @remarks.
* This function is no longer used in Windows NT 2000/XP/2003
*
*--*/
DWORD
NTAPI
DxDdIoctl(ULONG ulIoctl,
          PVOID pBuffer,
          ULONG ulBufferSize)
{
    return DDERR_UNSUPPORTED;
}

