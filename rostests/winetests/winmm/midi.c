/*
 * Test winmm midi
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

#define _WINE

#include <stdio.h>
#include <stddef.h>
#include "windows.h"
#include "mmsystem.h"
#include "wine/test.h"

extern const char* mmsys_error(MMRESULT error); /* from wave.c */

/* Test in order of increasing probability to hang.
 * On many UNIX systems, the Timidity softsynth provides
 * MIDI sequencer services and it is not particularly robust.
 */

#define MYCBINST 0x4CAFE5A8 /* not used with window or thread callbacks */
#define WHATEVER 0xFEEDF00D

static BOOL spurious_message(LPMSG msg)
{
  /* WM_DEVICECHANGE 0x0219 appears randomly */
  if(msg->message == WM_DEVICECHANGE) {
    trace("skipping spurious message %04x\n", msg->message);
    return TRUE;
  }
  return FALSE;
}

static UINT      cbmsg  = 0;
static DWORD_PTR cbval1 = WHATEVER;
static DWORD_PTR cbval2 = 0;
static DWORD_PTR cbinst = MYCBINST;

static void CALLBACK callback_func(HWAVEOUT hwo, UINT uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (winetest_debug>1)
        trace("Callback! msg=%x %lx %lx\n", uMsg, dwParam1, dwParam2);
    cbmsg = uMsg;
    cbval1 = dwParam1;   /* mhdr or 0 */
    cbval2 = dwParam2;   /* always 0 */
    cbinst = dwInstance; /* MYCBINST, see midiOut/StreamOpen */
}

#define test_notification(hwnd, command, m1, p2) test_notification_dbg(hwnd, command, m1, p2, __LINE__)
static void test_notification_dbg(HWND hwnd, const char* command, UINT m1, DWORD_PTR p2, int line)
{   /* Use message type 0 as meaning no message */
    MSG msg;
    if (hwnd) {
        /* msg.wParam is hmidiout, msg.lParam is the mhdr (or 0) */
        BOOL seen;
        do { seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE); }
        while(seen && spurious_message(&msg));
        if (m1 && !seen) {
          /* We observe transient delayed notification, mostly on native.
           * Perhaps the OS preempts the player thread after setting MHDR_DONE
           * or clearing MHDR_INQUEUE, before calling DriverCallback. */
          DWORD rc;
          trace_(__FILE__,line)("Waiting for delayed message %x from %s\n", m1, command);
          SetLastError(0xDEADBEEF);
          rc = MsgWaitForMultipleObjects(0, NULL, FALSE, 3000, QS_POSTMESSAGE);
          ok_(__FILE__,line)(rc==WAIT_OBJECT_0, "Wait failed: %04x %d\n", rc, GetLastError());
          seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE);
        }
        if (seen) {
            trace_(__FILE__,line)("Message %x, wParam=%lx, lParam=%lx from %s\n",
                  msg.message, msg.wParam, msg.lParam, command);
            ok_(__FILE__,line)(msg.hwnd==hwnd, "Didn't get the handle to our test window\n");
            ok_(__FILE__,line)(msg.message==m1 && msg.lParam==p2, "bad message %x/%lx from %s, expect %x/%lx\n", msg.message, msg.lParam, command, m1, p2);
        }
        else ok_(__FILE__,line)(m1==0, "Expect message %x from %s\n", m1, command);
    }
    else {
        /* FIXME: MOM_POSITIONCB and MOM_DONE are so close that a queue is needed. */
        if (cbmsg) {
            ok_(__FILE__,line)(cbmsg==m1 && cbval1==p2 && cbval2==0, "bad callback %x/%lx/%lx from %s, expect %x/%lx\n", cbmsg, cbval1, cbval2, command, m1, p2);
            cbmsg = 0; /* Mark as read */
            cbval1 = cbval2 = WHATEVER;
            ok_(__FILE__,line)(cbinst==MYCBINST, "callback dwInstance changed to %lx\n", cbinst);
        }
        else ok_(__FILE__,line)(m1==0, "Expect callback %x from %s\n", m1, command);
    }
}


