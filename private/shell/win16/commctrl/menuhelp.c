#include "ctlspriv.h"

#define MAININSYS

#ifndef	WIN32
/* This returns the index of a submenu in a parent menu.  The return is
 * < 0 if the submenu does not exist in the parent menu
 */
static int NEAR PASCAL GetMenuIndex(HMENU hMenu, HMENU hSubMenu)
{
  int i;

  if (!hMenu || !hSubMenu)
      return(-1);

  for (i=GetMenuItemCount(hMenu)-1; i>=0; --i)
    {
      if (hSubMenu == GetSubMenu(hMenu, i))
	  break;
    }

  return(i);
}
#endif	// WIN32


BOOL NEAR PASCAL IsMaxedMDI(HMENU hMenu)
{
  return(GetMenuItemID(hMenu, GetMenuItemCount(hMenu)-1) == SC_RESTORE);
}


/* Note that if iMessage is WM_COMMAND, it is assumed to have come from
 * a header bar or toolbar; do not pass in WM_COMMAND messages from any
 * other controls.
 */

#define MS_ID		GET_WM_MENUSELECT_CMD
#define MS_FLAGS	GET_WM_MENUSELECT_FLAGS
#define MS_MENU		GET_WM_MENUSELECT_HMENU

#define CMD_NOTIFY	GET_WM_COMMAND_CMD
#define CMD_ID		GET_WM_COMMAND_ID
#define CMD_CTRL	GET_WM_COMMAND_HWND

