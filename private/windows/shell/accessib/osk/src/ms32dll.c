// Copyright (c) 1997-1999 Microsoft Corporation
// File: ms32dll.c
// Additions, Bug Fixes 1999 
// a-anilk and v-mjgran
//

#define STRICT


#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <winable.h>


#include "kbmain.h"
#include "resource.h"
#include "kbus.h"
#include "keyrc.h"
#include "ms32dll.h"


#define DWELLTIMER 267
#define PAINTTIMER 101
#define NUM_OF_CLICK 200       //Number of Click before put up Madenta Dialog

BOOL g_fDoubleHookMsg = FALSE;		// To avoid repeat event management with both hooks
HWND g_hLastKey = NULL;

BOOL g_fRealChar = FALSE;		//v-mjgran: To know when the scanning key must be sent to the next hook

int   paintlineNo = 0;
HWND  lastMwnd = NULL;      // handle to window under mouse
HWND  DWin=NULL;            // a copy of Dwellwindow handle
BOOL  lastMdown = FALSE;    // Was last key down?
HHOOK hkMouse;              // handle to mouse hook
BOOL kbfAltKey = FALSE;     // flag AltKey
BOOL kbfCtrKey = FALSE;     // flag CtrKey


HWND CtrlHwnd=NULL;
HWND AltHwnd=NULL;
HWND CapHwnd=NULL;
HWND WinKeyHwnd=NULL;
BOOL IsDeadKeyPressed(HWND hScannedKey);
BOOL SendSAS();

BOOL WinKeyDown= FALSE;
BOOL bCtrl_Alt_Del = FALSE;   //Ctr+Alt+Del down
BOOL Shift=FALSE;      //Shift is down
BOOL Menu=FALSE;       //Menu is down
BOOL Control=FALSE;    //Control is down

// External variables
extern TOOLINFO		ti;
extern HWND			g_hToolTip;
extern BOOL kbfCapLetter;   // flag Capital Letters key
extern BOOL kbfCapLock;     // flag Capital Letters Lock key
extern DWORD platform;
extern HWND DeadHwnd=NULL;
extern HWND ShiftHwnd=NULL;
extern BOOL Shift_Dead=FALSE;
extern BOOL save_the_shift= FALSE;


extern BOOL g_fDrawShift = FALSE;
extern BOOL g_fDrawCapital;

HWND g_hBitmapLockHwnd = NULL;		//v-mjgran: CapLock when japaneese keyboard is bitmap type

// Functions
void SendKeys(HWND ldhwnd, UINT vk, UINT scanCode);
void SendKeysN(UINT vk, UINT scanCode);

/***************************************************************************/
/*     Functions Declaration      */
/***************************************************************************/

void DoButtonUp(HWND hwnd);

void SendAltCtrlDel();

//__declspec( dllexport ) BOOL InstallOtherMouseHook(HWND hwnd);
//__declspec( dllexport ) BOOL KillOtherMouseHook(void);



/***************************************************************************/
/*   functions not in this file              */
/***************************************************************************/
#include "scan.h"
#include "kbfunc.h"
#include "thanks.h"
#include "sdgutil.h"
#include "ms32dll.h"


/***********************************************************************/
/*	BOOL InitMouse(void)    */
/***********************************************************************/


BOOL WINAPI InitMouse(void)
{

	
	DWORD MainthreadID;

	MainthreadID = GetCurrentThreadId();

	hkMouse = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)MouseProc,NULL, MainthreadID);
	
//if(!InstallOtherMouseHook(kbmainhwnd))
//	MessageBox(0, "Fail", "Fail", MB_OK);

	if(hkMouse == NULL)
	{
		SendErrorMessage(IDS_MOUSE_HOOK);
		return (FALSE);
	}

	return(TRUE);

}


/***********************************************************************/
/*	void  KillMouse(void)    */
/***********************************************************************/


BOOL WINAPI KillMouse(void)
{

//if(!KillOtherMouseHook())
//	MessageBox(0, "Fail", "Fail", MB_OK);

	if(timerK1 != 0)
		killtime();

	if(UnhookWindowsHookEx(hkMouse) == 0)
		return (FALSE);

	return(TRUE);
}


/***********************************************************************/
/*	BOOL  CapLetterON(void)    */
/***********************************************************************/
BOOL WINAPI CapLetterON(void)
{
	return g_fDrawShift || g_fDrawCapital;
}


/***********************************************************************/
/*	BOOL  AltKeyPressed(void)    */
/***********************************************************************/
BOOL AltKeyPressed(void)
{
	return(Menu);
}

/***********************************************************************/
/*	BOOL  ControlKeyPressed(void)    */
/***********************************************************************/
BOOL ControlKeyPressed(void)
{
	return(Control);
}

/**************************************************************************/
/* MouseProc                                                              */
/* Filter function for the WH_MOUSE                                       */
/**************************************************************************/
//BOOL WantRepeat=FALSE;
extern HKL	hkl=0;
/**************************************************************************/



