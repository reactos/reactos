/*
 * MCI internal functions
 *
 * Copyright 1998/1999 Eric Pouech
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

/* TODO:
 * - implement WINMM (32bit) multitasking and use it in all MCI drivers
 *   instead of the home grown one 
 * - 16bit mmTaskXXX functions are currently broken because the 16
 *   loader does not support binary command lines => provide Wine's
 *   own mmtask.tsk not using binary command line.
 * - correctly handle the MCI_ALL_DEVICE_ID in functions.
 * - finish mapping 16 <=> 32 of MCI structures and commands
 * - implement auto-open feature (ie, when a string command is issued
 *   for a not yet opened device, MCI automatically opens it) 
 * - use a default registry setting to replace the [mci] section in
 *   configuration file (layout of info in registry should be compatible
 *   with all Windows' version - which use different layouts of course)
 * - implement automatic open
 *      + only works on string interface, on regular devices (don't work on all
 *        nor custom devices)
 * - command table handling isn't thread safe
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "mmsystem.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "wownt32.h"

#include "digitalv.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mci);

/* First MCI valid device ID (0 means error) */
#define MCI_MAGIC 0x0001

/* MCI settings */
static const WCHAR wszHklmMci  [] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\MCI";

static WINE_MCIDRIVER *MciDrivers;

static UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data);
static UINT MCI_SetCommandTable(HGLOBAL hMem, UINT uDevType);

/* dup a string and uppercase it */
static inline LPWSTR str_dup_upper( LPCWSTR str )
{
    INT len = (lstrlenW(str) + 1) * sizeof(WCHAR);
    LPWSTR p = HeapAlloc( GetProcessHeap(), 0, len );
    if (p)
    {
        memcpy( p, str, len );
        CharUpperW( p );
    }
    return p;
}

/**************************************************************************
 * 				MCI_GetDriver			[internal]
 */
static LPWINE_MCIDRIVER	MCI_GetDriver(UINT wDevID)
{
    LPWINE_MCIDRIVER	wmd = 0;

    EnterCriticalSection(&WINMM_cs);
    for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
	if (wmd->wDeviceID == wDevID)
	    break;
    }
    LeaveCriticalSection(&WINMM_cs);
    return wmd;
}

/**************************************************************************
 * 				MCI_GetDriverFromString		[internal]
 */
static UINT MCI_GetDriverFromString(LPCWSTR lpstrName)
{
    LPWINE_MCIDRIVER	wmd;
    UINT		ret = 0;

    if (!lpstrName)
	return 0;

    if (!wcsicmp(lpstrName, L"ALL"))
	return MCI_ALL_DEVICE_ID;

    EnterCriticalSection(&WINMM_cs);
    for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
	if (wmd->lpstrAlias && wcsicmp(wmd->lpstrAlias, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_cs);

    return ret;
}

/**************************************************************************
 * 			MCI_MessageToString			[internal]
 */
static const char* MCI_MessageToString(UINT wMsg)
{
#define CASE(s) case (s): return #s

    switch (wMsg) {
        CASE(DRV_LOAD);
        CASE(DRV_ENABLE);
        CASE(DRV_OPEN);
        CASE(DRV_CLOSE);
        CASE(DRV_DISABLE);
        CASE(DRV_FREE);
        CASE(DRV_CONFIGURE);
        CASE(DRV_QUERYCONFIGURE);
        CASE(DRV_INSTALL);
        CASE(DRV_REMOVE);
        CASE(DRV_EXITSESSION);
        CASE(DRV_EXITAPPLICATION);
        CASE(DRV_POWER);
	CASE(MCI_BREAK);
	CASE(MCI_CLOSE);
	CASE(MCI_CLOSE_DRIVER);
	CASE(MCI_COPY);
	CASE(MCI_CUE);
	CASE(MCI_CUT);
	CASE(MCI_DELETE);
	CASE(MCI_ESCAPE);
	CASE(MCI_FREEZE);
	CASE(MCI_PAUSE);
	CASE(MCI_PLAY);
	CASE(MCI_GETDEVCAPS);
	CASE(MCI_INFO);
	CASE(MCI_LOAD);
	CASE(MCI_OPEN);
	CASE(MCI_OPEN_DRIVER);
	CASE(MCI_PASTE);
	CASE(MCI_PUT);
	CASE(MCI_REALIZE);
	CASE(MCI_RECORD);
	CASE(MCI_RESUME);
	CASE(MCI_SAVE);
	CASE(MCI_SEEK);
	CASE(MCI_SET);
	CASE(MCI_SOUND);
	CASE(MCI_SPIN);
	CASE(MCI_STATUS);
	CASE(MCI_STEP);
	CASE(MCI_STOP);
	CASE(MCI_SYSINFO);
	CASE(MCI_UNFREEZE);
	CASE(MCI_UPDATE);
	CASE(MCI_WHERE);
	CASE(MCI_WINDOW);
	/* constants for digital video */
	CASE(MCI_CAPTURE);
	CASE(MCI_MONITOR);
	CASE(MCI_RESERVE);
	CASE(MCI_SETAUDIO);
	CASE(MCI_SIGNAL);
	CASE(MCI_SETVIDEO);
	CASE(MCI_QUALITY);
	CASE(MCI_LIST);
	CASE(MCI_UNDO);
	CASE(MCI_CONFIGURE);
	CASE(MCI_RESTORE);
#undef CASE
    default:
        return wine_dbg_sprintf("MCI_<<%04X>>", wMsg);
    }
}

static LPWSTR MCI_strdupAtoW( LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

static int MCI_MapMsgAtoW(UINT msg, DWORD_PTR dwParam1, DWORD_PTR *dwParam2)
{
    if (msg < DRV_RESERVED) return 0;

    switch (msg)
    {
    case MCI_CLOSE:
    case MCI_CONFIGURE:
    case MCI_PLAY:
    case MCI_SEEK:
    case MCI_STOP:
    case MCI_PAUSE:
    case MCI_GETDEVCAPS:
    case MCI_SPIN:
    case MCI_SET:
    case MCI_STEP:
    case MCI_RECORD:
    case MCI_BREAK:
    case MCI_STATUS:
    case MCI_CUE:
    case MCI_REALIZE:
    case MCI_PUT:
    case MCI_WHERE:
    case MCI_FREEZE:
    case MCI_UNFREEZE:
    case MCI_CUT:
    case MCI_COPY:
    case MCI_PASTE:
    case MCI_UPDATE:
    case MCI_RESUME:
    case MCI_DELETE:
    case MCI_MONITOR:
    case MCI_SIGNAL:
    case MCI_UNDO:
        return 0;

    case MCI_OPEN:
        {   /* MCI_ANIM_OPEN_PARMS is the largest known MCI_OPEN_PARMS
             * structure, larger than MCI_WAVE_OPEN_PARMS */
            MCI_ANIM_OPEN_PARMSA *mci_openA = (MCI_ANIM_OPEN_PARMSA*)*dwParam2;
            MCI_ANIM_OPEN_PARMSW *mci_openW;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD_PTR) + sizeof(*mci_openW));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_openW = (MCI_ANIM_OPEN_PARMSW *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_openW->dwCallback = mci_openA->dwCallback;

            if (dwParam1 & MCI_OPEN_TYPE)
            {
                if (dwParam1 & MCI_OPEN_TYPE_ID)
                    mci_openW->lpstrDeviceType = (LPCWSTR)mci_openA->lpstrDeviceType;
                else
                    mci_openW->lpstrDeviceType = MCI_strdupAtoW(mci_openA->lpstrDeviceType);
            }
            if (dwParam1 & MCI_OPEN_ELEMENT)
            {
                if (dwParam1 & MCI_OPEN_ELEMENT_ID)
                    mci_openW->lpstrElementName = (LPCWSTR)mci_openA->lpstrElementName;
                else
                    mci_openW->lpstrElementName = MCI_strdupAtoW(mci_openA->lpstrElementName);
            }
            if (dwParam1 & MCI_OPEN_ALIAS)
                mci_openW->lpstrAlias = MCI_strdupAtoW(mci_openA->lpstrAlias);
            /* We don't know how many DWORD follow, as
             * the structure depends on the device. */
            if (HIWORD(dwParam1))
                memcpy(&mci_openW->dwStyle, &mci_openA->dwStyle, sizeof(MCI_ANIM_OPEN_PARMSW) - sizeof(MCI_OPEN_PARMSW));
        }
        return 1;

    case MCI_WINDOW:
        if (dwParam1 & MCI_ANIM_WINDOW_TEXT)
        {
            MCI_ANIM_WINDOW_PARMSA *mci_windowA = (MCI_ANIM_WINDOW_PARMSA *)*dwParam2;
            MCI_ANIM_WINDOW_PARMSW *mci_windowW;

            mci_windowW = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_windowW));
            if (!mci_windowW) return -1;

            *dwParam2 = (DWORD_PTR)mci_windowW;

            mci_windowW->lpstrText = MCI_strdupAtoW(mci_windowA->lpstrText);

            if (dwParam1 & MCI_NOTIFY)
                mci_windowW->dwCallback = mci_windowA->dwCallback;
            if (dwParam1 & MCI_ANIM_WINDOW_HWND)
                mci_windowW->hWnd = mci_windowA->hWnd;
            if (dwParam1 & MCI_ANIM_WINDOW_STATE)
                mci_windowW->nCmdShow = mci_windowA->nCmdShow;

            return 1;
        }
        return 0;

    case MCI_SYSINFO:
        if (dwParam1 & (MCI_SYSINFO_INSTALLNAME | MCI_SYSINFO_NAME))
        {
            MCI_SYSINFO_PARMSA *mci_sysinfoA = (MCI_SYSINFO_PARMSA *)*dwParam2;
            MCI_SYSINFO_PARMSW *mci_sysinfoW;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_sysinfoW) + sizeof(DWORD_PTR));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_sysinfoW = (MCI_SYSINFO_PARMSW *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_sysinfoW->dwCallback = mci_sysinfoA->dwCallback;

            /* Size is measured in numbers of characters, despite what MSDN says. */
            mci_sysinfoW->dwRetSize = mci_sysinfoA->dwRetSize;
            mci_sysinfoW->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mci_sysinfoW->dwRetSize * sizeof(WCHAR));
            mci_sysinfoW->dwNumber = mci_sysinfoA->dwNumber;
            mci_sysinfoW->wDeviceType = mci_sysinfoA->wDeviceType;
            return 1;
        }
        return 0;
    case MCI_INFO:
        {
            MCI_DGV_INFO_PARMSA *mci_infoA = (MCI_DGV_INFO_PARMSA *)*dwParam2;
            MCI_DGV_INFO_PARMSW *mci_infoW;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_infoW) + sizeof(DWORD_PTR));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_infoW = (MCI_DGV_INFO_PARMSW *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_infoW->dwCallback = mci_infoA->dwCallback;

            /* Size is measured in numbers of characters. */
            mci_infoW->dwRetSize = mci_infoA->dwRetSize;
            mci_infoW->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mci_infoW->dwRetSize * sizeof(WCHAR));
            if (dwParam1 & MCI_DGV_INFO_ITEM)
                mci_infoW->dwItem = mci_infoA->dwItem;
            return 1;
        }
    case MCI_SAVE:
    case MCI_LOAD:
    case MCI_CAPTURE:
    case MCI_RESTORE:
        {   /* All these commands have the same layout: callback + string + optional rect */
            MCI_OVLY_LOAD_PARMSA *mci_loadA = (MCI_OVLY_LOAD_PARMSA *)*dwParam2;
            MCI_OVLY_LOAD_PARMSW *mci_loadW;

            mci_loadW = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_loadW));
            if (!mci_loadW) return -1;

            *dwParam2 = (DWORD_PTR)mci_loadW;
            if (dwParam1 & MCI_NOTIFY)
                mci_loadW->dwCallback = mci_loadA->dwCallback;
            mci_loadW->lpfilename = MCI_strdupAtoW(mci_loadA->lpfilename);
            if ((MCI_SAVE    == msg && dwParam1 & MCI_DGV_RECT) ||
                (MCI_LOAD    == msg && dwParam1 & MCI_OVLY_RECT) ||
                (MCI_CAPTURE == msg && dwParam1 & MCI_DGV_CAPTURE_AT) ||
                (MCI_RESTORE == msg && dwParam1 & MCI_DGV_RESTORE_AT))
                mci_loadW->rc = mci_loadA->rc;
            return 1;
        }
    case MCI_SOUND:
    case MCI_ESCAPE:
        {   /* All these commands have the same layout: callback + string */
            MCI_VD_ESCAPE_PARMSA *mci_vd_escapeA = (MCI_VD_ESCAPE_PARMSA *)*dwParam2;
            MCI_VD_ESCAPE_PARMSW *mci_vd_escapeW;

            mci_vd_escapeW = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_vd_escapeW));
            if (!mci_vd_escapeW) return -1;

            *dwParam2 = (DWORD_PTR)mci_vd_escapeW;
            if (dwParam1 & MCI_NOTIFY)
                mci_vd_escapeW->dwCallback = mci_vd_escapeA->dwCallback;
            mci_vd_escapeW->lpstrCommand = MCI_strdupAtoW(mci_vd_escapeA->lpstrCommand);
            return 1;
        }
    case MCI_SETAUDIO:
    case MCI_SETVIDEO:
        if (!(dwParam1 & (MCI_DGV_SETVIDEO_QUALITY | MCI_DGV_SETVIDEO_ALG
                        | MCI_DGV_SETAUDIO_QUALITY | MCI_DGV_SETAUDIO_ALG)))
            return 0;
        /* fall through to default */
    case MCI_RESERVE:
    case MCI_QUALITY:
    case MCI_LIST:
    default:
        FIXME("Message %s needs translation\n", MCI_MessageToString(msg));
        return 0; /* pass through untouched */
    }
}

