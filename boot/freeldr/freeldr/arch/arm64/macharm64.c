/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 machine initialization for UEFI
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <disk.h>
#include <arch/arm64/arm64.h>
#include <arch/uefi/machuefi.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/* Global ARM64 hardware information */
static ULONG Arm64ProcessorFeatures = 0;
static ULONG Arm64CacheLineSize = 64;
static BOOLEAN Arm64HwDetectRan = FALSE;
static PCONFIGURATION_COMPONENT_DATA RootNode = NULL;

/* ARM64 cache information */
static ULONG FirstLevelDcacheSize = 0;
static ULONG FirstLevelDcacheFillSize = 0;
static ULONG FirstLevelIcacheSize = 0;
static ULONG FirstLevelIcacheFillSize = 0;
static ULONG SecondLevelDcacheSize = 0;
static ULONG SecondLevelDcacheFillSize = 0;

/* CPU identification information */
static ULONG Arm64ProcessorType = 0;
static ULONG Arm64ProcessorRevision = 0;
static ULONG Arm64ProcessorArchitecture = 8; /* ARMv8 */

/* Forward declarations */
static VOID Arm64DetectCpuFeatures(VOID);
static VOID Arm64DetectCacheInfo(VOID);

/* ARM64 console output via UEFI */
static VOID Arm64ConsPutChar(int Ch)
{
    /* Use UEFI console implementation */
    UefiConsPutChar(Ch);
}

static BOOLEAN Arm64ConsKbHit(VOID)
{
    /* Use UEFI console implementation */
    return UefiConsKbHit();
}

static int Arm64ConsGetCh(VOID)
{
    /* Use UEFI console implementation */
    return UefiConsGetCh();
}

static VOID Arm64VideoClearScreen(UCHAR Attr)
{
    /* Use UEFI video implementation */
    UefiVideoClearScreen(Attr);
}

static VIDEODISPLAYMODE Arm64VideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
    /* Use UEFI video implementation */
    return UefiVideoSetDisplayMode(DisplayMode, Init);
}

static VOID Arm64VideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    /* Use UEFI video implementation */
    UefiVideoGetDisplaySize(Width, Height, Depth);
}

static ULONG Arm64VideoGetBufferSize(VOID)
{
    /* Use UEFI video implementation */
    return UefiVideoGetBufferSize();
}

static VOID Arm64VideoGetFontsFromFirmware(PULONG RomFontPointers)
{
    /* Use UEFI video implementation */
    UefiVideoGetFontsFromFirmware(RomFontPointers);
}

static VOID Arm64VideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* Use UEFI video implementation */
    UefiVideoSetTextCursorPosition(X, Y);
}

static VOID Arm64VideoHideShowTextCursor(BOOLEAN Show)
{
    /* Use UEFI video implementation */
    UefiVideoHideShowTextCursor(Show);
}

static VOID Arm64VideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    /* Use UEFI video implementation */
    UefiVideoPutChar(Ch, Attr, X, Y);
}

static VOID Arm64VideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    /* Use UEFI video implementation */
    UefiVideoCopyOffScreenBufferToVRAM(Buffer);
}

static BOOLEAN Arm64VideoIsPaletteFixed(VOID)
{
    /* Use UEFI video implementation */
    return UefiVideoIsPaletteFixed();
}

static VOID Arm64VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
    /* Use UEFI video implementation */
    UefiVideoSetPaletteColor(Color, Red, Green, Blue);
}

static VOID Arm64VideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue)
{
    /* Use UEFI video implementation */
    UefiVideoGetPaletteColor(Color, Red, Green, Blue);
}

static VOID Arm64VideoSync(VOID)
{
    /* Use UEFI video implementation */
    UefiVideoSync();
}

static VOID Arm64Beep(VOID)
{
    /* Use UEFI beep implementation */
    UefiPcBeep();
}

static VOID Arm64PrepareForReactOS(VOID)
{
    TRACE("ARM64: Preparing for ReactOS kernel handoff\n");
    
    /* Disable interrupts */
    Arm64DisableInterrupts();
    
    /* Disable timer interrupts */
    Arm64DisableTimerInterrupt();
    
    /* Complete cache maintenance using ARM64 specific routines */
    Arm64CompleteCacheMaintenance();
    
    /* Use UEFI preparation which exits boot services */
    UefiPrepareForReactOS();
    
    /* ARM64 specific final preparation */
    /* Disable MMU for kernel (it will set up its own) */
    Arm64DisableMMU();
    
    /* Final memory barriers */
    Arm64DataMemoryBarrier();
    Arm64InstructionBarrier();
    
    TRACE("ARM64: Ready for kernel handoff\n");
}

