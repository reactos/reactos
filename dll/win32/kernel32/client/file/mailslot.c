/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/mailslot.c
 * PURPOSE:         Mailslot functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMailslotA(IN LPCSTR lpName,
                IN DWORD nMaxMessageSize,
                IN DWORD lReadTimeout,
                IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    /* Call the W(ide) function */
    ConvertWin32AnsiObjectApiToUnicodeApi2(Mailslot, lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMailslotW(IN LPCWSTR lpName,
                IN DWORD nMaxMessageSize,
                IN DWORD lReadTimeout,
                IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING MailslotName;
    HANDLE MailslotHandle;
    NTSTATUS Status;
    BOOLEAN Result;
    LARGE_INTEGER DefaultTimeOut;
    IO_STATUS_BLOCK Iosb;
    ULONG Attributes = OBJ_CASE_INSENSITIVE;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    Result = RtlDosPathNameToNtPathName_U(lpName, &MailslotName, NULL, NULL);
    if (!Result)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    DPRINT("Mailslot name: %wZ\n", &MailslotName);

    if (lpSecurityAttributes)
    {
        SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if(lpSecurityAttributes->bInheritHandle) Attributes |= OBJ_INHERIT;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &MailslotName,
                               Attributes,
                               NULL,
                               SecurityDescriptor);

    if (lReadTimeout == MAILSLOT_WAIT_FOREVER)
    {
        /* Set the max */
        DefaultTimeOut.QuadPart = 0xFFFFFFFFFFFFFFFFLL;
    }
    else
    {
        /* Convert to NT format */
        DefaultTimeOut.QuadPart = lReadTimeout * -10000LL;
    }

    Status = NtCreateMailslotFile(&MailslotHandle,
                                  GENERIC_READ | SYNCHRONIZE | WRITE_DAC,
                                  &ObjectAttributes,
                                  &Iosb,
                                  FILE_WRITE_THROUGH,
                                  0,
                                  nMaxMessageSize,
                                  &DefaultTimeOut);

    if ((Status == STATUS_INVALID_DEVICE_REQUEST) ||
        (Status == STATUS_NOT_SUPPORTED))
    {
        Status = STATUS_OBJECT_NAME_INVALID;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, MailslotName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateMailslot failed (Status %x)!\n", Status);
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return MailslotHandle;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetMailslotInfo(IN HANDLE hMailslot,
                IN LPDWORD lpMaxMessageSize,
                IN LPDWORD lpNextSize,
                IN LPDWORD lpMessageCount,
                IN LPDWORD lpReadTimeout)
{
    FILE_MAILSLOT_QUERY_INFORMATION Buffer;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    Status = NtQueryInformationFile(hMailslot,
                                    &Iosb,
                                    &Buffer,
                                    sizeof(FILE_MAILSLOT_QUERY_INFORMATION),
                                    FileMailslotQueryInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed (Status %x)!\n", Status);
        BaseSetLastNTError(Status);
        return FALSE;
     }

     if (lpMaxMessageSize) *lpMaxMessageSize = Buffer.MaximumMessageSize;
     if (lpNextSize) *lpNextSize = Buffer.NextMessageSize;
     if (lpMessageCount) *lpMessageCount = Buffer.MessagesAvailable;

     if (lpReadTimeout)
     {
         if (Buffer.ReadTimeout.QuadPart == 0xFFFFFFFFFFFFFFFFLL)
         {
             *lpReadTimeout = MAILSLOT_WAIT_FOREVER;
         }
         else
         {
             Timeout.QuadPart = -Buffer.ReadTimeout.QuadPart;
             Timeout = RtlExtendedLargeIntegerDivide(Timeout, 10000, NULL);
             if (Timeout.HighPart == 0)
             {
                 *lpReadTimeout = Timeout.LowPart;
             }
             else
             {
                 *lpReadTimeout = 0xFFFFFFFE;
             }
         }
     }

   return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMailslotInfo(IN HANDLE hMailslot,
                IN DWORD lReadTimeout)
{
    FILE_MAILSLOT_SET_INFORMATION Buffer;
    LARGE_INTEGER Timeout;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    if (lReadTimeout == MAILSLOT_WAIT_FOREVER)
    {
        /* Set the max */
        Timeout.QuadPart = 0xFFFFFFFFFFFFFFFFLL;
    }
    else
    {
        /* Convert to NT format */
        Timeout.QuadPart = lReadTimeout * -10000LL;
    }

    Buffer.ReadTimeout = &Timeout;

    Status = NtSetInformationFile(hMailslot,
                                  &Iosb,
                                  &Buffer,
                                  sizeof(FILE_MAILSLOT_SET_INFORMATION),
                                  FileMailslotSetInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile failed (Status %x)!\n", Status);
        BaseSetLastNTError(Status);
        return FALSE;
     }

     return TRUE;
}

/* EOF */
