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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/usetup.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:      Eric Kohl
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include <ntos/minmax.h>
#include <reactos/resource.h>

#include "usetup.h"
#include "console.h"
#include "partlist.h"
#include "inicache.h"
#include "infcache.h"
#include "filequeue.h"
#include "progress.h"
#include "bootsup.h"
#include "registry.h"


typedef enum _PAGE_NUMBER
{
  START_PAGE,
  INTRO_PAGE,
  INSTALL_INTRO_PAGE,

  SELECT_PARTITION_PAGE,
  SELECT_FILE_SYSTEM_PAGE,
  CHECK_FILE_SYSTEM_PAGE,
  PREPARE_COPY_PAGE,
  INSTALL_DIRECTORY_PAGE,
  FILE_COPY_PAGE,
  REGISTRY_PAGE,
  BOOT_LOADER_PAGE,

  REPAIR_INTRO_PAGE,

  SUCCESS_PAGE,
  QUIT_PAGE,
  REBOOT_PAGE,			/* virtual page */
} PAGE_NUMBER, *PPAGE_NUMBER;

typedef struct _COPYCONTEXT
{
  ULONG TotalOperations;
  ULONG CompletedOperations;
  PPROGRESS ProgressBar;
} COPYCONTEXT, *PCOPYCONTEXT;


/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;

BOOLEAN PartDataValid;
PARTDATA PartData;

BOOLEAN ActivePartitionValid;
PARTDATA ActivePartition;

UNICODE_STRING SourcePath;
UNICODE_STRING SourceRootPath;

UNICODE_STRING InstallPath;
UNICODE_STRING DestinationPath;
UNICODE_STRING DestinationArcPath;
UNICODE_STRING DestinationRootPath;

UNICODE_STRING SystemRootPath; /* Path to the active partition */

HINF SetupInf;

HSPFILEQ SetupFileQueue = NULL;


/* FUNCTIONS ****************************************************************/

static VOID
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


