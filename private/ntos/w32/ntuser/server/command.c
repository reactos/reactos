
/*************************************************************************
*
* command.c
*
* WinStation command channel handler
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* $Author:
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>

#include <icadd.h>

extern HANDLE G_IcaVideoChannel;
extern HANDLE G_IcaCommandChannel;
extern HANDLE WinStationIcaApiPort;


extern NTSTATUS BrokenConnection(BROKENCLASS, BROKENSOURCECLASS);
extern NTSTATUS ShadowHotkey(VOID);

NTSTATUS
Win32CommandChannelThread(
    IN PVOID ThreadParameter)
{
    ICA_CHANNEL_COMMAND Command;
    PTEB                Teb;
    ULONG               ActualLength;
    NTSTATUS            Status;
    ULONG               Error;
    OVERLAPPED          Overlapped;

    /*
     * Initialize GDI accelerators.  Identify this thread as a server thread.
     */
    Teb = NtCurrentTeb();
    Teb->GdiClientPID = 4; // PID_SERVERLPC
    Teb->GdiClientTID = HandleToUlong(Teb->ClientId.UniqueThread);

    for ( ; ; ) {

        memset(&Overlapped, 0, sizeof(Overlapped));

        if (!ReadFile(G_IcaCommandChannel,
                      &Command,
                      sizeof(Command),
                      &ActualLength,
                      &Overlapped)) {

            Error = GetLastError();

            if (Error == ERROR_IO_PENDING) {
                
                /*
                 * check on the results of the asynchronous read
                 */
                if (!GetOverlappedResult(G_IcaCommandChannel, &Overlapped,
                                         &ActualLength, TRUE)) {
                    // wait for result

                    DBGHYD(("Command Channel: Error 0x%x from GetOverlappedResult\n",
                           GetLastError()));
                    break;
                }
            } else {
                DBGHYD(("Command Channel: Error 0x%x from ReadFile\n",
                       Error));
                break;
            }
        }

        if (ActualLength < sizeof(ICA_COMMAND_HEADER)) {
            
            DBGHYD(("Command Channel Thread bad length 0x%x\n",
                   ActualLength));
            continue;
        }
        
        switch (Command.Header.Command) {

        case ICA_COMMAND_BROKEN_CONNECTION:
            /*
             * broken procedure
             */
            Status = BrokenConnection(
                        Command.BrokenConnection.Reason,
                        Command.BrokenConnection.Source);
            
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("BrokenConnection failed with Status 0x%x\n",
                       Status));
            }
            break;

        case ICA_COMMAND_REDRAW_RECTANGLE:
            /*
             * setfocus ???
             */
            if (ActualLength < sizeof(ICA_COMMAND_HEADER) + sizeof(ICA_REDRAW_RECTANGLE)) {
                
                DBGHYD(("Command Channel: redraw rect bad length %d\n",
                       ActualLength));
                break;
            }
            Status = NtUserRemoteRedrawRectangle(
                         Command.RedrawRectangle.Rect.Left,
                         Command.RedrawRectangle.Rect.Top,
                         Command.RedrawRectangle.Rect.Right,
                         Command.RedrawRectangle.Rect.Bottom);

            if (!NT_SUCCESS(Status)) {
                DBGHYD(("NtUserRemoteRedrawRectangle failed with Status 0x%x\n",
                       Status));
            }
            break;

        case ICA_COMMAND_REDRAW_SCREEN: // setfocus
            
            Status = NtUserRemoteRedrawScreen();
            
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("NtUserRemoteRedrawScreen failed with Status 0x%x\n",
                       Status));
            }
            break;

        case ICA_COMMAND_STOP_SCREEN_UPDATES: // killfocus
            
            Status = NtUserRemoteStopScreenUpdates();
            
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("NtUserRemoteStopScreenUpdates failed with Status 0x%x\n",
                       Status));
            } else {
                IO_STATUS_BLOCK IoStatus;

                NtDeviceIoControlFile( G_IcaVideoChannel,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatus,
                                       IOCTL_VIDEO_ICA_STOP_OK,
                                       NULL,
                                       0,
                                       NULL,
                                       0);
            }
            break;

        case ICA_COMMAND_SHADOW_HOTKEY: // shadow hotkey
            
            Status = ShadowHotkey();
            
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("ShadowHotkey failed with Status 0x%x\n",
                       Status));
            }
            break;

        case ICA_COMMAND_DISPLAY_IOCTL:
            
            Status = NtUserCtxDisplayIOCtl(
                         Command.DisplayIOCtl.DisplayIOCtlFlags,
                         &Command.DisplayIOCtl.DisplayIOCtlData[0],
                         Command.DisplayIOCtl.cbDisplayIOCtlData);
            
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("NtUserCtxDisplayIOCtl failed with Status 0x%x\n",
                       Status));
            }
            break;

        default:
            
            DBGHYD(("Command Channel: Bad Command 0x%x\n",
                     Command.Header.Command));
            break;
        }

    }

    /*
     * Close command channel LPC port if there is one.
     */
    if (WinStationIcaApiPort) {
        NtClose(WinStationIcaApiPort);
        WinStationIcaApiPort = NULL;

    }


    ExitThread(0);
    /*
     * Make the compiler happy
     */
    return STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER(ThreadParameter);
}
