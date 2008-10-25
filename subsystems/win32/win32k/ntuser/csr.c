/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/ntuser/csr.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static HANDLE WindowsApiPort = NULL;
PEPROCESS CsrProcess = NULL;

NTSTATUS FASTCALL
CsrInit(void)
{
   NTSTATUS Status;
   UNICODE_STRING PortName;
   ULONG ConnectInfoLength;

   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   ConnectInfoLength = 0;
   Status = ZwConnectPort(&WindowsApiPort,
                          &PortName,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &ConnectInfoLength);
   if (! NT_SUCCESS(Status))
   {
      return Status;
   }

   CsrProcess = PsGetCurrentProcess();

   return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
co_CsrNotify(PCSR_API_MESSAGE Request)
{
   NTSTATUS Status;
   PEPROCESS OldProcess;

   if (NULL == CsrProcess)
   {
      return STATUS_INVALID_PORT_HANDLE;
   }

   Request->Header.u2.ZeroInit = 0;
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);

   /* Switch to the process in which the WindowsApiPort handle is valid */
   OldProcess = PsGetCurrentProcess();
   if (CsrProcess != OldProcess)
   {
      KeAttachProcess(&CsrProcess->Pcb);
   }

   UserLeaveCo();

   Status = ZwRequestWaitReplyPort(WindowsApiPort,
                                   &Request->Header,
                                   &Request->Header);

   UserEnterCo();

   if (CsrProcess != OldProcess)
   {
      KeDetachProcess();
   }

   if (NT_SUCCESS(Status))
   {
      Status = Request->Status;
   }

   return Status;
}


NTSTATUS
STDCALL
CsrInsertObject(HANDLE ObjectHandle,
                ACCESS_MASK DesiredAccess,
                PHANDLE Handle)
{
   NTSTATUS Status;
   HANDLE CsrProcessHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID Cid;

   /* Put CSR'S CID */
   Cid.UniqueProcess = CsrProcess->UniqueProcessId;
   Cid.UniqueThread = 0;

   /* Empty Attributes */
   InitializeObjectAttributes(&ObjectAttributes,
                              NULL,
                              0,
                              NULL,
                              NULL);

   /* Get a Handle to Csrss */
   Status = ZwOpenProcess(&CsrProcessHandle,
                          PROCESS_DUP_HANDLE,
                          &ObjectAttributes,
                          &Cid);

   if ((NT_SUCCESS(Status)))
   {
      /* Duplicate the Handle */
      Status = ZwDuplicateObject(NtCurrentProcess(),
                                 ObjectHandle,
                                 CsrProcessHandle,
                                 Handle,
                                 DesiredAccess,
                                 OBJ_INHERIT,
                                 0);

      /* Close our handle to CSRSS */
      ZwClose(CsrProcessHandle);
   }

   return Status;
}

NTSTATUS FASTCALL
CsrCloseHandle(HANDLE Handle)
{
   NTSTATUS Status;
   PEPROCESS OldProcess;

   /* Switch to the process in which the handle is valid */
   OldProcess = PsGetCurrentProcess();
   if (CsrProcess != OldProcess)
   {
      KeAttachProcess(&CsrProcess->Pcb);
   }

   Status = ZwClose(Handle);

   if (CsrProcess != OldProcess)
   {
      KeDetachProcess();
   }

   return Status;
}

/* EOF */
