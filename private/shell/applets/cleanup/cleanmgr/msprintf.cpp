/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    msprintf.cpp
**
** Purpose: Print functions
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "msprintf.h"
#include "resource.h"
#include "diskutil.h"       // cb1MEG




/*
**------------------------------------------------------------------------------
** Function definitions
**------------------------------------------------------------------------------
*/

TCHAR * cdecl SHFormatMessage( DWORD dwMessageId, ...)
{
    va_list   arg;
    va_start (arg, dwMessageId);
    LPVOID pBuffer = NULL;

    // use format message to build the string...
    DWORD dwRes = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                                NULL, dwMessageId, 0, (LPTSTR) & pBuffer, 0, &arg );
    return (TCHAR *) pBuffer;                  
}

