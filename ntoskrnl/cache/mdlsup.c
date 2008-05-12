/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/logsup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CcMdlRead(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          OUT PMDL *MdlChain,
          OUT PIO_STATUS_BLOCK IoStatus)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcMdlReadComplete(IN PFILE_OBJECT FileObject,
                  IN PMDL MdlChain)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcMdlReadComplete2(IN PFILE_OBJECT FileObject,
                   IN PMDL MdlChain)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcPrepareMdlWrite(IN PFILE_OBJECT FileObject,
                  IN PLARGE_INTEGER FileOffset,
                  IN ULONG Length,
                  OUT PMDL *MdlChain,
                  OUT PIO_STATUS_BLOCK IoStatus)
{
    UNIMPLEMENTED;
    while (TRUE); 
}

VOID
NTAPI
CcMdlWriteComplete(IN PFILE_OBJECT FileObject,
                   IN PLARGE_INTEGER FileOffset,
                   IN PMDL MdlChain)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcMdlWriteComplete2(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PMDL MdlChain)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcMdlWriteAbort(IN PFILE_OBJECT FileObject,
                IN PMDL MdlChain)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
