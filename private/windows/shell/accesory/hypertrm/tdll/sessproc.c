/*
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 7 $
 *	$Date: 8/18/99 10:52a $
 */
// #define	DEBUGSTR	1

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <time.h>

#include "stdtyp.h"
#include "assert.h"
#include "globals.h"
#include "session.h"
#include "session.hh"
#include "term.h"
#include "print.h"
#include "cnct.h"
#include "misc.h"
#include "banner.h"
#include "file_msc.h"
#include "errorbox.h"
#include "load_res.h"
#include "sf.h"

#include <tdll\cloop.h>
#include <tdll\com.h>
#include <tdll\timers.h>
#include <tdll\capture.h>
#include <tdll\xfer_msc.h>
#include <term\res.h>
#include <emu\emu.h>
#include <emu\emudlgs.h>
#include <tdll\property.h>
#include <tdll\tchar.h>
#include <tdll\backscrl.h>
//mpt:08-22-97 added HTML help
#if defined(INCL_USE_HTML_HELP)
#include <htmlhelp.h>
#endif

#include "tdll.h"
#include "hlptable.h"
#include "statusbr.h"
//*jcm
#include "open_msc.h"
#include "mc.h"
//*end of jcm

#if defined(TESTMENU) && !defined(NDEBUG)
#include <cncttapi\cncttapi.h>
#endif

#ifdef INCL_KEY_MACROS
    #include "tdll\keyutil.h"
#endif

#ifdef INCL_NAG_SCREEN
    #include "tdll\nagdlg.h"
    #include "tdll\register.h"
#endif

STATIC_FUNC void 	SP_WM_SIZE(const HWND hwnd, const unsigned fwSizeType,
					       	   const int iWidth, const int iHite);

STATIC_FUNC void 	SP_WM_CREATE(const HWND hwnd, const CREATESTRUCT *pcs);
STATIC_FUNC void 	SP_WM_DESTROY(const HWND hwnd);

STATIC_FUNC LRESULT SP_WM_CMD(const HWND hwnd, const int nId,
							  const int nNotify, const HWND hwndCtrl);

STATIC_FUNC void 	SP_WM_INITMENUPOPUP(const HWND hwnd, const HMENU hMenu,
										const UINT uPos, const BOOL fSysMenu);

STATIC_FUNC void 	SP_WM_CONTEXTMENU(const HWND hwnd);
STATIC_FUNC BOOL 	SP_WM_CLOSE(const HWND hwnd);
STATIC_FUNC int 	CheckOpenFile(const HSESSION hSession, ATOM aFile);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SessProc
 *
 * DESCRIPTION:
 *	Main window proc for term
 *
 */
LRESULT CALLBACK SessProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	HSESSION hSession;
#if defined(INCL_USE_HTML_HELP)
	TCHAR achHtmlFilename[100];
