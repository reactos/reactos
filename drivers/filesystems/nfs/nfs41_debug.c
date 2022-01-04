/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#define MINIRDR__NAME "Value is ignored, only fact of definition"
#include <rx.h>

#include "nfs41_driver.h"
#include "nfs41_debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <ntstrsafe.h>
#include <winerror.h>

#if defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7)
NTSTATUS NTAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                 ULONG *utf8_bytes_written,
                                 const WCHAR *uni_src, ULONG uni_bytes);
NTSTATUS NTAPI RtlUTF8ToUnicodeN(WCHAR *uni_dest, ULONG uni_bytes_max,
                                 ULONG *uni_bytes_written,
                                 const CHAR *utf8_src, ULONG utf8_bytes);
#endif /* defined(__REACTOS__) && (NTDDI_VERSION < NTDDI_WIN7) */

//#define INCLUDE_TIMESTAMPS

ULONG __cdecl DbgP(IN PCCH fmt, ...)
{
    CHAR msg[512];
    va_list args;
    NTSTATUS status;

    va_start(args, fmt);
    ASSERT(fmt != NULL);
    status = RtlStringCbVPrintfA(msg, sizeof(msg), fmt, args);
    if (NT_SUCCESS(status)) {
#ifdef INCLUDE_TIMESTAMPS
        LARGE_INTEGER timestamp, local_time;
        TIME_FIELDS time_fields;

        KeQuerySystemTime(&timestamp);
        ExSystemTimeToLocalTime(&timestamp,&local_time);
        RtlTimeToTimeFields(&local_time, &time_fields);

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "[%ld].[%02u:%02u:%02u.%u] %s", IoGetCurrentProcess(),
            time_fields.Hour, time_fields.Minute, time_fields.Second,
            time_fields.Milliseconds, msg);
#else
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "[%04x] %s", PsGetCurrentProcessId(), msg);
#endif
    }
    va_end(args);

    return 0;
}

ULONG __cdecl print_error(IN PCCH fmt, ...)
{
    CHAR msg[512];
    va_list args;
    NTSTATUS status;

    va_start(args, fmt);
    ASSERT(fmt != NULL);
    status = RtlStringCbVPrintfA(msg, sizeof(msg), fmt, args);
    if (NT_SUCCESS(status)) {
#ifdef INCLUDE_TIMESTAMPS
        LARGE_INTEGER timestamp, local_time;
        TIME_FIELDS time_fields;

        KeQuerySystemTime(&timestamp);
        ExSystemTimeToLocalTime(&timestamp,&local_time);
        RtlTimeToTimeFields(&local_time, &time_fields);

        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "[%ld].[%02u:%02u:%02u.%u] %s", IoGetCurrentProcess(),
            time_fields.Hour, time_fields.Minute, time_fields.Second,
            time_fields.Milliseconds, msg);
#else
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "[%04x] %s", PsGetCurrentProcessId(), msg);
#endif
    }
    va_end(args);

    return 0;
}

void print_hexbuf(int on, unsigned char *title, unsigned char *buf, int len)
{
    int j, k;
    LARGE_INTEGER timestamp, local_time;
    TIME_FIELDS time_fields;

    if (!on) return;

    KeQuerySystemTime(&timestamp);
    ExSystemTimeToLocalTime(&timestamp,&local_time);
    RtlTimeToTimeFields(&local_time, &time_fields);

    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
        "[%ld].[%02u:%02u:%02u.%u] %s\n", IoGetCurrentProcess(),
        time_fields.Hour, time_fields.Minute, time_fields.Second,
        time_fields.Milliseconds, title);
    for(j = 0, k = 0; j < len; j++, k++) {
        DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL,
            "%02x ", buf[j]);
        if (((k+1) % 30 == 0 && k > 0))
            DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "\n");
    }
    DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_ERROR_LEVEL, "\n");
}

