/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/dos.c
 * PURPOSE:         DOS32 Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/cpu.h"
#include "int32.h"

#include "dos.h"
#include "dos/dem.h"
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

#define INDOS_POINTER MAKELONG(0x00FE, 0x0070)

CALLBACK16 DosContext;

/*static*/ BYTE CurrentDrive;
static CHAR LastDrive = 'E';
static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];
static PBYTE InDos;

/* PUBLIC VARIABLES ***********************************************************/

PDOS_SYSVARS SysVars;

/* Echo state for INT 21h, AH = 01h and AH = 3Fh */
BOOLEAN DoEcho = FALSE;

DWORD DiskTransferArea;
WORD DosErrorLevel = 0x0000;
WORD DosLastError = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN DosChangeDrive(BYTE Drive)
{
    WCHAR DirectoryPath[DOS_CMDLINE_LENGTH];

    /* Make sure the drive exists */
    if (Drive > (LastDrive - 'A')) return FALSE;

    /* Find the path to the new current directory */
    swprintf(DirectoryPath, L"%c\\%S", Drive + 'A', CurrentDirectories[Drive]);

    /* Change the current directory of the process */
    if (!SetCurrentDirectory(DirectoryPath)) return FALSE;

    /* Set the current drive */
    CurrentDrive = Drive;

    /* Return success */
    return TRUE;
}

