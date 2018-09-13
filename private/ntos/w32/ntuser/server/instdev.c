
/**************************************************************************\
* Module Name: instdev.c
*
* Server device handling routine for the CSR stuff.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Created: 13-Mar-97
*
* History:
*   13-Mar-97 created by PaulaT
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"
#include "dbt.h"
#include "pnpmgr.h"


/**************************************************************************\
* SrvDeviceEvent
*
* User-mode pnp manager (in services.exe) has a message to deliver to an
* app that has registered for this notification but services.exe isn't
* in WinSta0\Default so we need a csr thread to simply send the message.
*
* PaulaT 06/04/97
*
\**************************************************************************/

ULONG
SrvDeviceEvent(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICEEVENTMSG a = (PDEVICEEVENTMSG)&m->u.ApiMessageData;
    USERTHREAD_USEDESKTOPINFO utudi;

    UNREFERENCED_PARAMETER(ReplyStatus);

    try {

        //
        // Set the desktop to the active desktop before sending the
        // message.
        //

        utudi.hThread = NULL;
        utudi.drdRestore.pdeskRestore = NULL;
        Status = NtUserSetInformationThread(NtCurrentThread(),
                                            UserThreadUseActiveDesktop,
                                            &utudi, sizeof(utudi));
        if (!NT_SUCCESS(Status)) {
            #if DBG
            KdPrint(("--> SrvDeviceEvent: NtUserSetInformationThread failed %d\n", Status));
            #endif
            goto Clean1;
        }

        //
        // Verify the window handle is still valid, if not, let the caller know
        // so it can be purged from the notification window hand list that the
        // user-mode pnp manager keeps.
        //

        if ((a->hWnd != HWND_BROADCAST) && !IsWindow(a->hWnd)) {
            Status = STATUS_INVALID_HANDLE;
            goto Clean0;
        }

        if (a->dwFlags) {

            //
            // This is a query so we have to send the message but use
            // timeouts so an app can't stall us forever.
            //

            RIPMSG3(RIP_VERBOSE, "--> SrvDeviceEvent: Sending WM_DEVICECHANGE to 0x%x, w 0x%x, l 0x%x",
                    (ULONG_PTR)a->hWnd,
                    a->wParam,
                    a->lParam);

            if (!SendMessageTimeout(a->hWnd, WM_DEVICECHANGE, a->wParam, a->lParam,
                                    SMTO_ABORTIFHUNG | SMTO_NORMAL,
                                    PNP_NOTIFY_TIMEOUT, &a->dwResult)) {
                Status = STATUS_UNSUCCESSFUL;
            }

        } else {

            //
            // It's not a query so just post it and return, we don't
            // care what the app returns.
            //

            RIPMSG3(RIP_VERBOSE, "--> SrvDeviceEvent: Posting WM_DEVICECHANGE to 0x%x, w 0x%x, l 0x%x",
                    (ULONG_PTR)a->hWnd,
                    a->wParam,
                    a->lParam);

            if (!PostMessage(a->hWnd, WM_DEVICECHANGE, a->wParam, a->lParam)) {
                Status = STATUS_UNSUCCESSFUL;
            }
        }

    Clean0:

        //
        // Reset this thread's desktop back to NULL before returning. This
        // decrements the desktop's reference count.
        //

        NtUserSetInformationThread(NtCurrentThread(),
                                   UserThreadUseDesktop,
                                   &utudi, sizeof(utudi));

    Clean1:
        ;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrint(("SrvDeviceEvent generated an exception (%d)\n", GetExceptionCode()));
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;

} // SrvDeviceEvent