LRESULT CALLBACK  MouseProc (int nCode, WPARAM wParam, LPARAM lParam )
{
	MOUSEHOOKSTRUCT  *mhs;
	BOOL fCallNextHook = TRUE;
	BOOL YesItIs =  FALSE;
	static HWND	lastKey=NULL;
	
	static HKL old_hkl=0;
	DWORD dwProcessId;
	DWORD dwlayout;

	
	//*** Language checking  Work both on Win95 & NT ***
	
	//Get the active window's thread
	dwlayout=GetWindowThreadProcessId(GetActiveWindow(), &dwProcessId);
	//Get the active window's keyboard layout
	hkl=GetKeyboardLayout(dwlayout);

	if (old_hkl==0)
		old_hkl = hkl;
	else if(old_hkl != hkl)
	{
		if(!ActivateKeyboardLayout(hkl, 0))
			SendErrorMessage(IDS_CANNOT_SWITCH_LANG);

		old_hkl = hkl;

        RedrawKeys();

	}


	//v-mjgran: If OSK has the focus, indicate it
     //Don't do anything if we are the active window
//	 if((nCode < 0) || (GetFocus() == kbmainhwnd) ||
//					(GetActiveWindow() == kbmainhwnd) || DialogOn)


	
	
	mhs =  (MOUSEHOOKSTRUCT *)lParam;

	YesItIs = IsOneOfOurKey(mhs->hwnd);

	
	 //v-mjgran: Hooks must also work when the OSK dialogs are opened, but only over OSK keys
	 if((nCode < 0) || !YesItIs)
	 {
		 g_hLastKey = NULL;
		 return CallNextHookEx(hkMouse, nCode, wParam, lParam);
	 }

	 // *** Return the color when the moment cursor on keyboard    ****
	 // but don't do anything else
	 if((lastKey != NULL) && !YesItIs && (mhs->hwnd== kbmainhwnd)
			&& (lastKey != kbmainhwnd) && (mhs->hwnd != lastKey))
	 {
		g_hLastKey = NULL;
		ReturnColors(lastKey, TRUE);
		lastKey= mhs->hwnd;
		lastMwnd= mhs->hwnd;
		return CallNextHookEx(hkMouse, nCode, wParam, lParam);
	 }

	 if (g_fDoubleHookMsg || (g_hLastKey && g_hLastKey == mhs->hwnd && PrefDwellinkey))
	 {
		 // v-mjgran: Do not repeat the process. This message has been managed already in
		 // the JournalRecordProc
		g_fDoubleHookMsg = FALSE;
		lastKey = g_hLastKey;
		return CallNextHookEx(hkMouse, nCode, wParam, lParam);
	 }


	 switch (nCode)
		{
		case HC_ACTION:
			{
			switch (wParam)
				{

			case WM_RBUTTONDOWN:     //Kill scanning temparory
				KillScanTimer(TRUE);  //kill scanning
			break;
			
			case WM_MOUSEMOVE:
			        // just moving around
	 				if(lastMwnd == mhs->hwnd)          // && (!WantRepeat))
	 					break;

					if((lastMwnd != kbmainhwnd) && (lastMwnd != NULL))
					{
						if(Prefhilitekey || PrefDwellinkey)
								ReturnColors(lastMwnd, TRUE);   //else return with lastMwnd
					}

					if(mhs->hwnd != kbmainhwnd)    //for dwell clicking
					{
							if(YesItIs)    //make sure the current window is Key or Predict Key
							{
								DWin=Dwellwindow = mhs->hwnd;    // Set the Dwell window

								if(Prefhilitekey)
									InvertColors(mhs->hwnd);

								if(PrefDwellinkey)
								{
									killtime();
									SetTimeControl(mhs->hwnd);
								}
							}

							lastKey = mhs->hwnd;
							g_hLastKey = NULL;
					}
					lastMwnd = mhs->hwnd;
			break;

			case WM_LBUTTONDOWN:      //for mouse clicking
									
					if(PrefDwellinkey)
						killtime();                      //  stop timer

					ReturnColors(Dwellwindow, FALSE);
					lastMdown = TRUE;
					if(mhs->hwnd != kbmainhwnd)
					{	
						

// *** Cannot send any redraw before (or even after (within short period of
// ***  time)), ToAscii() will eat the dead key	
// *** Therefore, cannot do Flashing with user keep pressing the key
						
						//InvertColors(mhs->hwnd);
						SendChar(mhs->hwnd);          //Send out the keystroke
				
					}
					
					else
					{
						fCallNextHook = FALSE;
					}
					
					lastMwnd = mhs->hwnd;
			break;

			case WM_LBUTTONUP:            //do pressing keys
					
					
                    // up in the key as we mouse down
                    if(lastMwnd == mhs->hwnd)
                    {
                        if(lastMdown)
						{	lastMdown = FALSE;
							if(lastMwnd != kbmainhwnd)
							{
								DoButtonUp(mhs->hwnd);
								//ReturnColors(lastMwnd, FALSE);
							}

							fCallNextHook = FALSE;
						}
					}

					else
					{
						lastMwnd = mhs->hwnd;
					}
			break;
				}
			}
		break;
		
		case HC_NOREMOVE:       // at this time we don't have to worry about this
		default:
			break;
		}

	
	 // if(fCallNextHook)
	 return CallNextHookEx(hkMouse, nCode, wParam, lParam);

	//  return 1;		//v-mjgran

	//return CallNextHookEx(hkMouse, nCode, wParam, lParam);    //message was processed
}





/**********************************************************************/
/*    BOOL IsOneOfOurKey(HWND hwnd)   */
/**********************************************************************/
BOOL IsOneOfOurKey(HWND Khwnd)
{
	register int i;

	for(i=1; i < lenKBkey; i++)
		if(lpkeyhwnd[i] == Khwnd)
			return(TRUE);
	return(FALSE);
}
/************************************************************************/

static BOOL lastDown = FALSE;
/************************************************************************/
/* DoButtonUp*/
/************************************************************************/
void DoButtonUp(HWND Childhwnd)
{
	if (g_hBitmapLockHwnd == Childhwnd)
	{
		//v-mjgran: Do no change the bitmap color. It will be change with WM_PAINT message
		return;
	}

	SetWindowLong (Childhwnd, 0, 0);

	//if dwell mode on, don't re-draw here. It will eat up the dead key.
	//This bug only happen in NT not Win95
	if(!PrefDwellinkey || (platform != VER_PLATFORM_WIN32_NT))
		InvalidateRect (Childhwnd, NULL, TRUE);

	if(Prefusesound == TRUE)
	{
		MakeClick(SND_DOWN);
		lastDown = TRUE;
	}
}
/***********************************************************************/
extern BOOL Capitalize=FALSE;
extern BOOL RALT=FALSE;
/**************************************************************************/
/* void MakeClick(int what)                                               */
/**************************************************************************/
void MakeClick(int what)
{	
	switch (what)
		{
		case SND_UP:
            PlaySound(MAKEINTRESOURCE(WAV_CLICKUP), hInst,
                      SND_SYNC|SND_RESOURCE);	
		break;

		case SND_DOWN:
            PlaySound(MAKEINTRESOURCE(WAV_CLICKDN), hInst,
                      SND_SYNC|SND_RESOURCE);
		break;
		}
	return;

} // MakeClick


/**************************************************************************/
/* void InvertColors(HWND chwnd)                                          */
/**************************************************************************/
void InvertColors(HWND chwnd)
{
	//stop invert the LED lights windows
//	if(chwnd==lpkeyhwnd[lenKBkey-3] || chwnd==lpkeyhwnd[lenKBkey-2]
//		|| chwnd==lpkeyhwnd[lenKBkey-1])
//		return;

	SetWindowLong(chwnd, 0, 4);
	DeleteObject((HGDIOBJ)SetClassLongPtr(chwnd, GCLP_HBRBACKGROUND,
			(LONG_PTR)CreateSolidBrush(RGB(0,0,0))));


	InvalidateRect(chwnd, NULL, TRUE);

} // InvertColors


