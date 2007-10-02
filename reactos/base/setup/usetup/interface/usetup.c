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
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

typedef enum _PAGE_NUMBER
{
  START_PAGE,
  INTRO_PAGE,
  LICENSE_PAGE,
  INSTALL_INTRO_PAGE,

//  SCSI_CONTROLLER_PAGE,

  DEVICE_SETTINGS_PAGE,
  COMPUTER_SETTINGS_PAGE,
  DISPLAY_SETTINGS_PAGE,
  KEYBOARD_SETTINGS_PAGE,
  LAYOUT_SETTINGS_PAGE,

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
  BOOT_LOADER_FLOPPY_PAGE,
  BOOT_LOADER_HARDDISK_PAGE,

  REPAIR_INTRO_PAGE,

  SUCCESS_PAGE,
  QUIT_PAGE,
  FLUSH_PAGE,
  REBOOT_PAGE,			/* virtual page */
} PAGE_NUMBER, *PPAGE_NUMBER;

/* GLOBALS ******************************************************************/

HANDLE ProcessHeap;
UNICODE_STRING SourceRootPath;
UNICODE_STRING SourceRootDir;
UNICODE_STRING SourcePath;
BOOLEAN IsUnattendedSetup = FALSE;
LONG UnattendDestinationDiskNumber;
LONG UnattendDestinationPartitionNumber;
LONG UnattendMBRInstallType = -1;
LONG UnattendFormatPartition = 0;
LONG AutoPartition = 0;
WCHAR UnattendInstallationDirectory[MAX_PATH];
BOOLEAN RepairUpdateFlag = FALSE;

/* LOCALS *******************************************************************/

static PPARTLIST PartitionList = NULL;

static PFILE_SYSTEM_LIST FileSystemList = NULL;

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


#define POPUP_WAIT_NONE    0
#define POPUP_WAIT_ANY_KEY 1
#define POPUP_WAIT_ENTER   2

static VOID
DrawBox(
	IN SHORT xLeft,
	IN SHORT yTop,
	IN SHORT Width,
	IN SHORT Height)
{
	COORD coPos;
	DWORD Written;

	/* draw upper left corner */
	coPos.X = xLeft;
	coPos.Y = yTop;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xDA, // '+',
		1,
		coPos,
		&Written);

	/* draw upper edge */
	coPos.X = xLeft + 1;
	coPos.Y = yTop;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xC4, // '-',
		Width - 2,
		coPos,
		&Written);

	/* draw upper right corner */
	coPos.X = xLeft + Width - 1;
	coPos.Y = yTop;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xBF, // '+',
		1,
		coPos,
		&Written);

	/* Draw right edge, inner space and left edge */
	for (coPos.Y = yTop + 1; coPos.Y < yTop + Height - 1; coPos.Y++)
	{
		coPos.X = xLeft;
		FillConsoleOutputCharacterA(
		StdOutput,
		0xB3, // '|',
		1,
		coPos,
		&Written);

		coPos.X = xLeft + 1;
		FillConsoleOutputCharacterA(
			StdOutput,
			' ',
			Width - 2,
			coPos,
			&Written);

		coPos.X = xLeft + Width - 1;
		FillConsoleOutputCharacterA(
			StdOutput,
			0xB3, // '|',
			1,
			coPos,
			&Written);
	}

	/* draw lower left corner */
	coPos.X = xLeft;
	coPos.Y = yTop + Height - 1;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xC0, // '+',
		1,
		coPos,
		&Written);

	/* draw lower edge */
	coPos.X = xLeft + 1;
	coPos.Y = yTop + Height - 1;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xC4, // '-',
		Width - 2,
		coPos,
		&Written);

	/* draw lower right corner */
	coPos.X = xLeft + Width - 1;
	coPos.Y = yTop + Height - 1;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xD9, // '+',
		1,
		coPos,
		&Written);
}

