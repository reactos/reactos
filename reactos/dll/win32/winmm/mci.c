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

/* to be cross checked:
 * - heapalloc for *sizeof(WCHAR) when needed
 * - size of string in WCHAR or bytes? (#chars for MCI_INFO, #bytes for MCI_SYSINFO)
 */

#include "winemm.h"

#include <mmsystem.h>
#include <wownt32.h>
#include <digitalv.h>

WINE_DEFAULT_DEBUG_CHANNEL(mci);

/* First MCI valid device ID (0 means error) */
#define MCI_MAGIC 0x0001

/* MCI settings */
static const WCHAR wszHklmMci  [] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','M','C','I',0};
static const WCHAR wszNull     [] = {0};
static const WCHAR wszAll      [] = {'A','L','L',0};
static const WCHAR wszMci      [] = {'M','C','I',0};
static const WCHAR wszOpen     [] = {'o','p','e','n',0};
static const WCHAR wszSystemIni[] = {'s','y','s','t','e','m','.','i','n','i',0};

static WINE_MCIDRIVER *MciDrivers;

static UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data);
static UINT MCI_SetCommandTable(HGLOBAL hMem, UINT uDevType);

/* dup a string and uppercase it */
static inline LPWSTR str_dup_upper( LPCWSTR str )
{
    INT len = (strlenW(str) + 1) * sizeof(WCHAR);
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

    if (!strcmpiW(lpstrName, wszAll))
	return MCI_ALL_DEVICE_ID;

    EnterCriticalSection(&WINMM_cs);
    for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
	if (wmd->lpstrElementName && strcmpW(wmd->lpstrElementName, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	if (wmd->lpstrDeviceType && strcmpiW(wmd->lpstrDeviceType, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	if (wmd->lpstrAlias && strcmpiW(wmd->lpstrAlias, lpstrName) == 0) {
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
const char* MCI_MessageToString(UINT wMsg)
{
    static char buffer[100];

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
	sprintf(buffer, "MCI_<<%04X>>", wMsg);
	return buffer;
    }
}

LPWSTR MCI_strdupAtoW( LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

LPSTR MCI_strdupWtoA( LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = HeapAlloc( GetProcessHeap(), 0, len );
    if (ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
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
    case MCI_SOUND:
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
    case MCI_SETAUDIO:
    case MCI_SIGNAL:
    case MCI_SETVIDEO:
    case MCI_LIST:
        return 0;

    case MCI_OPEN:
        {
            MCI_OPEN_PARMSA *mci_openA = (MCI_OPEN_PARMSA*)*dwParam2;
            MCI_OPEN_PARMSW *mci_openW;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD_PTR) + sizeof(*mci_openW) + 2 * sizeof(DWORD));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_openW = (MCI_OPEN_PARMSW *)ptr;

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
            /* FIXME: this is only needed for specific types of MCI devices, and
             * may cause a segfault if the two DWORD:s don't exist at the end of 
             * mci_openA
             */
            memcpy(mci_openW + 1, mci_openA + 1, 2 * sizeof(DWORD));
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

            mci_sysinfoW->dwRetSize = mci_sysinfoA->dwRetSize;
            mci_sysinfoW->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mci_sysinfoW->dwRetSize);
            mci_sysinfoW->dwNumber = mci_sysinfoA->dwNumber;
            mci_sysinfoW->wDeviceType = mci_sysinfoA->wDeviceType;
            return 1;
        }
    case MCI_INFO:
        {
            MCI_INFO_PARMSA *mci_infoA = (MCI_INFO_PARMSA *)*dwParam2;
            MCI_INFO_PARMSW *mci_infoW;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_infoW) + sizeof(DWORD_PTR));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_infoW = (MCI_INFO_PARMSW *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_infoW->dwCallback = mci_infoA->dwCallback;

            mci_infoW->dwRetSize = mci_infoA->dwRetSize * sizeof(WCHAR); /* it's not the same as SYSINFO !!! */
            mci_infoW->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mci_infoW->dwRetSize);
            return 1;
        }
    case MCI_SAVE:
        {
            MCI_SAVE_PARMSA *mci_saveA = (MCI_SAVE_PARMSA *)*dwParam2;
            MCI_SAVE_PARMSW *mci_saveW;

            mci_saveW = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_saveW));
            if (!mci_saveW) return -1;

            *dwParam2 = (DWORD_PTR)mci_saveW;
            if (dwParam1 & MCI_NOTIFY)
                mci_saveW->dwCallback = mci_saveA->dwCallback;
            mci_saveW->lpfilename = MCI_strdupAtoW(mci_saveA->lpfilename);
            return 1;
        }
    case MCI_LOAD:
        {
            MCI_LOAD_PARMSA *mci_loadA = (MCI_LOAD_PARMSA *)*dwParam2;
            MCI_LOAD_PARMSW *mci_loadW;

            mci_loadW = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_loadW));
            if (!mci_loadW) return -1;

            *dwParam2 = (DWORD_PTR)mci_loadW;
            if (dwParam1 & MCI_NOTIFY)
                mci_loadW->dwCallback = mci_loadA->dwCallback;
            mci_loadW->lpfilename = MCI_strdupAtoW(mci_loadA->lpfilename);
            return 1;
        }

    case MCI_ESCAPE:
        {
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
    default:
        FIXME("Message %s needs translation\n", MCI_MessageToString(msg));
        return -1;
    }
}