/* ARM64 memory management - using UEFI implementation */

static FREELDR_MEMORY_DESCRIPTOR* Arm64GetMemoryMap(PULONG MaxMemoryMapSize)
{
    /* Use UEFI memory management implementation */
    return UefiMemGetMemoryMap(MaxMemoryMapSize);
}

/* ARM64 CPU feature detection */
static VOID Arm64DetectCpuFeatures(VOID)
{
    ULONGLONG id_aa64isar0, id_aa64pfr0, midr;
    
    /* Read processor identification registers */
    midr = ARM64_READ_SYSREG(midr_el1);
    id_aa64isar0 = ARM64_READ_SYSREG(id_aa64isar0_el1);
    id_aa64pfr0 = ARM64_READ_SYSREG(id_aa64pfr0_el1);
    
    /* Extract processor information */
    Arm64ProcessorType = (ULONG)((midr >> 4) & 0xFFF);
    Arm64ProcessorRevision = (ULONG)(midr & 0xF);
    
    /* Check for various CPU features */
    Arm64ProcessorFeatures = 0;
    
    /* Check for AES support */
    if ((id_aa64isar0 & 0xF0) != 0)
    {
        Arm64ProcessorFeatures |= 0x1;
        TRACE("ARM64: AES encryption support detected\n");
    }
    
    /* Check for SHA support */
    if (((id_aa64isar0 >> 8) & 0xF) != 0)
    {
        Arm64ProcessorFeatures |= 0x2;
        TRACE("ARM64: SHA hash support detected\n");
    }
    
    /* Check for floating point support */
    if ((id_aa64pfr0 & 0xF) != 0xF)
    {
        Arm64ProcessorFeatures |= 0x4;
        TRACE("ARM64: Floating point support detected\n");
    }
    
    TRACE("ARM64: CPU Type=0x%lx, Revision=%lu, Features=0x%lx\n",
          Arm64ProcessorType, Arm64ProcessorRevision, Arm64ProcessorFeatures);
}

/* ARM64 cache information detection - Enhanced */
static VOID Arm64DetectCacheInfo(VOID)
{
    ULONGLONG ctr, ccsidr, clidr;
    ULONG level;
    
    /* Read cache type register */
    ctr = ARM64_READ_SYSREG(ctr_el0);
    
    /* Get cache line size using U-Boot method */
    Arm64CacheLineSize = (ULONG)Arm64GetCacheLineSize();
    
    /* Read cache level ID register */
    clidr = ARM64_READ_SYSREG(clidr_el1);
    ULONG loc = (ULONG)((clidr >> 24) & 0x7);  /* Level of Coherency */
    
    TRACE("ARM64: Cache LoC=%lu\n", loc);
    
    /* Detect L1 data cache */
    ARM64_WRITE_SYSREG(csselr_el1, 0);  /* L1 data cache */
    ARM64_ISB();
    
    ccsidr = ARM64_READ_SYSREG(ccsidr_el1);
    
    /* Calculate cache parameters using U-Boot method */
    ULONG line_size = 4 << ((ccsidr & 0x7) + 2);
    ULONG ways = ((ccsidr >> 3) & 0x3FF) + 1;
    ULONG sets = ((ccsidr >> 13) & 0x7FFF) + 1;
    
    FirstLevelDcacheSize = ways * sets * line_size;
    FirstLevelDcacheFillSize = line_size;
    
    /* Detect L1 instruction cache */
    ARM64_WRITE_SYSREG(csselr_el1, 1);  /* L1 instruction cache */
    ARM64_ISB();
    
    ccsidr = ARM64_READ_SYSREG(ccsidr_el1);
    
    line_size = 4 << ((ccsidr & 0x7) + 2);
    ways = ((ccsidr >> 3) & 0x3FF) + 1;
    sets = ((ccsidr >> 13) & 0x7FFF) + 1;
    
    FirstLevelIcacheSize = ways * sets * line_size;
    FirstLevelIcacheFillSize = line_size;
    
    /* Check for L2 cache */
    if (loc >= 2) {
        ARM64_WRITE_SYSREG(csselr_el1, 2);  /* L2 unified cache */
        ARM64_ISB();
        
        ccsidr = ARM64_READ_SYSREG(ccsidr_el1);
        
        line_size = 4 << ((ccsidr & 0x7) + 2);
        ways = ((ccsidr >> 3) & 0x3FF) + 1;
        sets = ((ccsidr >> 13) & 0x7FFF) + 1;
        
        SecondLevelDcacheSize = ways * sets * line_size;
        SecondLevelDcacheFillSize = line_size;
        
        TRACE("ARM64: L2 Cache=%lu KB, Line=%lu bytes\n",
              SecondLevelDcacheSize / 1024, line_size);
    }
    
    TRACE("ARM64: L1 DCache=%lu KB, ICache=%lu KB, Line=%lu bytes\n",
          FirstLevelDcacheSize / 1024, FirstLevelIcacheSize / 1024, Arm64CacheLineSize);
    
    /* Reset cache selection register */
    ARM64_WRITE_SYSREG(csselr_el1, 0);
    ARM64_ISB();
}

