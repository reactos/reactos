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
#include "filequeue.h"


#define START_PAGE			0
#define INTRO_PAGE			1
#define INSTALL_INTRO_PAGE		2

#define SELECT_PARTITION_PAGE		4
#define SELECT_FILE_SYSTEM_PAGE		5
#define CHECK_FILE_SYSTEM_PAGE		6
#define PREPARE_COPY_PAGE		7
#define INSTALL_DIRECTORY_PAGE		8
#define FILE_COPY_PAGE			9
#define INIT_SYSTEM_PAGE		10

#define REPAIR_INTRO_PAGE		20

#define SUCCESS_PAGE			100
#define QUIT_PAGE			101
#define REBOOT_PAGE			102


typedef struct _COPYCONTEXT
{
  ULONG TotalOperations;
  ULONG CompletedOperations;
  ULONG Progress;
} COPYCONTEXT, *PCOPYCONTEXT;


/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;

BOOLEAN PartDataValid;
PARTDATA PartData;

WCHAR InstallDir[51];

UNICODE_STRING SourcePath;
UNICODE_STRING SourceRootPath;

PINICACHE IniCache;

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
static ULONG
StartPage(PINPUT_RECORD Ir)
{
  NTSTATUS Status;
  WCHAR FileNameBuffer[MAX_PATH];
  UNICODE_STRING FileName;

  PINICACHESECTION Section;
  PWCHAR Value;


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

  IniCache = NULL;
  Status = IniCacheLoad(&IniCache,
			&FileName);
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
  Section = IniCacheGetSection(IniCache,
			       L"Version");
  if (Section == NULL)
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
  Status = IniCacheGetKey(Section,
			  L"Signature",
			  &Value);
  if (!NT_SUCCESS(Status))
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

#if 0
  PopupError("This is a test error.", "ENTER = Reboot computer");

  SetStatusText("   ENTER = Continue");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	{
	  return(INTRO_PAGE);
	}
    }

  return(START_PAGE);
#endif

  return(INTRO_PAGE);
}



