/*      File: D:\WACKER\tdll\tdll.c (Created: 26-Nov-1993)
 *
 *      Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 4 $
 *      $Date: 7/01/99 3:09p $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>

#include "stdtyp.h"
#include <term\res.h>
#include "globals.h"
#include "session.h"
#include "assert.h"
#include "misc.h"
#include "tdll.h"
#include "tchar.h"
#include "vu_meter.h"
#include "banner.h"
#include "mc.h"
#include "open_msc.h"
#ifdef INCL_DEFAULT_TELNET_APP
#include "telnetck.h"
#endif
#ifdef INCL_NAG_SCREEN
#include "nagdlg.h"
#include "register.h"
#endif

#define SESSION_CLASS TEXT("SESSION_WINDOW")

static BOOL InitDll(const HINSTANCE hInstance);
static int      HTCheckInstance(TCHAR *pachCmdLine);
BOOL WINAPI TDllEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

#if defined(INCL_PRIVATE_EDITION_BANNER)
BOOL RegisterBannerAboutClass(HANDLE hInstance); // see aboutdlg.c
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      TDllEntry
 *
 * DESCRIPTION:
 *      Currently, just initializes the C-Runtime library but may be used
 *      for other things later.
 *
 * ARGUMENTS:
 *      hInstDll        - Instance of this DLL
 *      fdwReason       - Why this entry point is called
 *      lpReserved      - reserved
 *
 * RETURNS:
 *      BOOL
 *
 */
