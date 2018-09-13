/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    dmgrinfo.c
**
** Purpose: Defines the CleanupMgrInfo class for the property tab
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

#include <stdio.h>
#include <string.h>

#ifdef DEBUG

void
DebugPrint(
    HRESULT hr,
    LPCSTR  lpFormat,
    ...
    )
{
    va_list marker;
    CHAR    MessageBuffer[512];
    void    *pMsgBuf = NULL;
    
    va_start(marker, lpFormat);
    vsprintf(MessageBuffer, lpFormat, marker);
    va_end(marker);

    if (hr != 0)
    {                       
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsgBuf,
            0, NULL);
                 
        wsprintfA(MessageBuffer, "%s %X (%s)", MessageBuffer, hr, (LPTSTR)pMsgBuf);                 
    }
    
    OutputDebugStringA("CLEANMGR: ");
    OutputDebugStringA(MessageBuffer);
    OutputDebugStringA("\r\n");

#ifdef MESSAGEBOX                        
    MessageBoxA(NULL, MessageBuffer, "CLEANMGR DEBUG MESSAGE", MB_OK);
#endif
                        
    LocalFree(pMsgBuf);
}

#endif  //DEBUG
