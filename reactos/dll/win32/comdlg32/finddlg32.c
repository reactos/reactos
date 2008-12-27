/*
 *  Common Dialog Boxes interface (32 bit)
 *  Find/Replace
 *
 * Copyright 1998,1999 Bertho A. Stultiens
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "cderr.h"
#include "dlgs.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"


/*-----------------------------------------------------------------------*/

static UINT		FindReplaceMessage;
static UINT		HelpMessage;

#define FR_MASK	(FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD | FR_REPLACEALL | FR_REPLACE | FR_FINDNEXT | FR_DIALOGTERM)
/* CRITICAL_SECTION COMDLG32_CritSect; */

/* Notes:
 * MS uses a critical section at a few locations. However, I fail to
 * see the reason for this. Their comdlg32.dll has a few race conditions
 * but _not_ at those places that are protected with the mutex (there are
 * globals that seem to hold info for the wndproc).
 *
 * FindText[AW]/ReplaceText[AW]
 * The find/replace calls are passed a structure that is _not_ used
 * internally. There is a local copy that holds the running info to
 * be able to combine xxxA and xxxW calls. The passed pointer is
 * returned upon sendmessage. Apps won't break this way when they rely
 * on the original pointer. This will work as long as the sizes of
 * FINDREPLACEA == FINDREPLACEW. The local copy will also prevent
 * the app to see the wine-specific extra flags to distinguish between
 * A/W and Find/Replace.
 */


/***********************************************************************
 *	COMDLG32_FR_GetFlags			[internal]
 * Returns the button state that needs to be reported to the caller.
 *	RETURNS
 *		Current state of check and radio buttons
 */
static DWORD COMDLG32_FR_GetFlags(HWND hDlgWnd)
{
	DWORD flags = 0;
	if(IsDlgButtonChecked(hDlgWnd, rad2) == BST_CHECKED)
        	flags |= FR_DOWN;
	if(IsDlgButtonChecked(hDlgWnd, chx1) == BST_CHECKED)
        	flags |= FR_WHOLEWORD;
	if(IsDlgButtonChecked(hDlgWnd, chx2) == BST_CHECKED)
        	flags |= FR_MATCHCASE;
        return flags;
}

/***********************************************************************
 *	COMDLG32_FR_HandleWMCommand		[internal]
 * Handle WM_COMMAND messages...
 */