#endif

	switch (uMsg)
		{
		// User pressed F1 key over the session window.
		// We also get this message from child windows if they do not process
		// it themselves.
		//
		case WM_HELP:
//#if 0 //mpt:3-10-98 for some reason, using this call causes an access violation
	    //            in HyperTrm.dll. Using the winhelp call gives us the same results.
        //mpt:4-30-98 Re-enabled for the NT folks - go figure
#if defined(INCL_USE_HTML_HELP)
		  	LoadString(glblQueryDllHinst(), IDS_HTML_HELPFILE, achHtmlFilename,
				sizeof(achHtmlFilename) / sizeof(TCHAR));

			HtmlHelp(0, achHtmlFilename, HH_HELP_FINDER, 0); //formely owned by hwnd - mpt
#else
			WinHelp(hwnd,
					glblQueryHelpFileName(),
					HELP_FINDER, // mrw:3/10/95
					(DWORD)(LPTSTR)"");
#endif
			return 0;

		case WM_CREATE:
			SP_WM_CREATE(hwnd, (CREATESTRUCT *)lPar);
			return 0;

		case WM_SIZE:
			SP_WM_SIZE(hwnd, (unsigned)wPar, LOWORD(lPar), HIWORD(lPar));
			return 0;

		case WM_COMMAND:
			return SP_WM_CMD(hwnd, LOWORD(wPar), HIWORD(wPar), (HWND)lPar);

		case WM_TIMER:
		case WM_FAKE_TIMER:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			TimerMuxProc(sessQueryTimerMux(hSession));
            return 0;

		case WM_SETFOCUS:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			SetFocus(sessQueryHwndTerminal(hSession));
			return 0;

		case WM_SYSCOLORCHANGE:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			SendDlgItemMessage(hwnd, IDC_TERMINAL_WIN, uMsg, wPar, lPar);
			return 0;

		case WM_GETMINMAXINFO:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			sessSetMinMaxInfo(hSession, (PMINMAXINFO)lPar);
			return 0;

		case WM_CONTEXTMENU:
			SP_WM_CONTEXTMENU(hwnd);
			return 0;

		case WM_INITMENUPOPUP:
			SP_WM_INITMENUPOPUP(hwnd, (HMENU)wPar, LOWORD(lPar), HIWORD(lPar));
			return 0;

		case WM_EXITMENULOOP:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_MAX_PARTS, 0);
			break;

		case WM_MENUSELECT:
			{
			TCHAR ach[128];

			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (HIWORD(wPar) & MF_POPUP)
				{
				SendMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_NOPARTS, 0, (LPARAM)(LPTSTR)"");
				return 0;
				}
			if (LOWORD(wPar))
				{
				LoadString(glblQueryDllHinst(),
							IDM_MENU_BASE+LOWORD(wPar),
							ach,
							sizeof(ach) / sizeof(TCHAR));
				SendMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_NOPARTS, 0, (LPARAM)(LPTSTR)ach);
				}
			}
			return 0;

		case WM_PASTE:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			PasteFromClipboardToHost(hwnd, hSession);
			return 0;

		case WM_DRAWITEM:
			if (wPar == IDC_STATUS_WIN)
				{
				hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

				sbr_WM_DRAWITEM(sessQueryHwndStatusbar(hSession),
					(LPDRAWITEMSTRUCT)lPar);

				return 1;
				}
			return 0;

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
			if (!SP_WM_CLOSE(hwnd))
				return 0;
			break;

		case WM_DESTROY:
			SP_WM_DESTROY(hwnd);
			return 0;

		/* --- Public Session Messages --- */

		case WM_NOTIFY:
			DecodeNotification(hwnd, wPar, lPar);
			return 1;

		case WM_SESS_NOTIFY:
			DecodeSessionNotification(hwnd, (NOTIFICATION)wPar, lPar);
			return 1;

		case WM_SESS_ENDDLG:
			if (IsWindow((HWND)lPar))
				{
				// I think that this needs to be done in this order.
				// Think about it.

				DestroyWindow((HWND)lPar);
				glblDeleteModelessDlgHwnd((HWND)lPar);
				}
			return 0;

		case WM_CMDLN_DIAL:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

			// mrw:4/21/95 by microsoft's request
			//if (sessQueryWindowShowCmd(hSession) != SW_SHOWMINIMIZED)

			sessCmdLnDial(hSession);
			return 0;

		case WM_SESS_SIZE_SHOW:
			sessSizeAndShow(hwnd, (int)wPar);
			return 0;
  		
		case WM_CNCT_DIALNOW:
			// wPar contains connection flags
			//
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			cnctConnect(sessQueryCnctHdl(hSession), (unsigned int)wPar);
			return 0;

		case WM_DISCONNECT:
			// wPar contains disconnection flags
			//
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			cnctDisconnect(sessQueryCnctHdl(hSession), (unsigned int)wPar);
			return 0;

		case WM_HT_QUERYOPENFILE:
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			return CheckOpenFile(hSession, (ATOM)lPar);

		case WM_SESS_SHOW_SIDEBAR:
			// Autoloads can't do this directly or they hang hypertrm.
			//
			hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			ShowWindow(sessQuerySidebarHwnd(hSession), SW_SHOW);

			// Force terminal window to fit
			//
			SendDlgItemMessage(hwnd, IDC_TERMINAL_WIN, WM_SIZE, 0, 0);
			return 0;

        case WM_ERROR_MSG:
            {
            TCHAR   ach[128];

			LoadString(glblQueryDllHinst(),
                (UINT)wPar,
                ach,
                sizeof(ach)/sizeof(TCHAR));

			MessageBox(hwnd, ach, "Message", MB_OK);
            }

            return 0;

		default:
			break;
		}

	return DefWindowProc(hwnd, uMsg, wPar, lPar);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_CMD
 *
 * DESCRIPTION:
 *	WM_COMMAND processor for SessProc()
 *
 * ARGUMENTS:
 *	hwnd		- session window handle
 *	nId 		- item, control, or accelerator identifier
 *	nNotify 	- notification code
 *	hwndCtrl	- handle of control
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC LRESULT SP_WM_CMD(const HWND hwnd, const int nId, const int nNotify,
					  const HWND hwndCtrl)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	DWORD 		   dwCnt;
	void		   *pv;
	LPTSTR		   pszFileName;
    TCHAR          ach[100], achList[100], achDir[_MAX_PATH];