static void test_midiIn_device(UINT udev, HWND hwnd)
{
    HMIDIIN hm;
    MMRESULT rc;
    MIDIINCAPSA capsA;
    MIDIHDR mhdr;

    rc = midiInGetDevCapsA(udev, &capsA, sizeof(capsA));
    ok((MIDIMAPPER==udev) ? (rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/)) : rc==0,
       "midiInGetDevCaps(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (!rc) {
        /* MIDI IN capsA.dwSupport may contain garbage, absent in old MS-Windows */
        trace("* %s: manufacturer=%d, product=%d, support=%X\n", capsA.szPname, capsA.wMid, capsA.wPid, capsA.dwSupport);
    }

    if (hwnd)
        rc = midiInOpen(&hm, udev, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiInOpen(&hm, udev, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    ok((MIDIMAPPER!=udev) ? rc==0 : (rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/)),
       "midiInOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiInOpen", MIM_OPEN, 0);

    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.dwFlags = MHDR_DONE;
    mhdr.dwUser = 0x56FA552C;
    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.dwBytesRecorded = 5;
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        rc = midiInPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset)-1);
        ok(rc==MMSYSERR_INVALPARAM, "midiInPrepare tiny rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_DONE, "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags |= MHDR_INQUEUE;
        rc = midiInPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiInPrepare old size rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_INQUEUE|MHDR_DONE)/*w9x*/ ||
           mhdr.dwFlags == MHDR_PREPARED, "dwFlags=%x\n", mhdr.dwFlags);
        trace("MIDIHDR flags=%x when unsent\n", mhdr.dwFlags);

        mhdr.dwFlags |= MHDR_INQUEUE|MHDR_DONE;
        rc = midiInPrepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiInPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_INQUEUE|MHDR_DONE), "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags &= ~MHDR_INQUEUE;
        rc = midiInUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiInUnprepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_DONE, "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags &= ~MHDR_DONE;
        rc = midiInUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiInUnprepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == 0, "dwFlags=%x\n", mhdr.dwFlags);

        rc = midiInPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiInPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_PREPARED, "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags |= MHDR_DONE;
        rc = midiInPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiInPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwBytesRecorded == 5, "BytesRec=%u\n", mhdr.dwBytesRecorded);

        mhdr.dwFlags |= MHDR_DONE;
        rc = midiInAddBuffer(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiAddBuffer rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_INQUEUE), "dwFlags=%x\n", mhdr.dwFlags);

        /* w95 does not set dwBytesRecorded=0 within midiInAddBuffer.  Wine does. */
        if (mhdr.dwBytesRecorded != 0)
            trace("dwBytesRecorded %u\n", mhdr.dwBytesRecorded);

        rc = midiInAddBuffer(hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_STILLPLAYING, "midiAddBuffer rc=%s\n", mmsys_error(rc));

        rc = midiInPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiInPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_INQUEUE), "dwFlags=%x\n", mhdr.dwFlags);
    }
    rc = midiInReset(hm); /* Return any pending buffer */
    ok(!rc, "midiInReset rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiInReset", MIM_LONGDATA, (DWORD_PTR)&mhdr);

    ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_DONE), "dwFlags=%x\n", mhdr.dwFlags);
    rc = midiInUnprepareHeader(hm, &mhdr, sizeof(mhdr));
    ok(!rc, "midiInUnprepare rc=%s\n", mmsys_error(rc));

    ok(mhdr.dwBytesRecorded == 0, "Did some MIDI HW send %u bytes?\n", mhdr.dwBytesRecorded);

    rc = midiInClose(hm);
    ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));

    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);
    HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    test_notification(hwnd, "midiInClose", MIM_CLOSE, 0);
    test_notification(hwnd, "midiIn over", 0, WHATEVER);
}

static void test_midi_infns(HWND hwnd)
{
    HMIDIIN hm;
    MMRESULT rc;
    UINT udev, ndevs = midiInGetNumDevs();

    rc = midiInOpen(&hm, ndevs, 0, 0, CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID, "midiInOpen udev>max rc=%s\n", mmsys_error(rc));
    if (!rc) {
        rc = midiInClose(hm);
        ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));
    }
    if (!ndevs) {
        trace("Found no MIDI IN device\n"); /* no skip for this common situation */
        rc = midiInOpen(&hm, MIDIMAPPER, 0, 0, CALLBACK_NULL);
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/), "midiInOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        if (!rc) {
            rc = midiInClose(hm);
            ok(!rc, "midiInClose rc=%s\n", mmsys_error(rc));
        }
        return;
    }
    trace("Found %d MIDI IN devices\n", ndevs);
    for (udev=0; udev < ndevs; udev++) {
        trace("** Testing device %d\n", udev);
        test_midiIn_device(udev, hwnd);
        Sleep(50);
    }
    trace("** Testing MIDI mapper\n");
    test_midiIn_device(MIDIMAPPER, hwnd);
}


static void test_midi_mci(HWND hwnd)
{
    MCIERROR err;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendStringA("sysinfo sequencer quantity", buf, sizeof(buf), hwnd);
    ok(!err, "mci sysinfo sequencer quantity returned %d\n", err);
    if (!err) trace("Found %s MCI sequencer devices\n", buf);

    if (!strcmp(buf, "0")) return;

    err = mciSendStringA("capability sequencer can record", buf, sizeof(buf), hwnd);
    ok(!err, "mci sysinfo sequencer quantity returned %d\n", err);
    if(!err) ok(!strcmp(buf, "false"), "capability can record is %s\n", buf);
}


