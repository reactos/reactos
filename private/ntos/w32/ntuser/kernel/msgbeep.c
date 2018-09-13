/****************************** Module Header ******************************\
* Module Name: msgbeep.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the xxxMessageBox API and related functions.
*
* History:
*  6-26-91 NigelT      Created it with some wood and a few nails
*  7 May 92 SteveDav   Getting closer to the real thing
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <ntddbeep.h>
#include <mmsystem.h>

/***************************************************************************\
* xxxOldMessageBeep (API)
*
* Send a beep to the beep device
*
* History:
* 09-25-91 JimA         Created.
\***************************************************************************/

BOOL xxxOldMessageBeep()
{
    BOOL b;
    if (TEST_PUDF(PUDF_BEEP)) {
        LeaveCrit();
        b = UserBeep(440, 125);
        EnterCrit();
        return b;
    } else {
        _UserSoundSentryWorker();
    }

    return TRUE;
}

/***************************************************************************\
* xxxMessageBeep (API)
*
*
* History:
*  6-26-91  NigelT      Wrote it.
* 24-Mar-92 SteveDav    Changed interface - no passing of strings
*                       If WINMM cannot be found or loaded, then use speaker
\***************************************************************************/

BOOL xxxMessageBeep(
    UINT dwType)
{
    UINT sndid;
    PTHREADINFO  pti = PtiCurrent();

    if (pti->TIF_flags & TIF_SYSTEMTHREAD) {
        xxxOldMessageBeep();
        return TRUE;
    }

    if (!TEST_PUDF(PUDF_BEEP)) {
        _UserSoundSentryWorker();
        return TRUE;
    }

    switch(dwType & MB_ICONMASK) {
    case MB_ICONHAND:
        sndid = USER_SOUND_SYSTEMHAND;
        break;

    case MB_ICONQUESTION:
        sndid = USER_SOUND_SYSTEMQUESTION;
        break;

    case MB_ICONEXCLAMATION:
        sndid = USER_SOUND_SYSTEMEXCLAMATION;
        break;

    case MB_ICONASTERISK:
        sndid = USER_SOUND_SYSTEMASTERISK;
        break;

    default:
        sndid = USER_SOUND_DEFAULT;
        break;
    }

    if (gspwndLogonNotify) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_PLAYEVENTSOUND, sndid);
    }

    _UserSoundSentryWorker();

    return TRUE;
}

/***************************************************************************\
* xxxPlayEventSound
*
* Play a sound
*
* History:
* 09-25-91 JimA         Created.
\***************************************************************************/

VOID PlayEventSound(UINT idSound)
{
    PTHREADINFO    pti = PtiCurrent();

    if (!TEST_PUDF(PUDF_EXTENDEDSOUNDS))
        return;

    if (pti->TIF_flags & TIF_SYSTEMTHREAD)
        return;

    if (gspwndLogonNotify) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_PLAYEVENTSOUND, idSound);
    }

//  BUGBUG -- we should only flash SoundSentry if a sound is played.  With the
//  new technique of posting to WinLogon, we can't determine this here.
//   _UserSoundSentryWorker();

}
