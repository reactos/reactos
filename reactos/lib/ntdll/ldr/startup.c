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
#include <wstring.h>

/* MACROS ********************************************************************/

/* TYPEDEFS ******************************************************************/

typedef int (*PEPFUNC)();
 
/* GLOBALS *******************************************************************/

/* FORWARD DECLARATIONS ******************************************************/

VOID LdrStartup(DWORD ImageBase);
static PEPFUNC LdrPEStartup(DWORD ImageBase);
static PEPFUNC LdrMZStartup(DWORD ImageBase);
static PEPFUNC LdrBinStartup(DWORD ImageBase);

/* FUNCTIONS *****************************************************************/

static VOID
LdrPrintMsg(char *s)
{
  int i;
  wchar_t USBuf[512];
  UNICODE_STRING UnicodeString;

  for (i = 0; s[i] != '\0'; i++)
    {
      USBuf[i] = s[i];
    }
  USBuf[i] = 0;
  RtlInitUnicodeString(&UnicodeString, USBuf);
  NtDisplayString(&UnicodeString);
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
  PEPFUNC EntryPoint;
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
  if (EntryPoint != 0)
    EntryPoint();
  /* FIXME: terminate the process */
}

static PEPFUNC
LdrMZStartup(DWORD ImageBase)
{
  int *foo = 0;

  /* FIXME: map VDM into low memory  */
  /* FIXME: Build/Load image sections  */
   
  *foo = 5;

  return 0;
}

static PEPFUNC
LdrPEStartup(DWORD ImageBase)
{
  int i;
  PVOID SectionBase;
  NTSTATUS Status;
  PEPFUNC EntryPoint;
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_NT_HEADERS NTHeaders;
  PIMAGE_SECTION_HEADER SectionList;
   
  DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
  SectionList = (PIMAGE_SECTION_HEADER) (ImageBase + DosHeader->e_lfanew + 
    sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER));
      
  /*  Initialize Image sections  */
  for (i = 0; i < NTHeaders->FileHeader.NumberOfSections; i++)
    {
      SectionBase = (PVOID)(ImageBase + SectionList[i].VirtualAddress);

      /* Initialize appropriate sections to zero  */
      if (SectionList[i].Characteristics & IMAGE_SECTION_INITIALIZED_DATA)
        {
          memset(SectionBase, 0, SectionList[i].SizeOfRawData);
        }
    }

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
          PVOID *LibraryExports;
          PVOID *ImportAddressList; // was pImpAddr
          PULONG FunctionNameList;
          DWORD pName;
          PWORD pHint;

          /*  Load the library module into the process  */
          /* FIXME: this should take a UNICODE string  */
          Status = LdrLoadDll(&LibraryBase,
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
            (LibNTHeaders->OptionalHeader.
             DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress +
             sizeof(IMAGE_EXPORT_DIRECTORY)));

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
  EntryPoint = 0;
  
  return EntryPoint;
}

static PEPFUNC
LdrBinStartup(DWORD ImageBase)
{
  return (PEPFUNC) ImageBase;
}


