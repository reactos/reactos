/*
 * Copyright (c) 1999, 2000
 *  Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef _MSC_VER
#include "stdarg.h"
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"
#else
#include <ddk/ntddk.h>
#include <net/ndis.h>
//#define PsGetCurrentProcess() IoGetCurrentProcess()
#define PsGetCurrentThread() ((PETHREAD) (KeGetCurrentThread()))
#endif

#include "debug.h"
#include "packet.h"
#include "win_bpf.h"

//-------------------------------------------------------------------

NTSTATUS
NPF_OpenDumpFile(POPEN_INSTANCE Open , PUNICODE_STRING fileName, BOOLEAN Append)
{
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWCHAR PathPrefix;
    USHORT PathLen;
    UNICODE_STRING FullFileName;
    ULONG FullFileNameLength;
    PDEVICE_OBJECT fsdDevice;

    FILE_STANDARD_INFORMATION StandardInfo;
    
    IF_LOUD(DbgPrint("NPF: OpenDumpFile.\n");)

    if(fileName->Buffer[0] == L'\\' &&
        fileName->Buffer[1] == L'?' &&
        fileName->Buffer[2] == L'?' &&
        fileName->Buffer[3] == L'\\'
    ){
        PathLen = 0;
    }
    else{
        PathPrefix = L"\\??\\";
        PathLen = 8;
    }
    
    // Insert the correct path prefix.
    FullFileNameLength = PathLen + fileName->MaximumLength;
    
#define NPF_TAG_FILENAME  TAG('0', 'D', 'W', 'A')
    FullFileName.Buffer = ExAllocatePoolWithTag(NonPagedPool, 
        FullFileNameLength,
        NPF_TAG_FILENAME);
    
    if (FullFileName.Buffer == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        return ntStatus;
    }
    
    FullFileName.Length = PathLen;
    FullFileName.MaximumLength = (USHORT)FullFileNameLength;
    
    if(PathLen)
        RtlMoveMemory (FullFileName.Buffer, PathPrefix, PathLen);
    
    RtlAppendUnicodeStringToString (&FullFileName, fileName);
    
    IF_LOUD(DbgPrint( "Packet: Attempting to open %wZ\n", &FullFileName);)
    
    InitializeObjectAttributes ( &ObjectAttributes,
        &FullFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );
    
    // Create the dump file
    ntStatus = ZwCreateFile( &Open->DumpFileHandle,
        SYNCHRONIZE | FILE_WRITE_DATA,
        &ObjectAttributes,
        &IoStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        (Append)?FILE_OPEN_IF:FILE_SUPERSEDE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0 );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        IF_LOUD(DbgPrint("NPF: Error opening file %x\n", ntStatus);)
        
        ExFreePool(FullFileName.Buffer);
        Open->DumpFileHandle=NULL;
        ntStatus = STATUS_NO_SUCH_FILE;
        return ntStatus;
    }
    
    ExFreePool(FullFileName.Buffer);
    
    ntStatus = ObReferenceObjectByHandle(Open->DumpFileHandle,
        FILE_WRITE_ACCESS,
#ifndef __GNUC__
        *IoFileObjectType,
#else
        IoFileObjectType,
#endif
        KernelMode,
        (PVOID)&Open->DumpFileObject,
        0);
    
    if ( !NT_SUCCESS( ntStatus ) )
    {
        IF_LOUD(DbgPrint("NPF: Error creating file, status=%x\n", ntStatus);)
            
        ZwClose( Open->DumpFileHandle );
        Open->DumpFileHandle=NULL;
        
        ntStatus = STATUS_NO_SUCH_FILE;
        return ntStatus;
    }
    
    fsdDevice = IoGetRelatedDeviceObject(Open->DumpFileObject);

    IF_LOUD(DbgPrint("NPF: Dump: write file created succesfully, status=%d \n",ntStatus);)

    return ntStatus;
}   

//-------------------------------------------------------------------

NTSTATUS
NPF_StartDump(POPEN_INSTANCE Open)
{
    NTSTATUS ntStatus;
    struct packet_file_header hdr;
    IO_STATUS_BLOCK IoStatus;
    NDIS_REQUEST pRequest;
    ULONG MediaType;
    OBJECT_ATTRIBUTES ObjectAttributes;

    IF_LOUD(DbgPrint("NPF: StartDump.\n");)

    // Init the file header
    hdr.magic = TCPDUMP_MAGIC;
    hdr.version_major = PCAP_VERSION_MAJOR;
    hdr.version_minor = PCAP_VERSION_MINOR;
    hdr.thiszone = 0; /*Currently not set*/
    hdr.snaplen = 1514;
    hdr.sigfigs = 0;

    // Detect the medium type
    switch (Open->Medium){
        
    case NdisMediumWan:
        hdr.linktype = DLT_EN10MB;
        break;
        
    case NdisMedium802_3:
        hdr.linktype = DLT_EN10MB;
        break;
        
    case NdisMediumFddi:
        hdr.linktype = DLT_FDDI;
        break;
        
    case NdisMedium802_5:           
        hdr.linktype = DLT_IEEE802; 
        break;
        
    case NdisMediumArcnet878_2:
        hdr.linktype = DLT_ARCNET;
        break;
        
    case NdisMediumAtm:
        hdr.linktype = DLT_ATM_RFC1483;
        break;
        
    default:
        hdr.linktype = DLT_EN10MB;
    }

    // Write the header.
    // We can use ZwWriteFile because we are in the context of the application
    ntStatus = ZwWriteFile(Open->DumpFileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatus,
        &hdr,
        sizeof(hdr),
        NULL,
        NULL );

    
    if ( !NT_SUCCESS( ntStatus ) )
    {
        IF_LOUD(DbgPrint("NPF: Error dumping file %x\n", ntStatus);)
        
        ZwClose( Open->DumpFileHandle );
        Open->DumpFileHandle=NULL;
        
        ntStatus = STATUS_NO_SUCH_FILE;
        return ntStatus;
    }

    Open->DumpOffset.QuadPart=24;
            
    ntStatus = PsCreateSystemThread(&Open->DumpThreadHandle,
        THREAD_ALL_ACCESS,
        (ACCESS_MASK)0L,
        0,
        0,
        (PKSTART_ROUTINE)NPF_DumpThread,
        Open);
    
    if ( !NT_SUCCESS( ntStatus ) )
    {
        IF_LOUD(DbgPrint("NPF: Error creating dump thread, status=%x\n", ntStatus);)
        
        ZwClose( Open->DumpFileHandle );
        Open->DumpFileHandle=NULL;

        return ntStatus;
    }  
    ntStatus = ObReferenceObjectByHandle(Open->DumpThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &Open->DumpThreadObject,
        0);
    if ( !NT_SUCCESS( ntStatus ) )
    {
        IF_LOUD(DbgPrint("NPF: Error creating dump thread, status=%x\n", ntStatus);)
        
        ObDereferenceObject(Open->DumpFileObject);
        ZwClose( Open->DumpFileHandle );
        Open->DumpFileHandle=NULL;

        return ntStatus;
    }  
  
    return ntStatus;
    
}

