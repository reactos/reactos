/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOVIRTUALKEYCODES TRUE
#define  NOICONS	         TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM	         TRUE
#define  NOCLIPBOARD       TRUE
#define  NODRAWTEXT	      TRUE
#define  NOMINMAX	         TRUE
#define  NOOPENFILE	      TRUE
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31

#include <stdlib.h>       /* adding for _searchenv and exit crt -sdj*/
#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"
#include "video.h"                           /* mbbx 1.04: per jtfx 1.1 */
#include <ctype.h>

VOID NEAR PASCAL DestroyWindows()
{
   if(hdbmyControls)
      DestroyWindow(hdbmyControls);
   if(hdbXferCtrls)
      DestroyWindow(hdbXferCtrls);
   if(hTermWnd)
      DestroyWindow(hTermWnd);
}

/*---------------------------------------------------------------------------*/
/* WinMain() - entry point from Windows                                [mbb] */
/*---------------------------------------------------------------------------*/

int APIENTRY WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, INT cmdShow)
{
   CHAR  errmsg[115],caption[18];
   VOID  (APIENTRY *lpfnRegisterPenApp)(WORD, BOOL) = NULL;
   // -sdj was unreferenced local var: DWORD tickCount;

   readDateTime(startTimer);      

   trmParams.comDevRef = ITMNOCOM;

   if(!initConnectors(TRUE))      
      return (FALSE);

   /* Added 02/2591 w-dougw  check that all windows are created. */
   if(!initWindows(hInstance, hPrevInstance, cmdShow))
   {
      LoadString(hInstance,STR_ERRCAPTION,caption,sizeof(caption));
      LoadString(hInstance,STR_OUTOFMEMORY,errmsg,79);
      MessageBox(NULL,errmsg,caption,MB_ICONHAND|MB_SYSTEMMODAL);
      return(FALSE);
   }
   initDialogs();
   if(!setup())
   {
      LoadString(hInstance,STR_ERRCAPTION,caption,sizeof(caption));
      LoadString(hInstance,STR_OUTOFMEMORY,errmsg,79);
      MessageBox(NULL,errmsg,caption,MB_ICONHAND|MB_SYSTEMMODAL);
      return(FALSE);  
   }
   DEBOUT("Calling: %s\n","readCmdLine()");
   readCmdLine(lpszCmdLine);                 /* mbbx 2.00 ... */
   DEBOUT("Outof: %s\n","readCmdLine()");

   DEBOUT("Calling: %s\n","PrintFileInit()");
   PrintFileInit(); /* jtfterm */
   DEBOUT("Outof: %s\n","PrintFileInit()");

   /* Register as a good little pen-windows app
    */
   /* NOTE**** have to confirm that this is the way to go GetSystemMet RC-sdj*/
   /* added typecasting of (HANDLE) to param 1 */
   if (lpfnRegisterPenApp = (VOID *)GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS),
         "RegisterPenApp"))
       (*lpfnRegisterPenApp)(1, TRUE);

   DEBOUT("Calling: %s\n","mainProcess()");
   mainProcess();                            /* now load _WINMAIN segment */
   DEBOUT("Outof: %s\n","mainProcess()");

   /* Make sure to de-register if you register
    */
   if (lpfnRegisterPenApp)
       (*lpfnRegisterPenApp)(1, FALSE);

   PrintFileShutDown(); /* jtfterm */

   DestroyWindow(hdbXferCtrls);        /* rjs swat */
   DestroyWindow(hdbmyControls);       /* jtf 3.33 */
   DestroyWindow(hItWnd);              /* rjs swat */

   freeItResources();

   exit(msg.wParam);
/*ExitProcess((DWORD)msg.wParam);  should this be used instead of exit()?-sdj*/
}


/* was
BOOL registerIt(hInstance)
HANDLE   hInstance;
*/