static VOID
PopupError(PCHAR Text,
	   PCHAR Status)
{
  SHORT xScreen;
  SHORT yScreen;
  SHORT yTop;
  SHORT xLeft;
  COORD coPos;
  ULONG Written;
  ULONG Length;
  ULONG MaxLength;
  ULONG Lines;
  PCHAR p;
  PCHAR pnext;
  BOOLEAN LastLine;
  SHORT Width;
  SHORT Height;

  /* Count text lines and longest line */
  MaxLength = 0;
  Lines = 0;
  pnext = Text;
  while (TRUE)
    {
      p = strchr(pnext, '\n');
      if (p == NULL)
	{
	  Length = strlen(pnext);
	  LastLine = TRUE;
	}
      else
	{
	  Length = (ULONG)(p - pnext);
	  LastLine = FALSE;
	}

      Lines++;
      if (Length > MaxLength)
	MaxLength = Length;

      if (LastLine == TRUE)
	break;

      pnext = p + 1;
    }

  /* Check length of status line */
  if (Status != NULL)
    {
      Length = strlen(Status);
      if (Length > MaxLength)
	MaxLength = Length;
    }

  GetScreenSize(&xScreen, &yScreen);

  Width = MaxLength + 4;
  Height = Lines + 2;
  if (Status != NULL)
    Height += 2;

  yTop = (yScreen - Height) / 2;
  xLeft = (xScreen - Width) / 2;


  /* Set screen attributes */
  coPos.X = xLeft;
  for (coPos.Y = yTop; coPos.Y < yTop + Height; coPos.Y++)
    {
      FillConsoleOutputAttribute(0x74,
				 Width,
				 coPos,
				 &Written);
    }

  /* draw upper left corner */
  coPos.X = xLeft;
  coPos.Y = yTop;
  FillConsoleOutputCharacter(0xDA, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw upper edge */
  coPos.X = xLeft + 1;
  coPos.Y = yTop;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Width - 2,
			     coPos,
			     &Written);

  /* draw upper right corner */
  coPos.X = xLeft + Width - 1;
  coPos.Y = yTop;
  FillConsoleOutputCharacter(0xBF, // '+',
			     1,
			     coPos,
			     &Written);

  /* Draw right edge, inner space and left edge */
  for (coPos.Y = yTop + 1; coPos.Y < yTop + Height - 1; coPos.Y++)
    {
      coPos.X = xLeft;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);

      coPos.X = xLeft + 1;
      FillConsoleOutputCharacter(' ',
				 Width - 2,
				 coPos,
				 &Written);

      coPos.X = xLeft + Width - 1;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);
    }

  /* draw lower left corner */
  coPos.X = xLeft;
  coPos.Y = yTop + Height - 1;
  FillConsoleOutputCharacter(0xC0, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw lower edge */
  coPos.X = xLeft + 1;
  coPos.Y = yTop + Height - 1;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Width - 2,
			     coPos,
			     &Written);

  /* draw lower right corner */
  coPos.X = xLeft + Width - 1;
  coPos.Y = yTop + Height - 1;
  FillConsoleOutputCharacter(0xD9, // '+',
			     1,
			     coPos,
			     &Written);

  /* Print message text */
  coPos.Y = yTop + 1;
  pnext = Text;
  while (TRUE)
    {
      p = strchr(pnext, '\n');
      if (p == NULL)
	{
	  Length = strlen(pnext);
	  LastLine = TRUE;
	}
      else
	{
	  Length = (ULONG)(p - pnext);
	  LastLine = FALSE;
	}

      if (Length != 0)
	{
	  coPos.X = xLeft + 2;
	  WriteConsoleOutputCharacters(pnext,
				       Length,
				       coPos);
	}

      if (LastLine == TRUE)
	break;

      coPos.Y++;
      pnext = p + 1;
    }

  /* Print separator line and status text */
  if (Status != NULL)
    {
      coPos.Y = yTop + Height - 3;
      coPos.X = xLeft;
      FillConsoleOutputCharacter(0xC3, // '+',
				 1,
				 coPos,
				 &Written);

      coPos.X = xLeft + 1;
      FillConsoleOutputCharacter(0xC4, // '-',
				 Width - 2,
				 coPos,
				 &Written);

      coPos.X = xLeft + Width - 1;
      FillConsoleOutputCharacter(0xB4, // '+',
				 1,
				 coPos,
				 &Written);

      coPos.Y++;
      coPos.X = xLeft + 2;
      WriteConsoleOutputCharacters(Status,
				   min(strlen(Status), Width - 4),
				   coPos);
    }
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
  BOOL Result = FALSE;

  PopupError("ReactOS is not completely installed on your\n"
	     "computer. If you quit Setup now, you will need to\n"
	     "run Setup again to install ReactOS.\n"
	     "\n"
	     "  * Press ENTER to continue Setup.\n"
	     "  * Press F3 to quit Setup.",
	     "F3= Quit  ENTER = Continue");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))	/* F3 */
	{
	  Result = TRUE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	{
	  Result = FALSE;
	  break;
	}
    }

  return(Result);
}



/*
 * Start page
 * RETURNS
 *	Number of the next page.
 */