static void COMDLG32_FR_HandleWMCommand(HWND hDlgWnd, COMDLG32_FR_Data *pData, int Id, int NotifyCode)
{
	DWORD flag;

	pData->user_fr.fra->Flags &= ~FR_MASK;	/* Clear return flags */
	if(pData->fr.Flags & FR_WINE_REPLACE)	/* Replace always goes down... */
		pData->user_fr.fra->Flags |= FR_DOWN;

	if(NotifyCode == BN_CLICKED)
        {
	       	switch(Id)
		{
		case IDOK: /* Find Next */
			if(GetDlgItemTextA(hDlgWnd, edt1, pData->fr.lpstrFindWhat, pData->fr.wFindWhatLen) > 0)
                        {
				pData->user_fr.fra->Flags |= COMDLG32_FR_GetFlags(hDlgWnd) | FR_FINDNEXT;
                                if(pData->fr.Flags & FR_WINE_UNICODE)
                                {
                                    MultiByteToWideChar( CP_ACP, 0, pData->fr.lpstrFindWhat, -1,
                                                         pData->user_fr.frw->lpstrFindWhat,
                                                         0x7fffffff );
                                }
                                else
                                {
                                	strcpy(pData->user_fr.fra->lpstrFindWhat, pData->fr.lpstrFindWhat);
                                }
				SendMessageA(pData->fr.hwndOwner, FindReplaceMessage, 0, (LPARAM)pData->user_fr.fra);
                        }
			break;

		case IDCANCEL:
			pData->user_fr.fra->Flags |= COMDLG32_FR_GetFlags(hDlgWnd) | FR_DIALOGTERM;
			SendMessageA(pData->fr.hwndOwner, FindReplaceMessage, 0, (LPARAM)pData->user_fr.fra);
        	        DestroyWindow(hDlgWnd);
			break;

                case psh2: /* Replace All */
                	flag = FR_REPLACEALL;
                        goto Replace;

                case psh1: /* Replace */
                	flag = FR_REPLACE;
Replace:
			if((pData->fr.Flags & FR_WINE_REPLACE)
                        && GetDlgItemTextA(hDlgWnd, edt1, pData->fr.lpstrFindWhat, pData->fr.wFindWhatLen) > 0)
                        {
				pData->fr.lpstrReplaceWith[0] = 0; /* In case the next GetDlgItemText Fails */
				GetDlgItemTextA(hDlgWnd, edt2, pData->fr.lpstrReplaceWith, pData->fr.wReplaceWithLen);
				pData->user_fr.fra->Flags |= COMDLG32_FR_GetFlags(hDlgWnd) | flag;
                                if(pData->fr.Flags & FR_WINE_UNICODE)
                                {
                                    MultiByteToWideChar( CP_ACP, 0, pData->fr.lpstrFindWhat, -1,
                                                         pData->user_fr.frw->lpstrFindWhat,
                                                         0x7fffffff );
                                    MultiByteToWideChar( CP_ACP, 0, pData->fr.lpstrReplaceWith, -1,
                                                         pData->user_fr.frw->lpstrReplaceWith,
                                                         0x7fffffff );
                                }
                                else
                                {
                                	strcpy(pData->user_fr.fra->lpstrFindWhat, pData->fr.lpstrFindWhat);
                                	strcpy(pData->user_fr.fra->lpstrReplaceWith, pData->fr.lpstrReplaceWith);
                                }
				SendMessageA(pData->fr.hwndOwner, FindReplaceMessage, 0, (LPARAM)pData->user_fr.fra);
                        }
			break;

		case pshHelp:
			pData->user_fr.fra->Flags |= COMDLG32_FR_GetFlags(hDlgWnd);
			SendMessageA(pData->fr.hwndOwner, HelpMessage, (WPARAM)hDlgWnd, (LPARAM)pData->user_fr.fra);
			break;
                }
        }
        else if(NotifyCode == EN_CHANGE && Id == edt1)
	{
		BOOL enable = SendDlgItemMessageA(hDlgWnd, edt1, WM_GETTEXTLENGTH, 0, 0) > 0;
		EnableWindow(GetDlgItem(hDlgWnd, IDOK), enable);
                if(pData->fr.Flags & FR_WINE_REPLACE)
                {
			EnableWindow(GetDlgItem(hDlgWnd, psh1), enable);
			EnableWindow(GetDlgItem(hDlgWnd, psh2), enable);
                }
	}
}

/***********************************************************************
 *	COMDLG32_FindReplaceDlgProc		[internal]
 * [Find/Replace]Text32[A/W] window procedure.
 */
