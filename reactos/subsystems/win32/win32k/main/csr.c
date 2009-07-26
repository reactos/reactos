/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss. ROS SPECIFIC!
 * FILE:             subsys/win32k/ntuser/csr.c
 * PROGRAMER:        Parts are by Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
#include <handle.h>

//#define NDEBUG
#include <debug.h>

static HANDLE WindowsApiPort = NULL;
PEPROCESS CsrProcess = NULL;

VOID NTAPI
RosUserConnectCsrss(void)
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
   if (! NT_SUCCESS(Status))
   {
      DPRINT1("There is a problem connecting to CSRSS, Status=0x%08x\n", Status);
      SetLastNtError(Status);
   }

   CsrProcess = PsGetCurrentProcess();
   DPRINT("Win32k registered with CSRSS\n");
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

   Status = ZwRequestWaitReplyPort(WindowsApiPort,
                                   &Request->Header,
                                   &Request->Header);

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
APIENTRY
CsrInsertObject(HANDLE ObjectHandle,
                ACCESS_MASK DesiredAccess,
                PHANDLE Handle)
{
#ifdef WIN32K_USES_PROPER_OB
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
#else
    /* No csrss - no luck */
    if (!CsrProcess) return STATUS_UNSUCCESSFUL;

    /* Duplicate the handle */
    *Handle = (HANDLE)duplicate_handle(PsGetCurrentProcessWin32Process(),
        (obj_handle_t)ObjectHandle, PsGetProcessWin32Process(CsrProcess), 0, 0, DUP_HANDLE_SAME_ACCESS);

    return STATUS_SUCCESS;
#endif
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

/* Below is a part written by me */
NTSTATUS
NTAPI
CsrNotifyCreateDesktop(HDESK Desktop)
{
    CSR_API_MESSAGE Request;
    NTSTATUS Status;

    /* Create a handle for CSRSS and notify CSRSS */
    Request.Type = MAKE_CSR_API(CREATE_DESKTOP, CSR_GUI);

    /* Create a duplicate handle in csrss's process space */
    Status = CsrInsertObject(Desktop,
                             GENERIC_ALL,
                             (HANDLE*)&Request.Data.CreateDesktopRequest.DesktopHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create desktop handle for CSRSS\n");
        return Status;
    }

    /* Notify CSRSS */
    Status = co_CsrNotify(&Request);
    if (!NT_SUCCESS(Status))
    {
        //CsrCloseHandle(Request.Data.CreateDesktopRequest.DesktopHandle);
        DPRINT1("Failed to notify CSRSS about new desktop, Status 0x%08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CsrNotifyShowDesktop(HWND DesktopWindow, ULONG Width, ULONG Height)
{
   CSR_API_MESSAGE Request;

   Request.Type = MAKE_CSR_API(SHOW_DESKTOP, CSR_GUI);
   Request.Data.ShowDesktopRequest.DesktopWindow = DesktopWindow;
   Request.Data.ShowDesktopRequest.Width = Width;
   Request.Data.ShowDesktopRequest.Height = Height;

   return co_CsrNotify(&Request);
}

/* EOF */