#if defined(INCL_USE_HTML_HELP)
    TCHAR          achHtmlFilename[100];
#endif

	switch (nId)
		{
		case IDC_TOOLBAR_WIN:
			/* Got a notification from the toolbar */
			return ToolbarNotification(hwnd, nId, nNotify, hwndCtrl);

		/* --- File Menu --- */

		case IDM_NEW:
			if (!sessDisconnectToContinue(hSession, hwnd))
				break;

			if (SaveSession(hSession, hwnd))
				{
				if (ReinitializeSessionHandle(hSession, TRUE) == FALSE)
					break;

				cnctConnect(sessQueryCnctHdl(hSession), CNCT_NEW);
				}

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_KEY_PARTS, 0);
			break;

		case IDM_OPEN:
			if (!sessDisconnectToContinue(hSession, hwnd))
				break;

			// In the OpenSession() we will ask if the user wants to save
			// existing opened new session, or save silently, only after the
			// user has commited to opening a new one by pressing the OK button.
			// -jac. 10-06-94 03:56pm
			if (OpenSession(hSession, hwnd) >= 0)
				{
				PostMessage(sessQueryHwndStatusbar(hSession),
					SBR_NTFY_REFRESH, (WPARAM)SBR_MAX_PARTS, 0);

				// Run through the connection procedure. This message is
				// Posted instead of calling cnctConnect directly to avoid
				// a focus problem on the connection dialog.  Calling
				// cnctConnect from here leaves focus on the terminal screen,
				// instead of on the connection dialog, which is where we want it.
				//
				PostMessage(hwnd, WM_COMMAND, IDM_ACTIONS_DIAL, 0);
				}

			break;

		case IDM_SAVE:
			SilentSaveSession(hSession, hwnd, TRUE);
			break;

		case IDM_SAVE_AS:
			SaveAsSession(hSession, hwnd);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_KEY_PARTS, 0);
			break;

		case IDA_CONTEXT_MENU:		// SHIFT+F10
			SP_WM_CONTEXTMENU(hwnd);
			return 0;

		case IDM_PAGESETUP:
		case IDM_CHOOSEPRINT:
 			printPageSetup(sessQueryPrintHdl(hSession), hwnd);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_KEY_PARTS, 0);
			break;

		case IDM_PRINT:
		case IDM_CONTEXT_PRINT:
			printsetSetup(sessQueryPrintHdl(hSession), hwnd);
			break;

		case IDM_PROPERTIES:
			DoInternalProperties(hSession, hwnd);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_ALL_PARTS, 0);
			break;

		case IDM_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		/* --- Edit Menu --- */

		case IDM_SELECT_ALL:
		case IDM_CONTEXT_SELECT_ALL:
			SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_MARK_ALL, 0, 0);
			break;

		case IDM_PASTE:
		case IDM_CONTEXT_PASTE:
			SendMessage(hwnd, WM_PASTE, 0, 0L);
			break;

		case IDM_COPY:
		case IDM_CONTEXT_COPY:
			if (CopyMarkedTextFromTerminal(hSession, &pv, &dwCnt, TRUE))
				{
				CopyBufferToClipBoard(hwnd, dwCnt, pv);
				SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_UNMARK, 0, 0);
				free(pv);	// free allocated buffer from terminal
				pv = NULL;
				}
			break;

#if defined(INCL_TERMINAL_CLEAR)
		case IDM_CLEAR_BACKSCROLL:
		case IDM_CONTEXT_CLEAR_BACKSCROLL:
			backscrlFlush(sessQueryBackscrlHdl(hSession));
			break;

		case IDM_CLEAR_SCREEN:
		case IDM_CONTEXT_CLEAR_SCREEN:
            emuEraseTerminalScreen(sessQueryEmuHdl(hSession));
            emuHomeHostCursor(sessQueryEmuHdl(hSession));
        	NotifyClient(hSession, EVENT_TERM_UPDATE, 0);
	        RefreshTermWindow(sessQueryHwndTerminal(hSession));
           	break;
#endif

		/* --- View Menu --- */

		case IDM_VIEW_TOOLBAR:
			sessSetToolbarVisible(hSession,
				!sessQueryToolbarVisible(hSession));

			SP_WM_SIZE(hwnd, (unsigned)(-1), 0, 0);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_MAX_PARTS, 0);
			break;

		case IDM_VIEW_STATUS:
			sessSetStatusbarVisible(hSession,
				!sessQueryStatusbarVisible(hSession));

			SP_WM_SIZE(hwnd, (unsigned)(-1), 0, 0);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_MAX_PARTS, 0);
			break;

		case IDM_VIEW_FONTS:
			DisplayFontDialog(hSession, FALSE);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_KEY_PARTS, 0);
			break;

        case IDM_VIEW_SNAP:
		case IDM_CONTEXT_SNAP:
			sessSnapToTermWindow(hwnd);
			break;

