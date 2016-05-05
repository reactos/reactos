#include "btrfs_drv.h"

enum read_data_status {
    ReadDataStatus_Pending,
    ReadDataStatus_Success,
    ReadDataStatus_Cancelling,
    ReadDataStatus_Cancelled,
    ReadDataStatus_Error,
    ReadDataStatus_CRCError,
    ReadDataStatus_MissingDevice
};

struct read_data_context;

typedef struct {
    struct read_data_context* context;
    UINT8* buf;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_data_status status;
} read_data_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
    UINT32 buflen;
    UINT64 num_stripes;
    UINT64 type;
    read_data_stripe* stripes;
} read_data_context;

static NTSTATUS STDCALL read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_data_stripe* stripe = conptr;
    read_data_context* context = (read_data_context*)stripe->context;
    UINT64 i;
    BOOL complete;
    
    if (stripe->status == ReadDataStatus_Cancelling) {
        stripe->status = ReadDataStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        // FIXME - calculate and compare checksum
        
        stripe->status = ReadDataStatus_Success;
            
        for (i = 0; i < context->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Pending) {
                context->stripes[i].status = ReadDataStatus_Cancelling;
                IoCancelIrp(context->stripes[i].Irp);
            }
        }
            
        goto end;
    } else {
        stripe->status = ReadDataStatus_Error;
    }
    
end:
    complete = TRUE;
        
    for (i = 0; i < context->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Pending || context->stripes[i].status == ReadDataStatus_Cancelling) {
            complete = FALSE;
            break;
        }
    }
    
    if (complete)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS STDCALL read_data(device_extension* Vcb, UINT64 addr, UINT32 length, UINT8* buf) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_data_context* context;
    UINT64 i/*, type*/, offset;
    NTSTATUS Status;
    device** devices;
    
    // FIXME - make this work with RAID
    
    if (Vcb->log_to_phys_loaded) {
        chunk* c = get_chunk_from_address(Vcb, addr);
        
        if (!c) {
            ERR("get_chunk_from_address failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        ci = c->chunk_item;
        offset = c->offset;
        devices = c->devices;
    }
    
//     if (ci->type & BLOCK_FLAG_DUPLICATE) {
//         type = BLOCK_FLAG_DUPLICATE;
//     } else if (ci->type & BLOCK_FLAG_RAID0) {
//         FIXME("RAID0 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID1) {
//         FIXME("RAID1 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID10) {
//         FIXME("RAID10 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID5) {
//         FIXME("RAID5 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else if (ci->type & BLOCK_FLAG_RAID6) {
//         FIXME("RAID6 not yet supported\n");
//         return STATUS_NOT_IMPLEMENTED;
//     } else { // SINGLE
//         type = 0;
//     }

    cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context, sizeof(read_data_context));
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    context->stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe) * ci->num_stripes, ALLOC_TAG);
    if (!context->stripes) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context->stripes, sizeof(read_data_stripe) * ci->num_stripes);
    
    context->buflen = length;
    context->num_stripes = ci->num_stripes;
//     context->type = type;
    
    // FIXME - for RAID, check beforehand whether there's enough devices to satisfy request
    
    for (i = 0; i < ci->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        if (!devices[i]) {
            context->stripes[i].status = ReadDataStatus_MissingDevice;
            context->stripes[i].buf = NULL;
        } else {
            context->stripes[i].context = (struct read_data_context*)context;
            context->stripes[i].buf = ExAllocatePoolWithTag(NonPagedPool, length, ALLOC_TAG);
            
            if (!context->stripes[i].buf) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            context->stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, FALSE);
            
            if (!context->stripes[i].Irp) {
                ERR("IoAllocateIrp failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;
            
            if (devices[i]->devobj->Flags & DO_BUFFERED_IO) {
                FIXME("FIXME - buffered IO\n");
            } else if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                context->stripes[i].Irp->MdlAddress = IoAllocateMdl(context->stripes[i].buf, length, FALSE, FALSE, NULL);
                if (!context->stripes[i].Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
                
                MmProbeAndLockPages(context->stripes[i].Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                context->stripes[i].Irp->UserBuffer = context->stripes[i].buf;
            }

            IrpSp->Parameters.Read.Length = length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = addr - offset + cis[i].offset;
            
            context->stripes[i].Irp->UserIosb = &context->stripes[i].iosb;
            
            IoSetCompletionRoutine(context->stripes[i].Irp, read_data_completion, &context->stripes[i], TRUE, TRUE, TRUE);

            context->stripes[i].status = ReadDataStatus_Pending;
        }
    }
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status != ReadDataStatus_MissingDevice) {
            IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
        }
    }

    KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
    
    // FIXME - if checksum error, write good data over bad
    
    // check if any of the stripes succeeded
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Success) {
            RtlCopyMemory(buf, context->stripes[i].buf, length);
            Status = STATUS_SUCCESS;
            goto exit;
        }
    }
    
    // if not, see if we got a checksum error
    
