/* 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Interface to CSRSS / USERSRV
 * FILE:             subsystems/win32/win32k/ntuser/csr.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>

static HANDLE WindowsApiPort = NULL;
// See gpepCSRSS in ntuser/ntuser.c and its initialization into NtUserInitialize()
PEPROCESS CsrProcess = NULL;

NTSTATUS FASTCALL
CsrInit(void)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    ULONG ConnectInfoLength;
    SECURITY_QUALITY_OF_SERVICE Qos;   

    RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
    ConnectInfoLength = 0;
    Qos.Length = sizeof(Qos);
    Qos.ImpersonationLevel = SecurityDelegation;
    Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    Qos.EffectiveOnly = FALSE;

    Status = ZwConnectPort(&WindowsApiPort,
                           &PortName,
                           &Qos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfoLength);
    if (!NT_SUCCESS(Status))
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

/* EOF */
