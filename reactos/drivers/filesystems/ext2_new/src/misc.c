/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             misc.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
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

int Ext2LinuxError (NTSTATUS Status)
{
    switch (Status) {
    case STATUS_ACCESS_DENIED:
        return (-EACCES);

    case STATUS_ACCESS_VIOLATION:
        return (-EFAULT);

    case STATUS_BUFFER_TOO_SMALL:
        return (-ETOOSMALL);

    case STATUS_INVALID_PARAMETER:
        return (-EINVAL);

    case STATUS_NOT_IMPLEMENTED:
    case STATUS_NOT_SUPPORTED:
        return (-EOPNOTSUPP);

    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        return (-EADDRNOTAVAIL);

    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_NETWORK_BUSY:
    case STATUS_INVALID_NETWORK_RESPONSE:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        return (-ENETDOWN);

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        return (-ENETUNREACH);

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        return (-ECONNABORTED);

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_CONNECTION_RESET:
    case STATUS_PORT_UNREACHABLE:
        return (-ECONNRESET);

    case STATUS_INSUFFICIENT_RESOURCES:
        return (-ENOMEM);

    case STATUS_PAGEFILE_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_TOO_MANY_ADDRESSES:
    case STATUS_REMOTE_RESOURCES:
        return (-ENOBUFS);

    case STATUS_INVALID_CONNECTION:
        return (-ENOTCONN);

    case STATUS_PIPE_DISCONNECTED:
        return (-ESHUTDOWN);

    case STATUS_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_LINK_TIMEOUT:
        return (-ETIMEDOUT);

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        return (-ECONNREFUSED);

    case STATUS_HOST_UNREACHABLE:
        return (-EHOSTUNREACH);

    case STATUS_PENDING:
    case STATUS_DEVICE_NOT_READY:
        return (-EAGAIN);

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        return (-EINTR);

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        return (-EMSGSIZE);

    case STATUS_ADDRESS_ALREADY_EXISTS:
        return (-EADDRINUSE);
    }

    if (NT_SUCCESS (Status))
        return 0;

    return (-EINVAL);
}

NTSTATUS Ext2WinntError(int rc)
{
    switch (rc) {

    case 0:
        return STATUS_SUCCESS;

    case -EPERM:
    case -EACCES:
        return STATUS_ACCESS_DENIED;

    case -ENOENT:
        return  STATUS_OBJECT_NAME_NOT_FOUND;

    case -EFAULT:
        return STATUS_ACCESS_VIOLATION;

    case -ETOOSMALL:
        return STATUS_BUFFER_TOO_SMALL;

    case -EBADMSG:
    case -EBADF:
    case -EINVAL:
    case -EFBIG:
        return STATUS_INVALID_PARAMETER;

    case -EBUSY:
        return STATUS_DEVICE_BUSY;

    case -ENOSYS:
        return STATUS_NOT_IMPLEMENTED;

    case -ENOSPC:
        return STATUS_DISK_FULL;

    case -EOPNOTSUPP:
        return STATUS_NOT_SUPPORTED;

    case -EDEADLK:
        return STATUS_POSSIBLE_DEADLOCK;

    case -EEXIST:
        return STATUS_OBJECT_NAME_COLLISION;

    case -EIO:
        return STATUS_UNEXPECTED_IO_ERROR;

    case -ENOTDIR:
        return STATUS_NOT_A_DIRECTORY;

    case -EISDIR:
        return STATUS_FILE_IS_A_DIRECTORY;

    case -ENOTEMPTY:
        return STATUS_DIRECTORY_NOT_EMPTY;

    case -ENODEV:
        return STATUS_NO_SUCH_DEVICE;

    case -ENXIO:
        return STATUS_INVALID_ADDRESS;

    case -EADDRNOTAVAIL:
        return STATUS_INVALID_ADDRESS;

    case -ENETDOWN:
        return STATUS_UNEXPECTED_NETWORK_ERROR;

    case -ENETUNREACH:
        return STATUS_NETWORK_UNREACHABLE;

    case -ECONNABORTED:
        return STATUS_CONNECTION_ABORTED;

    case -ECONNRESET:
        return STATUS_CONNECTION_RESET;

    case -ENOMEM:
        return STATUS_INSUFFICIENT_RESOURCES;

    case -ENOBUFS:
        return STATUS_NO_MEMORY;

    case -ENOTCONN:
        return STATUS_INVALID_CONNECTION;

    case -ESHUTDOWN:
        return STATUS_CONNECTION_DISCONNECTED;

    case -ETIMEDOUT:
        return STATUS_TIMEOUT;

    case -ECONNREFUSED:
        return STATUS_CONNECTION_REFUSED;

    case -EHOSTUNREACH:
        return STATUS_HOST_UNREACHABLE;

    case -EAGAIN:
        return STATUS_DEVICE_NOT_READY;

    case -EINTR:
        return  STATUS_CANCELLED;

    case -EMSGSIZE:
        return STATUS_INVALID_BUFFER_SIZE;

    case -EADDRINUSE:
        return STATUS_ADDRESS_ALREADY_EXISTS;
    }

    return STATUS_UNSUCCESSFUL;
}

BOOLEAN Ext2IsDot(PUNICODE_STRING name)
{
    return (name->Length == 2 && name->Buffer[0] == L'.');
}

BOOLEAN Ext2IsDotDot(PUNICODE_STRING name)
{
    return (name->Length == 4 && name->Buffer[0] == L'.' &&
            name->Buffer[1] == L'.');
}