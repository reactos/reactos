/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/sysdll.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   26/01/00  Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static PVOID SystemDllEntryPoint = NULL;
static PVOID SystemDllApcDispatcher = NULL;
static PVOID SystemDllCallbackDispatcher = NULL;
static PVOID SystemDllExceptionDispatcher = NULL;

/* FUNCTIONS *****************************************************************/

PVOID LdrpGetSystemDllExceptionDispatcher(VOID)
{
  return(SystemDllExceptionDispatcher);
}

PVOID LdrpGetSystemDllCallbackDispatcher(VOID)
{
  return(SystemDllCallbackDispatcher);
}

PVOID LdrpGetSystemDllEntryPoint(VOID)
{
   return(SystemDllEntryPoint);
}

PVOID LdrpGetSystemDllApcDispatcher(VOID)
{
   return(SystemDllApcDispatcher);
}

NTSTATUS LdrpMapSystemDll(HANDLE ProcessHandle,
			  PVOID* LdrStartupAddr)
/*
 * FUNCTION: LdrpMapSystemDll maps the system dll into the specified process
 * address space and returns its startup address.
 * PARAMETERS:
 *   ProcessHandle
 *              Points to the process to map the system dll into
 * 
 *   LdrStartupAddress
 *              Receives the startup address of the system dll on function
 *              completion
 * 
 * RETURNS: Status
 */
{
   CHAR			BlockBuffer [1024];
   DWORD			ImageBase;
   ULONG			ImageSize;
   NTSTATUS		Status;
   OBJECT_ATTRIBUTES	FileObjectAttributes;
   HANDLE			FileHandle;
   HANDLE			NTDllSectionHandle;
   UNICODE_STRING		DllPathname;
   PIMAGE_DOS_HEADER	DosHeader;
   PIMAGE_NT_HEADERS	NTHeaders;
   PEPROCESS Process;
   ANSI_STRING ProcedureName;
   ULONG ViewSize;

   /*
    * Locate and open NTDLL to determine ImageBase
    * and LdrStartup
    */
   RtlInitUnicodeString(&DllPathname,
			L"\\SystemRoot\\system32\\ntdll.dll");
   InitializeObjectAttributes(&FileObjectAttributes,
			      &DllPathname,
			      0,
			      NULL,
			      NULL);
   DPRINT("Opening NTDLL\n");
   Status = ZwOpenFile(&FileHandle,
		       FILE_ALL_ACCESS,
		       &FileObjectAttributes,
		       NULL,
		       FILE_SHARE_READ,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL open failed (Status %x)\n", Status);
	return Status;
     }
   Status = ZwReadFile(FileHandle,
		       0,
		       0,
		       0,
		       0,
		       BlockBuffer,
		       sizeof(BlockBuffer),
		       0,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL header read failed (Status %x)\n", Status);
	ZwClose(FileHandle);
	return Status;
     }

   /*
    * FIXME: this will fail if the NT headers are
    * more than 1024 bytes from start.
    */
   DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
   NTHeaders = (PIMAGE_NT_HEADERS) (BlockBuffer + DosHeader->e_lfanew);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	DbgPrint("NTDLL format invalid\n");
	ZwClose(FileHandle);	
	return(STATUS_UNSUCCESSFUL);
     }
   ImageBase = NTHeaders->OptionalHeader.ImageBase;
   ImageSize = NTHeaders->OptionalHeader.SizeOfImage;
   
   /*
    * Create a section for NTDLL
    */
   DPRINT("Creating section\n");
   Status = ZwCreateSection(&NTDllSectionHandle,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_READWRITE,
			    SEC_IMAGE | SEC_COMMIT,
			    FileHandle);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL create section failed (Status %x)\n", Status);
	ZwClose(FileHandle);	
	return(Status);
     }
   ZwClose(FileHandle);
   
   /*
    * Map the NTDLL into the process
    */
   ViewSize = 0;
   ImageBase = 0;
   Status = ZwMapViewOfSection(NTDllSectionHandle,
			       ProcessHandle,
			       (PVOID*)&ImageBase,
			       0,
			       ViewSize,
			       NULL,
			       &ViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL map view of secion failed (Status %x)", Status);
	ZwClose(NTDllSectionHandle);
	return(Status);
     }

   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_ALL_ACCESS,
				      PsProcessType,
				      KernelMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ObReferenceObjectByProcess() failed (Status %x)\n", Status);
	return(Status);
     }

   DPRINT("Attaching to Process\n");
   KeAttachProcess(Process);

   /*
    * retrieve ntdll's startup address
    */   
   if (SystemDllEntryPoint == NULL)
     {
       RtlInitAnsiString (&ProcedureName,
			  "LdrInitializeThunk");
       Status = LdrGetProcedureAddress ((PVOID)ImageBase,
					&ProcedureName,
					0,
					&SystemDllEntryPoint);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	   KeDetachProcess();
	   ObDereferenceObject(Process);
	   ZwClose(NTDllSectionHandle);
	   return (Status);
	 }
       *LdrStartupAddr = SystemDllEntryPoint;
     }

   /*
    * Retrieve the offset of the APC dispatcher from NTDLL
    */
   if (SystemDllApcDispatcher == NULL)
     {
       RtlInitAnsiString (&ProcedureName,
			  "KiUserApcDispatcher");
       Status = LdrGetProcedureAddress ((PVOID)ImageBase,
					&ProcedureName,
					0,
					&SystemDllApcDispatcher);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	   KeDetachProcess();
	   ObDereferenceObject(Process);
	   ZwClose(NTDllSectionHandle);
	   return (Status);
	 }
     }

   /*
    * Retrieve the offset of the exception dispatcher from NTDLL
    */
   if (SystemDllExceptionDispatcher == NULL)
     {
       RtlInitAnsiString (&ProcedureName,
			  "KiUserExceptionDispatcher");
       Status = LdrGetProcedureAddress ((PVOID)ImageBase,
					&ProcedureName,
					0,
					&SystemDllExceptionDispatcher);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	   KeDetachProcess();
	   ObDereferenceObject(Process);
	   ZwClose(NTDllSectionHandle);
	   return (Status);
	 }
     }

   /*
    * Retrieve the offset of the callback dispatcher from NTDLL
    */
   if (SystemDllCallbackDispatcher == NULL)
     {
       RtlInitAnsiString (&ProcedureName,
			  "KiUserCallbackDispatcher");
       Status = LdrGetProcedureAddress ((PVOID)ImageBase,
					&ProcedureName,
					0,
					&SystemDllCallbackDispatcher);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	   KeDetachProcess();
	   ObDereferenceObject(Process);
	   ZwClose(NTDllSectionHandle);
	   return (Status);
	 }
     }
   
   KeDetachProcess();
   ObDereferenceObject(Process);

   ZwClose(NTDllSectionHandle);

   return(STATUS_SUCCESS);
}

/* EOF */
