/* $Id: internal.h,v 1.6 2004/05/07 11:18:53 ekohl Exp $ 
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
CreateUserHive (LPCWSTR lpKeyName);

#endif /* _INTERNAL_H */

/* EOF */
