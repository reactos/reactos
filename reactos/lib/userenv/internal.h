/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/internal.h
 * PURPOSE:         internal stuff
 * PROGRAMMER:      Eric Kohl
 */

#ifndef _INTERNAL_H
#define _INTERNAL_H

/* debug.h */
void
DebugPrint (char* fmt,...);

#define DPRINT1 DebugPrint("(%s:%d) ",__FILE__,__LINE__), DebugPrint
#define CHECKPOINT1 do { DebugPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT
#endif	/* __GNUC__ */
#define CHECKPOINT

/* directory.c */
BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath);

BOOL
CreateDirectoryPath (LPCWSTR lpPathName,
		     LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL
RemoveDirectoryPath (LPCWSTR lpPathName);

/* misc.c */
typedef struct _DYN_FUNCS
{
  HMODULE hModule;
  union
  {
    PVOID foo;
    struct
    {
      HRESULT (STDCALL *CoInitialize)(LPVOID pvReserved);
      HRESULT (STDCALL *CoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);
      HRESULT (STDCALL *CoUninitialize)(VOID);
    };
  } fn;
} DYN_FUNCS, *PDYN_FUNCS;

typedef struct _DYN_MODULE
{
  LPWSTR Library;    /* dll file name */
  LPSTR Functions[]; /* function names */
} DYN_MODULE, *PDYN_MODULE;

extern DYN_MODULE DynOle32;

BOOL
LoadDynamicImports(PDYN_MODULE Module, PDYN_FUNCS DynFuncs);

VOID
UnloadDynamicImports(PDYN_FUNCS DynFuncs);

LPWSTR
AppendBackslash (LPWSTR String);

BOOL
GetUserSidFromToken (HANDLE hToken,
		     PUNICODE_STRING SidString);

/* profile.c */
BOOL
AppendSystemPostfix (LPWSTR lpName,
		     DWORD dwMaxLength);

/* registry.c */
BOOL
CreateUserHive (LPCWSTR lpKeyName,
		LPCWSTR lpProfilePath);

/* setup.c */
BOOL
UpdateUsersShellFolderSettings(LPCWSTR lpUserProfilePath,
			       HKEY hUserKey);

/* userenv.c */
extern HINSTANCE hInstance;

#endif /* _INTERNAL_H */

/* EOF */
