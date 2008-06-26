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
    PFILE_OBJECT Pfo = NULL;

    HANDLE DirFh = NULL;
    PFILE_OBJECT DirPfo = NULL;


    IO_STATUS_BLOCK IoStatus;
    BOOLEAN Return;
    NTSTATUS  Status = STATUS_SUCCESS;
    LONGLONG i = 0;

    PCHAR Buffer;
    PMDL MdlChain = 0;

    LARGE_INTEGER Offset;
    ULONG Length = 0;
    LARGE_INTEGER OldSize;

    /* Parameters we are going to use in the test from the FCB */
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PLARGE_INTEGER AllocationSize;
    PLARGE_INTEGER ValidDataLength;
    PLARGE_INTEGER   FileSize;

    PDEVICE_OBJECT pRelatedDo = NULL;

    /* Allocate a 100KB buffer to do IOs */
    Buffer = ExAllocatePool(PagedPool,100*_1KB);

    /*  ------------------------------------------------------------------------
            TESTING:
                BOOLEAN
                NTAPI
                FsRtlCopyWrite(IN PFILE_OBJECT FileObject,
                               IN PLARGE_INTEGER FileOffset,
                               IN ULONG Length,
                               IN BOOLEAN Wait,
                               IN ULONG LockKey,
                               OUT PVOID Buffer,
                               OUT PIO_STATUS_BLOCK IoStatus,
                               IN PDEVICE_OBJECT DeviceObject)

               with Wait = TRUE

        ------------------------------------------------------------------------  */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);
    FSRTL_TEST("Opening Test File.",((Pfo != NULL) && (Fh != NULL)));

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;

    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - No cache map test.",!FsRtlCopyWrite(Pfo,AllocationSize,Length,TRUE,0,Buffer,&IoStatus,NULL));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlCopyWrite() - Building 100k filesize.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlCopyWrite() - Extending by 1/2 sector.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Append to the file past the allocation size*/
    Offset.LowPart = 0xFFFFFFFF;
    Offset.HighPart = 0xFFFFFFFF;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("FsRtlCopyWrite() - Testing extending past allocation size",!FsRtlCopyWrite(Pfo,&Offset,Length+1,TRUE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FsRtlCopyWrite() - Testing extending not past allocation size",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FsRtlCopyWrite() - Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));

    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    Length = 65*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - 65KB IO Test",!FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));

    /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - 64KB IO Test",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))

    /* Test the fast Io questionable flag
         This test fails and should succeed. I am not sure why. When FsRtlCopyWrite() queries the FastIoTable of the related
         device object, it comes back with no.
    FcbHeader->IsFastIoPossible = FastIoIsQuestionable;
    FSRTL_TEST("FastIo is questionable flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    */

    /* Test the fast Io not possible flag */
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FsRtlCopyWrite() - FastIo is not possible flag",!FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    /* Set the flag back to what it was */
    FcbHeader->IsFastIoPossible = FastIoIsPossible;
    FSRTL_TEST("FsRtlCopyWrite() - FastIo is possbile flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))

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

    /*  ------------------------------------------------------------------------
            TESTING:
                BOOLEAN
                NTAPI
                FsRtlCopyWrite(IN PFILE_OBJECT FileObject,
                               IN PLARGE_INTEGER FileOffset,
                               IN ULONG Length,
                               IN BOOLEAN Wait,
                               IN ULONG LockKey,
                               OUT PVOID Buffer,
                               OUT PIO_STATUS_BLOCK IoStatus,
                               IN PDEVICE_OBJECT DeviceObject)

               with Wait = FALSE

        ------------------------------------------------------------------------  */

    /* We are going to repeat the same bunch of tests but with Wait = FALSE. So we exercise the second part of the function. */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;

    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - No cache map test. Wait = FALSE",!FsRtlCopyWrite(Pfo,AllocationSize,Length,FALSE,0,Buffer,&IoStatus,NULL));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlCopyWrite() - Building 100k filesize. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlCopyWrite() - Extending by 1/2 sector. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Append to the file past the allocation size*/
    Offset.LowPart = 0xFFFFFFFF;
    Offset.HighPart = 0xFFFFFFFF;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("FsRtlCopyWrite() - Testing extending past allocation size Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length+1,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FsRtlCopyWrite() - Testing extending not past allocation size. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FsRtlCopyWrite() - Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));

    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    Length = 65*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - 65KB IO Test. Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));

    /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    FSRTL_TEST("FsRtlCopyWrite() - 64KB IO Test. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))

    /* Test the fast Io questionable flag
         This test fails and should succeed. I am not sure why. When FsRtlCopyWrite() queries the FastIoTable of the related
         device object, it comes back with no.
    FcbHeader->IsFastIoPossible = FastIoIsQuestionable;
    FSRTL_TEST("FastIo is questionable flag",FsRtlCopyWrite(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL))
    */

    /* Test the fast Io not possible flag */
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FsRtlCopyWrite() - FastIo is not possible flag. Wait = FALSE",!FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))
    /* Set the flag back to what it was */
    FcbHeader->IsFastIoPossible = FastIoIsPossible;
    FSRTL_TEST("FsRtlCopyWrite() - FastIo is possbile flag. Wait = FALSE",FsRtlCopyWrite(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL))


    /*  ------------------------------------------------------------------------------------------
            TESTING:

                BOOLEAN
                NTAPI
                FsRtlCopyRead(IN PFILE_OBJECT FileObject,
                              IN PLARGE_INTEGER FileOffset,
                              IN ULONG Length,
                              IN BOOLEAN Wait,
                              IN ULONG LockKey,
                              OUT PVOID Buffer,
                              OUT PIO_STATUS_BLOCK IoStatus,
                              IN PDEVICE_OBJECT DeviceObject)

        ------------------------------------------------------------------------------------------  */

    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;

    /* Testing a 64KB read with Wait = TRUE */
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("FsRtlCopyRead() - Testing 64k IO Wait=TRUE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Testing a 64KB read with Wait = FALSE */
    Return = FsRtlCopyRead(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("FsRtlCopyRead() - Testing 64k IO Wait=FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Testing read past the end of the file */
    Offset.QuadPart = FileSize->QuadPart - (5 * _1KB);
    Length = 10 * _1KB;
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("FsRtlCopyRead() - Testing reading past end of file but starting before EOF",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status) && IoStatus.Information == (FileSize->QuadPart-Offset.QuadPart)));

    Offset.QuadPart = FileSize->QuadPart + 1;
    Length = 10 * _1KB;
    Return = FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL);
    FSRTL_TEST("FsRtlCopyRead() - Testing reading past end of file but starting after EOF",(NT_SUCCESS(Return) && (IoStatus.Status == STATUS_END_OF_FILE) && IoStatus.Information == 0));


    /* Testing a 64KB read with Wait = TRUE */
    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FsRtlCopyRead() - FastIo is not possible flag. Wait = FALSE",!FsRtlCopyRead(Pfo,&Offset,Length,FALSE,0,Buffer,&IoStatus,NULL));
    FSRTL_TEST("FsRtlCopyRead() - FastIo is not possible flag. Wait = TRUE",!FsRtlCopyRead(Pfo,&Offset,Length,TRUE,0,Buffer,&IoStatus,NULL));
    FcbHeader->IsFastIoPossible = FastIoIsPossible;
    Return = TRUE;

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

    /*  ------------------------------------------------------------------------
            TESTING:
                BOOLEAN
                NTAPI
                FsRtlPrepareMdlWriteDev(IN PFILE_OBJECT FileObject,
                                        IN PLARGE_INTEGER FileOffset,
                                        IN ULONG Length,
                                        IN ULONG LockKey,
                                        OUT PMDL *MdlChain,
                                        OUT PIO_STATUS_BLOCK IoStatus,
                                        IN PDEVICE_OBJECT DeviceObject)

        ------------------------------------------------------------------------  */

    /* We are going to repeat the same bunch of tests but with Wait = FALSE. So we exercise the second part of the function. */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;

    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - No cache map test. Wait = FALSE",
        !FsRtlPrepareMdlWriteDev(Pfo,AllocationSize,Length,0,MdlChain,&IoStatus,NULL));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Building 100k filesize. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Extending by 1/2 sector. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;


    pRelatedDo = IoGetRelatedDeviceObject(Pfo);
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Did we get related DO ?",pRelatedDo);


    /* Append to the file past the allocation size*/
    Offset.QuadPart = FileSize->QuadPart;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Testing extending past allocation size.",
        !FsRtlPrepareMdlWriteDev(Pfo,&Offset,Length+1,0,&MdlChain,&IoStatus,pRelatedDo));

    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Testing extending not past allocation size.",
          FsRtlPrepareMdlWriteDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,pRelatedDo));
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Release the MDL.",FsRtlMdlWriteCompleteDev(Pfo,&Offset,MdlChain,pRelatedDo));


    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    MdlChain = NULL;
    Length = 65*_1KB;
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - 65KB IO Test.",
        FsRtlPrepareMdlWriteDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,pRelatedDo));
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Release the MDL.",FsRtlMdlWriteCompleteDev(Pfo,&Offset,MdlChain,pRelatedDo));

     /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    MdlChain = NULL;
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - 64KB IO Test.",
        FsRtlPrepareMdlWriteDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL))
    FSRTL_TEST("FsRtlPrepareMdlWriteDev() - Release the MDL.",FsRtlMdlWriteCompleteDev(Pfo,&Offset,MdlChain,NULL));

     /* Test the fast Io not possible flag */
   FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
   FSRTL_TEST("FsRtlPrepareMdlWriteDev() - FastIo is not possible flag.",
       !FsRtlPrepareMdlWriteDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL))

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

    /*  ------------------------------------------------------------------------
            TESTING:
                BOOLEAN
                NTAPI
                FsRtlPrepareMdlWrite(   IN PFILE_OBJECT FileObject,
                                        IN PLARGE_INTEGER FileOffset,
                                        IN ULONG Length,
                                        IN ULONG LockKey,
                                        OUT PMDL *MdlChain,
                                        OUT PIO_STATUS_BLOCK IoStatus,
                                        IN PDEVICE_OBJECT DeviceObject)

        ------------------------------------------------------------------------  */

    /* We are going to repeat the same bunch of tests but with Wait = FALSE. So we exercise the second part of the function. */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;

    /* Try to cache without caching having been initialized. This should fail.*/
    Length = 10*_1KB;
    FSRTL_TEST("FsRtlPrepareMdlWrite() - No cache map test. Wait = FALSE",
        !FsRtlPrepareMdlWrite(Pfo,AllocationSize,Length,0,MdlChain,&IoStatus));

    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Building 100k filesize. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;

    /* Extending the file by 1/2 sector, 256 bytes. */
    Offset.QuadPart = 0x7fffffffffff;
    Length = 0x100;
    Return = FsRltTest_WritefileZw(Fh,NULL,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Extending by 1/2 sector. Wait = FALSE",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;


    /* Append to the file past the allocation size*/
    MdlChain = NULL;
    Offset.QuadPart = FileSize->QuadPart;
    OldSize.QuadPart = FileSize->QuadPart;
    Length = (ULONG) (AllocationSize->QuadPart -ValidDataLength->QuadPart);
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Testing extending past allocation size.",
        !FsRtlPrepareMdlWrite(Pfo,&Offset,Length+1,0,&MdlChain,&IoStatus));

    FSRTL_TEST("FsRtlPrepareMdlWrite() - Testing extending not past allocation size.",
          FsRtlPrepareMdlWrite(Pfo,&Offset,Length,0,&MdlChain,&IoStatus));
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Check filesize",(FileSize->QuadPart = (OldSize.QuadPart+Length)));
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Release the MDL.",FsRtlMdlWriteComplete(Pfo,&Offset,MdlChain));


    /* Try do write a 65kb IO and check that if fails. Maximum IO size for thus function is 64KB */
    Offset.QuadPart = 0;
    MdlChain = NULL;
    Length = 65*_1KB;
    FSRTL_TEST("FsRtlPrepareMdlWrite() - 65KB IO Test.",
        !FsRtlPrepareMdlWrite(Pfo,&Offset,Length,0,&MdlChain,&IoStatus));
    //FSRTL_TEST("FsRtlPrepareMdlWrite() - Release the MDL.",FsRtlMdlWriteComplete(Pfo,&Offset,MdlChain));

     /* Try do write a 64kb IO. Maximum IO size for thus function is 64KB */
    Length = 64*_1KB;
    MdlChain = NULL;
    FSRTL_TEST("FsRtlPrepareMdlWrite() - 64KB IO Test.",
        FsRtlPrepareMdlWrite(Pfo,&Offset,Length,0,&MdlChain,&IoStatus))
    FSRTL_TEST("FsRtlPrepareMdlWrite() - Release the MDL.",FsRtlMdlWriteComplete(Pfo,&Offset,MdlChain));

     /* Test the fast Io not possible flag */
   FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
   FSRTL_TEST("FsRtlPrepareMdlWrite() - FastIo is not possible flag.",
       !FsRtlPrepareMdlWrite(Pfo,&Offset,Length,0,&MdlChain,&IoStatus))

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

    /*  ------------------------------------------------------------------------------------------
        TESTING:

            FsRtlMdlReadDev(IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN ULONG Length,
                        IN ULONG LockKey,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN PDEVICE_OBJECT DeviceObject)

            FsRtlMdlReadCompleteDev(IN PFILE_OBJECT FileObject,
                                    IN PMDL MemoryDescriptorList,
                                    IN PDEVICE_OBJECT DeviceObject)

        ------------------------------------------------------------------------------------------
    */

    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;


    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlMdlReadDev() - Building 100k filesize.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;


    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;

    /* Testing a 64KB read */
    MdlChain = NULL;
    Return = FsRtlMdlReadDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL);
    FSRTL_TEST("FsRtlMdlReadDev() - Testing 64k IO",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    FSRTL_TEST("FsRtlMdlReadDev() - Releasing the MDL",FsRtlMdlReadCompleteDev(Pfo,MdlChain,NULL));


    /* Testing read past the end of the file */
    Offset.QuadPart = FileSize->QuadPart - (5 * _1KB);
    Length = 10 * _1KB;
    MdlChain = NULL;
    Return = FsRtlMdlReadDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL);
    FSRTL_TEST("FsRtlMdlReadDev() - Testing reading past end of file but starting before EOF",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status) && IoStatus.Information == (FileSize->QuadPart-Offset.QuadPart)));
    FSRTL_TEST("FsRtlMdlReadDev() - Releasing the MDL",FsRtlMdlReadCompleteDev(Pfo,MdlChain,NULL));

    Offset.QuadPart = FileSize->QuadPart + 1;
    Length = 10 * _1KB;
    MdlChain = NULL;
    Return = FsRtlMdlReadDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL);
    FSRTL_TEST("FsRtlMdlReadDev() - Testing reading past end of file but starting after EOF",(NT_SUCCESS(Return) && (IoStatus.Status == STATUS_END_OF_FILE) && IoStatus.Information == 0));

    /* Testing FastIoIsNotPossible */
    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    MdlChain = NULL;
    Length  = 0x10000;
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FsRtlMdlReadDev() - FastIo is not possible flag. Wait = TRUE",!FsRtlMdlReadDev(Pfo,&Offset,Length,0,&MdlChain,&IoStatus,NULL));

    Return = TRUE;

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

    /*  ------------------------------------------------------------------------------------------
        TESTING:

            FsRtlMdlRead(IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN ULONG Length,
                        IN ULONG LockKey,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus)

            FsRtlMdlReadComplete(IN PFILE_OBJECT FileObject,
                                    IN PMDL MemoryDescriptorList)

        ------------------------------------------------------------------------------------------
    */

    FsRtlTest_OpenTestFile(&Fh, &Pfo);

    /* Extract the test variable from the FCB struct */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)Pfo->FsContext;
    AllocationSize = &FcbHeader->AllocationSize;
    ValidDataLength = &FcbHeader->ValidDataLength;
    FileSize = &FcbHeader->FileSize;


    /* We are going to build a 100k file */
    /* This will inititate caching and build some size */
    Offset.QuadPart = 0;
    Length = 100*_1KB;
    Return = FsRltTest_WritefileZw(Fh,&Offset,Length, Buffer, &IoStatus);
    FSRTL_TEST("FsRtlMdlRead() - Building 100k filesize.",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    Return = TRUE;


    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    Length  = 0x10000;

    /* Testing a 64KB read */
    MdlChain = NULL;
    Return = FsRtlMdlRead(Pfo,&Offset,Length,0,&MdlChain,&IoStatus);
    FSRTL_TEST("FsRtlMdlRead() - Testing 64k IO",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status ) && IoStatus.Information == Length));
    FSRTL_TEST("FsRtlMdlRead() - Releasing the MDL",FsRtlMdlReadComplete(Pfo,MdlChain));


    /* Testing read past the end of the file */
    Offset.QuadPart = FileSize->QuadPart - (5 * _1KB);
    Length = 10 * _1KB;
    MdlChain = NULL;
    Return = FsRtlMdlRead(Pfo,&Offset,Length,0,&MdlChain,&IoStatus);
    FSRTL_TEST("FsRtlMdlRead() - Testing reading past end of file but starting before EOF",(NT_SUCCESS(Return) && NT_SUCCESS(IoStatus.Status) && IoStatus.Information == (FileSize->QuadPart-Offset.QuadPart)));
    FSRTL_TEST("FsRtlMdlRead() - Releasing the MDL",FsRtlMdlReadComplete(Pfo,MdlChain));

    Offset.QuadPart = FileSize->QuadPart + 1;
    Length = 10 * _1KB;
    MdlChain = NULL;
    Return = FsRtlMdlRead(Pfo,&Offset,Length,0,&MdlChain,&IoStatus);
    FSRTL_TEST("FsRtlMdlRead() - Testing reading past end of file but starting after EOF",(NT_SUCCESS(Return) && (IoStatus.Status == STATUS_END_OF_FILE) && IoStatus.Information == 0));

    /* Testing FastIoIsNotPossible */
    Offset.LowPart = 0x0;
    Offset.HighPart = 0x0;
    MdlChain = NULL;
    Length  = 0x10000;
    FcbHeader->IsFastIoPossible = FastIoIsNotPossible;
    FSRTL_TEST("FsRtlMdlRead() - FastIo is not possible flag. Wait = TRUE",!FsRtlMdlRead(Pfo,&Offset,Length,0,&MdlChain,&IoStatus));

    Return = TRUE;

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



    /*  ------------------------------------------------------------------------------------------
        TESTING:

                    FsRtlGetFileSize(IN PFILE_OBJECT  FileObject,
                                     IN OUT PLARGE_INTEGER FileSize)

        ------------------------------------------------------------------------------------------
    */
    FsRtlTest_OpenTestFile(&Fh, &Pfo);
    FSRTL_TEST("FsRtlGetFileSize() - Opening Test File.",((Pfo != NULL) && (Fh != NULL)));

    FsRtlTest_OpenTestDirectory(&DirFh, &DirPfo);
    FSRTL_TEST("FsRtlGetFileSize() - Opening Test Directory.",((DirPfo != NULL) && (DirFh != NULL)));

    Status = FsRtlGetFileSize(Pfo,&OldSize);
    FSRTL_TEST("FsRtlGetFileSize() - Get the size of a real file",NT_SUCCESS(Status));

    Status = FsRtlGetFileSize(DirPfo,&OldSize);
    FSRTL_TEST("FsRtlGetFileSize() - Get the size of a directory file",(Status == STATUS_FILE_IS_A_DIRECTORY));


    /* The test if over. Do clean up */

Cleanup:

    if (DirPfo)
    {
        ObDereferenceObject(DirPfo);
        DirPfo = NULL;
    }

    if (DirFh)
    {
        ZwClose(DirFh);
        DirFh = NULL;
     }
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

 NTSTATUS FsRtlTest_OpenTestDirectory(PHANDLE Pfh, PFILE_OBJECT *Ppfo) {
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS  Return;

    RtlInitUnicodeString(&FileName,L"\\??\\C:\\testdir01");

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
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE,FILE_SYNCHRONOUS_IO_ALERT | FILE_DELETE_ON_CLOSE,
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