/*---------------------------------------------------------------------------*/
/* initWndClass() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL initWndClass()                          /* mbbx 1.04 ... */
{
   WNDCLASS    wndClass;

   wndClass.style          = CS_HREDRAW | CS_VREDRAW;
   wndClass.lpfnWndProc    = DC_WndProc;
   wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = 0;
   wndClass.hInstance      = hInst;
   wndClass.hIcon          = (HICON) NULL; 
   wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
   wndClass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE+1);    /* mbbx 2.00 ... */
   wndClass.lpszMenuName   = (LPSTR) szAppName_private;
   wndClass.lpszClassName  = (LPSTR) szAppName_private;

   if(!RegisterClass((LPWNDCLASS) &wndClass))   /* register DYNACOMM class */
      return(FALSE);

   wndClass.style          = CS_DBLCLKS; /* jtf 3.21 | CS_HREDRAW | CS_VREDRAW; */
   wndClass.lpfnWndProc    = TF_WndProc;
   wndClass.hIcon          = (HICON) NULL;
   wndClass.hCursor        = LoadCursor(NULL, IDC_IBEAM);
   wndClass.hbrBackground  = (HBRUSH) NULL;
   wndClass.lpszMenuName   = (LPSTR) NULL;
   wndClass.lpszClassName  = (LPSTR) DC_WNDCLASS;

   if(!RegisterClass((LPWNDCLASS) &wndClass))   /* register TERMINAL class */
      return(FALSE);

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* initPort () - Initialize hTE text rectangles  and init thePort            */
/* thePort is always 0 or an active DC of hTermWnd                           */
/* portLocks is count of number of un'releasePort'ed getPort calls           */

extern BOOL insertionPoint;

VOID initPort ()
{
   insertionPoint = TRUE;
   thePort   = 0;
   portLocks = 0;
   hTE.active = TRUE;
   hTE.selStart = hTE.selEnd = MAXROWCOL;

   /* Added 02/22/91 for win 3.1 common dialog interface */
   hDevNames = NULL;
   hDevMode  = NULL;
}


/*---------------------------------------------------------------------------*/
/* initIcon()                                                                */
/*---------------------------------------------------------------------------*/

extern WNDPROC lpNextFlash;                  /* mbbx 1.04: defined in ICON.C ... */


VOID initIcon()
{
   // -sdj was unreferenced var:  CHAR	temp[10];

   icon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICO_DYNACOMM));

   icon.flash = FALSE;

   icon.dx = GetSystemMetrics(SM_CXICON)/16;
   icon.dy = GetSystemMetrics(SM_CYICON)/16;

   lpNextFlash = MakeProcInstance(nextFlash, hInst);
}



/*---------------------------------------------------------------------------*/
/* createWindows() - Determine tube size and create all Windows.             */
/*---------------------------------------------------------------------------*/

BOOL createWindows(INT   cmdShow)
{
   INT      ndx;
   // -sdj was unreferenced local var: HMENU	hSysMenu;
   BYTE     work[80]; // -sdj was unrefenced var: , work1[80], work2[80];

   LoadString(hInst, STR_APPNAME, (LPSTR) work, MINRESSTR);
   strcpy(work+strlen(work), " - ");
   LoadString(hInst, STR_TERMINAL, (LPSTR) work+strlen(work), MINRESSTR);

   //sdj: using the values from the registry instead of hardcoding it

   if(!(hItWnd = CreateWindow((LPSTR) szAppName_private,
                         (LPSTR) work,
                         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			 WindowXPosition,
			 WindowYPosition,
			 WindowWidth,
			 WindowHeight, // was hardcoded to: 10,10,400,400, win31:CW_USEDEFAULT, 0, CW_USEDEFAULT, 0
                         (HWND) NULL,
                         (HMENU) NULL,
                         (HANDLE) hInst,
                         (LPSTR) NULL)))
	return(FALSE);

   LoadString(hInst, STR_INI_MAXIMIZED, (LPSTR) work, MINRESSTR);
   if(!GetProfileInt((LPSTR) szAppName_private, (LPSTR) work, 0) || 
      (cmdShow == SW_SHOWMINNOACTIVE) || (cmdShow == SW_SHOWMINIMIZED) || (cmdShow == SW_MINIMIZE))
   {
      ShowWindow(hItWnd, cmdShow);
   }
   else
      ShowWindow(hItWnd, SW_SHOWMAXIMIZED);

   if(!(hdbmyControls = CreateDialog(hInst, getResId(IDDBMYCONTROLS), 
    hItWnd, (WNDPROC)lpdbmyControls)))
      return(FALSE);
   for(ndx = 0; ndx < DCS_NUMFKEYS; ndx += 1)   /* mbbx 2.00: moved from hidemyControls... */
   {
      fKeyHandles[ndx] = GetDlgItem(hdbmyControls, IDFK1 + ndx);
      DEBOUT("createWindows: fKeyHandles[]=%lx from GetDlgItem()\n",fKeyHandles[ndx]);
   }
   LoadString(hInst, STR_TERMINAL, (LPSTR) work, MINRESSTR);
   if(!(hTermWnd = CreateWindow((LPSTR) DC_WNDCLASS,
                           (LPSTR) work,
                           /* Removed WS_THICKFRAME jtf 3.21 */
                           WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPSIBLINGS | CS_BYTEALIGNWINDOW,
                           0, 0, 0, 0,
                           hItWnd,
                           (HMENU) NULL,
                           (HANDLE) hInst,
                           (LPSTR) NULL))) 
	return(FALSE);


   if(!(hdbXferCtrls = CreateDialog(hInst, getResId(IDDBXFERCTRLS),
                                    hTermWnd, (WNDPROC)lpdbmyControls)))
   {
	return(FALSE);
   }

	/* mbbx 1.04 */
   xferCtlStop = GetDlgItem(hdbXferCtrls, IDSTOP);    /* mbbx 2.00: moved from hidemyControls()... */
   xferCtlPause = GetDlgItem(hdbXferCtrls, IDPAUSE);
   xferCtlScale = GetDlgItem(hdbXferCtrls, IDSCALE);
   showXferCtrls(0);
}