static PAGE_NUMBER
StartPage(PINPUT_RECORD Ir)
{
  NTSTATUS Status;
  WCHAR FileNameBuffer[MAX_PATH];
  UNICODE_STRING FileName;

  INFCONTEXT Context;
  PWCHAR Value;
  ULONG ErrorLine;


  SetStatusText("   Please wait...");

  Status = GetSourcePaths(&SourcePath,
			  &SourceRootPath);
  if (!NT_SUCCESS(Status))
    {
      PrintTextXY(6, 15, "GetSourcePath() failed (Status 0x%08lx)", Status);
    }
  else
    {
      PrintTextXY(6, 15, "SourcePath: '%wZ'", &SourcePath);
      PrintTextXY(6, 16, "SourceRootPath: '%wZ'", &SourceRootPath);
    }


  /* Load txtsetup.sif from install media. */
  wcscpy(FileNameBuffer, SourceRootPath.Buffer);
  wcscat(FileNameBuffer, L"\\install\\txtsetup.sif");
  RtlInitUnicodeString(&FileName,
		       FileNameBuffer);

  Status = InfOpenFile(&SetupInf,
		       &FileName,
		       &ErrorLine);
  if (!NT_SUCCESS(Status))
    {
      PopupError("Setup failed to load the file TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Open 'Version' section */
  if (!InfFindFirstLine (SetupInf, L"Version", L"Signature", &Context))
    {
      PopupError("Setup found a corrupt TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }


  /* Get pointer 'Signature' key */
  if (!InfGetData (&Context, NULL, &Value))
    {
      PopupError("Setup found a corrupt TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Check 'Signature' string */
  if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
      PopupError("Setup found an invalid signature in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  return(INTRO_PAGE);
}



static PAGE_NUMBER
RepairIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The repair functions are not implemented yet.");

  SetTextXY(8, 15, "\xfa  Press ESC to return to the main page.");

  SetTextXY(8, 17, "\xfa  Press ENTER to reboot your computer.");

  SetStatusText("   ESC = Main page  ENTER = Reboot");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return(REBOOT_PAGE);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  return(INTRO_PAGE);
	}
    }

  return(REPAIR_INTRO_PAGE);
}


/*
 * First setup page
 * RETURNS
 *	TRUE: setup/repair completed successfully
 *	FALSE: setup/repair terminated by user
 */
static PAGE_NUMBER
IntroPage(PINPUT_RECORD Ir)
{
  SetHighlightedTextXY(6, 8, "Welcome to ReactOS Setup");

  SetTextXY(6, 11, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 12, "computer and prepares the second part of the setup.");

  SetTextXY(8, 15, "\xfa  Press ENTER to install ReactOS.");

  SetTextXY(8, 17, "\xfa  Press E to start the emergency repair console.");

  SetTextXY(8, 19, "\xfa  Press R to repair ReactOS.");

  SetTextXY(8, 21, "\xfa  Press F3 to quit without installing ReactOS.");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return(INSTALL_INTRO_PAGE);
	}
#if 0
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'E') /* E */
	{
	  return(RepairConsole());
	}
#endif
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R') /* R */
	{
	  return(REPAIR_INTRO_PAGE);
	}
    }

  return(INTRO_PAGE);
}


static PAGE_NUMBER
InstallIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The following functions are missing:");
  SetTextXY(8, 13, "- Creating and deleting harddisk partitions.");
  SetTextXY(8, 14, "- Formatting partitions.");
  SetTextXY(8, 15, "- Support for non-FAT file systems.");
  SetTextXY(8, 16, "- Checking file systems.");



  SetTextXY(8, 21, "\xfa  Press ENTER to install ReactOS.");

  SetTextXY(8, 23, "\xfa  Press F3 to quit without installing ReactOS.");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return(SELECT_PARTITION_PAGE);
	}
    }

  return(INSTALL_INTRO_PAGE);
}


static PAGE_NUMBER
SelectPartitionPage(PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  PPARTLIST PartList;
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "The list below shows existing partitions and unused disk");
  SetTextXY(6, 9, "space for new partitions.");

  SetTextXY(8, 11, "\xfa  Press UP or DOWN to select a list entry.");
  SetTextXY(8, 13, "\xfa  Press ENTER to install ReactOS onto the selected partition.");
  SetTextXY(8, 15, "\xfa  Press C to create a new partition.");
  SetTextXY(8, 17, "\xfa  Press D to delete an existing partition.");

  SetStatusText("   Please wait...");

  RtlFreeUnicodeString(&DestinationPath);
  RtlFreeUnicodeString(&DestinationRootPath);

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
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    {
	      DestroyPartitionList(PartList);
	      return(QUIT_PAGE);
	    }
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownPartitionList(PartList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpPartitionList(PartList);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  PartDataValid = GetSelectedPartition(PartList,
					       &PartData);
	  ActivePartitionValid = GetActiveBootPartition(PartList,
							&ActivePartition);
	  DestroyPartitionList(PartList);

	  RtlFreeUnicodeString(&DestinationRootPath);
	  swprintf(PathBuffer,
		   L"\\Device\\Harddisk%lu\\Partition%lu",
		   PartData.DiskNumber,
		   PartData.PartNumber);
	  RtlCreateUnicodeString(&DestinationRootPath,
				 PathBuffer);

	  RtlFreeUnicodeString(&SystemRootPath);
	  swprintf(PathBuffer,
		   L"\\Device\\Harddisk%lu\\Partition%lu",
		   ActivePartition.DiskNumber,
		   ActivePartition.PartNumber);
	  RtlCreateUnicodeString(&SystemRootPath,
				 PathBuffer);

	  return(SELECT_FILE_SYSTEM_PAGE);
	}

      /* FIXME: Update status text */

    }

  DestroyPartitionList(PartList);

  return(SELECT_PARTITION_PAGE);
}