static BOOLEAN DosChangeDirectory(LPSTR Directory)
{
    BYTE DriveNumber;
    DWORD Attributes;
    LPSTR Path;

    /* Make sure the directory path is not too long */
    if (strlen(Directory) >= DOS_DIR_LENGTH)
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Get the drive number */
    DriveNumber = Directory[0] - 'A';

    /* Make sure the drive exists */
    if (DriveNumber > (LastDrive - 'A'))
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Get the file attributes */
    Attributes = GetFileAttributesA(Directory);

    /* Make sure the path exists and is a directory */
    if ((Attributes == INVALID_FILE_ATTRIBUTES)
        || !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Check if this is the current drive */
    if (DriveNumber == CurrentDrive)
    {
        /* Change the directory */
        if (!SetCurrentDirectoryA(Directory))
        {
            DosLastError = LOWORD(GetLastError());
            return FALSE;
        }
    }

    /* Get the directory part of the path */
    Path = strchr(Directory, '\\');
    if (Path != NULL)
    {
        /* Skip the backslash */
        Path++;
    }

    /* Set the directory for the drive */
    if (Path != NULL)
    {
        strncpy(CurrentDirectories[DriveNumber], Path, DOS_DIR_LENGTH);
    }
    else
    {
        CurrentDirectories[DriveNumber][0] = '\0';
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN DosControlBreak(VOID)
{
    setCF(0);

    /* Call interrupt 0x23 */
    Int32Call(&DosContext, 0x23);

    if (getCF())
    {
        DosTerminateProcess(CurrentPsp, 0, 0);
        return TRUE;
    }

    return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID WINAPI DosInt20h(LPWORD Stack)
{
    /* This is the exit interrupt */
    DosTerminateProcess(Stack[STACK_CS], 0, 0);
}

VOID WINAPI DosInt21h(LPWORD Stack)
{
    BYTE Character;
    SYSTEMTIME SystemTime;
    PCHAR String;
    PDOS_INPUT_BUFFER InputBuffer;
    PDOS_COUNTRY_CODE_BUFFER CountryCodeBuffer;
    INT Return;

    (*InDos)++;

    /* Save the value of SS:SP on entry in the PSP */
    SEGMENT_TO_PSP(CurrentPsp)->LastStack =
    MAKELONG(getSP() + (STACK_FLAGS + 1) * 2, getSS());

    /* Check the value in the AH register */
    switch (getAH())
    {
        /* Terminate Program */
        case 0x00:
        {
            DosTerminateProcess(Stack[STACK_CS], 0, 0);
            break;
        }

        /* Read Character from STDIN with Echo */
        case 0x01:
        {
            DPRINT("INT 21h, AH = 01h\n");

            // FIXME: Under DOS 2+, input / output handle may be redirected!!!!
            DoEcho = TRUE;
            Character = DosReadCharacter(DOS_INPUT_HANDLE);
            DoEcho = FALSE;

            // FIXME: Check whether Ctrl-C / Ctrl-Break is pressed, and call INT 23h if so.
            // Check also Ctrl-P and set echo-to-printer flag.
            // Ctrl-Z is not interpreted.

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
                    setAL(DosReadCharacter(DOS_INPUT_HANDLE));
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

        /* Character Input without Echo */
        case 0x07:
        case 0x08:
        {
            DPRINT("Char input without echo\n");

            Character = DosReadCharacter(DOS_INPUT_HANDLE);

            // FIXME: For 0x07, do not check Ctrl-C/Break.
            //        For 0x08, do check those control sequences and if needed,
            //        call INT 0x23.

            setAL(Character);
            break;
        }

        /* Write string to STDOUT */
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
            WORD Count = 0;
            InputBuffer = (PDOS_INPUT_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            DPRINT("Read Buffered Input\n");

            while (Count < InputBuffer->MaxLength)
            {
                /* Try to read a character (wait) */
                Character = DosReadCharacter(DOS_INPUT_HANDLE);

                switch (Character)
                {
                    /* Extended character */
                    case '\0':
                    {
                        /* Read the scancode */
                        DosReadCharacter(DOS_INPUT_HANDLE);
                        break;
                    }

                    /* Ctrl-C */
                    case 0x03:
                    {
                        DosPrintCharacter(DOS_OUTPUT_HANDLE, '^');
                        DosPrintCharacter(DOS_OUTPUT_HANDLE, 'C');

                        if (DosControlBreak())
                        {
                            /* Set the character to a newline to exit the loop */
                            Character = '\r';
                        }

                        break;
                    }

                    /* Backspace */
                    case '\b':
                    {
                        if (Count > 0)
                        {
                            Count--;

                            /* Erase the character */
                            DosPrintCharacter(DOS_OUTPUT_HANDLE, '\b');
                            DosPrintCharacter(DOS_OUTPUT_HANDLE, ' ');
                            DosPrintCharacter(DOS_OUTPUT_HANDLE, '\b');
                        }

                        break;
                    }

                    default:
                    {
                        /* Append it to the buffer */
                        InputBuffer->Buffer[Count] = Character;

                        /* Check if this is a special character */
                        if (Character < 0x20 && Character != 0x0A && Character != 0x0D)
                        {
                            DosPrintCharacter(DOS_OUTPUT_HANDLE, '^');
                            Character += 'A' - 1;
                        }

                        /* Echo the character */
                        DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);
                    }
                }

                if (Character == '\r') break;
                if (Character == '\b') continue;
                Count++; /* Carriage returns are NOT counted */
            }

            /* Update the length */
            InputBuffer->Length = Count;

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
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(CurrentPsp);

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
            setAL(LastDrive - 'A' + 1);
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
            setAL(CurrentDrive);
            break;
        }

        /* Set Disk Transfer Area */
        case 0x1A:
        {
            DiskTransferArea = MAKELONG(getDX(), getDS());
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
            DosClonePsp(getDX(), getCS());
            break;
        }

        /* Parse Filename into FCB */
        case 0x29:
        {
            PCHAR FileName = (PCHAR)SEG_OFF_TO_PTR(getDS(), getSI());
            PDOS_FCB Fcb = (PDOS_FCB)SEG_OFF_TO_PTR(getES(), getDI());
            BYTE Options = getAL();
            INT i;
            CHAR FillChar = ' ';

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
                if (Options & (1 << 1)) Fcb->DriveNumber = CurrentDrive + 1;
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
            setES(HIWORD(DiskTransferArea));
            setBX(LOWORD(DiskTransferArea));
            break;
        }

        /* Get DOS Version */
        case 0x30:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(CurrentPsp);

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
            DosTerminateProcess(CurrentPsp, getAL(), getDX());
            break;
        }

        /* Extended functionalities */
        case 0x33:
        {
            if (getAL() == 0x06)
            {
                /*
                 * DOS 5+ - GET TRUE VERSION NUMBER
                 * This function always returns the true version number, unlike
                 * AH=30h, whose return value may be changed with SETVER.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2730.htm
                 * for more information.
                 */

                /*
                 * Return the true DOS version: Minor:Major in BH:BL
                 * The Windows NT DOS box returns BX=3205h (version 5.50).
                 */
                setBX(NTDOS_VERSION);

                /* DOS revision 0 */
                setDL(0x00);

                /* Unpatched DOS */
                setDH(0x00);
            }
            // else
            // {
                // /* Invalid subfunction */
                // setAL(0xFF);
            // }

            break;
        }

        /* Get Address of InDOS flag */
        case 0x34:
        {
            setES(HIWORD(INDOS_POINTER));
            setBX(LOWORD(INDOS_POINTER));
            break;
        }

        /* Get Interrupt Vector */
        case 0x35:
        {
            DWORD FarPointer = ((PDWORD)BaseAddress)[getAL()];

            /* Read the address from the IDT into ES:BX */
            setES(HIWORD(FarPointer));
            setBX(LOWORD(FarPointer));
            break;
        }

        /* Get Free Disk Space */
        case 0x36:
        {
            CHAR RootPath[3] = "X:\\";
            DWORD SectorsPerCluster;
            DWORD BytesPerSector;
            DWORD NumberOfFreeClusters;
            DWORD TotalNumberOfClusters;

            if (getDL() == 0) RootPath[0] = 'A' + CurrentDrive;
            else RootPath[0] = 'A' + getDL() - 1;

            if (GetDiskFreeSpaceA(RootPath,
                                  &SectorsPerCluster,
                                  &BytesPerSector,
                                  &NumberOfFreeClusters,
                                  &TotalNumberOfClusters))
            {
                setAX(LOWORD(SectorsPerCluster));
                setCX(LOWORD(BytesPerSector));
                setBX(LOWORD(NumberOfFreeClusters));
                setDX(LOWORD(TotalNumberOfClusters));
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
            if (getAL() == 0x00)
            {
                /*
                 * DOS 2+ - "SWITCHAR" - GET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 4.0+.
                 * MS-DOS 5+ always return AL=00h/DL=2Fh.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2752.htm
                 * for more information.
                 */
                setDL('/');
                setAL(0x00);
            }
            else if (getAL() == 0x01)
            {
                /*
                 * DOS 2+ - "SWITCHAR" - SET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 5+.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2753.htm
                 * for more information.
                 */
                // getDL();
                setAL(0xFF);
            }
            else if (getAL() == 0x02)
            {
                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                // setDL();
                setAL(0xFF);
            }
            else if (getAL() == 0x03)
            {
                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                // getDL();
                setAL(0xFF);
            }
            else
            {
                /* Invalid subfunction */
                setAL(0xFF);
            }

            break;
        }

        /* Get/Set Country-dependent Information */
        case 0x38:
        {
            CountryCodeBuffer = (PDOS_COUNTRY_CODE_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            if (getAL() == 0x00)
            {
                /* Get */
                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE,
                                       &CountryCodeBuffer->TimeFormat,
                                       sizeof(CountryCodeBuffer->TimeFormat) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY,
                                       &CountryCodeBuffer->CurrencySymbol,
                                       sizeof(CountryCodeBuffer->CurrencySymbol) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                                       &CountryCodeBuffer->ThousandSep,
                                       sizeof(CountryCodeBuffer->ThousandSep) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                                       &CountryCodeBuffer->DecimalSep,
                                       sizeof(CountryCodeBuffer->DecimalSep) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;
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
                setAX(DosLastError);
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
            LPCSTR FileName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            WORD ErrorCode = DosOpenFile(&FileHandle, FileName, getAL());

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

            DoEcho = TRUE;
            ErrorCode = DosReadFile(getBX(),
                                    MAKELONG(getDX(), getDS()),
                                    getCX(),
                                    &BytesRead);
            DoEcho = FALSE;

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
                setAL(FileName[0] - 'A');
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
                setAX(DosLastError);
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
                setAX(DosLastError);
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
                DriveNumber = CurrentDrive;
            }
            else
            {
                /* Decrement DriveNumber since it was 1-based */
                DriveNumber--;
            }

            if (DriveNumber <= LastDrive - 'A')
            {
                /*
                 * Copy the current directory into the target buffer.
                 * It doesn't contain the drive letter and the backslash.
                 */
                strncpy(String, CurrentDirectories[DriveNumber], DOS_DIR_LENGTH);
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
                setAX(DosLastError);
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
                setAX(ERROR_ARENA_TRASHED);
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
                setAX(DosLastError);
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
            DWORD ReturnAddress = MAKELONG(Stack[STACK_IP], Stack[STACK_CS]);
            WORD ErrorCode;

            if (OrgAL <= DOS_LOAD_OVERLAY)
            {
                DOS_EXEC_TYPE LoadType = (DOS_EXEC_TYPE)OrgAL;

#ifndef STANDALONE
                if (LoadType == DOS_LOAD_AND_EXECUTE)
                {
                    /* Create a new process */
                    ErrorCode = DosCreateProcess(ProgramName,
                                                 ParamBlock,
                                                 ReturnAddress);
                }
                else
#endif
                {
                    /* Just load an executable */
                    ErrorCode = DosLoadExecutable(LoadType,
                                                  ProgramName,
                                                  ParamBlock,
                                                  NULL,
                                                  NULL,
                                                  ReturnAddress);
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

        /* Terminate With Return Code */
        case 0x4C:
        {
            DosTerminateProcess(CurrentPsp, getAL(), 0);
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
            setAX(DosErrorLevel);
            DosErrorLevel = 0x0000; // Clear it
            break;
        }

        /* Find First File */
        case 0x4E:
        {
            WORD Result = (WORD)demFileFindFirst(FAR_POINTER(DiskTransferArea),
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
            WORD Result = (WORD)demFileFindNext(FAR_POINTER(DiskTransferArea));

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
            setBX(CurrentPsp);
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
            setBX(FIELD_OFFSET(DOS_SYSVARS, FirstDpb));

            break;
        }

        /* Create Child PSP */
        case 0x55:
        {
            DosCreatePsp(getDX(), getSI());
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

        /* Get/Set Memory Management Options */
        case 0x58:
        {
            if (getAL() == 0x00)
            {
                /* Get allocation strategy */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(DosAllocStrategy);
            }
            else if (getAL() == 0x01)
            {
                /* Set allocation strategy */

                if ((getBL() & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                    == (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                {
                    /* Can't set both */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(ERROR_INVALID_PARAMETER);
                    break;
                }

                if ((getBL() & 0x3F) > DOS_ALLOC_LAST_FIT)
                {
                    /* Invalid allocation strategy */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(ERROR_INVALID_PARAMETER);
                    break;
                }

                DosAllocStrategy = getBL();
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (getAL() == 0x02)
            {
                /* Get UMB link state */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAL(DosUmbLinked ? 0x01 : 0x00);
            }
            else if (getAL() == 0x03)
            {
                /* Set UMB link state */
                if (getBX()) DosLinkUmb();
                else DosUnlinkUmb();
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                /* Invalid or unsupported function */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
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
            // It should be a ASCIZ path ending with a '\' + 13 zero bytes
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
                    setAX(DosLastError);
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
                    setAX(DosLastError);
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

        /* Set Handle Count */
        case 0x67:
        {
            if (!DosResizeHandleTable(getBX()))
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
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

        /* Unsupported */
        default:
        {
            DPRINT1("DOS Function INT 0x21, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());

            setAL(0); // Some functions expect AL to be 0 when it's not supported.
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }

    (*InDos)--;
}

VOID WINAPI DosBreakInterrupt(LPWORD Stack)
{
    /* Set CF to terminate the running process */
    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
}

VOID WINAPI DosInt27h(LPWORD Stack)
{
    DosTerminateProcess(getCS(), 0, (getDX() + 0x0F) >> 4);
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

VOID WINAPI DosInt2Fh(LPWORD Stack)
{
    switch (getAH())
    {
        /* Extended Memory Specification */
        case 0x43:
        {
            DWORD DriverEntry;
            if (!XmsGetDriverEntry(&DriverEntry)) break;

            if (getAL() == 0x00)
            {
                /* The driver is loaded */
                setAL(0x80);
            }
            else if (getAL() == 0x10)
            {
                setES(HIWORD(DriverEntry));
                setBX(LOWORD(DriverEntry));
            }
            else
            {
                DPRINT1("Unknown DOS XMS Function: INT 0x2F, AH = 43h, AL = %xh\n", getAL());
            }

            break;
        }
        
        default:
        {
            DPRINT1("DOS Internal System Function INT 0x2F, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

BOOLEAN DosKRNLInitialize(VOID)
{
#if 1

    UCHAR i;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];
    LPSTR Path;
    PDOS_SFT Sft;

    const BYTE NullDriverRoutine[] = {
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

    FILE *Stream;
    WCHAR Buffer[256];

    /* Setup the InDOS flag */
    InDos = (PBYTE)FAR_POINTER(INDOS_POINTER);
    *InDos = 0;

    /* Clear the current directory buffer */
    RtlZeroMemory(CurrentDirectories, sizeof(CurrentDirectories));

    /* Get the current directory */
    if (!GetCurrentDirectoryA(MAX_PATH, CurrentDirectory))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Convert that to a DOS path */
    if (!GetShortPathNameA(CurrentDirectory, DosDirectory, DOS_DIR_LENGTH))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Set the drive */
    CurrentDrive = DosDirectory[0] - 'A';

    /* Get the directory part of the path */
    Path = strchr(DosDirectory, '\\');
    if (Path != NULL)
    {
        /* Skip the backslash */
        Path++;
    }

    /* Set the directory */
    if (Path != NULL)
    {
        strncpy(CurrentDirectories[CurrentDrive], Path, DOS_DIR_LENGTH);
    }

    /* Read CONFIG.SYS */
    Stream = _wfopen(DOS_CONFIG_PATH, L"r");
    if (Stream != NULL)
    {
        while (fgetws(Buffer, 256, Stream))
        {
            // TODO: Parse the line
        }
        fclose(Stream);
    }

    /* Initialize the list of lists */
    SysVars = (PDOS_SYSVARS)SEG_OFF_TO_PTR(DOS_DATA_SEGMENT, 0);
    RtlZeroMemory(SysVars, sizeof(DOS_SYSVARS));
    SysVars->FirstMcb = FIRST_MCB_SEGMENT;
    SysVars->FirstSft = MAKELONG(MASTER_SFT_OFFSET, DOS_DATA_SEGMENT);

    /* Initialize the NUL device driver */
    SysVars->NullDevice.Link = 0xFFFFFFFF;
    SysVars->NullDevice.DeviceAttributes = DOS_DEVATTR_NUL | DOS_DEVATTR_CHARACTER;
    SysVars->NullDevice.StrategyRoutine = FIELD_OFFSET(DOS_SYSVARS, NullDriverRoutine);
    SysVars->NullDevice.InterruptRoutine = SysVars->NullDevice.StrategyRoutine + 6;
    RtlFillMemory(SysVars->NullDevice.DeviceName,
                  sizeof(SysVars->NullDevice.DeviceName),
                  ' ');
    RtlCopyMemory(SysVars->NullDevice.DeviceName, "NUL", strlen("NUL"));
    RtlCopyMemory(SysVars->NullDriverRoutine,
                  NullDriverRoutine,
                  sizeof(NullDriverRoutine));

    /* Initialize the SFT */
    Sft = (PDOS_SFT)FAR_POINTER(SysVars->FirstSft);
    Sft->Link = 0xFFFFFFFF;
    Sft->NumDescriptors = DOS_SFT_SIZE;

    for (i = 0; i < Sft->NumDescriptors; i++)
    {
        /* Clear the file descriptor entry */
        RtlZeroMemory(&Sft->FileDescriptors[i], sizeof(DOS_FILE_DESCRIPTOR));
    }

#endif

    /* Initialize the callback context */
    InitializeContext(&DosContext, 0x0070, 0x0000);

    /* Register the DOS 32-bit Interrupts */
    RegisterDosInt32(0x20, DosInt20h        );
    RegisterDosInt32(0x21, DosInt21h        );
//  RegisterDosInt32(0x22, DosInt22h        ); // Termination
    RegisterDosInt32(0x23, DosBreakInterrupt); // Ctrl-C / Ctrl-Break
//  RegisterDosInt32(0x24, DosInt24h        ); // Critical Error
    RegisterDosInt32(0x27, DosInt27h        ); // Terminate and Stay Resident
    RegisterDosInt32(0x29, DosFastConOut    ); // DOS 2+ Fast Console Output
    RegisterDosInt32(0x2F, DosInt2Fh        );

    /* Load the CON driver */
    ConDrvInitialize();

    /* Load the XMS driver (HIMEM) */
    XmsInitialize();

    /* Load the EMS driver */
    if (!EmsDrvInitialize(EMS_TOTAL_PAGES))
    {
        DPRINT1("Could not initialize EMS. EMS will not be available.\n"
                "Try reducing the number of EMS pages.\n");
    }

    return TRUE;
}

/* EOF */
