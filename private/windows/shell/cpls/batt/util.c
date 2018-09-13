
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Battery Class Installer utility routines

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/


#include "proj.h"




#if defined(DEBUG)


DWORD   BattDebugPrintLevel = TF_ERROR | TF_WARNING;



void 
CPUBLIC
CommonDebugMsgW(
    DWORD       Flag,
    LPCSTR      Message,
    ...
    )

/*++

Routine Description:

    This is the wide char version of CommonDebugMsgA

Arguments:

    Flag                - Debug print level for message

    Message             - Format string for message

    ...                 - Arguments for the format string 

Return Value:

    

--*/
{
    WCHAR               tmpBuffer[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
    va_list             variableArgs;

    
    
    if (Flag & BattDebugPrintLevel)
    {
        int         wideCharCount;
        int         tmpInt;
        WCHAR       wideBuffer[MAX_BUF];

        lstrcpyW (tmpBuffer, L"BATTERY CLASS INSTALLER: ");
        wideCharCount = lstrlenW (tmpBuffer);
        va_start(variableArgs, Message);

        // (We convert the string, rather than simply input an 
        // LPCWSTR parameter, so the caller doesn't have to wrap
        // all the string constants with the TEXT() macro.)

        tmpInt = MultiByteToWideChar(CP_ACP, 0, Message, lstrlenA (Message), wideBuffer, MAX_BUF);
        if (!tmpInt) {
            lstrcatW (tmpBuffer, L"Debug string too long to print\n");
        
        } else {
            wvsprintfW (&tmpBuffer[wideCharCount-sizeof(WCHAR)], wideBuffer, variableArgs);
        }

        va_end(variableArgs);
        OutputDebugStringW(tmpBuffer);
    }
}






void 
CPUBLIC
CommonDebugMsgA(
    DWORD       Flag,
    LPCSTR      Message,
    ...
    )

/*++

Routine Description:

    Debug spew

Arguments:

    Flag                - Debug print level for message

    Message             - Format string for message

    ...                 - Arguments for the format string 

Return Value:

    

--*/
{
    UCHAR               tmpBuffer[DEBUG_PRINT_BUFFER_LEN];    // Largest path plus extra
    va_list             variableArgs;

    
    if (Flag & BattDebugPrintLevel)
    {
        int         charCount;

        lstrcpyA (tmpBuffer, "BATTERY CLASS INSTALLER: ");
        charCount = lstrlenA (tmpBuffer);
        va_start(variableArgs, Message);

        wvsprintfA (&tmpBuffer[charCount-1], Message, variableArgs);
        va_end(variableArgs);
        OutputDebugStringA(tmpBuffer);
    }
}


#endif      // #if defined(DEBUG)
