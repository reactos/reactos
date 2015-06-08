////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: SecurSup.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Get/Set Security" dispatch entry points.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_SECURITY

#ifdef UDF_ENABLE_SECURITY

NTSTATUS UDFConvertToSelfRelative(
    IN OUT PSECURITY_DESCRIPTOR* SecurityDesc);

/*UCHAR FullControlSD[] = {
0x01, 0x00, 0x04, 0x80, 0x4c, 0x00, 0x00, 0x00,
0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x14, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38, 0x00,
0x02, 0x00, 0x00, 0x00, 0x00, 0x09, 0x18, 0x00,
0x00, 0x00, 0x00, 0x10, 0x01, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x18, 0x00,
0xff, 0x01, 0x1f, 0x00, 0x01, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00
};*/

/*************************************************************************
*
* Function: UDFGetSecurity()
*
* Description:
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant.
*
*************************************************************************/
NTSTATUS
UDFGetSecurity(
    IN PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    IN PIRP           Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    KdPrint(("UDFGetSecurity\n"));
//    BrutePoint();

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    ASSERT(!UDFIsFSDevObj(DeviceObject));
    //  Call the common Lock Control routine, with blocking allowed if
    //  synchronous
    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonGetSecurity(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    }

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFGetSecurity()


/*************************************************************************
*
* Function: UDFCommonGetSecurity()
*
* Description:
*  This is the common routine for getting Security (ACL) called
*  by both the fsd and fsp threads
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant
*
*************************************************************************/
NTSTATUS
UDFCommonGetSecurity(
    IN PtrUDFIrpContext PtrIrpContext,
    IN PIRP             Irp)
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    BOOLEAN             PostRequest = FALSE;
    BOOLEAN             CanWait = FALSE;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    BOOLEAN             AcquiredFCB = FALSE;
    PFILE_OBJECT        FileObject = NULL;
    PtrUDFFCB           Fcb = NULL;
    PtrUDFCCB           Ccb = NULL;
    PVOID               PtrSystemBuffer = NULL;
    ULONG               BufferLength = 0;

    KdPrint(("UDFCommonGetSecurity\n"));

    _SEH2_TRY {

        // First, get a pointer to the current I/O stack location.
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // Get the FCB and CCB pointers.
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);

/*        if(!Fcb->Vcb->ReadSecurity)
            try_return(RC = STATUS_NOT_IMPLEMENTED);*/

        NtReqFcb = Fcb->NTRequiredFCB;
        CanWait = ((PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);

        // Acquire the FCB resource shared
        UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
        if (!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
//        if (!UDFAcquireResourceShared(&(NtReqFcb->MainResource), CanWait)) {
            PostRequest = TRUE;
            try_return(RC = STATUS_PENDING);
        }
        AcquiredFCB = TRUE;

        PtrSystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
        if(!PtrSystemBuffer)
            try_return(RC = STATUS_INVALID_USER_BUFFER);
        BufferLength = IrpSp->Parameters.QuerySecurity.Length;

        if(!NtReqFcb->SecurityDesc) {
            RC = UDFAssignAcl(Fcb->Vcb, FileObject, Fcb, NtReqFcb);
            if(!NT_SUCCESS(RC))
                try_return(RC);
        }

        _SEH2_TRY {
            RC = SeQuerySecurityDescriptorInfo(&(IrpSp->Parameters.QuerySecurity.SecurityInformation),
                                          (PSECURITY_DESCRIPTOR)PtrSystemBuffer,
                                          &BufferLength,
                                          &(NtReqFcb->SecurityDesc) );
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            RC = STATUS_BUFFER_TOO_SMALL;
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        // Release the FCB resources if acquired.
        if (AcquiredFCB) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            AcquiredFCB = FALSE;
        }

        if (PostRequest) {
            // Perform appropriate post related processing here
            RC = UDFPostRequest(PtrIrpContext, Irp);
        } else
        if(!AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = BufferLength;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } // end of "__finally" processing

    return(RC);
}

#ifndef UDF_READ_ONLY_BUILD
#ifndef DEMO
/*************************************************************************
*
* Function: UDFSetSecurity()
*
* Description:
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant.
*
*************************************************************************/
NTSTATUS
UDFSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    IN PIRP           Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    KdPrint(("UDFSetSecurity\n"));
//    BrutePoint();

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    //  Call the common Lock Control routine, with blocking allowed if
    //  synchronous
    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonSetSecurity(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    }

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFSetSecurity()


/*************************************************************************
*
* Function: UDFCommonSetSecurity()
*
* Description:
*  This is the common routine for getting Security (ACL) called
*  by both the fsd and fsp threads
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant
*
*************************************************************************/
NTSTATUS
UDFCommonSetSecurity(
    IN PtrUDFIrpContext PtrIrpContext,
    IN PIRP             Irp)
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    BOOLEAN             PostRequest = FALSE;
    BOOLEAN             CanWait = FALSE;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    BOOLEAN             AcquiredFCB = FALSE;
    PFILE_OBJECT        FileObject = NULL;
    PtrUDFFCB           Fcb = NULL;
    PtrUDFCCB           Ccb = NULL;
    ACCESS_MASK         DesiredAccess = 0;

    KdPrint(("UDFCommonSetSecurity\n"));

    _SEH2_TRY {

        // First, get a pointer to the current I/O stack location.
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // Get the FCB and CCB pointers.
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);

        if(!Fcb->Vcb->WriteSecurity)
            try_return(RC = STATUS_NOT_IMPLEMENTED);

        NtReqFcb = Fcb->NTRequiredFCB;
        CanWait = ((PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);

        // Acquire the FCB resource exclusive
        UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
        if (!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
            PostRequest = TRUE;
            try_return(RC = STATUS_PENDING);
        }
        AcquiredFCB = TRUE;

//OWNER_SECURITY_INFORMATION
        if(IrpSp->Parameters.SetSecurity.SecurityInformation & OWNER_SECURITY_INFORMATION)
            DesiredAccess |= WRITE_OWNER;
//GROUP_SECURITY_INFORMATION
        if(IrpSp->Parameters.SetSecurity.SecurityInformation & GROUP_SECURITY_INFORMATION)
            DesiredAccess |= WRITE_OWNER;
//DACL_SECURITY_INFORMATION
        if(IrpSp->Parameters.SetSecurity.SecurityInformation & DACL_SECURITY_INFORMATION)
            DesiredAccess |= WRITE_DAC;
//SACL_SECURITY_INFORMATION 
        if(IrpSp->Parameters.SetSecurity.SecurityInformation & SACL_SECURITY_INFORMATION)
            DesiredAccess |= ACCESS_SYSTEM_SECURITY;

        _SEH2_TRY {
            UDFConvertToSelfRelative(&(NtReqFcb->SecurityDesc));

            KdDump(NtReqFcb->SecurityDesc, RtlLengthSecurityDescriptor(NtReqFcb->SecurityDesc));
            KdPrint(("\n"));

            RC = SeSetSecurityDescriptorInfo(/*FileObject*/ NULL,
                                          &(IrpSp->Parameters.SetSecurity.SecurityInformation),
                                          IrpSp->Parameters.SetSecurity.SecurityDescriptor,
                                          &(NtReqFcb->SecurityDesc),
                                          NonPagedPool,
                                          IoGetFileObjectGenericMapping() );

            KdDump(NtReqFcb->SecurityDesc, RtlLengthSecurityDescriptor(NtReqFcb->SecurityDesc));

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            RC = STATUS_INVALID_PARAMETER;
        }
        if(NT_SUCCESS(RC)) {
            NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_SD_MODIFIED;

            UDFNotifyFullReportChange( Fcb->Vcb, Fcb->FileInfo,
                                       FILE_NOTIFY_CHANGE_SECURITY,
                                       FILE_ACTION_MODIFIED);
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {

        // Release the FCB resources if acquired.
        if (AcquiredFCB) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            AcquiredFCB = FALSE;
        }

        if (PostRequest) {
            // Perform appropriate post related processing here
            RC = UDFPostRequest(PtrIrpContext, Irp);
        } else
        if(!AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } // end of "__finally" processing

    return(RC);
} // ens UDFCommonSetSecurity()

#endif //DEMO
#endif //UDF_READ_ONLY_BUILD
#endif //UDF_ENABLE_SECURITY

NTSTATUS
UDFReadSecurity(
    IN PVCB Vcb,
    IN PtrUDFFCB Fcb,
    IN PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
#ifdef UDF_ENABLE_SECURITY
    PUDF_FILE_INFO FileInfo = NULL;
    PUDF_FILE_INFO SDirInfo = NULL;
    PUDF_FILE_INFO AclInfo = NULL;
    NTSTATUS RC;
    ULONG NumberBytesRead;
    PERESOURCE Res1 = NULL;

    KdPrint(("UDFReadSecurity\n"));

    _SEH2_TRY {

        FileInfo = Fcb->FileInfo;
        ASSERT(FileInfo);
        if(!FileInfo) {
            KdPrint(("  Volume Security\n"));
            try_return(RC = STATUS_NO_SECURITY_ON_OBJECT);
        }
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
            KdPrint(("  No Security on blank volume\n"));
            try_return(RC = STATUS_NO_SECURITY_ON_OBJECT);
        }

        // Open Stream Directory
        RC = UDFOpenStreamDir__(Vcb, FileInfo, &SDirInfo);

        if(RC == STATUS_NOT_FOUND)
            try_return(RC = STATUS_NO_SECURITY_ON_OBJECT);
        if(!NT_SUCCESS(RC)) {
            if(UDFCleanUpFile__(Vcb, SDirInfo)) {
                if(SDirInfo) MyFreePool__(SDirInfo);
            }
            SDirInfo = NULL;
            try_return(RC);
        }
        // Acquire SDir exclusively if Fcb present
        if(SDirInfo->Fcb) {
            BrutePoint();
            UDF_CHECK_PAGING_IO_RESOURCE(SDirInfo->Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(Res1 = &(SDirInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
        }

        // Open Acl Stream
        RC = UDFOpenFile__(Vcb,
                           FALSE,TRUE,&(UDFGlobalData.AclName),
                           SDirInfo,&AclInfo,NULL);
        if(RC == STATUS_OBJECT_NAME_NOT_FOUND)
            try_return(RC = STATUS_NO_SECURITY_ON_OBJECT);
        if(!NT_SUCCESS(RC)) {
            if(UDFCleanUpFile__(Vcb, AclInfo)) {
                if(AclInfo) MyFreePool__(AclInfo);
            }
            AclInfo = NULL;
            try_return(RC);
        }

        NumberBytesRead = (ULONG)UDFGetFileSize(AclInfo);
        (*SecurityDesc) = DbgAllocatePool(NonPagedPool, NumberBytesRead);
        if(!(*SecurityDesc))
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        RC = UDFReadFile__(Vcb, AclInfo, 0, NumberBytesRead,
                       FALSE, (PCHAR)(*SecurityDesc), &NumberBytesRead);
        if(!NT_SUCCESS(RC))
            try_return(RC);

        RC = RtlValidSecurityDescriptor(*SecurityDesc);

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(AclInfo) {
            UDFCloseFile__(Vcb, AclInfo);
            if(UDFCleanUpFile__(Vcb, AclInfo))
                MyFreePool__(AclInfo);
        }

        if(SDirInfo) {
            UDFCloseFile__(Vcb, SDirInfo);
            if(UDFCleanUpFile__(Vcb, SDirInfo))
                MyFreePool__(SDirInfo);
        }

        if(!NT_SUCCESS(RC) && (*SecurityDesc)) {
            DbgFreePool(*SecurityDesc);
            (*SecurityDesc) = NULL;
        }
        if(Res1)
            UDFReleaseResource(Res1);
    }

    return RC;
#else
    return STATUS_NO_SECURITY_ON_OBJECT;
#endif //UDF_ENABLE_SECURITY

} // end UDFReadSecurity()

#ifdef UDF_ENABLE_SECURITY
NTSTATUS
UDFConvertToSelfRelative(
    IN OUT PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
    NTSTATUS RC;
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR NewSD;
    ULONG Len;

    KdPrint(("  UDFConvertToSelfRelative\n"));

    if(!(*SecurityDesc))
        return STATUS_NO_SECURITY_ON_OBJECT;

    SecurityInformation = FULL_SECURITY_INFORMATION;
    Len = RtlLengthSecurityDescriptor(*SecurityDesc);
    ASSERT(Len <= 1024);
    NewSD = (PSECURITY_DESCRIPTOR)DbgAllocatePool(NonPagedPool, Len);
    if(!NewSD)
        return STATUS_INSUFFICIENT_RESOURCES;
    _SEH2_TRY {
        RC = SeQuerySecurityDescriptorInfo(&SecurityInformation, NewSD, &Len, SecurityDesc);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        RC = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(NT_SUCCESS(RC)) {
        DbgFreePool(*SecurityDesc);
        *SecurityDesc = NewSD;
    } else {
        DbgFreePool(NewSD);
    }
    return RC;
} // end UDFConvertToSelfRelative()

NTSTATUS
UDFInheritAcl(
    IN PVCB Vcb,
    IN PSECURITY_DESCRIPTOR* ParentSecurityDesc,
    IN OUT PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
    NTSTATUS RC;
    SECURITY_INFORMATION SecurityInformation;
    ULONG Len;

    KdPrint(("  UDFInheritAcl\n"));

    if(!(*ParentSecurityDesc)) {
        *SecurityDesc = NULL;
        return STATUS_SUCCESS;
    }

    SecurityInformation = FULL_SECURITY_INFORMATION;
    Len = RtlLengthSecurityDescriptor(*ParentSecurityDesc);
    *SecurityDesc = (PSECURITY_DESCRIPTOR)DbgAllocatePool(NonPagedPool, Len);
    if(!(*SecurityDesc))
        return STATUS_INSUFFICIENT_RESOURCES;
    _SEH2_TRY {
        RC = SeQuerySecurityDescriptorInfo(&SecurityInformation, *SecurityDesc, &Len, ParentSecurityDesc);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        RC = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(!NT_SUCCESS(RC)) {
        DbgFreePool(*SecurityDesc);
        *SecurityDesc = NULL;
    }
    return RC;
} // end UDFInheritAcl()

NTSTATUS
UDFBuildEmptyAcl(
    IN PVCB Vcb,
    IN PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
    NTSTATUS RC;
    ULONG Len = 2 * (sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + sizeof(ULONG)*4 /*RtlLengthSid(SeExports->SeWorldSid)*/);

    KdPrint(("  UDFBuildEmptyAcl\n"));
    // Create Security Descriptor
    (*SecurityDesc) = (PSECURITY_DESCRIPTOR)DbgAllocatePool(NonPagedPool,
           sizeof(SECURITY_DESCRIPTOR) + Len);
    if(!(*SecurityDesc))
        return STATUS_INSUFFICIENT_RESOURCES;

    RC = RtlCreateSecurityDescriptor(*SecurityDesc, SECURITY_DESCRIPTOR_REVISION);

    if(!NT_SUCCESS(RC)) {
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
    }
    return RC;
} // end UDFBuildEmptyAcl()

NTSTATUS
UDFBuildFullControlAcl(
    IN PVCB Vcb,
    IN PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
    NTSTATUS RC;
    PACL Acl;
    ULONG Len = sizeof(ACL) + 2*(sizeof(ACCESS_ALLOWED_ACE) + sizeof(ULONG)*4 /*- sizeof(ULONG)*/ /*+ RtlLengthSid(SeExports->SeWorldSid)*/);

    KdPrint(("  UDFBuildFullControlAcl\n"));
    // Create Security Descriptor
    RC = UDFBuildEmptyAcl(Vcb, SecurityDesc);
    if(!NT_SUCCESS(RC))
        return RC;

    // Set Owner
    RC = RtlSetOwnerSecurityDescriptor(*SecurityDesc, SeExports->SeWorldSid, FALSE);
    if(!NT_SUCCESS(RC)) {
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }

    // Set Group
    RC = RtlSetGroupSecurityDescriptor(*SecurityDesc, SeExports->SeWorldSid, FALSE);
    if(!NT_SUCCESS(RC)) {
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }

    // Create empty Acl
    Acl = (PACL)DbgAllocatePool(NonPagedPool, Len);
    if(!Acl) {
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }
    RtlZeroMemory(Acl, Len);

    RC = RtlCreateAcl(Acl, Len, ACL_REVISION);
    if(!NT_SUCCESS(RC)) {
        DbgFreePool(Acl);
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }

    // Add (All)(All) access for Everyone
/*    RC = RtlAddAccessAllowedAce(Acl, ACL_REVISION,
                                GENERIC_ALL,
                                SeExports->SeWorldSid);*/

    RC = RtlAddAccessAllowedAce(Acl, ACL_REVISION,
                                FILE_ALL_ACCESS,
                                SeExports->SeWorldSid);

    if(!NT_SUCCESS(RC)) {
        DbgFreePool(Acl);
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }

    // Add Acl to Security Descriptor
    RC = RtlSetDaclSecurityDescriptor(*SecurityDesc, TRUE, Acl, FALSE);
    if(!NT_SUCCESS(RC)) {
        DbgFreePool(Acl);
        DbgFreePool(*SecurityDesc);
        *((PULONG)SecurityDesc) = NULL;
        return RC;
    }

    RC = UDFConvertToSelfRelative(SecurityDesc);

    DbgFreePool(Acl);

    return RC;
} // end UDFBuildFullControlAcl()

#endif // UDF_ENABLE_SECURITY

NTSTATUS
UDFAssignAcl(
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject, // OPTIONAL
    IN PtrUDFFCB Fcb,
    IN PtrUDFNTRequiredFCB NtReqFcb
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
#ifdef UDF_ENABLE_SECURITY
//    SECURITY_INFORMATION SecurityInformation;

//    KdPrint(("  UDFAssignAcl\n"));
    if(!NtReqFcb->SecurityDesc) {

        PSECURITY_DESCRIPTOR ExplicitSecurity = NULL;

        if(UDFIsAStreamDir(Fcb->FileInfo) || UDFIsAStream(Fcb->FileInfo)) {
            // Stream/SDir security
            NtReqFcb->SecurityDesc = Fcb->FileInfo->ParentFile->Dloc->CommonFcb->SecurityDesc;
            return STATUS_SUCCESS;
        } else
        if(!Fcb->FileInfo) {
            // Volume security
            if(Vcb->RootDirFCB &&
               Vcb->RootDirFCB->FileInfo &&
               Vcb->RootDirFCB->FileInfo->Dloc &&
               Vcb->RootDirFCB->FileInfo->Dloc->CommonFcb) {
                RC = UDFInheritAcl(Vcb, &(Vcb->RootDirFCB->FileInfo->Dloc->CommonFcb->SecurityDesc), &ExplicitSecurity);
            } else {
                NtReqFcb->SecurityDesc = NULL;
                RC = STATUS_NO_SECURITY_ON_OBJECT;
            }
            return RC;
        }

        RC = UDFReadSecurity(Vcb, Fcb, &ExplicitSecurity);
        if(RC == STATUS_NO_SECURITY_ON_OBJECT) {
            if(!Fcb->FileInfo->ParentFile) {
                RC = UDFBuildFullControlAcl(Vcb, &ExplicitSecurity);
            } else {
                RC = UDFInheritAcl(Vcb, &(Fcb->FileInfo->ParentFile->Dloc->CommonFcb->SecurityDesc), &ExplicitSecurity);
            }
/*            if(NT_SUCCESS(RC)) {
                NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_SD_MODIFIED;
            }*/
        }
        if(NT_SUCCESS(RC)) {

//            SecurityInformation = FULL_SECURITY_INFORMATION;
            NtReqFcb->SecurityDesc = ExplicitSecurity;

/*            RC = SeSetSecurityDescriptorInfo(FileObject,
                                          &SecurityInformation,
                                          ExplicitSecurity,
                                          &(NtReqFcb->SecurityDesc),
                                          NonPagedPool,
                                          IoGetFileObjectGenericMapping() );*/

        }
    }
#endif //UDF_ENABLE_SECURITY
    return RC;
} // end UDFAssignAcl()


VOID
UDFDeassignAcl(
    IN PtrUDFNTRequiredFCB NtReqFcb,
    IN BOOLEAN AutoInherited
    )
{
#ifdef UDF_ENABLE_SECURITY
//    NTSTATUS RC = STATUS_SUCCESS;

//    KdPrint(("  UDFDeassignAcl\n"));
    if(!NtReqFcb->SecurityDesc)
        return;

    if(AutoInherited) {
        NtReqFcb->SecurityDesc = NULL;
        return;
    }

    SeDeassignSecurity(&(NtReqFcb->SecurityDesc));
    NtReqFcb->SecurityDesc = NULL; // HA BCRK CLU4
#endif //UDF_ENABLE_SECURITY
    return;
} // end UDFDeassignAcl()

NTSTATUS
UDFWriteSecurity(
    IN PVCB Vcb,
    IN PtrUDFFCB Fcb,
    IN PSECURITY_DESCRIPTOR* SecurityDesc
    )
{
#ifdef UDF_ENABLE_SECURITY
    PUDF_FILE_INFO FileInfo = NULL;
    PUDF_FILE_INFO SDirInfo = NULL;
    PUDF_FILE_INFO AclInfo = NULL;
    PERESOURCE Res1 = NULL;
#ifndef DEMO
    NTSTATUS RC;
    ULONG NumberBytesRead;
#endif //DEMO

//    KdPrint(("UDFWriteSecurity\n"));

#if !defined(DEMO) && !defined(UDF_READ_ONLY_BUILD)

    if(!Vcb->WriteSecurity ||
       (Vcb->VCBFlags & (UDF_VCB_FLAGS_VOLUME_READ_ONLY |
                         UDF_VCB_FLAGS_MEDIA_READ_ONLY)))

#endif //!defined(DEMO) && !defined(UDF_READ_ONLY_BUILD)

        return STATUS_SUCCESS;

#if !defined(DEMO) && !defined(UDF_READ_ONLY_BUILD)

    _SEH2_TRY {

        FileInfo = Fcb->FileInfo;
        ASSERT(FileInfo);
        if(!FileInfo) {
            KdPrint(("  Volume Security\n"));
            try_return(RC = STATUS_SUCCESS);
        }

        if(!(Fcb->NTRequiredFCB->NtReqFCBFlags & UDF_NTREQ_FCB_SD_MODIFIED))
            try_return(RC = STATUS_SUCCESS);

        // Open Stream Directory
        RC = UDFOpenStreamDir__(Vcb, FileInfo, &SDirInfo);

        if(RC == STATUS_NOT_FOUND) {
            RC = UDFCreateStreamDir__(Vcb, FileInfo, &SDirInfo);
        }
        if(!NT_SUCCESS(RC)) {
            if(UDFCleanUpFile__(Vcb, SDirInfo)) {
                if(SDirInfo) MyFreePool__(SDirInfo);
            }
            SDirInfo = NULL;
            try_return(RC);
        }
        // Acquire SDir exclusively if Fcb present
        if(SDirInfo->Fcb) {
            BrutePoint();
            UDF_CHECK_PAGING_IO_RESOURCE(SDirInfo->Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(Res1 = &(SDirInfo->Fcb->NTRequiredFCB->MainResource),TRUE);
        }

        // Open Acl Stream
        RC = UDFOpenFile__(Vcb,
                           FALSE,TRUE,&(UDFGlobalData.AclName),
                           SDirInfo,&AclInfo,NULL);
        if(RC == STATUS_OBJECT_NAME_NOT_FOUND) {
            RC = UDFCreateFile__(Vcb, FALSE, &(UDFGlobalData.AclName),
                               0, 0, FALSE, FALSE, SDirInfo, &AclInfo);
        }
        if(!NT_SUCCESS(RC)) {
            if(UDFCleanUpFile__(Vcb, AclInfo)) {
                if(AclInfo) MyFreePool__(AclInfo);
            }
            AclInfo = NULL;
            try_return(RC);
        }

        if(!(*SecurityDesc)) {
            UDFFlushFile__(Vcb, AclInfo);
            RC = UDFUnlinkFile__(Vcb, AclInfo, TRUE);
            try_return(RC);
        }
        NumberBytesRead = RtlLengthSecurityDescriptor(*SecurityDesc);

        RC = UDFWriteFile__(Vcb, AclInfo, 0, NumberBytesRead,
                       FALSE, (PCHAR)(*SecurityDesc), &NumberBytesRead);
        if(!NT_SUCCESS(RC))
            try_return(RC);

        Fcb->NTRequiredFCB->NtReqFCBFlags &= ~UDF_NTREQ_FCB_SD_MODIFIED;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(AclInfo) {
            UDFCloseFile__(Vcb, AclInfo);
            if(UDFCleanUpFile__(Vcb, AclInfo))
                MyFreePool__(AclInfo);
        }

        if(SDirInfo) {
            UDFCloseFile__(Vcb, SDirInfo);
            if(UDFCleanUpFile__(Vcb, SDirInfo))
                MyFreePool__(SDirInfo);
        }
        if(Res1)
            UDFReleaseResource(Res1);
    }

    return RC;

#endif //!defined(DEMO) && !defined(UDF_READ_ONLY_BUILD)
#endif //UDF_ENABLE_SECURITY

    return STATUS_SUCCESS;

} // end UDFWriteSecurity()

PSECURITY_DESCRIPTOR
UDFLookUpAcl(
    IN PVCB      Vcb,
    PFILE_OBJECT FileObject, // OPTIONAL
    IN PtrUDFFCB Fcb
    )
{
    UDFAssignAcl(Vcb, FileObject, Fcb, Fcb->NTRequiredFCB);
    return (Fcb->NTRequiredFCB->SecurityDesc);
} // end UDFLookUpAcl()


NTSTATUS
UDFCheckAccessRights(
    PFILE_OBJECT FileObject, // OPTIONAL
    PACCESS_STATE AccessState,
    PtrUDFFCB    Fcb,
    PtrUDFCCB    Ccb,        // OPTIONAL
    ACCESS_MASK  DesiredAccess,
    USHORT       ShareAccess
    )
{
    NTSTATUS RC;
    BOOLEAN SecurityCheck = TRUE;
    BOOLEAN ROCheck = FALSE;
#ifdef UDF_ENABLE_SECURITY
    PSECURITY_DESCRIPTOR SecDesc;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ACCESS_MASK LocalAccessMask;
#endif //UDF_ENABLE_SECURITY

    // Check attr compatibility
    ASSERT(Fcb);
    ASSERT(Fcb->Vcb);
#ifdef UDF_READ_ONLY_BUILD
    goto treat_as_ro;
#endif //UDF_READ_ONLY_BUILD

#ifdef EVALUATION_TIME_LIMIT
    if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
        Fcb->Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
        goto treat_as_ro;
    }
#endif //EVALUATION_TIME_LIMIT
    if(Fcb->FCBFlags & UDF_FCB_READ_ONLY) {
        ROCheck = TRUE;
    } else
    if((Fcb->Vcb->origIntegrityType == INTEGRITY_TYPE_OPEN) &&
        Ccb && !(Ccb->CCBFlags & UDF_CCB_VOLUME_OPEN) &&
       (Fcb->Vcb->CompatFlags & UDF_VCB_IC_DIRTY_RO)) {
        AdPrint(("force R/O on dirty\n"));
        ROCheck = TRUE;
    }
    if(ROCheck) {
#if defined(EVALUATION_TIME_LIMIT) || defined(UDF_READ_ONLY_BUILD)
treat_as_ro:
#endif //EVALUATION_TIME_LIMIT
        ACCESS_MASK  DesiredAccessMask = 0;
       
        if(Fcb->Vcb->CompatFlags & UDF_VCB_IC_WRITE_IN_RO_DIR) {
            if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
                DesiredAccessMask = (FILE_WRITE_EA |
                                     DELETE);
            } else {
                DesiredAccessMask = (FILE_WRITE_DATA |
                                     FILE_APPEND_DATA |
                                     FILE_WRITE_EA |
                                     DELETE);
            }
        } else {
                DesiredAccessMask = (FILE_WRITE_DATA |
                                     FILE_APPEND_DATA |
                                     FILE_WRITE_EA |
                                     FILE_DELETE_CHILD |
                                     FILE_ADD_SUBDIRECTORY |
                                     FILE_ADD_FILE |
                                     DELETE);
        }
        if(DesiredAccess & DesiredAccessMask)
            return STATUS_ACCESS_DENIED;
    }
#ifdef UDF_ENABLE_SECURITY
    // Check Security
    // NOTE: we should not perform security check if an empty DesiredAccess
    // was specified. AFAIU, SeAccessCheck() will return FALSE in this case.
    SecDesc = UDFLookUpAcl(Fcb->Vcb, FileObject, Fcb);
    if(SecDesc && DesiredAccess) {
        SeCaptureSubjectContext(&SubjectContext);
        SecurityCheck =
            SeAccessCheck(SecDesc,
                          &SubjectContext,
                          FALSE,
                          DesiredAccess,
                          Ccb ? Ccb->PreviouslyGrantedAccess : 0,
                          NULL,
                          IoGetFileObjectGenericMapping(),
                          UserMode,
                          Ccb ? &(Ccb->PreviouslyGrantedAccess) : &LocalAccessMask,
                          &RC);
        SeReleaseSubjectContext(&SubjectContext);

        if(!SecurityCheck) {
            return RC;
        } else
#endif //UDF_ENABLE_SECURITY
        if(DesiredAccess & ACCESS_SYSTEM_SECURITY) {
            SecurityCheck = SeSinglePrivilegeCheck(SeExports->SeSecurityPrivilege, UserMode);
            if(!SecurityCheck)
                return STATUS_ACCESS_DENIED;
            Ccb->PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;
        }
#ifdef UDF_ENABLE_SECURITY
    }
#endif //UDF_ENABLE_SECURITY
#ifdef EVALUATION_TIME_LIMIT
    if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
        Fcb->Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
    }
#endif //EVALUATION_TIME_LIMIT
    if(FileObject) {
        if (Fcb->OpenHandleCount) {
            // The FCB is currently in use by some thread.
            // We must check whether the requested access/share access
            // conflicts with the existing open operations.
            RC = IoCheckShareAccess(DesiredAccess, ShareAccess, FileObject,
                                            &(Fcb->NTRequiredFCB->FCBShareAccess), TRUE);
#ifndef UDF_ENABLE_SECURITY
            if(Ccb)
                Ccb->PreviouslyGrantedAccess |= DesiredAccess;
            IoUpdateShareAccess(FileObject, &(Fcb->NTRequiredFCB->FCBShareAccess));
#endif //UDF_ENABLE_SECURITY
        } else {
            IoSetShareAccess(DesiredAccess, ShareAccess, FileObject, &(Fcb->NTRequiredFCB->FCBShareAccess));
#ifndef UDF_ENABLE_SECURITY
            if(Ccb)
                Ccb->PreviouslyGrantedAccess = DesiredAccess;
#endif //UDF_ENABLE_SECURITY
            RC = STATUS_SUCCESS;
        }
    } else {
        // we get here if given file was opened for internal purposes
        RC = STATUS_SUCCESS;
    }
#ifdef EVALUATION_TIME_LIMIT
    if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
        Fcb->Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
    }
#endif //EVALUATION_TIME_LIMIT
    return RC;
} // end UDFCheckAccessRights()

NTSTATUS
UDFSetAccessRights(
    PFILE_OBJECT FileObject,
    PACCESS_STATE AccessState,
    PtrUDFFCB    Fcb,
    PtrUDFCCB    Ccb,
    ACCESS_MASK  DesiredAccess,
    USHORT       ShareAccess
    )
{
#ifndef UDF_ENABLE_SECURITY
    ASSERT(Ccb);
    ASSERT(Fcb->FileInfo);

    return UDFCheckAccessRights(FileObject, AccessState, Fcb, Ccb, DesiredAccess, ShareAccess);

#else //UDF_ENABLE_SECURITY

    NTSTATUS RC;
    // Set Security on Object
    PSECURITY_DESCRIPTOR SecDesc;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    BOOLEAN AutoInherit;

    ASSERT(Ccb);
    ASSERT(Fcb->FileInfo);

    SecDesc = UDFLookUpAcl(Fcb->Vcb, FileObject, Fcb);
    AutoInherit = UDFIsAStreamDir(Fcb->FileInfo) || UDFIsAStream(Fcb->FileInfo);

    if(SecDesc && !AutoInherit) {
        // Get caller's User/Primary Group info
        SeCaptureSubjectContext(&SubjectContext);
        RC = SeAssignSecurity(
                         Fcb->FileInfo->ParentFile->Dloc->CommonFcb->SecurityDesc,
//                         NULL,
                         AccessState->SecurityDescriptor,
                         &(Fcb->NTRequiredFCB->SecurityDesc),
                         UDFIsADirectory(Fcb->FileInfo),
                         &SubjectContext,
                         IoGetFileObjectGenericMapping(),
                         NonPagedPool);
        SeReleaseSubjectContext(&SubjectContext);
        UDFConvertToSelfRelative(&(Fcb->NTRequiredFCB->SecurityDesc));

        if(!NT_SUCCESS(RC)) {
Clean_Up_SD:
            UDFDeassignAcl(Fcb->NTRequiredFCB, AutoInherit);
            return RC;
        }
    }

    RC = UDFCheckAccessRights(FileObject, AccessState, Fcb, Ccb, DesiredAccess, ShareAccess);
    if(!NT_SUCCESS(RC))
        goto Clean_Up_SD;
    return RC;

#endif //UDF_ENABLE_SECURITY

} // end UDFSetAccessRights()

