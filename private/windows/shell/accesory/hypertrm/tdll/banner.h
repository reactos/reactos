/*	File: \wacker\tdll\banner.h (created 16-Mar-94)
 *
 *	Copyright 1994 by Hilgraeve, Inc -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

#define	BANNER_DISPLAY_CLASS	"BannerDisplayClass"
#define WACKER_VERSION			"0.10"
#define BANNER_TIME 			4000

#define BANNER_WINDOW_STYLE  WS_POPUP | WS_VISIBLE

BOOL bannerRegisterClass(HANDLE hInstance);
HWND bannerCreateBanner(HANDLE hInstance, LPTSTR pszTitle);
LPTSTR bnrBuildLotNum(LPTSTR);
VOID FAR PASCAL utilDrawBitmap(HWND hWnd, HDC hDC, HBITMAP hBitmap, SHORT xStart, SHORT yStart);

extern const TCHAR *achVersion;