static void MCI_UnmapMsgAtoW(UINT msg, DWORD_PTR dwParam1, DWORD_PTR dwParam2,
                              DWORD result)
{
    switch (msg)
    {
    case MCI_OPEN:
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_OPEN_PARMSA *mci_openA = (MCI_OPEN_PARMSA *)*ptr;
            MCI_OPEN_PARMSW *mci_openW = (MCI_OPEN_PARMSW *)dwParam2;

            mci_openA->wDeviceID = mci_openW->wDeviceID;

            if (dwParam1 & MCI_OPEN_TYPE)
            {
                if (!(dwParam1 & MCI_OPEN_TYPE_ID))
                    HeapFree(GetProcessHeap(), 0, (LPWSTR)mci_openW->lpstrDeviceType);
            }
            if (dwParam1 & MCI_OPEN_ELEMENT)
            {
                if (!(dwParam1 & MCI_OPEN_ELEMENT_ID))
                    HeapFree(GetProcessHeap(), 0, (LPWSTR)mci_openW->lpstrElementName);
            }
            if (dwParam1 & MCI_OPEN_ALIAS)
                HeapFree(GetProcessHeap(), 0, (LPWSTR)mci_openW->lpstrAlias);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;
    case MCI_WINDOW:
        if (dwParam1 & MCI_ANIM_WINDOW_TEXT)
        {
            MCI_ANIM_WINDOW_PARMSW *mci_windowW = (MCI_ANIM_WINDOW_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_windowW->lpstrText);
            HeapFree(GetProcessHeap(), 0, mci_windowW);
        }
        break;

    case MCI_SYSINFO:
        if (dwParam1 & (MCI_SYSINFO_INSTALLNAME | MCI_SYSINFO_NAME))
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_SYSINFO_PARMSA *mci_sysinfoA = (MCI_SYSINFO_PARMSA *)*ptr;
            MCI_SYSINFO_PARMSW *mci_sysinfoW = (MCI_SYSINFO_PARMSW *)dwParam2;

            if (!result)
            {
                WideCharToMultiByte(CP_ACP, 0,
                                    mci_sysinfoW->lpstrReturn, -1,
                                    mci_sysinfoA->lpstrReturn, mci_sysinfoA->dwRetSize,
                                    NULL, NULL);
            }

            HeapFree(GetProcessHeap(), 0, mci_sysinfoW->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;
    case MCI_INFO:
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_INFO_PARMSA *mci_infoA = (MCI_INFO_PARMSA *)*ptr;
            MCI_INFO_PARMSW *mci_infoW = (MCI_INFO_PARMSW *)dwParam2;

            if (!result)
            {
                WideCharToMultiByte(CP_ACP, 0,
                                    mci_infoW->lpstrReturn, -1,
                                    mci_infoA->lpstrReturn, mci_infoA->dwRetSize,
                                    NULL, NULL);
            }

            HeapFree(GetProcessHeap(), 0, mci_infoW->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;
    case MCI_SAVE:
    case MCI_LOAD:
    case MCI_CAPTURE:
    case MCI_RESTORE:
        {   /* All these commands have the same layout: callback + string + optional rect */
            MCI_OVLY_LOAD_PARMSW *mci_loadW = (MCI_OVLY_LOAD_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_loadW->lpfilename);
            HeapFree(GetProcessHeap(), 0, mci_loadW);
        }
        break;
    case MCI_SOUND:
    case MCI_ESCAPE:
        {   /* All these commands have the same layout: callback + string */
            MCI_VD_ESCAPE_PARMSW *mci_vd_escapeW = (MCI_VD_ESCAPE_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_vd_escapeW->lpstrCommand);
            HeapFree(GetProcessHeap(), 0, mci_vd_escapeW);
        }
        break;

    default:
        FIXME("Message %s needs unmapping\n", MCI_MessageToString(msg));
        break;
    }
}

/**************************************************************************
 * 				MCI_GetDevTypeFromFileName	[internal]
 */
static	DWORD	MCI_GetDevTypeFromFileName(LPCWSTR fileName, LPWSTR buf, UINT len)
{
    LPCWSTR	tmp;
    HKEY	hKey;
    if ((tmp = wcsrchr(fileName, '.'))) {
	if (RegOpenKeyExW( HKEY_LOCAL_MACHINE,
			   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MCI Extensions",
			   0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS) {
	    DWORD dwLen = len;
	    LONG lRet = RegQueryValueExW( hKey, tmp + 1, 0, 0, (void*)buf, &dwLen ); 
	    RegCloseKey( hKey );
	    if (lRet == ERROR_SUCCESS) return 0;
        }
	TRACE("No ...\\MCI Extensions entry for %s found.\n", debugstr_w(tmp));
    }
    return MCIERR_EXTENSION_NOT_FOUND;
}

/**************************************************************************
 * 				MCI_GetDevTypeFromResource	[internal]
 */
static	UINT	MCI_GetDevTypeFromResource(LPCWSTR lpstrName)
{
    WCHAR	buf[32];
    UINT	uDevType;
    for (uDevType = MCI_DEVTYPE_FIRST; uDevType <= MCI_DEVTYPE_LAST; uDevType++) {
	if (LoadStringW(hWinMM32Instance, uDevType, buf, ARRAY_SIZE(buf))) {
	    /* FIXME: ignore digits suffix */
	    if (!wcsicmp(buf, lpstrName))
		return uDevType;
	}
    }
    return 0;
}

#define	MAX_MCICMDTABLE			20
#define MCI_COMMAND_TABLE_NOT_LOADED	0xFFFE

typedef struct tagWINE_MCICMDTABLE {
    UINT		uDevType;
    HGLOBAL             hMem;
    const BYTE*		lpTable;
    UINT		nVerbs;		/* number of verbs in command table */
    LPCWSTR*		aVerbs;		/* array of verbs to speed up the verb look up process */
} WINE_MCICMDTABLE, *LPWINE_MCICMDTABLE;

static WINE_MCICMDTABLE S_MciCmdTable[MAX_MCICMDTABLE];

/**************************************************************************
 * 				MCI_IsCommandTableValid		[internal]
 */
static	BOOL		MCI_IsCommandTableValid(UINT uTbl)
{
    const BYTE* lmem;
    LPCWSTR     str;
    DWORD	flg;
    WORD	eid;
    int		idx = 0;
    BOOL	inCst = FALSE;

    TRACE("Dumping cmdTbl=%d [lpTable=%p devType=%d]\n",
	  uTbl, S_MciCmdTable[uTbl].lpTable, S_MciCmdTable[uTbl].uDevType);

    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].lpTable)
	return FALSE;

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
        str = (LPCWSTR)lmem;
        lmem += (lstrlenW(str) + 1) * sizeof(WCHAR);
        flg = *(const DWORD*)lmem;
        eid = *(const WORD*)(lmem + sizeof(DWORD));
        lmem += sizeof(DWORD) + sizeof(WORD);
        idx ++;
        /* TRACE("cmd=%s %08lx %04x\n", debugstr_w(str), flg, eid); */
        switch (eid) {
        case MCI_COMMAND_HEAD:          if (!*str || !flg) return FALSE; idx = 0;		break;	/* check unicity of str in table */
        case MCI_STRING:                if (inCst) return FALSE;				break;
        case MCI_HWND:                  /* Occurs inside MCI_CONSTANT as in "window handle default" */
        case MCI_HPAL:
        case MCI_HDC:
        case MCI_INTEGER:               if (!*str) return FALSE;				break;
        case MCI_END_COMMAND:           if (*str || flg || idx == 0) return FALSE; idx = 0;	break;
        case MCI_RETURN:		if (*str || idx != 1) return FALSE;			break;
        case MCI_FLAG:		        if (!*str) return FALSE;				break;
        case MCI_END_COMMAND_LIST:	if (*str || flg) return FALSE;	idx = 0;		break;
        case MCI_RECT:		        if (!*str || inCst) return FALSE;			break;
        case MCI_CONSTANT:              if (inCst) return FALSE; inCst = TRUE;			break;
        case MCI_END_CONSTANT:	        if (*str || flg || !inCst) return FALSE; inCst = FALSE; break;
        default:			return FALSE;
        }
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}

/**************************************************************************
 * 				MCI_DumpCommandTable		[internal]
 */
static	BOOL		MCI_DumpCommandTable(UINT uTbl)
{
    const BYTE*	lmem;
    LPCWSTR	str;
    WORD	eid;

    if (!MCI_IsCommandTableValid(uTbl)) {
	ERR("Ooops: %d is not valid\n", uTbl);
	return FALSE;
    }

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
	do {
	    /* DWORD flg; */
	    str = (LPCWSTR)lmem;
	    lmem += (lstrlenW(str) + 1) * sizeof(WCHAR);
	    /* flg = *(const DWORD*)lmem; */
	    eid = *(const WORD*)(lmem + sizeof(DWORD));
            /* TRACE("cmd=%s %08lx %04x\n", debugstr_w(str), flg, eid); */
	    lmem += sizeof(DWORD) + sizeof(WORD);
	} while (eid != MCI_END_COMMAND && eid != MCI_END_COMMAND_LIST);
        /* EPP TRACE(" => end of command%s\n", (eid == MCI_END_COMMAND_LIST) ? " list" : ""); */
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}


/**************************************************************************
 * 				MCI_GetCommandTable		[internal]
 */
static	UINT		MCI_GetCommandTable(UINT uDevType)
{
    UINT	uTbl;
    WCHAR	buf[32];
    LPCWSTR	str = NULL;

    /* first look up existing for existing devType */
    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (S_MciCmdTable[uTbl].lpTable && S_MciCmdTable[uTbl].uDevType == uDevType)
	    return uTbl;
    }

    /* well try to load id */
    if (uDevType >= MCI_DEVTYPE_FIRST && uDevType <= MCI_DEVTYPE_LAST) {
	if (LoadStringW(hWinMM32Instance, uDevType, buf, ARRAY_SIZE(buf))) {
	    str = buf;
	}
    } else if (uDevType == 0) {
	str = L"CORE";
    }
    uTbl = MCI_NO_COMMAND_TABLE;
    if (str) {
	HRSRC 	hRsrc = FindResourceW(hWinMM32Instance, str, (LPCWSTR)RT_RCDATA);
	HANDLE	hMem = 0;

	if (hRsrc) hMem = LoadResource(hWinMM32Instance, hRsrc);
	if (hMem) {
	    uTbl = MCI_SetCommandTable(hMem, uDevType);
	} else {
	    WARN("No command table found in resource %p[%s]\n",
		 hWinMM32Instance, debugstr_w(str));
	}
    }
    TRACE("=> %d\n", uTbl);
    return uTbl;
}