/*---------------------------------------------------------------------------*/
/* sizeWindows() -                                                           */
/*---------------------------------------------------------------------------*/

VOID sizeWindows()
{
   RECT  fKeysRect;
   RECT  ctrlsRect;
   RECT  termRect;

   setDefaultFonts();

   GetWindowRect(hdbmyControls, (LPRECT) &fKeysRect);
   GetWindowRect(fKeyHandles[0], (LPRECT) &ctrlsRect);   /* mbbx 2.00: fkeys... */
   MoveWindow(hdbmyControls, 0, fKeysRect.top, fKeysRect.right, 
              fKeysHeight = ((ctrlsRect.bottom - ctrlsRect.top) * 2), FALSE);

   GetClientRect(hItWnd, (LPRECT) &fKeysRect);  /* mbbx 2.00: may not init maximized... */
   sizeFkeys(MAKELONG(fKeysRect.right, fKeysRect.bottom));


   GetWindowRect(hdbXferCtrls, (LPRECT) &ctrlsRect);  /* mbbx 1.04: fkeys... */
   ctrlsHeight = ctrlsRect.bottom - ctrlsRect.top;

   initChildSize(&termRect);
   MoveWindow(hTermWnd, 0, 0, termRect.right, termRect.bottom, FALSE); /* jtf 3.21 */
}
/*---------------------------------------------------------------------------*/
/* initWindows() -                                                           */
/*---------------------------------------------------------------------------*/

BOOL initWindows(HANDLE   hInstance, HANDLE   hPrevInstance, INT      cmdShow)
{

   BYTE     str[MINRESSTR];

   hInst = hInstance;

   /* Added 02/26/91 for window existence */
   hItWnd = NULL;
   hdbmyControls = NULL;
   hTermWnd = NULL;
   hdbXferCtrls = NULL;
   hEdit = NULL;
   fKeyHdl = NULL;

   LoadString(hInst, STR_APPNAME_PRIVATE, (LPSTR) szAppName_private, 20);
   LoadString(hInst, STR_APPNAME, (LPSTR) szAppName, 20);
   LoadString(hInst, STR_DEVELOPER, (LPSTR) szMessage, 80);
   LoadString(hInst, STR_NOMEMORY,(LPSTR)NoMemStr,sizeof(NoMemStr)); /* rjs msoft ??? */


   //sdj: lets try and see if the x,y,width,height are in the win.ini
   //sdj: if so use them and save the values in the global variables
   //sdj: so that these can be used by the createwindow call during init
   //sdj: and can be saved using getwindowrect just before the user exits
   //sdj: the terminal app.

   //sdj: defaults are	 10,10,400,400. win31 had: CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,

   LoadString(hInst, STR_INI_XPOSITION, (LPSTR) str, MINRESSTR);
   WindowXPosition = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, 10);

   LoadString(hInst, STR_INI_YPOSITION, (LPSTR) str, MINRESSTR);
   WindowYPosition = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, 10);

   LoadString(hInst, STR_INI_WIDTH, (LPSTR) str, MINRESSTR);
   WindowWidth = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, 400);

   LoadString(hInst, STR_INI_HEIGHT, (LPSTR) str, MINRESSTR);
   WindowHeight = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, 400);




   setDefaultAttrib(TRUE);                   /* mbbx 1.04: ...szAppName loaded */

   if(!hPrevInstance)
   {
      if(!initWndClass())                    /* mbbx 1.04 ... */
          return(FALSE);
   }

   if(!(lpdbmyControls = MakeProcInstance(dbmyControls, hInst)))
     return(FALSE);


   initPort();
   initIcon();

   theBrush   = GetStockObject (WHITE_BRUSH);
   blackBrush = GetStockObject (BLACK_BRUSH);

   if(!createWindows(cmdShow))
     return(FALSE);

   sizeWindows();

   maxScreenLine = MAXSCREENLINE;            /* rjs moved from size windows */

   hMenu = GetMenu(hItWnd);

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* initDialogs() - Do all dialogbox initialization. [scf]                    */
/*---------------------------------------------------------------------------*/

