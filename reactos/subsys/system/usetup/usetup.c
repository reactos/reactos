/* $Id: usetup.c,v 1.1 2002/09/08 18:28:43 ekohl Exp $
 *
 * smss.c - Session Manager
 * 
 * ReactOS Operating System
 * 
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
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 * 	19990529 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>
#include <ddk/ntddblue.h>

#include <ntdll/rtl.h>

#include <ntos/keyboard.h>

#include "usetup.h"


HANDLE ProcessHeap;

/* FUNCTIONS ****************************************************************/

void
DisplayString(LPCWSTR lpwString)
{
  UNICODE_STRING us;

  RtlInitUnicodeString(&us, lpwString);
  NtDisplayString(&us);
}


void
PrintString(char* fmt,...)
{
  char buffer[512];
  va_list ap;
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  RtlInitAnsiString(&AnsiString, buffer);
  RtlAnsiStringToUnicodeString(&UnicodeString,
			       &AnsiString,
			       TRUE);
  NtDisplayString(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);
}


/*
 * Confirm quit setup
 * RETURNS
 *	TRUE: Quit setup.
 *	FALSE: Don't quit setup.
 */
static BOOL
ConfirmQuit(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;
  SHORT yTop;
  SHORT xLeft;
  BOOL Result = FALSE;
  PUSHORT pAttributes = NULL;
  PUCHAR pCharacters = NULL;
  COORD Pos;

  GetScreenSize(&xScreen, &yScreen);
  yTop = (yScreen - 10) / 2;
  xLeft = (xScreen - 52) / 2;

  /* Save screen */
  Pos.X = 0;
  Pos.Y = 0;
  pAttributes = (PUSHORT)RtlAllocateHeap(RtlGetProcessHeap(),
					 0,
					 xScreen * yScreen * sizeof(USHORT));
  ReadConsoleOutputAttributes(pAttributes,
			      xScreen * yScreen,
			      Pos,
			      NULL);
  pCharacters = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
					0,
					xScreen * yScreen * sizeof(UCHAR));
  ReadConsoleOutputCharacters(pCharacters,
			      xScreen * yScreen,
			      Pos,
			      NULL);

  /* Draw popup window */
  SetTextXY(xLeft, yTop,
	    "+----------------------------------------------------+");
  SetTextXY(xLeft, yTop + 1,
	    "| ReactOS 0.0.20 is not completely installed on your |");
  SetTextXY(xLeft, yTop + 2,
	    "| computer. If you quit Setup now, you will need to  |");
  SetTextXY(xLeft, yTop + 3,
	    "| run Setup again to install ReactOS.                |");
  SetTextXY(xLeft, yTop + 4,
	    "|                                                    |");
  SetTextXY(xLeft, yTop + 5,
	    "|   * Press ENTER to continue Setup.                 |");
  SetTextXY(xLeft, yTop + 6,
	    "|   * Press F3 to quit Setup.                        |");
  SetTextXY(xLeft, yTop + 7,
	    "+----------------------------------------------------+");
  SetTextXY(xLeft, yTop + 8,
	    "| F3= Quit  ENTER = Continue                         |");
  SetTextXY(xLeft, yTop + 9,
	    "+----------------------------------------------------+");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  Result = TRUE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  Result = FALSE;
	  break;
	}
    }

  /* Restore screen */
  WriteConsoleOutputAttributes(pAttributes,
			       xScreen * yScreen,
			       Pos,
			       NULL);

  WriteConsoleOutputCharacters(pCharacters,
			       xScreen * yScreen,
			       Pos);
  RtlFreeHeap(RtlGetProcessHeap(),
	      0,
	      pAttributes);
  RtlFreeHeap(RtlGetProcessHeap(),
	      0,
	      pCharacters);

  return(Result);
}



