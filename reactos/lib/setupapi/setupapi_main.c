/*
 *      SETUPAPI library functions
 *      32-bit version of setupx_main.c
 *
 *      Copyright 2004 Aleksey Bragin
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
 *
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "setupx16.h"
#include "setupapi_private.h"
#include "winerror.h"
#include "wine/debug.h"

//WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/*
 * Returns pointer to a string list with the first entry being number
 * of strings.
 *
 * Hmm. Should this be InitSubstrData(), GetFirstSubstr() and GetNextSubstr()
 * instead?
 */
static LPSTR *SETUPX_GetSubStrings(LPSTR start, char delimiter)
{
    LPSTR p, q;
    LPSTR *res = NULL;
    DWORD count = 0;
    int len;

    p = start;

    while (1)
    {
	/* find beginning of real substring */
	while ( (*p == ' ') || (*p == '\t') || (*p == '"') ) p++;

	/* find end of real substring */
	q = p;
	while ( (*q)
	     &&	(*q != ' ') && (*q != '\t') && (*q != '"')
	     && (*q != ';') && (*q != delimiter) ) q++;
	if (q == p)
	    break;
	len = (int)q - (int)p;

	/* alloc entry for new substring in steps of 32 units and copy over */
	if (count % 32 == 0)
	{ /* 1 for count field + current count + 32 */
	    if (res)
    		res = HeapReAlloc(GetProcessHeap(), 0, res, (1+count+32)*sizeof(LPSTR));
	    else
		res = HeapAlloc(GetProcessHeap(), 0, (1+count+32)*sizeof(LPSTR));	    
	}
	*(res+1+count) = HeapAlloc(GetProcessHeap(), 0, len+1);
	strncpy(*(res+1+count), p, len);
	(*(res+1+count))[len] = '\0';
	count++;

	/* we are still within last substring (before delimiter),
	 * so get out of it */
	while ((*q) && (*q != ';') && (*q != delimiter)) q++;
	if ((!*q) || (*q == ';'))
	    break;
	p = q+1;
    }

    /* put number of entries at beginning of list */
    *(DWORD *)res = count;
    return res;
}

static void SETUPX_FreeSubStrings(LPSTR *substr)
{
    DWORD count = *(DWORD *)substr;
    LPSTR *pStrings = substr+1;
    DWORD n;

    for (n=0; n < count; n++)
	HeapFree(GetProcessHeap(), 0, *pStrings++);

    HeapFree(GetProcessHeap(), 0, substr);
}

// Temp for debugging
#ifdef FIREBALL_DOING_DEBUG
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
         LPSTR lpszCmdLine, int nCmdShow )
{
    void WINAPI InstallHinfSection(HWND hwnd, HINSTANCE handle, LPCSTR lpszCmdLine, INT nCmdShow);
    
    InstallHinfSection(NULL, hInstance, lpszCmdLine, nCmdShow);
    return 0;
}
#endif


/***********************************************************************
 *		InstallHinfSection (SETUPAPI.@)
 *
 * hwnd = parent window
 * handle = instance of SETUPAPI.DLL
 * lpszCmdLine = e.g. "DefaultInstall 132 C:\MYINSTALL\MYDEV.INF"
 * Here "DefaultInstall" is the .inf file section to be installed (optional).
 * The 132 value is made of the HOW_xxx flags and sometimes 128 (-> setupx16.h).
 *
 * nCmdShow = nCmdShow of CreateProcess
 */
// TODO: 1) Set errors
//       2) Take flags into account
void WINAPI InstallHinfSection(HWND hwnd, HINSTANCE handle, LPCSTR lpszCmdLine, INT nCmdShow)
{
	LPSTR *pSub;
    DWORD count;
    HINF hInf = 0;
    RETERR16 res = OK, tmp;
    WORD wFlags;
    BOOL reboot = FALSE;

    TRACE("(%04x, %04x, %s, %d);\n", hwnd, hinst, lpszCmdLine, nCmdShow);

    pSub = SETUPX_GetSubStrings((LPSTR)lpszCmdLine, ' ');
    count = *(DWORD *)pSub;

    if (count < 2) /* invalid number of arguments ? */
    {
        SETUPX_FreeSubStrings(pSub);	
        return;
    }

	hInf = SetupOpenInfFileA(*(pSub+count), NULL, INF_STYLE_WIN4, NULL);
    if (hInf == (HINF)INVALID_HANDLE_VALUE)
    {
		//res = ERROR_FILE_NOT_FOUND; /* yes, correct */
		SETUPX_FreeSubStrings(pSub);
		return;
    }

	// FIXME: Take result in mind	
	SetupInstallFromInfSectionA(hwnd, hInf, (PCSTR)(*(pSub+1)), SPINST_ALL/*flags*/,
                                NULL/*HKEY key_root*/, "C:\\"/*PCWSTR src_root*/, 0/*UINT copy_flags*/,
                                NULL/*PSP_FILE_CALLBACK_W callback*/, NULL/*PVOID context*/,
                                NULL/*HDEVINFO devinfo*/, NULL/*PSP_DEVINFO_DATA devinfo_data*/);
	
	// release alloced memory
	SetupCloseInfFile(hInf);
    SETUPX_FreeSubStrings(pSub);	
	
/*
	wFlags = atoi(*(pSub+count-1)) & ~128;
    switch (wFlags)
    {
	case HOW_ALWAYS_SILENT_REBOOT:
	case HOW_SILENT_REBOOT:
	    reboot = TRUE;
	    break;
	case HOW_ALWAYS_PROMPT_REBOOT:
	case HOW_PROMPT_REBOOT:
            if (MessageBoxA(HWND_32(hwnd), "You must restart Wine before the new settings will take effect.\n\nDo you want to exit Wine now ?", "Systems Settings Change", MB_YESNO|MB_ICONQUESTION) == IDYES)
                reboot = TRUE;
	    break;
	default:
	    ERR("invalid flags %d !\n", wFlags);
	    goto end;
    }
*/
}