VOID initDialogs()                           /* mbbx: remove ALL of these... */
{
   lpdbDialing    = MakeProcInstance(dbDialing, hInst);
}


/*---------------------------------------------------------------------------*/
/* dbPortInit() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbPortInit(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
#ifdef WIN32
   WORD	temp_wParam;	
#endif

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);
      initComDevSelect(hDlg, ITMCONNECTOR, TRUE);
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         break;

      case ITMCONNECTOR:
         if(GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
            break;
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   trmParams.comPortRef = getComDevSelect(hDlg, ITMCONNECTOR, &trmParams.newDevRef);
   trmParams.fResetDevice = TRUE;
#ifdef WIN32
   /* code in next block passes address of wParam to function*/
   /* so we pass temp variable instead, since we need extract ID from wParam under WIN32*/
   temp_wParam = GET_WM_COMMAND_ID(wParam, lParam);
#endif

#ifdef ORGCODE
   trmParams.comPortRef = getComDevSelect(hDlg, ITMCONNECTOR, (BYTE *) &wParam);
#else
   trmParams.comPortRef = getComDevSelect(hDlg, ITMCONNECTOR, (BYTE *) &temp_wParam);
#endif
   resetSerial(&trmParams, TRUE, TRUE, FALSE);   /* slc swat */

   if(trmParams.comDevRef != trmParams.newDevRef)
   {
      exitSerial();
      return(TRUE);
   }
   exitSerial();

#ifdef ORGCODE
   EndDialog(hDlg, (INT) getComDevSelect(hDlg, ITMCONNECTOR, (BYTE *) &wParam));
#else
   if (!(getComDevSelect(hDlg, ITMCONNECTOR, (BYTE *) &temp_wParam)))
    {
    EndDialog(hDlg,TRUE);
    }
   else{
    EndDialog(hDlg, (INT) getComDevSelect(hDlg, ITMCONNECTOR, (BYTE *) &temp_wParam));
       }
#endif
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* setProfileExtent() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

/* mbbx: 1.01 - moved from itutil1.c */

BOOL NEAR setProfileExtent(BYTE  *section, BYTE  *extent)
{
   BOOL  setProfileExtent = FALSE;
   BYTE  str[80];
   BYTE  temp[80];

   if(!GetProfileString((LPSTR) section, (LPSTR) extent, (LPSTR) NULL_STR, (LPSTR) temp, 80))
   {
      strcpy(temp, extent);
      AnsiLower((LPSTR) temp);
      sprintf(str, "%s.exe ^.%s", szAppName_private, temp);
      AnsiLower((LPSTR) str);
      WriteProfileString((LPSTR) section, (LPSTR) temp, (LPSTR) str);
      setProfileExtent = TRUE;
   }

   return(setProfileExtent);
}


