/* $Id: main.cpp,v 1.1 2001/01/27 22:38:43 ea Exp $
 *
 * FILE       : main.cpp
 * AUTHOR     : unknown (sources found on www.telnet.org)
 * PROJECT    : ReactOS Operating System
 * DESCRIPTION: telnet client for the W32 subsystem
 * DATE       : 2001-01-21
 * REVISIONS
 *	2001-02-21 ea	Modified to compile under 0.0.16 src tree
 *	2001-02-27 ea	If run with no argument, it asks for a hostname.
 */
///////////////////////////////////////////////////////////////////////////////
//
// File:
//        main.cpp
// 
// Purpose:
//        This file provdes the main entry point for the project, and all the
//        global scope support routines.
//
// Notes:
//        This file expects to be linked without the C-Runtime. If compiling,
//        please force the entry point symbol to be "main", and do not link in
//        the default librarys.
//        This means that no c-runtime functions can be used anywhere in the
//        project. I expect this will also exclude any MFC based additions.
//
///////////////////////////////////////////////////////////////////////////////

#include <winsock.h>
#include <windows.h>
#include <process.h>
#include <stdlib.h>

#include "telnet.h"

///////////////////////////////////////////////////////////////////////////////

//
// Our simple replacement for the c-runtime includes getting the StandardInput,
// StandardOutput & StandardError handles, and providing new and delete operators, that work
// with the win32 heap functions.
//

//
// standard handles needed for CRT emulation
//
HANDLE hHeap;
HANDLE StandardInput;
HANDLE StandardOutput;
HANDLE StandardError;

//
// new will use the win32 heap functions.
//
void* operator new(unsigned int nSize)
{
  return HeapAlloc(hHeap,0,nSize);
}

//
// delete operator provides all memory de-allocation.
// HeapFree doesn't accept NULL.
//
void operator delete(void* pMem)
{
  if(pMem) HeapFree(hHeap,0,pMem);
}



void err(char const* s, ...)
{
	char	buf [1024];
	DWORD	nout;
	
	wvsprintf (buf, s, (char*)(s + sizeof(int)));
	WriteFile (StandardError,"Error: ", 7, & nout, NULL);
	WriteFile (StandardError, buf, lstrlen(buf), & nout, NULL);
	WriteFile (StandardError, "\r\n\r\n", 4, & nout, NULL);
#ifdef _DEBUG
	OutputDebugString(buf);
	OutputDebugString("\n");
#endif
	ExitProcess (ERROR_SUCCESS);
}


int main(int argc, char * argv[])
{
	WSADATA	wd;
	int	errn;
	char	name [256] = {'\0'};
	short	port = IPPORT_TELNET; /* default tcp port */

	///////////////////////////////////////
	// CRT emulation init
	// Get the IO handles
	StandardInput = GetStdHandle(STD_INPUT_HANDLE);
	StandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	StandardError = GetStdHandle(STD_ERROR_HANDLE);

	// Get the heap 
	hHeap = GetProcessHeap();

	// Explicitly force the console into a good mode (actually all we are doing is turning
	// mouse input off.
	SetConsoleMode (
		StandardInput,
		(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT)
		);

	///////////////////////////////////////
	// Init winsock

	if (errn = WSAStartup (0x0101, & wd))
	{
		err(sockmsg(errn));
	}

	/* hostname */
	if (1 < argc)
	{
		lstrcpy (name, argv [1]);
	}
	/*
	 * Default port is IPPORT_TELNET.
	 * User may hand one.
	 */
	if (3 == argc)
	{
		port = atoi (argv[2]);
		if (port <= 0)
		{
			struct servent * service = NULL;
			
			service = getservbyname (argv[2], "tcp");
			if (NULL == service)
			{
				err("Invalid service name specified");
			}
			port = service->s_port;
		}
		else
		{
			err("Invalid port specified");
		}
	}
	/* Too many arguments */
	if (3 < argc)
	{
		err("Usage: telnet <hostname> [<port>]");
	}
	/* No argument */
	if (1 == argc)
	{
		DWORD Count;
		char *c;
		
		WriteFile (StandardError,"host: ", 6, & Count, NULL);
		ReadFile (StandardInput, name, sizeof name, & Count, NULL);
		c = name;
		while (*c > ' ') ++ c;
		*c = '\0';
	}

	// guess what this does.
	telnet (name, port);

	//Bye bye...
	WSACleanup ();

	// Exit process terminates any waiting threads.
	// (Its the CRT that makes a process close when the main thread exits.
	// The WinAPI will leave the process as is for as long as it has a 
	// thread any thread.
	ExitProcess (EXIT_SUCCESS);
}

/* EOF */
