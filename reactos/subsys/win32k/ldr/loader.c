/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: loader.c,v 1.15 2004/04/09 20:03:18 navaraf Exp $
 *
 */

#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <ddk/ntapi.h>

#define NDEBUG
#include <debug.h>

#ifdef __USE_W32API
PIMAGE_NT_HEADERS STDCALL
RtlImageNtHeader(IN PVOID BaseAddress);
#endif

/*
 * This is copied from ntdll...  It's needed for loading keyboard dlls.
 */

PVOID
STDCALL
RtlImageDirectoryEntryToData (
	PVOID	BaseAddress,
	BOOLEAN	bFlag,
	ULONG	Directory,
	PULONG	Size
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG Va;
	ULONG Count;

	NtHeader = RtlImageNtHeader (BaseAddress);
	if (NtHeader == NULL)
		return NULL;

	if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
		return NULL;

	Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
	if (Va == 0)
		return NULL;

	if (Size)
		*Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

	if (bFlag)
		return (PVOID)(BaseAddress + Va);

	/* image mapped as ordinary file, we must find raw pointer */
	SectionHeader = (PIMAGE_SECTION_HEADER)(NtHeader + 1);
	Count = NtHeader->FileHeader.NumberOfSections;
	while (Count--)
	{
		if (SectionHeader->VirtualAddress == Va)
			return (PVOID)(BaseAddress + SectionHeader->PointerToRawData);
		SectionHeader++;
	}

	return NULL;
}

/*
 * Blatantly stolen from ldr/utils.c in ntdll.  I can't link ntdll from
 * here, though.
 */
NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &i);

   DPRINT("ExportDir %x i %lu\n", ExportDir, i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
        return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
        /* by name */
        OrdinalPtr = (PUSHORT)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
        NamePtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNames);
        for( i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
          {
             if (!_strnicmp(Name->Buffer, (char*)(BaseAddress + *NamePtr), Name->Length))
               {
                  *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
                  return STATUS_SUCCESS;
               }
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
        /* by ordinal */
        Ordinal &= 0x0000FFFF;
        if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
          {
             *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[Ordinal - ExportDir->Base]);
             return STATUS_SUCCESS;
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

PVOID STDCALL
EngFindImageProcAddress(IN HANDLE Module,
			IN LPSTR ProcName)
{
  PVOID Function;
  NTSTATUS Status;
  ANSI_STRING ProcNameString;
  RtlInitAnsiString(&ProcNameString, ProcName);
  Status = LdrGetProcedureAddress(Module, 
				  &ProcNameString,
				  0,
				  &Function);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(Function);
}


/*
 * @implemented
 */
HANDLE
STDCALL
EngLoadImage (LPWSTR DriverName)
{
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  NTSTATUS Status;

  RtlInitUnicodeString(&GdiDriverInfo.ModuleName, DriverName);
  Status = ZwSetSystemInformation(SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ModuleBase;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
EngLoadModule(LPWSTR ModuleName)
{
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  NTSTATUS Status;

  // FIXME: should load as readonly

  RtlInitUnicodeString (&GdiDriverInfo.ModuleName, ModuleName);
  Status = ZwSetSystemInformation (SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  if (!NT_SUCCESS(Status)) return NULL;

  return (HANDLE)GdiDriverInfo.ModuleBase;
}

/* EOF */