void print_ioctl(int on, int op)
{
    if(!on) return;
    switch(op) {
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            DbgP("IRP_MJ_FILE_SYSTEM_CONTROL\n");
            break;
        case IRP_MJ_DEVICE_CONTROL:
            DbgP("IRP_MJ_DEVICE_CONTROL\n");
            break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            DbgP("IRP_MJ_INTERNAL_DEVICE_CONTROL\n");
            break;
        default:
            DbgP("UNKNOWN MJ IRP %d\n", op);
    };
}

void print_fs_ioctl(int on, int op)
{
    if(!on) return;
    switch(op) {
        case IOCTL_NFS41_INVALCACHE:
            DbgP("IOCTL_NFS41_INVALCACHE\n");
            break;
        case IOCTL_NFS41_READ:
            DbgP("IOCTL_NFS41_UPCALL\n");
            break;
        case IOCTL_NFS41_WRITE:
            DbgP("IOCTL_NFS41_DOWNCALL\n");
            break;
        case IOCTL_NFS41_ADDCONN:
            DbgP("IOCTL_NFS41_ADDCONN\n");
            break;
        case IOCTL_NFS41_DELCONN:
            DbgP("IOCTL_NFS41_DELCONN\n");
            break;
        case IOCTL_NFS41_GETSTATE:
            DbgP("IOCTL_NFS41_GETSTATE\n");
            break;
        case IOCTL_NFS41_START:
            DbgP("IOCTL_NFS41_START\n");
            break;
        case IOCTL_NFS41_STOP:
            DbgP("IOCTL_NFS41_STOP\n");
            break;
        default:
            DbgP("UNKNOWN FS IOCTL %d\n", op);
    };
}

void print_driver_state(int state)
{
    switch (state) {
        case NFS41_START_DRIVER_STARTABLE:
            DbgP("NFS41_START_DRIVER_STARTABLE\n");
            break;
        case NFS41_START_DRIVER_STOPPED:
            DbgP("NFS41_START_DRIVER_STOPPED\n");
            break;
        case NFS41_START_DRIVER_START_IN_PROGRESS:
            DbgP("NFS41_START_DRIVER_START_IN_PROGRESS\n");
            break;
        case NFS41_START_DRIVER_STARTED:
            DbgP("NFS41_START_DRIVER_STARTED\n");
            break;
        default:
            DbgP("UNKNOWN DRIVER STATE %d\n", state);
    };

}

void print_basic_info(int on, PFILE_BASIC_INFORMATION info)
{
    if (!on) return;
    DbgP("BASIC_INFO: Create=%lx Access=%lx Write=%lx Change=%lx Attr=%x\n",
        info->CreationTime.QuadPart, info->LastAccessTime.QuadPart,
        info->LastWriteTime.QuadPart, info->ChangeTime.QuadPart,
        info->FileAttributes);
}
void print_std_info(int on, PFILE_STANDARD_INFORMATION info)
{
    if (!on) return;
    DbgP("STD_INFO: Type=%s #Links=%d Alloc=%lx EOF=%lx Delete=%d\n",
        info->Directory?"DIR":"FILE", info->NumberOfLinks,
        info->AllocationSize.QuadPart, info->EndOfFile.QuadPart,
        info->DeletePending);
}

void print_ea_info(int on, PFILE_FULL_EA_INFORMATION info)
{
    if (!on) return;
    DbgP("FULL_EA_INFO: NextOffset=%d Flags=%x EaNameLength=%d "
        "ExValueLength=%x EaName=%s\n", info->NextEntryOffset, info->Flags,
        info->EaNameLength, info->EaValueLength, info->EaName);
    if (info->EaValueLength)
        print_hexbuf(0, (unsigned char *)"eavalue",
            (unsigned char *)info->EaName + info->EaNameLength + 1,
            info->EaValueLength);
}

void print_get_ea(int on, PFILE_GET_EA_INFORMATION info)
{
    if (!on || !info) return;
    DbgP("GET_EA_INFO: NextOffset=%d EaNameLength=%d EaName=%s\n",
        info->NextEntryOffset, info->EaNameLength, info->EaName);
}