//-------------------------------------------------------------------
// Dump Thread
//-------------------------------------------------------------------

VOID NPF_DumpThread(POPEN_INSTANCE Open)
{
    ULONG       FrozenNic;

    IF_LOUD(DbgPrint("NPF: In the work routine.  Parameter = 0x%0x\n",Open);)

    while(TRUE){

        // Wait until some packets arrive or the timeout expires
        NdisWaitEvent(&Open->DumpEvent, 5000);  

        IF_LOUD(DbgPrint("NPF: Worker Thread - event signalled\n");)
            
        if(Open->DumpLimitReached ||
            Open->BufSize==0){      // BufSize=0 means that this instance was closed, or that the buffer is too
                                    // small for any capture. In both cases it is better to end the dump

            IF_LOUD(DbgPrint("NPF: Worker Thread - Exiting happily\n");)
            IF_LOUD(DbgPrint("Thread: Dumpoffset=%I64d\n",Open->DumpOffset.QuadPart);)

            PsTerminateSystemThread(STATUS_SUCCESS);
            return;
        }
        
        NdisResetEvent(&Open->DumpEvent);

        // Write the content of the buffer to the file
        if(NPF_SaveCurrentBuffer(Open) != STATUS_SUCCESS){
            PsTerminateSystemThread(STATUS_SUCCESS);
            return;
        }
    
    }
    
}

//-------------------------------------------------------------------

