/* $Id: kdebug.c,v 1.24 2001/04/26 03:58:32 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdebug.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  21/10/99: Created
 */

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/kd.h>
#include <internal/mm.h>
#include "../dbg/kdb.h"

/* serial debug connection */
#define DEFAULT_DEBUG_PORT      2	/* COM2 */
#define DEFAULT_DEBUG_BAUD_RATE 19200	/* 19200 Baud */

/* bochs debug output */
#define BOCHS_LOGGER_PORT (0xe9)


/* TYPEDEFS ****************************************************************/

#define ScreenDebug  (0x1)
#define SerialDebug  (0x2)
#define BochsDebug   (0x4)
#define FileLogDebug (0x8)

/* VARIABLES ***************************************************************/

BOOLEAN
__declspec(dllexport)
KdDebuggerEnabled = FALSE;		/* EXPORTED */

BOOLEAN
__declspec(dllexport)
KdDebuggerNotPresent = TRUE;		/* EXPORTED */


static BOOLEAN KdpBreakPending = FALSE;
static BOOLEAN KdpBreakRecieved = FALSE;
static ULONG KdpDebugType = ScreenDebug | BochsDebug;

/* PRIVATE FUNCTIONS ********************************************************/

static void
PrintString (char* fmt,...)
{
	char buffer[512];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	HalDisplayString (buffer);
}


VOID
KdInitSystem (
	ULONG			Reserved,
	PLOADER_PARAMETER_BLOCK	LoaderBlock
	)
{
	KD_PORT_INFORMATION PortInfo;
	ULONG Value;
	PCHAR p1, p2;

	/* set debug port default values */
	PortInfo.ComPort  = DEFAULT_DEBUG_PORT;
	PortInfo.BaudRate = DEFAULT_DEBUG_BAUD_RATE;

	/*
	 * parse kernel command line
	 */

	/* check for 'DEBUGPORT' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "DEBUGPORT", 9))
		{
			p2 += 9;
			if (*p2 != '=')
				break;
			p2++;
			if (!_strnicmp (p2, "SCREEN", 6))
			{
				p2 += 6;
				KdDebuggerEnabled = TRUE;
				KdpDebugType |= ScreenDebug;
			}
			else if (!_strnicmp (p2, "BOCHS", 5))
			{
				p2 += 5;
				KdDebuggerEnabled = TRUE;
				KdpDebugType |= BochsDebug;
			}
			else if (!_strnicmp (p2, "COM", 3))
			{
				p2 += 3;
				Value = (ULONG)atol (p2);
				if (Value > 0 && Value < 5)
				{
					KdDebuggerEnabled = TRUE;
					KdpDebugType |= SerialDebug;
					PortInfo.ComPort = Value;
				}
			}
			break;
		}
		p1 = p2;
	}
	

	
	/* check for 'BAUDRATE' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "BAUDRATE", 8))
		{
			p2 += 8;
			if (*p2 != '=')
				break;
			p2++;
			Value = (ULONG)atol (p2);
			if (Value > 0)
			{
				KdDebuggerEnabled = TRUE;
				KdpDebugType = KdpDebugType | SerialDebug;
				PortInfo.BaudRate = Value;
			}
			break;
		}
		p1 = p2;
	}

	/* Check for 'DEBUG'. Dont' accept 'DEBUGPORT'!*/
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "DEBUG", 5) &&
		    _strnicmp (p2, "DEBUGPORT", 9))
		{
			p2 += 5;
			KdDebuggerEnabled = TRUE;
			KdpDebugType = KdpDebugType | SerialDebug;
			break;
		}
		p1 = p2;
	}

	/* Check for 'NODEBUG' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "NODEBUG", 7))
		{
			p2 += 7;
			KdDebuggerEnabled = FALSE;
			break;
		}
		p1 = p2;
	}

	/* Check for 'CRASHDEBUG' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "CRASHDEBUG", 10))
		{
			p2 += 10;
			KdDebuggerEnabled = FALSE;
			break;
		}
		p1 = p2;
	}

	/* Check for 'BREAK' */
	p1 = (PCHAR)LoaderBlock->CommandLine;
	while (p1 && (p2 = strchr (p1, '/')))
	{
		p2++;
		if (!_strnicmp (p2, "BREAK", 5))
		{
			p2 += 7;
			KdpBreakPending = TRUE;
			break;
		}
		p1 = p2;
	}

#ifdef DBGPRINT_FILE_LOG
	KdpDebugType |= FileLogDebug;
	DebugLogInit();
