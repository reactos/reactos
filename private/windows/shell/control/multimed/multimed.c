/****************************************************************************
 *
 *   File : multimed.c
 *
 *   Description :
 *        Top level control panel applet code for multimedia for
 *        Windows NT
 *
 *   Copyright (c) 1993  Microsoft Corporation
 *
 *****************************************************************************/

/*****************************************************************************

    Design

    This module contains the code and data (apart from the icons for
    the 3 applets) to support 3 control panel applets for multi-media :

    sound   - Setting system sounds

    midimap - Midi mapper

    drivers - Installation and configuration of installable drivers through
              installable drivers interface

    The interface in is as for all control panel applets.  This (super)
    applet returns a number of internal applets to the CPL_GETCOUNT
    message depending on :

    waveOutGetNumDevs returns non-zero - then sound is supported.

    midiOutGetNumDevs returns non-zero or midiInGetNumDevs returns
    non-zero - then midimap is supported.

    Interface to sub-applets.  For packaging and historical reasons the
    other applets are separate files :

    sound - sound.dll
    midimap - midimap.dll  - is also a midi driver
    drivers - drivers.dll

    When an applet (which is supported for the current configuration of
    the system as determined above) is run (and ONLY THEN) via the
    CPL_DBLCLK message we call LoadLibrary for the (sub) applet and
    call its entry point (usually a 'cut-down' CplApplet).

    To do this each sub-applet's icon and string have fixed ids defined in
    multimed.h.

*****************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <cpl.h>
#include <cphelp.h>
#include "multimed.h"

   // This applet has been neutered to provide only MIDIMAP.DLL's CPL
   // interface; the others are now supported by MMSYS.CPL.
   //
// #define EXTRA_APPLETS
#ifdef EXTRA_APPLETS
 enum {
    SoundsApplet = 0,
    DriversApplet,
    MidiMapApplet,
    ACMApplet,
    NumberOfApplets
 };

 struct {
    LPCTSTR     AppletFileName;
    DWORD       dwHelpContext;
    HINSTANCE   ActiveHandle;
    APPLET_PROC AppletEntryPoint;
    BOOL        AppInUse;
    NEWCPLINFO  CplInfo;
 }
 AppletInfo[] = { { TEXT("sound.dll"), IDH_CHILD_SND },
                  { TEXT("drivers.dll"), IDH_CHILD_DRIVERS },
                  { TEXT("midimap.dll"), IDH_CHILD_MIDI },
                  { TEXT("msacm32.drv"), 0 } };  // No context for ACM
#else
 enum {
    MidiMapApplet = 0,
    NumberOfApplets
 };

 struct {
    LPCTSTR     AppletFileName;
    DWORD       dwHelpContext;
    HINSTANCE   ActiveHandle;
    APPLET_PROC AppletEntryPoint;
    BOOL        AppInUse;
    NEWCPLINFO  CplInfo;
 }
 AppletInfo[] = { { TEXT("midimap.dll"), IDH_CHILD_MIDI } };
#endif


 int IdMapping[NumberOfApplets];
 int TotalApplets;

 BOOL LoadDataPart(int AppletIndex)
 {
     UINT          OldErrorMode;
     LPNEWCPLINFO  lpCplInfo;
     HINSTANCE     DataOnlyHandle;

     OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

     DataOnlyHandle =
         LoadLibraryEx(AppletInfo[AppletIndex].AppletFileName,
                       NULL,
                       DONT_RESOLVE_DLL_REFERENCES);

     SetErrorMode(OldErrorMode);

     if (DataOnlyHandle == NULL) {
         return FALSE;
     }

    /*
     *  Cache all Cpl data now so we're not embarrassed by errors later
     */

     lpCplInfo = &AppletInfo[AppletIndex].CplInfo;

     lpCplInfo->dwSize = sizeof(NEWCPLINFO);
     lpCplInfo->lData = 0; // Applets we use expect this
     lpCplInfo->dwHelpContext = AppletInfo[AppletIndex].dwHelpContext;
     lpCplInfo->hIcon =
         LoadIcon(DataOnlyHandle,
                  MAKEINTRESOURCE(ID_ICON));

     if (lpCplInfo->hIcon == NULL ||
         !LoadString(DataOnlyHandle,
                     IDS_NAME,
                     lpCplInfo->szName,
                     sizeof(lpCplInfo->szName)) ||
         !LoadString(DataOnlyHandle,
                     IDS_INFO,
                     lpCplInfo->szInfo,
                     sizeof(lpCplInfo->szInfo)) ||
         !LoadString(DataOnlyHandle,
                     IDS_CONTROL_HLP,
                     lpCplInfo->szHelpFile,
                     sizeof(lpCplInfo->szHelpFile))) {

         FreeLibrary(DataOnlyHandle);
         return FALSE;
     }

     FreeLibrary(DataOnlyHandle);
     return TRUE;
 }

 LONG CPlApplet(HWND hCplWnd, UINT uMsg, LONG lParam1, LONG lParam2)
 {
    LONG ReturnCode = 0L;    // The default apparently
    int                i;

    switch (uMsg) {
    case CPL_INIT:

      /*
       *  I've no idea why this is a better place to initialize than
       *  CPL_GETCOUNT but why not?
       *
       */

      /*
       *   Check there's somebody home
       */

       for (i = 0; i < NumberOfApplets; i++) {

           /*
           **  Don't put up useless junk!
           */

           if (i == MidiMapApplet && midiOutGetNumDevs() == 0
#ifdef EXTRA_APPLETS
            || i == ACMApplet && waveOutGetNumDevs() == 0
#endif
              )
           {
               continue;
           }

           if (LoadDataPart(i)) {
               IdMapping[TotalApplets++] = i;
           }
       }

      /*
       *  Only succeed if we support something
       */

       ReturnCode = TotalApplets != 0;

       break;

    case CPL_GETCOUNT:
       return TotalApplets;
       break;

    case CPL_NEWINQUIRE:
        {
            LPNEWCPLINFO lpCplInfo;
            int iApplet;

            iApplet = IdMapping[lParam1];

            lpCplInfo = (LPNEWCPLINFO)lParam2;

            *lpCplInfo = AppletInfo[iApplet].CplInfo;
        }

        break;

    case CPL_DBLCLK:

       /*
        *  The job here is to
        *  1. If the applet is not already loaded
        *     -- load it
        *     -- Pass it a cpl_init message - this will do for our applets(!)
        *
        *  2. Pass it a CPL_DBLCLK message with the parameters we got
        *
        */
        {
            int iApplet;

            iApplet = IdMapping[lParam1];


            if (AppletInfo[iApplet].ActiveHandle == NULL) {

                UINT OldErrorMode;

                OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                AppletInfo[iApplet].ActiveHandle =
                    LoadLibrary(AppletInfo[iApplet].AppletFileName);

                SetErrorMode(OldErrorMode);

                if (AppletInfo[iApplet].ActiveHandle != NULL) {
                    AppletInfo[iApplet].AppletEntryPoint =
                        (APPLET_PROC)GetProcAddress(
                            AppletInfo[iApplet].ActiveHandle,
                            "CPlApplet");
                }
                if (AppletInfo[iApplet].AppletEntryPoint != NULL) {
                    (*AppletInfo[iApplet].AppletEntryPoint)
                        (hCplWnd, CPL_INIT, 0, 0);
                }
            }

            if (AppletInfo[iApplet].AppletEntryPoint != NULL) {
                (*AppletInfo[iApplet].AppletEntryPoint)
                    (hCplWnd, uMsg, lParam1, lParam2);
            }
        }

        break;

    case CPL_EXIT:

       /*
        *  Unload all our friends
        */


        {
            int i;
            for (i = 0; i < NumberOfApplets; i++) {

                if (AppletInfo[i].ActiveHandle != NULL) {
                    if (AppletInfo[i].AppletEntryPoint != NULL) {
                        (*AppletInfo[i].AppletEntryPoint)
                            (hCplWnd, CPL_EXIT, 0, 0);
                    }
                    FreeLibrary(AppletInfo[i].ActiveHandle);
                }
            }
        }


        break;

    }

    return ReturnCode;
 }