NTSTATUS NPF_SaveCurrentBuffer(POPEN_INSTANCE Open)
{
    UINT        Thead;
    UINT        Ttail;
    UINT        TLastByte;
    PUCHAR      CurrBuff;
    NTSTATUS    ntStatus;
    IO_STATUS_BLOCK IoStatus;
    PMDL        lMdl;
    UINT        SizeToDump;

    
    Thead=Open->Bhead;
    Ttail=Open->Btail;
    TLastByte=Open->BLastByte;
    
    IF_LOUD(DbgPrint("NPF: NPF_SaveCurrentBuffer.\n");)

    // Get the address of the buffer
    CurrBuff=Open->Buffer;
    //
    // Fill the application buffer
    //
    if( Ttail < Thead )
    {
        if(Open->MaxDumpBytes &&
            (UINT)Open->DumpOffset.QuadPart + GetBuffOccupation(Open) > Open->MaxDumpBytes)
        {
            // Size limit reached
            UINT PktLen;
            
            SizeToDump = 0;
            
            // Scan the buffer to detect the exact amount of data to save
            while(TRUE){
                PktLen = ((struct sf_pkthdr*)(CurrBuff + Thead + SizeToDump))->caplen + sizeof(struct sf_pkthdr);
                
                if((UINT)Open->DumpOffset.QuadPart + SizeToDump + PktLen > Open->MaxDumpBytes)
                    break;
                
                SizeToDump += PktLen;
            }
            
        }
        else
            SizeToDump = TLastByte-Thead;
        
        lMdl=IoAllocateMdl(CurrBuff+Thead, SizeToDump, FALSE, FALSE, NULL);
        if (lMdl == NULL)
        {
            // No memory: stop dump
            IF_LOUD(DbgPrint("NPF: dump thread: Failed to allocate Mdl\n");)
            return STATUS_UNSUCCESSFUL;
        }
        
        MmBuildMdlForNonPagedPool(lMdl);
        
        // Write to disk
        NPF_WriteDumpFile(Open->DumpFileObject,
            &Open->DumpOffset,
            SizeToDump,
            lMdl,
            &IoStatus);
        
        IoFreeMdl(lMdl);
        
        if(!NT_SUCCESS(IoStatus.Status)){
            // Error
            return STATUS_UNSUCCESSFUL;
        }
        
        if(SizeToDump != TLastByte-Thead){
            // Size limit reached.
            Open->DumpLimitReached = TRUE;
    
            // Awake the application
            KeSetEvent(Open->ReadEvent,0,FALSE);

            return STATUS_UNSUCCESSFUL;
        }
        
        // Update the packet buffer
        Open->DumpOffset.QuadPart+=(TLastByte-Thead);
        Open->BLastByte=Ttail;
        Open->Bhead=0;
    }

    if( Ttail > Thead ){
        
        if(Open->MaxDumpBytes &&
            (UINT)Open->DumpOffset.QuadPart + GetBuffOccupation(Open) > Open->MaxDumpBytes)
        {
            // Size limit reached
            UINT PktLen;
                        
            SizeToDump = 0;
            
            // Scan the buffer to detect the exact amount of data to save
            while(Thead + SizeToDump < Ttail){

                PktLen = ((struct sf_pkthdr*)(CurrBuff + Thead + SizeToDump))->caplen + sizeof(struct sf_pkthdr);
                
                if((UINT)Open->DumpOffset.QuadPart + SizeToDump + PktLen > Open->MaxDumpBytes)
                    break;
                
                SizeToDump += PktLen;
            }
            
        }
        else
            SizeToDump = Ttail-Thead;
                
        lMdl=IoAllocateMdl(CurrBuff+Thead, SizeToDump, FALSE, FALSE, NULL);
        if (lMdl == NULL)
        {
            // No memory: stop dump
            IF_LOUD(DbgPrint("NPF: dump thread: Failed to allocate Mdl\n");)
            return STATUS_UNSUCCESSFUL;
        }
        
        MmBuildMdlForNonPagedPool(lMdl);
        
        // Write to disk
        NPF_WriteDumpFile(Open->DumpFileObject,
            &Open->DumpOffset,
            SizeToDump,
            lMdl,
            &IoStatus);
        
        IoFreeMdl(lMdl);
        
        if(!NT_SUCCESS(IoStatus.Status)){
            // Error
            return STATUS_UNSUCCESSFUL;
        }
        
        if(SizeToDump != Ttail-Thead){
            // Size limit reached.
            Open->DumpLimitReached = TRUE;

            // Awake the application
            KeSetEvent(Open->ReadEvent,0,FALSE);
            
            return STATUS_UNSUCCESSFUL;
        }
        
        // Update the packet buffer
        Open->DumpOffset.QuadPart+=(Ttail-Thead);           
        Open->Bhead=Ttail;
        
    }

    return STATUS_SUCCESS;
}