static INT_PTR CALLBACK COMDLG32_FindReplaceDlgProc(HWND hDlgWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	COMDLG32_FR_Data *pdata = GetPropA(hDlgWnd, (LPSTR)COMDLG32_Atom);
	INT_PTR retval = TRUE;

	if(iMsg == WM_INITDIALOG)
        {
                pdata = (COMDLG32_FR_Data *)lParam;
        	if(!SetPropA(hDlgWnd, (LPSTR)COMDLG32_Atom, (HANDLE)pdata))
                {
			ERR("Could not Set prop; invent a gracefull exit?...\n");
                	DestroyWindow(hDlgWnd);
                        return FALSE;
                }
        	SendDlgItemMessageA(hDlgWnd, edt1, EM_SETLIMITTEXT, (WPARAM)pdata->fr.wFindWhatLen, 0);
	        SendDlgItemMessageA(hDlgWnd, edt1, WM_SETTEXT, 0, (LPARAM)pdata->fr.lpstrFindWhat);
                if(pdata->fr.Flags & FR_WINE_REPLACE)
                {
	        	SendDlgItemMessageA(hDlgWnd, edt2, EM_SETLIMITTEXT, (WPARAM)pdata->fr.wReplaceWithLen, 0);
		        SendDlgItemMessageA(hDlgWnd, edt2, WM_SETTEXT, 0, (LPARAM)pdata->fr.lpstrReplaceWith);
                }

        	if(!(pdata->fr.Flags & FR_SHOWHELP))
			ShowWindow(GetDlgItem(hDlgWnd, pshHelp), SW_HIDE);
	        if(pdata->fr.Flags & FR_HIDEUPDOWN)
        	{
			ShowWindow(GetDlgItem(hDlgWnd, rad1), SW_HIDE);
			ShowWindow(GetDlgItem(hDlgWnd, rad2), SW_HIDE);
			ShowWindow(GetDlgItem(hDlgWnd, grp1), SW_HIDE);
	        }
        	else if(pdata->fr.Flags & FR_NOUPDOWN)
	        {
			EnableWindow(GetDlgItem(hDlgWnd, rad1), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, rad2), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, grp1), FALSE);
        	}
                else
                {
			SendDlgItemMessageA(hDlgWnd, rad1, BM_SETCHECK, pdata->fr.Flags & FR_DOWN ? 0 : BST_CHECKED, 0);
			SendDlgItemMessageA(hDlgWnd, rad2, BM_SETCHECK, pdata->fr.Flags & FR_DOWN ? BST_CHECKED : 0, 0);
                }

	        if(pdata->fr.Flags & FR_HIDEMATCHCASE)
			ShowWindow(GetDlgItem(hDlgWnd, chx2), SW_HIDE);
        	else if(pdata->fr.Flags & FR_NOMATCHCASE)
			EnableWindow(GetDlgItem(hDlgWnd, chx2), FALSE);
                else
			SendDlgItemMessageA(hDlgWnd, chx2, BM_SETCHECK, pdata->fr.Flags & FR_MATCHCASE ? BST_CHECKED : 0, 0);

	        if(pdata->fr.Flags & FR_HIDEWHOLEWORD)
			ShowWindow(GetDlgItem(hDlgWnd, chx1), SW_HIDE);
        	else if(pdata->fr.Flags & FR_NOWHOLEWORD)
			EnableWindow(GetDlgItem(hDlgWnd, chx1), FALSE);
                else
			SendDlgItemMessageA(hDlgWnd, chx1, BM_SETCHECK, pdata->fr.Flags & FR_WHOLEWORD ? BST_CHECKED : 0, 0);

		/* We did the init here, now call the hook if requested */

		/* We do not do ShowWindow if hook exists and is FALSE  */
		/*   per MSDN Article Q96135                            */
              	if((pdata->fr.Flags & FR_ENABLEHOOK)
	             && ! pdata->fr.lpfnHook(hDlgWnd, iMsg, wParam, (LPARAM) &pdata->fr))
		        return TRUE;
		ShowWindow(hDlgWnd, SW_SHOWNORMAL);
		UpdateWindow(hDlgWnd);
                return TRUE;
        }

	if(pdata && (pdata->fr.Flags & FR_ENABLEHOOK))
	{
		retval = pdata->fr.lpfnHook(hDlgWnd, iMsg, wParam, lParam);
	}
	else
		retval = FALSE;

       	if(pdata && !retval)
        {
        	retval = TRUE;
		switch(iMsg)
        	{
	        case WM_COMMAND:
			COMDLG32_FR_HandleWMCommand(hDlgWnd, pdata, LOWORD(wParam), HIWORD(wParam));
                	break;

	        case WM_CLOSE:
			COMDLG32_FR_HandleWMCommand(hDlgWnd, pdata, IDCANCEL, BN_CLICKED);
        		break;

	        case WM_HELP:
        		/* Heeeeelp! */
        		FIXME("Got WM_HELP. Who is gonna supply it?\n");
	                break;

        	case WM_CONTEXTMENU:
        		/* Heeeeelp! */
        		FIXME("Got WM_CONTEXTMENU. Who is gonna supply it?\n");
        	        break;
		/* FIXME: Handle F1 help */

	        default:
		        retval = FALSE;	/* We did not handle the message */
	        }
        }

        /* WM_DESTROY is a special case.
         * We need to ensure that the allocated memory is freed just before
         * the dialog is killed. We also need to remove the added prop.
         */
        if(iMsg == WM_DESTROY)
        {
        	RemovePropA(hDlgWnd, (LPSTR)COMDLG32_Atom);
        	HeapFree(GetProcessHeap(), 0, pdata);
        }

        return retval;
}

/***********************************************************************
 *	COMDLG32_FR_CheckPartial		[internal]
 * Check various fault conditions in the supplied parameters that
 * cause an extended error to be reported.
 *	RETURNS
 *		TRUE: Success
 *		FALSE: Failure
 */
