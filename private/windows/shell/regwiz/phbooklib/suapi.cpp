// ############################################################################
#include "pch.hpp" 
#include "phbk.h"
#include "debug.h"
#include "phbkrc.h"
#include "misc.h"
//#include "ras.h"
#include <ras.h>
#pragma pack (4)
//#if !defined(WIN16)
//#include <rnaph.h>
//#endif
#pragma pack ()
#include "suapi.h"

#define ERROR_USER_EXIT 0x8b0bffff
#define MB_MYERROR (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)

#define NOTIFY_CODE (HIWORD(wParam))

#define WM_SHOWSTATEMSG WM_USER+1

char szTemp[100];

char szValidPhoneCharacters[] = {"0123456789AaBbCcDdPpTtWw!@$ -.()+*#,&\0"};

// ############################################################################
BOOL CSelectNumDlg::FHasPhoneNumbers(LPLINECOUNTRYENTRY pLCE)
{
	LPIDLOOKUPELEMENT pIDLookUp;
	IDLOOKUPELEMENT LookUpTarget;
	CPhoneBook far *pcPBTemp;
	PACCESSENTRY pAE = NULL, pAELast = NULL;
	DWORD dwCountryID;

	pcPBTemp = ((CPhoneBook far*)m_dwPhoneBook);

	LookUpTarget.dwID = pLCE->dwCountryID;

	pIDLookUp = NULL;
	pIDLookUp = (LPIDLOOKUPELEMENT)bsearch(&LookUpTarget,pcPBTemp->m_rgIDLookUp,
		(int)pcPBTemp->m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIDLookUpElements);

	if (!pIDLookUp) return FALSE; // no such country

	pAE = pIDLookUp->pFirstAE;
	if (!pAE) return FALSE; // no phone numbers at all

	dwCountryID = pAE->dwCountryID;

	pAELast = &(pcPBTemp->m_rgPhoneBookEntry[pcPBTemp->m_cPhoneBookEntries - 1]);
	while (pAELast > pAE && 
		   pAE->dwCountryID == dwCountryID)
	{
		if ((pAE->fType & m_bMask) == m_fType) return TRUE;
		pAE++;
	}
	return FALSE; // no phone numbers of the right type

//	return ((BOOL)(pIDLookUp->pFirstAE));
}

