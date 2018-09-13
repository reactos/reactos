/*--------------------------------------------------------------------
|
| LangPlay.c - Sample Win app to play AVI movies using MCIWnd. Handles
|	multiple language track movies and lets the
|	user select the track to listen to at playback.
|
|
+--------------------------------------------------------------------*/
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992, 1993  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <mmsystem.h>		
#include <digitalv.h>
#include "win32.h"
#include <vfw.h>		
#include "langplay.h"





/**************************************************************
************************ GLOBALS ******************************
**************************************************************/
/* AVI stuff to keep around */
HWND hwndMovie;			/* window handle of the movie */
BOOL fMovieOpen = FALSE;	/* Open flag: TRUE == movie open, FALSE = none */
HMENU hMenuBar = NULL;		/* menu bar handle */
char szAppName [] = "LangPlay";

// struct for handling multi-language support
typedef struct langs_tag {
	WORD		wLangTag;	// language type tag
	char		achName[64];	// stream name  (limited to 64 chars by AVIStreamInfo)
} LANGS, FAR *LPLANGS;

#define 	NOAUDIO		0	// no audio stream
int		iCurLang;		// current language selected (0 == NONE)


/* function declarations */
long FAR PASCAL _export WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void fileOpenMovie(HWND hWnd);
void menubarUpdate(HWND hWnd);
void titlebarUpdate(HWND hWnd, LPSTR lpstrMovie);

/* language specific functions */
BOOL enumLangs(HWND hwnd, LPSTR lpstrMovie);
void buildLangMenu(HWND hwnd, LPLANGS lpLangs, DWORD dwLangs);
void switchLang(HWND hWnd, int iLangStream);



/********************************************************************
************************** FUNCTIONS ********************************
********************************************************************/


/*--------------------------------------------------------------+
| initApp - initialize the app overall.				|
|								|
| Returns the Window handle for the app on success, NULL if	|
| there is a failure.						|
|								|
+--------------------------------------------------------------*/
HWND initApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nCmdShow)
{
	HWND		hWnd;	/* window handle to return */
	int		iWinHeight;
	WORD	wVer;

	/* first let's make sure we are running on 1.1 */
	wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a){
		/* oops, we are too old, blow out of here */
		MessageBeep(MB_ICONHAND);
		MessageBox(NULL, "Video for Windows version is too old",
			  "LangPlay Error", MB_OK|MB_ICONSTOP);
		return FALSE;
	}

	if (!hPrevInstance){
		WNDCLASS    wndclass;

		wndclass.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = WndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon (hInstance, "AppIcon");
		wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wndclass.lpszMenuName  = szAppName;
		wndclass.lpszClassName = szAppName;

		if (!RegisterClass(&wndclass)){
			MessageBox(NULL, "RegisterClass failure", szAppName, MB_OK);
			return NULL;
		}
	}

	iWinHeight = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
   			(GetSystemMetrics(SM_CYFRAME) * 2);
   			
	/* create the main window for the app */
	hWnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW |
		WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 180, iWinHeight,
		NULL, NULL, hInstance, NULL);

	if (hWnd == NULL){
		MessageBox(NULL, "CreateWindow failure", szAppName, MB_OK);
		return NULL;
	}

	hMenuBar = GetMenu(hWnd);	/* get the menu bar handle */
	menubarUpdate(hWnd);		/* update menu bar to disable Movie menu */

	/* Show the main window */
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	/* create the movie window using MCIWnd that has no file open initially */
	hwndMovie = MCIWndCreate(hWnd, hInstance, WS_CHILD |WS_VISIBLE | MCIWNDF_NOOPEN |
   				MCIWNDF_NOERRORDLG | MCIWNDF_NOTIFYSIZE, NULL);

	if (!hwndMovie){
		/* we didn't get the movie window, destroy the app's window and bail out */
		DestroyWindow(hWnd);
		return NULL;
	}
	return hWnd;
}




/*--------------------------------------------------------------+
| WinMain - main routine.					|
|								|
+--------------------------------------------------------------*/
int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPSTR lpszCmdParam, int nCmdShow)
{
	HWND        hWnd;
	MSG         msg;

	if ((hWnd = initApp(hInstance, hPrevInstance,nCmdShow)) == NULL)
		return 0;	/* died initializing, bail out */

	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}