static VOID Arm64GetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    /* Use UEFI implementation */
    UefiGetExtendedBIOSData(ExtendedBIOSDataArea, ExtendedBIOSDataSize);
}

static UCHAR Arm64GetFloppyCount(VOID)
{
    /* Use UEFI implementation */
    return UefiGetFloppyCount();
}

static BOOLEAN Arm64DiskReadLogicalSectors(IN UCHAR DriveNumber,
                                           IN ULONGLONG SectorNumber,
                                           IN ULONG SectorCount,
                                           OUT PVOID Buffer)
{
    /* Use UEFI disk I/O implementation */
    return UefiDiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

static BOOLEAN Arm64DiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    /* Use UEFI implementation */
    return UefiDiskGetDriveGeometry(DriveNumber, Geometry);
}

static ULONG Arm64DiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    /* Use UEFI implementation */
    return UefiDiskGetCacheableBlockCount(DriveNumber);
}


static PCONFIGURATION_COMPONENT_DATA Arm64HwDetect(const CHAR* Options)
{
    if (Arm64HwDetectRan)
        return RootNode;
    
    Arm64HwDetectRan = TRUE;
    
    TRACE("ARM64: Hardware detection started\n");
    
    /* Detect ARM64 specific CPU features and cache information */
    Arm64DetectCpuFeatures();
    Arm64DetectCacheInfo();
    
    /* Use UEFI hardware detection for most components */
    RootNode = UefiHwDetect(Options);
    
    /* Add ARM64 specific hardware information to the tree */
    if (RootNode)
    {
        /* Add ARM64 processor information to existing tree */
        PCONFIGURATION_COMPONENT_DATA ProcessorKey;
        CHAR Buffer[128];
        
        /* Find or create processor node */
        FldrCreateComponentKey(RootNode,
                              ProcessorClass,
                              CentralProcessor,
                              0,
                              0,
                              0,
                              "ARM64 Processor",
                              &ProcessorKey);
        
        /* Add processor identifier */
        RtlStringCbPrintfA(Buffer, sizeof(Buffer),
                           "ARM64 Family %lu Model %lu Stepping %lu",
                           Arm64ProcessorArchitecture,
                           Arm64ProcessorType,
                           Arm64ProcessorRevision);
        
        FldrSetIdentifier(ProcessorKey, Buffer);
        
        /* Add cache information if available */
        if (FirstLevelDcacheSize > 0)
        {
            PCONFIGURATION_COMPONENT_DATA CacheKey;
            FldrCreateComponentKey(ProcessorKey,
                                  CacheClass,
                                  PrimaryDcache,
                                  0,
                                  0,
                                  FirstLevelDcacheSize,
                                  "L1 Data Cache",
                                  &CacheKey);
        }
        
        if (FirstLevelIcacheSize > 0)
        {
            PCONFIGURATION_COMPONENT_DATA CacheKey;
            FldrCreateComponentKey(ProcessorKey,
                                  CacheClass,
                                  PrimaryIcache,
                                  0,
                                  0,
                                  FirstLevelIcacheSize,
                                  "L1 Instruction Cache",
                                  &CacheKey);
        }
    }
    
    /* Initialize RAMDISK if available */
    RamDiskInitialize(TRUE, NULL, NULL);
    
    TRACE("ARM64: Hardware detection completed\n");
    
    return RootNode;
}

static VOID Arm64HwIdle(VOID)
{
    /* Use UEFI idle implementation with ARM64 WFI fallback */
    UefiHwIdle();
    
    /* ARM64 wait for interrupt instruction as backup */
    __asm__ volatile ("wfi" ::: "memory");
}

