/* $Id: resource.c,v 1.2 2002/09/07 15:12:58 chorns Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/resource.c
 * PURPOSE:         Resource loader
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID *Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL)
{
   PIMAGE_SECTION_HEADER Section;
   PIMAGE_NT_HEADERS NtHeader;
   ULONG SectionRva;
   ULONG SectionVa;
   ULONG DataSize;
   ULONG Offset = 0;
   ULONG Data;

   Data = (ULONG)RtlImageDirectoryEntryToData (BaseAddress,
					       TRUE,
					       IMAGE_DIRECTORY_ENTRY_RESOURCE,
					       &DataSize);
   if (Data == 0)
	return STATUS_RESOURCE_DATA_NOT_FOUND;

   if ((ULONG)BaseAddress & 1)
     {
	/* loaded as ordinary file */
	NtHeader = RtlImageNtHeader((PVOID)((ULONG)BaseAddress & ~1UL));
	Offset = (ULONG)BaseAddress - Data + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	Section = RtlImageRvaToSection (NtHeader, BaseAddress, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
	if (Section == NULL)
	  {
	     return STATUS_RESOURCE_DATA_NOT_FOUND;
	  }

	if (Section->Misc.VirtualSize < ResourceDataEntry->OffsetToData)
	  {
	     SectionRva = RtlImageRvaToSection (NtHeader, BaseAddress, ResourceDataEntry->OffsetToData)->VirtualAddress;
	     SectionVa = RtlImageRvaToVa(NtHeader, BaseAddress, SectionRva, NULL);
	     Offset = SectionRva - SectionVa + Data - Section->VirtualAddress;
	  }
     }

   if (Resource)
     {
	*Resource = (PVOID)(ResourceDataEntry->OffsetToData - Offset + (ULONG)BaseAddress);
     }

   if (Size)
     {
	*Size = ResourceDataEntry->Size;
     }

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry)
{
   PIMAGE_RESOURCE_DIRECTORY ResDir;
   PIMAGE_RESOURCE_DIRECTORY ResBase;
   PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG EntryCount;
   PWCHAR ws;
   ULONG i;
   ULONG Id;

   DPRINT ("LdrFindResource_U()\n");

   /* Get the pointer to the resource directory */
   ResDir = (PIMAGE_RESOURCE_DIRECTORY)
	RtlImageDirectoryEntryToData (BaseAddress,
				      TRUE,
				      IMAGE_DIRECTORY_ENTRY_RESOURCE,
				      &i);
   if (ResDir == NULL)
     {
	return STATUS_RESOURCE_DATA_NOT_FOUND;
     }

   DPRINT("ResourceDirectory: %x\n", (ULONG)ResDir);

   ResBase = ResDir;

   /* Let's go into resource tree */
   for (i = 0; i < Level; i++)
     {
	DPRINT("ResDir: %x\n", (ULONG)ResDir);
	Id = ((PULONG)ResourceInfo)[i];
	EntryCount = ResDir->NumberOfNamedEntries;
	ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
	DPRINT("ResEntry %x\n", (ULONG)ResEntry);
	if (Id & 0xFFFF0000)
	  {
	     /* Resource name is a unicode string */
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name & 0x80000000)
		    {
		       ws = (PWCHAR)((ULONG)ResDir + (ResEntry->Name & 0x7FFFFFFF));
		       if (!wcsncmp((PWCHAR)Id, ws + 1, *ws ) &&
			   wcslen((PWCHAR)Id) == (int)*ws )
			 {
			    goto found;
			 }
		    }
	       }
	  }
	else
	  {
	     /* We use ID number instead of string */
	     ResEntry += EntryCount;
	     EntryCount = ResDir->NumberOfIdEntries;
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name == Id)
		    {
		     DPRINT("ID entry found %x\n", Id);
		     goto found;
		    }
	       }
	  }
	DPRINT("Error %lu\n", i);

	  switch (i)
	  {
	     case 0:
		return STATUS_RESOURCE_TYPE_NOT_FOUND;

	     case 1:
		return STATUS_RESOURCE_NAME_NOT_FOUND;

	     case 2:
		if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries)
		  {
		     /* Use the first available language */
		     ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
		     break;
		  }
		return STATUS_RESOURCE_LANG_NOT_FOUND;

	     case 3:
		return STATUS_RESOURCE_DATA_NOT_FOUND;

	     default:
		return STATUS_INVALID_PARAMETER;
	  }
found:;
	ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResBase +
		(ResEntry->OffsetToData & 0x7FFFFFFF));
     }
   DPRINT("ResourceDataEntry: %x\n", (ULONG)ResDir);

   if (ResourceDataEntry)
     {
	*ResourceDataEntry = (PVOID)ResDir;
     }

  return Status;
}

/* EOF */
