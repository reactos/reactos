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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "mmsystem.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"

#include "digitalv.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mci);

static	int			MCI_InstalledCount;
static  LPSTR                   MCI_lpInstallNames /* = NULL */;

WINMM_MapType  (*pFnMciMapMsg16To32A)  (WORD,WORD,DWORD*) /* = NULL */;
WINMM_MapType  (*pFnMciUnMapMsg16To32A)(WORD,WORD,DWORD) /* = NULL */;
WINMM_MapType  (*pFnMciMapMsg32ATo16)  (WORD,WORD,DWORD,DWORD*) /* = NULL */;
WINMM_MapType  (*pFnMciUnMapMsg32ATo16)(WORD,WORD,DWORD,DWORD) /* = NULL */;

/* First MCI valid device ID (0 means error) */
#define MCI_MAGIC 0x0001

/* dup a string and uppercase it */
inline static LPSTR str_dup_upper( LPCSTR str )
{
    INT len = strlen(str) + 1;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, len );
    if (p)
    {
        memcpy( p, str, len );
        CharUpperA( p );
    }
    return p;
}

/**************************************************************************
 * 				MCI_GetDriver			[internal]
 */
LPWINE_MCIDRIVER	MCI_GetDriver(UINT16 wDevID)
{
    LPWINE_MCIDRIVER	wmd = 0;

    EnterCriticalSection(&WINMM_IData->cs);
    for (wmd = WINMM_IData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
	if (wmd->wDeviceID == wDevID)
	    break;
    }
    LeaveCriticalSection(&WINMM_IData->cs);
    return wmd;
}

/**************************************************************************
 * 				MCI_GetDriverFromString		[internal]
 */
