/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003, 2004 ReactOS Team
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
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
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
#include "format.h"
#include "fslist.h"
#include "cabinet.h"
#include "filesup.h"
#include "drivesup.h"
#include "genlist.h"
#include "settings.h"

#define NDEBUG
#include <debug.h>


typedef enum _PAGE_NUMBER
{
  START_PAGE,
  INTRO_PAGE,
  INSTALL_INTRO_PAGE,

  DEVICE_SETTINGS_PAGE,
  COMPUTER_SETTINGS_PAGE,
  DISPLAY_SETTINGS_PAGE,
  KEYBOARD_SETTINGS_PAGE,
  LAYOUT_SETTINGS_PAGE,
  POINTER_SETTINGS_PAGE,

  SELECT_PARTITION_PAGE,
  CREATE_PARTITION_PAGE,
  DELETE_PARTITION_PAGE,

  SELECT_FILE_SYSTEM_PAGE,
  FORMAT_PARTITION_PAGE,
  CHECK_FILE_SYSTEM_PAGE,

  PREPARE_COPY_PAGE,
  INSTALL_DIRECTORY_PAGE,
  FILE_COPY_PAGE,
  REGISTRY_PAGE,
  BOOT_LOADER_PAGE,

  REPAIR_INTRO_PAGE,

  EMERGENCY_INTRO_PAGE,

  SUCCESS_PAGE,
  QUIT_PAGE,
  FLUSH_PAGE,
  REBOOT_PAGE,			/* virtual page */
} PAGE_NUMBER, *PPAGE_NUMBER;

typedef struct _COPYCONTEXT
{
  ULONG TotalOperations;
  ULONG CompletedOperations;
  PPROGRESSBAR ProgressBar;
} COPYCONTEXT, *PCOPYCONTEXT;


/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;
UNICODE_STRING SourceRootPath;
BOOLEAN IsUnattendedSetup;
LONG UnattendDestinationDiskNumber;
LONG UnattendDestinationPartitionNumber;
WCHAR UnattendInstallationDirectory[MAX_PATH];

/* LOCALS *******************************************************************/

static PPARTLIST PartitionList = NULL;

static PFILE_SYSTEM_LIST FileSystemList = NULL;


static UNICODE_STRING SourcePath;

static UNICODE_STRING InstallPath;

/* Path to the install directory */
static UNICODE_STRING DestinationPath;
static UNICODE_STRING DestinationArcPath;
static UNICODE_STRING DestinationRootPath;

/* Path to the active partition (boot manager) */
static UNICODE_STRING SystemRootPath;

static HINF SetupInf;

static HSPFILEQ SetupFileQueue = NULL;

static BOOLEAN WarnLinuxPartitions = TRUE;

static PGENERIC_LIST ComputerList = NULL;
static PGENERIC_LIST DisplayList = NULL;
static PGENERIC_LIST KeyboardList = NULL;
static PGENERIC_LIST LayoutList = NULL;
static PGENERIC_LIST PointerList = NULL;


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
	     "  \x07  Press ENTER to continue Setup.\n"
	     "  \x07  Press F3 to quit Setup.",
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

  return Result;
}


VOID
CheckUnattendedSetup(VOID)
{
  WCHAR UnattendInfPath[MAX_PATH];
  UNICODE_STRING FileName;
  INFCONTEXT Context;
  HINF UnattendInf;
  ULONG ErrorLine;
  NTSTATUS Status;
  LONG IntValue;
  PWCHAR Value;

  if (DoesFileExist(SourcePath.Buffer, L"unattend.inf") == FALSE)
    {
      DPRINT("Does not exist: %S\\%S\n", SourcePath.Buffer, L"unattend.inf");
      IsUnattendedSetup = FALSE;
      return;
    }

  wcscpy(UnattendInfPath, SourcePath.Buffer);
  wcscat(UnattendInfPath, L"\\unattend.inf");

  RtlInitUnicodeString(&FileName,
		       UnattendInfPath);

  /* Load txtsetup.sif from install media. */

  Status = InfOpenFile(&UnattendInf,
		       &FileName,
		       &ErrorLine);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("InfOpenFile() failed with status 0x%x\n", Status);
      return;
    }

  /* Open 'Unattend' section */
  if (!InfFindFirstLine(UnattendInf, L"Unattend", L"Signature", &Context))
    {
      DPRINT("InfFindFirstLine() failed for section 'Unattend'\n");
      InfCloseFile(UnattendInf);
      return;
    }

  /* Get pointer 'Signature' key */
  if (!InfGetData(&Context, NULL, &Value))
    {
      DPRINT("InfGetData() failed for key 'Signature'\n");
      InfCloseFile(UnattendInf);
      return;
    }

  /* Check 'Signature' string */
  if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
      DPRINT("Signature not $ReactOS$\n");
      InfCloseFile(UnattendInf);
      return;
    }

  /* Search for 'DestinationDiskNumber' in the 'Unattend' section */
  if (!InfFindFirstLine(UnattendInf, L"Unattend", L"DestinationDiskNumber", &Context))
    {
      DPRINT("InfFindFirstLine() failed for key 'DestinationDiskNumber'\n");
      InfCloseFile(UnattendInf);
      return;
    }
  if (!InfGetIntField(&Context, 0, &IntValue))
    {
      DPRINT("InfGetIntField() failed for key 'DestinationDiskNumber'\n");
      InfCloseFile(UnattendInf);
      return;
    }
  UnattendDestinationDiskNumber = IntValue;

  /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
  if (!InfFindFirstLine(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
      DPRINT("InfFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
      InfCloseFile(UnattendInf);
      return;
    }
  if (!InfGetIntField(&Context, 0, &IntValue))
    {
      DPRINT("InfGetIntField() failed for key 'DestinationPartitionNumber'\n");
      InfCloseFile(UnattendInf);
      return;
    }
  UnattendDestinationPartitionNumber = IntValue;

  /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
  if (!InfFindFirstLine(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
      DPRINT("InfFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
      InfCloseFile(UnattendInf);
      return;
    }

  /* Get pointer 'InstallationDirectory' key */
  if (!InfGetData(&Context, NULL, &Value))
    {
      DPRINT("InfGetData() failed for key 'InstallationDirectory'\n");
      InfCloseFile(UnattendInf);
      return;
    }
  wcscpy(UnattendInstallationDirectory, Value);

  InfCloseFile(UnattendInf);

  IsUnattendedSetup = TRUE;

  DPRINT("Running unattended setup\n");
}


/*
 * Start page
 * RETURNS
 *	Number of the next page.
 */
static PAGE_NUMBER
StartPage(PINPUT_RECORD Ir)
{
  SYSTEM_DEVICE_INFORMATION Sdi;
  NTSTATUS Status;
  WCHAR FileNameBuffer[MAX_PATH];
  UNICODE_STRING FileName;
  INFCONTEXT Context;
  PWCHAR Value;
  ULONG ErrorLine;
  ULONG ReturnSize;

  SetStatusText("   Please wait...");


  /* Check whether a harddisk is available */
  Status = NtQuerySystemInformation (SystemDeviceInformation,
				     &Sdi,
				     sizeof(SYSTEM_DEVICE_INFORMATION),
				     &ReturnSize);
  if (!NT_SUCCESS (Status))
    {
      PrintTextXY(6, 15, "NtQuerySystemInformation() failed (Status 0x%08lx)", Status);
      PopupError("Setup could not retrieve system drive information.\n",
		 "ENTER = Reboot computer");
      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  if (Sdi.NumberOfDisks == 0)
    {
      PopupError("Setup could not find a harddisk.\n",
		 "ENTER = Reboot computer");
      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  /* Get the source path and source root path */
  Status = GetSourcePaths(&SourcePath,
			  &SourceRootPath);
  if (!NT_SUCCESS(Status))
    {
      PrintTextXY(6, 15, "GetSourcePath() failed (Status 0x%08lx)", Status);
      PopupError("Setup could not find its source drive.\n",
		 "ENTER = Reboot computer");
      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }
#if 0
  else
    {
      PrintTextXY(6, 15, "SourcePath: '%wZ'", &SourcePath);
      PrintTextXY(6, 16, "SourceRootPath: '%wZ'", &SourceRootPath);
    }
#endif

  /* Load txtsetup.sif from install media. */
  wcscpy(FileNameBuffer, SourceRootPath.Buffer);
  wcscat(FileNameBuffer, L"\\reactos\\txtsetup.sif");
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	}
    }

  CheckUnattendedSetup();

  return INTRO_PAGE;
}


/*
 * First setup page
 * RETURNS
 *	Next page number.
 */
static PAGE_NUMBER
IntroPage(PINPUT_RECORD Ir)
{
  SetHighlightedTextXY(6, 8, "Welcome to ReactOS Setup");

  SetTextXY(6, 11, "This part of the setup copies the ReactOS Operating System to your");
  SetTextXY(6, 12, "computer and prepares the second part of the setup.");

  SetTextXY(8, 15, "\x07  Press ENTER to install ReactOS.");
  SetTextXY(8, 17, "\x07  Press E to start the emergency console.");
  SetTextXY(8, 19, "\x07  Press R to repair ReactOS.");
  SetTextXY(8, 21, "\x07  Press F3 to quit without installing ReactOS.");

  SetStatusText("   ENTER = Continue   F3 = Quit");

  if (IsUnattendedSetup)
    {
      return INSTALL_INTRO_PAGE;
    }

  while (TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return INSTALL_INTRO_PAGE;
	}
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'E') /* E */
	{
	  return EMERGENCY_INTRO_PAGE;
	}
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R') /* R */
	{
	  return REPAIR_INTRO_PAGE;
	}
    }

  return INTRO_PAGE;
}


static PAGE_NUMBER
EmergencyIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The emergency console is not implemented yet.");

  SetTextXY(8, 15, "\x07  Press ESC to return to the main page.");

  SetTextXY(8, 17, "\x07  Press ENTER to reboot your computer.");

  SetStatusText("   ESC = Main page  ENTER = Reboot");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return REBOOT_PAGE;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  return INTRO_PAGE;
	}
    }

  return REPAIR_INTRO_PAGE;
}