static BOOL
InstallIntroPage(PINPUT_RECORD Ir)
{
  ClearScreen();

  SetTextXY(4, 3, " ReactOS 0.0.20 Setup ");
  SetTextXY(4, 4, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");

  SetTextXY(6, 8, "Install intro page");

#if 0
  SetTextXY(6, 10, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 11, "computer and prepairs the second part of the setup.");

  SetTextXY(8, 14, "\xf9  Press ENTER to start the ReactOS setup.");

  SetTextXY(8, 17, "\xf9  Press F3 to quit without installing ReactOS.");
#endif

  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(FALSE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	break;
    }

  return(TRUE);
}


#if 0
static BOOL
RepairIntroPage(PINPUT_RECORD Ir)
{
  ClearScreen();

}
#endif


/*
 * First setup page
 * RETURNS
 *	TRUE: setup/repair completed successfully
 *	FALSE: setup/repair terminated by user
 */
static BOOL
IntroPage(PINPUT_RECORD Ir)
{
CHECKPOINT1;
  ClearScreen();

  SetTextXY(4, 3, " ReactOS 0.0.20 Setup ");
  SetTextXY(4, 4, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");

  SetTextXY(6, 8, "Welcome to the ReactOS Setup");

  SetTextXY(6, 10, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 11, "computer and prepares the second part of the setup.");

  SetTextXY(8, 14, "\xf9  Press ENTER to install ReactOS.");

#if 0
  SetTextXY(8, 16, "\xf9  Press R to repair ReactOS.");
#endif

  SetTextXY(8, 17, "\xf9  Press F3 to quit without installing ReactOS.");

  SetStatusText("   ENTER = Continue   F3 = Quit");
CHECKPOINT1;

  while(TRUE)
    {
CHECKPOINT1;
      ConInKey(Ir);
CHECKPOINT1;

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(FALSE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(InstallIntroPage(Ir));
	}
#if 0
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R')
	{
	  return(RepairIntroPage(Ir));
	}
#endif
    }

  return(FALSE);
}


static VOID
QuitPage(PINPUT_RECORD Ir)
{
  ClearScreen();

  SetTextXY(4, 3, " ReactOS 0.0.20 Setup ");
  SetTextXY(4, 4, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");


  SetTextXY(6, 10, "ReactOS is not completely installed");

  SetTextXY(8, 10, "Remove floppy disk from drive A:.");
  SetTextXY(9, 10, "Remove cdrom from cdrom drive.");

  SetTextXY(11, 10, "Press ENTER to reboot your computer.");

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return;
	}
    }
}


static VOID
SuccessPage(PINPUT_RECORD Ir)
{
  ClearScreen();

  SetTextXY(4, 3, " ReactOS 0.0.20 Setup ");
  SetTextXY(4, 4, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");


  SetTextXY(6, 10, "The basic components of ReactOS have been installed successfully.");

  SetTextXY(8, 10, "Remove floppy disk from drive A:.");
  SetTextXY(9, 10, "Remove cdrom from cdrom drive.");

  SetTextXY(11, 10, "Press ENTER to reboot your computer.");

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return;
	}
    }
}


VOID
NtProcessStartup(PPEB Peb)
{
  NTSTATUS Status;
  INPUT_RECORD Ir;

  RtlNormalizeProcessParams(Peb->ProcessParameters);

  ProcessHeap = Peb->ProcessHeap;

  Status = AllocConsole();
  if (!NT_SUCCESS(Status))
    {
      PrintString("Console initialization failed! (Status %lx)\n", Status);
      goto ByeBye;
    }

CHECKPOINT1;
  if (IntroPage(&Ir) == TRUE)
    {
      /* Display success message */
      SetCursorXY(5, 49);
      ConOutPrintf("Press any key to continue\n");
      ConInKey(&Ir);
//      SuccessPage(&Ir)
    }
  else
    {
      /* Display termination message */
      QuitPage(&Ir);
    }

  /* Reboot */

ConsoleExit:
  FreeConsole();

  PrintString("*** System halted ***\n", Status);
  for(;;);

ByeBye:
  /* Raise a hard error (crash the system/BSOD) */
  NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		   0,0,0,0,0);

//   NtTerminateProcess(NtCurrentProcess(), 0);
}

/* EOF */