VOID print_srv_call(int on, IN PMRX_SRV_CALL p)
{
    if (!on) return;
    DbgP("PMRX_SRV_CALL %p\n", p);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("pSrvCallName %wZ\n", p->pSrvCallName);
    //DbgP("pPrincipalName %wZ\n", p->pPrincipalName);
    //DbgP("PDomainName %wZ\n", p->pDomainName);
    //DbgP("Flags %08lx\n", p->Flags);
    //DbgP("MaximumNumberOfCloseDelayedFiles %ld\n", p->MaximumNumberOfCloseDelayedFiles);
    //DbgP("Status %ld\n", p->Status);
    DbgP("*****************\n");
#endif
}

VOID print_net_root(int on, IN PMRX_NET_ROOT p)
{
    if (!on) return;
    DbgP("PMRX_NET_ROOT %p\n", p);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    DbgP("\tpSrvCall %p\n", p->pSrvCall);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("Flags %08lx\n", p->Flags);
    DbgP("\tNumberOfFcbs %ld\n", p->NumberOfFcbs);
    DbgP("\tNumberofSrvOpens %ld\n", p->NumberOfSrvOpens);
    //DbgP("MRxNetRootState %ld\n", p->MRxNetRootState);
    //DbgP("Type %ld\n", p->Type);
    //DbgP("DeviceType %ld\n", p->DeviceType);
    //DbgP("pNetRootName %wZ\n", p->pNetRootName);
    //DbgP("InnerNamePrefix %wZ\n", &p->InnerNamePrefix);
    DbgP("*****************\n");
#endif
}

VOID print_v_net_root(int on, IN PMRX_V_NET_ROOT p)
{
    if (!on) return;
    DbgP("PMRX_V_NET_ROOT %p\n", p);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    DbgP("\tpNetRoot %p\n", p->pNetRoot);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("Flags %08lx\n", p->Flags);
    DbgP("\tNumberofOpens %ld\n", p->NumberOfOpens);
    DbgP("\tNumberofFobxs %ld\n", p->NumberOfFobxs);
    //DbgP("LogonId\n");
    //DbgP("pUserDomainName %wZ\n", p->pUserDomainName);
    //DbgP("pUserName %wZ\n", p->pUserName);
    //DbgP("pPassword %wZ\n", p->pPassword);
    //DbgP("SessionId %ld\n", p->SessionId);
    //DbgP("ConstructionStatus %08lx\n", p->ConstructionStatus);
    //DbgP("IsExplicitConnection %d\n", p->IsExplicitConnection);
    DbgP("*****************\n");
#endif
}

void print_file_object(int on, PFILE_OBJECT file)
{
    if (!on) return;
    DbgP("FsContext %p FsContext2 %p\n", file->FsContext, file->FsContext2);
    DbgP("DeletePending %d ReadAccess %d WriteAccess %d DeleteAccess %d\n",
        file->DeletePending, file->WriteAccess, file->DeleteAccess);
    DbgP("SharedRead %d SharedWrite %d SharedDelete %d Flags %x\n",
        file->SharedRead, file->SharedWrite, file->SharedDelete, file->Flags);
}

void print_fo_all(int on, PRX_CONTEXT c)
{
    if (!on) return;
    if (c->pFcb && c->pRelevantSrvOpen)
        DbgP("OpenCount %d FCB %p SRV %p FOBX %p VNET %p NET %p\n",
            c->pFcb->OpenCount, c->pFcb, c->pRelevantSrvOpen, c->pFobx,
            c->pRelevantSrvOpen->pVNetRoot, c->pFcb->pNetRoot);
}

VOID print_fcb(int on, IN PMRX_FCB p)
{
    if (!on) return;
    DbgP("PMRX_FCB %p OpenCount %d\n", p, p->OpenCount);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    DbgP("\tpNetRoot %p\n", p->pNetRoot);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("FcbState %ld\n", p->FcbState);
    //DbgP("UncleanCount %ld\n", p->UncleanCount);
    //DbgP("UncachedUncleanCount %ld\n", p->UncachedUncleanCount);
    DbgP("\tOpenCount %ld\n", p->OpenCount);
    //DbgP("OutstandingLockOperationsCount %ld\n", p->OutstandingLockOperationsCount);
    //DbgP("ActualAllocationLength %ull\n", p->ActualAllocationLength);
    //DbgP("Attributes %ld\n", p->Attributes);
    //DbgP("IsFileWritten %d\n", p->IsFileWritten);
    //DbgP("fShouldBeOrphaned %d\n", p->fShouldBeOrphaned);
    //DbgP("fMiniInited %ld\n", p->fMiniInited);
    //DbgP("CachedNetRootType %c\n", p->CachedNetRootType);
    //DbgP("SrvOpenList\n");
    //DbgP("SrvOpenListVersion %ld\n", p->SrvOpenListVersion);
    DbgP("*****************\n");
#endif
}