static PAGE_NUMBER
RepairIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The repair functions are not implemented yet.");

  SetTextXY(8, 15, "\x07  Press ESC to return to the main page.");

  SetTextXY(8, 17, "\x07  Press ENTER to reboot your computer.");

  SetStatusText("   ESC = Main page  ENTER = Reboot");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return REBOOT_PAGE;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  return INTRO_PAGE;
	}
    }

  return REPAIR_INTRO_PAGE;
}


static PAGE_NUMBER
InstallIntroPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  SetTextXY(6, 12, "The following limitations apply:");
  SetTextXY(8, 13, "- Setup can not handle more than one primary partition per disk.");
  SetTextXY(8, 14, "- Setup can not delete a primary partition from a disk");
  SetTextXY(8, 15, "  as long as extended partitions exist on this disk.");
  SetTextXY(8, 16, "- Setup can not delete the first extended partition from a disk");
  SetTextXY(8, 17, "  as long as other extended partitions exist on this disk.");
  SetTextXY(8, 18, "- Setup supports FAT file systems only.");
  SetTextXY(8, 19, "- File system checks are not implemented yet.");


  SetTextXY(8, 23, "\x07  Press ENTER to install ReactOS.");

  SetTextXY(8, 25, "\x07  Press F3 to quit without installing ReactOS.");


  SetStatusText("   ENTER = Continue   F3 = Quit");

  if (IsUnattendedSetup)
    {
      return(SELECT_PARTITION_PAGE);
    }

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return INSTALL_INTRO_PAGE;
}


static PAGE_NUMBER
DeviceSettingsPage(PINPUT_RECORD Ir)
{
  static ULONG Line = 17;

  /* Initialize the computer settings list */
  if (ComputerList == NULL)
    {
      ComputerList = CreateComputerTypeList(SetupInf);
      if (ComputerList == NULL)
	{
	  /* FIXME: report error */
	}
    }

  /* Initialize the display settings list */
  if (DisplayList == NULL)
    {
      DisplayList = CreateDisplayDriverList(SetupInf);
      if (DisplayList == NULL)
	{
	  /* FIXME: report error */
	}
    }

  /* Initialize the keyboard settings list */
  if (KeyboardList == NULL)
    {
      KeyboardList = CreateKeyboardDriverList(SetupInf);
      if (KeyboardList == NULL)
	{
	  /* FIXME: report error */
	}
    }

  /* Initialize the keyboard layout list */
  if (LayoutList == NULL)
    {
      LayoutList = CreateKeyboardLayoutList(SetupInf);
      if (LayoutList == NULL)
	{
	  /* FIXME: report error */
	  PopupError("Setup failed to load the keyboard layout list.\n",
		     "ENTER = Reboot computer");

	  while (TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		{
		  return QUIT_PAGE;
		}
	    }
	}
    }

  /* Initialize the pointer settings list */
  if (PointerList == NULL)
    {
      PointerList = CreateMouseDriverList(SetupInf);
      if (PointerList == NULL)
	{
	  /* FIXME: report error */
	}
    }

  SetTextXY(6, 8, "The list below shows the current device settings.");

  SetTextXY(8, 11, "       Computer:");
  SetTextXY(8, 12, "        Display:");
  SetTextXY(8, 13, "       Keyboard:");
  SetTextXY(8, 14, "Keyboard layout:");
  SetTextXY(8, 15, " Pointer device:");

  SetTextXY(8, 17, "         Accept:");

  SetTextXY(25, 11, GetGenericListEntry(ComputerList)->Text);
  SetTextXY(25, 12, GetGenericListEntry(DisplayList)->Text);
  SetTextXY(25, 13, GetGenericListEntry(KeyboardList)->Text);
  SetTextXY(25, 14, GetGenericListEntry(LayoutList)->Text);
  SetTextXY(25, 15, GetGenericListEntry(PointerList)->Text);

  SetTextXY(25, 17, "Accept these device settings");
  InvertTextXY (24, Line, 48, 1);


  SetTextXY(6, 20, "You can change the hardware settings by pressing the UP or DOWN keys");
  SetTextXY(6, 21, "to select an entry. Then press the ENTER key to select alternative");
  SetTextXY(6, 22, "settings.");

  SetTextXY(6, 24, "When all settings are correct, select \"Accept these device settings\"");
  SetTextXY(6, 25, "and press ENTER.");

  SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  NormalTextXY (24, Line, 48, 1);
	  if (Line == 15)
	    Line = 17;
	  else if (Line == 17)
	    Line = 11;
	  else
	    Line++;
	  InvertTextXY (24, Line, 48, 1);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  NormalTextXY (24, Line, 48, 1);
	  if (Line == 11)
	    Line = 17;
	  else if (Line == 17)
	    Line = 15;
	  else
	    Line--;
	  InvertTextXY (24, Line, 48, 1);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  if (Line == 11)
	    return COMPUTER_SETTINGS_PAGE;
	  else if (Line == 12)
	    return DISPLAY_SETTINGS_PAGE;
	  else if (Line == 13)
	    return KEYBOARD_SETTINGS_PAGE;
	  else if (Line == 14)
	    return LAYOUT_SETTINGS_PAGE;
	  else if (Line == 15)
	    return POINTER_SETTINGS_PAGE;
	  else if (Line == 17)
	    return SELECT_PARTITION_PAGE;
	}
    }

  return DEVICE_SETTINGS_PAGE;
}


