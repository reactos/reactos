/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/dos.c
 * PURPOSE:         DOS32 Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/cpu.h"
#include "int32.h"

#include "dos.h"
#include "dos/dem.h"
#include "country.h"
#include "device.h"
#include "handle.h"
#include "dosfiles.h"
#include "memory.h"
#include "process.h"
#include "himem.h"

#include "bios/bios.h"

#include "io.h"
#include "hardware/ps2.h"

#include "emsdrv.h"

/* PRIVATE VARIABLES **********************************************************/

CALLBACK16 DosContext;

/* PUBLIC VARIABLES ***********************************************************/

/* Global DOS data area contained in guest memory */
PDOS_DATA DosData;
/* Easy accessors to useful DOS data area parts */
PDOS_SYSVARS SysVars;
PDOS_SDA Sda;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN DosChangeDrive(BYTE Drive)
{
    CHAR DirectoryPath[DOS_CMDLINE_LENGTH + 1];

    /* Make sure the drive exists */
    if (Drive >= SysVars->NumLocalDrives) return FALSE;

    RtlZeroMemory(DirectoryPath, sizeof(DirectoryPath));

    /* Find the path to the new current directory */
    snprintf(DirectoryPath,
             DOS_CMDLINE_LENGTH,
             "%c:\\%s",
             'A' + Drive,
             DosData->CurrentDirectories[Drive]);

    /* Change the current directory of the process */
    if (!SetCurrentDirectoryA(DirectoryPath)) return FALSE;

    /* Set the current drive */
    Sda->CurrentDrive = Drive;

    /* Return success */
    return TRUE;
}