static VOID
PopupError(PCHAR Text,
	   PCHAR Status,
	   PINPUT_RECORD Ir,
	   ULONG WaitEvent)
{
  SHORT yTop;
  SHORT xLeft;
  COORD coPos;
  DWORD Written;
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
      FillConsoleOutputAttribute(StdOutput,
				 FOREGROUND_RED | BACKGROUND_WHITE,
				 Width,
				 coPos,
				 &Written);
    }

  DrawBox(xLeft, yTop, Width, Height);

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
	  WriteConsoleOutputCharacterA(StdOutput,
				       pnext,
				       Length,
				       coPos,
				       &Written);
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
      FillConsoleOutputCharacterA(StdOutput,
				 0xC3, // '+',
				 1,
				 coPos,
				 &Written);

      coPos.X = xLeft + 1;
      FillConsoleOutputCharacterA(StdOutput,
				 0xC4, // '-',
				 Width - 2,
				 coPos,
				 &Written);

      coPos.X = xLeft + Width - 1;
      FillConsoleOutputCharacterA(StdOutput,
				 0xB4, // '+',
				 1,
				 coPos,
				 &Written);

      coPos.Y++;
      coPos.X = xLeft + 2;
      WriteConsoleOutputCharacterA(StdOutput,
				   Status,
				   min(strlen(Status), (SIZE_T)Width - 4),
				   coPos,
				   &Written);
    }

  if (WaitEvent == POPUP_WAIT_NONE)
    return;

  while (TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (WaitEvent == POPUP_WAIT_ANY_KEY
       || Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
        {
	      return;
	    }
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
	     "F3= Quit  ENTER = Continue",
	     NULL, POPUP_WAIT_NONE);

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
  INFCONTEXT Context;
  HINF UnattendInf;
  UINT ErrorLine;
  INT IntValue;
  PWCHAR Value;

  if (DoesFileExist(SourcePath.Buffer, L"unattend.inf") == FALSE)
    {
      DPRINT("Does not exist: %S\\%S\n", SourcePath.Buffer, L"unattend.inf");
      return;
    }

  wcscpy(UnattendInfPath, SourcePath.Buffer);
  wcscat(UnattendInfPath, L"\\unattend.inf");

  /* Load 'unattend.inf' from install media. */
  UnattendInf = SetupOpenInfFileW(UnattendInfPath,
		       NULL,
		       INF_STYLE_WIN4,
		       &ErrorLine);
  if (UnattendInf == INVALID_HANDLE_VALUE)
    {
      DPRINT("SetupOpenInfFileW() failed\n");
      return;
    }

  /* Open 'Unattend' section */
  if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"Signature", &Context))
    {
      DPRINT("SetupFindFirstLineW() failed for section 'Unattend'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }

  /* Get pointer 'Signature' key */
  if (!INF_GetData(&Context, NULL, &Value))
    {
      DPRINT("INF_GetData() failed for key 'Signature'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }

  /* Check 'Signature' string */
  if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
      DPRINT("Signature not $ReactOS$\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }

  /* Check if Unattend setup is enabled */
  if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"UnattendSetupEnabled", &Context))
    {
      DPRINT("Can't find key 'UnattendSetupEnabled'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  if (!INF_GetData(&Context, NULL, &Value))
    {
      DPRINT("Can't read key 'UnattendSetupEnabled'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  if (_wcsicmp(Value, L"yes") != 0)
    {
      DPRINT("Unattend setup is disabled by 'UnattendSetupEnabled' key!\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }

  /* Search for 'DestinationDiskNumber' in the 'Unattend' section */
  if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"DestinationDiskNumber", &Context))
    {
      DPRINT("SetupFindFirstLine() failed for key 'DestinationDiskNumber'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  if (!SetupGetIntField(&Context, 1, &IntValue))
    {
      DPRINT("SetupGetIntField() failed for key 'DestinationDiskNumber'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  UnattendDestinationDiskNumber = (LONG)IntValue;

  /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
  if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
      DPRINT("SetupFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  if (!SetupGetIntField(&Context, 1, &IntValue))
    {
      DPRINT("SetupGetIntField() failed for key 'DestinationPartitionNumber'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  UnattendDestinationPartitionNumber = IntValue;

  /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
  if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
      DPRINT("SetupFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }

  /* Get pointer 'InstallationDirectory' key */
  if (!INF_GetData(&Context, NULL, &Value))
    {
      DPRINT("INF_GetData() failed for key 'InstallationDirectory'\n");
      SetupCloseInfFile(UnattendInf);
      return;
    }
  wcscpy(UnattendInstallationDirectory, Value);

  IsUnattendedSetup = TRUE;

  /* Search for 'MBRInstallType' in the 'Unattend' section */
  if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"MBRInstallType", &Context))
    {
      if (SetupGetIntField(&Context, 1, &IntValue))
        {
          UnattendMBRInstallType = IntValue;
        }
    }
  /* Search for 'FormatPartition' in the 'Unattend' section */
  if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"FormatPartition", &Context))
    {
      if (SetupGetIntField(&Context, 1, &IntValue))
        {
          UnattendFormatPartition = IntValue;
        }
    }
  if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"AutoPartition", &Context))
    {
      if (SetupGetIntField(&Context, 1, &IntValue))
        {
          AutoPartition = IntValue;
        }
    }
  SetupCloseInfFile(UnattendInf);

  DPRINT("Running unattended setup\n");
}


/*
 * Start page
 * RETURNS
 *	Number of the next page.
 */
static PAGE_NUMBER
SetupStartPage(PINPUT_RECORD Ir)
{
  SYSTEM_DEVICE_INFORMATION Sdi;
  NTSTATUS Status;
  WCHAR FileNameBuffer[MAX_PATH];
  INFCONTEXT Context;
  PWCHAR Value;
  UINT ErrorLine;
  SIZE_T ReturnSize;

  CONSOLE_SetStatusText("   Please wait...");


  /* Check whether a harddisk is available */
  Status = NtQuerySystemInformation (SystemDeviceInformation,
				     &Sdi,
				     sizeof(SYSTEM_DEVICE_INFORMATION),
				     &ReturnSize);
  if (!NT_SUCCESS (Status))
    {
      CONSOLE_PrintTextXY(6, 15, "NtQuerySystemInformation() failed (Status 0x%08lx)", Status);
      PopupError("Setup could not retrieve system drive information.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  if (Sdi.NumberOfDisks == 0)
    {
      PopupError("Setup could not find a harddisk.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Get the source path and source root path */
  Status = GetSourcePaths(&SourcePath,
			  &SourceRootPath,
			  &SourceRootDir);
  if (!NT_SUCCESS(Status))
    {
      CONSOLE_PrintTextXY(6, 15, "GetSourcePaths() failed (Status 0x%08lx)", Status);
      PopupError("Setup could not find its source drive.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }
#if 0
  else
    {
      CONSOLE_PrintTextXY(6, 15, "SourcePath: '%wZ'", &SourcePath);
      CONSOLE_PrintTextXY(6, 16, "SourceRootPath: '%wZ'", &SourceRootPath);
      CONSOLE_PrintTextXY(6, 17, "SourceRootDir: '%wZ'", &SourceRootDir);
    }
#endif

  /* Load txtsetup.sif from install media. */
  wcscpy(FileNameBuffer, SourcePath.Buffer);
  wcscat(FileNameBuffer, L"\\txtsetup.sif");

  SetupInf = SetupOpenInfFileW(FileNameBuffer,
		       NULL,
		       INF_STYLE_WIN4,
		       &ErrorLine);
  if (SetupInf == INVALID_HANDLE_VALUE)
    {
      PopupError("Setup failed to load the file TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Open 'Version' section */
  if (!SetupFindFirstLineW (SetupInf, L"Version", L"Signature", &Context))
    {
      PopupError("Setup found a corrupt TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }


  /* Get pointer 'Signature' key */
  if (!INF_GetData (&Context, NULL, &Value))
    {
      PopupError("Setup found a corrupt TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Check 'Signature' string */
  if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
      PopupError("Setup found an invalid signature in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
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
  CONSOLE_SetHighlightedTextXY(6, 8, "Welcome to ReactOS Setup");

  CONSOLE_SetTextXY(6, 11, "This part of the setup copies the ReactOS Operating System to your");
  CONSOLE_SetTextXY(6, 12, "computer and prepares the second part of the setup.");

  CONSOLE_SetTextXY(8, 15, "\x07  Press ENTER to install ReactOS.");
  CONSOLE_SetTextXY(8, 17, "\x07  Press R to repair ReactOS.");
  CONSOLE_SetTextXY(8, 19, "\x07  Press L to view the ReactOS Licensing Terms and Conditions");
  CONSOLE_SetTextXY(8, 21, "\x07  Press F3 to quit without installing ReactOS.");

  CONSOLE_SetTextXY(6, 23, "For more information on ReactOS, please visit:");
  CONSOLE_SetHighlightedTextXY(6, 24, "http://www.reactos.org");

  CONSOLE_SetStatusText("   ENTER = Continue  R = Repair F3 = Quit");

  if (IsUnattendedSetup)
    {
      //TODO
      //read options from inf
      ComputerList = CreateComputerTypeList(SetupInf);
      DisplayList = CreateDisplayDriverList(SetupInf);
      KeyboardList = CreateKeyboardDriverList(SetupInf);
      LayoutList = CreateKeyboardLayoutList(SetupInf);
      return INSTALL_INTRO_PAGE;
    }

  while (TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
      break;
	}
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R') /* R */
	{
	  return REPAIR_INTRO_PAGE;
      break;
	}
      else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'L') /* R */
	{
	  return LICENSE_PAGE;
      break;
	}
    }

  return INTRO_PAGE;
}

/*
 * License Page
 * RETURNS
 *	Back to main setup page.
 */
static PAGE_NUMBER
LicensePage(PINPUT_RECORD Ir)
{
  CONSOLE_SetHighlightedTextXY(6, 6, "Licensing:");

  CONSOLE_SetTextXY(8, 8, "The ReactOS System is licensed under the terms of the");
  CONSOLE_SetTextXY(8, 9, "GNU GPL with parts containing code from other compatible");
  CONSOLE_SetTextXY(8, 10, "licenses such as the X11 or BSD and GNU LGPL licenses.");
  CONSOLE_SetTextXY(8, 11, "All software that is part of the ReactOS system is");
  CONSOLE_SetTextXY(8, 12, "therefore released under the GNU GPL as well as maintaining");
  CONSOLE_SetTextXY(8, 13, "the original license.");

  CONSOLE_SetTextXY(8, 15, "This software comes with NO WARRANTY or restrictions on usage");
  CONSOLE_SetTextXY(8, 16, "save applicable local and international law. The licensing of");
  CONSOLE_SetTextXY(8, 17, "ReactOS only covers distribution to third parties.");

  CONSOLE_SetTextXY(8, 18, "If for some reason you did not receive a copy of the");
  CONSOLE_SetTextXY(8, 19, "GNU General Public License with ReactOS please visit");
  CONSOLE_SetHighlightedTextXY(8, 20, "http://www.gnu.org/licenses/licenses.html");

  CONSOLE_SetHighlightedTextXY(6, 22, "Warranty:");

  CONSOLE_SetTextXY(8, 24, "This is free software; see the source for copying conditions.");
  CONSOLE_SetTextXY(8, 25, "There is NO warranty; not even for MERCHANTABILITY or");
  CONSOLE_SetTextXY(8, 26, "FITNESS FOR A PARTICULAR PURPOSE");

  CONSOLE_SetStatusText("   ENTER = Return");

  while (TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
      {
          return INTRO_PAGE;
          break;
      }
    }

  return LICENSE_PAGE;
}

static PAGE_NUMBER
RepairIntroPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  CONSOLE_SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  CONSOLE_SetTextXY(6, 12, "The repair functions are not implemented yet.");

  CONSOLE_SetTextXY(8, 15, "\x07  Press U for Updating OS.");
  
  CONSOLE_SetTextXY(8, 17, "\x07  Press R for the Recovery Console.");

  CONSOLE_SetTextXY(8, 19, "\x07  Press ESC to return to the main page.");

  CONSOLE_SetTextXY(8, 21, "\x07  Press ENTER to reboot your computer.");

  CONSOLE_SetStatusText("   ESC = Main page  ENTER = Reboot");

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return REBOOT_PAGE;
	}
    else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'U') /* U */
	{
	  RepairUpdateFlag = TRUE;
	  return INSTALL_INTRO_PAGE;
	}
    else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R') /* R */
	{
	  return INTRO_PAGE;
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
  CONSOLE_SetUnderlinedTextXY(4, 3, " ReactOS " KERNEL_VERSION_STR " Setup ");

  CONSOLE_SetTextXY(6, 8, "ReactOS Setup is in an early development phase. It does not yet");
  CONSOLE_SetTextXY(6, 9, "support all the functions of a fully usable setup application.");

  CONSOLE_SetTextXY(6, 12, "The following limitations apply:");
  CONSOLE_SetTextXY(8, 13, "- Setup can not handle more than one primary partition per disk.");
  CONSOLE_SetTextXY(8, 14, "- Setup can not delete a primary partition from a disk");
  CONSOLE_SetTextXY(8, 15, "  as long as extended partitions exist on this disk.");
  CONSOLE_SetTextXY(8, 16, "- Setup can not delete the first extended partition from a disk");
  CONSOLE_SetTextXY(8, 17, "  as long as other extended partitions exist on this disk.");
  CONSOLE_SetTextXY(8, 18, "- Setup supports FAT file systems only.");
  CONSOLE_SetTextXY(8, 19, "- File system checks are not implemented yet.");


  CONSOLE_SetTextXY(8, 23, "\x07  Press ENTER to install ReactOS.");

  CONSOLE_SetTextXY(8, 25, "\x07  Press F3 to quit without installing ReactOS.");


  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");

  if (RepairUpdateFlag)
    {
      //return SELECT_PARTITION_PAGE; 
      return DEVICE_SETTINGS_PAGE;
    }
    
  if (IsUnattendedSetup)
    {
      return SELECT_PARTITION_PAGE;
    }

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
//	  return SCSI_CONTROLLER_PAGE;
	}
    }

  return INSTALL_INTRO_PAGE;
}


#if 0
static PAGE_NUMBER
ScsiControllerPage(PINPUT_RECORD Ir)
{
  SetTextXY(6, 8, "Setup detected the following mass storage devices:");

  /* FIXME: print loaded mass storage driver descriptions */
#if 0
  SetTextXY(8, 10, "TEST device");
#endif


  SetStatusText("   ENTER = Continue   F3 = Quit");

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

  return SCSI_CONTROLLER_PAGE;
}
#endif


static PAGE_NUMBER
DeviceSettingsPage(PINPUT_RECORD Ir)
{
  static ULONG Line = 16;

  /* Initialize the computer settings list */
  if (ComputerList == NULL)
    {
      ComputerList = CreateComputerTypeList(SetupInf);
      if (ComputerList == NULL)
	{
	  PopupError("Setup failed to load the computer type list.\n",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}
    }

  /* Initialize the display settings list */
  if (DisplayList == NULL)
    {
      DisplayList = CreateDisplayDriverList(SetupInf);
      if (DisplayList == NULL)
	{
	  PopupError("Setup failed to load the display settings list.\n",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}
    }

  /* Initialize the keyboard settings list */
  if (KeyboardList == NULL)
    {
      KeyboardList = CreateKeyboardDriverList(SetupInf);
      if (KeyboardList == NULL)
	{
	  PopupError("Setup failed to load the keyboard type list.\n",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
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
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}
    }

  CONSOLE_SetTextXY(6, 8, "The list below shows the current device settings.");

  CONSOLE_SetTextXY(8, 11, "       Computer:");
  CONSOLE_SetTextXY(8, 12, "        Display:");
  CONSOLE_SetTextXY(8, 13, "       Keyboard:");
  CONSOLE_SetTextXY(8, 14, "Keyboard layout:");

  CONSOLE_SetTextXY(8, 16, "         Accept:");

  CONSOLE_SetTextXY(25, 11, GetGenericListEntry(ComputerList)->Text);
  CONSOLE_SetTextXY(25, 12, GetGenericListEntry(DisplayList)->Text);
  CONSOLE_SetTextXY(25, 13, GetGenericListEntry(KeyboardList)->Text);
  CONSOLE_SetTextXY(25, 14, GetGenericListEntry(LayoutList)->Text);

  CONSOLE_SetTextXY(25, 16, "Accept these device settings");
  CONSOLE_InvertTextXY (24, Line, 48, 1);


  CONSOLE_SetTextXY(6, 19, "You can change the hardware settings by pressing the UP or DOWN keys");
  CONSOLE_SetTextXY(6, 20, "to select an entry. Then press the ENTER key to select alternative");
  CONSOLE_SetTextXY(6, 21, "settings.");

  CONSOLE_SetTextXY(6, 23, "When all settings are correct, select \"Accept these device settings\"");
  CONSOLE_SetTextXY(6, 24, "and press ENTER.");

  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");
  
  if (RepairUpdateFlag) 
    {
      return SELECT_PARTITION_PAGE;
    }
  
  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  CONSOLE_NormalTextXY (24, Line, 48, 1);
	  if (Line == 14)
	    Line = 16;
	  else if (Line == 16)
	    Line = 11;
	  else
	    Line++;
	  CONSOLE_InvertTextXY (24, Line, 48, 1);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  CONSOLE_NormalTextXY (24, Line, 48, 1);
	  if (Line == 11)
	    Line = 16;
	  else if (Line == 16)
	    Line = 14;
	  else
	    Line--;
	  CONSOLE_InvertTextXY (24, Line, 48, 1);
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
	  else if (Line == 16)
	    return SELECT_PARTITION_PAGE;
	}
    }

  return DEVICE_SETTINGS_PAGE;
}


static PAGE_NUMBER
ComputerSettingsPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(6, 8, "You want to change the type of computer to be installed.");

  CONSOLE_SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired computer type.");
  CONSOLE_SetTextXY(8, 11, "   Then press ENTER.");

  CONSOLE_SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  CONSOLE_SetTextXY(8, 14, "   the computer type.");

  DrawGenericList(ComputerList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  CONSOLE_SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(ComputerList);

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
  CONSOLE_SetTextXY(6, 8, "You want to change the type of display to be installed.");

  CONSOLE_SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired display type.");
  CONSOLE_SetTextXY(8, 11, "   Then press ENTER.");

  CONSOLE_SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  CONSOLE_SetTextXY(8, 14, "   the display type.");

  DrawGenericList(DisplayList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  CONSOLE_SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(DisplayList);

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
  CONSOLE_SetTextXY(6, 8, "You want to change the type of keyboard to be installed.");

  CONSOLE_SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired keyboard type.");
  CONSOLE_SetTextXY(8, 11, "   Then press ENTER.");

  CONSOLE_SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  CONSOLE_SetTextXY(8, 14, "   the keyboard type.");

  DrawGenericList(KeyboardList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  CONSOLE_SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(KeyboardList);

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
  CONSOLE_SetTextXY(6, 8, "You want to change the keyboard layout to be installed.");

  CONSOLE_SetTextXY(8, 10, "\x07  Press the UP or DOWN key to select the desired keyboard");
  CONSOLE_SetTextXY(8, 11, "    layout. Then press ENTER.");

  CONSOLE_SetTextXY(8, 13, "\x07  Press the ESC key to return to the previous page without changing");
  CONSOLE_SetTextXY(8, 14, "   the keyboard layout.");

  DrawGenericList(LayoutList,
		  2,
		  18,
		  xScreen - 3,
		  yScreen - 3);

  CONSOLE_SetStatusText("   ENTER = Continue   ESC = Cancel   F3 = Quit");

  SaveGenericListState(LayoutList);

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
SelectPartitionPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(6, 8, "The list below shows existing partitions and unused disk");
  CONSOLE_SetTextXY(6, 9, "space for new partitions.");

  CONSOLE_SetTextXY(8, 11, "\x07  Press UP or DOWN to select a list entry.");
  CONSOLE_SetTextXY(8, 13, "\x07  Press ENTER to install ReactOS onto the selected partition.");
  CONSOLE_SetTextXY(8, 15, "\x07  Press C to create a new partition.");
  CONSOLE_SetTextXY(8, 17, "\x07  Press D to delete an existing partition.");

  CONSOLE_SetStatusText("   Please wait...");

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
		  "F3= Quit  ENTER = Continue",
		  NULL, POPUP_WAIT_NONE);
      while (TRUE)
	{
	  CONSOLE_ConInKey (Ir);

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
      if (!SelectPartition(PartitionList, UnattendDestinationDiskNumber, UnattendDestinationPartitionNumber))
        {
          if (AutoPartition)
            {
              PPARTENTRY PartEntry = PartEntry = PartitionList->CurrentPartition;
              ULONG MaxSize = (PartEntry->UnpartitionedLength + (1 << 19)) >> 20;  /* in MBytes (rounded) */
              CreateNewPartition (PartitionList,
                			 	  MaxSize,
                				  TRUE);
              return (SELECT_FILE_SYSTEM_PAGE);
            }
        }
      else
        {
          return(SELECT_FILE_SYSTEM_PAGE);
        }
    }

  while(TRUE)
    {
      /* Update status text */
      if (PartitionList->CurrentPartition == NULL ||
	  PartitionList->CurrentPartition->Unpartitioned == TRUE)
	{
	  CONSOLE_SetStatusText ("   ENTER = Install   C = Create Partition   F3 = Quit");
	}
      else
	{
	  CONSOLE_SetStatusText ("   ENTER = Install   D = Delete Partition   F3 = Quit");
	}

      CONSOLE_ConInKey(Ir);

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
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'C') /* C */
	{
	  if (PartitionList->CurrentPartition->Unpartitioned == FALSE)
	    {
	      PopupError ("You can not create a new Partition inside\n"
			  "of an already existing Partition!\n"
			  "\n"
			  "  * Press any key to continue.",
			  NULL,
			  Ir, POPUP_WAIT_ANY_KEY);

	      return SELECT_PARTITION_PAGE;
	    }

	  return CREATE_PARTITION_PAGE;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'D') /* D */
	{
	  if (PartitionList->CurrentPartition->Unpartitioned == TRUE)
	    {
	      PopupError ("You can not delete unpartitioned disk space!\n"
			  "\n"
			  "  * Press any key to continue.",
			  NULL,
			  Ir, POPUP_WAIT_ANY_KEY);

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
  DWORD Written;

  coPos.X = Left;
  coPos.Y = Top;
  memset(buf, '_', sizeof(buf));
  buf[FieldLength - strlen(FieldContent)] = 0;
  strcat(buf, FieldContent);

  WriteConsoleOutputCharacterA (StdOutput,
			        buf,
			        strlen (buf),
			        coPos,
			        &Written);
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
  DWORD Written;
  CHAR Buffer[100];
  ULONG Index;
  CHAR ch;
  SHORT iLeft;
  SHORT iTop;

  if (Quit != NULL)
    *Quit = FALSE;

  if (Cancel != NULL)
    *Cancel = FALSE;

  DrawBox(Left, Top, Right - Left + 1, Bottom - Top + 1);

  /* Print message */
  coPos.X = Left + 2;
  coPos.Y = Top + 2;
  strcpy (Buffer, "Size of new partition:");
  iLeft = coPos.X + strlen (Buffer) + 1;
  iTop = coPos.Y;
  WriteConsoleOutputCharacterA (StdOutput,
				 Buffer,
				 strlen (Buffer),
				 coPos,
				 &Written);

  sprintf (Buffer, "MB (max. %lu MB)", MaxSize);
  coPos.X = iLeft + PARTITION_SIZE_INPUT_FIELD_LENGTH + 1;
  coPos.Y = iTop;
  WriteConsoleOutputCharacterA (StdOutput,
				Buffer,
				strlen (Buffer),
				coPos,
				&Written);

  sprintf(Buffer, "%lu", MaxSize);
  Index = strlen(Buffer);
  DrawInputField (PARTITION_SIZE_INPUT_FIELD_LENGTH,
		  iLeft,
		  iTop,
		  Buffer);

  while (TRUE)
    {
      CONSOLE_ConInKey (&Ir);

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

  CONSOLE_SetStatusText ("   Please wait...");

  CONSOLE_SetTextXY (6, 8, "You have chosen to create a new partition on");

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
      CONSOLE_PrintTextXY (6, 10,
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
      CONSOLE_PrintTextXY (6, 10,
		   "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu).",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id);
    }


  CONSOLE_SetTextXY (6, 12, "Please enter the size of the new partition in megabytes.");

#if 0
  CONSOLE_PrintTextXY (8, 10, "Maximum size of the new partition is %I64u MB",
	       PartitionList->CurrentPartition->UnpartitionedLength / (1024*1024));
#endif

  CONSOLE_SetStatusText ("   ENTER = Create Partition   ESC = Cancel   F3 = Quit");

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

  CONSOLE_SetTextXY (6, 8, "You have chosen to delete the partition");

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
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0x280000000LL) /* 10 GB */
    {
      PartSize = (PartEntry->PartInfo[0].PartitionLength.QuadPart + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0xA00000LL) /* 10 MB */
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
      CONSOLE_PrintTextXY (6, 10,
		   "   %c%c  Type %lu    %I64u %s",
		   (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		   (PartEntry->DriveLetter == 0) ? '-' : ':',
		   PartEntry->PartInfo[0].PartitionType,
		   PartSize,
		   Unit);
    }
  else
    {
      CONSOLE_PrintTextXY (6, 10,
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
      CONSOLE_PrintTextXY (6, 12,
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
      CONSOLE_PrintTextXY (6, 12,
		   "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu).",
		   DiskSize,
		   Unit,
		   DiskEntry->DiskNumber,
		   DiskEntry->Port,
		   DiskEntry->Bus,
		   DiskEntry->Id);
    }

  CONSOLE_SetTextXY (8, 18, "\x07  Press D to delete the partition.");
  CONSOLE_SetTextXY (11, 19, "WARNING: All data on this partition will be lost!");

  CONSOLE_SetTextXY (8, 21, "\x07  Press ESC to cancel.");

  CONSOLE_SetStatusText ("   D = Delete Partition   ESC = Cancel   F3 = Quit");

  while (TRUE)
    {
      CONSOLE_ConInKey (Ir);

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
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'D') /* D */
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
  if (PartEntry->PartInfo[0].PartitionLength.QuadPart >= 0x280000000LL) /* 10 GB */
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
      CONSOLE_SetTextXY(6, 8, "Setup created a new partition on");

#if 0
  CONSOLE_PrintTextXY(8, 10, "Partition %lu (%I64u %s) %s of",
	      PartEntry->PartInfo[0].PartitionNumber,
	      PartSize,
	      PartUnit,
	      PartType);
#endif

  CONSOLE_PrintTextXY(8, 10, "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ).",
	      DiskEntry->DiskNumber,
	      DiskSize,
	      DiskUnit,
	      DiskEntry->Port,
	      DiskEntry->Bus,
	      DiskEntry->Id,
	      &DiskEntry->DriverName);

      CONSOLE_SetTextXY(6, 12, "This Partition will be formatted next.");


      PartEntry->AutoCreate = FALSE;
    }
  else if (PartEntry->New == TRUE)
    {
      CONSOLE_SetTextXY(6, 8, "You chose to install ReactOS on a new or unformatted Partition.");
      CONSOLE_SetTextXY(6, 10, "This Partition will be formatted next.");
    }
  else
    {
      CONSOLE_SetTextXY(6, 8, "Setup install ReactOS onto Partition");

      if (PartType == NULL)
	{
	  CONSOLE_PrintTextXY (8, 10,
		       "%c%c  Type %lu    %I64u %s",
		       (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		       (PartEntry->DriveLetter == 0) ? '-' : ':',
		       PartEntry->PartInfo[0].PartitionType,
		       PartSize,
		       PartUnit);
	}
      else
	{
	  CONSOLE_PrintTextXY (8, 10,
		       "%c%c  %s    %I64u %s",
		       (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
		       (PartEntry->DriveLetter == 0) ? '-' : ':',
		       PartType,
		       PartSize,
		       PartUnit);
	}

      CONSOLE_PrintTextXY(6, 12, "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ).",
		  DiskEntry->DiskNumber,
		  DiskSize,
		  DiskUnit,
		  DiskEntry->Port,
		  DiskEntry->Bus,
		  DiskEntry->Id,
		  &DiskEntry->DriverName);
    }


  CONSOLE_SetTextXY(6, 17, "Select a file system from the list below.");

  CONSOLE_SetTextXY(8, 19, "\x07  Press UP or DOWN to select a file system.");
  CONSOLE_SetTextXY(8, 21, "\x07  Press ENTER to format the partition.");
  CONSOLE_SetTextXY(8, 23, "\x07  Press ESC to select another partition.");

  if (FileSystemList == NULL)
    {
      FileSystemList = CreateFileSystemList (6, 26, PartEntry->New, L"FAT");
      if (FileSystemList == NULL)
	{
	  /* FIXME: show an error dialog */
	  return QUIT_PAGE;
	}

      /* FIXME: Add file systems to list */
    }
  DrawFileSystemList (FileSystemList);

  CONSOLE_SetStatusText ("   ENTER = Continue   ESC = Cancel   F3 = Quit");
  if (RepairUpdateFlag) 
    {
       return (CHECK_FILE_SYSTEM_PAGE);
    //return SELECT_PARTITION_PAGE;
    }
    
  if (IsUnattendedSetup)
    {
      if (UnattendFormatPartition)
        {
          return FORMAT_PARTITION_PAGE;
        }
      return(CHECK_FILE_SYSTEM_PAGE);
    }

  while (TRUE)
    {
      CONSOLE_ConInKey (Ir);

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
	  if (!FileSystemList->Selected->FormatFunc)
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
  PLIST_ENTRY Entry;
#endif


  CONSOLE_SetTextXY(6, 8, "Format partition");

  CONSOLE_SetTextXY(6, 10, "Setup will now format the partition. Press ENTER to continue.");

  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");


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
      if (!IsUnattendedSetup)
        {
          CONSOLE_ConInKey(Ir);
        }

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit (Ir) == TRUE)
	    {
	      return QUIT_PAGE;
	    }
	  break;
	}
      else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN || IsUnattendedSetup) /* ENTER */
	{
	  CONSOLE_SetStatusText ("   Please wait ...");

        if (PartEntry->PartInfo[0].PartitionType == PARTITION_ENTRY_UNUSED)
        {
            if (wcscmp(FileSystemList->Selected->FileSystem, L"FAT") == 0)
            {
                if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (4200LL * 1024LL))
                {
                    /* FAT12 CHS partition (disk is smaller than 4.1MB) */
                    PartEntry->PartInfo[0].PartitionType = PARTITION_FAT_12;
                }
                else if (PartEntry->PartInfo[0].StartingOffset.QuadPart < (1024LL * 255LL * 63LL * 512LL))
                {
                    /* Partition starts below the 8.4GB boundary ==> CHS partition */

                    if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (32LL * 1024LL * 1024LL))
                    {
                        /* FAT16 CHS partition (partiton size < 32MB) */
                        PartEntry->PartInfo[0].PartitionType = PARTITION_FAT_16;
                    }
                    else if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (512LL * 1024LL * 1024LL))
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

                    if (PartEntry->PartInfo[0].PartitionLength.QuadPart < (512LL * 1024LL * 1024LL))
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
            }
            else if (!FileSystemList->Selected->FormatFunc)
                return QUIT_PAGE;
        }

	  CheckActiveBootPartition (PartitionList);

#ifndef NDEBUG
	  CONSOLE_PrintTextXY (6, 12,
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
		      CONSOLE_PrintTextXY (6, Line,
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
			  "ENTER = Reboot computer",
			  Ir, POPUP_WAIT_ENTER);
	      return QUIT_PAGE;
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


            if (FileSystemList->Selected->FormatFunc)
            {
                Status = FormatPartition(&DestinationRootPath, FileSystemList->Selected);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("FormatPartition() failed with status 0x%08lx\n", Status);
                    /* FIXME: show an error dialog */
                    return QUIT_PAGE;
                }

                PartEntry->New = FALSE;

                CheckActiveBootPartition(PartitionList);
            }

            if (wcscmp(FileSystemList->Selected->FileSystem, L"FAT") == 0)
            {
                /* FIXME: Install boot code. This is a hack! */
                if ((PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32_XINT13)
                 || (PartEntry->PartInfo[0].PartitionType == PARTITION_FAT32))
                {
                    wcscpy(PathBuffer, SourceRootPath.Buffer);
                    wcscat(PathBuffer, L"\\loader\\fat32.bin");

                    DPRINT("Install FAT32 bootcode: %S ==> %S\n", PathBuffer,
                        DestinationRootPath.Buffer);
                    Status = InstallFat32BootCodeToDisk(PathBuffer,
                        DestinationRootPath.Buffer);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("InstallFat32BootCodeToDisk() failed with status 0x%08lx\n", Status);
                        /* FIXME: show an error dialog */
                        DestroyFileSystemList(FileSystemList);
                        FileSystemList = NULL;
                        return QUIT_PAGE;
                    }
                }
                else
                {
                    wcscpy(PathBuffer, SourceRootPath.Buffer);
                    wcscat(PathBuffer, L"\\loader\\fat.bin");

                    DPRINT("Install FAT bootcode: %S ==> %S\n", PathBuffer,
                        DestinationRootPath.Buffer);
                    Status = InstallFat16BootCodeToDisk(PathBuffer,
                        DestinationRootPath.Buffer);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("InstallFat16BootCodeToDisk() failed with status 0x%.08x\n", Status);
                        /* FIXME: show an error dialog */
                        DestroyFileSystemList(FileSystemList);
                        FileSystemList = NULL;
                        return QUIT_PAGE;
                    }
                }
            }
            else if (FileSystemList->Selected->FormatFunc)
            {
                DestroyFileSystemList(FileSystemList);
                FileSystemList = NULL;
                return QUIT_PAGE;
            }

#ifndef NDEBUG
            CONSOLE_SetStatusText ("   Done.  Press any key ...");
            CONSOLE_ConInKey(Ir);
#endif

            DestroyFileSystemList(FileSystemList);
            FileSystemList = NULL;
            return INSTALL_DIRECTORY_PAGE;
        }
    }

    return FORMAT_PARTITION_PAGE;
}


static ULONG
CheckFileSystemPage(PINPUT_RECORD Ir)
{
  PFILE_SYSTEM_ITEM CurrentFileSystem;
  WCHAR PathBuffer[MAX_PATH];
  CHAR Buffer[MAX_PATH];
  NTSTATUS Status;

  /* FIXME: code duplicated in FormatPartitionPage */
  /* Set DestinationRootPath */
  RtlFreeUnicodeString(&DestinationRootPath);
  swprintf(PathBuffer,
    L"\\Device\\Harddisk%lu\\Partition%lu",
    PartitionList->CurrentDisk->DiskNumber,
    PartitionList->CurrentPartition->PartInfo[0].PartitionNumber);
  RtlCreateUnicodeString(&DestinationRootPath, PathBuffer);
  DPRINT("DestinationRootPath: %wZ\n", &DestinationRootPath);

  /* Set SystemRootPath */
  RtlFreeUnicodeString(&SystemRootPath);
  swprintf(PathBuffer,
    L"\\Device\\Harddisk%lu\\Partition%lu",
    PartitionList->ActiveBootDisk->DiskNumber,
    PartitionList->ActiveBootPartition->PartInfo[0].PartitionNumber);
  RtlCreateUnicodeString(&SystemRootPath, PathBuffer);
  DPRINT("SystemRootPath: %wZ\n", &SystemRootPath);

  CONSOLE_SetTextXY(6, 8, "Setup is now checking the selected partition.");

  CONSOLE_SetStatusText("   Please wait...");

  /* WRONG: first filesystem is not necesseraly the one of the current partition! */
  CurrentFileSystem = CONTAINING_RECORD(FileSystemList->ListHead.Flink, FILE_SYSTEM_ITEM, ListEntry);

  if (!CurrentFileSystem->ChkdskFunc)
  {
    sprintf(Buffer,
      "Setup is currently unable to check a partition formatted in %S.\n"
      "\n"
      "  \x07  Press ENTER to continue Setup.\n"
      "  \x07  Press F3 to quit Setup.",
      CurrentFileSystem->FileSystem);
    PopupError(
      Buffer,
      "F3= Quit  ENTER = Continue",
      NULL, POPUP_WAIT_NONE);

    while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x00
       && Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3) /* F3 */
      {
        if (ConfirmQuit(Ir))
          return QUIT_PAGE;
        else
          return CHECK_FILE_SYSTEM_PAGE;
      }
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == VK_RETURN) /* ENTER */
      {
        return INSTALL_DIRECTORY_PAGE;
      }
    }
  }
  else
  {
    Status = ChkdskPartition(&DestinationRootPath, CurrentFileSystem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ChkdskPartition() failed with status 0x%08lx\n", Status);
        sprintf(Buffer, "Setup failed to verify the selected partition.\n"
          "(Status 0x%08lx).\n", Status);
        PopupError(Buffer,
          "ENTER = Reboot computer",
          Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    return INSTALL_DIRECTORY_PAGE;
  }
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
	   DiskEntry->BiosDiskNumber,
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
  if (!SetupFindFirstLineW (SetupInf, L"SetupData", L"DefaultPath", &Context))
    {
      PopupError("Setup failed to find the 'SetupData' section\n"
		 "in TXTSETUP.SIF.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Read the 'DefaultPath' data */
  if (INF_GetData (&Context, NULL, &DefaultPath))
    {
      wcscpy(InstallDir, DefaultPath);
    }
  else
    {
      wcscpy(InstallDir, L"\\ReactOS");
    }
  Length = wcslen(InstallDir);

  CONSOLE_SetTextXY(6, 8, "Setup installs ReactOS files onto the selected partition. Choose a");
  CONSOLE_SetTextXY(6, 9, "directory where you want ReactOS to be installed:");

  CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);

  CONSOLE_SetTextXY(6, 14, "To change the suggested directory, press BACKSPACE to delete");
  CONSOLE_SetTextXY(6, 15, "characters and then type the directory where you want ReactOS to");
  CONSOLE_SetTextXY(6, 16, "be installed.");

  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");

  if (IsUnattendedSetup)
    {
      return(InstallDirectoryPage1 (InstallDir, DiskEntry, PartEntry));
    }

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

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
	      CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);
	    }
	}
      else if (isprint(Ir->Event.KeyEvent.uChar.AsciiChar))
	{
	  if (Length < 50)
	    {
	      InstallDir[Length] = (WCHAR)Ir->Event.KeyEvent.uChar.AsciiChar;
	      Length++;
	      InstallDir[Length] = 0;
	      CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);
	    }
	}
    }

  return(INSTALL_DIRECTORY_PAGE);
}

static BOOLEAN
AddSectionToCopyQueue(HINF InfFile,
		       PWCHAR SectionName,
		       PWCHAR SourceCabinet,
			   PCUNICODE_STRING DestinationPath,
		       PINPUT_RECORD Ir)
{
  INFCONTEXT FilesContext;
  INFCONTEXT DirContext;
  PWCHAR FileKeyName;
  PWCHAR FileKeyValue;
  PWCHAR DirKeyValue;
  PWCHAR TargetFileName;

  /* Search for the SectionName section */
  if (!SetupFindFirstLineW (InfFile, SectionName, NULL, &FilesContext))
    {
      char Buffer[128];
      sprintf(Buffer, "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n", SectionName);
      PopupError(Buffer, "ENTER = Reboot computer", Ir, POPUP_WAIT_ENTER);
      return(FALSE);
    }

  /*
   * Enumerate the files in the section
   * and add them to the file queue.
   */
  do
    {
      /* Get source file name and target directory id */
      if (!INF_GetData (&FilesContext, &FileKeyName, &FileKeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("INF_GetData() failed\n");
	  break;
	}

      /* Get optional target file name */
      if (!INF_GetDataField (&FilesContext, 2, &TargetFileName))
	TargetFileName = NULL;

      DPRINT ("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

      /* Lookup target directory */
      if (!SetupFindFirstLineW (InfFile, L"Directories", FileKeyValue, &DirContext))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("SetupFindFirstLine() failed\n");
	  break;
	}

      if (!INF_GetData (&DirContext, NULL, &DirKeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("INF_GetData() failed\n");
	  break;
	}

      if (!SetupQueueCopy(SetupFileQueue,
			  SourceCabinet,
			  SourceRootPath.Buffer,
			  SourceRootDir.Buffer,
			  FileKeyName,
			  DirKeyValue,
			  TargetFileName))
	{
	  /* FIXME: Handle error! */
	  DPRINT1("SetupQueueCopy() failed\n");
	}
    }
  while (SetupFindNextLine(&FilesContext, &FilesContext));

  return TRUE;
}

static BOOLEAN
PrepareCopyPageInfFile(HINF InfFile,
		       PWCHAR SourceCabinet,
		       PINPUT_RECORD Ir)
{
  WCHAR PathBuffer[MAX_PATH];
  INFCONTEXT DirContext;
  PWCHAR AdditionalSectionName = NULL;
  PWCHAR KeyValue;
  ULONG Length;
  NTSTATUS Status;

  /* Add common files */
  if (!AddSectionToCopyQueue(InfFile, L"SourceFiles", SourceCabinet, &DestinationPath, Ir))
    return FALSE;

  /* Add specific files depending of computer type */
  if (SourceCabinet == NULL)
  {
    if (!ProcessComputerFiles(InfFile, ComputerList, &AdditionalSectionName))
      return FALSE;
    if (AdditionalSectionName)
    {
      if (!AddSectionToCopyQueue(InfFile, AdditionalSectionName, SourceCabinet, &DestinationPath, Ir))
        return FALSE;
    }
  }

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
  Status = SetupCreateDirectory(PathBuffer);
  if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
    {
      DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
      PopupError("Setup could not create the install directory.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return(FALSE);
    }


  /* Search for the 'Directories' section */
  if (!SetupFindFirstLineW(InfFile, L"Directories", NULL, &DirContext))
    {
      if (SourceCabinet)
	{
	  PopupError("Setup failed to find the 'Directories' section\n"
		     "in the cabinet.\n", "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	}
      else
	{
	  PopupError("Setup failed to find the 'Directories' section\n"
		     "in TXTSETUP.SIF.\n", "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
        }
      return(FALSE);
    }

  /* Enumerate the directory values and create the subdirectories */
  do
    {
      if (!INF_GetData (&DirContext, NULL, &KeyValue))
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

	  Status = SetupCreateDirectory(PathBuffer);
	  if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
	    {
	      DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
	      PopupError("Setup could not create install directories.",
			 "ENTER = Reboot computer",
			 Ir, POPUP_WAIT_ENTER);
	      return(FALSE);
	    }
	}
    }
  while (SetupFindNextLine (&DirContext, &DirContext));

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
  UINT ErrorLine;
  PVOID InfFileData;

  CONSOLE_SetTextXY(6, 8, "Setup prepares your computer for copying the ReactOS files. ");

  CONSOLE_SetStatusText("   Building the file copy list...");

  /* Create the file queue */
  SetupFileQueue = SetupOpenFileQueue();
  if (SetupFileQueue == NULL)
    {
      PopupError("Setup failed to open the copy file queue.\n",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return(QUIT_PAGE);
    }

  if (!PrepareCopyPageInfFile(SetupInf, NULL, Ir))
    {
      return QUIT_PAGE;
    }

  /* Search for the 'Cabinets' section */
  if (!SetupFindFirstLineW (SetupInf, L"Cabinets", NULL, &CabinetsContext))
    {
      return FILE_COPY_PAGE;
    }

  /*
   * Enumerate the directory values in the 'Cabinets'
   * section and parse their inf files.
   */
  do
    {
      if (!INF_GetData (&CabinetsContext, NULL, &KeyValue))
	break;

      wcscpy(PathBuffer, SourcePath.Buffer);
      wcscat(PathBuffer, L"\\");
      wcscat(PathBuffer, KeyValue);

#ifdef __REACTOS__
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
			 "ENTER = Reboot computer",
			 Ir, POPUP_WAIT_ENTER);
	      return QUIT_PAGE;
	    }
	}
      else
	{
	  DPRINT("Cannot open cabinet: %S.\n", CabinetGetCabinetName());

	  PopupError("Cabinet not found.\n",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}

      InfHandle = INF_OpenBufferedFileA((CHAR*) InfFileData,
				   InfFileSize,
				   (const CHAR*) NULL,
				   INF_STYLE_WIN4,
				   &ErrorLine);
      if (InfHandle == INVALID_HANDLE_VALUE)
	{
	  PopupError("Cabinet has no valid inf file.\n",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}

      CabinetCleanup();

      if (!PrepareCopyPageInfFile(InfHandle, KeyValue, Ir))
        {
          return QUIT_PAGE;
        }
#endif
    }
  while (SetupFindNextLine (&CabinetsContext, &CabinetsContext));

  return FILE_COPY_PAGE;
}

VOID
NTAPI
SetupUpdateMemoryInfo(IN PCOPYCONTEXT CopyContext,
                      IN BOOLEAN First)
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

    /* Get the memory information from the system */
    NtQuerySystemInformation(SystemPerformanceInformation,
                             &PerfInfo,
                             sizeof(PerfInfo),
                             NULL);

    /* Check if this is initial setup */
    if (First)
    {
        /* Set maximum limits to be total RAM pages */
        ProgressSetStepCount(CopyContext->MemoryBars[0], PerfInfo.CommitLimit);
        ProgressSetStepCount(CopyContext->MemoryBars[1], PerfInfo.CommitLimit);
        ProgressSetStepCount(CopyContext->MemoryBars[2], PerfInfo.CommitLimit);
    }

    /* Set current values */
    ProgressSetStep(CopyContext->MemoryBars[0], PerfInfo.PagedPoolPages);
    ProgressSetStep(CopyContext->MemoryBars[1], PerfInfo.NonPagedPoolPages);
    ProgressSetStep(CopyContext->MemoryBars[2], PerfInfo.AvailablePages);

    /* Check if memory dropped below 40%! */
    if (CopyContext->MemoryBars[2]->Percent <= 40)
    {
        /* Wait a while until Mm does its thing */
        LARGE_INTEGER Interval;
        Interval.QuadPart = -1 * 15 * 1000 * 100;
        NtDelayExecution(FALSE, &Interval);
    }
}

static UINT CALLBACK
FileCopyCallback(PVOID Context,
		 UINT Notification,
		 UINT_PTR Param1,
		 UINT_PTR Param2)
{
  PCOPYCONTEXT CopyContext;

  CopyContext = (PCOPYCONTEXT)Context;

  switch (Notification)
    {
      case SPFILENOTIFY_STARTSUBQUEUE:
	CopyContext->TotalOperations = (ULONG)Param2;
	ProgressSetStepCount(CopyContext->ProgressBar,
			     CopyContext->TotalOperations);
    SetupUpdateMemoryInfo(CopyContext, TRUE);
	break;

      case SPFILENOTIFY_STARTCOPY:
	/* Display copy message */
    CONSOLE_SetStatusText("                                                   \xB3 Copying file: %S", (PWSTR)Param1);
    SetupUpdateMemoryInfo(CopyContext, FALSE);
	break;

      case SPFILENOTIFY_ENDCOPY:
	CopyContext->CompletedOperations++;
	ProgressNextStep(CopyContext->ProgressBar);
    SetupUpdateMemoryInfo(CopyContext, FALSE);
	break;
    }

  return 0;
}

static
PAGE_NUMBER
FileCopyPage(PINPUT_RECORD Ir)
{
    COPYCONTEXT CopyContext;

    /* Display status text */
    CONSOLE_SetStatusText("                                                           \xB3 Please wait...    ");

    /* Displey information text */
    CONSOLE_SetTextXY(11, 12, "Please wait while ReactOS Setup copies files to your ReactOS");
    CONSOLE_SetTextXY(30, 13, "installation folder.");
    CONSOLE_SetTextXY(20, 14, "This may take several minutes to complete.");

    /* Create context for the copy process */
    CopyContext.DestinationRootPath = DestinationRootPath.Buffer;
    CopyContext.InstallPath = InstallPath.Buffer;
    CopyContext.TotalOperations = 0;
    CopyContext.CompletedOperations = 0;

    /* Create the progress bar as well */
    CopyContext.ProgressBar = CreateProgressBar(13,
                                                26,
                                                xScreen - 13,
                                                yScreen - 20,
                                                10,
                                                24,
                                                TRUE,
                                                "Setup is copying files...");

    /* Create the paged pool progress bar */
    CopyContext.MemoryBars[0] = CreateProgressBar(13,
                                                  40,
                                                  18,
                                                  43,
                                                  10,
                                                  44,
                                                  FALSE,
                                                  "Paged Memory");

    /* Create the non paged pool progress bar */
    CopyContext.MemoryBars[1] = CreateProgressBar(28,
                                                  40,
                                                  33,
                                                  43,
                                                  24,
                                                  44,
                                                  FALSE,
                                                  "Nonpaged Memory");

    /* Create the global memory progress bar */
    CopyContext.MemoryBars[2] = CreateProgressBar(43,
                                                  40,
                                                  48,
                                                  43,
                                                  40,
                                                  44,
                                                  FALSE,
                                                  "Free Memory");

    /* Do the file copying */
    SetupCommitFileQueueW(NULL,
                          SetupFileQueue,
                          FileCopyCallback,
                          &CopyContext);

    /* If we get here, we're done, so cleanup the queue and progress bar */
    SetupCloseFileQueue(SetupFileQueue);
    DestroyProgressBar(CopyContext.ProgressBar);
    DestroyProgressBar(CopyContext.MemoryBars[0]);
    DestroyProgressBar(CopyContext.MemoryBars[1]);
    DestroyProgressBar(CopyContext.MemoryBars[2]);

    /* Go display the next page */
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

  CONSOLE_SetTextXY(6, 8, "Setup is updating the system configuration");

  CONSOLE_SetStatusText("   Creating registry hives...");
  
  if (RepairUpdateFlag) 
    {
    	return SUCCESS_PAGE;
    }
    
  if (!SetInstallPathValue(&DestinationPath))
    {
      DPRINT("SetInstallPathValue() failed\n");
      PopupError("Setup failed to set the initialize the registry.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Create the default hives */
#ifdef __REACTOS__
  Status = NtInitializeRegistry(TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtInitializeRegistry() failed (Status %lx)\n", Status);
      PopupError("Setup failed to create the registry hives.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }
#else
  RegInitializeRegistry();
#endif

  /* Update registry */
  CONSOLE_SetStatusText("   Updating registry hives...");

  if (!SetupFindFirstLineW(SetupInf, L"HiveInfs.Install", NULL, &InfContext))
    {
      DPRINT1("SetupFindFirstLine() failed\n");
      PopupError("Setup failed to find the registry data files.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  do
    {
      INF_GetDataField (&InfContext, 0, &Action);
      INF_GetDataField (&InfContext, 1, &File);
      INF_GetDataField (&InfContext, 2, &Section);

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

      CONSOLE_SetStatusText("   Importing %S...", File);

      if (!ImportRegistryFile(File, Section, Delete))
	{
	  DPRINT("Importing %S failed\n", File);

	  PopupError("Setup failed to import a hive file.",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}
    }
  while (SetupFindNextLine (&InfContext, &InfContext));

  /* Update display registry settings */
  CONSOLE_SetStatusText("   Updating display registry settings...");
  if (!ProcessDisplayRegistry(SetupInf, DisplayList))
    {
      PopupError("Setup failed to update display registry settings.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Update keyboard layout settings */
  CONSOLE_SetStatusText("   Updating keyboard layout settings...");
  if (!ProcessKeyboardLayoutRegistry(LayoutList))
    {
      PopupError("Setup failed to update keyboard layout settings.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  /* Update the mounted devices list */
  SetMountedDeviceValues(PartitionList);

  CONSOLE_SetStatusText("   Done...");

  return BOOT_LOADER_PAGE;
}


static PAGE_NUMBER
BootLoaderPage(PINPUT_RECORD Ir)
{
  UCHAR PartitionType;
  BOOLEAN InstallOnFloppy;
  USHORT Line = 12;

  CONSOLE_SetStatusText("   Please wait...");

  PartitionType = PartitionList->ActiveBootPartition->PartInfo[0].PartitionType;

  if (PartitionType == PARTITION_ENTRY_UNUSED)
    {
      DPRINT("Error: active partition invalid (unused)\n");
      InstallOnFloppy = TRUE;
    }
  else if (PartitionType == 0x0A)
    {
      /* OS/2 boot manager partition */
      DPRINT("Found OS/2 boot manager partition\n");
      InstallOnFloppy = TRUE;
    }
  else if (PartitionType == 0x83)
    {
      /* Linux ext2 partition */
      DPRINT("Found Linux ext2 partition\n");
      InstallOnFloppy = TRUE;
    }
  else if (PartitionType == PARTITION_IFS)
    {
      /* NTFS partition */
      DPRINT("Found NTFS partition\n");
      InstallOnFloppy = TRUE;
    }
  else if ((PartitionType == PARTITION_FAT_12) ||
	   (PartitionType == PARTITION_FAT_16) ||
	   (PartitionType == PARTITION_HUGE) ||
	   (PartitionType == PARTITION_XINT13) ||
	   (PartitionType == PARTITION_FAT32) ||
	   (PartitionType == PARTITION_FAT32_XINT13))
    {
      DPRINT("Found FAT partition\n");
      InstallOnFloppy = FALSE;
    }
  else
    {
      /* Unknown partition */
      DPRINT("Unknown partition found\n");
      InstallOnFloppy = TRUE;
    }

  if (InstallOnFloppy == TRUE)
    {
      return BOOT_LOADER_FLOPPY_PAGE;
    }

  if (IsUnattendedSetup)
    {
      if (UnattendMBRInstallType == 0) /* skip MBR installation */
        {
          return SUCCESS_PAGE;
        }
      else if (UnattendMBRInstallType == 1) /* install on floppy */
        {
          return BOOT_LOADER_FLOPPY_PAGE;
        }
      else if (UnattendMBRInstallType == 2) /* install on hdd */
        {
          return BOOT_LOADER_HARDDISK_PAGE;
        }
    }

  CONSOLE_SetTextXY(6, 8, "Setup is installing the boot loader");

  CONSOLE_SetTextXY(8, 12, "Install bootloader on the harddisk (MBR).");
  CONSOLE_SetTextXY(8, 13, "Install bootloader on a floppy disk.");
  CONSOLE_SetTextXY(8, 14, "Skip install bootloader.");
  CONSOLE_InvertTextXY (8, Line, 48, 1);

  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN)) /* DOWN */
	{
	  CONSOLE_NormalTextXY (8, Line, 48, 1);

	  Line++;
      if (Line<12) Line=14;
      if (Line>14) Line=12;



	  CONSOLE_InvertTextXY (8, Line, 48, 1);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP)) /* UP */
	{
	  CONSOLE_NormalTextXY (8, Line, 48, 1);

	  Line--;
      if (Line<12) Line=14;
      if (Line>14) Line=12;


	  CONSOLE_InvertTextXY (8, Line, 48, 1);
	}
      else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	       (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	{
	  if (Line == 12)
	    {
	      return BOOT_LOADER_HARDDISK_PAGE;
	    }
	  else if (Line == 13)
	    {
	      return BOOT_LOADER_FLOPPY_PAGE;
	    }
      else if (Line == 14)
	    {
	       return SUCCESS_PAGE;;
	    }

	  return BOOT_LOADER_PAGE;
	}

    }

  return BOOT_LOADER_PAGE;
}


static PAGE_NUMBER
BootLoaderFloppyPage(PINPUT_RECORD Ir)
{
  NTSTATUS Status;

  CONSOLE_SetTextXY(6, 8, "Setup cannot install the bootloader on your computers");
  CONSOLE_SetTextXY(6, 9, "harddisk");

  CONSOLE_SetTextXY(6, 13, "Please insert a formatted floppy disk in drive A: and");
  CONSOLE_SetTextXY(6, 14, "press ENTER.");


  CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");
//  SetStatusText("   Please wait...");

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
	  (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
	{
	  if (ConfirmQuit(Ir) == TRUE)
	    return QUIT_PAGE;
	  break;
	}
      else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)	/* ENTER */
	{
	  if (DoesFileExist(L"\\Device\\Floppy0", L"\\") == FALSE)
	    {
	      PopupError("No disk in drive A:.",
			 "ENTER = Continue",
			 Ir, POPUP_WAIT_ENTER);
	      return BOOT_LOADER_FLOPPY_PAGE;
	    }

	  Status = InstallFatBootcodeToFloppy(&SourceRootPath,
					      &DestinationArcPath);
	  if (!NT_SUCCESS(Status))
	    {
	      /* Print error message */
	      return BOOT_LOADER_FLOPPY_PAGE;
	    }

	  return SUCCESS_PAGE;
	}
    }

  return BOOT_LOADER_FLOPPY_PAGE;
}


static PAGE_NUMBER
BootLoaderHarddiskPage(PINPUT_RECORD Ir)
{
  UCHAR PartitionType;
  NTSTATUS Status;

  PartitionType = PartitionList->ActiveBootPartition->PartInfo[0].PartitionType;
  if ((PartitionType == PARTITION_FAT_12) ||
      (PartitionType == PARTITION_FAT_16) ||
      (PartitionType == PARTITION_HUGE) ||
      (PartitionType == PARTITION_XINT13) ||
      (PartitionType == PARTITION_FAT32) ||
      (PartitionType == PARTITION_FAT32_XINT13))
    {
      Status = InstallFatBootcodeToPartition(&SystemRootPath,
					     &SourceRootPath,
					     &DestinationArcPath,
					     PartitionType);
      if (!NT_SUCCESS(Status))
	{
	  PopupError("Setup failed to install the FAT bootcode on the system partition.",
		     "ENTER = Reboot computer",
		     Ir, POPUP_WAIT_ENTER);
	  return QUIT_PAGE;
	}

      return SUCCESS_PAGE;
    }
  else
    {
      PopupError("failed to install FAT bootcode on the system partition.",
		 "ENTER = Reboot computer",
		 Ir, POPUP_WAIT_ENTER);
      return QUIT_PAGE;
    }

  return BOOT_LOADER_HARDDISK_PAGE;
}


static PAGE_NUMBER
QuitPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(10, 6, "ReactOS is not completely installed");

  CONSOLE_SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  CONSOLE_SetTextXY(10, 9, "all CD-ROMs from CD-Drives.");

  CONSOLE_SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  CONSOLE_SetStatusText("   Please wait ...");

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

  CONSOLE_SetStatusText("   ENTER = Reboot computer");

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return FLUSH_PAGE;
	}
    }
}


static PAGE_NUMBER
SuccessPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(10, 6, "The basic components of ReactOS have been installed successfully.");

  CONSOLE_SetTextXY(10, 8, "Remove floppy disk from Drive A: and");
  CONSOLE_SetTextXY(10, 9, "all CD-ROMs from CD-Drive.");

  CONSOLE_SetTextXY(10, 11, "Press ENTER to reboot your computer.");

  CONSOLE_SetStatusText("   ENTER = Reboot computer");

  if (IsUnattendedSetup)
    {
      return FLUSH_PAGE;
    }

  while(TRUE)
    {
      CONSOLE_ConInKey(Ir);

      if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
	{
	  return FLUSH_PAGE;
	}
    }
}


static PAGE_NUMBER
FlushPage(PINPUT_RECORD Ir)
{
  CONSOLE_SetTextXY(10, 6, "The system is now making sure all data is stored on your disk");

  CONSOLE_SetTextXY(10, 8, "This may take a minute");
  CONSOLE_SetTextXY(10, 9, "When finished, your computer will reboot automatically");

  CONSOLE_SetStatusText("   Flushing cache");

  return REBOOT_PAGE;
}


VOID
RunUSetup(VOID)
{
  INPUT_RECORD Ir;
  PAGE_NUMBER Page;
  LARGE_INTEGER Time;

  NtQuerySystemTime(&Time);

  if (!CONSOLE_Init())
    {
      PrintString("Unable to open the console\n\n");
      PrintString("The most common cause of this is using an USB keyboard\n");
      PrintString("USB keyboards are not fully supported yet\n");

      /* Raise a hard error (crash the system/BSOD) */
      NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		       0,0,0,0,0);
    }

  /* Initialize global unicode strings */
  RtlInitUnicodeString(&SourcePath, NULL);
  RtlInitUnicodeString(&SourceRootPath, NULL);
  RtlInitUnicodeString(&SourceRootDir, NULL);
  RtlInitUnicodeString(&InstallPath, NULL);
  RtlInitUnicodeString(&DestinationPath, NULL);
  RtlInitUnicodeString(&DestinationArcPath, NULL);
  RtlInitUnicodeString(&DestinationRootPath, NULL);
  RtlInitUnicodeString(&SystemRootPath, NULL);

  /* Hide the cursor */
  CONSOLE_SetCursorType(TRUE, FALSE);

  Page = START_PAGE;
  while (Page != REBOOT_PAGE)
    {
      CONSOLE_ClearScreen();

      CONSOLE_SetUnderlinedTextXY(4, 3, " ReactOS " KERNEL_VERSION_STR " Setup ");

      switch (Page)
	{
	  /* Start page */
	  case START_PAGE:
	    Page = SetupStartPage(&Ir);
	    break;

	  /* License page */
	  case LICENSE_PAGE:
	    Page = LicensePage(&Ir);
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
	  case SCSI_CONTROLLER_PAGE:
	    Page = ScsiControllerPage(&Ir);
	    break;
#endif

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
	    Page = (PAGE_NUMBER) FormatPartitionPage(&Ir);
	    break;

	  case CHECK_FILE_SYSTEM_PAGE:
	    Page = (PAGE_NUMBER) CheckFileSystemPage(&Ir);
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

	  case BOOT_LOADER_FLOPPY_PAGE:
	    Page = BootLoaderFloppyPage(&Ir);
	    break;

	  case BOOT_LOADER_HARDDISK_PAGE:
	    Page = BootLoaderHarddiskPage(&Ir);
	    break;


	  /* Repair pages */
	  case REPAIR_INTRO_PAGE:
	    Page = RepairIntroPage(&Ir);
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

  /// THE FOLLOWING DPRINT IS FOR THE SYSTEM REGRESSION TOOL
  /// DO NOT REMOVE!!!
  DPRINT1("SYSREG_CHECKPOINT:USETUP_COMPLETE\n");

  FreeConsole();

  /* Avoid bugcheck */
  Time.QuadPart += 50000000;
  NtDelayExecution(FALSE, &Time);

  /* Reboot */
  NtShutdownSystem(ShutdownReboot);
  NtTerminateProcess(NtCurrentProcess(), 0);
}


#ifdef __REACTOS__

VOID NTAPI
NtProcessStartup(PPEB Peb)
{
  RtlNormalizeProcessParams(Peb->ProcessParameters);

  ProcessHeap = Peb->ProcessHeap;
  INF_SetHeap(ProcessHeap);
  RunUSetup();
}
#endif /* __REACTOS__ */

/* EOF */
