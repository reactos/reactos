/*
 * Test winmm mci
 *
 * Copyright 2006 Jan Zerebecki
 *           2009 Jörg Höhle
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
#include "mmreg.h"
#include "wine/test.h"

/* The tests use the MCI's own save capability to create the tempfile.wav to play.
 * To use a pre-existing file, write-protect it. */
static MCIERROR ok_saved = MCIERR_FILE_NOT_FOUND;

typedef union {
      MCI_STATUS_PARMS    status;
      MCI_WAVE_SET_PARMS  set;
      MCI_WAVE_OPEN_PARMS open;
      MCI_SEEK_PARMS      seek;
    } MCI_PARMS_UNION;

static const char* dbg_mcierr(MCIERROR err)
{
     switch (err) {
     case 0: return "0=NOERROR";
#define X(label) case label: return #label ;
     X(MCIERR_INVALID_DEVICE_ID)
     X(MCIERR_UNRECOGNIZED_KEYWORD)
     X(MCIERR_UNRECOGNIZED_COMMAND)
     X(MCIERR_HARDWARE)
     X(MCIERR_INVALID_DEVICE_NAME)
     X(MCIERR_OUT_OF_MEMORY)
     X(MCIERR_DEVICE_OPEN)
     X(MCIERR_CANNOT_LOAD_DRIVER)
     X(MCIERR_MISSING_COMMAND_STRING)
     X(MCIERR_PARAM_OVERFLOW)
     X(MCIERR_MISSING_STRING_ARGUMENT)
     X(MCIERR_BAD_INTEGER)
     X(MCIERR_PARSER_INTERNAL)
     X(MCIERR_DRIVER_INTERNAL)
     X(MCIERR_MISSING_PARAMETER)
     X(MCIERR_UNSUPPORTED_FUNCTION)
     X(MCIERR_FILE_NOT_FOUND)
     X(MCIERR_DEVICE_NOT_READY)
     X(MCIERR_INTERNAL)
     X(MCIERR_DRIVER)
     X(MCIERR_CANNOT_USE_ALL)
     X(MCIERR_MULTIPLE)
     X(MCIERR_EXTENSION_NOT_FOUND)
     X(MCIERR_OUTOFRANGE)
     X(MCIERR_FLAGS_NOT_COMPATIBLE)
     X(MCIERR_FILE_NOT_SAVED)
     X(MCIERR_DEVICE_TYPE_REQUIRED)
     X(MCIERR_DEVICE_LOCKED)
     X(MCIERR_DUPLICATE_ALIAS)
     X(MCIERR_BAD_CONSTANT)
     X(MCIERR_MUST_USE_SHAREABLE)
     X(MCIERR_MISSING_DEVICE_NAME)
     X(MCIERR_BAD_TIME_FORMAT)
     X(MCIERR_NO_CLOSING_QUOTE)
     X(MCIERR_DUPLICATE_FLAGS)
     X(MCIERR_INVALID_FILE)
     X(MCIERR_NULL_PARAMETER_BLOCK)
     X(MCIERR_UNNAMED_RESOURCE)
     X(MCIERR_NEW_REQUIRES_ALIAS)
     X(MCIERR_NOTIFY_ON_AUTO_OPEN)
     X(MCIERR_NO_ELEMENT_ALLOWED)
     X(MCIERR_NONAPPLICABLE_FUNCTION)
     X(MCIERR_ILLEGAL_FOR_AUTO_OPEN)
     X(MCIERR_FILENAME_REQUIRED)
     X(MCIERR_EXTRA_CHARACTERS)
     X(MCIERR_DEVICE_NOT_INSTALLED)
     X(MCIERR_GET_CD)
     X(MCIERR_SET_CD)
     X(MCIERR_SET_DRIVE)
     X(MCIERR_DEVICE_LENGTH)
     X(MCIERR_DEVICE_ORD_LENGTH)
     X(MCIERR_NO_INTEGER)
     X(MCIERR_WAVE_OUTPUTSINUSE)
     X(MCIERR_WAVE_SETOUTPUTINUSE)
     X(MCIERR_WAVE_INPUTSINUSE)
     X(MCIERR_WAVE_SETINPUTINUSE)
     X(MCIERR_WAVE_OUTPUTUNSPECIFIED)
     X(MCIERR_WAVE_INPUTUNSPECIFIED)
     X(MCIERR_WAVE_OUTPUTSUNSUITABLE)
     X(MCIERR_WAVE_SETOUTPUTUNSUITABLE)
     X(MCIERR_WAVE_INPUTSUNSUITABLE)
     X(MCIERR_WAVE_SETINPUTUNSUITABLE)
     X(MCIERR_SEQ_DIV_INCOMPATIBLE)
     X(MCIERR_SEQ_PORT_INUSE)
     X(MCIERR_SEQ_PORT_NONEXISTENT)
     X(MCIERR_SEQ_PORT_MAPNODEVICE)
     X(MCIERR_SEQ_PORT_MISCERROR)
     X(MCIERR_SEQ_TIMER)
     X(MCIERR_SEQ_PORTUNSPECIFIED)
     X(MCIERR_SEQ_NOMIDIPRESENT)
     X(MCIERR_NO_WINDOW)
     X(MCIERR_CREATEWINDOW)
     X(MCIERR_FILE_READ)
     X(MCIERR_FILE_WRITE)
     X(MCIERR_NO_IDENTITY)
#undef X
     default: {
         static char name[20]; /* Not to be called twice in a parameter list! */
         sprintf(name, "MMSYSERR %u", err);
         return name;
         }
     }
}