typedef struct
{
    LPCSTR RegValName;
    LPCSTR StdString; /* fallback string; sub dir of windows directory */
} LDID_DATA;

static const LDID_DATA LDID_Data[34] =
{
    { /* 0 (LDID_NULL) -- not defined */
	NULL,
	NULL
    },
    { /* 1 (LDID_SRCPATH) = source of installation. hmm, what to do here ? */
	"SourcePath", /* hmm, does SETUPX have to care about updating it ?? */
	NULL
    },
    { /* 2 (LDID_SETUPTEMP) = setup temp dir */
	"SetupTempDir",
	NULL
    },
    { /* 3 (LDID_UNINSTALL) = uninstall backup dir */
	"UninstallDir",
	NULL
    },
    { /* 4 (LDID_BACKUP) = backup dir */
	"BackupDir",
	NULL
    },
    { /* 5 (LDID_SETUPSCRATCH) = setup scratch dir */
	"SetupScratchDir",
	NULL
    },
    { /* 6 -- not defined */
	NULL,
	NULL
    },
    { /* 7 -- not defined */
	NULL,
	NULL
    },
    { /* 8 -- not defined */
	NULL,
	NULL
    },
    { /* 9 -- not defined */
	NULL,
	NULL
    },
    { /* 10 (LDID_WIN) = windows dir */
	"WinDir",
        ""
    },
    { /* 11 (LDID_SYS) = system dir */
	"SysDir",
	NULL /* call GetSystemDirectory() instead */
    },
    { /* 12 (LDID_IOS) = IOSubSys dir */
        NULL, /* FIXME: registry string ? */
	"SYSTEM\\IOSUBSYS"
    },
    { /* 13 (LDID_CMD) = COMMAND dir */
	NULL, /* FIXME: registry string ? */
	"COMMAND"
    },
    { /* 14 (LDID_CPL) = control panel dir */
	NULL,
	""
    },
    { /* 15 (LDID_PRINT) = windows printer dir */
	NULL,
	"SYSTEM" /* correct ?? */
    },
    { /* 16 (LDID_MAIL) = destination mail dir */
	NULL,
	""
    },
    { /* 17 (LDID_INF) = INF dir */
	"SetupScratchDir", /* correct ? */
	"INF"
    },
    { /* 18 (LDID_HELP) = HELP dir */
	NULL, /* ??? */
	"HELP"
    },
    { /* 19 (LDID_WINADMIN) = Admin dir */
	"WinAdminDir",
	""
    },
    { /* 20 (LDID_FONTS) = Fonts dir */
	NULL, /* ??? */
	"FONTS"
    },
    { /* 21 (LDID_VIEWERS) = Viewers */
	NULL, /* ??? */
	"SYSTEM\\VIEWERS"
    },
    { /* 22 (LDID_VMM32) = VMM32 dir */
	NULL, /* ??? */
	"SYSTEM\\VMM32"
    },
    { /* 23 (LDID_COLOR) = ICM dir */
	"ICMPath",
	"SYSTEM\\COLOR"
    },
    { /* 24 (LDID_APPS) = root of boot drive ? */
	"AppsDir",
	"C:\\"
    },
    { /* 25 (LDID_SHARED) = shared dir */
	"SharedDir",
	""
    },
    { /* 26 (LDID_WINBOOT) = Windows boot dir */
	"WinBootDir",
	""
    },
    { /* 27 (LDID_MACHINE) = machine specific files */
	"MachineDir",
	NULL
    },
    { /* 28 (LDID_HOST_WINBOOT) = Host Windows boot dir */
	"HostWinBootDir",
	NULL
    },
    { /* 29 -- not defined */
	NULL,
	NULL
    },
    { /* 30 (LDID_BOOT) = Root of boot drive */
	"BootDir",
	NULL
    },
    { /* 31 (LDID_BOOT_HOST) = Root of boot drive host */
	"BootHost",
	NULL
    },
    { /* 32 (LDID_OLD_WINBOOT) = subdir of root */
	"OldWinBootDir",
	NULL
    },
    { /* 33 (LDID_OLD_WIN) = old win dir */
	"OldWinDir",
	NULL
    }
    /* the rest (34-38) isn't too interesting, so I'll forget about it */
};
