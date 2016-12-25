/*
 * Test MCI CD-ROM access
 *
 * Copyright 2010 Jörg Höhle
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include "windows.h"
#include "mmsystem.h"
#include "wine/test.h"

typedef union {
      MCI_STATUS_PARMS     status;
      MCI_GETDEVCAPS_PARMS caps;
      MCI_OPEN_PARMSA      open;
      MCI_PLAY_PARMS       play;
      MCI_SEEK_PARMS       seek;
      MCI_SAVE_PARMSA      save;
      MCI_GENERIC_PARMS    gen;
    } MCI_PARMS_UNION;

extern const char* dbg_mcierr(MCIERROR err); /* from mci.c */

static BOOL spurious_message(LPMSG msg)
{
  /* WM_DEVICECHANGE 0x0219 appears randomly */
  if(msg->message != MM_MCINOTIFY) {
    trace("skipping spurious message %04x\n",msg->message);
    return TRUE;
  }
  return FALSE;
}

/* A single ok() in each code path allows us to prefix this with todo_wine */
#define test_notification(hwnd, command, type) test_notification_dbg(hwnd, command, type, __LINE__)
static void test_notification_dbg(HWND hwnd, const char* command, WPARAM type, int line)
{   /* Use type 0 as meaning no message */
    MSG msg;
    BOOL seen;
    do { seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE); }
    while(seen && spurious_message(&msg));
    if(type && !seen) {
      /* We observe transient delayed notification, mostly on native.
       * Notification is not always present right when mciSend returns. */
      trace_(__FILE__,line)("Waiting for delayed notification from %s\n", command);
      MsgWaitForMultipleObjects(0, NULL, FALSE, 3000, QS_POSTMESSAGE);
      seen = PeekMessageA(&msg, hwnd, MM_MCINOTIFY, MM_MCINOTIFY, PM_REMOVE);
    }
    if(!seen)
      ok_(__FILE__,line)(type==0, "Expect message %04lx from %s\n", type, command);
    else if(msg.hwnd != hwnd)
        ok_(__FILE__,line)(msg.hwnd == hwnd, "Didn't get the handle to our test window\n");
    else if(msg.message != MM_MCINOTIFY)
        ok_(__FILE__,line)(msg.message == MM_MCINOTIFY, "got %04x instead of MM_MCINOTIFY from command %s\n", msg.message, command);
    else ok_(__FILE__,line)(msg.wParam == type, "got %04lx instead of MCI_NOTIFY_xyz %04lx from command %s\n", msg.wParam, type, command);
}

#define CDFRAMES_PERSEC                 75
static DWORD MSF_Add(DWORD d1, DWORD d2)
{
    WORD c, m, s, f;
    f = MCI_MSF_FRAME(d1)  + MCI_MSF_FRAME(d2);
    c = f / CDFRAMES_PERSEC;
    f = f % CDFRAMES_PERSEC;
    s = MCI_MSF_SECOND(d1) + MCI_MSF_SECOND(d2) + c;
    c = s / 60;
    s = s % 60;
    m = MCI_MSF_MINUTE(d1) + MCI_MSF_MINUTE(d2) + c; /* may be > 60 */
    return MCI_MAKE_MSF(m,s,f);
}

static MCIERROR ok_open = 0; /* MCIERR_CANNOT_LOAD_DRIVER */

/* TODO show that shareable flag is not what Wine implements. */

