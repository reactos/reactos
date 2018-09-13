/*
**------------------------------------------------------------------------------
** Module:  Disk Space Cleanup Property Sheets
** File:    midebug.cpp
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
    LPCTSTR  lpFormat,
    ...
    )
{
    va_list marker;
    TCHAR    MessageBuffer[512];
    void    *pMsgBuf;
    
    va_start(marker, lpFormat);
    wvsprintf(MessageBuffer, lpFormat, marker);
    va_end(marker);

    if (hr != 0)
    {                       
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsgBuf,
            0, NULL);
                 
        wsprintf(MessageBuffer, TEXT("%s %X (%s)"), MessageBuffer, hr, (LPTSTR)pMsgBuf);                 
        LocalFree(pMsgBuf);
    }
    
    OutputDebugString(TEXT("DATACLEN: "));
    OutputDebugString(MessageBuffer);
    OutputDebugString(TEXT("\r\n"));
}

#endif  //DEBUG