// ############################################################################
LRESULT CSelectNumDlg::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT lResult)
{
	LRESULT lRet = TRUE;
	unsigned int idx;
	int iCurIndex;
	int iLastIndex;
	PACCESSENTRY pAE = NULL;
	LPSTR p, p2;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		CPhoneBook far *pcPBTemp;
		pcPBTemp = ((CPhoneBook far *)m_dwPhoneBook);
		m_hwndDlg = hwndDlg;

		// figure out if we are in the middle of an AUTODIAL
		//

		if (m_dwFlags & AUTODIAL_IN_PROGRESS)
		{
			EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDBACK),FALSE);
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLSIGNUP),SW_HIDE);
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLDIALERR),SW_HIDE);
		} else if (m_dwFlags& DIALERR_IN_PROGRESS){
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLSIGNUP),SW_HIDE);
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLAUTODIAL),SW_HIDE);
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_CMDBACK),SW_HIDE);
			SetDlgItemText(m_hwndDlg,IDC_CMDNEXT,GetSz(IDS_OK));
		}else {
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLAUTODIAL),SW_HIDE);
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLDIALERR),SW_HIDE);
		}

		// Fill in country list and select current country
		//

		iCurIndex = -1;		// 0xFFFFFFFF
		// NOTE: it might be nice for INTL testing purposes to fill this combo box with
		// a list sorted by the country ID instead of the country name.
		for (idx=0;idx<pcPBTemp->m_pLineCountryList->dwNumCountries;idx++)
		{
			if (FHasPhoneNumbers(pcPBTemp->m_rgNameLookUp[idx].pLCE))
			{
				wsprintf(szTemp,"%s (%ld)",
							pcPBTemp->m_rgNameLookUp[idx].psCountryName,
							pcPBTemp->m_rgNameLookUp[idx].pLCE->dwCountryID);
				iLastIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,
														CB_ADDSTRING,0,
														(LPARAM)((LPSTR) &szTemp[0]));
				SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_SETITEMDATA,
									(WPARAM)iLastIndex,
									(LPARAM)pcPBTemp->m_rgNameLookUp[idx].pLCE->dwCountryID);
				if (pcPBTemp->m_rgNameLookUp[idx].pLCE->dwCountryID == m_dwCountryID)
				{
					iCurIndex = iLastIndex;
				}
			}
		}

		if (iCurIndex != -1)	// 0xFFFFFFFF
		{
			SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_SETCURSEL,(WPARAM)iCurIndex,0);
		} else {
			SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_SETCURSEL,0,0);
			iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_GETITEMDATA,0,0);
			if (iCurIndex != CB_ERR) m_dwCountryID = iCurIndex;
		}

		// Copy country to label
		//
		if (GetDlgItemText(m_hwndDlg,IDC_CMBCOUNTRY,szTemp,100))
		{
			SetDlgItemText(m_hwndDlg,IDC_LBLCOUNTRY,szTemp);
		}

		// Initialize Last Selection Method
		//

		m_dwFlags &= (~FREETEXT_SELECTION_METHOD);
		m_dwFlags |= PHONELIST_SELECTION_METHOD;

		// Fill in region list and select current region
		//
		FillRegion();

		// Fill in phone numbers
		//
		FillNumber();

		SetFocus(GetDlgItem(m_hwndDlg,IDC_CMBCOUNTRY));
		lRet = FALSE;
		break;
	case WM_SHOWSTATEMSG:
		if (wParam)
		{
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLSTATEMSG),SW_SHOW);
		} else {
			ShowWindow(GetDlgItem(m_hwndDlg,IDC_LBLSTATEMSG),SW_HIDE);
		}
		break;
		// 1/9/96 jmazner  added for Normandy #13185
	case WM_CLOSE:
		if ((m_dwFlags & (AUTODIAL_IN_PROGRESS|DIALERR_IN_PROGRESS)) == 0) 
		{
			if (MessageBox(hwndDlg,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
				MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
				EndDialog(hwndDlg,IDC_CMDCANCEL);
		} else {
			EndDialog(hwndDlg,IDC_CMDCANCEL);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_CMBCOUNTRY:
				if (NOTIFY_CODE == CBN_SELCHANGE)
				{
					iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_GETCURSEL,0,0);
					if (iCurIndex == CB_ERR) break;

					iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBCOUNTRY,CB_GETITEMDATA,(WPARAM)iCurIndex,0);
					if (iCurIndex == CB_ERR) break;
					m_dwCountryID = iCurIndex;  //REVIEW: data type????

					FillRegion();
					m_wRegion = 0;

					FillNumber();
				}
				break;
			case IDC_CMBREGION:
				if (NOTIFY_CODE == CBN_SELCHANGE)
				{
					iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_GETCURSEL,0,0);
					if (iCurIndex == CB_ERR) break;

					iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_GETITEMDATA,(WPARAM)iCurIndex,0);
					if (iCurIndex == CB_ERR) break;
					m_wRegion = (WORD) iCurIndex; //REVIEW: data type???

					FillNumber();
				}
				break;
			case IDC_CMBNUMBER:
				if ((NOTIFY_CODE == CBN_SELCHANGE) || (NOTIFY_CODE == CBN_DROPDOWN))
				{
					//iCurIndex = SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_GETCURSEL,0,0);
					//if (iCurIndex == CB_ERR) break;

					EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDNEXT),TRUE);

					//iCurIndex = SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_GETITEMDATA,(WPARAM)iCurIndex,0);
					//if (iCurIndex == CB_ERR) break;
					//if (!lstrcpy(&m_szDunFile[0],&((PACCESSENTRY)iCurIndex)->szDataCenter[0]))
					//{
					//	AssertSz(0,"Failed to copy data center from AE\n");
					//	break;
					//}

					// Set Last Selection Method
					//

					m_dwFlags &= (~FREETEXT_SELECTION_METHOD);
					m_dwFlags |= PHONELIST_SELECTION_METHOD;

				} else if (NOTIFY_CODE == CBN_EDITCHANGE) {

					if (SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,WM_GETTEXTLENGTH,0,0))
					{
						EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDNEXT),TRUE);
					} else {
						EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDNEXT),FALSE);
					}

					// Set Last Selection Method
					//

					m_dwFlags &= (~PHONELIST_SELECTION_METHOD);
					m_dwFlags |= FREETEXT_SELECTION_METHOD;
				}

				break;
			case IDC_CMDNEXT:
				if ((m_dwFlags & PHONELIST_SELECTION_METHOD) == PHONELIST_SELECTION_METHOD)
				{
					DWORD dwItemData;

					iCurIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_GETCURSEL,0,0);
					if (iCurIndex == CB_ERR) break;

					dwItemData = (DWORD)SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_GETITEMDATA,(WPARAM)iCurIndex,0);
					if (iCurIndex == CB_ERR) break;

					// Get the relavant information out of the AE structures
					//

					pAE = (PACCESSENTRY)dwItemData;
					((CPhoneBook far *)m_dwPhoneBook)->GetCanonical(pAE,m_szPhoneNumber);
					lstrcpy(m_szDunFile,pAE->szDataCenter);
				} else {

					// Parse the text that the user entered
					//

					if (GetDlgItemText(m_hwndDlg,IDC_CMBNUMBER,m_szPhoneNumber,RAS_MaxPhoneNumber))
					{
						m_szPhoneNumber[RAS_MaxPhoneNumber] = '\0';
						for (p = m_szPhoneNumber;*p && *p != ':';p++);
						if (*p)
						{
							*p = '\0';
							p++;
							lstrcpy(m_szDunFile,p);
						} else {
							m_szDunFile[0] = '\0';
						}

						// Check that the phone number on contains valid characters
						//

						for (p = m_szPhoneNumber;*p;p++)
						{
							for(p2 = szValidPhoneCharacters;*p2;p2++)
							{
								if (*p == *p2)
									break; // p2 for loop
							}
							if (!*p2) break; // p for loop
						}

						if (*p)
						{
							MessageBox(m_hwndDlg,GetSz(IDS_INVALIDPHONE),GetSz(IDS_TITLE),MB_MYERROR);
							//MsgBox(IDS_INVALIDPHONE,MB_MYERROR);
							break; // switch statement
						}
					} else {
						AssertSz(0,"You should never be able to hit NEXT with nothing in the phone number.\n");
					}
				}
				EndDialog(m_hwndDlg,IDC_CMDNEXT);
				break;
			case IDC_CMDCANCEL:
				if ((m_dwFlags & (AUTODIAL_IN_PROGRESS|DIALERR_IN_PROGRESS)) == 0) 
				{
					if (MessageBox(hwndDlg,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
						MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
						EndDialog(hwndDlg,IDC_CMDCANCEL);
				} else {
					EndDialog(hwndDlg,IDC_CMDCANCEL);
				}
			break;
				EndDialog(m_hwndDlg,IDC_CMDCANCEL);
				break;
			case IDC_CMDBACK:
				EndDialog(m_hwndDlg,IDC_CMDBACK);
				break;
		}
	default:
		lRet = FALSE;
		break;
	}// switch

	return lRet;
}

