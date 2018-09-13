/*	-	-	-	-	-	-	-	-	*/
//
//	sound.h
//
//	Copyright (C) 1994 Microsoft Corporation.  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

DECLARE_HANDLE(HSOUND);
typedef HSOUND * PHSOUND;

/*	-	-	-	-	-	-	-	-	*/
void FAR PASCAL soundOnDone(
	HSOUND	hs);
MMRESULT FAR PASCAL soundOpen(
	LPCTSTR	pszSound,
	HWND	hwndNotify,
	PHSOUND	phs);
MMRESULT FAR PASCAL soundClose(
	HSOUND	hs);
MMRESULT FAR PASCAL soundPlay(
	HSOUND	hs);
MMRESULT FAR PASCAL soundStop(
	HSOUND	hs);

/*	-	-	-	-	-	-	-	-	*/
