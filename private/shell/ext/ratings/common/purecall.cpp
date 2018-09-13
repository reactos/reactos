/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* PURECALL.C -- Implementation of __purecall function.
 *
 * History:
 *	04/11/94	gregj	Created
 */

#include "npcommon.h"

extern "C" {

/*
 * This function serves to avoid linking CRT code like assert etc.
 * we really don;t do anything when pure virtual function is not redefined
 */

int __cdecl  _purecall(void)
{
#ifdef DEBUG
	DebugBreak();
#endif

	return(FALSE);
}

int __cdecl atexit(void (__cdecl *)(void))
{
	return 0;
}

};
