/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/virtual.c
 * PURPOSE:              Passing the Virtualxxx functions onto the kernel
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType)
{
   return(VirtualFreeEx(GetCurrentProcess(),lpAddress,dwSize,dwFreeType));
}