static DWORD MCI_UnmapMsgAtoW(UINT msg, DWORD_PTR dwParam1, DWORD_PTR dwParam2,
                              DWORD result)
{
    switch (msg)
    {
    case MCI_OPEN:
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_OPEN_PARMSA *mci_openA = (MCI_OPEN_PARMSA *)*ptr;
            MCI_OPEN_PARMSW *mci_openW = (MCI_OPEN_PARMSW *)(ptr + 1);

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
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_SYSINFO_PARMSA *mci_sysinfoA = (MCI_SYSINFO_PARMSA *)*ptr;
            MCI_SYSINFO_PARMSW *mci_sysinfoW = (MCI_SYSINFO_PARMSW *)(ptr + 1);

            if (!result)
            {
                mci_sysinfoA->dwNumber = mci_sysinfoW->dwNumber;
                mci_sysinfoA->wDeviceType = mci_sysinfoW->wDeviceType;
                if (dwParam1 & MCI_SYSINFO_QUANTITY)
                    *(DWORD*)mci_sysinfoA->lpstrReturn = *(DWORD*)mci_sysinfoW->lpstrReturn;
                else
                    WideCharToMultiByte(CP_ACP, 0,
                                        mci_sysinfoW->lpstrReturn, mci_sysinfoW->dwRetSize,
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
            MCI_INFO_PARMSW *mci_infoW = (MCI_INFO_PARMSW *)(ptr + 1);

            if (!result)
            {
                WideCharToMultiByte(CP_ACP, 0,
                                    mci_infoW->lpstrReturn, mci_infoW->dwRetSize / sizeof(WCHAR),
                                    mci_infoA->lpstrReturn, mci_infoA->dwRetSize,
                                    NULL, NULL);
            }

            HeapFree(GetProcessHeap(), 0, mci_infoW->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;
    case MCI_SAVE:
        {
            MCI_SAVE_PARMSW *mci_saveW = (MCI_SAVE_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_saveW->lpfilename);
            HeapFree(GetProcessHeap(), 0, mci_saveW);
        }
        break;
    case MCI_LOAD:
        {
            MCI_LOAD_PARMSW *mci_loadW = (MCI_LOAD_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_loadW->lpfilename);
            HeapFree(GetProcessHeap(), 0, mci_loadW);
        }
        break;
    case MCI_ESCAPE:
        {
            MCI_VD_ESCAPE_PARMSW *mci_vd_escapeW = (MCI_VD_ESCAPE_PARMSW *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void*)mci_vd_escapeW->lpstrCommand);
            HeapFree(GetProcessHeap(), 0, mci_vd_escapeW);
        }
        break;

    default:
        FIXME("Message %s needs unmapping\n", MCI_MessageToString(msg));
        break;
    }

    return result;
}

/**************************************************************************
 * 				MCI_GetDevTypeFromFileName	[internal]
 */
static	DWORD	MCI_GetDevTypeFromFileName(LPCWSTR fileName, LPWSTR buf, UINT len)
{
    LPCWSTR	tmp;
    HKEY	hKey;
    static const WCHAR keyW[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\',
                                 'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                 'M','C','I',' ','E','x','t','e','n','s','i','o','n','s',0};
    if ((tmp = strrchrW(fileName, '.'))) {
	if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, keyW,
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
        lmem += (strlenW(str) + 1) * sizeof(WCHAR);
        flg = *(const DWORD*)lmem;
        eid = *(const WORD*)(lmem + sizeof(DWORD));
        lmem += sizeof(DWORD) + sizeof(WORD);
        idx ++;
        /* TRACE("cmd=%s %08lx %04x\n", debugstr_w(str), flg, eid); */
        switch (eid) {
        case MCI_COMMAND_HEAD:          if (!*str || !flg) return FALSE; idx = 0;		break;	/* check unicity of str in table */
        case MCI_STRING:                if (inCst) return FALSE;				break;
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
    DWORD	flg;
    WORD	eid;

    if (!MCI_IsCommandTableValid(uTbl)) {
	ERR("Ooops: %d is not valid\n", uTbl);
	return FALSE;
    }

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
	do {
	    str = (LPCWSTR)lmem;
	    lmem += (strlenW(str) + 1) * sizeof(WCHAR);
	    flg = *(const DWORD*)lmem;
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
	if (LoadStringW(hWinMM32Instance, uDevType, buf, sizeof(buf) / sizeof(WCHAR))) {
	    str = buf;
	}
    } else if (uDevType == 0) {
        static const WCHAR wszCore[] = {'C','O','R','E',0};
	str = wszCore;
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
		lmem += (strlenW(str) + 1) * sizeof(WCHAR);
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
		lmem += (strlenW(str) + 1) * sizeof(WCHAR);
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
    HeapFree(GetProcessHeap(), 0, wmd->lpstrElementName);

    HeapFree(GetProcessHeap(), 0, wmd);
    return TRUE;
}

/**************************************************************************
 * 				MCI_OpenMciDriver		[internal]
 */
static	BOOL	MCI_OpenMciDriver(LPWINE_MCIDRIVER wmd, LPCWSTR drvTyp, DWORD_PTR lp)
{
    WCHAR	libName[128];

    if (!DRIVER_GetLibName(drvTyp, wszMci, libName, sizeof(libName)))
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
	if (strcmpiW(strDevTyp, wszAll) == 0) {
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
        dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_FinishOpen				[internal]
 */
static	DWORD	MCI_FinishOpen(LPWINE_MCIDRIVER wmd, LPMCI_OPEN_PARMSW lpParms,
			       DWORD dwParam)
{
    if (dwParam & MCI_OPEN_ELEMENT)
    {
        wmd->lpstrElementName = HeapAlloc(GetProcessHeap(),0,(strlenW(lpParms->lpstrElementName)+1) * sizeof(WCHAR));
        strcpyW( wmd->lpstrElementName, lpParms->lpstrElementName );
    }
    if (dwParam & MCI_OPEN_ALIAS)
    {
        wmd->lpstrAlias = HeapAlloc(GetProcessHeap(), 0, (strlenW(lpParms->lpstrAlias)+1) * sizeof(WCHAR));
        strcpyW( wmd->lpstrAlias, lpParms->lpstrAlias);
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
	if (strcmpiW(S_MciCmdTable[uTbl].aVerbs[idx], verb) == 0)
	    return S_MciCmdTable[uTbl].aVerbs[idx];
    }

    return NULL;
}

/**************************************************************************
 * 				MCI_GetReturnType		[internal]
 */
static	DWORD		MCI_GetReturnType(LPCWSTR lpCmd)
{
    lpCmd = (LPCWSTR)((const BYTE*)(lpCmd + strlenW(lpCmd) + 1) + sizeof(DWORD) + sizeof(WORD));
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
    return (WORD)*(const DWORD*)(lpCmd + strlenW(lpCmd) + 1);
}

/**************************************************************************
 * 				MCI_GetDWord			[internal]
 */
static	BOOL		MCI_GetDWord(DWORD_PTR *data, LPWSTR* ptr)
{
    DWORD	val;
    LPWSTR	ret;

    val = strtoulW(*ptr, &ret, 0);

    switch (*ret) {
    case '\0':	break;
    case ' ':	ret++; break;
    default:	return FALSE;
    }

    *data |= val;
    *ptr = ret;
    return TRUE;
}

/**************************************************************************
 * 				MCI_GetString		[internal]
 */
static	DWORD	MCI_GetString(LPWSTR* str, LPWSTR* args)
{
    LPWSTR      ptr = *args;

    /* see if we have a quoted string */
    if (*ptr == '"') {
	ptr = strchrW(*str = ptr + 1, '"');
	if (!ptr) return MCIERR_NO_CLOSING_QUOTE;
	/* FIXME: shall we escape \" from string ?? */
	if (ptr[-1] == '\\') TRACE("Ooops: un-escaped \"\n");
	*ptr++ = '\0'; /* remove trailing " */
	if (*ptr != ' ' && *ptr != '\0') return MCIERR_EXTRA_CHARACTERS;
    } else {
	ptr = strchrW(ptr, ' ');

	if (ptr) {
	    *ptr++ = '\0';
	} else {
	    ptr = *args + strlenW(*args);
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
static	DWORD	MCI_ParseOptArgs(DWORD_PTR *data, int _offset, LPCWSTR lpCmd,
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
	TRACE("args=%s offset=%d\n", debugstr_w(args), offset);

	do { /* loop on options for command table for the requested verb */
	    str = (LPCWSTR)lmem;
	    lmem += ((len = strlenW(str)) + 1) * sizeof(WCHAR);
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
	    }

	    if (strncmpiW(args, str, len) == 0 &&
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
		    break;
		case MCI_INTEGER:
		    if (inCst) {
			data[offset] |= flg;
			*dwFlags |= cflg;
			inCst = FALSE;
		    } else {
			*dwFlags |= flg;
			if (!MCI_GetDWord(&(data[offset]), &args)) {
			    return MCIERR_BAD_INTEGER;
			}
		    }
		    break;
		case MCI_RECT:
		    /* store rect in data (offset..offset+3) */
		    *dwFlags |= flg;
		    if (!MCI_GetDWord(&(data[offset+0]), &args) ||
			!MCI_GetDWord(&(data[offset+1]), &args) ||
			!MCI_GetDWord(&(data[offset+2]), &args) ||
			!MCI_GetDWord(&(data[offset+3]), &args)) {
			ERR("Bad rect %s\n", debugstr_w(args));
			return MCIERR_BAD_INTEGER;
		    }
		    break;
		case MCI_STRING:
		    *dwFlags |= flg;
		    if ((dwRet = MCI_GetString((LPWSTR*)&data[offset], &args)))
			return dwRet;
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
		case MCI_INTEGER:		if (!inCst) offset++;	break;
		case MCI_END_CONSTANT:
		case MCI_STRING:		offset++; break;
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
	    ERR("Internal data[] buffer overflow\n");
	    return MCIERR_PARSER_INTERNAL;
	}
    }
    return 0;
}

/**************************************************************************
 * 				MCI_HandleReturnValues	[internal]
 */
static	DWORD	MCI_HandleReturnValues(DWORD dwRet, LPWINE_MCIDRIVER wmd, DWORD retType, 
                                       DWORD_PTR *data, LPWSTR lpstrRet, UINT uRetLen)
{
    static const WCHAR wszLd  [] = {'%','l','d',0};
    static const WCHAR wszLd4 [] = {'%','l','d',' ','%','l','d',' ','%','l','d',' ','%','l','d',0};
    static const WCHAR wszCol3[] = {'%','d',':','%','d',':','%','d',0};
    static const WCHAR wszCol4[] = {'%','d',':','%','d',':','%','d',':','%','d',0};

    if (lpstrRet) {
	switch (retType) {
	case 0: /* nothing to return */
	    break;
	case MCI_INTEGER:
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
	    case MCI_INTEGER_RETURNED:
		snprintfW(lpstrRet, uRetLen, wszLd, data[1]);
		break;
	    case MCI_RESOURCE_RETURNED:
		/* return string which ID is HIWORD(data[1]),
		 * string is loaded from mmsystem.dll */
		LoadStringW(hWinMM32Instance, HIWORD(data[1]), lpstrRet, uRetLen);
		break;
	    case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
		/* return string which ID is HIWORD(data[1]),
		 * string is loaded from driver */
		/* FIXME: this is wrong for a 16 bit handle */
		LoadStringW(GetDriverModuleHandle(wmd->hDriver),
			    HIWORD(data[1]), lpstrRet, uRetLen);
		break;
	    case MCI_COLONIZED3_RETURN:
		snprintfW(lpstrRet, uRetLen, wszCol3,
			  LOBYTE(LOWORD(data[1])), HIBYTE(LOWORD(data[1])),
			  LOBYTE(HIWORD(data[1])));
		break;
	    case MCI_COLONIZED4_RETURN:
		snprintfW(lpstrRet, uRetLen, wszCol4,
			  LOBYTE(LOWORD(data[1])), HIBYTE(LOWORD(data[1])),
			  LOBYTE(HIWORD(data[1])), HIBYTE(HIWORD(data[1])));
		break;
	    default:	ERR("Ooops (%04X)\n", HIWORD(dwRet));
	    }
	    break;
	case MCI_STRING:
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
		/* nothing to do data[1] == lpstrRet */
		break;
	    case MCI_INTEGER_RETURNED:
		data[1] = *(LPDWORD)lpstrRet;
		snprintfW(lpstrRet, uRetLen, wszLd, data[1]);
		break;
	    default:
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
		break;
	    }
	    break;
	case MCI_RECT:
	    if (dwRet & 0xFFFF0000ul)
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
	    snprintfW(lpstrRet, uRetLen, wszLd4,
                      data[1], data[2], data[3], data[4]);
	    break;
	default:		ERR("oops\n");
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
    LPWSTR		verb, dev, args;
    LPWINE_MCIDRIVER	wmd = 0;
    DWORD		dwFlags = 0, dwRet = 0;
    int			offset = 0;
    DWORD_PTR	data[MCI_DATA_SIZE];
    DWORD		retType;
    LPCWSTR		lpCmd = 0;
    LPWSTR		devAlias = NULL;
    static const WCHAR  wszNew[] = {'n','e','w',0};
    static const WCHAR  wszSAliasS[] = {' ','a','l','i','a','s',' ',0};
    static const WCHAR wszTypeS[] = {'t','y','p','e',' ',0};

    TRACE("(%s, %p, %d, %p)\n", 
          debugstr_w(lpstrCommand), lpstrRet, uRetLen, hwndCallback);

    /* format is <command> <device> <optargs> */
    if (!(verb = HeapAlloc(GetProcessHeap(), 0, (strlenW(lpstrCommand)+1) * sizeof(WCHAR))))
	return MCIERR_OUT_OF_MEMORY;
    strcpyW( verb, lpstrCommand );
    CharLowerW(verb);

    memset(data, 0, sizeof(data));

    if (!(args = strchrW(verb, ' '))) {
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }
    *args++ = '\0';
    if ((dwRet = MCI_GetString(&dev, &args))) {
	goto errCleanUp;
    }

    /* Determine devType from open */
    if (!strcmpW(verb, wszOpen)) {
	LPWSTR	devType, tmp;
        WCHAR	buf[128];

	/* case dev == 'new' has to be handled */
	if (!strcmpW(dev, wszNew)) {
	    dev = 0;
	    if ((devType = strstrW(args, wszTypeS)) != NULL) {
		devType += 5;
		tmp = strchrW(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = str_dup_upper(devType);
		if (tmp) *tmp = ' ';
		/* dwFlags and data[2] will be correctly set in ParseOpt loop */
	    } else {
		WARN("open new requires device type\n");
		dwRet = MCIERR_MISSING_DEVICE_NAME;
		goto errCleanUp;
	    }
	} else if ((devType = strchrW(dev, '!')) != NULL) {
	    *devType++ = '\0';
	    tmp = devType; devType = dev; dev = tmp;

	    dwFlags |= MCI_OPEN_TYPE;
	    data[2] = (DWORD_PTR)devType;
	    devType = str_dup_upper(devType);
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data[3] = (DWORD_PTR)dev;
	} else if (DRIVER_GetLibName(dev, wszMci, buf, sizeof(buf))) {
            /* this is the name of a mci driver's type */
	    tmp = strchrW(dev, ' ');
	    if (tmp) *tmp = '\0';
	    data[2] = (DWORD_PTR)dev;
	    devType = str_dup_upper(dev);
	    if (tmp) *tmp = ' ';
	    dwFlags |= MCI_OPEN_TYPE;
	} else {
	    if ((devType = strstrW(args, wszTypeS)) != NULL) {
		devType += 5;
		tmp = strchrW(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = str_dup_upper(devType);
		if (tmp) *tmp = ' ';
		/* dwFlags and data[2] will be correctly set in ParseOpt loop */
	    } else {
		if ((dwRet = MCI_GetDevTypeFromFileName(dev, buf, sizeof(buf))))
		    goto errCleanUp;

		devType = str_dup_upper(buf);
	    }
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data[3] = (DWORD_PTR)dev;
	}
	if ((devAlias = strstrW(args, wszSAliasS))) {
            WCHAR*      tmp2;
	    devAlias += 7;
	    if (!(tmp = strchrW(devAlias,' '))) tmp = devAlias + strlenW(devAlias);
	    if (tmp) *tmp = '\0';
            tmp2 = HeapAlloc(GetProcessHeap(), 0, (tmp - devAlias + 1) * sizeof(WCHAR) );
            memcpy( tmp2, devAlias, (tmp - devAlias) * sizeof(WCHAR) );
            tmp2[tmp - devAlias] = 0;
            data[4] = (DWORD_PTR)tmp2;
	    /* should be done in regular options parsing */
	    /* dwFlags |= MCI_OPEN_ALIAS; */
	} else if (dev == 0) {
	    /* "open new" requires alias */
	    dwRet = MCIERR_NEW_REQUIRES_ALIAS;
	    goto errCleanUp;
	}

	dwRet = MCI_LoadMciDriver(devType, &wmd);
	if (dwRet == MCIERR_DEVICE_NOT_INSTALLED)
	    dwRet = MCIERR_INVALID_DEVICE_NAME;
	HeapFree(GetProcessHeap(), 0, devType);
	if (dwRet) {
	    MCI_UnLoadMciDriver(wmd);
	    goto errCleanUp;
	}
    } else if (!(wmd = MCI_GetDriver(mciGetDeviceIDW(dev)))) {
	/* auto open */
        static const WCHAR wszOpenWait[] = {'o','p','e','n',' ','%','s',' ','w','a','i','t',0};
	WCHAR   buf[128];
	sprintfW(buf, wszOpenWait, dev);

	if ((dwRet = mciSendStringW(buf, NULL, 0, 0)) != 0)
	    goto errCleanUp;

	wmd = MCI_GetDriver(mciGetDeviceIDW(dev));
	if (!wmd) {
	    /* FIXME: memory leak, MCI driver is not closed */
	    dwRet = MCIERR_INVALID_DEVICE_ID;
	    goto errCleanUp;
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

    /* set return information */
    switch (retType = MCI_GetReturnType(lpCmd)) {
    case 0:		offset = 1;	break;
    case MCI_INTEGER:	offset = 2;	break;
    case MCI_STRING:	data[1] = (DWORD_PTR)lpstrRet; data[2] = uRetLen; offset = 3; break;
    case MCI_RECT:	offset = 5;	break;
    default:	ERR("oops\n");
    }

    TRACE("verb=%s on dev=%s; offset=%d\n", 
          debugstr_w(verb), debugstr_w(dev), offset);

    if ((dwRet = MCI_ParseOptArgs(data, offset, lpCmd, args, &dwFlags)))
	goto errCleanUp;

    /* set up call back */
    if (dwFlags & MCI_NOTIFY) {
	data[0] = (DWORD_PTR)hwndCallback;
    }

    /* FIXME: the command should get it's own notification window set up and
     * ask for device closing while processing the notification mechanism
     */
    if (lpstrRet && uRetLen) *lpstrRet = '\0';

    TRACE("[%d, %s, %08x, %08x/%s %08x/%s %08x/%s %08x/%s %08x/%s %08x/%s]\n",
	  wmd->wDeviceID, MCI_MessageToString(MCI_GetMessage(lpCmd)), dwFlags,
	  data[0], debugstr_w((WCHAR *)data[0]), data[1], debugstr_w((WCHAR *)data[1]),
	  data[2], debugstr_w((WCHAR *)data[2]), data[3], debugstr_w((WCHAR *)data[3]),
	  data[4], debugstr_w((WCHAR *)data[4]), data[5], debugstr_w((WCHAR *)data[5]));

    if (strcmpW(verb, wszOpen) == 0) {
	if ((dwRet = MCI_FinishOpen(wmd, (LPMCI_OPEN_PARMSW)data, dwFlags)))
	    MCI_UnLoadMciDriver(wmd);
	/* FIXME: notification is not properly shared across two opens */
    } else {
	dwRet = MCI_SendCommand(wmd->wDeviceID, MCI_GetMessage(lpCmd), dwFlags, (DWORD_PTR)data);
    }
    TRACE("=> 1/ %x (%s)\n", dwRet, debugstr_w(lpstrRet));
    dwRet = MCI_HandleReturnValues(dwRet, wmd, retType, data, lpstrRet, uRetLen);
    TRACE("=> 2/ %x (%s)\n", dwRet, debugstr_w(lpstrRet));

errCleanUp:
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
        lpwstrRet = HeapAlloc(GetProcessHeap(), 0, uRetLen * sizeof(WCHAR));
        if (!lpwstrRet) {
            WARN("no memory\n");
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
	    sprintf(strRet, "Unknown MCI error (%lu)", ret);
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

    TRACE("(%08X, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    /* only two low bytes are generic, the other ones are dev type specific */
#define WINE_MCIDRIVER_SUPP	(0xFFFF0000|MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT| \
                         MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID| \
                         MCI_NOTIFY|MCI_WAIT)
    if ((dwParam & ~WINE_MCIDRIVER_SUPP) != 0) {
	FIXME("Unsupported yet dwFlags=%08lX\n", dwParam & ~WINE_MCIDRIVER_SUPP);
    }
#undef WINE_MCIDRIVER_SUPP

    strDevTyp[0] = 0;

    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
	    WORD uDevType = LOWORD(lpParms->lpstrDeviceType);

	    if (uDevType < MCI_DEVTYPE_FIRST ||
		uDevType > MCI_DEVTYPE_LAST ||
		!LoadStringW(hWinMM32Instance, uDevType,
                             strDevTyp, sizeof(strDevTyp) / sizeof(WCHAR))) {
		dwRet = MCIERR_BAD_INTEGER;
		goto errCleanUp;
	    }
	} else {
	    LPWSTR	ptr;
	    if (lpParms->lpstrDeviceType == NULL) {
		dwRet = MCIERR_NULL_PARAMETER_BLOCK;
		goto errCleanUp;
	    }
	    strcpyW(strDevTyp, lpParms->lpstrDeviceType);
	    ptr = strchrW(strDevTyp, '!');
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
            static const WCHAR wszCdAudio[] = {'C','D','A','U','D','I','O',0};
	    if (GetDriveTypeW(lpParms->lpstrElementName) != DRIVE_CDROM) {
		dwRet = MCIERR_EXTENSION_NOT_FOUND;
		goto errCleanUp;
	    }
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    strcpyW(strDevTyp, wszCdAudio);
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
	TRACE("Failed to open driver (MCI_OPEN_DRIVER) [%08x], closing\n", dwRet);
	/* FIXME: is dwRet the correct ret code ? */
	goto errCleanUp;
    }

    /* only handled devices fall through */
    TRACE("wDevID=%04X wDeviceID=%d dwRet=%d\n", wmd->wDeviceID, lpParms->wDeviceID, dwRet);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify((HWND)lpParms->dwCallback, wmd->wDeviceID, MCI_NOTIFY_SUCCESSFUL);

    return 0;
errCleanUp:
    if (wmd) MCI_UnLoadMciDriver(wmd);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify((HWND)lpParms->dwCallback, 0, MCI_NOTIFY_FAILURE);
    return dwRet;
}

/**************************************************************************
 * 			MCI_Close				[internal]
 */
static	DWORD MCI_Close(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwParam, lpParms);

    /* Every device must handle MCI_NOTIFY on its own. */
    if ((UINT16)wDevID == (UINT16)MCI_ALL_DEVICE_ID) {
	/* FIXME: shall I notify once after all is done, or for
	 * each of the open drivers ? if the latest, which notif
	 * to return when only one fails ?
	 */
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

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD_PTR)lpParms);

    MCI_UnLoadMciDriver(wmd);

    if (dwParam & MCI_NOTIFY)
        mciDriverNotify(lpParms ? (HWND)lpParms->dwCallback : 0,
                        wDevID,
                        dwRet ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);

    return dwRet;
}

/**************************************************************************
 * 			MCI_WriteString				[internal]
 */
static DWORD MCI_WriteString(LPWSTR lpDstStr, DWORD dstSize, LPCWSTR lpSrcStr)
{
    DWORD	ret = 0;

    if (lpSrcStr) {
        dstSize /= sizeof(WCHAR);
	if (dstSize <= strlenW(lpSrcStr)) {
	    lstrcpynW(lpDstStr, lpSrcStr, dstSize - 1);
	    ret = MCIERR_PARAM_OVERFLOW;
	} else {
	    strcpyW(lpDstStr, lpSrcStr);
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
    WCHAR		buf[2048], *s = buf, *p;
    LPWINE_MCIDRIVER	wmd;
    HKEY		hKey;

    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
        return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("(%08x, %08X, %p[num=%d, wDevTyp=%u])\n",
	  uDevID, dwFlags, lpParms, lpParms->dwNumber, lpParms->wDeviceType);

    switch (dwFlags & ~MCI_SYSINFO_OPEN) {
    case MCI_SYSINFO_QUANTITY:
	if (lpParms->wDeviceType < MCI_DEVTYPE_FIRST || lpParms->wDeviceType > MCI_DEVTYPE_LAST) {
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
		if (GetPrivateProfileStringW(wszMci, 0, wszNull, buf, sizeof(buf) / sizeof(buf[0]), wszSystemIni))
		    for (s = buf; *s; s += strlenW(s) + 1) cnt++;
	    }
	} else {
	    if (dwFlags & MCI_SYSINFO_OPEN) {
		TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %u\n", lpParms->wDeviceType);
		EnterCriticalSection(&WINMM_cs);
		for (wmd = MciDrivers; wmd; wmd = wmd->lpNext) {
		    if (wmd->wType == lpParms->wDeviceType) cnt++;
		}
		LeaveCriticalSection(&WINMM_cs);
	    } else {
		TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %u\n", lpParms->wDeviceType);
		FIXME("Don't know how to get # of MCI devices of a given type\n");
		cnt = 1;
	    }
	}
	*(DWORD*)lpParms->lpstrReturn = cnt;
	TRACE("(%d) => '%d'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = MCI_INTEGER_RETURNED;
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE("MCI_SYSINFO_INSTALLNAME\n");
	if ((wmd = MCI_GetDriver(uDevID))) {
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize,
				  wmd->lpstrDeviceType);
	} else {
	    *lpParms->lpstrReturn = 0;
	    ret = MCIERR_INVALID_DEVICE_ID;
	}
	TRACE("(%d) => %s\n", lpParms->dwNumber, debugstr_w(lpParms->lpstrReturn));
	break;
    case MCI_SYSINFO_NAME:
	TRACE("MCI_SYSINFO_NAME\n");
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    FIXME("Don't handle MCI_SYSINFO_NAME|MCI_SYSINFO_OPEN (yet)\n");
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	} else {
	    s = NULL;
	    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, wszHklmMci, 0, 
                               KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS) {
		if (RegQueryInfoKeyW( hKey, 0, 0, 0, &cnt, 
                                      0, 0, 0, 0, 0, 0, 0) == ERROR_SUCCESS && 
                    lpParms->dwNumber <= cnt) {
		    DWORD bufLen = sizeof(buf)/sizeof(buf[0]);
		    if (RegEnumKeyExW(hKey, lpParms->dwNumber - 1, 
                                      buf, &bufLen, 0, 0, 0, 0) == ERROR_SUCCESS)
                        s = buf;
		}
	        RegCloseKey( hKey );
	    }
	    if (!s) {
		if (GetPrivateProfileStringW(wszMci, 0, wszNull, buf, sizeof(buf) / sizeof(buf[0]), wszSystemIni)) {
		    for (p = buf; *p; p += strlenW(p) + 1, cnt++) {
                        TRACE("%d: %s\n", cnt, debugstr_w(p));
			if (cnt == lpParms->dwNumber - 1) {
			    s = p;
			    break;
			}
		    }
		}
	    }
	    ret = s ? MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize / sizeof(WCHAR), s) : MCIERR_OUTOFRANGE;
	}
	TRACE("(%d) => %s\n", lpParms->dwNumber, debugstr_w(lpParms->lpstrReturn));
	break;
    default:
	TRACE("Unsupported flag value=%08x\n", dwFlags);
	ret = MCIERR_UNRECOGNIZED_COMMAND;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Break				[internal]
 */
static	DWORD MCI_Break(UINT wDevID, DWORD dwFlags, LPMCI_BREAK_PARMS lpParms)
{
    DWORD	dwRet = 0;

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_NOTIFY)
	mciDriverNotify((HWND)lpParms->dwCallback, wDevID,
                        (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);

    return dwRet;
}

/**************************************************************************
 * 			MCI_Sound				[internal]
 */
static	DWORD MCI_Sound(UINT wDevID, DWORD dwFlags, LPMCI_SOUND_PARMSW lpParms)
{
    DWORD	dwRet = 0;

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_SOUND_NAME)
        dwRet = sndPlaySoundW(lpParms->lpstrSoundName, SND_SYNC) ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
    else
        dwRet = MMSYSERR_ERROR; /* what should be done ??? */
    if (dwFlags & MCI_NOTIFY)
	mciDriverNotify((HWND)lpParms->dwCallback, wDevID,
                        (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);

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
		TRACE("Changing %08x to %08x\n", lmgp->dwReturn, LOWORD(lmgp->dwReturn));
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
		TRACE("Changing %08lx to %08x\n", lsp->dwReturn, LOWORD(lsp->dwReturn));
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
	    FIXME("Got non null hiword for dwRet=0x%08lx for command %s\n",
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

    TRACE("(%04x, %08lx)\n", uDeviceID, data);

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

    TRACE("(%08x, %s, %08lx, %08lx)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2);
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2);
    TRACE("=> %08x\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciSendCommandA			[WINMM.@]
 */
DWORD WINAPI mciSendCommandA(MCIDEVICEID wDevID, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD ret;
    int mapped;

    TRACE("(%08x, %s, %08lx, %08lx)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    mapped = MCI_MapMsgAtoW(wMsg, dwParam1, &dwParam2);
    if (mapped == -1)
    {
        FIXME("message %04x mapping failed\n", wMsg);
        return MMSYSERR_NOMEM;
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

/******************************************************************
 *		MyUserYield
 *
 * Internal wrapper to call USER.UserYield16 (in fact through a Wine only export from USER32).
 */
static void MyUserYield(void)
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        FARPROC proc = GetProcAddress( mod, "UserYield16" );
        if (proc) proc();
    }
}

/**************************************************************************
 * 				MCI_DefYieldProc	       	[internal]
 */
static UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data)
{
    INT16	ret;

    TRACE("(0x%04x, 0x%08x)\n", wDevID, data);

    if ((HIWORD(data) != 0 && HWND_16(GetActiveWindow()) != HIWORD(data)) ||
	(GetAsyncKeyState(LOWORD(data)) & 1) == 0) {
	MyUserYield();
	ret = 0;
    } else {
	MSG		msg;

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

    TRACE("(%u, %p, %08x)\n", uDeviceID, fpYieldProc, dwYieldData);

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
    FIXME("(%u, %s) stub\n", dwElementID, debugstr_w(lpstrType));
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

    if ((wmd = MCI_GetDriver(uDeviceID))) ret = (HTASK)wmd->CreatorThread;

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
	MyUserYield();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}
