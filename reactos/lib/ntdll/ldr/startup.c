/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <string.h>
#include <internal/string.h>
#include <wchar.h>

#define NDEBUG
#include <ntdll/ntdll.h>

VOID WINAPI __RtlInitHeap(LPVOID base, ULONG minsize, ULONG maxsize);

/* MACROS ********************************************************************/

#define RVA(m, b) ((ULONG)b + m->BaseAddress)

/* TYPEDEFS ******************************************************************/

typedef NTSTATUS (*PEPFUNC)(VOID);

typedef struct _DLL
{
   PIMAGE_NT_HEADERS Headers;
   PVOID BaseAddress;
   struct _DLL* Prev;
   struct _DLL* Next;
} DLL, *PDLL;

/* GLOBALS *******************************************************************/

static DLL DllListHead;

/* FORWARD DECLARATIONS ******************************************************/

static PEPFUNC LdrPEStartup(DWORD ImageBase, HANDLE SectionHandle);

/* FUNCTIONS *****************************************************************/

static NTSTATUS LdrMapSections(PVOID ImageBase, HANDLE SectionHandle,
			       PIMAGE_NT_HEADERS NTHeaders)
{
   ULONG i;
   NTSTATUS Status;
   
   for (i=0; i<NTHeaders->FileHeader.NumberOfSections; i++)
     {
	PIMAGE_SECTION_HEADER Sections;
	LARGE_INTEGER Offset;
	ULONG Base;
	
	Sections = (PIMAGE_SECTION_HEADER)SECHDROFFSET(ImageBase);
	Base = Sections[i].VirtualAddress + ImageBase;
	SET_LARGE_INTEGER_HIGH_PART(Offset,0);
	SET_LARGE_INTEGER_LOW_PART(Offset,Sections[i].PointerToRawData);
	Status = ZwMapViewOfSection(SectionHandle,
				    NtCurrentProcess(),
				    (PVOID *)&Base,
				    0,
				    Sections[i].Misc.VirtualSize,
				    &Offset,
				    &Sections[i].Misc.VirtualSize,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     return Status;
	  }
     }
   return(STATUS_SUCCESS);
}

static NTSTATUS LdrLoadDll(PDLL* Base, PCHAR Name)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   DLL* current;
   PIMAGE_OPTIONAL_HEADER OptionalHeader;
   
   DPRINT("LdrLoadDll(Name %s)\n",Name);
   
   current = &DllListHead;
   do
     {
	OptionalHeader = &current->Headers->OptionalHeader;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)OptionalHeader->DataDirectory[
                     IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	ExportDir = ((ULONG)ExportDir + (ULONG)current->BaseAddress);
	
	DPRINT("ExportDir %x\n",ExportDir);
	DPRINT("Scanning %x\n",ExportDir->Name);
	DPRINT("Scanning %s\n",ExportDir->Name + current->BaseAddress);
	if (strcmp(ExportDir->Name + current->BaseAddress, Name) == 0)
	  {
	     *Base = current;
	     return(STATUS_SUCCESS);
	  }
	
	current = current->Next;
     } while (current != &DllListHead);
   
   return(STATUS_UNSUCCESSFUL);
}

#define HEAP_BASE (0xa0000000)

/*   LdrStartup
 * FUNCTION:
 *   Handles Process Startup Activities.
 * ARGUMENTS:
 *   DWORD    ImageBase  The base address of the process image
 */