/**************************************************************************/
/* void ReturnColors(HWND chwnd, BOOL inval)                              */
// Repaint the key
/**************************************************************************/
void ReturnColors(HWND chwnd, BOOL inval)
{
	int index;
	COLORREF selcolor;
	BOOL Doit=FALSE;       //Do some check before do redraw, save some time! :-)

	stopPaint = TRUE;

	index = GetWindowLong(chwnd, GWL_ID);  //order of the key in the array

	if(index < lenKBkey && index >= 0)
		switch (KBkey[index].ktype)
		{
			case KNORMAL_TYPE:
				if(chwnd!= DeadHwnd)
				{	selcolor = COLOR_WINDOW;
					Doit= TRUE;
					SetWindowLong(chwnd, 0, 0);
				}
			break;
			case KMODIFIER_TYPE:
				if((chwnd!=ShiftHwnd)&&(chwnd!=CtrlHwnd)&&(chwnd!=AltHwnd)
							&&(chwnd!=CapHwnd)&&(chwnd!=WinKeyHwnd))
				{
					if (!chwnd || chwnd != g_hBitmapLockHwnd)
						SetWindowLong(chwnd, 0, 0);

					selcolor = COLOR_MENU;
					Doit= TRUE;
				}
			break;
			case KDEAD_TYPE:
				selcolor = COLOR_MENU;
				Doit= TRUE;
				SetWindowLong(chwnd, 0, 0);
			break;
			case NUMLOCK_TYPE:
				if(RedrawNumLock()==0)         //RedrawNumLock return 0 if NumLock is OFF
				{	selcolor = COLOR_WINDOW;   //then just draw it as un-hilite
					Doit= TRUE;
					SetWindowLong(chwnd, 0, 0);
				}
			break;

			case SCROLLOCK_TYPE:
				if(RedrawScrollLock()==0)         //RedrawNumLock return 0 if NumLock is OFF
				{	selcolor = COLOR_WINDOW;   //then just draw it as un-hilite
					Doit= TRUE;
					SetWindowLong(chwnd, 0, 0);
				}
			break;

		}

	if(Doit)     //Doit TRUE = we are on KEYS or PREDICT KEYS , if true then redraw it!!
	{
		DeleteObject((HGDIOBJ)SetClassLongPtr(chwnd, GCLP_HBRBACKGROUND, selcolor));

		if(inval == TRUE)
		{
			InvalidateRect(chwnd,NULL, TRUE);
			UpdateWindow(chwnd);
		}
	}
}// ReturnColors
/*******************************************************************************/
/*void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)*/
/*******************************************************************************/
void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	static int lastF = -1;
	POINT pt;
	HWND	temphwnd;
	int x=0;


	//Stop all the Dwell timer
	killtime();


	if(PrefDwellinkey != TRUE)
		return;


	SetWindowLong(DWin,0, 0);


	// check if the mouse is over our dwellwindow
	GetCursorPos(&pt);                		// check if it is a dwelling window
	temphwnd = WindowFromPoint(pt);
	
	//If not the Dwell window, do nothing.
	if(DWin != temphwnd)
	{
		if(DWin != NULL)
			InvalidateRect(DWin, NULL, TRUE);
		return;
	}

	// Repeated 'Function keys' (f1 - f12) clicking can make a mess in the
    // "host" program.
	// Then.... Don't let stay over that key.

	x = GetWindowLong(DWin, GWL_ID);
	if(x < 13)
	{
		if(lastF != x)
			lastF = x;
		else
			return;
	}
	else
		lastF = -1;


	//Send out the char
	SendChar(DWin);

	//Redraw the key to original color
	ReturnColors(DWin, FALSE);

	//Redraw the key as button up
	DoButtonUp(DWin);	
	
	
	DWin=NULL;

}// YourTimeIsOver
/**************************************************************************/
/* void killtime(void)                                                 	  */
/**************************************************************************/
void killtime(void)
{

		stopPaint = TRUE;

		KillTimer(kbmainhwnd, timerK1);
		timerK1 = 0;
		KillTimer(kbmainhwnd, timerK2);

		if((Dwellwindow!= NULL) && (Dwellwindow != kbmainhwnd))
			InvalidateRect(Dwellwindow, NULL, TRUE);

}
/**************************************************************************/
/* void SetTimeControl(void)                                           	  */
/**************************************************************************/
void SetTimeControl(HWND  hwnd)
{
    int timepiece;

	if(PrefDwellinkey == FALSE)
		return;

	//*** On keyboard key  ***
	if(!Prefhilitekey)           //if not hilite key make the key black for Dwell
		InvertColors(hwnd);

    timepiece=(int)((float)PrefDwellTime * (float)1);   //1.5

	timerK1 = SetTimer(kbmainhwnd, DWELLTIMER, timepiece, YourTimeIsOver);
	stopPaint = FALSE;
	PaintBucket(hwnd);

}// SetTimeControl


/**************************************************************************/
/* void PaintBucket(void)                                             	  */
/**************************************************************************/
void PaintBucket(HWND  hwnd)
{	
    int timepiece;					// time between bucket's line

	paintlineNo = 0;

	timepiece = (int)((float)PrefDwellTime * (float)0.07);

	timerK2 = SetTimer(kbmainhwnd, PAINTTIMER, timepiece, Painttime);

}// PaintBucket
/**************************************************************************/
/* void CALLBACK Painttime(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)*/
/**************************************************************************/
void CALLBACK Painttime(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	POINT pt;

	GetCursorPos(&pt);
	if(!(IsOneOfOurKey(WindowFromPoint(pt))))
	{
		killtime();
		ReturnColors(Dwellwindow, TRUE);
	}

	if(stopPaint == TRUE)
		return;

	SetWindowLong(Dwellwindow, 0, 5);
	InvalidateRect(Dwellwindow, NULL, FALSE);
}// Painttime


/********************************************************************
* void PaintLine(HWND hwnd, HDC hdc, RECT rect, int Wline)
*
* Paint the bucket
********************************************************************/
void PaintLine(HWND hwnd, HDC hdc, RECT rect)
{
	POINT bPoint[3];
	HPEN oldhpen;
	HPEN hPenWhite;

	LOGPEN lpWhite = { PS_SOLID, 1, 1, RGB (255, 255, 255) };

	hPenWhite = CreatePenIndirect(&lpWhite);
	oldhpen = SelectObject(hdc, hPenWhite);

	bPoint[0].x = 0;
	bPoint[0].y = rect.bottom -(1 * paintlineNo);
	bPoint[1].x = rect.right;
	bPoint[1].y = bPoint[0].y;
	bPoint[2].x = 0;
	bPoint[2].y = rect.bottom -(1 * paintlineNo);

	if(stopPaint != TRUE)
		Polyline(hdc, bPoint, 3);

	SelectObject(hdc, oldhpen);
	DeleteObject(hPenWhite);

	paintlineNo++;
	paintlineNo++;
} // PaintLine


/**************************************************************************/
void ReDrawModifierKey(void)
{
	if(ShiftHwnd!=NULL)
	{
		SetWindowLong(ShiftHwnd, 0, 0);
		if(!PrefshowActivekey)
		{
			InvalidateRect(ShiftHwnd, NULL, TRUE);
		}
		ShiftHwnd= NULL;
	}

	if(CtrlHwnd!= NULL)
	{
		SetWindowLong(CtrlHwnd, 0, 0);
		InvalidateRect(CtrlHwnd, NULL, TRUE);
		CtrlHwnd= NULL;

	}

	if(AltHwnd!= NULL)
	{
		SetWindowLong(AltHwnd, 0, 0);
		InvalidateRect(AltHwnd, NULL, TRUE);
		AltHwnd= NULL;
	}

}

/**************************************************************************/


