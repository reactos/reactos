/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/cdfs/rw.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))


/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsReadFile(PDEVICE_EXTENSION DeviceExt,
             PFILE_OBJECT FileObject,
             PUCHAR Buffer,
             ULONG Length,
             ULONG ReadOffset,
             ULONG IrpFlags,
             PULONG LengthRead)
             /*
             * FUNCTION: Reads data from a file
             */
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFCB Fcb;
    ULONG ToRead = Length;

    DPRINT("CdfsReadFile(ReadOffset %lu  Length %lu)\n", ReadOffset, Length);

    *LengthRead = 0;

    if (Length == 0)
        return(STATUS_SUCCESS);

    Fcb = (PFCB)FileObject->FsContext;

    if (ReadOffset >= Fcb->Entry.DataLengthL)
        return(STATUS_END_OF_FILE);
        
    if (ReadOffset + Length > Fcb->Entry.DataLengthL)
        ToRead = Fcb->Entry.DataLengthL - ReadOffset;

    DPRINT("Reading %u bytes at %u\n", Length, ReadOffset);

    if (!(IrpFlags & (IRP_NOCACHE|IRP_PAGING_IO)))
    {
        LARGE_INTEGER FileOffset;
        IO_STATUS_BLOCK IoStatus;
        CC_FILE_SIZES FileSizes;
        
        DPRINT("Using cache\n");

        if (FileObject->PrivateCacheMap == NULL)
        {
            FileSizes.AllocationSize = Fcb->RFCB.AllocationSize;
            FileSizes.FileSize = Fcb->RFCB.FileSize;
            FileSizes.ValidDataLength = Fcb->RFCB.ValidDataLength;

            DPRINT("Attach FCB to File: Size %08x%08x\n", 
                Fcb->RFCB.ValidDataLength.HighPart,
                Fcb->RFCB.ValidDataLength.LowPart);

            _SEH2_TRY
            {
                CcInitializeCacheMap(FileObject,
                    &FileSizes,
                    FALSE,
                    &(CdfsGlobalData->CacheMgrCallbacks),
                    Fcb);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                return _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }

        FileOffset.QuadPart = (LONGLONG)ReadOffset;
        _SEH2_TRY
        {
            CcCopyRead(FileObject,
                &FileOffset,
                ToRead,
                TRUE,
                Buffer,
                &IoStatus);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus.Information = 0;
            IoStatus.Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        *LengthRead = IoStatus.Information;

        Status = IoStatus.Status;
    }
    else
    {
        ULONG ActualReadOffset = ROUND_DOWN(ReadOffset, BLOCKSIZE);
        ULONG nBlocks = (ROUND_UP(ReadOffset + ToRead, BLOCKSIZE) - ActualReadOffset) / BLOCKSIZE;
        PUCHAR PageBuf;
        BOOLEAN bFreeBuffer = FALSE;
        if ((ReadOffset % BLOCKSIZE) != 0 || (ToRead % BLOCKSIZE) != 0)
        {
            PageBuf = ExAllocatePool(NonPagedPool, nBlocks * BLOCKSIZE);
            if (!PageBuf)
            {
                return STATUS_NO_MEMORY;
            }
            bFreeBuffer = TRUE;
        }
        else
        {
            PageBuf = Buffer;
        }
        Status = CdfsReadSectors(DeviceExt->StorageDevice,
            Fcb->Entry.ExtentLocationL + (ActualReadOffset / BLOCKSIZE),
            nBlocks,
            (PVOID)PageBuf,
            FALSE);
        
        if(NT_SUCCESS(Status))
        {
            *LengthRead = ToRead;
            if(bFreeBuffer)
            {
                /* Copy what we've got */
                RtlCopyMemory(Buffer, PageBuf + (ReadOffset - ActualReadOffset), ToRead);
            }
            /* Zero out the rest */
            if(ToRead != Length)
                RtlZeroMemory(Buffer + ToRead, Length - ToRead);
        }
        
        if(bFreeBuffer)
            ExFreePool(PageBuf);
    }
    
    return Status;
}


NTSTATUS NTAPI
CdfsRead(PDEVICE_OBJECT DeviceObject,
         PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    PVOID Buffer = NULL;
    ULONG ReadLength;
    LARGE_INTEGER ReadOffset;
    ULONG ReturnedReadLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CdfsRead(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    DeviceExt = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;

    ReadLength = Stack->Parameters.Read.Length;
    ReadOffset = Stack->Parameters.Read.ByteOffset;
    if (ReadLength) Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);

    Status = CdfsReadFile(DeviceExt,
        FileObject,
        Buffer,
        ReadLength,
        ReadOffset.u.LowPart,
        Irp->Flags,
        &ReturnedReadLength);
    if (NT_SUCCESS(Status))
    {
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            FileObject->CurrentByteOffset.QuadPart =
                ReadOffset.QuadPart + ReturnedReadLength;
        }
        Irp->IoStatus.Information = ReturnedReadLength;
    }
    else
    {
        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return(Status);
}


NTSTATUS NTAPI
CdfsWrite(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    DPRINT("CdfsWrite(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    return(STATUS_NOT_SUPPORTED);
}

/* EOF */