/*--------------------------------------------------------------+
| WndProc - window proc for the app				|
|								|
+--------------------------------------------------------------*/
long FAR PASCAL _export WndProc (HWND hWnd, UINT message, WPARAM wParam,
						LPARAM lParam)
{
	PAINTSTRUCT ps;
	WORD w;
	WORD	wMenu;
	RECT	rc;

	switch (message){
		case WM_CREATE:
			return 0;

		case WM_INITMENUPOPUP:
			/* be sure this isn't the system menu */
			if (HIWORD(lParam))
				return DefWindowProc(hWnd, WM_INITMENUPOPUP,
						wParam, lParam);

			wMenu = LOWORD(lParam);
			switch (wMenu){
				case 0:   /* file menu */
					/* turn on/off CLOSE & PLAY */
					if (fMovieOpen) w = MF_ENABLED|MF_BYCOMMAND;
					else		w = MF_GRAYED|MF_BYCOMMAND;
					EnableMenuItem((HMENU)wParam, IDM_CLOSE, w);
					break;
			} /* switch */
			break;
		
		case WM_COMMAND:
			if (wParam >= IDM_STREAM){
				// the command is to switch the audio stream
				switchLang(hWnd, wParam - IDM_STREAM + 1);	
				return 0;
			}
			/* handle the menu commands */
			switch (wParam) {
				/* File Menu */
				case IDM_OPEN:
					fileOpenMovie(hWnd);
					break;
				case IDM_CLOSE:
					fMovieOpen = FALSE;
					MCIWndClose(hwndMovie); 	// close the movie
					ShowWindow(hwndMovie, SW_HIDE);	//hide the window
					menubarUpdate(hWnd);
					titlebarUpdate(hWnd, NULL);	// title bar back to plain
					break;
				case IDM_EXIT:
					PostMessage(hWnd, WM_CLOSE, 0, 0L);
					break;
				
				/* audio menu */
				case IDM_NONE:
					switchLang(hWnd, NOAUDIO);
					break;
						
			}
			return 0;
		
    		case WM_PAINT:
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return 0;
		
		case WM_SIZE:
			if (hwndMovie && fMovieOpen)
				MoveWindow(hwndMovie,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
        		break;

		case WM_DESTROY:
			if (fMovieOpen)
				MCIWndClose(hwndMovie);  // close an open movie
			MCIWndDestroy(hwndMovie);    // now destroy the MCIWnd window
			PostQuitMessage(0);
			return 0;
		
    		case MCIWNDM_NOTIFYSIZE:
    			if (fMovieOpen){
	    			/* adjust to size of the movie window */
				GetWindowRect(hwndMovie, &rc);
				AdjustWindowRect(&rc, GetWindowLong(hWnd, GWL_STYLE), TRUE);
#ifndef WIN32
				rc.bottom++;	// AdjustWindowRect is broken
#endif
				SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left,
                    			rc.bottom - rc.top,
                    			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                	} else {
	                	/* movie closed, adjust to the default size */
	                	int iWinHeight;
	                	
				iWinHeight = GetSystemMetrics(SM_CYCAPTION) +
						GetSystemMetrics(SM_CYMENU) +
	   					(GetSystemMetrics(SM_CYFRAME) * 2);
	   			SetWindowPos(hWnd, NULL, 0, 0, 180, iWinHeight,
	   				SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
	                }
	   		break;
  	
		case WM_ACTIVATE:
		case WM_QUERYNEWPALETTE:
		case WM_PALETTECHANGED:
			//
			// Forward palette-related messages through to the MCIWnd,
			// so it can do the right thing.
			//
			if (hwndMovie)
				return SendMessage(hwndMovie, message, wParam, lParam);
			break;
	} /* switch */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*--------------------------------------------------------------+
| menubarUpdate - update the menu bar based on the <fMovieOpen> |
|		  flag value.  This will turn on/off the	|
|		  Movie menu.					|
|								|
+--------------------------------------------------------------*/
void menubarUpdate(HWND hWnd)
{
	WORD w;

	if (fMovieOpen){
		w = MF_ENABLED|MF_BYPOSITION;
	} else {
		w = MF_GRAYED|MF_BYPOSITION;
	}
	EnableMenuItem(hMenuBar, 1, w);	/* change the Movie menu (#1) */
	DrawMenuBar(hWnd);	/* re-draw the menu bar */
}

/*--------------------------------------------------------------+
| titlebarUpdate - update the title bar to include the name	|
|		   of the movie playing.			|
|								|
+--------------------------------------------------------------*/
void titlebarUpdate(HWND hWnd, LPSTR lpstrMovie)
{
	char achNewTitle[BUFFER_LENGTH];	// space for the title

	if (lpstrMovie != NULL)
		wsprintf((LPSTR)achNewTitle,"%s - %s", (LPSTR)szAppName,lpstrMovie);
	else
		lstrcpy((LPSTR)achNewTitle, (LPSTR)szAppName);
	SetWindowText(hWnd, (LPSTR)achNewTitle);
}

/*--------------------------------------------------------------+
| fileOpenMovie - open an AVI movie. Use CommDlg open box to	|
|	        open and then handle the initialization to	|
|		show the movie and position it properly.  Keep	|
|		the movie paused when opened.			|
|								|
|		Sets <fMovieOpened> on success.			|
+--------------------------------------------------------------*/
void fileOpenMovie(HWND hWnd)
{
	OPENFILENAME ofn;

	static char szFile [BUFFER_LENGTH];
	static char szFileTitle [BUFFER_LENGTH];
	
	/* use the OpenFile dialog to get the filename */
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Video for Windows\0*.avi\0\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	
	/* use MCIWnd to get our filename */
	if (GetOpenFileNamePreview(&ofn)){
	
		/* we got a filename, now close any old movie and open */
		/* the new one.					*/
		if (fMovieOpen)
			MCIWndClose(hwndMovie);	
		
		enumLangs(hWnd, ofn.lpstrFile);	// find out the languages used in the file.
	
		/* try to open the file */
		fMovieOpen = TRUE;		// assume the best
		if (MCIWndOpen(hwndMovie, ofn.lpstrFile, 0) == 0){
			/* we opened the file o.k., now set up to */
		 	/* play it.				   */
		 	ShowWindow(hwndMovie, SW_SHOW);
	 	} else {
			/* generic error for open */
			MessageBox(hWnd, "Unable to open Movie", NULL,
			      MB_ICONEXCLAMATION|MB_OK);
			fMovieOpen = FALSE;
	 	}
   	}
	/* update menu bar */
	menubarUpdate(hWnd);
	if (fMovieOpen)
		titlebarUpdate(hWnd, (LPSTR)ofn.lpstrFileTitle);
	else
		titlebarUpdate(hWnd, NULL);

	/* cause an update to occur */
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
}



/*--------------------------------------------------------------+
| enumLangs - enumerate the audio streams in a file and set up 	|
|	      the global ghLangs as a handle to the array of	|
|	      language streams.					|								|
|								|
| To do this:							|
|	1. Open the file using AVIFileOpen().			|
|	2. Open all Audio Streams, getting the pavi for it	|
|	3. Get the stream info on all audio streams to get names|
|	4. If names don't exist use "Audio Strean n"		|
|	5. Close the file					|
| 	6. Build the audio stream menu 				|
|								|
| Return TRUE if successfully set up menu, FALSE on any error	|
|								|
+--------------------------------------------------------------*/
BOOL enumLangs(HWND hWnd, LPSTR lpstrMovie)
{
	PAVIFILE	pFile;
	PAVISTREAM 	pStream;
	AVIFILEINFO aviInfo;
	AVISTREAMINFO aviStream;
	DWORD		dwNumStreams;
	DWORD		dwNumAudioStreams = 0L;
	LPLANGS		lpLangs;
	LPLANGS		lpLang;
	HANDLE		hLangs;
	
	DWORD		dw;
	
	
	AVIFileInit();
	
	// go open the file
	if (AVIFileOpen((PAVIFILE far *)&pFile, lpstrMovie, OF_READ, NULL) != 0)
		return FALSE;
	
	// get the file information	
	AVIFileInfo(pFile, (LPAVIFILEINFO)&aviInfo, sizeof(aviInfo));
	dwNumStreams = aviInfo.dwStreams;		// grab the number of streams
	
	// loop through the streams and find the # of audio streams
	for (dw = 0L; dw < dwNumStreams; dw++){
		AVIFileGetStream(pFile, (PAVISTREAM far *)&pStream, 0, dw);
		AVIStreamInfo(pStream, (LPAVISTREAMINFO)&aviStream, sizeof(aviStream));
		if (aviStream.fccType == streamtypeAUDIO)
			dwNumAudioStreams++;
		AVIStreamClose(pStream);
		
	}
	
	// we now know how many audio streams we are dealing with, we need to allocate
	// enough memory for the Langs array and then get all the audio stream information
	// again.
	hLangs = GlobalAlloc(GHND, (sizeof(LANGS) *  dwNumAudioStreams));
	if (hLangs == NULL)
		return FALSE;
	
	lpLangs = GlobalLock(hLangs);  // get the memory
	
	// loop through the audio streams and fill out the array
	for (dw = 0L, lpLang = lpLangs; dw < dwNumAudioStreams; dw++, lpLang++) {
		AVIFileGetStream(pFile, (PAVISTREAM far *)&pStream, streamtypeAUDIO, dw);
		AVIStreamInfo(pStream, (LPAVISTREAMINFO)&aviStream, sizeof(aviStream));
		if (aviStream.szName && *aviStream.szName != '\0')
			lstrcpy((LPSTR)lpLang->achName, (LPSTR)aviStream.szName);
		else
			wsprintf((LPSTR)lpLang->achName, "Audio Stream %lu",dw);
		AVIStreamClose(pStream);	
		
	}
	
	// now build the menu for this
	buildLangMenu(hWnd, lpLangs, dwNumAudioStreams);
	
	// close up and deallocate resources
	AVIFileClose(pFile);
	AVIFileExit();
	
	GlobalUnlock(hLangs);
	GlobalFree(hLangs);
	
	return TRUE;
}


/*--------------------------------------------------------------+
| buildLangMenu	- build up the language menu for the audio	|
| 				  streams available.		|
|								|
| hwnd is the main application window handle			|
| lang points to an array of LANGSTRUCT entries already filled	|
|      in by the caller.					|
+--------------------------------------------------------------*/
void buildLangMenu(HWND hwnd, LPLANGS lpLangs, DWORD dwLangs)
{
	UINT 	i;
	HMENU  	hMenu;
	LPLANGS	lplang;
	UINT	uNumMenus;
	
	// go through menu chain and get the Audio Stream pop-up menu
	hMenu = GetMenu(hwnd);			// get the menu bar
	hMenu = GetSubMenu(hMenu, 1);	// get the Audio Stream menu
	
	uNumMenus = GetMenuItemCount(hMenu);	// how many items are on this menu?
	if (uNumMenus > 1){
		// we've got a menu with items already, time to delete all of them
		// except for the first one (NONE).  NOTE: Item 0 == first item
		// be sure to delete in reverse order so you get them all.
		for ( --uNumMenus; uNumMenus; uNumMenus--) {
			DeleteMenu(hMenu, uNumMenus, MF_BYPOSITION);
		}
	}
	
	// loop through the languages and add menus to the existing menu
	for (i=0, lplang = lpLangs; i<dwLangs; i++, lplang++){
		AppendMenu(hMenu, MF_ENABLED | MF_STRING, IDM_STREAM+i, lplang->achName);
	}
	
	// get default set up
	if (dwLangs)
		iCurLang = 1;		// use first audio stream
	else
		iCurLang = NOAUDIO;	// else none
	
	/* set up the checkmark initially */		
	CheckMenuItem(hMenu, (iCurLang), MF_BYPOSITION | MF_CHECKED);	
}


/*------------------------------------------------------------------+
| switchLang - switch audio stream playback							|
|																	|
| iLangStream == the audio stream to switch to (-1 == NONE)			|
| Be sure to update iCurrLang global to be the current audio stream	|
| selected.															|
|																	|
+------------------------------------------------------------------*/
void switchLang(HWND hWnd, int iLangStream)
{
	HMENU 					hMenu;
	char					achStrBuff[256];


	// if user just picked the same stream then just get out of here
	if (iCurLang == iLangStream)
		return;
    	
  	// go through menu chain and get the Audio Stream pop-up menu
	hMenu = GetMenu(hWnd);		// get the menu bar
	hMenu = GetSubMenu(hMenu, 1);	// get the Audio Stream menu

	// turn off the checkmark from the old item
	CheckMenuItem(hMenu, (iCurLang), MF_BYPOSITION | MF_UNCHECKED);	
	
	// turn on the checkmark on the new item
	CheckMenuItem(hMenu, (iLangStream), MF_BYPOSITION | MF_CHECKED);
		
	if (iLangStream == NOAUDIO){
		// turn off all audio
		MCIWndSendString(hwndMovie, "setaudio off");
	} else {
		// turn on audio & the specific stream
		wsprintf(achStrBuff, "setaudio stream to %d", iLangStream);

		// send the command
		MCIWndSendString(hwndMovie, achStrBuff);


		if (iCurLang == NOAUDIO){
			// audio was off, turn it on
			MCIWndSendString(hwndMovie, "setaudio on");
		}
	}
	
	iCurLang = iLangStream;		// set the current stream
}		


/*--------------------------- end of file ----------------------*/