static PAGE_NUMBER
ComputerSettingsPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "You want to change the type of computer to be installed.");

  SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired computer type.");
  SetTextXY(8, 11, "   Then press ENTER.");

  SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  SetTextXY(8, 14, "   the computer type.");

  GetScreenSize(&xScreen, &yScreen);

  DrawGenericList(ComputerList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(ComputerList);

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownGenericList (ComputerList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpGenericList (ComputerList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  RestoreGenericListState(ComputerList);
	  return DEVICE_SETTINGS_PAGE;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return COMPUTER_SETTINGS_PAGE;
}


static PAGE_NUMBER
DisplaySettingsPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "You want to change the type of display to be installed.");

  SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired display type.");
  SetTextXY(8, 11, "   Then press ENTER.");

  SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  SetTextXY(8, 14, "   the display type.");

  GetScreenSize(&xScreen, &yScreen);

  DrawGenericList(DisplayList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(DisplayList);

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownGenericList (DisplayList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpGenericList (DisplayList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  RestoreGenericListState(DisplayList);
	  return DEVICE_SETTINGS_PAGE;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return DISPLAY_SETTINGS_PAGE;
}


static PAGE_NUMBER
KeyboardSettingsPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "You want to change the type of keyboard to be installed.");

  SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired keyboard type.");
  SetTextXY(8, 11, "   Then press ENTER.");

  SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  SetTextXY(8, 14, "   the keyboard type.");

  GetScreenSize(&xScreen, &yScreen);

  DrawGenericList(KeyboardList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(KeyboardList);

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownGenericList (KeyboardList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpGenericList (KeyboardList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  RestoreGenericListState(KeyboardList);
	  return DEVICE_SETTINGS_PAGE;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return DISPLAY_SETTINGS_PAGE;
}


static PAGE_NUMBER
LayoutSettingsPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "You want to change the keyboard layout to be installed.");

  SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired keyboard");
  SetTextXY(8, 11, "    layout. Then press ENTER.");

  SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  SetTextXY(8, 14, "   the keyboard layout.");

  GetScreenSize(&xScreen, &yScreen);

  DrawGenericList(LayoutList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(LayoutList);

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownGenericList (LayoutList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpGenericList (LayoutList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  RestoreGenericListState(LayoutList);
	  return DEVICE_SETTINGS_PAGE;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return DISPLAY_SETTINGS_PAGE;
}


static PAGE_NUMBER
PointerSettingsPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "You want to change the pointing device to be installed.");

  SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired pointing");
  SetTextXY(8, 11, "    device. Then press ENTER.");

  SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  SetTextXY(8, 14, "   the pointing device.");

  GetScreenSize(&xScreen, &yScreen);

  DrawGenericList(PointerList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(PointerList);

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownGenericList(PointerList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpGenericList(PointerList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  RestoreGenericListState(PointerList);
	  return DEVICE_SETTINGS_PAGE;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return DEVICE_SETTINGS_PAGE;
	}
    }

  return DISPLAY_SETTINGS_PAGE;
}


static PAGE_NUMBER
SelectPartitionPage(PINPUT_RECORD Ir)
{
  SHORT xScreen;
  SHORT yScreen;

  SetTextXY(6, 8, "The list below shows existing partitions and unused disk");
  SetTextXY(6, 9, "space for new partitions.");

  SetTextXY(8, 11, "\x07  Press UP or DOWN to select a list entry.");
  SetTextXY(8, 13, "\x07  Press ENTER to install ReactOS onto the selected partition.");
  SetTextXY(8, 15, "\x07  Press C to create a new partition.");
  SetTextXY(8, 17, "\x07  Press D to delete an existing partition.");

  SetStatusText("   Please wait...");

  GetScreenSize(&xScreen, &yScreen);

  if (PartitionList == NULL)
    {
      PartitionList = CreatePartitionList (2,
					   19,
					   xScreen - 3,
					   yScreen - 3);
      if (PartitionList == NULL)
	{
	  /* FIXME: show an error dialog */
	  return QUIT_PAGE;
	}
    }

  CheckActiveBootPartition (PartitionList);

  DrawPartitionList (PartitionList);

  /* Warn about partitions created by Linux Fdisk */
  if (WarnLinuxPartitions == TRUE &&
      CheckForLinuxFdiskPartitions (PartitionList) == TRUE)
    {
      PopupError ("Setup found that at least one harddisk contains an incompatible\n"
		  "partition table that can not be handled properly!\n"
		  "\n"
		  "Creating or deleting partitions can destroy the partiton table.\n"
		  "\n"
		  "  \x07  Press F3 to quit Setup."
		  "  \x07  Press ENTER to continue.",
		  "F3= Quit  ENTER = Continue");
      while (TRUE)
	{
	  ConInKey (Ir);

	  if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	      (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	    {
	      return QUIT_PAGE;
	    }
	  else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
	    {
	      WarnLinuxPartitions = FALSE;
	      return SELECT_PARTITION_PAGE;
	    }
	}
    }

  if (IsUnattendedSetup)
    {
      SelectPartition(PartitionList,
        UnattendDestinationDiskNumber,
        UnattendDestinationPartitionNumber);
      return(SELECT_FILE_SYSTEM_PAGE);
    }

  while(TRUE)
    {
      /* Update status text */
      if (PartitionList->CurrentPartition == NULL ||
	  PartitionList->CurrentPartition->Unpartitioned == TRUE)
	{
	  SetStatusText ("   ENTER = Install   C = Create Partition   F3 = Quit");
	}
      else
	{
	  SetStatusText ("   ENTER = Install   D = Delete Partition   F3 = Quit");
	}

      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    {
	      DestroyPartitionList (PartitionList);
	      PartitionList = NULL;
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownPartitionList (PartitionList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpPartitionList (PartitionList);
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
	{
	  if (PartitionList->CurrentPartition == NULL ||
	      PartitionList->CurrentPartition->Unpartitioned == TRUE)
	    {
	      CreateNewPartition (PartitionList,
				  0ULL,
				  TRUE);
	    }

	  return SELECT_FILE_SYSTEM_PAGE;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_C) /* C */
	{
	  if (PartitionList->CurrentPartition->Unpartitioned == FALSE)
	    {
	      PopupError ("You can not create a new Partition inside\n"
			  "of an already existing Partition!\n"
			  "\n"
			  "  * Press any key to continue.",
			  NULL);
	      ConInKey (Ir);

	      return SELECT_PARTITION_PAGE;
	    }

	  return CREATE_PARTITION_PAGE;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_D) /* D */
	{
	  if (PartitionList->CurrentPartition->Unpartitioned == TRUE)
	    {
	      PopupError ("You can not delete unpartitioned disk space!\n"
			  "\n"
			  "  * Press any key to continue.",
			  NULL);
	      ConInKey (Ir);

	      return SELECT_PARTITION_PAGE;
	    }

	  return DELETE_PARTITION_PAGE;
	}
    }

  return SELECT_PARTITION_PAGE;
}


static VOID
DrawInputField(ULONG FieldLength,
  SHORT Left,
  SHORT Top,
  PCHAR FieldContent)
{
  CHAR buf[100];
  COORD coPos;

  coPos.X = Left;
  coPos.Y = Top;
  memset(buf, '_', sizeof(buf));
  buf[FieldLength - strlen(FieldContent)] = 0;
  strcat(buf, FieldContent);

  WriteConsoleOutputCharacters (buf,
			        strlen (buf),
			        coPos);
}


#define PARTITION_SIZE_INPUT_FIELD_LENGTH 6

static VOID
ShowPartitionSizeInputBox(SHORT Left,
			  SHORT Top,
			  SHORT Right,
			  SHORT Bottom,
			  ULONG MaxSize,
			  PCHAR InputBuffer,
			  PBOOLEAN Quit,
			  PBOOLEAN Cancel)
{
  INPUT_RECORD Ir;
  COORD coPos;
  ULONG Written;
  SHORT i;
  CHAR Buffer[100];
  ULONG Index;
  CHAR ch;
  SHORT iLeft;
  SHORT iTop;

  if (Quit != NULL)
    *Quit = FALSE;

  if (Cancel != NULL)
    *Cancel = FALSE;

  /* draw upper left corner */
  coPos.X = Left;
  coPos.Y = Top;
  FillConsoleOutputCharacter(0xDA, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw upper edge */
  coPos.X = Left + 1;
  coPos.Y = Top;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Right - Left - 1,
			     coPos,
			     &Written);

  /* draw upper right corner */
  coPos.X = Right;
  coPos.Y = Top;
  FillConsoleOutputCharacter(0xBF, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw left and right edge */
  for (i = Top + 1; i < Bottom; i++)
    {
      coPos.X = Left;
      coPos.Y = i;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);

      coPos.X = Right;
      FillConsoleOutputCharacter(0xB3, //'|',
				 1,
				 coPos,
				 &Written);
    }

  /* draw lower left corner */
  coPos.X = Left;
  coPos.Y = Bottom;
  FillConsoleOutputCharacter(0xC0, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw lower edge */
  coPos.X = Left + 1;
  coPos.Y = Bottom;
  FillConsoleOutputCharacter(0xC4, // '-',
			     Right - Left - 1,
			     coPos,
			     &Written);

  /* draw lower right corner */
  coPos.X = Right;
  coPos.Y = Bottom;
  FillConsoleOutputCharacter(0xD9, // '+',
			     1,
			     coPos,
			     &Written);

  /* Print message */
  coPos.X = Left + 2;
  coPos.Y = Top + 2;
  strcpy (Buffer, "Size of new partition:");
  iLeft = coPos.X + strlen (Buffer) + 1;
  iTop = coPos.Y;
  WriteConsoleOutputCharacters (Buffer,
				 strlen (Buffer),
				 coPos);

  sprintf (Buffer, "MB (max. %lu MB)", MaxSize);
  coPos.X = iLeft + PARTITION_SIZE_INPUT_FIELD_LENGTH + 1;
  coPos.Y = iTop;
  WriteConsoleOutputCharacters (Buffer,
				strlen (Buffer),
				coPos);

  Buffer[0] = 0;
  Index = 0;
  DrawInputField (PARTITION_SIZE_INPUT_FIELD_LENGTH,
		  iLeft,
		  iTop,
		  Buffer);

  while (TRUE)
    {
      ConInKey (&Ir);

      if ((Ir.Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir.Event.KeyEvent.wVirtualKeyCode == VK_F3))	/* F3 */
	{
	  if (Quit != NULL)
	    *Quit = TRUE;
	  Buffer[0] = 0;
	  break;
	}
      else if (Ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)	/* ENTER */
	{
	  break;
	}
      else if (Ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)	/* ESCAPE */
	{
	  if (Cancel != NULL)
	    *Cancel = TRUE;
	  Buffer[0] = 0;
	  break;
	}
      else if ((Ir.Event.KeyEvent.wVirtualKeyCode == VK_BACK) &&  /* BACKSPACE */
	       (Index > 0))
	{
	  Index--;
	  Buffer[Index] = 0;
	  DrawInputField (PARTITION_SIZE_INPUT_FIELD_LENGTH,
			  iLeft,
			  iTop,
			  Buffer);
	}
      else if ((Ir.Event.KeyEvent.uChar.AsciiChar != 0x00) &&
	       (Index < PARTITION_SIZE_INPUT_FIELD_LENGTH))
	{
	  ch = Ir.Event.KeyEvent.uChar.AsciiChar;
	  if ((ch >= '0') && (ch <= '9'))
	    {
	      Buffer[Index] = ch;
	      Index++;
	      Buffer[Index] = 0;
	      DrawInputField (PARTITION_SIZE_INPUT_FIELD_LENGTH,
			      iLeft,
			      iTop,
			      Buffer);
	    }
	}
    }

  strcpy (InputBuffer,
	  Buffer);
}


static PAGE_NUMBER
CreatePartitionPage (PINPUT_RECORD Ir)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  SHORT xScreen;
  SHORT yScreen;
  BOOLEAN Quit;
  BOOLEAN Cancel;
  CHAR InputBuffer[50];
  ULONG MaxSize;
  ULONGLONG PartSize;
  ULONGLONG DiskSize;
  PCHAR Unit;

  if (PartitionList == NULL ||
      PartitionList->CurrentDisk == NULL ||
      PartitionList->CurrentPartition == NULL)
    {
      /* FIXME: show an error dialog */
      return QUIT_PAGE;
    }

  DiskEntry = PartitionList->CurrentDisk;
  PartEntry = PartitionList->CurrentPartition;

  SetStatusText ("   Please wait...");

  GetScreenSize (&xScreen, &yScreen);

  SetTextXY (6, 8, "You have chosen to create a new partition on");

#if 0
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 19)) >> 20;
      if (DiskSize == 0)
	DiskSize = 1;
      Unit = "MB";
    }

  if (DiskEntry->DriverName.Length > 0)
    {
      PrintTextXY (6, 10,
		   "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ.",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id,
		   &DiskEntry->DriverName);
    }
  else
    {
      PrintTextXY (6, 10,
		   "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu).",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id);
    }


  SetTextXY (6, 12, "Please enter the size of the new partition in megabytes.");

#if 0
  PrintTextXY (8, 10, "Maximum size of the new partition is %I64u MB",
	       PartitionList->CurrentPartition->UnpartitionedLength / (1024*1024));
#endif

  SetStatusText ("   ENTER = Create Partition   ESC = Cancel   F3 = Quit");

  PartEntry = PartitionList->CurrentPartition;
  while (TRUE)
    {
      MaxSize = (PartEntry->UnpartitionedLength + (1 << 19)) >> 20;  /* in MBytes (rounded) */
      ShowPartitionSizeInputBox (12, 14, xScreen - 12, 17, /* left, top, right, bottom */
				 MaxSize, InputBuffer, &Quit, &Cancel);
      if (Quit == TRUE)
	{
	  if (ConfirmQuit (Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	}
      else if (Cancel == TRUE)
	{
	  return SELECT_PARTITION_PAGE;
	}
      else
	{
	  PartSize = atoi (InputBuffer);
	  if (PartSize < 1)
	    {
	      /* Too small */
	      continue;
	    }

	  if (PartSize > MaxSize)
	    {
	      /* Too large */
	      continue;
	    }

	  /* Convert to bytes */
	  if (PartSize == MaxSize)
	    {
	      /* Use all of the unpartitioned disk space */
	      PartSize = PartEntry->UnpartitionedLength;
	    }
	  else
	    {
	      /* Round-up by cylinder size */
	      PartSize = ROUND_UP (PartSize * 1024 * 1024,
				   DiskEntry->CylinderSize);

	      /* But never get larger than the unpartitioned disk space */
	      if (PartSize > PartEntry->UnpartitionedLength)
		PartSize = PartEntry->UnpartitionedLength;
	    }

	  DPRINT ("Partition size: %I64u bytes\n", PartSize);

	  CreateNewPartition (PartitionList,
			      PartSize,
			      FALSE);

	  return SELECT_PARTITION_PAGE;
	}
    }

  return CREATE_PARTITION_PAGE;
}


static PAGE_NUMBER
DeletePartitionPage (PINPUT_RECORD Ir)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  ULONGLONG DiskSize;
  ULONGLONG PartSize;
  PCHAR Unit;
  PCHAR PartType;

  if (PartitionList == NULL ||
      PartitionList->CurrentDisk == NULL ||
      PartitionList->CurrentPartition == NULL)
    {
      /* FIXME: show an error dialog */
      return QUIT_PAGE;
    }

  DiskEntry = PartitionList->CurrentDisk;
  PartEntry = PartitionList->CurrentPartition;

  SetTextXY (6, 8, "You have chosen to delete the partition");

  /* Determine partition type */
  PartType = NULL;
  if (PartEntry->New == TRUE)
    {
      PartType = "New (Unformatted)";
    }
  else if (PartEntry->Unpartitioned == FALSE)
    {
      if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_12) ||
	  (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_16) ||
	  (PartEntry->PartInfo[0].PartitionType == PARTITION_HUGE) ||
	  (PartEntry->PartInfo[0].PartitionType == PARTITION_XINT13))
	{
	  PartType = "FAT";
	}
      else if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	       (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
	{
	  PartType = "FAT32";
	}
      else if (PartEntry->PartInfo[0].PartitionType == PARTITION_IFS)
	{
	  PartType = "NTFS"; /* FIXME: Not quite correct! */
	}
    }

#if 0
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0x280000000ULL) /* 10 GB */
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0xA00000ULL) /* 10 MB */
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 19)) >> 20;
      Unit = "MB";
    }
  else
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 9)) >> 10;
      Unit = "KB";
    }

  if (PartType == NULL)
    {
      PrintTextXY (6, 10,
		   "   %c%c  Type %lu    %I64u %s",
		   (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		   (PartEntry->DriveLetter == 0) ? '-' : ':',
		   PartEntry->PartInfo[0].PartitionType,
		   PartSize,
		   Unit);
    }
  else
    {
      PrintTextXY (6, 10,
		   "   %c%c  %s    %I64u %s",
		   (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		   (PartEntry->DriveLetter == 0) ? '-' : ':',
		   PartType,
		   PartSize,
		   Unit);
    }

#if 0
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 19)) >> 20;
      if (DiskSize == 0)
	DiskSize = 1;
      Unit = "MB";
    }

  if (DiskEntry->DriverName.Length > 0)
    {
      PrintTextXY (6, 12,
		   "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ.",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id,
		   &DiskEntry->DriverName);
    }
  else
    {
      PrintTextXY (6, 12,
		   "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu).",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id);
    }

  SetTextXY (8, 18, "\x07  Press D to delete the partition.");
  SetTextXY (11, 19, "WARNING: All data on this partition will be lost!");

  SetTextXY (8, 21, "\x07  Press ESC to cancel.");

  SetStatusText ("   D = Delete Partition   ESC = Cancel   F3 = Quit");

  while (TRUE)
    {
      ConInKey (Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit (Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)  /* ESC */
	{
	  return SELECT_PARTITION_PAGE;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_D) /* D */
	{
	  DeleteCurrentPartition (PartitionList);

	  return SELECT_PARTITION_PAGE;
	}
    }

  return DELETE_PARTITION_PAGE;
}