// ############################################################################
/***** 1/9/96  jmazner  Normandy #13185
CAccessNumDlg::CAccessNumDlg()
{
	m_szDunPrimary[0] = '\0';
	m_szDunSecondary[0] = '\0';
	m_szPrimary[0] = '\0';
	m_szSecondary[0] = '\0';
	m_rgAccessEntry = NULL;
	m_wNumber = 0;
	m_dwPhoneBook=0;
}
*********/

// ############################################################################
/********* 1/9/96 jmazner  Normandy #13185
                           This was dead code, unused anywhere in icwphbk
LRESULT CAccessNumDlg::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
										LRESULT lResult)
{
	LRESULT lRet = TRUE;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd,IDC_TXTPRIMARY,EM_SETLIMITTEXT,RAS_MaxPhoneNumber,0);
		SendDlgItemMessage(hwnd,IDC_TXTSECONDARY,EM_SETLIMITTEXT,RAS_MaxPhoneNumber,0);

/ *
		// turn AccessEntries into phone numbers
		if(m_szPrimary[0] == '\0')
		{
			LPIDLOOKUPELEMENT pIDLookUp;
			CPhoneBook *pcPBTemp;
			pcPBTemp = ((CPhoneBook far *)m_dwPhoneBook);

			AssertSz(m_dwPhoneBook,"No phonebook set");

			//For the primary phone number
			pIDLookUp = (LPIDLOOKUPELEMENT)bsearch(&m_rgAccessEntry[0]->dwCountryID,
				pcPBTemp->m_rgIDLookUp,pcPBTemp->m_pLineCountryList->dwNumCountries,
				sizeof(IDLOOKUPELEMENT),CompareIdxLookUpElements);
			SzCanonicalFromAE (m_szPrimary, m_rgAccessEntry[0], pIDLookUp->pLCE);

			if (m_rgAccessEntry[1])
			{
				if (m_rgAccessEntry[0]->dwCountryID != m_rgAccessEntry[1]->dwCountryID)
				{
					pIDLookUp = (LPIDLOOKUPELEMENT)bsearch(&m_rgAccessEntry[0]->dwCountryID,
						pcPBTemp->m_rgIDLookUp,pcPBTemp->m_pLineCountryList->dwNumCountries,
						sizeof(IDLOOKUPELEMENT),CompareIdxLookUpElements);
				}
				SzCanonicalFromAE (m_szSecondary, m_rgAccessEntry[1], pIDLookUp->pLCE);
			}
		}
* /
		SendDlgItemMessage(hwnd,IDC_TXTPRIMARY,WM_SETTEXT,0,(LPARAM)&m_szPrimary[0]);
		SendDlgItemMessage(hwnd,IDC_TXTSECONDARY,WM_SETTEXT,0,(LPARAM)&m_szSecondary[0]);
		break;

		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CMDOK:
			// Check that we have at least one phone number
			// Leave dialog
			GetDlgItemText(hwnd,IDC_TXTPRIMARY,&m_szPrimary[0],RAS_MaxPhoneNumber);
			GetDlgItemText(hwnd,IDC_TXTSECONDARY,&m_szSecondary[0],RAS_MaxPhoneNumber);

			if (m_szPrimary[0])
				m_wNumber=1;
			else
				m_wNumber=0;

			if (m_szSecondary[0])
				m_wNumber++;

			EndDialog(hwnd,IDC_CMDOK);
			break;
		case IDC_CMDEXIT:
			// Verify with user
			// Get the hell out of Dodge
			if (MessageBox(hwnd,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
				MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
				EndDialog(hwnd,IDC_CMDEXIT);
			break;
		case IDC_CMDCHANGEPRIMARY:
			// hide dialog
			ShowWindow(hwnd,SW_HIDE);
			// show new dialog
			CSelectNumDlg far *pcSelectNumDlg;
			pcSelectNumDlg = new CSelectNumDlg;
			if (!pcSelectNumDlg)
			{
				MessageBox(hwnd,GetSz(IDS_NOTENOUGHMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
//				MsgBox(IDS_NOTENOUGHMEMORY,MB_MYERROR);
				ShowWindow(hwnd,SW_SHOW);
				break;
			}
			int irc;
			pcSelectNumDlg->m_dwPhoneBook = m_dwPhoneBook;
			pcSelectNumDlg->m_dwCountryID = m_dwCountryID;
			pcSelectNumDlg->m_wRegion = m_wRegion;
			irc = DialogBoxParam(g_hInstDll,MAKEINTRESOURCE(IDD_SELECTNUMBER),
									g_hWndMain,(DLGPROC)PhbkGenericDlgProc,
									(LPARAM)pcSelectNumDlg);
			ShowWindow(hwnd,SW_SHOW);
			switch (irc)
			{
				case -1:
					MessageBox(hwnd,GetSz(IDS_NOTENOUGHMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
//					MsgBox(IDS_NOTENOUGHMEMORY,MB_MYERROR);
					goto DlgProcExit;
					// break;
				case IDC_CMDOK:
					m_dwCountryID = pcSelectNumDlg->m_dwCountryID;
					m_wRegion = pcSelectNumDlg->m_wRegion;
					lstrcpy(m_szDunPrimary,pcSelectNumDlg->m_szDunFile);
					SetDlgItemText(hwnd,IDC_TXTPRIMARY,pcSelectNumDlg->m_szPhoneNumber);
					break;
			}
			break;
		}
		break;
	default:
		lRet = FALSE;
		break;
	}
DlgProcExit:
	return lRet;
}

*********************/

