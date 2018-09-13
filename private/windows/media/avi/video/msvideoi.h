/****************************************************************************/
/*                                                                          */
/*        MSVIDEOI.H - Internal Include file for Video APIs                 */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1992, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifdef BUILDDLL
#undef WINAPI
#define WINAPI FAR PASCAL _loadds
#endif

/****************************************************************************

                   Digital Video Driver Structures

****************************************************************************/

#define MAXVIDEODRIVERS 10

/****************************************************************************

                            Globals

****************************************************************************/

extern UINT      wTotalVideoDevs;                  // total video devices
// The module handle is used in drawdib to load strings from the resource file
extern HINSTANCE ghInst;                           // our module handle
extern BOOL gfIsRTL;
extern SZCODE szVideo[];
extern SZCODE szSystemIni[];
extern SZCODE szDrivers[];

// If the following structure changes, update AVICAP and AVICAP.32 also!!!
typedef struct tCapDriverInfo {
   TCHAR szKeyEnumName[MAX_PATH];
   TCHAR szDriverName[MAX_PATH];
   TCHAR szDriverDescription[MAX_PATH];
   TCHAR szDriverVersion[80];
   TCHAR szSoftwareKey[MAX_PATH];
   DWORD dnDevNode;         // Set if this is a PnP device
   BOOL  fOnlySystemIni;    // If the [path]drivername is only in system.ini
   BOOL  fDisabled;         // User has disabled driver in the control panel
   BOOL  fActive;           // Reserved
} CAPDRIVERINFO, FAR *LPCAPDRIVERINFO;

/* internal video function prototypes */

#ifdef _WIN32
/*
 * don't lock pages in NT
 */
#define HugePageLock(x, y)		(TRUE)
#define HugePageUnlock(x, y)
#else

BOOL FAR PASCAL HugePageLock(LPVOID lpArea, DWORD dwLength);
void FAR PASCAL HugePageUnlock(LPVOID lpArea, DWORD dwLength);

#define videoGetErrorTextW videoGetErrorText

#endif

/****************************************************************************
****************************************************************************/
