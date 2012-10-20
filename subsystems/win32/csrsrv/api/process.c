/*
 * subsystems/win32/csrss/csrsrv/api/process.c
 *
 * 
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>
    


/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *	CSRSS API
 *********************************************************************/

/***
 *** Some APIs from here will go to basesrv.dll, some others to winsrv.dll.
 *** Furthermore, this structure uses the old definition of APIs list.
 *** The new one is in fact three arrays, one of APIs pointers, one other of
 *** corresponding indexes, and the third one of names (not very efficient...).
 ***/

/* EOF */