#ifdef WIN16
// ############################################################################
// NAME: SetNonBoldDlg
//
//	Set all the child controls in a window to a non-bold version of the
//	current control font.
//
// Parameters: HWND hDlg	Handle to the dialog window
//
// Created 8/12/96	ValdonB (creatively borrowed from IE)
// ############################################################################

void
SetNonBoldDlg(HWND hDlg)
{
    HFONT hfontDlg = (HFONT) NULL;
    LOGFONT lFont;
    HWND hCtl;
    if ((hfontDlg = (HFONT) SendMessage(hDlg, WM_GETFONT, 0, 0L)))
    {
        if (GetObject(hfontDlg, sizeof(LOGFONT), (LPSTR) &lFont))
        {
            lFont.lfWeight = FW_NORMAL;
            if (hfontDlg = CreateFontIndirect((LPLOGFONT) &lFont))
            {
                // apply the font to all the child controls
                for (hCtl = GetWindow(hDlg, GW_CHILD);
                        NULL != hCtl;
                        hCtl = GetWindow(hCtl, GW_HWNDNEXT))
                {
                    SendMessage(hCtl, WM_SETFONT, (WPARAM) hfontDlg, 0);
                }
            }
        }
    }
}


// ############################################################################
// NAME: SetNonBoldDlg
//
// 	The dialog was modified earlier by unbolding the dialog font, and
// 	getting each control in the dialog to use that font. This function
// 	is called when the dialog is being destroyed so the font can be
// 	deleted
//
// Parameters: HWND hDlg	Handle to the dialog window
//
// Created 8/12/96	ValdonB (creatively borrowed from IE)
// ############################################################################

