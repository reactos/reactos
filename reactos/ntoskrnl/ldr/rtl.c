/* $Id: rtl.c,v 1.8 2000/06/29 23:35:40 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loader utilities
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

#if 0
static PVOID LdrGetExportAddress(PMODULE_OBJECT ModuleObject,
				 PUCHAR Name,
				 USHORT Hint)
{
  WORD  Idx;
  DWORD  ExportsStartRVA, ExportsEndRVA;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD  FunctionList, NameList;
  PIMAGE_SECTION_HEADER  SectionHeader;
  PIMAGE_EXPORT_DIRECTORY  ExportDirectory;

  ExportsStartRVA = ModuleObject->Image.PE.OptionalHeader->DataDirectory
    [IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
  ExportsEndRVA = ExportsStartRVA + 
    ModuleObject->Image.PE.OptionalHeader->DataDirectory
      [IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

  /*  Get the IMAGE_SECTION_HEADER that contains the exports.  This is
      usually the .edata section, but doesn't have to be.  */
  SectionHeader = LdrPEGetEnclosingSectionHeader(ExportsStartRVA, ModuleObject);

  if (!SectionHeader)
    {
      return 0;
    }

  ExportDirectory = MakePtr(PIMAGE_EXPORT_DIRECTORY,
                            ModuleObject->Base,
                            SectionHeader->VirtualAddress);

  FunctionList = (PDWORD)((DWORD)ExportDirectory->AddressOfFunctions + ModuleObject->Base);
  NameList = (PDWORD)((DWORD)ExportDirectory->AddressOfNames + ModuleObject->Base);
  OrdinalList = (PWORD)((DWORD)ExportDirectory->AddressOfNameOrdinals + ModuleObject->Base);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDirectory->NumberOfNames; Idx++)
        {
#if 0
          DPRINT("  Name:%s  NameList[%d]:%s\n", 
                 Name, 
                 Idx, 
                 (DWORD) ModuleObject->Base + NameList[Idx]);

#endif
          if (!strcmp(Name, (PCHAR) ((DWORD)ModuleObject->Base + NameList[Idx])))
            {
              ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
                FunctionList[OrdinalList[Idx]]);
              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
        FunctionList[Hint - ExportDirectory->Base]);
    }
  if (ExportAddress == 0)
    {
       DbgPrint("Export not found for %d:%s\n", Hint, 
		Name != NULL ? Name : "(Ordinal)");
       KeBugCheck(0);
    }

  return  ExportAddress;
}
#endif

PIMAGE_NT_HEADERS STDCALL RtlImageNtHeader (IN PVOID BaseAddress)
{
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("BaseAddress %x\n", BaseAddress);
   DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
   DPRINT("DosHeader %x\n", DosHeader);
   NTHeaders = (PIMAGE_NT_HEADERS)(BaseAddress + DosHeader->e_lfanew);
   DPRINT("NTHeaders %x\n", NTHeaders);
   DPRINT("DosHeader->e_magic %x DosHeader->e_lfanew %x\n",
	  DosHeader->e_magic, DosHeader->e_lfanew);
   DPRINT("*NTHeaders %x\n", *(PULONG)NTHeaders);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	return(NULL);
     }
   return(NTHeaders);
}


/* EOF */
