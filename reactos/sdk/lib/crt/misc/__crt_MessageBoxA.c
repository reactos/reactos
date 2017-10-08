/*
 * PROJECT:         ReactOS C runtime library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/sdk/crt/misc/__crt_MessageBoxA.c
 * PURPOSE:         __crt_MessageBoxA implementation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <precomp.h>

/*****************************************************************************
 * \brief Displays a message box.
 *
 * \param pszText - The message to be displayed.
 * \param uType - The contents and behavior of the message box.
 * \return Identifies the button that was pressed by the user.
 * \see MessageBox
 *
 *****************************************************************************/
int
__cdecl
__crt_MessageBoxA (
    _In_opt_ const char *pszText,
    _In_ unsigned int uType)
{
    HMODULE hmodUser32;
    int (WINAPI *pMessageBoxA)(HWND, LPCTSTR, LPCTSTR, UINT);
    int iResult;

    /* Get MessageBoxA function pointer */
    hmodUser32 = LoadLibrary("user32.dll");
    pMessageBoxA = (PVOID)GetProcAddress(hmodUser32, "MessageBoxA");
    if (!pMessageBoxA)
    {
        abort();
    }

    /* Display a message box */
    iResult = pMessageBoxA(NULL,
                           pszText,
                           "ReactOS C Runtime Library",
                           uType);

    FreeLibrary(hmodUser32);
    return iResult;
}