VOID print_srv_open(int on, IN PMRX_SRV_OPEN p)
{
    if (!on) return;
    DbgP("PMRX_SRV_OPEN %p\n", p);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    DbgP("\tpFcb %p\n", p->pFcb);
    DbgP("\tpVNetRoot %p\n", p->pVNetRoot);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("Flags %08lx\n", p->Flags);
    //DbgP("pAlreadyPrefixedName %wZ\n", p->pAlreadyPrefixedName);
    //DbgP("UncleanFobxCount %ld\n", p->UncleanFobxCount);
    DbgP("\tOpenCount %ld\n", p->OpenCount);
    //DbgP("Key %p\n", p->Key);
    //DbgP("DesiredAccess\n");
    //DbgP("ShareAccess %ld\n", p->ShareAccess);
    //DbgP("CreateOptions %ld\n", p->CreateOptions);
    //DbgP("BufferingFlags %ld\n", p->BufferingFlags);
    //DbgP("ulFileSizeVersion %ld\n", p->ulFileSizeVersion);
    //DbgP("SrvOpenQLinks\n");
    DbgP("*****************\n");
#endif
}

VOID print_fobx(int on, IN PMRX_FOBX p)
{
    if (!on) return;
    DbgP("PMRX_FOBX %p\n", p);
#if 0
    DbgP("\tNodeReferenceCount %ld\n", p->NodeReferenceCount);
    DbgP("\tpSrvOpen %p\n", p->pSrvOpen);
    DbgP("\tAssociatedFileObject %p\n", p->AssociatedFileObject);
    //DbgP("Context %p\n", p->Context);
    //DbgP("Context2 %p\n", p->Context2);
    //DbgP("Flags %08lx\n", p->Flags);
    DbgP("*****************\n");
#endif
}

VOID print_irp_flags(int on, PIRP irp)
{
    if (!on) return;
    if (irp->Flags)
        DbgP("IRP FLAGS: 0x%x %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
            irp->Flags,
            (irp->Flags & IRP_NOCACHE)?"NOCACHE":"",
            (irp->Flags & IRP_PAGING_IO)?"PAGING_IO":"",
            (irp->Flags & IRP_MOUNT_COMPLETION)?"MOUNT":"",
            (irp->Flags & IRP_SYNCHRONOUS_API)?"SYNC":"",
            (irp->Flags & IRP_ASSOCIATED_IRP)?"ASSOC_IPR":"",
            (irp->Flags & IRP_BUFFERED_IO)?"BUFFERED":"",
            (irp->Flags & IRP_DEALLOCATE_BUFFER)?"DEALLOC_BUF":"",
            (irp->Flags & IRP_INPUT_OPERATION)?"INPUT_OP":"",
            (irp->Flags & IRP_SYNCHRONOUS_PAGING_IO)?"SYNC_PAGIN_IO":"",
            (irp->Flags & IRP_CREATE_OPERATION)?"CREATE_OP":"",
            (irp->Flags & IRP_READ_OPERATION)?"READ_OP":"",
            (irp->Flags & IRP_WRITE_OPERATION)?"WRITE_OP":"",
            (irp->Flags & IRP_CLOSE_OPERATION)?"CLOSE_OP":"",
            (irp->Flags & IRP_DEFER_IO_COMPLETION)?"DEFER_IO":"");
}

