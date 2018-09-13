/*	File: D:\WACKER\tdll\assert.h (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

/*
This file is slated for destruction.  Well not exactly.  It will be
renamed to something else (possibly debug.h).  Anyways, here are 
some rules for the road.  The nature of these functions are such that
they go away in the production version.  To "communicate" this idea
to the casual reader, we are prefixing the name of the macro with
Dbg to denote that this is a debug thingy.  Assert will reamain assert
since its meaning and intention are well known to C programmers.  Also,
make sure the function in assert.c compiles to an empty function in
when built as a production version.  Other functions in assert.c already
do this do look to them for an example. - mrw
*/

#if !defined(INCL_ASSERT)
#define INCL_ASSERT

void DoAssertDebug(TCHAR *file, int line);
void __cdecl DoDbgOutStr(TCHAR *achFmt, ...);
void DoShowLastError(const TCHAR *file, const int line);

#if !defined(NDEBUG)
	#define assert(X) if (!(X)) DoAssertDebug(TEXT(__FILE__), __LINE__)

	#if defined(DEBUGSTR)
		#define DbgOutStr(A,B,C,D,E,F) DoDbgOutStr(A,B,C,D,E,F)

	#else
		#define DbgOutStr(A,B,C,D,E,F)

	#endif

	#define DbgShowLastError()	DoShowLastError(TEXT(__FILE__), __LINE__)

#else
	#define assert(X)
	#define DbgOutStr(A,B,C,D,E,F)
	#define DbgShowLastError()

#endif

#endif
