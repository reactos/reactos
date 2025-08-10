/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AMD64 machine initialization
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <disk.h>
#include <arch/pc/pcbios.h>

/* Minimal implementations for AMD64 */
static VOID Amd64ConsPutChar(int Ch)
{
    /* Output to serial port */
    WRITE_PORT_UCHAR((PUCHAR)0x3F8, (UCHAR)Ch);
}

static BOOLEAN Amd64ConsKbHit(VOID)
{
    return FALSE;
}

static int Amd64ConsGetCh(VOID)
{
    return -1;
}

static VOID Amd64VideoClearScreen(UCHAR Attr)
{
    /* Stub */
}

static VIDEODISPLAYMODE Amd64VideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
    return VideoTextMode;
}

static VOID Amd64VideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    *Width = 80;
    *Height = 25;
    *Depth = 16;
}

static ULONG Amd64VideoGetBufferSize(VOID)
{
    return 80 * 25 * 2;
}

static VOID Amd64VideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    /* Stub */
}

static VOID Amd64VideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* Stub */
}

static VOID Amd64VideoHideShowTextCursor(BOOLEAN Show)
{
    /* Stub */
}

static VOID Amd64VideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    /* Stub - just output to serial */
    Amd64ConsPutChar(Ch);
}

static VOID Amd64VideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    /* Stub */
}

static BOOLEAN Amd64VideoIsPaletteFixed(VOID)
{
    return TRUE;
}

static VOID Amd64VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
    /* Stub */
}

static VOID Amd64VideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue)
{
    *Red = *Green = *Blue = 0;
}

static VOID Amd64VideoSync(VOID)
{
    /* Stub */
}

static VOID Amd64Beep(VOID)
{
    /* Stub */
}

static VOID Amd64PrepareForReactOS(VOID)
{
    /* Stub */
}

static FREELDR_MEMORY_DESCRIPTOR MemoryMap[32];
static ULONG MemoryMapCount = 0;

static FREELDR_MEMORY_DESCRIPTOR* Amd64GetMemoryMap(PULONG MaxMemoryMapSize)
{
    /* Return a minimal memory map */
    if (MemoryMapCount == 0)
    {
        /* First 1MB is reserved */
        MemoryMap[0].BasePage = 0;
        MemoryMap[0].PageCount = 256;
        MemoryMap[0].MemoryType = LoaderFirmwarePermanent;
        
        /* 1MB to 16MB is free */
        MemoryMap[1].BasePage = 256;
        MemoryMap[1].PageCount = 3840;
        MemoryMap[1].MemoryType = LoaderFree;
        
        /* 16MB to 512MB is free */
        MemoryMap[2].BasePage = 4096;
        MemoryMap[2].PageCount = 126976;
        MemoryMap[2].MemoryType = LoaderFree;
        
        MemoryMapCount = 3;
    }
    
    *MaxMemoryMapSize = MemoryMapCount;
    return MemoryMap;
}

static VOID Amd64GetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    *ExtendedBIOSDataArea = 0;
    *ExtendedBIOSDataSize = 0;
}

static UCHAR Amd64GetFloppyCount(VOID)
{
    return 0;
}