#ifdef INCL_KEY_MACROS
		case IDM_KEY_MACROS:
			DoDialog(glblQueryDllHinst(),
					MAKEINTRESOURCE(IDD_KEYSUMMARYDLG),
					hwnd,
					KeySummaryDlg,
					(LPARAM)hSession);
            break;
#endif

		/* --- Actions Menu --- */

		case IDM_ACTIONS_DIAL:
 			cnctConnect(sessQueryCnctHdl(hSession), 0);

			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_KEY_PARTS, 0);
			break;

		case IDM_ACTIONS_HANGUP:
			//mpt:10-28-97 added exit upon disconnect feature
			cnctDisconnect(sessQueryCnctHdl(hSession),
				sessQueryExit(hSession) ? DISCNCT_EXIT : 0);
			break;

		case IDM_ACTIONS_SEND:
		case IDM_CONTEXT_SEND:
			DoDialog(glblQueryDllHinst(),
					MAKEINTRESOURCE(IDD_TRANSFERSEND),
					hwnd,				/* parent window */
					TransferSendDlg,
					(LPARAM)hSession);
			break;

		case IDM_ACTIONS_RCV:
		case IDM_CONTEXT_RECEIVE:
			/*
			 * This will probably need to be modeless later.
			 * Maybe only for Upper Wacker.
			 */
			DoDialog(glblQueryDllHinst(),
					MAKEINTRESOURCE(IDD_TRANSFERRECEIVE),
					hwnd,
					TransferReceiveDlg,
					(LPARAM)hSession);
			break;

		case IDM_ACTIONS_CAP:
			DoDialog(glblQueryDllHinst(),
					MAKEINTRESOURCE(IDD_CAPTURE),
					hwnd,
					CaptureFileDlg,
					(LPARAM)hSession);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_CAPT_PART_NO, 0);
			break;

		case IDM_CAPTURE_STOP:
			cpfSetCaptureState(sessQueryCaptureFileHdl(hSession),
								CPF_CAPTURE_OFF);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_CAPT_PART_NO, 0);
			break;

		case IDM_CAPTURE_PAUSE:
			cpfSetCaptureState(sessQueryCaptureFileHdl(hSession),
								CPF_CAPTURE_PAUSE);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_CAPT_PART_NO, 0);
			break;

		case IDM_CAPTURE_RESUME:
			cpfSetCaptureState(sessQueryCaptureFileHdl(hSession),
								CPF_CAPTURE_ON);
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_CAPT_PART_NO, 0);
			break;

		case IDM_ACTIONS_SEND_TEXT:
			LoadString(glblQueryDllHinst(), IDS_SND_TXT_FILE, ach,
				sizeof(ach)/sizeof(TCHAR));

			resLoadFileMask(glblQueryDllHinst(), IDS_CPF_FILES1, 2, achList,
				sizeof(achList) / sizeof(TCHAR));

			//Changed to use working folder rather than current folder - mpt 8-18-99
            if ( !GetWorkingDirectory( achDir, sizeof(achDir) / sizeof(TCHAR)) )
				{
				GetCurrentDirectory(sizeof(achDir) / sizeof(TCHAR), achDir);
				}

			pszFileName = gnrcFindFileDialog(hwnd, ach, achDir, achList);

			if (pszFileName)
				{
				CLoopSendTextFile(sessQueryCLoopHdl(hSession), pszFileName);
				free(pszFileName);
				pszFileName = NULL;
				}
			break;

		case IDM_ACTIONS_PRINT:
			if (!printQueryStatus(emuQueryPrintEchoHdl(sessQueryEmuHdl(hSession))))
				{
				if (printVerifyPrinter(sessQueryPrintHdl(hSession)) == -1)
					break;
				}
			else
				{
				printEchoClose(emuQueryPrintEchoHdl(sessQueryEmuHdl(hSession)));
				}

			printStatusToggle(emuQueryPrintEchoHdl(sessQueryEmuHdl(hSession)));
			PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
				(WPARAM)SBR_PRNE_PART_NO, 0);
			break;

        case IDM_ACTIONS_WAIT_FOR_CALL:
 			cnctConnect(sessQueryCnctHdl(hSession), CNCT_ANSWER);
            break;

        case IDM_ACTIONS_STOP_WAITING:
            cnctDisconnect(sessQueryCnctHdl(hSession), DISCNCT_NOBEEP);
            break;

		/* --- Help Menu --- */

		case IDM_HELPTOPICS:
