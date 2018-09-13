
/*************************************************************************
*
* sendmsg.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Terminal Server (Hydra) specific code
*
* Processend message to winstation
*
* $Author:  Ara bernardi
*
*************************************************************************/

//
// Includes
//

#include "precomp.h"
#pragma hdrstop

#include "dbt.h"
#include "ntdddisk.h"
#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>
#include <winuser.h>


//
// Global variables
//

#if DBG
void DumpOutLastErrorString()
{
    LPVOID  lpMsgBuf;

    DWORD   error = GetLastError();

    DBGHYD(("GetLastError() = 0x%lx \n", error ));

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
        );
        //
        // Process any inserts in lpMsgBuf.
        // ...
        // Display the string.
        //
        DBGHYD(("%s\n", (LPCTSTR)lpMsgBuf ));

        //
        // Free the buffer.
        //
        LocalFree( lpMsgBuf );
}
#endif

#if DBG
#define DumpOutLastError    DumpOutLastErrorString()
#else
#define DumpOutLastError
#endif


/*******************************************************************************
 *
 *  RemoteDoBrroadcastSystemMessage
 *
 * ENTRY:
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/

NTSTATUS
RemoteDoBroadcastSystemMessage(
    PWINSTATION_APIMSG pMsg)
{
    LONG rc;
    WINSTATIONBROADCASTSYSTEMMSG     *pmsg;
    LPARAM      tmpLPARAM;
    NTSTATUS    status;


    pmsg = &(pMsg->u.bMsg);

    if ( pmsg->bufferSize )
    {
        // we have a databuffer, set the lParam to our copied data buffer
        tmpLPARAM = (LPARAM)pmsg->dataBuffer;
    }
    else
    {
        tmpLPARAM = pmsg->lParam ;
    }

    rc = BroadcastSystemMessage( pmsg->dwFlags, &pmsg->dwRecipients,
                    pmsg->uiMessage, pmsg->wParam, tmpLPARAM );

    status = STATUS_SUCCESS;

    pmsg->Response = rc;

    return status ;
}



NTSTATUS
RemoteDoSendWindowMessage(
    PWINSTATION_APIMSG pMsg)
{
    LONG rc;
    NTSTATUS status;


    WINSTATIONSENDWINDOWMSG  *pmsg;
    LPARAM  tmpLPARAM;

    pmsg = &(pMsg->u.sMsg);

    if ( pmsg->bufferSize )
    {
        // we have a databuffer, set the lParam to our copied data buffer
        tmpLPARAM = (LPARAM)pmsg->dataBuffer;
    }
    else
    {
        tmpLPARAM = (LPARAM)pmsg->lParam;
    }

    //
    // No need to worry about disconnected sessions (desktop), since msg is sent to a specific hwnd.
    // I have verified this imperically.
    //

    rc = (LONG)SendMessage( pmsg->hWnd, pmsg->Msg,
                    pmsg->wParam, tmpLPARAM  );

    status = STATUS_SUCCESS;

    pmsg->Response = rc;

    return status ;
}

