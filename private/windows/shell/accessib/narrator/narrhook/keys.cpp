/*************************************************************************
	Project:    Narrator
    Module:     keys.cpp

    Author:     Paul Blenkhorn
    Date:       April 1997
    
    Notes:      Credit to be given to MSAA team - bits of code have been 
				lifted from:
					Babble, Inspect, and Snapshot.

    Copyright (C) 1997-1998 by Microsoft Corporation.  All rights reserved.
    See bottom of file for disclaimer
    
    History: Add features, Bug fixes : 1999 Anil Kumar
*************************************************************************/
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <ole2ver.h>
#include <commctrl.h>
#include "commdlg.h"
#include <string.h>
#include <initguid.h>
#include <oleacc.h>
#include <TCHAR.H>
#include "..\Narrator\Narrator.h"
#include "keys.h"
#include "getprop.h"
#include "..\Narrator\resource.h"
#include "resource.h"
// #include "sharemem.h"

#include "list.h"       // include list.h before helpthd.h, GINFO needs CList
#include "HelpThd.h"
#include <stdio.h>
#define ARRAYSIZE(x)   (sizeof(x) / sizeof(*x))

// ROBSI: 99-10-09
#define MAX_NAME 4196 // 4K (beyond Max of MSAA)
#define MAX_VALUE 512

// When building with VC5, we need winable.h since the active
// accessibility structures are not in VC5's winuser.h.  winable.h can
// be found in the active accessibility SDK
#ifdef VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT
#include <winable.h>
#endif

#define STATE_MASK (STATE_SYSTEM_CHECKED | STATE_SYSTEM_MIXED | STATE_SYSTEM_READONLY | STATE_SYSTEM_BUSY | STATE_SYSTEM_MARQUEED | STATE_SYSTEM_ANIMATED | STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE)

	  
// Local functions
void Navigate(BOOL fForward);
void DoDirection(int navDir);
void Home(int x);
void MoveToEnd(int x);
void SpeakKeyboard(int nOption);
void SpeakWindow(int nOption);
void SpeakRepeat(int nOption);
void SpeakItem(int nOption);
void SpeakMainItems(int nOption);
void SpeakMute(int nOption);
void GetObjectProperty(IAccessible*, long, int, LPTSTR, UINT);
long AddAccessibleObjects(IAccessible*, VARIANT*);
BOOL AddItem(IAccessible*, VARIANT*);
void SpeakObjectInfo(LPOBJINFO poiObj, BOOL SpeakExtra);

// MSAA event handlers
BOOL OnFocusChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);
BOOL OnValueChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);
BOOL OnSelectChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                          DWORD dwmsTimeStamp);
BOOL OnLocationChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                            LONG idChild, DWORD dwmsTimeStamp);
BOOL OnStateChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);

// More local routines
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild );
void FilterGUID(TCHAR* szSpeak); 

// For debugging
// void FileWrite(LPTSTR pszTextLocal);


// Hot keys
// All Ctrl + arrow keys are removed as 
// they clash with pre-defined hotkeys
HOTK rgHotKeys[] =
{   //Key       SHIFT					Function            Parameter
    { VK_F12,     MSR_CTRL | MSR_SHIFT,	SpeakKeyboard,          0},  
    { VK_SPACE, MSR_CTRL | MSR_SHIFT,   SpeakWindow,            1},  
    { VK_RETURN,MSR_CTRL | MSR_SHIFT,	SpeakMainItems,         0},  
    { VK_INSERT,MSR_CTRL | MSR_SHIFT,   SpeakItem,              0},  
    // { VK_UP,    MSR_CTRL | MSR_SHIFT,   DoDirection,            MSR_KEYUP},  
    // { VK_DOWN,  MSR_CTRL | MSR_SHIFT,   DoDirection,		    MSR_KEYDOWN}, 
	// { VK_LEFT,  MSR_CTRL | MSR_SHIFT,   DoDirection,			MSR_KEYLEFT},  
	// { VK_RIGHT, MSR_CTRL | MSR_SHIFT,   DoDirection,			MSR_KEYRIGHT},
    { VK_HOME,	MSR_ALT,				Home,					0},  
    { VK_END,	MSR_ALT,				MoveToEnd,				0},  
    // {  VK_UP,    MSR_ALT,				Navigate,               0},
    // { VK_DOWN,  MSR_ALT,				Navigate,               1},
    // { VK_LEFT,  MSR_ALT,			    Navigate,               0},
    // { VK_RIGHT, MSR_ALT,				Navigate,               1}
};

// a-anilk: this is better than defining as a constant - you don't have to worry
// about making the table and the count match up.
#define CKEYS_HOT (sizeof(rgHotKeys)/sizeof(HOTK))


#define MSR_DONOWT   0
#define MSR_DOCHAR   1
#define MSR_DOWORD	 2
#define MSR_DOLINE	 3
#define MSR_DOLINED  4
#define MSR_DOCHARR  5
#define MSR_DOWORDR  6


#define MSR_DOOBJECT 4
#define MSR_DOWINDOW 5
#define MAX_TEXT_ROLE 128

// Global Variables
//#pragma data_seg(".sdata")
// JMC: We should never name a datasegment '.sdata' since that is reserved
// for special use by the Alpha compiler.  Instead, we use 'Shared'
// note that any variable in the shared data segment MUST be initialized
// when they are declared, or else they won't end up in the shared segment,
// and you won't get a warning or anything.

#pragma data_seg("Shared")

int g_AutoRead = FALSE; // Did we get to ReadWindow through focus change or CTRL_ALT_SPACE flag
int g_SpeakWindowSoon = FALSE; // Flag to indicate the we have a new window ... speak it when sensible

int g_LeftHandSide = 0; // Store left hand side of HTML window we want to read
int g_doingPassword = FALSE;
int g_ReadLast = MSR_DONOWT; // store last speech event
int g_MsrDoNext = MSR_DONOWT; // keyboard flag set when curor keys are used ... let us know what to read when caret has moved

HWND    g_hwndMSR = NULL;
HHOOK   g_hhookKey = NULL;
HHOOK   g_hhookMouse = NULL;
HWINEVENTHOOK   g_hEventHook;
// These variables must be initialized - the registry stuff depends on that fact.
__declspec (dllexport) TCHAR CurrentText[MAX_TEXT] = TEXT("");
__declspec (dllexport) int TrackSecondary = TRUE;
__declspec (dllexport) int TrackCaret = TRUE;
__declspec (dllexport) int TrackInputFocus = FALSE;
__declspec (dllexport) int EchoChars = MSR_ECHOALNUM | MSR_ECHOSPACE | MSR_ECHODELETE | MSR_ECHOMODIFIERS | MSR_ECHOENTER | MSR_ECHOBACK | MSR_ECHOTAB;
__declspec (dllexport) int AnnounceWindow = TRUE;
__declspec (dllexport) int AnnounceMenu = TRUE;
__declspec (dllexport) int AnnouncePopup = TRUE;
__declspec (dllexport) int AnnounceToolTips = FALSE; // this ain't working properly - taken out!
__declspec (dllexport) int ReviewStyle = TRUE;
__declspec (dllexport) int ReviewLevel = 0;

// Global variables to control events and speech
BOOL    g_bInternetExplorer = FALSE;
BOOL    g_bHTML_Help = FALSE;
UINT    g_MSG_MSR_Cursor = 0;
POINT   g_ptCurrentMouse = { -1,-1};
BOOL    g_bMouseUp = TRUE;			// flag for mouse up/down
HWND    g_HelpWnd = NULL;
BOOL    g_JustHadSpace = 0;
BOOL    g_JustHadShiftKeys = 0;
BOOL	g_bListFocus = FALSE;		// To avoid double speaking of list items...
BOOL	g_bStartPressed = FALSE;
LONG    cProcessesMinus1 = -1;      // number of attached processes minus 1 cuz the Interlocked APIs suck
TCHAR   pszTextLocal[2000] = { 0 }; // PB: 22 Nov 1998.  Make it work!!! Make this Global and Shared!
#pragma data_seg()
#pragma comment(linker, "-section:Shared,rws") 

// This has to be outside the shared data segment because it is init'ed with 
// each call to DllMain

HINSTANCE g_Hinst = NULL;
DWORD	  g_tidMain=0;	// ROBSI: 10-10-99

// These are class names, This could change from one OS to another and in 
// different OS releases.I have grouped them here : Anil.
// These names may have to changed for Win9x and other releases
#define CLASS_WINSWITCH		TEXT("#32771")  // This is WinSwitch class. Disguises itself :-)AK
#define CLASS_HTMLHELP_IE	TEXT("HTML_Internet Explorer")
#define CLASS_IE_FRAME		TEXT("IEFrame")
#define CLASS_IE_MAINWND	TEXT("Internet Explorer_Server")
#define CLASS_LISTVIEW		TEXT("SysListView32")
#define CLASS_HTMLHELP		TEXT("HH Parent")
#define CLASS_TOOLBAR		TEXT("ToolbarWindow32")



/*************************************************************************
    Function:   SpeakString
    Purpose:    Send speak string message back to original application
    Inputs:     TCHAR *str
    Returns:    void
    History:
*************************************************************************/
void SpeakString(TCHAR * str)
{
    lstrcpyn(CurrentText,str,MAX_TEXT);

    // Need to sendmessage as otherwise other messages could fire and overwrite
    // this one.
    SendMessage(g_hwndMSR, WM_MSRSPEAK, (WPARAM) CurrentText, 0);
}

/*************************************************************************
    Function:   SpeakStr
    Purpose:    Send speak string message back to original application
    Inputs:     TCHAR *str
    Returns:    void
    History:
    
      This one uses Postmessage to make focus change work for ALT-TAB???????

*************************************************************************/
void SpeakStr(TCHAR * str)
{
    lstrcpyn(CurrentText,str,MAX_TEXT);

    PostMessage(g_hwndMSR, WM_MSRSPEAK, (WPARAM) CurrentText, 0);
}


/*************************************************************************
    Function:   SpeakStringAll
    Purpose:    Speak the string, but put out a space first to make sure the
                string is fresh - i.e. stop duplicate string pruning from 
                occuring
    Inputs:     TCHAR *str
    Returns:    void
    History:
*************************************************************************/
void SpeakStringAll(TCHAR * str)
{
    SpeakString(TEXT(" ")); // stop speech filter losing duplicates
    SpeakString(str);
}

/*************************************************************************
    Function:   SpeakStringId
    Purpose:    Speak a string loaded as a resource ID
    Inputs:     UINT id
    Returns:    void
    History:
*************************************************************************/
void SpeakStringId(UINT id)
{
	if (LoadString(g_Hinst, id, CurrentText, 256) == 0)
	{
		DBPRINTF (TEXT("LoadString failed on hinst %lX id %u\r\n"),g_Hinst,id);
		SpeakString(TEXT("TEXT NOT FOUND!"));
	}
	else 
    {
		DBPRINTF (TEXT("LoadString succeeded on hinst %lX id %u\r\n"),g_Hinst,id);
		SendMessage(g_hwndMSR, WM_MSRSPEAK, (WPARAM) CurrentText, 0);
        SpeakString(TEXT(" ")); // stop speech filter losing duplicates

		// SendMessage(g_hwndMSR, WM_MSRSPEAK, (WPARAM) CurrentText, 0);
	}
}


/*************************************************************************
    Function:   SetSecondary
    Purpose:    Set secondary focus position & posibly move mouse pointer
    Inputs:     Position: x & y
				Whether to move cursor: MoveCursor
    Returns:    void
    History:
*************************************************************************/
void SetSecondary(long x, long y, int MoveCursor)
{
	g_ptCurrentMouse.x = x;
	g_ptCurrentMouse.y = y;
	if (MoveCursor)
	{
		// Check if co-ordinates are valid, At many places causes 
		// the cursor to vanish...
		if ( x > 0 && y > 0 )
			SetCursorPos(x,y);
	}

	// Tell everyone where the cursor is.
	// g_MSG_MSR_Cursor set using RegisterWindowMessage below in InitMSAA
	SendMessage(HWND_BROADCAST,g_MSG_MSR_Cursor,x,y);
}


/*************************************************************************
    Function:   BackToApplication
    Purpose:    Set the focus back to the application that we came from with F12
    Inputs:     void
    Returns:    void
    History:
*************************************************************************/
void BackToApplication(void)
{
	SetForegroundWindow(g_HelpWnd);
}


