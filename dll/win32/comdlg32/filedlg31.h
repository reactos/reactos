/*
 * Win3.1 style File Dialog interface (32 bit)
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
 */

#ifndef _WINE_DLL_FILEDLG31_H
#define _WINE_DLL_FILEDLG31_H

#define FD31_OFN_PROP "FILEDLG_OFN"

typedef struct tagFD31_DATA
{
    HWND hwnd; /* file dialog window handle */
    BOOL hook; /* TRUE if the dialog is hooked */
    UINT lbselchstring; /* registered message id */
    UINT fileokstring; /* registered message id */
    LPARAM lParam; /* save original lparam */
    LPCVOID template; /* template for 32 bits resource */
    BOOL open; /* TRUE if open dialog, FALSE if save dialog */
    LPOPENFILENAMEW ofnW; /* pointer either to the original structure or
                             a W copy for A/16 API */
    LPOPENFILENAMEA ofnA; /* original structure if 32bits ansi dialog */
} FD31_DATA, *PFD31_DATA;

extern BOOL FD32_GetTemplate(PFD31_DATA lfs) DECLSPEC_HIDDEN;

extern BOOL FD31_Init(void) DECLSPEC_HIDDEN;
extern PFD31_DATA FD31_AllocPrivate(LPARAM lParam, UINT dlgType, BOOL IsUnicode) DECLSPEC_HIDDEN;
extern void FD31_DestroyPrivate(PFD31_DATA lfs) DECLSPEC_HIDDEN;
extern BOOL FD31_CallWindowProc(const FD31_DATA *lfs, UINT wMsg, WPARAM wParam,
                                LPARAM lParam) DECLSPEC_HIDDEN;
extern LONG FD31_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam) DECLSPEC_HIDDEN;
extern LONG FD31_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
                            int savedlg, const DRAWITEMSTRUCT *lpdis) DECLSPEC_HIDDEN;
extern LRESULT FD31_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
                              UINT control, const FD31_DATA *lfs) DECLSPEC_HIDDEN;
extern int FD31_GetFldrHeight(void) DECLSPEC_HIDDEN;

#endif /* _WINE_DLL_FILEDLG31_H */