static PAGE_NUMBER
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

  SetTextXY(6, 8, "Setup will install ReactOS on");

  PrintTextXY(8, 10, "Partition %lu (%I64u %s) %s of",
	      PartData.PartNumber,
	      PartSize,
	      PartUnit,
	      PartType);

  PrintTextXY(8, 12, "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ).",
	      PartData.DiskNumber,
	      DiskSize,
	      DiskUnit,
	      PartData.Port,
	      PartData.Bus,
	      PartData.Id,
	      &PartData.DriverName);


  SetTextXY(6, 17, "Select a file system for the partition from the list below.");

  SetTextXY(8, 19, "\xfa  Press UP or DOWN to select a file system.");
  SetTextXY(8, 21, "\xfa  Press ENTER to format the partition.");
  SetTextXY(8, 23, "\xfa  Press ESC to select another partition.");

  /* FIXME: use a real list later */
  SetInvertedTextXY(6, 26, " Keep current file system (no changes) ");


  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  return(SELECT_PARTITION_PAGE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
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

  SetStatusText("   Please wait ...");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return(INSTALL_DIRECTORY_PAGE);
	}
    }

  return(CHECK_FILE_SYSTEM_PAGE);
}


static PAGE_NUMBER
InstallDirectoryPage(PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  WCHAR InstallDir[51];
  PWCHAR DefaultPath;
  INFCONTEXT Context;
  ULONG Length;
  NTSTATUS Status;

  /* Search for 'DefaultPath' in the 'SetupData' section */
  if (!InfFindFirstLine (SetupInf, L"SetupData", L"DefaultPath", &Context))
    {
      PopupError("Setup failed to find the 'SetupData' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Read the 'DefaultPath' data */
  if (InfGetData (&Context, NULL, &DefaultPath))
    {
      wcscpy(InstallDir, DefaultPath);
    }
  else
    {
      wcscpy(InstallDir, L"\\ReactOS");
    }
  Length = wcslen(InstallDir);

  SetTextXY(6, 8, "Setup installs ReactOS files onto the selected partition. Choose a");
  SetTextXY(6, 9, "directory where you want ReactOS to be installed:");

  SetInputTextXY(8, 11, 51, InstallDir);

  SetTextXY(6, 14, "To change the suggested directory, press BACKSPACE to delete");
  SetTextXY(6, 15, "characters and then type the directory where you want ReactOS to");
  SetTextXY(6, 16, "be installed.");

  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return(QUIT_PAGE);
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  /* Create 'InstallPath' string */
	  RtlFreeUnicodeString(&InstallPath);
	  RtlCreateUnicodeString(&InstallPath,
				 InstallDir);

	  /* Create 'DestinationPath' string */
	  RtlFreeUnicodeString(&DestinationPath);
	  wcscpy(PathBuffer,
		 DestinationRootPath.Buffer);
	  if (InstallDir[0] != L'\\')
	    wcscat(PathBuffer,
		   L"\\");
	  wcscat(PathBuffer, InstallDir);
	  RtlCreateUnicodeString(&DestinationPath,
				 PathBuffer);

	  /* Create 'DestinationArcPath' */
	  RtlFreeUnicodeString(&DestinationArcPath);
	  swprintf(PathBuffer,
		   L"multi(0)disk(0)rdisk(%lu)partition(%lu)",
		   PartData.DiskNumber,
		   PartData.PartNumber);
	  if (InstallDir[0] != L'\\')
	    wcscat(PathBuffer,
		   L"\\");
	  wcscat(PathBuffer, InstallDir);
	  RtlCreateUnicodeString(&DestinationArcPath,
				 PathBuffer);

	  return(PREPARE_COPY_PAGE);
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x08) /* BACKSPACE */
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
	      InstallDir[Length] = (WCHAR)Ir->Event.KeyEvent.uChar.AsciiChar;
	      Length++;
	      InstallDir[Length] = 0;
	      SetInputTextXY(8, 11, 51, InstallDir);
	    }
	}
    }

  return(INSTALL_DIRECTORY_PAGE);
}


