/* $Id: internal.h,v 1.4 2004/01/16 15:31:53 ekohl Exp $ 
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

/* directory.h */
BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath);

/* misc.h */
LPWSTR
AppendBackslash (LPWSTR String);

BOOL
AppendSystemPostfix (LPWSTR lpName,
		     DWORD dwMaxLength);


/* registry.h */
BOOL
CreateUserHive (LPCWSTR lpKeyName);

#endif /* _INTERNAL_H */

/* EOF */
