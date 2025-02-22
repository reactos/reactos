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
CODE_SEG("INIT") NTSTATUS NTAPI FsRtlInitializeWorkerThread(VOID);
extern KSEMAPHORE FsRtlpUncSemaphore;

static const UCHAR LegalAnsiCharacterArray[] =
{
  0,                                                                          /* CTRL+@, 0x00 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+A, 0x01 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+B, 0x02 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+C, 0x03 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+D, 0x04 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+E, 0x05 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+F, 0x06 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+G, 0x07 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+H, 0x08 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+I, 0x09 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+J, 0x0a */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+K, 0x0b */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+L, 0x0c */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+M, 0x0d */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+N, 0x0e */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+O, 0x0f */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+P, 0x10 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+Q, 0x11 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+R, 0x12 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+S, 0x13 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+T, 0x14 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+U, 0x15 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+V, 0x16 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+W, 0x17 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+X, 0x18 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+Y, 0x19 */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+Z, 0x1a */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+[, 0x1b */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+\, 0x1c */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+], 0x1d */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+^, 0x1e */
  FSRTL_OLE_LEGAL,                                                            /* CTRL+_, 0x1f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ` ',    0x20 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `!',    0x21 */
  FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `"',    0x22 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `#',    0x23 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `$',    0x24 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `%',    0x25 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `&',    0x26 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `'',    0x27 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `(',    0x28 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `)',    0x29 */
  FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `*',    0x2a */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `+',    0x2b */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `,',    0x2c */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `-',    0x2d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `.',    0x2e */
  0,                                                                          /* `/',    0x2f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `0',    0x30 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `1',    0x31 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `2',    0x32 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `3',    0x33 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `4',    0x34 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `5',    0x35 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `6',    0x36 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `7',    0x37 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `8',    0x38 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `9',    0x39 */
  FSRTL_NTFS_LEGAL,                                                           /* `:',    0x3a */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `;',    0x3b */
  FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `<',    0x3c */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `=',    0x3d */
  FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `>',    0x3e */
  FSRTL_OLE_LEGAL | FSRTL_WILD_CHARACTER,                                     /* `?',    0x3f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `@',    0x40 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `A',    0x41 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `B',    0x42 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `C',    0x43 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `D',    0x44 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `E',    0x45 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `F',    0x46 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `G',    0x47 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `H',    0x48 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `I',    0x49 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `J',    0x4a */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `K',    0x4b */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `L',    0x4c */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `M',    0x4d */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `N',    0x4e */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `O',    0x4f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `P',    0x50 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Q',    0x51 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `R',    0x52 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `S',    0x53 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `T',    0x54 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `U',    0x55 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `V',    0x56 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `W',    0x57 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `X',    0x58 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Y',    0x59 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Z',    0x5a */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `[',    0x5b */
  0,                                                                          /* `\',    0x5c */
  FSRTL_OLE_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `]',    0x5d */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `^',    0x5e */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `_',    0x5f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ``',    0x60 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `a',    0x61 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `b',    0x62 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `c',    0x63 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `d',    0x64 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `e',    0x65 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `f',    0x66 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `g',    0x67 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `h',    0x68 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `i',    0x69 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `j',    0x6a */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `k',    0x6b */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `l',    0x6c */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `m',    0x6d */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `n',    0x6e */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `o',    0x6f */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `p',    0x70 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `q',    0x71 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `r',    0x72 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `s',    0x73 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `t',    0x74 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `u',    0x75 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `v',    0x76 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `w',    0x77 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `x',    0x78 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `y',    0x79 */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `z',    0x7a */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `{',    0x7b */
  FSRTL_OLE_LEGAL,                                                            /* `|',    0x7c */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `}',    0x7d */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `~',    0x7e */
  FSRTL_OLE_LEGAL | FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL     /*         0x7f */
};

const UCHAR * const FsRtlLegalAnsiCharacterArray = LegalAnsiCharacterArray;

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
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
    KeInitializeSemaphore(&FsRtlpUncSemaphore, 1, MAXLONG);

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