static PAGE_NUMBER
SelectFileSystemPage (PINPUT_RECORD Ir)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  ULONGLONG DiskSize;
  ULONGLONG PartSize;
  PCHAR DiskUnit;
  PCHAR PartUnit;
  PCHAR PartType;

  if (PartitionList == NULL ||
      PartitionList->CurrentDisk == NULL ||
      PartitionList->CurrentPartition == NULL)
    {
      /* FIXME: show an error dialog */
      return QUIT_PAGE;
    }

  DiskEntry = PartitionList->CurrentDisk;
  PartEntry = PartitionList->CurrentPartition;

  /* adjust disk size */
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 29)) >> 30;
      DiskUnit = "GB";
    }
  else
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 19)) >> 20;
      DiskUnit = "MB";
    }

  /* adjust partition size */
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0x280000000ULL) /* 10 GB */
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 29)) >> 30;
      PartUnit = "GB";
    }
  else
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 19)) >> 20;
      PartUnit = "MB";
    }

  /* adjust partition type */
  if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_12) ||
      (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT_16) ||
      (PartEntry->PartInfo[0].PartitionType == PARTITION_HUGE) ||
      (PartEntry->PartInfo[0].PartitionType == PARTITION_XINT13))
    {
      PartType = "FAT";
    }
  else if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	   (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
    {
      PartType = "FAT32";
    }
  else if (PartEntry->PartInfo[0].PartitionType == PARTITION_IFS)
    {
      PartType = "NTFS"; /* FIXME: Not quite correct! */
    }
  else if (PartEntry->PartInfo[0].PartitionType == PARTITION_ENTRY_UNUSED)
    {
      PartType = "Unused";
    }
  else
    {
      PartType = "Unknown";
    }

  if (PartEntry->AutoCreate == TRUE)
    {
      SetTextXY(6, 8, "Setup created a new partition on");

#if 0
  PrintTextXY(8, 10, "Partition %lu (%I64u %s) %s of",
	      PartEntry->PartInfo[0].PartitionNumber,
	      PartSize,
	      PartUnit,
	      PartType);
#endif

  PrintTextXY(8, 10, "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ).",
	      DiskEntry->DiskNumber,
	      DiskSize,
	      DiskUnit,
	      DiskEntry->Port,
	      DiskEntry->Bus,
	      DiskEntry->Id,
	      &DiskEntry->DriverName);

      SetTextXY(6, 12, "This Partition will be formatted next.");


      PartEntry->AutoCreate = FALSE;
    }
  else if (PartEntry->New == TRUE)
    {
      SetTextXY(6, 8, "You chose to install ReactOS on a new or unformatted Partition.");
      SetTextXY(6, 10, "This Partition will be formatted next.");
    }
  else
    {
      SetTextXY(6, 8, "Setup install ReactOS onto Partition");

      if (PartType == NULL)
	{
	  PrintTextXY (8, 10,
		       "%c%c  Type %lu    %I64u %s",
		       (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		       (PartEntry->DriveLetter == 0) ? '-' : ':',
		       PartEntry->PartInfo[0].PartitionType,
		       PartSize,
		       PartUnit);
	}
      else
	{
	  PrintTextXY (8, 10,
		       "%c%c  %s    %I64u %s",
		       (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		       (PartEntry->DriveLetter == 0) ? '-' : ':',
		       PartType,
		       PartSize,
		       PartUnit);
	}

      PrintTextXY(6, 12, "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ).",
		  DiskEntry->DiskNumber,
		  DiskSize,
		  DiskUnit,
		  DiskEntry->Port,
		  DiskEntry->Bus,
		  DiskEntry->Id,
		  &DiskEntry->DriverName);
    }


  SetTextXY(6, 17, "Select a file system from the list below.");

  SetTextXY(8, 19, "\x07  Press UP or DOWN to select a file system.");
  SetTextXY(8, 21, "\x07  Press ENTER to format the partition.");
  SetTextXY(8, 23, "\x07  Press ESC to select another partition.");

  if (FileSystemList == NULL)
    {
      FileSystemList = CreateFileSystemList (6, 26, PartEntry->New, FsFat);
      if (FileSystemList == NULL)
	{
	  /* FIXME: show an error dialog */
	  return QUIT_PAGE;
	}

      /* FIXME: Add file systems to list */
    }
  DrawFileSystemList (FileSystemList);

  SetStatusText ("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  if (IsUnattendedSetup)
    {
      return(CHECK_FILE_SYSTEM_PAGE);
    }

  while (TRUE)
    {
      ConInKey (Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit (Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)) /* ESC */
	{
	  return SELECT_PARTITION_PAGE;
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  ScrollDownFileSystemList (FileSystemList);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  ScrollUpFileSystemList (FileSystemList);
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
	{
	  if (FileSystemList->CurrentFileSystem == FsKeep)
	    {
	      return CHECK_FILE_SYSTEM_PAGE;
	    }
	  else
	    {
	      return FORMAT_PARTITION_PAGE;
	    }
	}
    }

  return SELECT_FILE_SYSTEM_PAGE;
}


static ULONG
FormatPartitionPage (PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  NTSTATUS Status;

#ifndef NDEBUG
  ULONG Line;
  ULONG i;
#endif


  SetTextXY(6, 8, "Format partition");

  SetTextXY(6, 10, "Setup will now format the partition. Press ENTER to continue.");

  SetStatusText("   ENTER = Continue   F3 = Quit");


  if (PartitionList == NULL ||
      PartitionList->CurrentDisk == NULL ||
      PartitionList->CurrentPartition == NULL)
    {
      /* FIXME: show an error dialog */
      return QUIT_PAGE;
    }

  DiskEntry = PartitionList->CurrentDisk;
  PartEntry = PartitionList->CurrentPartition;

  while(TRUE)
    {
      ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit (Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
	{
	  SetStatusText ("   Please wait ...");

	  if (PartEntry->PartInfo[0].PartitionType == PARTITION_ENTRY_UNUSED)
	    {
	      switch (FileSystemList->CurrentFileSystem)
	        {
		  case FsFat:
		    if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (4200ULL * 1024ULL))
		      {
			/* FAT12 CHS partition (disk is smaller than 4.1MB) */
			PartEntry->PartInfo[0].PartitionType = PARTITION_FAT_12;
		      }
		    else if (PartEntry->PartInfo[0].StartingOffset.QuadPart < (1024ULL * 255ULL * 63ULL * 512ULL))
		      {
			/* Partition starts below the 8.4GB boundary ==> CHS partition */

			if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (32ULL * 1024ULL * 1024ULL))
			  {
			    /* FAT16 CHS partition (partiton size < 32MB) */
			    PartEntry->PartInfo[0].PartitionType = PARTITION_FAT_16;
			  }
			else if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (512ULL * 1024ULL * 1024ULL))
			  {
			    /* FAT16 CHS partition (partition size < 512MB) */
			    PartEntry->PartInfo[0].PartitionType = PARTITION_HUGE;
			  }
			else
			  {
			    /* FAT32 CHS partition (partition size >= 512MB) */
			    PartEntry->PartInfo[0].PartitionType = PARTITION_FAT32;
			  }
		      }
		    else
		      {
			/* Partition starts above the 8.4GB boundary ==> LBA partition */

			if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (512ULL * 1024ULL * 1024ULL))
			  {
			    /* FAT16 LBA partition (partition size < 512MB) */
			    PartEntry->PartInfo[0].PartitionType = PARTITION_XINT13;
			  }
			else
			  {
			    /* FAT32 LBA partition (partition size >= 512MB) */
			    PartEntry->PartInfo[0].PartitionType = PARTITION_FAT32_XINT13;
			  }
		      }
		    break;

		  case FsKeep:
		    break;

		  default:
		    return QUIT_PAGE;
		}
	    }

	  CheckActiveBootPartition (PartitionList);

#ifndef NDEBUG
	  PrintTextXY (6, 12,
		       "Disk: %I64u  Cylinder: %I64u  Track: %I64u",
		       DiskEntry->DiskSize,
		       DiskEntry->CylinderSize,
		       DiskEntry->TrackSize);

	  Line = 13;
	  DiskEntry = PartitionList->CurrentDisk;
	  Entry = DiskEntry->PartListHead.Flink;
	  while (Entry != &DiskEntry->PartListHead)
	    {
	      PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

	      if (PartEntry->Unpartitioned == FALSE)
		{

		  for (i = 0; i < 4; i++)
		    {
		      PrintTextXY (6, Line,
				   "%2u:  %2u  %c  %12I64u  %12I64u  %2u  %c",
				   i,
				   PartEntry->PartInfo[i].PartitionNumber,
				   PartEntry->PartInfo[i].BootIndicator ? 'A' : '-',
				   PartEntry->PartInfo[i].StartingOffset.QuadPart,
				   PartEntry->PartInfo[i].PartitionLength.QuadPart,
				   PartEntry->PartInfo[i].PartitionType,
				   PartEntry->PartInfo[i].RewritePartition ? '*' : ' ');

		      Line++;
		    }

		  Line++;
		}

	      Entry = Entry->Flink;
	    }

	  /* Restore the old entry */
	  PartEntry = PartitionList->CurrentPartition;
#endif

	  if (WritePartitionsToDisk (PartitionList) == FALSE)
	    {
	      DPRINT ("WritePartitionsToDisk() failed\n");

	      PopupError ("Setup failed to write partition tables.\n",
			  "ENTER = Reboot computer");

	      while (TRUE)
		{
		  ConInKey (Ir);

		  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
		    {
		      return QUIT_PAGE;
		    }
		}
	    }

	  /* Set DestinationRootPath */
	  RtlFreeUnicodeString (&DestinationRootPath);
	  swprintf (PathBuffer,
		    L"\\Device\\Harddisk%lu\\Partition%lu",
		    PartitionList->CurrentDisk->DiskNumber,
		    PartitionList->CurrentPartition->PartInfo[0].PartitionNumber);
	  RtlCreateUnicodeString (&DestinationRootPath,
				  PathBuffer);
	  DPRINT ("DestinationRootPath: %wZ\n", &DestinationRootPath);


	  /* Set SystemRootPath */
	  RtlFreeUnicodeString (&SystemRootPath);
	  swprintf (PathBuffer,
		    L"\\Device\\Harddisk%lu\\Partition%lu",
		    PartitionList->ActiveBootDisk->DiskNumber,
		    PartitionList->ActiveBootPartition->PartInfo[0].PartitionNumber);
	  RtlCreateUnicodeString (&SystemRootPath,
				  PathBuffer);
	  DPRINT ("SystemRootPath: %wZ\n", &SystemRootPath);


	  switch (FileSystemList->CurrentFileSystem)
	    {
	      case FsFat:
		Status = FormatPartition (&DestinationRootPath);
		if (!NT_SUCCESS (Status))
		  {
		    DPRINT1 ("FormatPartition() failed with status 0x%.08x\n", Status);
		    /* FIXME: show an error dialog */
		    return QUIT_PAGE;
		  }

		PartEntry->New = FALSE;
		if (FileSystemList != NULL)
		  {
		    DestroyFileSystemList (FileSystemList);
		    FileSystemList = NULL;
		  }

		CheckActiveBootPartition (PartitionList);

		/* FIXME: Install boot code. This is a hack! */
		if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13) ||
		    (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32))
		  {
		    wcscpy (PathBuffer, SourceRootPath.Buffer);
		    wcscat (PathBuffer, L"\\loader\\fat32.bin");

		    DPRINT ("Install FAT32 bootcode: %S ==> %S\n", PathBuffer,
			    DestinationRootPath.Buffer);
		    Status = InstallFat32BootCodeToDisk (PathBuffer,
						         DestinationRootPath.Buffer);
		    if (!NT_SUCCESS (Status))
		      {
		        DPRINT1 ("InstallFat32BootCodeToDisk() failed with status 0x%.08x\n", Status);
		        /* FIXME: show an error dialog */
		        return QUIT_PAGE;
		      }
		  }
		else
		  {
		    wcscpy (PathBuffer, SourceRootPath.Buffer);
		    wcscat (PathBuffer, L"\\loader\\fat.bin");

		    DPRINT ("Install FAT bootcode: %S ==> %S\n", PathBuffer,
			    DestinationRootPath.Buffer);
		    Status = InstallFat16BootCodeToDisk (PathBuffer,
						         DestinationRootPath.Buffer);
		    if (!NT_SUCCESS (Status))
		      {
		        DPRINT1 ("InstallFat16BootCodeToDisk() failed with status 0x%.08x\n", Status);
		        /* FIXME: show an error dialog */
		        return QUIT_PAGE;
		      }
		  }
		break;

	      case FsKeep:
		break;

	      default:
		return QUIT_PAGE;
	    }

#ifndef NDEBUG
	  SetStatusText ("   Done.  Press any key ...");
	  ConInKey(Ir);
#endif

	  return INSTALL_DIRECTORY_PAGE;
	}
    }

  return FORMAT_PARTITION_PAGE;
}