static BOOL spurious_message(LPMSG msg)
{
  /* WM_DEVICECHANGE 0x0219 appears randomly */
  if(msg->message != MM_MCINOTIFY) {
    trace("skipping spurious message %04x\n",msg->message);
    return TRUE;
  }
  return FALSE;
}

static void test_notification(HWND hwnd, const char* command, WPARAM type)
{   /* Use type 0 as meaning no message */
    MSG msg;
    BOOL seen;
    do { seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE); }
    while(seen && spurious_message(&msg));
    if(type==0)
        ok(!seen, "Expect no message from command %s\n", command);
    else
        ok(seen, "PeekMessage should succeed for command %s\n", command);
    if(seen) {
        ok(msg.hwnd == hwnd, "Didn't get the handle to our test window\n");
        ok(msg.message == MM_MCINOTIFY, "got %04x instead of MM_MCINOTIFY from command %s\n", msg.message, command);
        ok(msg.wParam == type, "got %04lx instead of MCI_NOTIFY_xyz %04lx from command %s\n", msg.wParam, type, command);
    }
}
static void test_notification1(HWND hwnd, const char* command, WPARAM type)
{   /* This version works with todo_wine prefix. */
    MSG msg;
    BOOL seen;
    do { seen = PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE); }
    while(seen && spurious_message(&msg));
    if(type==0)
        ok(!seen, "Expect no message from command %s\n", command);
    else if(seen)
      ok(msg.message == MM_MCINOTIFY && msg.wParam == type,"got %04lx instead of MCI_NOTIFY_xyz %04lx from command %s\n", msg.wParam, type, command);
    else ok(seen, "PeekMessage should succeed for command %s\n", command);
}

static void test_openCloseWAVE(HWND hwnd)
{
    MCIERROR err;
    MCI_GENERIC_PARMS parm;
    const char command_open[] = "open new type waveaudio alias mysound notify";
    const char command_close_my[] = "close mysound notify";
    const char command_close_all[] = "close all notify";
    const char command_sysinfo[] = "sysinfo waveaudio quantity open";
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    test_notification(hwnd, "-prior to any command-", 0);

    err = mciSendString("sysinfo all quantity", buf, sizeof(buf), hwnd);
    todo_wine ok(!err,"mci %s returned %s\n", command_open, dbg_mcierr(err));
    if(!err) trace("[MCI] with %s drivers\n", buf);

    err = mciSendString("open new type waveaudio alias r shareable", buf, sizeof(buf), NULL);
    ok(err==MCIERR_UNSUPPORTED_FUNCTION,"mci open new shareable returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendString("close r", NULL, 0, NULL);
        ok(!err,"mci close shareable returned %s\n", dbg_mcierr(err));
    }

    err = mciSendString(command_open, buf, sizeof(buf), hwnd);
    ok(!err,"mci %s returned %s\n", command_open, dbg_mcierr(err));
    ok(!strcmp(buf,"1"), "mci open deviceId: %s, expected 1\n", buf);
    /* Wine<=1.1.33 used to ignore anything past alias XY */
    test_notification(hwnd,"open new alias notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendString("status mysound time format", buf, sizeof(buf), hwnd);
    ok(!err,"mci status time format returned %s\n", dbg_mcierr(err));
    if(!err) {
        if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_ENGLISH)
            ok(!strcmp(buf,"milliseconds"), "mci status time format: %s\n", buf);
        else trace("locale-dependent time format: %s (ms)\n", buf);
    }

    err = mciSendString(command_close_my, NULL, 0, hwnd);
    ok(!err,"mci %s returned %s\n", command_close_my, dbg_mcierr(err));
    test_notification(hwnd, command_close_my, MCI_NOTIFY_SUCCESSFUL);
    Sleep(5);
    test_notification(hwnd, command_close_my, 0);

    err = mciSendString("open no-such-file-exists.wav alias y", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND,"open no-such-file.wav returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendString("close y", NULL, 0, NULL);
        ok(!err,"close y returned %s\n", dbg_mcierr(err));
    }

    err = mciSendString("open no-such-dir\\file.wav alias y type waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND || broken(err==MCIERR_INVALID_FILE /* Win9X */),"open no-such-dir/file.wav returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendString("close y", NULL, 0, NULL);
        ok(!err,"close y returned %s\n", dbg_mcierr(err));
    }

    err = mciSendString(command_close_all, NULL, 0, NULL);
    todo_wine ok(!err,"mci %s (without buffer) returned %s\n", command_close_all, dbg_mcierr(err));

    memset(buf, 0, sizeof(buf));
    err = mciSendString(command_close_all, buf, sizeof(buf), hwnd);
    todo_wine ok(!err,"mci %s (with output buffer) returned %s\n", command_close_all, dbg_mcierr(err));
    ok(buf[0] == 0, "mci %s changed output buffer: %s\n", command_close_all, buf);
    /* No notification left, everything closed already */
    test_notification(hwnd, command_close_all, 0);
    /* TODO test close all sends one notification per open device */

    memset(buf, 0, sizeof(buf));
    err = mciSendString(command_sysinfo, buf, sizeof(buf), NULL);
    ok(!err,"mci %s returned %s\n", command_sysinfo, dbg_mcierr(err));
    todo_wine ok(buf[0] == '0' && buf[1] == 0, "mci %s, expected output buffer '0', got: '%s'\n", command_sysinfo, buf);

    err = mciSendString("open new type waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_NEW_REQUIRES_ALIAS,"mci open new without alias returned %s\n", dbg_mcierr(err));

    err = mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0); /* from MSDN */
    ok(!err,"mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, 0) returned %s\n", dbg_mcierr(err));

    err = mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, 0);
    ok(!err,"mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, 0) returned %s\n", dbg_mcierr(err));

    parm.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err,"mciSendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, hwnd) returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, command_close_all, 0); /* None left */
}

