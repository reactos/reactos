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
      MCI_INFO_PARMSA     info;
      MCI_STATUS_PARMS    status;
      MCI_WAVE_SET_PARMS  set;
      MCI_WAVE_OPEN_PARMSA open;
      MCI_GETDEVCAPS_PARMS caps;
      MCI_SYSINFO_PARMSA  sys;
      MCI_SEEK_PARMS      seek;
      MCI_GENERIC_PARMS   gen;
    } MCI_PARMS_UNION;

const char* dbg_mcierr(MCIERROR err)
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

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), 0, 0);
    return lstrcmpA(buf, stra);
}

static void test_mciParser(HWND hwnd)
{
    MCIERROR err;
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    test_notification(hwnd, "-prior to parser test-", 0);

    /* Get a handle on an MCI device, works even without sound. */
    parm.open.lpstrDeviceType = "waveaudio";
    parm.open.lpstrElementName = ""; /* "new" at the command level */
    parm.open.lpstrAlias = "x"; /* to enable mciSendStringA */
    parm.open.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommandA(0, MCI_OPEN,
            MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_ALIAS | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err,"mciCommand open new type waveaudio alias x notify: %s\n", dbg_mcierr(err));
    wDeviceID = parm.open.wDeviceID;
    ok(!strcmp(parm.open.lpstrDeviceType,"waveaudio"), "open modified device type\n");

    test_notification(hwnd, "MCI_OPEN", MCI_NOTIFY_SUCCESSFUL);
    test_notification(hwnd, "MCI_OPEN no #2", 0);

    err = mciSendStringA("open avivideo alias a", buf, sizeof(buf), hwnd);
    ok(!err,"open another: %s\n", dbg_mcierr(err));

    buf[0]='z';
    err = mciSendStringA("", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_MISSING_COMMAND_STRING,"empty string: %s\n", dbg_mcierr(err));
    ok(!buf[0], "error buffer %s\n", buf);

    buf[0]='d';
    err = mciSendStringA("open", buf, sizeof(buf), NULL);
    ok(err==MCIERR_MISSING_DEVICE_NAME,"open void: %s\n", dbg_mcierr(err));
    ok(!buf[0], "open error buffer %s\n", buf);

    err = mciSendStringA("open notify", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_INVALID_DEVICE_NAME,"open notify: %s\n", dbg_mcierr(err));

    err = mciSendStringA("open new", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_NEW_REQUIRES_ALIAS,"open new: %s\n", dbg_mcierr(err));

    err = mciSendStringA("open new type waveaudio alias r shareable shareable", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_DUPLICATE_FLAGS,"open new: %s\n", dbg_mcierr(err));
    if(!err) mciSendStringA("close r", NULL, 0, NULL);

    err = mciSendStringA("status x position wait wait", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_DUPLICATE_FLAGS,"status wait wait: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x length length", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_FLAGS_NOT_COMPATIBLE,"status 2xlength: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x length position", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_FLAGS_NOT_COMPATIBLE,"status length+position: %s\n", dbg_mcierr(err));

    buf[0]='I';
    err = mciSendStringA("set x time format milliseconds time format ms", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_FLAGS_NOT_COMPATIBLE,"status length+position: %s\n", dbg_mcierr(err));
    ok(!buf[0], "set error buffer %s\n", buf);

    /* device's response, not a parser test */
    err = mciSendStringA("status x", buf, sizeof(buf), NULL);
    ok(err==MCIERR_MISSING_PARAMETER,"status waveaudio nokeyword: %s\n", dbg_mcierr(err));

    buf[0]='G';
    err = mciSendStringA("status a", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_UNSUPPORTED_FUNCTION,"status avivideo nokeyword: %s\n", dbg_mcierr(err));
    ok(!buf[0], "status error buffer %s\n", buf);

    err = mciSendStringA("status x track", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"status waveaudio no track: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x track 3", buf, sizeof(buf), NULL);
    ok(err==MCIERR_MISSING_PARAMETER,"status waveaudio track 3: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x 2 track 3", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_OUTOFRANGE,"status 2(position) track 3: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x 0x4", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_BAD_CONSTANT, "status 0x4: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x 4", buf, sizeof(buf), hwnd);
    ok(!err,"status 4(mode): %s\n", dbg_mcierr(err));
    if(!err)ok(!strcmp(buf,"stopped"), "status 4(mode), got: %s\n", buf);

    err = mciSendStringA("status x 4 notify", buf, sizeof(buf), hwnd);
    todo_wine ok(!err,"status 4(mode) notify: %s\n", dbg_mcierr(err));
    if(!err)ok(!strcmp(buf,"stopped"), "status 4(mode), got: %s\n", buf);
    test_notification(hwnd, "status 4 notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("set x milliseconds", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_UNRECOGNIZED_KEYWORD,"set milliseconds: %s\n", dbg_mcierr(err));

    err = mciSendStringA("set x milliseconds ms", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_UNRECOGNIZED_KEYWORD,"set milliseconds ms: %s\n", dbg_mcierr(err));

    err = mciSendStringA("capability x can   save", buf, sizeof(buf), hwnd);
    todo_wine ok(!err,"capability can (space) save: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x nsa", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_BAD_CONSTANT,"status nsa: %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek x to 0:0:0:0:0", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"seek to 0:0:0:0:0 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek x to 0:0:0:0:", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"seek to 0:0:0:0: returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek x to :0:0:0:0", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"seek to :0:0:0:0 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek x to 256:0:0:0", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"seek to 256:0:0:0 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek x to 0:256", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"seek to 0:256 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status all time format", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_CANNOT_USE_ALL,"status all: %s\n", dbg_mcierr(err));

    err = mciSendStringA("cue all", buf, sizeof(buf), NULL);
    ok(err==MCIERR_UNRECOGNIZED_COMMAND,"cue all: %s\n", dbg_mcierr(err));

    err = mciSendStringA("open all", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_CANNOT_USE_ALL,"open all: %s\n", dbg_mcierr(err));

    /* avivideo is not a known MCI_DEVTYPE resource name */
    err = mciSendStringA("sysinfo avivideo quantity", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_DEVICE_TYPE_REQUIRED,"sysinfo sequencer quantity: %s\n", dbg_mcierr(err));

    err = mciSendStringA("sysinfo digitalvideo quantity", buf, sizeof(buf), hwnd);
    ok(!err,"sysinfo digitalvideo quantity: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"0"), "sysinfo digitalvideo quantity returned %s\n", buf);

    /* quantity 0 yet open 1 (via type "avivideo"), fun */
    err = mciSendStringA("sysinfo digitalvideo quantity open", buf, sizeof(buf), hwnd);
    ok(!err,"sysinfo digitalvideo quantity open: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"1"), "sysinfo digitalvideo quantity open returned %s\n", buf);

    err = mciSendStringA("put a window at 0 0", buf, sizeof(buf), NULL);
    ok(err==MCIERR_BAD_INTEGER,"put incomplete rect: %s\n", dbg_mcierr(err));

    /*w9X-w2k report code from device last opened, newer versions compare them all
     * and return the one error code or MCIERR_MULTIPLE if they differ. */
    err = mciSendStringA("pause all", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_MULTIPLE || broken(err==MCIERR_NONAPPLICABLE_FUNCTION),"pause all: %s\n", dbg_mcierr(err));
    ok(!buf[0], "pause error buffer %s\n", buf);

    /* MCI_STATUS' dwReturn is a DWORD_PTR, others' a plain DWORD. */
    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status time format: %s\n", dbg_mcierr(err));
    if(!err) ok(MCI_FORMAT_MILLISECONDS==parm.status.dwReturn,"status time format: %ld\n",parm.status.dwReturn);

    parm.status.dwItem = MCI_STATUS_MODE;
    parm.status.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(MCI_MODE_STOP==parm.status.dwReturn,"STATUS mode: %ld\n",parm.status.dwReturn);

    err = mciSendStringA("status x mode", buf, sizeof(buf), hwnd);
    ok(!err,"status mode: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "stopped"), "status mode is %s\n", buf);

    parm.caps.dwItem = MCI_GETDEVCAPS_USES_FILES;
    parm.caps.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand getdevcaps files: %s\n", dbg_mcierr(err));
    if(!err) ok(1==parm.caps.dwReturn,"getdevcaps files: %d\n",parm.caps.dwReturn);

    parm.caps.dwItem = MCI_GETDEVCAPS_HAS_VIDEO;
    parm.caps.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand getdevcaps video: %s\n", dbg_mcierr(err));
    if(!err) ok(0==parm.caps.dwReturn,"getdevcaps video: %d\n",parm.caps.dwReturn);

    parm.caps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    parm.caps.dwReturn = 0xFEEDABAD;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand getdevcaps video: %s\n", dbg_mcierr(err));
    if(!err) ok(MCI_DEVTYPE_WAVEFORM_AUDIO==parm.caps.dwReturn,"getdevcaps device type: %d\n",parm.caps.dwReturn);

    err = mciSendStringA("capability x uses files", buf, sizeof(buf), hwnd);
    ok(!err,"capability files: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "true"), "capability files is %s\n", buf);

    err = mciSendStringA("capability x has video", buf, sizeof(buf), hwnd);
    ok(!err,"capability video: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "false"), "capability video is %s\n", buf);

    err = mciSendStringA("capability x device type", buf, sizeof(buf), hwnd);
    ok(!err,"capability device type: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf, "waveaudio"), "capability device type is %s\n", buf);

    err = mciSendCommandA(wDeviceID, MCI_CLOSE, 0, 0);
    ok(!err,"mciCommand close returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("close a", buf, sizeof(buf), hwnd);
    ok(!err,"close avi: %s\n", dbg_mcierr(err));

    test_notification(hwnd, "-end of 1st set-", 0);
}

static void test_openCloseWAVE(HWND hwnd)
{
    MCIERROR err;
    MCI_PARMS_UNION parm;
    const char command_open[] = "open new type waveaudio alias mysound notify";
    const char command_close_my[] = "close mysound notify";
    const char command_close_all[] = "close all notify";
    const char command_sysinfo[] = "sysinfo waveaudio quantity open";
    char buf[1024];
    DWORD intbuf[3] = { 0xDEADF00D, 99, 0xABADCAFE };
    memset(buf, 0, sizeof(buf));
    test_notification(hwnd, "-prior to any command-", 0);

    /* Avoid Sysinfo quantity with notify because Win9x and newer differ. */
    err = mciSendStringA("sysinfo all quantity", buf, sizeof(buf), hwnd);
    ok(!err,"mci sysinfo all quantity returned %s\n", dbg_mcierr(err));
    if(!err) trace("[MCI] with %s drivers\n", buf);

    parm.sys.lpstrReturn = (LPSTR)&intbuf[1];
    parm.sys.dwRetSize = sizeof(DWORD);
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_QUANTITY | MCI_WAIT,
            (DWORD_PTR)&parm);
    ok(!err, "mciCommand sysinfo all quantity returned %s\n", dbg_mcierr(err));
    if(!err) ok(atoi(buf)==intbuf[1],"sysinfo all quantity string and command differ\n");

    parm.sys.dwRetSize = sizeof(DWORD)-1;
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_QUANTITY, (DWORD_PTR)&parm);
    ok(err == MCIERR_PARAM_OVERFLOW || broken(!err/* Win9x */),
            "mciCommand sysinfo with too small buffer returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("open new type waveaudio alias r shareable", buf, sizeof(buf), NULL);
    ok(err==MCIERR_UNSUPPORTED_FUNCTION,"mci open new shareable returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendStringA("close r", NULL, 0, NULL);
        ok(!err,"mci close shareable returned %s\n", dbg_mcierr(err));
    }

    err = mciGetDeviceIDA("waveaudio");
    ok(!err, "mciGetDeviceIDA waveaudio returned %u, expected 0\n", err);

    err = mciSendStringA(command_open, buf, sizeof(buf), hwnd);
    ok(!err,"mci %s returned %s\n", command_open, dbg_mcierr(err));
    ok(!strcmp(buf,"1"), "mci open deviceId: %s, expected 1\n", buf);
    /* Wine<=1.1.33 used to ignore anything past alias XY */
    test_notification(hwnd,"open new alias notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status mysound time format", buf, sizeof(buf), hwnd);
    ok(!err,"mci status time format returned %s\n", dbg_mcierr(err));
    if(!err) {
        if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_ENGLISH)
            ok(!strcmp(buf,"milliseconds"), "mci status time format: %s\n", buf);
        else trace("locale-dependent time format: %s (ms)\n", buf);
    }

    memset(buf, 0, sizeof(buf));
    parm.sys.dwNumber = 1;
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = sizeof(buf);
    parm.sys.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO,
            MCI_SYSINFO_NAME | MCI_SYSINFO_OPEN | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err,"mciCommand MCI_SYSINFO all name 1 open notify: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"mysound"), "sysinfo name returned %s\n", buf);
    test_notification(hwnd, "SYSINFO name notify\n", MCI_NOTIFY_SUCCESSFUL);

    memset(buf, 0, sizeof(buf));
    parm.sys.dwNumber = 1;
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = 8; /* mysound\0 */
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME | MCI_SYSINFO_OPEN,
            (DWORD_PTR)&parm);
    ok(!err,"mciCommand MCI_SYSINFO all name 1 open buffer[8]: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"mysound"), "sysinfo name returned %s\n", buf);

    memset(buf, 0, sizeof(buf));
    /* dwRetSize counts characters, not bytes, despite what MSDN says. */
    parm.sys.dwNumber = 1;
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = 8; /* mysound\0 */
    /* MCI_..._PARMSA and PARMSW share the same layout, use one for both tests. */
    err = mciSendCommandW(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME | MCI_SYSINFO_OPEN, (DWORD_PTR)&parm);
    ok(!err || broken(err==MMSYSERR_NOTSUPPORTED/* Win9x */), "mciCommandW MCI_SYSINFO all name 1 open buffer[8]: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp_wa((LPWSTR)buf,"mysound"), "sysinfo name 1 open contents\n");

    memset(buf, 0, sizeof(buf));
    buf[0] = 'Y';
    parm.sys.dwNumber = 1;
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = 7; /* too short for mysound\0 */
    err = mciSendCommandW(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME | MCI_SYSINFO_OPEN, (DWORD_PTR)&parm);
    ok(err==MCIERR_PARAM_OVERFLOW || broken(err==MMSYSERR_NOTSUPPORTED/* Win9x */), "mciCommandW MCI_SYSINFO all name 1 open too small: %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"Y"), "output buffer %s\n", buf);

    /* Win9x overwrites the tiny buffer and returns success, newer versions signal overflow. */
    memset(buf, 0, sizeof(buf));
    buf[0] = 'Y';
    parm.sys.dwNumber = 1;
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO; /* ignored */
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = 2; /* too short for mysound\0 */
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_NAME | MCI_SYSINFO_OPEN,
            (DWORD_PTR)&parm);
    ok(err==MCIERR_PARAM_OVERFLOW || broken(!err /* Win9x */),"mciCommand MCI_SYSINFO all name 1 open too small: %s\n", dbg_mcierr(err));
    ok(!strcmp(buf, err ? "Y" : "mysound"), "sysinfo short name returned %s\n", buf);

    err = mciSendStringA("sysinfo mysound quantity open", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_DEVICE_TYPE_REQUIRED,"sysinfo alias quantity: %s\n", dbg_mcierr(err));

    err = mciSendStringA("sysinfo nosuchalias quantity open", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_DEVICE_TYPE_REQUIRED,"sysinfo unknown quantity open: %s\n", dbg_mcierr(err));

    err = mciSendStringA("sysinfo all installname", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_CANNOT_USE_ALL,"sysinfo all installname: %s\n", dbg_mcierr(err));

    buf[0] = 'M'; buf[1] = 0;
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = sizeof(buf);
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_SYSINFO, MCI_SYSINFO_INSTALLNAME, (DWORD_PTR)&parm);
    ok(err==MCIERR_CANNOT_USE_ALL,"mciCommand MCI_SYSINFO all installname: %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"M"), "output buffer %s\n", buf);

    err = mciSendStringA("sysinfo nodev installname", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_INVALID_DEVICE_NAME,"sysinfo nodev installname: %s\n", dbg_mcierr(err));
    ok(!buf[0], "sysinfo error buffer %s\n", buf);

    buf[0] = 'K';
    parm.sys.lpstrReturn = buf;
    parm.sys.dwRetSize = sizeof(buf);
    err = mciSendCommandW(24000, MCI_SYSINFO, MCI_SYSINFO_INSTALLNAME, (DWORD_PTR)&parm);
    ok(err==MCIERR_INVALID_DEVICE_NAME || broken(err==MMSYSERR_NOTSUPPORTED/* Win9x */), "mciCommand MCI_SYSINFO nodev installname: %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"K"), "output buffer %s\n", buf);

    buf[0] = 0; buf[1] = 'A'; buf[2] = 'j'; buf[3] = 0;
    parm.info.lpstrReturn = buf;
    parm.info.dwRetSize = 2;
    err = mciSendCommandA(1, MCI_INFO, MCI_INFO_PRODUCT, (DWORD_PTR)&parm);
    ok(!err, "mciCommand MCI_INFO product: %s\n", dbg_mcierr(err));
    ok(buf[0] /* && !buf[1] */ && (buf[2] == 'j' || broken(!buf[2])), "info product output buffer %s\n", buf);
    /* Producing non-ASCII multi-byte output, native forgets to zero-terminate a too small buffer
     * with SendStringA, while SendStringW works correctly (jap. and chin. locale): ignore buf[1] */
    /* Bug in 64 bit Vista/w2k8/w7: mciSendStringW is used! (not in xp nor w2k3) */

    buf[0] = 'K'; buf[1] = 0;
    parm.info.dwRetSize = sizeof(buf);
    err = mciSendCommandW(1, MCI_INFO, 0x07000000, (DWORD_PTR)&parm);
    ok(err==MCIERR_UNRECOGNIZED_KEYWORD || broken(err==MMSYSERR_NOTSUPPORTED/* Win9x */), "mciCommand MCI_INFO other: %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"K"), "info output buffer %s\n", buf);

    err = mciGetDeviceIDA("all");
    ok(err == MCI_ALL_DEVICE_ID, "mciGetDeviceIDA all returned %u, expected MCI_ALL_DEVICE_ID\n", err);

    err = mciSendStringA(command_close_my, NULL, 0, hwnd);
    ok(!err,"mci %s returned %s\n", command_close_my, dbg_mcierr(err));
    test_notification(hwnd, command_close_my, MCI_NOTIFY_SUCCESSFUL);
    Sleep(5);
    test_notification(hwnd, command_close_my, 0);

    err = mciSendStringA("open no-such-file-exists.wav alias y buffer 6", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND,"open no-such-file.wav returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendStringA("close y", NULL, 0, NULL);
        ok(!err,"close y returned %s\n", dbg_mcierr(err));
    }

    err = mciSendStringA("open no-such-dir\\file.wav alias y type waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND || broken(err==MCIERR_INVALID_FILE /* Win9X */),"open no-such-dir/file.wav returned %s\n", dbg_mcierr(err));
    if(!err) {
        err = mciSendStringA("close y", NULL, 0, NULL);
        ok(!err,"close y returned %s\n", dbg_mcierr(err));
    }

    err = mciSendStringA("open ! alias no", buf, sizeof(buf), NULL);
    ok(err==MCIERR_INVALID_DEVICE_NAME,"open !(void): %s\n", dbg_mcierr(err));

    err = mciSendStringA("open !no-such-file-exists.wav alias no", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND || /* Win9X */err==MCIERR_INVALID_DEVICE_NAME,"open !name: %s\n", dbg_mcierr(err));

    /* FILE_NOT_FOUND stems from mciwave,
     * the complete name including ! is passed through since NT */
    err = mciSendStringA("open nosuchdevice!tempfile.wav alias no", buf, sizeof(buf), NULL);
    ok(err==MCIERR_FILE_NOT_FOUND || /* Win9X */err==MCIERR_INVALID_DEVICE_NAME,"open nosuchdevice!name: %s\n", dbg_mcierr(err));
    /* FIXME? use broken(INVALID_DEVICE_NAME) and have Wine not mimic Win9X? */

    err = mciSendStringA("close waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_INVALID_DEVICE_NAME,"close waveaudio: %s\n", dbg_mcierr(err));

    err = mciSendStringA(command_close_all, NULL, 0, NULL);
    ok(!err,"mci %s (without buffer) returned %s\n", command_close_all, dbg_mcierr(err));

    err = mciSendStringA(command_close_all, buf, sizeof(buf), hwnd);
    ok(!err,"mci %s (with output buffer) returned %s\n", command_close_all, dbg_mcierr(err));
    ok(buf[0] == 0, "mci %s output buffer: %s\n", command_close_all, buf);
    /* No notification left, everything closed already */
    test_notification(hwnd, command_close_all, 0);
    /* TODO test close all sends one notification per open device */

    err = mciSendStringA(command_sysinfo, buf, sizeof(buf), NULL);
    ok(!err,"mci %s returned %s\n", command_sysinfo, dbg_mcierr(err));
    ok(buf[0] == '0' && buf[1] == 0, "mci %s, expected output buffer '0', got: '%s'\n", command_sysinfo, buf);

    err = mciSendStringA("open new type waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_NEW_REQUIRES_ALIAS,"mci open new without alias returned %s\n", dbg_mcierr(err));

    parm.open.lpstrDeviceType = (LPSTR)MCI_DEVTYPE_WAVEFORM_AUDIO;
    err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&parm);
    ok(!err,"mciCommand OPEN_TYPE_ID waveaudio: %s\n", dbg_mcierr(err));

    if(!err) {
        MCIDEVICEID wDeviceID = parm.open.wDeviceID;
        parm.caps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
        err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&parm);
        ok(!err,"mciCommand MCI_GETDEVCAPS device type: %s\n", dbg_mcierr(err));
        ok(MCI_DEVTYPE_WAVEFORM_AUDIO==parm.caps.dwReturn,"mciCommand GETDEVCAPS says %u, expected %u\n", parm.caps.dwReturn, MCI_DEVTYPE_WAVEFORM_AUDIO);
    }

    ok(0xDEADF00D==intbuf[0] && 0xABADCAFE==intbuf[2],"DWORD buffer corruption\n");

    err = mciGetDeviceIDA("waveaudio");
    ok(err == 1, "mciGetDeviceIDA waveaudio returned %u, expected 1\n", err);

    err = mciSendStringA("open no-such-file.wav alias waveaudio", buf, sizeof(buf), NULL);
    ok(err==MCIERR_DUPLICATE_ALIAS, "mci open alias waveaudio returned %s\n", dbg_mcierr(err));
    /* If it were not already in use, open avivideo alias waveaudio would succeed,
     * making for funny test cases. */

    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0); /* from MSDN */
    ok(!err, "mciCommand close returned %s\n", dbg_mcierr(err));

    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, 0);
    ok(!err, "mciCommand close returned %s\n", dbg_mcierr(err));

    parm.gen.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err, "mciCommand close returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, command_close_all, 0); /* None left */
}

static void test_recordWAVE(HWND hwnd)
{
    WORD nch    = 1;
    WORD nbits  = 16;
    DWORD nsamp = 16000, expect;
    UINT ndevs  = waveInGetNumDevs();
    MCIERROR err, ok_pcm;
    MCIDEVICEID wDeviceID;
    MCI_PARMS_UNION parm;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    test_notification(hwnd, "-prior to recording-", 0);

    parm.open.lpstrDeviceType = "waveaudio";
    parm.open.lpstrElementName = ""; /* "new" at the command level */
    parm.open.lpstrAlias = "x"; /* to enable mciSendStringA */
    parm.open.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommandA(0, MCI_OPEN,
            MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_ALIAS | MCI_NOTIFY, (DWORD_PTR)&parm);
    ok(!err,"mciCommand open new type waveaudio alias x notify: %s\n", dbg_mcierr(err));
    wDeviceID = parm.open.wDeviceID;

    err = mciGetDeviceIDA("x");
    ok(err == wDeviceID, "mciGetDeviceIDA x returned %u, expected %u\n", err, wDeviceID);

    /* Only the alias is looked up. */
    err = mciGetDeviceIDA("waveaudio");
    ok(!err, "mciGetDeviceIDA waveaudio returned %u, expected 0\n", err);

    test_notification(hwnd, "open new", MCI_NOTIFY_SUCCESSFUL);
    test_notification(hwnd, "open new no #2", 0);

    /* Do not query time format as string because result depends on locale! */
    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status time format: %s\n", dbg_mcierr(err));
    ok(parm.status.dwReturn==MCI_FORMAT_MILLISECONDS,"status time format: %ld\n",parm.status.dwReturn);

    /* Info file fails until named in Open or Save. */
    err = mciSendStringA("info x file", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci info new file returned %s\n", dbg_mcierr(err));
    ok(!buf[0], "info error buffer %s\n", buf);

    err = mciSendStringA("status x length", buf, sizeof(buf), NULL);
    todo_wine ok(!err,"status x length initial: %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"0"), "mci status length expected 0, got: %s\n", buf);

    /* Check the default recording: 8-bits per sample, mono, 11kHz */
    err = mciSendStringA("status x samplespersec", buf, sizeof(buf), NULL);
    ok(!err,"mci status samplespersec returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"11025"), "mci status samplespersec expected 11025, got: %s\n", buf);

    /* MCI seems to solely support PCM, no need for ACM conversion. */
    err = mciSendStringA("set x format tag 2", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci set format tag 2 returned %s\n", dbg_mcierr(err));

    /* MCI appears to scan the available devices for support of this format,
     * returning MCIERR_OUTOFRANGE on machines with no sound.
     * However some w2k8/w7 machines return no error when there's no wave
     * input device (perhaps querying waveOutGetNumDevs instead of waveIn?),
     * still the record command below fails with MCIERR_WAVE_INPUTSUNSUITABLE.
     * Don't skip here, record will fail below. */
    err = mciSendStringA("set x format tag pcm", NULL, 0, NULL);
    ok(!err || err==MCIERR_OUTOFRANGE,"mci set format tag pcm returned %s\n", dbg_mcierr(err));
    ok_pcm = err;

    /* MSDN warns against not setting all wave format parameters.
     * Indeed, it produces strange results, incl.
     * inconsistent PCMWAVEFORMAT headers in the saved file.
     */
    err = mciSendStringA("set x bytespersec 22050 alignment 2 samplespersec 11025 channels 1 bitspersample 16", NULL, 0, NULL);
    ok(err==ok_pcm,"mci set 5 wave parameters returned %s\n", dbg_mcierr(err));
    /* Investigate: on w2k, set samplespersec 22050 sets nChannels to 2!
     *  err = mciSendStringA("set x samplespersec 22050", NULL, 0, NULL);
     *  ok(!err,"mci set samplespersec returned %s\n", dbg_mcierr(err));
     */

    /* Checks are generally performed immediately. */
    err = mciSendStringA("set x bitspersample 4", NULL, 0, NULL);
    todo_wine ok(err==MCIERR_OUTOFRANGE,"mci set bitspersample 4 returned %s\n", dbg_mcierr(err));

    parm.set.wFormatTag = WAVE_FORMAT_PCM;
    parm.set.nSamplesPerSec = nsamp;
    parm.set.wBitsPerSample = nbits;
    parm.set.nChannels      = nch;
    parm.set.nBlockAlign    = parm.set.nChannels * parm.set.wBitsPerSample /8;
    parm.set.nAvgBytesPerSec= parm.set.nSamplesPerSec * parm.set.nBlockAlign;
    err = mciSendCommandA(wDeviceID, MCI_SET,
            MCI_WAVE_SET_SAMPLESPERSEC | MCI_WAVE_SET_CHANNELS | MCI_WAVE_SET_BITSPERSAMPLE |
            MCI_WAVE_SET_BLOCKALIGN | MCI_WAVE_SET_AVGBYTESPERSEC| MCI_WAVE_SET_FORMATTAG,
            (DWORD_PTR)&parm);
    ok(err==ok_pcm,"mciCommand set wave format: %s\n", dbg_mcierr(err));

    parm.caps.dwItem = MCI_WAVE_GETDEVCAPS_INPUTS;
    parm.caps.dwCallback = (DWORD_PTR)hwnd;
    err = mciSendCommandA(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM | MCI_NOTIFY,
            (DWORD_PTR)&parm);
    ok(!err,"mciCommand MCI_GETDEVCAPS inputs: %s\n", dbg_mcierr(err));
    ok(parm.caps.dwReturn==ndevs,"mciCommand GETDEVCAPS claims %u inputs, expected %u\n", parm.caps.dwReturn, ndevs);
    ok(!ok_pcm || !parm.caps.dwReturn,"No input device accepts PCM!?\n");
    test_notification(hwnd, "GETDEVCAPS inputs", MCI_NOTIFY_SUCCESSFUL);

    /* A few ME machines pass all tests except set format tag pcm! */
    err = mciSendStringA("record x to 2000 wait", NULL, 0, hwnd);
    ok(err || !ok_pcm,"can record yet set wave format pcm returned %s\n", dbg_mcierr(ok_pcm));
    if(!ndevs) todo_wine /* with sound disabled */
    ok(ndevs>0 ? !err : err==MCIERR_WAVE_INPUTSUNSUITABLE,"mci record to 2000 returned %s\n", dbg_mcierr(err));
    else
    ok(ndevs>0 ? !err : err==MCIERR_WAVE_INPUTSUNSUITABLE,"mci record to 2000 returned %s\n", dbg_mcierr(err));
    if(err) {
        if (err==MCIERR_WAVE_INPUTSUNSUITABLE)
             skip("Please install audio driver. Everything is skipped.\n");
        else skip("Cannot record cause %s. Everything is skipped.\n", dbg_mcierr(err));

        err = mciSendStringA("close x", NULL, 0, NULL);
        ok(!err,"mci close returned %s\n", dbg_mcierr(err));
        test_notification(hwnd,"record skipped",0);
        return;
    }

    /* Query some wave format parameters depending on the time format. */
    err = mciSendStringA("status x position", buf, sizeof(buf), NULL);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    if(!err) todo_wine ok(!strcmp(buf,"2000"), "mci status position gave %s, expected 2000, some tests will fail\n", buf);

    err = mciSendStringA("set x time format 8", NULL, 0, NULL); /* bytes */
    ok(!err,"mci returned %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_POSITION;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status position: %s\n", dbg_mcierr(err));
    expect = 2 * nsamp * nch * nbits/8;
    if(!err) todo_wine ok(parm.status.dwReturn==expect,"recorded %lu bytes, expected %u\n",parm.status.dwReturn,expect);

    parm.set.dwTimeFormat = MCI_FORMAT_SAMPLES;
    err = mciSendCommandA(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand set time format samples: %s\n", dbg_mcierr(err));

    parm.status.dwItem = MCI_STATUS_POSITION;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status position: %s\n", dbg_mcierr(err));
    expect = 2 * nsamp;
    if(!err) todo_wine ok(parm.status.dwReturn==expect,"recorded %lu samples, expected %u\n",parm.status.dwReturn,expect);

    err = mciSendStringA("set x time format milliseconds", NULL, 0, NULL);
    ok(!err,"mci set time format milliseconds returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("save x tempfile1.wav", NULL, 0, NULL);
    ok(!err,"mci save returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("save x tempfile.wav", NULL, 0, NULL);
    ok(!err,"mci save returned %s\n", dbg_mcierr(err));
    if(!err) ok_saved = 0;

    /* Save must not rename the original file. */
    if (!DeleteFileA("tempfile1.wav"))
        todo_wine ok(FALSE, "Save must not rename the original file; DeleteFileA returned %d\n",
                GetLastError());

    err = mciSendStringA("set x channels 2", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci set channels after saving returned %s\n", dbg_mcierr(err));

    parm.seek.dwTo = 600;
    err = mciSendCommandA(wDeviceID, MCI_SEEK, MCI_TO | MCI_WAIT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand seek to 600: %s\n", dbg_mcierr(err));

    /* Truncate to current position */
    err = mciSendStringA("delete x", NULL, 0, NULL);
    todo_wine ok(!err,"mci delete returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status x length", buf, sizeof(buf), NULL);
    ok(!err,"mci status length returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"600"), "mci status length after delete gave %s, expected 600\n", buf);

    err = mciSendStringA("close x", NULL, 0, NULL);
    ok(!err,"mci close returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"record complete",0);
}

static void test_playWAVE(HWND hwnd)
{
    MCIERROR err;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendStringA("open waveaudio!tempfile.wav alias mysound", NULL, 0, NULL);
    ok(err==ok_saved,"mci open waveaudio!tempfile.wav returned %s\n", dbg_mcierr(err));
    if(err) {
        skip("Cannot open waveaudio!tempfile.wav for playing (%s), skipping\n", dbg_mcierr(err));
        return;
    }

    err = mciGetDeviceIDA("mysound");
    ok(err == 1, "mciGetDeviceIDA mysound returned %u, expected 1\n", err);

    err = mciGetDeviceIDA("tempfile.wav");
    ok(!err, "mciGetDeviceIDA tempfile.wav returned %u, expected 0\n", err);

    err = mciGetDeviceIDA("waveaudio");
    ok(!err, "mciGetDeviceIDA waveaudio returned %u, expected 0\n", err);

    err = mciSendStringA("status mysound length", buf, sizeof(buf), NULL);
    ok(!err,"mci status length returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"2000"), "mci status length gave %s, expected 2000, some tests will fail.\n", buf);

    err = mciSendStringA("cue output", NULL, 0, NULL);
    ok(err==MCIERR_UNRECOGNIZED_COMMAND,"mci incorrect cue output returned %s\n", dbg_mcierr(err));

    /* Test MCI to the bones -- Some todo_wine from Cue and
     * from Play from 0 to 0 are not worth fixing. */
    err = mciSendStringA("cue mysound output notify", NULL, 0, hwnd);
    ok(!err,"mci cue output after open file returned %s\n", dbg_mcierr(err));
    /* Notification is delayed as a play thread is started. */
    todo_wine test_notification(hwnd, "cue immediate", 0);

    /* Cue pretends to put the MCI into paused state. */
    err = mciSendStringA("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"paused"), "mci status mode: %s, expected (pseudo)paused\n", buf);

    /* Strange pause where Pause is rejected, unlike Play; Pause; Pause tested below */
    err = mciSendStringA("pause mysound", NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci pause after cue returned %s\n", dbg_mcierr(err));

    /* MCI appears to start the play thread in this border case.
     * Guessed that from (flaky) status mode and late notification arrival. */
    err = mciSendStringA("play mysound from 0 to 0 notify", NULL, 0, hwnd);
    ok(!err,"mci play from 0 to 0 returned %s\n", dbg_mcierr(err));
    todo_wine test_notification(hwnd, "cue aborted by play", MCI_NOTIFY_ABORTED);
    /* play's own notification follows below */

    err = mciSendStringA("play mysound from 250 to 0", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play from 250 to 0 returned %s\n", dbg_mcierr(err));

    Sleep(50); /* Give play from 0 to 0 time to finish. */
    todo_wine test_notification(hwnd, "play from 0 to 0", MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s after play from 0 to 0\n", buf);

    err = mciSendStringA("play MYSOUND from 250 to 0 notify", NULL, 0, hwnd);
    ok(err==MCIERR_OUTOFRANGE,"mci play from 250 to 0 notify returned %s\n", dbg_mcierr(err));
    /* No notification (checked below) sent if error */

    /* A second play caused Wine<1.1.33 to hang */
    err = mciSendStringA("play mysound from 500 to 220:5:0 wait", NULL, 0, NULL);
    ok(!err,"mci play from 500 to 220:5:0 (=1500) returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"1500"), "mci status position: %s\n", buf);

    /* mci will not play position < current */
    err = mciSendStringA("play mysound to 1000", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 1000 returned %s\n", dbg_mcierr(err));

    /* mci will not play to > end */
    err = mciSendStringA("play mysound TO 3000 notify", NULL, 0, hwnd);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 3000 notify returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("play mysound to 2000", NULL, 0, NULL);
    ok(!err,"mci play to 2000 returned %s\n", dbg_mcierr(err));

    /* Rejected while playing */
    err = mciSendStringA("cue mysound output", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci cue output while playing returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("play mysound to 3000", NULL, 0, NULL);
    ok(err==MCIERR_OUTOFRANGE,"mci play to 3000 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("stop mysound Wait", NULL, 0, NULL);
    ok(!err,"mci stop wait returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "play/cue/pause/stop", 0);

    err = mciSendStringA("Seek Mysound to 250 wait Notify", NULL, 0, hwnd);
    ok(!err,"mci seek to 250 wait notify returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"seek wait notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("seek mysound to 0xfa", NULL, 0, NULL);
    ok(err==MCIERR_BAD_INTEGER,"mci seek to 0xfa returned %s\n", dbg_mcierr(err));

    /* MCI_INTEGER always accepts colon notation */
    err = mciSendStringA("seek mysound to :1", NULL, 0, NULL);
    ok(!err,"mci seek to :1 (=256) returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek mysound to 250::", NULL, 0, NULL);
    ok(!err,"mci seek to 250:: returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("seek mysound to 250:0", NULL, 0, NULL);
    ok(!err,"mci seek to 250:0 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound position notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position notify returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"250"), "mci status position: %s\n", buf);
    /* Immediate commands like status also send notifications. */
    test_notification(hwnd,"status position",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    /* Another play from == to testcase */
    err = mciSendStringA("play mysound to 250 wait notify", NULL, 0, hwnd);
    ok(!err,"mci play (from 250) to 250 returned %s\n", dbg_mcierr(err));
    todo_wine test_notification(hwnd,"play to 250 wait notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("cue mysound output", NULL, 0, NULL);
    ok(!err,"mci cue output after play returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("close mysound", NULL, 0, NULL);
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

    err = mciSendStringA("open tempfile.wav alias mysound notify type waveaudio", buf, sizeof(buf), hwnd);
    ok(err==ok_saved,"mci open tempfile.wav returned %s\n", dbg_mcierr(err));
    if(err) {
        skip("Cannot open tempfile.wav for playing (%s), skipping\n", dbg_mcierr(err));
        return;
    }
    ok(!strcmp(buf,"1"), "mci open deviceId: %s, expected 1\n", buf);
    wDeviceID = atoi(buf);
    ok(wDeviceID,"mci open DeviceID: %d\n", wDeviceID);
    test_notification(hwnd,"open alias notify",MCI_NOTIFY_SUCCESSFUL);

    err = mciGetDeviceIDA("mysound");
    ok(err == wDeviceID, "mciGetDeviceIDA alias returned %u, expected %u\n", err, wDeviceID);

    /* Only the alias is looked up. */
    err = mciGetDeviceIDA("tempfile.wav");
    ok(!err, "mciGetDeviceIDA tempfile.wav returned %u, expected 0\n", err);

    err = mciGetDeviceIDA("waveaudio");
    ok(!err, "mciGetDeviceIDA waveaudio returned %u, expected 0\n", err);

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    err = mciSendStringA("play mysound notify", NULL, 0, hwnd);
    ok(!err,"mci play returned %s\n", dbg_mcierr(err));

    Sleep(500); /* milliseconds */

    /* Do not query time format as string because result depends on locale! */
    parm.status.dwItem = MCI_STATUS_TIME_FORMAT;
    err = mciSendCommandA(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&parm);
    ok(!err,"mciCommand status time format: %s\n", dbg_mcierr(err));
    if(!err) ok(parm.status.dwReturn==MCI_FORMAT_MILLISECONDS,"status time format: %ld\n",parm.status.dwReturn);

    parm.set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    err = mciSendCommandA(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&parm);
    ok(!err,"mciCommand set time format ms: %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    trace("position after Sleep: %sms\n", buf);
    p2 = atoi(buf);
    /* Check that the 2s sound plays at a normal pace, giving a wide margin to
     * account for timing granularity and small delays.
     */
    todo_wine ok(350 <= p2 && p2 <= 600, "%ums is not in the expected 350-600ms range\n", p2);
    /* Wine's asynchronous thread needs some time to start up. Furthermore, it
     * uses 3 buffers per second, so that the positions reported will be 333ms,
     * 667ms etc. at best, which is why it fails the above test. So add a
     * second test specifically to prevent Wine from getting even worse.
     * FIXME: To be removed when Wine is fixed and passes the above test.
     */
    ok(350 <= p2 && p2 <= 1000, "%ums is not even in the expected 350-1000ms range\n", p2);
    test_notification(hwnd,"play (nowait)",0);

    err = mciSendStringA("pause mysound wait", NULL, 0, hwnd);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound mode notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"paused"), "mci status mode: %s\n", buf);
    test_notification(hwnd,"play",MCI_NOTIFY_SUPERSEDED);
    test_notification(hwnd,"status",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    trace("position while paused: %sms\n",buf);
    p1 = atoi(buf);
    ok(p1>=p2, "position not increasing: %u > %u\n", p2, p1);

    err = mciSendStringA("stop mysound wait", NULL, 0, NULL);
    ok(!err,"mci stop returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("info mysound file notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci info file returned %s\n", dbg_mcierr(err));
    if(!err) { /* fully qualified name */
        int len = strlen(buf);
        todo_wine ok(len>2 && buf[1]==':',"Expected full pathname from info file: %s\n", buf);
        ok(len>=12 && !strcmp(&buf[len-12],"tempfile.wav"), "info file returned: %s\n", buf);
    }
    test_notification(hwnd,"info file",MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    err = mciSendStringA("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    trace("position once stopped: %sms\n",buf);
    p2 = atoi(buf);
    /* An XP machine let the position increase slightly after pause. */
    ok(p2>=p1 && p2<=p1+16,"position changed from %ums to %ums\n",p1,p2);

    /* No Resume once stopped (waveaudio, sequencer and cdaudio differ). */
    err = mciSendStringA("resume mysound wait", NULL, 0, NULL);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci resume wait returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("play mysound wait", NULL, 0, NULL);
    ok(!err,"mci play wait returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound position", buf, sizeof(buf), hwnd);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    todo_wine ok(!strcmp(buf,"2000"), "mci status position: %s\n", buf);

    err = mciSendStringA("seek mysound to start wait", NULL, 0, NULL);
    ok(!err,"mci seek to start wait returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("play mysound to 1000 notify", NULL, 0, hwnd);
    ok(!err,"mci play returned %s\n", dbg_mcierr(err));

    /* Sleep(200); not needed with Wine any more. */

    err = mciSendStringA("pause mysound notify", NULL, 0, NULL); /* notify no callback */
    ok(!err,"mci pause notify returned %s\n", dbg_mcierr(err));
    /* Supersede even though pause cannot notify given no callback */
    test_notification(hwnd,"pause aborted play #1 notification",MCI_NOTIFY_SUPERSEDED);
    test_notification(hwnd,"impossible pause notification",0);

    err = mciSendStringA("cue mysound output notify", NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci cue output while paused returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"cue output notify #2",0);

    err = mciSendStringA("resume mysound notify", NULL, 0, hwnd);
    ok(!err,"mci resume notify returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "resume notify", MCI_NOTIFY_SUCCESSFUL);

    /* Seek or even Stop used to hang Wine<1.1.32 on MacOS. */
    err = mciSendStringA("seek mysound to 0 wait", NULL, 0, NULL);
    ok(!err,"mci seek to start returned %s\n", dbg_mcierr(err));

    /* Seek stops. */
    err = mciSendStringA("status mysound mode", buf, sizeof(buf), NULL);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"stopped"), "mci status mode: %s\n", buf);

    err = mciSendStringA("seek mysound wait", NULL, 0, NULL);
    ok(err==MCIERR_MISSING_PARAMETER,"mci seek to nowhere returned %s\n", dbg_mcierr(err));

    /* cdaudio does not detect to start to end as error */
    err = mciSendStringA("seek mysound to start to 0", NULL, 0, NULL);
    ok(err==MCIERR_FLAGS_NOT_COMPATIBLE,"mci seek to start to 0 returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("PLAY mysound to 1000 notify", NULL, 0, hwnd);
    ok(!err,"mci play to 1000 notify returned %s\n", dbg_mcierr(err));

    /* Sleep(200); not needed with Wine any more. */
    /* Give it 400ms and resume will appear to complete below. */

    err = mciSendStringA("pause mysound wait", NULL, 0, NULL);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));
    /* Unlike sequencer and cdaudio, waveaudio's pause does not abort. */
    test_notification(hwnd,"pause aborted play #2 notification",0);

    err = mciSendStringA("resume mysound wait", NULL, 0, NULL);
    ok(!err,"mci resume wait returned %s\n", dbg_mcierr(err));
    /* Resume is a short asynchronous call, something else is playing. */

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), NULL);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"playing"), "mci status mode: %s\n", buf);

    /* Note extra space before alias */
    err = mciSendStringA("pause  mysound wait", NULL, 0, NULL);
    todo_wine ok(!err,"mci pause (space) wait returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("pause mysound wait", NULL, 0, NULL);
    ok(!err,"mci pause wait returned %s\n", dbg_mcierr(err));

    /* Better ask position only when paused, is it updated while playing? */
    err = mciSendStringA("status mysound position", buf, sizeof(buf), NULL);
    ok(!err,"mci status position returned %s\n", dbg_mcierr(err));
    /* TODO compare position < 900 */
    ok(strcmp(buf,"1000"), "mci resume waited\n");
    ok(strcmp(buf,"2000"), "mci resume played to end\n");
    trace("position after resume: %sms\n",buf);
    test_notification(hwnd,"play (aborted by pause/resume/pause)",0);

    err = mciSendStringA("close mysound wait", NULL, 0, NULL);
    ok(!err,"mci close wait returned %s\n", dbg_mcierr(err));
    test_notification(hwnd,"play (aborted by close)",MCI_NOTIFY_ABORTED);
}

static void test_AutoOpenWAVE(HWND hwnd)
{
    /* This test used(?) to cause intermittent crashes when Wine exits, after
     * fixme:winmm:MMDRV_Exit Closing while ll-driver open
     */
    UINT ndevs = waveOutGetNumDevs();
    MCIERROR err, ok_snd = ndevs ? 0 : MCIERR_HARDWARE;
    MCI_PARMS_UNION parm;
    char buf[512], path[300], command[330];
    DWORD intbuf[3] = { 0xDEADF00D, 99, 0xABADCAFE };
    memset(buf, 0, sizeof(buf)); memset(path, 0, sizeof(path));

    /* Do not crash on NULL buffer pointer */
    err = mciSendStringA("sysinfo waveaudio quantity open", NULL, 0, NULL);
    ok(err==MCIERR_PARAM_OVERFLOW,"mci sysinfo without buffer returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity open returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"0"), "sysinfo quantity open expected 0, got: %s, some more tests will fail.\n", buf);

    /* Who knows why some MS machines pass all tests but return MCIERR_HARDWARE here? */
    /* Wine returns MCIERR_HARDWARE when no default sound is found in win.ini or the registry. */
    err = mciSendStringA("sound NoSuchSoundDefined wait", NULL, 0, NULL);
    ok(err==ok_snd || err==MCIERR_HARDWARE, "mci sound NoSuchSoundDefined returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("sound SystemExclamation notify wait", NULL, 0, hwnd);
    ok(err==ok_snd || err==MCIERR_HARDWARE, "mci sound SystemExclamation returned %s\n", dbg_mcierr(err));
    test_notification(hwnd, "sound notify", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    Sleep(16); /* time to auto-close makes sysinfo below return expected error */
    err = mciSendStringA("sysinfo waveaudio notify name 1 open", buf, sizeof(buf), hwnd);
    ok(err==MCIERR_OUTOFRANGE,"sysinfo waveaudio name 1 returned %s\n", dbg_mcierr(err));
    if(!err) trace("sysinfo dangling open alias: %s\n", buf);
    test_notification(hwnd, "sysinfo name outofrange\n", err ? 0 : MCI_NOTIFY_SUCCESSFUL);

    err = mciSendStringA("play no-such-file-exists.wav notify", buf, sizeof(buf), NULL);
    todo_wine ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN,"mci auto-open notify returned %s\n", dbg_mcierr(err));
    /* FILE_NOT_FOUND in Wine because auto-open fails before testing the notify flag */

    test_notification(hwnd, "-prior to auto-open-", 0);

    err = mciSendStringA("play tempfile.wav notify", buf, sizeof(buf), hwnd);
    if(ok_saved==MCIERR_FILE_NOT_FOUND) todo_wine /* same as above */
    ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN,"mci auto-open play notify returned %s\n", dbg_mcierr(err));
    else
    ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN,"mci auto-open play notify returned %s\n", dbg_mcierr(err));

    if(err) /* FIXME: don't open twice yet, it confuses Wine. */
    err = mciSendStringA("play tempfile.wav", buf, sizeof(buf), hwnd);
    ok(err==ok_saved,"mci auto-open play returned %s\n", dbg_mcierr(err));

    if(err==MCIERR_FILE_NOT_FOUND) {
        skip("Cannot open tempfile.wav for auto-play, skipping\n");
        return;
    }

    err = mciSendStringA("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity after auto-open returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"1"), "sysinfo quantity open expected 1, got: %s\n", buf);

    parm.sys.lpstrReturn = (LPSTR)&intbuf[1];
    parm.sys.dwRetSize = 2*sizeof(DWORD); /* only one DWORD is used */
    parm.sys.wDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
    err = mciSendCommandA(0, MCI_SYSINFO, MCI_SYSINFO_QUANTITY | MCI_SYSINFO_OPEN, (DWORD_PTR)&parm);
    ok(!err, "mciCommand sysinfo waveaudio open notify returned %s\n", dbg_mcierr(err));
    if(!err) ok(atoi(buf)==intbuf[1],"sysinfo waveaudio quantity open string and command differ\n");

    err = mciSendStringA("sysinfo waveaudio name 1 open notify", buf, sizeof(buf), hwnd);
    ok(!err,"mci sysinfo waveaudio name after auto-open returned %s\n", dbg_mcierr(err));
    /* This is the alias, not necessarily a file name. */
    if(!err) ok(!strcmp(buf,"tempfile.wav"), "sysinfo name 1 open: %s\n", buf);
    test_notification(hwnd, "sysinfo name notify\n", MCI_NOTIFY_SUCCESSFUL);

    err = mciGetDeviceIDA("tempfile.wav");
    ok(err == 1, "mciGetDeviceIDA tempfile.wav returned %u, expected 1\n", err);

    /* Save the full pathname to the file. */
    err = mciSendStringA("info tempfile.wav file", path, sizeof(path), NULL);
    ok(!err,"mci info tempfile.wav file returned %s\n", dbg_mcierr(err));
    if(err) strcpy(path,"tempfile.wav");

    err = mciSendStringA("status tempfile.wav mode", NULL, 0, hwnd);
    ok(!err,"mci status tempfile.wav mode without buffer returned %s\n", dbg_mcierr(err));

    sprintf(command,"status \"%s\" mode",path);
    err = mciSendStringA(command, buf, sizeof(buf), hwnd);
    ok(!err,"mci status \"%s\" mode returned %s\n", path, dbg_mcierr(err));

    err = mciSendStringA("status tempfile.wav mode", buf, sizeof(buf), hwnd);
    ok(!err,"mci status tempfile.wav mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"playing"), "mci auto-open status mode, got: %s\n", buf);

    err = mciSendStringA("open tempfile.wav", buf, sizeof(buf), NULL);
    ok(err==MCIERR_DEVICE_OPEN, "mci open from auto-open returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("open foo.wav alias tempfile.wav", buf, sizeof(buf), NULL);
    ok(err==MCIERR_DUPLICATE_ALIAS, "mci open re-using alias returned %s\n", dbg_mcierr(err));

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
    err = mciSendStringA("status tempfile.wav mode notify", buf, sizeof(buf), hwnd);
    todo_wine ok(err==MCIERR_NOTIFY_ON_AUTO_OPEN, "mci status auto-open notify returned %s\n", dbg_mcierr(err));
    if(!err) {
        trace("Wine style MCI auto-close upon notification\n");

        /* "playing" because auto-close comes after the status call. */
        ok(!strcmp(buf,"playing"), "mci auto-open status mode notify, got: %s\n", buf);
        /* fixme:winmm:MMDRV_Exit Closing while ll-driver open
         *  is explained by failure to auto-close a device. */
        test_notification(hwnd,"status notify",MCI_NOTIFY_SUCCESSFUL);
        /* MCI received NOTIFY_SUPERSEDED and auto-closed the device. */

        /* Until this is implemented, force closing the device */
        err = mciSendStringA("close tempfile.wav", NULL, 0, hwnd);
        ok(!err,"mci auto-still-open stop returned %s\n", dbg_mcierr(err));
        Sleep(16);
        test_notification(hwnd,"auto-open",0);
    } else if(err==MCIERR_NOTIFY_ON_AUTO_OPEN) { /* MS style */
        trace("MS style MCI auto-open forbids notification\n");

        err = mciSendStringA("pause tempfile.wav", NULL, 0, hwnd);
        ok(!err,"mci auto-still-open pause returned %s\n", dbg_mcierr(err));

        err = mciSendStringA("status tempfile.wav mode", buf, sizeof(buf), hwnd);
        ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
        if(!err) ok(!strcmp(buf,"paused"), "mci auto-open status mode, got: %s\n", buf);

        /* Auto-close */
        err = mciSendStringA("stop tempfile.wav wait", NULL, 0, hwnd);
        ok(!err,"mci auto-still-open stop returned %s\n", dbg_mcierr(err));
        Sleep(16); /* makes sysinfo quantity open below succeed */
    }

    err = mciSendStringA("sysinfo waveaudio quantity open", buf, sizeof(buf), NULL);
    ok(!err,"mci sysinfo waveaudio quantity open after close returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"0"), "sysinfo quantity open expected 0 after auto-close, got: %s\n", buf);

    /* w95-WinME (not w2k/XP) switch to C:\ after auto-playing once.  Prevent
     * MCIERR_FILE_NOT_FOUND by using the full path name from the Info file command.
     */
    sprintf(command,"status \"%s\" mode wait",path);
    err = mciSendStringA(command, buf, sizeof(buf), hwnd);
    ok(!err,"mci re-auto-open status mode returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"stopped"), "mci re-auto-open status mode, got: %s\n", buf);

    /* This uses auto-open as well. */
    err = mciSendStringA("capability waveaudio outputs", buf, sizeof(buf), NULL);
    ok(!err,"mci capability waveaudio outputs returned %s\n", dbg_mcierr(err));
    /* Wine with no sound selected in winecfg's audio tab fails this test. */
    if(!err) ok(atoi(buf)==ndevs,"Expected %d audio outputs, got %s\n", ndevs, buf);

    err = mciSendStringA("capability waveaudio device type", buf, sizeof(buf), hwnd);
    ok(!err,"mci capability device type returned %s\n", dbg_mcierr(err));
    if(!err) ok(!strcmp(buf,"waveaudio"), "mci capability device type response: %s\n", buf);

    /* waveaudio forbids Pause without Play. */
    sprintf(command,"pause \"%s\"",path);
    err = mciSendStringA(command, NULL, 0, hwnd);
    ok(err==MCIERR_NONAPPLICABLE_FUNCTION,"mci auto-open pause returned %s\n", dbg_mcierr(err));

    ok(0xDEADF00D==intbuf[0] && 0xABADCAFE==intbuf[2],"DWORD buffer corruption\n");
}

static void test_playWaveTypeMpegvideo(void)
{
    MCIERROR err;
    MCIDEVICEID wDeviceID;
    MCI_PLAY_PARMS play_parm;
    MCI_STATUS_PARMS status_parm;
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    err = mciSendStringA("open tempfile.wav type MPEGVideo alias mysound", NULL, 0, NULL);
    ok(err==ok_saved,"mci open tempfile.wav type MPEGVideo returned %s\n", dbg_mcierr(err));
    if(err) {
        skip("Cannot open tempfile.wav type MPEGVideo for playing (%s), skipping\n", dbg_mcierr(err));
        return;
    }

    wDeviceID = mciGetDeviceIDA("mysound");
    ok(wDeviceID == 1, "mciGetDeviceIDA mysound returned %u, expected 1\n", wDeviceID);

    err = mciSendCommandA(wDeviceID, MCI_PLAY, 0, (DWORD_PTR)&play_parm);
    ok(!err,"mciCommand play returned %s\n", dbg_mcierr(err));

    err = mciSendStringA("status mysound mode", buf, sizeof(buf), NULL);
    ok(!err,"mci status mode returned %s\n", dbg_mcierr(err));
    ok(!strcmp(buf,"playing"), "mci status mode: %s\n", buf);

    status_parm.dwItem = MCI_STATUS_MODE;
    err = mciSendCommandA(wDeviceID, MCI_STATUS,
                          MCI_STATUS_ITEM,
                          (DWORD_PTR)&status_parm);
    ok(!err,"mciCommand status mode returned %s\n", dbg_mcierr(err));
    ok(status_parm.dwReturn == MCI_MODE_PLAY,
       "mciCommand status mode: %u\n", (DWORD)status_parm.dwReturn);

    err = mciSendStringA("close mysound", NULL, 0, NULL);
    ok(!err,"mci close returned %s\n", dbg_mcierr(err));
}

START_TEST(mci)
{
    char curdir[MAX_PATH], tmpdir[MAX_PATH];
    MCIERROR err;
    HWND hwnd;

    GetCurrentDirectoryA(MAX_PATH, curdir);
    GetTempPathA(MAX_PATH, tmpdir);
    SetCurrentDirectoryA(tmpdir);

    hwnd = CreateWindowExA(0, "static", "winmm test", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    test_mciParser(hwnd);
    test_openCloseWAVE(hwnd);
    test_recordWAVE(hwnd);
    if(waveOutGetNumDevs()){
        test_playWAVE(hwnd);
        test_asyncWAVE(hwnd);
        test_AutoOpenWAVE(hwnd);
        test_playWaveTypeMpegvideo();
    }else
        skip("No output devices available, skipping all output tests\n");
    /* Win9X hangs when exiting with something still open. */
    err = mciSendStringA("close all", NULL, 0, hwnd);
    ok(!err,"final close all returned %s\n", dbg_mcierr(err));
    ok(DeleteFileA("tempfile.wav") || ok_saved, "Delete tempfile.wav (cause auto-open?)\n");
    DestroyWindow(hwnd);

    SetCurrentDirectoryA(curdir);
}