/*************************************************************************
    Function:   InitKeys
    Purpose:    Set up processing for global hot keys
    Inputs:     HWND hwnd
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL InitKeys(HWND hwnd)
{
    HMODULE hModSelf;

    // If someone else has a hook installed, fail.
    if (g_hwndMSR)
        return(FALSE);

    // Save off the hwnd to send messages to
    g_hwndMSR = hwnd;
    // Get the module handle for this DLL
    hModSelf = GetModuleHandle(TEXT("NarrHook.dll"));

    if(!hModSelf)
        return(FALSE);
    
    // Set up the global keyboard hook
    g_hhookKey = SetWindowsHookEx(WH_KEYBOARD, // What kind of hook
                                KeyboardProc,// Proc to send to
                                hModSelf,    // Our Module
                                0);          // For all threads

    // and set up the global mouse hook
    g_hhookMouse = SetWindowsHookEx(WH_MOUSE,  // What kind of hook
                                  MouseProc, // Proc to send to
                                  hModSelf,  // Our Module
                                  0);        // For all threads

    // Return TRUE|FALSE based on result
    return(g_hhookKey != NULL &&
           g_hhookMouse != NULL);
}


/*************************************************************************
    Function:   UninitKeys
    Purpose:    Deinstall the hooks
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL UninitKeys(void)
{
    // Reset
    g_hwndMSR = NULL;

    // Check to see that the hook was installed
    if (g_hhookKey)
    {
        // It was, unhook it
        UnhookWindowsHookEx(g_hhookKey);

        // Reinit the variable
        g_hhookKey = NULL;

// ROBSI: 10-10-99
// The nesting here is incorrect. The mouse hook is not dependent upon
// the keyboard hook (see InitKeys above), so this needs to be restructured.
        if (g_hhookMouse) 
        {
			UnhookWindowsHookEx(g_hhookMouse);
			g_hhookMouse = NULL;
        }
        return(TRUE);
     }

    // The hook wasn't installed, weird - let's exit weird    
    return(FALSE);
}


/*************************************************************************
    Function:   KeyboardProc
    Purpose:    Gets called for keys hit
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
LRESULT CALLBACK KeyboardProc(int code,	        // hook code
                              WPARAM wParam,    // virtual-key code
                              LPARAM lParam)    // keystroke-message information
{
    int		state = 0;
    int		ihotk;
    
    if (code == HC_ACTION)
    {
        // If this is a key up, bail out now.
        if (!(lParam & 0x80000000))
        {
            g_bMouseUp = TRUE;
            g_SpeakWindowSoon = FALSE;
            g_JustHadSpace = FALSE;
            if (lParam & 0x20000000) 
            { // get ALT state
                state = MSR_ALT;
                SpeakMute(0);
            }
            
            if (GetKeyState(VK_SHIFT) < 0)
                state |= MSR_SHIFT;
            
            if (GetKeyState(VK_CONTROL) < 0)
                state |= MSR_CTRL;
            
            for (ihotk = 0; ihotk < CKEYS_HOT; ihotk++)
            {
                if ((rgHotKeys[ihotk].keyVal == wParam) && 
                    (state == rgHotKeys[ihotk].status))
                {
                    // Call the function
                    SpeakMute(0);
                    (*rgHotKeys[ihotk].lFunction)(rgHotKeys[ihotk].nOption);
                    return(1);
                }
            }


// ROBSI: 10-11-99 -- Work Item: Should be able to use the code in OnFocusChangedEvent
//								 that sets the g_doingPassword flag, but that means 
//								 changing the handling of StateChangeEvents to prevent
//								 calling OnFocusChangedEvent. For now, we'll just use
//								 call GetGUIThreadInfo to determine the focused window
//								 and then rely on OLEACC to tell us if it is a PWD field.
			// ROBSI <begin>
			HWND			hwndFocus = NULL;
			GUITHREADINFO	gui;

			// Use the foreground thread.  If nobody is the foreground, nobody has
			// the focus either.
			gui.cbSize = sizeof(GUITHREADINFO);
			if ( GetGUIThreadInfo(0, &gui) )
			{
				hwndFocus = gui.hwndFocus;
			}

			if (hwndFocus != NULL) 
			{
				// Use OLEACC to detect password fields. It turns out to be more 
				// reliable than SendMessage(GetFocus(), EM_GETPASSWORDCHAR, 0, 0L).
				IAccessible	*pIAcc = NULL;
				HRESULT hr = AccessibleObjectFromWindow(hwndFocus, OBJID_CLIENT, 
														IID_IAccessible, (void**)&pIAcc);
				if (S_OK == hr)
				{
					// Test the password bit...
					VARIANT varState;
					VARIANT	varSelf;
					VariantInit(&varState); 
					VariantInit(&varSelf); 
					varSelf.vt = VT_I4;
					varSelf.lVal = CHILDID_SELF;

					hr = pIAcc->get_accState(varSelf, &varState);

					if ((S_OK == hr) && (varState.lVal & STATE_SYSTEM_PROTECTED))
					{
						g_doingPassword = TRUE;
					}
				}

				// ROBSI: OLEACC does not always properly detect password fields.
				// Therefore, we use Win32 as a backup.
				if (!g_doingPassword)
				{
					TCHAR   szClassName[256];

					// Verify this control is an Edit or RichEdit control to avoid 
					// sending EM_ messages to random controls.
					// POTENTIAL BUG? If login dialog changes to another class, we'll break.
					if ( RealGetWindowClass( hwndFocus, szClassName, ARRAYSIZE(szClassName)) )
					{
						if ((0 == lstrcmp(szClassName, TEXT("Edit")))		||
							(0 == lstrcmp(szClassName, TEXT("RICHEDIT")))	||
							(0 == lstrcmp(szClassName, TEXT("RichEdit20A")))	||
							(0 == lstrcmp(szClassName, TEXT("RichEdit20W"))) 
						   )
						{
							g_doingPassword = (SendMessage(hwndFocus, EM_GETPASSWORDCHAR, 0, 0L) != NULL);
						}
					}
				}
				
				pIAcc->Release();
			}
			// ROBSI <end>

			if (g_doingPassword)
			{
				// ROBSI: 10-11-99
				// Go ahead and speak keys that are not printable but will
				// help the user understand what state they are in.
				switch (wParam)
				{
					case VK_CAPITAL:
						if (EchoChars & MSR_ECHOMODIFIERS)
						{
							SpeakMute(0);
							if ( GetKeyState(VK_CAPITAL) & 0x0F )
								SpeakStringId(IDS_CAPS_ON);
							else
								SpeakStringId(IDS_CAPS_OFF);
						}
						break;

					case VK_NUMLOCK:
						if (EchoChars & MSR_ECHOMODIFIERS)
						{
							SpeakMute(0);
							if ( GetKeyState(VK_NUMLOCK) & 0x0F )
								SpeakStringId(IDS_NUM_ON);
							else
								SpeakStringId(IDS_NUM_OFF);
						}
						break;

					case VK_DELETE:
						if (EchoChars & MSR_ECHODELETE)
						{
							SpeakMute(0);
							SpeakStringId(IDS_DELETE);
						}
						break;

					case VK_INSERT:
						if (EchoChars & MSR_ECHODELETE)
						{
							SpeakMute(0);
							SpeakStringId(IDS_INSERT);
						}
						break;

					case VK_BACK:
						if (EchoChars & MSR_ECHOBACK)
						{
							SpeakMute(0);
							SpeakStringId(IDS_BACKSPACE);
						}
						break;

					case VK_TAB:
						SpeakMute(0);

						if (EchoChars & MSR_ECHOTAB)
							SpeakStringId(IDS_TAB);
						break;

					case VK_CONTROL:
						SpeakMute(0); // always mute when control held down!

						if ((EchoChars & MSR_ECHOMODIFIERS) && !(g_JustHadShiftKeys & MSR_CTRL))
						{
							SpeakStringId(IDS_CONTROL);
							// ROBSI: Commenting out to avoid modifying Global State
							// g_JustHadShiftKeys |= MSR_CTRL;  
						}
						break;

					default:
						SpeakMute(0);
						SpeakStringId(IDS_PASS);
						break;
				}

			    return (CallNextHookEx(g_hhookKey, code, wParam, lParam));
			}


            TCHAR buff[20];
            BYTE KeyState[256];
            UINT ScanCode;
            GetKeyboardState(KeyState);
            
            if ((EchoChars & MSR_ECHOALNUM) && 
                (ScanCode = MapVirtualKeyEx((UINT)wParam, 2,GetKeyboardLayout(0)))) 
            {
#ifdef UNICODE
                ToUnicode((UINT)wParam,ScanCode,KeyState, buff,10,0);
#else
                ToAscii((UINT)wParam,ScanCode,KeyState,(unsigned short *)buff,0);
#endif
                
                // Use 'GetStringTypeEx()' instead of _istprint()
                buff[1] = 0;
                WORD wCharType;
                WORD fPrintable = C1_UPPER|C1_LOWER|C1_DIGIT|C1_SPACE|C1_PUNCT|C1_BLANK|C1_XDIGIT|C1_ALPHA;
                
                GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, buff, 1, &wCharType);
                if (wCharType & fPrintable)
				{
					SpeakMute(0);
					SpeakStringAll(buff);
				}
            }

			// All new: Add speech for all keys...AK
            switch (wParam) {
            case VK_SPACE:
                g_JustHadSpace = TRUE;
                if (EchoChars & MSR_ECHOSPACE)
				{
					SpeakMute(0);
					SpeakStringId(IDS_SPACE);
				}
                break;

			case VK_LWIN:
			case VK_RWIN:
                if (EchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					g_bStartPressed = TRUE;
                    SpeakStringId(IDS_WINKEY);
				}
				break;

			case VK_CAPITAL:
                if (EchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					if ( GetKeyState(VK_CAPITAL) & 0x0F )
						SpeakStringId(IDS_CAPS_ON);
					else
						SpeakStringId(IDS_CAPS_OFF);
				}
				break;

			case VK_SNAPSHOT:
                if (EchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					SpeakStringId(IDS_PRINT);
				}
				break;

			case VK_ESCAPE:
                if (EchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					SpeakStringId(IDS_ESC);
				}
				break;

			case VK_NUMLOCK:
                if (EchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					if ( GetKeyState(VK_NUMLOCK) & 0x0F )
						SpeakStringId(IDS_NUM_ON);
					else
						SpeakStringId(IDS_NUM_OFF);
				}
				break;

            case VK_DELETE:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_DELETE);
				}
                break;

			case VK_INSERT:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_INSERT);
				}
				break;

			case VK_HOME:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_HOME);
				}
				break;

			case VK_END:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_END);
				}
				break;

			case VK_PRIOR:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_PAGEUP);
				}
				break;

			case VK_NEXT:
                if (EchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_PAGEDOWN);
				}
				break;

            case VK_BACK:
                if (EchoChars & MSR_ECHOBACK)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_BACKSPACE);
				}
                break;

            case VK_TAB:
                SpeakMute(0);

                if (EchoChars & MSR_ECHOTAB)
                    SpeakStringId(IDS_TAB);
                break;

            case VK_CONTROL:
                SpeakMute(0); // always mute when control held down!

                if ((EchoChars & MSR_ECHOMODIFIERS) && !(g_JustHadShiftKeys & MSR_CTRL))
                {
                    SpeakStringId(IDS_CONTROL);
                    g_JustHadShiftKeys |= MSR_CTRL;
                }
                break;

            case VK_MENU:
                if ((EchoChars & MSR_ECHOMODIFIERS) && !(g_JustHadShiftKeys & MSR_ALT))
				{
					SpeakMute(0);
                    SpeakStringId(IDS_ALT);
				}
                break;

            case VK_SHIFT:
                if ((EchoChars & MSR_ECHOMODIFIERS) && !(g_JustHadShiftKeys & MSR_SHIFT))
				{
					SpeakMute(0);
	                SpeakStringId(IDS_SHIFT);
                    g_JustHadShiftKeys |= MSR_SHIFT;
				}
                break;

            case VK_RETURN:
                if (EchoChars & MSR_ECHOENTER)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_RETURN);
				}
                break;
            }
            
            // set flags for moving around edit controls
            g_MsrDoNext = MSR_DONOWT; 

			if (state == MSR_CTRL && (wParam == VK_LEFT || wParam == VK_RIGHT))
			{
				SpeakMute(0);
				g_MsrDoNext = MSR_DOWORD;
			}
			else if ((state & MSR_CTRL) && (state & MSR_SHIFT) && (wParam == VK_LEFT))
				g_MsrDoNext = MSR_DOWORD;
			else if ((state & MSR_CTRL) && (state & MSR_SHIFT) && (wParam == VK_RIGHT))
				g_MsrDoNext = MSR_DOWORDR;
			else if ((state & MSR_SHIFT) && (wParam == VK_LEFT)) 
				g_MsrDoNext = MSR_DOCHAR;
			else if ((state & MSR_SHIFT) && (wParam == VK_RIGHT)) 
				g_MsrDoNext = MSR_DOCHARR;
			else if ((state & MSR_CTRL) && (wParam == VK_UP || wParam == VK_DOWN))
				g_MsrDoNext = MSR_DOLINE;
			else if ((state & MSR_SHIFT) && (wParam == VK_UP))
				g_MsrDoNext = MSR_DOLINE;
			else if ((state & MSR_SHIFT) && (wParam == VK_DOWN))
				g_MsrDoNext = MSR_DOLINED;
			else if (state == 0) 
			{ // i.e. no shift keys
				switch (wParam) 
				{
					case VK_LEFT: 
					case VK_RIGHT:
						g_MsrDoNext = MSR_DOCHAR;
						SpeakMute(0);
						break;
            
					case VK_DOWN: 
					case VK_UP:
						g_MsrDoNext = MSR_DOLINE;
						SpeakMute(0);
						break;

					case VK_F3:
						if (GetForegroundWindow() == g_hwndMSR) 
						{
							PostMessage(g_hwndMSR, WM_MSRCONFIGURE, 0, 0);
							return(1);
						}
						break;
            
					case VK_F9:
						if (GetForegroundWindow() == g_hwndMSR) 
						{
							PostMessage(g_hwndMSR, WM_MSRQUIT, 0, 0);
							return(1);
						}
						 break;
				} // end switch wParam (keycode)
			} // end if no shift keys pressed
        } // end if key down
    } // end if code == HC_ACTION
    g_JustHadShiftKeys = state;

    return (CallNextHookEx(g_hhookKey, code, wParam, lParam));
}

/*************************************************************************
    Function:   MouseProc
    Purpose:    Gets called for mouse eventshit
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
LRESULT CALLBACK MouseProc(int code,	        // hook code
                              WPARAM wParam,    // virtual-key code
                              LPARAM lParam)    // keystroke-message information
{
    LRESULT retVal = CallNextHookEx(g_hhookMouse, code, wParam, lParam);

    if (code == HC_ACTION)
    {
		switch (wParam) 
        { // want to know if mouse is down
		    case WM_NCLBUTTONDOWN: 
            case WM_LBUTTONDOWN:
    		case WM_NCRBUTTONDOWN: 
            case WM_RBUTTONDOWN:
                // to keep sighted people happy when using mouse shut up 
                // the speech on mouse down
                // SpeakMute(0); 
                // Chnage to PostMessage works for now: a-anilk
                PostMessage(g_hwndMSR, WM_MUTE, 0, 0);
                // If it is then don't move mouse pointer when focus set!
			    g_bMouseUp = FALSE;
			    break;

		    case WM_NCLBUTTONUP: 
            case WM_LBUTTONUP:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONUP:
//			    g_bMouseUp = TRUE; Don't clear flag here - wait until key pressed before enabling auto mouse movemens again
			    break;
		}
    }

    return(retVal);
}


// --------------------------------------------------------------------------
//
//  Entry point:  DllMain()
//
// Some stuff only needs to be done the first time the DLL is loaded, and the
// last time it is unloaded, which is to set up the values for things in the 
// shared data segment, including SharedMemory support.
//
// InterlockedIncrement() and Decrement() return 1 if the result is 
// positive, 0 if  zero, and -1 if negative.  Therefore, the only
// way to use them practically is to start with a counter at -1.  
// Then incrementing from -1 to 0, the first time, will give you
// a unique value of 0.  And decrementing the last time from 0 to -1
// will give you a unique value of -1.
//
// --------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved)
{
    if ( dwReason == DLL_PROCESS_ATTACH )
        g_Hinst = hinst;

    return(TRUE);
}

/*************************************************************************
    Function:   WinEventProc
    Purpose:    Callback function handles events
    Inputs:     HWINEVENTHOOK hEvent - Handle of the instance of the event proc
                DWORD event - Event type constant
                HWND hwndMsg - HWND of window generating event
                LONG idObject - ID of object generating event
                LONG idChild - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns: 
    History:    
*************************************************************************/
void CALLBACK WinEventProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, 
                           LONG idObject, LONG idChild, DWORD idThread, 
                           DWORD dwmsEventTime)
{
    // NOTE: If any more events are handled by ProcessWinEvent, they must be 
    // added to this switch statement.
	// no longer will we get an IAccessible here - the helper thread will
	// get the info from the Stack and get and use the IAccessible there.

    switch (event)
    {
		case EVENT_OBJECT_STATECHANGE:
		case EVENT_OBJECT_VALUECHANGE:
		case EVENT_OBJECT_SELECTION:
		case EVENT_OBJECT_FOCUS:
		case EVENT_OBJECT_LOCATIONCHANGE:
		case EVENT_SYSTEM_MENUSTART:
		case EVENT_SYSTEM_MENUEND:
		case EVENT_SYSTEM_MENUPOPUPSTART:
		case EVENT_SYSTEM_MENUPOPUPEND:
		case EVENT_SYSTEM_SWITCHSTART:
		case EVENT_SYSTEM_FOREGROUND:
			AddEventInfoToStack(event, hwndMsg, idObject, idChild, 
								idThread, dwmsEventTime);
			break;
    } // end switch (event)
}


