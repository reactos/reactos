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

/* Forward declare */
typedef struct tagFD31_DATA FD31_DATA, *PFD31_DATA;

typedef struct tagFD31_CALLBACKS
{
    BOOL (CALLBACK *Init)(LPARAM lParam, PFD31_DATA lfs, DWORD data);
    BOOL (CALLBACK *CWP)(const FD31_DATA *lfs, UINT wMsg, WPARAM wParam,
                         LPARAM lParam); /* CWP instead of CallWindowProc to avoid macro expansion */
    void (CALLBACK *UpdateResult)(const FD31_DATA *lfs);
    void (CALLBACK *UpdateFileTitle)(const FD31_DATA *lfs);
    LRESULT (CALLBACK *SendLbGetCurSel)(const FD31_DATA *lfs);
    void (CALLBACK *Destroy)(const FD31_DATA *lfs);
} FD31_CALLBACKS, *PFD31_CALLBACKS;

struct tagFD31_DATA
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
    LPVOID private1632; /* 16/32 bit caller private data */
    PFD31_CALLBACKS callbacks; /* callbacks to handle 16/32 bit differences */
};

extern BOOL FD31_Init(void);
extern PFD31_DATA FD31_AllocPrivate(LPARAM lParam, UINT dlgType,
                                    PFD31_CALLBACKS callbacks, DWORD data);
extern void FD31_DestroyPrivate(PFD31_DATA lfs);
extern void FD31_MapOfnStructA(const OPENFILENAMEA *ofnA, LPOPENFILENAMEW ofnW, BOOL open);
extern void FD31_FreeOfnW(OPENFILENAMEW *ofnW);
extern BOOL FD31_CallWindowProc(const FD31_DATA *lfs, UINT wMsg, WPARAM wParam,
                                LPARAM lParam);
extern LONG FD31_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam);
extern LONG FD31_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
                            int savedlg, const DRAWITEMSTRUCT *lpdis);
extern LRESULT FD31_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
                              UINT control, const FD31_DATA *lfs);
extern int FD31_GetFldrHeight(void);

#endif /* _WINE_DLL_FILEDLG31_H */