//#if 0 //mpt:3-10-98 for some reason, using this call causes an access violation
	  //            in HyperTrm.dll. Using the winhelp call gives us the same results.
        //mpt:4-30-98 Re-enabled for the NT folks - go figure
#if defined(INCL_USE_HTML_HELP)
		  	LoadString(glblQueryDllHinst(), IDS_HTML_HELPFILE, achHtmlFilename,
				sizeof(achHtmlFilename) / sizeof(TCHAR));

			HtmlHelp(0, achHtmlFilename, HH_HELP_FINDER, 0);
#else
			WinHelp(hwnd,
					glblQueryHelpFileName(),
					HELP_FINDER,	// mrw:3/10/95
					(DWORD)(LPTSTR)"");
#endif
			break;

#if defined(INCL_NAG_SCREEN)
        case IDM_PURCHASE_INFO:
            DoUpgradeDlg(hwnd);
            break;

        case IDM_REG_CODE:
            DoRegisterDlg(hwnd);
            break;

        case IDM_REGISTER:
            DoRegister();
            break;

        case IDM_DISCUSSION:
            ShellExecute(NULL, "open", TEXT("http://www.hilgraeve.com/discuss"), NULL, NULL, SW_SHOW);
            break;
#endif		
        case IDM_ABOUT:
			AboutDlg(hwnd);
			break;

		/* --- Session Context Menu --- */

		// Other context menu items are placed with their main menu
		// equivalents.

		#if 0 // mrw, 1/27/95
		case IDM_CONTEXT_WHATS_THIS:
			WinHelp(hwnd, glblQueryHelpFileName(), HELP_CONTEXTPOPUP,
				(DWORD)(LPTSTR)IDH_TERM_CONTEXT_WHATS_THIS);
			break;
		#endif

		#if defined(TESTMENU) && !defined(NDEBUG)
		/* --- Test Menu --- */

		case IDM_TEST_SAVEAS:
			SaveAsSession(hSession, hwnd);
			break;

		case IDM_TEST_CLEARTERM:
			break;

		case IDM_TEST_CLEARBACK:
			break;

		case IDM_TEST_SELECTTERM:
			break;

		case IDM_TEST_SELECTBACK:
			break;

		case IDM_TEST_TESTFILE:
			{
			pszFileName = 0;

			pszFileName = gnrcFindFileDialog(hwnd,
							"Emulator test file",
							"D:\\WACKER",
							"Text files\0*.TXT\0Ansi files\0*.ans\0VT100 files\0*.100");

			if (pszFileName)
				{
				CLoopSendTextFile(sessQueryCLoopHdl(hSession), pszFileName);

				free(pszFileName);
				pszFileName = NULL;
				}
			}
			break;

		case IDM_TEST_BEZEL:
			SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_BEZEL, 0, 0);
			break;

		case IDM_TEST_SNAP:
			sessSnapToTermWindow(hwnd);
			break;

		case IDM_TEST_NEW_CONNECTION:
			DoDialog(glblQueryDllHinst(),
					MAKEINTRESOURCE(IDD_NEWCONNECTION),
					hwnd,
					NewConnectionDlg,
					(LPARAM)hSession);
			break;

		case IDM_TEST_FLUSH_BACKSCRL:
			backscrlFlush(sessQueryBackscrlHdl(hSession));
			break;

		case IDM_TEST_LOAD_ANSI:
			emuLoad(sessQueryEmuHdl(hSession), EMU_ANSI);
			break;

		case IDM_TEST_LOAD_MINITEL:
			emuLoad(sessQueryEmuHdl(hSession), EMU_MINI);
			break;

		case IDM_TEST_LOAD_VIEWDATA:
			emuLoad(sessQueryEmuHdl(hSession), EMU_VIEW);
			break;

		case IDM_TEST_LOAD_AUTO:
			emuLoad(sessQueryEmuHdl(hSession), EMU_AUTO);
			break;

		case IDM_TEST_LOAD_TTY:
			emuLoad(sessQueryEmuHdl(hSession), EMU_TTY);
			break;

		case IDM_TEST_LOAD_VT52:
			emuLoad(sessQueryEmuHdl(hSession), EMU_VT52);
			break;

		case IDM_TEST_LOAD_VT100:
			emuLoad(sessQueryEmuHdl(hSession), EMU_VT100);
			break;