/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
void ProcessWinEvent(DWORD event, HWND hwndMsg, LONG idObject, LONG 
                     idChild, DWORD idThread,DWORD dwmsEventTime)
{
	TCHAR   szName[256];
	
	// What type of event is coming through?
	// bring secondary focus here: Get from Object inspector
	// bring mouse pointer here if flag set.
	
	if (ReviewLevel != 2)
	{
		switch (event)
		{
			case EVENT_SYSTEM_SWITCHSTART:
				SpeakMute(0);
				SpeakString(TEXT("ALT TAB"));
				break;

			case EVENT_SYSTEM_MENUSTART:
			    break;

		    case EVENT_SYSTEM_MENUEND:
			    SpeakMute(0);
			    if (AnnounceMenu)
				    SpeakStringId(IDS_MENUEND);
			    break;
		    
		    case EVENT_SYSTEM_MENUPOPUPSTART:
			    if (AnnouncePopup)
				{
					SpeakMute(0);
				    SpeakStringId(IDS_POPUP);
				}
			    break;
			    
		    case EVENT_SYSTEM_MENUPOPUPEND:
			    SpeakMute(0);
			    if (AnnouncePopup)
				    SpeakStringId(IDS_POPUPEND);
			    break;
			    
		    case EVENT_OBJECT_STATECHANGE : 
			    // want to catch state changes on spacebar pressed
			    switch (g_JustHadSpace) 
			    { 
				    case 0 : // get out - only do this code if space just been pressed
					    break;
				    case 1 : 
				    case 2 : // ignore the first and second time round!
					    g_JustHadSpace++;
					    break;
				    case 3 : // second time around speak the item
					    OnFocusChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
					    g_JustHadSpace = 0;
					    break;
			    }
				OnStateChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
			    break;
			    
			case EVENT_OBJECT_VALUECHANGE : 
				 OnValueChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				    
			case EVENT_OBJECT_SELECTION : 
				if (GetParent(hwndMsg) == g_hwndMSR) 
				{
					// don't do this for our own or list box throws a wobbler!
					break; 
				}
				
				// this comes in for list items a second time after the focus 
				// changes BUT that gets filtered by the double speak check.
				// What this catches is list item changes when cursor down in 
				// combo boxes!
				// Make it just works for them.
				
				OnSelectChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				
			case EVENT_OBJECT_FOCUS:
				OnFocusChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				
			case EVENT_SYSTEM_FOREGROUND: // Window comes to front - speak its name!
				SpeakMute(0);
				SpeakStringId(IDS_FOREGROUND);
				GetWindowText(hwndMsg, szName, sizeof(szName));
				SpeakString(szName);
				
				if (AnnounceWindow) 
				{
					g_SpeakWindowSoon = TRUE; // read window when next focus set
				}
				
				break;
				
			case EVENT_OBJECT_LOCATIONCHANGE:
				// Only the caret
				if (idObject != OBJID_CARET)
					return;

				OnLocationChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
                break;

		} // end switch (event)
	} // end if review level != 2
	return;
}


/*************************************************************************
    Function:   OnValueChangedEvent
    Purpose:    Receives value events
    Inputs:     DWORD event        - What event are we processing
                HWND  hwnd         - HWND of window generating event
                LONG  idObject     - ID of object generating event
                LONG  idChild      - ID of child generating event (0 if object)
                DWORD idThread     - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnValueChangedEvent(DWORD event, HWND hwnd,  LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    OBJINFO         objCurrent;
	VARIANT         varRole;
    IAccessible*    pIAcc;
    VARIANT         varChild;
	TCHAR szName[200];

    // if we've not had a cursor movement then sack this as it could be 
    // scroll bar chaging or slider moving etc to reflect rapidy moving events

	//if (g_MsrDoNext == MSR_DONOWT)
	//	return(FALSE);
	
	 
    DBPRINTF (TEXT("Value change after cursor\r\n"));


    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
        objCurrent.hwnd = hwnd;
        objCurrent.plObj = (long*)pIAcc;
	    objCurrent.varChild = varChild;
	    
	    VariantInit(&varRole);

	    hr = pIAcc->get_accRole(varChild, &varRole);

		if(varRole.lVal != ROLE_SYSTEM_SPINBUTTON &&  g_MsrDoNext == MSR_DONOWT)
		{
			pIAcc->Release();
			return(FALSE);
		}

		g_MsrDoNext = MSR_DONOWT; // PB 22 Nov 1998 stop this firing mroe than once (probably)

	    if (varRole.vt == VT_I4 && (
		    (varRole.lVal == ROLE_SYSTEM_TEXT && g_MsrDoNext != MSR_DOLINE) ||
 		     //varRole.lVal == ROLE_SYSTEM_SPINBUTTON || // lose this for spin box
		     varRole.lVal == ROLE_SYSTEM_PUSHBUTTON || 
		     varRole.lVal == ROLE_SYSTEM_SCROLLBAR))
        {
			// GetObjectProperty(pIAcc, varChild.lVal, ID_ROLE, szName, sizeof(szName));
		    DBPRINTF (TEXT("Don't Speak <%s>\r\n"), szName);
		    ; // don't speak 'cos it's an edit box (or others) changing value!
//		    SpeakObjectInfo(&objCurrent, FALSE);
        }
        else if (!g_bInternetExplorer) // don't do this for IE .. it speaks edit box too much.
        {
		    DBPRINTF (TEXT("Now Speak!\r\n"));
			SpeakMute(0);
		    SpeakObjectInfo(&objCurrent, FALSE);
	    }
		else
		    DBPRINTF (TEXT("Do nowt!\r\n"));

        pIAcc->Release();
    }

    return(TRUE);
}


/*************************************************************************
    Function:   OnSelectChangedEvent
    Purpose:    Receives selection change events - not from MSR though
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
	Notes:		Maybe change this to only take combo-boxes?
*************************************************************************/
BOOL OnSelectChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                          DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    IAccessible*    pIAcc;
    OBJINFO         objCurrent;
	VARIANT         varRole;
    VARIANT         varChild;

    // if we've not had a cursor style movement then sack this as it could be 
    // scroll bar chaging or slider moving etc to reflect rapidy moving events

    DBPRINTF (TEXT("Select\r\n"));
	// if (g_MsrDoNext == MSR_DONOWT)
	//	return(FALSE);


    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
        objCurrent.hwnd = hwnd;
        objCurrent.plObj = (long*)pIAcc;
	    objCurrent.varChild = varChild;
	    
	    VariantInit(&varRole); // heuristic!
	    hr = pIAcc->get_accRole(varChild, &varRole);

	    if (varRole.vt == VT_I4 &&
		    varRole.lVal == ROLE_SYSTEM_LISTITEM) 
        {
			TCHAR buffer[100];
			GetClassName(hwnd,buffer,100); // Is it sysListView32

            // "Don't mute here ... we lose the previous speech message which will
			// have spoken the list item IF we were cursoring to list item.
			// SpeakMute(0);
		    // don't speak unless it's a listitem
		    // e.g. Current Selection for Joystick from Joystick setup.
		    // this does mean that some list items get spoken twice!:AK
			// if ( lstrcmpi(buffer, CLASS_LISTVIEW) != 0)
			if ( !g_bListFocus )
				SpeakObjectInfo(&objCurrent,FALSE);

			g_bListFocus = FALSE;
	    }
        pIAcc->Release();
    }
	
    return(TRUE);
}

