/* $Id: video.c,v 1.6 2002/09/08 10:23:44 chorns Exp $
 *
 * ReactOS Project
 */
#include <ddk/ntddk.h>

ULONG
InitializeVideoAddressSpace(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName;
   NTSTATUS Status;
   HANDLE PhysMemHandle;
   PVOID BaseAddress;
   LARGE_INTEGER Offset;
   ULONG ViewSize;
   PUCHAR TextMap;
   CHAR IVT[1024];
   CHAR BDA[256];

   /*
    * Open the physical memory section
    */
   RtlInitUnicodeStringFromLiteral(&PhysMemName, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PhysMemName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS, 
			  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Couldn't open \\Device\\PhysicalMemory\n");
	return(0);
     }

   /*
    * Map the BIOS and device registers into the address space
    */
   Offset.QuadPart = 0xa0000;
   ViewSize = 0x100000 - 0xa0000;
   BaseAddress = (PVOID)0xa0000;
   Status = NtMapViewOfSection(PhysMemHandle,
			       NtCurrentProcess(),
			       &BaseAddress,
			       0,
			       8192,
			       &Offset,
			       &ViewSize,
			       ViewUnmap,
			       0,
			       PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Couldn't map physical memory (%x)\n", Status);
	NtClose(PhysMemHandle);
	return(0);
     }
   NtClose(PhysMemHandle);
   if (BaseAddress != (PVOID)0xa0000)
     {
       DbgPrint("Couldn't map physical memory at the right address "
		"(was %x)\n", BaseAddress);
       return(0);
     }

   /*
    * Map some memory to use for the non-BIOS parts of the v86 mode address
    * space
    */
   BaseAddress = (PVOID)0x1;
   ViewSize = 0xa0000 - 0x1000;
   Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				    &BaseAddress,
				    0,
				    &ViewSize,
				    MEM_COMMIT,
				    PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("Failed to allocate virtual memory (Status %x)\n", Status);
       return(0);
     }
   if (BaseAddress != (PVOID)0x0)
     {
       DbgPrint("Failed to allocate virtual memory at right address "
		"(was %x)\n", BaseAddress);
       return(0);
     }

   /*
    * Get the real mode IVT from the kernel
    */
   Status = NtVdmControl(0, IVT);
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("NtVdmControl failed (status %x)\n", Status);
       return(0);
     }
   
   /*
    * Copy the real mode IVT into the right place
    */
   memcpy((PVOID)0x0, IVT, 1024);
   
   /*
    * Get the BDA from the kernel
    */
   Status = NtVdmControl(1, BDA);
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("NtVdmControl failed (status %x)\n", Status);
       return(0);
     }
   
   /*
    * Copy the BDA into the right place
    */
   memcpy((PVOID)0x400, BDA, 256);

   return(1);
}


/* EOF */
