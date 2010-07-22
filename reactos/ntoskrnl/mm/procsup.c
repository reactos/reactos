/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/procsup.c
 * PURPOSE:         Memory functions related to Processes
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID NTAPI MiRosTakeOverPebTebRanges(IN PEPROCESS Process);
    
/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(IN PEPROCESS Process)
{
    /* Lock the VAD, ARM3-owned ranges away */                            
    MiRosTakeOverPebTebRanges(Process);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmInitializeProcessAddressSpace(IN PEPROCESS Process,
                                IN PEPROCESS ProcessClone OPTIONAL,
                                IN PVOID Section OPTIONAL,
                                IN OUT PULONG Flags,
                                IN POBJECT_NAME_INFORMATION *AuditName OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMMSUPPORT ProcessAddressSpace = &Process->Vm;
    SIZE_T ViewSize = 0;
    PVOID ImageBase = 0;
    PROS_SECTION_OBJECT SectionObject = Section;

    /* Initialize the Addresss Space lock */
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    Process->Vm.WorkingSetExpansionLinks.Flink = NULL;

    /* Initialize AVL tree */
    ASSERT(Process->VadRoot.NumberGenericTableElements == 0);
    Process->VadRoot.BalancedRoot.u1.Parent = &Process->VadRoot.BalancedRoot;

    /* Acquire the Lock */
    MmLockAddressSpace(ProcessAddressSpace);
     
    /* Lock the VAD, ARM3-owned ranges away */
    MiRosTakeOverPebTebRanges(Process);

    /* The process now has an address space */
    Process->HasAddressSpace = TRUE;

    /* Check if there's a Section Object */
    if (SectionObject)
    {
        UNICODE_STRING FileName;
        PWCHAR szSrc;
        PCHAR szDest;
        USHORT lnFName = 0;

        /* Unlock the Address Space */
        DPRINT("Unlocking\n");
        MmUnlockAddressSpace(ProcessAddressSpace);

        DPRINT("Mapping process image. Section: %p, Process: %p, ImageBase: %p\n",
                 SectionObject, Process, &ImageBase);
        Status = MmMapViewOfSection(Section,
                                    (PEPROCESS)Process,
                                    (PVOID*)&ImageBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    0,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map process Image\n");
            return Status;
        }

        /* Save the pointer */
        Process->SectionBaseAddress = ImageBase;

        /* Determine the image file name and save it to EPROCESS */
        DPRINT("Getting Image name\n");
        FileName = SectionObject->FileObject->FileName;
        szSrc = (PWCHAR)((PCHAR)FileName.Buffer + FileName.Length);
        if (FileName.Buffer)
        {
            /* Loop the file name*/
            while (szSrc > FileName.Buffer)
            {
                /* Make sure this isn't a backslash */
                if (*--szSrc == OBJ_NAME_PATH_SEPARATOR)
                {
                    /* If so, stop it here */
                    szSrc++;
                    break;
                }
                else
                {
                    /* Otherwise, keep going */
                    lnFName++;
                }
            }
        }

        /* Copy the to the process and truncate it to 15 characters if necessary */
        szDest = Process->ImageFileName;
        lnFName = min(lnFName, sizeof(Process->ImageFileName) - 1);
        while (lnFName--) *szDest++ = (UCHAR)*szSrc++;
        *szDest = ANSI_NULL;

        /* Check if caller wants an audit name */
        if (AuditName)
        {
            /* Setup the audit name */
            SeInitializeProcessAuditName(SectionObject->FileObject,
                                         FALSE,
                                         AuditName);
        }

        /* Return status to caller */
        return Status;
    }

    /* Unlock the Address Space */
    DPRINT("Unlocking\n");
    MmUnlockAddressSpace(ProcessAddressSpace);

    /* Return status to caller */
    return Status;
}

VOID
NTAPI
MmCleanProcessAddressSpace(IN PEPROCESS Process)
{
    /* FIXME: Add part of MmDeleteProcessAddressSpace here */
}

NTSTATUS
NTAPI
MmDeleteProcessAddressSpace(PEPROCESS Process)
{
   PVOID Address;
   PMEMORY_AREA MemoryArea;

   DPRINT("MmDeleteProcessAddressSpace(Process %x (%s))\n", Process,
          Process->ImageFileName);

   MmLockAddressSpace(&Process->Vm);

   while ((MemoryArea = (PMEMORY_AREA)Process->Vm.WorkingSetExpansionLinks.Flink) != NULL)
   {
      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SECTION_VIEW:
             Address = (PVOID)MemoryArea->StartingAddress;
             MmUnlockAddressSpace(&Process->Vm);
             MmUnmapViewOfSection(Process, Address);
             MmLockAddressSpace(&Process->Vm);
             break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
             MmFreeVirtualMemory(Process, MemoryArea);
             break;

         case MEMORY_AREA_OWNED_BY_ARM3:
             MmFreeMemoryArea(&Process->Vm,
                              MemoryArea,
                              NULL,
                              NULL);
             break;

         default:
            KeBugCheck(MEMORY_MANAGEMENT);
      }
   }

   Mmi386ReleaseMmInfo(Process);

   MmUnlockAddressSpace(&Process->Vm);

   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

