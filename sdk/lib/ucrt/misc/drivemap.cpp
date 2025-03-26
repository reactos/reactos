/***
*drivemap.c - _getdrives
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _getdrives()
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <direct.h>

/***
*void _getdrivemap(void) - Get bit map of all available drives
*
*Purpose:
*
*Entry:
*
*Exit:
*       drive map with drive A in bit 0, B in 1, etc.
*
*Exceptions:
*
*******************************************************************************/

extern "C" unsigned long __cdecl _getdrives()
{
    return GetLogicalDrives();
}