void print_irps_flags(int on, PIO_STACK_LOCATION irps)
{
    if (!on) return;
    if (irps->Flags)
        DbgP("IRPSP FLAGS 0x%x %s %s %s %s\n", irps->Flags,
            (irps->Flags & SL_CASE_SENSITIVE)?"CASE_SENSITIVE":"",
            (irps->Flags & SL_OPEN_PAGING_FILE)?"PAGING_FILE":"",
            (irps->Flags & SL_FORCE_ACCESS_CHECK)?"ACCESS_CHECK":"",
            (irps->Flags & SL_OPEN_TARGET_DIRECTORY)?"TARGET_DIR":"");
}
void print_nt_create_params(int on, NT_CREATE_PARAMETERS params)
{
    if (!on) return;
    if (params.FileAttributes)
        DbgP("File attributes %x: %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
            params.FileAttributes,
            (params.FileAttributes & FILE_ATTRIBUTE_TEMPORARY)?"TEMPFILE ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_READONLY)?"READONLY ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_HIDDEN)?"HIDDEN ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_SYSTEM)?"SYSTEM ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)?"ARCHIVE ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)?"DIR ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_DEVICE)?"DEVICE ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_NORMAL)?"NORMAL ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)?"SPARSE_FILE ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)?"REPARSE_POINT ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_COMPRESSED)?"COMPRESSED ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)?"NOT INDEXED ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)?"ENCRYPTED ":"",
            (params.FileAttributes & FILE_ATTRIBUTE_VIRTUAL)?"VIRTUAL":"");

    if (params.Disposition  == FILE_SUPERSEDE)
        DbgP("Create Dispositions: FILE_SUPERSEDE\n");
    if (params.Disposition == FILE_CREATE)
        DbgP("Create Dispositions: FILE_CREATE\n");
    if (params.Disposition == FILE_OPEN)
        DbgP("Create Dispositions: FILE_OPEN\n");
    if (params.Disposition == FILE_OPEN_IF)
        DbgP("Create Dispositions: FILE_OPEN_IF\n");
    if (params.Disposition == FILE_OVERWRITE)
        DbgP("Create Dispositions: FILE_OVERWRITE\n");
    if (params.Disposition == FILE_OVERWRITE_IF)
        DbgP("Create Dispositions: FILE_OVERWRITE_IF\n");

    DbgP("Create Attributes: 0x%x %s %s %s %s %s %s %s %s %s %s %s %s %s %s "
        "%s %s\n", params.CreateOptions,
        (params.CreateOptions & FILE_DIRECTORY_FILE)?"DIRFILE":"",
        (params.CreateOptions & FILE_NON_DIRECTORY_FILE)?"FILE":"",
        (params.CreateOptions & FILE_DELETE_ON_CLOSE)?"DELETE_ON_CLOSE":"",
        (params.CreateOptions & FILE_WRITE_THROUGH)?"WRITE_THROUGH":"",
        (params.CreateOptions & FILE_SEQUENTIAL_ONLY)?"SEQUENTIAL":"",
        (params.CreateOptions & FILE_RANDOM_ACCESS)?"RANDOM":"",
        (params.CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)?"NO_BUFFERING":"",
        (params.CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)?"SYNC_ALERT":"",
        (params.CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)?"SYNC_NOALERT":"",
        (params.CreateOptions & FILE_CREATE_TREE_CONNECTION)?"CREATE_TREE_CONN":"",
        (params.CreateOptions & FILE_COMPLETE_IF_OPLOCKED)?"OPLOCKED":"",
        (params.CreateOptions & FILE_NO_EA_KNOWLEDGE)?"NO_EA":"",
        (params.CreateOptions & FILE_OPEN_REPARSE_POINT)?"OPEN_REPARSE":"",
        (params.CreateOptions & FILE_OPEN_BY_FILE_ID)?"BY_ID":"",
        (params.CreateOptions & FILE_OPEN_FOR_BACKUP_INTENT)?"4_BACKUP":"",
        (params.CreateOptions & FILE_RESERVE_OPFILTER)?"OPFILTER":"");

    DbgP("Share Access: %s %s %s\n",
        (params.ShareAccess & FILE_SHARE_READ)?"READ":"",
        (params.ShareAccess & FILE_SHARE_WRITE)?"WRITE":"",
        (params.ShareAccess & FILE_SHARE_DELETE)?"DELETE":"");

    DbgP("Desired Access: 0x%x %s %s %s %s %s %s %s %s %s %s %s\n",
        params.DesiredAccess,
        (params.DesiredAccess & FILE_READ_DATA)?"READ":"",
        (params.DesiredAccess & STANDARD_RIGHTS_READ)?"READ_ACL":"",
        (params.DesiredAccess & FILE_READ_ATTRIBUTES)?"GETATTR":"",
        (params.DesiredAccess & FILE_READ_EA)?"READ_EA":"",
        (params.DesiredAccess & FILE_WRITE_DATA)?"WRITE":"",
        (params.DesiredAccess & FILE_WRITE_ATTRIBUTES)?"SETATTR":"",
        (params.DesiredAccess & FILE_WRITE_EA)?"WRITE_EA":"",
        (params.DesiredAccess & FILE_APPEND_DATA)?"APPEND":"",
        (params.DesiredAccess & FILE_EXECUTE)?"EXEC":"",
        (params.DesiredAccess & FILE_LIST_DIRECTORY)?"LSDIR":"",
        (params.DesiredAccess & FILE_TRAVERSE)?"TRAVERSE":"",
        (params.DesiredAccess & FILE_LIST_DIRECTORY)?"LSDIR":"",
        (params.DesiredAccess & DELETE)?"DELETE":"",
        (params.DesiredAccess & READ_CONTROL)?"READ_CONTROL":"",
        (params.DesiredAccess & WRITE_DAC)?"WRITE_DAC":"",
        (params.DesiredAccess & WRITE_OWNER)?"WRITE_OWNER":"",
        (params.DesiredAccess & SYNCHRONIZE)?"SYNCHRONIZE":"");
}

