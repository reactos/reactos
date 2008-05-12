/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/copysup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG CcFastMdlReadWait;
ULONG CcFastMdlReadNotPossible;
ULONG CcFastReadNotPossible;
ULONG CcFastReadWait;
ULONG CcFastReadNoWait;
ULONG CcFastReadResourceMiss;

#define TAG_COPY_READ  TAG('C', 'o', 'p', 'y')
#define TAG_COPY_WRITE TAG('R', 'i', 't', 'e')

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CcCopyRead(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset,
           IN ULONG Length,
           IN BOOLEAN Wait,
           OUT PVOID Buffer,
           OUT PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS Status;
    ULONG AlignBase, AlignSize, DeltaBase;
    LARGE_INTEGER SectorBase;
    IO_STATUS_BLOCK IoStatusBlock = {{0}};
    KEVENT Event;
    PCHAR SystemBuffer;
    PMDL Mdl;
    BOOLEAN DirectRead = FALSE;
    DPRINT("CcCopyRead(FileObject 0x%p, FileOffset %I64x, "
           "Length %lx, Wait %d, Buffer 0x%p, IoStatus 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait,
           Buffer, IoStatus);

    /* Is this a page-aligned read? */
    if (((Length % PAGE_SIZE) == 0) && ((FileOffset->QuadPart % PAGE_SIZE) == 0))
    {
        /* Don't double-buffer */
        DirectRead = TRUE;
    }

    /* Do we need to double-buffer? */
    if (!DirectRead)
    {
        /* Align the buffer to page size */
        AlignBase = ROUND_DOWN(FileOffset->QuadPart, PAGE_SIZE);
        AlignSize = ROUND_UP(Length, PAGE_SIZE);
        SectorBase.QuadPart = AlignBase;

        /* Get the offset from page-aligned to request */
        DeltaBase = FileOffset->QuadPart - AlignBase;

        /* Our read may cross a page boundary, so account for that */
        if ((DeltaBase + Length) > AlignSize) AlignSize += PAGE_SIZE;

        /* Allocate a buffer */
        SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, AlignSize, TAG_COPY_READ);
    }
    else
    {
        /* We'll reuse the caller's buffer */
        SystemBuffer = Buffer;
        AlignSize = Length;
        SectorBase = *FileOffset;
        DeltaBase = 0;
    }

    /* Create an MDL for the transfer */
    Mdl = IoAllocateMdl(SystemBuffer, AlignSize, TRUE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(Mdl),
    Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

    /* Setup the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Read the page */
    Status = IoPageRead(FileObject, Mdl, &SectorBase, &Event, &IoStatusBlock);
    if (Status == STATUS_PENDING)
    {
        /* Do the wait */
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) DPRINT1("Status: %lx\n", Status);
    ASSERT(NT_SUCCESS(Status));

    /* Did we double buffer? */
    if (!DirectRead)
    {
        /* Now copy the actual data the caller expected */
        RtlCopyMemory(Buffer, SystemBuffer + DeltaBase, Length);

        /* Free our copy */
        ExFreePool(SystemBuffer);
    }

    /* Free the MDL */
    IoFreeMdl(Mdl);

    /* Check if we read less than the caller wanted */
    if (IoStatusBlock.Information < Length)
    {
        /* Only then do we write the real size */
        IoStatus->Information = IoStatusBlock.Information;
    }
    else
    {
        /* Otherwise, we'll fake that we read as much as the caller wanted */
        IoStatus->Information = Length;
    }

    /* Write status block */
    IoStatus->Status = Status;
    return TRUE;
}