static void test_midiOut_device(UINT udev, HWND hwnd)
{
    HMIDIOUT hm;
    MMRESULT rc;
    MIDIOUTCAPSA capsA;
    DWORD ovolume;
    UINT  udevid;
    MIDIHDR mhdr;

    rc = midiOutGetDevCapsA(udev, &capsA, sizeof(capsA));
    ok(!rc, "midiOutGetDevCaps(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (!rc) {
        trace("* %s: manufacturer=%d, product=%d, tech=%d, support=%X: %d voices, %d notes\n",
              capsA.szPname, capsA.wMid, capsA.wPid, capsA.wTechnology, capsA.dwSupport, capsA.wVoices, capsA.wNotes);
        ok(!((MIDIMAPPER==udev) ^ (MOD_MAPPER==capsA.wTechnology)), "technology %d on device %d\n", capsA.wTechnology, udev);
        if (MOD_MIDIPORT == capsA.wTechnology) {
            ok(capsA.wVoices == 0 && capsA.wNotes == 0, "external device with notes or voices\n");
            ok(capsA.wChannelMask == 0xFFFF, "external device channel mask %x\n", capsA.wChannelMask);
            ok(!(capsA.dwSupport & (MIDICAPS_VOLUME|MIDICAPS_LRVOLUME|MIDICAPS_CACHE)), "external device support=%X\n", capsA.dwSupport);
        }
    }

    if (hwnd)
        rc = midiOutOpen(&hm, udev, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiOutOpen(&hm, udev, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    if (rc == MMSYSERR_NOTSUPPORTED || rc == MMSYSERR_NODRIVER)
    {
        skip( "MIDI out not supported\n" );
        return;
    }
    ok(!rc, "midiOutOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiOutOpen", MOM_OPEN, 0);

    rc = midiOutGetVolume(hm, &ovolume);
    ok((capsA.dwSupport & MIDICAPS_VOLUME) ? rc==MMSYSERR_NOERROR : rc==MMSYSERR_NOTSUPPORTED, "midiOutGetVolume rc=%s\n", mmsys_error(rc));
    /* The native mapper responds with FFFFFFFF initially,
     * real devices with the volume GUI SW-synth settings. */
    if (!rc) trace("Current volume %x on device %d\n", ovolume, udev);

    /* The W95 ESFM Synthesis device reports NOTENABLED although
     * GetVolume by handle works and music plays. */
    rc = midiOutGetVolume(UlongToHandle(udev), &ovolume);
    ok((capsA.dwSupport & MIDICAPS_VOLUME) ? rc==MMSYSERR_NOERROR || broken(rc==MMSYSERR_NOTENABLED) : rc==MMSYSERR_NOTSUPPORTED, "midiOutGetVolume(dev=%d) rc=%s\n", udev, mmsys_error(rc));

    rc = midiOutGetVolume(hm, NULL);
    ok(rc==MMSYSERR_INVALPARAM, "midiOutGetVolume NULL rc=%s\n", mmsys_error(rc));

    /* Tests with midiOutSetvolume show that the midi mapper forwards
     * the value to the real device, but Get initially always reports
     * FFFFFFFF.  Therefore, a Get+SetVolume pair with the mapper is
     * not adequate to restore the value prior to tests.
     */
    if (winetest_interactive && (capsA.dwSupport & MIDICAPS_VOLUME)) {
        DWORD volume2 = (ovolume < 0x80000000) ? 0xC000C000 : 0x40004000;
        rc = midiOutSetVolume(hm, volume2);
        ok(!rc, "midiOutSetVolume rc=%s\n", mmsys_error(rc));
        if (!rc) {
            DWORD volume3;
            rc = midiOutGetVolume(hm, &volume3);
            ok(!rc, "midiOutGetVolume new rc=%s\n", mmsys_error(rc));
            if (!rc) trace("New volume %x on device %d\n", volume3, udev);
            todo_wine ok(volume2==volume3, "volume Set %x = Get %x\n", volume2, volume3);

            rc = midiOutSetVolume(hm, ovolume);
            ok(!rc, "midiOutSetVolume restore rc=%s\n", mmsys_error(rc));
        }
    }
    rc = midiOutGetDevCapsA((UINT_PTR)hm, &capsA, sizeof(capsA));
    ok(!rc, "midiOutGetDevCaps(dev=%d) by handle rc=%s\n", udev, mmsys_error(rc));
    rc = midiInGetDevCapsA((UINT_PTR)hm, (LPMIDIINCAPSA)&capsA, sizeof(DWORD));
    ok(rc==MMSYSERR_BADDEVICEID, "midiInGetDevCaps(dev=%d) by out handle rc=%s\n", udev, mmsys_error(rc));

    {   DWORD e = 0x006F4893; /* velocity, note (#69 would be 440Hz) channel */
        trace("ShortMsg type %x\n", LOBYTE(LOWORD(e)));
        rc = midiOutShortMsg(hm, e);
        ok(!rc, "midiOutShortMsg rc=%s\n", mmsys_error(rc));
        if (!rc) Sleep(400); /* Hear note */
    }

    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.dwFlags = MHDR_DONE;
    mhdr.dwUser   = 0x56FA552C;
    mhdr.dwOffset = 0xDEADBEEF;
    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        rc = midiOutLongMsg(hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_UNPREPARED, "midiOutLongMsg unprepared rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_DONE, "dwFlags=%x\n", mhdr.dwFlags);
        test_notification(hwnd, "midiOutLong unprepared", 0, WHATEVER);

        rc = midiOutPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset)-1);
        ok(rc==MMSYSERR_INVALPARAM, "midiOutPrepare tiny rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_DONE, "dwFlags=%x\n", mhdr.dwFlags);

        /* Since at least w2k, midiOutPrepare clears the DONE and INQUEUE flags.  w95 didn't. */
        /* mhdr.dwFlags |= MHDR_INQUEUE; would cause w95 to return STILLPLAYING from Unprepare */
        rc = midiOutPrepareHeader(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutPrepare old size rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_DONE)/*w9x*/ ||
           mhdr.dwFlags == MHDR_PREPARED, "dwFlags=%x\n", mhdr.dwFlags);
        trace("MIDIHDR flags=%x when unsent\n", mhdr.dwFlags);

        /* No flag is cleared when already prepared. */
        mhdr.dwFlags |= MHDR_DONE|MHDR_INQUEUE;
        rc = midiOutPrepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutPrepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_DONE|MHDR_INQUEUE), "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags |= MHDR_INQUEUE;
        rc = midiOutUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_STILLPLAYING, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_PREPARED|MHDR_DONE|MHDR_INQUEUE), "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags &= ~MHDR_INQUEUE;
        rc = midiOutUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == MHDR_DONE, "dwFlags=%x\n", mhdr.dwFlags);

        mhdr.dwFlags |= MHDR_INQUEUE;
        rc = midiOutUnprepareHeader(hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags == (MHDR_INQUEUE|MHDR_DONE), "dwFlags=%x\n", mhdr.dwFlags);

        HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    }
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);
    ok(mhdr.dwOffset==0xDEADBEEF, "MIDIHDR.dwOffset changed to %x\n", mhdr.dwOffset);

    rc = midiOutGetID(hm, &udevid);
    ok(!rc, "midiOutGetID rc=%s\n", mmsys_error(rc));
    if(!rc) ok(udevid==udev, "midiOutGetID gives %d, expect %d\n", udevid, udev);

    rc = midiOutReset(hm); /* Quiet everything */
    ok(!rc, "midiOutReset rc=%s\n", mmsys_error(rc));

    rc = midiOutClose(hm);
    ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiOutClose", MOM_CLOSE, 0);

    rc = midiOutOpen(&hm, udev, 0, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    /* w95 broken(rc==MMSYSERR_INVALPARAM) see WINMM_CheckCallback */
    ok(!rc, "midiOutOpen(dev=%d) 0 CALLBACK_WINDOW rc=%s\n", udev, mmsys_error(rc));
    /* PostMessage(hwnd=0) redirects to PostThreadMessage(GetCurrentThreadId())
     * which PeekMessage((HWND)-1) queries. */
    test_notification((HWND)-1, "midiOutOpen WINDOW->THREAD", 0, WHATEVER);
    test_notification(hwnd, "midiOutOpen WINDOW", 0, WHATEVER);
    if (!rc) {
        rc = midiOutClose(hm);
        ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
        test_notification((HWND)-1, "midiOutClose WINDOW->THREAD", 0, WHATEVER);
        test_notification(hwnd, "midiOutClose", 0, WHATEVER);
    }
    test_notification(hwnd, "midiOut over", 0, WHATEVER);
}