unsigned char * print_file_information_class(int InfoClass)
{
    switch(InfoClass) {
        case FileBothDirectoryInformation:
            return (unsigned char *)"FileBothDirectoryInformation";
        case FileDirectoryInformation:
            return (unsigned char *)"FileDirectoryInformation";
        case FileFullDirectoryInformation:
            return (unsigned char *)"FileFullDirectoryInformation";
        case FileIdBothDirectoryInformation:
            return (unsigned char *)"FileIdBothDirectoryInformation";
        case FileIdFullDirectoryInformation:
            return (unsigned char *)"FileIdFullDirectoryInformation";
        case FileNamesInformation:
            return (unsigned char *)"FileNamesInformation";
        case FileObjectIdInformation:
            return (unsigned char *)"FileObjectIdInformation";
        case FileQuotaInformation:
            return (unsigned char *)"FileQuotaInformation";
        case FileReparsePointInformation:
            return (unsigned char *)"FileReparsePointInformation";
        case FileAllInformation:
            return (unsigned char *)"FileAllInformation";
        case FileAttributeTagInformation:
            return (unsigned char *)"FileAttributeTagInformation";
        case FileBasicInformation:
            return (unsigned char *)"FileBasicInformation";
        case FileCompressionInformation:
            return (unsigned char *)"FileCompressionInformation";
        case FileEaInformation:
            return (unsigned char *)"FileEaInformation";
        case FileInternalInformation:
            return (unsigned char *)"FileInternalInformation";
        case FileNameInformation:
            return (unsigned char *)"FileNameInformation";
        case FileNetworkOpenInformation:
            return (unsigned char *)"FileNetworkOpenInformation";
        case FilePositionInformation:
            return (unsigned char *)"FilePositionInformation";
        case FileStandardInformation:
            return (unsigned char *)"FileStandardInformation";
        case FileStreamInformation:
            return (unsigned char *)"FileStreamInformation";
        case FileAllocationInformation:
            return (unsigned char *)"FileAllocationInformation";
        case FileDispositionInformation:
            return (unsigned char *)"FileDispositionInformation";
        case FileEndOfFileInformation:
            return (unsigned char *)"FileEndOfFileInformation";
        case FileLinkInformation:
            return (unsigned char *)"FileLinkInformation";
        case FileRenameInformation:
            return (unsigned char *)"FileRenameInformation";
        case FileValidDataLengthInformation:
            return (unsigned char *)"FileValidDataLengthInformation";
        default:
            return (unsigned char *)"UNKNOWN";
    }
}