/**************************************************************************
 * 				MCI_SetCommandTable		[internal]
 */
static UINT MCI_SetCommandTable(HGLOBAL hMem, UINT uDevType)
{
    int		        uTbl;
    static	BOOL	bInitDone = FALSE;

    /* <HACK>
     * The CORE command table must be loaded first, so that MCI_GetCommandTable()
     * can be called with 0 as a uDevType to retrieve it.
     * </HACK>
     */
    if (!bInitDone) {
	bInitDone = TRUE;
	MCI_GetCommandTable(0);
    }
    TRACE("(%p, %u)\n", hMem, uDevType);
    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (!S_MciCmdTable[uTbl].lpTable) {
	    const BYTE* lmem;
	    LPCWSTR 	str;
	    WORD	eid;
	    WORD	count;

	    S_MciCmdTable[uTbl].uDevType = uDevType;
	    S_MciCmdTable[uTbl].lpTable = LockResource(hMem);
	    S_MciCmdTable[uTbl].hMem = hMem;

	    if (TRACE_ON(mci)) {
		MCI_DumpCommandTable(uTbl);
	    }

	    /* create the verbs table */
	    /* get # of entries */
	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		str = (LPCWSTR)lmem;
		lmem += (lstrlenW(str) + 1) * sizeof(WCHAR);
		eid = *(const WORD*)(lmem + sizeof(DWORD));
		lmem += sizeof(DWORD) + sizeof(WORD);
		if (eid == MCI_COMMAND_HEAD)
		    count++;
	    } while (eid != MCI_END_COMMAND_LIST);

	    S_MciCmdTable[uTbl].aVerbs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(LPCWSTR));
	    S_MciCmdTable[uTbl].nVerbs = count;

	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		str = (LPCWSTR)lmem;
		lmem += (lstrlenW(str) + 1) * sizeof(WCHAR);
		eid = *(const WORD*)(lmem + sizeof(DWORD));
		lmem += sizeof(DWORD) + sizeof(WORD);
		if (eid == MCI_COMMAND_HEAD)
		    S_MciCmdTable[uTbl].aVerbs[count++] = str;
	    } while (eid != MCI_END_COMMAND_LIST);
	    /* assert(count == S_MciCmdTable[uTbl].nVerbs); */
	    return uTbl;
	}
    }

    return MCI_NO_COMMAND_TABLE;
}

/**************************************************************************
 * 				MCI_UnLoadMciDriver		[internal]
 */
static	BOOL	MCI_UnLoadMciDriver(LPWINE_MCIDRIVER wmd)
{
    LPWINE_MCIDRIVER*		tmp;

    if (!wmd)
	return TRUE;

    CloseDriver(wmd->hDriver, 0, 0);

    if (wmd->dwPrivate != 0)
	WARN("Unloading mci driver with non nul dwPrivate field\n");

    EnterCriticalSection(&WINMM_cs);
    for (tmp = &MciDrivers; *tmp; tmp = &(*tmp)->lpNext) {
	if (*tmp == wmd) {
	    *tmp = wmd->lpNext;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_cs);

    HeapFree(GetProcessHeap(), 0, wmd->lpstrDeviceType);
    HeapFree(GetProcessHeap(), 0, wmd->lpstrAlias);

    HeapFree(GetProcessHeap(), 0, wmd);
    return TRUE;
}

/**************************************************************************
 * 				MCI_OpenMciDriver		[internal]
 */
static	BOOL	MCI_OpenMciDriver(LPWINE_MCIDRIVER wmd, LPCWSTR drvTyp, DWORD_PTR lp)
{
    WCHAR	libName[128];

    if (!DRIVER_GetLibName(drvTyp, L"MCI", libName, sizeof(libName)))
	return FALSE;

    /* First load driver */
    wmd->hDriver = (HDRVR)DRIVER_TryOpenDriver32(libName, lp);
    return wmd->hDriver != NULL;
}

/**************************************************************************
 * 				MCI_LoadMciDriver		[internal]
 */
static	DWORD	MCI_LoadMciDriver(LPCWSTR _strDevTyp, LPWINE_MCIDRIVER* lpwmd)
{
    LPWSTR			strDevTyp = str_dup_upper(_strDevTyp);
    LPWINE_MCIDRIVER		wmd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wmd));
    MCI_OPEN_DRIVER_PARMSW	modp;
    DWORD			dwRet = 0;

    if (!wmd || !strDevTyp) {
	dwRet = MCIERR_OUT_OF_MEMORY;
	goto errCleanUp;
    }

    wmd->lpfnYieldProc = MCI_DefYieldProc;
    wmd->dwYieldData = VK_CANCEL;
    wmd->CreatorThread = GetCurrentThreadId();

    EnterCriticalSection(&WINMM_cs);
    /* wmd must be inserted in list before sending opening the driver, because it
     * may want to lookup at wDevID
     */
    wmd->lpNext = MciDrivers;
    MciDrivers = wmd;

    for (modp.wDeviceID = MCI_MAGIC;
	 MCI_GetDriver(modp.wDeviceID) != 0;
	 modp.wDeviceID++);

    wmd->wDeviceID = modp.wDeviceID;

    LeaveCriticalSection(&WINMM_cs);

    TRACE("wDevID=%04X\n", modp.wDeviceID);

    modp.lpstrParams = NULL;

    if (!MCI_OpenMciDriver(wmd, strDevTyp, (DWORD_PTR)&modp)) {
	/* silence warning if all is used... some bogus program use commands like
	 * 'open all'...
	 */
	if (wcsicmp(strDevTyp, L"ALL") == 0) {
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    FIXME("Couldn't load driver for type %s.\n",
		  debugstr_w(strDevTyp));
	    dwRet = MCIERR_DEVICE_NOT_INSTALLED;
	}
	goto errCleanUp;
    }

    /* FIXME: should also check that module's description is of the form
     * MODULENAME:[MCI] comment
     */

    /* some drivers will return 0x0000FFFF, some others 0xFFFFFFFF */
    wmd->uSpecificCmdTable = LOWORD(modp.wCustomCommandTable);
    wmd->uTypeCmdTable = MCI_COMMAND_TABLE_NOT_LOADED;

    TRACE("Loaded driver %p (%s), type is %d, cmdTable=%08x\n",
	  wmd->hDriver, debugstr_w(strDevTyp), modp.wType, modp.wCustomCommandTable);

    wmd->lpstrDeviceType = strDevTyp;
    wmd->wType = modp.wType;

    TRACE("mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n",
	  modp.wDeviceID, modp.wType, modp.wDeviceID);
    *lpwmd = wmd;
    return 0;
errCleanUp:
    MCI_UnLoadMciDriver(wmd);
    HeapFree(GetProcessHeap(), 0, strDevTyp);
    *lpwmd = 0;
    return dwRet;
}

/**************************************************************************
 * 			MCI_SendCommandFrom32			[internal]
 */
static DWORD MCI_SendCommandFrom32(MCIDEVICEID wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD		dwRet = MCIERR_INVALID_DEVICE_ID;
    LPWINE_MCIDRIVER	wmd = MCI_GetDriver(wDevID);

    if (wmd) {
        if(wmd->CreatorThread != GetCurrentThreadId())
            return MCIERR_INVALID_DEVICE_NAME;

        dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_FinishOpen				[internal]
 *
 * Three modes of operation:
 * 1 open foo.ext ...        -> OPEN_ELEMENT with lpstrElementName=foo.ext
 *   open sequencer!foo.ext     same         with lpstrElementName=foo.ext
 * 2 open new type waveaudio -> OPEN_ELEMENT with empty ("") lpstrElementName
 * 3 open sequencer          -> OPEN_ELEMENT unset, and
 *   capability sequencer       (auto-open)  likewise
 */
static	DWORD	MCI_FinishOpen(LPWINE_MCIDRIVER wmd, LPMCI_OPEN_PARMSW lpParms,
			       DWORD dwParam)
{
    LPCWSTR alias = NULL;
    /* Open always defines an alias for further reference */
    if (dwParam & MCI_OPEN_ALIAS) {         /* open ... alias */
        alias = lpParms->lpstrAlias;
        if (MCI_GetDriverFromString(alias))
            return MCIERR_DUPLICATE_ALIAS;
    } else {
        if ((dwParam & MCI_OPEN_ELEMENT)    /* open file.wav */
            && !(dwParam & MCI_OPEN_ELEMENT_ID))
            alias = lpParms->lpstrElementName;
        else if (dwParam & MCI_OPEN_TYPE )  /* open cdaudio */
            alias = wmd->lpstrDeviceType;
        if (alias && MCI_GetDriverFromString(alias))
            return MCIERR_DEVICE_OPEN;
    }
    if (alias) {
        wmd->lpstrAlias = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(alias)+1) * sizeof(WCHAR));
        if (!wmd->lpstrAlias) return MCIERR_OUT_OF_MEMORY;
        lstrcpyW( wmd->lpstrAlias, alias);
        /* In most cases, natives adds MCI_OPEN_ALIAS to the flags passed to the driver.
         * Don't.  The drivers don't care about the winmm alias. */
    }
    lpParms->wDeviceID = wmd->wDeviceID;

    return MCI_SendCommandFrom32(wmd->wDeviceID, MCI_OPEN_DRIVER, dwParam,
				 (DWORD_PTR)lpParms);
}