/*************************************************************************
    Function:   OnFocusChangedEvent
    Purpose:    Receives focus events
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnFocusChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                         LONG idChild, DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    TCHAR           szName[256];
	TCHAR           buffer[100];
    IAccessible*    pIAcc;
    VARIANT         varChild;
    RECT            rcCursor;
	VARIANT         varRole;
	VARIANT         varState;
	BOOL			switchWnd = FALSE;

    DBPRINTF (TEXT("Focus change\r\n"));

	hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (FAILED(hr))
		return (FALSE);

	// Check for Bogus events...
	if( !IsFocussedItem(hwnd, pIAcc, varChild) )
	{
		pIAcc->Release();
		return (FALSE);
	}

	// Ignore the first Start pressed events...
	if ( g_bStartPressed )
	{
		g_bStartPressed = FALSE;
		pIAcc->Release();
		return (FALSE);
	}

	// Have we got a password char in this one
	// if so then tell them and get out
	VariantInit(&varState); 
	hr = pIAcc->get_accState(varChild, &varState);
	g_doingPassword = (varState.lVal & STATE_SYSTEM_PROTECTED);

	if (g_doingPassword)
	{
		SpeakStringId(IDS_PASSWORD);
		pIAcc->Release();
		return(FALSE);
	}

	GetClassName(hwnd,buffer,100); // is it Internet Explorer in any of its many forms?
	g_bInternetExplorer = lstrcmpi(buffer, CLASS_HTMLHELP_IE) == 0 ||
					   lstrcmpi(buffer, CLASS_IE_FRAME) == 0 ||
					   lstrcmpi(buffer, CLASS_IE_MAINWND) == 0;
    g_bHTML_Help = FALSE;

	if (lstrcmpi(buffer, CLASS_WINSWITCH) == 0)
		switchWnd = TRUE;

	GetClassName(GetForegroundWindow(),buffer,100);
	if ((lstrcmpi(buffer, CLASS_HTMLHELP) == 0)|| (lstrcmpi(buffer, CLASS_IE_FRAME) == 0) ) { // have we got HTML Help?
		g_bInternetExplorer = TRUE;
		g_bHTML_Help = TRUE;
	}

    // Check to see if we are getting rapid focus changes
    // Consider using the Time stamp and saving away the last object
    
	VariantInit(&varRole); 

    // If the focus is being set to a list, a combo, or a dialog, 
    // don't say anything. We'll say something when the focus gets
    // set to one of the children.

	hr = pIAcc->get_accRole(varChild, &varRole); // heuristic!
	
	// Special casing stuff.. Avoid repeatation for list items...
	// Required to correctly process Auto suggest list boxes.
	// As list items also send SelectionChange : AK
	if (varRole.vt == VT_I4 )
    {
		switch ( varRole.lVal )
		{
			case ROLE_SYSTEM_DIALOG:
				DBPRINTF (TEXT("Get out of focus\r\n"));
				pIAcc->Release();
				return(FALSE); 
				break;

			case ROLE_SYSTEM_TITLEBAR:
				g_bMouseUp = FALSE;
				break;

			case ROLE_SYSTEM_LISTITEM:
				g_bListFocus = TRUE;
				break;

			default:
				break;
		}
	}


	if (idObject == OBJID_WINDOW) 
    {
		SpeakMute(0);
		SpeakStringId(IDS_WINDOW);
		GetWindowText(hwnd, szName, 256);
		SpeakString(szName);
	}
	
	// Actually Right and Bottom are Width and Height : 
    hr = pIAcc->accLocation(&rcCursor.left, &rcCursor.top,
	                        &rcCursor.right, &rcCursor.bottom, varChild);
		
	long xOff = (rcCursor.right<2) ? 0: 2; 
	long yOff = (rcCursor.bottom<2) ? 0: 2;

//	g_ptCurrentMouse.x = rcCursor.left + rcCursor.right/2;
//	g_ptCurrentMouse.y = rcCursor.top+yOff; // rcCursor.bottom/2;

	if (TrackInputFocus && g_bMouseUp) 
    { 
		// Gets into a recursive loop... Discard all furthur focus change events
		// for mouse positioning after processing the first one. :a-anilk
		g_bMouseUp = FALSE;
		
        // mouse to follow if it's not already in rectangle 
        // (e.g manually moving mouse in menu) and mouse button up

		POINT CursorPosition;
		GetCursorPos(&CursorPosition);

		if (CursorPosition.x < rcCursor.left 
			|| CursorPosition.x > (rcCursor.left+rcCursor.right)
			|| CursorPosition.y < rcCursor.top
			|| CursorPosition.y > (rcCursor.top+rcCursor.bottom))
        {
			SetSecondary(rcCursor.left + rcCursor.right/2,rcCursor.top+(rcCursor.bottom/2),
                         TRUE);
        }
	}
	else
		SetSecondary(rcCursor.left + rcCursor.right/2,rcCursor.top+(rcCursor.bottom/2),FALSE);

	OBJINFO objCurrent;

	objCurrent.hwnd = hwnd;
	objCurrent.plObj = (long*)pIAcc;
	objCurrent.varChild = varChild;
	
	// If the event is from the switch window, 
	// Then mute the current speech before proceeding...AK
	if ( switchWnd && g_bListFocus )
		SpeakMute(0);

	SpeakObjectInfo(&objCurrent,TRUE);

	if (g_SpeakWindowSoon) 
    {   
		SpeakWindow(0);
		g_SpeakWindowSoon = FALSE;
	}
    pIAcc->Release();
    return(TRUE);
}


/*************************************************************************
    Function:   OnStateChangedEvent
    Purpose:    Receives focus events
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnStateChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                         LONG idChild, DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    IAccessible*    pIAcc;
    VARIANT         varChild;
	VARIANT         varRole;

    DBPRINTF (TEXT("State change\r\n"));

	hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (FAILED(hr))
        return (FALSE);

	// Check for Bogus events...
	if( !IsFocussedItem(hwnd, pIAcc, varChild) )
	{
		pIAcc->Release();
		return (FALSE);
	}

	VariantInit(&varRole); 

	hr = pIAcc->get_accRole(varChild, &varRole); 
	
	// Special casing stuff.. Handle State change for 
	// Outline items only for now
	if (varRole.vt == VT_I4 )
    {
		switch ( varRole.lVal )
		{
			case ROLE_SYSTEM_OUTLINEITEM:
				{
					OBJINFO objCurrent;

					objCurrent.hwnd = hwnd;
					objCurrent.plObj = (long*)pIAcc;
					objCurrent.varChild = varChild;
					
					SpeakObjectInfo(&objCurrent,TRUE);
				}
				break;

			default:
				break;
		}
	}

    pIAcc->Release();
    return(TRUE);
}

/*************************************************************************
    Function:   OnLocationChangedEvent
    Purpose:    Receives location change events - for the caret
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnLocationChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                            LONG idChild, DWORD dwmsTimeStamp)
{

	//
	// Get the caret position and save it.
	//
	
	// flag set by key down code - here do appropriate action after 
	// caret has moved

	if (g_MsrDoNext) 
	{ // read char, word etc.
		WORD    wLineNumber;
		WORD    wLineIndex;
		WORD    wLineLength;
		DWORD   dwGetSel;
		DWORD    wStart;
		DWORD    wEnd;
		WORD    wColNumber;
		WORD    wEndWord;
        LPTSTR  pszTextShared;
//        LPTSTR  pszTextLocal; PB 22 Nov 1998 Make it work!!! Decalre this as Global Shared above!
        HANDLE  hProcess;
        int     nSomeInt;
		int *p; // PB 22 Nov 1998 Use this to get the size of the buffer in to array
		DWORD   LineStart;
		// Send the EM_GETSEL message to the edit control.
		// The low-order word of the return value is the character
		// position of the caret relative to the first character in the
		// edit control.
		dwGetSel = (WORD)SendMessage(hwnd, EM_GETSEL, (WPARAM)(LPDWORD) &wStart, (LPARAM)(LPDWORD) &wEnd);
		if (dwGetSel == -1) 
		{
			return (FALSE);
		}
		// wStart = LOWORD(dwGetSel);
		// wEnd = HIWORD(dwGetSel);
        DBPRINTF (TEXT("Selection Starts at %d\r\n"),wStart);
        DBPRINTF (TEXT("Selection Ends at %d\r\n"),wEnd);
		
		LineStart = wStart;

		// New: Check for the selected text: AK
		if ( g_MsrDoNext == MSR_DOCHARR ) 
			LineStart = wEnd;
		else if ( g_MsrDoNext == MSR_DOLINED )
			LineStart = wEnd - 1;
		else if ( g_MsrDoNext == MSR_DOWORDR )
			LineStart = wEnd;

        // SteveDon: get the line for the start of the selection 
		wLineNumber = (WORD)SendMessage(hwnd,EM_LINEFROMCHAR, LineStart, 0L);
        DBPRINTF (TEXT("Line number of start of selection is %d\r\n"),wLineNumber);
        
        // get the first character on that line that we're on.
		wLineIndex = (WORD)SendMessage(hwnd,EM_LINEINDEX, wLineNumber, 0L);
        DBPRINTF (TEXT("Index of first character on that line is %d\r\n"),wLineIndex);
		
        // get the length of the line we're on
		wLineLength = (WORD)SendMessage(hwnd,EM_LINELENGTH, LineStart, 0L);
        DBPRINTF (TEXT("The lnegth of the line of text is %d\r\n"),wLineLength);
		
		// Subtract the LineIndex from the start of the selection,
		// This result is the column number of the caret position.
		wColNumber = LineStart - wLineIndex;
        DBPRINTF (TEXT("The caret is in column %d of that line\r\n"),wColNumber);

        // if we can't hold the text we want, say nothing.
		if (wLineLength > MAX_TEXT) 
		{
			return (FALSE);
		}
		
        // To get the text of a line, send the EM_GETLINE message. When 
        // the message is sent, wParam is the line number to get and lParam
        // is a pointer to the buffer that will hold the text. When the message
        // is sent, the first word of the buffer specifies the maximum number 
        // of characters that can be copied to the buffer. 
        // We'll allocate the memory for the buffer in "shared" space so 
        // we can all see it. 
        // Allocate a buffer to hold it

		
		// PB 22 Nov 1998  Make it work!!! next 6 lines new.  Use global shared memory to do this!!!
        nSomeInt = wLineLength+1;
		if (nSomeInt >= 2000)
				nSomeInt = 1999;
		p = (int *) pszTextLocal;
		*p = nSomeInt;
        SendMessage(hwnd, EM_GETLINE, (WPARAM)wLineNumber, (LPARAM)pszTextLocal);
		pszTextLocal[nSomeInt] = 0;

        DBPRINTF (TEXT("The line of text is\r'%s'\r\n"),pszTextLocal);

		// At this stage, local var pszTextLocal points to a (possibly) empty string.
		// We deal with that later...

		switch (g_MsrDoNext) 
		{
			case MSR_DOWORDR:
			case MSR_DOWORD:
				if (wColNumber >= wLineLength) 
				{
					SpeakMute(0);
					SpeakStringId(IDS_LINEEND);
					break;
				}
				else 
				{
					for (wEndWord = wColNumber; wEndWord < wLineLength; wEndWord++) 
					{
						if (pszTextLocal[wEndWord] <= ' ') 
						{
							break;
						}
					} 
					wEndWord++;
					lstrcpyn(pszTextLocal,pszTextLocal+wColNumber,wEndWord-wColNumber);
					pszTextLocal[wEndWord-wColNumber] = 0;
					SpeakMute(0);
					SpeakStringAll(pszTextLocal);
				}
				break;
			
			case MSR_DOCHARR:
					wColNumber = LineStart - wLineIndex - 1;
					// Fall Through
			case MSR_DOCHAR: // OK now read character to left and right

				if (wColNumber >= wLineLength)
				{
					SpeakMute(0);
					SpeakStringId(IDS_LINEEND);
				}
				else if (pszTextLocal[wColNumber] == TEXT(' '))
				{
					SpeakMute(0);
					SpeakStringId(IDS_SPACE);
				}
				else 
				{
					pszTextLocal[0] = pszTextLocal[wColNumber];
					pszTextLocal[1] = 0;
					SpeakMute(0);
					SpeakStringAll(pszTextLocal);
				}
				break;

			case MSR_DOLINED:
					// Fall through
			case MSR_DOLINE:
				pszTextLocal[wLineLength] = 0; // add null
				SpeakMute(0);
				SpeakStringAll(pszTextLocal);
				// FileWrite(pszTextLocal);
				break;
		} // end switch (g_MsrDoNext)
	} // end if (g_MsrDoNext)

    RECT            rcCursor;
    IAccessible*    pIAcc;
    HRESULT         hr;
    VARIANT         varChild;

   	SetRectEmpty(&rcCursor); // now sort out mouse position as apprpropriate

    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
    	hr = pIAcc->accLocation(&rcCursor.left, &rcCursor.top, 
	    						&rcCursor.right, &rcCursor.bottom, 
		    					varChild);
		// Move mouse cursor, Only when Track mouse option is selcted: AK
        if (SUCCEEDED(hr) && TrackInputFocus)
        {
            SetSecondary(rcCursor.left,rcCursor.top+(rcCursor.bottom/2),
				        TrackCaret && g_bMouseUp);
        }
        pIAcc->Release();
    }
    
    return (TRUE);
}

/*************************************************************************
    Function:   InitMSAA
    Purpose:    Initalize the Active Accessibility subsystem, including
				initializing the helper thread, installing the WinEvent
				hook, and registering custom messages.
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:    
*************************************************************************/
BOOL InitMSAA(void)
{
    // Call this FIRST to initialize the helper thread
    InitHelperThread();

    // Set up event call back
    g_hEventHook = SetWinEventHook(EVENT_MIN,            // We want all events
                                 EVENT_MAX,            
                                 GetModuleHandle(TEXT("NarrHook.dll")), // Use our own module
                                 WinEventProc,         // Our callback function
                                 0,                    // All processes
                                 0,                    // All threads
                                 WINEVENT_OUTOFCONTEXT /* WINEVENT_INCONTEXT */);
// Receive async events
// JMC: For Safety, lets always be 'out of context'.  Who cares if there is a 
// performance penalty.
// By being out of context, we guarantee the we won't bring down other apps if 
// there is a bug in our  event hook.


    // Did we install correctly? 
    if (g_hEventHook) 
	{
        //
        // register own own message for giving the cursor position
        //
		g_MSG_MSR_Cursor = RegisterWindowMessage(TEXT("MSR cursor")); 
        return(TRUE);
	}

    // Did not install properly - fail
    return(FALSE);
}   