/*---------------------------------------------------------------------------*/
/* initFileDocData() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR initFileDocData(FILEDOCTYPE fileType, WORD strResID,BYTE  *fileExt,BYTE *szSection)   /* mbbx 2.00 ... */
{
   BYTE  work1[MINRESSTR], work2[80];

   LoadString(hInst, strResID, (LPSTR) work1, MINRESSTR);
   GetProfileString((LPSTR) szAppName_private, (LPSTR) work1, (LPSTR) NULL_STR, (LPSTR) work2, 80);

   getDataPath(fileType, fileDocData[fileType].filePath, work2);

   strcpy(fileDocData[fileType].fileExt, fileExt);
   if(!getFileType(work2, fileDocData[fileType].fileExt))
      strcpy(work2, fileDocData[fileType].fileExt);

   strcpy(fileDocData[fileType].fileName, fileDocData[fileType].fileExt+1);

   if(work2[strlen(work2)-1] != '*')
      return(setProfileExtent(szSection, fileDocData[fileType].fileExt+3));

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* initProfileData() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

#define DEFBUFFERLINES           100         /* mbbx 1.10... */

VOID initProfileData()                       /* mbbx: 1.01 ... */
{
   BYTE     str[MINRESSTR], str2[MINRESSTR], portName[16];
   INT      ndx;
   // -sdj eas unreferenced local var: FARPROC	lpdbPortInit;
   BOOL     notify;

   LoadString(hInst, STR_INI_PORT, (LPSTR) str, MINRESSTR);
   if(!GetProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) NULL_STR, (LPSTR) portName, 5))
   {
      trmParams.comDevRef = ITMNOCOM;        /* jtf 3.33 */
      trmParams.speed     = 1200;            /* jtf 3.33 */
      trmParams.dataBits  = ITMDATA8;        /* jtf 3.33 */
      trmParams.stopBits  = ITMSTOP1;        /* jtf 3.33 */
      trmParams.parity    = ITMNOPARITY;     /* jtf 3.33 */
      if((ndx = doSettings(IDDBPORTINIT, dbPortInit)) != -1)   /* mbbx 2.01.10 ... */
      {
         LoadString(hInst, (ndx > 0) ? STR_COM : STR_COM_CONNECT, (LPSTR) str2, MINRESSTR);
         sprintf(portName, str2, ndx);
         WriteProfileString((LPSTR) szAppName_private, (LPSTR) str, (LPSTR) portName);
      }
   }


   LoadString(hInst, STR_INI_SWAP, (LPSTR) str, MINRESSTR);
   if((ndx = GetProfileInt((LPSTR) szAppName_private, (LPSTR) str, 0)) > 0)
      *taskState.string = sprintf(taskState.string+1, "%d", SetSwapAreaSize(ndx));

   LoadString(hInst, STR_INI_INTL, (LPSTR) str, MINRESSTR);
   LoadString(hInst, STR_INI_IDATE, (LPSTR) str2, MINRESSTR);
   intlData.iDate = GetProfileInt((LPSTR) str, (LPSTR) str2, 0);
   LoadString(hInst, STR_INI_SDATE, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str, (LPSTR) str2, (LPSTR) "/", (LPSTR) intlData.sDate, 2);
   LoadString(hInst, STR_INI_ITIME, (LPSTR) str2, MINRESSTR);
   intlData.iTime = GetProfileInt((LPSTR) str, (LPSTR) str2, 0);
   LoadString(hInst, STR_INI_STIME, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str, (LPSTR) str2, (LPSTR) ":", (LPSTR) intlData.sTime, 2);
   LoadString(hInst, STR_INI_S1159, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str, (LPSTR) str2, (LPSTR) "AM", (LPSTR) intlData.s1159, 4);
   LoadString(hInst, STR_INI_S2359, (LPSTR) str2, MINRESSTR);
   GetProfileString((LPSTR) str, (LPSTR) str2, (LPSTR) "PM", (LPSTR) intlData.s2359, 4);

   LoadString(hInst, STR_INI_EXTENSIONS, (LPSTR) str, MINRESSTR);
   notify = initFileDocData(FILE_NDX_SETTINGS, STR_INI_SETTINGS, SETTINGS_FILE_TYPE, str);
   if(initFileDocData(FILE_NDX_TASK, STR_INI_TASK, TASK_FILE_TYPE, str))
      notify = TRUE;
   if(initFileDocData(FILE_NDX_SCRIPT, STR_INI_SCRIPT, SCRIPT_FILE_TYPE, str))
      notify = TRUE;
   if(initFileDocData(FILE_NDX_MEMO, STR_INI_MEMO, MEMO_FILE_TYPE, str))
      notify = TRUE;
   if(initFileDocData(FILE_NDX_DATA, STR_INI_DATA, DATA_FILE_TYPE, str))
      notify = TRUE;
   if(notify)
#ifdef ORGCODE
      SendMessage(0xFFFF, WM_WININICHANGE, 0, (LONG) ((LPSTR) str));
#else
      SendMessage((HWND)0xFFFFFFFF, WM_WININICHANGE, 0, (LONG) ((LPSTR) str));
#endif
}