/* ARM64 MMU initialization stub */
VOID Arm64InitializeMMU(VOID)
{
    /* This will be implemented when full MMU support is added */
    TRACE("ARM64: MMU initialization deferred to kernel\n");
}

/* Simple disk reading for UEFI */
static ARC_STATUS Arm64DiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    /* Just return success for now */
    *FileId = 1;
    return ESUCCESS;
}

static ARC_STATUS Arm64DiskClose(ULONG FileId)
{
    return ESUCCESS;
}

static ARC_STATUS Arm64DiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    return ESUCCESS;
}

/* Global state for disk reading */
static ULONGLONG CurrentDiskPosition = 0;

static ARC_STATUS Arm64DiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    extern UCHAR FrldrBootDrive;
    ULONG SectorSize = 512; /* Standard sector size */
    ULONGLONG StartSector;
    ULONG SectorCount;
    ULONG BytesToRead;
    
    /* Calculate sectors to read */
    StartSector = CurrentDiskPosition / SectorSize;
    BytesToRead = N;
    SectorCount = (BytesToRead + SectorSize - 1) / SectorSize;
    
    /* Try to read the sectors */
    if (Arm64DiskReadLogicalSectors(FrldrBootDrive, StartSector, SectorCount, Buffer))
    {
        *Count = BytesToRead;
        CurrentDiskPosition += BytesToRead;
        return ESUCCESS;
    }
    
    *Count = 0;
    return EIO;
}

static ARC_STATUS Arm64DiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
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

static const DEVVTBL Arm64DiskVtbl =
{
    Arm64DiskClose,
    Arm64DiskGetFileInformation,
    Arm64DiskOpen,
    Arm64DiskRead,
    Arm64DiskSeek,
};

static BOOLEAN Arm64InitializeBootDevices(VOID)
{
    /* Use UEFI boot device initialization */
    return UefiInitializeBootDevices();
}

static ULONG Arm64GetTime(VOID)
{
    /* Use UEFI time services */
    return UefiGetTime();
}

/* Initialize machine abstraction for ARM64 */
VOID Arm64MachInit(const char *CmdLine)
{
    TRACE("ARM64: Initializing machine abstraction layer\n");
    
    /* Initialize enhanced ARM64 subsystems */
    Arm64InitializeExceptions();
    Arm64InitializeTimer();
    Arm64InitializeMMU();
    
    /* Clear the machine vtable */
    RtlZeroMemory(&MachVtbl, sizeof(MachVtbl));
    
    /* Console functions */
    MachVtbl.ConsPutChar = Arm64ConsPutChar;
    MachVtbl.ConsKbHit = Arm64ConsKbHit;
    MachVtbl.ConsGetCh = Arm64ConsGetCh;
    
    /* Video functions */
    MachVtbl.VideoClearScreen = Arm64VideoClearScreen;
    MachVtbl.VideoSetDisplayMode = Arm64VideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = Arm64VideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = Arm64VideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = Arm64VideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = Arm64VideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = Arm64VideoHideShowTextCursor;
    MachVtbl.VideoPutChar = Arm64VideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = Arm64VideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = Arm64VideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = Arm64VideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = Arm64VideoGetPaletteColor;
    MachVtbl.VideoSync = Arm64VideoSync;
    
    /* System functions */
    MachVtbl.Beep = Arm64Beep;
    MachVtbl.PrepareForReactOS = Arm64PrepareForReactOS;
    MachVtbl.GetMemoryMap = Arm64GetMemoryMap;
    MachVtbl.GetExtendedBIOSData = Arm64GetExtendedBIOSData;
    MachVtbl.GetFloppyCount = Arm64GetFloppyCount;
    
    /* Disk functions */
    MachVtbl.DiskReadLogicalSectors = Arm64DiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = Arm64DiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = Arm64DiskGetCacheableBlockCount;
    
    /* Hardware detection and management */
    MachVtbl.HwDetect = Arm64HwDetect;
    MachVtbl.HwIdle = Arm64HwIdle;
    MachVtbl.InitializeBootDevices = Arm64InitializeBootDevices;
    MachVtbl.GetTime = Arm64GetTime;
    
    TRACE("ARM64: Machine abstraction layer initialized\n");
    
    /* Initialize UEFI video system */
    if (UefiInitializeVideo() != EFI_SUCCESS)
    {
        ERR("ARM64: Failed to initialize UEFI video\n");
    }
    
    /* Perform initial hardware detection */
    Arm64HwDetect(NULL);
    
    TRACE("ARM64: All subsystems initialized and ready\n");
}