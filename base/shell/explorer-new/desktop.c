/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

typedef struct _DESKCREATEINFO
{
    HANDLE hEvent;
    ITrayWindow *Tray;
    HANDLE hDesktop;
} DESKCREATEINFO, *PDESKCREATEINFO;

static DWORD CALLBACK
DesktopThreadProc(IN OUT LPVOID lpParameter)
{
    volatile DESKCREATEINFO *DeskCreateInfo = (volatile DESKCREATEINFO *)lpParameter;
    IShellDesktopTray *pSdt;
    HANDLE hDesktop;
    HRESULT hRet;

    OleInitialize(NULL);

    hRet = ITrayWindow_QueryInterface(DeskCreateInfo->Tray,
                                      &IID_IShellDesktopTray,
                                      (PVOID*)&pSdt);
    if (!SUCCEEDED(hRet))
        return 1;

    hDesktop = SHCreateDesktop(pSdt);

    IShellDesktopTray_Release(pSdt);
    if (hDesktop == NULL)
        return 1;

    (void)InterlockedExchangePointer(&DeskCreateInfo->hDesktop,
                                     hDesktop);

    if (!SetEvent(DeskCreateInfo->hEvent))
    {
        /* Failed to notify that we initialized successfully, kill ourselves
           to make the main thread wake up! */
        return 1;
    }

    SHDesktopMessageLoop(hDesktop);

    /* FIXME: Properly rundown the main thread! */
    ExitProcess(0);

    return 0;
}

HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray)
{
    HANDLE hThread;
    HANDLE hEvent;
    DWORD DesktopThreadId;
    HANDLE hDesktop = NULL;
    HANDLE Handles[2];
    DWORD WaitResult;

    hEvent = CreateEvent(NULL,
                         FALSE,
                         FALSE,
                         NULL);
    if (hEvent != NULL)
    {
        volatile DESKCREATEINFO DeskCreateInfo;

        DeskCreateInfo.hEvent = hEvent;
        DeskCreateInfo.Tray = Tray;
        DeskCreateInfo.hDesktop = NULL;

        hThread = CreateThread(NULL,
                               0,
                               DesktopThreadProc,
                               (PVOID)&DeskCreateInfo,
                               0,
                               &DesktopThreadId);
        if (hThread != NULL)
        {
            Handles[0] = hThread;
            Handles[1] = hEvent;

            for (;;)
            {
                WaitResult = MsgWaitForMultipleObjects(sizeof(Handles) / sizeof(Handles[0]),
                                                       Handles,
                                                       FALSE,
                                                       INFINITE,
                                                       QS_ALLEVENTS);
                if (WaitResult == WAIT_OBJECT_0 + (sizeof(Handles) / sizeof(Handles[0])))
                    TrayProcessMessages(Tray);
                else if (WaitResult != WAIT_FAILED && WaitResult != WAIT_OBJECT_0)
                {
                    hDesktop = DeskCreateInfo.hDesktop;
                    break;
                }
            }

            CloseHandle(hThread);
        }

        CloseHandle(hEvent);
    }

    return hDesktop;
}

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop)
{
    return;
}