static void test_position(HMIDISTRM hm, UINT typein, UINT typeout)
{
    MMRESULT rc;
    MMTIME mmtime;
    mmtime.wType = typein;
    rc = midiStreamPosition(hm, &mmtime, sizeof(MMTIME));
    /* Ugly, but a single ok() herein enables using the todo_wine prefix */
    ok(!rc && (mmtime.wType == typeout), "midiStreamPosition type %x converted to %x rc=%s\n", typein, mmtime.wType, mmsys_error(rc));
    if (!rc) switch(mmtime.wType) {
    case TIME_MS:
        trace("Stream position %ums\n", mmtime.u.ms);
        break;
    case TIME_TICKS:
        trace("Stream position %u ticks\n", mmtime.u.ticks);
        break;
    case TIME_MIDI:
        trace("Stream position song pointer %u\n", mmtime.u.midi.songptrpos);
        break;
    }
}

typedef struct midishortevent_tag { /* ideal size for MEVT_F_SHORT event type */
    DWORD dwDeltaTime;
    DWORD dwStreamID;
    DWORD dwEvent;
} MIDISHORTEVENT;

/* Native crashes on a second run with the const qualifier set on this data! */
static BYTE strmEvents[] = { /* A set of variable-sized MIDIEVENT structs */
    0, 0, 0, 0,  0, 0, 0, 0, /* dwDeltaTime and dwStreamID */
    0, 0, 0, MEVT_NOP | 0x40, /* with MEVT_F_CALLBACK */
    0, 0, 0, 0,  0, 0, 0, 0,
    0xE0, 0x93, 0x04, MEVT_TEMPO, /* 0493E0 == 300000 */
    0, 0, 0, 0,  0, 0, 0, 0,
    0x93, 0x48, 0x6F, MEVT_SHORTMSG,
};

static MIDISHORTEVENT strmNops[] = { /* Test callback + dwOffset */
  { 0, 0, (MEVT_NOP <<24)| MEVT_F_CALLBACK },
  { 0, 0, (MEVT_NOP <<24)| MEVT_F_CALLBACK },
};

