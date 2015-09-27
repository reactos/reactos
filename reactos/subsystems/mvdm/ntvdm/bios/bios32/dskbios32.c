/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dskbios32.c
 * PURPOSE:         VDM 32-bit Disk BIOS
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
// #include "../../memory.h"
// #include "cpu/bop.h"
#include "cpu/cpu.h" // for EMULATOR_FLAG_ZF
#include "int32.h"

#include "dskbios32.h"
// #include <bios/dskbios.h>
#include "bios32p.h"

#include "hardware/disk.h"


/* DEFINES ********************************************************************/

// Disks which are currently supported by the BIOS Disk module.
// NOTE: For the current implementation those are arrays of pointers to
// DISK_IMAGEs maintained by the Generic Disk Controller. In the future
// they will be arrays of objects containing disk information needed by
// the BIOS only.
static PDISK_IMAGE FloppyDrive[2] = {NULL};
static PDISK_IMAGE HardDrive[1]   = {NULL};

#pragma pack(push, 1)

// See: http://www.ctyme.com/intr/rb-2445.htm
typedef struct _FLOPPY_PARAM_TABLE
{
    BYTE Unused0;
    BYTE Unused1;
    BYTE MotorOffDelay;
    BYTE SectorSize;
    BYTE SectorsPerTrack;
    BYTE SectorGapLength;
    BYTE DataLength;
    BYTE FormatGapLength;
    BYTE FormatFillByte;
    BYTE HeadSettleTime;
    BYTE MotorStartTime;
} FLOPPY_PARAM_TABLE, *PFLOPPY_PARAM_TABLE;

typedef struct _FLOPPY_PARAM_TABLE_EX
{
    FLOPPY_PARAM_TABLE FloppyParamTable;

    // IBM Additions
    BYTE MaxTrackNumber;
    BYTE DataTransferRate;
    BYTE CmosDriveType;
} FLOPPY_PARAM_TABLE_EX, *PFLOPPY_PARAM_TABLE_EX;

#pragma pack(pop)

// Parameters for 1.44 MB Floppy, taken from SeaBIOS

#define FLOPPY_SIZE_CODE        0x02    // 512 byte sectors
#define FLOPPY_DATALEN          0xFF    // Not used - because size code is 0x02
#define FLOPPY_MOTOR_TICKS      37      // ~2 seconds
#define FLOPPY_FILLBYTE         0xF6
#define FLOPPY_GAPLEN           0x1B
#define FLOPPY_FORMAT_GAPLEN    0x6C

static const FLOPPY_PARAM_TABLE_EX FloppyParamTable =
{
    // PC-AT compatible table
    {
        0xAF, // step rate 12ms, head unload 240ms
        0x02, // head load time 4ms, DMA used
        FLOPPY_MOTOR_TICKS, // ~2 seconds
        FLOPPY_SIZE_CODE,
        18,
        FLOPPY_GAPLEN,
        FLOPPY_DATALEN,
        FLOPPY_FORMAT_GAPLEN,
        FLOPPY_FILLBYTE,
        0x0F, // 15ms
        0x08, // 1 second
    },

    // IBM Additions
    79,   // maximum track
    0,    // data transfer rate
    4,    // drive type in CMOS
};


#pragma pack(push, 1)

// See: http://www.ctyme.com/intr/rb-6135.htm
typedef struct _HARDDISK_PARAM_TABLE
{
    WORD Cylinders;
    BYTE Heads;
    WORD Unused0;
    WORD Unused1;
    BYTE Unused2;
    BYTE Control;
    BYTE StandardTimeout;
    BYTE FormatTimeout;
    BYTE CheckingTimeout;
    WORD LandZoneCylinder;
    BYTE SectorsPerTrack;
    BYTE Reserved;
} HARDDISK_PARAM_TABLE, *PHARDDISK_PARAM_TABLE;

#pragma pack(pop)

// static const HARDDISK_PARAM_TABLE HardDiskParamTable =
// {0};


/* PRIVATE FUNCTIONS **********************************************************/

