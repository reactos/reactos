/*--------------------------------------------------------------
 *
 * FILE:			SK_EX.C
 *
 * PURPOSE:		This File contains the interface routines
 *					that connect Serial Keys to the Mouse or keyboard.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * NOTES:		
 *					
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm 
 * without the express written permission of Black Diamond Software. 
 * This permission is available only in the form of a Software Source 
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Includes  ---------------------------------------------------------*/

#include	<windows.h>

#include 	"debug.h"
#include 	"vars.h"
#include	"sk_defs.h"
#include	"sk_comm.h"
#include 	"sk_ex.H"

#ifdef QUEUE_BUF
typedef	struct _KEYQUE
{
	BYTE	VirKey;
	BYTE	ScanCode;
	int		Flags;
} KEYQUE;

#define MAXKEYS 100

KEYQUE KeyQue[MAXKEYS];
int	KeyFront = 0;		// Pointer to front of Que
int	KeyBack	= 0;		// Pointer to Back of Que
#endif


#define 	CTRL		56
#define 	ALT			29
#define 	DEL			83

char    Key[3];
int     Push = 0;

POINT 		MouseAnchor;
HWND		MouseWnd;

static	HDESK	s_hdeskSave = NULL;
static	HDESK	s_hdeskUser = NULL;


// Local Function Prototypes -------------------------------------

static void SendAltCtrlDel();
static void CheckAltCtrlDel(int scanCode);
static void AddKey(BYTE VirKey, BYTE ScanCode, int Flags);


