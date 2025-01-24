/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/fullscreen.c
 * PURPOSE:         GUI Terminal Full-screen Mode
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

#include "guiterm.h"

/* FUNCTIONS ******************************************************************/

BOOL
EnterFullScreen(PGUI_CONSOLE_DATA GuiData)
{
    DEVMODEW DevMode;

    ZeroMemory(&DevMode, sizeof(DevMode));
    DevMode.dmSize = sizeof(DevMode);

    DevMode.dmDisplayFixedOutput = DMDFO_CENTER; // DMDFO_STRETCH // DMDFO_DEFAULT
    // DevMode.dmDisplayFlags = DMDISPLAYFLAGS_TEXTMODE;
    DevMode.dmPelsWidth  = 640; // GuiData->ActiveBuffer->ViewSize.X * GuiData->CharWidth;
    DevMode.dmPelsHeight = 480; // GuiData->ActiveBuffer->ViewSize.Y * GuiData->CharHeight;
    // DevMode.dmBitsPerPel = 32;
    DevMode.dmFields     = DM_DISPLAYFIXEDOUTPUT | /* DM_DISPLAYFLAGS | DM_BITSPERPEL | */ DM_PELSWIDTH | DM_PELSHEIGHT;

    return (ChangeDisplaySettingsW(&DevMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL);
}

VOID
LeaveFullScreen(PGUI_CONSOLE_DATA GuiData)
{
    ChangeDisplaySettingsW(NULL, CDS_RESET);
}


// static VOID
// GuiConsoleResize(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam);

VOID
SwitchFullScreen(PGUI_CONSOLE_DATA GuiData, BOOL FullScreen)
{
    PCONSRV_CONSOLE Console = GuiData->Console;

    /*
     * See:
     * http://stackoverflow.com/questions/2382464/win32-full-screen-and-hiding-taskbar
     * http://stackoverflow.com/questions/3549148/fullscreen-management-with-winapi
     * https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
     * https://devblogs.microsoft.com/oldnewthing/20050505-04/?p=35703
     * http://stackoverflow.com/questions/1400654/how-do-i-put-my-opengl-app-into-fullscreen-mode
     * https://web.archive.org/web/20180210071518/http://nehe.gamedev.net/tutorial/creating_an_opengl_window_win32/13001/
     * https://web.archive.org/web/20121001015230/http://www.reocities.com/pcgpe/dibs.html
     */

    /* If we are already in the given state, just bail out */
    if (FullScreen == GuiData->GuiInfo.FullScreen) return;

    /* Save the current window state if we are not already full-screen */
    if (!GuiData->GuiInfo.FullScreen)
    {
        GuiData->IsWndMax = IsZoomed(GuiData->hWindow);
        if (GuiData->IsWndMax)
            SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);

        /* Save its old position and size and show state */
        GuiData->WndPl.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(GuiData->hWindow, &GuiData->WndPl);

        /* Save the old window styles */
        GuiData->WndStyle   = GetWindowLongPtr(GuiData->hWindow, GWL_STYLE  );
        GuiData->WndStyleEx = GetWindowLongPtr(GuiData->hWindow, GWL_EXSTYLE);
    }

    if (FullScreen)
    {
        SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);

        /* Switch to full screen */
        if (EnterFullScreen(GuiData))
        {
            /* Save the new state */
            Console->FixedSize = TRUE;
            GuiData->GuiInfo.FullScreen = TRUE;

            GuiData->ActiveBuffer->OldViewSize = GuiData->ActiveBuffer->ViewSize;
            // GuiData->ActiveBuffer->OldScreenBufferSize = GuiData->ActiveBuffer->ScreenBufferSize;

            /* Change the window styles */
            SetWindowLongPtr(GuiData->hWindow, GWL_STYLE,
                             WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
            SetWindowLongPtr(GuiData->hWindow, GWL_EXSTYLE,
                             WS_EX_APPWINDOW);
            // SetWindowPos(GuiData->hWindow, NULL, 0, 0, 0, 0,
                         // SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         // SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            /* Reposition the window to the upper-left corner */
            SetWindowPos(GuiData->hWindow, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            /* Make it the foreground window */
            SetForegroundWindow(GuiData->hWindow);

            /* Resize it */
            // // GuiConsoleResizeWindow(GuiData);
            // GuiConsoleResize(GuiData, SIZE_RESTORED, MAKELPARAM(640, 480));

            PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            // SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }

        PostMessageW(GuiData->hWindow, WM_SYSCOMMAND,
                     GuiData->GuiInfo.FullScreen || GuiData->IsWndMax
                        ? SC_MAXIMIZE : SC_RESTORE,
                     0);
        // ShowWindowAsync(GuiData->hWindow, SW_RESTORE);
    }
    else
    {
        SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);

        /* Restore windowing mode */
        LeaveFullScreen(GuiData);

        /* Save the new state */
        GuiData->GuiInfo.FullScreen = FALSE;
        Console->FixedSize = FALSE;

        /*
         * Restore possible saved dimensions
         * of the active screen buffer view.
         */
        GuiData->ActiveBuffer->ViewSize = GuiData->ActiveBuffer->OldViewSize;
        // GuiData->ActiveBuffer->ScreenBufferSize = GuiData->ActiveBuffer->OldScreenBufferSize;

        /* Restore the window styles */
        SetWindowLongPtr(GuiData->hWindow, GWL_STYLE,
                         GuiData->WndStyle);
        SetWindowLongPtr(GuiData->hWindow, GWL_EXSTYLE,
                         GuiData->WndStyleEx);
        SetWindowPos(GuiData->hWindow, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_FRAMECHANGED /*| SWP_SHOWWINDOW*/);


        /* Restore the window to its original position */
        SetWindowPlacement(GuiData->hWindow, &GuiData->WndPl);

        PostMessageW(GuiData->hWindow, WM_SYSCOMMAND,
                     GuiData->IsWndMax ? SC_MAXIMIZE : SC_RESTORE,
                     0);
        // ShowWindowAsync(GuiData->hWindow, SW_RESTORE);

        /* Make it the foreground window */
        SetForegroundWindow(GuiData->hWindow);

        /* Resize it */
        // GuiConsoleResizeWindow(GuiData);

        // PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
        // // ShowWindowAsync(GuiData->hWindow, SW_RESTORE);
    }
}

VOID
GuiConsoleSwitchFullScreen(PGUI_CONSOLE_DATA GuiData)
{
    PCONSRV_CONSOLE Console = GuiData->Console;
    BOOL FullScreen;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    /* Switch to full-screen or to windowed mode */
    FullScreen = !GuiData->GuiInfo.FullScreen;
    DPRINT("GuiConsoleSwitchFullScreen - Switch to %s ...\n",
            (FullScreen ? "full-screen" : "windowed mode"));

    SwitchFullScreen(GuiData, FullScreen);

    LeaveCriticalSection(&Console->Lock);
}

/* EOF */
