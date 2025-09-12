/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/mdlsup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include "newcc.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

PMDL
NTAPI
CcpBuildCacheMdl(PFILE_OBJECT FileObject,
                 PLARGE_INTEGER FileOffset,
                 ULONG Length,
                 PIO_STATUS_BLOCK IOSB)
{
    PMDL Mdl;
    PVOID Bcb, Buffer;

    BOOLEAN Result = CcMapData(FileObject,
                               FileOffset,
                               Length,
                               PIN_WAIT,
                               &Bcb,
                               &Buffer);

    if (!Result)
    {
        IOSB->Information = 0;
        IOSB->Status = STATUS_UNSUCCESSFUL;
        return NULL;
    }

    IOSB->Information = Length;
    IOSB->Status = STATUS_SUCCESS;

    Mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);

    if (!Mdl)
    {
        IOSB->Information = 0;
        IOSB->Status = STATUS_NO_MEMORY;
        return NULL;
    }

    IOSB->Information = Length;
    IOSB->Status = STATUS_SUCCESS;

    return Mdl;
}

VOID
NTAPI
CcMdlRead(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          OUT PMDL *MdlChain,
          OUT PIO_STATUS_BLOCK IoStatus)
{
    *MdlChain = CcpBuildCacheMdl(FileObject, FileOffset, Length, IoStatus);
}

VOID
NTAPI
CcMdlReadComplete(IN PFILE_OBJECT FileObject,
                  IN PMDL MdlChain)
{
    IoFreeMdl(MdlChain);
}

VOID
NTAPI
CcMdlReadComplete2(IN PFILE_OBJECT FileObject,
                   IN PMDL MdlChain)
{
    UNIMPLEMENTED
}

VOID
NTAPI
CcPrepareMdlWrite(IN PFILE_OBJECT FileObject,
                  IN PLARGE_INTEGER FileOffset,
                  IN ULONG Length,
                  OUT PMDL *MdlChain,
                  OUT PIO_STATUS_BLOCK IoStatus)
{
    *MdlChain = CcpBuildCacheMdl(FileObject, FileOffset, Length, IoStatus);
}

VOID
NTAPI
CcMdlWriteComplete(IN PFILE_OBJECT FileObject,
                   IN PLARGE_INTEGER FileOffset,
                   IN PMDL MdlChain)
{
    IoFreeMdl(MdlChain);
}

VOID
NTAPI
CcMdlWriteComplete2(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PMDL MdlChain)
{
    UNIMPLEMENTED
}

VOID
NTAPI
CcMdlWriteAbort(IN PFILE_OBJECT FileObject,
                IN PMDL MdlChain)
{
    ASSERT(FALSE);
    IoFreeMdl(MdlChain);
}

/* EOF */