// sdj: added this code to get the list of comm ports which the serial driver
// sdj: recognized while loading the system.
// sdj: if this number of ports is > 9 then it is set to 9
// sdj: and if this info cannot be retrieved this this is set to 4 as before


void GetNumCommPortsInSystem(LPBYTE pMaxCom)
{

int   i,j, nEntries;
DWORD	dwSize, dwBufz;
DWORD	dwType;
HKEY	 hkey;
char	 szSerial[40];
char	 szCom[40];
char	 szCom2[40];
char	 szFormat[200];
BYTE	tmp1[TMPNSTR+1];
BYTE	tmp2[TMPNSTR+1];


//  NT Registry keys to find COM port to Serial Device mapping

char szRegSerialMap[] = "Hardware\\DeviceMap\\SerialComm";

  //////////////////////////////////////////////////////////////////////
  //	Get list of valid COM ports from DEVICEMAP in registry
  //////////////////////////////////////////////////////////////////////

*pMaxCom = 0;
i = 0;


if (!RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegSerialMap,
						     0L, KEY_READ, &hkey))
	 {
	     dwBufz = sizeof(szSerial);
	     dwSize = sizeof(szCom);
	     nEntries = i = 0;

	     while (!RegEnumValue (hkey, i++, szSerial, &dwBufz,
				     NULL, &dwType, szCom, &dwSize))
	     {
		 if (dwType != REG_SZ)
		     continue;

		++nEntries;

//		arComNumAndName[nEntries].Index = nEntries;
//		strcpy(arComNumAndName[nEntries].PortName,szCom);
//
		 dwSize = sizeof(szCom);
		 dwBufz = sizeof(szSerial);

	     if (nEntries > 19) break; // lets not bother with >= 20 entries

	     }
	    RegCloseKey (hkey);
	    *pMaxCom = nEntries;
	    arComNumAndName[0].Index = nEntries;

	 }
	 else
	{
	// sdj: could not find the list of comm ports in the registry, so
	// sdj: stick with the original terminal hardcoded value of com1-4
	LoadString(hInst, STR_NOCOMMPORTS, (LPSTR) tmp1, TMPNSTR);	  /* mbbx 1.00 */
	LoadString(hInst, STR_ERRCAPTION, (LPSTR) tmp2, TMPNSTR);
	MessageBox(NULL, (LPSTR) tmp1, (LPSTR)tmp2, MB_OK | MB_APPLMODAL);
	*pMaxCom = 0;
	 }

//sdj: if maxcomm is > 9 set it to 9
//
if (*pMaxCom > 9) *pMaxCom = 9;
//
//

strcpy(szCom,"COM%d");

for (i=1,j=1; i<= 9; i++)
    {
    sprintf(szCom2,szCom,i);
    if (QueryDosDevice(szCom2,szFormat,400))
	{
	arComNumAndName[j].Index = j;
	strcpy(arComNumAndName[j].PortName,szCom2);
	j++;
	}
    }


}

/*---------------------------------------------------------------------------*/
/* setup() - Reset all varibles, read settings file & emulation.       [scf] */
/*---------------------------------------------------------------------------*/