static BOOLEAN DosChangeDirectory(LPSTR Directory)
{
    BYTE DriveNumber;
    DWORD Attributes;
    LPSTR Path;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];

    /* Make sure the directory path is not too long */
    if (strlen(Directory) >= DOS_DIR_LENGTH)
    {
        Sda->LastErrorCode = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Check whether the directory string is of format "X:..." */
    if (strlen(Directory) >= 2 && Directory[1] == ':')
    {
        /* Get the drive number */
        DriveNumber = RtlUpperChar(Directory[0]) - 'A';

        /* Make sure the drive exists */
        if (DriveNumber >= SysVars->NumLocalDrives)
        {
            Sda->LastErrorCode = ERROR_PATH_NOT_FOUND;
            return FALSE;
        }
    }
    else
    {
        /* Keep the current drive number */
        DriveNumber = Sda->CurrentDrive;
    }

    /* Get the file attributes */
    Attributes = GetFileAttributesA(Directory);

    /* Make sure the path exists and is a directory */
    if ((Attributes == INVALID_FILE_ATTRIBUTES) ||
       !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        Sda->LastErrorCode = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Check if this is the current drive */
    if (DriveNumber == Sda->CurrentDrive)
    {
        /* Change the directory */
        if (!SetCurrentDirectoryA(Directory))
        {
            Sda->LastErrorCode = LOWORD(GetLastError());
            return FALSE;
        }
    }

    /* Get the (possibly new) current directory (needed if we specified a relative directory) */
    if (!GetCurrentDirectoryA(sizeof(CurrentDirectory), CurrentDirectory))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Convert it to a DOS path */
    if (!GetShortPathNameA(CurrentDirectory, DosDirectory, sizeof(DosDirectory)))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Get the directory part of the path and set the current directory for the drive */
    Path = strchr(DosDirectory, '\\');
    if (Path != NULL)
    {
        Path++; // Skip the backslash
        strncpy(DosData->CurrentDirectories[DriveNumber], Path, DOS_DIR_LENGTH);
    }
    else
    {
        DosData->CurrentDirectories[DriveNumber][0] = '\0';
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN DosIsFileOnCdRom(VOID)
{
    UINT DriveType;
    CHAR RootPathName[4];

    /* Construct a simple <letter>:\ string to get drive type */
    RootPathName[0] = Sda->CurrentDrive + 'A';
    RootPathName[1] = ':';
    RootPathName[2] = '\\';
    RootPathName[3] = ANSI_NULL;

    DriveType = GetDriveTypeA(RootPathName);
    return (DriveType == DRIVE_CDROM);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN DosControlBreak(VOID)
{
    setCF(0);

    /* Print an extra newline */
    DosPrintCharacter(DOS_OUTPUT_HANDLE, '\r');
    DosPrintCharacter(DOS_OUTPUT_HANDLE, '\n');

    /* Call interrupt 0x23 */
    Int32Call(&DosContext, 0x23);

    if (getCF())
    {
        DosTerminateProcess(Sda->CurrentPsp, 0, 0);
        return TRUE;
    }

    return FALSE;
}

VOID WINAPI DosInt20h(LPWORD Stack)
{
    /*
     * This is the exit interrupt (alias to INT 21h, AH=00h).
     * CS must be the PSP segment.
     */
    DosTerminateProcess(Stack[STACK_CS], 0, 0);
}

VOID WINAPI DosInt21h(LPWORD Stack)
{
    BYTE Character;
    SYSTEMTIME SystemTime;
    PCHAR String;

    Sda->InDos++;

    /* Save the value of SS:SP on entry in the PSP */
    SEGMENT_TO_PSP(Sda->CurrentPsp)->LastStack =
    MAKELONG(getSP() + (STACK_FLAGS + 1) * 2, getSS());

    /* Check the value in the AH register */
    switch (getAH())
    {
        /* Terminate Program */
        case 0x00:
        {
            /* CS must be the PSP segment */
            DosTerminateProcess(Stack[STACK_CS], 0, 0);
            break;
        }

        /* Read Character from STDIN with Echo */
        case 0x01:
        {
            DPRINT("INT 21h, AH = 01h\n");

            Character = DosReadCharacter(DOS_INPUT_HANDLE, TRUE);
            if (Character == 0x03 && DosControlBreak()) break;

            setAL(Character);
            break;
        }

        /* Write Character to STDOUT */
        case 0x02:
        {
            // FIXME: Under DOS 2+, output handle may be redirected!!!!
            Character = getDL();
            DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

            /*
             * We return the output character (DOS 2.1+).
             * Also, if we're going to output a TAB, then
             * don't return a TAB but a SPACE instead.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2554.htm
             * for more information.
             */
            setAL(Character == '\t' ? ' ' : Character);
            break;
        }

        /* Read Character from STDAUX */
        case 0x03:
        {
            // FIXME: Really read it from STDAUX!
            DPRINT1("INT 16h, 03h: Read character from STDAUX is HALFPLEMENTED\n");
            // setAL(DosReadCharacter());
            break;
        }

        /* Write Character to STDAUX */
        case 0x04:
        {
            // FIXME: Really write it to STDAUX!
            DPRINT1("INT 16h, 04h: Write character to STDAUX is HALFPLEMENTED\n");
            // DosPrintCharacter(getDL());
            break;
        }

        /* Write Character to Printer */
        case 0x05:
        {
            // FIXME: Really write it to printer!
            DPRINT1("INT 16h, 05h: Write character to printer is HALFPLEMENTED -\n\n");
            DPRINT1("0x%p\n", getDL());
            DPRINT1("\n\n-----------\n\n");
            break;
        }

        /* Direct Console I/O */
        case 0x06:
        {
            Character = getDL();

            // FIXME: Under DOS 2+, output handle may be redirected!!!!

            if (Character != 0xFF)
            {
                /* Output */
                DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

                /*
                 * We return the output character (DOS 2.1+).
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2558.htm
                 * for more information.
                 */
                setAL(Character);
            }
            else
            {
                /* Input */
                if (DosCheckInput())
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                    setAL(DosReadCharacter(DOS_INPUT_HANDLE, FALSE));
                }
                else
                {
                    /* No character available */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
                    setAL(0x00);
                }
            }

            break;
        }

        /* Direct Character Input without Echo */
        case 0x07:
        {
            DPRINT("Direct char input without echo\n");
            setAL(DosReadCharacter(DOS_INPUT_HANDLE, FALSE));
            break;
        }

        /* Character Input without Echo */
        case 0x08:
        {
            DPRINT("Char input without echo\n");

            Character = DosReadCharacter(DOS_INPUT_HANDLE, FALSE);
            if (Character == 0x03 && DosControlBreak()) break;

            setAL(Character);
            break;
        }

        /* Write String to STDOUT */
        case 0x09:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            while (*String != '$')
            {
                DosPrintCharacter(DOS_OUTPUT_HANDLE, *String);
                String++;
            }

            /*
             * We return the terminating character (DOS 2.1+).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2562.htm
             * for more information.
             */
            setAL('$'); // *String
            break;
        }

        /* Read Buffered Input */
        case 0x0A:
        {
            PDOS_INPUT_BUFFER InputBuffer = (PDOS_INPUT_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            DPRINT("Read Buffered Input\n");
            if (InputBuffer->MaxLength == 0) break;

            /* Read from standard input */
            InputBuffer->Length = DosReadLineBuffered(
                DOS_INPUT_HANDLE,
                MAKELONG(getDX() + FIELD_OFFSET(DOS_INPUT_BUFFER, Buffer), getDS()),
                InputBuffer->MaxLength
            );

            break;
        }

        /* Get STDIN Status */
        case 0x0B:
        {
            setAL(DosCheckInput() ? 0xFF : 0x00);
            break;
        }

        /* Flush Buffer and Read STDIN */
        case 0x0C:
        {
            BYTE InputFunction = getAL();

            /* Flush STDIN buffer */
            DosFlushFileBuffers(DOS_INPUT_HANDLE);

            /*
             * If the input function number contained in AL is valid, i.e.
             * AL == 0x01 or 0x06 or 0x07 or 0x08 or 0x0A, call ourselves
             * recursively with AL == AH.
             */
            if (InputFunction == 0x01 || InputFunction == 0x06 ||
                InputFunction == 0x07 || InputFunction == 0x08 ||
                InputFunction == 0x0A)
            {
                /* Call ourselves recursively */
                setAH(InputFunction);
                DosInt21h(Stack);
            }
            break;
        }

        /* Disk Reset */
        case 0x0D:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);

            // TODO: Flush what's needed.
            DPRINT1("INT 21h, 0Dh is UNIMPLEMENTED\n");

            /* Clear CF in DOS 6 only */
            if (PspBlock->DosVersion == 0x0006)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        /* Set Default Drive  */
        case 0x0E:
        {
            DosChangeDrive(getDL());
            setAL(SysVars->NumLocalDrives);
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x18:
        {
            /*
             * This function corresponds to the CP/M BDOS function
             * "get bit map of logged drives", which is meaningless
             * under MS-DOS.
             *
             * For: PTS-DOS 6.51 & S/DOS 1.0 - EXTENDED RENAME FILE USING FCB
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2584.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* Get Default Drive */
        case 0x19:
        {
            setAL(Sda->CurrentDrive);
            break;
        }

        /* Set Disk Transfer Area */
        case 0x1A:
        {
            Sda->DiskTransferArea = MAKELONG(getDX(), getDS());
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x1D:
        case 0x1E:
        {
            /*
             * Function 0x1D corresponds to the CP/M BDOS function
             * "get bit map of read-only drives", which is meaningless
             * under MS-DOS.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2592.htm
             * for more information.
             *
             * Function 0x1E corresponds to the CP/M BDOS function
             * "set file attributes", which was meaningless under MS-DOS 1.x.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2593.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x20:
        {
            /*
             * This function corresponds to the CP/M BDOS function
             * "get/set default user (sublibrary) number", which is meaningless
             * under MS-DOS.
             *
             * For: S/DOS 1.0+ & PTS-DOS 6.51+ - GET OEM REVISION
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2596.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* Set Interrupt Vector */
        case 0x25:
        {
            ULONG FarPointer = MAKELONG(getDX(), getDS());
            DPRINT1("Setting interrupt 0x%02X to %04X:%04X ...\n",
                    getAL(), HIWORD(FarPointer), LOWORD(FarPointer));

            /* Write the new far pointer to the IDT */
            ((PULONG)BaseAddress)[getAL()] = FarPointer;
            break;
        }

        /* Create New PSP */
        case 0x26:
        {
            /* DOS 2+ assumes that the caller's CS is the segment of the PSP to copy */
            DosClonePsp(getDX(), Stack[STACK_CS]);
            break;
        }

        /* Parse Filename into FCB */
        case 0x29:
        {
            PCHAR FileName = (PCHAR)SEG_OFF_TO_PTR(getDS(), getSI());
            PDOS_FCB Fcb = (PDOS_FCB)SEG_OFF_TO_PTR(getES(), getDI());
            BYTE Options = getAL();
            CHAR FillChar = ' ';
            UINT i;

            if (FileName[1] == ':')
            {
                /* Set the drive number */
                Fcb->DriveNumber = RtlUpperChar(FileName[0]) - 'A' + 1;

                /* Skip to the file name part */
                FileName += 2;
            }
            else
            {
                /* No drive number specified */
                if (Options & (1 << 1)) Fcb->DriveNumber = Sda->CurrentDrive + 1;
                else Fcb->DriveNumber = 0;
            }

            /* Parse the file name */
            i = 0;
            while ((*FileName > 0x20) && (i < 8))
            {
                if (*FileName == '.') break;
                else if (*FileName == '*')
                {
                    FillChar = '?';
                    break;
                }

                Fcb->FileName[i++] = RtlUpperChar(*FileName++);
            }

            /* Fill the whole field with blanks only if bit 2 is not set */
            if ((FillChar != ' ') || (i != 0) || !(Options & (1 << 2)))
            {
                for (; i < 8; i++) Fcb->FileName[i] = FillChar;
            }

            /* Skip to the extension part */
            while (*FileName > 0x20 && *FileName != '.') FileName++;
            if (*FileName == '.') FileName++;

            /* Now parse the extension */
            i = 0;
            FillChar = ' ';

            while ((*FileName > 0x20) && (i < 3))
            {
                if (*FileName == '*')
                {
                    FillChar = '?';
                    break;
                }

                Fcb->FileExt[i++] = RtlUpperChar(*FileName++);
            }

            /* Fill the whole field with blanks only if bit 3 is not set */
            if ((FillChar != ' ') || (i != 0) || !(Options & (1 << 3)))
            {
                for (; i < 3; i++) Fcb->FileExt[i] = FillChar;
            }

            break;
        }

        /* Get System Date */
        case 0x2A:
        {
            GetLocalTime(&SystemTime);
            setCX(SystemTime.wYear);
            setDX(MAKEWORD(SystemTime.wDay, SystemTime.wMonth));
            setAL(SystemTime.wDayOfWeek);
            break;
        }

        /* Set System Date */
        case 0x2B:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wYear  = getCX();
            SystemTime.wMonth = getDH();
            SystemTime.wDay   = getDL();

            /* Return success or failure */
            setAL(SetLocalTime(&SystemTime) ? 0x00 : 0xFF);
            break;
        }

        /* Get System Time */
        case 0x2C:
        {
            GetLocalTime(&SystemTime);
            setCX(MAKEWORD(SystemTime.wMinute, SystemTime.wHour));
            setDX(MAKEWORD(SystemTime.wMilliseconds / 10, SystemTime.wSecond));
            break;
        }

        /* Set System Time */
        case 0x2D:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wHour         = getCH();
            SystemTime.wMinute       = getCL();
            SystemTime.wSecond       = getDH();
            SystemTime.wMilliseconds = getDL() * 10; // In hundredths of seconds

            /* Return success or failure */
            setAL(SetLocalTime(&SystemTime) ? 0x00 : 0xFF);
            break;
        }

        /* Get Disk Transfer Area */
        case 0x2F:
        {
            setES(HIWORD(Sda->DiskTransferArea));
            setBX(LOWORD(Sda->DiskTransferArea));
            break;
        }

        /* Get DOS Version */
        case 0x30:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(Sda->CurrentPsp);

            /*
             * DOS 2+ - GET DOS VERSION
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2711.htm
             * for more information.
             */

            if (LOBYTE(PspBlock->DosVersion) < 5 || getAL() == 0x00)
            {
                /*
                 * Return DOS OEM number:
                 * 0x00 for IBM PC-DOS
                 * 0x02 for packaged MS-DOS
                 * 0xFF for NT DOS
                 */
                setBH(0xFF);
            }

            if (LOBYTE(PspBlock->DosVersion) >= 5 && getAL() == 0x01)
            {
                /*
                 * Return version flag:
                 * 1 << 3 if DOS is in ROM,
                 * 0 (reserved) if not.
                 */
                setBH(0x00);
            }

            /* Return DOS 24-bit user serial number in BL:CX */
            setBL(0x00);
            setCX(0x0000);

            /*
             * Return DOS version: Minor:Major in AH:AL
             * The Windows NT DOS box returns version 5.00, subject to SETVER.
             */
            setAX(PspBlock->DosVersion);

            break;
        }

        /* Terminate and Stay Resident */
        case 0x31:
        {
            DPRINT1("Process going resident: %u paragraphs kept\n", getDX());
            DosTerminateProcess(Sda->CurrentPsp, getAL(), getDX());
            break;
        }

        /* Extended functionalities */
        case 0x33:
        {
            switch (getAL())
            {
                /*
                 * DOS 4+ - GET BOOT DRIVE
                 */
                case 0x05:
                {
                    setDL(SysVars->BootDrive);
                    break;
                }

                /*
                 * DOS 5+ - GET TRUE VERSION NUMBER
                 * This function always returns the true version number, unlike
                 * AH=30h, whose return value may be changed with SETVER.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2730.htm
                 * for more information.
                 */
                case 0x06:
                {
                    /*
                     * Return the true DOS version: Minor:Major in BH:BL
                     * The Windows NT DOS box returns BX=3205h (version 5.50).
                     */
                    setBX(NTDOS_VERSION);

                    /* DOS revision 0 */
                    setDL(0x00);

                    /* Unpatched DOS */
                    setDH(0x00);

                    break;
                }

                default: // goto Default;
                {
                    DPRINT1("INT 21h, AH = %02Xh, subfunction AL = %02Xh NOT IMPLEMENTED\n",
                            getAH(), getAL());
                }
            }

            break;
        }

        /* Get Address of InDOS flag */
        case 0x34:
        {
            setES(DOS_DATA_SEGMENT);
            setBX(DOS_DATA_OFFSET(Sda.InDos));
            break;
        }

        /* Get Interrupt Vector */
        case 0x35:
        {
            ULONG FarPointer = ((PULONG)BaseAddress)[getAL()];

            /* Read the address from the IDT into ES:BX */
            setES(HIWORD(FarPointer));
            setBX(LOWORD(FarPointer));
            break;
        }

        /* Get Free Disk Space */
        case 0x36:
        {
            CHAR RootPath[] = "?:\\";
            DWORD SectorsPerCluster;
            DWORD BytesPerSector;
            DWORD NumberOfFreeClusters;
            DWORD TotalNumberOfClusters;

            if (getDL() == 0x00)
                RootPath[0] = 'A' + Sda->CurrentDrive;
            else
                RootPath[0] = 'A' + getDL() - 1;

            if (GetDiskFreeSpaceA(RootPath,
                                  &SectorsPerCluster,
                                  &BytesPerSector,
                                  &NumberOfFreeClusters,
                                  &TotalNumberOfClusters))
            {
                setAX(LOWORD(SectorsPerCluster));
                setCX(LOWORD(BytesPerSector));
                setBX(min(NumberOfFreeClusters, 0xFFFF));
                setDX(min(TotalNumberOfClusters, 0xFFFF));
            }
            else
            {
                /* Error */
                setAX(0xFFFF);
            }

            break;
        }

        /* SWITCH character - AVAILDEV */
        case 0x37:
        {
            switch (getAL())
            {
                /*
                 * DOS 2+ - "SWITCHAR" - GET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 4.0+.
                 * MS-DOS 5+ always return AL=00h/DL=2Fh.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2752.htm
                 * for more information.
                 */
                case 0x00:
                    setDL('/');
                    setAL(0x00);
                    break;

                /*
                 * DOS 2+ - "SWITCHAR" - SET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 5+.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2753.htm
                 * for more information.
                 */
                case 0x01:
                    // getDL();
                    setAL(0xFF);
                    break;

                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                case 0x02:
                    // setDL();
                    setAL(0xFF);
                    break;

                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                case 0x03:
                    // getDL();
                    setAL(0xFF);
                    break;

                /* Invalid subfunction */
                default:
                    setAL(0xFF);
                    break;
            }

            break;
        }

        /* Get/Set Country-dependent Information */
        case 0x38:
        {
            WORD CountryId = getAL() < 0xFF ? getAL() : getBX();
            WORD ErrorCode;

            ErrorCode = DosGetCountryInfo(&CountryId,
                                          (PDOS_COUNTRY_INFO)SEG_OFF_TO_PTR(getDS(), getDX()));

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setBX(CountryId);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Create Directory */
        case 0x39:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (CreateDirectoryA(String, NULL))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(LOWORD(GetLastError()));
            }

            break;
        }

        /* Remove Directory */
        case 0x3A:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (RemoveDirectoryA(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(LOWORD(GetLastError()));
            }

            break;
        }

        /* Set Current Directory */
        case 0x3B:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (DosChangeDirectory(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
            }

            break;
        }

        /* Create or Truncate File */
        case 0x3C:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                           CREATE_ALWAYS,
                                           getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Open File or Device */
        case 0x3D:
        {
            WORD FileHandle;
            BYTE AccessShareModes = getAL();
            LPCSTR FileName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            WORD ErrorCode = DosOpenFile(&FileHandle, FileName, AccessShareModes);

            /*
             * Check if we failed because we attempted to open a file for write
             * on a CDROM drive. In that situation, attempt to reopen for read
             */
            if (ErrorCode == ERROR_ACCESS_DENIED &&
                (AccessShareModes & 0x03) != 0 && DosIsFileOnCdRom())
            {
                ErrorCode = DosOpenFile(&FileHandle, FileName, 0);
            }

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Close File or Device */
        case 0x3E:
        {
            if (DosCloseHandle(getBX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Read from File or Device */
        case 0x3F:
        {
            WORD BytesRead = 0;
            WORD ErrorCode;

            DPRINT("DosReadFile(0x%04X)\n", getBX());

            ErrorCode = DosReadFile(getBX(),
                                    MAKELONG(getDX(), getDS()),
                                    getCX(),
                                    &BytesRead);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(BytesRead);
            }
            else if (ErrorCode != ERROR_NOT_READY)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Write to File or Device */
        case 0x40:
        {
            WORD BytesWritten = 0;
            WORD ErrorCode = DosWriteFile(getBX(),
                                          MAKELONG(getDX(), getDS()),
                                          getCX(),
                                          &BytesWritten);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(BytesWritten);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Delete File */
        case 0x41:
        {
            LPSTR FileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (demFileDelete(FileName) == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                /*
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2797.htm
                 * "AX destroyed (DOS 3.3) AL seems to be drive of deleted file."
                 */
                setAL(RtlUpperChar(FileName[0]) - 'A');
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* Seek File */
        case 0x42:
        {
            DWORD NewLocation;
            WORD ErrorCode = DosSeekFile(getBX(),
                                         MAKELONG(getDX(), getCX()),
                                         getAL(),
                                         &NewLocation);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the new offset in DX:AX */
                setDX(HIWORD(NewLocation));
                setAX(LOWORD(NewLocation));
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Get/Set File Attributes */
        case 0x43:
        {
            DWORD Attributes;
            LPSTR FileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (getAL() == 0x00)
            {
                /* Get the attributes */
                Attributes = GetFileAttributesA(FileName);

                /* Check if it failed */
                if (Attributes == INVALID_FILE_ATTRIBUTES)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
                else
                {
                    /* Return the attributes that DOS can understand */
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    setCX(Attributes & 0x00FF);
                }
            }
            else if (getAL() == 0x01)
            {
                /* Try to set the attributes */
                if (SetFileAttributesA(FileName, getCL()))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* IOCTL */
        case 0x44:
        {
            WORD Length = getCX();

            if (DosDeviceIoControl(getBX(), getAL(), MAKELONG(getDX(), getDS()), &Length))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(Length);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
            }

            break;
        }

        /* Duplicate Handle */
        case 0x45:
        {
            WORD NewHandle = DosDuplicateHandle(getBX());

            if (NewHandle != INVALID_DOS_HANDLE)
            {
                setAX(NewHandle);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
            }

            break;
        }

        /* Force Duplicate Handle */
        case 0x46:
        {
            if (DosForceDuplicateHandle(getBX(), getCX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Get Current Directory */
        case 0x47:
        {
            BYTE DriveNumber = getDL();
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getSI());

            /* Get the real drive number */
            if (DriveNumber == 0)
            {
                DriveNumber = Sda->CurrentDrive;
            }
            else
            {
                /* Decrement DriveNumber since it was 1-based */
                DriveNumber--;
            }

            if (DriveNumber < SysVars->NumLocalDrives)
            {
                /*
                 * Copy the current directory into the target buffer.
                 * It doesn't contain the drive letter and the backslash.
                 */
                strncpy(String, DosData->CurrentDirectories[DriveNumber], DOS_DIR_LENGTH);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(0x0100); // Undocumented, see Ralf Brown: http://www.ctyme.com/intr/rb-2933.htm
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_DRIVE);
            }

            break;
        }

        /* Allocate Memory */
        case 0x48:
        {
            WORD MaxAvailable = 0;
            WORD Segment = DosAllocateMemory(getBX(), &MaxAvailable);

            if (Segment != 0)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(Segment);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
                setBX(MaxAvailable);
            }

            break;
        }

        /* Free Memory */
        case 0x49:
        {
            if (DosFreeMemory(getES()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
            }

            break;
        }

        /* Resize Memory Block */
        case 0x4A:
        {
            WORD Size;

            if (DosResizeMemory(getES(), getBX(), &Size))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
                setBX(Size);
            }

            break;
        }

        /* Execute */
        case 0x4B:
        {
            BYTE OrgAL = getAL();
            LPSTR ProgramName = SEG_OFF_TO_PTR(getDS(), getDX());
            PDOS_EXEC_PARAM_BLOCK ParamBlock = SEG_OFF_TO_PTR(getES(), getBX());
            WORD ErrorCode;

            if (OrgAL <= DOS_LOAD_OVERLAY)
            {
                DOS_EXEC_TYPE LoadType = (DOS_EXEC_TYPE)OrgAL;

                if (LoadType == DOS_LOAD_AND_EXECUTE)
                {
                    /* Create a new process */
                    ErrorCode = DosCreateProcess(ProgramName,
                                                 ParamBlock,
                                                 MAKELONG(Stack[STACK_IP], Stack[STACK_CS]));
                }
                else
                {
                    /* Just load an executable */
                    ErrorCode = DosLoadExecutable(LoadType,
                                                  ProgramName,
                                                  ParamBlock,
                                                  NULL,
                                                  NULL,
                                                  MAKELONG(Stack[STACK_IP], Stack[STACK_CS]));
                }
            }
            else if (OrgAL == 0x05)
            {
                // http://www.ctyme.com/intr/rb-2942.htm
                DPRINT1("Set execution state is UNIMPLEMENTED\n");
                ErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
            }
            else
            {
                ErrorCode = ERROR_INVALID_FUNCTION;
            }

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Terminate with Return Code */
        case 0x4C:
        {
            DosTerminateProcess(Sda->CurrentPsp, getAL(), 0);
            break;
        }

        /* Get Return Code (ERRORLEVEL) */
        case 0x4D:
        {
            /*
             * According to Ralf Brown: http://www.ctyme.com/intr/rb-2976.htm
             * DosErrorLevel is cleared after being read by this function.
             */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            setAX(Sda->ErrorLevel);
            Sda->ErrorLevel = 0x0000; // Clear it
            break;
        }

        /* Find First File */
        case 0x4E:
        {
            WORD Result = (WORD)demFileFindFirst(FAR_POINTER(Sda->DiskTransferArea),
                                                 SEG_OFF_TO_PTR(getDS(), getDX()),
                                                 getCX());

            setAX(Result);

            if (Result == ERROR_SUCCESS)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Find Next File */
        case 0x4F:
        {
            WORD Result = (WORD)demFileFindNext(FAR_POINTER(Sda->DiskTransferArea));

            setAX(Result);

            if (Result == ERROR_SUCCESS)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Internal - Set Current Process ID (Set PSP Address) */
        case 0x50:
        {
            DosSetProcessContext(getBX());
            break;
        }

        /* Internal - Get Current Process ID (Get PSP Address) */
        case 0x51:
        /* Get Current PSP Address */
        case 0x62:
        {
            /*
             * Undocumented AH=51h is identical to the documented AH=62h.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2982.htm
             * and http://www.ctyme.com/intr/rb-3140.htm
             * for more information.
             */
            setBX(Sda->CurrentPsp);
            break;
        }

        /* Internal - Get "List of lists" (SYSVARS) */
        case 0x52:
        {
            /*
             * On return, ES points at the DOS data segment (see also INT 2F/AX=1203h).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2983.htm
             * for more information.
             */

            /* Return the DOS "list of lists" in ES:BX */
            setES(DOS_DATA_SEGMENT);
            setBX(DOS_DATA_OFFSET(SysVars.FirstDpb));
            break;
        }

        /* Create Child PSP */
        case 0x55:
        {
            DosCreatePsp(getDX(), getSI());
            DosSetProcessContext(getDX());
            break;
        }

        /* Rename File */
        case 0x56:
        {
            LPSTR ExistingFileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            LPSTR NewFileName      = (LPSTR)SEG_OFF_TO_PTR(getES(), getDI());

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2990.htm
             * for more information.
             */

            if (MoveFileA(ExistingFileName, NewFileName))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* File Attributes */
        case 0x57:
        {
            switch (getAL())
            {
                /* Get File's last-written Date and Time */
                case 0x00:
                {
                    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(getBX());
                    FILETIME LastWriteTime;
                    WORD FileDate, FileTime;

                    if (Descriptor == NULL)
                    {
                        /* Invalid handle */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        // Sda->LastErrorCode = ERROR_INVALID_HANDLE;
                        setAX(ERROR_INVALID_HANDLE);
                        break;
                    }

                    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
                    {
                        /* Invalid for devices */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        // setAX(ERROR_INVALID_FUNCTION);
                        setAX(ERROR_INVALID_HANDLE);
                        break;
                    }

                    /*
                     * Retrieve the last-written Win32 date and time,
                     * and convert it to DOS format.
                     */
                    if (!GetFileTime(Descriptor->Win32Handle,
                                     NULL, NULL, &LastWriteTime) ||
                        !FileTimeToDosDateTime(&LastWriteTime,
                                               &FileDate, &FileTime))
                    {
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        setAX(GetLastError());
                        break;
                    }

                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    setCX(FileTime);
                    setDX(FileDate);
                    break;
                }

                /* Set File's last-written Date and Time */
                case 0x01:
                {
                    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(getBX());
                    FILETIME LastWriteTime;
                    WORD FileDate = getDX();
                    WORD FileTime = getCX();

                    if (Descriptor == NULL)
                    {
                        /* Invalid handle */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        // Sda->LastErrorCode = ERROR_INVALID_HANDLE;
                        setAX(ERROR_INVALID_HANDLE);
                        break;
                    }

                    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
                    {
                        /* Invalid for devices */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        // setAX(ERROR_INVALID_FUNCTION);
                        setAX(ERROR_INVALID_HANDLE);
                        break;
                    }

                    /*
                     * Convert the new last-written DOS date and time
                     * to Win32 format and set it.
                     */
                    if (!DosDateTimeToFileTime(FileDate, FileTime,
                                               &LastWriteTime) ||
                        !SetFileTime(Descriptor->Win32Handle,
                                     NULL, NULL, &LastWriteTime))
                    {
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        setAX(GetLastError());
                        break;
                    }

                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                default: // goto Default;
                {
                    DPRINT1("INT 21h, AH = %02Xh, subfunction AL = %02Xh NOT IMPLEMENTED\n",
                            getAH(), getAL());
                }
            }

            break;
        }

        /* Get/Set Memory Management Options */
        case 0x58:
        {
            switch (getAL())
            {
                /* Get allocation strategy */
                case 0x00:
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    setAX(Sda->AllocStrategy);
                    break;
                }

                /* Set allocation strategy */
                case 0x01:
                {
                    if ((getBL() & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                        == (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                    {
                        /* Can't set both */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        setAX(ERROR_INVALID_PARAMETER);
                        break;
                    }

                    if ((getBL() & ~(DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                        > DOS_ALLOC_LAST_FIT)
                    {
                        /* Invalid allocation strategy */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        setAX(ERROR_INVALID_PARAMETER);
                        break;
                    }

                    Sda->AllocStrategy = getBL();
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                /* Get UMB link state */
                case 0x02:
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    setAL(SysVars->UmbLinked ? 0x01 : 0x00);
                    break;
                }

                /* Set UMB link state */
                case 0x03:
                {
                    BOOLEAN Success;

                    if (getBX())
                        Success = DosLinkUmb();
                    else
                        Success = DosUnlinkUmb();

                    if (Success)
                        Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    else
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                    break;
                }

                /* Invalid or unsupported function */
                default:
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(ERROR_INVALID_FUNCTION);
                }
            }

            break;
        }

        /* Get Extended Error Information */
        case 0x59:
        {
            DPRINT1("INT 21h, AH = 59h, BX = %04Xh - Get Extended Error Information is UNIMPLEMENTED\n",
                    getBX());
            break;
        }

        /* Create Temporary File */
        case 0x5A:
        {
            LPSTR PathName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            LPSTR FileName = PathName; // The buffer for the path and the full file name is the same.
            UINT  uRetVal;
            WORD  FileHandle;
            WORD  ErrorCode;

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3014.htm
             * for more information.
             */

            // FIXME: Check for buffer validity?
            // It should be a ASCIIZ path ending with a '\' + 13 zero bytes
            // to receive the generated filename.

            /* First create the temporary file */
            uRetVal = GetTempFileNameA(PathName, NULL, 0, FileName);
            if (uRetVal == 0)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
                break;
            }

            /* Now try to open it in read/write access */
            ErrorCode = DosOpenFile(&FileHandle, FileName, 2);
            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Create New File */
        case 0x5B:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                           CREATE_NEW,
                                           getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Lock/Unlock Region of File */
        case 0x5C:
        {
            if (getAL() == 0x00)
            {
                /* Lock region of file */
                if (DosLockFile(getBX(), MAKELONG(getDX(), getCX()), MAKELONG(getDI(), getSI())))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(Sda->LastErrorCode);
                }
            }
            else if (getAL() == 0x01)
            {
                /* Unlock region of file */
                if (DosUnlockFile(getBX(), MAKELONG(getDX(), getCX()), MAKELONG(getDI(), getSI())))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(Sda->LastErrorCode);
                }
            }
            else
            {
                /* Invalid subfunction */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* Canonicalize File Name or Path */
        case 0x60:
        {
            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3137.htm
             * for more information.
             */

            /*
             * We suppose that the DOS app gave to us a valid
             * 128-byte long buffer for the canonicalized name.
             */
            DWORD dwRetVal = GetFullPathNameA(SEG_OFF_TO_PTR(getDS(), getSI()),
                                              128,
                                              SEG_OFF_TO_PTR(getES(), getDI()),
                                              NULL);
            if (dwRetVal == 0)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }
            else
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(0x0000);
            }

            // FIXME: Convert the full path name into short version.
            // We cannot reliably use GetShortPathName, because it fails
            // if the path name given doesn't exist. However this DOS
            // function AH=60h should be able to work even for non-existing
            // path and file names.

            break;
        }

        /* Miscellaneous Internal Functions */
        case 0x5D:
        {
            switch (getAL())
            {
                /* Get Swappable Data Area */
                case 0x06:
                {
                    setDS(DOS_DATA_SEGMENT);
                    setSI(DOS_DATA_OFFSET(Sda.ErrorMode));
                    setCX(sizeof(DOS_SDA));
                    setDX(FIELD_OFFSET(DOS_SDA, LastAX));

                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                default: // goto Default;
                {
                    DPRINT1("INT 21h, AH = %02Xh, subfunction AL = %02Xh NOT IMPLEMENTED\n",
                            getAH(), getAL());
                }
            }

            break;
        }

        /* Extended Country Information */
        case 0x65:
        {
            switch (getAL())
            {
                case 0x01: case 0x02: case 0x03:
                case 0x04: case 0x05: case 0x06:
                case 0x07:
                {
                    WORD BufferSize = getCX();
                    WORD ErrorCode;
                    ErrorCode = DosGetCountryInfoEx(getAL(),
                                                    getBX(),
                                                    getDX(),
                                                    (PDOS_COUNTRY_INFO_2)SEG_OFF_TO_PTR(getES(), getDI()),
                                                    &BufferSize);
                    if (ErrorCode == ERROR_SUCCESS)
                    {
                        Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                        setCX(BufferSize);
                    }
                    else
                    {
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        setAX(ErrorCode);
                    }

                    break;
                }

                /* Country-dependent Character Capitalization -- Character */
                case 0x20:
                /* Country-dependent Filename Capitalization -- Character */
                case 0xA0:
                {
                    setDL(DosToUpper(getDL()));
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    // setAX(ERROR_SUCCESS);
                    break;
                }

                /* Country-dependent Character Capitalization -- Counted ASCII String */
                case 0x21:
                /* Country-dependent Filename Capitalization -- Counted ASCII String */
                case 0xA1:
                {
                    PCHAR Str = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());
                    // FIXME: Check for NULL ptr!!
                    DosToUpperStrN(Str, Str, getCX());
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    // setAX(ERROR_SUCCESS);
                    break;
                }

                /* Country-dependent Character Capitalization -- ASCIIZ String */
                case 0x22:
                /* Country-dependent Filename Capitalization -- ASCIIZ String */
                case 0xA2:
                {
                    PSTR Str = (PSTR)SEG_OFF_TO_PTR(getDS(), getDX());
                    // FIXME: Check for NULL ptr!!
                    DosToUpperStrZ(Str, Str);
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    // setAX(ERROR_SUCCESS);
                    break;
                }

                /* Determine if Character represents YES/NO Response */
                case 0x23:
                {
                    setAX(DosIfCharYesNo(MAKEWORD(getDL(), getDH())));
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                default: // goto Default;
                {
                    DPRINT1("INT 21h, AH = %02Xh, subfunction AL = %02Xh NOT IMPLEMENTED\n",
                            getAH(), getAL());
                }
            }

            break;
        }

        /* Set Handle Count */
        case 0x67:
        {
            if (!DosResizeHandleTable(getBX()))
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(Sda->LastErrorCode);
            }
            else Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        /* Commit File */
        case 0x68:
        case 0x6A:
        {
            /*
             * Function 6Ah is identical to function 68h,
             * and sets AH to 68h if success.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3176.htm
             * for more information.
             */
            setAH(0x68);

            if (DosFlushFileBuffers(getBX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* Extended Open/Create */
        case 0x6C:
        {
            WORD FileHandle;
            WORD CreationStatus;
            WORD ErrorCode;

            /* Check for AL == 00 */
            if (getAL() != 0x00)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
                break;
            }

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3179.htm
             * for the full detailed description.
             *
             * WARNING: BH contains some extended flags that are NOT SUPPORTED.
             */

            ErrorCode = DosCreateFileEx(&FileHandle,
                                        &CreationStatus,
                                        (LPCSTR)SEG_OFF_TO_PTR(getDS(), getSI()),
                                        getBL(),
                                        getDL(),
                                        getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setCX(CreationStatus);
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Long FileName Support */
        case 0x71:
        {
            DPRINT1("INT 21h LFN Support, AH = %02Xh, AL = %02Xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());

            setAL(0); // Some functions expect AL to be 0 when it's not supported.
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            break;
        }

        /* Unsupported */
        default: // Default:
        {
            DPRINT1("DOS Function INT 21h, AH = %02Xh, AL = %02Xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());

            setAL(0); // Some functions expect AL to be 0 when it's not supported.
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }

    Sda->InDos--;
}

VOID WINAPI DosBreakInterrupt(LPWORD Stack)
{
    /* Set CF to terminate the running process */
    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
}

VOID WINAPI DosAbsoluteRead(LPWORD Stack)
{
    /*
     * This call should leave the flags on the stack for some reason,
     * so move the stack by one word.
     * See: http://www.techhelpmanual.com/565-int_25h_26h__absolute_disk_read_write.html
     */
    Stack[STACK_INT_NUM] = Stack[STACK_IP];
    Stack[STACK_IP] = Stack[STACK_CS];
    Stack[STACK_CS] = Stack[STACK_FLAGS];
    setSP(LOWORD(getSP() - 2));

    // TODO: NOT IMPLEMENTED;
    UNIMPLEMENTED;

    /* General failure */
    setAX(0x800C);
    Stack[STACK_FLAGS - 1] |= EMULATOR_FLAG_CF;
}

VOID WINAPI DosAbsoluteWrite(LPWORD Stack)
{
    /*
     * This call should leave the flags on the stack for some reason,
     * so move the stack by one word.
     * See: http://www.techhelpmanual.com/565-int_25h_26h__absolute_disk_read_write.html
     */
    Stack[STACK_INT_NUM] = Stack[STACK_IP];
    Stack[STACK_IP] = Stack[STACK_CS];
    Stack[STACK_CS] = Stack[STACK_FLAGS];
    setSP(LOWORD(getSP() - 2));

    // TODO: NOT IMPLEMENTED;
    UNIMPLEMENTED;

    /* General failure */
    setAX(0x800C);
    Stack[STACK_FLAGS - 1] |= EMULATOR_FLAG_CF;
}

VOID WINAPI DosInt27h(LPWORD Stack)
{
    WORD KeepResident = (getDX() + 0x0F) >> 4;

    /* Terminate and Stay Resident. CS must be the PSP segment. */
    DPRINT1("Process going resident: %u paragraphs kept\n", KeepResident);
    DosTerminateProcess(Stack[STACK_CS], 0, KeepResident);
}

VOID WINAPI DosIdle(LPWORD Stack)
{
    /*
     * This will set the carry flag on the first call (to repeat the BOP),
     * and clear it in the next, so that exactly one HLT occurs.
     */
    setCF(!getCF());
}

VOID WINAPI DosFastConOut(LPWORD Stack)
{
    /*
     * This is the DOS 2+ Fast Console Output Interrupt.
     * The default handler under DOS 2.x and 3.x simply calls INT 10h/AH=0Eh.
     *
     * See Ralf Brown: http://www.ctyme.com/intr/rb-4124.htm
     * for more information.
     */

    /* Save AX and BX */
    USHORT AX = getAX();
    USHORT BX = getBX();

    /*
     * Set the parameters:
     * AL contains the character to print (already set),
     * BL contains the character attribute,
     * BH contains the video page to use.
     */
    setBL(DOS_CHAR_ATTRIBUTE);
    setBH(Bda->VideoPage);

    /* Call the BIOS INT 10h, AH=0Eh "Teletype Output" */
    setAH(0x0E);
    Int32Call(&DosContext, BIOS_VIDEO_INTERRUPT);

    /* Restore AX and BX */
    setBX(BX);
    setAX(AX);
}

VOID WINAPI DosInt2Ah(LPWORD Stack)
{
    DPRINT1("INT 2Ah, AX=%4xh called\n", getAX());
}

VOID WINAPI DosInt2Fh(LPWORD Stack)
{
    switch (getAH())
    {
        /* DOS 3+ Internal Utility Functions */
        case 0x12:
        {
            DPRINT1("INT 2Fh, AX=%4xh DOS Internal Utility Function called\n", getAX());

            switch (getAL())
            {
                /* Installation Check */
                case 0x00:
                {
                    setAL(0xFF);
                    break;
                }

                /* Get DOS Data Segment */
                case 0x03:
                {
                    setDS(DOS_DATA_SEGMENT);
                    break;
                }

                /* Compare FAR Pointers */
                case 0x14:
                {
                    PVOID PointerFromFarPointer1 = SEG_OFF_TO_PTR(getDS(), getSI());
                    PVOID PointerFromFarPointer2 = SEG_OFF_TO_PTR(getES(), getDI());
                    BOOLEAN AreEqual = (PointerFromFarPointer1 == PointerFromFarPointer2);

                    if (AreEqual)
                    {
                        Stack[STACK_FLAGS] |=  EMULATOR_FLAG_ZF;
                        Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    }
                    else
                    {
                        Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                        Stack[STACK_FLAGS] |=  EMULATOR_FLAG_CF;
                    }
                    break;
                }

                /* Set DOS Version Number to return */
                case 0x2F:
                {
                    WORD DosVersion = getDX();

                    // Special case: return the true DOS version when DX=00h
                    if (DosVersion == 0x0000)
                        DosData->DosVersion = DOS_VERSION;
                    else
                        DosData->DosVersion = DosVersion;

                    break;
                }
            }

            break;
        }

        /* Set Disk Interrupt Handler */
        case 0x13:
        {
            /* Save the old values of PrevInt13 and RomBiosInt13 */
            ULONG OldInt13     = BiosData->PrevInt13;
            ULONG OldBiosInt13 = BiosData->RomBiosInt13;

            /* Set PrevInt13 and RomBiosInt13 to their new values */
            BiosData->PrevInt13    = MAKELONG(getDX(), getDS());
            BiosData->RomBiosInt13 = MAKELONG(getBX(), getES());

            /* Return in DS:DX the old value of PrevInt13 */
            setDS(HIWORD(OldInt13));
            setDX(LOWORD(OldInt13));

            /* Return in DS:DX the old value of RomBiosInt13 */
            setES(HIWORD(OldBiosInt13));
            setBX(LOWORD(OldBiosInt13));

            break;
        }

        /* Mostly Windows 2.x/3.x/9x support */
        case 0x16:
        {
            /*
             * AL=80h is DOS/Windows/DPMI "Release Current Virtual Machine Time-slice"
             * Just do nothing in this case.
             */
            if (getAL() != 0x80) goto Default;
            break;
        }

        /* Extended Memory Specification */
        case 0x43:
        {
            DWORD DriverEntry;
            if (!XmsGetDriverEntry(&DriverEntry)) break;

            switch (getAL())
            {
                /* Installation Check */
                case 0x00:
                {
                    /* The driver is loaded */
                    setAL(0x80);
                    break;
                }

                /* Get Driver Address */
                case 0x10:
                {
                    setES(HIWORD(DriverEntry));
                    setBX(LOWORD(DriverEntry));
                    break;
                }

                default:
                    DPRINT1("Unknown DOS XMS Function: INT 2Fh, AH = 43h, AL = %02Xh\n", getAL());
                    break;
            }

            break;
        }

        default: Default:
        {
            DPRINT1("DOS Internal System Function INT 2Fh, AH = %02Xh, AL = %02Xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

BOOLEAN DosKRNLInitialize(VOID)
{
    UCHAR i;
    PDOS_SFT Sft;
    LPSTR Path;
    BOOLEAN Success = TRUE;
    DWORD dwRet;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];

    static const BYTE NullDriverRoutine[] =
    {
        /* Strategy routine entry */
        0x26, // mov [Request.Status], DOS_DEVSTAT_DONE
        0xC7,
        0x47,
        FIELD_OFFSET(DOS_REQUEST_HEADER, Status),
        LOBYTE(DOS_DEVSTAT_DONE),
        HIBYTE(DOS_DEVSTAT_DONE),

        /* Interrupt routine entry */
        0xCB, // retf
    };

    /* Set the data segment */
    setDS(DOS_DATA_SEGMENT);

    /* Initialize the global DOS data area */
    DosData = (PDOS_DATA)SEG_OFF_TO_PTR(DOS_DATA_SEGMENT, 0x0000);
    RtlZeroMemory(DosData, sizeof(*DosData));

    /* Initialize the DOS stack */
    setSS(DOS_DATA_SEGMENT);
    setSP(DOS_DATA_OFFSET(DosStack) + sizeof(DosData->DosStack) - sizeof(WORD));

    /* Initialize the list of lists */
    SysVars = &DosData->SysVars;
    RtlZeroMemory(SysVars, sizeof(*SysVars));
    SysVars->FirstSft = MAKELONG(DOS_DATA_OFFSET(Sft), DOS_DATA_SEGMENT);
    SysVars->CurrentDirs = MAKELONG(DOS_DATA_OFFSET(CurrentDirectories),
                                    DOS_DATA_SEGMENT);
    /*
     * The last drive can be redefined with the LASTDRIVE command.
     * At the moment, set the real maximum possible, 'Z'.
     */
    SysVars->NumLocalDrives = 'Z' - 'A' + 1; // See #define NUM_DRIVES in dos.h

    /* The boot drive is initialized to the %SYSTEMDRIVE% value */
    // NOTE: Using the NtSystemRoot system variable might be OS-specific...
    SysVars->BootDrive = RtlUpcaseUnicodeChar(SharedUserData->NtSystemRoot[0]) - 'A' + 1;

    /* Initialize the NUL device driver */
    SysVars->NullDevice.Link = MAXDWORD;
    SysVars->NullDevice.DeviceAttributes = DOS_DEVATTR_NUL | DOS_DEVATTR_CHARACTER;
    // Offset from within the DOS data segment
    SysVars->NullDevice.StrategyRoutine  = DOS_DATA_OFFSET(NullDriverRoutine);
    // Hardcoded to the RETF inside StrategyRoutine
    SysVars->NullDevice.InterruptRoutine = SysVars->NullDevice.StrategyRoutine + 6;
    RtlFillMemory(SysVars->NullDevice.DeviceName,
                  sizeof(SysVars->NullDevice.DeviceName),
                  ' ');
    RtlCopyMemory(SysVars->NullDevice.DeviceName, "NUL", strlen("NUL"));
    RtlCopyMemory(DosData->NullDriverRoutine,
                  NullDriverRoutine,
                  sizeof(NullDriverRoutine));

    /* Default DOS version to report */
    DosData->DosVersion = DOS_VERSION;

    /* Initialize the swappable data area */
    Sda = &DosData->Sda;
    RtlZeroMemory(Sda, sizeof(*Sda));

    /* Get the current directory and convert it to a DOS path */
    dwRet = GetCurrentDirectoryA(sizeof(CurrentDirectory), CurrentDirectory);
    if (dwRet == 0)
    {
        Success = FALSE;
        DPRINT1("GetCurrentDirectoryA failed (Error: %u)\n", GetLastError());
    }
    else if (dwRet > sizeof(CurrentDirectory))
    {
        Success = FALSE;
        DPRINT1("Current directory too long (%d > MAX_PATH) for GetCurrentDirectoryA\n", dwRet);
    }

    if (Success)
    {
        dwRet = GetShortPathNameA(CurrentDirectory, DosDirectory, sizeof(DosDirectory));
        if (dwRet == 0)
        {
            Success = FALSE;
            DPRINT1("GetShortPathNameA failed (Error: %u)\n", GetLastError());
        }
        else if (dwRet > sizeof(DosDirectory))
        {
            Success = FALSE;
            DPRINT1("Short path too long (%d > DOS_DIR_LENGTH) for GetShortPathNameA\n", dwRet);
        }
    }

    if (!Success)
    {
        /* We failed, use the boot drive instead */
        DosDirectory[0] = SysVars->BootDrive + 'A' - 1;
        DosDirectory[1] = ':';
        DosDirectory[2] = '\\';
        DosDirectory[3] = '\0';
    }

    /* Set the current drive */
    Sda->CurrentDrive = RtlUpperChar(DosDirectory[0]) - 'A';

    /* Get the directory part of the path and set the current directory */
    Path = strchr(DosDirectory, '\\');
    if (Path != NULL)
    {
        Path++; // Skip the backslash
        strncpy(DosData->CurrentDirectories[Sda->CurrentDrive], Path, DOS_DIR_LENGTH);
    }
    else
    {
        DosData->CurrentDirectories[Sda->CurrentDrive][0] = '\0';
    }

    /* Set the current PSP to the system PSP */
    Sda->CurrentPsp = SYSTEM_PSP;

    /* Initialize the SFT */
    Sft = (PDOS_SFT)FAR_POINTER(SysVars->FirstSft);
    Sft->Link = MAXDWORD;
    Sft->NumDescriptors = DOS_SFT_SIZE;

    for (i = 0; i < Sft->NumDescriptors; i++)
    {
        /* Clear the file descriptor entry */
        RtlZeroMemory(&Sft->FileDescriptors[i], sizeof(DOS_FILE_DESCRIPTOR));
    }

    /* Initialize memory management */
    DosInitializeMemory();

    /* Initialize the callback context */
    InitializeContext(&DosContext, DOS_CODE_SEGMENT, 0x0000);

    /* Register the DOS 32-bit Interrupts */
    RegisterDosInt32(0x20, DosInt20h        );
    RegisterDosInt32(0x21, DosInt21h        );
//  RegisterDosInt32(0x22, DosInt22h        ); // Termination
    RegisterDosInt32(0x23, DosBreakInterrupt); // Ctrl-C / Ctrl-Break
//  RegisterDosInt32(0x24, DosInt24h        ); // Critical Error
    RegisterDosInt32(0x25, DosAbsoluteRead  ); // Absolute Disk Read
    RegisterDosInt32(0x26, DosAbsoluteWrite ); // Absolute Disk Write
    RegisterDosInt32(0x27, DosInt27h        ); // Terminate and Stay Resident
    RegisterDosInt32(0x28, DosIdle          ); // DOS Idle Interrupt
    RegisterDosInt32(0x29, DosFastConOut    ); // DOS 2+ Fast Console Output
    RegisterDosInt32(0x2F, DosInt2Fh        ); // Multiplex Interrupt

    /* Unimplemented DOS interrupts */
    RegisterDosInt32(0x2A, DosInt2Ah); // DOS Critical Sections / Network
//  RegisterDosInt32(0x2E, NULL); // COMMAND.COM "Reload Transient"
//  COMMAND.COM adds support for INT 2Fh, AX=AE00h and AE01h "Installable Command - Installation Check & Execute"
//  COMMAND.COM adds support for INT 2Fh, AX=5500h "COMMAND.COM Interface"

    /* Reserved DOS interrupts */
    RegisterDosInt32(0x2B, NULL);
    RegisterDosInt32(0x2C, NULL);
    RegisterDosInt32(0x2D, NULL);

    /* Initialize country data */
    DosCountryInitialize();

    /* Load the CON driver */
    ConDrvInitialize();

    /* Load the XMS driver (HIMEM) */
    XmsInitialize();

    /* Load the EMS driver */
    if (!EmsDrvInitialize(EMS_SEGMENT, EMS_TOTAL_PAGES))
    {
        DosDisplayMessage("Could not initialize EMS. EMS will not be available.\n"
                          "Page frame segment or number of EMS pages invalid.\n");
    }

    /* Finally initialize the UMBs */
    DosInitializeUmb();

    return TRUE;
}

/* EOF */