static PDISK_IMAGE
GetDisk(IN BYTE DiskNumber)
{
    if (DiskNumber & 0x80)
    {
        DiskNumber &= ~0x80;

        if (DiskNumber >= ARRAYSIZE(HardDrive))
        {
            DPRINT1("GetDisk: HDD number 0x%02X invalid\n", DiskNumber | 0x80);
            return NULL;
        }

        return HardDrive[DiskNumber];
    }
    else
    {
        if (DiskNumber >= ARRAYSIZE(FloppyDrive))
        {
            DPRINT1("GetDisk: Floppy number 0x%02X invalid\n", DiskNumber);
            return NULL;
        }

        return FloppyDrive[DiskNumber];
    }
}

static VOID
AllDisksReset(VOID)
{
    Bda->LastDisketteOperation = 0;
    Bda->LastDiskOperation = 0;
}

/*static*/
VOID WINAPI BiosDiskService(LPWORD Stack)
{
    BYTE Drive;
    PDISK_IMAGE DiskImage;

    switch (getAH())
    {
        /* Disk -- Reset Disk System */
        case 0x00:
        {
            Drive = getDL();

            if (Drive & 0x80)
            {
                AllDisksReset();

                /* Return success */
                setAH(0x00);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                break;
            }

            Drive &= ~0x80;

            if (Drive >= ARRAYSIZE(FloppyDrive))
            {
                DPRINT1("BiosDiskService(0x00): Drive number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            // TODO: Reset drive

            /* Return success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Disk -- Get Status of Last Operation */
        case 0x01:
        {
            BYTE LastOperationStatus = 0x00;

            Drive = getDL();
            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x01): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            LastOperationStatus = DiskImage->LastOperationStatus;

            if (Drive & 0x80)
                Bda->LastDiskOperation = LastOperationStatus;
            else
                Bda->LastDisketteOperation = LastOperationStatus;

            /* Return last error */
            setAH(LastOperationStatus);
            if (LastOperationStatus == 0x00)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Disk -- Read Sectors into Memory */
        case 0x02:
        {
            BYTE Status;
            BYTE Head = getDH();
            BYTE NumSectors = getAL();

            // CH: Low eight bits of cylinder number
            // CL: High two bits of cylinder (bits 6-7, hard disk only)
            WORD Cylinder = MAKEWORD(getCH(), (getCL() >> 6) & 0x02);

            // CL: Sector number 1-63 (bits 0-5)
            BYTE Sector = (getCL() & 0x3F); // 1-based

            Drive = getDL();
            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x02): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Read the sectors */
            Status = ReadDisk(DiskImage, Cylinder, Head, Sector, NumSectors);
            if (Status == 0x00)
            {
                /* Return success */
                setAH(0x00);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                DPRINT1("BiosDiskService(0x02): Error when reading from disk number 0x%02X (0x%02X)\n", Drive, Status);

                /* Return error */
                setAH(Status);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Disk -- Write Disk Sectors */
        case 0x03:
        {
            BYTE Status;
            BYTE Head = getDH();
            BYTE NumSectors = getAL();

            // CH: Low eight bits of cylinder number
            // CL: High two bits of cylinder (bits 6-7, hard disk only)
            WORD Cylinder = MAKEWORD(getCH(), (getCL() >> 6) & 0x02);

            // CL: Sector number 1-63 (bits 0-5)
            BYTE Sector = (getCL() & 0x3F); // 1-based

            Drive = getDL();
            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x03): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Write the sectors */
            Status = WriteDisk(DiskImage, Cylinder, Head, Sector, NumSectors);
            if (Status == 0x00)
            {
                /* Return success */
                setAH(0x00);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                DPRINT1("BiosDiskService(0x03): Error when writing to disk number 0x%02X (0x%02X)\n", Drive, Status);

                /* Return error */
                setAH(Status);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Disk -- Verify Disk Sectors */
        case 0x04:

        /* Floppy/Fixed Disk -- Format Track */
        case 0x05:

        /* Fixed Disk -- Format Track and Set Bad Sector Flags */
        case 0x06:

        /* Fixed Disk -- Format Drive starting at Given Track */
        case 0x07:
            goto Default;

        /* Disk -- Get Drive Parameters */
        case 0x08:
        {
            WORD MaxCylinders;
            BYTE MaxHeads;
            BYTE PresentDrives = 0;
            BYTE i;

            Drive = getDL();
            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x08): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            // Minus 2 because it's the maximum cylinder number (not count),
            // and the last cylinder is reserved (for compatibility with BIOSes
            // which reserve it for testing purposes).
            MaxCylinders = DiskImage->DiskInfo.Cylinders - 2;
            // Minus 1 because it's the maximum head number (not count).
            MaxHeads     = DiskImage->DiskInfo.Heads - 1;

            // CL: Sector number 1-63 (bits 0-5)
            //     High two bits of cylinder (bits 6-7, hard disk only)
            setCL((DiskImage->DiskInfo.Sectors & 0x3F) |
                  ((HIBYTE(MaxCylinders) & 0x02) << 6));
            // CH: Low eight bits of cylinder number
            setCH(LOBYTE(MaxCylinders));

            setDH(MaxHeads);

            if (Drive & 0x80)
            {
                /* Count the number of active HDDs */
                for (i = 0; i < ARRAYSIZE(HardDrive); ++i)
                {
                    if (IsDiskPresent(HardDrive[i]))
                        ++PresentDrives;
                }

                /* Reset ES:DI to NULL */
                // FIXME: NONONO!! Apps expect (for example, MS-DOS kernel)
                // that this function does not modify ES:DI if it was called
                // for a HDD.
                // setES(0x0000);
                // setDI(0x0000);
            }
            else
            {
                /* Count the number of active floppies */
                for (i = 0; i < ARRAYSIZE(FloppyDrive); ++i)
                {
                    if (IsDiskPresent(FloppyDrive[i]))
                        ++PresentDrives;
                }

                /* ES:DI points to the floppy parameter table */
                setES(HIWORD(((PULONG)BaseAddress)[0x1E]));
                setDI(LOWORD(((PULONG)BaseAddress)[0x1E]));
            }
            setDL(PresentDrives);

            setBL(DiskImage->DiskType); // DiskGeometryList[DiskImage->DiskType].biosval
            setAL(0x00);

            /* Return success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Hard Disk -- Initialize Controller with Drive Parameters */
        case 0x09:

        /* Hard Disk -- Read Long Sectors */
        case 0x0A:

        /* Hard Disk -- Write Long Sectors */
        case 0x0B:
            goto Default;

        /* Hard Disk -- Seek to Cylinder */
        case 0x0C:
        {
            BYTE Status;
            BYTE Head = getDH();

            // CH: Low eight bits of cylinder number
            // CL: High two bits of cylinder (bits 6-7, hard disk only)
            WORD Cylinder = MAKEWORD(getCH(), (getCL() >> 6) & 0x02);

            // CL: Sector number 1-63 (bits 0-5)
            BYTE Sector = (getCL() & 0x3F); // 1-based

            Drive = getDL();
            if (!(Drive & 0x80))
            {
                DPRINT1("BiosDiskService(0x0C): Disk number 0x%02X is not a HDD\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x0C): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Set position */
            Status = SeekDisk(DiskImage, Cylinder, Head, Sector);
            if (Status == 0x00)
            {
                /* Return success */
                setAH(0x00);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                DPRINT1("BiosDiskService(0x0C): Error when seeking in disk number 0x%02X (0x%02X)\n", Drive, Status);

                /* Return error */
                setAH(Status);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Hard Disk -- Reset Hard Disks */
        case 0x0D:
        {
            // FIXME: Should do what 0x11 does.
            UNIMPLEMENTED;
        }

        /* Hard Disk -- Read Sector Buffer (XT only) */
        case 0x0E:

        /* Hard Disk -- Write Sector Buffer (XT only) */
        case 0x0F:
            goto Default;

        /* Hard Disk -- Check if Drive is ready */
        case 0x10:
        {
            Drive = getDL();
            if (!(Drive & 0x80))
            {
                DPRINT1("BiosDiskService(0x10): Disk number 0x%02X is not a HDD\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x10): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Return success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Hard Disk -- Recalibrate Drive */
        case 0x11:
        {
            BYTE Status;

            Drive = getDL();
            if (!(Drive & 0x80))
            {
                DPRINT1("BiosDiskService(0x11): Disk number 0x%02X is not a HDD\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x11): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Set position to zero */
            Status = SeekDisk(DiskImage, /*Cylinder*/ 0, /*Head*/ 0, /*Sector*/ 1);
            if (Status == 0x00)
            {
                /* Return success */
                setAH(0x00);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                DPRINT1("BiosDiskService(0x11): Error when recalibrating disk number 0x%02X (0x%02X)\n", Drive, Status);

                /* Return error */
                setAH(Status);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Hard Disk -- Controller RAM Diagnostic */
        case 0x12:

        /* Hard Disk -- Drive Diagnostic */
        case 0x13:

        /* Hard Disk -- Controller Internal Diagnostic */
        case 0x14:
            goto Default;

        /* Disk -- Get Disk Type */
        case 0x15:
        {
            Drive = getDL();
            DiskImage = GetDisk(Drive);
            if (!DiskImage || !IsDiskPresent(DiskImage))
            {
                DPRINT1("BiosDiskService(0x15): Disk number 0x%02X invalid\n", Drive);

                /* Return error */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            if (Drive & 0x80)
            {
                ULONG NumSectors;

                /* Hard disk */
                setAH(0x03);

                /* Number of 512-byte sectors in CX:DX */
                NumSectors = DiskImage->DiskInfo.Cylinders * DiskImage->DiskInfo.Heads
                                                           * DiskImage->DiskInfo.Sectors;
                setCX(HIWORD(NumSectors));
                setDX(LOWORD(NumSectors));
            }
            else
            {
                /* Floppy */
                setAH(0x01);
            }

            /* Return success */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Floppy Disk -- Detect Disk Change */
        case 0x16:

        /* Floppy Disk -- Set Disk Type for Format */
        case 0x17:

        /* Disk -- Set Media Type for Format */
        case 0x18:
            goto Default;

        default: Default:
        {
            DPRINT1("BIOS Function INT 13h, AH = 0x%02X, AL = 0x%02X, BH = 0x%02X NOT IMPLEMENTED\n",
                    getAH(), getAL(), getBH());
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DiskBios32Post(VOID)
{
    /*
     * Initialize BIOS Disk RAM dynamic data
     */

    /* Some vectors are in fact addresses to tables */
    // Diskette Parameters
    ((PULONG)BaseAddress)[0x1E] = MAKELONG(0xEFC7, BIOS_SEGMENT);
    // Hard Disk 0 Parameter Table Address
    ((PULONG)BaseAddress)[0x41] = (ULONG)NULL;
    // Hard Disk 1 Drive Parameter Table Address
    ((PULONG)BaseAddress)[0x46] = (ULONG)NULL;

    /* Relocated services by the BIOS (when needed) */
    ((PULONG)BaseAddress)[0x40] = (ULONG)NULL; // ROM BIOS Diskette Handler relocated by Hard Disk BIOS
    // RegisterBiosInt32(0x40, NULL); // ROM BIOS Diskette Handler relocated by Hard Disk BIOS

    /* Register the BIOS 32-bit Interrupts */
    RegisterBiosInt32(BIOS_DISK_INTERRUPT, BiosDiskService);

    /* Initialize the BDA */
    // Bda->LastDisketteOperation = 0;
    // Bda->LastDiskOperation = 0;
    AllDisksReset();
}

BOOLEAN DiskBios32Initialize(VOID)
{
    /*
     * Initialize BIOS Disk ROM static data
     */

    /* Floppy Parameter Table */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xEFC7),
                  &FloppyParamTable.FloppyParamTable,
                  sizeof(FloppyParamTable.FloppyParamTable));

    //
    // FIXME: Must be done by HW floppy controller!
    //

    /* Detect and initialize the supported disks */
    // TODO: the "Detect" part is missing.
    FloppyDrive[0] = RetrieveDisk(FLOPPY_DISK, 0);
    FloppyDrive[1] = RetrieveDisk(FLOPPY_DISK, 1);
    HardDrive[0]   = RetrieveDisk(HARD_DISK, 0);

    return TRUE;
}

VOID DiskBios32Cleanup(VOID)
{
}

/* EOF */
