/* $Id: csrterm.c,v 1.1 2002/03/17 22:15:39 ea Exp $
 *
 * PROJECT    : ReactOS Operating System / POSIX+ Environment Subsystem
 * DESCRIPTION: CSRTERM - A DEC VT-100 terminal emulator for the PSX subsystem
 * DESCRIPTION: that runs in the Win32 subsystem.
 * COPYRIGHT  :	Copyright (c) 2001-2002 Emanuele Aliberti
 * LICENSE    : GNU GPL v2
 * DATE       : 2001-05-05
 * AUTHOR     : Emanuele Aliberti <ea@iol.it>
 * NOTE       : This IS a Win32 program, but will fail if the PSX subsystem
 * NOTE       : is not installed. The PSX subsystem consists of two more
 * NOTE       : files: PSXSS.EXE, PSXDLL.DLL.
 * WARNING    : If you use this program under a real NT descendant, be
 * WARNING    : sure to have disabled the PSX subsystem.
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 2002-03-16 EA  Today it actually compiled.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define NTOS_MODE_USER
#include <ntos.h>
#include <psx/lpcproto.h>

#include "vt100.h"
#include "csrterm.h"

/*** OPTIONS *********************************************************/

#define PRIVATE static

#define INPUT_QUEUE_SIZE 32

#ifdef NDEBUG
#define TRACE
#else
#define TRACE OutputDebugString(__FUNCTION__)
#endif

/*** GLOBALS *********************************************************/

PRIVATE LPCSTR MyName = "CSRTERM";
PRIVATE CSRTERM_SESSION  Session;

/*** PRIVATE FUNCTIONS ***********************************************/
VOID STDCALL Debug_Print (LPCSTR Format, ...)
{
    CHAR    Buffer [512];
    va_list ArgumentPointer;
   
    va_start(ArgumentPointer, Format);
    vsprintf(Buffer, Format, ArgumentPointer);
    va_end(ArgumentPointer);
    OutputDebugStringA (Buffer);
}
/**********************************************************************
 *	OutPort/2						PRIVATE
 *
 * DESCRIPTION
 *	Notify to PSXSS that input data is ready by sending a
 *	software interrupt on the \POSIX+\SessionPort port.
 */
PRIVATE DWORD STDCALL OutPort (PCHAR Buffer, ULONG Size)
{
    NTSTATUS           Status;
    PSX_TERMINAL_READ  TerminalRead;
TRACE;
    if (Size > 0)
    {
        /* LPC */
	TerminalRead.Header.MessageType = LPC_NEW_MESSAGE;
	TerminalRead.PsxHeader.Context = PSX_CONNECTION_TYPE_TERMINAL;
	TerminalRead.PsxHeader.Procedure = PSX_TERMINAL_INTERRUPT;
	/* Terminal I/O */
        TerminalRead.Size = Size;
        RtlCopyMemory (TerminalRead.Buffer, Buffer, Size);
#if 0
        Status = NtRequestWaitReplyPort (
			Session.ServerPort.Handle,
			& TerminalRead
			/* FIXME */
			);
#endif
        if (!NT_SUCCESS(Status))
        {
                vtprintf ("%s: %s: NtRequestWaitReplyPort failed with %08x\n",
                    MyName, __FUNCTION__, Status);
                return 0;
	}
    }
    return Size;
}
/**********************************************************************
 *	ProcessConnectionRequest/1				PRIVATE
 *
 * DESCRIPTION
 *	Initialize our data for managing the control connection
 *	initiated by the PSXSS.EXE process.
 */
PRIVATE NTSTATUS STDCALL ProcessConnectionRequest (PPSX_MAX_MESSAGE Request)
{
TRACE;
    Session.SsLinkIsActive = TRUE;
    return STATUS_SUCCESS;
}
/**********************************************************************
 *	ProcessRequest/1					PRIVATE
 *
 * DESCRIPTION
 *
 */
PRIVATE NTSTATUS STDCALL ProcessRequest (PPSX_MAX_MESSAGE Request)
{
TRACE;
    /* TODO */
    vtprintf("TEST VT-100\n");

    return STATUS_SUCCESS;
}
/**********************************************************************
 *	PsxSessionPortListener/1				PRIVATE
 *
 * DESCRIPTION
 *	Manage messages from the PSXSS, that is LPC messages we get 
 *	from the PSXSS process to our \POSIX+\Sessions\P<pid> port.
 *
 * NOTE
 *	This function is the thread 's entry point created in
 *	CreateSessionObiects().
 */
