/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: usetup.c,v 1.5 2002/10/25 22:08:21 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user-mode setup application
 * FILE:        subsys/system/usetup/usetup.c
 * PURPOSE:     setup application
 * PROGRAMMERS: Eric Kohl (ekohl@rz-online.de)
 */

#include <ddk/ntddk.h>
#include <ddk/ntddblue.h>
#include <ddk/ntddscsi.h>

#include <ntdll/rtl.h>

#include <ntos/keyboard.h>

#include "usetup.h"
#include "partlist.h"


#define INTRO_PAGE			0
#define INSTALL_INTRO_PAGE		1

#define SELECT_PARTITION_PAGE		3
#define SELECT_FILE_SYSTEM_PAGE		4
#define CHECK_FILE_SYSTEM_PAGE		5
#define PREPARE_COPY_PAGE		6
#define INSTALL_DIRECTORY_PAGE		7
#define FILE_COPY_PAGE			8
#define INIT_SYSTEM_PAGE		9

#define SUCCESS_PAGE			100
#define QUIT_PAGE			101
#define REBOOT_PAGE			102


/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;

BOOL PartDataValid = FALSE;
PARTDATA PartData;

CHAR InstallDir[51];


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
#if 0
  Pos.X = 0;
  Pos.Y = 0;
  pAttributes = (PUSHORT)RtlAllocateHeap(ProcessHeap,
					 0,
					 xScreen * yScreen * sizeof(USHORT));
CHECKPOINT1;
DPRINT1("pAttributes %p\n", pAttributes);
  ReadConsoleOutputAttributes(pAttributes,
			      xScreen * yScreen,
			      Pos,
			      NULL);
CHECKPOINT1;
  pCharacters = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					0,
					xScreen * yScreen * sizeof(UCHAR));
CHECKPOINT1;
  ReadConsoleOutputCharacters(pCharacters,
			      xScreen * yScreen,
			      Pos,
			      NULL);
CHECKPOINT1;
#endif

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
#if 0
CHECKPOINT1;
  WriteConsoleOutputAttributes(pAttributes,
			       xScreen * yScreen,
			       Pos,
			       NULL);
CHECKPOINT1;

  WriteConsoleOutputCharacters(pCharacters,
			       xScreen * yScreen,
			       Pos);
CHECKPOINT1;

  RtlFreeHeap(ProcessHeap,
	      0,
	      pAttributes);
  RtlFreeHeap(ProcessHeap,
	      0,
	      pCharacters);
#endif

  return(Result);
}







#if 0
static ULONG
RepairIntroPage(PINPUT_RECORD Ir)
{

}
#endif


/*
 * First setup page
 * RETURNS
 *	TRUE: setup/repair completed successfully
 *	FALSE: setup/repair terminated by user
 */
static ULONG
IntroPage(PINPUT_RECORD Ir)
{
  SetHighlightedTextXY(6, 8, "Welcome to the ReactOS Setup");

  SetTextXY(6, 11, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 12, "computer and prepares the second part of the setup.");

  SetTextXY(8, 15, "\xf9  Press ENTER to install ReactOS.");

#if 0
  SetTextXY(8, 17, "\xf9  Press R to repair ReactOS.");
  SetTextXY(8, 19, "\xf9  Press F3 to quit without installing ReactOS.");
#endif

  SetTextXY(8, 17, "\xf9  Press F3 to quit without installing ReactOS.");

  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(INSTALL_INTRO_PAGE);
	}
#if 0
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R')
	{
	  return(REPAIR_INTRO_PAGE);
	}
#endif
    }

  return(INTRO_PAGE);
}


static ULONG
InstallIntroPage(PINPUT_RECORD Ir)
{
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
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(SELECT_PARTITION_PAGE);
	}
    }

  return(INSTALL_INTRO_PAGE);
}


static ULONG
SelectPartitionPage(PINPUT_RECORD Ir)
{
  PPARTLIST PartList;
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "The list below shows existing partitions and unused disk");
  SetTextXY(6, 9, "space for new partitions.");

  SetTextXY(8, 11, "\xf9  Press UP or DOWN to select a list entry.");
  SetTextXY(8, 13, "\xf9  Press ENTER to install ReactOS onto the selected partition.");
  SetTextXY(8, 15, "\xf9  Press C to create a new partition.");
  SetTextXY(8, 17, "\xf9  Press D to delete an existing partition.");

  SetStatusText("   Please wait...");

  GetScreenSize(&xScreen, &yScreen);

  PartList = CreatePartitionList(2, 19, xScreen - 3, yScreen - 3);
  if (PartList == NULL)
    {
      /* FIXME: show an error dialog */
      return(QUIT_PAGE);
    }

  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    {
	      DestroyPartitionList(PartList);
	      return(QUIT_PAGE);
	    }
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))
	{
	  ScrollDownPartitionList(PartList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))
	{
	  ScrollUpPartitionList(PartList);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  PartDataValid = GetPartitionData(PartList, &PartData);
	  DestroyPartitionList(PartList);
	  return(SELECT_FILE_SYSTEM_PAGE);
	}

      /* FIXME: Update status text */

    }

  DestroyPartitionList(PartList);

  return(SELECT_PARTITION_PAGE);
}