static MMRESULT playStream(HMIDISTRM hm, LPMIDIHDR lpMidiHdr)
{
    MMRESULT rc = midiStreamOut(hm, lpMidiHdr, sizeof(MIDIHDR));
    /* virtual machines may return MIDIERR_STILLPLAYING from the next request
     * even after MHDR_DONE is set. It's still too early, so add MHDR_INQUEUE. */
    if (!rc) while (!(lpMidiHdr->dwFlags & MHDR_DONE) || (lpMidiHdr->dwFlags & MHDR_INQUEUE)) { Sleep(100); }
    return rc;
}

static void test_midiStream(UINT udev, HWND hwnd)
{
    HMIDISTRM hm;
    MMRESULT rc, rc2;
    MIDIHDR mhdr;
    union {
        MIDIPROPTEMPO tempo;
        MIDIPROPTIMEDIV tdiv;
    } midiprop;

    if (hwnd)
        rc = midiStreamOpen(&hm, &udev, 1, (DWORD_PTR)hwnd, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    else
        rc = midiStreamOpen(&hm, &udev, 1, (DWORD_PTR)callback_func, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    if (rc == MMSYSERR_NOTSUPPORTED || rc == MMSYSERR_NODRIVER)
    {
        skip( "MIDI stream not supported\n" );
        return;
    }
    ok(!rc, "midiStreamOpen(dev=%d) rc=%s\n", udev, mmsys_error(rc));
    if (rc) return;

    test_notification(hwnd, "midiStreamOpen", MOM_OPEN, 0);

    midiprop.tempo.cbStruct = sizeof(midiprop.tempo);
    rc = midiStreamProperty(hm, (void*)&midiprop, MIDIPROP_GET|MIDIPROP_TEMPO);
    ok(!rc, "midiStreamProperty TEMPO rc=%s\n", mmsys_error(rc));
    ok(midiprop.tempo.dwTempo==500000, "default stream tempo %u microsec per quarter note\n", midiprop.tempo.dwTempo);

    midiprop.tdiv.cbStruct = sizeof(midiprop.tdiv);
    rc = midiStreamProperty(hm, (void*)&midiprop, MIDIPROP_GET|MIDIPROP_TIMEDIV);
    ok(!rc, "midiStreamProperty TIMEDIV rc=%s\n", mmsys_error(rc));
    todo_wine ok(24==LOWORD(midiprop.tdiv.dwTimeDiv), "default stream time division %u\n", midiprop.tdiv.dwTimeDiv);

    memset(&mhdr, 0, sizeof(mhdr));
    mhdr.dwUser   = 0x56FA552C;
    mhdr.dwOffset = 1234567890;
    mhdr.dwBufferLength = sizeof(strmEvents);
    mhdr.dwBytesRecorded = mhdr.dwBufferLength;
    mhdr.lpData = (LPSTR)&strmEvents[0];
    if (mhdr.lpData) {
        rc = midiOutLongMsg((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(rc==MIDIERR_UNPREPARED, "midiOutLongMsg unprepared rc=%s\n", mmsys_error(rc));
        test_notification(hwnd, "midiOutLong unprepared", 0, WHATEVER);

        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset)-1);
        ok(rc==MMSYSERR_INVALPARAM, "midiOutPrepare tiny rc=%s\n", mmsys_error(rc));
        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutPrepare old size rc=%s\n", mmsys_error(rc));
        ok(mhdr.dwFlags & MHDR_PREPARED, "MHDR.dwFlags when prepared %x\n", mhdr.dwFlags);

        /* The device is still in paused mode and should queue the message. */
        rc = midiStreamOut(hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiStreamOut old size rc=%s\n", mmsys_error(rc));
        rc2 = rc;
        trace("MIDIHDR flags=%x when submitted\n", mhdr.dwFlags);
        /* w9X/me does not set MHDR_ISSTRM when StreamOut exits,
         * but it will be set on all systems after the job is finished. */

        Sleep(90);
        /* Wine <1.1.39 started playing immediately */
        test_notification(hwnd, "midiStream still paused", 0, WHATEVER);

    /* MSDN asks to use midiStreamRestart prior to midiStreamOut()
     * because the starting state is 'pause', but some apps seem to
     * work with the inverse order: queue everything, then play.
     */

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart rc=%s\n", mmsys_error(rc));

        if (!rc2) while(mhdr.dwFlags & MHDR_INQUEUE) {
            trace("async MIDI still queued\n");
            Sleep(100);
        } /* Checking INQUEUE is not the recommended way to wait for the end of a job, but we're testing. */
        /* MHDR_ISSTRM is not necessarily set when midiStreamOut returns
         * rather than when the queue is eventually processed. */
        ok(mhdr.dwFlags & MHDR_ISSTRM, "MHDR.dwFlags %x no ISSTRM when out of queue\n", mhdr.dwFlags);
        if (!rc2) while(!(mhdr.dwFlags & MHDR_DONE)) {
            /* Never to be seen except perhaps on multicore */
            trace("async MIDI still not done\n");
            Sleep(100);
        }
        ok(mhdr.dwFlags & MHDR_DONE, "MHDR.dwFlags %x not DONE when out of queue\n", mhdr.dwFlags);
        test_notification(hwnd, "midiStream callback", MOM_POSITIONCB, (DWORD_PTR)&mhdr);
        test_notification(hwnd, "midiStreamOut", MOM_DONE, (DWORD_PTR)&mhdr);

        /* Native fills dwOffset regardless of the cbMidiHdr size argument to midiStreamOut */
        ok(1234567890!=mhdr.dwOffset, "play left MIDIHDR.dwOffset at %u\n", mhdr.dwOffset);

        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutUnprepare #2 rc=%s\n", mmsys_error(rc));

        trace("MIDIHDR stream flags=%x when finished\n", mhdr.dwFlags);
        ok(mhdr.dwFlags & MHDR_DONE, "MHDR.dwFlags when done %x\n", mhdr.dwFlags);

        test_position(hm, TIME_MS,      TIME_MS);
        test_position(hm, TIME_TICKS,   TIME_TICKS);
        todo_wine test_position(hm, TIME_MIDI,    TIME_MIDI);
        test_position(hm, TIME_SMPTE,   TIME_MS);
        test_position(hm, TIME_SAMPLES, TIME_MS);
        test_position(hm, TIME_BYTES,   TIME_MS);

        Sleep(400); /* Hear note */

        midiprop.tempo.cbStruct = sizeof(midiprop.tempo);
        rc = midiStreamProperty(hm, (void*)&midiprop, MIDIPROP_GET|MIDIPROP_TEMPO);
        ok(!rc, "midiStreamProperty TEMPO rc=%s\n", mmsys_error(rc));
        ok(0x0493E0==midiprop.tempo.dwTempo, "stream set tempo %u\n", midiprop.tdiv.dwTimeDiv);

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart #2 rc=%s\n", mmsys_error(rc));

        mhdr.dwFlags |= MHDR_ISSTRM;
        /* Preset flags (e.g. MHDR_ISSTRM) do not disturb. */
        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutPrepare used flags %x rc=%s\n", mhdr.dwFlags, mmsys_error(rc));
        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, offsetof(MIDIHDR,dwOffset));
        ok(!rc, "midiOutUnprepare used flags %x rc=%s\n", mhdr.dwFlags, mmsys_error(rc));

        rc = midiStreamRestart(hm);
        ok(!rc, "midiStreamRestart #3 rc=%s\n", mmsys_error(rc));
    }
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);
    ok(0==((MIDISHORTEVENT*)&strmEvents)[0].dwStreamID, "dwStreamID set to %x\n", ((LPMIDIEVENT)&strmEvents[0])->dwStreamID);

    /* dwBytesRecorded controls how much is played, not dwBufferLength
     * allowing to immediately forward packets from midiIn to midiOut */
    mhdr.dwOffset = 1234123123;
    mhdr.dwBufferLength = sizeof(strmNops);
    trace("buffer: %u\n", mhdr.dwBufferLength);
    mhdr.dwBytesRecorded = 0;
    mhdr.lpData = (LPSTR)&strmNops[0];
    strmNops[0].dwEvent |= MEVT_F_CALLBACK;
    strmNops[1].dwEvent |= MEVT_F_CALLBACK;

    rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
    ok(!rc, "midiOutPrepare rc=%s\n", mmsys_error(rc));

    rc = playStream(hm, &mhdr);
    ok(!rc, "midiStreamOut 0 bytes recorded rc=%s\n", mmsys_error(rc));

    test_notification(hwnd, "midiStreamOut", MOM_DONE, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "0 bytes recorded", 0, WHATEVER);

    /* FIXME: check dwOffset within callback
     * instead of the unspecified value afterwards */
    ok(1234123123==mhdr.dwOffset || broken(0==mhdr.dwOffset), "play 0 set MIDIHDR.dwOffset to %u\n", mhdr.dwOffset);
    /* w2k and later only set dwOffset when processing MEVT_T_CALLBACK,
     * while w9X/me/nt always sets it.  Have Wine behave like w2k because the
     * dwOffset slot does not exist in the small size MIDIHDR. */

    mhdr.dwOffset = 1234123123;
    mhdr.dwBytesRecorded = 1*sizeof(MIDISHORTEVENT);

    rc = playStream(hm, &mhdr);
    ok(!rc, "midiStreamOut 1 event out of 2 rc=%s\n", mmsys_error(rc));

    test_notification(hwnd, "1 of 2 events", MOM_POSITIONCB, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "1 of 2 events", MOM_DONE, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "1 of 2 events", 0, WHATEVER);
    ok(0==mhdr.dwOffset, "MIDIHDR.dwOffset 1/2 changed to %u\n", mhdr.dwOffset);

    mhdr.dwOffset = 1234123123;
    mhdr.dwBytesRecorded = 2*sizeof(MIDISHORTEVENT);

    rc = playStream(hm, &mhdr);
    ok(!rc, "midiStreamOut 1 event out of 2 rc=%s\n", mmsys_error(rc));

    test_notification(hwnd, "2 of 2 events", MOM_POSITIONCB, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "2 of 2 events", MOM_POSITIONCB, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "2 of 2 events", MOM_DONE, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "2 of 2 events", 0, WHATEVER);
    ok(sizeof(MIDISHORTEVENT)==mhdr.dwOffset, "MIDIHDR.dwOffset 2/2 changed to %u\n", mhdr.dwOffset);
    ok(mhdr.dwBytesRecorded == 2*sizeof(MIDISHORTEVENT), "dwBytesRecorded changed to %u\n", mhdr.dwBytesRecorded);

    strmNops[0].dwEvent &= ~MEVT_F_CALLBACK;
    strmNops[1].dwEvent &= ~MEVT_F_CALLBACK;
    mhdr.dwOffset = 1234123123;
    rc = playStream(hm, &mhdr);
    ok(!rc, "midiStreamOut 1 event out of 2 rc=%s\n", mmsys_error(rc));

    test_notification(hwnd, "0 CB in 2 events", MOM_DONE, (DWORD_PTR)&mhdr);
    test_notification(hwnd, "0 CB in 2 events", 0, WHATEVER);
    /* w9X/me/nt set dwOffset to the position played last */
    ok(1234123123==mhdr.dwOffset || broken(sizeof(MIDISHORTEVENT)==mhdr.dwOffset), "MIDIHDR.dwOffset nocb changed to %u\n", mhdr.dwOffset);

    mhdr.dwBytesRecorded = mhdr.dwBufferLength-1;
    rc = playStream(hm, &mhdr);
    ok(rc==MMSYSERR_INVALPARAM,"midiStreamOut dwBytesRecorded modulo MIDIEVENT rc=%s\n", mmsys_error(rc));
    if (!rc) {
         test_notification(hwnd, "2 of 2 events", MOM_DONE, (DWORD_PTR)&mhdr);
    }

    mhdr.dwBytesRecorded = mhdr.dwBufferLength+1;
    rc = playStream(hm, &mhdr);
    ok(rc==MMSYSERR_INVALPARAM,"midiStreamOut dwBufferLength<dwBytesRecorded rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "past MIDIHDR tests", 0, WHATEVER);

    rc = midiStreamStop(hm);
    ok(!rc, "midiStreamStop rc=%s\n", mmsys_error(rc));
    ok(mhdr.dwUser==0x56FA552C, "MIDIHDR.dwUser changed to %lx\n", mhdr.dwUser);

    rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
    ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));
    ok(0==strmNops[0].dwStreamID, "dwStreamID[0] set to %x\n", strmNops[0].dwStreamID);
    ok(0==strmNops[1].dwStreamID, "dwStreamID[1] set to %x\n", strmNops[1].dwStreamID);

    mhdr.dwBufferLength = 70000; /* > 64KB! */
    mhdr.lpData = HeapAlloc(GetProcessHeap(), 0 , mhdr.dwBufferLength);
    ok(mhdr.lpData!=NULL, "No %d bytes of memory!\n", mhdr.dwBufferLength);
    if (mhdr.lpData) {
        mhdr.dwFlags = 0;
        /* PrepareHeader detects the too large buffer is for a stream. */
        rc = midiOutPrepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        todo_wine ok(rc==MMSYSERR_INVALPARAM, "midiOutPrepare stream too large rc=%s\n", mmsys_error(rc));

        rc = midiOutUnprepareHeader((HMIDIOUT)hm, &mhdr, sizeof(mhdr));
        ok(!rc, "midiOutUnprepare rc=%s\n", mmsys_error(rc));

        HeapFree(GetProcessHeap(), 0, mhdr.lpData);
    }

    rc = midiStreamClose(hm);
    ok(!rc, "midiStreamClose rc=%s\n", mmsys_error(rc));
    test_notification(hwnd, "midiStreamClose", MOM_CLOSE, 0);
    test_notification(hwnd, "midiStream over", 0, WHATEVER);

    rc = midiStreamOpen(&hm, &udev, 1, 0, (DWORD_PTR)MYCBINST, CALLBACK_FUNCTION);
    ok(!rc /*w2k*/|| rc==MMSYSERR_INVALPARAM/*w98*/, "midiStreamOpen NULL function rc=%s\n", mmsys_error(rc));
    if (!rc) {
        trace("Device %d accepts NULL CALLBACK_FUNCTION\n", udev);
        rc = midiStreamClose(hm);
        ok(!rc, "midiStreamClose rc=%s\n", mmsys_error(rc));
    }

    rc = midiStreamOpen(&hm, &udev, 1, (DWORD_PTR)0xDEADBEEF, (DWORD_PTR)MYCBINST, CALLBACK_WINDOW);
    ok(rc==MMSYSERR_INVALPARAM, "midiStreamOpen bad window rc=%s\n", mmsys_error(rc));
    if (!rc) {
        rc = midiStreamClose(hm);
        ok(!rc, "midiStreamClose rc=%s\n", mmsys_error(rc));
    }
}