PRIVATE DWORD STDCALL PsxSessionPortListener (LPVOID Arg)
{
    NTSTATUS         Status;
    LPC_TYPE         RequestType;
    PSX_MAX_MESSAGE  Request;
    PPSX_MAX_MESSAGE Reply = NULL;
    BOOL             NullReply = FALSE;

TRACE;

    while (TRUE)
    {
        Reply = NULL;
        while (!NullReply)
        {
            Status = NtReplyWaitReceivePort (
                        Session.Port.Handle,
                        0,
                        (PLPC_MESSAGE) Reply,
                        (PLPC_MESSAGE) & Request
                        );
            if (!NT_SUCCESS(Status))
            {
                break;
            }
            RequestType = PORT_MESSAGE_TYPE(Request);
            switch (RequestType)
            {
            case LPC_CONNECTION_REQUEST:
                ProcessConnectionRequest (& Request);
                NullReply = TRUE;
                continue;
            case LPC_CLIENT_DIED:
            case LPC_PORT_CLOSED:
            case LPC_DEBUG_EVENT:
            case LPC_ERROR_EVENT:
            case LPC_EXCEPTION:
                NullReply = TRUE;
                continue;
            default:
                if (RequestType != LPC_REQUEST)
                {
                    NullReply = TRUE;
                    continue;
                }
            }
            Reply = & Request;
            Reply->PsxHeader.Status = ProcessRequest (& Request);
            NullReply = FALSE;
        }
        if ((STATUS_INVALID_HANDLE == Status) ||
            (STATUS_OBJECT_TYPE_MISMATCH == Status))
        {
            break;
        }
    }
    Session.SsLinkIsActive = FALSE;
    TerminateThread (GetCurrentThread(), Status);
}
/**********************************************************************
 *	CreateSessionObiects/1					PRIVATE
 *
 * DESCRIPTION
 *	Create the session objects which are mananged by our side:
 *
 *	\POSIX+\Sessions\P<pid>
 *	\POSIX+\Sessions\D<pid>
 */
PRIVATE NTSTATUS STDCALL CreateSessionObjects (DWORD Pid)
{
    NTSTATUS          Status;
    ULONG             Id = 0;
    OBJECT_ATTRIBUTES Oa;

TRACE;


    /* Critical section */
    Status = RtlInitializeCriticalSection (& Session.Lock);
    if (!NT_SUCCESS(Status))
    {
        vtprintf (
            "%s: %s: RtlInitializeCriticalSection failed with %08x\n",
            MyName, __FUNCTION__, Status);
        return Status;
    }
    /* Port and port management thread */
    swprintf (
        Session.Port.NameBuffer,
        PSX_NS_SESSION_PORT_TEMPLATE,
        PSX_NS_SUBSYSTEM_DIRECTORY_NAME,
        PSX_NS_SESSION_DIRECTORY_NAME,
        Pid
        );
    RtlInitUnicodeString (& Session.Port.Name, Session.Port.NameBuffer);
    InitializeObjectAttributes (& Oa, & Session.Port.Name, 0, NULL, NULL);
    Status = NtCreatePort (& Session.Port.Handle, & Oa, 0, 0, 0x10000);
    if (!NT_SUCCESS(Status))
    {
        RtlDeleteCriticalSection (& Session.Lock);
        vtprintf ("%s: %s: NtCreatePort failed with %08x\n",
            MyName, __FUNCTION__, Status);
        return Status;
    }
    Session.Port.Thread.Handle =
        CreateThread (
            NULL,
            0,
            PsxSessionPortListener,
            0,
            CREATE_SUSPENDED,
            & Session.Port.Thread.Id
            );
    if ((HANDLE) NULL == Session.Port.Thread.Handle)
    {
        Status = (NTSTATUS) GetLastError();
        NtClose (Session.Port.Handle);
        RtlDeleteCriticalSection (& Session.Lock);
        vtprintf ("%s: %s: CreateThread failed with %d\n",
            MyName, __FUNCTION__, Status);
        return Status;
    }
    /* Section */
    swprintf (
        Session.Section.NameBuffer,
        PSX_NS_SESSION_DATA_TEMPLATE,
        PSX_NS_SUBSYSTEM_DIRECTORY_NAME,
        PSX_NS_SESSION_DIRECTORY_NAME,
        Pid
    );
    RtlInitUnicodeString (& Session.Section.Name, Session.Section.NameBuffer); 
    InitializeObjectAttributes (& Oa, & Session.Section.Name, 0, 0, 0);
    Status = NtCreateSection (
                & Session.Section.Handle,
                0, /* DesiredAccess */
                & Oa,
                NULL, /* SectionSize OPTIONAL */
                PAGE_READWRITE, /* Protect 4 */
                SEC_COMMIT, /* Attributes */
                0 /* FileHandle: 0=pagefile.sys */
                );
    if (!NT_SUCCESS(Status))
	{
		NtClose (Session.Port.Handle);
		NtTerminateThread (Session.Port.Thread.Handle, Status);
		RtlDeleteCriticalSection (& Session.Lock);
		vtprintf ("%s: %s: NtCreateSection failed with %08x\n",
                    MyName, __FUNCTION__, Status);
		return Status;
	}
	Session.Section.BaseAddress = NULL;
	Session.Section.ViewSize = 0;
	Status = NtMapViewOfSection (
			Session.Section.Handle,
			NtCurrentProcess(),
			& Session.Section.BaseAddress,
			0, /* ZeroBits */
			0, /* Commitsize */
			0, /* SectionOffset */
			& Session.Section.ViewSize,
			ViewUnmap,
			0, /* AllocationType */
			PAGE_READWRITE /* Protect 4 */
			);
	if (!NT_SUCCESS(Status))
	{
		NtClose (Session.Port.Handle);
		NtTerminateThread (Session.Port.Thread.Handle, Status);
		NtClose (Session.Section.Handle);
		RtlDeleteCriticalSection (& Session.Lock);
		vtprintf ("%s: %s: NtMapViewOfSection failed with %08x\n",
                    MyName, __FUNCTION__, Status);
		return Status;
	}
	return Status;
}