static BOOLEAN Amd64DiskReadLogicalSectors(IN UCHAR DriveNumber,
                                           IN ULONGLONG SectorNumber,
                                           IN ULONG SectorCount,
                                           OUT PVOID Buffer)
{
/* Enable disk reading for testing */
#define USE_INT386_DISK_READ 1
#ifdef USE_INT386_DISK_READ
    /* This code causes a reboot due to mode switching issues */
    /* Disabled until we fix the CallRealMode implementation */
    REGS RegsIn, RegsOut;
    ULONG RetryCount;
    ULONG BytesPerSector = 2048; /* CD-ROM sectors are 2048 bytes */
    PVOID Int13Buffer = (PVOID)BIOSCALLBUFFER; /* Buffer in low memory */
    
    /* For CD-ROM, we use INT 13h extended read */
    typedef struct
    {
        UCHAR PacketSize;
        UCHAR Reserved;
        USHORT BlockCount;
        USHORT BufferOffset;
        USHORT BufferSegment;
        ULONGLONG LBA;
    } DISK_PACKET;
    
    DISK_PACKET* Packet = (DISK_PACKET*)BIOSCALLBUFFER;
    
    /* Can only read to low memory buffer, then copy */
    ULONG MaxSectorsPerRead = (0x10000 - sizeof(DISK_PACKET)) / BytesPerSector;
    if (MaxSectorsPerRead > 32) MaxSectorsPerRead = 32; /* Limit to 32 sectors */
    
    while (SectorCount > 0)
    {
        ULONG SectorsToRead = (SectorCount > MaxSectorsPerRead) ? MaxSectorsPerRead : SectorCount;
        
        /* Setup disk packet */
        RtlZeroMemory(Packet, sizeof(DISK_PACKET));
        Packet->PacketSize = sizeof(DISK_PACKET);
        Packet->BlockCount = (USHORT)SectorsToRead;
        Packet->BufferOffset = sizeof(DISK_PACKET);
        Packet->BufferSegment = BIOSCALLBUFSEGMENT;
        Packet->LBA = SectorNumber;
        
        /* Setup registers for INT 13h AH=42h (Extended Read) */
        RtlZeroMemory(&RegsIn, sizeof(RegsIn));
        RtlZeroMemory(&RegsOut, sizeof(RegsOut));
        RegsIn.b.ah = 0x42; /* Extended read */
        RegsIn.b.dl = DriveNumber;
        RegsIn.w.si = 0; /* Offset of packet */
        RegsIn.w.ds = BIOSCALLBUFSEGMENT; /* Segment of packet */
        
        /* Try up to 3 times */
        for (RetryCount = 0; RetryCount < 3; RetryCount++)
        {
            Int386(0x13, &RegsIn, &RegsOut);
            
            /* Check if successful */
            if ((RegsOut.x.eflags & EFLAGS_CF) == 0)
            {
                /* Copy from low memory buffer to destination */
                ULONG BytesToCopy = SectorsToRead * BytesPerSector;
                RtlCopyMemory(Buffer, (PVOID)(BIOSCALLBUFFER + sizeof(DISK_PACKET)), BytesToCopy);
                
                /* Update for next iteration */
                Buffer = (PVOID)((ULONG_PTR)Buffer + BytesToCopy);
                SectorNumber += SectorsToRead;
                SectorCount -= SectorsToRead;
                break;
            }
            
            /* Reset disk controller before retry */
            if (RetryCount < 2)
            {
                RtlZeroMemory(&RegsIn, sizeof(RegsIn));
                RtlZeroMemory(&RegsOut, sizeof(RegsOut));
                RegsIn.b.ah = 0x00; /* Reset disk */
                RegsIn.b.dl = DriveNumber;
                Int386(0x13, &RegsIn, &RegsOut);
            }
        }
        
        /* If all retries failed, return failure */
        if (RetryCount >= 3)
        {
            return FALSE;
        }
    }
    
    return TRUE;
#else
    /* For now, return failure to avoid the reboot issue */
    return FALSE;
#endif
}

static BOOLEAN Amd64DiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    return FALSE;
}

static ULONG Amd64DiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    return 0;
}

static PCONFIGURATION_COMPONENT_DATA Amd64HwDetect(const CHAR* Options)
{
    /* Return minimal hardware tree */
    return NULL;
}

static VOID Amd64HwIdle(VOID)
{
    /* Stub */
}

/* Simple disk reading for ISO CD-ROM in long mode */
static ARC_STATUS Amd64DiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    /* Just return success for now */
    *FileId = 1;
    return ESUCCESS;
}

static ARC_STATUS Amd64DiskClose(ULONG FileId)
{
    return ESUCCESS;
}

static ARC_STATUS Amd64DiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    return ESUCCESS;
}

/* Global state for disk reading */
static ULONGLONG CurrentDiskPosition = 0;

