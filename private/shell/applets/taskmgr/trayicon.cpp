//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       trayicon.CPP
//
//  History:    Jan-27-96   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

// Queue for messages to be processed by the worker thread

#define MSG_QUEUE_SIZE 5
CTrayNotification * g_apQueue[MSG_QUEUE_SIZE] = { NULL };
CRITICAL_SECTION    g_CSTrayThread;
UINT                g_cQueueSize              = 0;

/*++ DeliverTrayNotification (MAIN THREAD CODE)

Routine Description:

   Adds a tray notification block to the list of things to be done
   by the tray notify worker thread
    
Arguments:

    pNot                - The CTrayNotification object to be queued

Returns:
    
    TRUE                - Message added to queue
    FALSE               - Queue full

Revision History:

    Mar-27-95 Davepl  Created
    May-28-99 Jonburs Don't ignore NIM_DELETE when queue is full
*/

BOOL DeliverTrayNotification(CTrayNotification * pNot)
{
    EnterCriticalSection(&g_CSTrayThread);

    // If no worker thread is running fail
    
    if (0 == g_idTrayThread)
    {
        LeaveCriticalSection(&g_CSTrayThread);
        return FALSE;
    }

    // If the queue is full, fail unless the new notification
    // is NIM_DELETE
    if (MSG_QUEUE_SIZE == g_cQueueSize)
    {
        if (NIM_DELETE != pNot->m_Message)
        {
            LeaveCriticalSection(&g_CSTrayThread);
            return FALSE;
        }
        else
        {
            // Replace last entry on the queue.  Do NOT post.
            delete g_apQueue[g_cQueueSize - 1];
            g_apQueue[g_cQueueSize - 1] = pNot;
            LeaveCriticalSection(&g_CSTrayThread);
            return TRUE;
        }
    }

    // Add notification to the queue and post a message to the
    // worker thread

    g_apQueue[g_cQueueSize++] = pNot;
    PostThreadMessage(g_idTrayThread, PM_NOTIFYWAITING, 0, 0);
    
    LeaveCriticalSection(&g_CSTrayThread);

    return TRUE;
}

/*++ TrayThreadMessageLoop (WORKER THREAD CODE)

Routine Description:

   Waits for messages telling it a notification packet is ready
   in the queue, then dispatches it to the tray  
    
   Mar-27-95 Davepl  Created
   May-28-99 Jonburs Check for NIM_DELETE during PM_QUITTRAYTHREAD

--*/

DWORD TrayThreadMessageLoop(LPVOID)
{
    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0))
    {
        switch(msg.message)
        {
            case PM_NOTIFYWAITING:
            {
                // Take a message out of the queue

                EnterCriticalSection(&g_CSTrayThread);
        
                ASSERT(g_cQueueSize);
                CTrayNotification * pNot = g_apQueue[0];
                for (UINT i = 0; i < g_cQueueSize; i++)
                {
                    g_apQueue[i] = g_apQueue[i+1];
                }
                g_cQueueSize--;

                LeaveCriticalSection(&g_CSTrayThread);

                // Give it to the tray to process.  If it blocks, our queue
                // will fill, but taskman itself won't hang

                Tray_NotifyIcon(pNot->m_hWnd,
                                pNot->m_uCallbackMessage,
                                pNot->m_Message,
                                pNot->m_hIcon,            
                                pNot->m_szTip);

                delete pNot;

                break;
            }

            case PM_QUITTRAYTHREAD:
            {
                // Delete all messages pending

                EnterCriticalSection(&g_CSTrayThread);                

                while (g_cQueueSize)
                {
                    // Need to check if a delete notification was on
                    // the queue to ensure that our icon gets removed
                    CTrayNotification * pNot = g_apQueue[g_cQueueSize - 1];

                    if (NIM_DELETE == pNot->m_Message)
                    {
                        Tray_NotifyIcon(pNot->m_hWnd,
                                        pNot->m_uCallbackMessage,
                                        pNot->m_Message,
                                        pNot->m_hIcon,            
                                        pNot->m_szTip);
                    }

                    delete pNot;
                    g_cQueueSize--;
                }

                g_idTrayThread = 0;
                LeaveCriticalSection(&g_CSTrayThread);
                DeleteCriticalSection(&g_CSTrayThread);

                PostQuitMessage(0);
                break;
            }

            default:
            {
                ASSERT(0 && "Taskman tray worker got unexpected message");
                break;
            }
        }
    }
    
    return 0;
}


/*++ Tray_NotifyIcon (WORKER THREAD CODE)

Routine Description:

   Handles adding, updating, etc., of icon on the tray
    
Arguments:

    hWnd                - icon owner
    uCallbackMessage    - message ID to be used for callback
    Message             - argument to Shell_NotifyIcon (NIM_ADD, NIM_DELETE, etc)
    hIcon               - Handle to the icon
    lpTip               - tooltip text for the icon

Revision History:

    Jan-27-95 Davepl  Created

--*/

void Tray_NotifyIcon(HWND    hWnd,
                     UINT    uCallbackMessage,
                     DWORD   Message,
                     HICON   hIcon,            
                     LPCTSTR lpTip)
{
    NOTIFYICONDATA NotifyIconData;

    NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
    NotifyIconData.uID              = uCallbackMessage;
    NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    NotifyIconData.uCallbackMessage = uCallbackMessage;

    NotifyIconData.hWnd = hWnd;
    NotifyIconData.hIcon = hIcon;

    if (lpTip) 
    {
        lstrcpyn(NotifyIconData.szTip, lpTip, ARRAYSIZE(NotifyIconData.szTip));
    } 
    else 
    {
        NotifyIconData.szTip[0] = 0;
    }

    Shell_NotifyIcon(Message, &NotifyIconData);
}

/*++ Tray_Notify (MAIN THREAD CODE)

Routine Description:

   Handles notifications sent by the tray
    
Revision History:

    Jan-27-95 Davepl  Created

--*/

void Tray_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)                     
{                                                                              
    switch (lParam) 
    {
        case WM_LBUTTONDBLCLK:                                                 
            ShowRunningInstance();
            break;                                                             

        case WM_RBUTTONDOWN:
        {
            HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_TRAYMENU);

            // Display the tray icons context menu at the current cursor location
                        
            if (hPopup)
            {
                POINT pt;
                GetCursorPos(&pt);

                if (IsWindowVisible(g_hMainWnd))
                {
                    DeleteMenu(hPopup, IDM_RESTORETASKMAN, MF_BYCOMMAND);
                }
                else
                {
                    SetMenuDefaultItem(hPopup, IDM_RESTORETASKMAN, FALSE);
                }

                CheckMenuItem(hPopup, IDM_ALWAYSONTOP,   
                    MF_BYCOMMAND | (g_Options.m_fAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));

                SetForegroundWindow(hWnd);
                g_fInPopup = TRUE;
                TrackPopupMenuEx(hPopup, 0, pt.x, pt.y, hWnd, NULL);
                g_fInPopup = FALSE;
                DestroyMenu(hPopup);
            }
            break;
        }
    }                                                                          
}                                                                              
