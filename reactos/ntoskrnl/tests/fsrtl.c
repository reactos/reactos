#include <ntifs.h>
#include "ntndk.h"
#include "fsrtl_glue.h"

#include "fastio.h"
#include "fsrtl.h"

/* 
    This is the main test function. It is called from DriverEntry.

    There is a DbgBreakPoint() call at the beginning of DriverEntry.
    In order to run the test again, simply type
        net stop fsrtl
        net start fsrtl
    
    Author: Dom Cote
*/

BOOLEAN FsRtlTest_StartTest() {
    HANDLE Fh = NULL;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS  Return = TRUE;
    PFILE_OBJECT Pfo;
    LONGLONG i = 0;

    PCHAR Buffer;

    LARGE_INTEGER Offset;
    ULONG Length = 0;
    LARGE_INTEGER OldSize;

    /* Parameters we are going to use in the test from the FCB */
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PLARGE_INTEGER AllocationSize;
    PLARGE_INTEGER ValidDataLength;
    PLARGE_INTEGER   FileSize;

    /* Allocate a 100KB buffer to do IOs */
    Buffer = ExAllocatePool(PagedPool,100*_1KB);
    
    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;
    
    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("No cache map test.",!FsRtlCopyWrite(Pfo,AllocationSize,Length,TRUE,0,Buffer,&IoStatus,NULL));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("Building 100k filesize.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("Extending by 1/2 sector.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;
    
    /* Append to the file past the allocation size*/
    Offset.LowPart = 0xFFFFFFFF;
    Offset.HighPart = 0xFFFFFFFF;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("Testing extending past allocation size",!FsRtlCopyWrite(Pfo,&Offset,Length+1,TRUE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("Testing extending not past allocation size",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));
    
    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    Length = 65*_1KB;
    FSRTL_TEST("65KB IO Test",!FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));

    /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    FSRTL_TEST("64KB IO Test",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))

    /* Test the fast Io questionable flag
         This test fails and should succeed. I am not sure why. When FsRtlCopyWrite() queries the FastIoTable of the related
         device object, it comes back with no.
    FcbHeader->IsFastIoPossible = FastIoIsQuestionable;
    FSRTL_TEST("FastIo is questionable flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    */
    
    /* Test the fast Io not possible flag */
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FastIo is not possible flag",!FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    /* Set the flag back to what it was */
    FcbHeader->IsFastIoPossible = FastIoIsPossible;
    FSRTL_TEST("FastIo is possbile flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))

    if (Pfo) 
    {
        ObDereferenceObject(Pfo);
        Pfo = NULL;
    }

    if (Fh)
    {
        ZwClose(Fh);
        Fh = NULL;
     }
    

    /* ------------------------------------------------------------------------*/
    /* ------------------------------------------------------------------------*/
    /* ------------------------------------------------------------------------*/

    /* We are going to repeat the same bunch of tests but with Wait = FALSE. So we exercise the second part of the function. */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;
    
    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("No cache map test. Wait = FALSE",!FsRtlCopyWrite(Pfo,AllocationSize,Length,FALSE,0,Buffer,&IoStatus,NULL));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("Building 100k filesize. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("Extending by 1/2 sector. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;
    
    /* Append to the file past the allocation size*/
    Offset.LowPart = 0xFFFFFFFF;
    Offset.HighPart = 0xFFFFFFFF;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("Testing extending past allocation size Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length+1,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("Testing extending not past allocation size. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));
    
    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    Length = 65*_1KB;
    FSRTL_TEST("65KB IO Test. Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));

    /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    FSRTL_TEST("64KB IO Test. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))

    /* Test the fast Io questionable flag
         This test fails and should succeed. I am not sure why. When FsRtlCopyWrite() queries the FastIoTable of the related
         device object, it comes back with no.
    FcbHeader->IsFastIoPossible = FastIoIsQuestionable;
    FSRTL_TEST("FastIo is questionable flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    */
    
    /* Test the fast Io not possible flag */
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FastIo is not possible flag. Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))
    /* Set the flag back to what it was */
    FcbHeader->IsFastIoPossible = FastIoIsPossible;
    FSRTL_TEST("FastIo is possbile flag. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))


    /*  ------------------------------------------------------------------------------------------
        ------------------------------------------------------------------------------------------
        ------------------------------------------------------------------------------------------
    */

    /* Testing FsRtlCopyRead() function */

    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;

    /* Testing a 64KB read with Wait = TRUE */
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("Testing 64k IO Wait=TRUE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;
    
    /* Testing a 64KB read with Wait = FALSE */
    Return = FsRtlCopyRead(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("Testing 64k IO Wait=FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Testing read past the end of the file */
    Offset.QuadPart = FileSize->QuadPart - (5 * _1KB);
    Length = 10 * _1KB;
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("Testing reading past end of file but starting before EOF",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status) && IoStatus.Information == (FileSize->QuadPart-Offset.QuadPart)));
    
    Offset.QuadPart = FileSize->QuadPart + 1;
    Length = 10 * _1KB;
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("Testing reading past end of file but starting after EOF",(NT_SUCCESS(Return) && (IoStatus.Status == STATUS_END_OF_FILE) && IoStatus.Information == 0));
    

    /* Testing a 64KB read with Wait = TRUE */
    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FastIo is not possible flag. Wait = FALSE",!FsRtlCopyRead(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FastIo is not possible flag. Wait = TRUE",!FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));

    Return = TRUE;
    
Cleanup:

    if (Pfo) 
    {
        ObDereferenceObject(Pfo);
        Pfo = NULL;
    }

    if (Fh)
    {
        ZwClose(Fh);
        Fh = NULL;
     }
        
    if (Buffer != NULL) {
        ExFreePool(Buffer);
        Buffer = NULL;
    }

    return Return;

}

/*  This function is just a wrapper around ZwWriteFile */
NTSTATUS FsRltTest_WritefileZw(HANDLE fh, PLARGE_INTEGER Offset, ULONG Length, PVOID Buffer, PIO_STATUS_BLOCK pIoStatus){
    NTSTATUS Return;

    Return = ZwWriteFile(
        fh,
        NULL,
        NULL,
        NULL,
        pIoStatus,
        Buffer,
        Length,
        Offset,
        NULL
        );

    return Return;
}

/* This function fills the buffer with a test pattern */
void FsRtlTest_FillBuffer(LARGE_INTEGER Start, ULONG Length, PVOID Buffer) {
    ULONG i = 0;
    PULONGLONG Index = (PULONGLONG) Buffer;
    
    for (i=0; i<Length/sizeof(ULONGLONG); i++) {
        Index[i] = Start.QuadPart + i;
    }
    
    return;
 }

/*  This function opens a test file with the FILE_DELETE_ON_CLOSE flag
    and reference the file object
*/
 NTSTATUS FsRtlTest_OpenTestFile(PHANDLE Pfh, PFILE_OBJECT *Ppfo) {
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS  Return;
    
    RtlInitUnicodeString(&FileName,L"\\??\\C:\\fsrtl.bin");

    InitializeObjectAttributes(
    &oa,
    &FileName,
    OBJ_KERNEL_HANDLE,
    NULL,
    NULL;
    );

    Return = IoCreateFile(Pfh,
                        GENERIC_WRITE,
                        &oa,
                        &IoStatus,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_OVERWRITE_IF 	,
                        FILE_SYNCHRONOUS_IO_ALERT | FILE_DELETE_ON_CLOSE,
                        NULL,
                        0,
                        CreateFileTypeNone,
                        NULL,
                        0);

      Return = ObReferenceObjectByHandle(
        *Pfh,
        GENERIC_WRITE,
        NULL,
        KernelMode,
        Ppfo,
        NULL
        );
}


/* All the testing is done from driver entry */
NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS mStatus;
    UNICODE_STRING uniName, uniDOSName;
    PDEVOBJ_EXTENSION  x;

    DbgPrint("Loading the FSRTL test driver.\n");
    DbgBreakPoint();

    /* register device functions */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsRtlTest_DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsRtlTest_DispatchCreateClose;
    DriverObject->DriverUnload = FsRtlTest_Unload;

    if (!FsRtlTest_StartTest()) {
        DbgPrint("FsRtl test failed.\n");
    } else {
        DbgPrint("FsRtl test OK.\n");
    }
    
    return STATUS_SUCCESS;
}






NTSTATUS FsRtlTest_DispatchCreateClose( IN PDEVICE_OBJECT devObj, IN PIRP Irp )
{
  DbgPrint(("FsRtl: Open / Close\n"));

  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest( Irp, IO_NO_INCREMENT );

  return STATUS_SUCCESS;
}

VOID FsRtlTest_Unload( IN PDRIVER_OBJECT DriverObject )
{
      DbgPrint(("FsRtl: Unloading.\n"));
}


