/****************************************************************************
    thunks.c

    Contains code for thunking msvideo.dll from 16bit to 32bit

    Copyright (c) Microsoft Corporation 1994. All rights reserved

    Basic Structure:

        When loaded, the 16 bit DLL will try to find the corresponding
        32 bit DLL.  There are two of these.

        The 32bit videoXXXX api entry points are in AVICAP32.DLL
        The 32bit ICM code is in MSVFW32.DLL

        The thunk initialisation routine will check the code is running on
        Windows NT.  If not, no further initialisation is done.  This
        ensures that the same binary can be used on NT, and other Windows
        platforms.

        The code then attempts to access the special entry points provided
        in KERNEL.DLL for loading and calling 32 bit entry points.  If this
        link fails, no thunking can be done.

        Each of the 32 bit DLLs is loaded in turn, and GetProcAddress32 is
        called for the special thunk entry point.  If all works then two
        global flags are set (independently of each other).

        gfVideo32 == TRUE means that the videoXXXX apis can be called.
        gfICM32 == TRUE means that the 32 bit ICM code is available.

****************************************************************************/

#include <windows.h>
#define MMNOMCI
#include <win32.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <msviddrv.h>
#include <compman.h>
#include "vidthunk.h"
#include "msvideoi.h"


SZCODE    gszKernel[]             = TEXT("KERNEL");
SZCODE    gszLoadLibraryEx32W[]   = TEXT("LoadLibraryEx32W");
//SZCODE    gszFreeLibraryEx32W[]   = TEXT("FreeLibraryEx32W");
SZCODE    gszFreeLibrary32W[]     = TEXT("FreeLibrary32W");
SZCODE    gszGetProcAddress32W[]  = TEXT("GetProcAddress32W");
SZCODE    gszCallproc32W[]        = TEXT("CallProc32W");
SZCODE    gszVideoThunkEntry[]    = TEXT("videoThunk32");
SZCODE    gszVideo32[]            = TEXT("avicap32.dll");
SZCODE    gszICMThunkEntry[]      = TEXT("ICMThunk32");
SZCODE    gszICMThunkEntry2[]      = TEXT("ICMTHUNK32");
SZCODE    gszICM32[]              = TEXT("msvfw32.dll");

VIDTHUNK pvth;

