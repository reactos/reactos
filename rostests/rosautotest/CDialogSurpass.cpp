/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class for managing all the configuration parameters
 * COPYRIGHT:   Copyright 2011 
 */

#include "precomp.h"

BOOL CALLBACK PrintWindow(HWND hwnd, LPARAM lParam)
{
    CHAR WindowTitle[100];
    int lenght;

    lenght = GetWindowTextA(hwnd, WindowTitle, 100);
    if(lenght == 0)
        return TRUE;

    StringOut( string(WindowTitle) + "\n" );

    return TRUE;
}


void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook,
                           DWORD event,
                           HWND hwnd,
                           LONG idObject,
                           LONG idChild,
                           DWORD dwEventThread,
                           DWORD dwmsEventTime)
{
    /* make sure we got the correct event */
    if(event == EVENT_SYSTEM_DIALOGSTART)
    {
        /* wait for some time to make sure that the dialog is hung */
        Sleep(30 * 1000);

        /* Check if it is still open */
        if(IsWindow(hwnd))
        {
            /* Print an error message */
            StringOut("Closing following dialog box:\n");
            PrintWindow(hwnd, NULL);
            EnumChildWindows(hwnd, PrintWindow, NULL);

            /* Close the dialog */
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
    }
}


DWORD WINAPI DialogSurpassThread(LPVOID lpThreadParameter)
{
    MSG dummy;

    /* Install event notifications */
    SetWinEventHook(EVENT_SYSTEM_DIALOGSTART,
                    EVENT_SYSTEM_DIALOGSTART,
                    NULL,
                    WinEventProc,
                    0,
                    0,
                    WINEVENT_OUTOFCONTEXT);

    while(GetMessage(&dummy, 0,0,0))
    {
        /* There is no need to dispatch messages here */
        /* Actually this block will never be executed */
    }

    return 0;
}

CDialogSurpass::CDialogSurpass()
{
    /* Creat the trhead that will receive notifications */
    hThread = CreateThread(NULL,
                           0,
                           DialogSurpassThread,
                           NULL,
                           0,
                           &ThreadID);
}

CDialogSurpass::~CDialogSurpass()
{
    /* Notify the thread to close */
    PostThreadMessage(ThreadID, WM_QUIT, 0, 0);

    /* Wait for it close */
    WaitForSingleObject(hThread, INFINITE);

    /* Now close its handle*/
    CloseHandle(hThread);
}