//-------------------------------------------------------------------

NTSTATUS NPF_CloseDumpFile(POPEN_INSTANCE Open){
    NTSTATUS    ntStatus;
    IO_STATUS_BLOCK IoStatus;
    PMDL        WriteMdl;
    PUCHAR      VMBuff;
    UINT        VMBufLen;


    IF_LOUD(DbgPrint("NPF: NPF_CloseDumpFile.\n");)
    IF_LOUD(DbgPrint("Dumpoffset=%d\n",Open->DumpOffset.QuadPart);)

DbgPrint("1\n");
    // Consistency check
    if(Open->DumpFileHandle == NULL)
        return STATUS_UNSUCCESSFUL;

DbgPrint("2\n");
    ZwClose( Open->DumpFileHandle );

    ObDereferenceObject(Open->DumpFileObject);
/*
    if(Open->DumpLimitReached == TRUE)
        // Limit already reached: don't save the rest of the buffer.
        return STATUS_SUCCESS;
*/
DbgPrint("3\n");

    NPF_OpenDumpFile(Open,&Open->DumpFileName, TRUE);

    // Flush the buffer to file 
    NPF_SaveCurrentBuffer(Open);

    // Close The file
    ObDereferenceObject(Open->DumpFileObject);
    ZwClose( Open->DumpFileHandle );
    
    Open->DumpFileHandle = NULL;

    ObDereferenceObject(Open->DumpFileObject);

    return STATUS_SUCCESS;
}

//-------------------------------------------------------------------

#ifndef __GNUC__
static NTSTATUS 
#else
NTSTATUS STDCALL
#endif
PacketDumpCompletion(PDEVICE_OBJECT DeviceObject,
                                PIRP Irp,
                                PVOID Context)
{

    // Copy the status information back into the "user" IOSB
    *Irp->UserIosb = Irp->IoStatus;
    
    // Wake up the mainline code
    KeSetEvent(Irp->UserEvent, 0, FALSE);
          
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//-------------------------------------------------------------------

VOID NPF_WriteDumpFile(PFILE_OBJECT FileObject,
                                PLARGE_INTEGER Offset,
                                ULONG Length,
                                PMDL Mdl,
                                PIO_STATUS_BLOCK IoStatusBlock)
{
    PIRP irp;
    KEVENT event;
    PIO_STACK_LOCATION ioStackLocation;
    PDEVICE_OBJECT fsdDevice = IoGetRelatedDeviceObject(FileObject);
    NTSTATUS Status;
 
    // Set up the event we'll use
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    
    // Allocate and build the IRP we'll be sending to the FSD
    irp = IoAllocateIrp(fsdDevice->StackSize, FALSE);

    if (!irp) {
        // Allocation failed, presumably due to memory allocation failure
        IoStatusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
        IoStatusBlock->Information = 0;

        return;
    }

    irp->MdlAddress = Mdl;
    irp->UserEvent = &event;
    irp->UserIosb = IoStatusBlock;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject= FileObject;
    irp->RequestorMode = KernelMode;

    // Indicate that this is a WRITE operation
    irp->Flags = IRP_WRITE_OPERATION;

    // Set up the next I/O stack location
    ioStackLocation = IoGetNextIrpStackLocation(irp);
    ioStackLocation->MajorFunction = IRP_MJ_WRITE;
    ioStackLocation->MinorFunction = 0;
    ioStackLocation->DeviceObject = fsdDevice;
    ioStackLocation->FileObject = FileObject;
    IoSetCompletionRoutine(irp, PacketDumpCompletion, 0, TRUE, TRUE, TRUE);
    ioStackLocation->Parameters.Write.Length = Length;
    ioStackLocation->Parameters.Write.ByteOffset = *Offset;


    // Send it on.  Ignore the return code
    (void) IoCallDriver(fsdDevice, irp);

    // Wait for the I/O to complete.
    KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, 0);

    // Free the IRP now that we are done with it
    IoFreeIrp(irp);

    return;
}
