/*
 * 				Shell Library Functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wownt32.h"
#include "shellapi.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"
#include "shlwapi.h"

#include "wine/winbase16.h"
#include "shell32_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


typedef struct {     /* structure for dropped files */
 WORD     wSize;
 POINT16  ptMousePos;
 BOOL16   fInNonClientArea;
 /* memory block with filenames follows */
} DROPFILESTRUCT16, *LPDROPFILESTRUCT16;

static const char lpstrMsgWndCreated[] = "OTHERWINDOWCREATED";
static const char lpstrMsgWndDestroyed[] = "OTHERWINDOWDESTROYED";
static const char lpstrMsgShellActivate[] = "ACTIVATESHELLWINDOW";

static HWND	SHELL_hWnd = 0;
static HHOOK	SHELL_hHook = 0;
static UINT	uMsgWndCreated = 0;
static UINT	uMsgWndDestroyed = 0;
static UINT	uMsgShellActivate = 0;

/***********************************************************************
 * DllEntryPoint [SHELL.101]
 *
 * Initialization code for shell.dll. Automatically loads the
 * 32-bit shell32.dll to allow thunking up to 32-bit code.
 *
 * RETURNS
 *  Success: TRUE. Initialization completed successfully.
 *  Failure: FALSE.
 */
BOOL WINAPI SHELL_DllEntryPoint(DWORD Reason, HINSTANCE16 hInst,
				WORD ds, WORD HeapSize, DWORD res1, WORD res2)
{
    return TRUE;
}

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b)
{
  DragAcceptFiles(HWND_32(hWnd), b);
}

/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
UINT16 WINAPI DragQueryFile16(
	HDROP16 hDrop,
	WORD wFile,
	LPSTR lpszFile,
	WORD wLength)
{
 	LPSTR lpDrop;
	UINT i = 0;
	LPDROPFILESTRUCT16 lpDropFileStruct = (LPDROPFILESTRUCT16) GlobalLock16(hDrop);

	TRACE("(%04x, %x, %p, %u)\n", hDrop,wFile,lpszFile,wLength);

	if(!lpDropFileStruct) goto end;

	lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->wSize;

	while (i++ < wFile)
	{
	  while (*lpDrop++); /* skip filename */
	  if (!*lpDrop)
	  {
	    i = (wFile == 0xFFFF) ? i : 0;
	    goto end;
	  }
	}

	i = strlen(lpDrop);
	if (!lpszFile ) goto end;   /* needed buffer size */
	lstrcpynA (lpszFile, lpDrop, wLength);
end:
	GlobalUnlock16(hDrop);
	return i;
}

/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void WINAPI DragFinish16(HDROP16 h)
{
    TRACE("\n");
    GlobalFree16((HGLOBAL16)h);
}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL16 WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p)
{
  LPDROPFILESTRUCT16 lpDropFileStruct;
  BOOL16           bRet;
  TRACE("\n");
  lpDropFileStruct = (LPDROPFILESTRUCT16) GlobalLock16(hDrop);

  memcpy(p,&lpDropFileStruct->ptMousePos,sizeof(POINT16));
  bRet = lpDropFileStruct->fInNonClientArea;

  GlobalUnlock16(hDrop);
  return bRet;
}

/*************************************************************************
 *             FindExecutable   (SHELL.21)
 */
HINSTANCE16 WINAPI FindExecutable16( LPCSTR lpFile, LPCSTR lpDirectory,
                                     LPSTR lpResult )
{ return HINSTANCE_16(FindExecutableA( lpFile, lpDirectory, lpResult ));
}

/*************************************************************************
 *             AboutDlgProc   (SHELL.33)
 */
BOOL16 WINAPI AboutDlgProc16( HWND16 hWnd, UINT16 msg, WPARAM16 wParam,
                               LPARAM lParam )
{ return (BOOL16)AboutDlgProc( HWND_32(hWnd), msg, wParam, lParam );
}


