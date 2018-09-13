/*  Screen Doors*/

#define STRICT
#include <windows.h>
#include "Init_End.h"
#include "kbmain.h"     //keyboard header file from 3keyboar

#include "resource.h"

/*************************************************************/
//Functions in this file
/*************************************************************/
#include "sdgutil.h"

/************************************************************/
//Functions in other files
/************************************************************/
#include "fileutil.h"
#include "dgsett.h"

BOOL rSetWindowPos(void);
BOOL Read_Map_Setting(LPCSTR filename);
BOOL BringUpBowser(void);
/*************************************************************/
//Global vars
/*************************************************************/
LOGFONT   lf;
extern BOOL  Setting_ReadSuccess=FALSE;
extern DWORD platform;


/**************************************************************************/
/* SendInitErrorMessage  - error msg                                          */
/**************************************************************************/
void SendErrorMessage(UINT ids_string)
{
    TCHAR str[256]=TEXT("");
    TCHAR title[256]=TEXT("");

	LoadString(hInst, ids_string, &str[0], 256);
	LoadString(hInst, IDS_TITLE1, &title[0], 256);
	MessageBox(kbmainhwnd, str, title,MB_ICONHAND | MB_OK);
}// SendErrorMessage

/******************************************************************************/
//initially set all the preferences (except some keyboard preference which 
// set at kbmain.c)
/******************************************************************************/
void PredictInit(void)
{

	 // *** use the setting read from Registry ***
	 if(Setting_ReadSuccess=OpenUserSetting())
	 {
		g_margin			= kbPref->g_margin;			   	// Margin between rows and columns
		smallKb 			= kbPref->smallKb;				// TRUE when working with Small Keyboard
		PrefTextKeyColor 	= kbPref->PrefTextKeyColor;   	// Prefered Color for text in keys
		PrefCharKeyColor 	= kbPref->PrefCharKeyColor; 	// ditto normal key
		PrefModifierKeyColor= kbPref->PrefModifierKeyColor;	// ditto modifier key
		PrefDeadKeyColor 	= kbPref->PrefDeadKeyColor; 	// ditto dead key
		PrefBackgroundColor	= kbPref->PrefBackgroundColor;  // ditto Keyboard backgraund
		PrefDeltakeysize 	= kbPref->PrefDeltakeysize;		// Preference increment in key size
		PrefshowActivekey 	= kbPref->PrefshowActivekey;	// Show cap letters in keys
		KBLayout			= kbPref->KBLayout;				// 101, 102, 106, KB layout
		Pref3dkey 			= kbPref->Pref3dkey = FALSE;    //  ONLY Use 2d keys
		Prefusesound 		= kbPref->Prefusesound;			// Use click sound
		PrefAlwaysontop     = kbPref->PrefAlwaysontop;      // windows always on top
		
		//if Scanning on, we don't want hilite key
		if(kbPref->PrefScanning)
			Prefhilitekey        = FALSE;          
		else
			Prefhilitekey = kbPref->Prefhilitekey  = TRUE;    //hilite key under cursor

		PrefDwellinkey       = kbPref->PrefDwellinkey;    // TRUE for dwelling
		PrefDwellTime        = kbPref->PrefDwellTime;     // How long to dwell

		PrefScanning         = kbPref->PrefScanning;
		PrefScanTime         = kbPref->PrefScanTime;

		g_fShowWarningAgain	= kbPref->fShowWarningAgain;   // Show initial warning message again
		
		prefUM	= CheckUM();

		// font
		plf = &lf; 				// pointer to the actual font

		plf->lfHeight	 		= kbPref->lf.lfHeight;
		plf->lfWidth 			= kbPref->lf.lfWidth;
		plf->lfEscapement 		= kbPref->lf.lfEscapement;
		plf->lfOrientation 		= kbPref->lf.lfOrientation;
		plf->lfWeight 			= kbPref->lf.lfWeight;
		plf->lfItalic 			= kbPref->lf.lfItalic ;
		plf->lfUnderline 		= kbPref->lf.lfUnderline;
		plf->lfStrikeOut 		= kbPref->lf.lfStrikeOut;
		plf->lfCharSet 			= kbPref->lf.lfCharSet;
		plf->lfOutPrecision 	= kbPref->lf.lfOutPrecision;
		plf->lfClipPrecision 	= kbPref->lf.lfClipPrecision;
		plf->lfQuality 			= kbPref->lf.lfQuality ;
		plf->lfPitchAndFamily 	= kbPref->lf.lfPitchAndFamily;

        wsprintf(plf->lfFaceName, TEXT("%hs"), kbPref->lf.lfFaceName);

//"MS SHELL DLG" is the alias to default font 
//wsprintf(plf->lfFaceName, TEXT("%hs"), "MS SHELL DLG");

		newFont = TRUE;


		//Use 101 keyboard layout   (default is 101 and Actual layout)
		if(KBLayout == 101)
		{
			//The setting say use Block layout, so switch to Block structure
			if(!kbPref->Actual)
				BlockKB();
		}

		// Use 102 keyboard layout
		else if(KBLayout == 102)
			EuropeanKB();

		//Use 106 keyboard layout
		else
			JapaneseKB();
	 }

    else
    {
        SendErrorMessage(IDS_SETTING_DAMAGE);
   		ExitProcess(0);
    }
}

/**************************************************************/
DWORD WhatPlatform(void)
{	OSVERSIONINFO	osverinfo;

	osverinfo.dwOSVersionInfoSize = (DWORD)sizeof(OSVERSIONINFO);
	GetVersionEx(&osverinfo);
	return osverinfo.dwPlatformId;

}
/**************************************************************/
// Check to see the keyboard is out of screen or not with the given
// Screen resoultion (scrCX, scrCY)
/**************************************************************/
BOOL IsOutOfScreen(int scrCX, int scrCY)
{	
	//Check left and top
	if(kbPref->KB_Rect.left < 0 || kbPref->KB_Rect.top < 0)
		return TRUE;

	//Check right and bottom
	if(kbPref->KB_Rect.right > scrCX || kbPref->KB_Rect.bottom > scrCY)
		return TRUE;

	return FALSE;

}