/**********************************************************************
 *	CreateTerminalToPsxChannel/0					PRIVATE
 *
 * DESCRIPTION
 *
 */
PRIVATE NTSTATUS STDCALL CreateTerminalToPsxChannel (VOID)
{
    PSX_CONNECT_PORT_DATA        ConnectData;
    ULONG                        ConnectDataLength = sizeof ConnectData;
    SECURITY_QUALITY_OF_SERVICE  Sqos;
    NTSTATUS                     Status;

TRACE;


    /*
     * Initialize the connection data object before
     * calling PSXSS.
     */
    ConnectData.ConnectionType = PSX_CONNECTION_TYPE_TERMINAL;
    ConnectData.Version = PSX_LPC_PROTOCOL_VERSION;
    /*
     * Try connecting to \POSIX+\SessionPort.
     */
    Status = NtConnectPort (
                & Session.ServerPort.Handle,
                & Session.ServerPort.Name,
                & Sqos,
                NULL,
                NULL,
                0,
                & ConnectData,
                & ConnectDataLength
                );
    if (STATUS_SUCCESS != Status)
    {
        vtprintf ("%s: %s: NtConnectPort failed with %08x\n",
            MyName, __FUNCTION__, Status);
        return Status;
    }
    Session.Identifier = ConnectData.PortIdentifier;
    return STATUS_SUCCESS;
}

/**********************************************************************
 *	InitializeSsIoChannel					PRIVATE
 *
 * DESCRIPTION
 *	Create our objects in the system name space
 *	(CreateSessionObjects) and then connect to the session port
 *	(CreateControChannel).
 */
PRIVATE NTSTATUS STDCALL InitializeSsIoChannel (VOID)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	DWORD     Pid = GetCurrentProcessId();

TRACE;


	Status = CreateSessionObjects (Pid);
	if (STATUS_SUCCESS != Status)
	{
		vtprintf ("%s: %s: CreateSessionObjects failed with %08x\n",
                    MyName, __FUNCTION__, Status);
		return Status;
	}
	Status = CreateTerminalToPsxChannel ();
	if (STATUS_SUCCESS != Status)
	{
		vtprintf ("%s: %s: CreateTerminalToPsxChannel failed with %08x\n",
		    MyName, __FUNCTION__, Status);
		return Status;
	}
	return STATUS_SUCCESS;
}
/**********************************************************************
 *	PsxCreateLeaderProcess/1				PRIVATE
 *
 * DESCRIPTION
 *	Create a new PSXSS process.
 */
PRIVATE NTSTATUS STDCALL PsxCreateLeaderProcess (char * Command)
{

TRACE;

	if (NULL == Command)
	{
		Command = "/bin/sh";
	}
	/* TODO: request PSXSS to init the process slot */
	return STATUS_NOT_IMPLEMENTED;
}
/**********************************************************************
 *	PrintInformationProcess/0
 *
 * DESCRIPTION
 */