static ULONG
SelectFileSystemPage(PINPUT_RECORD Ir)
{
  ULONGLONG DiskSize;
  ULONGLONG PartSize;
  PCHAR DiskUnit;
  PCHAR PartUnit;
  PCHAR PartType;

  if (PartDataValid == FALSE)
    {
      /* FIXME: show an error dialog */
      return(QUIT_PAGE);
    }

  /* adjust disk size */
  if (PartData.DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (PartData.DiskSize + (1 << 29)) >> 30;
      DiskUnit = "GB";
    }
  else
    {
      DiskSize = (PartData.DiskSize + (1 << 19)) >> 20;
      DiskUnit = "MB";
    }

  /* adjust partition size */
  if (PartData.PartSize >= 0x280000000ULL) /* 10 GB */
    {
      PartSize = (PartData.PartSize + (1 << 29)) >> 30;
      PartUnit = "GB";
    }
  else
    {
      PartSize = (PartData.PartSize + (1 << 19)) >> 20;
      PartUnit = "MB";
    }

  /* adjust partition type */
  if ((PartData.PartType == PARTITION_FAT_12) ||
      (PartData.PartType == PARTITION_FAT_16) ||
      (PartData.PartType == PARTITION_HUGE) ||
      (PartData.PartType == PARTITION_XINT13))
    {
      PartType = "FAT";
    }
  else if ((PartData.PartType == PARTITION_FAT32) ||
	   (PartData.PartType == PARTITION_FAT32_XINT13))
    {
      PartType = "FAT32";
    }
  else if (PartData.PartType == PARTITION_IFS)
    {
      PartType = "NTFS"; /* FIXME: Not quite correct! */
    }
  else
    {
      PartType = "Unknown";
    }

  SetTextXY(6, 8, "ReactOS will be installed");

  PrintTextXY(8, 9, "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu.",
	      PartData.DiskNumber,
	      DiskSize,
	      DiskUnit,
	      PartData.Port,
	      PartData.Bus,
	      PartData.Id);

  PrintTextXY(8, 10, "on Partition %lu (%I64u %s) %s",
	      PartData.PartNumber,
	      PartSize,
	      PartUnit,
	      PartType);

  SetTextXY(6, 13, "Select a file system for the partition from the list below.");

  SetTextXY(8, 15, "\xf9  Press UP or DOWN to select a file system.");
  SetTextXY(8, 17, "\xf9  Press ENTER to format the partition.");
  SetTextXY(8, 19, "\xf9  Press ESC to select another partition.");

  /* FIXME: use a real list later */
  SetInvertedTextXY(6, 22, " Keep current file system (no changes) ");


  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x1B) /* ESC */
	{
	  return(SELECT_PARTITION_PAGE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(CHECK_FILE_SYSTEM_PAGE);
	}
    }

  return(SELECT_FILE_SYSTEM_PAGE);
}


static ULONG
CheckFileSystemPage(PINPUT_RECORD Ir)
{

  SetTextXY(6, 8, "Check file system");

  SetTextXY(6, 10, "At present, ReactOS can not check file systems.");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(INSTALL_DIRECTORY_PAGE);
	}
    }

  return(CHECK_FILE_SYSTEM_PAGE);
}


static ULONG
InstallDirectoryPage(PINPUT_RECORD Ir)
{
  ULONG Length;

  SetTextXY(6, 8, "Setup installs ReactOS files onto the selected partition. Choose a");
  SetTextXY(6, 9, "directory where you want ReactOS to be installed:");

  strcpy(InstallDir, "\\reactos");
  Length = strlen(InstallDir);

  SetInputTextXY(8, 11, 51, InstallDir);

  SetTextXY(6, 14, "To change the suggested directory, press BACKSPACE to delete");
  SetTextXY(6, 15, "characters and then type the directory where you want ReactOS to");
  SetTextXY(6, 16, "be installed.");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* Return */
	{
	  return(PREPARE_COPY_PAGE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x08) /* Backspace */
	{
	  if (Length > 0)
	    {
	      Length--;
	      InstallDir[Length] = 0;
	      SetInputTextXY(8, 11, 51, InstallDir);
	    }
	}
      else if (isprint(Ir->Event.KeyEvent.uChar.AsciiChar))
	{
	  if (Length < 50)
	    {
	      InstallDir[Length] = Ir->Event.KeyEvent.uChar.AsciiChar;
	      Length++;
	      InstallDir[Length] = 0;
	      SetInputTextXY(8, 11, 51, InstallDir);
	    }
	}
    }

  return(INSTALL_DIRECTORY_PAGE);
}