BOOL setup()                                 /* mbbx 2.00: no cmd line... */
{
   // -sdj was unreferenced local var: BYTE path[PATHLEN+1];
   // -sdj was unreferenced local var: BYTE tmp1[TMPNSTR+1];
   // -sdj was unreferenced local var: INT	ndx;
   SetRect ((LPRECT) &cursorRect, 0, 0, 0, 0);
   vScrollShowing = TRUE;
   serNdx         = 0;
   cursorTick     = -1l;
   cursBlinkOn    = FALSE;
   cursorOn       = TRUE;
   activCursor    = 1;
   prtFlag        = FALSE;
   useScrap       = FALSE;
   copiedTable    = FALSE;
   *fKeyStr       = 0;                       /* mbbx 2.00: fKeySeq... */
   fKeyNdx        = 1;
   scrapSeq       = FALSE;

   xferFlag       = XFRNONE;
   xferPaused     = FALSE;
   xferBreak      = FALSE;                   /* mbbx 2.00: xfer ctrls */
   xferEndTimer   = 0;
   xferWaitEcho   = FALSE;
   xferViewPause  = 0;                       /* mbbx: auto line count */
   xferViewLine   = 0;
   xferPSChar     = 0;                           /* mbbx 1.02: packet switching */
   *strRXErrors   =
   *strRXBytes    =
   *strRXFname    =
   *strRXFork     = 0;
   taskInit();
   keyMapInit();                             /* mbbx 1.04: keymap */

   debugFlg       = FALSE;                   /* how does this get enabled??? */

   mdmOnLine      = FALSE;
   dialing        = FALSE;
   answerMode     = FALSE;
   protectMode    = FALSE;                   /* mbbx: emulation state */
   KER_getflag    = FALSE;

   gotCommEvent   = TRUE;
   sPort	  = NULL;
   bPortIsGood	  = FALSE;		     /* it is not yet opened */
					     //-sdj for telnet-quit processing
   bPortDisconnected = TRUE;		     /* this will ensure that reads dont
						occur before resetserial inits this to
						FALSE, it will be set to true by checkcommevent
						if for some reason, wait(getlasterror) is not
						IO_PENDING */

   bgOutStandingWrite = FALSE;               /* slc swat */

   //MaxComPortNumberInMenu = 5;
						/* this will get set to proper
						value when the win.ini is
						looked at for COMn entries,
						but during init, this needs
						to be set to > COM1 -sdj:04/21/92 */

   GetNumCommPortsInSystem(&MaxComPortNumberInMenu);

   if((hemulKeyInfo = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) SIZEOFEMULKEYINFO)) == NULL)
      return(FALSE);


   initProfileData();                        /* mbbx: 1.01 */

   hTE.hText = NULL;
   DEBOUT("Calling: %s\n","setDefaults()");
   setDefaults();
   DEBOUT("Outof: %s\n","setDefaults()");

   DEBOUT("Calling: %s\n","setFKeyLevel()");
   setFKeyLevel(1, FALSE);           /*   jtfterm */
   DEBOUT("Outof: %s\n","setFKeyLevel()");

   DEBOUT("Calling: %s\n","termInitSetup()");
   termInitSetup(NULL);
   DEBOUT("Outof: %s\n","termInitSetup()");

   strcpy(szMessage, szAppName);
   return(TRUE);
}

/*---------------------------------------------------------------------------*/
/* readCmdLine() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

BOOL fileDocExist(WORD  fileType, BYTE  *filePath)
{
   BOOL  fileDocExist;
   BYTE  savePath[PATHLEN], testPath[PATHLEN];
   BYTE  OEMname[STR255];

   strcpy(savePath, filePath);
   getDataPath(fileType, testPath, savePath);
   strcat(testPath, savePath);

   // JYF -- replace below lines with followin if()
   //        to remove the use of AnsiToOem()
   //
   //AnsiToOem((LPSTR) testPath, (LPSTR) OEMname); /* jtf 3.20 */
   //if(fileDocExist = fileExist(OEMname)) /* jtf 3.20 */

   if (fileDocExist = fileExist(testPath))
      strcpy(filePath, testPath);


   return(fileDocExist);
}


WORD NEAR getFileDocType(BYTE  *filePath)
{
   BYTE  fileExt[16];
   WORD  fileType;

   *fileExt = 0;
   if(!getFileType(filePath, fileExt))
   {
      forceExtension(filePath, NO_FILE_TYPE+2, FALSE);
      if(fileDocExist(FILE_NDX_DATA, filePath) || fileDocExist(FILE_NDX_SETTINGS, filePath))
         return(FILE_NDX_SETTINGS); /* jtf 3.11 */
   }

   for(fileType = FILE_NDX_SETTINGS; fileType <= FILE_NDX_MEMO; fileType += 1)
   {
      if(*fileExt == 0)
         forceExtension(filePath, fileDocData[fileType].fileExt+2, TRUE);
      else if((fileType < FILE_NDX_MEMO) && (strcmp(fileDocData[fileType].fileExt+2, fileExt) != 0))
         continue;

      if(fileDocExist(FILE_NDX_DATA, filePath) || fileDocExist(fileType, filePath) || (*fileExt != 0))
         return(fileType);
   }

   return(FILE_NDX_DATA);
}


BOOL NEAR initTermFile(BYTE  *filePath)
{
   getDataPath(FILE_NDX_SETTINGS, fileDocData[FILE_NDX_SETTINGS].filePath, filePath);

   LoadString(hInst, STR_TERMINAL, (LPSTR) termData.title, MINRESSTR);
                                             /* mbbx 2.00: no forced extents... */
   return(termFile(fileDocData[FILE_NDX_SETTINGS].filePath, filePath, 
                   fileDocData[FILE_NDX_SETTINGS].fileExt, termData.title, TF_DEFTITLE));
}


