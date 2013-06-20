/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/envir.h
 * PURPOSE:         LLB Environment Functions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

typedef struct _ATAG_HEADER
{
    ULONG Size;
    ULONG Tag;
} ATAG_HEADER, *PATAG_HEADER;

typedef struct _ATAG_CORE
{
    ULONG Flags;
    ULONG PageSize;
    ULONG RootDev;
} ATAG_CORE, *PATAG_CORE;

typedef struct _ATAG_MEM
{
    ULONG Size;
    ULONG Start;
} ATAG_MEM, *PATAG_MEM;

typedef struct _ATAG_REVISION
{
    ULONG Rev;
} ATAG_REVISION, *PATAG_REVISION;

typedef struct _ATAG_INITRD2
{
    ULONG Start;
    ULONG Size;
} ATAG_INITRD2, *PATAG_INITRD2;

typedef struct _ATAG_CMDLINE
{
    CHAR CmdLine[ANYSIZE_ARRAY];
} ATAG_CMDLINE, *PATAG_CMDLINE;

typedef struct _ATAG
{
    ATAG_HEADER Hdr;
    union
    {
        ATAG_CORE Core;
        ATAG_MEM Mem;
        ATAG_REVISION Revision;
        ATAG_INITRD2 InitRd2;
        ATAG_CMDLINE CmdLine;
    } u;
} ATAG, *PATAG;

#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54420005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008
#define ATAG_CMDLINE    0x54410009

PCHAR
NTAPI
LlbEnvRead(
    IN PCHAR Option
);

BOOLEAN
NTAPI
LlbEnvGetRamDiskInformation(
    IN PULONG Base,
    IN PULONG Size
);

VOID
NTAPI
LlbEnvGetMemoryInformation(
    IN PULONG Base,
    IN PULONG Size
);

VOID
NTAPI
LlbEnvParseArguments(
    IN PATAG Arguments
);

/* EOF */