static ULONG
PrepareCopyPage(PINPUT_RECORD Ir)
{

  SetTextXY(6, 8, "Preparing to copy files");


  SetTextXY(6, 12, "Build file copy list");

  SetTextXY(6, 14, "Create directories");

  SetStatusText("   Please wait...");




  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(FILE_COPY_PAGE);
	}
    }

  return(PREPARE_COPY_PAGE);
}


static ULONG
FileCopyPage(PINPUT_RECORD Ir)
{

  SetTextXY(6, 8, "Copying files");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(INIT_SYSTEM_PAGE);
	}
    }

  return(FILE_COPY_PAGE);
}


static ULONG
InitSystemPage(PINPUT_RECORD Ir)
{

  SetTextXY(6, 8, "Initializing system settings");


  SetTextXY(6, 12, "Create registry hives");

  SetTextXY(6, 14, "Update registry hives");

  SetTextXY(6, 16, "Install/update boot manager");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(SUCCESS_PAGE);
	}
    }

  return(INIT_SYSTEM_PAGE);
}


static ULONG
QuitPage(PINPUT_RECORD Ir)
{
  SetTextXY(10, 6, "ReactOS is not completely installed");

  SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  SetTextXY(10, 9, "all CD-ROMs from CD-Drive.");

  SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(REBOOT_PAGE);
	}
    }
}


static ULONG
SuccessPage(PINPUT_RECORD Ir)
{
  SetTextXY(10, 6, "The basic components of ReactOS have been installed successfully.");

  SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  SetTextXY(10, 9, "all CD-ROMs from CD-Drive.");

  SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
	{
	  return(REBOOT_PAGE);
	}
    }
}


VOID STDCALL
NtProcessStartup(PPEB Peb)
{
  NTSTATUS Status;
  INPUT_RECORD Ir;
  ULONG Page;

  RtlNormalizeProcessParams(Peb->ProcessParameters);

  ProcessHeap = Peb->ProcessHeap;

  Status = AllocConsole();
  if (!NT_SUCCESS(Status))
    {
      PrintString("Console initialization failed! (Status %lx)\n", Status);

      /* Raise a hard error (crash the system/BSOD) */
      NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		       0,0,0,0,0);
    }

  Page = INTRO_PAGE;
  while (Page != REBOOT_PAGE)
    {
      ClearScreen();

      SetUnderlinedTextXY(4, 3, " ReactOS 0.0.20 Setup ");

      switch (Page)
	{
	  case INTRO_PAGE:
	    Page = IntroPage(&Ir);
	    break;

	  case INSTALL_INTRO_PAGE:
	    Page = InstallIntroPage(&Ir);
	    break;

#if 0
	  case OEM_DRIVER_PAGE:
	    Page = OemDriverPage(&Ir);
	    break;
#endif

#if 0
	  case DEVICE_SETTINGS_PAGE:
#endif

	  case SELECT_PARTITION_PAGE:
	    Page = SelectPartitionPage(&Ir);
	    break;

	  case SELECT_FILE_SYSTEM_PAGE:
	    Page = SelectFileSystemPage(&Ir);
	    break;

	  case CHECK_FILE_SYSTEM_PAGE:
	    Page = CheckFileSystemPage(&Ir);
	    break;

	  case INSTALL_DIRECTORY_PAGE:
	    Page = InstallDirectoryPage(&Ir);
	    break;

	  case PREPARE_COPY_PAGE:
	    Page = PrepareCopyPage(&Ir);
	    break;

	  case FILE_COPY_PAGE:
	    Page = FileCopyPage(&Ir);
	    break;

	  case INIT_SYSTEM_PAGE:
	    Page = InitSystemPage(&Ir);
	    break;


	  case SUCCESS_PAGE:
	    Page = SuccessPage(&Ir);
	    break;

	  case QUIT_PAGE:
	    Page = QuitPage(&Ir);
	    break;
	}
    }

  /* Reboot */
  FreeConsole();
  NtShutdownSystem(ShutdownReboot);
}

/* EOF */
