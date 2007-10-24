
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
* The function DxDxgGenericThunk redirect dx call to other thing.
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
* always return DDHAL_DRIVER_NOTHANDLED
*
* @remarks.
* This api are not longer use in Windows NT 2000/XP/2003
*
*--*/
DWORD
STDCALL
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
* The function DxDdIoctl is the ioctl call to diffent dx functions 
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
* always return DDERR_UNSUPPORTED
*
* @remarks.
* This api are not longer use in Windows NT 2000/XP/2003
*
*--*/
DWORD
STDCALL
DxDdIoctl(ULONG ulIoctl,
          PVOID pBuffer,
          ULONG ulBufferSize)
{
    return DDERR_UNSUPPORTED;
}