#ifndef WIN32
//--------------------------------------------------------------------------;
//
//  BOOL InitThunks
//
//  Description:
//
//
//  Arguments:
//      None.
//
//  Return (BOOL):
//
//  History:
//
//
//--------------------------------------------------------------------------;
void FAR PASCAL InitThunks(VOID)
{
    HMODULE   hmodKernel;
    DWORD     (FAR PASCAL *lpfnLoadLibraryEx32W)(LPCSTR, DWORD, DWORD);
    LPVOID    (FAR PASCAL *lpfnGetProcAddress32W)(DWORD, LPCSTR);
    DWORD     (FAR PASCAL *lpfnFreeLibrary32W)(DWORD hLibModule);

    //
    //  Check if we're WOW
    //

//  if (!(GetWinFlags() & WF_WINNT)) {
//      //DPF(("Not running in WOW... returning FALSE\n"));
//      return;
//  }

    //
    //  See if we can find the thunking routine entry points in KERNEL
    //

    hmodKernel = GetModuleHandle(gszKernel);

    if (hmodKernel == NULL)
    {
	DPF(("Cannot link to kernel module... returning FALSE\n"));
        return;   // !!!!
    }

    *(FARPROC *)&lpfnLoadLibraryEx32W =
        GetProcAddress(hmodKernel, gszLoadLibraryEx32W);
    if (lpfnLoadLibraryEx32W == NULL)
    {
	DPF(("Cannot get address of LoadLibrary32... returning FALSE\n"));
        return;
    }

    *(FARPROC *)&lpfnGetProcAddress32W =
        GetProcAddress(hmodKernel, gszGetProcAddress32W);
    if (lpfnGetProcAddress32W == NULL)
    {
	DPF(("Cannot get address of GetProcAddress32... returning FALSE\n"));
        return;
    }

    *(FARPROC *)&pvth.lpfnCallproc32W =
        GetProcAddress(hmodKernel, gszCallproc32W);
    if (pvth.lpfnCallproc32W == NULL)
    {
	DPF(("Cannot get address of CallProc32... returning FALSE\n"));
        return;
    }

    // In case we need to unload our 32 bit libraries...
    *(FARPROC *)&lpfnFreeLibrary32W =
        GetProcAddress(hmodKernel, gszFreeLibrary32W);


    //
    //  See if we can get pointers to our thunking entry points
    //

    pvth.dwVideo32Handle = (*lpfnLoadLibraryEx32W)(gszVideo32, 0L, 0L);

    if (pvth.dwVideo32Handle != 0)
    {
        pvth.lpVideoThunkEntry = (*lpfnGetProcAddress32W)(pvth.dwVideo32Handle, gszVideoThunkEntry);
        if (pvth.lpVideoThunkEntry != NULL)
        {
            gfVideo32 = TRUE;
        } else {
	    DPF(("Cannot get address of video thunk entry...\n"));
            if (lpfnFreeLibrary32W)
            {
                (*lpfnFreeLibrary32W)(pvth.dwVideo32Handle);
            }
	}
    } else {
	DPF(("Cannot load Video32 DLL...\n"));
    }	

    pvth.dwICM32Handle = (*lpfnLoadLibraryEx32W)(gszICM32, 0L, 0L);

    if (pvth.dwICM32Handle != 0)
    {
        pvth.lpICMThunkEntry = (*lpfnGetProcAddress32W)(pvth.dwICM32Handle, gszICMThunkEntry);
        if (pvth.lpICMThunkEntry != NULL)
        {
	    DPF(("ICM thunks OK!!\n"));
            gfICM32 = TRUE;
        } else {
	    DPF(("Cannot get address of ICM thunk entry... trying #2\n"));
            pvth.lpICMThunkEntry = (*lpfnGetProcAddress32W)(pvth.dwICM32Handle, gszICMThunkEntry2);
            if (pvth.lpICMThunkEntry != NULL)
            {
    	        DPF(("ICM thunks OK!! (at second time of trying)\n"));
                gfICM32 = TRUE;
            } else {
        	DPF(("Cannot get address of ICM thunk entry2...\n"));
                if (lpfnFreeLibrary32W)
                {
                    (*lpfnFreeLibrary32W)(pvth.dwICM32Handle);
                }
            }
	}
    } else {
	DPF(("Cannot load ICM32 DLL...\n"));
    }	

    return;
}
#endif // !WIN32


//
// The following functions generate calls to the 32-bit side
//