PRIVATE VOID STDCALL PrintInformationProcess (VOID)
{

TRACE;

	vtputs ("Leader:");
	vtprintf ("  UniqueProcess %08x\n", Session.Client.UniqueProcess);
	vtprintf ("   UniqueThread %08x\n", Session.Client.UniqueThread);
}
/**********************************************************************
 *	PostMortem/0
 *
 * DESCRIPTION
 */
PRIVATE INT STDCALL PostMortem (VOID)
{
	DWORD ExitCode;

TRACE;


	PrintInformationProcess ();
	if (TRUE == GetExitCodeProcess (Session.Client.UniqueProcess, & ExitCode))
	{
		vtprintf ("       ExitCode %d\n", ExitCode);
	}
	return 0;
}
/**********************************************************************
 *	InputTerminalEmulator/0
 *
 * DESCRIPTION
 *	Process user terminal input.
 *
 * NOTE
 *	This code is run in the main thread.
 */
PRIVATE BOOL STDCALL InputTerminalEmulator (VOID)
{
    HANDLE        StandardInput;
    INPUT_RECORD  InputRecord [INPUT_QUEUE_SIZE];
    DWORD         NumberOfEventsRead = 0;
    INT           CurrentEvent;


TRACE;

    StandardInput = GetStdHandle (STD_INPUT_HANDLE);
    if (INVALID_HANDLE_VALUE == StandardInput)
    {
        return FALSE;
    }
    while ((TRUE == Session.SsLinkIsActive) && 
           ReadConsoleInput (
            StandardInput,
            InputRecord,
	    (sizeof InputRecord) / sizeof (INPUT_RECORD),
	    & NumberOfEventsRead
	    ))
    {
        for (  CurrentEvent = 0;
	       (CurrentEvent < NumberOfEventsRead);
	       CurrentEvent ++
	       )
        {
            switch (InputRecord [CurrentEvent].EventType)
            {
            case KEY_EVENT:
                OutPort (& InputRecord [CurrentEvent].Event.KeyEvent.uChar.AsciiChar, 1);
		break;
            case MOUSE_EVENT:
	        /* TODO: send a sequence of move cursor codes */
                /* InputRecord [CurrentEvent].Event.MouseEvent; */
		break;
            case WINDOW_BUFFER_SIZE_EVENT:
                /* TODO: send a SIGWINCH signal to the leader process. */
		/* InputRecord [CurrentEvent].Event.WindowBufferSizeEvent.dwSize; */
		break;
            /* Next events should be ignored. */
            case MENU_EVENT:
		vtprintf ("%s: %s: MENU_EVENT received from CSRSS\n", MyName, __FUNCTION__);
            case FOCUS_EVENT:
		vtprintf ("%s: %s: FOCUS_EVENT received from CSRSS\n", MyName, __FUNCTION__);
		break;
	    }
        }
	NumberOfEventsRead = 0;
    }
    return TRUE;
}
/**********************************************************************
 *	Startup/1
 *
 * DESCRIPTION
 *	Initialize the program.
 */
PRIVATE VOID STDCALL Startup (LPSTR Command)
{
	NTSTATUS Status;
	DWORD    ThreadId;


TRACE;

	/* PSX process info */
	Session.Client.UniqueProcess = INVALID_HANDLE_VALUE;
	Session.Client.UniqueThread  = INVALID_HANDLE_VALUE;
	/* Initialize the VT-100 emulator */
	vtInitVT100 ();	
	/* Connect to PSXSS */
	Status = InitializeSsIoChannel ();
	if (!NT_SUCCESS(Status))
	{
		vtprintf ("%s: failed to connect to PSXSS (Status=%08x)!\n",
			MyName, Status);
		exit (EXIT_FAILURE);
	}
	/* Create the leading process for this session */
	Status = PsxCreateLeaderProcess (Command);
	if (!NT_SUCCESS(Status))
	{
		vtprintf ("%s: failed to create the PSX process (Status=%08x)!\n",
			MyName, Status);
		exit (EXIT_FAILURE);
	}
}
/**********************************************************************
 *	Shutdown/0					PRIVATE
 *
 * DESCRIPTION
 *	Shutdown the program.
 */
PRIVATE INT STDCALL Shutdown (VOID)
{

TRACE;

    /* TODO: try exiting cleanly: close any open resource */
    /* TODO: notify PSXSS the session is terminating */
    RtlDeleteCriticalSection (& Session.Lock);
    return PostMortem ();
}
/**********************************************************************
 *
 *	ENTRY POINT					PUBLIC
 *
 *********************************************************************/
int main (int argc, char * argv [])
{

TRACE;

    Startup (argv[1]); /* Initialization */
    InputTerminalEmulator (); /* Process user input */
    return Shutdown ();
}
/* EOF */