static ULONG
CheckFileSystemPage(PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];

  SetTextXY(6, 8, "Check file system");

  SetTextXY(6, 10, "At present, ReactOS can not check file systems.");

  SetStatusText("   Please wait ...");


  SetStatusText("   ENTER = Continue   F3 = Quit");


  /* Set DestinationRootPath */
  RtlFreeUnicodeString (&DestinationRootPath);
  swprintf (PathBuffer,
	    L"\\Device\\Harddisk%lu\\Partition%lu",
	    PartitionList->CurrentDisk->DiskNumber,
	    PartitionList->CurrentPartition->PartInfo[0].PartitionNumber);
  RtlCreateUnicodeString (&DestinationRootPath,
			  PathBuffer);
  DPRINT ("DestinationRootPath: %wZ\n", &DestinationRootPath);

  /* Set SystemRootPath */
  RtlFreeUnicodeString (&SystemRootPath);
  swprintf (PathBuffer,
	    L"\\Device\\Harddisk%lu\\Partition%lu",
	    PartitionList->ActiveBootDisk->DiskNumber,
	    PartitionList->ActiveBootPartition->PartInfo[0].PartitionNumber);
  RtlCreateUnicodeString (&SystemRootPath,
			  PathBuffer);
  DPRINT ("SystemRootPath: %wZ\n", &SystemRootPath);


  if (IsUnattendedSetup)
    {
      return(INSTALL_DIRECTORY_PAGE);
    }

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
InstallDirectoryPage1(PWCHAR InstallDir, PDISKENTRY DiskEntry, PPARTENTRY PartEntry)
{
  WCHAR PathBuffer[MAX_PATH];

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
	   DiskEntry->DiskNumber,
	   PartEntry->PartInfo[0].PartitionNumber);
  if (InstallDir[0] != L'\\')
    wcscat(PathBuffer,
	   L"\\");
  wcscat(PathBuffer, InstallDir);
  RtlCreateUnicodeString(&DestinationArcPath,
			 PathBuffer);

  return(PREPARE_COPY_PAGE);
}