void
DeleteDlgFont
(
    HWND hDlg
)
{
	HFONT hfont = NULL;

	hfont = (HFONT)SendMessage(hDlg,WM_GETFONT,0,0);
	if (hfont) DeleteObject(hfont);
}
#endif	// WIN16



// ############################################################################
#ifdef WIN16
extern "C" BOOL CALLBACK __export PhbkGenericDlgProc(
#else
extern "C" __declspec(dllexport) BOOL CALLBACK PhbkGenericDlgProc(
#endif
    HWND  hwndDlg,	// handle to dialog box
    UINT  uMsg,	// message
    WPARAM  wParam,	// first message parameter
    LPARAM  lParam 	// second message parameter
   )
{
#if defined(WIN16)
	RECT	MyRect;
	RECT	DTRect;
#endif
//#ifdef DEBUG
//	DebugBreak();
//#endif
	CDialog far *pcDlg = NULL;
	LRESULT lRet;
	switch (uMsg)
	{
	case WM_DESTROY:
		ReleaseBold(GetDlgItem(hwndDlg,IDC_LBLTITLE));
		break;
	case WM_INITDIALOG:
		pcDlg = (CDialog far *)lParam;
		SetWindowLongPtr(hwndDlg,DWLP_USER,lParam);
		lRet = TRUE;
#if defined(WIN16)
		//
		// Move the window to the center of the screen
		//
		GetWindowRect(hwndDlg, &MyRect);
		if (0 == MyRect.left && 0 == MyRect.top)
		{
			GetWindowRect(GetDesktopWindow(), &DTRect);
			MoveWindow(hwndDlg, (DTRect.right - MyRect.right) / 2, (DTRect.bottom - MyRect.bottom) /2,
								MyRect.right, MyRect.bottom, FALSE);
		}
		SetNonBoldDlg(hwndDlg);
#endif
		MakeBold(GetDlgItem(hwndDlg,IDC_LBLTITLE));
		break;
#if defined(WIN16)
	case WM_SYSCOLORCHANGE:
		Ctl3dColorChange();
		break;
#endif
	// 1/9/96  jmazner Normandy #13185, moved to CSelectNumDlg::DlgProc
	//case WM_CLOSE:
	//	if (MessageBox(hwndDlg,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
	//		MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
	//		EndDialog(hwndDlg,IDC_CMDCANCEL);
	//	lRet = TRUE;
	//	break;
//		//PostQuitMessage(0);
//		EndDialog(hwndDlg,FALSE);
//		lRet = TRUE;
//		break;
	default:
		// let the system process the message
		lRet = FALSE;
	}

	if (!pcDlg) pcDlg = (CDialog far*)GetWindowLongPtr(hwndDlg,DWLP_USER);
	if (pcDlg)
		lRet = pcDlg->DlgProc(hwndDlg,uMsg,wParam,lParam,lRet);

	return (BOOL)lRet;
}

// ############################################################################
HRESULT CSelectNumDlg::FillRegion()
{
	HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
	int iCurIndex;
	int iLastIndex;
	unsigned int idx;
	CPhoneBook far *pcPBTemp;
	pcPBTemp = ((CPhoneBook far *)m_dwPhoneBook);
	int iDebugIdx;
	
	// Fill in region list
	//

	SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_RESETCONTENT,0,0);
	SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_ADDSTRING,0,(LPARAM)GetSz(IDS_NATIONWIDE));
	iCurIndex = -1;		// 0xFFFFFFFF;
	m_fHasRegions = FALSE;
	for (idx=0;idx<pcPBTemp->m_cStates;idx++)
	{
		if (pcPBTemp->m_rgState[idx].dwCountryID == m_dwCountryID)
		{
			PACCESSENTRY pAE = NULL, pAELast = NULL;
			pAE = pcPBTemp->m_rgState[idx].paeFirst;
			Assert(pAE);
			pAELast = &(pcPBTemp->m_rgPhoneBookEntry[pcPBTemp->m_cPhoneBookEntries - 1]);
			while (pAELast > pAE && 
				pAE->dwCountryID == m_dwCountryID &&
				pAE->wStateID == idx+1)
			{
				if ((pAE->fType & m_bMask) == m_fType)
					goto AddRegion;
				pAE++;
			}
			continue;

AddRegion:
			m_fHasRegions = TRUE;

			iLastIndex = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_ADDSTRING,0,(LPARAM)&pcPBTemp->m_rgState[idx].szStateName[0]);
			iDebugIdx = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_SETITEMDATA,(WPARAM)iLastIndex,(LPARAM)idx+1);
			if ((idx+1) == m_wRegion)
			{
				iCurIndex = iLastIndex;
			}
		}
	}

	// select current region or nation wide
	//

	if (iCurIndex != -1)	// 0xFFFFFFFF
	{
		SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_SETCURSEL,(WPARAM)iCurIndex,0);
	} else {
		m_wRegion = 0;	// Nationwide
		SendDlgItemMessage(m_hwndDlg,IDC_CMBREGION,CB_SETCURSEL,0,0);
	}

	EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMBREGION),m_fHasRegions);
	PostMessage(m_hwndDlg,WM_SHOWSTATEMSG,m_fHasRegions,0);

	hr = ERROR_SUCCESS;
	return hr;
}

