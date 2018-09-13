/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    srvvdm.c

Abstract:

    This file contains all VDM functions

Author:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


ULONG
SrvVDMConsoleOperation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_VDM_MSG a = (PCONSOLE_VDM_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (!(Console->Flags & CONSOLE_VDM_REGISTERED) ||
        (Console->VDMProcessId != CONSOLE_CLIENTPROCESSID())) {
        Status = STATUS_INVALID_PARAMETER;
    } else {
        switch (a->iFunction) {
            case VDM_HIDE_WINDOW:
                PostMessage(Console->hWnd,
                             CM_HIDE_WINDOW,
                             0,
                             0
                           );
                break;
            case VDM_IS_ICONIC:
                a->Bool = IsIconic(Console->hWnd);
                break;
            case VDM_CLIENT_RECT:
                GetClientRect(Console->hWnd,&a->Rect);
                break;
            case VDM_CLIENT_TO_SCREEN:
                ClientToScreen(Console->hWnd,&a->Point);
                break;
            case VDM_SCREEN_TO_CLIENT:
                ScreenToClient(Console->hWnd,&a->Point);
                break;
            case VDM_IS_HIDDEN:
                a->Bool = ((Console->Flags & CONSOLE_NO_WINDOW) != 0);
                break;
            case VDM_FULLSCREEN_NOPAINT:
                if (a->Bool) {
                    Console->Flags |= CONSOLE_FULLSCREEN_NOPAINT;
                } else {
                    Console->Flags &= ~CONSOLE_FULLSCREEN_NOPAINT;
                }
                break;
#if defined(FE_SB)
            case VDM_SET_VIDEO_MODE:
                Console->fVDMVideoMode = (a->Bool != 0);
                break;
#if defined(i386)
            case VDM_SAVE_RESTORE_HW_STATE:
                if (ISNECPC98(gdwMachineId)) {
                    // This function is used by MVDM to save/restore HW state
                    // when it executes DOS-AP on Fullscreen again.
                    // It is called from MVDM\SOFTPC\HOST\SRC\NT_FULSC.C.
                    VIDEO_HARDWARE_STATE State;
                    ULONG StateSize = sizeof(State);

                    State.StateHeader = Console->StateBuffer;
                    State.StateLength = Console->StateLength;


                    Status = GdiFullscreenControl(a->Bool ? FullscreenControlRestoreHardwareState
                                                          : FullscreenControlSaveHardwareState,
                                                  &State,
                                                  StateSize,
                                                  &State,
                                                  &StateSize);
                }
                break;
            case VDM_VIDEO_IOCTL:
                if (ISNECPC98(gdwMachineId)) {
                    // This function is used by MVDM to access CG.
                    // It is called from MVDM\SOFTPC\HOST\SRC\NT_CGW.C.
                    PVOID InBuffer;

                    if (!CsrValidateMessageBuffer(m, &a->VDMIoctlParam.lpvInBuffer, a->VDMIoctlParam.cbInBuffer, sizeof(BYTE)) ||
                        !CsrValidateMessageBuffer(m, &a->VDMIoctlParam.lpvOutBuffer, a->VDMIoctlParam.cbOutBuffer, sizeof(BYTE))) {
                        UnlockConsole(Console);
                        return STATUS_INVALID_PARAMETER;
                    }
                
                    InBuffer = ConsoleHeapAlloc(
                                         MAKE_TAG( TMP_DBCS_TAG ),
                                         a->VDMIoctlParam.cbInBuffer + sizeof(DWORD)
                                        );
                    if (InBuffer == NULL)
                    {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }

                    *((PDWORD)InBuffer) = a->VDMIoctlParam.dwIoControlCode;
                    RtlCopyMemory((PBYTE)InBuffer + sizeof(DWORD),
                                  a->VDMIoctlParam.lpvInBuffer,
                                  a->VDMIoctlParam.cbInBuffer
                                 );

                    Status = GdiFullscreenControl(
                                 FullscreenControlSpecificVideoControl,
                                 InBuffer,
                                 ConsoleHeapSize(InBuffer),
                                 a->VDMIoctlParam.lpvOutBuffer,
                                 a->VDMIoctlParam.lpvOutBuffer ?
                                     &a->VDMIoctlParam.cbOutBuffer : 0
                                 );

                    ConsoleHeapFree(InBuffer);
                }
                break;
#endif
#endif
            default:
                ASSERT(FALSE);
        }
    }

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}