/**************************************************************************
 * 				MCI_FindCommand		[internal]
 */
static	LPCWSTR		MCI_FindCommand(UINT uTbl, LPCWSTR verb)
{
    UINT	idx;

    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].lpTable)
	return NULL;

    /* another improvement would be to have the aVerbs array sorted,
     * so that we could use a dichotomic search on it, rather than this dumb
     * array look up
     */
    for (idx = 0; idx < S_MciCmdTable[uTbl].nVerbs; idx++) {
	if (wcsicmp(S_MciCmdTable[uTbl].aVerbs[idx], verb) == 0)
	    return S_MciCmdTable[uTbl].aVerbs[idx];
    }

    return NULL;
}

/**************************************************************************
 * 				MCI_GetReturnType		[internal]
 */
static	DWORD		MCI_GetReturnType(LPCWSTR lpCmd)
{
    lpCmd = (LPCWSTR)((const BYTE*)(lpCmd + lstrlenW(lpCmd) + 1) + sizeof(DWORD) + sizeof(WORD));
    if (*lpCmd == '\0' && *(const WORD*)((const BYTE*)(lpCmd + 1) + sizeof(DWORD)) == MCI_RETURN) {
	return *(const DWORD*)(lpCmd + 1);
    }
    return 0L;
}

/**************************************************************************
 * 				MCI_GetMessage			[internal]
 */
static	WORD		MCI_GetMessage(LPCWSTR lpCmd)
{
    return (WORD)*(const DWORD*)(lpCmd + lstrlenW(lpCmd) + 1);
}

/**************************************************************************
 * 				MCI_GetDWord			[internal]
 *
 * Accept 0 -1 255 255:0 255:255:255:255 :::1 1::: 2::3 ::4: 12345678
 * Refuse -1:0 0:-1 :: 256:0 1:256 0::::1
 */
static	BOOL		MCI_GetDWord(DWORD* data, LPWSTR* ptr)
{
    LPWSTR	ret = *ptr;
    DWORD	total = 0, shift = 0;
    BOOL	sign = FALSE, digits = FALSE;

    while (*ret == ' ' || *ret == '\t') ret++;
    if (*ret == '-') {
	ret++;
	sign = TRUE;
    }
    for(;;) {
	DWORD	val = 0;
	while ('0' <= *ret && *ret <= '9') {
	    val = *ret++ - '0' + 10 * val;
	    digits = TRUE;
	}
	switch (*ret) {
	case '\0':	break;
	case '\t':
	case ' ':	ret++; break;
	default:	return FALSE;
	case ':':
	    if ((val >= 256) || (shift >= 24))	return FALSE;
	    total |= val << shift;
	    shift += 8;
	    ret++;
	    continue;
	}

	if (!digits)				return FALSE;
	if (shift && (val >= 256 || sign))	return FALSE;
	total |= val << shift;
	*data = sign ? -total : total;
	*ptr = ret;
	return TRUE;
    }
}

/**************************************************************************
 * 				MCI_GetString		[internal]
 */
static	DWORD	MCI_GetString(LPWSTR* str, LPWSTR* args)
{
    LPWSTR      ptr = *args;

    /* see if we have a quoted string */
    if (*ptr == '"') {
	ptr = wcschr(*str = ptr + 1, '"');
	if (!ptr) return MCIERR_NO_CLOSING_QUOTE;
	/* FIXME: shall we escape \" from string ?? */
	if (ptr[-1] == '\\') TRACE("Ooops: un-escaped \"\n");
	*ptr++ = '\0'; /* remove trailing " */
	if (*ptr != ' ' && *ptr != '\0') return MCIERR_EXTRA_CHARACTERS;
    } else {
	ptr = wcschr(ptr, ' ');

	if (ptr) {
	    *ptr++ = '\0';
	} else {
	    ptr = *args + lstrlenW(*args);
	}
	*str = *args;
    }

    *args = ptr;
    return 0;
}

#define	MCI_DATA_SIZE	16

/**************************************************************************
 * 				MCI_ParseOptArgs		[internal]
 */
static	DWORD	MCI_ParseOptArgs(DWORD* data, int _offset, LPCWSTR lpCmd,
				 LPWSTR args, LPDWORD dwFlags)
{
    int		len, offset;
    const char* lmem;
    LPCWSTR     str;
    DWORD	dwRet, flg, cflg = 0;
    WORD	eid;
    BOOL	inCst, found;

    /* loop on arguments */
    while (*args) {
	lmem = (const char*)lpCmd;
	found = inCst = FALSE;
	offset = _offset;

	/* skip any leading white space(s) */
	while (*args == ' ') args++;
	TRACE("args=%s\n", debugstr_w(args));

	do { /* loop on options for command table for the requested verb */
	    str = (LPCWSTR)lmem;
	    lmem += ((len = lstrlenW(str)) + 1) * sizeof(WCHAR);
	    flg = *(const DWORD*)lmem;
	    eid = *(const WORD*)(lmem + sizeof(DWORD));
	    lmem += sizeof(DWORD) + sizeof(WORD);
            /* TRACE("\tcmd=%s inCst=%c eid=%04x\n", debugstr_w(str), inCst ? 'Y' : 'N', eid); */

	    switch (eid) {
	    case MCI_CONSTANT:
		inCst = TRUE;	cflg = flg;	break;
	    case MCI_END_CONSTANT:
		/* there may be additional integral values after flag in constant */
		if (inCst && MCI_GetDWord(&(data[offset]), &args)) {
		    *dwFlags |= cflg;
		}
		inCst = FALSE;	cflg = 0;
		break;
	    case MCI_RETURN:
		if (offset != _offset) {
		    FIXME("MCI_RETURN not in first position\n");
		    return MCIERR_PARSER_INTERNAL;
		}
	    }

	    if (wcsnicmp(args, str, len) == 0 &&
                ((eid == MCI_STRING && len == 0) || args[len] == 0 || args[len] == ' ')) {
		/* store good values into data[] */
		args += len;
		while (*args == ' ') args++;
		found = TRUE;

		switch (eid) {
		case MCI_COMMAND_HEAD:
		case MCI_RETURN:
		case MCI_END_COMMAND:
		case MCI_END_COMMAND_LIST:
		case MCI_CONSTANT: 	/* done above */
		case MCI_END_CONSTANT:  /* done above */
		    break;
		case MCI_FLAG:
		    *dwFlags |= flg;
		    TRACE("flag=%08lx\n", flg);
		    break;
		case MCI_HWND:
		case MCI_HPAL:
		case MCI_HDC:
		case MCI_INTEGER:
		    if (inCst) {
			data[offset] |= flg;
			*dwFlags |= cflg;
			inCst = FALSE;
			TRACE("flag=%08lx constant=%08lx\n", cflg, flg);
		    } else {
			*dwFlags |= flg;
			if (!MCI_GetDWord(&(data[offset]), &args)) {
			    return MCIERR_BAD_INTEGER;
			}
			TRACE("flag=%08lx int=%ld\n", flg, data[offset]);
		    }
		    break;
		case MCI_RECT:
		    /* store rect in data (offset..offset+3) */
		    *dwFlags |= flg;
		    if (!MCI_GetDWord(&(data[offset+0]), &args) ||
			!MCI_GetDWord(&(data[offset+1]), &args) ||
			!MCI_GetDWord(&(data[offset+2]), &args) ||
			!MCI_GetDWord(&(data[offset+3]), &args)) {
			return MCIERR_BAD_INTEGER;
		    }
		    TRACE("flag=%08lx for rectangle\n", flg);
		    break;
		case MCI_STRING:
		    *dwFlags |= flg;
		    if ((dwRet = MCI_GetString((LPWSTR*)&data[offset], &args)))
			return dwRet;
		    TRACE("flag=%08lx string=%s\n", flg, debugstr_w(*(LPWSTR*)&data[offset]));
		    break;
		default:	ERR("oops\n");
		}
		/* exit inside while loop, except if just entered in constant area definition */
		if (!inCst || eid != MCI_CONSTANT) eid = MCI_END_COMMAND;
	    } else {
		/* have offset incremented if needed */
		switch (eid) {
		case MCI_COMMAND_HEAD:
		case MCI_RETURN:
		case MCI_END_COMMAND:
		case MCI_END_COMMAND_LIST:
		case MCI_CONSTANT:
		case MCI_FLAG:			break;
		case MCI_HWND:
		case MCI_HPAL:
		case MCI_HDC:			if (!inCst) offset += sizeof(HANDLE)/sizeof(DWORD); break;
		case MCI_INTEGER:		if (!inCst) offset++;	break;
		case MCI_END_CONSTANT:		offset++; break;
		case MCI_STRING:		offset += sizeof(LPWSTR)/sizeof(DWORD); break;
		case MCI_RECT:			offset += 4; break;
		default:			ERR("oops\n");
		}
	    }
	} while (eid != MCI_END_COMMAND);
	if (!found) {
	    WARN("Optarg %s not found\n", debugstr_w(args));
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
	if (offset == MCI_DATA_SIZE) {
	    FIXME("Internal data[] buffer overflow\n");
	    return MCIERR_PARSER_INTERNAL;
	}
    }
    return 0;
}

/**************************************************************************
 * 				MCI_HandleReturnValues	[internal]
 */
static	DWORD	MCI_HandleReturnValues(DWORD dwRet, LPWINE_MCIDRIVER wmd, DWORD retType,
                                       MCI_GENERIC_PARMS *params, LPWSTR lpstrRet, UINT uRetLen)
{
    if (lpstrRet) {
	switch (retType) {
	case 0: /* nothing to return */
	    break;
	case MCI_INTEGER:
        {
            DWORD data = *(DWORD *)(params + 1);
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
	    case MCI_INTEGER_RETURNED:
		swprintf(lpstrRet, uRetLen, L"%d", data);
		break;
	    case MCI_RESOURCE_RETURNED:
		/* return string which ID is HIWORD(data),
		 * string is loaded from mmsystem.dll */
		LoadStringW(hWinMM32Instance, HIWORD(data), lpstrRet, uRetLen);
		break;
	    case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
		/* return string which ID is HIWORD(data),
		 * string is loaded from driver */
		/* FIXME: this is wrong for a 16 bit handle */
		LoadStringW(GetDriverModuleHandle(wmd->hDriver),
			    HIWORD(data), lpstrRet, uRetLen);
		break;
	    case MCI_COLONIZED3_RETURN:
		swprintf(lpstrRet, uRetLen, L"%02d:%02d:%02d",
			  LOBYTE(LOWORD(data)), HIBYTE(LOWORD(data)),
			  LOBYTE(HIWORD(data)));
		break;
	    case MCI_COLONIZED4_RETURN:
		swprintf(lpstrRet, uRetLen, L"%02d:%02d:%02d:%02d",
			  LOBYTE(LOWORD(data)), HIBYTE(LOWORD(data)),
			  LOBYTE(HIWORD(data)), HIBYTE(HIWORD(data)));
		break;
	    default:	ERR("Ooops (%04X)\n", HIWORD(dwRet));
	    }
	    break;
        }
	case 13: /* MCI_INTEGER64 */
        {
	    DWORD_PTR data = *(DWORD_PTR *)(params + 1);
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
	    case MCI_INTEGER_RETURNED:
		swprintf(lpstrRet, uRetLen, L"%Id", data);
		break;
	    case MCI_RESOURCE_RETURNED:
		/* return string which ID is HIWORD(data),
		 * string is loaded from mmsystem.dll */
		LoadStringW(hWinMM32Instance, HIWORD(data), lpstrRet, uRetLen);
		break;
	    case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
		/* return string which ID is HIWORD(data),
		 * string is loaded from driver */
		/* FIXME: this is wrong for a 16 bit handle */
		LoadStringW(GetDriverModuleHandle(wmd->hDriver),
			    HIWORD(data), lpstrRet, uRetLen);
		break;
	    case MCI_COLONIZED3_RETURN:
		swprintf(lpstrRet, uRetLen, L"%02d:%02d:%02d",
			  LOBYTE(LOWORD(data)), HIBYTE(LOWORD(data)),
			  LOBYTE(HIWORD(data)));
		break;
	    case MCI_COLONIZED4_RETURN:
		swprintf(lpstrRet, uRetLen, L"%02d:%02d:%02d:%02d",
			  LOBYTE(LOWORD(data)), HIBYTE(LOWORD(data)),
			  LOBYTE(HIWORD(data)), HIBYTE(HIWORD(data)));
		break;
	    default:	ERR("Ooops (%04X)\n", HIWORD(dwRet));
	    }
	    break;
        }
	case MCI_STRING:
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
		/* nothing to do data[0] == lpstrRet */
		break;
	    case MCI_INTEGER_RETURNED:
            {
                DWORD *data = (DWORD *)(params + 1);
		*data = *(LPDWORD)lpstrRet;
		swprintf(lpstrRet, uRetLen, L"%d", *data);
		break;
            }
	    default:
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
		break;
	    }
	    break;
	case MCI_RECT:
        {
            DWORD *data = (DWORD *)(params + 1);
	    if (dwRet & 0xFFFF0000ul)
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
	    swprintf(lpstrRet, uRetLen, L"%d %d %d %d", data[0], data[1], data[2], data[3]);
	    break;
        }
	default:		FIXME("Unknown MCI return type %ld\n", retType);
	}
    }
    return LOWORD(dwRet);
}