/**************************************************************************/
//Handle the Window Keys and App Key. Send out keystroke or key combination
//using SendInput.
/**************************************************************************/
void Extra_Key(HWND hwnd, int index)
{	UINT scancode;
	UINT vk;
	static UINT vk_win;
	static UINT scancode_win;

	//Previous Window Key Down. Now user press char key, or Window key again
	if(WinKeyDown)
	{
		WinKeyDown = FALSE;


		//Re-Draw the window key
		SetWindowLong(WinKeyHwnd, 0, 0);
		SetClassLongPtr(WinKeyHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
		InvalidateRect(WinKeyHwnd, NULL, TRUE);
		
		
		WinKeyHwnd= NULL;

		
		//Left Window Key, previously down, now release it.
		if(lstrcmp(KBkey[index].skCap,TEXT("lwin"))==0)
		{	
			INPUT	rgInput[1];

            scancode= MapVirtualKey(VK_LWIN, 0);

			//LWIN up
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = VK_LWIN;
            rgInput[0].ki.wScan = (WORD) scancode;

			SendInput(1, rgInput,sizeof(INPUT));

		}

		
		//Right Window Key, previously down, now release it.
		else if(lstrcmp(KBkey[index].skCap,TEXT("rwin"))==0)
		{	
			INPUT	rgInput[1];

            scancode= MapVirtualKey(VK_RWIN, 0);
			
			
			//RWIN up
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = VK_RWIN;
            rgInput[0].ki.wScan = (WORD) scancode;

			SendInput(1, rgInput,sizeof(INPUT));

		}

		
		else  //Window key conbination. We send (letter + Win key up)
		{
			INPUT	rgInput[3];


			vk = MapVirtualKey(KBkey[index].scancode[0],1);


			//key down
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = 0;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = (WORD) vk;
            rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];

			//key up
			rgInput[1].type = INPUT_KEYBOARD;
            rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
            rgInput[1].ki.dwExtraInfo = 0;
            rgInput[1].ki.wVk = (WORD) vk;
            rgInput[1].ki.wScan = (WORD) KBkey[index].scancode[0];

			//Win key up
			rgInput[2].type = INPUT_KEYBOARD;
            rgInput[2].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[2].ki.dwExtraInfo = 0;
            rgInput[2].ki.wVk = (WORD) vk_win;
            rgInput[2].ki.wScan = (WORD) scancode_win;

			SendInput(3, rgInput,sizeof(INPUT));

		}
	}
	else
	{	
		//App Key
		if(lstrcmp(KBkey[index].textL,TEXT("MenuKeyUp"))==0)
		{
			INPUT	rgInput[2];

            SetWindowLong(hwnd,0,0);
			InvalidateRect(hwnd, NULL, TRUE);
			scancode= MapVirtualKey(VK_APPS, 0);


			//App key down
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = VK_APPS;
            rgInput[0].ki.wScan = (WORD) scancode;

			//App key up
			rgInput[1].type = INPUT_KEYBOARD;
            rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
            rgInput[1].ki.dwExtraInfo = 0;
            rgInput[1].ki.wVk = VK_APPS;
            rgInput[1].ki.wScan = (WORD) scancode;

			SendInput(2, rgInput,sizeof(INPUT));
			//Make click sound
			if(Prefusesound)
				MakeClick(SND_DOWN);

		}

		//Left Window Key Down
		else if(lstrcmp(KBkey[index].skCap,TEXT("lwin"))==0)
		{
			INPUT rgInput[1];

			scancode= MapVirtualKey(VK_LWIN, 0);
			
			//LWIN down
			rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = VK_LWIN;
            rgInput[0].ki.wScan = (WORD) scancode;

			SendInput(1, rgInput,sizeof(INPUT));


			vk_win= VK_LWIN;
			scancode_win= scancode;
			WinKeyDown= TRUE;

			//Change the Win key color to make it stay down
			SetWindowLong(hwnd, 0, 4);	
			DeleteObject((HGDIOBJ)SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
			InvalidateRect(hwnd, NULL, TRUE);
			WinKeyHwnd= hwnd;
		}

		//Right Window Key Down
		else
		{
			INPUT	rgInput[1];

			scancode= MapVirtualKey(VK_RWIN, 0);
			
			//RWIN down
	        rgInput[0].type = INPUT_KEYBOARD;
            rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
            rgInput[0].ki.dwExtraInfo = 0;
            rgInput[0].ki.wVk = VK_RWIN;
            rgInput[0].ki.wScan = (WORD) scancode;

			SendInput(1, rgInput,sizeof(INPUT));


			vk_win= VK_RWIN;
			scancode_win= scancode;
			WinKeyDown= TRUE;

			//Change the Win key color to make it stay down
			SetWindowLong(hwnd, 0, 4);	
			DeleteObject((HGDIOBJ)SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
			InvalidateRect(hwnd, NULL, TRUE);
            WinKeyHwnd= hwnd;
		}
	}

}
/**************************************************************************/
void NumPad(UINT sc, HWND hwnd)
{	
	INPUT   rgInput[2];

	switch (sc)
		{
	case 0x47:  // 7
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			//Down
			SendKeysN(VK_NUMPAD7, sc);
		else      //OFF
			SendKeysN(VK_HOME, sc);
	break;

	case 0x48:  //8
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD8, sc);
		else        //OFF
			SendKeysN(VK_UP, sc);
	break;
	
	case 0x49:  //9
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD9, sc);
		else       //OFF
			SendKeysN(VK_PRIOR, sc);
	break;

	case 0x4A:  //-

			SendKeysN(VK_SUBTRACT, sc);
	break;

	case 0x4E:   //+
			SendKeysN(VK_ADD, sc);
	break;
	
	case 0x4B:   //4
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			
			SendKeysN(VK_NUMPAD4, sc);
		else      //OFF
			SendKeysN(VK_LEFT, sc);
	break;

	case 0x4C:   //5
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
		
			SendKeysN(VK_NUMPAD5, sc);
	break;

	case 0x4D:    //6
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD6, sc);
		else      //OFF
			SendKeysN(VK_RIGHT, sc);
	break;

	case 0x4F:   //1
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD1, sc);
		else	//OFF
			SendKeysN(VK_END, sc);
	break;

	case 0x50:   //2
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD2, sc);
		else	//OFF
			SendKeysN(VK_DOWN, sc);
	break;

	case 0x51:    //3
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD3, sc);
		else	//OFF
			SendKeysN(VK_NEXT, sc);
	break;
	
	case 0x52:    //0
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_NUMPAD0, sc);
		else	//OFF
			SendKeysN(VK_INSERT, sc);
	break;

	case 0x53:    //.
		if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			SendKeysN(VK_DECIMAL, sc);
		else	//OFF
		{
			HWND hWnd;

			//User press Ctrl+Alt+Del
			if(Control && Menu)
			{	//change back to its normal state (key up)
				SetWindowLong(AltHwnd, 0, 0);
				SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
				InvalidateRect(AltHwnd, NULL, TRUE);
				
				//change back to its normal state (key up)
				SetWindowLong(CtrlHwnd, 0, 0);
				SetClassLongPtr(CtrlHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
				InvalidateRect(CtrlHwnd, NULL, TRUE);
				
				bCtrl_Alt_Del = TRUE;
				
				SendSAS();
			}

			SendKeysN(VK_DELETE, sc);
			
		}		
	break;
		}

	
	if(PrefDwellinkey && (platform == VER_PLATFORM_WIN32_NT))
		InvalidateRect(hwnd, NULL, TRUE);

}

