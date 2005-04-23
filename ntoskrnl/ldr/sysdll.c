/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/sysdll.c
 * PURPOSE:         Loaders for PE executables
 * 
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Skywing
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PVOID SystemDllEntryPoint = NULL;
PVOID SystemDllApcDispatcher = NULL;
PVOID SystemDllCallbackDispatcher = NULL;
PVOID SystemDllExceptionDispatcher = NULL;
PVOID SystemDllRaiseExceptionDispatcher = NULL;

PVOID LdrpSystemDllBase = NULL;
PVOID LdrpSystemDllSection = NULL;

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

PVOID LdrpGetSystemDllRaiseExceptionDispatcher(VOID)
{
   return(SystemDllRaiseExceptionDispatcher);
}

NTSTATUS
STDCALL
LdrpGetSystemDllEntryPoints(VOID)
{
    ANSI_STRING ProcedureName;
    NTSTATUS Status;
    
    /* Retrieve ntdll's startup address */
    DPRINT("Getting Entrypoint: %p\n", LdrpSystemDllBase);
    RtlInitAnsiString(&ProcedureName, "LdrInitializeThunk");
    Status = LdrGetProcedureAddress((PVOID)LdrpSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &SystemDllEntryPoint);
    
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Get User APC Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserApcDispatcher");
    Status = LdrGetProcedureAddress((PVOID)LdrpSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &SystemDllApcDispatcher);
    
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }
    
    /* Get Exception Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserExceptionDispatcher");
    Status = LdrGetProcedureAddress((PVOID)LdrpSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &SystemDllExceptionDispatcher);
    
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }
    
    /* Get Callback Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserCallbackDispatcher");
    Status = LdrGetProcedureAddress((PVOID)LdrpSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &SystemDllCallbackDispatcher);
    
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }
    
    /* Get Raise Exception Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiRaiseUserExceptionDispatcher");
    Status = LdrGetProcedureAddress((PVOID)LdrpSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &SystemDllRaiseExceptionDispatcher);
    
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Return success */
    return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
LdrpMapSystemDll(PEPROCESS Process, 
                 PVOID *DllBase)
{
    NTSTATUS Status;
    ULONG ViewSize = 0;
    PVOID ImageBase = 0;
    
    /* Map the System DLL */
    DPRINT("Mapping System DLL\n");
    Status = MmMapViewOfSection(LdrpSystemDllSection,
                                Process,
                                (PVOID*)&ImageBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                0,
                                MEM_COMMIT,
                                PAGE_READWRITE);
    
    if (!NT_SUCCESS(Status)) {
    
        DPRINT1("Failed to map System DLL Into Process\n");
    }
    
    if (DllBase) *DllBase = ImageBase;
    
    return Status;
}

NTSTATUS
STDCALL
LdrpInitializeSystemDll(VOID)
{
    UNICODE_STRING DllPathname = ROS_STRING_INITIALIZER(L"\\SystemRoot\\system32\\ntdll.dll");
    OBJECT_ATTRIBUTES FileObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle;
    HANDLE NTDllSectionHandle;
    NTSTATUS Status;
    CHAR BlockBuffer[1024];
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NTHeaders;
    
    /* Locate and open NTDLL to determine ImageBase and LdrStartup */
    InitializeObjectAttributes(&FileObjectAttributes,
                               &DllPathname,
                               0,
                               NULL,
                               NULL);
    
    DPRINT("Opening NTDLL\n");
    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ACCESS,
                        &FileObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    
    if (!NT_SUCCESS(Status)) {
        DPRINT1("NTDLL open failed (Status %x)\n", Status);
        return Status;
     }
     
     /* Load NTDLL is valid */
     DPRINT("Reading NTDLL\n");
     Status = ZwReadFile(FileHandle,
                         0,
                         0,
                         0,
                         &Iosb,
                         BlockBuffer,
                         sizeof(BlockBuffer),
                         0,
                         0);
    if (!NT_SUCCESS(Status) || Iosb.Information != sizeof(BlockBuffer)) {
        
        DPRINT1("NTDLL header read failed (Status %x)\n", Status);
        ZwClose(FileHandle);
        return Status;
    }
    
    /* Check if it's valid */
    DosHeader = (PIMAGE_DOS_HEADER)BlockBuffer;
    NTHeaders = (PIMAGE_NT_HEADERS)(BlockBuffer + DosHeader->e_lfanew);
    
    if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) || 
        (DosHeader->e_lfanew == 0L) || 
        (*(PULONG) NTHeaders != IMAGE_NT_SIGNATURE)) {
        
        DPRINT1("NTDLL format invalid\n");
        ZwClose(FileHandle);	
        return(STATUS_UNSUCCESSFUL);
    }
    
    /* Create a section for NTDLL */
    DPRINT("Creating section\n");
    Status = ZwCreateSection(&NTDllSectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_IMAGE | SEC_COMMIT,
                             FileHandle);
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1("NTDLL create section failed (Status %x)\n", Status);
        ZwClose(FileHandle);	
        return(Status);
    }
    ZwClose(FileHandle);   
    
    /* Reference the Section */
    DPRINT("ObReferenceObjectByHandle section: %d\n", NTDllSectionHandle);
    Status = ObReferenceObjectByHandle(NTDllSectionHandle,
                                       SECTION_ALL_ACCESS,
                                       MmSectionObjectType,
                                       KernelMode,
                                       (PVOID*)&LdrpSystemDllSection,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        
        DPRINT1("NTDLL section reference failed (Status %x)\n", Status);
        return(Status);
    }
    
    /* Map it */
    LdrpMapSystemDll(PsGetCurrentProcess(), &LdrpSystemDllBase);
    DPRINT("LdrpSystemDllBase: %x\n", LdrpSystemDllBase);
    
    /* Now get the Entrypoints */
    LdrpGetSystemDllEntryPoints();
    
    return STATUS_SUCCESS;
}

/* EOF */
