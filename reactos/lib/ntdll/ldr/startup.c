/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

/* MACROS ********************************************************************/

/* GLOBALS *******************************************************************/

/* FORWARD DECLARATIONS ******************************************************/

VOID LdrStartup(DWORD ImageBase);
static VOID LdrPEStartup(DWORD ImageBase);
static VOID LdrMZStartup(DWORD ImageBase);
static VOID LdrBinStartup(DWORD ImageBase);

/* FUNCTIONS *****************************************************************/

static VOID
LdrPrintMsg(const char *s)
{
  ANSI_STRING AnsiString;
  UNICODE_STRING UnicodeString;

  RtlInitAnsiString(&AnsiString, s);
  RtlAnsiToUnicodeString(&AnsiString, &UnicodeString, TRUE);
  NtDisplayString(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);
}

/*   LdrStartup
 * FUNCTION:
 *   Handles Process Startup Activities.
 * ARGUMENTS:
 *   DWORD    ImageBase  The base address of the process image
 */

VOID
LdrStartup(DWORD ImageBase)
{
  int (*EntryPoint)(...);
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && 
      PEDosHeader->e_lfanew != 0L &&
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) == IMAGE_PE_MAGIC)
    {
      EntryPoint = LdrPEStartup(ImageBase);
    }
  else if (PEDosHeader->e_magic == 0x54AD)
    {
      EntryPoint = LdrMZStartup(ImageBase);
    }
  else /*  Assume bin format and load  */
    {
      EntryPoint = LdrBinStartup(ImageBase);
    }
  /* FIXME: {else} could check for a.out, ELF, COFF, etc. images here... */

  /* FIXME: where does the return status go? */
  EntryPoint();
  /* FIXME: terminate the process */
}

static VOID
LdrMZStartup(DWORD ImageBase)
{

  /* FIXME: map VDM into low memory  */
  /* FIXME: Build/Load image sections  */
   
  (*0) = 5;
}

static VOID
LdrPEStartup(DWORD ImageBase)
{
  int i;
  PVOID SectionBase;
  NTSTATUS Status;
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_NT_HEADERS NTHeaders;
   
  DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  NTHeaders = (PIMAGE_NT_HEADERS)((PUCHAR) ImageBase + DosHeader->e_lfanew);
      
  /*  Initialize Image sections  */
  for (i = 0; i < NTHeaders->FileHeader->NumberOfSections; i++)
    {
      SectionBase = (PVOID)(ImageBase + SectionList[i].s_vaddr);

      /* Initialize appropriate sections to zero  */
      if (SectionList[i].s_flags & STYP_BSS)
        {
          memset(SectionBase, 0, SectionList[i].s_size);
        }
    }

  /* FIXME: if actual load address is different from ImageBase, then reloc  */
  if (ImageBase != (DWORD) NTHeaders->OptionalHeader->ImageBase)
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
                        LdrPrintMsg(
                          "TYPE_RELOC_HIGHADJ fixup not implemented, sorry\n");
                        return 0;
                      
                      default:
                        LdrPrintMsg("unexpected fixup type\n");
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
  if (NTHeaders->OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
      VirtualAddress != 0)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        (ImageBase + NTHeaders->OptionalHeader->
         DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          DWORD LibraryBase;
          PIMAGE_DOS_HEADER LibDosHeader;
          PIMAGE_NT_HEADERS LibNTHeaders;
          PVOID *LibraryExports;
          PVOID *ImportAddressList; // was pImpAddr
          PULONG FunctionNameList;
          DWORD pName;
          PWORD pHint;

          /*  Load the library module into the process  */
          /* FIXME: this should take a UNICODE string  */
          Status = LdrLoadLibrary(ProcessHandle,
                                  &LibraryBase,
                                  (PCHAR)(ImageBase +
                                          ImportModuleDirectory->dwRVAModuleName));
          if (!NT_SUCCESS(Status))
            {
              return 0;
            }    

          /*  Get the address of the export list for the library  */
          LibDosHeader = (PIMAGE_DOS_HEADER) LibraryBase;
          LibNTHeaders = (PIMAGE_NT_HEADERS)(LibraryBase + 
                                             LibDosHeader->e_lfanew);
          LibraryExports = (PVOID *)(LibraryBase + 
            (LibNTHeaders->OptionalHeader->
             DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress +
             sizeof(IMAGE_EXPORT_DIRECTORY));

          /*  Get the import address list  */
          ImportAddressList = (PVOID *)
            ((PCHAR)NTHeaders->OptionalHeader->ImageBase + 
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
                  *ImportAddressList = LibraryExports[(*FunctionNameList) & 0x7fffffff];
                }
              else // hint-name
                {
                  pName = (DWORD)(ImageBase + *FunctionNameList + 2);
                  pHint = (PWORD)(ImageBase + *FunctionNameList);

                  /* FIXME: verify name  */

                  *ImportAddressList = LibraryExports[*pHint];
                }

              /* FIXME: verify value of hint  */

              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }
    }
      
  /* FIXME: locate the entry point for the image  */
  
  return EntryPoint;  
}

static VOID
LdrBinStartup(DWORD ImageBase)
{
  return ImageBase;
}