/*************************************************************************
 *             ShellAbout   (SHELL.22)
 */
BOOL16 WINAPI ShellAbout16( HWND16 hWnd, LPCSTR szApp, LPCSTR szOtherStuff,
                            HICON16 hIcon )
{ return ShellAboutA( HWND_32(hWnd), szApp, szOtherStuff, HICON_32(hIcon) );
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 */
HGLOBAL16 WINAPI InternalExtractIcon16(HINSTANCE16 hInstance,
                                     LPCSTR lpszExeFileName, UINT16 nIconIndex, WORD n )
{
    HGLOBAL16 hRet = 0;
    HICON16 *RetPtr = NULL;
    OFSTRUCT ofs;

	TRACE("(%04x,file %s,start %d,extract %d\n",
		       hInstance, lpszExeFileName, nIconIndex, n);

	if (!n)
	  return 0;

	hRet = GlobalAlloc16(GMEM_FIXED | GMEM_ZEROINIT, sizeof(*RetPtr) * n);
	RetPtr = (HICON16*)GlobalLock16(hRet);

	if (nIconIndex == (UINT16)-1)  /* get number of icons */
	{
	  RetPtr[0] = PrivateExtractIconsA(ofs.szPathName, 0, 0, 0, NULL, NULL, 0, LR_DEFAULTCOLOR);
	}
	else
	{
	  UINT ret;
	  HICON *icons;

	  icons = HeapAlloc(GetProcessHeap(), 0, n * sizeof(*icons));
	  ret = PrivateExtractIconsA(ofs.szPathName, nIconIndex,
	                             GetSystemMetrics(SM_CXICON),
	                             GetSystemMetrics(SM_CYICON),
	                             icons, NULL, n, LR_DEFAULTCOLOR);
	  if ((ret != 0xffffffff) && ret)
	  {
	    int i;
	    for (i = 0; i < n; i++) RetPtr[i] = HICON_16(icons[i]);
	  }
	  else
	  {
	    GlobalFree16(hRet);
	    hRet = 0;
	  }
	  HeapFree(GetProcessHeap(), 0, icons);
	}
	return hRet;
}

/*************************************************************************
 *             ExtractIcon   (SHELL.34)
 */
HICON16 WINAPI ExtractIcon16( HINSTANCE16 hInstance, LPCSTR lpszExeFileName,
	UINT16 nIconIndex )
{   TRACE("\n");
    return HICON_16(ExtractIconA(HINSTANCE_32(hInstance), lpszExeFileName, nIconIndex));
}

/*************************************************************************
 *             ExtractIconEx   (SHELL.40)
 */
HICON16 WINAPI ExtractIconEx16(
	LPCSTR lpszFile, INT16 nIconIndex, HICON16 *phiconLarge,
	HICON16 *phiconSmall, UINT16 nIcons
) {
    HICON	*ilarge,*ismall;
    UINT16	ret;
    int		i;

    if (phiconLarge)
    	ilarge = HeapAlloc(GetProcessHeap(),0,nIcons*sizeof(HICON));
    else
    	ilarge = NULL;
    if (phiconSmall)
    	ismall = HeapAlloc(GetProcessHeap(),0,nIcons*sizeof(HICON));
    else
    	ismall = NULL;
    ret = HICON_16(ExtractIconExA(lpszFile,nIconIndex,ilarge,ismall,nIcons));
    if (ilarge) {
    	for (i=0;i<nIcons;i++)
	    phiconLarge[i]=HICON_16(ilarge[i]);
	HeapFree(GetProcessHeap(),0,ilarge);
    }
    if (ismall) {
    	for (i=0;i<nIcons;i++)
	    phiconSmall[i]=HICON_16(ismall[i]);
	HeapFree(GetProcessHeap(),0,ismall);
    }
    return ret;
}

/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON16 WINAPI ExtractAssociatedIcon16(HINSTANCE16 hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    return HICON_16(ExtractAssociatedIconA(HINSTANCE_32(hInst), lpIconPath,
		    lpiIcon));
}

/*************************************************************************
 *				FindEnvironmentString	[SHELL.38]
 *
 * Returns a pointer into the DOS environment... Ugh.
 */
static LPSTR SHELL_FindString(LPSTR lpEnv, LPCSTR entry)
{ UINT16 l;

  TRACE("\n");

  l = strlen(entry);
  for( ; *lpEnv ; lpEnv+=strlen(lpEnv)+1 )
  { if( strncasecmp(lpEnv, entry, l) )
      continue;
	if( !*(lpEnv+l) )
	    return (lpEnv + l); 		/* empty entry */
	else if ( *(lpEnv+l)== '=' )
	    return (lpEnv + l + 1);
    }
    return NULL;
}

/**********************************************************************/

SEGPTR WINAPI FindEnvironmentString16(LPCSTR str)
{ SEGPTR  spEnv;
  LPSTR lpEnv,lpString;
  TRACE("\n");

  spEnv = GetDOSEnvironment16();

  lpEnv = MapSL(spEnv);
  lpString = (spEnv)?SHELL_FindString(lpEnv, str):NULL;

    if( lpString )		/*  offset should be small enough */
	return spEnv + (lpString - lpEnv);
    return (SEGPTR)NULL;
}

/*************************************************************************
 *              		DoEnvironmentSubst      [SHELL.37]
 *
 * Replace %KEYWORD% in the str with the value of variable KEYWORD
 * from "DOS" environment. If it is not found the %KEYWORD% is left
 * intact. If the buffer is too small, str is not modified.
 *
 * PARAMS
 *  str        [I] '\0' terminated string with %keyword%.
 *             [O] '\0' terminated string with %keyword% substituted.
 *  length     [I] size of str.
 *
 * RETURNS
 *  str length in the LOWORD and 1 in HIWORD if subst was successful.
 */
DWORD WINAPI DoEnvironmentSubst16(LPSTR str,WORD length)
{
  LPSTR   lpEnv = MapSL(GetDOSEnvironment16());
  LPSTR   lpstr = str;
  LPSTR   lpend;
  LPSTR   lpBuffer = HeapAlloc( GetProcessHeap(), 0, length);
  WORD    bufCnt = 0;
  WORD    envKeyLen;
  LPSTR   lpKey;
  WORD    retStatus = 0;
  WORD    retLength = length;

  CharToOemA(str,str);

  TRACE("accept %s\n", str);

  while( *lpstr && bufCnt <= length - 1 ) {
     if ( *lpstr != '%' ) {
        lpBuffer[bufCnt++] = *lpstr++;
        continue;
     }

     for( lpend = lpstr + 1; *lpend && *lpend != '%'; lpend++) /**/;

     envKeyLen = lpend - lpstr - 1;
     if( *lpend != '%' || envKeyLen == 0)
        goto err; /* "%\0" or "%%" found; back off and whine */

     *lpend = '\0';
     lpKey = SHELL_FindString(lpEnv, lpstr+1);
     *lpend = '%';
     if( lpKey ) {
         int l = strlen(lpKey);

         if( bufCnt + l > length - 1 )
                goto err;

        memcpy(lpBuffer + bufCnt, lpKey, l);
        bufCnt += l;
     } else { /* Keyword not found; Leave the %KEYWORD% intact */
        if( bufCnt + envKeyLen + 2 > length - 1 )
            goto err;

         memcpy(lpBuffer + bufCnt, lpstr, envKeyLen + 2);
        bufCnt += envKeyLen + 2;
     }

     lpstr = lpend + 1;
  }

  if (!*lpstr && bufCnt <= length - 1) {
      memcpy(str,lpBuffer, bufCnt);
      str[bufCnt] = '\0';
      retLength = bufCnt + 1;
      retStatus = 1;
  }

  err:
  if (!retStatus)
      WARN("-- Env subst aborted - string too short or invalid input\n");
  TRACE("-- return %s\n", str);

  OemToCharA(str,str);
  HeapFree( GetProcessHeap(), 0, lpBuffer);

  return (DWORD)MAKELONG(retLength, retStatus);
}

/*************************************************************************
 *				SHELL_HookProc
 *
 * 32-bit version of the system-wide WH_SHELL hook.
 */
static LRESULT WINAPI SHELL_HookProc(INT code, WPARAM wParam, LPARAM lParam)
{
    TRACE("%i, %lx, %08lx\n", code, wParam, lParam );

    if (SHELL_hWnd)
    {
        switch( code )
        {
        case HSHELL_WINDOWCREATED:
            PostMessageA( SHELL_hWnd, uMsgWndCreated, wParam, 0 );
            break;
        case HSHELL_WINDOWDESTROYED:
            PostMessageA( SHELL_hWnd, uMsgWndDestroyed, wParam, 0 );
            break;
        case HSHELL_ACTIVATESHELLWINDOW:
            PostMessageA( SHELL_hWnd, uMsgShellActivate, wParam, 0 );
            break;
        }
    }
    return CallNextHookEx( SHELL_hHook, code, wParam, lParam );
}

/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 * System-wide WH_SHELL hook.
 */
LRESULT WINAPI ShellHookProc16(INT16 code, WPARAM16 wParam, LPARAM lParam)
{
    return SHELL_HookProc( code, wParam, lParam );
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
BOOL WINAPI RegisterShellHook16(HWND16 hWnd, UINT16 uAction)
{
    TRACE("%04x [%u]\n", hWnd, uAction );

    switch( uAction )
    {
    case 2:  /* register hWnd as a shell window */
        if( !SHELL_hHook )
        {
            SHELL_hHook = SetWindowsHookExA( WH_SHELL, SHELL_HookProc,
                                             GetModuleHandleA("shell32.dll"), 0 );
            if ( SHELL_hHook )
            {
                uMsgWndCreated = RegisterWindowMessageA( lpstrMsgWndCreated );
                uMsgWndDestroyed = RegisterWindowMessageA( lpstrMsgWndDestroyed );
                uMsgShellActivate = RegisterWindowMessageA( lpstrMsgShellActivate );
            }
            else
                WARN("-- unable to install ShellHookProc()!\n");
        }

        if ( SHELL_hHook )
            return ((SHELL_hWnd = HWND_32(hWnd)) != 0);
        break;

    default:
        WARN("-- unknown code %i\n", uAction );
        SHELL_hWnd = 0; /* just in case */
    }
    return FALSE;
}


/***********************************************************************
 *           DriveType   (SHELL.262)
 */
UINT16 WINAPI DriveType16( UINT16 drive )
{
    UINT ret;
    char path[] = "A:\\";
    path[0] += drive;
    ret = GetDriveTypeA(path);
    switch(ret)  /* some values are not supported in Win16 */
    {
    case DRIVE_CDROM:
        ret = DRIVE_REMOTE;
        break;
    case DRIVE_NO_ROOT_DIR:
        ret = DRIVE_UNKNOWN;
        break;
    }
    return ret;
}


/* 0 and 1 are valid rootkeys in win16 shell.dll and are used by
 * some programs. Do not remove those cases. -MM
 */
static inline void fix_win16_hkey( HKEY *hkey )
{
    if (*hkey == 0 || *hkey == (HKEY)1) *hkey = HKEY_CLASSES_ROOT;
}

/******************************************************************************
 *           RegOpenKey   [SHELL.1]
 */
DWORD WINAPI RegOpenKey16( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    fix_win16_hkey( &hkey );
    return RegOpenKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegCreateKey   [SHELL.2]
 */
DWORD WINAPI RegCreateKey16( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    fix_win16_hkey( &hkey );
    return RegCreateKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegCloseKey   [SHELL.3]
 */
DWORD WINAPI RegCloseKey16( HKEY hkey )
{
    fix_win16_hkey( &hkey );
    return RegCloseKey( hkey );
}

/******************************************************************************
 *           RegDeleteKey   [SHELL.4]
 */
DWORD WINAPI RegDeleteKey16( HKEY hkey, LPCSTR name )
{
    fix_win16_hkey( &hkey );
    return RegDeleteKeyA( hkey, name );
}

/******************************************************************************
 *           RegSetValue   [SHELL.5]
 */
DWORD WINAPI RegSetValue16( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    fix_win16_hkey( &hkey );
    return RegSetValueA( hkey, name, type, data, count );
}

/******************************************************************************
 *           RegQueryValue   [SHELL.6]
 *
 * NOTES
 *    Is this HACK still applicable?
 *
 * HACK
 *    The 16bit RegQueryValue doesn't handle selectorblocks anyway, so we just
 *    mask out the high 16 bit.  This (not so much incidently) hopefully fixes
 *    Aldus FH4)
 */
DWORD WINAPI RegQueryValue16( HKEY hkey, LPCSTR name, LPSTR data, LPDWORD count
)
{
    fix_win16_hkey( &hkey );
    if (count) *count &= 0xffff;
    return RegQueryValueA( hkey, name, data, (LONG*) count );
}

/******************************************************************************
 *           RegEnumKey   [SHELL.7]
 */
DWORD WINAPI RegEnumKey16( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    fix_win16_hkey( &hkey );
    return RegEnumKeyA( hkey, index, name, name_len );
}

/*************************************************************************
 *           SHELL_Execute16 [Internal]
 */
static UINT_PTR SHELL_Execute16(const WCHAR *lpCmd, WCHAR *env, BOOL shWait,
			    LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out)
{
    UINT ret;
    char sCmd[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, lpCmd, -1, sCmd, MAX_PATH, NULL, NULL);
    ret = WinExec16(sCmd, (UINT16)psei->nShow);
    psei_out->hInstApp = HINSTANCE_32(ret);
    return ret;
}

/*************************************************************************
 *                              ShellExecute            [SHELL.20]
 */
HINSTANCE16 WINAPI ShellExecute16( HWND16 hWnd, LPCSTR lpOperation,
                                   LPCSTR lpFile, LPCSTR lpParameters,
                                   LPCSTR lpDirectory, INT16 iShowCmd )
{
    SHELLEXECUTEINFOW seiW;
    WCHAR *wVerb = NULL, *wFile = NULL, *wParameters = NULL, *wDirectory = NULL;
    HANDLE hProcess = 0;

    seiW.lpVerb = lpOperation ? __SHCloneStrAtoW(&wVerb, lpOperation) : NULL;
    seiW.lpFile = lpFile ? __SHCloneStrAtoW(&wFile, lpFile) : NULL;
    seiW.lpParameters = lpParameters ? __SHCloneStrAtoW(&wParameters, lpParameters) : NULL;
    seiW.lpDirectory = lpDirectory ? __SHCloneStrAtoW(&wDirectory, lpDirectory) : NULL;

    seiW.cbSize = sizeof(seiW);
    seiW.fMask = 0;
    seiW.hwnd = HWND_32(hWnd);
    seiW.nShow = iShowCmd;
    seiW.lpIDList = 0;
    seiW.lpClass = 0;
    seiW.hkeyClass = 0;
    seiW.dwHotKey = 0;
    seiW.hProcess = hProcess;

    SHELL_execute( &seiW, SHELL_Execute16 );

    SHFree(wVerb);
    SHFree(wFile);
    SHFree(wParameters);
    SHFree(wDirectory);

    return HINSTANCE_16(seiW.hInstApp);
}