static ULONG
RepairIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The repair functions are not implemented yet.");

  SetTextXY(8, 15, "\xf9  Press ESC to return to the main page.");

  SetTextXY(8, 17, "\xf9  Press ENTER to reboot your computer.");

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
static ULONG
IntroPage(PINPUT_RECORD Ir)
{
  SetHighlightedTextXY(6, 8, "Welcome to ReactOS Setup");

  SetTextXY(6, 11, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 12, "computer and prepares the second part of the setup.");

  SetTextXY(8, 15, "\xf9  Press ENTER to install ReactOS.");

  SetTextXY(8, 17, "\xf9  Press E to start the emergency repair console.");

  SetTextXY(8, 19, "\xf9  Press R to repair ReactOS.");

  SetTextXY(8, 21, "\xf9  Press F3 to quit without installing ReactOS.");


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


static ULONG
InstallIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The following functions are missing:");
  SetTextXY(8, 13, "- Creating and deleting harddisk partitions.");
  SetTextXY(8, 14, "- Formatting partitions.");
  SetTextXY(8, 15, "- Support for non-FAT file systems.");
  SetTextXY(8, 16, "- Checking file systems.");
  SetTextXY(8, 17, "- Installing the bootloader.");



  SetTextXY(8, 21, "\xf9  Press ENTER to install ReactOS.");

  SetTextXY(8, 23, "\xf9  Press F3 to quit without installing ReactOS.");


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


static ULONG
InstallDirectoryPage(PINPUT_RECORD Ir)
{
  ULONG Length;

  SetTextXY(6, 8, "Setup installs ReactOS files onto the selected partition. Choose a");
  SetTextXY(6, 9, "directory where you want ReactOS to be installed:");

  wcscpy(InstallDir, L"\\reactos");
  Length = wcslen(InstallDir);

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


static ULONG
PrepareCopyPage(PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  PINICACHESECTION DirSection;
  PINICACHESECTION FilesSection;
  PINICACHEITERATOR Iterator;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  ULONG Length;
  NTSTATUS Status;

  PWCHAR FileKeyName;
  PWCHAR FileKeyValue;
  PWCHAR DirKeyName;
  PWCHAR DirKeyValue;

  SetTextXY(6, 8, "Setup prepares your computer for copying the ReactOS files. ");


  SetTextXY(8, 12, "Build file copy list");

  SetTextXY(8, 14, "Create directories");

  SetStatusText("   Please wait...");


  /*
   * Build the file copy list
   */
  SetInvertedTextXY(8, 12, "Build file copy list");


  /* Open 'Directories' section */
  DirSection = IniCacheGetSection(IniCache,
				  L"Directories");
  if (DirSection == NULL)
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

  /* Open 'SourceFiles' section */
  FilesSection = IniCacheGetSection(IniCache,
				    L"SourceFiles");
  if (FilesSection == NULL)
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
  Iterator = IniCacheFindFirstValue(FilesSection,
				    &FileKeyName,
				    &FileKeyValue);
  if (Iterator != NULL)
  {
    do
    {
      DPRINT("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

      /* Lookup target directory */
      Status = IniCacheGetKey(DirSection,
			      FileKeyValue,
			      &DirKeyValue);
      if (!NT_SUCCESS(Status))
      {
	/* FIXME: Handle error! */
	DPRINT1("IniCacheGetKey() failed (Status 0x%lX)\n", Status);
      }

      if (SetupQueueCopy(SetupFileQueue,
			 SourceRootPath.Buffer,
			 L"\\install",
			 FileKeyName,
			 DirKeyValue,
			 NULL) == FALSE)
      {
	/* FIXME: Handle error! */
	DPRINT1("SetupQueueCopy() failed\n");
      }
    }
    while (IniCacheFindNextValue(Iterator, &FileKeyName, &FileKeyValue));

    IniCacheFindClose(Iterator);
  }

  /* Report that the file queue has been built */
  SetTextXY(8, 12, "Build file copy list");
  SetHighlightedTextXY(50, 12, "Done");

  /* create directories */
  SetInvertedTextXY(8, 14, "Create directories");


  /*
   * FIXME:
   * Install directories like '\reactos\test' are not handled yet.
   */

  /* Build full install directory name */
  swprintf(PathBuffer,
	   L"\\Device\\Harddisk%lu\\Partition%lu",
	   PartData.DiskNumber,
	   PartData.PartNumber);
  if (InstallDir[0] != L'\\')
    wcscat(PathBuffer, L"\\");
  wcscat(PathBuffer, InstallDir);

  /* Remove trailing backslash */
  Length = wcslen(PathBuffer);
  if ((Length > 0) && (PathBuffer[Length - 1] == '\\'))
    PathBuffer[Length - 1] = 0;

  /* Create the install directory */
  Status = CreateDirectory(PathBuffer);
  if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
  {
    DPRINT1("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);

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


  /* Enumerate the directory values and create the subdirectories */
  Iterator = IniCacheFindFirstValue(DirSection,
				    &KeyName,
				    &KeyValue);
  if (Iterator != NULL)
  {
    do
    {
      if (KeyValue[0] == L'\\' && KeyValue[1] != 0)
      {
        DPRINT("Absolute Path: '%S'\n", KeyValue);

	swprintf(PathBuffer,
		 L"\\Device\\Harddisk%lu\\Partition%lu",
		 PartData.DiskNumber,
		 PartData.PartNumber);
	wcscat(PathBuffer, KeyValue);

	DPRINT("FullPath: '%S'\n", PathBuffer);
      }
      else if (KeyValue[0] != L'\\')
      {
	DPRINT("RelativePath: '%S'\n", KeyValue);
	swprintf(PathBuffer,
		 L"\\Device\\Harddisk%lu\\Partition%lu",
		 PartData.DiskNumber,
		 PartData.PartNumber);

	if (InstallDir[0] != L'\\')
	  wcscat(PathBuffer, L"\\");
	wcscat(PathBuffer, InstallDir);
	wcscat(PathBuffer, L"\\");
	wcscat(PathBuffer, KeyValue);

	DPRINT("FullPath: '%S'\n", PathBuffer);

	Status = CreateDirectory(PathBuffer);
	if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
	  {
	    DPRINT1("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);

	    PopupError("Setup could not create install directories.",
		       "ENTER = Reboot computer");

	    while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	      {
		IniCacheFindClose(Iterator);
		return(QUIT_PAGE);
	      }
	    }
	  }
      }
    }
    while (IniCacheFindNextValue(Iterator, &KeyName, &KeyValue));

    IniCacheFindClose(Iterator);
  }


  SetTextXY(8, 14, "Create directories");
  SetHighlightedTextXY(50, 14, "Done");


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
	  return(FILE_COPY_PAGE);
	}
    }

  return(PREPARE_COPY_PAGE);
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
      break;
  }

  return(0);
}


static ULONG
FileCopyPage(PINPUT_RECORD Ir)
{
  WCHAR TargetRootPath[MAX_PATH];
  COPYCONTEXT CopyContext;

  CopyContext.TotalOperations = 0;
  CopyContext.CompletedOperations = 0;
  CopyContext.Progress = 0;

  SetStatusText("   Please wait...");

  SetTextXY(6, 8, "Copying files");

  swprintf(TargetRootPath,
	   L"\\Device\\Harddisk%lu\\Partition%lu",
	   PartData.DiskNumber,
	   PartData.PartNumber);

  SetupCommitFileQueue(SetupFileQueue,
		       TargetRootPath,
		       InstallDir,
		       (PSP_FILE_CALLBACK)FileCopyCallback,
		       &CopyContext);

  SetupCloseFileQueue(SetupFileQueue);

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

  SetStatusText("   Please wait...");


  /*
   * Create registry hives
   */


  /*
   * Update registry
   */

  /* FIXME: Create key '\Registry\Machine\System\Setup' */

  /* FIXME: Create value 'SystemSetupInProgress' */



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

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
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
  ULONG Page;

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

	  case INIT_SYSTEM_PAGE:
	    Page = InitSystemPage(&Ir);
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