#endif /* DBGPRINT_FILE_LOG */

	/* print some information */
	if (KdDebuggerEnabled == TRUE)
	{
		if (KdpDebugType & ScreenDebug)
		{
			PrintString ("\n   Screen debugging enabled\n\n");
		}
		if (KdpDebugType & BochsDebug)
		{
			PrintString ("\n   Bochs debugging enabled\n\n");
		}
		if (KdpDebugType & SerialDebug)
		{
			PrintString ("\n   Serial debugging enabled: COM%ld %ld Baud\n\n",
			             PortInfo.ComPort, PortInfo.BaudRate);
		}
		if (KdpDebugType & FileLogDebug)
		  {
		    PrintString("\n   File log debugging enabled\n\n");
		  }
	}
	else
		PrintString ("\n   Debugging disabled\n\n");


	/* initialize debug port */
	if (KdDebuggerEnabled && (KdpDebugType & SerialDebug))
	{
		KdPortInitialize (&PortInfo,
		                  0,
		                  0);
	}
}


ULONG KdpPrintString (PANSI_STRING String)
{
   PCH pch = String->Buffer;
   
   if (KdpDebugType & ScreenDebug)
     {
	HalDisplayString (String->Buffer);
     }
   if (KdpDebugType & SerialDebug)
     {
	while (*pch != 0)
	  {
	     if (*pch == '\n')
	       {
		  KdPortPutByte ('\r');
	       }
	     KdPortPutByte (*pch);
	     pch++;
	  }
	}
   if (KdpDebugType & BochsDebug)
     {
	while (*pch != 0)
	  {
	     if (*pch == '\n')
	       {
		  WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
	       }
	     WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *pch);
	     pch++;
	  }
     }
#ifdef DBGPRINT_FILE_LOG
   if (KdpDebugType & FileLogDebug)
     {
       DebugLogWrite(String->Buffer);
     }
#endif /* DBGPRINT_FILE_LOG */
   return (ULONG)String->Length;
}

/* PUBLIC FUNCTIONS *********************************************************/

/* NTOSKRNL.KdPollBreakIn */

BOOLEAN
STDCALL
KdPollBreakIn (
	VOID
	)
{
	BOOLEAN Result = FALSE;
	UCHAR ByteRead;

	if (KdDebuggerEnabled == FALSE || KdpDebugType != SerialDebug)
		return Result;

//	Flags = KiDisableInterrupts();

	HalDisplayString ("Waiting for kernel debugger connection...\n");

	if (KdPortPollByte (&ByteRead))
	{
		if (ByteRead == 0x62)
		{
			if (KdpBreakPending == TRUE)
			{
				KdpBreakPending = FALSE;
				KdpBreakRecieved = TRUE;
				Result = TRUE;
			}
			HalDisplayString ("   Kernel debugger connected\n");
		}
		else
		{
			HalDisplayString ("   Kernel debugger connection failed\n");
		}
	}

//	KiRestoreInterrupts (Flags);

	return Result;
}

VOID STDCALL
KeEnterKernelDebugger (VOID)
{
	HalDisplayString ("\n\n *** Entered kernel debugger ***\n");

#if 1
	for (;;)
		__asm__("hlt\n\t");
#else
   for(;;);
#endif
}

VOID STDCALL
KdSystemDebugControl(ULONG Code)
{
  /* A - Dump the entire contents of the non-paged pool. */
  if (Code == 0)
    {
      MiDebugDumpNonPagedPool(FALSE);
    }
  /* B - Bug check the system. */
  else if (Code == 1)
    {
      KeBugCheck(0);
    }
  /* 
   * C -  Dump statistics about the distribution of tagged blocks in 
   *      the non-paged pool.
   */
  else if (Code == 2)
    {
      MiDebugDumpNonPagedPoolStats(FALSE);
    }
  /* 
   * D - Dump the blocks created in the non-paged pool since the last
   * SysRq + D and SysRq + E command.
   */
  else if (Code == 3)
    {
      MiDebugDumpNonPagedPool(TRUE);
    }
  /* E - Dump statistics about the tags of newly created blocks. */
  else if (Code == 4)
    {
      MiDebugDumpNonPagedPoolStats(TRUE);
    }
  /* F */
  else if (Code == 5)
    {
    }
  /* G */
  else if (Code == 6)
    {
    }
  /* H */
  else if (Code == 7)
    {
    }
  /* I */
  else if (Code == 8)
    {
    }
  /* J */
  else if (Code == 9)
    {
    }
  /* K - Enter the system debugger. */
  else if (Code == 10)
    {
#ifdef KDBG
      KdbEnter();
#else /* KDBG */
      DbgPrint("No local kernel debugger\n");
#endif /* not KDBG */
    }
}

/* EOF */