VOID
NTAPI
CcFastCopyRead(IN PFILE_OBJECT FileObject,
               IN ULONG FileOffset,
               IN ULONG Length,
               IN ULONG PageCount,
               OUT PVOID Buffer,
               OUT PIO_STATUS_BLOCK IoStatus)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcCopyWrite(IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN ULONG Length,
            IN BOOLEAN Wait,
            IN PVOID Buffer)
{
    NTSTATUS Status;
    ULONG AlignBase, AlignSize, DeltaBase, WriteEnd, AlignEnd, DeltaEnd;
    LARGE_INTEGER SectorBase, ReadSector;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PCHAR SystemBuffer;
    PMDL Mdl, ReadMdl;
    BOOLEAN DirectWrite = FALSE;
    DPRINT("CcCopyWrite(FileObject 0x%p, FileOffset %I64x, "
           "Length %lx, Wait %d, Buffer 0x%p)\n",
           FileObject, FileOffset->QuadPart, Length, Wait,
           Buffer);
    DPRINT("File name: %wZ\n", &FileObject->FileName);

    /* It this a page-aligned write? If so, we don't need to double-buffer */
    if (((Length % PAGE_SIZE) == 0) && ((FileOffset->QuadPart % PAGE_SIZE) == 0))
    {
        /* Don't double-buffer */
        DirectWrite = TRUE;
    }

    /* Do we need to double-buffer? */
    if (!DirectWrite)
    {
        /* Align the buffer to page size */
        AlignBase = ROUND_DOWN(FileOffset->QuadPart, PAGE_SIZE);
        AlignSize = ROUND_UP(Length, PAGE_SIZE);
        WriteEnd = FileOffset->QuadPart + Length;
        AlignEnd = ROUND_UP(WriteEnd, PAGE_SIZE);
        SectorBase.QuadPart = AlignBase;

        /* Get the offset from page-aligned to request */
        DeltaBase = FileOffset->QuadPart - AlignBase;
        DeltaEnd = AlignEnd - WriteEnd;

        /* Our write may cross a page boundary, so account for that */
        if ((DeltaBase + Length) > AlignSize) AlignSize += PAGE_SIZE;

        /* Allocate a buffer */
        SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, AlignSize, TAG_COPY_WRITE);
    }
    else
    {
        /* We'll reuse the caller's buffer */
        SystemBuffer = Buffer;
        AlignSize = Length;
        SectorBase = *FileOffset;
        DeltaBase = 0;
    }

    /* Setup the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Create an MDL for the write transfer */
    Mdl = IoAllocateMdl(SystemBuffer, AlignSize, TRUE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(Mdl),
    Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

    /* If this is double-buffered, we'll need the aligned original data */
    if (!DirectWrite)
    {
        //
        // We only want to make two reads at most. Suppose:
        //
        //      [                                               ]
        // --------------------------------------------------------------
        // |    |        |              |               |       |       |
        // |    |        |              |               |       |       |
        // |    |        |              |               |       |       |
        // --------------------------------------------------------------
        //      [                                               ]
        //
        // Is the region being written to. Our approach will be to:
        //
        // 1) Allocate a buffer large enough for the entire page-bounded region
        // 2) Read the original data at the left of the [
        // 3) Read the original data at the right of the ]
        // 4) Copy the caller's buffer on top of ours, at the right offset
        //
        // We can optimize a read away if either the [ region is page-aligned or
        // if the ] region is page-aligned.
        //
        // If both are page-aligned, we've already optimized this and we wouldn't
        // be in this path.
        //
        // It is also possible for the request to be less than a single page!
        // In this case, we'll only do the first I/O and skip ths second, and
        // we'll slightly modify our buffer copy to only cover 1 page.
        //
        if (DeltaBase)
        {
            /* Create an MDL for the read transfer */
            ReadMdl = IoAllocateMdl(SystemBuffer, PAGE_SIZE, TRUE, FALSE, NULL);
            MmBuildMdlForNonPagedPool(ReadMdl),
            ReadMdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

            /* We have an offset from a page boundary, so we need to do a read */
            ReadSector = SectorBase;
            Status = IoPageRead(FileObject,
                                ReadMdl,
                                &ReadSector,
                                &Event,
                                &IoStatusBlock);
            if (Status == STATUS_PENDING)
            {
                /* Do the wait */
                KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }

            /* This shouldn't fail */
            ASSERT(NT_SUCCESS(Status));

            /* Free the MDL */
            IoFreeMdl(ReadMdl);
        }

        /* Now check if we read up to a page boundary, or have an offset */
        if ((DeltaEnd))// && (Length > PAGE_SIZE))
        {
            /* Create an MDL for the read transfer */
            ReadMdl = IoAllocateMdl(SystemBuffer + ROUND_DOWN(Length, PAGE_SIZE),
                                    PAGE_SIZE,
                                    TRUE,
                                    FALSE,
                                    NULL);
            MmBuildMdlForNonPagedPool(ReadMdl),
            ReadMdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

            /* We have an offset from a page boundary, so we need to do a read */
            ReadSector.QuadPart += ROUND_DOWN(Length, PAGE_SIZE);
            Status = IoPageRead(FileObject,
                                ReadMdl,
                                &SectorBase,
                                &Event,
                                &IoStatusBlock);
            if (Status == STATUS_PENDING)
            {
                /* Do the wait */
                KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
                Status = IoStatusBlock.Status;
            }

            /* This shouldn't fail */
            ASSERT(NT_SUCCESS(Status));

            /* Free the MDL */
            IoFreeMdl(ReadMdl);
        }

        /* Okay, now we have the original data, write our modified data on top */
        ASSERT((SystemBuffer + DeltaBase + Length) <= (SystemBuffer + AlignSize));
        RtlCopyMemory(SystemBuffer + DeltaBase, Buffer, Length);
    }

    /* And write the modified contents back */
    Status = IoSynchronousPageWrite(FileObject,
                                    Mdl,
                                    &SectorBase,
                                    &Event,
                                    &IoStatusBlock);
    if (Status == STATUS_PENDING)
    {
        /* Do the wait */
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) DPRINT1("Status: %lx\n", Status);
    ASSERT(NT_SUCCESS(Status));

    /* Free our buffer */
    if (!DirectWrite) ExFreePool(SystemBuffer);

    /* Free the MDL */
    IoFreeMdl(Mdl);

    /* FIXME:? Check if we wrote less than the caller wanted */
    return TRUE;
}

VOID
NTAPI
CcFastCopyWrite(IN PFILE_OBJECT FileObject,
                IN ULONG FileOffset,
                IN ULONG Length,
                IN PVOID Buffer)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcCanIWrite(IN PFILE_OBJECT FileObject,
            IN ULONG BytesToWrite,
            IN BOOLEAN Wait,
            IN UCHAR Retrying)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
CcDeferWrite(IN PFILE_OBJECT FileObject,
             IN PCC_POST_DEFERRED_WRITE PostRoutine,
             IN PVOID Context1,
             IN PVOID Context2,
             IN ULONG BytesToWrite,
             IN BOOLEAN Retrying)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