static PAGE_NUMBER
InstallDirectoryPage(PINPUT_RECORD Ir)
{
  PDISKENTRY DiskEntry;
  PPARTENTRY PartEntry;
  WCHAR InstallDir[51];
  PWCHAR DefaultPath;
  INFCONTEXT Context;
  ULONG Length;

  if (PartitionList == NULL ||
      PartitionList->CurrentDisk == NULL ||
      PartitionList->CurrentPartition == NULL)
    {
      /* FIXME: show an error dialog */
      return QUIT_PAGE;
    }

  DiskEntry = PartitionList->CurrentDisk;
  PartEntry = PartitionList->CurrentPartition;

  /* Search for 'DefaultPath' in the 'SetupData' section */
  if (!InfFindFirstLine (SetupInf, L"SetupData", L"DefaultPath", &Context))
    {
      PopupError("Setup failed to find the 'SetupData' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer");

      while (TRUE)
	{
	  ConInKey (Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
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

  if (IsUnattendedSetup)
    {
      return(InstallDirectoryPage1 (InstallDir, DiskEntry, PartEntry));
    }

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
	  return (InstallDirectoryPage1 (InstallDir, DiskEntry, PartEntry));
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


static BOOLEAN
PrepareCopyPageInfFile(HINF InfFile,
		       PWCHAR SourceCabinet,
		       PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  INFCONTEXT FilesContext;
  INFCONTEXT DirContext;
  PWCHAR KeyValue;
  ULONG Length;
  NTSTATUS Status;
  PWCHAR FileKeyName;
  PWCHAR FileKeyValue;
  PWCHAR DirKeyValue;
  PWCHAR TargetFileName;

  /* Search for the 'SourceFiles' section */
  if (!InfFindFirstLine (InfFile, L"SourceFiles", NULL, &FilesContext))
    {
      PopupError("Setup failed to find the 'SourceFiles' section\n"
		 "in TXTSETUP.SIF.\n",  // FIXME
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(FALSE);
	    }
	}
    }

  /*
   * Enumerate the files in the 'SourceFiles' section
   * and add them to the file queue.
   */
  do
    {
      /* Get source file name and target directory id */
      if (!InfGetData (&FilesContext, &FileKeyName, &FileKeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("InfGetData() failed\n");
	  break;
	}

      /* Get optional target file name */
      if (!InfGetDataField (&FilesContext, 2, &TargetFileName))
	TargetFileName = NULL;

      DPRINT ("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

      /* Lookup target directory */
      if (!InfFindFirstLine (InfFile, L"Directories", FileKeyValue, &DirContext))
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
			  SourceCabinet,
			  SourceRootPath.Buffer,
			  L"\\reactos",
			  FileKeyName,
			  DirKeyValue,
			  TargetFileName))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("SetupQueueCopy() failed\n");
	}
    }
  while (InfFindNextLine(&FilesContext, &FilesContext));


  /* Create directories */

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
	      return(FALSE);
	    }
	}
    }


  /* Search for the 'Directories' section */
  if (!InfFindFirstLine(InfFile, L"Directories", NULL, &DirContext))
    {
      if (SourceCabinet)
	{
	  PopupError("Setup failed to find the 'Directories' section\n"
		     "in the cabinet.\n", "ENTER = Reboot computer");
	}
      else
	{
	  PopupError("Setup failed to find the 'Directories' section\n"
		     "in TXTSETUP.SIF.\n", "ENTER = Reboot computer");
        }

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return(FALSE);
	    }
	}
    }

  /* Enumerate the directory values and create the subdirectories */
  do
    {
      if (!InfGetData (&DirContext, NULL, &KeyValue))
	{
	  DPRINT1("break\n");
	  break;
	}

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
		      return(FALSE);
		    }
		}
	    }
	}
    }
  while (InfFindNextLine (&DirContext, &DirContext));

  return(TRUE);
}