unsigned char *print_fs_information_class(int InfoClass)
{
    switch (InfoClass) {
        case FileFsAttributeInformation:
            return (unsigned char *)"FileFsAttributeInformation";
        case FileFsControlInformation:
            return (unsigned char *)"FileFsControlInformation";
        case FileFsDeviceInformation:
            return (unsigned char *)"FileFsDeviceInformation";
        case FileFsDriverPathInformation:
            return (unsigned char *)"FileFsDriverPathInformation";
        case FileFsFullSizeInformation:
            return (unsigned char *)"FileFsFullSizeInformation";
        case FileFsObjectIdInformation:
            return (unsigned char *)"FileFsObjectIdInformation";
        case FileFsSizeInformation:
            return (unsigned char *)"FileFsSizeInformation";
        case FileFsVolumeInformation:
            return (unsigned char *)"FileFsVolumeInformation";
        default:
            return (unsigned char *)"UNKNOWN";
    }
}

void print_caching_level(int on, ULONG flag, PUNICODE_STRING name)
{
    if (!on) return;
    switch(flag) {
        case 0:
            DbgP("enable_caching: DISABLE_CACHING %wZ\n", name);
            break;
        case 1:
            DbgP("enable_caching: ENABLE_READ_CACHING %wZ\n", name);
            break;
        case 2:
            DbgP("enable_caching: ENABLE_WRITE_CACHING %wZ\n", name);
            break;
        case 3:
            DbgP("enable_caching: ENABLE_READWRITE_CACHING %wZ\n", name);
            break;
    }
}

const char *opcode2string(int opcode)
{
    switch(opcode) {
    case NFS41_SHUTDOWN: return "NFS41_SHUTDOWN";
    case NFS41_MOUNT: return "NFS41_MOUNT";
    case NFS41_UNMOUNT: return "NFS41_UNMOUNT";
    case NFS41_OPEN: return "NFS41_OPEN";
    case NFS41_CLOSE: return "NFS41_CLOSE";
    case NFS41_READ: return "NFS41_READ";
    case NFS41_WRITE: return "NFS41_WRITE";
    case NFS41_LOCK: return "NFS41_LOCK";
    case NFS41_UNLOCK: return "NFS41_UNLOCK";
    case NFS41_DIR_QUERY: return "NFS41_DIR_QUERY";
    case NFS41_FILE_QUERY: return "NFS41_FILE_QUERY";
    case NFS41_FILE_SET: return "NFS41_FILE_SET";
    case NFS41_EA_SET: return "NFS41_EA_SET";
    case NFS41_EA_GET: return "NFS41_EA_GET";
    case NFS41_SYMLINK: return "NFS41_SYMLINK";
    case NFS41_VOLUME_QUERY: return "NFS41_VOLUME_QUERY";
    case NFS41_ACL_QUERY: return "NFS41_ACL_QUERY";
    case NFS41_ACL_SET: return "NFS41_ACL_SET";
    default: return "UNKNOWN";
    }
}

void print_acl_args(
    SECURITY_INFORMATION info)
{
    DbgP("Security query: %s %s %s\n",
        (info & OWNER_SECURITY_INFORMATION)?"OWNER":"",
        (info & GROUP_SECURITY_INFORMATION)?"GROUP":"",
        (info & DACL_SECURITY_INFORMATION)?"DACL":"",
        (info & SACL_SECURITY_INFORMATION)?"SACL":"");
}

