#include <stdio.h>
#include <stddef.h>

typedef struct LIST_ENTRY {
    struct LIST_ENTRY *Flink;
    struct LIST_ENTRY *Blink;
} LIST_ENTRY;

typedef struct GUID {
    unsigned int Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

typedef struct {
    unsigned long long QuadPart;
} PHYSICAL_ADDRESS;

typedef struct LOADER_PARAMETER_EXTENSION {
    unsigned int Size;
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    void* LoaderPagesSpanned;  /* pointer, not ULONG_PTR on x64 */
    void* HeadlessLoaderBlock;
    void* SMBiosEPSHeader;
    void* DrvDBImage;
    unsigned int DrvDBSize;
    void* NetworkLoaderBlock;
    void* HalpIRQLToTPR;  /* Only on x86 */
    void* HalpVectorToIRQL;  /* Only on x86 */
    LIST_ENTRY FirmwareDescriptorListHead;
    void* AcpiTable;
    unsigned int AcpiTableSize;
    unsigned int Flags;  /* BootViaWinload:1, BootViaEFI:1, Reserved:30 */
    void* LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    void* WmdTestResult;
    GUID BootIdentifier;
    unsigned int ResumePages;
    void* DumpHeader;
    struct {
        PHYSICAL_ADDRESS FrameBufferBase;
        unsigned int FrameBufferSize;
        unsigned int ScreenWidth;
        unsigned int ScreenHeight;
        unsigned int PixelsPerScanLine;
        unsigned int PixelFormat;
    } UefiFramebuffer;
} LOADER_PARAMETER_EXTENSION;

int main() {
    printf("sizeof(LOADER_PARAMETER_EXTENSION) = %lu\n", sizeof(LOADER_PARAMETER_EXTENSION));
    printf("offsetof(UefiFramebuffer) = %lu\n", offsetof(LOADER_PARAMETER_EXTENSION, UefiFramebuffer));
    printf("offsetof(UefiFramebuffer.FrameBufferBase) = %lu\n", 
           offsetof(LOADER_PARAMETER_EXTENSION, UefiFramebuffer.FrameBufferBase));
    printf("offsetof(Size) = %lu\n", offsetof(LOADER_PARAMETER_EXTENSION, Size));
    printf("offsetof(Flags) = %lu\n", offsetof(LOADER_PARAMETER_EXTENSION, Flags));
    return 0;
}
