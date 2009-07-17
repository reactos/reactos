/*
 * Win16 BiDi functions
 * Copyright 2000 Erez Volk
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTE: Right now, most of these functions do nothing.
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);

/***********************************************************************
 *		ChangeDialogTemplate   (USER.905)
 * FIXME: The prototypes of this function have not been found yet.
 */
LONG WINAPI ChangeDialogTemplate16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiMessageBoxEx   (USER.910)
 * FIXME: The prototypes of this function have not been found yet.
 */
LONG WINAPI BiDiMessageBoxEx16(void) { FIXME("stub (no prototype)\n"); return 0; }


/******************************************************************************
 *                    ChangeKeyboardCodePage [USER.924]
 * Change the keyboard layouts to a new pair.
 * If the language IDs are set to -1, the language is not to be changed.
 */
DWORD WINAPI ChangeKeyboardCodePage16( UINT16 iLangLeft, UINT16 iLangRight )
{
    FIXME( "( %hu, %hu ): stub\n", iLangLeft, iLangRight );
    return 0;
}


/******************************************************************************
 *                    ChangeKeyboardLanguage [USER.912]
 * Change the keyboard layouts to a new pair.
 *
 * RETURNS
 *    The old keyboard layout pair.
 */
DWORD WINAPI ChangeKeyboardLanguage16( UINT16 iLangLeft, UINT iLangRight )
{
    FIXME( "( %hu, %hu ): stub\n", iLangLeft, iLangRight );
    return 0;
}


/******************************************************************************
 *                    CreateDialogIndirectParamML [USER.916]
 */
HWND16 WINAPI CreateDialogIndirectParamML16( HINSTANCE16 hinstWnd,
                                             const void * lpvDlgTmp,
                                             HWND16 hwndOwner,
                                             DLGPROC16 dlgProc,
                                             LPARAM lParamInit,
                                             UINT16 iCodePage, UINT16 iLang,
                                             LPCSTR lpDlgName,
                                             HINSTANCE16 hinstLoad )
{
    FIXME( "( %04hx, %p, %04hx, %p, %08lx, %hu, %hu, %p, %04hx ): stub\n",
           hinstWnd, lpvDlgTmp, hwndOwner, dlgProc, lParamInit,
           iCodePage, iLang, lpDlgName, hinstLoad );
    return 0;
}


/******************************************************************************
 *                    DialogBoxIndirectParamML [USER.918]
 */
HWND16 WINAPI DialogBoxIndirectParamML16( HINSTANCE16 hinstWnd,
                                          HGLOBAL16 hglbDlgTemp,
                                          HWND16 hwndOwner,
                                          DLGPROC16 dlgprc,
                                          LPARAM lParamInit,
                                          UINT16 iCodePage, UINT16 iLang,
                                          LPCSTR lpDlgName,
                                          HINSTANCE16 hinstLoad )
{
    FIXME( "( %04hx, %04hx, %04hx, %p, %08lx, %hu, %hu, %p, %04hx ): stub\n",
           hinstWnd, hglbDlgTemp, hwndOwner, dlgprc, lParamInit,
           iCodePage, iLang, lpDlgName, hinstLoad );
    return 0;
}



/******************************************************************************
 *                    FindLanguageResource [USER.923]
 */
HRSRC16 WINAPI FindLanguageResource16( HINSTANCE16 hinst, LPCSTR lpRes,
                                       LPCSTR lpResType, UINT16 iLang )
{
    FIXME( "( %04hx, %p, %p, %hu ): stub\n", hinst, lpRes, lpResType, iLang );
    return 0;
}


/******************************************************************************
 *                    GetAppCodePage [USER.915]
 * Returns the code page and language of the window
 *
 * RETURNS
 *    The low word contains the code page, the high word contains the resource language.
 */
DWORD WINAPI GetAppCodePage16( HWND16 hwnd )
{
    FIXME( "( %04hx ): stub\n", hwnd );
    return 0;
}


/******************************************************************************
 *                    GetBaseCodePage [USER.922]
 * Returns the base code page and resource language.
 * For example, Hebrew windows will return HebrewCodePage in the low word
 * and English in the high word.
 */
DWORD WINAPI GetBaseCodePage16( void )
{
    FIXME( ": stub\n" );
    return 0;
}



/******************************************************************************
 *                    GetCodePageSystemFont [USER.913]
 * Returns the stock font for the requested code page.
 */
HFONT16 WINAPI GetCodePageSystemFont16( UINT16 iFont, UINT16 iCodePage )
{
    FIXME( "( %hu, %hu ): stub\n", iFont, iCodePage );
    return 0;
}



/******************************************************************************
 *                    GetLanguageName [USER.907]
 * Returns the name of one language in (possibly) a different language.
 * Currently only handles language 0 (english).
 *
 * RETURNS
 *    Success: The number of bytes copied to the buffer, not including the null.
 *    Failure: 0
 */