/**************************************************************************
 * 				mciSendStringW		[WINMM.@]
 */
DWORD WINAPI mciSendStringW(LPCWSTR lpstrCommand, LPWSTR lpstrRet,
			    UINT uRetLen, HWND hwndCallback)
{
    LPWSTR		verb, dev, args, devType = NULL;
    LPWINE_MCIDRIVER	wmd = 0;
    MCIDEVICEID		uDevID, auto_open = 0;
    DWORD		dwFlags = 0, dwRet = 0;
    int			offset = 0;
    DWORD		retType;
    LPCWSTR		lpCmd = 0;
    WORD		wMsg = 0;
    union
    {
        MCI_GENERIC_PARMS  generic;
        MCI_OPEN_PARMSW    open;
        MCI_SOUND_PARMSW   sound;
        MCI_SYSINFO_PARMSW sysinfo;
        DWORD              dw[MCI_DATA_SIZE];
    } data;

    TRACE("(%s, %p, %d, %p)\n", 
          debugstr_w(lpstrCommand), lpstrRet, uRetLen, hwndCallback);
    if (lpstrRet && uRetLen) *lpstrRet = '\0';

    if (!lpstrCommand[0])
        return MCIERR_MISSING_COMMAND_STRING;

    /* format is <command> <device> <optargs> */
    if (!(verb = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(lpstrCommand)+1) * sizeof(WCHAR))))
	return MCIERR_OUT_OF_MEMORY;
    lstrcpyW( verb, lpstrCommand );
    CharLowerW(verb);

    memset(&data, 0, sizeof(data));

    if (!(args = wcschr(verb, ' '))) {
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }
    *args++ = '\0';
    if ((dwRet = MCI_GetString(&dev, &args))) {
	goto errCleanUp;
    }
    uDevID = wcsicmp(dev, L"ALL") ? 0 : MCI_ALL_DEVICE_ID;

    /* Determine devType from open */
    if (!wcscmp(verb, L"open")) {
	LPWSTR	tmp;
        WCHAR	buf[128];

	/* case dev == 'new' has to be handled */
	if (!wcscmp(dev, L"new")) {
	    dev = 0;
	    if ((devType = wcsstr(args, L"type ")) != NULL) {
		devType += 5;
		tmp = wcschr(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = str_dup_upper(devType);
		if (tmp) *tmp = ' ';
		/* dwFlags and data[2] will be correctly set in ParseOpt loop */
	    } else {
		WARN("open new requires device type\n");
		dwRet = MCIERR_MISSING_DEVICE_NAME;
		goto errCleanUp;
	    }
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data.open.lpstrElementName = &L""[0];
	} else if ((devType = wcschr(dev, '!')) != NULL) {
	    *devType++ = '\0';
	    tmp = devType; devType = dev; dev = tmp;

	    dwFlags |= MCI_OPEN_TYPE;
	    data.open.lpstrDeviceType = devType;
	    devType = str_dup_upper(devType);
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data.open.lpstrElementName = dev;
	} else if (DRIVER_GetLibName(dev, L"MCI", buf, sizeof(buf))) {
            /* this is the name of a mci driver's type */
	    tmp = wcschr(dev, ' ');
	    if (tmp) *tmp = '\0';
	    data.open.lpstrDeviceType = dev;
	    devType = str_dup_upper(dev);
	    if (tmp) *tmp = ' ';
	    dwFlags |= MCI_OPEN_TYPE;
	} else {
	    if ((devType = wcsstr(args, L"type ")) != NULL) {
		devType += 5;
		tmp = wcschr(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = str_dup_upper(devType);
		if (tmp) *tmp = ' ';
		/* dwFlags and lpstrDeviceType will be correctly set in ParseOpt loop */
	    } else {
		if ((dwRet = MCI_GetDevTypeFromFileName(dev, buf, sizeof(buf))))
		    goto errCleanUp;

		devType = str_dup_upper(buf);
	    }
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data.open.lpstrElementName = dev;
	}
	if (MCI_ALL_DEVICE_ID == uDevID) {
	    dwRet = MCIERR_CANNOT_USE_ALL;
	    goto errCleanUp;
	}
	if (!wcsstr(args, L" alias ") && !dev) {
	    dwRet = MCIERR_NEW_REQUIRES_ALIAS;
	    goto errCleanUp;
	}

	dwRet = MCI_LoadMciDriver(devType, &wmd);
	if (dwRet == MCIERR_DEVICE_NOT_INSTALLED)
	    dwRet = MCIERR_INVALID_DEVICE_NAME;
	if (dwRet)
	    goto errCleanUp;
    } else if ((MCI_ALL_DEVICE_ID != uDevID) && !(wmd = MCI_GetDriver(mciGetDeviceIDW(dev)))
	       && (lpCmd = MCI_FindCommand(MCI_GetCommandTable(0), verb))) {
	/* auto-open uses the core command table */
	switch (MCI_GetMessage(lpCmd)) {
	case MCI_SOUND:   /* command does not use a device name */
	case MCI_SYSINFO:
	    break;
	case MCI_CLOSE:   /* don't auto-open for close */
	case MCI_BREAK:   /* no auto-open for system commands */
	    dwRet = MCIERR_INVALID_DEVICE_NAME;
	    goto errCleanUp;
	    break;
	default:
	    {
		WCHAR   buf[138], retbuf[6];
		swprintf(buf, ARRAY_SIZE(buf), L"open %s wait", dev);
		/* open via mciSendString handles quoting, dev!file syntax and alias creation */
		if ((dwRet = mciSendStringW(buf, retbuf, ARRAY_SIZE(retbuf), 0)) != 0)
		    goto errCleanUp;
		auto_open = wcstoul(retbuf, NULL, 10);
		TRACE("auto-opened %u for %s\n", auto_open, debugstr_w(dev));

		/* FIXME: test for notify flag (how to preparse?) before opening */
		wmd = MCI_GetDriver(auto_open);
		if (!wmd) {
		    ERR("No auto-open device %u\n", auto_open);
		    dwRet = MCIERR_INVALID_DEVICE_ID;
		    goto errCleanUp;
		}
	    }
	}
    }

    /* get the verb in the different command tables */
    if (wmd) {
	/* try the device specific command table */
	lpCmd = MCI_FindCommand(wmd->uSpecificCmdTable, verb);
	if (!lpCmd) {
	    /* try the type specific command table */
	    if (wmd->uTypeCmdTable == MCI_COMMAND_TABLE_NOT_LOADED)
		wmd->uTypeCmdTable = MCI_GetCommandTable(wmd->wType);
	    if (wmd->uTypeCmdTable != MCI_NO_COMMAND_TABLE)
		lpCmd = MCI_FindCommand(wmd->uTypeCmdTable, verb);
	}
    }
    /* try core command table */
    if (!lpCmd) lpCmd = MCI_FindCommand(MCI_GetCommandTable(0), verb);

    if (!lpCmd) {
	TRACE("Command %s not found!\n", debugstr_w(verb));
	dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	goto errCleanUp;
    }
    wMsg = MCI_GetMessage(lpCmd);

    /* set return information */
    offset = sizeof(data.generic);
    switch (retType = MCI_GetReturnType(lpCmd)) {
    case 0:
        break;
    case MCI_INTEGER:
        offset += sizeof(DWORD);
        break;
    case MCI_STRING:
        data.sysinfo.lpstrReturn = lpstrRet;
        data.sysinfo.dwRetSize = uRetLen;
        offset = FIELD_OFFSET( MCI_SYSINFO_PARMSW, dwNumber );
        break;
    case MCI_RECT:
        offset += 4 * sizeof(DWORD);
        break;
    case 13: /* MCI_INTEGER64 */
	offset += sizeof(DWORD_PTR);
        break;
    default:
	FIXME("Unknown MCI return type %ld\n", retType);
	dwRet = MCIERR_PARSER_INTERNAL;
	goto errCleanUp;
    }

    TRACE("verb=%s on dev=%s; offset=%d\n", 
          debugstr_w(verb), debugstr_w(dev), offset);

    if ((dwRet = MCI_ParseOptArgs(data.dw, offset / sizeof(DWORD), lpCmd, args, &dwFlags)))
	goto errCleanUp;

    /* set up call back */
    if (auto_open) {
	if (dwFlags & MCI_NOTIFY) {
	    dwRet = MCIERR_NOTIFY_ON_AUTO_OPEN;
	    goto errCleanUp;
	}
	/* FIXME: the command should get its own notification window set up and
	 * ask for device closing while processing the notification mechanism.
	 * hwndCallback = ...
	 * dwFlags |= MCI_NOTIFY;
	 * In the meantime special-case all commands but PLAY and RECORD below. */
    }
    if (dwFlags & MCI_NOTIFY) {
	data.generic.dwCallback = (DWORD_PTR)hwndCallback;
    }

    switch (wMsg) {
    case MCI_OPEN:
	if (wcscmp(verb, L"open")) {
	    FIXME("Cannot open with command %s\n", debugstr_w(verb));
	    dwRet = MCIERR_DRIVER_INTERNAL;
	    wMsg = 0;
	    goto errCleanUp;
	}
	break;
    case MCI_SYSINFO:
	/* Requirements on dev depend on the flags:
	 * alias with INSTALLNAME, name like "digitalvideo"
	 * with QUANTITY and NAME. */
	{
	    data.sysinfo.wDeviceType = MCI_ALL_DEVICE_ID;
	    if (uDevID != MCI_ALL_DEVICE_ID) {
		if (dwFlags & MCI_SYSINFO_INSTALLNAME)
		    wmd = MCI_GetDriver(mciGetDeviceIDW(dev));
		else if (!(data.sysinfo.wDeviceType = MCI_GetDevTypeFromResource(dev))) {
		    dwRet = MCIERR_DEVICE_TYPE_REQUIRED;
		    goto errCleanUp;
		}
	    }
	}
	break;
    case MCI_SOUND:
	/* FIXME: name is optional, "sound" is a valid command.
	 * FIXME: Parse "sound notify" as flag, not as name. */
	data.sound.lpstrSoundName = dev;
	dwFlags |= MCI_SOUND_NAME;
	break;
    }

    TRACE("[%d, %s, %08lx, %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx]\n",
	  wmd ? wmd->wDeviceID : uDevID, MCI_MessageToString(wMsg), dwFlags,
	  data.dw[0], data.dw[1], data.dw[2], data.dw[3], data.dw[4],
	  data.dw[5], data.dw[6], data.dw[7], data.dw[8], data.dw[9]);

    if (wMsg == MCI_OPEN) {
	if ((dwRet = MCI_FinishOpen(wmd, &data.open, dwFlags)))
	    goto errCleanUp;
	/* FIXME: notification is not properly shared across two opens */
    } else {
	dwRet = MCI_SendCommand(wmd ? wmd->wDeviceID : uDevID, wMsg, dwFlags, (DWORD_PTR)&data);
    }
    if (!LOWORD(dwRet)) {
	TRACE("=> 1/ %lx (%s)\n", dwRet, debugstr_w(lpstrRet));
	dwRet = MCI_HandleReturnValues(dwRet, wmd, retType, &data.generic, lpstrRet, uRetLen);
	TRACE("=> 2/ %lx (%s)\n", dwRet, debugstr_w(lpstrRet));
    } else
	TRACE("=> %lx\n", dwRet);

errCleanUp:
    if (auto_open) {
	/* PLAY and RECORD are the only known non-immediate commands */
	if (LOWORD(dwRet) || !(wMsg == MCI_PLAY || wMsg == MCI_RECORD))
	    MCI_SendCommand(auto_open, MCI_CLOSE, 0, 0);
	else
	    FIXME("leaking auto-open device %u\n", auto_open);
    }
    if (wMsg == MCI_OPEN && LOWORD(dwRet) && wmd)
	MCI_UnLoadMciDriver(wmd);
    HeapFree(GetProcessHeap(), 0, devType);
    HeapFree(GetProcessHeap(), 0, verb);
    return dwRet;
}

/**************************************************************************
 * 				mciSendStringA			[WINMM.@]
 */
DWORD WINAPI mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrRet,
			    UINT uRetLen, HWND hwndCallback)
{
    LPWSTR 	lpwstrCommand;
    LPWSTR      lpwstrRet = NULL;
    UINT	ret;
    INT len;

    /* FIXME: is there something to do with lpstrReturnString ? */
    len = MultiByteToWideChar( CP_ACP, 0, lpstrCommand, -1, NULL, 0 );
    lpwstrCommand = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpstrCommand, -1, lpwstrCommand, len );
    if (lpstrRet)
    {
        if (uRetLen) *lpstrRet = '\0'; /* NT-w2k3 use memset(lpstrRet, 0, uRetLen); */
        lpwstrRet = HeapAlloc(GetProcessHeap(), 0, uRetLen * sizeof(WCHAR));
        if (!lpwstrRet) {
            HeapFree( GetProcessHeap(), 0, lpwstrCommand );
            return MCIERR_OUT_OF_MEMORY;
        }
    }
    ret = mciSendStringW(lpwstrCommand, lpwstrRet, uRetLen, hwndCallback);
    if (!ret && lpwstrRet)
        WideCharToMultiByte( CP_ACP, 0, lpwstrRet, -1, lpstrRet, uRetLen, NULL, NULL );
    HeapFree(GetProcessHeap(), 0, lpwstrCommand);
    HeapFree(GetProcessHeap(), 0, lpwstrRet);
    return ret;
}