// ############################################################################
CSelectNumDlg::CSelectNumDlg()
{
	m_dwCountryID = 0;
	m_wRegion = 0;
	m_dwPhoneBook = 0;
	m_szPhoneNumber[0] = '\0';
	m_szDunFile[0] = '\0';
	m_fType = 0;
	m_bMask = 0;
	m_fHasRegions = FALSE;
	m_hwndDlg = NULL;
	m_dwFlags = 0;
}

// ############################################################################
HRESULT CSelectNumDlg::FillNumber()
{
	HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
	PACCESSENTRY pAELast, pAE = NULL;
	CPhoneBook far *pcPBTemp;
	unsigned int idx;
	pcPBTemp = ((CPhoneBook far *)m_dwPhoneBook);
	
	// Check if we need to look up the number from the region or from the country
	//

	if (m_fHasRegions && m_wRegion)
		pAE = pcPBTemp->m_rgState[m_wRegion-1].paeFirst;
	

	// Find the Access Entries for the country
	//

	if (!pAE)
	{
		LPIDLOOKUPELEMENT pIDLookUp, pLookUpTarget;

		pLookUpTarget = (LPIDLOOKUPELEMENT)GlobalAlloc(GPTR,sizeof(IDLOOKUPELEMENT));
		Assert(pLookUpTarget);
		if (!pLookUpTarget) goto FillNumberExit;
		pLookUpTarget->dwID = m_dwCountryID;

		pIDLookUp = NULL;
		pIDLookUp = (LPIDLOOKUPELEMENT)bsearch(pLookUpTarget,pcPBTemp->m_rgIDLookUp,
			(int)pcPBTemp->m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIDLookUpElements);
		if (pIDLookUp)
			pAE = pIDLookUp->pFirstAE;
	}

	// Fill the list for whatever AE's we found
	//
	
	SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_RESETCONTENT,0,0);
	if (pAE)
	{
		//pAELast = pcPBTemp->m_rgPhoneBookEntry + pcPBTemp->m_cPhoneBookEntries;
		pAELast = &(pcPBTemp->m_rgPhoneBookEntry[pcPBTemp->m_cPhoneBookEntries - 1]);
		while (pAELast > pAE && pAE->dwCountryID == m_dwCountryID && pAE->wStateID == m_wRegion)
		{
			if ((pAE->fType & m_bMask) == m_fType)
			{
				wsprintf(szTemp,"%s (%s) %s",pAE->szCity,pAE->szAreaCode,
							pAE->szAccessNumber);
				idx = (int)SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,
												CB_ADDSTRING,0,
												(LPARAM)((LPSTR) &szTemp[0]));
				if (idx == -1) goto FillNumberExit;
				SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_SETITEMDATA,
									(WPARAM)idx,(LPARAM)pAE);
			}
			pAE++;
		}

		// Select the first item
		//

		if (SendDlgItemMessage(m_hwndDlg,IDC_CMBNUMBER,CB_SETCURSEL,0,0) == CB_ERR)
				EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDNEXT),FALSE);
		else
				EnableWindow(GetDlgItem(m_hwndDlg,IDC_CMDNEXT),TRUE);
		hr = ERROR_SUCCESS;
	}
FillNumberExit:
	return hr;
}
