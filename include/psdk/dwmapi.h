/*
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
 */

#ifndef __WINE_DWMAPI_H
#define __WINE_DWMAPI_H

#include "wtypes.h"
#include "uxtheme.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DWMAPI
# define DWMAPI        STDAPI
# define DWMAPI_(type) STDAPI_(type)
#endif

DECLARE_HANDLE(HTHUMBNAIL);
typedef HTHUMBNAIL *PHTHUMBNAIL;

typedef struct _MilMatrix3x2D
{
    DOUBLE S_11;
    DOUBLE S_12;
    DOUBLE S_21;
    DOUBLE S_22;
    DOUBLE DX;
    DOUBLE DY;
} MilMatrix3x2D;

#define DWM_BB_ENABLE                 0x00000001
#define DWM_BB_BLURREGION             0x00000002
#define DWM_BB_TRANSITIONONMAXIMIZED  0x00000004

typedef struct _DWM_BLURBEHIND
{
    DWORD dwFlags;
    BOOL fEnable;
    HRGN hRgnBlur;
    BOOL fTransitionOnMaximized;
} DWM_BLURBEHIND, *PDWM_BLURBEHIND;

DWMAPI DwmDefWindowProc(HWND, UINT, WPARAM, LPARAM, LRESULT*);
DWMAPI DwmEnableBlurBehindWindow(HWND, const DWM_BLURBEHIND *);
DWMAPI DwmEnableComposition(UINT);
DWMAPI DwmEnableMMCSS(BOOL);
DWMAPI DwmExtendFrameIntoClientArea(HWND,const MARGINS*);
DWMAPI DwmGetColorizationColor(DWORD*,BOOL);
DWMAPI DwmIsCompositionEnabled(BOOL*);
DWMAPI DwmRegisterThumbnail(HWND, HWND, PHTHUMBNAIL);
DWMAPI DwmSetWindowAttribute(HWND, DWORD, LPCVOID, DWORD);
DWMAPI DwmUnregisterThumbnail(HTHUMBNAIL);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_DWMAPI_H */