#if defined(INCL_VT220)
		case IDM_TEST_LOAD_VT220:
			emuLoad(sessQueryEmuHdl(hSession), EMU_VT220);
			break;
#endif

#if defined(INCL_VT320)
		case IDM_TEST_LOAD_VT320:
			emuLoad(sessQueryEmuHdl(hSession), EMU_VT320);
			break;
#endif

		case IDM_TEST_SESSNAME:
			{
			TCHAR ach[256];
			sessQueryName(hSession, ach, sizeof(ach));
			MessageBox(hwnd, ach, "Message", MB_OK);
			}
			break;

		#endif

		default:
			break;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_CREATE
 *
 * DESCRIPTION:
 *	Does the WM_CREATE stuff for the frameproc window.
 *
 * ARGUMENTS:
 *	hwnd	- frame window handle.
 *  *pcs    - pointer to CREATESTRUCT, structure passed from CreateWindowEx().
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void SP_WM_CREATE(const HWND hwnd, const CREATESTRUCT *pcs)
	{
	HSESSION  hSession;

	hSession = CreateSessionHandle(hwnd);

	// Need to set even if hSession is zero so the destroy handle routine
	// does not try to destroy a non-handle.

	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hSession);

	if (hSession == 0)
		{
		assert(FALSE);
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		return;
		}

	if (InitializeSessionHandle(hSession, hwnd, pcs) == FALSE)
		{
		assert(FALSE);
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		return;
		}

	if (glblQueryProgramStatus())
		{
		/* Something has shut use down, don't continue */
		return;
		}

	// mrw, 1/27/95 SetWindowContextHelpId(hwnd, IDH_TERM_WINDOW);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_DESTROY
 *
 * DESCRIPTION:
 *	WM_DESTROY message processor for session window.
 *
 * ARGUMENTS:
 *	hwnd	- session window handle.
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void SP_WM_DESTROY(const HWND hwnd)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (GetFileSizeFromName(glblQueryHelpFileName(), NULL))
		WinHelp(hwnd, glblQueryHelpFileName(), HELP_QUIT, 0L);

	// It appears that our subclassed statusbar window doesn't get the
	// WM_DESTROY message when its parent, the session window, is getting
	// destroyed, so we force it...
	//
	DestroyWindow(sessQueryHwndStatusbar(hSession));

	if (hSession)
		DestroySessionHandle(hSession);

	PostQuitMessage(0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_SIZE
 *
 * DESCRIPTION:
 *	WM_SIZE message processor for sessproc.
 *
 * ARGUMENTS:
 *	hwnd		- session window
 *	fwSizeType	- from WM_SIZE
 *	iWidth		- width of window
 *	iHige		- hite of window
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void SP_WM_SIZE(const HWND hwnd,
					   const unsigned fwSizeType,
					   const int iWidth,
					   const int iHite)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	HWND hwndDisplay;

	// DbgOutStr("WM_SIZE %d (%d x %d)\r\n", fwSizeType, iWidth, iHite, 0,0);

	/*
	 * We need a bunch of fiddling around for the transfer display
	 */
	if (hSession)
		{
		hwndDisplay = xfrGetDisplayWindow(sessQueryXferHdl(hSession));
		if (IsWindow(hwndDisplay))
			{
			switch (fwSizeType)
				{
				case SIZE_MINIMIZED:
				case SIZE_MAXHIDE:
					// DbgOutStr("Iconic\r\n", 0,0,0,0,0);
					if (IsWindowVisible(hwndDisplay))
						ShowWindow(hwndDisplay, SW_HIDE);
					break;
				case SIZE_MAXIMIZED:
				case SIZE_RESTORED:
				case SIZE_MAXSHOW:
					if (!IsWindowVisible(hwndDisplay))
						ShowWindow(hwndDisplay, SW_SHOWDEFAULT);
					break;
				default:
					break;
				}
			}
		}

	SendDlgItemMessage(hwnd, IDC_STATUS_WIN, WM_SIZE, 0, 0);
	SendDlgItemMessage(hwnd, IDC_TOOLBAR_WIN, WM_SIZE, 0, 0);
	SendDlgItemMessage(hwnd, IDC_TERMINAL_WIN, WM_SIZE, 0, 0);
	SendDlgItemMessage(hwnd, IDC_SIDEBAR_WIN, WM_SIZE, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_INITMENUPOPUP
 *
 * DESCRIPTION:
 *	WM_INITMENUPOPUP message handler for session window.
 *
 * ARGUMENTS:
 *	hwnd	- session window handle
 *	hMenu	- menu handle of popup
 *	uPos	- position of menu item that invoked popup
 *	fSysMenu- TRUE if system menu popup
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void SP_WM_INITMENUPOPUP(const HWND hwnd, const HMENU hMenu,
								const UINT uPos, const BOOL fSysMenu)
	{
	HMENU 	hWinMenu;
	int 	nOK, nIdx;
	TCHAR 	ach[50];

	// Popup menus are referenced by position since id's can't be assigned
	// (grrrrr!).  Actual menu init functions are in sessmenu.c

	#define MENU_FILE_POS		0
	#define MENU_EDIT_POS		1
	#define MENU_VIEW_POS		2
	#define MENU_CALL_POS		3
	#define MENU_TRANSFER_POS	4
	#define MENU_HELP_POS		5

	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	// Display help for top menu item.
	// I suppose we will have to be read from the resource file once we know
	// what the strings and their ids are.
	//
	LoadString(glblQueryDllHinst(),
				IDM_MENU_BASE+uPos,
				ach,
				sizeof(ach) / sizeof(TCHAR));
	//wsprintf(ach, "Help for menu item %d\0", uPos);
	SendMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_NOPARTS, 0, (LPARAM)(LPTSTR)ach);

	if (fSysMenu)
		return;

	/*
	 * This makes sure we only handle top level menu items here.
	 *
	 * The problem is that secondary popup menus also cause a WM_INITPOPUP
	 * message to get sent.  The only way to tell one of these messages from
	 * another is to check the menu handle.  As MRW alluded to above, this
	 * would be less of a problem if they did this stuff with IDs instead of
	 * offsets.
	 */

	hWinMenu = GetMenu(hwnd);
	nOK = FALSE;

	for (nIdx = 0; nIdx <= MENU_HELP_POS; nIdx += 1)
		{
		if (hMenu == GetSubMenu(hWinMenu, nIdx))
			nOK = TRUE;
		}

	if (!nOK)
		return;

	/* --- Ok, its a top-level menu, let's have at it --- */

	switch (uPos)
		{
	case MENU_FILE_POS:
		break;

	case MENU_EDIT_POS:
		sessInitMenuPopupEdit(hSession, hMenu);
		break;

	case MENU_VIEW_POS:
		sessInitMenuPopupView(hSession, hMenu);
		break;

	case MENU_CALL_POS:
		sessInitMenuPopupCall(hSession, hMenu);
		break;

	case MENU_TRANSFER_POS:
		sessInitMenuPopupTransfer(hSession, hMenu);
		break;

	case MENU_HELP_POS:
		sessInitMenuPopupHelp(hSession, hMenu);
        break;

	default:
		break;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DecodeSessionNotification
 *
 * DESCRIPTION:
 *	Receives a NotifyClient event and routes the notification.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle
 *	nEvent		- notification event
 *	lExtra		- additional data to pass
 *
 * RETURNS:
 *	void
 *
 */
void DecodeSessionNotification(const HWND hwndSession,
								const NOTIFICATION nEvent,
								const LPARAM lExtra)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwndSession,
															GWLP_USERDATA);

	switch (nEvent) /*lint -e787 -e788 */
		{
	case EVENT_TERM_UPDATE:
		// This message must be sent, not posted so initialization
		// at program startup works - mrw
		SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_GETUPDATE, 0, 0);
		break;

	case EVENT_TERM_TRACK:
		PostMessage(sessQueryHwndTerminal(hSession), WM_TERM_TRACK, 0, 0);
		break;

	case EVENT_EMU_CLRATTR:
		// This message must be sent, not posted so initialization
		// at program startup works - mrw
		SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_CLRATTR, 0, 0);
		break;

	case EVENT_EMU_SETTINGS:
		// This message must be sent, not posted so initialization
		// at program startup works - mrw
		SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_EMU_SETTINGS, 0, 0);
		break;

	case EVENT_CONNECTION_OPENED:
		//cnctMessage(sessQueryCnctHdl(hSession), IDS_CNCT_OPEN);
		emuNotify(sessQueryEmuHdl(hSession), EMU_EVENT_CONNECTED);
		cnctSetStartTime(sessQueryCnctHdl(hSession));
		CLoopControl(sessQueryCLoopHdl(hSession), CLOOP_SET, CLOOP_CONNECTED);
		break;

	case EVENT_CONNECTION_INPROGRESS:
		emuNotify(sessQueryEmuHdl(hSession), EMU_EVENT_CONNECTING);
		break;

	case EVENT_CONNECTION_CLOSED:
		//cnctMessage(sessQueryCnctHdl(hSession), IDS_CNCT_CLOSE);
		emuNotify(sessQueryEmuHdl(hSession), EMU_EVENT_DISCONNECTED);
		CLoopControl(sessQueryCLoopHdl(hSession), CLOOP_CLEAR, CLOOP_CONNECTED);
		break;

	case EVENT_HOST_XFER_REQ:
		xfrDoAutostart(sessQueryXferHdl(hSession), (long)lExtra);
		break;

	case EVENT_CLOOP_SEND:
		PostMessage(sessQueryHwndTerminal(hSession), WM_TERM_TRACK,
			(WPARAM)1, 0);
		break;

	case EVENT_PORTONLY_OPEN:
		cnctConnect(sessQueryCnctHdl(hSession), CNCT_PORTONLY);
		break;

    case EVENT_LOST_CONNECTION:
        cnctDisconnect(sessQueryCnctHdl(hSession), 0);
        break;

	default:
		break;
		} /*lint +e787 +e788 */

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DecodeNotification
 *
 * DESCRIPTION:
 *	Receives a NotifyClient event and routes the notification.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle
 *	wPar        - standard wPar to window proc
 *	lPar        - standard lPar to window proc, points to NMHDR structure for
 *	              details of notification.  See WM_NOTIFY.
 *
 * RETURNS:
 *	void
 *
 */