static ARC_STATUS Amd64DiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    extern UCHAR FrldrBootDrive;
    ULONG SectorSize = 2048; /* CD-ROM sector size */
    ULONGLONG StartSector;
    ULONG SectorCount;
    ULONG BytesToRead;
    
    /* Calculate sectors to read */
    StartSector = CurrentDiskPosition / SectorSize;
    BytesToRead = N;
    SectorCount = (BytesToRead + SectorSize - 1) / SectorSize;
    
    /* Try to read the sectors */
    if (Amd64DiskReadLogicalSectors(FrldrBootDrive, StartSector, SectorCount, Buffer))
    {
        *Count = BytesToRead;
        CurrentDiskPosition += BytesToRead;
        return ESUCCESS;
    }
    
    *Count = 0;
    return EIO;
}

static ARC_STATUS Amd64DiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    switch (SeekMode)
    {
        case SeekAbsolute:
            CurrentDiskPosition = Position->QuadPart;
            break;
        case SeekRelative:
            CurrentDiskPosition += Position->QuadPart;
            break;
        default:
            return EINVAL;
    }
    
    Position->QuadPart = CurrentDiskPosition;
    return ESUCCESS;
}

static const DEVVTBL Amd64DiskVtbl =
{
    Amd64DiskClose,
    Amd64DiskGetFileInformation,
    Amd64DiskOpen,
    Amd64DiskRead,
    Amd64DiskSeek,
};

static BOOLEAN Amd64InitializeBootDevices(VOID)
{
    extern UCHAR FrldrBootDrive;
    extern ULONG FrldrBootPartition;
    extern CCHAR FrLdrBootPath[MAX_PATH];
    extern VOID FsRegisterDevice(PCSTR DeviceName, const DEVVTBL* FuncTable);
    
    /* For CD-ROM boot, we need to set up the boot path */
    /* Boot partition 0xFF indicates CD-ROM boot (see isoboot.S) */
    if (FrldrBootPartition == 0xFF)
    {
        /* We're booting from CD-ROM */
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)cdrom(0)");
        
        /* Register a minimal disk driver for the boot device */
        /* Note: This won't actually work for reading, but allows filesystem registration */
        FsRegisterDevice(FrLdrBootPath, &Amd64DiskVtbl);
    }
    else if (FrldrBootDrive < 0x80)
    {
        /* Floppy boot */
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)fdisk(%u)", FrldrBootDrive);
    }
    else
    {
        /* Hard disk boot */
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)rdisk(%u)partition(%lu)", 
                           FrldrBootDrive - 0x80, FrldrBootPartition);
    }
    
    /* Output debug info */
    Amd64ConsPutChar('[');
    for (const char *p = FrLdrBootPath; *p; p++)
        Amd64ConsPutChar(*p);
    Amd64ConsPutChar(']');
    
    return TRUE;
}

VOID Amd64MachInit(const char *CmdLine)
{
    /* Setup minimal vtbl for AMD64 */
    RtlZeroMemory(&MachVtbl, sizeof(MachVtbl));
    
    MachVtbl.ConsPutChar = Amd64ConsPutChar;
    MachVtbl.ConsKbHit = Amd64ConsKbHit;
    MachVtbl.ConsGetCh = Amd64ConsGetCh;
    MachVtbl.VideoClearScreen = Amd64VideoClearScreen;
    MachVtbl.VideoSetDisplayMode = Amd64VideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = Amd64VideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = Amd64VideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = Amd64VideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = Amd64VideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = Amd64VideoHideShowTextCursor;
    MachVtbl.VideoPutChar = Amd64VideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = Amd64VideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = Amd64VideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = Amd64VideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = Amd64VideoGetPaletteColor;
    MachVtbl.VideoSync = Amd64VideoSync;
    MachVtbl.Beep = Amd64Beep;
    MachVtbl.PrepareForReactOS = Amd64PrepareForReactOS;
    MachVtbl.GetMemoryMap = Amd64GetMemoryMap;
    MachVtbl.GetExtendedBIOSData = Amd64GetExtendedBIOSData;
    MachVtbl.GetFloppyCount = Amd64GetFloppyCount;
    MachVtbl.DiskReadLogicalSectors = Amd64DiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = Amd64DiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = Amd64DiskGetCacheableBlockCount;
    MachVtbl.HwDetect = Amd64HwDetect;
    MachVtbl.HwIdle = Amd64HwIdle;
    MachVtbl.InitializeBootDevices = Amd64InitializeBootDevices;
}