static void test_play(HWND hwnd)
{
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    MCIERROR err, ok_hw;
    DWORD numtracks, track, duration;
    DWORD factor = winetest_interactive ? 3 : 1;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    parm.gen.dwCallback = (DWORD_PTR)hwnd; /* once to rule them all */

    err = mciSendStringA("open cdaudio alias c notify shareable", buf, sizeof(buf), hwnd);
    ok(!err || err == MCIERR_CANNOT_LOAD_DRIVER || err == MCIERR_MUST_USE_SHAREABLE,
       "mci open cdaudio notify returned %s\n", dbg_mcierr(err));
    ok_open = err;
    test_notification(hwnd, "open alias notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);
    /* Native returns MUST_USE_SHAREABLE when there's trouble with the hardware
     * (e.g. unreadable disk) or when Media Player already has the device open,
     * yet adding that flag does not help get past this error. */

    if(err) {
        skip("Cannot open any cdaudio device, %s.\n", dbg_mcierr(err));
        return;
    }
    wDeviceID = atoi(buf);
    ok(!strcmp(buf,"1"), "mci open deviceId: %s, expected 1\n", buf);
    /* Win9X-ME may start the MCI and media player upon insertion of a CD. */

    err = mciSendStringA("sysinfo all name 1 open", buf, sizeof(buf), NULL);
    ok(!err,"sysinfo all name 1 returned %s\n", dbg_mcierr(err));
    if(!err && wDeviceID != 1) trace("Device '%s' is open.\n", buf);

    err = mciSendStringA("capability c has video notify", buf, sizeof(buf), hwnd);
    ok(!err, "capability video: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "false"), "capability video is %s\n", buf);
    test_notification(hwnd, "capability notify", MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("capability c can play", buf, sizeof(buf), hwnd);
    ok(!err, "capability video: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "true"), "capability play is %s\n", buf);

    err = mciSendStringA("capability c", buf, sizeof(buf), NULL);
    ok(err == MCIERR_MISSING_PARAMETER, "capability nokeyword: %s\n", dbg_mcierr(err));

    parm.caps.dwItem = 0x4001;
    parm.caps.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
    ok(err == MCIERR_UNSUPPORTED_FUNCTION, "GETDEVCAPS %x: %s\n", parm.caps.dwItem, dbg_mcierr(err));

    parm.caps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
    ok(!err, "GETDEVCAPS device type: %s\n", dbg_mcierr(err));
    if(!err) ok( parm.caps.dwReturn == MCI_DEVTYPE_CD_AUDIO, "getdevcaps device type: %u\n", parm.caps.dwReturn);

    err = mciSendCommandA(wDeviceID, MCI_RECORD, 0, (DWORD_PTR)&parm);
    ok(err == MCIERR_UNSUPPORTED_FUNCTION, "MCI_RECORD: %s\n", dbg_mcierr(err));

    /* Wine's MCI_MapMsgAtoW crashes on MCI_SAVE without parm->lpfilename */
    parm.save.lpfilename = "foo";
    err = mciSendCommandA(wDeviceID, MCI_SAVE, 0, (DWORD_PTR)&parm);
    ok(err == MCIERR_UNSUPPORTED_FUNCTION, "MCI_SAVE: %s\n", dbg_mcierr(err));

    /* commands from the core set are UNSUPPORTED, others UNRECOGNIZED */
    err = mciSendCommandA(wDeviceID, MCI_STEP, 0, (DWORD_PTR)&parm);
    ok(err == MCIERR_UNRECOGNIZED_COMMAND, "MCI_STEP: %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err, "STATUS time format: %s\n", dbg_mcierr(err));
    if(!err) ok(parm.status.dwReturn == MCI_FORMAT_MSF, "status time default format: %ld\n", parm.status.dwReturn);

    /* "CD-Audio" */
    err = mciSendStringA("info c product wait notify", buf, sizeof(buf), hwnd);
    ok(!err, "info product: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "info notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    parm.status.dwItem = MCI_STATUS_MEDIA_PRESENT;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(err || parm.status.dwReturn == TRUE || parm.status.dwReturn == FALSE,
       "STATUS media present: %s\n", dbg_mcierr(err));

    if (parm.status.dwReturn != TRUE) {
        skip("No CD-ROM in drive.\n");
        return;
    }

    parm.status.dwItem = MCI_STATUS_MODE;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err, "STATUS mode: %s\n", dbg_mcierr(err));
    switch(parm.status.dwReturn) {
    case MCI_MODE_NOT_READY:
        skip("CD-ROM mode not ready (DVD in drive?)\n");
        return;
    case MCI_MODE_OPEN: /* should not happen with MEDIA_PRESENT */
        skip("CD-ROM drive is open\n");
        /* set door closed may not work. */
        return;
    default: /* play/record/seek/pause */
        ok(parm.status.dwReturn==MCI_MODE_STOP, "STATUS mode is %lx\n", parm.status.dwReturn);
        /* fall through */
    case MCI_MODE_STOP: /* normal */
        break;
    }

    /* Initial mode is "stopped" with a CD in drive */
    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped"), "status mode is initially %s\n", buf);

    err = mciSendStringA("status c ready", buf, sizeof(buf), hwnd);
    ok(!err, "status ready: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "true"), "status ready with media is %s\n", buf);

    err = mciSendStringA("info c product identity", buf, sizeof(buf), hwnd);
    ok(!err, "info 2flags: %s\n", dbg_mcierr(err)); /* not MCIERR_FLAGS_NOT_COMPATIBLE */
    /* Precedence rule p>u>i verified experimentally, not tested here. */

    err = mciSendStringA("info c identity", buf, sizeof(buf), hwnd);
    ok(!err || err == MCIERR_HARDWARE, "info identity: %s\n", dbg_mcierr(err));
    /* a blank disk causes MCIERR_HARDWARE and other commands to fail likewise. */
    ok_hw = err;

    err = mciSendStringA("info c upc", buf, sizeof(buf), hwnd);
    ok(err == ok_hw || err == MCIERR_NO_IDENTITY, "info upc: %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    parm.status.dwReturn = 0;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(err == ok_hw, "STATUS number of tracks: %s\n", dbg_mcierr(err));
    numtracks = parm.status.dwReturn;
    /* cf. MAXIMUM_NUMBER_TRACKS */
    ok(0 < numtracks && numtracks <= 99, "number of tracks=%ld\n", parm.status.dwReturn);

    err = mciSendStringA("status c length", buf, sizeof(buf), hwnd);
    ok(err == ok_hw, "status length: %s\n", dbg_mcierr(err));
    if(!err) trace("CD length %s\n", buf);

    if(err) { /* MCIERR_HARDWARE when given a blank disk */
        skip("status length %s (blank disk?)\n", dbg_mcierr(err));
        return;
    }

    /* Linux leaves the drive at some random position,
     * native initialises to the start position below. */
    err = mciSendStringA("status c position", buf, sizeof(buf), hwnd);
    ok(!err, "status position: %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf, "00:02:00") || !strcmp(buf, "00:02:33") || !strcmp(buf, "00:03:00"),
                "status position initially %s\n", buf);
    /* 2 seconds is the initial position even with data tracks. */

    err = mciSendStringA("status c position start notify", buf, sizeof(buf), hwnd);
    ok(err == ok_hw, "status position start: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "00:02:00") || !strcmp(buf, "00:02:33") || !strcmp(buf, "00:03:00"),
                "status position start %s\n", buf);
    test_notification(hwnd, "status notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status c position start track 1 notify", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_FLAGS_NOT_COMPATIBLE, "status position start: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "status 2flags", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("play c from 00:02:00 to 00:01:00 notify", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_OUTOFRANGE, "play 2s to 1s: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "play 2s to 1s", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("resume c", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_HARDWARE || /* Win9x */ err == MCIERR_UNSUPPORTED_FUNCTION,
       "resume without play: %s\n", dbg_mcierr(err)); /* not NONAPPLICABLE_FUNCTION */
    /* vmware with a .iso (data-only) yields no error on NT/w2k */

    err = mciSendStringA("seek c wait", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_MISSING_PARAMETER, "seek noflag: %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek c to start to end", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_FLAGS_NOT_COMPATIBLE || broken(!err), "seek to start+end: %s\n", dbg_mcierr(err));
    /* Win9x only errors out with Seek to start to <position> */

    /* set Wine to a defined position before play */
    err = mciSendStringA("seek c to start notify", buf, sizeof(buf), hwnd);
    ok(!err, "seek to start: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "seek to start", err ? 0 : MCI_NOTIFY_SUCCESSFUL);
    /* Win9X Status position / current track then sometimes report the end position / track! */

    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped"), "status mode after seek is %s\n", buf);

    /* MCICDA ignores MCI_SET_VIDEO */
    err = mciSendStringA("set c video on", buf, sizeof(buf), hwnd);
    ok(!err, "set video: %s\n", dbg_mcierr(err));

    /* One xp machine ignored SET_AUDIO, one w2k and one w7 machine honoured it
     * and simultaneously toggled the mute button in the mixer control panel.
     * Or does it only depend on the HW, not the OS?
     * Some vmware machines return MCIERR_HARDWARE. */
    err = mciSendStringA("set c audio all on", buf, sizeof(buf), hwnd);
    ok(!err || err == MCIERR_HARDWARE, "set audio: %s\n", dbg_mcierr(err));

    err = mciSendStringA("set c time format ms", buf, sizeof(buf), hwnd);
    ok(!err, "set time format ms: %s\n", dbg_mcierr(err));

    memset(buf, 0, sizeof(buf));
    err = mciSendStringA("status c position start", buf, sizeof(buf), hwnd);
    ok(!err, "status position start (ms): %s\n", dbg_mcierr(err));
    duration = atoi(buf);
    if(!err) ok(duration > 2000, "status position initially %sms\n", buf);
    /* 00:02:00 corresponds to 2001 ms, 02:33 -> 2441 etc. */

    err = mciSendStringA("status c position start track 1", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_FLAGS_NOT_COMPATIBLE, "status position start+track: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status c notify wait", buf, sizeof(buf), hwnd);
    ok(err == MCIERR_MISSING_PARAMETER, "status noflag: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status c length track 1", buf, sizeof(buf), hwnd);
    ok(!err, "status length (ms): %s\n", dbg_mcierr(err));
    if(!err) {
        trace("track #1 length %sms\n", buf);
        duration = atoi(buf);
    } else duration = 2001; /* for the position test below */

    if (0) { /* causes some native systems to return Seek and Play with MCIERR_HARDWARE */
        /* depending on capability can eject only? */
        err = mciSendStringA("set c door closed notify", buf, sizeof(buf), hwnd);
        ok(!err, "set door closed: %s\n", dbg_mcierr(err));
        test_notification(hwnd, "door closed", err ? 0 : MCI_NOTIFY_SUCCESSFUL);
    }
    /* Changing the disk while the MCI device is open causes the Status
     * command to report stale data.  Native obviously caches the TOC. */

    /* status type track is localised, strcmp("audio|other") may fail. */
    parm.status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
    parm.status.dwTrack = 1;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
    ok(!err, "STATUS type track 1: %s\n", dbg_mcierr(err));
    ok(parm.status.dwReturn==MCI_CDA_TRACK_OTHER || parm.status.dwReturn==MCI_CDA_TRACK_AUDIO,
       "unknown track type %lx\n", parm.status.dwReturn);

    if (parm.status.dwReturn == MCI_CDA_TRACK_OTHER) {
        /* Find an audio track */
        parm.status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        parm.status.dwTrack = numtracks;
        parm.status.dwReturn = 0xFEEDABAD;
        err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
        ok(!err, "STATUS type track %u: %s\n", numtracks, dbg_mcierr(err));
        ok(parm.status.dwReturn == MCI_CDA_TRACK_OTHER || parm.status.dwReturn == MCI_CDA_TRACK_AUDIO,
           "unknown track type %lx\n", parm.status.dwReturn);
        track = (!err && parm.status.dwReturn == MCI_CDA_TRACK_AUDIO) ? numtracks : 0;

        /* Seek to start (above) skips over data tracks
         * In case of a data only CD, it seeks to the end of disk, however
         * another Status position a few seconds later yields MCIERR_HARDWARE. */
        parm.status.dwItem = MCI_STATUS_POSITION;
        parm.status.dwReturn = 2000;
        err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
        ok(!err || broken(err == MCIERR_HARDWARE), "STATUS position: %s\n", dbg_mcierr(err));

        if(!err && track) ok(parm.status.dwReturn > duration,
            "Seek did not skip data tracks, position %lums\n", parm.status.dwReturn);
        /* dwReturn > start + length(#1) may fail because of small position report fluctuation.
         * On some native systems, status position fluctuates around the target position;
         * Successive calls return varying positions! */

        err = mciSendStringA("set c time format msf", buf, sizeof(buf), hwnd);
        ok(!err, "set time format msf: %s\n", dbg_mcierr(err));

        parm.status.dwItem = MCI_STATUS_LENGTH;
        parm.status.dwTrack = 1;
        parm.status.dwReturn = 0xFEEDABAD;
        err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
        ok(!err, "STATUS length track %u: %s\n", parm.status.dwTrack, dbg_mcierr(err));
        duration = parm.status.dwReturn;
        trace("track #1 length: %02um:%02us:%02uframes\n",
              MCI_MSF_MINUTE(duration), MCI_MSF_SECOND(duration), MCI_MSF_FRAME(duration));
        ok(duration>>24==0, "CD length high bits %08X\n", duration);

        /* TODO only with mixed CDs? */
        /* play track 1 to length silently works with data tracks */
        parm.play.dwFrom = MCI_MAKE_MSF(0,2,0);
        parm.play.dwTo = duration; /* omitting 2 seconds from end */
        err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, (DWORD_PTR)&parm);
        ok(!err, "PLAY data to %08X: %s\n", duration, dbg_mcierr(err));

        Sleep(1500*factor); /* Time to spin up, hopefully less than track length */

        err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
        ok(!err, "status mode: %s\n", dbg_mcierr(err));
        if(!err) ok(!strcmp(buf, "stopped"), "status mode on data is %s\n", buf);
    } else if (parm.status.dwReturn == MCI_CDA_TRACK_AUDIO) {
        skip("Got no mixed data+audio CD.\n");
        track = 1;
    } else track = 0;

    if (!track) {
        skip("Found no audio track.\n");
        return;
    }

    err = mciSendStringA("set c time format msf", buf, sizeof(buf), hwnd);
    ok(!err, "set time format msf: %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_LENGTH;
    parm.status.dwTrack = numtracks;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
    ok(!err, "STATUS length track %u: %s\n", parm.status.dwTrack, dbg_mcierr(err));
    duration = parm.status.dwReturn;
    trace("last track length: %02um:%02us:%02uframes\n",
          MCI_MSF_MINUTE(duration), MCI_MSF_SECOND(duration), MCI_MSF_FRAME(duration));
    ok(duration>>24==0, "CD length high bits %08X\n", duration);

    parm.status.dwItem = MCI_STATUS_POSITION;
    /* dwTrack is still set */
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
    ok(!err, "STATUS position start track %u: %s\n", parm.status.dwTrack, dbg_mcierr(err));
    trace("last track position: %02um:%02us:%02uframes\n",
          MCI_MSF_MINUTE(parm.status.dwReturn), MCI_MSF_SECOND(parm.status.dwReturn), MCI_MSF_FRAME(parm.status.dwReturn));

    /* Seek to position + length always works, esp.
     * for the last track it's NOT the position of the lead-out. */
    parm.seek.dwTo = MSF_Add(parm.status.dwReturn, duration);
    err = mciSendCommandA(wDeviceID, MCI_SEEK, MCI_TO, (DWORD_PTR)&parm);
    ok(!err, "SEEK to %08X position last + length: %s\n", parm.seek.dwTo, dbg_mcierr(err));

    parm.seek.dwTo = MSF_Add(parm.seek.dwTo, MCI_MAKE_MSF(0,0,1));
    err = mciSendCommandA(wDeviceID, MCI_SEEK, MCI_TO, (DWORD_PTR)&parm);
    ok(err == MCIERR_OUTOFRANGE, "SEEK past %08X position last + length: %s\n", parm.seek.dwTo, dbg_mcierr(err));

    err = mciSendStringA("set c time format tmsf", buf, sizeof(buf), hwnd);
    ok(!err, "set time format tmsf: %s\n", dbg_mcierr(err));

    parm.play.dwFrom = track;
    err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "PLAY from %u notify: %s\n", track, dbg_mcierr(err));

    if(err) {
        skip("Cannot manage to play track %u.\n", track);
        return;
    }

    Sleep(1800*factor); /* Time to spin up, hopefully less than track length */

    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "playing"), "status mode during play is %s\n", buf);

    err = mciSendStringA("pause c", buf, sizeof(buf), hwnd);
    ok(!err, "pause: %s\n", dbg_mcierr(err));

    test_notification(hwnd, "pause should abort notification", MCI_NOTIFY_ABORTED);

    /* Native returns stopped when paused,
     * yet the Stop command is different as it would disallow Resume. */
    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf, "stopped"), "status mode while paused is %s\n", buf);

    err = mciSendCommandA(wDeviceID, MCI_RESUME, 0, 0);
    ok(!err || /* Win9x */ err == MCIERR_UNSUPPORTED_FUNCTION,
       "RESUME without parms: %s\n", dbg_mcierr(err));

    Sleep(1300*factor);

    /* Native continues to play without interruption */
    err = mciSendCommandA(wDeviceID, MCI_PLAY, 0, 0);
    todo_wine ok(!err, "PLAY without parms: %s\n", dbg_mcierr(err));

    parm.play.dwFrom = MCI_MAKE_TMSF(numtracks,0,1,0);
    parm.play.dwTo = 1;
    err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO, (DWORD_PTR)&parm);
    ok(err == MCIERR_OUTOFRANGE, "PLAY: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "playing"), "status mode after play is %s\n", buf);

    err = mciSendCommandA(wDeviceID, MCI_STOP, MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "STOP notify: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "STOP notify", MCI_NOTIFY_SUCCESSFUL);
    test_notification(hwnd, "STOP #1", 0);

    parm.play.dwFrom = track;
    err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "PLAY from %u notify: %s\n", track, dbg_mcierr(err));

    Sleep(1600*factor);

    parm.seek.dwTo = 1; /* not <track>, to test position below */
    err = mciSendCommandA(wDeviceID, MCI_SEEK, MCI_TO, (DWORD_PTR)&parm);
    ok(!err, "SEEK to %u notify: %s\n", track, dbg_mcierr(err));
    /* Note that native's Status position / current track may move the head
     * and reflect the new position only seconds after issuing the command. */

    /* Seek stops */
    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped"), "status mode after play is %s\n", buf);

    test_notification(hwnd, "Seek aborts Play", MCI_NOTIFY_ABORTED);
    test_notification(hwnd, "Seek", 0);

    parm.play.dwFrom = track;
    parm.play.dwTo = MCI_MAKE_TMSF(track,0,0,21); /* 21 frames, subsecond */
    err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "PLAY from %u notify: %s\n", track, dbg_mcierr(err));

    Sleep(2200*factor);

    err = mciSendStringA("status c mode", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped") || broken(!strcmp(buf, "playing")), "status mode after play is %s\n", buf);
    if(!err && !strcmp(buf, "playing")) trace("status playing after sleep\n");

    /* Playing to end asynchronously sends no notification! */
    test_notification(hwnd, "PLAY to end", 0);

    err = mciSendStringA("status c mode notify", buf, sizeof(buf), hwnd);
    ok(!err, "status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped") || broken(!strcmp(buf, "playing")), "status mode after play is %s\n", buf);
    if(!err && !strcmp(buf, "playing")) trace("status still playing\n");
    /* Some systems report playing even after Sleep(3900ms) yet the successful
     * notification tests (not ABORTED) indicates they are finished. */

    test_notification(hwnd, "dangling from PLAY", MCI_NOTIFY_SUPERSEDED);
    test_notification(hwnd, "status mode", MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("stop c", buf, sizeof(buf), hwnd);
    ok(!err, "stop: %s\n", dbg_mcierr(err));

    test_notification(hwnd, "PLAY to end", 0);

    /* length as MSF despite set time format TMSF */
    parm.status.dwItem = MCI_STATUS_LENGTH;
    parm.status.dwTrack = numtracks;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD_PTR)&parm);
    ok(!err, "STATUS length track %u: %s\n", parm.status.dwTrack, dbg_mcierr(err));
    ok(duration == parm.status.dwReturn, "length MSF<>TMSF %08lX\n", parm.status.dwReturn);

    /* Play from position start to start+length always works. */
    /* TODO? also play it using MSF */
    parm.play.dwFrom = numtracks;
    parm.play.dwTo = (duration << 8) | numtracks; /* as TMSF */
    err = mciSendCommandA(wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "PLAY (TMSF) from %08X to %08X: %s\n", parm.play.dwFrom, parm.play.dwTo, dbg_mcierr(err));

    Sleep(1400*factor);

    err = mciSendStringA("status c current track", buf, sizeof(buf), hwnd);
    ok(!err, "status track: %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(numtracks == atoi(buf), "status current track gave %s, expected %u\n", buf, numtracks);
    /* fails in Wine because SEEK is independent on IOCTL_CDROM_RAW_READ */

    err = mciSendCommandA(wDeviceID, MCI_STOP, 0, 0);
    ok(!err, "STOP: %s\n", dbg_mcierr(err));
    test_notification(hwnd, "STOP aborts", MCI_NOTIFY_ABORTED);
    test_notification(hwnd, "STOP final", 0);
}

static void test_openclose(HWND hwnd)
{
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    MCIERROR err;
    char drive[] = {'a',':','\\','X','\0'};
    if (ok_open == MCIERR_CANNOT_LOAD_DRIVER) {
        /* todo_wine Every open below should yield this same error. */
        skip("CD-ROM device likely not installed or disabled.\n");
        return;
    }

    /* Bug in native since NT: After OPEN "c" without MCI_OPEN_ALIAS fails with
     * MCIERR_DEVICE_OPEN, any subsequent OPEN fails with EXTENSION_NOT_FOUND! */
    parm.open.lpstrAlias = "x"; /* with alias, OPEN "c" behaves normally */
    parm.open.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
    parm.open.lpstrElementName = drive;
    for ( ; strlen(drive); drive[strlen(drive)-1] = 0)
    for (drive[0] = 'a'; drive[0] <= 'z'; drive[0]++) {
        err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
                MCI_OPEN_SHAREABLE | MCI_OPEN_ALIAS, (DWORD_PTR)&parm);
        ok(!err || err == MCIERR_INVALID_FILE, "OPEN %s type: %s\n", drive, dbg_mcierr(err));
        /* open X:\ fails in Win9x/NT. Only open X: works everywhere. */
        if(!err) {
            wDeviceID = parm.open.wDeviceID;
            trace("ok with %s\n", drive);
            err = mciSendCommandA(wDeviceID, MCI_CLOSE, 0, 0);
            ok(!err,"mciCommand close returned %s\n", dbg_mcierr(err));
        }
    }
    drive[0] = '\\';
    err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
            MCI_OPEN_SHAREABLE, (DWORD_PTR)&parm);
    ok(err == MCIERR_INVALID_FILE, "OPEN %s type: %s\n", drive, dbg_mcierr(err));
    if(!err) mciSendCommandA(parm.open.wDeviceID, MCI_CLOSE, 0, 0);

    if (0) {
        parm.open.lpstrElementName = (LPCSTR)0xDEADBEEF;
        err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID |
                MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE, (DWORD_PTR)&parm);
        todo_wine ok(err == MCIERR_FLAGS_NOT_COMPATIBLE, "OPEN elt_ID: %s\n", dbg_mcierr(err));
        if(!err) mciSendCommandA(parm.open.wDeviceID, MCI_CLOSE, 0, 0);
    }
}

START_TEST(mcicda)
{
    MCIERROR err;
    HWND hwnd;
    hwnd = CreateWindowExA(0, "static", "mcicda test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    test_notification(hwnd, "-prior to tests-", 0);
    test_play(hwnd);
    test_openclose(hwnd);
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_STOP, 0, 0);
    todo_wine ok(!err || broken(err == MCIERR_HARDWARE /* blank CD or testbot without CD-ROM */),
       "STOP all returned %s\n", dbg_mcierr(err));
    err = mciSendStringA("close all", NULL, 0, hwnd);
    ok(!err, "final close all returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "-tests complete-", 0);
    DestroyWindow(hwnd);
}