static void test_recordWAVE(HWND hwnd)
{
    WORD nch    = 1;
    WORD nbits  = 16;
    DWORD nsamp = 16000, expect;
    MCIERROR err, ok_pcm;
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    test_notification(hwnd, "-prior to recording-", 0);

    parm.open.lpstrDeviceType = "waveaudio";
    parm.open.lpstrElementName = ""; /* "new" at the command level */
    parm.open.lpstrAlias = "x"; /* to enable mciSendString */
    parm.open.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommand(0, MCI_OPEN,
        MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_ALIAS | MCI_NOTIFY,
        (DWORD_PTR)&parm);
    ok(!err,"mciCommand open new type waveaudio alias x notify: %s\n", dbg_mcierr(err));
    wDeviceID = parm.open.wDeviceID;

    /* In Wine, both MCI_Open and the individual drivers send notifications. */
    test_notification(hwnd, "open new", MCI_NOTIFY_SUCCESSFUL);
    todo_wine test_notification1(hwnd, "open new no #2", 0);

    /* Do not query time format as string because result depends on locale! */
    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    err = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status time format: %s\n", dbg_mcierr(err));
    ok(parm.status.dwReturn==MCI_FORMAT_MILLISECONDS,"status time format: %ld\n",parm.status.dwReturn);

    /* Info file fails until named in Open or Save. */
    err = mciSendString("info x file", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci info new file returned %s\n", dbg_mcierr(err));

    /* Check the default recording: 8-bits per sample, mono, 11kHz */
    err = mciSendString("status x samplespersec", buf, sizeof(buf), NULL);
    ok(!err,"mci status samplespersec returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"11025"), "mci status samplespersec expected 11025, got: %s\n", buf);

    /* MCI seems to solely support PCM, no need for ACM conversion. */
    err = mciSendString("set x format tag 2", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci set format tag 2 returned %s\n", dbg_mcierr(err));

    /* MCI appears to scan the available devices for support of this format,
     * returning MCIERR_OUTOFRANGE on machines with no sound.
     * Don't skip here, record will fail below. */
    err = mciSendString("set x format tag pcm", NULL, 0, NULL);
    ok(!err || err==MCIERR_OUTOFRANGE,"mci set format tag pcm returned %s\n", dbg_mcierr(err));
    ok_pcm = err;

    err = mciSendString("set x samplespersec 41000 alignment 4 channels 2", NULL, 0, NULL);
    ok(err==ok_pcm,"mci set samples+align+channels returned %s\n", dbg_mcierr(err));

    /* Investigate: on w2k, set samplespersec 22050 sets nChannels to 2!
     *  err = mciSendString("set x samplespersec 22050", NULL, 0, NULL);
     *  ok(!err,"mci set samplespersec returned %s\n", dbg_mcierr(err));
     */

    parm.set.wFormatTag = WAVE_FORMAT_PCM;
    parm.set.nSamplesPerSec = nsamp;
    parm.set.wBitsPerSample = nbits;
    parm.set.nChannels      = nch;
    parm.set.nBlockAlign    = parm.set.nChannels * parm.set.wBitsPerSample /8;
    parm.set.nAvgBytesPerSec= parm.set.nSamplesPerSec * parm.set.nBlockAlign;
    err = mciSendCommand(wDeviceID, MCI_SET,
        MCI_WAVE_SET_SAMPLESPERSEC | MCI_WAVE_SET_CHANNELS |
        MCI_WAVE_SET_BITSPERSAMPLE | MCI_WAVE_SET_BLOCKALIGN |
        MCI_WAVE_SET_AVGBYTESPERSEC| MCI_WAVE_SET_FORMATTAG, (DWORD_PTR)&parm);
    ok(err==ok_pcm,"mciCommand set wave format: %s\n", dbg_mcierr(err));

    /* A few ME machines pass all tests except set format tag pcm! */
    err = mciSendString("record x to 2000 wait", NULL, 0, hwnd);
    ok(err || !ok_pcm,"can record yet set wave format pcm returned %s\n", dbg_mcierr(ok_pcm));
    ok(!err || err==(ok_pcm==MCIERR_OUTOFRANGE ? MCIERR_WAVE_INPUTSUNSUITABLE : 0),"mci record to 2000 returned %s\n", dbg_mcierr(err));
    if(err) {
        if (err==MCIERR_WAVE_INPUTSUNSUITABLE)
             skip("Please install audio driver. Everything is skipped.\n");
        else skip("Cannot record cause %s. Everything is skipped.\n", dbg_mcierr(err));

        err = mciSendString("close x", NULL, 0, NULL);
        ok(!err,"mci close returned %s\n", dbg_mcierr(err));
        test_notification(hwnd,"record skipped",0);
        return;
    }

    /* Query some wave format parameters depending on the time format. */
    err = mciSendString("status x position", buf, sizeof(buf), NULL);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf,"2000"), "mci status position gave %s, expected 2000, some tests will fail\n", buf);

    err = mciSendString("set x time format 8", NULL, 0, NULL); /* bytes */
    ok(!err,"mci returned %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_POSITION;
    err = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status position: %s\n", dbg_mcierr(err));
    expect = 2 * nsamp * nch * nbits/8;
    if(!err) todo_wine ok(parm.status.dwReturn==expect,"recorded %lu bytes, expected %u\n",parm.status.dwReturn,expect);

    parm.set.dwTimeFormat = MCI_FORMAT_SAMPLES;
    err = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand set time format samples: %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_POSITION;
    err = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status position: %s\n", dbg_mcierr(err));
    expect = 2 * nsamp;
    if(!err) todo_wine ok(parm.status.dwReturn==expect,"recorded %lu samples, expected %u\n",parm.status.dwReturn,expect);

    err = mciSendString("set x time format milliseconds", NULL, 0, NULL);
    ok(!err,"mci set time format milliseconds returned %s\n", dbg_mcierr(err));

    err = mciSendString("save x tempfile1.wav", NULL, 0, NULL);
    ok(!err,"mci save returned %s\n", dbg_mcierr(err));

    err = mciSendString("save x tempfile.wav", NULL, 0, NULL);
    ok(!err,"mci save returned %s\n", dbg_mcierr(err));
    if(!err) ok_saved = 0;

    /* Save must not rename the original file. */
    if (!DeleteFile("tempfile1.wav"))
        todo_wine ok(FALSE, "Save must not rename the original file; DeleteFile returned %d\n", GetLastError());

    err = mciSendString("set x channels 2", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci set channels after saving returned %s\n", dbg_mcierr(err));

    parm.seek.dwTo = 600;
    err = mciSendCommand(wDeviceID, MCI_SEEK, MCI_TO | MCI_WAIT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand seek to 600: %s\n", dbg_mcierr(err));

    /* Truncate to current position */
    err = mciSendString("delete x", NULL, 0, NULL);
    todo_wine ok(!err,"mci delete returned %s\n", dbg_mcierr(err));

    buf[0]='\0';
    err = mciSendString("status x length", buf, sizeof(buf), NULL);
    ok(!err,"mci status length returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"600"), "mci status length after delete gave %s, expected 600\n", buf);

    err = mciSendString("close x", NULL, 0, NULL);
    ok(!err,"mci close returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"record complete",0);
}

static void test_playWAVE(HWND hwnd)
{
    MCIERROR err;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendString("open waveaudio!tempfile.wav alias mysound", NULL, 0, NULL);
    ok(err==ok_saved,"mci open waveaudio!tempfile.wav returned %s\n", dbg_mcierr(err));
    if(err) {
        skip("Cannot open waveaudio!tempfile.wav for playing (%s), skipping\n", dbg_mcierr(err));
        return;
    }

    err = mciSendString("status mysound length", buf, sizeof(buf), NULL);
    ok(!err,"mci status length returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"2000"), "mci status length gave %s, expected 2000, some tests will fail.\n", buf);

    err = mciSendString("cue output", NULL, 0, NULL);
    todo_wine ok(err==MCIERR_UNRECOGNIZED_COMMAND,"mci incorrect cue output returned %s\n", dbg_mcierr(err));

    /* Test MCI to the bones -- Some todo_wine from Cue and
     * from Play from 0 to 0 are not worth fixing. */
    err = mciSendString("cue mysound output notify", NULL, 0, hwnd);
    ok(!err,"mci cue output after open file returned %s\n", dbg_mcierr(err));
    /* Notification is delayed as a play thread is started. */
    todo_wine test_notification1(hwnd, "cue immediate", 0);

    /* Cue pretends to put the MCI into paused state. */
    err = mciSendString("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"paused"), "mci status mode: %s, expected (pseudo)paused\n", buf);

    /* Strange pause where Pause is rejected, unlike Play; Pause; Pause tested below */
    err = mciSendString("pause mysound", NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci pause after cue returned %s\n", dbg_mcierr(err));

    /* MCI appears to start the play thread in this border case.
     * Guessed that from (flaky) status mode and late notification arrival. */
    err = mciSendString("play mysound from 0 to 0 notify", NULL, 0, hwnd);
    ok(!err,"mci play from 0 to 0 returned %s\n", dbg_mcierr(err));
    todo_wine test_notification1(hwnd, "cue aborted by play", MCI_NOTIFY_ABORTED);
    /* play's own notification follows below */

    err = mciSendString("play mysound from 250 to 0", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play from 250 to 0 returned %s\n", dbg_mcierr(err));

    Sleep(50); /* Give play from 0 to 0 time to finish. */
    todo_wine test_notification1(hwnd, "play from 0 to 0", MCI_NOTIFY_SUCCESSFUL);

    err = mciSendString("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s after play from 0 to 0\n", buf);

    err = mciSendString("play MYSOUND from 250 to 0 notify", NULL, 0, hwnd);
    ok(err==MCIERR_OUTOFRANGE,"mci play from 250 to 0 notify returned %s\n", dbg_mcierr(err));
    /* No notification (checked below) sent if error */

    /* A second play caused Wine<1.1.33 to hang */
    err = mciSendString("play mysound from 500 to 1500 wait", NULL, 0, NULL);
    ok(!err,"mci play from 500 to 1500 returned %s\n", dbg_mcierr(err));

    memset(buf, 0, sizeof(buf));
    err = mciSendString("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"1500"), "mci status position: %s\n", buf);

    /* mci will not play position < current */
    err = mciSendString("play mysound to 1000", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 1000 returned %s\n", dbg_mcierr(err));

    /* mci will not play to > end */
    err = mciSendString("play mysound TO 3000 notify", NULL, 0, hwnd);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 3000 notify returned %s\n", dbg_mcierr(err));

    err = mciSendString("play mysound to 2000", NULL, 0, NULL);
    ok(!err,"mci play to 2000 returned %s\n", dbg_mcierr(err));

    /* Rejected while playing */
    err = mciSendString("cue mysound output", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci cue output while playing returned %s\n", dbg_mcierr(err));

    err = mciSendString("play mysound to 3000", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 3000 returned %s\n", dbg_mcierr(err));

    err = mciSendString("stop mysound Wait", NULL, 0, NULL);
    ok(!err,"mci stop wait returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "play/cue/pause/stop", 0);

    err = mciSendString("Seek Mysound to 250 wait Notify", NULL, 0, hwnd);
    ok(!err,"mci seek to 250 wait notify returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"seek wait notify",MCI_NOTIFY_SUCCESSFUL);

    memset(buf, 0, sizeof(buf));
    err = mciSendString("status mysound position notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position notify returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"250"), "mci status position: %s\n", buf);
    /* Immediate commands like status also send notifications. */
    test_notification(hwnd,"status position",MCI_NOTIFY_SUCCESSFUL);

    memset(buf, 0, sizeof(buf));
    err = mciSendString("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    /* Another play from == to testcase */
    err = mciSendString("play mysound to 250 wait notify", NULL, 0, hwnd);
    ok(!err,"mci play (from 250) to 250 returned %s\n", dbg_mcierr(err));
    todo_wine test_notification1(hwnd,"play to 250 wait notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendString("cue mysound output", NULL, 0, NULL);
    ok(!err,"mci cue output after play returned %s\n", dbg_mcierr(err));

    err = mciSendString("close mysound", NULL, 0, NULL);
    ok(!err,"mci close returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"after close",0);
}

static void test_asyncWAVE(HWND hwnd)
{
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    int err, p1, p2;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendString("open tempfile.wav alias mysound notify", buf, sizeof(buf), hwnd);
    ok(err==ok_saved,"mci open tempfile.wav returned %s\n", dbg_mcierr(err));
    if(err) {
        skip("Cannot open tempfile.wav for playing (%s), skipping\n", dbg_mcierr(err));
        return;
    }
    ok(!strcmp(buf,"1"), "mci open deviceId: %s, expected 1\n", buf);
    wDeviceID = atoi(buf);
    ok(wDeviceID,"mci open DeviceID: %d\n", wDeviceID);
    test_notification(hwnd,"open alias notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendString("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    err = mciSendString("play mysound notify", NULL, 0, hwnd);
    ok(!err,"mci play returned %s\n", dbg_mcierr(err));

    /* Give Wine's asynchronous thread time to start up.  Furthermore,
     * it uses 3 buffers per second, so that the positions reported
     * will be 333ms, 667ms etc. at best. */
    Sleep(100); /* milliseconds */

    /* Do not query time format as string because result depends on locale! */
    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    err = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status time format: %s\n", dbg_mcierr(err));
    if(!err) ok(parm.status.dwReturn==MCI_FORMAT_MILLISECONDS,"status time format: %ld\n",parm.status.dwReturn);

    parm.set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    err = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand set time format ms: %s\n", dbg_mcierr(err));

    buf[0]=0;
    err = mciSendString("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    ok(strcmp(buf,"2000"), "mci status position: %s, expected 2000\n", buf);
    trace("position after Sleep: %sms\n",buf);
    p2 = atoi(buf);
    /* Some machines reach 79ms only during the 100ms sleep. */
    ok(p2>=67,"not enough time elapsed %ums\n",p2);
    test_notification(hwnd,"play (nowait)",0);

    err = mciSendString("pause mysound wait", NULL, 0, hwnd);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));

    buf[0]=0;
    err = mciSendString("status mysound mode notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"paused"), "mci status mode: %s\n", buf);
    test_notification(hwnd,"play",MCI_NOTIFY_SUPERSEDED);
    test_notification(hwnd,"status",MCI_NOTIFY_SUCCESSFUL);

    buf[0]=0;
    err = mciSendString("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    trace("position while paused: %sms\n",buf);
    p1 = atoi(buf);
    ok(p1>=p2, "position not increasing: %u > %u\n", p2, p1);

    err = mciSendString("stop mysound wait", NULL, 0, NULL);
    ok(!err,"mci stop returned %s\n", dbg_mcierr(err));

    buf[0]=0;
    err = mciSendString("info mysound file notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci info file returned %s\n", dbg_mcierr(err));
    if(!err) { /* fully qualified name */
        int len = strlen(buf);
        todo_wine ok(len>2 && buf[1]==':',"Expected full pathname from info file: %s\n", buf);
        ok(len>=12 && !strcmp(&buf[len-12],"tempfile.wav"), "info file returned: %s\n", buf);
    }
    test_notification(hwnd,"info file",MCI_NOTIFY_SUCCESSFUL);

    buf[0]=0;
    err = mciSendString("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    buf[0]=0;
    err = mciSendString("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    trace("position once stopped: %sms\n",buf);
    p2 = atoi(buf);
    /* An XP machine let the position increase slightly after pause. */
    ok(p2>=p1 && p2<=p1+16,"position changed from %ums to %ums\n",p1,p2);

    /* No Resume once stopped (waveaudio, sequencer and cdaudio differ). */
    err = mciSendString("resume mysound wait", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci resume wait returned %s\n", dbg_mcierr(err));

    err = mciSendString("play mysound wait", NULL, 0, NULL);
    ok(!err,"mci play wait returned %s\n", dbg_mcierr(err));

    buf[0]=0;
    err = mciSendString("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"2000"), "mci status position: %s\n", buf);

    err = mciSendString("seek mysound to start wait", NULL, 0, NULL);
    ok(!err,"mci seek to start wait returned %s\n", dbg_mcierr(err));

    err = mciSendString("play mysound to 1000 notify", NULL, 0, hwnd);
    ok(!err,"mci play returned %s\n", dbg_mcierr(err));

    /* Sleep(200); not needed with Wine any more. */

    err = mciSendString("pause mysound notify", NULL, 0, NULL); /* notify no callback */
    ok(!err,"mci pause notify returned %s\n", dbg_mcierr(err));
    /* Supersede even though pause cannot notify given no callback */
    test_notification(hwnd,"pause aborted play #1 notification",MCI_NOTIFY_SUPERSEDED);
    test_notification(hwnd,"impossible pause notification",0);

    err = mciSendString("cue mysound output notify", NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci cue output while paused returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"cue output notify #2",0);

    err = mciSendString("resume mysound notify", NULL, 0, hwnd);
    ok(!err,"mci resume notify returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "resume notify", MCI_NOTIFY_SUCCESSFUL);

    /* Seek or even Stop used to hang Wine<1.1.32 on MacOS. */
    err = mciSendString("seek mysound to 0 wait", NULL, 0, NULL);
    ok(!err,"mci seek to start returned %s\n", dbg_mcierr(err));

    /* Seek stops. */
    err = mciSendString("status mysound mode", buf, sizeof(buf), NULL);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    err = mciSendString("seek mysound wait", NULL, 0, NULL);
    ok(err==MCIERR_MISSING_PARAMETER,"mci seek to nowhere returned %s\n", dbg_mcierr(err));

    /* cdaudio does not detect to start to end as error */
    err = mciSendString("seek mysound to start to 0", NULL, 0, NULL);
    ok(err==MCIERR_FLAGS_NOT_COMPATIBLE,"mci seek to start to 0 returned %s\n", dbg_mcierr(err));

    err = mciSendString("PLAY mysound to 1000 notify", NULL, 0, hwnd);
    ok(!err,"mci play to 1000 notify returned %s\n", dbg_mcierr(err));

    /* Sleep(200); not needed with Wine any more. */
    /* Give it 400ms and resume will appear to complete below. */

    err = mciSendString("pause mysound wait", NULL, 0, NULL);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));
    /* Unlike sequencer and cdaudio, waveaudio's pause does not abort. */
    test_notification(hwnd,"pause aborted play #2 notification",0);

    err = mciSendString("resume mysound wait", NULL, 0, NULL);
    ok(!err,"mci resume wait returned %s\n", dbg_mcierr(err));
    /* Resume is a short asynchronous call, something else is playing. */

    err = mciSendString("status mysound mode", buf, sizeof(buf), NULL);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"playing"), "mci status mode: %s\n", buf);

    /* Note extra space before alias */
    err = mciSendString("pause  mysound wait", NULL, 0, NULL);
    todo_wine ok(!err,"mci pause (space) wait returned %s\n", dbg_mcierr(err));

    err = mciSendString("pause mysound wait", NULL, 0, NULL);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));

    /* Better ask position only when paused, is it updated while playing? */
    buf[0]='\0';
    err = mciSendString("status mysound position", buf, sizeof(buf), NULL);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    /* TODO compare position < 900 */
    ok(strcmp(buf,"1000"), "mci resume waited\n");
    ok(strcmp(buf,"2000"), "mci resume played to end\n");
    trace("position after resume: %sms\n",buf);
    test_notification(hwnd,"play (aborted by pause/resume/pause)",0);

    err = mciSendString("close mysound wait", NULL, 0, NULL);
    ok(!err,"mci close wait returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"play (aborted by close)",MCI_NOTIFY_ABORTED);
}

static void test_AutoOpenWAVE(HWND hwnd)
{
    /* This test used(?) to cause intermittent crashes when Wine exits, after
     * fixme:winmm:MMDRV_Exit Closing while ll-driver open
     */
    MCIERROR err, ok_snd;
    char buf[512], path[300], command[330];
    memset(buf, 0, sizeof(buf)); memset(path, 0, sizeof(path));

    /* Do not crash on NULL buffer pointer */
    err = mciSendString("sysinfo waveaudio quantity open", NULL, 0, NULL);
    ok(err==MCIERR_PARAM_OVERFLOW,"mci sysinfo without buffer returned %s\n", dbg_mcierr(err));

    err = mciSendString("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity open returned %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf,"0"), "sysinfo quantity open expected 0, got: %s, some more tests will fail.\n", buf);

    ok_snd = waveOutGetNumDevs() ? 0 : MCIERR_HARDWARE;
    err = mciSendString("sound NoSuchSoundDefined wait", NULL, 0, NULL);
    todo_wine ok(err==ok_snd,"mci sound NoSuchSoundDefined returned %s\n", dbg_mcierr(err));

    err = mciSendString("sound SystemExclamation notify wait", NULL, 0, hwnd);
    todo_wine ok(err==ok_snd,"mci sound SystemExclamation returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "sound notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    buf[0]=0;
    err = mciSendString("sysinfo waveaudio name 1 open", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_OUTOFRANGE,"sysinfo waveaudio name 1 returned %s\n", dbg_mcierr(err));
    if(!err) trace("sysinfo dangling open alias: %s\n", buf);

    err = mciSendString("play no-such-file-exists.wav notify", buf, sizeof(buf), NULL);
    if(err==MCIERR_FILE_NOT_FOUND) { /* a Wine detector */
        /* Unsupported auto-open leaves the file open, preventing clean-up */
        skip("Skipping auto-open tests in Wine\n");
        return;
    }

    err = mciSendString("play tempfile.wav notify", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN,"mci auto-open play notify returned %s\n", dbg_mcierr(err));

    if(err) /* FIXME: don't open twice yet, it confuses Wine. */
    err = mciSendString("play tempfile.wav", buf, sizeof(buf), hwnd);
    ok(err==ok_saved,"mci auto-open play returned %s\n", dbg_mcierr(err));

    if(err==MCIERR_FILE_NOT_FOUND) {
        skip("Cannot open tempfile.wav for auto-play, skipping\n");
        return;
    }

    buf[0]=0;
    err = mciSendString("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity after auto-open returned %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf,"1"), "sysinfo quantity open expected 1, got: %s\n", buf);

    buf[0]=0;
    err = mciSendString("sysinfo waveaudio name 1 open", buf, sizeof(buf), NULL);
    todo_wine ok(!err,"mci sysinfo waveaudio name after auto-open returned %s\n", dbg_mcierr(err));
    /* This is the alias, not necessarily a file name. */
    if(!err) ok(!strcmp(buf,"tempfile.wav"), "sysinfo name 1 open: %s\n", buf);

    /* Save the full pathname to the file. */
    err = mciSendString("info tempfile.wav file", path, sizeof(path), NULL);
    ok(!err,"mci info tempfile.wav file returned %s\n", dbg_mcierr(err));
    if(err) strcpy(path,"tempfile.wav");

    err = mciSendString("status tempfile.wav mode", NULL, 0, hwnd);
    ok(!err,"mci status tempfile.wav mode without buffer returned %s\n", dbg_mcierr(err));

    sprintf(command,"status \"%s\" mode",path);
    err = mciSendString(command, buf, sizeof(buf), hwnd);
    ok(!err,"mci status \"%s\" mode returned %s\n", path, dbg_mcierr(err));

    buf[0]=0;
    err = mciSendString("status tempfile.wav mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status tempfile.wav mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"playing"), "mci auto-open status mode, got: %s\n", buf);

    err = mciSendString("open tempfile.wav", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_DEVICE_OPEN, "mci open from auto-open returned %s\n", dbg_mcierr(err));

    /* w2k/xp and Wine differ. While the device is busy playing, it is
     * regularly open and accessible via the filename: subsequent
     * commands must not cause auto-open each.  In Wine, a subsequent
     * command with notify request may cause the initial play
     * notification to be superseded, in turn causing MCI to close the
     * device.  I.e. MCI uses the auto-open notification for itself,
     * that's why it's not available to the app.  On w2k/xp,
     * subsequent commands with notify requests are returned with
     * MCIERR_NOTIFY_ON_AUTO_OPEN and thus don't abort the original
     * command.
     */
    err = mciSendString("status tempfile.wav mode notify", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN, "mci status auto-open notify returned %s\n", dbg_mcierr(err));
    if(!err) {
        trace("Wine style MCI auto-close upon notification\n");

        /* "playing" because auto-close comes after the status call. */
        todo_wine ok(!strcmp(buf,"playing"), "mci auto-open status mode notify, got: %s\n", buf);
        /* fixme:winmm:MMDRV_Exit Closing while ll-driver open
         *  is explained by failure to auto-close a device. */
        test_notification(hwnd,"status notify",MCI_NOTIFY_SUCCESSFUL);
        /* MCI received NOTIFY_SUPERSEDED and auto-closed the device. */
        Sleep(16);
        test_notification(hwnd,"auto-open",0);
    } else if(err==MCIERR_NOTIFY_ON_AUTO_OPEN) { /* MS style */
        trace("MS style MCI auto-open forbids notification\n");

        err = mciSendString("pause tempfile.wav", NULL, 0, hwnd);
        ok(!err,"mci auto-still-open pause returned %s\n", dbg_mcierr(err));

        err = mciSendString("status tempfile.wav mode", buf, sizeof(buf), hwnd);
        ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
        if(!err) ok(!strcmp(buf,"paused"), "mci auto-open status mode, got: %s\n", buf);

        /* Auto-close */
        err = mciSendString("stop tempfile.wav", NULL, 0, hwnd);
        ok(!err,"mci auto-still-open stop returned %s\n", dbg_mcierr(err));
        Sleep(16); /* makes sysinfo quantity open below succeed */
    }

    err = mciSendString("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity open after close returned %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf,"0"), "sysinfo quantity open expected 0 after auto-close, got: %s\n", buf);

    /* w95-WinME (not w2k/XP) switch to C:\ after auto-playing once.  Prevent
     * MCIERR_FILE_NOT_FOUND by using the full path name from the Info file command.
     */
    sprintf(command,"status \"%s\" mode wait",path);
    err = mciSendString(command, buf, sizeof(buf), hwnd);
    ok(!err,"mci re-auto-open status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"stopped"), "mci re-auto-open status mode, got: %s\n", buf);

    err = mciSendString("capability waveaudio device type", buf, sizeof(buf), hwnd);
    ok(!err,"mci capability device type returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"waveaudio"), "mci capability device type response: %s\n", buf);

    /* waveaudio forbids Pause without Play. */
    sprintf(command,"pause \"%s\"",path);
    err = mciSendString(command, NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci auto-open pause returned %s\n", dbg_mcierr(err));
}

START_TEST(mci)
{
    MCIERROR err;
    HWND hwnd;
    hwnd = CreateWindowExA(0, "static", "winmm test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    test_openCloseWAVE(hwnd);
    test_recordWAVE(hwnd);
    test_playWAVE(hwnd);
    test_asyncWAVE(hwnd);
    test_AutoOpenWAVE(hwnd);
    /* Win9X hangs when exiting with something still open. */
    err = mciSendString("close all", NULL, 0, hwnd);
    todo_wine ok(!err,"final close all returned %s\n", dbg_mcierr(err));
    ok(DeleteFile("tempfile.wav")||ok_saved,"Delete tempfile.wav (cause auto-open?)\n");
    DestroyWindow(hwnd);
}