static BOOL COMDLG32_FR_CheckPartial(
	const FINDREPLACEA *pfr,	/* [in] Find structure */
	BOOL Replace			/* [in] True if called as replace */
) {
	if(!pfr)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_GENERALCODES);
                return FALSE;
	}

        if(pfr->lStructSize != sizeof(FINDREPLACEA))
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_STRUCTSIZE);
        	return FALSE;
        }

        if(!IsWindow(pfr->hwndOwner))
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_DIALOGFAILURE);
        	return FALSE;
        }

	if((pfr->wFindWhatLen < 1 || !pfr->lpstrFindWhat)
        ||(Replace && (pfr->wReplaceWithLen < 1 || !pfr->lpstrReplaceWith)))
        {
        	COMDLG32_SetCommDlgExtendedError(FRERR_BUFFERLENGTHZERO);
                return FALSE;
        }

	if((FindReplaceMessage = RegisterWindowMessageA(FINDMSGSTRINGA)) == 0)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
                return FALSE;
        }
	if((HelpMessage = RegisterWindowMessageA(HELPMSGSTRINGA)) == 0)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_REGISTERMSGFAIL);
                return FALSE;
        }

        if((pfr->Flags & FR_ENABLEHOOK) && !pfr->lpfnHook)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_NOHOOK);
                return FALSE;
        }

        if((pfr->Flags & (FR_ENABLETEMPLATE | FR_ENABLETEMPLATEHANDLE)) && !pfr->hInstance)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_NOHINSTANCE);
                return FALSE;
        }

        if((pfr->Flags & FR_ENABLETEMPLATE) && !pfr->lpTemplateName)
        {
        	COMDLG32_SetCommDlgExtendedError(CDERR_NOTEMPLATE);
                return FALSE;
        }

	return TRUE;
}

/***********************************************************************
 *	COMDLG32_FR_DoFindReplace		[internal]
 * Actual load and creation of the Find/Replace dialog.
 *	RETURNS
 *		Window handle to created dialog:Success
 *		NULL:Failure
 */
static HWND COMDLG32_FR_DoFindReplace(
	COMDLG32_FR_Data *pdata	/* [in] Internal data structure */
) {
	HWND hdlgwnd = 0;
        HGLOBAL loadrc;
        DWORD error;
        LPDLGTEMPLATEW rcs;

	TRACE("hInst=%p, Flags=%08x\n", pdata->fr.hInstance, pdata->fr.Flags);

        if(!(pdata->fr.Flags & FR_ENABLETEMPLATEHANDLE))
        {
	        HMODULE hmod = COMDLG32_hInstance;
		HRSRC htemplate;
        	if(pdata->fr.Flags & FR_ENABLETEMPLATE)
	        {
			hmod = pdata->fr.hInstance;
                        if(pdata->fr.Flags & FR_WINE_UNICODE)
                        {
				htemplate = FindResourceW(hmod, (LPCWSTR)pdata->fr.lpTemplateName, (LPWSTR)RT_DIALOG);
                        }
                        else
                        {
				htemplate = FindResourceA(hmod, pdata->fr.lpTemplateName, (LPCSTR)RT_DIALOG);
                        }
        	}
		else
        	{
			int rcid = pdata->fr.Flags & FR_WINE_REPLACE ? REPLACEDLGORD
								     : FINDDLGORD;
			htemplate = FindResourceA(hmod, MAKEINTRESOURCEA(rcid), (LPCSTR)RT_DIALOG);
		}
		if(!htemplate)
       	        {
                	error = CDERR_FINDRESFAILURE;
                       	goto cleanup;
               	}

	        loadrc = LoadResource(hmod, htemplate);
        }
        else
        {
        	loadrc = (HGLOBAL)pdata->fr.hInstance;
        }

        if(!loadrc)
        {
		error = CDERR_LOADRESFAILURE;
		goto cleanup;
	}

        if((rcs = LockResource(loadrc)) == NULL)
        {
		error = CDERR_LOCKRESFAILURE;
		goto cleanup;
        }

        hdlgwnd = CreateDialogIndirectParamA(COMDLG32_hInstance,
        				     rcs,
                                             pdata->fr.hwndOwner,
                                             COMDLG32_FindReplaceDlgProc,
                                             (LPARAM)pdata);
	if(!hdlgwnd)
        {
        	error = CDERR_DIALOGFAILURE;
cleanup:
        	COMDLG32_SetCommDlgExtendedError(error);
                HeapFree(GetProcessHeap(), 0, pdata);
        }
        return hdlgwnd;
}

/***********************************************************************
 *	FindTextA 				[COMDLG32.@]
 *
 * See FindTextW.
 */
HWND WINAPI FindTextA(
	LPFINDREPLACEA pfr	/* [in] Find/replace structure*/
) {
	COMDLG32_FR_Data *pdata;

        TRACE("LPFINDREPLACE=%p\n", pfr);

	if(!COMDLG32_FR_CheckPartial(pfr, FALSE))
        	return 0;

        if((pdata = COMDLG32_AllocMem(sizeof(COMDLG32_FR_Data))) == NULL)
        	return 0; /* Error has been set */

        pdata->user_fr.fra = pfr;
        pdata->fr = *pfr;
	return COMDLG32_FR_DoFindReplace(pdata);
}