/**************************************************************************
 * 				mciExecute			[WINMM.@]
 */
BOOL WINAPI mciExecute(LPCSTR lpstrCommand)
{
    char	strRet[256];
    DWORD	ret;

    TRACE("(%s)!\n", lpstrCommand);

    ret = mciSendStringA(lpstrCommand, strRet, sizeof(strRet), 0);
    if (ret != 0) {
	if (!mciGetErrorStringA(ret, strRet, sizeof(strRet))) {
	    sprintf(strRet, "Unknown MCI error (%ld)", ret);
	}
	MessageBoxA(0, strRet, "Error in mciExecute()", MB_OK);
    }
    /* FIXME: what shall I return ? */
    return TRUE;
}

/**************************************************************************
 *                    	mciLoadCommandResource  		[WINMM.@]
 *
 * Strangely, this function only exists as a UNICODE one.
 */
UINT WINAPI mciLoadCommandResource(HINSTANCE hInst, LPCWSTR resNameW, UINT type)
{
    UINT        ret = MCI_NO_COMMAND_TABLE;
    HRSRC	hRsrc = 0;
    HGLOBAL     hMem;

    TRACE("(%p, %s, %d)!\n", hInst, debugstr_w(resNameW), type);

    /* if a file named "resname.mci" exits, then load resource "resname" from it
     * otherwise directly from driver
     * We don't support it (who uses this feature ?), but we check anyway
     */
    if (!type) {
#if 0
        /* FIXME: we should put this back into order, but I never found a program
         * actually using this feature, so we may not need it
         */
	char		buf[128];
	OFSTRUCT       	ofs;

	strcat(strcpy(buf, resname), ".mci");
	if (OpenFile(buf, &ofs, OF_EXIST) != HFILE_ERROR) {
	    FIXME("NIY: command table to be loaded from '%s'\n", ofs.szPathName);
	}
#endif
    }
    if ((hRsrc = FindResourceW(hInst, resNameW, (LPWSTR)RT_RCDATA)) &&
        (hMem = LoadResource(hInst, hRsrc))) {
        ret = MCI_SetCommandTable(hMem, type);
        FreeResource(hMem);
    }
    else WARN("No command table found in module for %s\n", debugstr_w(resNameW));

    TRACE("=> %04x\n", ret);
    return ret;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[WINMM.@]
 */
BOOL WINAPI mciFreeCommandResource(UINT uTable)
{
    TRACE("(%08x)!\n", uTable);

    if (uTable >= MAX_MCICMDTABLE || !S_MciCmdTable[uTable].lpTable)
	return FALSE;

    FreeResource(S_MciCmdTable[uTable].hMem);
    S_MciCmdTable[uTable].hMem = NULL;
    S_MciCmdTable[uTable].lpTable = NULL;
    HeapFree(GetProcessHeap(), 0, S_MciCmdTable[uTable].aVerbs);
    S_MciCmdTable[uTable].aVerbs = 0;
    S_MciCmdTable[uTable].nVerbs = 0;
    return TRUE;
}

/**************************************************************************
 * 			MCI_Open				[internal]
 */
static	DWORD MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSW lpParms)
{
    WCHAR			strDevTyp[128];
    DWORD 			dwRet;
    LPWINE_MCIDRIVER		wmd = NULL;

    TRACE("(%08lX, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    /* only two low bytes are generic, the other ones are dev type specific */
#define WINE_MCIDRIVER_SUPP	(0xFFFF0000|MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT| \
                         MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID| \
                         MCI_NOTIFY|MCI_WAIT)
    if ((dwParam & ~WINE_MCIDRIVER_SUPP) != 0)
        FIXME("Unsupported yet dwFlags=%08lX\n", dwParam);
#undef WINE_MCIDRIVER_SUPP

    strDevTyp[0] = 0;

    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
	    WORD uDevType = LOWORD(lpParms->lpstrDeviceType);

	    if (uDevType < MCI_DEVTYPE_FIRST || uDevType > MCI_DEVTYPE_LAST ||
		!LoadStringW(hWinMM32Instance, uDevType, strDevTyp, ARRAY_SIZE(strDevTyp))) {
		dwRet = MCIERR_BAD_INTEGER;
		goto errCleanUp;
	    }
	} else {
	    LPWSTR	ptr;
	    if (lpParms->lpstrDeviceType == NULL) {
		dwRet = MCIERR_NULL_PARAMETER_BLOCK;
		goto errCleanUp;
	    }
	    lstrcpyW(strDevTyp, lpParms->lpstrDeviceType);
	    ptr = wcschr(strDevTyp, '!');
	    if (ptr) {
		/* this behavior is not documented in windows. However, since, in
		 * some occasions, MCI_OPEN handling is translated by WinMM into
		 * a call to mciSendString("open <type>"); this code shall be correct
		 */
		if (dwParam & MCI_OPEN_ELEMENT) {
		    ERR("Both MCI_OPEN_ELEMENT(%s) and %s are used\n",
			debugstr_w(lpParms->lpstrElementName), 
                        debugstr_w(strDevTyp));
		    dwRet = MCIERR_UNRECOGNIZED_KEYWORD;
		    goto errCleanUp;
		}
		dwParam |= MCI_OPEN_ELEMENT;
		*ptr++ = 0;
		/* FIXME: not a good idea to write in user supplied buffer */
		lpParms->lpstrElementName = ptr;
	    }

	}
	TRACE("devType=%s !\n", debugstr_w(strDevTyp));
    }

    if (dwParam & MCI_OPEN_ELEMENT) {
	TRACE("lpstrElementName=%s\n", debugstr_w(lpParms->lpstrElementName));

	if (dwParam & MCI_OPEN_ELEMENT_ID) {
	    FIXME("Unsupported yet flag MCI_OPEN_ELEMENT_ID\n");
	    dwRet = MCIERR_UNRECOGNIZED_KEYWORD;
	    goto errCleanUp;
	}

	if (!lpParms->lpstrElementName) {
	    dwRet = MCIERR_NULL_PARAMETER_BLOCK;
	    goto errCleanUp;
	}

	/* type, if given as a parameter, supersedes file extension */
	if (!strDevTyp[0] &&
	    MCI_GetDevTypeFromFileName(lpParms->lpstrElementName,
				       strDevTyp, sizeof(strDevTyp))) {
	    if (GetDriveTypeW(lpParms->lpstrElementName) != DRIVE_CDROM) {
		dwRet = MCIERR_EXTENSION_NOT_FOUND;
		goto errCleanUp;
	    }
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    lstrcpyW(strDevTyp, L"CDAUDIO");
	}
    }

    if (strDevTyp[0] == 0) {
	FIXME("Couldn't load driver\n");
	dwRet = MCIERR_INVALID_DEVICE_NAME;
	goto errCleanUp;
    }

    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE("Alias=%s !\n", debugstr_w(lpParms->lpstrAlias));
	if (!lpParms->lpstrAlias) {
	    dwRet = MCIERR_NULL_PARAMETER_BLOCK;
	    goto errCleanUp;
	}
    }

    if ((dwRet = MCI_LoadMciDriver(strDevTyp, &wmd))) {
	goto errCleanUp;
    }

    if ((dwRet = MCI_FinishOpen(wmd, lpParms, dwParam))) {
	TRACE("Failed to open driver (MCI_OPEN_DRIVER) [%08lx], closing\n", dwRet);
	/* FIXME: is dwRet the correct ret code ? */
	goto errCleanUp;
    }

    /* only handled devices fall through */
    TRACE("wDevID=%04X wDeviceID=%d dwRet=%ld\n", wmd->wDeviceID, lpParms->wDeviceID, dwRet);
    return 0;