UINT	MCI_GetDriverFromString(LPCSTR lpstrName)
{
    LPWINE_MCIDRIVER	wmd;
    UINT		ret = 0;

    if (!lpstrName)
	return 0;

    if (!lstrcmpiA(lpstrName, "ALL"))
	return MCI_ALL_DEVICE_ID;

    EnterCriticalSection(&WINMM_IData->cs);
    for (wmd = WINMM_IData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
	if (wmd->lpstrElementName && strcmp(wmd->lpstrElementName, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	if (wmd->lpstrDeviceType && strcasecmp(wmd->lpstrDeviceType, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	if (wmd->lpstrAlias && strcasecmp(wmd->lpstrAlias, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_IData->cs);

    return ret;
}

/**************************************************************************
 * 			MCI_MessageToString			[internal]
 */
const char* MCI_MessageToString(UINT16 wMsg)
{
    static char buffer[100];

#define CASE(s) case (s): return #s

    switch (wMsg) {
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

/**************************************************************************
 * 				MCI_GetDevTypeFromFileName	[internal]
 */
static	DWORD	MCI_GetDevTypeFromFileName(LPCSTR fileName, LPSTR buf, UINT len)
{
    LPSTR	tmp;
    HKEY	hKey;

    if ((tmp = strrchr(fileName, '.'))) {
	if (RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MCI Extensions",
			   0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS) {
	    DWORD dwLen = len;
	    LONG lRet = RegQueryValueExA( hKey, tmp + 1, 0, 0, buf, &dwLen ); 
	    RegCloseKey( hKey );
	    if (lRet == ERROR_SUCCESS) return 0;
        }
	TRACE("No ...\\MCI Extensions entry for '%s' found.\n", tmp);
    }
    return MCIERR_EXTENSION_NOT_FOUND;
}

#define	MAX_MCICMDTABLE			20
#define MCI_COMMAND_TABLE_NOT_LOADED	0xFFFE

typedef struct tagWINE_MCICMDTABLE {
    UINT		uDevType;
    LPCSTR		lpTable;
    UINT		nVerbs;		/* number of verbs in command table */
    LPCSTR*		aVerbs;		/* array of verbs to speed up the verb look up process */
} WINE_MCICMDTABLE, *LPWINE_MCICMDTABLE;

static WINE_MCICMDTABLE S_MciCmdTable[MAX_MCICMDTABLE];

/**************************************************************************
 * 				MCI_IsCommandTableValid		[internal]
 */
static	BOOL		MCI_IsCommandTableValid(UINT uTbl)
{
    LPCSTR	lmem, str;
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
	do {
	    str = lmem;
	    lmem += strlen(lmem) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    lmem += sizeof(DWORD) + sizeof(WORD);
	    idx ++;
	    /* EPP 	    TRACE("cmd='%s' %08lx %04x\n", str, flg, eid); */
	    switch (eid) {
	    case MCI_COMMAND_HEAD:	if (!*str || !flg) return FALSE; idx = 0;			break;	/* check unicity of str in table */
	    case MCI_STRING:            if (inCst) return FALSE;					break;
	    case MCI_INTEGER:           if (!*str) return FALSE;					break;
	    case MCI_END_COMMAND:       if (*str || flg || idx == 0) return FALSE; idx = 0;		break;
	    case MCI_RETURN:		if (*str || idx != 1) return FALSE;				break;
	    case MCI_FLAG:		if (!*str) return FALSE;					break;
	    case MCI_END_COMMAND_LIST:	if (*str || flg) return FALSE;	idx = 0;			break;
	    case MCI_RECT:		if (!*str || inCst) return FALSE;				break;
	    case MCI_CONSTANT:          if (inCst) return FALSE; inCst = TRUE;				break;
	    case MCI_END_CONSTANT:	if (*str || flg || !inCst) return FALSE; inCst = FALSE;		break;
	    default:			return FALSE;
	    }
	} while (eid != MCI_END_COMMAND_LIST);
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}

/**************************************************************************
 * 				MCI_DumpCommandTable		[internal]
 */
static	BOOL		MCI_DumpCommandTable(UINT uTbl)
{
    LPCSTR	lmem;
    LPCSTR	str;
    DWORD	flg;
    WORD	eid;

    if (!MCI_IsCommandTableValid(uTbl)) {
	ERR("Ooops: %d is not valid\n", uTbl);
	return FALSE;
    }

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
	do {
	    str = lmem;
	    lmem += strlen(lmem) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    TRACE("cmd='%s' %08lx %04x\n", str, flg, eid);
	    lmem += sizeof(DWORD) + sizeof(WORD);
	} while (eid != MCI_END_COMMAND && eid != MCI_END_COMMAND_LIST);
	TRACE(" => end of command%s\n", (eid == MCI_END_COMMAND_LIST) ? " list" : "");
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}


/**************************************************************************
 * 				MCI_GetCommandTable		[internal]
 */
static	UINT		MCI_GetCommandTable(UINT uDevType)
{
    UINT	uTbl;
    char	buf[32];
    LPCSTR	str = NULL;

    /* first look up existing for existing devType */
    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (S_MciCmdTable[uTbl].lpTable && S_MciCmdTable[uTbl].uDevType == uDevType)
	    return uTbl;
    }

    /* well try to load id */
    if (uDevType >= MCI_DEVTYPE_FIRST && uDevType <= MCI_DEVTYPE_LAST) {
	if (LoadStringA(WINMM_IData->hWinMM32Instance, uDevType, buf, sizeof(buf))) {
	    str = buf;
	}
    } else if (uDevType == 0) {
	str = "CORE";
    }
    uTbl = MCI_NO_COMMAND_TABLE;
    if (str) {
	HRSRC 	hRsrc = FindResourceA(WINMM_IData->hWinMM32Instance, str, (LPCSTR)RT_RCDATA);
	HANDLE	hMem = 0;

	if (hRsrc) hMem = LoadResource(WINMM_IData->hWinMM32Instance, hRsrc);
	if (hMem) {
	    uTbl = MCI_SetCommandTable(LockResource(hMem), uDevType);
	} else {
	    WARN("No command table found in resource %p[%s]\n",
		 WINMM_IData->hWinMM32Instance, str);
	}
    }
    TRACE("=> %d\n", uTbl);
    return uTbl;
}

/**************************************************************************
 * 				MCI_SetCommandTable		[internal]
 */
UINT MCI_SetCommandTable(void *table, UINT uDevType)
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

    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (!S_MciCmdTable[uTbl].lpTable) {
	    LPCSTR 	lmem, str;
	    WORD	eid;
	    WORD	count;

	    S_MciCmdTable[uTbl].uDevType = uDevType;
	    S_MciCmdTable[uTbl].lpTable = table;

	    if (TRACE_ON(mci)) {
		MCI_DumpCommandTable(uTbl);
	    }

	    /* create the verbs table */
	    /* get # of entries */
	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		lmem += strlen(lmem) + 1;
		eid = *(LPWORD)(lmem + sizeof(DWORD));
		lmem += sizeof(DWORD) + sizeof(WORD);
		if (eid == MCI_COMMAND_HEAD)
		    count++;
	    } while (eid != MCI_END_COMMAND_LIST);

	    S_MciCmdTable[uTbl].aVerbs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(LPCSTR));
	    S_MciCmdTable[uTbl].nVerbs = count;

	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		str = lmem;
		lmem += strlen(lmem) + 1;
		eid = *(LPWORD)(lmem + sizeof(DWORD));
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
 * 				MCI_DeleteCommandTable		[internal]
 */
static	BOOL	MCI_DeleteCommandTable(UINT uTbl)
{
    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].lpTable)
	return FALSE;

    S_MciCmdTable[uTbl].lpTable = NULL;
    if (S_MciCmdTable[uTbl].aVerbs) {
	HeapFree(GetProcessHeap(), 0, S_MciCmdTable[uTbl].aVerbs);
	S_MciCmdTable[uTbl].aVerbs = 0;
    }
    return TRUE;
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

    EnterCriticalSection(&WINMM_IData->cs);
    for (tmp = &WINMM_IData->lpMciDrvs; *tmp; tmp = &(*tmp)->lpNext) {
	if (*tmp == wmd) {
	    *tmp = wmd->lpNext;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_IData->cs);

    HeapFree(GetProcessHeap(), 0, wmd->lpstrDeviceType);
    HeapFree(GetProcessHeap(), 0, wmd->lpstrAlias);
    HeapFree(GetProcessHeap(), 0, wmd->lpstrElementName);

    HeapFree(GetProcessHeap(), 0, wmd);
    return TRUE;
}

/**************************************************************************
 * 				MCI_OpenMciDriver		[internal]
 */
static	BOOL	MCI_OpenMciDriver(LPWINE_MCIDRIVER wmd, LPCSTR drvTyp, LPARAM lp)
{
    char	libName[128];

    if (!DRIVER_GetLibName(drvTyp, "mci", libName, sizeof(libName)))
	return FALSE;

    wmd->bIs32 = 0xFFFF;
    /* First load driver */
    if ((wmd->hDriver = (HDRVR)DRIVER_TryOpenDriver32(libName, lp))) {
	wmd->bIs32 = TRUE;
    } else if (WINMM_CheckForMMSystem() && pFnMciMapMsg32ATo16) {
	WINMM_MapType 	res;

	switch (res = pFnMciMapMsg32ATo16(0, DRV_OPEN, 0, &lp)) {
	case WINMM_MAP_MSGERROR:
	    TRACE("Not handled yet (DRV_OPEN)\n");
	    break;
	case WINMM_MAP_NOMEM:
	    TRACE("Problem mapping msg=DRV_OPEN from 32a to 16\n");
	    break;
	case WINMM_MAP_OK:
	case WINMM_MAP_OKMEM:
	    if ((wmd->hDriver = OpenDriverA(drvTyp, "mci", lp)))
		wmd->bIs32 = FALSE;
	    if (res == WINMM_MAP_OKMEM)
		pFnMciUnMapMsg32ATo16(0, DRV_OPEN, 0, lp);
	    break;
	}
    }
    return (wmd->bIs32 == 0xFFFF) ? FALSE : TRUE;
}

/**************************************************************************
 * 				MCI_LoadMciDriver		[internal]
 */
static	DWORD	MCI_LoadMciDriver(LPCSTR _strDevTyp, LPWINE_MCIDRIVER* lpwmd)
{
    LPSTR			strDevTyp = str_dup_upper(_strDevTyp);
    LPWINE_MCIDRIVER		wmd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wmd));
    MCI_OPEN_DRIVER_PARMSA	modp;
    DWORD			dwRet = 0;

    if (!wmd || !strDevTyp) {
	dwRet = MCIERR_OUT_OF_MEMORY;
	goto errCleanUp;
    }

    wmd->lpfnYieldProc = MCI_DefYieldProc;
    wmd->dwYieldData = VK_CANCEL;
    wmd->CreatorThread = GetCurrentThreadId();

    EnterCriticalSection(&WINMM_IData->cs);
    /* wmd must be inserted in list before sending opening the driver, coz' it
     * may want to lookup at wDevID
     */
    wmd->lpNext = WINMM_IData->lpMciDrvs;
    WINMM_IData->lpMciDrvs = wmd;

    for (modp.wDeviceID = MCI_MAGIC;
	 MCI_GetDriver(modp.wDeviceID) != 0;
	 modp.wDeviceID++);

    wmd->wDeviceID = modp.wDeviceID;

    LeaveCriticalSection(&WINMM_IData->cs);

    TRACE("wDevID=%04X \n", modp.wDeviceID);

    modp.lpstrParams = NULL;

    if (!MCI_OpenMciDriver(wmd, strDevTyp, (LPARAM)&modp)) {
	/* silence warning if all is used... some bogus program use commands like
	 * 'open all'...
	 */
	if (strcasecmp(strDevTyp, "all") == 0) {
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    FIXME("Couldn't load driver for type %s.\n"
		  "If you don't have a windows installation accessible from Wine,\n"
		  "you perhaps forgot to create a [mci] section in system.ini\n",
		  strDevTyp);
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
	  wmd->hDriver, strDevTyp, modp.wType, modp.wCustomCommandTable);

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
 * 			MCI_FinishOpen				[internal]
 */
static	DWORD	MCI_FinishOpen(LPWINE_MCIDRIVER wmd, LPMCI_OPEN_PARMSA lpParms,
			       DWORD dwParam)
{
    if (dwParam & MCI_OPEN_ELEMENT)
    {
        wmd->lpstrElementName = HeapAlloc(GetProcessHeap(),0,strlen(lpParms->lpstrElementName)+1);
        strcpy( wmd->lpstrElementName, lpParms->lpstrElementName );
    }
    if (dwParam & MCI_OPEN_ALIAS)
    {
        wmd->lpstrAlias = HeapAlloc(GetProcessHeap(), 0, strlen(lpParms->lpstrAlias)+1);
        strcpy( wmd->lpstrAlias, lpParms->lpstrAlias);
    }
    lpParms->wDeviceID = wmd->wDeviceID;

    return MCI_SendCommandFrom32(wmd->wDeviceID, MCI_OPEN_DRIVER, dwParam,
				 (DWORD)lpParms);
}

/**************************************************************************
 * 				MCI_FindCommand		[internal]
 */
static	LPCSTR		MCI_FindCommand(UINT uTbl, LPCSTR verb)
{
    UINT	idx;

    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].lpTable)
	return NULL;

    /* another improvement would be to have the aVerbs array sorted,
     * so that we could use a dichotomic search on it, rather than this dumb
     * array look up
     */
    for (idx = 0; idx < S_MciCmdTable[uTbl].nVerbs; idx++) {
	if (strcasecmp(S_MciCmdTable[uTbl].aVerbs[idx], verb) == 0)
	    return S_MciCmdTable[uTbl].aVerbs[idx];
    }

    return NULL;
}

/**************************************************************************
 * 				MCI_GetReturnType		[internal]
 */
static	DWORD		MCI_GetReturnType(LPCSTR lpCmd)
{
    lpCmd += strlen(lpCmd) + 1 + sizeof(DWORD) + sizeof(WORD);
    if (*lpCmd == '\0' && *(LPWORD)(lpCmd + 1 + sizeof(DWORD)) == MCI_RETURN) {
	return *(LPDWORD)(lpCmd + 1);
    }
    return 0L;
}

/**************************************************************************
 * 				MCI_GetMessage			[internal]
 */
static	WORD		MCI_GetMessage(LPCSTR lpCmd)
{
    return (WORD)*(LPDWORD)(lpCmd + strlen(lpCmd) + 1);
}

/**************************************************************************
 * 				MCI_GetDWord			[internal]
 */
static	BOOL		MCI_GetDWord(LPDWORD data, LPSTR* ptr)
{
    DWORD	val;
    LPSTR	ret;

    val = strtoul(*ptr, &ret, 0);

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
static	DWORD	MCI_GetString(LPSTR* str, LPSTR* args)
{
    LPSTR	ptr = *args;

    /* see if we have a quoted string */
    if (*ptr == '"') {
	ptr = strchr(*str = ptr + 1, '"');
	if (!ptr) return MCIERR_NO_CLOSING_QUOTE;
	/* FIXME: shall we escape \" from string ?? */
	if (ptr[-1] == '\\') TRACE("Ooops: un-escaped \"\n");
	*ptr++ = '\0'; /* remove trailing " */
	if (*ptr != ' ' && *ptr != '\0') return MCIERR_EXTRA_CHARACTERS;
	*ptr++ = '\0';
    } else {
	ptr = strchr(ptr, ' ');

	if (ptr) {
	    *ptr++ = '\0';
	} else {
	    ptr = *args + strlen(*args);
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
static	DWORD	MCI_ParseOptArgs(LPDWORD data, int _offset, LPCSTR lpCmd,
				 LPSTR args, LPDWORD dwFlags)
{
    int		len, offset;
    LPCSTR	lmem, str;
    DWORD	dwRet, flg, cflg = 0;
    WORD	eid;
    BOOL	inCst, found;

    /* loop on arguments */
    while (*args) {
	lmem = lpCmd;
	found = inCst = FALSE;
	offset = _offset;

	/* skip any leading white space(s) */
	while (*args == ' ') args++;
	TRACE("args='%s' offset=%d\n", args, offset);

	do { /* loop on options for command table for the requested verb */
	    str = lmem;
	    lmem += (len = strlen(lmem)) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    lmem += sizeof(DWORD) + sizeof(WORD);
/* EPP 	    TRACE("\tcmd='%s' inCst=%c eid=%04x\n", str, inCst ? 'Y' : 'N', eid); */

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

	    if (strncasecmp(args, str, len) == 0 &&
		(args[len] == 0 || args[len] == ' ')) {
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
		    /* store rect in data (offset...offset+3) */
		    *dwFlags |= flg;
		    if (!MCI_GetDWord(&(data[offset+0]), &args) ||
			!MCI_GetDWord(&(data[offset+1]), &args) ||
			!MCI_GetDWord(&(data[offset+2]), &args) ||
			!MCI_GetDWord(&(data[offset+3]), &args)) {
			ERR("Bad rect '%s'\n", args);
			return MCIERR_BAD_INTEGER;
		    }
		    break;
		case MCI_STRING:
		    *dwFlags |= flg;
		    if ((dwRet = MCI_GetString((LPSTR*)&data[offset], &args)))
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
	    WARN("Optarg '%s' not found\n", args);
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
                                       LPDWORD data, LPSTR lpstrRet, UINT uRetLen)
{
    if (lpstrRet) {
	switch (retType) {
	case 0: /* nothing to return */
	    break;
	case MCI_INTEGER:
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
	    case MCI_INTEGER_RETURNED:
		snprintf(lpstrRet, uRetLen, "%ld", data[1]);
		break;
	    case MCI_RESOURCE_RETURNED:
		/* return string which ID is HIWORD(data[1]),
		 * string is loaded from mmsystem.dll */
		LoadStringA(WINMM_IData->hWinMM32Instance, HIWORD(data[1]),
			    lpstrRet, uRetLen);
		break;
	    case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
		/* return string which ID is HIWORD(data[1]),
		 * string is loaded from driver */
		/* FIXME: this is wrong for a 16 bit handle */
		LoadStringA(GetDriverModuleHandle(wmd->hDriver),
			    HIWORD(data[1]), lpstrRet, uRetLen);
		break;
	    case MCI_COLONIZED3_RETURN:
		snprintf(lpstrRet, uRetLen, "%d:%d:%d",
			 LOBYTE(LOWORD(data[1])), HIBYTE(LOWORD(data[1])),
			 LOBYTE(HIWORD(data[1])));
		break;
	    case MCI_COLONIZED4_RETURN:
		snprintf(lpstrRet, uRetLen, "%d:%d:%d:%d",
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
		snprintf(lpstrRet, uRetLen, "%ld", data[1]);
		break;
	    default:
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
		break;
	    }
	    break;
	case MCI_RECT:
	    if (dwRet & 0xFFFF0000ul)
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
	    snprintf(lpstrRet, uRetLen, "%ld %ld %ld %ld",
		       data[1], data[2], data[3], data[4]);
	    break;
	default:		ERR("oops\n");
	}
    }
    return LOWORD(dwRet);
}

/**************************************************************************
 * 				mciSendStringA		[WINMM.@]
 */
DWORD WINAPI mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrRet,
			    UINT uRetLen, HWND hwndCallback)
{
    LPSTR		verb, dev, args;
    LPWINE_MCIDRIVER	wmd = 0;
    DWORD		dwFlags = 0, dwRet = 0;
    int			offset = 0;
    DWORD		data[MCI_DATA_SIZE];
    DWORD		retType;
    LPCSTR		lpCmd = 0;
    LPSTR		devAlias = NULL;
    BOOL		bAutoOpen = FALSE;

    TRACE("('%s', %p, %d, %p)\n", lpstrCommand, lpstrRet, uRetLen, hwndCallback);

    /* format is <command> <device> <optargs> */
    if (!(verb = HeapAlloc(GetProcessHeap(), 0, strlen(lpstrCommand)+1)))
	return MCIERR_OUT_OF_MEMORY;
    strcpy( verb, lpstrCommand );
    CharLowerA(verb);

    memset(data, 0, sizeof(data));

    if (!(args = strchr(verb, ' '))) {
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }
    *args++ = '\0';
    if ((dwRet = MCI_GetString(&dev, &args))) {
	goto errCleanUp;
    }

    /* case dev == 'new' has to be handled */
    if (!strcmp(dev, "new")) {
	FIXME("'new': NIY as device name\n");
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }

    /* otherwise, try to grab devType from open */
    if (!strcmp(verb, "open")) {
	LPSTR	devType, tmp;

	if ((devType = strchr(dev, '!')) != NULL) {
	    *devType++ = '\0';
	    tmp = devType; devType = dev; dev = tmp;

	    dwFlags |= MCI_OPEN_TYPE;
	    data[2] = (DWORD)devType;
	    devType = str_dup_upper(devType);
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data[3] = (DWORD)dev;
	} else if (strchr(dev, '.') == NULL) {
	    tmp = strchr(dev,' ');
	    if (tmp) *tmp = '\0';
	    data[2] = (DWORD)dev;
	    devType = str_dup_upper(dev);
	    if (tmp) *tmp = ' ';
	    dwFlags |= MCI_OPEN_TYPE;
	} else {
	    if ((devType = strstr(args, "type ")) != NULL) {
		devType += 5;
		tmp = strchr(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = str_dup_upper(devType);
		if (tmp) *tmp = ' ';
		/* dwFlags and data[2] will be correctly set in ParseOpt loop */
	    } else {
		char	buf[32];
		if ((dwRet = MCI_GetDevTypeFromFileName(dev, buf, sizeof(buf))))
		    goto errCleanUp;

		devType = str_dup_upper(buf);
	    }
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data[3] = (DWORD)dev;
	}
	if ((devAlias = strstr(args," alias "))) {
            char *tmp2;
	    devAlias += 7;
	    if (!(tmp = strchr(devAlias,' '))) tmp = devAlias + strlen(devAlias);
	    if (tmp) *tmp = '\0';
            tmp2 = HeapAlloc(GetProcessHeap(), 0, tmp - devAlias + 1 );
            memcpy( tmp2, devAlias, tmp - devAlias );
            tmp2[tmp - devAlias] = 0;
            data[4] = (DWORD)tmp2;
	    /* should be done in regular options parsing */
	    /* dwFlags |= MCI_OPEN_ALIAS; */
	}

	dwRet = MCI_LoadMciDriver(devType, &wmd);
	if (dwRet == MCIERR_DEVICE_NOT_INSTALLED)
	    dwRet = MCIERR_INVALID_DEVICE_NAME;
	HeapFree(GetProcessHeap(), 0, devType);
	if (dwRet) {
	    MCI_UnLoadMciDriver(wmd);
	    goto errCleanUp;
	}
    } else if (!(wmd = MCI_GetDriver(mciGetDeviceIDA(dev)))) {
	/* auto open */
	char	buf[128];
	sprintf(buf, "open %s wait", dev);

	if ((dwRet = mciSendStringA(buf, NULL, 0, 0)) != 0)
	    goto errCleanUp;

	wmd = MCI_GetDriver(mciGetDeviceIDA(dev));
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
	TRACE("Command '%s' not found!\n", verb);
	dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	goto errCleanUp;
    }

    /* set up call back */
    if (hwndCallback != 0) {
	dwFlags |= MCI_NOTIFY;
	data[0] = (DWORD)hwndCallback;
    }

    /* set return information */
    switch (retType = MCI_GetReturnType(lpCmd)) {
    case 0:		offset = 1;	break;
    case MCI_INTEGER:	offset = 2;	break;
    case MCI_STRING:	data[1] = (DWORD)lpstrRet; data[2] = uRetLen; offset = 3; break;
    case MCI_RECT:	offset = 5;	break;
    default:	ERR("oops\n");
    }

    TRACE("verb='%s' on dev='%s'; offset=%d\n", verb, dev, offset);

    if ((dwRet = MCI_ParseOptArgs(data, offset, lpCmd, args, &dwFlags)))
	goto errCleanUp;

    if (bAutoOpen && (dwFlags & MCI_NOTIFY)) {
	dwRet = MCIERR_NOTIFY_ON_AUTO_OPEN;
	goto errCleanUp;
    }
    /* FIXME: the command should get it's own notification window set up and
     * ask for device closing while processing the notification mechanism
     */
    if (lpstrRet && uRetLen) *lpstrRet = '\0';

    TRACE("[%d, %s, %08lx, %08lx/%s %08lx/%s %08lx/%s %08lx/%s %08lx/%s %08lx/%s]\n",
	  wmd->wDeviceID, MCI_MessageToString(MCI_GetMessage(lpCmd)), dwFlags,
	  data[0], debugstr_a((char *)data[0]), data[1], debugstr_a((char *)data[1]),
	  data[2], debugstr_a((char *)data[2]), data[3], debugstr_a((char *)data[3]),
	  data[4], debugstr_a((char *)data[4]), data[5], debugstr_a((char *)data[5]));

    if (strcmp(verb, "open") == 0) {
	if ((dwRet = MCI_FinishOpen(wmd, (LPMCI_OPEN_PARMSA)data, dwFlags)))
	    MCI_UnLoadMciDriver(wmd);
	/* FIXME: notification is not properly shared across two opens */
    } else {
	dwRet = MCI_SendCommand(wmd->wDeviceID, MCI_GetMessage(lpCmd), dwFlags, (DWORD)data, TRUE);
    }
    TRACE("=> 1/ %lx (%s)\n", dwRet, lpstrRet);
    dwRet = MCI_HandleReturnValues(dwRet, wmd, retType, data, lpstrRet, uRetLen);
    TRACE("=> 2/ %lx (%s)\n", dwRet, lpstrRet);

errCleanUp:
    HeapFree(GetProcessHeap(), 0, verb);
    HeapFree(GetProcessHeap(), 0, devAlias);
    return dwRet;
}

/**************************************************************************
 * 				mciSendStringW			[WINMM.@]
 */
DWORD WINAPI mciSendStringW(LPCWSTR lpwstrCommand, LPWSTR lpwstrRet,
			    UINT uRetLen, HWND hwndCallback)
{
    LPSTR 	lpstrCommand;
    LPSTR       lpstrRet = NULL;
    UINT	ret;
    INT len;

    /* FIXME: is there something to do with lpstrReturnString ? */
    len = WideCharToMultiByte( CP_ACP, 0, lpwstrCommand, -1, NULL, 0, NULL, NULL );
    lpstrCommand = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, lpwstrCommand, -1, lpstrCommand, len, NULL, NULL );
    if (lpwstrRet)
    {
        lpstrRet = HeapAlloc(GetProcessHeap(), 0, uRetLen * sizeof(WCHAR));
        if (!lpstrRet) return MMSYSERR_NOMEM;
    }
    ret = mciSendStringA(lpstrCommand, lpstrRet, uRetLen, hwndCallback);
    if (lpwstrRet)
        MultiByteToWideChar( CP_ACP, 0, lpstrRet, -1, lpwstrRet, uRetLen );
    HeapFree(GetProcessHeap(), 0, lpstrCommand);
    if (lpstrRet) HeapFree(GetProcessHeap(), 0, lpstrRet);
    return ret;
}

/**************************************************************************
 * 				mciExecute			[WINMM.@]
 * 				mciExecute			[MMSYSTEM.712]
 */
DWORD WINAPI mciExecute(LPCSTR lpstrCommand)
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
    return 0;
}

/**************************************************************************
 *                    	mciLoadCommandResource  		[WINMM.@]
 *
 * Strangely, this function only exists as an UNICODE one.
 */
UINT WINAPI mciLoadCommandResource(HINSTANCE hInst, LPCWSTR resNameW, UINT type)
{
    HRSRC	        hRsrc = 0;
    HGLOBAL      	hMem;
    UINT16		ret = MCI_NO_COMMAND_TABLE;

    TRACE("(%p, %s, %d)!\n", hInst, debugstr_w(resNameW), type);

    /* if a file named "resname.mci" exits, then load resource "resname" from it
     * otherwise directly from driver
     * We don't support it (who uses this feature ?), but we check anyway
     */
    if (!type) {
#if 0
        /* FIXME: we should put this back into order, but I never found a program
         * actually using this feature, so we not need it
         */
	char		buf[128];
	OFSTRUCT       	ofs;

	strcat(strcpy(buf, resname), ".mci");
	if (OpenFile(buf, &ofs, OF_EXIST) != HFILE_ERROR) {
	    FIXME("NIY: command table to be loaded from '%s'\n", ofs.szPathName);
	}
#endif
    }
    if (!(hRsrc = FindResourceW(hInst, resNameW, (LPWSTR)RT_RCDATA))) {
	WARN("No command table found in resource\n");
    } else if ((hMem = LoadResource(hInst, hRsrc))) {
	ret = MCI_SetCommandTable(LockResource(hMem), type);
    } else {
	WARN("Couldn't load resource.\n");
    }
    TRACE("=> %04x\n", ret);
    return ret;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[WINMM.@]
 */
BOOL WINAPI mciFreeCommandResource(UINT uTable)
{
    TRACE("(%08x)!\n", uTable);

    return MCI_DeleteCommandTable(uTable);
}

/**************************************************************************
 * 			MCI_SendCommandFrom32			[internal]
 */
DWORD MCI_SendCommandFrom32(MCIDEVICEID wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD		dwRet = MCIERR_INVALID_DEVICE_ID;
    LPWINE_MCIDRIVER	wmd = MCI_GetDriver(wDevID);

    if (wmd) {
	if (wmd->bIs32) {
	    dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
	} else if (pFnMciMapMsg32ATo16) {
	    WINMM_MapType	res;

	    switch (res = pFnMciMapMsg32ATo16(wmd->wType, wMsg, dwParam1, &dwParam2)) {
	    case WINMM_MAP_MSGERROR:
		TRACE("Not handled yet (%s)\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_DRIVER_INTERNAL;
		break;
	    case WINMM_MAP_NOMEM:
		TRACE("Problem mapping msg=%s from 32a to 16\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_OUT_OF_MEMORY;
		break;
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
		if (res == WINMM_MAP_OKMEM)
		    pFnMciUnMapMsg32ATo16(wmd->wType, wMsg, dwParam1, dwParam2);
		break;
	    }
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_SendCommandFrom16			[internal]
 */
DWORD MCI_SendCommandFrom16(MCIDEVICEID wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD		dwRet = MCIERR_INVALID_DEVICE_ID;
    LPWINE_MCIDRIVER	wmd = MCI_GetDriver(wDevID);

    if (wmd) {
	dwRet = MCIERR_INVALID_DEVICE_ID;

	if (wmd->bIs32 && pFnMciMapMsg16To32A) {
	    WINMM_MapType		res;

	    switch (res = pFnMciMapMsg16To32A(wmd->wType, wMsg, &dwParam2)) {
	    case WINMM_MAP_MSGERROR:
		TRACE("Not handled yet (%s)\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_DRIVER_INTERNAL;
		break;
	    case WINMM_MAP_NOMEM:
		TRACE("Problem mapping msg=%s from 16 to 32a\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_OUT_OF_MEMORY;
		break;
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
		if (res == WINMM_MAP_OKMEM)
		    pFnMciUnMapMsg16To32A(wmd->wType, wMsg, dwParam2);
		break;
	    }
	} else {
	    dwRet = SendDriverMessage(wmd->hDriver, wMsg, dwParam1, dwParam2);
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_Open				[internal]
 */
static	DWORD MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSA lpParms)
{
    char			strDevTyp[128];
    DWORD 			dwRet;
    LPWINE_MCIDRIVER		wmd = NULL;

    TRACE("(%08lX, %p)\n", dwParam, lpParms);
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
	    WORD uDevType = LOWORD((DWORD)lpParms->lpstrDeviceType);

	    if (uDevType < MCI_DEVTYPE_FIRST ||
		uDevType > MCI_DEVTYPE_LAST ||
		!LoadStringA(WINMM_IData->hWinMM32Instance, uDevType, strDevTyp, sizeof(strDevTyp))) {
		dwRet = MCIERR_BAD_INTEGER;
		goto errCleanUp;
	    }
	} else {
	    LPSTR	ptr;
	    if (lpParms->lpstrDeviceType == NULL) {
		dwRet = MCIERR_NULL_PARAMETER_BLOCK;
		goto errCleanUp;
	    }
	    strcpy(strDevTyp, lpParms->lpstrDeviceType);
	    ptr = strchr(strDevTyp, '!');
	    if (ptr) {
		/* this behavior is not documented in windows. However, since, in
		 * some occasions, MCI_OPEN handling is translated by WinMM into
		 * a call to mciSendString("open <type>"); this code shall be correct
		 */
		if (dwParam & MCI_OPEN_ELEMENT) {
		    ERR("Both MCI_OPEN_ELEMENT(%s) and %s are used\n",
			lpParms->lpstrElementName, strDevTyp);
		    dwRet = MCIERR_UNRECOGNIZED_KEYWORD;
		    goto errCleanUp;
		}
		dwParam |= MCI_OPEN_ELEMENT;
		*ptr++ = 0;
		/* FIXME: not a good idea to write in user supplied buffer */
		lpParms->lpstrElementName = ptr;
	    }

	}
	TRACE("devType='%s' !\n", strDevTyp);
    }

    if (dwParam & MCI_OPEN_ELEMENT) {
	TRACE("lpstrElementName='%s'\n", lpParms->lpstrElementName);

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
	    if (GetDriveTypeA(lpParms->lpstrElementName) != DRIVE_CDROM) {
		dwRet = MCIERR_EXTENSION_NOT_FOUND;
		goto errCleanUp;
	    }
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    strcpy(strDevTyp, "CDAUDIO");
	}
    }

    if (strDevTyp[0] == 0) {
	FIXME("Couldn't load driver\n");
	dwRet = MCIERR_INVALID_DEVICE_NAME;
	goto errCleanUp;
    }

    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE("Alias='%s' !\n", lpParms->lpstrAlias);
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
static	DWORD MCI_Close(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if (wDevID == MCI_ALL_DEVICE_ID) {
	LPWINE_MCIDRIVER	next;

	EnterCriticalSection(&WINMM_IData->cs);
	/* FIXME: shall I notify once after all is done, or for
	 * each of the open drivers ? if the latest, which notif
	 * to return when only one fails ?
	 */
	for (wmd = WINMM_IData->lpMciDrvs; wmd; ) {
	    next = wmd->lpNext;
	    MCI_Close(wmd->wDeviceID, dwParam, lpParms);
	    wmd = next;
	}
	LeaveCriticalSection(&WINMM_IData->cs);
	return 0;
    }

    if (!(wmd = MCI_GetDriver(wDevID))) {
	return MCIERR_INVALID_DEVICE_ID;
    }

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD)lpParms);

    MCI_UnLoadMciDriver(wmd);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify((HWND)lpParms->dwCallback, wDevID,
                        (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);

    return dwRet;
}

/**************************************************************************
 * 			MCI_WriteString				[internal]
 */
DWORD	MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr)
{
    DWORD	ret = 0;

    if (lpSrcStr) {
	if (dstSize <= strlen(lpSrcStr)) {
	    lstrcpynA(lpDstStr, lpSrcStr, dstSize - 1);
	    ret = MCIERR_PARAM_OVERFLOW;
	} else {
	    strcpy(lpDstStr, lpSrcStr);
	}
    } else {
	*lpDstStr = 0;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Sysinfo				[internal]
 */
static	DWORD MCI_SysInfo(UINT uDevID, DWORD dwFlags, LPMCI_SYSINFO_PARMSA lpParms)
{
    DWORD		ret = MCIERR_INVALID_DEVICE_ID;
    LPWINE_MCIDRIVER	wmd;

    if (lpParms == NULL)			return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("(%08x, %08lX, %08lX[num=%ld, wDevTyp=%u])\n",
	  uDevID, dwFlags, (DWORD)lpParms, lpParms->dwNumber, lpParms->wDeviceType);

    switch (dwFlags & ~MCI_SYSINFO_OPEN) {
    case MCI_SYSINFO_QUANTITY:
	{
	    DWORD	cnt = 0;

	    if (lpParms->wDeviceType < MCI_DEVTYPE_FIRST ||
		lpParms->wDeviceType > MCI_DEVTYPE_LAST) {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers\n");
		    EnterCriticalSection(&WINMM_IData->cs);
		    for (wmd = WINMM_IData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
			cnt++;
		    }
		    LeaveCriticalSection(&WINMM_IData->cs);
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers\n");
		    cnt = MCI_InstalledCount;
		}
	    } else {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %u\n",
			  lpParms->wDeviceType);
		    EnterCriticalSection(&WINMM_IData->cs);
		    for (wmd = WINMM_IData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
			if (wmd->wType == lpParms->wDeviceType)
			    cnt++;
		    }
		    LeaveCriticalSection(&WINMM_IData->cs);
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %u\n",
			  lpParms->wDeviceType);
		    FIXME("Don't know how to get # of MCI devices of a given type\n");
		    cnt = 1;
		}
	    }
	    *(DWORD*)lpParms->lpstrReturn = cnt;
	}
	TRACE("(%ld) => '%ld'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = MCI_INTEGER_RETURNED;
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE("MCI_SYSINFO_INSTALLNAME \n");
	if ((wmd = MCI_GetDriver(uDevID))) {
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize,
				  wmd->lpstrDeviceType);
	} else {
	    *lpParms->lpstrReturn = 0;
	    ret = MCIERR_INVALID_DEVICE_ID;
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    case MCI_SYSINFO_NAME:
	TRACE("MCI_SYSINFO_NAME\n");
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    FIXME("Don't handle MCI_SYSINFO_NAME|MCI_SYSINFO_OPEN (yet)\n");
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	} else if (lpParms->dwNumber > MCI_InstalledCount) {
	    ret = MCIERR_OUTOFRANGE;
	} else {
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = MCI_lpInstallNames;

	    while (--count > 0) ptr += strlen(ptr) + 1;
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, ptr);
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    default:
	TRACE("Unsupported flag value=%08lx\n", dwFlags);
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
static	DWORD MCI_Sound(UINT wDevID, DWORD dwFlags, LPMCI_SOUND_PARMS lpParms)
{
    DWORD	dwRet = 0;

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_SOUND_NAME)
        dwRet = sndPlaySoundA(lpParms->lpstrSoundName, SND_SYNC) ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
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
DWORD	MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1,
			DWORD dwParam2, BOOL bFrom32)
{
    DWORD		dwRet = MCIERR_UNRECOGNIZED_COMMAND;

    switch (wMsg) {
    case MCI_OPEN:
	if (bFrom32) {
	    dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
	} else if (pFnMciMapMsg16To32A) {
	    switch (pFnMciMapMsg16To32A(0, wMsg, &dwParam2)) {
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
		pFnMciUnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_CLOSE:
	if (bFrom32) {
	    dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
	} else if (pFnMciMapMsg16To32A) {
	    switch (pFnMciMapMsg16To32A(0, wMsg, &dwParam2)) {
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		pFnMciUnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_SYSINFO:
	if (bFrom32) {
	    dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
	} else if (pFnMciMapMsg16To32A) {
	    switch (pFnMciMapMsg16To32A(0, wMsg, &dwParam2)) {
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
		pFnMciUnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_BREAK:
	if (bFrom32) {
	    dwRet = MCI_Break(wDevID, dwParam1, (LPMCI_BREAK_PARMS)dwParam2);
	} else if (pFnMciMapMsg16To32A) {
	    switch (pFnMciMapMsg16To32A(0, wMsg, &dwParam2)) {
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = MCI_Break(wDevID, dwParam1, (LPMCI_BREAK_PARMS)dwParam2);
		pFnMciUnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_SOUND:
	if (bFrom32) {
	    dwRet = MCI_Sound(wDevID, dwParam1, (LPMCI_SOUND_PARMS)dwParam2);
	} else if (pFnMciMapMsg16To32A) {
	    switch (pFnMciMapMsg16To32A(0, wMsg, &dwParam2)) {
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		dwRet = MCI_Sound(wDevID, dwParam1, (LPMCI_SOUND_PARMS)dwParam2);
		pFnMciUnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    default:
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    dwRet = (bFrom32) ?
		MCI_SendCommandFrom32(wDevID, wMsg, dwParam1, dwParam2) :
		MCI_SendCommandFrom16(wDevID, wMsg, dwParam1, dwParam2);
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
LRESULT		MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2)
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

		lmgp = (LPMCI_GETDEVCAPS_PARMS)(void*)dwParam2;
		TRACE("Changing %08lx to %08lx\n", lmgp->dwReturn, (DWORD)LOWORD(lmgp->dwReturn));
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

		lsp = (LPMCI_STATUS_PARMS)(void*)dwParam2;
		TRACE("Changing %08lx to %08lx\n", lsp->dwReturn, (DWORD)LOWORD(lsp->dwReturn));
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
 * 			MCI_Init			[internal]
 *
 * Initializes the MCI internal variables.
 *
 */
BOOL MCI_Init(void)
{
    LPSTR	ptr1, ptr2;
    HKEY	hWineConf;
    HKEY	hkey;
    DWORD	err;
    DWORD 	type;
    DWORD 	count = 2048;

    MCI_InstalledCount = 0;
    ptr1 = MCI_lpInstallNames = HeapAlloc(GetProcessHeap(), 0, count);

    if (!MCI_lpInstallNames)
	return FALSE;

    /* FIXME: should do also some registry diving here ? */
    if (!(err = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config", &hWineConf)) &&
	!(err = RegOpenKeyA(hWineConf, "options", &hkey))) {
	err = RegQueryValueExA(hkey, "mci", 0, &type, MCI_lpInstallNames, &count);
	RegCloseKey(hkey);

    }
    if (!err) {
	TRACE("Wine => '%s' \n", ptr1);
	while ((ptr2 = strchr(ptr1, ':')) != 0) {
	    *ptr2++ = 0;
	    TRACE("---> '%s' \n", ptr1);
	    MCI_InstalledCount++;
	    ptr1 = ptr2;
	}
	MCI_InstalledCount++;
	TRACE("---> '%s' \n", ptr1);
	ptr1 += strlen(ptr1) + 1;
    } else {
	GetPrivateProfileStringA("mci", NULL, "", MCI_lpInstallNames, count, "SYSTEM.INI");
	while (strlen(ptr1) > 0) {
	    TRACE("---> '%s' \n", ptr1);
	    ptr1 += strlen(ptr1) + 1;
	    MCI_InstalledCount++;
	}
    }
    RegCloseKey(hWineConf);
    return TRUE;
}