void print_open_error(int on, int status)
{
    if (!on) return;
    switch (status) {
    case STATUS_ACCESS_DENIED:
        DbgP("[ERROR] nfs41_Create: STATUS_ACCESS_DENIED\n");
        break;
    case STATUS_NETWORK_ACCESS_DENIED:
        DbgP("[ERROR] nfs41_Create: STATUS_NETWORK_ACCESS_DENIED\n");
        break;
    case STATUS_OBJECT_NAME_INVALID:
        DbgP("[ERROR] nfs41_Create: STATUS_OBJECT_NAME_INVALID\n");
        break;
    case STATUS_OBJECT_NAME_COLLISION:
        DbgP("[ERROR] nfs41_Create: STATUS_OBJECT_NAME_COLLISION\n");
        break;
    case STATUS_FILE_INVALID:
        DbgP("[ERROR] nfs41_Create: STATUS_FILE_INVALID\n");
        break;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        DbgP("[ERROR] nfs41_Create: STATUS_OBJECT_NAME_NOT_FOUND\n");
        break;
    case STATUS_NAME_TOO_LONG:
        DbgP("[ERROR] nfs41_Create: STATUS_NAME_TOO_LONG\n");
        break;
    case STATUS_OBJECT_PATH_NOT_FOUND:
        DbgP("[ERROR] nfs41_Create: STATUS_OBJECT_PATH_NOT_FOUND\n");
        break;
    case STATUS_BAD_NETWORK_PATH:
        DbgP("[ERROR] nfs41_Create: STATUS_BAD_NETWORK_PATH\n");
        break;
    case STATUS_SHARING_VIOLATION:
        DbgP("[ERROR] nfs41_Create: STATUS_SHARING_VIOLATION\n");
        break;
    case ERROR_REPARSE:
        DbgP("[ERROR] nfs41_Create: STATUS_REPARSE\n");
        break;
    case ERROR_TOO_MANY_LINKS:
        DbgP("[ERROR] nfs41_Create: STATUS_TOO_MANY_LINKS\n");
        break;
    case ERROR_DIRECTORY:
        DbgP("[ERROR] nfs41_Create: STATUS_FILE_IS_A_DIRECTORY\n");
        break;
    case ERROR_BAD_FILE_TYPE:
        DbgP("[ERROR] nfs41_Create: STATUS_NOT_A_DIRECTORY\n");
        break;
    default:
        DbgP("[ERROR] nfs41_Create: STATUS_INSUFFICIENT_RESOURCES\n");
        break;
    }
}

void print_wait_status(int on, const char *prefix, NTSTATUS status,
                       const char *opcode, PVOID entry, LONGLONG xid)
{
    if (!on) return;
    switch (status) {
    case STATUS_SUCCESS:
        if (opcode)
            DbgP("%s Got a wakeup call, finishing %s entry=%p xid=%lld\n",
                prefix, opcode, entry, xid);
        else
            DbgP("%s Got a wakeup call\n", prefix);
        break;
    case STATUS_USER_APC:
        DbgP("%s KeWaitForSingleObject returned STATUS_USER_APC\n", prefix);
        break;
    case STATUS_ALERTED:
        DbgP("%s KeWaitForSingleObject returned STATUS_ALERTED\n", prefix);
        break;
    default:
        DbgP("%s KeWaitForSingleObject returned %d\n", prefix, status);
    }
}
/* This is taken from toaster/func.  Rumor says this should be replaced
 * with a WMI interface???
 */
ULONG
dprintk(
    IN PCHAR func,
    IN ULONG flags,
    IN PCHAR format,
    ...)
{
    #define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    CHAR      debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS status, rv = STATUS_SUCCESS;

    va_start(list, format);

    if (format)
    {
        //
        // Use the safe string function, RtlStringCbVPrintfA, instead of _vsnprintf.
        // RtlStringCbVPrintfA NULL terminates the output buffer even if the message
        // is longer than the buffer. This prevents malicious code from compromising
        // the security of the system.
        //
        status = RtlStringCbVPrintfA(debugMessageBuffer, sizeof(debugMessageBuffer),
                                    format, list);

        if (!NT_SUCCESS(status))
            rv = DbgPrintEx(PNFS_FLTR_ID, DPFLTR_MASK | flags,
                            "RtlStringCbVPrintfA failed %x \n", status);
        else
            rv = DbgPrintEx(PNFS_FLTR_ID, DPFLTR_MASK | flags, "%s    %s: %s\n",
                    PNFS_TRACE_TAG, func, debugMessageBuffer);
    }
    va_end(list);

    return rv;
}

