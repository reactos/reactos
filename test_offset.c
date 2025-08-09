#include <stdio.h>
#include <stddef.h>

/* Simplified LOADER_PARAMETER_EXTENSION structure */
typedef struct {
    unsigned int Size;
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    unsigned char LoaderPagesSpanned[8];
    void* HeadlessLoaderBlock;
    void* SMBiosEPSHeader;
    void* DrvDBImage;
    unsigned int DrvDBSize;
    void* NetworkLoaderBlock;
    void* HalpIRQLToTPR;
    void* HalpVectorToIRQL;
    struct { void *Flink, *Blink; } FirmwareDescriptorListHead;
    void* AcpiTable;
    unsigned int AcpiTableSize;
    unsigned int Flags;
    void* LoaderPerformanceData;
    struct { void *Flink, *Blink; } BootApplicationPersistentData;
    void* WmdTestResult;
    unsigned char BootIdentifier[16];
    unsigned int ResumePages;
    void* DumpHeader;
    /* ReactOS UEFI Extensions */
    struct {
        unsigned long long FrameBufferBase;
        unsigned int FrameBufferSize;
        unsigned int ScreenWidth;
        unsigned int ScreenHeight;
        unsigned int PixelsPerScanLine;
        unsigned int PixelFormat;
    } UefiFramebuffer;
} LOADER_PARAMETER_EXTENSION;

int main() {
    printf("Size of LOADER_PARAMETER_EXTENSION: %lu bytes\n", sizeof(LOADER_PARAMETER_EXTENSION));
    printf("Offset of UefiFramebuffer: %lu bytes\n", offsetof(LOADER_PARAMETER_EXTENSION, UefiFramebuffer));
    printf("Offset of FrameBufferBase: %lu bytes\n", 
           offsetof(LOADER_PARAMETER_EXTENSION, UefiFramebuffer.FrameBufferBase));
    return 0;
}