//     for (i = 0; i < ci->num_stripes; i++) {
//         if (context->stripes[i].status == ReadDataStatus_CRCError) {
//             WARN("stripe %llu had a checksum error\n", i);
//             
//             Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
//             goto exit;
//         }
//     }
    
    // failing that, return the first error we encountered
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            Status = context->stripes[i].iosb.Status;
            goto exit;
        }
    }
    
    // if we somehow get here, return STATUS_INTERNAL_ERROR
    
    Status = STATUS_INTERNAL_ERROR;

exit:

    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].Irp) {
            if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                MmUnlockPages(context->stripes[i].Irp->MdlAddress);
                IoFreeMdl(context->stripes[i].Irp->MdlAddress);
            }
            IoFreeIrp(context->stripes[i].Irp);
        }
        
        if (context->stripes[i].buf)
            ExFreePool(context->stripes[i].buf);
    }

    ExFreePool(context->stripes);
    ExFreePool(context);
        
    return Status;
}

static NTSTATUS STDCALL read_stream(fcb* fcb, UINT8* data, UINT64 start, ULONG length, ULONG* pbr) {
    UINT8* xattrdata;
    UINT16 xattrlen;
    ULONG readlen;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);
    
    if (pbr) *pbr = 0;
    
    if (!get_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, &xattrdata, &xattrlen)) {
        ERR("get_xattr failed\n");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    if (start >= xattrlen) {
        TRACE("tried to read beyond end of stream\n");
        Status = STATUS_END_OF_FILE;
        goto end;
    }
    
    if (length == 0) {
        WARN("tried to read zero bytes\n");
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    if (start + length < xattrlen)
        readlen = length;
    else
        readlen = (ULONG)xattrlen - (ULONG)start;
    
    RtlCopyMemory(data + start, xattrdata, readlen);
    
    if (pbr) *pbr = readlen;
    
    Status = STATUS_SUCCESS;
    
end:
    ExFreePool(xattrdata);
    
    return Status;
}

NTSTATUS STDCALL read_file(device_extension* Vcb, root* subvol, UINT64 inode, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr) {
    KEY searchkey;
    NTSTATUS Status;
    traverse_ptr tp, next_tp;
    EXTENT_DATA* ed;
    UINT64 bytes_read = 0;
    
    TRACE("(%p, %llx, %llx, %p, %llx, %llx, %p)\n", Vcb, subvol->id, inode, data, start, length, pbr);
    
    if (pbr)
        *pbr = 0;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start;

    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto exit;
    }

    if (tp.item->key.obj_id < searchkey.obj_id || tp.item->key.obj_type < searchkey.obj_type) {
        if (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
            tp = next_tp;
            
            TRACE("moving on to %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("couldn't find EXTENT_DATA for inode %llx in subvol %llx\n", searchkey.obj_id, subvol->id);
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
    if (tp.item->key.offset > start) {
        ERR("first EXTENT_DATA was after offset\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit;
    }
    
//     while (TRUE) {
//         BOOL foundnext = find_next_item(Vcb, &tp, &next_tp, NULL, FALSE);
//         
//         if (!foundnext || next_tp.item->key.obj_id != inode ||
//             next_tp.item->key.obj_type != TYPE_EXTENT_DATA || next_tp.item->key.offset > start) {
//             if (foundnext)
//                 free_traverse_ptr(&next_tp);
//             
//             break;
//         }
//         
//         free_traverse_ptr(&tp);
//         tp = next_tp;
//     }
    
    do {
        UINT64 len;
        EXTENT_DATA2* ed2;
        
        ed = (EXTENT_DATA*)tp.item->data;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }
        
        ed2 = (EXTENT_DATA2*)ed->data;
        
        len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
        
        if (tp.item->key.offset + len < start) {
            ERR("Tried to read beyond end of file\n");
            Status = STATUS_END_OF_FILE;
            goto exit;
        }
        
        if (ed->compression != BTRFS_COMPRESSION_NONE) {
            FIXME("FIXME - compression not yet supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
        }
        
        if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
            WARN("Encryption not supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
        }
        
        if (ed->encoding != BTRFS_ENCODING_NONE) {
            WARN("Other encodings not supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto exit;
        }
        
        switch (ed->type) {
            case EXTENT_TYPE_INLINE:
            {
                UINT64 off = start + bytes_read - tp.item->key.offset;
                UINT64 read = len - off;
                
                if (read > length) read = length;
                
                RtlCopyMemory(data + bytes_read, &ed->data[off], read);
                
                bytes_read += read;
                length -= read;
                break;
            }
            
            case EXTENT_TYPE_REGULAR:
            {
                UINT64 off = start + bytes_read - tp.item->key.offset;
                UINT32 to_read, read;
                UINT8* buf;
                
                read = len - off;
                if (read > length) read = length;
                
                if (ed2->address == 0) {
                    RtlZeroMemory(data + bytes_read, read);
                } else {
                    to_read = sector_align(read, Vcb->superblock.sector_size);
                    
                    buf = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
                    
                    if (!buf) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto exit;
                    }
                    
                    // FIXME - load checksums
                    
                    Status = read_data(Vcb, ed2->address + ed2->offset + off, to_read, buf);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_data returned %08x\n", Status);
                        ExFreePool(buf);
                        goto exit;
                    }
                    
                    RtlCopyMemory(data + bytes_read, buf, read);
                    
                    ExFreePool(buf);
                }
                
                bytes_read += read;
                length -= read;
                
                break;
            }
           
            case EXTENT_TYPE_PREALLOC:
            {
                UINT64 off = start + bytes_read - tp.item->key.offset;
                UINT32 read = len - off;
                
                if (read > length) read = length;

                RtlZeroMemory(data + bytes_read, read);

                bytes_read += read;
                length -= read;
                
                break;
            }
                
            default:
                WARN("Unsupported extent data type %u\n", ed->type);
                Status = STATUS_NOT_IMPLEMENTED;
                goto exit;
        }
        
        if (length > 0) {
            BOOL foundnext = find_next_item(Vcb, &tp, &next_tp, FALSE);
            
            if (!foundnext)
                break;
            else if (next_tp.item->key.obj_id != inode ||
                next_tp.item->key.obj_type != TYPE_EXTENT_DATA ||
                next_tp.item->key.offset != tp.item->key.offset + len
            ) {
                break;
            } else {
                TRACE("found next key (%llx,%x,%llx)\n", next_tp.item->key.obj_id, next_tp.item->key.obj_type, next_tp.item->key.offset);
                
                tp = next_tp;
            }
        } else
            break;
    } while (TRUE);
    
    Status = STATUS_SUCCESS;
    if (pbr)
        *pbr = bytes_read;
    
exit:
    return Status;
}

NTSTATUS STDCALL drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    UINT8* data;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    UINT64 start;
    ULONG length, bytes_read;
    NTSTATUS Status;
    BOOL top_level;
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    TRACE("read\n");
    
    Irp->IoStatus.Information = 0;
    
    if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
        CcMdlReadComplete(IrpSp->FileObject, Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        Status = STATUS_SUCCESS;
        bytes_read = 0;
        
        goto exit;
    }
    
    start = IrpSp->Parameters.Read.ByteOffset.QuadPart;
    length = IrpSp->Parameters.Read.Length;
    bytes_read = 0;
    
    if (!fcb || !fcb->Vcb || !fcb->subvol) {
        Status = STATUS_INTERNAL_ERROR; // FIXME - invalid param error?
        goto exit;
    }
    
    TRACE("file = %S (fcb = %p)\n", file_desc(FileObject), fcb);
    TRACE("offset = %llx, length = %x\n", start, length);
    TRACE("paging_io = %s, no cache = %s\n", Irp->Flags & IRP_PAGING_IO ? "TRUE" : "FALSE", Irp->Flags & IRP_NOCACHE ? "TRUE" : "FALSE");

    if (fcb->type == BTRFS_TYPE_DIRECTORY) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }
    
    if (!(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForReadAccess(&fcb->lock, Irp)) {
        WARN("tried to read locked region\n");
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto exit;
    }
    
    if (length == 0) {
        WARN("tried to read zero bytes\n");
        Status = STATUS_SUCCESS;
        goto exit;
    }
    
    if (start >= fcb->Header.FileSize.QuadPart) {
        TRACE("tried to read with offset after file end (%llx >= %llx)\n", start, fcb->Header.FileSize.QuadPart);
        Status = STATUS_END_OF_FILE;
        goto exit;
    }
    
    TRACE("FileObject %p fcb %p FileSize = %llx st_size = %llx (%p)\n", FileObject, fcb, fcb->Header.FileSize.QuadPart, fcb->inode_item.st_size, &fcb->inode_item.st_size);
//     int3;
    
    if (Irp->Flags & IRP_NOCACHE || !(IrpSp->MinorFunction & IRP_MN_MDL)) {
        data = map_user_buffer(Irp);
        
        if (Irp->MdlAddress && !data) {
            ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }
        
        if (length + start > fcb->Header.ValidDataLength.QuadPart) {
            RtlZeroMemory(data + (fcb->Header.ValidDataLength.QuadPart - start), length - (fcb->Header.ValidDataLength.QuadPart - start));
            length = fcb->Header.ValidDataLength.QuadPart - start;
        }
    }
        
    if (!(Irp->Flags & IRP_NOCACHE)) {
        BOOL wait;
        
        Status = STATUS_SUCCESS;
        
        try {
            if (!FileObject->PrivateCacheMap) {
                CC_FILE_SIZES ccfs;
                
                ccfs.AllocationSize = fcb->Header.AllocationSize;
                ccfs.FileSize = fcb->Header.FileSize;
                ccfs.ValidDataLength = fcb->Header.ValidDataLength;
                
                TRACE("calling CcInitializeCacheMap (%llx, %llx, %llx)\n",
                            ccfs.AllocationSize.QuadPart, ccfs.FileSize.QuadPart, ccfs.ValidDataLength.QuadPart);
                CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, FileObject);

                CcSetReadAheadGranularity(FileObject, READ_AHEAD_GRANULARITY);
            }
            
            // FIXME - uncomment this when async is working
    //         wait = IoIsOperationSynchronous(Irp) ? TRUE : FALSE;
            wait = TRUE;
            
            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                CcMdlRead(FileObject,&IrpSp->Parameters.Read.ByteOffset, length, &Irp->MdlAddress, &Irp->IoStatus);
            } else {
                TRACE("CcCopyRead(%p, %llx, %x, %u, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart, length, wait, data, &Irp->IoStatus);
                TRACE("sizes = %llx, %llx, %llx\n", fcb->Header.AllocationSize, fcb->Header.FileSize, fcb->Header.ValidDataLength);
                if (!CcCopyRead(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus)) {
                    TRACE("CcCopyRead failed\n");
                    
                    IoMarkIrpPending(Irp);
                    Status = STATUS_PENDING;
                    goto exit;
                }
                TRACE("CcCopyRead finished\n");
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
        
        if (NT_SUCCESS(Status)) {
            Status = Irp->IoStatus.Status;
            bytes_read = Irp->IoStatus.Information;
        } else
            ERR("EXCEPTION - %08x\n", Status);
    } else {
        if (!(Irp->Flags & IRP_PAGING_IO) && FileObject->SectionObjectPointer->DataSectionObject) {
            IO_STATUS_BLOCK iosb;
            
            CcFlushCache(FileObject->SectionObjectPointer, &IrpSp->Parameters.Read.ByteOffset, length, &iosb);
            
            if (!NT_SUCCESS(iosb.Status)) {
                ERR("CcFlushCache returned %08x\n", iosb.Status);
                Status = iosb.Status;
                goto exit;
            }
        }
        
        acquire_tree_lock(fcb->Vcb, FALSE);
    
        if (fcb->ads)
            Status = read_stream(fcb, data, start, length, &bytes_read);
        else
            Status = read_file(fcb->Vcb, fcb->subvol, fcb->inode, data, start, length, &bytes_read);
        
        release_tree_lock(fcb->Vcb, FALSE);
        
        TRACE("read %u bytes\n", bytes_read);
        
        Irp->IoStatus.Information = bytes_read;
    }
    
exit:
    Irp->IoStatus.Status = Status;
    
    if (FileObject->Flags & FO_SYNCHRONOUS_IO && !(Irp->Flags & IRP_PAGING_IO))
        FileObject->CurrentByteOffset.QuadPart = start + (NT_SUCCESS(Status) ? bytes_read : 0);
    
    // fastfat doesn't do this, but the Wine ntdll file test seems to think we ought to
    if (Irp->UserIosb)
        *Irp->UserIosb = Irp->IoStatus;
    
    TRACE("Irp->IoStatus.Status = %08x\n", Irp->IoStatus.Status);
    TRACE("Irp->IoStatus.Information = %lu\n", Irp->IoStatus.Information);
    TRACE("returning %08x\n", Status);
    
    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    return Status;
}