DWORD FAR PASCAL videoMessage32(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2)
{
   /*
    *  Check there's something on the other side!
    */

    if (!gfVideo32) {
	DPF(("videoMessage32 - no video thunks... returning FALSE\n"));
        return DV_ERR_INVALHANDLE;
    }

   /*
    *  Watch out for hvideos being passed
    */

    if (msg == DVM_STREAM_INIT) {
        ((LPVIDEO_STREAM_INIT_PARMS)dwP1)->hVideo = hVideo;
    }

    return((DWORD)(pvth.lpfnCallproc32W)(vidThunkvideoMessage32,
                           (DWORD)hVideo,
                           (DWORD)msg,
                           (DWORD)dwP1,
                           (DWORD)dwP2,
                           pvth.lpVideoThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}


DWORD FAR PASCAL videoGetNumDevs32(void)
{
    if (!gfVideo32) {
	DPF(("videoGetNumDevs32 - no video thunks... returning FALSE\n"));
        return 0;
    }

    return((DWORD)(pvth.lpfnCallproc32W)(vidThunkvideoGetNumDevs32,
                           (DWORD)0,
                           (DWORD)0,
                           (DWORD)0,
                           (DWORD)0,
                           pvth.lpVideoThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}

DWORD FAR PASCAL videoClose32(HVIDEO hVideo)
{
   /*
    *  Check there's something on the other side!
    */

    if (!gfVideo32) {
	DPF(("videoClose32 - no video thunks... returning FALSE\n"));
        return DV_ERR_INVALHANDLE;
    }

    return((DWORD)(pvth.lpfnCallproc32W)(vidThunkvideoClose32,
                           (DWORD)hVideo,
                           (DWORD)0,
                           (DWORD)0,
                           (DWORD)0,
                           pvth.lpVideoThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}

DWORD FAR PASCAL videoOpen32(LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags)
{
    DWORD dwRetc;

    if (!gfVideo32) {
	DPF(("videoOpen32 - no video thunks... returning FALSE\n"));
        return DV_ERR_NOTDETECTED;
    }

    dwRetc = ((DWORD)(pvth.lpfnCallproc32W)(vidThunkvideoOpen32,
                           (DWORD)lphVideo,
                           (DWORD)dwDeviceID,
                           (DWORD)dwFlags,
                           (DWORD)0,
                           pvth.lpVideoThunkEntry,
                           0L, // no mapping of pointers
                           5L));

    if (dwRetc == DV_ERR_OK) {
#ifdef DEBUG
        if (Is32bitHandle(*lphVideo)) {
            //OutputDebugString("\nMSVIDEO : 32-bit handle does not fit in 16 bits!");
            DebugBreak();
        }
#endif

        *lphVideo = Make32bitHandle(*lphVideo);
    }

    return dwRetc;
}


DWORD FAR PASCAL videoGetDriverDesc32(DWORD wDriverIndex,
        			LPSTR lpszName, short cbName,
        			LPSTR lpszVer, short cbVer)
{
    if (!gfVideo32) {
	DPF(("videoGetDriverDesc32 - no video thunks... returning FALSE\n"));
        return DV_ERR_NOTDETECTED;
    }

    return (BOOL)(pvth.lpfnCallproc32W)(vidThunkvideoGetDriverDesc32,
                           (DWORD)wDriverIndex,
                           (DWORD)lpszName,
                           (DWORD)lpszVer,
                           (DWORD) MAKELONG(cbName, cbVer),
                           pvth.lpVideoThunkEntry,
                           0L, // no mapping of pointers
                           5L);	// 5 params
}



/*
 * The ICM thunking uses the same mechanism as for Video, but calls to a
 * different 32 bit DLL.
 */

BOOL FAR PASCAL ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicInfo)
{
    if (!gfICM32) {
        //OutputDebugString("ICInfo32: gfICM32 is not set - returning FALSE\n");
        return FALSE;
    }

    return ((BOOL)(pvth.lpfnCallproc32W)(compThunkICInfo32,
                           (DWORD)fccType,
                           (DWORD)fccHandler,
                           (DWORD)lpicInfo,
                           (DWORD)0,
                           pvth.lpICMThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}


LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2)
{
   /*
    *  Check there's something on the other side!
    */

    if (!gfICM32) {
#ifdef DEBUG
        OutputDebugString("ICSendMessage32: gfICM32 is not set - returning FALSE\n");
#endif
        return ICERR_BADHANDLE;
    }

    return ((LRESULT)(pvth.lpfnCallproc32W)(compThunkICSendMessage32,
                           (DWORD)hic,
                           (DWORD)msg,
                           (DWORD)dwP1,
                           (DWORD)dwP2,
                           pvth.lpICMThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}

DWORD FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode)
{
   /*
    *  Check there's something on the other side!
    */

    if (!gfICM32) {
#ifdef DEBUG
        OutputDebugString("ICOpen32: gfICM32 is not set - returning FALSE\n");
#endif
        return NULL;
    }

    return ((DWORD)(pvth.lpfnCallproc32W)(compThunkICOpen32,
                           (DWORD)fccType,
                           (DWORD)fccHandler,
                           (DWORD)wMode,
                           (DWORD)0,
                           pvth.lpICMThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}


LRESULT FAR PASCAL ICClose32(DWORD hic)
{
   /*
    *  Check there's something on the other side!
    */

    if (!gfICM32) {
#ifdef DEBUG
        OutputDebugString("ICClose32: gfICM32 is not set - returning FALSE\n");
#endif
        return ICERR_BADHANDLE;
    }

    return ((LRESULT)(pvth.lpfnCallproc32W)(compThunkICClose32,
                           (DWORD)hic,
                           (DWORD)0,
                           (DWORD)0,
                           (DWORD)0,
                           pvth.lpICMThunkEntry,
                           0L, // no mapping of pointers
                           5L));
}