errCleanUp:
    if (wmd) MCI_UnLoadMciDriver(wmd);
    return dwRet;
}

/**************************************************************************
 * 			MCI_Close				[internal]
 */
static	DWORD MCI_Close(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    /* Every device must handle MCI_NOTIFY on its own. */
    if ((UINT16)wDevID == (UINT16)MCI_ALL_DEVICE_ID) {
	while (MciDrivers) {
            /* Retrieve the device ID under lock, but send the message without,
             * the driver might be calling some winmm functions from another
             * thread before being fully stopped.
             */
            EnterCriticalSection(&WINMM_cs);
            if (!MciDrivers)
            {
                LeaveCriticalSection(&WINMM_cs);
                break;
            }
            wDevID = MciDrivers->wDeviceID;
            LeaveCriticalSection(&WINMM_cs);
            MCI_Close(wDevID, dwParam, lpParms);
	}
	return 0;
    }

    if (!(wmd = MCI_GetDriver(wDevID))) {
	return MCIERR_INVALID_DEVICE_ID;
    }

    if(wmd->CreatorThread != GetCurrentThreadId())
        return MCIERR_INVALID_DEVICE_NAME;

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD_PTR)lpParms);

    MCI_UnLoadMciDriver(wmd);

    return dwRet;
}

/**************************************************************************
 * 			MCI_WriteString				[internal]
 */
static DWORD MCI_WriteString(LPWSTR lpDstStr, DWORD dstSize, LPCWSTR lpSrcStr)
{
    DWORD	ret = 0;

    if (lpSrcStr) {
	if (dstSize <= lstrlenW(lpSrcStr)) {
	    ret = MCIERR_PARAM_OVERFLOW;
	} else {
	    lstrcpyW(lpDstStr, lpSrcStr);
	}
    } else {
	*lpDstStr = 0;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Sysinfo				[internal]
 */
static	DWORD MCI_SysInfo(UINT uDevID, DWORD dwFlags, LPMCI_SYSINFO_PARMSW lpParms)
{
    DWORD		ret = MCIERR_INVALID_DEVICE_ID, cnt = 0;
    WCHAR		buf[2048], *s, *p;
    LPWINE_MCIDRIVER	wmd;
    HKEY		hKey;

    if (lpParms == NULL)			return MCIERR_NULL_PARAMETER_BLOCK;
    if (lpParms->lpstrReturn == NULL)		return MCIERR_PARAM_OVERFLOW;

    TRACE("(%08x, %08lX, %p[num=%ld, wDevTyp=%u])\n",
	  uDevID, dwFlags, lpParms, lpParms->dwNumber, lpParms->wDeviceType);
    if ((WORD)MCI_ALL_DEVICE_ID == LOWORD(uDevID))
	uDevID = MCI_ALL_DEVICE_ID; /* Be compatible with Win9x */

    switch (dwFlags & ~(MCI_SYSINFO_OPEN|MCI_NOTIFY|MCI_WAIT)) {
    case MCI_SYSINFO_QUANTITY:
	if (lpParms->dwRetSize < sizeof(DWORD))
	    return MCIERR_PARAM_OVERFLOW;
	/* Win9x returns 0 for 0 < uDevID < (UINT16)MCI_ALL_DEVICE_ID */
	if (uDevID == MCI_ALL_DEVICE_ID) {
	    /* wDeviceType == MCI_ALL_DEVICE_ID is not recognized. */
	    if (dwFlags & MCI_SYSINFO_OPEN) {
		TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers\n");
		EnterCriticalSection(&WINMM_cs);
		for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
		    cnt++;
		}
		LeaveCriticalSection(&WINMM_cs);
	    } else {
		TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers\n");
		if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, wszHklmMci,
			  	   0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS) {
		    RegQueryInfoKeyW( hKey, 0, 0, 0, &cnt, 0, 0, 0, 0, 0, 0, 0);
		    RegCloseKey( hKey );
		}
		if (GetPrivateProfileStringW(L"MCI", 0, L"", buf, ARRAY_SIZE(buf), L"system.ini"))
		    for (s = buf; *s; s += lstrlenW(s) + 1) cnt++;
	    }
	} else {
	    if (dwFlags & MCI_SYSINFO_OPEN) {
		TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %d\n", lpParms->wDeviceType);
		EnterCriticalSection(&WINMM_cs);
		for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
		    if (wmd->wType == lpParms->wDeviceType) cnt++;
		}
		LeaveCriticalSection(&WINMM_cs);
	    } else {
		TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %d\n", lpParms->wDeviceType);
		FIXME("Don't know how to get # of MCI devices of a given type\n");
		/* name = LoadStringW(hWinMM32Instance, LOWORD(lpParms->wDeviceType))
		 * then lookup registry and/or system.ini for name, ignoring digits suffix */
		switch (LOWORD(lpParms->wDeviceType)) {
		case MCI_DEVTYPE_CD_AUDIO:
		case MCI_DEVTYPE_WAVEFORM_AUDIO:
		case MCI_DEVTYPE_SEQUENCER:
		    cnt = 1;
		    break;
		default: /* "digitalvideo" gets 0 because it's not in the registry */
		    cnt = 0;
		}
	    }
	}
	*(DWORD*)lpParms->lpstrReturn = cnt;
	TRACE("(%ld) => '%ld'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = MCI_INTEGER_RETURNED;
	/* return ret; Only Win9x sends a notification in this case. */
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE("MCI_SYSINFO_INSTALLNAME\n");
	if ((wmd = MCI_GetDriver(uDevID))) {
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize,
				  wmd->lpstrDeviceType);
	} else {
	    ret = (uDevID == MCI_ALL_DEVICE_ID)
		? MCIERR_CANNOT_USE_ALL : MCIERR_INVALID_DEVICE_NAME;
	}
	TRACE("(%ld) => %s\n", lpParms->dwNumber, debugstr_w(lpParms->lpstrReturn));
	break;
    case MCI_SYSINFO_NAME:
	s = NULL;
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    /* Win9x returns 0 for 0 < uDevID < (UINT16)MCI_ALL_DEVICE_ID */
	    TRACE("MCI_SYSINFO_NAME: nth alias of type %d\n",
		  uDevID == MCI_ALL_DEVICE_ID ? MCI_ALL_DEVICE_ID : lpParms->wDeviceType);
	    EnterCriticalSection(&WINMM_cs);
	    for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
		/* wDeviceType == MCI_ALL_DEVICE_ID is not recognized. */
		if (uDevID == MCI_ALL_DEVICE_ID ||
		    lpParms->wDeviceType == wmd->wType) {
		    cnt++;
		    if (cnt == lpParms->dwNumber) {
			s = wmd->lpstrAlias;
			break;
		    }
		}
	    }
	    LeaveCriticalSection(&WINMM_cs);
	    ret = s ? MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, s) : MCIERR_OUTOFRANGE;
	} else if (MCI_ALL_DEVICE_ID == uDevID) {
	    TRACE("MCI_SYSINFO_NAME: device #%ld\n", lpParms->dwNumber);
	    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, wszHklmMci, 0, 
                               KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS) {
		if (RegQueryInfoKeyW( hKey, 0, 0, 0, &cnt, 
                                      0, 0, 0, 0, 0, 0, 0) == ERROR_SUCCESS && 
                    lpParms->dwNumber <= cnt) {
		    DWORD bufLen = ARRAY_SIZE(buf);
		    if (RegEnumKeyExW(hKey, lpParms->dwNumber - 1, 
                                      buf, &bufLen, 0, 0, 0, 0) == ERROR_SUCCESS)
                        s = buf;
		}
	        RegCloseKey( hKey );
	    }
	    if (!s) {
		if (GetPrivateProfileStringW(L"MCI", 0, L"", buf, ARRAY_SIZE(buf), L"system.ini")) {
		    for (p = buf; *p; p += lstrlenW(p) + 1, cnt++) {
                        TRACE("%ld: %s\n", cnt, debugstr_w(p));
			if (cnt == lpParms->dwNumber - 1) {
			    s = p;
			    break;
			}
		    }
		}
	    }
	    ret = s ? MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, s) : MCIERR_OUTOFRANGE;
	} else {
	    FIXME("MCI_SYSINFO_NAME: nth device of type %d\n", lpParms->wDeviceType);
	    /* Cheating: what is asked for is the nth device from the registry. */
	    if (1 != lpParms->dwNumber || /* Handle only one of each kind. */
		lpParms->wDeviceType < MCI_DEVTYPE_FIRST || lpParms->wDeviceType > MCI_DEVTYPE_LAST)
		ret = MCIERR_OUTOFRANGE;
	    else {
		LoadStringW(hWinMM32Instance, LOWORD(lpParms->wDeviceType),
			    lpParms->lpstrReturn, lpParms->dwRetSize);
		ret = 0;
	    }
	}
	TRACE("(%ld) => %s\n", lpParms->dwNumber, debugstr_w(lpParms->lpstrReturn));
	break;
    default:
	TRACE("Unsupported flag value=%08lx\n", dwFlags);
	ret = MCIERR_UNRECOGNIZED_KEYWORD;
    }
    if ((dwFlags & MCI_NOTIFY) && HRESULT_CODE(ret)==0)
	mciDriverNotify((HWND)lpParms->dwCallback, uDevID, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 			MCI_Break				[internal]
 */
static	DWORD MCI_Break(UINT wDevID, DWORD dwFlags, LPMCI_BREAK_PARMS lpParms)
{
    DWORD dwRet;

    if (lpParms == NULL)
        return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("(%08x, %08lX, vkey %04X, hwnd %p)\n", wDevID, dwFlags,
          lpParms->nVirtKey, lpParms->hwndBreak);

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_BREAK, dwFlags, (DWORD_PTR)lpParms);
    if (!dwRet && (dwFlags & MCI_NOTIFY))
        mciDriverNotify((HWND)lpParms->dwCallback, wDevID, MCI_NOTIFY_SUCCESSFUL);
    return dwRet;
}