/*************************************************************************
    Function:   UnInitMSAA
    Purpose:    Shuts down the Active Accessibility subsystem
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:    
*************************************************************************/
BOOL UnInitMSAA(void)
{
    // Remove the WinEvent hook
    UnhookWinEvent(g_hEventHook);

    // Call this LAST so that the helper thread can finish up. 
    UnInitHelperThread();
    
    return(TRUE);
}

// --------------------------------------------------------------------------
//
//  GetObjectAtCursor()
//
//  Gets the object the cursor is over.
//
// --------------------------------------------------------------------------
IAccessible * GetObjectAtCursor(VARIANT * pvarChild,HRESULT* pResult)
{
    POINT   pt;
    IAccessible * pIAcc;
    HRESULT hr;

    //
    // Get cursor object & position
    //
    if (g_ptCurrentMouse.x < 0)
		GetCursorPos(&pt);
	else
		pt = g_ptCurrentMouse;
	
    //
    // Get object here.
    //
    VariantInit(pvarChild);
    hr = AccessibleObjectFromPoint(pt, &pIAcc, pvarChild);

    *pResult = hr;
    if (!SUCCEEDED(hr)) {
        return(NULL);
	}
    
    return(pIAcc);
}


/*************************************************************************
    Function:   SpeakItem
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakItem(int nOption)
{
    TCHAR tszDesc[256];
    VARIANT varChild;
    IAccessible* pIAcc;
    HRESULT hr;
    POINT ptMouse;
    BSTR bstr;

	SpeakString(TEXT(" ")); // reset last utterence
    // Important to init variants
    VariantInit(&varChild);

    //
    // Get cursor object & position
    //
    if (g_ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_ptCurrentMouse;

    hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
    
    // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
    {
        hr = pIAcc->get_accDescription(varChild, &bstr);

	    if (bstr)
		{
#ifdef UNICODE
			lstrcpyn(tszDesc,bstr,sizeof(tszDesc));
#else
			// If we got back a string, use that instead.
			WideCharToMultiByte(CP_ACP, 0, bstr, -1, tszDesc, sizeof(tszDesc), NULL, NULL);
#endif
	        SysFreeString(bstr);
            SpeakStringAll(tszDesc);
		}
        if (pIAcc)
            pIAcc->Release();
        
    }
    return;
}



/*************************************************************************
    Function:   SpeakMute
    Purpose:    causes the system to shut up.
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakMute(int nOption)
{
    SendMessage(g_hwndMSR, WM_MUTE, 0, 0);
}


/*************************************************************************
    Function:   SpeakObjectInfo
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakObjectInfo(LPOBJINFO poiObj, BOOL ReadExtra)
{
    BSTR            bstrName;
    IAccessible*    pIAcc;
    long*           pl;
    HRESULT         hr;
    TCHAR           szName[MAX_TEXT]; // PB: 22 Nov. 1998 Make buffer bigger
    TCHAR           szSpeak[MAX_TEXT];
    TCHAR           szRole[MAX_TEXT_ROLE];  
    TCHAR           szState[MAX_TEXT_ROLE];
    TCHAR           szValue[MAX_TEXT_ROLE];
	VARIANT         varRole;
    VARIANT         varState;
	BOOL            bSayValue = TRUE;
	BOOL			bReadHTMLEdit = FALSE;
	DWORD			Role = 0;
    UINT            sizeVal = ARRAYSIZE(szName);

    bstrName = NULL;

    // Truncate them 
    szName[0] = TEXT('\0');
    szSpeak[0] = TEXT('\0');
    szRole[0] = TEXT('\0');
    szState[0] = TEXT('\0');
    szValue[0] = TEXT('\0');

    DBPRINTF (TEXT("Speak object info ...\r\n"));
    // Get the object out of the struct
    pl = poiObj->plObj;
    pIAcc =(IAccessible*)pl;

	GetObjectProperty(pIAcc, poiObj->varChild.lVal, ID_NAME, szName, sizeVal);
	DBPRINTF (TEXT("Name <%s>\r\n"),szName);    
	if (szName[0] == -1) // name going to be garbage
		LoadString(g_Hinst, IDS_NAMELESS, szSpeak, ARRAYSIZE(szSpeak)); // For now change "IDS_NAMELESS" in Resources to be just space!
	else
		lstrcpyn(szSpeak,szName, ARRAYSIZE(szSpeak));

	szName[0] = TEXT('\0');

	VariantInit(&varRole);
	hr = pIAcc->get_accRole(poiObj->varChild, &varRole);

	if (FAILED(hr)) 
    {
		DBPRINTF (TEXT("Failed role!\r\n"));
		MessageBeep(MB_OK);
		return;
	}

	if (varRole.vt == VT_I4) 
    {
		// Role == ROLE_SYSTEM_CLIENT is being used pretty commonly in
		// many controls. Donot ignore these....AK
		/*if (varRole.lVal == ROLE_SYSTEM_CLIENT) {
			DBPRINTF (TEXT("Failed client!\r\n"));
			return;
		}*/
		Role = varRole.lVal; // save for use below (if ReadExtra)

    	GetRoleText(varRole.lVal,szRole, ARRAYSIZE(szRole));
		DBPRINTF (TEXT("Role <%s>\r\n"),szRole);

		// Special casing stuff: 
		// Outline Items give out their level No. in the tree in the Value
		// field, So Don't speak it. 
		switch(varRole.lVal)
		{
			case ROLE_SYSTEM_STATICTEXT:
			case ROLE_SYSTEM_OUTLINEITEM:
			{
				DBPRINTF (TEXT("Don't do Value!\r\n"));
				bSayValue = FALSE; // don't speak value for text - it may be HTML link
			}
				break;

			// If the text is from combo -box then speak up 
			case ROLE_SYSTEM_TEXT:
				bReadHTMLEdit = TRUE;
				bSayValue = TRUE; // Speak text in combo box
				break;

			case ROLE_SYSTEM_LISTITEM:
			{
				FilterGUID(szSpeak); 
			}
			break;

            case ROLE_SYSTEM_SPINBUTTON:
				// Remove the Wizard97 spin box utterances....AK
				{
					HWND hWnd, hWndP;
					WindowFromAccessibleObject(pIAcc, &hWnd);
					if ( hWnd != NULL)
					{
						hWndP = GetParent(hWnd);

						LONG_PTR style = GetWindowLongPtr(hWndP, GWL_STYLE);
						if ( style & WS_DISABLED)
							return;
					}
				
				}
				break;

			default:
				break;
		}
	}
    else if (varRole.vt == VT_BSTR)
    {
#ifdef UNICODE
        lstrcpyn(szRole,varRole.bstrVal,ARRAYSIZE(szRole));
#else
		// If we got back a string, use that instead.
        WideCharToMultiByte(CP_ACP, 0, varRole.bstrVal, -1,szRole, ARRAYSIZE(szRole), NULL, NULL);
#endif
    }
	DBPRINTF (TEXT("Role 2<%s>\r\n"),szRole);

    // This will free a BSTR, etc.
    VariantClear(&varRole);

	if ( (lstrlen(szRole) > 0) && 
		(varRole.lVal != ROLE_SYSTEM_CLIENT) ) 
    {
	    lstrcatn(szSpeak, TEXT(", "),MAX_TEXT);
	    lstrcatn(szSpeak, szRole, MAX_TEXT);
		szRole[0] = TEXT('\0');
	}

    //
    // add value string if there is one
    //
    hr = pIAcc->get_accValue(poiObj->varChild, &bstrName);
    if (bstrName)
    {
#ifdef UNICODE
		lstrcpy(szName, bstrName); //, sizeof(bstrName) * sizeof(TCHAR));
#else
		// If we got back a string, use that instead.
        WideCharToMultiByte(CP_ACP, 0, bstrName,-1, szName, ARRAYSIZE(szName), NULL, NULL);
#endif
        SysFreeString(bstrName);
    }

