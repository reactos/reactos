/* Copyright (c) Mark Harmstone 2016-17
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "btrfs_drv.h"

#define SEF_DACL_AUTO_INHERIT 0x01
#define SEF_SACL_AUTO_INHERIT 0x02

typedef struct {
    UCHAR revision;
    UCHAR elements;
    UCHAR auth[6];
    uint32_t nums[8];
} sid_header;

static sid_header sid_BA = { 1, 2, SECURITY_NT_AUTHORITY, {32, 544}}; // BUILTIN\Administrators
static sid_header sid_SY = { 1, 1, SECURITY_NT_AUTHORITY, {18}};      // NT AUTHORITY\SYSTEM
static sid_header sid_BU = { 1, 2, SECURITY_NT_AUTHORITY, {32, 545}}; // BUILTIN\Users
static sid_header sid_AU = { 1, 1, SECURITY_NT_AUTHORITY, {11}};      // NT AUTHORITY\Authenticated Users

typedef struct {
    UCHAR flags;
    ACCESS_MASK mask;
    sid_header* sid;
} dacl;

static dacl def_dacls[] = {
    { 0, FILE_ALL_ACCESS, &sid_BA },
    { OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, FILE_ALL_ACCESS, &sid_BA },
    { 0, FILE_ALL_ACCESS, &sid_SY },
    { OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, FILE_ALL_ACCESS, &sid_SY },
    { OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, &sid_BU },
    { OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE, &sid_AU },
    { 0, FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE | DELETE, &sid_AU },
    // FIXME - Mandatory Label\High Mandatory Level:(OI)(NP)(IO)(NW)
    { 0, 0, NULL }
};

extern LIST_ENTRY uid_map_list, gid_map_list;
extern ERESOURCE mapping_lock;

void add_user_mapping(WCHAR* sidstring, ULONG sidstringlength, uint32_t uid) {
    unsigned int i, np;
    uint8_t numdashes;
    uint64_t val;
    ULONG sidsize;
    sid_header* sid;
    uid_map* um;

    if (sidstringlength < 4 ||
        sidstring[0] != 'S' ||
        sidstring[1] != '-' ||
        sidstring[2] != '1' ||
        sidstring[3] != '-') {
        ERR("invalid SID\n");
        return;
    }

    sidstring = &sidstring[4];
    sidstringlength -= 4;

    numdashes = 0;
    for (i = 0; i < sidstringlength; i++) {
        if (sidstring[i] == '-') {
            numdashes++;
            sidstring[i] = 0;
        }
    }

    sidsize = 8 + (numdashes * 4);
    sid = ExAllocatePoolWithTag(PagedPool, sidsize, ALLOC_TAG);
    if (!sid) {
        ERR("out of memory\n");
        return;
    }

    sid->revision = 0x01;
    sid->elements = numdashes;

    np = 0;
    while (sidstringlength > 0) {
        val = 0;
        i = 0;
        while (sidstring[i] != '-' && i < sidstringlength) {
            if (sidstring[i] >= '0' && sidstring[i] <= '9') {
                val *= 10;
                val += sidstring[i] - '0';
            } else
                break;

            i++;
        }

        i++;
        TRACE("val = %u, i = %u, ssl = %lu\n", (uint32_t)val, i, sidstringlength);

        if (np == 0) {
            sid->auth[0] = (uint8_t)((val & 0xff0000000000) >> 40);
            sid->auth[1] = (uint8_t)((val & 0xff00000000) >> 32);
            sid->auth[2] = (uint8_t)((val & 0xff000000) >> 24);
            sid->auth[3] = (uint8_t)((val & 0xff0000) >> 16);
            sid->auth[4] = (uint8_t)((val & 0xff00) >> 8);
            sid->auth[5] = val & 0xff;
        } else {
            sid->nums[np-1] = (uint32_t)val;
        }

        np++;

        if (sidstringlength > i) {
            sidstringlength -= i;

            sidstring = &sidstring[i];
        } else
            break;
    }

    um = ExAllocatePoolWithTag(PagedPool, sizeof(uid_map), ALLOC_TAG);
    if (!um) {
        ERR("out of memory\n");
        ExFreePool(sid);
        return;
    }

    um->sid = sid;
    um->uid = uid;

    InsertTailList(&uid_map_list, &um->listentry);
}

void add_group_mapping(WCHAR* sidstring, ULONG sidstringlength, uint32_t gid) {
    unsigned int i, np;
    uint8_t numdashes;
    uint64_t val;
    ULONG sidsize;
    sid_header* sid;
    gid_map* gm;

    if (sidstringlength < 4 || sidstring[0] != 'S' || sidstring[1] != '-' || sidstring[2] != '1' || sidstring[3] != '-') {
        ERR("invalid SID\n");
        return;
    }

    sidstring = &sidstring[4];
    sidstringlength -= 4;

    numdashes = 0;
    for (i = 0; i < sidstringlength; i++) {
        if (sidstring[i] == '-') {
            numdashes++;
            sidstring[i] = 0;
        }
    }

    sidsize = 8 + (numdashes * 4);
    sid = ExAllocatePoolWithTag(PagedPool, sidsize, ALLOC_TAG);
    if (!sid) {
        ERR("out of memory\n");
        return;
    }

    sid->revision = 0x01;
    sid->elements = numdashes;

    np = 0;
    while (sidstringlength > 0) {
        val = 0;
        i = 0;
        while (i < sidstringlength && sidstring[i] != '-') {
            if (sidstring[i] >= '0' && sidstring[i] <= '9') {
                val *= 10;
                val += sidstring[i] - '0';
            } else
                break;

            i++;
        }

        i++;
        TRACE("val = %u, i = %u, ssl = %lu\n", (uint32_t)val, i, sidstringlength);

        if (np == 0) {
            sid->auth[0] = (uint8_t)((val & 0xff0000000000) >> 40);
            sid->auth[1] = (uint8_t)((val & 0xff00000000) >> 32);
            sid->auth[2] = (uint8_t)((val & 0xff000000) >> 24);
            sid->auth[3] = (uint8_t)((val & 0xff0000) >> 16);
            sid->auth[4] = (uint8_t)((val & 0xff00) >> 8);
            sid->auth[5] = val & 0xff;
        } else
            sid->nums[np-1] = (uint32_t)val;

        np++;

        if (sidstringlength > i) {
            sidstringlength -= i;

            sidstring = &sidstring[i];
        } else
            break;
    }

    gm = ExAllocatePoolWithTag(PagedPool, sizeof(gid_map), ALLOC_TAG);
    if (!gm) {
        ERR("out of memory\n");
        ExFreePool(sid);
        return;
    }

    gm->sid = sid;
    gm->gid = gid;

    InsertTailList(&gid_map_list, &gm->listentry);
}

NTSTATUS uid_to_sid(uint32_t uid, PSID* sid) {
    LIST_ENTRY* le;
    sid_header* sh;
    UCHAR els;

    ExAcquireResourceSharedLite(&mapping_lock, true);

    le = uid_map_list.Flink;
    while (le != &uid_map_list) {
        uid_map* um = CONTAINING_RECORD(le, uid_map, listentry);

        if (um->uid == uid) {
            *sid = ExAllocatePoolWithTag(PagedPool, RtlLengthSid(um->sid), ALLOC_TAG);
            if (!*sid) {
                ERR("out of memory\n");
                ExReleaseResourceLite(&mapping_lock);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(*sid, um->sid, RtlLengthSid(um->sid));
            ExReleaseResourceLite(&mapping_lock);
            return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&mapping_lock);

    if (uid == 0) { // root
        // FIXME - find actual Administrator account, rather than SYSTEM (S-1-5-18)
        // (of form S-1-5-21-...-500)

        els = 1;

        sh = ExAllocatePoolWithTag(PagedPool, sizeof(sid_header) + ((els - 1) * sizeof(uint32_t)), ALLOC_TAG);
        if (!sh) {
            ERR("out of memory\n");
            *sid = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        sh->revision = 1;
        sh->elements = els;

        sh->auth[0] = 0;
        sh->auth[1] = 0;
        sh->auth[2] = 0;
        sh->auth[3] = 0;
        sh->auth[4] = 0;
        sh->auth[5] = 5;

        sh->nums[0] = 18;
    } else {
        // fallback to S-1-22-1-X, Samba's SID scheme
        sh = ExAllocatePoolWithTag(PagedPool, sizeof(sid_header), ALLOC_TAG);
        if (!sh) {
            ERR("out of memory\n");
            *sid = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        sh->revision = 1;
        sh->elements = 2;

        sh->auth[0] = 0;
        sh->auth[1] = 0;
        sh->auth[2] = 0;
        sh->auth[3] = 0;
        sh->auth[4] = 0;
        sh->auth[5] = 22;

        sh->nums[0] = 1;
        sh->nums[1] = uid;
    }

    *sid = sh;

    return STATUS_SUCCESS;
}

uint32_t sid_to_uid(PSID sid) {
    LIST_ENTRY* le;
    sid_header* sh = sid;

    ExAcquireResourceSharedLite(&mapping_lock, true);

    le = uid_map_list.Flink;
    while (le != &uid_map_list) {
        uid_map* um = CONTAINING_RECORD(le, uid_map, listentry);

        if (RtlEqualSid(sid, um->sid)) {
            ExReleaseResourceLite(&mapping_lock);
            return um->uid;
        }

        le = le->Flink;
    }

    ExReleaseResourceLite(&mapping_lock);

    if (RtlEqualSid(sid, &sid_SY))
        return 0; // root

    // Samba's SID scheme: S-1-22-1-X
    if (sh->revision == 1 && sh->elements == 2 && sh->auth[0] == 0 && sh->auth[1] == 0 && sh->auth[2] == 0 && sh->auth[3] == 0 &&
        sh->auth[4] == 0 && sh->auth[5] == 22 && sh->nums[0] == 1)
        return sh->nums[1];

    return UID_NOBODY;
}

static void gid_to_sid(uint32_t gid, PSID* sid) {
    sid_header* sh;
    UCHAR els;

    // FIXME - do this properly?

    // fallback to S-1-22-2-X, Samba's SID scheme
    els = 2;
    sh = ExAllocatePoolWithTag(PagedPool, sizeof(sid_header) + ((els - 1) * sizeof(uint32_t)), ALLOC_TAG);
    if (!sh) {
        ERR("out of memory\n");
        *sid = NULL;
        return;
    }

    sh->revision = 1;
    sh->elements = els;

    sh->auth[0] = 0;
    sh->auth[1] = 0;
    sh->auth[2] = 0;
    sh->auth[3] = 0;
    sh->auth[4] = 0;
    sh->auth[5] = 22;

    sh->nums[0] = 2;
    sh->nums[1] = gid;

    *sid = sh;
}

static ACL* load_default_acl() {
    uint16_t size, i;
    ACL* acl;
    ACCESS_ALLOWED_ACE* aaa;

    size = sizeof(ACL);
    i = 0;
    while (def_dacls[i].sid) {
        size += sizeof(ACCESS_ALLOWED_ACE);
        size += 8 + (def_dacls[i].sid->elements * sizeof(uint32_t)) - sizeof(ULONG);
        i++;
    }

    acl = ExAllocatePoolWithTag(PagedPool, size, ALLOC_TAG);
    if (!acl) {
        ERR("out of memory\n");
        return NULL;
    }

    acl->AclRevision = ACL_REVISION;
    acl->Sbz1 = 0;
    acl->AclSize = size;
    acl->AceCount = i;
    acl->Sbz2 = 0;

    aaa = (ACCESS_ALLOWED_ACE*)&acl[1];
    i = 0;
    while (def_dacls[i].sid) {
        aaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        aaa->Header.AceFlags = def_dacls[i].flags;
        aaa->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) + 8 + (def_dacls[i].sid->elements * sizeof(uint32_t));
        aaa->Mask = def_dacls[i].mask;

        RtlCopyMemory(&aaa->SidStart, def_dacls[i].sid, 8 + (def_dacls[i].sid->elements * sizeof(uint32_t)));

        aaa = (ACCESS_ALLOWED_ACE*)((uint8_t*)aaa + aaa->Header.AceSize);

        i++;
    }

    return acl;
}

static void get_top_level_sd(fcb* fcb) {
    NTSTATUS Status;
    SECURITY_DESCRIPTOR sd;
    ULONG buflen;
    ACL* acl = NULL;
    PSID usersid = NULL, groupsid = NULL;

    Status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    if (!NT_SUCCESS(Status)) {
        ERR("RtlCreateSecurityDescriptor returned %08lx\n", Status);
        goto end;
    }

    Status = uid_to_sid(fcb->inode_item.st_uid, &usersid);
    if (!NT_SUCCESS(Status)) {
        ERR("uid_to_sid returned %08lx\n", Status);
        goto end;
    }

    Status = RtlSetOwnerSecurityDescriptor(&sd, usersid, false);

    if (!NT_SUCCESS(Status)) {
        ERR("RtlSetOwnerSecurityDescriptor returned %08lx\n", Status);
        goto end;
    }

    gid_to_sid(fcb->inode_item.st_gid, &groupsid);
    if (!groupsid) {
        ERR("out of memory\n");
        goto end;
    }

    Status = RtlSetGroupSecurityDescriptor(&sd, groupsid, false);

    if (!NT_SUCCESS(Status)) {
        ERR("RtlSetGroupSecurityDescriptor returned %08lx\n", Status);
        goto end;
    }

    acl = load_default_acl();

    if (!acl) {
        ERR("out of memory\n");
        goto end;
    }

    Status = RtlSetDaclSecurityDescriptor(&sd, true, acl, false);

    if (!NT_SUCCESS(Status)) {
        ERR("RtlSetDaclSecurityDescriptor returned %08lx\n", Status);
        goto end;
    }

    // FIXME - SACL_SECURITY_INFORMATION

    buflen = 0;

    // get sd size
    Status = RtlAbsoluteToSelfRelativeSD(&sd, NULL, &buflen);
    if (Status != STATUS_SUCCESS && Status != STATUS_BUFFER_TOO_SMALL) {
        ERR("RtlAbsoluteToSelfRelativeSD 1 returned %08lx\n", Status);
        goto end;
    }

    if (buflen == 0 || Status == STATUS_SUCCESS) {
        TRACE("RtlAbsoluteToSelfRelativeSD said SD is zero-length\n");
        goto end;
    }

    fcb->sd = ExAllocatePoolWithTag(PagedPool, buflen, ALLOC_TAG);
    if (!fcb->sd) {
        ERR("out of memory\n");
        goto end;
    }

    Status = RtlAbsoluteToSelfRelativeSD(&sd, fcb->sd, &buflen);

    if (!NT_SUCCESS(Status)) {
        ERR("RtlAbsoluteToSelfRelativeSD 2 returned %08lx\n", Status);
        ExFreePool(fcb->sd);
        fcb->sd = NULL;
        goto end;
    }

end:
    if (acl)
        ExFreePool(acl);

    if (usersid)
        ExFreePool(usersid);

    if (groupsid)
        ExFreePool(groupsid);
}

void fcb_get_sd(fcb* fcb, struct _fcb* parent, bool look_for_xattr, PIRP Irp) {
    NTSTATUS Status;
    PSID usersid = NULL, groupsid = NULL;
    SECURITY_SUBJECT_CONTEXT subjcont;
    ULONG buflen;
    PSECURITY_DESCRIPTOR* abssd;
    PSECURITY_DESCRIPTOR newsd;
    PACL dacl, sacl;
    PSID owner, group;
    ULONG abssdlen = 0, dacllen = 0, sacllen = 0, ownerlen = 0, grouplen = 0;
    uint8_t* buf;

    if (look_for_xattr && get_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_NTACL, EA_NTACL_HASH, (uint8_t**)&fcb->sd, (uint16_t*)&buflen, Irp))
        return;

    if (!parent) {
        get_top_level_sd(fcb);
        return;
    }

    SeCaptureSubjectContext(&subjcont);

    Status = SeAssignSecurityEx(parent->sd, NULL, (void**)&fcb->sd, NULL, fcb->type == BTRFS_TYPE_DIRECTORY, SEF_DACL_AUTO_INHERIT,
                                &subjcont, IoGetFileObjectGenericMapping(), PagedPool);
    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurityEx returned %08lx\n", Status);
        return;
    }

    Status = RtlSelfRelativeToAbsoluteSD(fcb->sd, NULL, &abssdlen, NULL, &dacllen, NULL, &sacllen, NULL, &ownerlen,
                                         NULL, &grouplen);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL) {
        ERR("RtlSelfRelativeToAbsoluteSD returned %08lx\n", Status);
        return;
    }

    if (abssdlen + dacllen + sacllen + ownerlen + grouplen == 0) {
        ERR("RtlSelfRelativeToAbsoluteSD returned zero lengths\n");
        return;
    }

    buf = (uint8_t*)ExAllocatePoolWithTag(PagedPool, abssdlen + dacllen + sacllen + ownerlen + grouplen, ALLOC_TAG);
    if (!buf) {
        ERR("out of memory\n");
        return;
    }

    abssd = (PSECURITY_DESCRIPTOR)buf;
    dacl = (PACL)(buf + abssdlen);
    sacl = (PACL)(buf + abssdlen + dacllen);
    owner = (PSID)(buf + abssdlen + dacllen + sacllen);
    group = (PSID)(buf + abssdlen + dacllen + sacllen + ownerlen);

    Status = RtlSelfRelativeToAbsoluteSD(fcb->sd, abssd, &abssdlen, dacl, &dacllen, sacl, &sacllen, owner, &ownerlen,
                                         group, &grouplen);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL) {
        ERR("RtlSelfRelativeToAbsoluteSD returned %08lx\n", Status);
        ExFreePool(buf);
        return;
    }

    Status = uid_to_sid(fcb->inode_item.st_uid, &usersid);
    if (!NT_SUCCESS(Status)) {
        ERR("uid_to_sid returned %08lx\n", Status);
        ExFreePool(buf);
        return;
    }

    RtlSetOwnerSecurityDescriptor(abssd, usersid, false);

    gid_to_sid(fcb->inode_item.st_gid, &groupsid);
    if (!groupsid) {
        ERR("out of memory\n");
        ExFreePool(usersid);
        ExFreePool(buf);
        return;
    }

    RtlSetGroupSecurityDescriptor(abssd, groupsid, false);

    buflen = 0;

    Status = RtlAbsoluteToSelfRelativeSD(abssd, NULL, &buflen);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL) {
        ERR("RtlAbsoluteToSelfRelativeSD returned %08lx\n", Status);
        ExFreePool(usersid);
        ExFreePool(groupsid);
        ExFreePool(buf);
        return;
    }

    if (buflen == 0) {
        ERR("RtlAbsoluteToSelfRelativeSD returned a buffer size of 0\n");
        ExFreePool(usersid);
        ExFreePool(groupsid);
        ExFreePool(buf);
        return;
    }

    newsd = ExAllocatePoolWithTag(PagedPool, buflen, ALLOC_TAG);
    if (!newsd) {
        ERR("out of memory\n");
        ExFreePool(usersid);
        ExFreePool(groupsid);
        ExFreePool(buf);
        return;
    }

    Status = RtlAbsoluteToSelfRelativeSD(abssd, newsd, &buflen);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlAbsoluteToSelfRelativeSD returned %08lx\n", Status);
        ExFreePool(usersid);
        ExFreePool(groupsid);
        ExFreePool(buf);
        return;
    }

    ExFreePool(fcb->sd);
    fcb->sd = newsd;

    ExFreePool(usersid);
    ExFreePool(groupsid);
    ExFreePool(buf);
}

static NTSTATUS get_file_security(PFILE_OBJECT FileObject, SECURITY_DESCRIPTOR* relsd, ULONG* buflen, SECURITY_INFORMATION flags) {
    NTSTATUS Status;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;

    if (fcb->ads) {
        if (fileref && fileref->parent)
            fcb = fileref->parent->fcb;
        else {
            ERR("could not get parent fcb for stream\n");
            return STATUS_INTERNAL_ERROR;
        }
    }

    // Why (void**)? Is this a bug in mingw?
    Status = SeQuerySecurityDescriptorInfo(&flags, relsd, buflen, (void**)&fcb->sd);

    if (Status == STATUS_BUFFER_TOO_SMALL)
        TRACE("SeQuerySecurityDescriptorInfo returned %08lx\n", Status);
    else if (!NT_SUCCESS(Status))
        ERR("SeQuerySecurityDescriptorInfo returned %08lx\n", Status);

    return Status;
}

_Dispatch_type_(IRP_MJ_QUERY_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_query_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    SECURITY_DESCRIPTOR* sd;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    ULONG buflen;
    bool top_level;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    ccb* ccb = FileObject ? FileObject->FsContext2 : NULL;

    FsRtlEnterFileSystem();

    TRACE("query security\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_query_security(DeviceObject, Irp);
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!ccb) {
        ERR("no ccb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (Irp->RequestorMode == UserMode && !(ccb->access & READ_CONTROL)) {
        WARN("insufficient permissions\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.QuerySecurity.SecurityInformation & OWNER_SECURITY_INFORMATION)
        TRACE("OWNER_SECURITY_INFORMATION\n");

    if (IrpSp->Parameters.QuerySecurity.SecurityInformation & GROUP_SECURITY_INFORMATION)
        TRACE("GROUP_SECURITY_INFORMATION\n");

    if (IrpSp->Parameters.QuerySecurity.SecurityInformation & DACL_SECURITY_INFORMATION)
        TRACE("DACL_SECURITY_INFORMATION\n");

    if (IrpSp->Parameters.QuerySecurity.SecurityInformation & SACL_SECURITY_INFORMATION)
        TRACE("SACL_SECURITY_INFORMATION\n");

    TRACE("length = %lu\n", IrpSp->Parameters.QuerySecurity.Length);

    sd = map_user_buffer(Irp, NormalPagePriority);
    TRACE("sd = %p\n", sd);

    if (Irp->MdlAddress && !sd) {
        ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    buflen = IrpSp->Parameters.QuerySecurity.Length;

    Status = get_file_security(IrpSp->FileObject, sd, &buflen, IrpSp->Parameters.QuerySecurity.SecurityInformation);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information = IrpSp->Parameters.QuerySecurity.Length;
    else if (Status == STATUS_BUFFER_TOO_SMALL) {
        Irp->IoStatus.Information = buflen;
        Status = STATUS_BUFFER_OVERFLOW;
    } else
        Irp->IoStatus.Information = 0;

end:
    TRACE("Irp->IoStatus.Information = %Iu\n", Irp->IoStatus.Information);

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("returning %08lx\n", Status);

    FsRtlExitFileSystem();

    return Status;
}

static NTSTATUS set_file_security(device_extension* Vcb, PFILE_OBJECT FileObject, SECURITY_DESCRIPTOR* sd, PSECURITY_INFORMATION flags, PIRP Irp) {
    NTSTATUS Status;
    fcb* fcb = FileObject->FsContext;
    ccb* ccb = FileObject->FsContext2;
    file_ref* fileref = ccb ? ccb->fileref : NULL;
    SECURITY_DESCRIPTOR* oldsd;
    LARGE_INTEGER time;
    BTRFS_TIME now;

    TRACE("(%p, %p, %p, %lx)\n", Vcb, FileObject, sd, *flags);

    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;

    if (fcb->ads) {
        if (fileref && fileref->parent)
            fcb = fileref->parent->fcb;
        else {
            ERR("could not find parent fcb for stream\n");
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (!fcb || !ccb)
        return STATUS_INVALID_PARAMETER;

    ExAcquireResourceExclusiveLite(fcb->Header.Resource, true);

    if (is_subvol_readonly(fcb->subvol, Irp)) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }

    oldsd = fcb->sd;

    Status = SeSetSecurityDescriptorInfo(NULL, flags, sd, (void**)&fcb->sd, PagedPool, IoGetFileObjectGenericMapping());

    if (!NT_SUCCESS(Status)) {
        ERR("SeSetSecurityDescriptorInfo returned %08lx\n", Status);
        goto end;
    }

    ExFreePool(oldsd);

    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);

    fcb->inode_item.transid = Vcb->superblock.generation;

    if (!ccb->user_set_change_time)
        fcb->inode_item.st_ctime = now;

    fcb->inode_item.sequence++;

    fcb->sd_dirty = true;
    fcb->sd_deleted = false;
    fcb->inode_item_changed = true;

    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;

    mark_fcb_dirty(fcb);

    queue_notification_fcb(fileref, FILE_NOTIFY_CHANGE_SECURITY, FILE_ACTION_MODIFIED, NULL);

end:
    ExReleaseResourceLite(fcb->Header.Resource);

    return Status;
}

_Dispatch_type_(IRP_MJ_SET_SECURITY)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS __stdcall drv_set_security(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    ccb* ccb = FileObject ? FileObject->FsContext2 : NULL;
    device_extension* Vcb = DeviceObject->DeviceExtension;
    ULONG access_req = 0;
    bool top_level;

    FsRtlEnterFileSystem();

    TRACE("set security\n");

    top_level = is_top_level(Irp);

    if (Vcb && Vcb->type == VCB_TYPE_VOLUME) {
        Status = vol_set_security(DeviceObject, Irp);
        goto end;
    } else if (!Vcb || Vcb->type != VCB_TYPE_FS) {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    if (!ccb) {
        ERR("no ccb\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.SetSecurity.SecurityInformation & OWNER_SECURITY_INFORMATION) {
        TRACE("OWNER_SECURITY_INFORMATION\n");
        access_req |= WRITE_OWNER;
    }

    if (IrpSp->Parameters.SetSecurity.SecurityInformation & GROUP_SECURITY_INFORMATION) {
        TRACE("GROUP_SECURITY_INFORMATION\n");
        access_req |= WRITE_OWNER;
    }

    if (IrpSp->Parameters.SetSecurity.SecurityInformation & DACL_SECURITY_INFORMATION) {
        TRACE("DACL_SECURITY_INFORMATION\n");
        access_req |= WRITE_DAC;
    }

    if (IrpSp->Parameters.SetSecurity.SecurityInformation & SACL_SECURITY_INFORMATION) {
        TRACE("SACL_SECURITY_INFORMATION\n");
        access_req |= ACCESS_SYSTEM_SECURITY;
    }

    if (Irp->RequestorMode == UserMode && (ccb->access & access_req) != access_req) {
        Status = STATUS_ACCESS_DENIED;
        WARN("insufficient privileges\n");
        goto end;
    }

    Status = set_file_security(DeviceObject->DeviceExtension, FileObject, IrpSp->Parameters.SetSecurity.SecurityDescriptor,
                               &IrpSp->Parameters.SetSecurity.SecurityInformation, Irp);

end:
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    TRACE("returning %08lx\n", Status);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    FsRtlExitFileSystem();

    return Status;
}

static bool search_for_gid(fcb* fcb, PSID sid) {
    LIST_ENTRY* le;

    le = gid_map_list.Flink;
    while (le != &gid_map_list) {
        gid_map* gm = CONTAINING_RECORD(le, gid_map, listentry);

        if (RtlEqualSid(sid, gm->sid)) {
            fcb->inode_item.st_gid = gm->gid;
            return true;
        }

        le = le->Flink;
    }

    return false;
}

void find_gid(struct _fcb* fcb, struct _fcb* parfcb, PSECURITY_SUBJECT_CONTEXT subjcont) {
    NTSTATUS Status;
    TOKEN_OWNER* to;
    TOKEN_PRIMARY_GROUP* tpg;
    TOKEN_GROUPS* tg;

    if (parfcb && parfcb->inode_item.st_mode & S_ISGID) {
        fcb->inode_item.st_gid = parfcb->inode_item.st_gid;
        return;
    }

    ExAcquireResourceSharedLite(&mapping_lock, true);

    if (!subjcont || !subjcont->PrimaryToken || IsListEmpty(&gid_map_list)) {
        ExReleaseResourceLite(&mapping_lock);
        return;
    }

    Status = SeQueryInformationToken(subjcont->PrimaryToken, TokenOwner, (void**)&to);
    if (!NT_SUCCESS(Status))
        ERR("SeQueryInformationToken returned %08lx\n", Status);
    else {
        if (search_for_gid(fcb, to->Owner)) {
            ExReleaseResourceLite(&mapping_lock);
            ExFreePool(to);
            return;
        }

        ExFreePool(to);
    }

    Status = SeQueryInformationToken(subjcont->PrimaryToken, TokenPrimaryGroup, (void**)&tpg);
    if (!NT_SUCCESS(Status))
        ERR("SeQueryInformationToken returned %08lx\n", Status);
    else {
        if (search_for_gid(fcb, tpg->PrimaryGroup)) {
            ExReleaseResourceLite(&mapping_lock);
            ExFreePool(tpg);
            return;
        }

        ExFreePool(tpg);
    }

    Status = SeQueryInformationToken(subjcont->PrimaryToken, TokenGroups, (void**)&tg);
    if (!NT_SUCCESS(Status))
        ERR("SeQueryInformationToken returned %08lx\n", Status);
    else {
        ULONG i;

        for (i = 0; i < tg->GroupCount; i++) {
            if (search_for_gid(fcb, tg->Groups[i].Sid)) {
                ExReleaseResourceLite(&mapping_lock);
                ExFreePool(tg);
                return;
            }
        }

        ExFreePool(tg);
    }

    ExReleaseResourceLite(&mapping_lock);
}

NTSTATUS fcb_get_new_sd(fcb* fcb, file_ref* parfileref, ACCESS_STATE* as) {
    NTSTATUS Status;
    PSID owner;
    BOOLEAN defaulted;

    Status = SeAssignSecurityEx(parfileref ? parfileref->fcb->sd : NULL, as->SecurityDescriptor, (void**)&fcb->sd, NULL, fcb->type == BTRFS_TYPE_DIRECTORY,
                                SEF_SACL_AUTO_INHERIT, &as->SubjectSecurityContext, IoGetFileObjectGenericMapping(), PagedPool);

    if (!NT_SUCCESS(Status)) {
        ERR("SeAssignSecurityEx returned %08lx\n", Status);
        return Status;
    }

    Status = RtlGetOwnerSecurityDescriptor(fcb->sd, &owner, &defaulted);
    if (!NT_SUCCESS(Status)) {
        ERR("RtlGetOwnerSecurityDescriptor returned %08lx\n", Status);
        fcb->inode_item.st_uid = UID_NOBODY;
    } else {
        fcb->inode_item.st_uid = sid_to_uid(owner);
    }

    find_gid(fcb, parfileref ? parfileref->fcb : NULL, &as->SubjectSecurityContext);

    return STATUS_SUCCESS;
}