/**************************************************************************
 * 			MCI_Sound				[internal]
 */
static	DWORD MCI_Sound(UINT wDevID, DWORD dwFlags, LPMCI_SOUND_PARMSW lpParms)
{
    DWORD	dwRet;

    if (dwFlags & MCI_SOUND_NAME) {
	if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
	else dwRet = PlaySoundW(lpParms->lpstrSoundName, NULL,
				SND_ALIAS    | (dwFlags & MCI_WAIT ? SND_SYNC : SND_ASYNC))
		? 0 : MCIERR_HARDWARE;
    } else   dwRet = PlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMDEFAULT, NULL,
				SND_ALIAS_ID | (dwFlags & MCI_WAIT ? SND_SYNC : SND_ASYNC))
		? 0 : MCIERR_HARDWARE;

    if (!dwRet && lpParms && (dwFlags & MCI_NOTIFY))
        mciDriverNotify((HWND)lpParms->dwCallback, wDevID, MCI_NOTIFY_SUCCESSFUL);
    return dwRet;
}

/**************************************************************************
 * 			MCI_SendCommand				[internal]
 */
DWORD	MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD		dwRet = MCIERR_UNRECOGNIZED_COMMAND;

    switch (wMsg) {
    case MCI_OPEN:
        dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSW)dwParam2);
	break;
    case MCI_CLOSE:
        dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
	break;
    case MCI_SYSINFO:
        dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSW)dwParam2);
	break;
    case MCI_BREAK:
        dwRet = MCI_Break(wDevID, dwParam1, (LPMCI_BREAK_PARMS)dwParam2);
	break;
    case MCI_SOUND:
        dwRet = MCI_Sound(wDevID, dwParam1, (LPMCI_SOUND_PARMSW)dwParam2);
	break;
    default:
      if ((UINT16)wDevID == (UINT16)MCI_ALL_DEVICE_ID) {
	    FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    dwRet = MCI_SendCommandFrom32(wDevID, wMsg, dwParam1, dwParam2);
	}
	break;
    }
    return dwRet;
}

/**************************************************************************
 * 				MCI_CleanUp			[internal]
 *
 * Some MCI commands need to be cleaned-up (when not called from
 * mciSendString), because MCI drivers return extra information for string
 * transformation. This function gets rid of them.
 */
static LRESULT	MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD_PTR dwParam2)
{
    if (LOWORD(dwRet))
	return LOWORD(dwRet);

    switch (wMsg) {
    case MCI_GETDEVCAPS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	    {
		LPMCI_GETDEVCAPS_PARMS	lmgp;

		lmgp = (LPMCI_GETDEVCAPS_PARMS)dwParam2;
		TRACE("Changing %08lx to %08x\n", lmgp->dwReturn, LOWORD(lmgp->dwReturn));
		lmgp->dwReturn = LOWORD(lmgp->dwReturn);
	    }
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc(%s)\n",
		  HIWORD(dwRet), MCI_MessageToString(wMsg));
	}
	break;
    case MCI_STATUS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	    {
		LPMCI_STATUS_PARMS	lsp;

		lsp = (LPMCI_STATUS_PARMS)dwParam2;
		TRACE("Changing %08Ix to %08x\n", lsp->dwReturn, LOWORD(lsp->dwReturn));
		lsp->dwReturn = LOWORD(lsp->dwReturn);
	    }
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc(%s)\n",
		  HIWORD(dwRet), MCI_MessageToString(wMsg));
	}
	break;
    case MCI_SYSINFO:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x)\n", HIWORD(dwRet));
	}
	break;
    default:
	if (HIWORD(dwRet)) {
	    FIXME("Got non null hiword for dwRet=0x%08Ix for command %s\n",
		  dwRet, MCI_MessageToString(wMsg));
	}
	break;
    }
    return LOWORD(dwRet);
}

/**************************************************************************
 * 				mciGetErrorStringW		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringW(MCIERROR wError, LPWSTR lpstrBuffer, UINT uLength)
{
    BOOL		ret = FALSE;

    if (lpstrBuffer != NULL && uLength > 0 &&
	wError >= MCIERR_BASE && wError <= MCIERR_CUSTOM_DRIVER_BASE) {

	if (LoadStringW(hWinMM32Instance, wError, lpstrBuffer, uLength) > 0) {
	    ret = TRUE;
	}
    }
    return ret;
}

/**************************************************************************
 * 				mciGetErrorStringA		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringA(MCIERROR dwError, LPSTR lpstrBuffer, UINT uLength)
{
    BOOL		ret = FALSE;

    if (lpstrBuffer != NULL && uLength > 0 &&
	dwError >= MCIERR_BASE && dwError <= MCIERR_CUSTOM_DRIVER_BASE) {

	if (LoadStringA(hWinMM32Instance, dwError, lpstrBuffer, uLength) > 0) {
	    ret = TRUE;
	}
    }
    return ret;
}

/**************************************************************************
 *			mciDriverNotify				[WINMM.@]
 */
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, MCIDEVICEID wDevID, UINT wStatus)
{
    TRACE("(%p, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageW(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[WINMM.@]
 */
DWORD_PTR WINAPI mciGetDriverData(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x)\n", uDeviceID);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return 0L;
    }

    return wmd->dwPrivate;
}

/**************************************************************************
 * 			mciSetDriverData			[WINMM.@]
 */
BOOL WINAPI mciSetDriverData(MCIDEVICEID uDeviceID, DWORD_PTR data)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08Ix)\n", uDeviceID, data);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->dwPrivate = data;
    return TRUE;
}

/**************************************************************************
 * 				mciSendCommandW			[WINMM.@]
 *
 */
DWORD WINAPI mciSendCommandW(MCIDEVICEID wDevID, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD	dwRet;

    TRACE("(%08x, %s, %08Ix, %08Ix)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2);
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2);
    TRACE("=> %08lx\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciSendCommandA			[WINMM.@]
 */
DWORD WINAPI mciSendCommandA(MCIDEVICEID wDevID, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD ret;
    int mapped;

    TRACE("(%08x, %s, %08Ix, %08Ix)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    mapped = MCI_MapMsgAtoW(wMsg, dwParam1, &dwParam2);
    if (mapped == -1)
    {
        FIXME("message %04x mapping failed\n", wMsg);
        return MCIERR_OUT_OF_MEMORY;
    }
    ret = mciSendCommandW(wDevID, wMsg, dwParam1, dwParam2);
    if (mapped)
        MCI_UnmapMsgAtoW(wMsg, dwParam1, dwParam2, ret);
    return ret;
}

/**************************************************************************
 * 				mciGetDeviceIDA    		[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDA(LPCSTR lpstrName)
{
    LPWSTR w = MCI_strdupAtoW(lpstrName);
    UINT ret = MCIERR_OUT_OF_MEMORY;

    if (w)
    {
        ret = mciGetDeviceIDW(w);
        HeapFree(GetProcessHeap(), 0, w);
    }
    return ret;
}

/**************************************************************************
 * 				mciGetDeviceIDW		       	[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDW(LPCWSTR lpwstrName)
{
    return MCI_GetDriverFromString(lpwstrName); 
}

/**************************************************************************
 * 				MCI_DefYieldProc	       	[internal]
 */
static UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data)
{
    INT16	ret;
    MSG		msg;

    TRACE("(0x%04x, 0x%08lx)\n", wDevID, data);

    if ((HIWORD(data) != 0 && HWND_16(GetActiveWindow()) != HIWORD(data)) ||
	(GetAsyncKeyState(LOWORD(data)) & 1) == 0) {
        PeekMessageW(&msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE);
	ret = 0;
    } else {
	msg.hwnd = HWND_32(HIWORD(data));
	while (!PeekMessageW(&msg, msg.hwnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
	ret = -1;
    }
    return ret;
}

/**************************************************************************
 * 				mciSetYieldProc			[WINMM.@]
 */
BOOL WINAPI mciSetYieldProc(MCIDEVICEID uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->lpfnYieldProc = fpYieldProc;
    wmd->dwYieldData   = dwYieldData;

    return TRUE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementIDA	[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDFromElementIDA(DWORD dwElementID, LPCSTR lpstrType)
{
    LPWSTR w = MCI_strdupAtoW(lpstrType);
    UINT ret = 0;

    if (w)
    {
        ret = mciGetDeviceIDFromElementIDW(dwElementID, w);
        HeapFree(GetProcessHeap(), 0, w);
    }
    return ret;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementIDW	[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDFromElementIDW(DWORD dwElementID, LPCWSTR lpstrType)
{
    /* FIXME: that's rather strange, there is no
     * mciGetDeviceIDFromElementID32A in winmm.spec
     */
    FIXME("(%lu, %s) stub\n", dwElementID, debugstr_w(lpstrType));
    return 0;
}

/**************************************************************************
 * 				mciGetYieldProc			[WINMM.@]
 */
YIELDPROC WINAPI mciGetYieldProc(MCIDEVICEID uDeviceID, DWORD* lpdwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return NULL;
    }
    if (!wmd->lpfnYieldProc) {
	WARN("No proc set\n");
	return NULL;
    }
    if (lpdwYieldData) *lpdwYieldData = wmd->dwYieldData;
    return wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetCreatorTask		[WINMM.@]
 */
HTASK WINAPI mciGetCreatorTask(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    HTASK ret = 0;

    if ((wmd = MCI_GetDriver(uDeviceID))) ret = (HTASK)(DWORD_PTR)wmd->CreatorThread;

    TRACE("(%u) => %p\n", uDeviceID, ret);
    return ret;
}

/**************************************************************************
 * 			mciDriverYield				[WINMM.@]
 */
UINT WINAPI mciDriverYield(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    UINT		ret = 0;

    TRACE("(%04x)\n", uDeviceID);

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc) {
        MSG msg;
        PeekMessageW(&msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE);
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}