// ROBSI: 10-10-99, Bug?
// We are not properly testing bSayValue here. Therefore, outline items are
// speaking their indentation level -- their accValue. According to comments
// above, this should be skipped. However, below we are explicitly loading
// IDS_TREELEVEL and using this level. Which is correct?
	// If not IE, read values for combo box, Edit etc.., For IE, read only for edit boxes
	if ( ((!g_bInternetExplorer && bSayValue ) || ( g_bInternetExplorer && bReadHTMLEdit ) )
		&& lstrlen(szName) > 0)  
    { // i.e. got a value
			lstrcatn(szSpeak,TEXT(", "),MAX_TEXT);
			lstrcatn(szSpeak,szName,MAX_TEXT);
			szName[0] = TEXT('\0');
	}

	hr = pIAcc->get_accState(poiObj->varChild, &varState);

	if (FAILED(hr)) 
    {
		MessageBeep(MB_OK);
		return;
	}

	if (varState.vt == VT_BSTR)
	{
		// If we got back a string, use that.
#ifdef UNICODE
		lstrcpyn(szState,varState.bstrVal,ARRAYSIZE(szState));
#else
		// If we got back a string, use that instead.
        WideCharToMultiByte(CP_ACP, 0, varState.bstrVal, -1,
                            szState, ARRAYSIZE(szState), NULL, NULL);
#endif
    }
    else if (varState.vt == VT_I4)
    {
        int     iStateBit;
        DWORD   lStateBits;
        LPTSTR  lpszT;
        UINT    cchT;
        UINT    cchBuf = sizeof(szState);

		// PB 22 Nov 1998.  Take these next three lines out as someone (!!??!!??!**!!)
		//					has made the View: Details list objects in Explorer INVISIBLE!!!
		//					So make them speak even if INVISIBLE.
        // This is still like this !! a-anilk
/*		if (varState.lVal & STATE_SYSTEM_INVISIBLE) { // don't do invisible ones.
DBPRINTF (TEXT("Invisible\r\n"));    
			return;
		}
*/
        // We have a mask of standard states.  Make a string.
        lpszT = szState;
        
        for (iStateBit = 0, 
             lStateBits = 1; iStateBit < 32; 
             iStateBit++, (lStateBits <<= 1))
        {
            if (varState.lVal & lStateBits & STATE_MASK)
            {
                cchT = GetStateText(lStateBits, lpszT, cchBuf);
                lpszT += cchT;
                cchBuf -= cchT;
                *lpszT++ = ',';
                *lpszT++ = ' ';
            }
        }

        //
        // Clip off final ", "
        //
        if (varState.lVal)
        {
            *(lpszT-2) = 0;
            *(lpszT-1) = 0;
        }
        else
            GetStateText(0, szState, cchBuf);
    }

	if (lstrlen(szState) > 0) 
    {
	    lstrcatn(szSpeak, TEXT(", "), MAX_TEXT);
	    lstrcatn(szSpeak, szState, MAX_TEXT);
        szState[0] = TEXT('\0');
	}

	if (ReadExtra && ( // Speak extra information if just got focus on this item
		Role == ROLE_SYSTEM_CHECKBUTTON || 
		Role == ROLE_SYSTEM_PUSHBUTTON || 
		Role == ROLE_SYSTEM_RADIOBUTTON ||
        Role == ROLE_SYSTEM_MENUITEM || 
		Role == ROLE_SYSTEM_OUTLINEITEM || 
		Role == ROLE_SYSTEM_LISTITEM)
	   ) {
		switch (Role) {
			case ROLE_SYSTEM_CHECKBUTTON:
				{
					// Change due to localization issues:a-anilk
					TCHAR szTemp[MAX_TEXT_ROLE];
					
					if (varState.lVal & STATE_SYSTEM_CHECKED)
						LoadString(g_Hinst, IDS_TO_UNCHECK, szTemp, MAX_TEXT_ROLE);
					else
						LoadString(g_Hinst, IDS_TO_CHECK, szTemp, MAX_TEXT_ROLE);
					// GetObjectProperty(pIAcc, poiObj->varChild.lVal, ID_DEFAULT, szName, 256);
					// wsprintf(szTemp, szTempLate, szName);
					lstrcatn(szSpeak, szTemp, MAX_TEXT);
				}
				break;

			case ROLE_SYSTEM_PUSHBUTTON:
				{
					if ( !(varState.lVal & STATE_SYSTEM_UNAVAILABLE) )
					{
						LoadString(g_Hinst, IDS_TOPRESS, szName, 256);
						lstrcatn(szSpeak, szName, MAX_TEXT);
					}
				}
				break;

			case ROLE_SYSTEM_RADIOBUTTON:
	            LoadString(g_Hinst, IDS_TOSELECT, szName, 256);
				lstrcatn(szSpeak, szName, MAX_TEXT);
                break;

                // To distinguish between menu items with sub-menu and without one.
                // For submenus, It speaks - ', Has a sub-menu': a-anilk
            case ROLE_SYSTEM_MENUITEM:
                {
                    long count = 0;
                    pIAcc->get_accChildCount(&count);
	                
                    // count = 1 for all menu items with sub menus
                    if ( count == 1 )
                    {
                        LoadString(g_Hinst, IDS_SUBMENU, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
                    }
                }
				
				break;

			case ROLE_SYSTEM_OUTLINEITEM:
				{
					// Read out the level in the tree....
					// And also the status as Expanded or Collapsed....:AK
					TCHAR buffer[64];

					if ( varState.lVal & STATE_SYSTEM_COLLAPSED )
					{
						LoadString(g_Hinst, IDS_TEXPAND, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
					else if ( varState.lVal & STATE_SYSTEM_EXPANDED )
					{
						LoadString(g_Hinst, IDS_TCOLLAP, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
					
					hr = pIAcc->get_accValue(poiObj->varChild, &bstrName);

					LoadString(g_Hinst, IDS_TREELEVEL, szName, 256);
                    wsprintf(buffer, szName, bstrName);
					lstrcatn(szSpeak, buffer, MAX_TEXT);
					
					SysFreeString(bstrName);
				}
				break;

			case ROLE_SYSTEM_LISTITEM:
				{
					// The list item is selectable, But not selected...:a-anilk
					if ( (varState.lVal & STATE_SYSTEM_SELECTABLE ) &&
							(!(varState.lVal & STATE_SYSTEM_SELECTED)) )
					{
						LoadString(g_Hinst, IDS_NOTSEL, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
				}
				break;
		}
	}

    DBPRINTF (TEXT("<%s>\r\n"),szSpeak);
    SpeakString(szSpeak);

    return;
}

/*************************************************************************
    Function:   SpeakMainItems
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakMainItems(int nOption)
{
    VARIANT varChild;
    IAccessible* pIAcc=NULL;
    HRESULT hr;
    POINT ptMouse;

//    DBPRINTF(TEXT("SpeakMainItems.\n"));
 
	SpeakString(TEXT(" "));
    //
    // Get cursor object & position
    //
    if (g_ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_ptCurrentMouse;

    // Important to init variants
    VariantInit(&varChild);

    hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
   // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
        {
	        OBJINFO objCurrent;

	        objCurrent.hwnd = WindowFromPoint(ptMouse);
		    objCurrent.plObj = (long*)pIAcc;
			objCurrent.varChild = varChild;
			SpeakObjectInfo(&objCurrent,FALSE);
            pIAcc->Release();
	}
	return;
}


/*************************************************************************
    Function:   SpeakKeyboard
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakKeyboard(int nOption)
{
    TCHAR szName[128];
    VARIANT varChild;
    IAccessible* pIAcc;
    HRESULT hr;
    POINT ptMouse;

    //
    // Get cursor object & position
    //
    if (g_ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_ptCurrentMouse;

   // Important to init variants
   VariantInit(&varChild);
   hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
    
    // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
    {
		SpeakStringId(IDS_KEYBOARD);

		GetObjectProperty(pIAcc, varChild.lVal, ID_SHORTCUT, szName, sizeof(szName));
        SpeakString(szName);

        if (pIAcc)
            pIAcc->Release();
        
    }
    return;
}


// --------------------------------------------------------------------------
//
//  DoDirection()
//
//  This finds the next object to the left/above/right/bottom of the current
//  one.  Then it moves the cursor there.
//
// --------------------------------------------------------------------------
void DoDirection(int navDir)
{
    IAccessible* pobj;
    VARIANT      varRet;
    RECT         rcItem;
    HRESULT      hr;
	POINT		 tmpSearch;
	RECT		 rect;
	RECT		 CurrentRect;
	int			 stepX, stepY;
	POINT		 OldCursorPos;

    //
    // What's under the cursor?
    //
    pobj = GetObjectAtCursor(&varRet,&hr);
    if (!pobj)
    {
        MessageBeep(0);
        return;
    }

	GetCursorPos(&OldCursorPos);
    SetRectEmpty(&rcItem);

    pobj->accLocation(&rcItem.left, &rcItem.top, &rcItem.right, &rcItem.bottom, varRet);
	pobj->Release();
	pobj = NULL;

	tmpSearch = g_ptCurrentMouse;

	GetWindowRect(GetForegroundWindow(),&rect);
	switch (navDir) 
    {
		case MSR_KEYUP:
			stepX = 0;
			stepY = -10;
			if (rcItem.right/rcItem.bottom > 4)
				tmpSearch.x = rcItem.left+2;
			else
				tmpSearch.x = rcItem.left + rcItem.right/2;
			tmpSearch.y = rcItem.top - 5;
			break;
		case MSR_KEYDOWN:
			stepX = 0;
			stepY = 10;
			if (rcItem.right/rcItem.bottom > 4)
				tmpSearch.x = rcItem.left+2;
			else
				tmpSearch.x = rcItem.left + rcItem.right/2;
			tmpSearch.y = rcItem.top+5;
			break;
		case MSR_KEYLEFT:
			stepX = -10;
			stepY = 0;
			tmpSearch.x = rcItem.left-5; // reasonable to catch next item to left
			if (rcItem.bottom < 30)
				tmpSearch.y = rcItem.top + rcItem.bottom/2; 
			else
				tmpSearch.y = rcItem.top + 8; 
			break;
		case MSR_KEYRIGHT:
			stepX = 10;
			stepY = 0;
			tmpSearch.x = rcItem.left+5;// reasonable to catch next item to left
			if (rcItem.bottom < 30)
				tmpSearch.y = rcItem.top + rcItem.bottom/2; 
			else
				tmpSearch.y = rcItem.top + 8; 
			break;
	}

	for (int i = 0; i < 500; i++) 
    {
		tmpSearch.x += stepX;
		tmpSearch.y += stepY;
		if (tmpSearch.x <= rect.left  || 
            tmpSearch.y <= rect.top   || 
            tmpSearch.x >= rect.right || 
            tmpSearch.y >= rect.bottom) 
        {
			SpeakStringId(TXT_NOMORE); // gone outside the bounds of this window!
			if (pobj)
				pobj->Release();
			return;
		}

		hr = AccessibleObjectFromPoint(tmpSearch, &pobj, &varRet);
		if (FAILED(hr)) 
        {
			continue;
		}

	    hr = pobj->accLocation(&CurrentRect.left, &CurrentRect.top, &CurrentRect.right, &CurrentRect.bottom, varRet);
		if (FAILED(hr)) 
        {
			if (pobj) 
            {
				pobj->Release();
				pobj = NULL;
			}
			continue;
		}

		// is it new object containing the current object - i.e. window or frame?
		if (CurrentRect.left <= rcItem.left && 
            CurrentRect.top <= rcItem.top   && 
            (CurrentRect.left+CurrentRect.right) >= (rcItem.left+rcItem.right) && 
            (CurrentRect.top+CurrentRect.bottom) >= (rcItem.top+rcItem.bottom)) 
        {
			if (stepY < 0 && tmpSearch.y - CurrentRect.top < 10) 
            { 
                // i.e. going up AND title of window, grouping etc.
//				SpeakString(TEXT("Found top"));
				break;
			}
			pobj->Release();
			pobj = NULL;
			continue;
		}
		break;
	} /* for i = 0 to 500 */

    if (CurrentRect.right || CurrentRect.bottom)
    {
        // rcItem.right & rcItem.bottom have the width & height values
        // not the right & bottom coords.
		// if much wider than tall then put cursor at top-left hand side
		// - heuristic for list boxes etc..
		SetSecondary(tmpSearch.x,tmpSearch.y,TrackSecondary);
        SpeakMainItems(0);
    }

	pobj->Release();
    return;
}

/*************************************************************************
    Function:   Home
    Purpose:    
    Inputs:     
    Returns:    
    History:
    
      ALT_HOME to take secondary cursor to top of this window
*************************************************************************/
void Home(int x)
{
    RECT rect;
    GetWindowRect(GetForegroundWindow(),&rect);

	// Set it to show the title bar 48, max system icon size
    SetSecondary(rect.left + 48/*(rect.right - rect.left)/2*/, rect.top + 5,TrackSecondary);
    SpeakMainItems(0);
}


/*************************************************************************
    Function:   MoveToEnd
    Purpose:    
    Inputs:     
    Returns:    
    History:
    
        ALT_END to take secondary cursor to top of this window
*************************************************************************/
void MoveToEnd(int x)
{
    RECT rect;
    GetWindowRect(GetForegroundWindow(),&rect);

    SetSecondary(rect.left+ 48 /*(rect.right - rect.left)/2*/,rect.bottom - 8,TrackSecondary);
    SpeakMainItems(0);
}

/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void Navigate(BOOL fForward)
{
    IAccessible* pobj;
    VARIANT varElement;
    VARIANT varReturn;
    RECT    rcCursor;
    HRESULT hr;
    int     dwNavT;

    // 
    // Get what's under the cursor
    //
	pobj = GetObjectAtCursor(&varElement,&hr);
    if (! pobj)
    {
        MessageBeep(0);
        return;
    }

    SetRectEmpty(&rcCursor);

    //
    // Navigate from here
    //

    // If we are on the container, try getting the first child.
    if ((varElement.vt == VT_I4) && (varElement.lVal == 0))
    {
        dwNavT = (fForward ? NAVDIR_FIRSTCHILD : NAVDIR_LASTCHILD);
    }
    else
    {
        dwNavT = (fForward ? NAVDIR_NEXT : NAVDIR_PREVIOUS);
    }

TryAgain:
    VariantInit(&varReturn);
    varReturn.vt = 0; varReturn.lVal = 0;
    hr = pobj->accNavigate(dwNavT, varElement, &varReturn);
    if (!SUCCEEDED(hr))
    {
        MessageBeep(0);
        goto BailOut;
    }

    //
    // What did we get back?
    //

    if (varReturn.vt == VT_I4)
    {
        // If varElement is empty/zero and dwNavT isn't NAVDIR_FIRSTCHILD,
        // then we got a non-object peer of an object.  So get this one's
        // parent, and pass the id into THAT.
        if ((dwNavT < NAVDIR_FIRSTCHILD) && 
            ((varElement.vt == VT_EMPTY) ||
             (varElement.vt == VT_I4 && varElement.lVal == 0) ))
        {
            varElement.pdispVal = NULL;
            hr = pobj->get_accParent(&varElement.pdispVal);
            pobj->Release();
            pobj = NULL;

            if (!SUCCEEDED(hr))
            {
                MessageBeep(0);
                return;
            }

            if (!varElement.pdispVal)
            {
                return;
            }

            hr = varElement.pdispVal->QueryInterface(IID_IAccessible,
                (void**)&pobj);
            varElement.pdispVal->Release();

            if (!SUCCEEDED(hr))
            {
                MessageBeep(0);
                return;
            }
        }

GotTheLocation:
		{
			// Great, a peer object.  Get its location by going through
			// the object we have.
			VARIANT varT;
			BOOL fInvisible;

			VariantInit(&varT);
			hr = pobj->get_accState(varReturn, &varT);

            fInvisible = ((varT.vt == VT_I4) && 
                         (varT.lVal & STATE_SYSTEM_INVISIBLE)) ? TRUE : FALSE;

			VariantClear(&varT);

			hr = pobj->accLocation(&rcCursor.left, &rcCursor.top,
			    &rcCursor.right, &rcCursor.bottom, varReturn);

			// If invissible, go to the next one
                  // Note: right is width, bottom is height.
			if (fInvisible || FAILED(hr) ||
			  (!rcCursor.right && !rcCursor.bottom))
			{
			    varElement = varReturn;
			    dwNavT = (fForward ? NAVDIR_NEXT : NAVDIR_PREVIOUS);
			    goto TryAgain;
			}
		}
    }
    else if (varReturn.vt == VT_DISPATCH)
    {
//DescendAgain:
        // OK, we got back a real object peer.  Ask it directly its location.
        pobj->Release();
        pobj = NULL;

        hr = varReturn.pdispVal->QueryInterface(IID_IAccessible,
            (void**)&pobj);
        varReturn.pdispVal->Release();

        // Did we get an IAccessible for this one?
        if (!SUCCEEDED(hr))
        {
            MessageBeep(0);
            return;
        }

        // No need to 
        varReturn.vt = VT_I4;
        varReturn.lVal = 0;
        goto GotTheLocation;

    }
    else if (varReturn.vt == VT_EMPTY)
    {
        //
        // Are we on a node that is a real object that in fact does
        // handle peer-to-peer navigation?
        //
        if ((dwNavT >= NAVDIR_FIRSTCHILD) && (varElement.vt == VT_I4) &&
            (varElement.lVal == 0))
        {
            dwNavT = (fForward ? NAVDIR_NEXT : NAVDIR_PREVIOUS);
            goto TryAgain;
        }

        dwNavT = (fForward ? NAVDIR_NEXT : NAVDIR_PREVIOUS);

        // There are no more next items. So get the next object after the
        // container.
        if ((varElement.vt == VT_I4) && (varElement.lVal != 0))
        {
            VariantInit(&varElement);
            varElement.vt = VT_I4;
            varElement.lVal = 0;
            goto TryAgain;
        }
        else
        {
            // There aren't any peers of this one.  So move
            // up to our parent and ask IT what is next.

            // We would like to be able to know what our own 
            // id is. I.E. What the parent thinks of as our ID.
            varReturn.pdispVal = NULL;
            hr = pobj->get_accParent(&varReturn.pdispVal);
            pobj->Release();
            pobj = NULL;

            if (!SUCCEEDED(hr))
            {
                MessageBeep(0);
                return;
            }

            if (!varReturn.pdispVal)
            {
                return;
            }

            hr = varReturn.pdispVal->QueryInterface(IID_IAccessible,
                (void**)&pobj);
            varReturn.pdispVal->Release();

            if (!SUCCEEDED(hr))
            {
                MessageBeep(0);
                return;
            }

            // Ask this guy from himself.
            // this is a problem. We should be starting at the parent's
            // child that we used to get the parent in the first place.
            // But the child object doesn't know it's own ID!!
            VariantInit(&varElement);
            varElement.vt = VT_I4;
            varElement.lVal = 0;
            goto TryAgain;
        }
    }
    else
    {
        // No clue what this.
        VariantClear(&varReturn);
        MessageBeep(0);
    }

    if (rcCursor.right || rcCursor.bottom)
    {
        // This tiny offset is to make sure that we do place
        // the pointer within the object boundary. since
        // hittest, visual and accLocation boundaries sometimes
        // do not align perfectly, especially the top left
        // location. e.g. Windows Explorer detail view
        // list items; IE3 toolbar buttons.
        long xOff = (rcCursor.right<2) ? 0: 2; 
        long yOff = (rcCursor.bottom<2) ? 0: 2;
		long tempX, tempY;
		RECT rect;
		tempX = rcCursor.left+xOff;
		tempY = rcCursor.top+yOff;
		GetWindowRect(GetForegroundWindow(),&rect);
		if (tempX < rect.left || tempY < rect.top) 
        {
			SpeakStringId(TXT_NOMORE);
			goto BailOut;
		}
		if (tempX > rect.right || tempY > rect.bottom) 
        {
			SpeakStringId(TXT_NOMORE);
			goto BailOut;
		}

		SetSecondary(rcCursor.left+xOff,rcCursor.top+yOff,TrackSecondary);
        SpeakMainItems(0);
    }

BailOut:
    pobj->Release();
    return;
}

#define LEFT_ID		0
#define RIGHT_ID	1
#define TOP_ID		2
#define BOTTOM_ID	3
#define SPOKEN_ID	4
#define SPATIAL_SIZE 2500
long ObjLocation[5][SPATIAL_SIZE];
int ObjIndex;

#define MAX_SPEAK 8192
/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpatialRead(RECT rc)
{
    int left_min, top_min, index_min; // current minimum object
    int i, j; // loop vars
    DBPRINTF(TEXT("In Recursive read\n"));

    for (i = 0; i < ObjIndex; i++) 
    {
        left_min = 20000;
        top_min = 20000;
        index_min = -1;
        
        if (g_bInternetExplorer)
        {
            for (j = 0; j < ObjIndex; j++) 
            {
                if (ObjLocation[SPOKEN_ID][j] == 0 && // not been spoken before
                    (ObjLocation[TOP_ID][j] < top_min  || // check if more top-left than previous ones - give or take on height (i.e. 5 pixels)
                     (ObjLocation[TOP_ID][j] == top_min && ObjLocation[LEFT_ID][j] < left_min) // more left on this line
                    )
                   ) 
                { // OK got a candidate
                    index_min = j;
                    top_min = ObjLocation[TOP_ID][j];
                    left_min = ObjLocation[LEFT_ID][j];
                }
            } // for j
        } // end if Internet Explorer
        else 
        {
            for (j = 0; j < ObjIndex; j++) 
            { 
                if (ObjLocation[SPOKEN_ID][j] == 0 && // not been spoken before
                    // check if enclosed by current rectangle (semi-hierarcical - with recursion!)
                    (ObjLocation[LEFT_ID][j] >= rc.left && ObjLocation[LEFT_ID][j] <= rc.left + rc.right &&
                     ObjLocation[TOP_ID][j] >= rc.top && ObjLocation[TOP_ID][j] <= rc.top + rc.bottom
                    ) &&
                    
                    // also check if more top-left than previous ones - give or take on height (i.e. 10 pixels)
                    ( (ObjLocation[TOP_ID][j] < top_min + 10 && ObjLocation[LEFT_ID][j] <= left_min)
                    //      or just higher up
                    || (ObjLocation[TOP_ID][j] < top_min)
                    )
                   ) 
                { // OK got a candidate
                    index_min = j;
                    top_min = ObjLocation[TOP_ID][j];
                    left_min = ObjLocation[LEFT_ID][j];
                }
            } // for j
        } // end not Internet Explorer

        if (index_min >= 0) 
        { // got one!
            HWND hwndList; 
            TCHAR szText[MAX_SPEAK];
            RECT rect;
            ObjLocation[SPOKEN_ID][index_min] = 1; // don't do this one again
            hwndList = GetDlgItem(g_hwndMSR, IDC_WINDOWINFO);
            SendMessage(hwndList, LB_GETTEXT, index_min, (LPARAM) szText);
            SpeakString(szText);
            if (g_bInternetExplorer) // no recursion for IE
                continue;
            rect.left = ObjLocation[LEFT_ID][index_min];
            rect.right = ObjLocation[RIGHT_ID][index_min];
            rect.top = ObjLocation[TOP_ID][index_min];
            rect.bottom = ObjLocation[BOTTOM_ID][index_min];
            SpatialRead(rect);
        }
    } // for i
}

//--------------------------------------------------------------------------
//
//  SpeakWindow()
//
//  Fills in a tree view with the descendants of the given top level window.
//  If hwnd is 0, use the previously saved hwnd to build the tree.
//
//--------------------------------------------------------------------------
void SpeakWindow(int nOption)
{
    IAccessible*  pacc;
    RECT rect;
    TCHAR szName[128];
    VARIANT varT;
    HWND ForeWnd;
    TCHAR buffer[100];

    szName[0] = NULL;
    DBPRINTF (TEXT("SpeakWindow\n"));
    g_AutoRead = nOption; // set global flag to tell code in AddItem if we're to read edit box contents (don't do it if just got focus as the edit box has probably been spoken already
    
    ForeWnd = GetForegroundWindow();		// Check if we're in HTML Help
    GetClassName(ForeWnd,buffer,100); 
    g_bHTML_Help = 0;
	if ((lstrcmpi(buffer, CLASS_HTMLHELP) == 0) ) 
    {
        g_bInternetExplorer = TRUE;
        g_bHTML_Help = TRUE;
        GetWindowRect(ForeWnd, &rect); // get the left hand side of our window to use later
        g_LeftHandSide = rect.left;
    }
    
	else if ( (lstrcmpi(buffer, CLASS_IE_FRAME) == 0) ||
		(lstrcmpi(buffer, CLASS_IE_MAINWND) == 0))
	{
        g_bInternetExplorer = TRUE;
        g_bHTML_Help = FALSE;
        GetWindowRect(ForeWnd, &rect); // get the left hand side of our window to use later
        g_LeftHandSide = rect.left;
	}
    // Inititalise stack for tree information
    ObjIndex = 0; 
    //
    // Get the object for the root.
    //
    pacc = NULL;
    /*if (g_bInternetExplorer && !g_bHTML_Help) 
    { // get current point from CurrentPoint then get parent then get all peers
        AccessibleObjectFromPoint(g_ptCurrentMouse, &pacc, &varT);
    }
    else*/
        AccessibleObjectFromWindow(GetForegroundWindow(), OBJID_WINDOW, IID_IAccessible, (void**)&pacc);
    
    if (nOption == 1) 
    { // if it was a keyboard press then speak the window's name
        SpeakStringId(IDS_WINDOW);
        GetWindowText(GetForegroundWindow(), szName, 128);
        SpeakString(szName);
    }
    
    if (pacc)
    {
        HWND hwndList; // first clear the list box used to store the window info
        hwndList = GetDlgItem(g_hwndMSR, IDC_WINDOWINFO);
        SendMessage(hwndList, LB_RESETCONTENT, 0, 0); 
        
        VariantInit(&varT);
        AddAccessibleObjects(pacc, &varT); // recursively go off and get the information
        pacc->Release();
        GetWindowRect(GetForegroundWindow(),&rect);
        if (ReviewStyle) 
        {
            SpatialRead(rect);
        }
    }
}

//--------------------------------------------------------------------------
//
//  AddItem()
//
//  Parameters?
//  Return Values?
//
//--------------------------------------------------------------------------
BOOL AddItem(IAccessible* pacc, VARIANT* pvar)
{
    TCHAR           szItemString[MAX_TEXT] = TEXT(" ");
    TCHAR           szName[MAX_NAME] = TEXT(" ");
    TCHAR           szRole[128] = TEXT(" ");
    TCHAR           szState[128] = TEXT(" ");
	TCHAR			szValue[MAX_VALUE] = TEXT(" ");
	TCHAR			szLink[32];
    VARIANT         varT;
    BSTR            bszT;
	BOOL			DoMore = TRUE;
	BOOL			GotStaticText = FALSE;
	BOOL			GotGraphic = FALSE;
	BOOL			GotText = FALSE;
	BOOL			GotNameless = FALSE;
	BOOL			GotInvisible = FALSE;
	BOOL			GotLink = FALSE;
	int				lastRole = 0;
	static TCHAR	szLastName[MAX_NAME] = TEXT(" ");

#ifdef _DEBUG
    static int g_n = 0;

    //DBPRINTF (TEXT("AddItem - %i - %s\r\n"),g_n++,szLastName);   
#endif

    //
    // Get object state first.  If we are skipping invisible dudes, we want
    // to bail out now.
    //
    VariantInit(&varT);
    pacc->get_accState(*pvar, &varT);
    if (varT.vt == VT_BSTR) {
#ifdef UNICODE
		lstrcpyn(szState, varT.bstrVal,128);
#else
        WideCharToMultiByte(CP_ACP, 0, varT.bstrVal, -1, szState, 128, 
                            NULL, NULL);
#endif
    }
    else if (varT.vt == VT_I4)
    {
        // Only use the ones we care about
        varT.lVal &= STATE_SYSTEM_UNAVAILABLE | 
                     STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_CHECKED;

		GotInvisible = varT.lVal & STATE_SYSTEM_INVISIBLE;

        // Bail out if not shown - unless IE around - more checks for IE invisible objects below
        if (!g_bInternetExplorer && GotInvisible) 
        {
            return(NULL);
        }

		if (varT.lVal != 0)
			GetStateText(varT.lVal, szState, 128);
		else
			szState[0] = 0;
    }

    VariantClear(&varT);
    //
    // Get object role.
    //
    VariantInit(&varT);
    pacc->get_accRole(*pvar, &varT);

    if (varT.vt == VT_BSTR) {
#ifdef UNICODE
		lstrcpyn(szRole,varT.bstrVal,128);
#else
        WideCharToMultiByte(CP_ACP, 0, varT.bstrVal, -1, szRole, 128, 
                            NULL, NULL);
#endif
    }
    else if (varT.vt == VT_I4) 
    {
		switch (varT.lVal) 
        {
            case ROLE_SYSTEM_WINDOW: 
            case ROLE_SYSTEM_TABLE : 
            case ROLE_SYSTEM_DOCUMENT:
            {
                // it's a window - don't read it - read its kids
				return(TRUE); // but carry on searching down
            }

            case ROLE_SYSTEM_LIST:       
            case ROLE_SYSTEM_GROUPING:
            case ROLE_SYSTEM_SLIDER:     
            case ROLE_SYSTEM_STATUSBAR:
            case ROLE_SYSTEM_BUTTONMENU: 
            case ROLE_SYSTEM_COMBOBOX: 
            case ROLE_SYSTEM_DROPLIST:   
            case ROLE_SYSTEM_OUTLINE:    
            case ROLE_SYSTEM_TOOLBAR:
                DoMore = NULL;    // i.e. speak it but no more children
                break;

            // Some of the CLIENT fields in office2000 are not spoken because 
            // we don't add. We may need to specail case for office :a-anilk
            case ROLE_SYSTEM_CLIENT : // for now work with this for IE ...???
            case ROLE_SYSTEM_PANE :
                return(TRUE);

			case ROLE_SYSTEM_CELL: // New - works for HTML Help!
				return(TRUE);

            case ROLE_SYSTEM_SEPARATOR:  
            case ROLE_SYSTEM_TITLEBAR: 
            case ROLE_SYSTEM_GRIP: 
            case ROLE_SYSTEM_MENUBAR:    
            case ROLE_SYSTEM_SCROLLBAR:
                return(NULL); // don't speak it or it's children

            case ROLE_SYSTEM_GRAPHIC: // this works for doing icons!
                GotGraphic = TRUE;
                break;

			case ROLE_SYSTEM_LINK:
				GotLink = TRUE;
				break;

            case ROLE_SYSTEM_TEXT:
                GotText = TRUE;
                break;
            case ROLE_SYSTEM_SPINBUTTON:
				// Remove the Wizard97 spin box utterances....
				{
					HWND hWnd, hWndP;
					WindowFromAccessibleObject(pacc, &hWnd);
					if ( hWnd != NULL)
					{
						hWndP = GetParent(hWnd);

						LONG_PTR style = GetWindowLongPtr(hWndP, GWL_STYLE);
						if ( style & WS_DISABLED)
							return NULL;
					}
				
					DoMore = NULL;    // i.e. speak it but no more children
				}

			case ROLE_SYSTEM_PAGETAB:
				// Hack to not read them if they are disabled...
				// Needed for WIZARD97 style :AK
				{
					HWND hWnd;
					WindowFromAccessibleObject(pacc, &hWnd);
					if ( hWnd != NULL)
					{
						LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
						if ( style & WS_DISABLED)
							return NULL;
					}
				}
				break;
		} // switch

        // only read invisible text and graphics in IE
		if (g_bInternetExplorer && GotInvisible && 
			!(varT.lVal == ROLE_SYSTEM_TEXT || varT.lVal == ROLE_SYSTEM_GRAPHIC))
        {
			return(TRUE);
        }
		
		GetRoleText(varT.lVal, szRole, 128);

// ROBSI: 10-10-99, BUG? Why (Role == Static) or (IE)??
		if (varT.lVal == ROLE_SYSTEM_STATICTEXT || g_bInternetExplorer) 
        {
            // don't speak role for this 
            // speech is better without
			szRole[0] = 0;                  
			GotStaticText = TRUE;
		}
	}
    else
        szRole[0] = 0;	// lstrcpy(szRole, TEXT("UNKNOWN"));

	VariantClear(&varT);

    //
    // Get object name.
    //
    bszT = NULL;
    pacc->get_accName(*pvar, &bszT);
    if (bszT)
    {
#ifdef UNICODE
		lstrcpyn(szName, bszT, MAX_NAME);
#else
        WideCharToMultiByte(CP_ACP, 0, bszT, -1, szName, MAX_NAME, NULL, NULL);
#endif
        SysFreeString(bszT);
		if (szName[0] == -1) 
        { // name going to be garbage
			LoadString(g_Hinst, IDS_NAMELESS, szName, 256);
			GotNameless = TRUE;
		}
    }
    else 
    {
		LoadString(g_Hinst, IDS_NAMELESS, szName, 256);
		GotNameless = TRUE;
	}

    bszT = NULL;
    pacc->get_accValue(*pvar, &bszT); // get value string if there is one

	szValue[0] = 0;
    if (bszT)
    {
#ifdef UNICODE
		lstrcpyn(szValue, bszT, MAX_VALUE);
		// lstrcpy(szValue, bszT);
#else
        WideCharToMultiByte(CP_ACP, 0, bszT, -1, szValue, MAX_VALUE, NULL, NULL);
#endif
        SysFreeString(bszT);
    }
   
    //
    // make sure these are terminated for the compare
    //
    szLastName[MAX_NAME - 1]=TEXT('\0');
    szName[MAX_NAME - 1]=TEXT('\0');

    //
    // don't want to repeat name that OLEACC got from static text
    // so if this name is the same as the previous name - don't speak it.
    //
	if (lstrcmp(szName,szLastName) == 0)
		szName[0] = 0; 

	if (GotStaticText)
		lstrcpyn(szLastName, szName, MAX_NAME);
	else
		szLastName[0] = 0;

	lstrcpy(szItemString,szName);

    if (g_bInternetExplorer) 
    {
        if (GotText && szName[0] == 0)      // no real text
            return(NULL);
        
        if (GotNameless && szValue[0] == 0) // nameless with no link
            return(NULL);
        
        if (GotLink/*szValue[0]*/)  
        {
            // got a link
            // GotLink = TRUE;
            LoadString(g_Hinst, IDS_LINK, szLink, 32);
            lstrcatn(szItemString,szLink,MAX_TEXT);
        }
    }
    else if (GotText && g_AutoRead)
        lstrcatn(szItemString,szValue,MAX_TEXT);

    if (!GotText && !GotLink && !GotGraphic) 
    {
        
        if (lstrlen(szName) && lstrlen(szRole))
            lstrcatn(szItemString,TEXT(", "),MAX_TEXT);
        
        if (lstrlen(szRole)) 
        {
            lstrcatn(szItemString,szRole,MAX_TEXT);
            if (lstrlen(szValue) || lstrlen(szState))
                lstrcatn(szItemString, TEXT(", "),MAX_TEXT);
        }
        if (lstrlen(szValue)) 
        {
            lstrcatn(szItemString,szValue,MAX_TEXT);
            
            if (lstrlen(szState))
                lstrcatn(szItemString,TEXT(", "),MAX_TEXT);
        }
        if (lstrlen(szState))
            lstrcatn(szItemString,szState,MAX_TEXT);
        
		// Too much speech of period/comma. Just a space is fine...
        lstrcatn(szItemString, TEXT(" "),MAX_TEXT);
    }

    if (ReviewStyle)  
    {
        HWND hwndList; 
        
        if (ObjIndex >= SPATIAL_SIZE) // only store so many
            return(DoMore);
        
        pacc->accLocation(&ObjLocation[LEFT_ID][ObjIndex], 
                          &ObjLocation[TOP_ID][ObjIndex],
                          &ObjLocation[RIGHT_ID][ObjIndex], 
                          &ObjLocation[BOTTOM_ID][ObjIndex], 
                          *pvar);
        
        // Dreadfull Hack/heuristic!
        // bin information as it's the left hand side of the HTML help window
        if (g_bHTML_Help && (ObjLocation[LEFT_ID][ObjIndex] < g_LeftHandSide + 220))
            return(DoMore);
        
        hwndList = GetDlgItem(g_hwndMSR, IDC_WINDOWINFO);
        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szItemString); 
        
        ObjLocation[SPOKEN_ID][ObjIndex] = 0;
        ObjIndex++;
    }
    else
        SpeakString(szItemString);

    return(DoMore);
}
        

// --------------------------------------------------------------------------
//
//  AddAccessibleObjects()
//    
//  This is a recursive function.  It adds an item for the parent, then 
//  adds items for its children if it has any.  It returns the number of
//  items added.  
//
//	Parameters:
//	IAccessible*	pacc	Pointer to an IAccessible interface for the
//							object being added. Start with the 'root' object.
//	VARIANT*		pvarT	Pointer to a variant that contains the ID of the
//							child object to retrieve.
//
//	The first call, pacc points to the top level window object,
//					pvarT points to a variant that is VT_EMPTY
// 
//	There are 2 possible ways to find all the children of an object:
//	The first, and most efficient, is to ask for the IEnumVARIANT interface,
//	which (if it is supported) will allow you to quickly get all the 
//	children, or if that interface does not exist, then you should call
//  IAccessible:get_accChildCount to see how many children there are, then
//	IAccessible:get_accChild to get each child, passing child id's of 1,2,3,
//  ..cChildren.
//
// --------------------------------------------------------------------------
long AddAccessibleObjects(IAccessible* pacc, VARIANT* pvarT)
{
    VARIANT         varT;
    long            lAdded;

    lAdded = 0;

    if (AddItem(pacc,pvarT))
    {
        lAdded++;

        //
        // Is this an object in its own right?
		// We can tell it is an object because the variant is empty.
		// 
		// If the variant is not empty, the value is the child ID.
        //
        if (pvarT->vt == VT_EMPTY)
        {
            IEnumVARIANT*   penum;		// pointer to enumeration interface
            IDispatch*      pdisp;		// pointer to dispatch interface
            IAccessible*    paccChild;	// pointer to child's accessible 
                                        // interface
            int             ichild;		// which child (1...cchildren)
            long            cchildren;	// number of children
            ULONG           cfetched;	// number of objects returned 
                                        // (used by penum->Next)
            //
            // Loop through our children.
            //
            penum = NULL;
            pacc->QueryInterface(IID_IEnumVARIANT, (void**)&penum);
            if (penum)
                penum->Reset();

            cchildren = 0;
            pacc->get_accChildCount(&cchildren);
            ichild = 1;

            while (ichild <= cchildren)
            {
                VariantInit(&varT);

                //      
                // Get the next child.
                //
                if (penum)
                {
                    cfetched = 0;

                    // get next (1 child,child in varT,
                    //           &cfetched = num children-should be 1)
                    // note that varT may be an IDispatch interface

                    penum->Next(1, &varT, &cfetched);	

                    if (!cfetched)
                        break;	// leave the while (all children) loop
                }
                else
                {
                    varT.vt = VT_I4;
                    varT.lVal = ichild;
                }

				// varT can have two values right now. It can be a VT_I4 or
				// a VT_DISPATCH. If it is a VT_I4, it is either because
				// we set it that way (the parent object doesn't support 
				// IEnumVARIANT), or because the parent wants us to talk to
				// it to get information about its children. 
				//
				// In any case, if varT is a VT_I4, we now need to try to 
				// get a pointer to the Child's IAccessible by asking the 
				// parent for it. (pacc->get_accChild) The parent will 
				// either give us the child's IAccessible by changing 
				// varT to be VT_DISPATCH and filling in pdisp, or it
				// will return the child's ID in varT and pdisp will be 
				// null. If pdisp is null, then we call AddAccessibleObjects 
				// with the child's ID, otherwise we jump down to the place 
				// we would be at if we got back a VT_DISPATCH when we called
				// penum->Next().
				//
				// If varT is a VT_DISPATCH, then we QI for IAccessible
				// using varT.pdispval and then call AddAccessibleObjects()

				if (varT.vt == VT_I4) 
                {
                    //
                    // Is this child really an object, make sure?
                    //
                    pdisp = NULL;
                    pacc->get_accChild(varT, &pdisp);
                    if (pdisp)
                        goto ChildIsAnObject;

                    // Add just it to the outline
                    lAdded += AddAccessibleObjects(pacc, &varT);
                }
                else if ((varT.vt == VT_DISPATCH) && varT.pdispVal)
                {
                    pdisp = varT.pdispVal;

ChildIsAnObject:
                    paccChild = NULL;
                    pdisp->QueryInterface(IID_IAccessible, (void**)&paccChild);
                    pdisp->Release();

                    //
                    // Add this object and all its children to the tree also
                    //
                    if (paccChild)
                    {
                        VariantInit(&varT);
                        lAdded += AddAccessibleObjects(paccChild, &varT);
                        paccChild->Release();
                    }
                }
                else
                    VariantClear(&varT);

                ichild++;
            }

            if (penum)
                penum->Release();
        }
    }

    return(lAdded);
}


// --------------------------------------------------------------------------
// Helper method to filter out bogus focus events...
// Returns FALSE if the focus event is bogus, Otherwise returns TRUE
// a-anilk: 05-28-99
// --------------------------------------------------------------------------
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild )
{
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 
	// Is it toolbar, We cannot determine who had focus!!!
	if ((lstrcmpi(buffer, CLASS_TOOLBAR) == 0) ||
		(lstrcmpi(buffer, CLASS_IE_MAINWND) == 0))
			return TRUE;

	VARIANT varState;
	HRESULT hr;
	
	VariantInit(&varState); 
	hr = pAcc->get_accState(varChild, &varState);

	
	if ( hr == S_OK)
	{
		if ( ! (varState.lVal & STATE_SYSTEM_FOCUSED) )
			return FALSE;
	}
	else if (FAILED(hr)) // ROBSI: 10-11-99. If OLEACC returns an error, assume no focus.
	{
		return FALSE;
	}

	return TRUE;
}


#ifdef _DEBUG
//--------------------------------------------------------------------------
//
// This function takes the arguments and then uses wvsprintf to
// format the string into a buffer. It then uses OutputDebugString to show 
// the string.
//
//--------------------------------------------------------------------------
void FAR CDECL PrintIt(LPTSTR strFmt, ...)
{
static TCHAR StringBuf[4096] = {0};
static int  len;
va_list		ArgList;

	va_start(ArgList, strFmt);
	len = wvsprintf (StringBuf,strFmt,ArgList);
	if (len > 0)
	{
		OutputDebugString (TEXT("narrhook: "));
		OutputDebugString (StringBuf);
	}
	va_end (ArgList);
	return;
}

#endif // _DEBUG

/*void FileWrite(LPTSTR pszTextLocal)
{
	FILE * fp = fopen("d:\\NarLog.txt", "a+");
	fputws((const wchar_t*) pszTextLocal, fp);
	fputws((const wchar_t*) "\n", fp);
	fclose(fp);
}*/


#define TAB_KEY 0x09
#define CURLY_KEY 0x7B
// Helper method Filters GUID's that can appear in names: AK
void FilterGUID(TCHAR* szSpeak)
{
	// the GUID's have a Tab followed by a {0087....
	// If you find this pattern. Then donot speak that:AK
	while(*szSpeak != NULL)
	{
		if ( (*szSpeak == TAB_KEY) &&
			  (*(++szSpeak) == CURLY_KEY) )
		{
			*(--szSpeak) = NULL;
			return;
		}

		szSpeak++;
	}
}

/*************************************************************************
    THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY REFERRED TO
    AS "SOFTWARE") IS PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN
    NO EVENT SHALL MICROSOFT CORPORATION OR ITS SUPPLIERS BE LIABLE FOR
    ANY DAMAGES WHATSOEVER INCLUDING DIRECT, INDIRECT, INCIDENTAL,
    CONSEQUENTIAL, LOSS OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF
    MICROSOFT CORPORATION OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES. SOME STATES DO NOT ALLOW THE EXCLUSION OR
    LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES SO THE
    FOREGOING LIMITATION MAY NOT APPLY.
*************************************************************************/
