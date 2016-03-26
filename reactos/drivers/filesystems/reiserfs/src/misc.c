/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             misc.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdLog2)
#pragma alloc_text(PAGE, RfsdSysTime)
#pragma alloc_text(PAGE, RfsdInodeTime)
#pragma alloc_text(PAGE, RfsdOEMToUnicode)
#pragma alloc_text(PAGE, RfsdUnicodeToOEM)
#endif

ULONG
RfsdLog2(ULONG Value)
{
    ULONG Order = 0;

    PAGED_CODE();

    ASSERT(Value > 0);

    while (Value) {
        Order++;
        Value >>= 1;
    }

    return (Order - 1);
}

LARGE_INTEGER
RfsdSysTime (IN ULONG i_time)
{
    LARGE_INTEGER SysTime;

    PAGED_CODE();

    RtlSecondsSince1970ToTime(i_time, &SysTime);

    return SysTime;
}

ULONG
RfsdInodeTime (IN LARGE_INTEGER SysTime)
{
    ULONG   RfsdTime;

    PAGED_CODE();

    RtlTimeToSecondsSince1970(&SysTime, &RfsdTime);

    return RfsdTime;
}

ULONG
RfsdMbsToUnicode(
    IN OUT PUNICODE_STRING Unicode,
    IN     PANSI_STRING    Mbs   )
{
    ULONG Length = 0;
    int i, mbc = 0;
    WCHAR  uc;

    /* Count the length of the resulting Unicode. */
    for (i = 0; i < Mbs->Length; i += mbc) {

        mbc = PAGE_TABLE->char2uni(
                    (PUCHAR)&(Mbs->Buffer[i]),
                    Mbs->Length - i,
                    &uc
                    );

        if (mbc <= 0) {

            /* Invalid character. */
            return 0;
        }

        Length += 2;
    }

    if (Unicode) {
        if (Unicode->MaximumLength < Length) {

            DbgBreak();
            return 0;
        }

        Unicode->Length = 0;
        mbc = 0;

        for (i = 0; i < Mbs->Length; i += mbc) {

            mbc = PAGE_TABLE->char2uni(
                    (PUCHAR)&(Mbs->Buffer[i]),
                    Mbs->Length - i,
                    &uc
                    );
            Unicode->Buffer[Unicode->Length/2] = uc;

            Unicode->Length += 2;
        }
    }

    return Length;
}

ULONG
RfsdUnicodeToMbs (
         IN OUT PANSI_STRING Mbs,
         IN PUNICODE_STRING  Unicode)
{
    ULONG Length = 0;
    UCHAR mbs[0x10];
    int i, mbc;

    /* Count the length of the resulting mbc-8. */
    for (i = 0; i < (Unicode->Length / 2); i++) {

        RtlZeroMemory(mbs, 0x10);
        mbc = PAGE_TABLE->uni2char(
                    Unicode->Buffer[i],
                    mbs,
                    0x10
                    );

        if (mbc <= 0) {

            /* Invalid character. */
            return 0;
        }

        Length += mbc;
    }

    if (Mbs) {

        if (Mbs->MaximumLength < Length) {

            DbgBreak();
            return 0;
        }

        Mbs->Length = 0;

        for (i = 0; i < (Unicode->Length / 2); i++) {

            mbc = PAGE_TABLE->uni2char(
                    Unicode->Buffer[i],
                    mbs,
                    0x10
                    );

            RtlCopyMemory(
                    (PUCHAR)&(Mbs->Buffer[Mbs->Length]),
                    &mbs[0],
                    mbc
                    );

            Mbs->Length += (USHORT)mbc;
        }
    }

    return Length;
}

ULONG
RfsdOEMToUnicodeSize(
        IN PANSI_STRING Oem
        )
{
    if (PAGE_TABLE) {
        return RfsdMbsToUnicode(NULL, Oem);
    }

    return RtlOemStringToCountedUnicodeSize(Oem);
}

NTSTATUS
RfsdOEMToUnicode(
         IN OUT PUNICODE_STRING Unicode,
         IN     POEM_STRING     Oem    )
{
    NTSTATUS  Status;

    PAGED_CODE();

    if (PAGE_TABLE) {
        Status = RfsdMbsToUnicode(Unicode, Oem);

        if (Status == Unicode->Length) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        goto errorout;
    }


    Status = RtlOemStringToUnicodeString(
                    Unicode, 
                    Oem,
                    FALSE );

    if (!NT_SUCCESS(Status))
    {
        DbgBreak();
        goto errorout;
    }

errorout:

    return Status;
}

ULONG
RfsdUnicodeToOEMSize(
        IN PUNICODE_STRING Unicode
        )
{
    if (PAGE_TABLE) {

        return RfsdUnicodeToMbs(NULL, Unicode);
    }

    return RtlxUnicodeStringToOemSize(Unicode);
}

NTSTATUS
RfsdUnicodeToOEM (
         IN OUT POEM_STRING Oem,
         IN PUNICODE_STRING Unicode)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (PAGE_TABLE) {

        Status = RfsdUnicodeToMbs(Oem, Unicode);

        if (Status == Oem->Length) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        goto errorout;
    }

    Status = RtlUnicodeStringToOemString(
                            Oem,
                            Unicode,
                            FALSE );

    if (!NT_SUCCESS(Status))
    {
        DbgBreak();
        goto errorout;
    }
    
errorout:

    return Status;
}
