
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    unixui.cxx

Abstract:

    Contains Unix fixes

    Contents:
        //UnixAdjustButtonSpacing
        UnixRemoveMoreInfoButton

Author:

    Sriram Nambakam (v-sriran) 07-Dec-1998

Revision History:

    07-Dec-1998 v-sriran
        Created

--*/

#include <wininetp.h>
#include "ierrui.hxx"
#include "iehelpid.h"
#include "unixui.h"
#include <mainwin.h>

#if 0

#define MOVE_LEFT   0
#define MOVE_CENTER 1
#define MOVE_RIGHT  2

static void hAdjustButtonSpacing(HWND   hwnd,
                                 short  buttonSpacingStyle,
                                 short  cButtons,
                                 short  dwSpacing,
                                 ...);
                    

/* HACKHACK
 * On Unix, we draw a focus rectangle around the button that is currently
 * selected. And, the focus rectangle will overlap with the next button.
 * We programatically move the button to the *left*, to make it look good.
 */

void UnixAdjustButtonSpacing(HWND  hwnd, DWORD dwDlgId)
{
     switch(dwDlgId)
     {
           case IDD_HTTP_TO_HTTPS_ZONE_CROSSING:
                /* In these the More Info button is ID_TELL_ME_ABOUT_SECURITY */
                hAdjustButtonSpacing(hwnd,
                                     MOVE_LEFT,
                                     1,
                                     5,
                                     IDOK);
                
                break;
#ifdef NOT_YET_IMPLEMENTED
           case IDD_HTTPS_TO_HTTP_ZONE_CROSSING:
                break;
           case IDD_MIXED_SECURITY:
                break;
           case IDD_INVALID_CA:
                break;
           case IDD_BAD_CN:
                break;
           case IDD_CONFIRM_COOKIE:
                /* In this the More Info button is IDC_COOKIE_DETAILS */
                break;
#endif /* NOT_YET_IMPLEMENTED */
     }
}

/* 
 * This function assumes all buttons are of equal width for MOVE_CENTER
 */
void hAdjustButtonSpacing(HWND    hwnd,
                          short   buttonSpacingStyle,
                          short   cButtons,
                          short   dwSpacing,
                          ...)
{
     va_list Arguments;
     DWORD dwButtonId;

     va_start(Arguments, dwSpacing);

     if (buttonSpacingStyle != MOVE_CENTER && dwSpacing)
     {
        HWND hCurButton;
        RECT rect;

        for (short i = 0; i < cButtons; i++)
        {
            dwButtonId = (DWORD)va_arg(Arguments,ULONG);
            if ((hCurButton = GetDlgItem(hwnd, dwButtonId)))
            {
               GetWindowRect(hCurButton, &rect); 
               ScreenToClient(hwnd, (LPPOINT)&rect);
               SetWindowPos(hCurButton,
                            NULL,
                            (buttonSpacingStyle == MOVE_LEFT ? rect.left-dwSpacing : rect.left+dwSpacing),
                            rect.top,
                            0,
                            0,
                            SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
            }
        }
     }
     else /* MOVE_CENTER; dwSpacing does not matter */
     {
     }

     va_end(Arguments);

     return;
}
#endif /* 0 */

#define MOVE_CENTER 0x0001

static void hRemoveAndAdjust(HWND   hwnd,
                             DWORD  dwFlags,
                             short  cButtons,
                             ...);

/* If we have Button1 Button2 "More Info"
 * The following function makes it
 * <nothing> Button1 Button2
 * And, you have to pass the ids of "More Info" Button2 Button1 in the
 * variable argument list in that order
 */

void UnixRemoveMoreInfoButton(HWND  hwnd, DWORD dwDlgId)
{
     switch(dwDlgId)
     {
           case IDD_HTTP_TO_HTTPS_ZONE_CROSSING:
                /* In these the More Info button is ID_TELL_ME_ABOUT_SECURITY */
                hRemoveAndAdjust(hwnd,
                                 MOVE_CENTER,
                                 2,
                                 ID_TELL_ME_ABOUT_SECURITY,
                                 IDOK);
                
                break;
           case IDD_HTTPS_TO_HTTP_ZONE_CROSSING:
                hRemoveAndAdjust(hwnd,
                                 0,
                                 3,
                                 ID_TELL_ME_ABOUT_SECURITY,
                                 IDCANCEL,
                                 IDOK);
                break;
#ifdef NOT_YET_IMPLEMENTED
           case IDD_MIXED_SECURITY:
                break;
           case IDD_INVALID_CA:
                break;
           case IDD_BAD_CN:
                break;
           case IDD_CONFIRM_COOKIE:
                /* In this the More Info button is IDC_COOKIE_DETAILS */
                break;
#endif /* NOT_YET_IMPLEMENTED */
     }
}

void hRemoveAndAdjust(HWND   hwnd,
                      DWORD  dwFlags,
                      short  cButtons,
                      ...)
{
     va_list Arguments;
     DWORD   dwMoreInfoButtonId, dwButtonId;
     HWND    hMoreInfoButton, hDefButton;
     RECT    rectCur, rectPrev;

     hDefButton = MwRemoveDefPushButtonStyle(hwnd);

     /* Expect the first button to be the More Info Button
      * Hide the More Info Button, and move the other buttons to
      * the appropriate positions
      */

     if (cButtons < 2)
        goto Cleanup;

     /* We should use MOVE_CENTER only if there are two buttons in the move
      * list, and we hide the more info button, and move the other one to
      * the center
      */
     if ((dwFlags & MOVE_CENTER) && cButtons != 2)
        goto Cleanup;

     va_start(Arguments, cButtons);

     dwMoreInfoButtonId = (DWORD)va_arg(Arguments, ULONG);
     hMoreInfoButton = GetDlgItem(hwnd, dwMoreInfoButtonId);
     if (!hMoreInfoButton)
        goto Cleanup;

     GetWindowRect(hMoreInfoButton, &rectPrev); 
     ScreenToClient(hwnd, (LPPOINT)&rectPrev);

     if (dwFlags & MOVE_CENTER)
     {
        HWND hCurButton;
        dwButtonId = (DWORD)va_arg(Arguments,ULONG);
        if ((hCurButton = GetDlgItem(hwnd, dwButtonId)))
        {
           RECT rectDlg;

           GetWindowRect(hCurButton, &rectCur); 
           ScreenToClient(hwnd, (LPPOINT)&rectCur);
      
           GetWindowRect(hwnd, &rectDlg);
           ScreenToClient(hwnd, (LPPOINT)&rectDlg);
           SetWindowPos(hCurButton,
                        NULL,
                        ((rectDlg.right - rectDlg.left)/2)-((rectCur.right-rectCur.left)/2),
                        rectCur.top,
                        0,
                        0,
                        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
        }
     }
     else
     {
        HWND hCurButton;

        for (short i = 1; i < cButtons; i++)
        {
            dwButtonId = (DWORD)va_arg(Arguments,ULONG);
            if ((hCurButton = GetDlgItem(hwnd, dwButtonId)))
            {
               GetWindowRect(hCurButton, &rectCur); 
               ScreenToClient(hwnd, (LPPOINT)&rectCur);
               SetWindowPos(hCurButton,
                            NULL,
                            rectPrev.left,
                            rectPrev.top,
                            0,
                            0,
                            SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
               memcpy(&rectPrev, &rectCur, sizeof(RECT));
            }
        }
     }

     /* Hide the More Info Button */
     ShowWindow(hMoreInfoButton, SW_HIDE);

Cleanup:

     if (hDefButton)
        MwRestoreDefPushButtonStyle(hDefButton);

     return;
}