static BOOL scan_subkeys(HKEY parent, const LPCSTR *sub_keys)
{
    char name[64];
    DWORD index = 0;
    DWORD name_len = sizeof(name);
    BOOL found_vmware = FALSE;

    if (sub_keys[0] == NULL)
    {
       /* We're at the deepest level, check "Identifier" value now */
       char *test;
       if (RegQueryValueExA(parent, "Identifier", NULL, NULL, (LPBYTE) name, &name_len) != ERROR_SUCCESS)
           return FALSE;
       for (test = name; test < name + lstrlenA(name) - 6 && ! found_vmware; test++)
       {
           char c = test[6];
           test[6] = '\0';
           found_vmware = (lstrcmpiA(test, "VMware") == 0);
           test[6] = c;
       }
       return found_vmware;
    }

    while (RegEnumKeyExA(parent, index, name, &name_len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS &&
           ! found_vmware) {
        char c = name[lstrlenA(sub_keys[0])];
        name[lstrlenA(sub_keys[0])] = '\0';
        if (lstrcmpiA(name, sub_keys[0]) == 0) {
            HKEY sub_key;
            name[lstrlenA(sub_keys[0])] = c;
            if (RegOpenKeyExA(parent, name, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &sub_key) == ERROR_SUCCESS) {
                found_vmware = scan_subkeys(sub_key, sub_keys + 1);
                RegCloseKey(sub_key);
            }
        }

        name_len = sizeof(name);
        index++;
    }

    return found_vmware;
}

/*
 * Usual method to detect whether running inside a VMware virtual machine involves direct port I/O requiring
 * some assembly and an exception handler. Can't do that in Wine tests. Alternative method of querying WMI
 * is not available on NT4. So instead we look at the device map and check the Identifier value in the
 * registry keys HKLM\HARDWARE\DEVICEMAP\SCSI\Scsi Port x\Scsi Bus x\Target Id x\Logical Unit Id x (where
 * x is some number). If the Identifier value contains the string "VMware" we assume running in a VMware VM.
 */
static BOOL on_vmware(void)
{
    static const LPCSTR sub_keys[] = { "Scsi Port ", "Scsi Bus ", "Target Id ", "Logical Unit Id ", NULL };
    HKEY scsi;
    BOOL found_vmware = FALSE;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\Scsi", 0, KEY_ENUMERATE_SUB_KEYS, &scsi) != ERROR_SUCCESS)
        return FALSE;

    found_vmware = scan_subkeys(scsi, sub_keys);

    RegCloseKey(scsi);

    return found_vmware;
}

static void test_midi_outfns(HWND hwnd)
{
    HMIDIOUT hm;
    MMRESULT rc;
    UINT udev, ndevs = midiOutGetNumDevs();

    rc = midiOutOpen(&hm, ndevs, 0, 0, CALLBACK_NULL);
    ok(rc==MMSYSERR_BADDEVICEID, "midiOutOpen udev>max rc=%s\n", mmsys_error(rc));
    if (!rc) {
        rc = midiOutClose(hm);
        ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
    }
    if (!ndevs) {
        MIDIOUTCAPSA capsA;
        skip("Found no MIDI out device\n");

        rc = midiOutGetDevCapsA(MIDIMAPPER, &capsA, sizeof(capsA));
        /* GetDevCaps and Open must return compatible results */
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*nt,w2k*/), "midiOutGetDevCaps MAPPER with no MIDI rc=%s\n", mmsys_error(rc));

        rc = midiOutOpen(&hm, MIDIMAPPER, 0, 0, CALLBACK_NULL);
        if (rc==MIDIERR_INVALIDSETUP) todo_wine /* Wine without snd-seq */
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*w2k*/), "midiOutOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        else
        ok(rc==MMSYSERR_BADDEVICEID || broken(rc==MMSYSERR_NODRIVER /*w2k sound disabled*/),
           "midiOutOpen MAPPER with no MIDI rc=%s\n", mmsys_error(rc));
        if (!rc) {
            rc = midiOutClose(hm);
            ok(!rc, "midiOutClose rc=%s\n", mmsys_error(rc));
        }
        return;
    }
    trace("Found %d MIDI OUT devices\n", ndevs);

    test_midi_mci(hwnd);

    for (udev=0; udev < ndevs; udev++) {
        MIDIOUTCAPSA capsA;
        rc = midiOutGetDevCapsA(udev, &capsA, sizeof(capsA));
        if (rc || strcmp(capsA.szPname, "Creative Sound Blaster MPU-401") != 0 || ! on_vmware()) {
            trace("** Testing device %d\n", udev);
            test_midiOut_device(udev, hwnd);
            Sleep(800); /* Let the synth rest */
            test_midiStream(udev, hwnd);
            Sleep(800);
        }
        else
            win_skip("Skipping this device on VMware, driver problem\n");
    }
    trace("** Testing MIDI mapper\n");
    test_midiOut_device(MIDIMAPPER, hwnd);
    Sleep(800);
    test_midiStream(MIDIMAPPER, hwnd);
}

START_TEST(midi)
{
    HWND hwnd = 0;
    if (1) /* select 1 for CALLBACK_WINDOW or 0 for CALLBACK_FUNCTION */
    hwnd = CreateWindowExA(0, "static", "winmm midi test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    test_midi_infns(hwnd);
    test_midi_outfns(hwnd);
    if (hwnd) DestroyWindow(hwnd);
}