void DecodeNotification(const HWND hwndSession, WPARAM wPar, LPARAM lPar)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwndSession, GWLP_USERDATA);
	NMHDR *pN = (NMHDR *)lPar;

	switch (pN->code) /*lint -e787 -e788 */
		{
	case TTN_NEEDTEXT:
		ToolbarNeedsText(hSession, (long)lPar);
		break;

	default:
		break;
		} /*lint +e787 +e788 */

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SP_WM_CONTEXTMENU
 *
 * DESCRIPTION:
 *	WM_CONTEXTMENU message handler for session window.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 */
STATIC_FUNC void SP_WM_CONTEXTMENU(const HWND hwnd)
	{
	RECT  rc;
	POINT pt;
	HWND  hwndToolbar, hwndStatus;
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	GetCursorPos((LPPOINT)&pt);
	hwndToolbar = sessQueryHwndToolbar(hSession);
	if (IsWindowVisible(hwndToolbar))
		{
		GetClientRect(hwndToolbar, (LPRECT)&rc);
		ScreenToClient(hwndToolbar, (LPPOINT)&pt);
		if (PtInRect((LPRECT)&rc, pt))
			return;
		}

	GetCursorPos((LPPOINT)&pt);
	hwndStatus = sessQueryHwndStatusbar(hSession);
	if (IsWindowVisible(hwndStatus))
		{
		GetClientRect(hwndStatus, (LPRECT)&rc);
		ScreenToClient(hwndStatus, (LPPOINT)&pt);
		if (PtInRect((LPRECT)&rc, pt))
			return;
		}
	GetCursorPos((LPPOINT)&pt);
	GetClientRect(hwnd, (LPRECT)&rc);
	ScreenToClient(hwnd, (LPPOINT)&pt);

	if (PtInRect((LPRECT)&rc, pt))
		HandleContextMenu(hwnd, pt);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  SP_WM_CLOSE
 *
 * DESCRIPTION:
 *  WM_CLOSE message processing.
 *
 * ARGUMENTS:
 *  hwnd - session window.
 *
 * RETURNS:
 *  TRUE if all OK, FALSE otherwise.
 *
 */
STATIC_FUNC BOOL SP_WM_CLOSE(const HWND hwnd)
	{
	HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (!sessDisconnectToContinue(hSession, hwnd))
		return FALSE;

	if (!SaveSession(hSession, hwnd))
		return FALSE;

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CheckOpenFile
 *
 * DESCRIPTION:
 *	Checks if given atom matches current system file name
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *	aFile		- atom of session file
 *
 * RETURNS:
 *	TRUE if current system file matches the one in the atom
 *
 * AUTHOR: Mike Ward, 27-Jan-1995
 */
static int CheckOpenFile(const HSESSION hSession, ATOM aFile)
	{
	TCHAR ach[256];
	TCHAR achSessFile[256];

	ach[0] = TEXT('\0');

	if (GlobalGetAtomName(aFile, ach, sizeof(ach)))
		{
		achSessFile[0] = TEXT('\0');

		sfGetSessionFileName(sessQuerySysFileHdl(hSession),
			sizeof(achSessFile), achSessFile);

		return !StrCharCmpi(achSessFile, ach);
		}

	return FALSE;
	}