static PAGE_NUMBER
PrepareCopyPage(PINPUT_RECORD Ir)
{
  HINF InfHandle;
  WCHAR PathBuffer[MAX_PATH];
  INFCONTEXT CabinetsContext;
  ULONG InfFileSize;
  PWCHAR KeyValue;
  NTSTATUS Status;
  ULONG ErrorLine;
  PVOID InfFileData;

  SetTextXY(6, 8, "Setup prepares your computer for copying the ReactOS files. ");

  SetStatusText("   Building the file copy list...");

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

  if (!PrepareCopyPageInfFile(SetupInf, NULL, Ir))
    {
      return QUIT_PAGE;
    }

  /* Search for the 'Cabinets' section */
  if (!InfFindFirstLine (SetupInf, L"Cabinets", NULL, &CabinetsContext))
    {
      return FILE_COPY_PAGE;
    }

  /*
   * Enumerate the directory values in the 'Cabinets'
   * section and parse their inf files.
   */
  do
    {
      if (!InfGetData (&CabinetsContext, NULL, &KeyValue))
	break;

      wcscpy(PathBuffer, SourcePath.Buffer);
      wcscat(PathBuffer, L"\\");
      wcscat(PathBuffer, KeyValue);

      CabinetInitialize();
      CabinetSetEventHandlers(NULL, NULL, NULL);
      CabinetSetCabinetName(PathBuffer);

      if (CabinetOpen() == CAB_STATUS_SUCCESS)
	{
	  DPRINT("Cabinet %S\n", CabinetGetCabinetName());

	  InfFileData = CabinetGetCabinetReservedArea(&InfFileSize);
	  if (InfFileData == NULL)
	    {
	      PopupError("Cabinet has no setup script.\n",
			 "ENTER = Reboot computer");

	      while(TRUE)
		{
		  ConInKey(Ir);

		  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		    {
		      return QUIT_PAGE;
		    }
		}
	    }
	}
      else
	{
	  DPRINT("Cannot open cabinet: %S.\n", CabinetGetCabinetName());

	  PopupError("Cabinet not found.\n",
		     "ENTER = Reboot computer");

	  while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		{
		  return QUIT_PAGE;
		}
	    }
	}

      Status = InfOpenBufferedFile(&InfHandle,
				   InfFileData,
				   InfFileSize,
				   &ErrorLine);
      if (!NT_SUCCESS(Status))
	{
	  PopupError("Cabinet has no valid inf file.\n",
		     "ENTER = Reboot computer");

	  while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		{
		  return QUIT_PAGE;
		}
	    }
	}

      CabinetCleanup();

      if (!PrepareCopyPageInfFile(InfHandle, KeyValue, Ir))
        {
          return QUIT_PAGE;
        }
    }
  while (InfFindNextLine (&CabinetsContext, &CabinetsContext));

  return FILE_COPY_PAGE;
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

  return 0;
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

  return REGISTRY_PAGE;
}


