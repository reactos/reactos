/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/init.c
 * PURPOSE:         Loaders for PE executables
 * 
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/


#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* 
 * HACK! No matter what i did, i couldnt get it working when i put these into ntos\rtl.h
 * (got redefinition problems, since ntdll\rtl.h somehow include ntos\rtl.h).
 * We need to merge ntos\rtl.h and ntdll\rtl.h to get this working. -Gunnar
 */
typedef struct _RTL_PROCESS_INFO
{
   ULONG Size;
   HANDLE ProcessHandle;
   HANDLE ThreadHandle;
   CLIENT_ID ClientId;
   SECTION_IMAGE_INFORMATION ImageInfo;
} RTL_PROCESS_INFO, *PRTL_PROCESS_INFO;

NTSTATUS
STDCALL
RtlCreateUserProcess (
   IN PUNICODE_STRING         ImageFileName,
   IN ULONG          Attributes,
   IN PRTL_USER_PROCESS_PARAMETERS  ProcessParameters,
   IN PSECURITY_DESCRIPTOR    ProcessSecutityDescriptor OPTIONAL,
   IN PSECURITY_DESCRIPTOR    ThreadSecurityDescriptor OPTIONAL,
   IN HANDLE            ParentProcess OPTIONAL,
   IN BOOLEAN           CurrentDirectory,
   IN HANDLE            DebugPort OPTIONAL,
   IN HANDLE            ExceptionPort OPTIONAL,
   OUT   PRTL_PROCESS_INFO    ProcessInfo
   );

NTSTATUS
STDCALL
RtlCreateProcessParameters (
   OUT   PRTL_USER_PROCESS_PARAMETERS  *ProcessParameters,
   IN PUNICODE_STRING   ImagePathName OPTIONAL,
   IN PUNICODE_STRING   DllPath OPTIONAL,
   IN PUNICODE_STRING   CurrentDirectory OPTIONAL,
   IN PUNICODE_STRING   CommandLine OPTIONAL,
   IN PWSTR    Environment OPTIONAL,
   IN PUNICODE_STRING   WindowTitle OPTIONAL,
   IN PUNICODE_STRING   DesktopInfo OPTIONAL,
   IN PUNICODE_STRING   ShellInfo OPTIONAL,
   IN PUNICODE_STRING   RuntimeInfo OPTIONAL
   );
NTSTATUS
STDCALL
RtlDestroyProcessParameters (
   IN PRTL_USER_PROCESS_PARAMETERS  ProcessParameters
   );



/* FUNCTIONS *****************************************************************/



INIT_FUNCTION
NTSTATUS
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle)
{
   UNICODE_STRING ImagePath;
   HANDLE SystemProcessHandle;
   NTSTATUS Status;
   PRTL_USER_PROCESS_PARAMETERS Params=NULL;
   RTL_PROCESS_INFO Info;

   /* Get the absolute path to smss.exe. */
   RtlRosInitUnicodeStringFromLiteral(&ImagePath,
      L"\\SystemRoot\\system32\\smss.exe");


   Status = ObCreateHandle(
      PsGetCurrentProcess(),
      PsInitialSystemProcess,
      PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
      FALSE,
      &SystemProcessHandle
      );

   if(!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create a handle for the system process!\n");
      return Status;
   }

   
   Status = RtlCreateProcessParameters(
      &Params,
      &ImagePath,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
      );
   
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to create ppb!\n");
    ZwClose(SystemProcessHandle);
    return Status;
  }
   

   DPRINT("Creating process\n");

   Status = RtlCreateUserProcess(
      &ImagePath,
      OBJ_CASE_INSENSITIVE, //Valid are OBJ_INHERIT and OBJ_CASE_INSENSITIVE.
      Params,
      NULL,
      NULL,
      SystemProcessHandle,
      FALSE,
      NULL,
      NULL,
      &Info
      );

   ZwClose(SystemProcessHandle);
   RtlDestroyProcessParameters(Params);
  
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("NtCreateProcess() failed (Status %lx)\n", Status);
      return(Status);
   }

   ZwResumeThread(Info.ThreadHandle, NULL);

   *ProcessHandle = Info.ProcessHandle;
   *ThreadHandle= Info.ThreadHandle;

   DPRINT("Process created successfully\n");

   return(STATUS_SUCCESS);
}

/* EOF */