BOOL WINAPI TDllEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
	{
	/* --- Docs say to call _CRT_INIT before any of our code - mrw --- */

	if (_CRT_INIT(hInstDll, fdwReason, lpReserved) == FALSE)
		return FALSE;

	switch (fdwReason)
		{
		case DLL_PROCESS_ATTACH:
			if (!InitDll(hInstDll))
				return FALSE;
			break;

		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		default:
			break;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      InitDll
 *
 * DESCRIPTION:
 *      Intializes the application.  Done only for the first instance.
 *
 */
static BOOL InitDll(const HINSTANCE hInstance)
	{
	WNDCLASS  wc;

	glblSetDllHinst(hInstance);

	// Session Class

	wc.style                 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC)SessProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon                 = extLoadIcon(MAKEINTRESOURCE(IDI_PROG));
	wc.hCursor               = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = 0;
	wc.lpszMenuName  = TEXT("MainMenu");
	wc.lpszClassName = SESSION_CLASS;

	if (RegisterClass(&wc) == FALSE)
		{
		assert(FALSE);
		return FALSE;
		}

	if (RegisterTerminalClass(hInstance) == FALSE)
		{
		assert(FALSE);
		return FALSE;
		}

	if (RegisterVuMeterClass(hInstance) == FALSE)
		{
		assert(FALSE);
		return FALSE;
		}

	if (RegisterSidebarClass(hInstance) == FALSE)
		{
		assert(FALSE);
		return FALSE;
		}

    #if defined(INCL_PRIVATE_EDITION_BANNER)
    if (RegisterBannerAboutClass(hInstance) == FALSE)
	{
	assert(FALSE);
	return FALSE;
	}
    #endif

	// Documentation says this should be called once per application
	// If we use any of the common controls which we do.

	InitCommonControls();

    #if defined(INCL_PRIVATE_EDITION_BANNER)
    LoadLibrary("RICHED32");
    #endif

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      InitInstance
 *
 * DESCRIPTION:
 *      Creates the session window.  Does instance specific stuff.
 *
 * ARGUMENTS:
 *      HINSTANCE       hInstance       - apps instance handle.
 *      LPTSTR          lpCmdLine       - copy of the command line.
 *      int             nCmdShow        - passed from WinMain arg list.
 *
 */
BOOL InitInstance(const HINSTANCE hInstance,
					const LPTSTR lpCmdLine,
					const int nCmdShow)
	{
	HWND    hwnd;
	HWND    hwndBanner;
	TCHAR   ach[100];
	HACCEL  hAccel;
	DWORD   dwStart, dwNow;

	#if !defined(NDEBUG)
    #if !defined(NO_SMARTHEAP)
	MemRegisterTask();
    #endif
	#endif

	// Save program's instance handle

	glblSetHinst(hInstance);

	if (HTCheckInstance(lpCmdLine) == TRUE)
		return FALSE;

	// Get program title

	LoadString(glblQueryDllHinst(),
				IDS_GNRL_APPNAME,
				ach,
				sizeof(ach) / sizeof(TCHAR));

	// Read program's help file name from the resource file.
	// JMH 12/12/96 Moved up here so the default telnet dialog
	// could use its context help.
	//
	glblSetHelpFileName();

#ifdef INCL_NAG_SCREEN
    // Do the nag screen stuff
    //
    if ( IsEval() && IsTimeToNag() )
        {
        DoDialog(glblQueryDllHinst(),
                MAKEINTRESOURCE(IDD_NAG_SCREEN),
                0,
                DefaultNagDlgProc,
                0);
        }

    // Display the registration reminder dialog if needed...
    //
    if (!IsRegisteredUser())
        {
        if (IsInitialRun())
            {
            DoRegister();
            SetLastReminderDate();
            }

        else if (IsWillingToBeReminded())
            {
            if (IsTimeToRemind())
                DoRegistrationReminderDlg(0);
            }
        }

#endif

#ifdef INCL_DEFAULT_TELNET_APP
    // Do the default telnet app stuff
    //
    if ( !IsHyperTerminalDefaultTelnetApp() && QueryTelnetCheckFlag() )
        {
#ifndef NT_EDITION
		//ask the user if they want HT to be the default telnet app
        DoDialog(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_DEFAULT_TELNET),
            0, DefaultTelnetAppDlgProc, 0);
#else
		//mpt:8-9-97 MS asked us to make HT the default telnet app without asking - yippee!
		SetTelnetCheckFlag(FALSE);	//so we don't check again
		SetDefaultTelnetApp();		//just do it without asking
#endif
        }
#endif

    // Setup and display the banner

	bannerRegisterClass(glblQueryDllHinst());
		hwndBanner = bannerCreateBanner(glblQueryDllHinst(), ach);

	UpdateWindow(hwndBanner);
	glblSetHwndBanner(hwndBanner);

	dwStart = GetTickCount();

	// Load accerator table for program.

	hAccel = LoadAccelerators(glblQueryDllHinst(), MAKEINTRESOURCE(IDA_WACKER));

	if (hAccel == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	glblSetAccelHdl(hAccel);

	// mpt:07-30-97
	if ( IsNT() )
		CreateUserDirectory();

	// Create a main window for this application instance.  Pass command
	// line string as user data to be stored and acted on later.

	hwnd = CreateWindowEx(
	  WS_EX_WINDOWEDGE,
	  SESSION_CLASS,
	  ach,
	  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	  0,
	  0,
	  glblQueryDllHinst(),
	  lpCmdLine
      );

	if (!IsWindow(hwnd))
		{
		assert(FALSE);
		return FALSE;
		}

	// Needed in the message loop unfortunately
	//
	glblSetHwndFrame(hwnd);

	// Leave banner up at least BANNER_TIME
	//
	if (!glblQueryProgramStatus())
		{
		dwNow = GetTickCount();

		if ( (dwNow - dwStart) < BANNER_TIME)
            {
        #ifdef USE_PRIVATE_EDITION_3_BANNER
            // The HTPE 3.0 banner has a button on it. If we just sleep,
            // no Windows messages will be processed, so you can't push
            // the button. We need to sleep, but we also need to pump
            // messages as well. - cab:11/29/96
            //
            Rest(BANNER_TIME - (dwNow - dwStart));
        #else
			Sleep(BANNER_TIME - (dwNow - dwStart));
        #endif
            }
		}

	// Post a message to size and show the window.
	//
	PostMessage(hwnd, WM_SESS_SIZE_SHOW, (WPARAM)nCmdShow, 0);

	// Posting this next messages kicks off the connection stuff
	//
	PostMessage(hwnd, WM_CMDLN_DIAL, (WPARAM)nCmdShow, 0);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  MessageLoop
 *
 * DESCRIPTION:
 *  Wackers main message loop
 *
 * ARGUMENTS:
 *  void
 *
 * RETURNS:
 *  void
 *
 */
int MessageLoop(void)
    {
	MSG msg;

	while (GetMessage(&msg, 0, 0, 0))
		{
		if (!CheckModelessMessage(&msg))
			ProcessMessage(&msg);
		}

    return (int)msg.wParam;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      GetFileNameFromCmdLine
 *
 * DESCRIPTION:
 *      Extracts the file name, if any, from the command line and fully
 *      qualifies it.
 *
 * ARGUMENTS:
 *      pachCmdLine   Copy of command line
 *      pachFileName  Place to put the result
 *      nSize             Size of buffer pointed to by pachFileName (in bytes)
 *
 * RETURNS:
 *
 *
 */
int GetFileNameFromCmdLine(TCHAR *pachCmdLine, TCHAR *pachFileName, int nSize)
	{
	int nIdx = 0;
	TCHAR *pszStr;
	TCHAR acName[300];
	TCHAR acPath[300];
	TCHAR ach[20];
	TCHAR *pachFile;
	TCHAR *pszTelnet = TEXT("telnet:"); //jmh 3/24/97
	LPWIN32_FIND_DATA       pstFindData;
    HANDLE                              hSearchHdl = INVALID_HANDLE_VALUE;
    int fTelnetCmdLnDial = FALSE;

	pachFileName[0] = TEXT('\0');
	nIdx = 0;

    //Just in case there is no command line. This happened when one specified '/t' 
    //with no hostname on the command line - mpt 05/28/99
    if ( pachCmdLine[0] == TEXT('\0') )
        {
        return 0;
        }
    
    for (pszStr = pachCmdLine;
			*pszStr != TEXT('\0');
			pszStr = StrCharNext(pszStr))
		{
		/*
		 * This works because we only allow certain characters as switches
		 */
		if (*pszStr == TEXT('/'))
			{
			/* Process as a switch */
			pszStr = StrCharNext(pszStr); // skip the switch char
            //JMH 03-24-97 Test for special case here...
            if (*pszStr == TEXT('\0'))
                {
                break;
                }
            else if (*pszStr == TEXT('T') || *pszStr == TEXT('t'))
                {
                fTelnetCmdLnDial = TRUE;
                }
			}
		else
			{
			/* Copy all non switch stuff to the buffer */
			if (nIdx < (sizeof(acName) - 1))
				{
				// JFH:6/9/95 acName[nIdx++] = *pszStr;
				if (IsDBCSLeadByte(*pszStr))
					{
					MemCopy(&acName[nIdx], pszStr, (size_t)2 * sizeof(TCHAR));
					nIdx += 2;
					}
				else
					{
					acName[nIdx++] = *pszStr;
					}
				}
			}
		}
	acName[nIdx] = TEXT('\0');

	/* Trim leading and trailing spaces */
	pszStr = TCHAR_Trim(acName);

//jmh 3/24/97 Needed to copy this from sessCheckAndLoadCmdLn(), and modify it
// slightly. This needs to be here because it affects the filename, and we want
// to make sure we're comparing the same name as in other instances.
#if defined(INCL_WINSOCK)
	// If this is a telnet address from the browser, it will usually be preceeded
	// by the string telnet:  If so, we must remove it or it will confuse some of
	// the code to follow  jkh, 03/22/1997
    if (fTelnetCmdLnDial)
        {
		nIdx = strlen(pszTelnet);
                if (_strnicmp(acName, pszTelnet, nIdx) == 0)
			{
			// Remove the telnet string from the front of acName
			memmove(acName, &acName[nIdx], (strlen(acName) - nIdx) + 1);
			}
		}

	// See if URL contains a port number. This will take the form of
    // addr:nnn where nnn is the port number i.e. culine.colorado.edu:860
    // or there might be the name of an assigned port like hilgraeve.com:finger.
    // We support numeric port right now, may add port names later. jkh, 3/22/1997
    pszStr = strchr(acName, TEXT(':'));
    if (pszStr && isdigit(pszStr[1]))
        {
        *pszStr = TEXT('\0');
        }
#endif

	// Now that the parsing is done, open the session file if one
	// was supplied.  A session name that is not fully qualified may have
	// been passed in on the command line.  Fully qulaify that name, then
	// carry on.
	//
	if (GetFullPathName(acName, sizeof(acPath), acPath, &pachFile) <
			(sizeof(acName) / sizeof(TCHAR)))
		{
		StrCharCopy(acName, acPath);
		}

	// Get hypertrm extension
	//
	ach[0] = TEXT('\0');

	LoadString(glblQueryDllHinst(), IDS_GNRL_HAS, ach,
		sizeof(ach) / sizeof(TCHAR));

	// For now, lets assume all session files end in HT.  If it
	// doesn't append the .HT.  John Hile and I have discussed
	// this and feel it should be otherwise but don't want to
	// make a change this late in the ball game. - mrw,3/2/95
	//
	if ((pszStr = strrchr(acName, '.')))
		{
                if (_stricmp(pszStr, ach) != 0 && _stricmp(pszStr, ".TRM") != 0)
			strcat(pszStr, ach);
		}

	else
		{
		strcat(acName, ach);
		}

	if (acName[0] != TEXT('\0'))
		{
		// Convert the possible short file name (i.e., the 8.3 format)
		// to the long file name.  There is no straight forward way to
		// do this so we must call the FindFirstFile() API to get the
		// long file name.
		//
		pstFindData = malloc(sizeof(WIN32_FIND_DATA));
		if (pstFindData)
			{
			memset(pstFindData, 0, sizeof(WIN32_FIND_DATA));
				
			hSearchHdl = FindFirstFile((LPCSTR)acName, pstFindData);
			if (hSearchHdl != INVALID_HANDLE_VALUE)
				{
				// Keep the path for later...
				//
				memset(acPath, 0, sizeof(acPath));
				StrCharCopy(acPath, acName);
				mscStripName(acPath);

				// acName & pstFindData->cFileName are of the same size,
				// MAX_PATH = 260.
				//
				StrCharCat(acPath, pstFindData->cFileName);
				StrCharCopy(acName, acPath);
				FindClose(hSearchHdl);
				}
			free(pstFindData);
			pstFindData = NULL;
			}
		}

	if (StrCharGetStrLength(acName) < nSize)
		StrCharCopy(pachFileName, acName);

	return 0;
	}

// Used for CheckInstCallback and HTCheckInstance
//
static int fKillTheApp;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      CheckInstCallback
 *
 * DESCRIPTION:
 *      Callback for EnumWindows
 *
 * ARGUMENTS:
 *      hwnd    - window handle from EmuWindows
 *      lPar    - optional data parameter (file name in this case)
 *
 * RETURNS:
 *      FALSE if it finds another instance using the same file name.
 *      TRUE if is doesn't
 *
 * AUTHOR: Mike Ward, 27-Jan-1995
 */
BOOL CALLBACK CheckInstCallback(HWND hwnd, LPARAM lPar)
	{
	BOOL  fRet = TRUE;
	TCHAR szClass[256];
	GetClassName(hwnd, szClass, sizeof(szClass));

	if (StrCharCmpi(szClass, SESSION_CLASS) == 0) // mrw, 2/12/95
		{
		ATOM aFile = GlobalAddAtom((TCHAR *)lPar);

		if (SendMessage(hwnd, WM_HT_QUERYOPENFILE, 0, (LPARAM)aFile))
			{
			if (!IsZoomed(hwnd))
				ShowWindow(hwnd, SW_RESTORE);

			SetForegroundWindow(hwnd);
			fKillTheApp = TRUE;
			fRet = FALSE;
			}

		GlobalDeleteAtom(aFile);
		}

	return fRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      CheckInstance
 *
 * DESCRIPTION:
 *      Checks if another instance of HyperTerminal is using this file.
 *
 * ARGUMENTS:
 *      pachFile        - filename this guy is trying to open
 *
 * RETURNS:
 *      TRUE=yes, another HT using it.
 *      FALSE=nope
 *
 * AUTHOR: Mike Ward, 27-Jan-1995
 */
static int HTCheckInstance(TCHAR *pachCmdLine)
	{
	TCHAR achPath[300];

	// Get the filename
	//
	GetFileNameFromCmdLine(pachCmdLine, achPath, sizeof(achPath));

	// If EnumWindows callback (CheckInstCallback) matches the given
	// path, it will set the fKillApp guy to TRUE
	//
	fKillTheApp = FALSE;

	// EnumWindows will give our callback toplevel windows one at a time
	//
	EnumWindows(CheckInstCallback, (LPARAM)achPath);
	return fKillTheApp;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  Rest
 *
 * DESCRIPTION:
 *  This function causes the current thread to sleep for the given
 *  number of milliseconds. However, it will still process Windows
 *  messages.
 *
 * ARGUMENTS:
 *  dwMilliSecs - Number of milliseconds to sleep for.
 *
 * AUTHOR:  C. Baumgartner, 11/29/96
 */
void Rest(DWORD dwMilliSecs)
    {

    MSG   msg;
    DWORD dwStart = GetTickCount();
    DWORD dwStop = dwStart + dwMilliSecs;

    while( dwStop > GetTickCount() )
        {
        if ( GetMessage(&msg, NULL, 0, 0) )
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
        }
    }