UINT WINAPI GetLanguageName16( UINT16 iLang, UINT16 iName,
                               LPSTR lpszName, UINT16 cbBuffer )
{
    if ( (iLang == 0) && (iName == 0) ) {
        if ( !lpszName || cbBuffer < 8 ) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        strcpy( lpszName, "English" );
        return 7;
    }

    FIXME( "( %hu, %hu, %p, %hu ): No BiDi16\n", iLang, iName, lpszName, cbBuffer );
    return 0;
}


/******************************************************************************
 *                    GetNumLanguages [USER.906]
 * Returns the number of languages in the system.
 */
UINT WINAPI GetNumLanguages16( void )
{
    FIXME( ": No Bidi16\n" );
    return 1;
}



/******************************************************************************
 *                    GetProcessDefaultLayout [USER.1001]
 *
 * Gets the default layout for parentless windows.
 * Right now, just returns 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 */
BOOL16 WINAPI GetProcessDefaultLayout16( DWORD *pdwDefaultLayout )
{
    FIXME( "( %p ): no BiDi16\n", pdwDefaultLayout );
    return GetProcessDefaultLayout( pdwDefaultLayout );
}


/******************************************************************************
 *                   LoadLanguageString [USER.919]
 * Loads a string for a specific language.
 *
 * RETURNS
 *    SUCCESS: The length of the string loaded.
 *    FAILURE: Zero.
 */
UINT16 WINAPI LoadLanguageString16( HINSTANCE16 hinst, UINT16 id, UINT16 iLang,
                                    LPSTR lpszText, INT16 nBytes )
{
    FIXME( "( %04hx, %hu, %hu, %p, %hd ): stub\n", hinst, id, iLang, lpszText, nBytes );
    return 0;
}



/******************************************************************************
 *                   LoadSystemLanguageString [USER.902]
 * Loads a string which is in one of the system language modules.
 *
 * RETURNS
 *    Success: The length of the string loaded
 *    Failure: Zero
 */
UINT WINAPI LoadSystemLanguageString16( HINSTANCE16 hinstCaller, UINT16 id,
                                        LPSTR lpszText, INT16 nBytes, UINT16 iLang )
{
    FIXME( "( %04hx, %hu, %p, %hd, %hu ): stub\n", hinstCaller, id, lpszText, nBytes, iLang );
    return 0;
}


/***********************************************************************
 *           MessageBoxEx [USER.930]
 * The multilingual version of MessageBox.
 */
INT16 WINAPI MessageBoxEx16( HWND16 hwndParent, LPCSTR lpszText,
                             LPCSTR lpszTitle, UINT16 fuStyle, UINT16 iLang )
{
    FIXME( "( %04hx, %p, %p, %hu, %hu ): stub\n", hwndParent, lpszText, lpszTitle,
           fuStyle, iLang );
    return 0;
}


/***********************************************************************
 *           QueryCodePage   [USER.914]
 * Query code page specific data.
 */
LRESULT WINAPI QueryCodePage16( UINT16 idxLang, UINT16 msg,
                                WPARAM16 wParam, LPARAM lParam )
{
    FIXME( "( %hu, %hu, %04hx, %08lx ): stub\n", idxLang, msg, wParam, lParam );
    return 0;
}

/***********************************************************************
 *           SetAppCodePage   [USER.920]
 * Set the code page and language of the window to new values.
 *
 * RETURNS
 *    The low word contains the old code page, the high word contains
 *       the old resource language.
 */
DWORD WINAPI SetAppCodePage16( HWND16 hwnd, UINT16 iCodePage, UINT16 iLang,
                               UINT16 fRedraw )
{
    FIXME( "( %04hx, %hu, %hu, %hu ): stub\n", hwnd, iCodePage, iLang, fRedraw );
    return 0;
}

/***********************************************************************
 *           SetDlgItemTextEx   [USER.911]
 * Sets the title or text of a control in a dialog box.
 * Currently only works for language 0 (english)
 */
void WINAPI SetDlgItemTextEx16( HWND16 hwnd, INT16 id,
                                LPCSTR lpszText, UINT16 iLang )
{
    FIXME( "( %04hx, %hd, %p, %hu ): stub\n", hwnd, id, lpszText, iLang );
}

/******************************************************************************
 *                    SetProcessDefaultLayout [USER.1000]
 *
 * Sets the default layout for parentless windows.
 * Right now, only accepts 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 */
BOOL16 WINAPI SetProcessDefaultLayout16( DWORD dwDefaultLayout )
{
    FIXME( "( %08x ): No BiDi16\n", dwDefaultLayout );
    return SetProcessDefaultLayout( dwDefaultLayout );
}

/******************************************************************************
 *                    SetWindowTextEx [USER.909]
 * Sets the given window's title to the specified text in the specified language.
 */
void WINAPI SetWindowTextEx16( HWND16 hwnd, LPCSTR lpsz, UINT16 iLang )
{
    FIXME( "( %04hx, %p, %hu ): stub\n", hwnd, lpsz, iLang );
}
