/* $Id$
 *
 * ReactOS Project
 */

#include <csrss.h>

#define NDEBUG
#include <debug.h>

ULONG
InitializeVideoAddressSpace(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
   NTSTATUS Status;
   HANDLE PhysMemHandle;
   PVOID BaseAddress;
   PVOID NullAddress;
   LARGE_INTEGER Offset;
   ULONG ViewSize;
   CHAR IVTAndBda[1024+256];

   /*
    * Open the physical memory section
    */
   InitializeObjectAttributes(&ObjectAttributes,
			      &PhysMemName,
			      0,
			      NULL,
			      NULL);
   Status = ZwOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS,
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
    * Get the real mode IVT and BDA from the kernel
    */
   Status = NtVdmControl(VdmInitialize, IVTAndBda);
    if (!NT_SUCCESS(Status))
     {
       DbgPrint("NtVdmControl failed (status %x)\n", Status);
       return(0);
     }

   /*
    * Copy the IVT and BDA into the right place
    */
   NullAddress = (PVOID)0x0; /* Workaround for GCC 3.4 */
   memcpy(NullAddress, IVTAndBda, 1024);
   memcpy((PVOID)0x400, &IVTAndBda[1024], 256);

   return(1);
}


/* EOF */