/***********************************************************************
 *	ReplaceTextA 				[COMDLG32.@]
 *
 * See ReplaceTextW.
 */
HWND WINAPI ReplaceTextA(
	LPFINDREPLACEA pfr	/* [in] Find/replace structure*/
) {
	COMDLG32_FR_Data *pdata;

        TRACE("LPFINDREPLACE=%p\n", pfr);

	if(!COMDLG32_FR_CheckPartial(pfr, TRUE))
        	return 0;

        if((pdata = COMDLG32_AllocMem(sizeof(COMDLG32_FR_Data))) == NULL)
        	return 0; /* Error has been set */

        pdata->user_fr.fra = pfr;
        pdata->fr = *pfr;
	pdata->fr.Flags |= FR_WINE_REPLACE;
	return COMDLG32_FR_DoFindReplace(pdata);
}

/***********************************************************************
 *	FindTextW 				[COMDLG32.@]
 *
 * Create a modeless find-text dialog box.
 *
 *	RETURNS
 *		Window handle to created dialog: Success
 *		NULL: Failure
 */
HWND WINAPI FindTextW(
	LPFINDREPLACEW pfr	/* [in] Find/replace structure*/
) {
	COMDLG32_FR_Data *pdata;
        DWORD len;

        TRACE("LPFINDREPLACE=%p\n", pfr);

	if(!COMDLG32_FR_CheckPartial((LPFINDREPLACEA)pfr, FALSE))
        	return 0;

        len = WideCharToMultiByte( CP_ACP, 0, pfr->lpstrFindWhat, pfr->wFindWhatLen,
                                   NULL, 0, NULL, NULL );
        if((pdata = COMDLG32_AllocMem(sizeof(COMDLG32_FR_Data) + len)) == NULL)
                return 0; /* Error has been set */

        pdata->user_fr.frw = pfr;
        pdata->fr = *(LPFINDREPLACEA)pfr;	/* FINDREPLACEx have same size */
	pdata->fr.Flags |= FR_WINE_UNICODE;
        pdata->fr.lpstrFindWhat = (LPSTR)(pdata + 1);  /* Set string pointer */
        WideCharToMultiByte( CP_ACP, 0, pfr->lpstrFindWhat, pfr->wFindWhatLen,
                             pdata->fr.lpstrFindWhat, len, NULL, NULL );
        return COMDLG32_FR_DoFindReplace(pdata);
}

/***********************************************************************
 *	ReplaceTextW 				[COMDLG32.@]
 *
 * Create a modeless replace-text dialog box.
 *
 *	RETURNS
 *		Window handle to created dialog: Success
 *		NULL: Failure
 */
HWND WINAPI ReplaceTextW(
	LPFINDREPLACEW pfr	/* [in] Find/replace structure*/
) {
	COMDLG32_FR_Data *pdata;
        DWORD len1, len2;

        TRACE("LPFINDREPLACE=%p\n", pfr);

	if(!COMDLG32_FR_CheckPartial((LPFINDREPLACEA)pfr, FALSE))
        	return 0;

        len1 = WideCharToMultiByte( CP_ACP, 0, pfr->lpstrFindWhat, pfr->wFindWhatLen,
                                    NULL, 0, NULL, NULL );
        len2 = WideCharToMultiByte( CP_ACP, 0, pfr->lpstrReplaceWith, pfr->wReplaceWithLen,
                                    NULL, 0, NULL, NULL );
        if((pdata = COMDLG32_AllocMem(sizeof(COMDLG32_FR_Data) + len1 + len2)) == NULL)
                return 0; /* Error has been set */

        pdata->user_fr.frw = pfr;
        pdata->fr = *(LPFINDREPLACEA)pfr;	/* FINDREPLACEx have same size */
	pdata->fr.Flags |= FR_WINE_REPLACE | FR_WINE_UNICODE;
        pdata->fr.lpstrFindWhat = (LPSTR)(pdata + 1);  /* Set string pointer */
        pdata->fr.lpstrReplaceWith = pdata->fr.lpstrFindWhat + len1;

        WideCharToMultiByte( CP_ACP, 0, pfr->lpstrFindWhat, pfr->wFindWhatLen,
                             pdata->fr.lpstrFindWhat, len1, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, pfr->lpstrReplaceWith, pfr->wReplaceWithLen,
                             pdata->fr.lpstrReplaceWith, len2, NULL, NULL );
        return COMDLG32_FR_DoFindReplace(pdata);
}