VOID LdrStartup(HANDLE SectionHandle, DWORD ImageBase)
{
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   char buffer[512];
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("LdrStartup(ImageBase %x, SectionHandle %x)\n",ImageBase,
	   SectionHandle);
   
   DllListHead.BaseAddress = 0x80000000;
   DllListHead.Prev = &DllListHead;
   DllListHead.Next = &DllListHead;
   PEDosHeader = (PIMAGE_DOS_HEADER)DllListHead.BaseAddress;
   DllListHead.Headers = (PIMAGE_NT_HEADERS)(DllListHead.BaseAddress +
					     PEDosHeader->e_lfanew);
   
  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
      PEDosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DPRINT("Image has bad header\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);
   __RtlInitHeap(HEAP_BASE,
		 NTHeaders->OptionalHeader.SizeOfHeapCommit, 
		 NTHeaders->OptionalHeader.SizeOfHeapReserve);
   EntryPoint = LdrPEStartup(ImageBase, SectionHandle);

   if (EntryPoint == NULL)
     {
	DPRINT("Failed to initialize image\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }
   
   DPRINT("Transferring control to image\n");
   Status = EntryPoint();
   ZwTerminateProcess(NtCurrentProcess(),Status);
}

static PVOID LdrGetExport(PDLL Module, PUCHAR SymbolName)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PDWORD* ExFunctions;
   PDWORD* ExNames;
   USHORT* ExOrdinals;
   ULONG i;
   PVOID ExName;
   ULONG Ordinal;
   
   DPRINT("LdrFindExport(Module %x, SymbolName %s)\n",
	  Module, SymbolName);
   
   ExportDir = (Module->BaseAddress + 
		(Module->Headers->OptionalHeader.
		 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
   
   /*  Get header pointers  */
   ExNames = (PDWORD*)RVA(Module, ExportDir->AddressOfNames);
   ExOrdinals = (USHORT*)RVA(Module, ExportDir->AddressOfNameOrdinals);
   ExFunctions = (PDWORD*)RVA(Module, ExportDir->AddressOfFunctions);
   for (i=0; i<ExportDir->NumberOfFunctions; i++)
     {
	ExName = RVA(Module, ExNames[i]);
	if (strcmp(ExName,SymbolName) == 0)
	  {
	     Ordinal = ExOrdinals[i];
	     return(RVA(Module, ExFunctions[Ordinal]));
	  }
     }
   return(NULL);
}

static PEPFUNC LdrPEStartup(DWORD ImageBase, HANDLE SectionHandle)
{
  int i;
  PVOID SectionBase;
  NTSTATUS Status;
  PEPFUNC EntryPoint;
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_NT_HEADERS NTHeaders;
  PIMAGE_SECTION_HEADER SectionList;
   char buffer[512];
   PDLL Module;
   
  DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
  SectionList = (PIMAGE_SECTION_HEADER) (ImageBase + DosHeader->e_lfanew + 
    sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER));
      
  /*  Initialize Image sections  */
   LdrMapSections(ImageBase, SectionHandle, NTHeaders);
   
  /* FIXME: if actual load address is different from ImageBase, then reloc  */
  if (ImageBase != (DWORD) NTHeaders->OptionalHeader.ImageBase)
    {
      USHORT NumberOfEntries;
      PUSHORT pValue16;
      ULONG RelocationRVA;
      ULONG Delta32, Offset;
      PULONG pValue32;
      PRELOCATION_DIRECTORY RelocationDir;
      PRELOCATION_ENTRY RelocationBlock;

      RelocationRVA = NTHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      if (RelocationRVA)
        {
          RelocationDir = (PRELOCATION_DIRECTORY)
            ((PCHAR)ImageBase + RelocationRVA);
          while (RelocationDir->SizeOfBlock)
            {
              Delta32 = (unsigned long)(ImageBase - 
                                        NTHeaders->OptionalHeader.ImageBase);
              RelocationBlock = (PRELOCATION_ENTRY) 
                (RelocationRVA + ImageBase + sizeof(RELOCATION_DIRECTORY));
              NumberOfEntries = 
                (RelocationDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
                sizeof(RELOCATION_ENTRY);
              for (i = 0; i < NumberOfEntries; i++)
                {
                  Offset = (RelocationBlock[i].TypeOffset & 0xfff) + 
                    RelocationDir->VirtualAddress;
                  switch (RelocationBlock[i].TypeOffset >> 12)
                    {
                      case TYPE_RELOC_ABSOLUTE:
                        break;

                      case TYPE_RELOC_HIGH:
                        pValue16 = (PUSHORT) (ImageBase + Offset);
                        *pValue16 += Delta32 >> 16;
                        break;

                      case TYPE_RELOC_LOW:
                        pValue16 = (PUSHORT)(ImageBase + Offset);
                        *pValue16 += Delta32 & 0xffff;
                        break;

                      case TYPE_RELOC_HIGHLOW:
                        pValue32 = (PULONG) (ImageBase + Offset);
                        *pValue32 += Delta32;
                        break;

                      case TYPE_RELOC_HIGHADJ:
                        /* FIXME: do the highadjust fixup  */
                        DPRINT(
                          "TYPE_RELOC_HIGHADJ fixup not implemented, sorry\n");
                        return 0;
                      
                      default:
                        DPRINT("unexpected fixup type\n");
                        return 0;
                    }
                }
              RelocationRVA += RelocationDir->SizeOfBlock;
              RelocationDir = (PRELOCATION_DIRECTORY)(ImageBase + 
                                                      RelocationRVA);
            }
        }
    }

  /* FIXME: do import fixups/load required libraries  */
  /*  Resolve Import Library references  */
  if (NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
      VirtualAddress != 0)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        (ImageBase + NTHeaders->OptionalHeader.
         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          DWORD LibraryBase;
          PIMAGE_DOS_HEADER LibDosHeader;
          PIMAGE_NT_HEADERS LibNTHeaders;
          PVOID *ImportAddressList; // was pImpAddr
          PULONG FunctionNameList;
          DWORD pName;
          PWORD pHint;

          Status = LdrLoadDll(&Module,
                              (PCHAR)(ImageBase +
                                      ImportModuleDirectory->dwRVAModuleName));
          if (!NT_SUCCESS(Status))
            {
              return 0;
            }    

          /*  Get the import address list  */
          ImportAddressList = (PVOID *)
            (NTHeaders->OptionalHeader.ImageBase + 
             ImportModuleDirectory->dwRVAFunctionAddressList);

          /*  Get the list of functions to import  */
          if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
              FunctionNameList = (PULONG) (ImageBase + 
                                ImportModuleDirectory->dwRVAFunctionNameList);
            }
          else
            {
              FunctionNameList = (PULONG) (ImageBase + 
                             ImportModuleDirectory->dwRVAFunctionAddressList);
            }

          /*  Walk through function list and fixup addresses  */
          while(*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
//                  *ImportAddressList = LibraryExports[(*FunctionNameList) & 0x7fffffff];
		   DPRINT("Import by ordinal unimplemented\n");
		   for(;;);
                }
              else // hint-name
                {
                  pName = (DWORD)(ImageBase + *FunctionNameList + 2);
                  pHint = (PWORD)(ImageBase + *FunctionNameList);

                  /* FIXME: verify name  */
		   
		   if (strcmp(pName,"vsprintf")==0)
		     {
			DPRINT("Fixing up reference to %s at %x\n",
			       pName,ImportAddressList);
			DPRINT("pHint %x\n",pHint);
		     }
		   
		   
		   
                  *ImportAddressList = LdrGetExport(Module,pName);
                }
              /* FIXME: verify value of hint  */

              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }
    }
      
  /* FIXME: locate the entry point for the image  */
  EntryPoint = NTHeaders->OptionalHeader.ImageBase +
               NTHeaders->OptionalHeader.AddressOfEntryPoint;
  
  return EntryPoint;
}



