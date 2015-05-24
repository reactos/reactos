/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/fsrtlpc.c
 * PURPOSE:         Contains initialization and support code for the FsRtl
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PERESOURCE FsRtlPagingIoResources;
ULONG FsRtlPagingIoResourceSelector;
NTSTATUS NTAPI INIT_FUNCTION FsRtlInitializeWorkerThread(VOID);

static const UCHAR LegalAnsiCharacterArray[] =
{
  0,                                                        /* CTRL+@, 0x00 */
  0,                                                        /* CTRL+A, 0x01 */
  0,                                                        /* CTRL+B, 0x02 */
  0,                                                        /* CTRL+C, 0x03 */
  0,                                                        /* CTRL+D, 0x04 */
  0,                                                        /* CTRL+E, 0x05 */
  0,                                                        /* CTRL+F, 0x06 */
  0,                                                        /* CTRL+G, 0x07 */
  0,                                                        /* CTRL+H, 0x08 */
  0,                                                        /* CTRL+I, 0x09 */
  0,                                                        /* CTRL+J, 0x0a */
  0,                                                        /* CTRL+K, 0x0b */
  0,                                                        /* CTRL+L, 0x0c */
  0,                                                        /* CTRL+M, 0x0d */
  0,                                                        /* CTRL+N, 0x0e */
  0,                                                        /* CTRL+O, 0x0f */
  0,                                                        /* CTRL+P, 0x10 */
  0,                                                        /* CTRL+Q, 0x11 */
  0,                                                        /* CTRL+R, 0x12 */
  0,                                                        /* CTRL+S, 0x13 */
  0,                                                        /* CTRL+T, 0x14 */
  0,                                                        /* CTRL+U, 0x15 */
  0,                                                        /* CTRL+V, 0x16 */
  0,                                                        /* CTRL+W, 0x17 */
  0,                                                        /* CTRL+X, 0x18 */
  0,                                                        /* CTRL+Y, 0x19 */
  0,                                                        /* CTRL+Z, 0x1a */
  0,                                                        /* CTRL+[, 0x1b */
  0,                                                        /* CTRL+\, 0x1c */
  0,                                                        /* CTRL+], 0x1d */
  0,                                                        /* CTRL+^, 0x1e */
  0,                                                        /* CTRL+_, 0x1f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ` ',    0x20 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `!',    0x21 */
  FSRTL_WILD_CHARACTER,                                     /* `"',    0x22 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `#',    0x23 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `$',    0x24 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `%',    0x25 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `&',    0x26 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `'',    0x27 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `(',    0x28 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `)',    0x29 */
  FSRTL_WILD_CHARACTER,                                     /* `*',    0x2a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `+',    0x2b */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `,',    0x2c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `-',    0x2d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `.',    0x2e */
  0,                                                        /* `/',    0x2f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `0',    0x30 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `1',    0x31 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `2',    0x32 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `3',    0x33 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `4',    0x34 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `5',    0x35 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `6',    0x36 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `7',    0x37 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `8',    0x38 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `9',    0x39 */
  FSRTL_NTFS_LEGAL,                                         /* `:',    0x3a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `;',    0x3b */
  FSRTL_WILD_CHARACTER,                                     /* `<',    0x3c */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `=',    0x3d */
  FSRTL_WILD_CHARACTER,                                     /* `>',    0x3e */
  FSRTL_WILD_CHARACTER,                                     /* `?',    0x3f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `@',    0x40 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `A',    0x41 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `B',    0x42 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `C',    0x43 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `D',    0x44 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `E',    0x45 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `F',    0x46 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `G',    0x47 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `H',    0x48 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `I',    0x49 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `J',    0x4a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `K',    0x4b */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `L',    0x4c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `M',    0x4d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `N',    0x4e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `O',    0x4f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `P',    0x50 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Q',    0x51 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `R',    0x52 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `S',    0x53 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `T',    0x54 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `U',    0x55 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `V',    0x56 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `W',    0x57 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `X',    0x58 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Y',    0x59 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Z',    0x5a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `[',    0x5b */
  0,                                                        /* `\',    0x5c */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `]',    0x5d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `^',    0x5e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `_',    0x5f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ``',    0x60 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `a',    0x61 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `b',    0x62 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `c',    0x63 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `d',    0x64 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `e',    0x65 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `f',    0x66 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `g',    0x67 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `h',    0x68 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `i',    0x69 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `j',    0x6a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `k',    0x6b */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `l',    0x6c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `m',    0x6d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `n',    0x6e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `o',    0x6f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `p',    0x70 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `q',    0x71 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `r',    0x72 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `s',    0x73 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `t',    0x74 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `u',    0x75 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `v',    0x76 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `w',    0x77 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `x',    0x78 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `y',    0x79 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `z',    0x7a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `{',    0x7b */
  0,                                                        /* `|',    0x7c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `}',    0x7d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `~',    0x7e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL     /*         0x7f */
};

const UCHAR * const FsRtlLegalAnsiCharacterArray = LegalAnsiCharacterArray;

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
INIT_FUNCTION
FsRtlInitSystem(VOID)
{
    ULONG i;

    /* Initialize the list for granted locks */
    ExInitializePagedLookasideList(&FsRtlFileLockLookasideList,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(FILE_LOCK),
                                   IFS_POOL_TAG,
                                   0);

    FsRtlInitializeTunnels();
    FsRtlInitializeLargeMcbs();

    /* Allocate the Resource Buffer */
    FsRtlPagingIoResources = FsRtlAllocatePoolWithTag(NonPagedPool,
                                                      FSRTL_MAX_RESOURCES *
                                                      sizeof(ERESOURCE),
                                                      'eRsF');

    /* Initialize the Resources */
    for (i = 0; i < FSRTL_MAX_RESOURCES; i++)
    {
        ExInitializeResource(&FsRtlPagingIoResources[i]);
    }

    return NT_SUCCESS(FsRtlInitializeWorkerThread());
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlAllocateResource
 * @implemented NT 4.0
 *
 *     The FsRtlAllocateResource routine returns a pre-initialized ERESOURCE
 *     for use by a File System Driver.
 *
 * @return A Pointer to a pre-initialized ERESOURCE.
 *
 * @remarks The File System Library only provides up to 16 Resources.
 *
 *--*/
PERESOURCE
NTAPI
FsRtlAllocateResource(VOID)
{
    PAGED_CODE();

    /* Return a preallocated ERESOURCE */
    return &FsRtlPagingIoResources[FsRtlPagingIoResourceSelector++ %
                                   FSRTL_MAX_RESOURCES];
}

