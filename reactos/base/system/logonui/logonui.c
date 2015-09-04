/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Logon User Interface Host
 * FILE:        subsys/system/logonui/logonui.c
 * PROGRAMMERS: Ged Murphy (gedmurphy@reactos.org)
 */

#include "logonui.h"

/* DATA **********************************************************************/




/* GLOBALS ******************************************************************/

PINFO g_pInfo = NULL;


/* FUNCTIONS ****************************************************************/


static HDC
DrawBaseBackground(HDC hdcDesktop)
{
    HDC hdcMem;

    hdcMem = NT5_DrawBaseBackground(hdcDesktop);

    return hdcMem;
}

static VOID
DrawLogoffScreen(HDC hdcMem)
{
    /* Draw the logoff icon */
    NT5_CreateLogoffScreen(L"Saving your settings...", hdcMem);
}

static ULONG
GetULONG(LPWSTR String)
{
    UINT i, Length;
    ULONG Value;
    LPWSTR StopString;

    i = 0;
    /* Get the string length */
    Length = (UINT)wcslen(String);

    /* Check the string only consists of numbers */
    while ((i < Length) && ((String[i] < L'0') || (String[i] > L'9'))) i++;
    if ((i >= Length) || ((String[i] < L'0') || (String[i] > L'9')))
    {
        return (ULONG)-1;
    }

    /* Convert it */
    Value = wcstoul(&String[i], &StopString, 10);

    return Value;
}

static ULONG
GetULONG2(LPWSTR String1, LPWSTR String2, PINT i)
{
    ULONG Value;

    /* Check the first string value */
    Value = GetULONG(String1);
    if (Value == (ULONG)-1)
    {
        /* Check the second string value isn't a switch */
        if (String2[0] != L'-')
        {
            /* Check the value */
            Value = GetULONG(String2);
            *i += 1;
        }
    }

    return Value;
}

static BOOL
ParseCmdline(int argc, WCHAR* argv[])
{
    return TRUE;
}

static VOID
Run(VOID)
{
    HWND hDesktopWnd;
    HDC hdcDesktop, hdcMem;

    /* Get the screen size */
    g_pInfo->cx = GetSystemMetrics(SM_CXSCREEN);
    g_pInfo->cy = GetSystemMetrics(SM_CYSCREEN);

    hDesktopWnd = GetDesktopWindow();

    /* Get the DC for the desktop */
    hdcDesktop = GetDCEx(hDesktopWnd, NULL, DCX_CACHE);
    if (hdcDesktop)
    {
        /* Initialize the base background onto a DC */
        hdcMem = DrawBaseBackground(hdcDesktop);
        if (hdcMem)
        {
            /* TEST : Draw logoff screen */
            DrawLogoffScreen(hdcMem);

            /* Blit the off-screen DC to the desktop */
            BitBlt(hdcDesktop,
                   0,
                   0,
                   g_pInfo->cx,
                   g_pInfo->cy,
                   hdcMem,
                   0,
                   0,
                   SRCCOPY);

            /* Delete the memory DC */
            DeleteDC(hdcMem);
        }

        /* Release the desktop DC */
        ReleaseDC(hDesktopWnd, hdcDesktop);
    }
}

int WINAPI
wWinMain(IN HINSTANCE hInst,
         IN HINSTANCE hPrevInstance,
         IN LPWSTR lpszCmdLine,
         IN int nCmdShow)
{
    LPWSTR *lpArgs;
    INT NumArgs;

    /* Allocate memory for the data */
    g_pInfo = (PINFO)HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(INFO));
    if (!g_pInfo) return -1;

    g_pInfo->hInstance = hInst;

    /* Get the command line args */
    lpArgs = CommandLineToArgvW(lpszCmdLine, &NumArgs);
    if (lpArgs)
    {
        /* Parse the command line */
        if (ParseCmdline(NumArgs, lpArgs))
        {
            /* Start the main routine */
            Run();
        }
    }

    /* Free the data */
    HeapFree(GetProcessHeap(),
             0,
             g_pInfo);

    return 0;
}

/* EOF */
