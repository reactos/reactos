/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             misc.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2Log2)
#pragma alloc_text(PAGE, Ext2NtTime)
#pragma alloc_text(PAGE, Ext2LinuxTime)
#pragma alloc_text(PAGE, Ext2OEMToUnicode)
#pragma alloc_text(PAGE, Ext2UnicodeToOEM)
#pragma alloc_text(PAGE, Ext2Sleep)
#endif

ULONG
Ext2Log2(ULONG Value)
{
    ULONG Order = 0;

    ASSERT(Value > 0);

    while (Value) {
        Order++;
        Value >>= 1;
    }

    return (Order - 1);
}

LARGE_INTEGER
Ext2NtTime (IN ULONG i_time)
{
    LARGE_INTEGER SysTime;

    SysTime.QuadPart = 0;
    RtlSecondsSince1970ToTime(i_time, &SysTime);

    return SysTime;
}

ULONG
Ext2LinuxTime (IN LARGE_INTEGER SysTime)
{
    ULONG   Ext2Time = 0;

    if (!RtlTimeToSecondsSince1970(&SysTime, &Ext2Time)) {
        LARGE_INTEGER NtTime;
        KeQuerySystemTime(&NtTime);
        RtlTimeToSecondsSince1970(&NtTime, &Ext2Time);
    }

    return Ext2Time;
}


ULONG
Ext2MbsToUnicode(
    struct nls_table *     PageTable,
    IN OUT PUNICODE_STRING Unicode,
    IN     PANSI_STRING    Mbs   )
{
    ULONG Length = 0;
    int i, mbc = 0;
    WCHAR  uc;

    /* Count the length of the resulting Unicode. */
    for (i = 0; i < Mbs->Length; i += mbc) {

        mbc = PageTable->char2uni(
                    (PUCHAR)&(Mbs->Buffer[i]),
                    Mbs->Length - i,
                    &uc
                    );

        if (mbc <= 0) {

            /* invalid character. */
            if (mbc == 0 && Length > 0) {
                break;
            }
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

            mbc = PageTable->char2uni(
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
Ext2UnicodeToMbs (
    struct nls_table *  PageTable,
    IN OUT PANSI_STRING Mbs,
    IN PUNICODE_STRING  Unicode)
{
    ULONG Length = 0;
    UCHAR mbs[0x10];
    int i, mbc;

    /* Count the length of the resulting mbc-8. */
    for (i = 0; i < (Unicode->Length / 2); i++) {

        RtlZeroMemory(mbs, 0x10);
        mbc = PageTable->uni2char(
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

            mbc = PageTable->uni2char(
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
Ext2OEMToUnicodeSize(
    IN PEXT2_VCB        Vcb,
    IN PANSI_STRING     Oem
    )
{
    ULONG   Length = 0;

    if (Vcb->Codepage.PageTable) {
        Length = Ext2MbsToUnicode(Vcb->Codepage.PageTable, NULL, Oem);
        if (Length > 0) {
            goto errorout;
        }
    }

    Length = RtlOemStringToCountedUnicodeSize(Oem);

errorout:

    return Length;
}


NTSTATUS
Ext2OEMToUnicode(
    IN PEXT2_VCB           Vcb,
    IN OUT PUNICODE_STRING Unicode,
    IN     POEM_STRING     Oem
    )
{
    NTSTATUS  Status;


    if (Vcb->Codepage.PageTable) {
        Status = Ext2MbsToUnicode(Vcb->Codepage.PageTable,
                                  Unicode, Oem);

        if (Status >0 && Status == Unicode->Length) {
            Status = STATUS_SUCCESS;
            goto errorout;
        }
    }

    Status = RtlOemStringToUnicodeString(
                    Unicode, Oem, FALSE );

    if (!NT_SUCCESS(Status)) {
        DbgBreak();
        goto errorout;
    }

errorout:

    return Status;
}

ULONG
Ext2UnicodeToOEMSize(
        IN PEXT2_VCB       Vcb,
        IN PUNICODE_STRING Unicode
        )
{
    ULONG   Length = 0;

    if (Vcb->Codepage.PageTable) {
        Length = Ext2UnicodeToMbs(Vcb->Codepage.PageTable,
                                NULL, Unicode);
        if (Length > 0) {
            return Length;
        }

        DbgBreak();
    }

    return RtlxUnicodeStringToOemSize(Unicode);
}


NTSTATUS
Ext2UnicodeToOEM (
        IN PEXT2_VCB        Vcb,
        IN OUT POEM_STRING  Oem,
        IN PUNICODE_STRING  Unicode)
{
    NTSTATUS Status;

    if (Vcb->Codepage.PageTable) {

        Status = Ext2UnicodeToMbs(Vcb->Codepage.PageTable,
                                  Oem, Unicode);
        if (Status > 0 && Status == Oem->Length) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
            DbgBreak();
        }

        goto errorout;
    }

    Status = RtlUnicodeStringToOemString(
                    Oem, Unicode, FALSE );

    if (!NT_SUCCESS(Status))
    {
        DbgBreak();
        goto errorout;
    }
    
errorout:

    return Status;
}

VOID
Ext2Sleep(ULONG ms)
{
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = (LONGLONG)ms*1000*(-10); /* ms/1000 sec*/
    KeDelayExecutionThread(KernelMode, TRUE, &Timeout);
}