/*---------------------------------------------------------------
 *	Public Functions
/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SetAnchor()
 *
 *	TYPE		Global
 *
 * PURPOSE		Sets an anchor to the current mouse position within
 *				the current window.
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SetAnchor()
{
	GetCursorPos(&MouseAnchor);

#ifdef DEBUG
{
	char buf[70];
	wsprintfA(buf, "SkEx_SetAnchor( x %d y %d )", MouseAnchor.x, MouseAnchor.y);
	DBG_OUT(buf);
}
#endif

//	MouseWnd = GetActiveWindow();
//	ScreenToClient(MouseWnd, &MouseAnchor);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_GetAnchor()
 *
 *	TYPE		Global
 *
 * PURPOSE		Returns the mouse postion within the active window
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL SkEx_GetAnchor(LPPOINT Mouse)
{
#if 0
	HWND	CurrentWnd;

	CurrentWnd = GetActiveWindow();

	if (CurrentWnd != MouseWnd)			// Has the Active window Changed?
		return(FALSE);					// Yes Return False

	ClientToScreen(MouseWnd, &MouseAnchor);	// Convert Window to Screen

#endif

	Mouse->x = MouseAnchor.x;
	Mouse->y = MouseAnchor.y;

#ifdef DEBUG
{
	char buf[70];
	wsprintfA(buf, "SkEx_GetAnchor( x %d y %d )", MouseAnchor.x, MouseAnchor.y);
	DBG_OUT(buf);
}
#endif

	return(TRUE);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendBeep()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Down events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendBeep()
{
	MessageBeep(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SetBaud(int Baud)
 *
 *	TYPE		Global
 *
 * PURPOSE		Sets the Baudrate for the current port
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SetBaud(int Baud)
{
	DBG_OUT("SkEx_SetBaud()");

	SetCommBaud(Baud);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendKeyDown()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Down events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendKeyDown(int scanCode)
{
	BYTE c;
	int	Flags = 0;

	if (scanCode & 0xE000)				// Is this and Extended Key
	{
		Flags  = KEYEVENTF_EXTENDEDKEY;	// Yes - Set Ext Flag
		scanCode &= 0x000000FF;			// Clear out extended value
	}
	c = (BYTE)MapVirtualKey(scanCode, 3);

	if (scanCode == ALT || scanCode == CTRL || scanCode == DEL)
		CheckAltCtrlDel(scanCode);

#ifdef DEBUG
{
	char buf[60];
	wsprintfA(buf, "SkEx_SendKeyDown(Virtual %d Scan %d Flag %d)", c, scanCode, Flags);
	DBG_OUT(buf);
}
#endif
	DeskSwitchToInput();         
	keybd_event(c, (BYTE) scanCode, Flags, 0L);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendKeyDown()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Up events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendKeyUp(int scanCode)
{
	BYTE	c;
	int		Flags = 0;

	if (Push)
	{
		Key[0] = Key[1] = Key[2] = 0;	// Clear Buffer
		Push = 0;						// Reset AltCtrlDel
	}

	if (scanCode & 0xE000)				// Is this and Extended Key
	{
		Flags  = KEYEVENTF_EXTENDEDKEY;	// Yes - Set Ext Flag
		scanCode &= 0xFF;				// Clear out extended value
	}

	Flags += KEYEVENTF_KEYUP;
	c = (BYTE) MapVirtualKey(scanCode, 3);

#ifdef DEBUG
{
	char buf[60];
	wsprintfA(buf, "SkEx_SendKeyUp(Virtual %d Scan %d Flags %d)", c, scanCode, Flags);
	DBG_OUT(buf);
}
#endif
	DeskSwitchToInput();         
	keybd_event(c, (BYTE) scanCode, Flags, 0L);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendMouse()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Mouse Events to the Event manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendMouse(MOUSEKEYSPARAM *p)
{

#ifdef DEBUG
{
	char buf[70];
	wsprintfA(buf, "SkEx_SendMouse(Stat %d x %d y %d )", p->Status, p->Delta_X, p->Delta_Y);
	DBG_OUT(buf);
}
#endif

	// 
	// This code converts screen positions relative to logical Mouse Positions
	// The Mouse range is from 0, 0 to 65536, 65536
#if 0

	int	Wide, High;

	if (p->Status & MOUSEEVENTF_ABSOLUTE)		// Is this Absolute Cordinates
	{							
		Wide = GetSystemMetrics(SM_CXSCREEN);	// Yes - Convert to Screen
		High = GetSystemMetrics(SM_CYSCREEN);

		p->Delta_X = (p->Delta_X * 65536) / Wide;
		p->Delta_Y = (p->Delta_Y * 65536) / High;
	}

#ifdef DEBUG
	wsprintf(buf, "Screen Wide %d High %d", Wide, High);
	DBG_OUT(buf);
#endif
#endif

	p->Delta_X /= 2;
	p->Delta_Y /= 2;

	DeskSwitchToInput();         

	mouse_event
	(
		(DWORD) p->Status,		// Flags specifing motion / click variants
		(DWORD) p->Delta_X,		// Horizontal Position or change
		(DWORD) p->Delta_Y,		// Vertical Position or change
		0,						// Reserved
		GetMessageExtraInfo()	// 32 bit Extra Value
	);
}

#ifdef QUEUE_BUF
/*---------------------------------------------------------------
 *
 * FUNCTION	SendKey()
 *
 *	TYPE		Global
 *
 * PURPOSE		This Function Send keys from the Que to Windows NT
 *				
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SendKey()
{
	if (KeyBack == KeyFront)		// Are there Keys in the Que?
		return;						// No - Exit;

#ifdef DEBUG
{
	char buf[70];
	wsprintf(buf, "SkEx_SendKey(KeyBack %d )", KeyBack);
	DBG_OUT(buf);
}
#endif
	DeskSwitchToInput();         
	keybd_event						// Process the Key Event
	(
		KeyQue[KeyBack].VirKey,
		KeyQue[KeyBack].ScanCode,
		KeyQue[KeyBack].Flags, 0L
	);

	KeyBack++;						// Increment Key pointer
	if (KeyBack == MAXKEYS)			// Are we at the End of the buffer
		KeyBack = 0;				// Yes - Reset to start.
}			  

/*---------------------------------------------------------------
 *	Local Functions
/*---------------------------------------------------------------
 *
 * FUNCTION	AddKey(BYTE VirKey, BYTE ScanCode, int Flags)
 *
 *	TYPE		Local
 *
 * PURPOSE		Adds a key to the Key Que.  
 *						   
 * INPUTS		BYTE 	VirKey 	- Virtual Key
 *				BYTE 	ScanCode- 
 *				int		Flags	-
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void AddKey(BYTE VirKey, BYTE ScanCode, int Flags)
{

#ifdef DEBUG
{				 
	char buf[70];
	wsprintf(buf, "AddKey(KeyFront %d )", KeyFront);
	DBG_OUT(buf);
}
#endif

	// Add Keys to Que
	KeyQue[KeyFront].VirKey 	= VirKey;	
	KeyQue[KeyFront].ScanCode	= ScanCode;
	KeyQue[KeyFront].Flags		= Flags;

	KeyFront++;							// Point to next Que
	if (KeyFront == MAXKEYS)			// Are we at the End of the buffer
		KeyFront = 0;					// Yes - Reset to start.

	// Process the Key Event
	DeskSwitchToInput();         
	keybd_event(VirKey, ScanCode, Flags, 0L);

}
#endif		// QUE

/*---------------------------------------------------------------
 *
 * FUNCTION	CheckAltCtrlDel(int scanCode)
 *
 *	TYPE		Local
 *
 * PURPOSE		Checks for the condition of Alt-Ctrl-Del key 
 *				Combination.
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void CheckAltCtrlDel(int scanCode)
{
	int val;

	DBG_OUT("CheckAltCtrlDel()");
	Key[Push] = (char)scanCode;		// Save Scan Code
	Push++;							// Inc Index

	if (Push != 3)					// Have we got 3 keys?
		return;						// No - Exit
	
	val = Key[0] + Key[1] + Key[2];	// Sum up the Keys

	if (val = (ALT+CTRL+DEL))		// Is Buffer Alt=Ctrl=Del
		SendAltCtrlDel();			// Yes - Send command
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SendAltCtrlDel()
 *
 *	TYPE		Local
 *
 * PURPOSE		Signal system reset
 *				
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void SendAltCtrlDel()
{
	HWINSTA hwinsta;
	HDESK hdesk;
	HWND hwndSAS;
	HWINSTA	hwinstaSave;
	HDESK	hdeskSave;

	DBG_OUT("SendAltCtrlDel()");

	hwinstaSave = GetProcessWindowStation();
	hdeskSave = GetThreadDesktop(GetCurrentThreadId());

	hwinsta = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
	SetProcessWindowStation(hwinsta);
	hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
	SetThreadDesktop(hdesk);

	hwndSAS = FindWindow(NULL, TEXT("SAS window"));
////PostMessage(hwndSAS, WM_HOTKEY, 0, 0);
	SendMessage(hwndSAS, WM_HOTKEY, 0, 0);

	if (NULL != hdeskSave)
	{
		SetThreadDesktop(hdeskSave);
	}

	if (NULL != hwinstaSave)
	{
		SetProcessWindowStation(hwinstaSave);
	}
	
	CloseDesktop(hdesk);
	CloseWindowStation(hwinsta);
}

BOOL DeskSwitchToInput()
{
	BOOL fOk = FALSE;
	HANDLE	hNewDesktop;

	// We are switching desktops

	DBG_OUT("DoDeskSwitch()");
	
	// get current Input desktop
	hNewDesktop = OpenInputDesktop(		
			0L,
			FALSE,
			MAXIMUM_ALLOWED);

	if (NULL == hNewDesktop)
	{
		DBG_OUT("OpenInputDesktop failed");
	}
	else
	{
		fOk = SetThreadDesktop(hNewDesktop);	// attach thread to desktop
		if (!fOk)
		{
			DBG_ERR("Failed SetThreadDesktop()");
		}
		else
		{
			if (NULL != s_hdeskUser)
			{
				CloseDesktop(s_hdeskUser);		// close old desktop
			}
			s_hdeskUser = hNewDesktop;		// save desktop
		}
	}
	return(fOk);
}