static PAGE_NUMBER
RegistryPage(PINPUT_RECORD Ir)
{
  INFCONTEXT InfContext;
  PWSTR Action;
  PWSTR File;
  PWSTR Section;
  BOOLEAN Delete;
  NTSTATUS Status;

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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	}
    }

  do
    {
      InfGetDataField (&InfContext, 0, &Action);
      InfGetDataField (&InfContext, 1, &File);
      InfGetDataField (&InfContext, 2, &Section);

      DPRINT("Action: %S  File: %S  Section %S\n", Action, File, Section);

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
	  DPRINT("Importing %S failed\n", File);

	  PopupError("Setup failed to import a hive file.",
		     "ENTER = Reboot computer");

	  while(TRUE)
	    {
	      ConInKey(Ir);

	      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
		{
		  return QUIT_PAGE;
		}
	    }
	}
    }
  while (InfFindNextLine (&InfContext, &InfContext));

  /* Update display registry settings */
  SetStatusText("   Updating display registry settings...");
  if (!ProcessDisplayRegistry(SetupInf, DisplayList))
    {
      PopupError("Setup failed to update display registry settings.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  /* Update keyboard layout settings */
  SetStatusText("   Updating keyboard layout settings...");
  if (!ProcessKeyboardLayoutRegistry(LayoutList))
    {
      PopupError("Setup failed to update keyboard layout settings.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  /* Update mouse registry settings */
  SetStatusText("   Updating mouse registry settings...");
  if (!ProcessMouseRegistry(SetupInf, PointerList))
    {
      PopupError("Setup failed to update mouse registry settings.",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  SetStatusText("   Done...");

  return BOOT_LOADER_PAGE;
}


static PAGE_NUMBER
BootLoaderPage(PINPUT_RECORD Ir)
{
  WCHAR SrcPath[MAX_PATH];
  WCHAR DstPath[MAX_PATH];
  NTSTATUS Status;

  SetTextXY(6, 8, "Installing the boot loader");

  SetStatusText("   Please wait...");

  if (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_ENTRY_UNUSED)
    {
      DPRINT1("Error: active partition invalid (unused)\n");
      PopupError("The active partition is unused (invalid).\n",
		 "ENTER = Reboot computer");

      while(TRUE)
	{
	  ConInKey(Ir);

	  if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	    {
	      return QUIT_PAGE;
	    }
	}
    }

  if (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == 0x0A)
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
	      return QUIT_PAGE;
	    }
	}
    }
  else if (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == 0x83)
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
	      return QUIT_PAGE;
	    }
	}
    }
  else if (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_IFS)
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
	      return QUIT_PAGE;
	    }
	}
    }
  else if ((PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT_12) ||
	   (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT_16) ||
	   (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_HUGE) ||
	   (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_XINT13) ||
	   (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	   (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
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
	    return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	  }
	}

	/* Install new bootcode */
	if ((PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	    (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
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
		return QUIT_PAGE;
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
		return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	    return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	  }
	}

	/* Install new bootsector */
	if ((PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	    (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
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
		return QUIT_PAGE;
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
		return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	    return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	  }
	}

	/* Install new bootsector */
	if ((PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32) ||
	    (PartitionList->ActiveBootPartition->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13))
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
		return QUIT_PAGE;
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
		return QUIT_PAGE;
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
	      return QUIT_PAGE;
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
	      return QUIT_PAGE;
	    }
	}
    }

  return SUCCESS_PAGE;
}


static PAGE_NUMBER
QuitPage(PINPUT_RECORD Ir)
{
  SetTextXY(10, 6, "ReactOS is not completely installed");

  SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  SetTextXY(10, 9, "all CD-ROMs from CD-Drives.");

  SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  SetStatusText("   Please wait ...");

  /* Destroy partition list */
  if (PartitionList != NULL)
    {
      DestroyPartitionList (PartitionList);
      PartitionList = NULL;
    }

  /* Destroy filesystem list */
  if (FileSystemList != NULL)
    {
      DestroyFileSystemList (FileSystemList);
      FileSystemList = NULL;
    }

  /* Destroy computer settings list */
  if (ComputerList != NULL)
    {
      DestroyGenericList(ComputerList, TRUE);
      ComputerList = NULL;
    }

  /* Destroy display settings list */
  if (DisplayList != NULL)
    {
      DestroyGenericList(DisplayList, TRUE);
      DisplayList = NULL;
    }

  /* Destroy keyboard settings list */
  if (KeyboardList != NULL)
    {
      DestroyGenericList(KeyboardList, TRUE);
      KeyboardList = NULL;
    }

  /* Destroy keyboard layout list */
  if (LayoutList != NULL)
    {
      DestroyGenericList(LayoutList, TRUE);
      LayoutList = NULL;
    }

  /* Destroy pointer device list */
  if (PointerList != NULL)
    {
      DestroyGenericList(PointerList, TRUE);
      PointerList = NULL;
    }

  SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return FLUSH_PAGE;
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

  if (IsUnattendedSetup)
    {
      return FLUSH_PAGE;
    }

  while(TRUE)
    {
      ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return FLUSH_PAGE;
	}
    }
}


static PAGE_NUMBER
FlushPage(PINPUT_RECORD Ir)
{
  SetTextXY(10, 6, "The system is now making sure all data is stored on your disk");

  SetTextXY(10, 8, "This may take a minute");
  SetTextXY(10, 9, "When finished, your computer will reboot automatically");

  SetStatusText("   Flushing cache");

  return REBOOT_PAGE;
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


  /* Initialize global unicode strings */
  RtlInitUnicodeString(&SourcePath, NULL);
  RtlInitUnicodeString(&SourceRootPath, NULL);
  RtlInitUnicodeString(&InstallPath, NULL);
  RtlInitUnicodeString(&DestinationPath, NULL);
  RtlInitUnicodeString(&DestinationArcPath, NULL);
  RtlInitUnicodeString(&DestinationRootPath, NULL);
  RtlInitUnicodeString(&SystemRootPath, NULL);

  /* Hide the cursor */
  SetCursorType(TRUE, FALSE);

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

	  case DEVICE_SETTINGS_PAGE:
	    Page = DeviceSettingsPage(&Ir);
	    break;

	  case COMPUTER_SETTINGS_PAGE:
	    Page = ComputerSettingsPage(&Ir);
	    break;

	  case DISPLAY_SETTINGS_PAGE:
	    Page = DisplaySettingsPage(&Ir);
	    break;

	  case KEYBOARD_SETTINGS_PAGE:
	    Page = KeyboardSettingsPage(&Ir);
	    break;

	  case LAYOUT_SETTINGS_PAGE:
	    Page = LayoutSettingsPage(&Ir);
	    break;

	  case POINTER_SETTINGS_PAGE:
	    Page = PointerSettingsPage(&Ir);
	    break;

	  case SELECT_PARTITION_PAGE:
	    Page = SelectPartitionPage(&Ir);
	    break;

	  case CREATE_PARTITION_PAGE:
	    Page = CreatePartitionPage(&Ir);
	    break;

	  case DELETE_PARTITION_PAGE:
	    Page = DeletePartitionPage(&Ir);
	    break;

	  case SELECT_FILE_SYSTEM_PAGE:
	    Page = SelectFileSystemPage(&Ir);
	    break;

	  case FORMAT_PARTITION_PAGE:
	    Page = FormatPartitionPage(&Ir);
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


	  /* Emergency pages */
	  case EMERGENCY_INTRO_PAGE:
	    Page = EmergencyIntroPage(&Ir);
	    break;


	  case SUCCESS_PAGE:
	    Page = SuccessPage(&Ir);
	    break;

	  case FLUSH_PAGE:
	    Page = FlushPage(&Ir);
	    break;

	  case QUIT_PAGE:
	    Page = QuitPage(&Ir);
	    break;

	  case REBOOT_PAGE:
	    break;
	}
    }

  /* Reboot */
  FreeConsole();
  NtShutdownSystem(ShutdownReboot);
}

/* EOF */