void WINAPI MenuHelp(UINT iMessage, WPARAM wParam, LPARAM lParam,
      HMENU hMainMenu, HINSTANCE hAppInst, HWND hwndStatus, UINT FAR *lpwIDs)
{
  UINT wID;
  UINT FAR *lpwPopups;
  int i;
  char szString[256];
  BOOL bUpdateNow = TRUE;
#if defined(WINDOWS_ME)
  MENUITEMINFO mii;
#endif

  switch (iMessage)
    {
      case WM_MENUSELECT:
	if ((WORD)MS_FLAGS(wParam, lParam)==(WORD)-1 && MS_MENU(wParam, lParam)==0)
	  {
#ifndef WIN32
EndMenuHelp:
#endif
	    SendMessage(hwndStatus, SB_SIMPLE, 0, 0L);
	    break;
	  }

	szString[0] = '\0';
#if defined(WINDOWS_ME)
#ifdef WIN32
    i = MS_ID(wParam, lParam);
#else
    i = GetMenuIndex((HMENU)MS_MENU(wParam, lParam), (HMENU)MS_ID(wParam, lParam));
#endif
	GetMenuItemInfo((HMENU)MS_MENU(wParam, lParam), i, TRUE, &mii);
	mii.fState = mii.fType & MFT_RIGHTORDER ?SBT_RTLREADING :0;
#endif
	if (!(MS_FLAGS(wParam, lParam)&MF_SEPARATOR))
	  {
	    if (MS_FLAGS(wParam, lParam)&MF_POPUP)
	      {
		/* We don't want to update immediately in case the menu is
		 * about to pop down, with an item selected.  This gets rid
		 * of some flashing text.
		 */
		bUpdateNow = FALSE;

		/* First check if this popup is in our list of popup menus
		 */
		for (lpwPopups=lpwIDs+2; *lpwPopups; lpwPopups+=2)
		  {
		    /* lpwPopups is a list of string ID/menu handle pairs
		     * and MS_ID(wParam, lParam) is the menu handle of the selected popup
		     */
		    if (*(lpwPopups+1) == (UINT)MS_ID(wParam, lParam))
		      {
			wID = *lpwPopups;
			goto LoadTheString;
		      }
		  }

		/* Check if the specified popup is in the main menu;
		 * note that if the "main" menu is in the system menu,
		 * we will be OK as long as the menu is passed in correctly.
		 * In fact, an app could handle all popups by just passing in
		 * the proper hMainMenu.
		 */
		if ((HMENU)MS_MENU(wParam, lParam) == hMainMenu)
		  {
#ifdef	WIN32
		    i = MS_ID(wParam, lParam);
#else	// WIN32
		    i = GetMenuIndex((HMENU)MS_MENU(wParam, lParam), (HMENU)MS_ID(wParam, lParam));
		    if (i >= 0)
#endif	// WIN32
		      {
			if (IsMaxedMDI(hMainMenu))
			  {
			    if (!i)
			      {
				wID = IDS_SYSMENU;
				hAppInst = HINST_THISDLL;
				goto LoadTheString;
			      }
			    else
				--i;
			  }
			wID = (UINT)(i + lpwIDs[1]);
			goto LoadTheString;
		      }
		  }

		/* This assumes all app defined popups in the system menu
		 * have been listed above
		 */
		if ((MS_FLAGS(wParam, lParam)&MF_SYSMENU))
		  {
		    wID = IDS_SYSMENU;
		    hAppInst = HINST_THISDLL;
		    goto LoadTheString;
		  }

		goto NoString;
	      }
	    else if (MS_ID(wParam, lParam) >= MINSYSCOMMAND)
	      {
		wID = (UINT)(MS_ID(wParam, lParam) + MH_SYSMENU);
		hAppInst = HINST_THISDLL;
	      }
	    else
	      {
		wID = (UINT)(MS_ID(wParam, lParam) + lpwIDs[0]);
	      }

LoadTheString:
	    LoadString(hAppInst, wID, szString, sizeof(szString));
	  }

NoString:
#if defined(WINDOWS_ME)
	SendMessage(hwndStatus, SB_SETTEXT, mii.fState|SBT_NOBORDERS|255,
	      (LPARAM)(LPSTR)szString);
#else
	SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
	      (LPARAM)(LPSTR)szString);
#endif
	SendMessage(hwndStatus, SB_SIMPLE, 1, 0L);

	if (bUpdateNow)
	    UpdateWindow(hwndStatus);
	break;

#ifndef WIN32

      case WM_COMMAND:
	switch (CMD_NOTIFY(wParam, lParam))
	  {
#ifdef WANT_SUCKY_HEADER
      // BUGBUG: these are now WM_NOTIFY messages
	    case HBN_BEGINDRAG:
	      bUpdateNow = FALSE;
	      wID = IDS_HEADER;
	      goto BeginSomething;

	    case HBN_BEGINADJUST:
	      wID = IDS_HEADERADJ;
	      goto BeginSomething;
#endif
	    case TBN_BEGINADJUST:
	      /* We don't want to update immediately in case the operation is
	       * aborted immediately.
	       */
	      bUpdateNow = FALSE;
	      wID = IDS_TOOLBARADJ;
	      goto BeginSomething;

BeginSomething:
	      SendMessage(hwndStatus, SB_SIMPLE, 1, 0L);
	      hAppInst = HINST_THISDLL;
	      goto LoadTheString;

	    case TBN_BEGINDRAG:
	      MenuHelp(WM_MENUSELECT, (WPARAM)CMD_CTRL(wParam, lParam), 0L,
		    hMainMenu, hAppInst, hwndStatus, lpwIDs);
	      break;
#ifdef WANT_SUCKY_HEADER
	    case HBN_ENDDRAG:
            case HBN_ENDADJUST:
#endif
	    case TBN_ENDDRAG:
	    case TBN_ENDADJUST:
	      goto EndMenuHelp;

	    default:
	      break;
	  }
	break;
#endif	// !WIN32

      default:
	break;
    }
}


BOOL WINAPI ShowHideMenuCtl(HWND hWnd, WPARAM wParam, LPINT lpInfo)
{
  HWND hCtl;
  UINT uTool, uShow = MF_UNCHECKED | MF_BYCOMMAND;
  HMENU hMainMenu;
  BOOL bRet = FALSE;

  hMainMenu = (HMENU)lpInfo[1];

  for (uTool=0; ; ++uTool, lpInfo+=2)
    {
      if ((WPARAM)lpInfo[0] == wParam)
	  break;
      if (!lpInfo[0])
	  goto DoTheCheck;
    }

  if (!(GetMenuState(hMainMenu, wParam, MF_BYCOMMAND)&MF_CHECKED))
      uShow = MF_CHECKED | MF_BYCOMMAND;

  switch (uTool)
    {
      case 0:
	bRet = SetMenu(hWnd, (HMENU)((uShow&MF_CHECKED) ? hMainMenu : 0));
	break;

      default:
	hCtl = GetDlgItem(hWnd, lpInfo[1]);
	if (hCtl)
	  {
	    ShowWindow(hCtl, (uShow&MF_CHECKED) ? SW_SHOW : SW_HIDE);
	    bRet = TRUE;
	  }
	else
	    uShow = MF_UNCHECKED | MF_BYCOMMAND;
	break;
    }

DoTheCheck:
  CheckMenuItem(hMainMenu, wParam, uShow);

#ifdef MAININSYS
  hMainMenu = GetSubMenu(GetSystemMenu(hWnd, FALSE), 0);
  if (hMainMenu)
      CheckMenuItem(hMainMenu, wParam, uShow);
#endif

  return(bRet);
}


void WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo)
{
  RECT rc;
  HWND hCtl;

  GetClientRect(hWnd, lprc);

  /* Get past the menu
   */
  for (lpInfo+=2; lpInfo[0]; lpInfo+=2)
    {
      hCtl = GetDlgItem(hWnd, lpInfo[1]);
      /* We check the style bit because the parent window may not be visible
       * yet (still in the create message)
       */
      if (!hCtl || !(GetWindowStyle(hCtl) & WS_VISIBLE))
	  continue;

      GetWindowRect(hCtl, &rc);
      ScreenToClient(hWnd, (LPPOINT)&rc);
      ScreenToClient(hWnd, ((LPPOINT)&rc)+1);

      SubtractRect(lprc, lprc, &rc);
    }
}

#if 0
// BUGBUG: nuke this stuff for WIN32

#define NibbleToChar(x) (N2C[x])
static char N2C[] =
  {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
  } ;

BOOL WINAPI MyWritePrivateProfileStruct(LPCSTR szSection, LPCSTR szKey,
      LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile)
{
  PSTR pLocal, pTemp;
  BOOL bRet;
  BYTE FAR *lpByte;

  /* NULL lpStruct erases the the key */

  if (lpStruct == NULL) {
      if (szFile && *szFile)
          return WritePrivateProfileString(szSection, szKey, NULL, szFile);
      else
          WriteProfileString(szSection, szKey, NULL);
  }

  pLocal = (PSTR)LocalAlloc(LPTR, uSizeStruct*2 + 1);
  if (!pLocal)
      return(FALSE);

  lpByte = lpStruct;

  for (pTemp=pLocal; uSizeStruct>0; --uSizeStruct, ++lpByte)
    {
      BYTE bStruct;

      bStruct = *lpByte;
      *pTemp++ = NibbleToChar((bStruct>>4)&0x000f);
      *pTemp++ = NibbleToChar(bStruct&0x000f);
    }

  *pTemp = '\0';

  if (szFile && *szFile)
      bRet = WritePrivateProfileString(szSection, szKey, pLocal, szFile);
  else
      bRet = WriteProfileString(szSection, szKey, pLocal);

  LocalFree((HLOCAL)pLocal);
  return(bRet);
}

/* Note that the following works for both upper and lower case, and will
 * return valid values for garbage chars
 */
#define CharToNibble(x) ((x)>='0'&&(x)<='9' ? (x)-'0' : ((10+(x)-'A')&0x000f))

BOOL WINAPI MyGetPrivateProfileStruct(LPCSTR szSection, LPCSTR szKey,
      LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile)
{
  PSTR pLocal, pTemp;
  int nLen;
  BYTE FAR *lpByte;

  nLen = uSizeStruct*2 + 10;
  pLocal = (PSTR)LocalAlloc(LPTR, nLen);
  if (!pLocal)
      return(FALSE);

  if (szFile && *szFile)
      nLen = GetPrivateProfileString(szSection, szKey, c_szNULL, pLocal, nLen,
	    szFile);
  else
      nLen = GetProfileString(szSection, szKey, c_szNULL, pLocal, nLen);
  if ((UINT)nLen != uSizeStruct*2)
    {
      LocalFree((HLOCAL)pLocal);
      return(FALSE);
    }

  lpByte = lpStruct;

  for (pTemp=pLocal; uSizeStruct>0; --uSizeStruct, ++lpByte)
    {
      BYTE bStruct;
      char cTemp;

      cTemp = *pTemp++;
      bStruct = (BYTE)CharToNibble(cTemp);
      cTemp = *pTemp++;
      bStruct = (BYTE)((bStruct<<4) | CharToNibble(cTemp));

      *lpByte = bStruct;
    }

  LocalFree((HLOCAL)pLocal);
  return(TRUE);
}

#endif