VOID NEAR readCmdLine(LPSTR lpszCmdLine)
{
   INT   ndx, ndx2;
   BYTE  filePath[PATHLEN];
   BYTE  tmpFilePath[PATHLEN];
   INT   nEditWnd = 0;
   BYTE  OEMname[STR255];              /* jtf 3.20 */
   BYTE  work[STR255];                 /* jtf 3.28 */
   BYTE  work1[STR255];                /* jtf 3.28 */
   INT   testFlag;
   
   saveFileType = FILE_NDX_SETTINGS; /* jtf 3.11 */

   AnsiUpper(lpszCmdLine);
   for(ndx = 0; lpszCmdLine[ndx] != 0; )     /* mbbx 2.00 ... */
   {
      while(lpszCmdLine[ndx] == 0x20)
         ndx++;

      if(lpszCmdLine[ndx] == 0)
         break;

      for(ndx2 = 0; (filePath[ndx2] = lpszCmdLine[ndx]) != 0; ndx2 += 1)
      {
         ndx += 1;
         if(filePath[ndx2] == 0x20)
         {
            filePath[ndx2] = 0;
            break;
         }
      }
      strcpy(work1,filePath);
      
      if(!activTerm)
         initTermFile(filePath);
   }

   ndx = lstrlen(lpszCmdLine);
   while(ndx)
   {
      ndx--;
      if(isspace(lpszCmdLine[ndx]))
         lpszCmdLine[ndx] = 0x00;
   }

   if ((!activTerm) && (lstrlen(lpszCmdLine) > 0))
   {
      LoadString(hInst, STRERRNOFILE, (LPSTR) work, STR255-1); /* jtf 3.15 */
      strcpy(filePath,work1);
      forceExtension(filePath, SETTINGS_FILE_TYPE+2, FALSE);
      sprintf(work1, work, filePath);
      testFlag = MessageBox(GetActiveWindow(), (LPSTR) work1, (LPSTR) szAppName, MB_OKCANCEL);

      if (testFlag==IDOK)
      {
         if (filePath[1]==':')
         {
            filePath[0]='A';
         }
         else
         {
            strcpy(work,filePath);
            strcpy(filePath,"A:");
            strcpy(filePath+2,work);
         }
      
         initTermFile(filePath);
      }
   }
   if(!activTerm)                            /* mbbx 2.00: term init... */
   {
      LoadString(hInst, STR_AUTOLOAD, (LPSTR) filePath, PATHLEN); /* jtf 3.17 */

      // JYF -- replace below two lines with the following if()
      //        to remove the use of AnsiToOem()
      //
      //AnsiToOem((LPSTR) filePath, (LPSTR) OEMname); /* jtf 3.20 */
      //if (fileExist(OEMname)) /* jtf 3.20 */

      if (fileExist(filePath))
         initTermFile(filePath);
      else
         {
         _searchenv( filePath, "PATH", tmpFilePath );
         if(strlen(tmpFilePath)>0)
            initTermFile(tmpFilePath);
         }

      if(!activTerm)
      {
         if((nEditWnd -= 1) >= 0)
            termData.flags |= TF_HIDE;
         else
            saveFileType = FILE_NDX_SETTINGS;

         activTerm = TRUE;
         resetSerial(&trmParams, TRUE, TRUE,0);

         if(!(termData.flags & TF_HIDE))
            showTerminal(TRUE, TRUE);
      }
   }
   if(!IsIconic(hItWnd))   /* rjs bugs 015 */
      sizeTerm(0L); /* jtf 3.21 */
}


/*---------------------------------------------------------------------------*/
/* freeItResources()- Free up all windows resource b/4 back to DOS executive.*/
/*                    Internal house keeping.  Note: Close that serial port. */
/*---------------------------------------------------------------------------*/

VOID freeItResources()
{
   // -sdj was unreferenced local var: INT	ndx;

   exitSerial();
   keyMapCancel();                           /* mbbx 1.04: keymap */

   DeleteObject(hTE.hFont);
   clearFontCache();                         /* mbbx 2.00: redundant code... */

   GlobalFree(hTE.hText);
   GlobalFree(hemulKeyInfo);

   FreeProcInstance(lpdbmyControls);         /* mbbx 1.04: per jtfx 1.1 ... */
   FreeProcInstance(lpdbDialing);
   FreeProcInstance(lpNextFlash);
}