static PAGE_NUMBER
PrepareCopyPage(PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  INFCONTEXT FilesContext;
  INFCONTEXT DirContext;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  ULONG Length;
  NTSTATUS Status;

  PWCHAR FileKeyName;
  PWCHAR FileKeyValue;
  PWCHAR DirKeyName;
  PWCHAR DirKeyValue;

  SetTextXY(6, 8, "Setup prepares your computer for copying the ReactOS files. ");


//  SetTextXY(8, 12, "Build file copy list");

//  SetTextXY(8, 14, "Create directories");

//  SetStatusText("   Please wait...");


  /*
   * Build the file copy list
   */
  SetStatusText("   Building the file copy list...");
//  SetInvertedTextXY(8, 12, "Build file copy list");



  /* Search for the 'SourceFiles' section */
  if (!InfFindFirstLine (SetupInf, L"SourceFiles", NULL, &FilesContext))
    {
      PopupError("Setup failed to find the 'SourceFiles' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }


  /* Create the file queue */
  SetupFileQueue = SetupOpenFileQueue();
  if (SetupFileQueue == NULL)
    {
      PopupError("Setup failed to open the copy file queue.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /*
   * Enumerate the files in the 'SourceFiles' section
   * and add them to the file queue.
   */
  do
    {
      if (!InfGetData (&FilesContext, &FileKeyName, &FileKeyValue))
	break;

      DPRINT1("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

      /* Lookup target directory */
      if (!InfFindFirstLine (SetupInf, L"Directories", FileKeyValue, &DirContext))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("InfFindFirstLine() failed\n");
	  break;
	}

      if (!InfGetData (&DirContext, NULL, &DirKeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("InfGetData() failed\n");
	  break;
	}

      if (!SetupQueueCopy(SetupFileQueue,
			  SourceRootPath.Buffer,
			  L"\\install",
			  FileKeyName,
			  DirKeyValue,
			  NULL))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("SetupQueueCopy() failed\n");
	}
    }
  while (InfFindNextLine(&FilesContext, &FilesContext));


  /* Create directories */
  SetStatusText("   Creating directories...");

  /*
   * FIXME:
   * Install directories like '\reactos\test' are not handled yet.
   */

  /* Get destination path */
  wcscpy(PathBuffer, DestinationPath.Buffer);

  /* Remove trailing backslash */
  Length = wcslen(PathBuffer);
  if ((Length > 0) && (PathBuffer[Length - 1] == '\\'))
    {
      PathBuffer[Length - 1] = 0;
    }

  /* Create the install directory */
  Status = CreateDirectory(PathBuffer);
  if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
    {
      DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
      PopupError("Setup could not create the install directory.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }


  /* Search for the 'Directories' section */
  if (!InfFindFirstLine(SetupInf, L"Directories", NULL, &DirContext))
    {
      PopupError("Setup failed to find the 'Directories' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Enumerate the directory values and create the subdirectories */
  do
    {
      if (!InfGetData (&DirContext, NULL, &KeyValue))
	break;

      if (KeyValue[0] == L'\\' && KeyValue[1] != 0)
	{
	      DPRINT("Absolute Path: '%S'\n", KeyValue);

	      wcscpy(PathBuffer, DestinationRootPath.Buffer);
	      wcscat(PathBuffer, KeyValue);

	      DPRINT("FullPath: '%S'\n", PathBuffer);
	}
      else if (KeyValue[0] != L'\\')
	{
	  DPRINT("RelativePath: '%S'\n", KeyValue);
	  wcscpy(PathBuffer, DestinationPath.Buffer);
	  wcscat(PathBuffer, L"\\");
	  wcscat(PathBuffer, KeyValue);

	  DPRINT("FullPath: '%S'\n", PathBuffer);

	  Status = CreateDirectory(PathBuffer);
	  if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
	    {
	      DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
	      PopupError("Setup could not create install directories.",
			 "ENTER = Reboot computer");

	      while (TRUE)
		{
		  ConInKey(Ir);

		  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		    {
		      return(QUIT_PAGE);
		    }
		}
	    }
	}
    }
  while (InfFindNextLine (&DirContext, &DirContext));

  return(FILE_COPY_PAGE);
}


static ULONG
FileCopyCallback(PVOID Context,
		 ULONG Notification,
		 PVOID Param1,
		 PVOID Param2)
{
  PCOPYCONTEXT CopyContext;

  CopyContext = (PCOPYCONTEXT)Context;

  switch (Notification)
    {
      case SPFILENOTIFY_STARTSUBQUEUE:
	CopyContext->TotalOperations = (ULONG)Param2;
	ProgressSetStepCount(CopyContext->ProgressBar,
			     CopyContext->TotalOperations);
	break;

      case SPFILENOTIFY_STARTCOPY:
	/* Display copy message */
	PrintTextXYN(6, 16, 60, "Copying file: %S", (PWSTR)Param1);

	PrintTextXYN(6, 18, 60, "File %lu of %lu",
		     CopyContext->CompletedOperations + 1,
		     CopyContext->TotalOperations);
	break;

      case SPFILENOTIFY_ENDCOPY:
	CopyContext->CompletedOperations++;
	ProgressNextStep(CopyContext->ProgressBar);
	break;
    }

  return(0);
}


static PAGE_NUMBER
FileCopyPage(PINPUT_RECORD Ir)
{
  COPYCONTEXT CopyContext;
  SHORT xScreen;
  SHORT yScreen;

  SetStatusText("   Please wait...");

  SetTextXY(6, 8, "Copying files");

  GetScreenSize(&xScreen, &yScreen);

  CopyContext.TotalOperations = 0;
  CopyContext.CompletedOperations = 0;
  CopyContext.ProgressBar = CreateProgressBar(6,
					      yScreen - 14,
					      xScreen - 7,
					      yScreen - 10);

  SetupCommitFileQueue(SetupFileQueue,
		       DestinationRootPath.Buffer,
		       InstallPath.Buffer,
		       (PSP_FILE_CALLBACK)FileCopyCallback,
		       &CopyContext);

  SetupCloseFileQueue(SetupFileQueue);

  DestroyProgressBar(CopyContext.ProgressBar);

  return(REGISTRY_PAGE);
}


static PAGE_NUMBER
RegistryPage(PINPUT_RECORD Ir)
{
  INFCONTEXT InfContext;
  NTSTATUS Status;

  PWSTR Action;
  PWSTR File;
  PWSTR Section;
  BOOLEAN Delete;


  SetTextXY(6, 8, "Setup is updating the system configuration");

  SetStatusText("   Creating registry hives...");

  if (!SetInstallPathValue(&DestinationPath))
    {
      DPRINT("SetInstallPathValue() failed\n");
      PopupError("Setup failed to set the initialize the registry.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Create the default hives */
  Status = NtInitializeRegistry(TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtInitializeRegistry() failed (Status %lx)\n", Status);
      PopupError("Setup failed to create the registry hives.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  /* Update registry */
  SetStatusText("   Updating registry hives...");

  if (!InfFindFirstLine(SetupInf, L"HiveInfs.Install", NULL, &InfContext))
    {
      DPRINT1("InfFindFirstLine() failed\n");
      PopupError("Setup failed to find the registry data files.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  do
    {
      InfGetDataField (&InfContext, 0, &Action);
      InfGetDataField (&InfContext, 1, &File);
      InfGetDataField (&InfContext, 2, &Section);

      DPRINT1("Action: %S  File: %S  Section %S\n", Action, File, Section);

      if (!_wcsicmp (Action, L"AddReg"))
	{
	  Delete = FALSE;
	}
      else if (!_wcsicmp (Action, L"DelReg"))
	{
	  Delete = TRUE;
	}
      else
	{
	  continue;
	}

      SetStatusText("   Importing %S...", File);

      if (!ImportRegistryFile(File, Section, Delete))
	{
	  DPRINT1("Importing %S failed\n", File);

	  PopupError("Setup failed to import a hive file.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		{
		  return(QUIT_PAGE);
		}
	    }
	}
    }
  while (InfFindNextLine (&InfContext, &InfContext));

  SetStatusText("   Done...");

  return(BOOT_LOADER_PAGE);
}


static PAGE_NUMBER
BootLoaderPage(PINPUT_RECORD Ir)
{
  WCHAR SrcPath[MAX_PATH];
  WCHAR DstPath[MAX_PATH];
  PINICACHE IniCache;
  PINICACHESECTION IniSection;
  NTSTATUS Status;

  SetTextXY(6, 8, "Installing the boot loader");

  SetStatusText("   Please wait...");

  if (ActivePartitionValid == FALSE)
    {
      DPRINT1("Error: no active partition found\n");
      PopupError("Setup could not find an active partiton\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  if (ActivePartition.PartType == PARTITION_ENTRY_UNUSED)
    {
      DPRINT1("Error: active partition invalid (unused)\n");
      PopupError("The active partition is unused (invalid).\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  if (ActivePartition.PartType == 0x0A)
    {
      /* OS/2 boot manager partition */
      DPRINT1("Found OS/2 boot manager partition\n");
      PopupError("Setup found an OS/2 boot manager partiton.\n"
		 "The OS/2 boot manager is not supported yet!",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }
  else if (ActivePartition.PartType == 0x83)
    {
      /* Linux ext2 partition */
      DPRINT1("Found Linux ext2 partition\n");
      PopupError("Setup found a Linux ext2 partiton.\n"
		 "Linux ext2 partitions are not supported yet!",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }
  else if (ActivePartition.PartType == PARTITION_IFS)
    {
      /* NTFS partition */
      DPRINT1("Found NTFS partition\n");
      PopupError("Setup found an NTFS partiton.\n"
		 "NTFS partitions are not supported yet!",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }
  else if ((ActivePartition.PartType == PARTITION_FAT_12) ||
	   (ActivePartition.PartType == PARTITION_FAT_16) ||
	   (ActivePartition.PartType == PARTITION_HUGE) ||
	   (ActivePartition.PartType == PARTITION_XINT13) ||
	   (ActivePartition.PartType == PARTITION_FAT32) ||
	   (ActivePartition.PartType == PARTITION_FAT32_XINT13))
  {
    /* FAT or FAT32 partition */
    DPRINT1("System path: '%wZ'\n", &SystemRootPath);

    if (DoesFileExist(SystemRootPath.Buffer, L"ntldr") == TRUE ||
	DoesFileExist(SystemRootPath.Buffer, L"boot.ini") == TRUE)
    {
      /* Search root directory for 'ntldr' and 'boot.ini'. */
      DPRINT1("Found Microsoft Windows NT/2000/XP boot loader\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath.Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath.Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT1("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	PopupError("Setup failed to copy 'freeldr.sys'.",
		   "ENTER = Reboot computer");

	while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	  {
	    return(QUIT_PAGE);
	  }
	}
      }

      /* Create or update freeldr.ini */
      if (DoesFileExist(SystemRootPath.Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new 'freeldr.ini' */
	DPRINT1("Create new 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = CreateFreeLoaderIniForReactos(DstPath,
					       DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForReactos() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to create 'freeldr.ini'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}

	/* Install new bootcode */
	if ((ActivePartition.PartType == PARTITION_FAT32) ||
	    (ActivePartition.PartType == PARTITION_FAT32_XINT13))
	{
	  /* Install FAT32 bootcode */
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");
	  wcscpy(DstPath, SystemRootPath.Buffer);
	  wcscat(DstPath, L"\\bootsect.ros");

	  DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, DstPath);
	  Status = InstallFat32BootCodeToFile(SrcPath,
					      DstPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToFile() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT32 bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}
	else
	{
	  /* Install FAT16 bootcode */
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");
	  wcscpy(DstPath, SystemRootPath.Buffer);
	  wcscat(DstPath, L"\\bootsect.ros");

	  DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, DstPath);
	  Status = InstallFat16BootCodeToFile(SrcPath,
					      DstPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToFile() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}

	/* Update 'boot.ini' */
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\boot.ini");

	DPRINT1("Update 'boot.ini': %S\n", DstPath);
	Status = UpdateBootIni(DstPath,
			       L"C:\\bootsect.ros",
			       L"\"ReactOS\"");
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateBootIni() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to update \'boot.ini\'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	DPRINT1("Update existing 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to update 'freeldr.ini'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}
      }
    }
    else if (DoesFileExist(SystemRootPath.Buffer, L"io.sys") == TRUE ||
	     DoesFileExist(SystemRootPath.Buffer, L"msdos.sys") == TRUE)
    {
      /* Search for root directory for 'io.sys' and 'msdos.sys'. */
      DPRINT1("Found Microsoft DOS or Windows 9x boot loader\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath.Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath.Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	PopupError("Setup failed to copy 'freeldr.sys'.",
		   "ENTER = Reboot computer");

	while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	  {
	    return(QUIT_PAGE);
	  }
	}
      }

      /* Create or update 'freeldr.ini' */
      if (DoesFileExist(SystemRootPath.Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new 'freeldr.ini' */
	DPRINT1("Create new 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = CreateFreeLoaderIniForDos(DstPath,
					   DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForDos() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to create 'freeldr.ini'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}

	/* Save current bootsector as 'BOOTSECT.DOS' */
	wcscpy(SrcPath, SystemRootPath.Buffer);
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\bootsect.dos");

	DPRINT1("Save bootsector: %S ==> %S\n", SrcPath, DstPath);
	Status = SaveCurrentBootSector(SrcPath,
				       DstPath);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("SaveCurrentBootSector() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to save the current bootsector.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}

	/* Install new bootsector */
	if ((ActivePartition.PartType == PARTITION_FAT32) ||
	    (ActivePartition.PartType == PARTITION_FAT32_XINT13))
	{
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");

	  DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath.Buffer);
	  Status = InstallFat32BootCodeToDisk(SrcPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToDisk() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT32 bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}
	else
	{
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");

	  DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, SystemRootPath.Buffer);
	  Status = InstallFat16BootCodeToDisk(SrcPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to update 'freeldr.ini'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}
      }
    }
    else
    {
      /* No or unknown boot loader */
      DPRINT1("No or unknown boot loader found\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath.Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath.Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT1("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	PopupError("Setup failed to copy 'freeldr.sys'.",
		   "ENTER = Reboot computer");

	while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	  {
	    return(QUIT_PAGE);
	  }
	}
      }

      /* Create or update 'freeldr.ini' */
      if (DoesFileExist(SystemRootPath.Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new freeldr.ini */
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	DPRINT1("Copy: %S ==> %S\n", SrcPath, DstPath);
	Status = CreateFreeLoaderIniForReactos(DstPath,
					       DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForReactos() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to create \'freeldr.ini\'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}

	/* Save current bootsector as 'BOOTSECT.OLD' */
	wcscpy(SrcPath, SystemRootPath.Buffer);
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\bootsect.old");

	DPRINT1("Save bootsector: %S ==> %S\n", SrcPath, DstPath);
	Status = SaveCurrentBootSector(SrcPath,
				       DstPath);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("SaveCurrentBootSector() failed (Status %lx)\n", Status);
	  PopupError("Setup failed save the current bootsector.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}

	/* Install new bootsector */
	if ((ActivePartition.PartType == PARTITION_FAT32) ||
	    (ActivePartition.PartType == PARTITION_FAT32_XINT13))
	{
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");

	  DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath.Buffer);
	  Status = InstallFat32BootCodeToDisk(SrcPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToDisk() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT32 bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}
	else
	{
	  wcscpy(SrcPath, SourceRootPath.Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");

	  DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, SystemRootPath.Buffer);
	  Status = InstallFat16BootCodeToDisk(SrcPath,
					      SystemRootPath.Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
	    PopupError("Setup failed to install the FAT bootcode.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		return(QUIT_PAGE);
	      }
	    }
	  }
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	wcscpy(DstPath, SystemRootPath.Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath.Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  PopupError("Setup failed to update 'freeldr.ini'.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	  {
	    ConInKey(Ir);

	    if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	  }
	}
      }
    }
  }
  else
    {
      /* Unknown partition */
      DPRINT1("Unknown partition found\n");
      PopupError("Setup found an unknown partiton type.\n"
		 "This partition type is not supported!",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(QUIT_PAGE);
	    }
	}
    }

  return(SUCCESS_PAGE);
}



static PAGE_NUMBER
QuitPage(PINPUT_RECORD Ir)
{
  SetTextXY(10, 6, "ReactOS is not completely installed");

  SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  SetTextXY(10, 9, "all CD-ROMs from CD-Drives.");

  SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return(REBOOT_PAGE);
	}
    }
}


static PAGE_NUMBER
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

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
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
  PAGE_NUMBER Page;

  RtlNormalizeProcessParams(Peb->ProcessParameters);

  ProcessHeap = Peb->ProcessHeap;

  Status = AllocConsole();
  if (!NT_SUCCESS(Status))
    {
      PrintString("AllocConsole() failed (Status = 0x%08lx)\n", Status);

      /* Raise a hard error (crash the system/BSOD) */
      NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		       0,0,0,0,0);
    }

  PartDataValid = FALSE;

  /* Initialize global unicode strings */
  RtlInitUnicodeString(&SourcePath, NULL);
  RtlInitUnicodeString(&SourceRootPath, NULL);
  RtlInitUnicodeString(&InstallPath, NULL);
  RtlInitUnicodeString(&DestinationPath, NULL);
  RtlInitUnicodeString(&DestinationArcPath, NULL);
  RtlInitUnicodeString(&DestinationRootPath, NULL);
  RtlInitUnicodeString(&SystemRootPath, NULL);


  Page = START_PAGE;
  while (Page != REBOOT_PAGE)
    {
      ClearScreen();

      SetUnderlinedTextXY(4, 3, " ReactOS " KERNEL_VERSION_STR " Setup ");

      switch (Page)
	{
	  /* Start page */
	  case START_PAGE:
	    Page = StartPage(&Ir);
	    break;

	  /* Intro page */
	  case INTRO_PAGE:
	    Page = IntroPage(&Ir);
	    break;

	  /* Install pages */
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

	  case REGISTRY_PAGE:
	    Page = RegistryPage(&Ir);
	    break;

	  case BOOT_LOADER_PAGE:
	    Page = BootLoaderPage(&Ir);
	    break;


	  /* Repair pages */
	  case REPAIR_INTRO_PAGE:
	    Page = RepairIntroPage(&Ir);
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