/**************************************************************************/
/* void SendChar(BYTE scancode)                                           */
// Send out the keystroke using SendInput
// Given: the window handle of the key the user press
/**************************************************************************/
void SendChar(HWND ldhwnd)
{	UINT vk;
	int index;
	BOOL extend=FALSE;
	static BOOL bCapDown;	
	static UINT Dead_vk = 0, Dead_scancode=0;
	static BOOL DeadKey = FALSE;
	static int count = 0;			//v-mjgran: do not show the Madenta dialog after 200 keystrokes
	static BOOL bDisplayH = FALSE;
	static BOOL fRALTPrevious;
	static BOOL fShiftPrevious;

	if(bCtrl_Alt_Del)  //if previously press Ctrl+Alt+Del, release Alt and Ctrl keys
		ReleaseAltCtrlKeys();

/*	//v-mjgran: do not show the Madenta dialog after 200 keystrokes		
	if(count == NUM_OF_CLICK)
	{	
		ThanksDlgFunc(kbmainhwnd, 0, 0, 0);
		
		count = 0;

		return;
	}
	else
		count++;
*/

	if (((GetFocus() == kbmainhwnd) || (GetActiveWindow() == kbmainhwnd))
		&& (count < 3) )
	{
		//v-mjgran: OSK has the focus and the user type or use scan mode
		POINT pt;
		bDisplayH = TRUE;
		GetCursorPos(&pt);   
		count++;

		SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)TRUE,(LPARAM)&ti);
        SendMessage(g_hToolTip, TTM_TRACKPOSITION,0, (LPARAM)MAKELPARAM(pt.x+10, pt.y+10));

		SetTimer(kbmainhwnd, 1014, 4000, NULL);
	}
	else if ( bDisplayH )
	{
		bDisplayH = FALSE;
		SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)FALSE,(LPARAM)&ti);
	}

	index = GetWindowLong(ldhwnd, GWL_ID);  //order of the key in the array

	//Make sure we are in the range(# of keys)
	if(index < 0 || index > lenKBkey)
	{
		return;
	}


	//Extra Keys (Window Keys, App Key)
	if((lstrcmp(KBkey[index].textL,TEXT("winlogoUp"))==0) ||
       (lstrcmp(KBkey[index].textL,TEXT("MenuKeyUp"))==0) || WinKeyDown)
	{	
		Extra_Key(ldhwnd, index);
		return;
	}


    //extended key
	if (KBkey[index].scancode[0] == 0xE0)
	{
		if ((KBkey[index].scancode[1] >= 0x47) &&
            (KBkey[index].scancode[1] <= 0x53))
		{
			//Incase of Arrow keys/ Home/ End keys do
			// special procesing.
			switch (KBkey[index].scancode[1])
			{
				case 0x47:  // Home
					   vk = VK_HOME;
					break;

				case 0x48:  //UP
						vk = VK_UP;
					break;
				
				case 0x49:  //PGUP
						vk = VK_PRIOR;
					break;

				case 0x4B:   //LEFT
					 vk = VK_LEFT;
					break;

				case 0x4D:    //RIGHT
						vk = VK_RIGHT;
					break;

				case 0x4F:   //END
						vk = VK_END;
					break;

				case 0x50:   //DOWN
						vk = VK_DOWN;
					break;

				case 0x51:    //PGDOWN
						vk = VK_NEXT;
					break;
				
				case 0x52:    //INS
						vk = VK_INSERT;
					break;

				case 0x53:    //DEL
						//Down
						vk = VK_DELETE;

						if(Control && Menu)
						{	
							bCtrl_Alt_Del = TRUE;
							SendSAS();
						}
					break;
			}

			// Do the processing here itself
			SendKeys(ldhwnd, vk, KBkey[index].scancode[1]);
			return;
			// vk = MapVirtualKey(KBkey[index].scancode[1], 3);
		}
		else
        vk = MapVirtualKey(KBkey[index].scancode[1], 1);
		
		extend=TRUE;
	}
	
	
	//NumPad
	else if((KBkey[index].scancode[0] >= 0x47) &&
            (KBkey[index].scancode[0] <= 0x53))
	{
        NumPad(KBkey[index].scancode[0], ldhwnd);
		return;
	}
	
	//other keys
	else
	{
		vk = MapVirtualKey(KBkey[index].scancode[0], 1);
	}



	switch (KBkey[index].name)	//(vk)
		{
	case KB_PSC:  //Print screen
		{
			SendKeysN(VK_SNAPSHOT, 0);

		}
	break;

	case KB_LCTR:	//case VK_CONTROL:
	case KB_RCTR:
		Control= !Control;

		if(Control)    //user press once
		{	

			INPUT	rgInput[1];
			
			//Control down
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = 0;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_CONTROL;   //In here I didn't use vk which return from MapVirtualKey
			                                  //because for the RCtrl, the vk value is wrong. I assume
											  //ctrl is always VK_CONTROL in all languages
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];

			SendInput(1, rgInput, sizeof(INPUT));

			
			//Change the ctrl color to let it stay down
			SetWindowLong(ldhwnd, 0, 4);	
			DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
			SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);
			CtrlHwnd = ldhwnd;
			InvalidateRect(ldhwnd, NULL, TRUE);
		}
		else           //user press again
		{				
			INPUT	rgInput[1];

			//Control up
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_CONTROL;
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];

			SendInput(1, rgInput, sizeof(INPUT));

			
			//change back to its normal state (key up)
			SetWindowLong(CtrlHwnd, 0, 0);
			SetClassLongPtr(CtrlHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(CtrlHwnd,  GWLP_USERDATA,0);
			InvalidateRect(CtrlHwnd, NULL, TRUE);
			CtrlHwnd = NULL;
		}
	break;

	case KB_CAPLOCK:	//case VK_CAPITAL:
		{

		SendKeysN(vk, KBkey[index].scancode[0]);

		kbfCapLetter = !kbfCapLetter;
		kbfCapLock  = !kbfCapLock;		//v-mjgran: Change the CapLock value

		//CapLock DOWN
		
		//v-mjgran: To know if CapLock is down, the variable is kbfCapLock, because kbfCapLetter is also 
		//modified when Shift is press
		//if(kbfCapLetter)
		if (kbfCapLock )
		{
			bCapDown = TRUE;

			//change caplock key color to let it stay down
			SetWindowLong(ldhwnd, 0, 4);	
			DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
						 (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
			SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);
			CapHwnd = ldhwnd;
		}
		//CapLock UP
		else
		{
			bCapDown = FALSE;

			//change the key color back to normal (key up) state
			SetWindowLong(CapHwnd, 0, 0);		
			SetClassLongPtr(CapHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(CapHwnd,  GWLP_USERDATA,0);
			CapHwnd = NULL;
		}

        RedrawKeys();

		}
	break;

	case KB_LSHIFT:		//VK_SHIFT:
	case KB_RSHIFT:	

    	Shift= !Shift;
		
		if(Shift)        //user press once
		{	
			INPUT	rgInput[1];

			if (!DeadKey)
			{
				//Shift down
				rgInput[0].type = INPUT_KEYBOARD;
				rgInput[0].ki.dwFlags = 0;
				rgInput[0].ki.dwExtraInfo = 0;
				rgInput[0].ki.wVk = VK_SHIFT;
				rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];
				
				SendInput(1, rgInput, sizeof(INPUT));
			}
			else
			{
				fShiftPrevious = TRUE;
			}


			//Change the shift key color to let it stay down
			SetWindowLong(ldhwnd, 0, 4);
			DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
			SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);

			ShiftHwnd = ldhwnd;
		}
		else             //user press again
		{	
			INPUT	rgInput[1];

			//Shift up
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_SHIFT;
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];
			
			SendInput(1, rgInput, sizeof(INPUT));
			


			//Return the shift key color
			SetWindowLong(ShiftHwnd, 0, 0);
			SetClassLongPtr(ShiftHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(ShiftHwnd,  GWLP_USERDATA,0);
			ShiftHwnd = NULL;
		}

	if(!kbfCapLock)   //Cap Lock NOT on
			kbfCapLetter = !kbfCapLetter;
		
		RedrawKeys(); //redraw for caps
		

	break;

	case KB_LALT:	//VK_LMENU:
		Menu= !Menu;

		if (Menu)         //user press once
		{	
			INPUT	rgInput[1];

			//LMENU down
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = 0;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_MENU;
			rgInput[0].ki.wScan = 0x38;

			SendInput(1, rgInput, sizeof(INPUT));

			
			//Change the alt key color to let it stay down
			SetWindowLong(ldhwnd, 0, 4);	
			DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
                         (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
			SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);
			AltHwnd = ldhwnd;
			InvalidateRect(AltHwnd, NULL, TRUE);
		}
		else              //user press again
		{	
			INPUT	rgInput[1];

			//LMENU up
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_MENU;
			rgInput[0].ki.wScan = 0x38;
			
			SendInput(1, rgInput, sizeof(INPUT));

			
			//Return the alt key color
			SetWindowLong(AltHwnd, 0, 0);
			SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(AltHwnd,  GWLP_USERDATA,0);
			InvalidateRect(AltHwnd, NULL, TRUE);
			AltHwnd = NULL;
		}
    RedrawKeys();
	break;

	case KB_RALT:	//VK_RMENU:

		Menu = !Menu;
		
		//Non-English Keyboard (nor japanese keyboard)
		if(hkl != (HKL)0x04090409 && LOWORD(hkl) != 0x0411)
		{
			if(Menu)      //user press once
			{
                // the 'right alt' get translate internally to ctrl+alt,
                // but we have to do it ourself

				INPUT	rgInput[2];

				//v-mjgran
				if (!DeadKey)
				{
					//LControl down
					rgInput[0].type = INPUT_KEYBOARD;
					rgInput[0].ki.dwFlags = 0;
					rgInput[0].ki.dwExtraInfo = 0;
					rgInput[0].ki.wVk = VK_CONTROL;
					rgInput[0].ki.wScan = 0x1D;

					//Lmenu down
					rgInput[1].type = INPUT_KEYBOARD;
					rgInput[1].ki.dwFlags = 0;
					rgInput[1].ki.dwExtraInfo = 0;
					rgInput[1].ki.wVk = VK_MENU;
					rgInput[1].ki.wScan = 0x38;
					
					SendInput(2, rgInput, sizeof(INPUT));
				}
				else
				{
					fRALTPrevious = TRUE;
				}


				//Change the alt key color to let it stay down
				SetWindowLong(ldhwnd, 0, 4);	
				DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
							 (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
				SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);
				AltHwnd = ldhwnd;
			}

			else          //user press again
			{	
				INPUT	rgInput[2];

				//LControl Up
				rgInput[0].type = INPUT_KEYBOARD;
				rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
				rgInput[0].ki.dwExtraInfo = 0;
				rgInput[0].ki.wVk = VK_CONTROL;
				rgInput[0].ki.wScan = 0x1D;

				//LMenu Up
				rgInput[1].type = INPUT_KEYBOARD;
				rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
				rgInput[1].ki.dwExtraInfo = 0;
				rgInput[1].ki.wVk = VK_MENU;
				rgInput[1].ki.wScan = 0x38;

				SendInput(2, rgInput, sizeof(INPUT));

				
				//Return the alt key color
				SetWindowLong(AltHwnd, 0, 0);
				SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
				SetWindowLongPtr(AltHwnd,  GWLP_USERDATA,0);
				AltHwnd = NULL;
			}

			RALT = !RALT;
            RedrawKeys();
		}

		//English keyboard
		else
		{	if (Menu)         //user press once
			{	
				INPUT	rgInput[1];	

				//menu down
				rgInput[0].type = INPUT_KEYBOARD;
				rgInput[0].ki.dwFlags = 0;
				rgInput[0].ki.dwExtraInfo = 0;
				rgInput[0].ki.wVk = VK_MENU;
				rgInput[0].ki.wScan = 0x38;
				
				SendInput(1, rgInput, sizeof(INPUT));
			

				//Change the alt key color to let it stay down
				SetWindowLong(ldhwnd, 0, 4);	
				DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd, GCLP_HBRBACKGROUND,
							 (LONG_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))));	
				SetWindowLongPtr(ldhwnd,  GWLP_USERDATA, 1);
				AltHwnd = ldhwnd;
				InvalidateRect(AltHwnd, NULL, TRUE);
			}
			else              //user press again
			{	
				INPUT rgInput[1];

				//Menu Up
				rgInput[0].type = INPUT_KEYBOARD;
				rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
				rgInput[0].ki.dwExtraInfo = 0;
				rgInput[0].ki.wVk = VK_MENU;
				rgInput[0].ki.wScan = 0x38;
				
				SendInput(1, rgInput, sizeof(INPUT));


				//Return the alt key color
				SetWindowLong(AltHwnd, 0, 0);
				SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
				SetWindowLongPtr(AltHwnd,  GWLP_USERDATA,0);
				InvalidateRect(AltHwnd, NULL, TRUE);
				AltHwnd = NULL;
			}
		}

		extend = FALSE;
	break;

	case KB_NUMLOCK:  //VK_NUMLOCK:
		{
			SendKeysN(VK_NUMLOCK, 0x45);

			RedrawNumLock();
		}
	break;

	case KB_SCROLL:
		{
			SendKeysN(VK_SCROLL, 0x46);

			RedrawScrollLock();

//			InvalidateRect(ldhwnd, NULL, TRUE);     //Redraw the key (in case it is in Dwell)
		}
	break;

   case BITMAP:
		{
			//v_mjgran: To manage the CapLock japanese key
			if (!g_hBitmapLockHwnd)
			{
				SetWindowLong(lpkeyhwnd[index], 0, 4);
				g_hBitmapLockHwnd = ldhwnd;
			}
			else
			{
				SetWindowLong(lpkeyhwnd[index], 0, 1);
				g_hBitmapLockHwnd = NULL;
			}

			InvalidateRect(ldhwnd, NULL, TRUE);     //Redraw the key (in case it is in Dwell)
		}

		// Fall through to send the key input...

	default:

		if (extend)      //extended key
		{	
			INPUT rgInput[2];


			// Here we need to be careful, The Arrow keys should always work as arrow keys!
			// Patch from a-anilk. I really don't understand, Why so much of code is required.
			// Some day, I would like to see this function be in 10 lines.
			//extend key down
			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = (WORD) vk;
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[1];
					
			//extend key up
			rgInput[1].type = INPUT_KEYBOARD;
			rgInput[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP;
			rgInput[1].ki.dwExtraInfo = 0;
			rgInput[1].ki.wVk = (WORD) vk;
			rgInput[1].ki.wScan = (WORD) KBkey[index].scancode[1];

			SendInput(2, rgInput, sizeof(INPUT));


			//Make click sound
			if(Prefusesound)
				MakeClick(SND_UP);

			InvalidateRect(ldhwnd, NULL, TRUE);     //Redraw the key (in case it is in Dwell)
			
            if(KBkey[index].scancode[1] == 0x53 && Control && Menu)
				bCtrl_Alt_Del = TRUE;
		}

		else             //Non-Extended key
		{
			BOOL fCurrentDeadKey = IsDeadKey(vk, KBkey[index].scancode[0]);		//v-mjgran


            Shift_Dead = FALSE; // For Shift + CapLock handling


            //The previous keystroke is dead key, so we need to
            // send Deadkey+Char at the same time
			if(DeadKey)
            {
				INPUT	rgInput1[2], rgInput2[1];

				Shift_Dead = TRUE; //For Shift + CapLock handling

				//user press shift to get the dead key before,
                // I simulate the shift again
				//this should be transparent to the user


				if(save_the_shift)
				{
					// SHIFT down
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = 0;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_SHIFT;
					rgInput2[0].ki.wScan = 0x2A;

					SendInput(1, rgInput2, sizeof(INPUT));
				}

				SendKeysN(Dead_vk, Dead_scancode);


				//v-mjgran: If the RALT is pressed after the dead key
				if (fRALTPrevious)
				{
					//LControl down
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = 0;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_CONTROL;
					rgInput2[0].ki.wScan = 0x1D;

					//Lmenu down
					rgInput2[1].type = INPUT_KEYBOARD;
					rgInput2[1].ki.dwFlags = 0;
					rgInput2[1].ki.dwExtraInfo = 0;
					rgInput2[1].ki.wVk = VK_MENU;
					rgInput2[1].ki.wScan = 0x38;

					SendInput(2, rgInput2, sizeof(INPUT));

//v-mjgran			fRALTPrevious = FALSE;
				}	
				
				//release the shift
				if(save_the_shift)
				{
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = KEYEVENTF_KEYUP;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_SHIFT;
					rgInput2[0].ki.wScan = 0x2A;

					SendInput(1, rgInput2, sizeof(INPUT));
				}

				if (fShiftPrevious)
				{
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = 0;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_SHIFT;
					rgInput2[0].ki.wScan = 0x2A;

					SendInput(1, rgInput2, sizeof(INPUT));
//v-mjgran			fShiftPrevious = FALSE;

				}

				SendKeysN(vk, KBkey[index].scancode[0]);

				//v-mjgran: If the RALT is pressed after the dead key
				if (fRALTPrevious)
				{
					//LControl down
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = KEYEVENTF_KEYUP;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_CONTROL;
					rgInput2[0].ki.wScan = 0x1D;

					//Lmenu down
					rgInput2[1].type = INPUT_KEYBOARD;
					rgInput2[1].ki.dwFlags = KEYEVENTF_KEYUP;
					rgInput2[1].ki.dwExtraInfo = 0;
					rgInput2[1].ki.wVk = VK_MENU;
					rgInput2[1].ki.wScan = 0x38;

					SendInput(2, rgInput2, sizeof(INPUT));

					fRALTPrevious = FALSE;
				}

				if (fShiftPrevious)
				{
					rgInput2[0].type = INPUT_KEYBOARD;
					rgInput2[0].ki.dwFlags = KEYEVENTF_KEYUP;
					rgInput2[0].ki.dwExtraInfo = 0;
					rgInput2[0].ki.wVk = VK_SHIFT;
					rgInput2[0].ki.wScan = 0x2A;

					SendInput(1, rgInput2, sizeof(INPUT));
					fShiftPrevious = FALSE;

				}

				//Reset these vars
				DeadKey = FALSE;
				Dead_vk = 0;
				Dead_scancode = 0;
				save_the_shift = FALSE;


				//Make click sound
				if(Prefusesound)
					MakeClick(SND_UP);


				//Return the dead key color
				//Only do it in NT because in win95 will eat my dead key
                // from the message queue because it call ToAscii
				if(platform == VER_PLATFORM_WIN32_NT)
				{
				
                	SetWindowLong(DeadHwnd, 0, 0);
					SetClassLongPtr(DeadHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
					InvalidateRect(DeadHwnd, NULL, TRUE);
					DeadHwnd = NULL;
				}	


			}


            //We have a dead key, just save the info. Not send keystroke
			
			//else if (!Menu && IsDeadKey(vk, KBkey[index].scancode[0]))		//v-mjgran
			//else if (DeadKey=IsDeadKey(vk, KBkey[index].scancode[0]))
			else if (!Menu && fCurrentDeadKey)		//v-mjgran
            {
				//DeadKey=IsDeadKey(vk, KBkey[index].scancode[0]);		 //v-mjgran
				DeadKey = fCurrentDeadKey;

				Dead_vk = vk;
				Dead_scancode = KBkey[index].scancode[0];
				save_the_shift = Shift;

				//Change the dead key color to let it look like stay down
				//Only do it in NT because in win95 will eat my dead key
                // from the message queue because it call ToAscii

				if(platform == VER_PLATFORM_WIN32_NT)
				{	SetWindowLong(ldhwnd, 0, 4);
					DeleteObject((HGDIOBJ)SetClassLongPtr(ldhwnd,
                                 GCLP_HBRBACKGROUND,
                                 (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
					DeadHwnd = ldhwnd;
					InvalidateRect(DeadHwnd, NULL, TRUE);
				}

			}
			
			else     //not a dead key, just normal key
			{	
				INPUT   rgInput[2];

				// MessageBox(NULL, NULL, NULL, MB_OKCANCEL);

				//BUGBUG: MapVirtualKey returns 0 for 'Break' key. Special case for 'Break'.
				if(!vk && KBkey[index].scancode[0] == 0xE1)
				{
					if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
						SendKeysN(3, KBkey[index].scancode[2]);
					else
						SendKeysN(19, KBkey[index].scancode[0]);
				}
				else
				{
					SendKeysN(vk, KBkey[index].scancode[0]);
				}

				//Make click sound
				if(Prefusesound)
					MakeClick(SND_UP);
				
				//Re-paint the key. Cannot do it in DoButtonDown() because
                // in dwell mode it will eat up my dead key
				//This bug only happen in NT not win95

				if(PrefDwellinkey && (platform == VER_PLATFORM_WIN32_NT))
					InvalidateRect(ldhwnd, NULL, TRUE);
			}

		}


		if (Shift)   //If previous SHIFT is down, now release it.
		{	
			INPUT	rgInput[1];

			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_SHIFT;
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];
					
			SendInput(1, rgInput, sizeof(INPUT));

			
			Shift=FALSE;

			if(!kbfCapLock)
				kbfCapLetter = !kbfCapLetter;

			//Return the shift key color
			SetWindowLong(ShiftHwnd, 0, 0);
			SetClassLongPtr(ShiftHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(ShiftHwnd,  GWLP_USERDATA,0);
			ShiftHwnd = NULL;

			//Re-draw the keys
            RedrawKeys();
		}
		
		if (Menu)   //If previous MENU is down, now release it.
		{
			/* v-mjgran: The key is a normal key, the name can not be  KB_ALT
			   if(KBkey[index].name==KB_LALT)    //left alt key
			*/

			if(AltHwnd == lpkeyhwnd[97])		//lalt
			{
				if((WORD) KBkey[index].scancode[0] != 0x0f)		//TAB
				{

					INPUT	rgInput[1];

					rgInput[0].type = INPUT_KEYBOARD;
					rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
					rgInput[0].ki.dwExtraInfo = 0;
					rgInput[0].ki.wVk = VK_MENU;
					rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];
						
					SendInput(1, rgInput, sizeof(INPUT));

				
					//Return the alt key color
					SetWindowLong(AltHwnd, 0, 0);
					SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
					SetWindowLongPtr(AltHwnd,  GWLP_USERDATA,0);
					InvalidateRect(AltHwnd, NULL, TRUE);
					AltHwnd = NULL;

					Menu=FALSE;		//v-mjgran
				}
			}
			else        //right alt (AltGr) key. I convert internally to CONTROL and MENU
			{	
				INPUT	rgInput[2];

				rgInput[0].type = INPUT_KEYBOARD;
				rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
				rgInput[0].ki.dwExtraInfo = 0;
				rgInput[0].ki.wVk = VK_CONTROL;
				rgInput[0].ki.wScan = 0x1D;

				rgInput[1].type = INPUT_KEYBOARD;
				rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
				rgInput[1].ki.dwExtraInfo = 0;
				rgInput[1].ki.wVk = VK_MENU;
				rgInput[1].ki.wScan = 0x38;
				
				SendInput(2, rgInput, sizeof(INPUT));


				
				RALT = FALSE;

				//Return the alt key color
				SetWindowLong(AltHwnd, 0, 0);
				SetClassLongPtr(AltHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
				SetWindowLongPtr(AltHwnd,  GWLP_USERDATA,0);
				AltHwnd = NULL;
				
                //Redraw the keys
                RedrawKeys();

				Menu=FALSE;		//v-mjgran
			}
		}
		
		if (Control)   //Previouslly press Control, now release it.
		{
			INPUT	rgInput[1];

			rgInput[0].type = INPUT_KEYBOARD;
			rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
			rgInput[0].ki.dwExtraInfo = 0;
			rgInput[0].ki.wVk = VK_CONTROL;
			rgInput[0].ki.wScan = (WORD) KBkey[index].scancode[0];


			SendInput(1, rgInput, sizeof(INPUT));


			Control=FALSE;
			
			//change back to its normal state (key up)
			SetWindowLong(CtrlHwnd, 0, 0);
			SetClassLongPtr(CtrlHwnd, GCLP_HBRBACKGROUND, COLOR_MENU);
			SetWindowLongPtr(CtrlHwnd,  GWLP_USERDATA,0);
			InvalidateRect(CtrlHwnd, NULL, TRUE);
			CtrlHwnd = NULL;


			//Re-draw the keys
            RedrawKeys();

		}

	}      //end switch
}


/**************************************************************************/
BOOL IsDeadKey(UINT vk, UINT scancode)
{
    BYTE	KeyState[256]="";
	TCHAR	buf[50]=TEXT("");
	UINT	wFlag=0;
	int     iRet;

	if(Shift || (LOBYTE(GetKeyState(VK_CAPITAL))== 1))
		KeyState[VK_SHIFT] = 0x80;

#ifdef UNICODE
	iRet = ToUnicodeEx(vk, scancode, &KeyState[0], buf, 50, wFlag, hkl);
#else
	iRet = ToAsciiEx(vk, scancode, &KeyState[0], (LPWORD)buf, wFlag, hkl);
#endif

	//ToAscii return 2 in win95, < 0 in NT when dead key pass to it
	if(iRet == 2 || iRet < 0)
		return TRUE;
	else
		return FALSE;
}

/**************************************************************************/
//Send key up for Alt and Ctrl after user press Ctrl+Alt+Del
//For some reason, after user press Ctrl+Alt+Del from osk, the Alt and Ctrl
//keys still down.
/**************************************************************************/
void ReleaseAltCtrlKeys(void)
{	INPUT	rgInput[2];
				
	//Menu UP
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = VK_MENU;
	rgInput[0].ki.wScan = 0x38;

				
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = VK_CONTROL;
	rgInput[1].ki.wScan = 0x1D;

	SendInput(2, rgInput, sizeof(INPUT));

	bCtrl_Alt_Del = FALSE;

}
/**************************************************************************/

void SendKeys(HWND ldhwnd, UINT vk, UINT scanCode)
{
	INPUT rgInput[2];

	// MessageBox(NULL, NULL, NULL, MB_OK);
	// Here we need to be careful, The Arrow keys should always work as arrow keys!
	// Patch from a-anilk. I really don't understand, Why so much of code is required.
	// Some day, I would like to see this function be in 10 lines.
	//extend key down
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = (WORD) vk;
	// rgInput[0].ki.wScan = (WORD) scanCode;
		
	//extend key up
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = (WORD) vk;
	// rgInput[1].ki.wScan = (WORD) scanCode;

	SendInput(2, rgInput, sizeof(INPUT));


	//Make click sound
	if(Prefusesound)
	MakeClick(SND_UP);

	InvalidateRect(ldhwnd, NULL, TRUE);     //Redraw the key (in case it is in Dwell)
}

void SendKeysN(UINT vk, UINT scanCode)
{
	INPUT rgInput[2];

	// I really don't understand, Why so much of code is required.
	// Some day, We could reduce 90%
	//extend key down
	rgInput[0].type = INPUT_KEYBOARD;
	rgInput[0].ki.dwFlags = 0;
	rgInput[0].ki.dwExtraInfo = 0;
	rgInput[0].ki.wVk = (WORD) vk;
	rgInput[0].ki.wScan = (WORD) scanCode;
		
	//extend key up
	rgInput[1].type = INPUT_KEYBOARD;
	rgInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
	rgInput[1].ki.dwExtraInfo = 0;
	rgInput[1].ki.wVk = (WORD) vk;
	rgInput[1].ki.wScan = (WORD) scanCode;

	SendInput(2, rgInput, sizeof(INPUT));
}

BOOL IsDeadKeyPressed(HWND hScannedKey)
{
	return (hScannedKey!= NULL &&
		    (hScannedKey == CtrlHwnd || hScannedKey == AltHwnd || 
		     hScannedKey == ShiftHwnd || hScannedKey == CapHwnd));
}


// Sending SAS...
// DWORD StartUtilManService(LPVOID);

BOOL SendSAS()
{
	HWND hWnd = NULL;
	HANDLE hThread = NULL;
	DWORD threadID = 0;
	
	hWnd = FindWindow(NULL, TEXT("SAS window"));

	// For now limit SAS to logon for security reasons
	/*
	if ( !hWnd )
	{
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartUtilManService, NULL, 0, &threadID);
	}
	else */
	if ( hWnd )
	{
		PostMessage(hWnd, WM_HOTKEY, 0, (LPARAM) 2e0003);
		return TRUE;
	}

	return FALSE;
}

// AK
// #define UM_SERVICE_CONTROL_SAS 130 : for SAS from user desktop

/*DWORD StartUtilManService(LPVOID inParam)
{
    SC_HANDLE hSCMan, hService;
    SERVICE_STATUS serStat, ssStatus;
    int waitCount = 0;

    hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	
    if ( hSCMan )
    {
        hService = OpenService( hSCMan, TEXT("UtilMan"), SERVICE_ALL_ACCESS);
		
        if ( hService )
        {
            QueryServiceStatus(hService, &ssStatus);
			
            if (ssStatus.dwCurrentState != SERVICE_RUNNING)
			{ 
				// If UM is not running Then start it
                LPCTSTR args[3];
				// args[0] = UTILMAN_START_BYHOTKEY;
				args[1] = 0;
				
                StartService(hService, 0, NULL);
				
				
                // We need a WAIT here
                while(QueryServiceStatus(hService, &ssStatus) )
                {
                    if ( ssStatus.dwCurrentState == SERVICE_RUNNING)
                        break;
					
                    Sleep(500);
                    waitCount++;
                    
                    // We cannot afford to wait for more than 2-3 sec
                    if (waitCount > 10)
                        break;
                }
            }
			
            if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
                ControlService(hService, UM_SERVICE_CONTROL_SAS, &serStat);
			
            CloseServiceHandle(hService);
        } 

		else
		{
			// If its Access Denied. Say so. 
		}
		
        CloseServiceHandle(hSCMan);
    }
	return 0;
}

*/
