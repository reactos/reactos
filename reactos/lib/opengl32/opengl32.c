/* $Id: opengl32.c,v 1.3 2004/02/01 17:18:47 royce Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.c
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include "opengl32.h"

#define EXPORT __declspec(dllexport)

const char* OPENGL32_funcnames[GLIDX_COUNT] =
{
#define X(X) #X,
	GLFUNCS_MACRO
#undef X
};

static void OPENGL32_ThreadDetach()
{
	/* FIXME - do we need to release some HDC or something? */
	lpvData = (OPENGL32_ThreadData*)TlsGetValue ( OPENGL32_tls );
	if ( lpvData != NULL )
		LocalFree((HLOCAL) lpvData );
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	GLTHREADDATA* lpData = NULL;
	printf("OpenGL32.DLL DllMain called!\n");

	switch ( Reason )
	{
	/* The DLL is loading due to process 
	 * initialization or a call to LoadLibrary.
	 */
	case DLL_PROCESS_ATTACH:
		OPENGL32_tls = TlsAlloc();
		if ( 0xFFFFFFFF == OPENGL32_tls )
			return FALSE;
		OPENGL32_processdata.funclist_count = 1;
		OPENGL32_processdata.list = malloc ( sizeof(GLFUNCLIST*) * OPENGL32_processdata.funclist_count );
		OPENGL32_processdata.list[0] = malloc ( sizeof(GLFUNCLIST) );
		memset ( OPENGL32_processdata.list[0], 0, sizeof(GLFUNCLIST) );
		/* FIXME - load mesa32 into first funclist */
		/* FIXME - get list of ICDs from registry */
		// No break: Initialize the index for first thread.

	/* The attached process creates a new thread. */
	case DLL_THREAD_ATTACH:
		lpData = (GLTHREADDATA*)LocalAlloc(LPTR, sizeof(GLTHREADDATA));
		if ( lpData != NULL )
		{
			memset ( lpData, 0, sizeof(GLTHREADDATA) );
			(void)TlsSetValue ( OPENGL32_tls, lpData );
		}
		lpData->hdc = NULL;
		/* FIXME - defaulting to mesa3d, but shouldn't */
		lpData->list = OPENGL32_processdata.list[0];
		break;

	/* The thread of the attached process terminates. */
	case DLL_THREAD_DETACH:
		/* Release the allocated memory for this thread.*/
		OPENGL32_ThreadDetach();
		break;

	/* DLL unload due to process termination or FreeLibrary. */
	case DLL_PROCESS_DETACH:
		OPENGL32_ThreadDetach();
		TlsFree(OPENGL32_tls);
		break;
	}
	return TRUE;
}

