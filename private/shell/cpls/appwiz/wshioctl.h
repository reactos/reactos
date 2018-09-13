/*****************************************************************************
 *
 *  (C) Copyright MICROSOFT Corp., 1994, 1995
 *
 *  Title:      WSHOICTL.H - IOCTL interface for Wshell
 *
 *  Version:    4.00
 *
 *  Date:       30-Nov-1988
 *
 *  Author:     RAL
 *
 *----------------------------------------------------------------------------
 *
 *  Change log:
 *
 *     DATE     REV                 DESCRIPTION
 *  ----------- --- ----------------------------------------------------------
 *  01-Aug-1994 RAL Original
 *  05-Apr-1995 [stevecat] NT and Unicode port
 *
 *****************************************************************************/

#ifndef _WSHIOCTL_H
#define _WSHIOCTL_H


#define SHELLFILENAME TEXT("\\\\.\\SHELL")

//
// Flags for _SHELL_SuggestSingleMSDOSMode
//
// SSAMFLAG_KILLVM
//        A fatal application error has occurred.  Display a warning box
//        unconditionally.  Regardless of the answer, terminate the VM.
//        If this bit is set, the call does not return.
//
//  SSAMFLAG_TIMER
//        Not used.  Sorry.
//
//  SSAM_REQREALMODE
//        App requires *real* mode, not V86 mode, not EMM stuff, not
//        QEMM.  Just pure unadulterated real mode.  Also known as
//        SSAM_COMANCHE, because Comanche does an "lgdt" to enter
//        protected mode without checking if it is safe to do so.
//        This flag is inspected by AppWiz to decide how to set up
//        the config.sys and autoexec.bat.
//
//  SSAM_KILLUNLESSTOLD
//        Suggest Single MS-DOS mode (unless suppressed via PIF), and
//        if the answer is "Okay", then kill the VM.  If the user
//        says, "Keep running", then let it stay.
//
//  SSAM_FROMREGLIST
//        This app was run from a command prompt, triggered by registry
//        settings.  Just re-execute it in its own VM so that APPS.INF
//        settings will take effect.
//
//  SSAM_FAILEDAPI
//        This app just made an API call that was unsuccessful or
//        unsupported.  If the app terminates within 0.1 second,
//        then suggest single-app mode.  If the app continues
//        execution, then don't suggest.
//

#define SSAMFLAG_KILLVM         0x0000001
#define SSAMFLAG_TIMER          0x0000002
#define SSAMFLAG_REQREALMODE    0x0000004
#define SSAMFLAG_KILLUNLESSTOLD 0x0000008
#define SSAMFLAG_FROMREGLIST    0x0000010
#define SSAMFLAG_FAILEDAPI      0x0000020

//
//  IOCTL codes
//
#define WSHIOCTL_GETVERSION       0
#define WSHIOCTL_BLUESCREEN       1
#define WSHIOCTL_GET1APPINFO      2
#define WSHIOCTL_SIGNALSEM        3
#define WSHIOCTL_MAX              4        /* Remember, _MAX = _LIMIT + 1 */

//
//  Result codes
//
#define SSR_CONTINUE       0
#define SSR_CLOSEVM        1
#define SSR_KILLAPP        2

//
//  Sizes for strings
//
#define MAXVMTITLESIZE      32
#define MAXVMPROGSIZE       64
#define MAXVMCMDSIZE        64
#define MAXVMDIRSIZE        64
#define MAXPIFPATHSIZE      260

typedef struct _SINGLEAPPSTRUC {    /* shex */

        DWORD        SSA_dwFlags;
        DWORD        SSA_VMHandle;
        DWORD        SSA_ResultPtr;
        DWORD        SSA_Semaphore;
        TCHAR        SSA_PIFPath[MAXPIFPATHSIZE];
        TCHAR        SSA_VMTitle[MAXVMTITLESIZE];
        TCHAR        SSA_ProgName[MAXVMPROGSIZE];
        TCHAR        SSA_CommandLine[MAXVMCMDSIZE];
        TCHAR        SSA_CurDir[MAXVMCMDSIZE];

} SINGLEAPPSTRUC;

//
// Structures for WSHIOCTL_BLUESCREEN.
//
// lpvInBuffer must point to a BLUESCREENINFO structure.
// lpvOutBuffer must point to a DWORD which receives the message box result.
// The message box result is an IDXX value, as defined in windows.h.
//

/* H2INCSWITCHES -t */
typedef struct _BLUESCREENINFO {    /* bsi */

    TCHAR *pszText;         /* Message text (OEM character set) */
    TCHAR *pszTitle;        /* Message title (OEM character set) */
                            /* NULL means "Windows" */
    DWORD  flStyle;         /* Message box flags (see windows.h) */
                            /* Add'l flags defined in ddk\inc\shell.h */

} BLUESCREENINFO;
/* H2INCSWITCHES -t- */



#endif // _WSHIOCTL_H
