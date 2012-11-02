/* 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/ntuser/csr.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

static HANDLE WindowsApiPort = NULL;
PEPROCESS CsrProcess = NULL;

NTSTATUS FASTCALL
CsrInit(void)
{
   NTSTATUS Status;
   UNICODE_STRING PortName;
   ULONG ConnectInfoLength;
   SECURITY_QUALITY_OF_SERVICE Qos;   

   ERR("CsrInit\n");

   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   ConnectInfoLength = 0;
   Qos.Length = sizeof(Qos);
   Qos.ImpersonationLevel = SecurityDelegation;
   Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
   Qos.EffectiveOnly = FALSE;

   CsrProcess = PsGetCurrentProcess();
   ERR("CsrInit - CsrProcess = 0x%p\n", CsrProcess);

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
      ERR("CsrInit - Status = 0x%p\n", Status);
      return Status;
   }

   return STATUS_SUCCESS;
}

/* EOF */
