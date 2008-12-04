/*
 * Usage: rsym input-file output-file
 *
 * There are two sources of information: the .stab/.stabstr
 * sections of the executable and the COFF symbol table. Most
 * of the information is in the .stab/.stabstr sections.
 * However, most of our asm files don't contain .stab directives,
 * so routines implemented in assembler won't show up in the
 * .stab section. They are present in the COFF symbol table.
 * So, we mostly use the .stab/.stabstr sections, but we augment
 * the info there with info from the COFF symbol table when
 * possible.
 *
 * This is a tool and is compiled using the host compiler,
 * i.e. on Linux gcc and not mingw-gcc (cross-compiler).
 * Therefore we can't include SDK headers and we have to
 * duplicate some definitions here.
 * Also note that the internal functions are "old C-style",
 * returning an int, where a return of 0 means success and
 * non-zero is failure.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "rsym.h"

static int
CompareSymEntry(const PROSSYM_ENTRY SymEntry1, const PROSSYM_ENTRY SymEntry2)
{
  if (SymEntry1->Address < SymEntry2->Address)
    {
      return -1;
    }

  if (SymEntry2->Address < SymEntry1->Address)
    {
      return +1;
    }

  return 0;
}

static int
GetStabInfo(void *FileData, PIMAGE_FILE_HEADER PEFileHeader,
            PIMAGE_SECTION_HEADER PESectionHeaders,
            ULONG *StabSymbolsLength, void **StabSymbolsBase,
            ULONG *StabStringsLength, void **StabStringsBase)
{
  ULONG Idx;

  /* Load .stab and .stabstr sections if available */
  *StabSymbolsBase = NULL;
  *StabSymbolsLength = 0;
  *StabStringsBase = NULL;
  *StabStringsLength = 0;

  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      /* printf("section: '%.08s'\n", PESectionHeaders[Idx].Name); */
      if ((strncmp((char*)PESectionHeaders[Idx].Name, ".stab", 5) == 0)
        && (PESectionHeaders[Idx].Name[5] == 0))
        {
           /* printf(".stab section found. Size %d\n",
               PESectionHeaders[Idx].SizeOfRawData); */

           *StabSymbolsLength = PESectionHeaders[Idx].SizeOfRawData;
           *StabSymbolsBase = (void *)((char *) FileData + PESectionHeaders[Idx].PointerToRawData);
        }

      if (strncmp((char*)PESectionHeaders[Idx].Name, ".stabstr", 8) == 0)
        {
           /* printf(".stabstr section found. Size %d\n",
               PESectionHeaders[Idx].SizeOfRawData); */

           *StabStringsLength = PESectionHeaders[Idx].SizeOfRawData;
           *StabStringsBase = (void *)((char *) FileData + PESectionHeaders[Idx].PointerToRawData);
        }
    }

  return 0;
}

static int
GetCoffInfo(void *FileData, PIMAGE_FILE_HEADER PEFileHeader,
            PIMAGE_SECTION_HEADER PESectionHeaders,
            ULONG *CoffSymbolsLength, void **CoffSymbolsBase,
            ULONG *CoffStringsLength, void **CoffStringsBase)
{

  if (0 == PEFileHeader->PointerToSymbolTable || 0 == PEFileHeader->NumberOfSymbols)
    {
      /* No COFF symbol table */
      *CoffSymbolsLength = 0;
      *CoffStringsLength = 0;
    }
  else
    {
      *CoffSymbolsLength = PEFileHeader->NumberOfSymbols * sizeof(COFF_SYMENT);
      *CoffSymbolsBase = (void *)((char *) FileData + PEFileHeader->PointerToSymbolTable);
      *CoffStringsLength = *((ULONG *) ((char *) *CoffSymbolsBase + *CoffSymbolsLength));
      *CoffStringsBase = (void *)((char *) *CoffSymbolsBase + *CoffSymbolsLength);
    }

  return 0;
}

static ULONG
FindOrAddString(char *StringToFind, ULONG *StringsLength, void *StringsBase)
{
  char *Search, *End;

  Search = (char *) StringsBase;
  End = Search + *StringsLength;

  while (Search < End)
    {
      if (0 == strcmp(Search, StringToFind))
        {
          return Search - (char *) StringsBase;
        }
      Search += strlen(Search) + 1;
    }

  strcpy(Search, StringToFind);
  *StringsLength += strlen(StringToFind) + 1;

  return Search - (char *) StringsBase;
}

static int
ConvertStabs(ULONG *SymbolsCount, PROSSYM_ENTRY *SymbolsBase,
             ULONG *StringsLength, void *StringsBase,
             ULONG StabSymbolsLength, void *StabSymbolsBase,
             ULONG StabStringsLength, void *StabStringsBase,
             ULONG_PTR ImageBase, PIMAGE_FILE_HEADER PEFileHeader,
             PIMAGE_SECTION_HEADER PESectionHeaders)
{
  PSTAB_ENTRY StabEntry;
  ULONG Count, i;
  ULONG_PTR Address, LastFunctionAddress;
  int First = 1;
  char *Name;
  ULONG NameLen;
  char FuncName[256];
  PROSSYM_ENTRY Current;

  StabEntry = StabSymbolsBase;
  Count = StabSymbolsLength / sizeof(STAB_ENTRY);
  *SymbolsCount = 0;
  if (Count == 0)
    {
      /* No symbol info */
      *SymbolsBase = NULL;
      return 0;
    }

  *SymbolsBase = malloc(Count * sizeof(ROSSYM_ENTRY));
  if (NULL == *SymbolsBase)
    {
      fprintf(stderr, "Failed to allocate memory for converted .stab symbols\n");
      return 1;
    }
  Current = *SymbolsBase;
  memset ( Current, 0, sizeof(*Current) );

  LastFunctionAddress = 0;
  for (i = 0; i < Count; i++)
    {
      if ( 0 == LastFunctionAddress )
        {
          Address = StabEntry[i].n_value - ImageBase;
        }
      else
        {
          Address = LastFunctionAddress + StabEntry[i].n_value;
        }
      switch (StabEntry[i].n_type)
        {
          case N_SO:
          case N_SOL:
          case N_BINCL:
            Name = (char *) StabStringsBase + StabEntry[i].n_strx;
            if (StabStringsLength < StabEntry[i].n_strx
                ||'\0' == *Name || '/' == Name[strlen(Name) - 1]
                || '\\' == Name[strlen(Name) - 1]
                || StabEntry[i].n_value < ImageBase)
              {
                continue;
              }
            if ( First || Address != Current->Address )
              {
                if ( !First )
                  {
                    memset ( ++Current, 0, sizeof(*Current) );
                    Current->FunctionOffset = Current[-1].FunctionOffset;
                  }
                else
                  First = 0;
                Current->Address = Address;
              }
            Current->FileOffset = FindOrAddString((char *)StabStringsBase
                                                  + StabEntry[i].n_strx,
                                                  StringsLength,
                                                  StringsBase);
            break;
          case N_FUN:
            if (0 == StabEntry[i].n_desc || StabEntry[i].n_value < ImageBase)
              {
                LastFunctionAddress = 0; /* line # 0 = end of function */
                continue;
              }
            if ( First || Address != Current->Address )
              {
                if ( !First )
                  memset ( ++Current, 0, sizeof(*Current) );
                else
                  First = 0;
                Current->Address = Address;
                Current->FileOffset = Current[-1].FileOffset;
              }
            Name = (char *) StabStringsBase + StabEntry[i].n_strx;
            NameLen = strcspn(Name, ":");
            if (sizeof(FuncName) <= NameLen)
              {
                free(*SymbolsBase);
                fprintf(stderr, "Function name too long\n");
                return 1;
              }
            memcpy(FuncName, Name, NameLen);
            FuncName[NameLen] = '\0';
            Current->FunctionOffset = FindOrAddString(FuncName,
                                                      StringsLength,
                                                      StringsBase);
            Current->SourceLine = 0;
            LastFunctionAddress = Address;
            break;
          case N_SLINE:
            if ( First || Address != Current->Address )
              {
                if ( !First )
                  {
                    memset ( ++Current, 0, sizeof(*Current) );
                    Current->FileOffset = Current[-1].FileOffset;
                    Current->FunctionOffset = Current[-1].FunctionOffset;
                  }
                else
                  First = 0;
                Current->Address = Address;
              }
            Current->SourceLine = StabEntry[i].n_desc;
            break;
          default:
            continue;
        }
    }
  *SymbolsCount = (Current - *SymbolsBase + 1);

  qsort(*SymbolsBase, *SymbolsCount, sizeof(ROSSYM_ENTRY), (int (*)(const void *, const void *)) CompareSymEntry);

  return 0;
}

static int
ConvertCoffs(ULONG *SymbolsCount, PROSSYM_ENTRY *SymbolsBase,
             ULONG *StringsLength, void *StringsBase,
             ULONG CoffSymbolsLength, void *CoffSymbolsBase,
             ULONG CoffStringsLength, void *CoffStringsBase,
             ULONG_PTR ImageBase, PIMAGE_FILE_HEADER PEFileHeader,
             PIMAGE_SECTION_HEADER PESectionHeaders)
{
  ULONG Count, i;
  PCOFF_SYMENT CoffEntry;
  char FuncName[256];
  char *p;
  PROSSYM_ENTRY Current;

  CoffEntry = (PCOFF_SYMENT) CoffSymbolsBase;
  Count = CoffSymbolsLength / sizeof(COFF_SYMENT);
  *SymbolsBase = malloc(Count * sizeof(ROSSYM_ENTRY));
  if (NULL == *SymbolsBase)
    {
      fprintf(stderr, "Unable to allocate memory for converted COFF symbols\n");
      return 1;
    }
  *SymbolsCount = 0;
  Current = *SymbolsBase;

  for (i = 0; i < Count; i++)
    {
      if (ISFCN(CoffEntry[i].e_type) || C_EXT == CoffEntry[i].e_sclass)
        {
          Current->Address = CoffEntry[i].e_value;
          if (0 < CoffEntry[i].e_scnum)
            {
              if (PEFileHeader->NumberOfSections < CoffEntry[i].e_scnum)
                {
                  free(*SymbolsBase);
                  fprintf(stderr, "Invalid section number %d in COFF symbols (only %d sections present)\n",
                          CoffEntry[i].e_scnum, PEFileHeader->NumberOfSections);
                  return 1;
                }
              Current->Address += PESectionHeaders[CoffEntry[i].e_scnum - 1].VirtualAddress;
            }
          Current->FileOffset = 0;
          if (0 == CoffEntry[i].e.e.e_zeroes)
            {
              if (sizeof(FuncName) <= strlen((char *) CoffStringsBase + CoffEntry[i].e.e.e_offset))
                {
                  free(*SymbolsBase);
                  fprintf(stderr, "Function name too long\n");
                  return 1;
                }
              strcpy(FuncName, (char *) CoffStringsBase + CoffEntry[i].e.e.e_offset);
            }
          else
            {
              memcpy(FuncName, CoffEntry[i].e.e_name, E_SYMNMLEN);
              FuncName[E_SYMNMLEN] = '\0';
            }

          /* Name demangling: stdcall */
          p = strrchr(FuncName, '@');
          if (NULL != p)
            {
              *p = '\0';
            }
          p = ('_' == FuncName[0] || '@' == FuncName[0] ? FuncName + 1 : FuncName);
          Current->FunctionOffset = FindOrAddString(p,
                                                                         StringsLength,
                                                                         StringsBase);
          Current->SourceLine = 0;
          memset ( ++Current, 0, sizeof(*Current) );
        }
      i += CoffEntry[i].e_numaux;
    }

  *SymbolsCount = (Current - *SymbolsBase + 1);
  qsort(*SymbolsBase, *SymbolsCount, sizeof(ROSSYM_ENTRY), (int (*)(const void *, const void *)) CompareSymEntry);

  return 0;
}

static int
MergeStabsAndCoffs(ULONG *MergedSymbolCount, PROSSYM_ENTRY *MergedSymbols,
                   ULONG StabSymbolsCount, PROSSYM_ENTRY StabSymbols,
                   ULONG CoffSymbolsCount, PROSSYM_ENTRY CoffSymbols)
{
  ULONG StabIndex, j;
  ULONG CoffIndex;
  ULONG_PTR StabFunctionStartAddress;
  ULONG StabFunctionStringOffset, NewStabFunctionStringOffset;

  *MergedSymbolCount = 0;
  if (StabSymbolsCount == 0)
    {
      *MergedSymbols = NULL;
      return 0;
    }
  *MergedSymbols = malloc(StabSymbolsCount * sizeof(ROSSYM_ENTRY));
  if (NULL == *MergedSymbols)
    {
      fprintf(stderr, "Unable to allocate memory for merged symbols\n");
      return 1;
    }

  StabFunctionStartAddress = 0;
  StabFunctionStringOffset = 0;
  CoffIndex = 0;
  for (StabIndex = 0; StabIndex < StabSymbolsCount; StabIndex++)
    {
      (*MergedSymbols)[*MergedSymbolCount] = StabSymbols[StabIndex];
      for (j = StabIndex + 1;
           j < StabSymbolsCount && StabSymbols[j].Address == StabSymbols[StabIndex].Address;
           j++)
        {
          if (0 != StabSymbols[j].FileOffset && 0 == (*MergedSymbols)[*MergedSymbolCount].FileOffset)
            {
              (*MergedSymbols)[*MergedSymbolCount].FileOffset = StabSymbols[j].FileOffset;
            }
          if (0 != StabSymbols[j].FunctionOffset && 0 == (*MergedSymbols)[*MergedSymbolCount].FunctionOffset)
            {
              (*MergedSymbols)[*MergedSymbolCount].FunctionOffset = StabSymbols[j].FunctionOffset;
            }
          if (0 != StabSymbols[j].SourceLine && 0 == (*MergedSymbols)[*MergedSymbolCount].SourceLine)
            {
              (*MergedSymbols)[*MergedSymbolCount].SourceLine = StabSymbols[j].SourceLine;
            }
        }
      StabIndex = j - 1;
      while (CoffIndex < CoffSymbolsCount
          && CoffSymbols[CoffIndex + 1].Address <= (*MergedSymbols)[*MergedSymbolCount].Address)
        {
          CoffIndex++;
        }
      NewStabFunctionStringOffset = (*MergedSymbols)[*MergedSymbolCount].FunctionOffset;
      if (CoffSymbolsCount > 0 &&
          CoffSymbols[CoffIndex].Address < (*MergedSymbols)[*MergedSymbolCount].Address
          && StabFunctionStartAddress < CoffSymbols[CoffIndex].Address
          && 0 != CoffSymbols[CoffIndex].FunctionOffset)
        {
          (*MergedSymbols)[*MergedSymbolCount].FunctionOffset = CoffSymbols[CoffIndex].FunctionOffset;
        }
      if (StabFunctionStringOffset != NewStabFunctionStringOffset)
        {
          StabFunctionStartAddress = (*MergedSymbols)[*MergedSymbolCount].Address;
        }
      StabFunctionStringOffset = NewStabFunctionStringOffset;
      (*MergedSymbolCount)++;
    }

  return 0;
}

static PIMAGE_SECTION_HEADER
FindSectionForRVA(DWORD RVA, unsigned NumberOfSections, PIMAGE_SECTION_HEADER SectionHeaders)
{
  unsigned Section;

  for (Section = 0; Section < NumberOfSections; Section++)
    {
      if (SectionHeaders[Section].VirtualAddress <= RVA &&
          RVA < SectionHeaders[Section].VirtualAddress + SectionHeaders[Section].Misc.VirtualSize)
        {
          return SectionHeaders + Section;
        }
    }

  return NULL;
}

static int
IncludeRelocationsForSection(PIMAGE_SECTION_HEADER SectionHeader)
{
  static char *BlacklistedSections[] =
    {
      ".edata",
      ".idata",
      ".reloc"
    };
  char SectionName[IMAGE_SIZEOF_SHORT_NAME];
  unsigned i;

  if (0 != (SectionHeader->Characteristics & IMAGE_SCN_LNK_REMOVE))
    {
      return 0;
    }

  for (i = 0; i < sizeof(BlacklistedSections) / sizeof(BlacklistedSections[0]); i++)
    {
      strncpy(SectionName, BlacklistedSections[i], IMAGE_SIZEOF_SHORT_NAME);
      if (0 == memcmp(SectionName, SectionHeader->Name, IMAGE_SIZEOF_SHORT_NAME))
        {
          return 0;
        }
    }

  return 1;
}

static int
ProcessRelocations(ULONG *ProcessedRelocsLength, void **ProcessedRelocs,
                   void *RawData, PIMAGE_OPTIONAL_HEADER OptHeader,
                   unsigned NumberOfSections, PIMAGE_SECTION_HEADER SectionHeaders)
{
  PIMAGE_SECTION_HEADER RelocSectionHeader, TargetSectionHeader;
  PIMAGE_BASE_RELOCATION BaseReloc, End, AcceptedRelocs;
  int Found;

  if (OptHeader->NumberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_BASERELOC
      || 0 == OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
    {
      /* No relocation entries */
      *ProcessedRelocsLength = 0;
      *ProcessedRelocs = NULL;
      return 0;
    }

  RelocSectionHeader = FindSectionForRVA(OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                                         NumberOfSections, SectionHeaders);
  if (NULL == RelocSectionHeader)
    {
      fprintf(stderr, "Can't find section header for relocation data\n");
      return 1;
    }

  *ProcessedRelocs = malloc(RelocSectionHeader->SizeOfRawData);
  if (NULL == *ProcessedRelocs)
    {
      fprintf(stderr, "Failed to allocate %lu bytes for relocations\n", RelocSectionHeader->SizeOfRawData);
      return 1;
    }
  *ProcessedRelocsLength = 0;

  BaseReloc = (PIMAGE_BASE_RELOCATION) ((char *) RawData
                                        + RelocSectionHeader->PointerToRawData
                                        + (OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress -
                                           RelocSectionHeader->VirtualAddress));
  End = (PIMAGE_BASE_RELOCATION) ((char *) BaseReloc
                                  + OptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);

  while (BaseReloc < End && 0 < BaseReloc->SizeOfBlock)
    {
      TargetSectionHeader = FindSectionForRVA(BaseReloc->VirtualAddress, NumberOfSections,
                                              SectionHeaders);
      if (NULL != TargetSectionHeader && IncludeRelocationsForSection(TargetSectionHeader))
        {
          AcceptedRelocs = *ProcessedRelocs;
          Found = 0;
          while (AcceptedRelocs < (PIMAGE_BASE_RELOCATION) ((char *) *ProcessedRelocs +
                                                            *ProcessedRelocsLength)
                 && ! Found)
            {
              Found = BaseReloc->SizeOfBlock == AcceptedRelocs->SizeOfBlock
                      && 0 == memcmp(BaseReloc, AcceptedRelocs, AcceptedRelocs->SizeOfBlock);
              AcceptedRelocs= (PIMAGE_BASE_RELOCATION) ((char *) AcceptedRelocs +
                                                        AcceptedRelocs->SizeOfBlock);
            }
          if (! Found)
            {
              memcpy((char *) *ProcessedRelocs + *ProcessedRelocsLength,
                     BaseReloc, BaseReloc->SizeOfBlock);
              *ProcessedRelocsLength += BaseReloc->SizeOfBlock;
            }
        }
      BaseReloc = (PIMAGE_BASE_RELOCATION)((char *) BaseReloc + BaseReloc->SizeOfBlock);
    }

  return 0;
}

static int
CreateOutputFile(FILE *OutFile, void *InData,
                 PIMAGE_DOS_HEADER InDosHeader, PIMAGE_FILE_HEADER InFileHeader,
                 PIMAGE_OPTIONAL_HEADER InOptHeader, PIMAGE_SECTION_HEADER InSectionHeaders,
                 ULONG RosSymLength, void *RosSymSection)
{
  ULONG StartOfRawData;
  unsigned Section;
  void *OutHeader, *ProcessedRelocs, *PaddedRosSym, *Data;
  PIMAGE_DOS_HEADER OutDosHeader;
  PIMAGE_FILE_HEADER OutFileHeader;
  PIMAGE_OPTIONAL_HEADER OutOptHeader;
  PIMAGE_SECTION_HEADER OutSectionHeaders, CurrentSectionHeader;
  DWORD CheckSum;
  ULONG Length, i;
  ULONG ProcessedRelocsLength;
  ULONG RosSymOffset, RosSymFileLength;
  int InRelocSectionIndex;
  PIMAGE_SECTION_HEADER OutRelocSection;

  StartOfRawData = 0;
  for (Section = 0; Section < InFileHeader->NumberOfSections; Section++)
    {
      if ((0 == StartOfRawData
           || InSectionHeaders[Section].PointerToRawData < StartOfRawData)
          && 0 != InSectionHeaders[Section].PointerToRawData
          && 0 == (InSectionHeaders[Section].Characteristics & IMAGE_SCN_LNK_REMOVE))
        {
          StartOfRawData = InSectionHeaders[Section].PointerToRawData;
        }
    }
  OutHeader = malloc(StartOfRawData);
  if (NULL == OutHeader)
    {
      fprintf(stderr, "Failed to allocate %lu bytes for output file header\n", StartOfRawData);
      return 1;
    }
  memset(OutHeader, '\0', StartOfRawData);

  OutDosHeader = (PIMAGE_DOS_HEADER) OutHeader;
  memcpy(OutDosHeader, InDosHeader, InDosHeader->e_lfanew + sizeof(ULONG));

  OutFileHeader = (PIMAGE_FILE_HEADER)((char *) OutHeader + OutDosHeader->e_lfanew + sizeof(ULONG));
  memcpy(OutFileHeader, InFileHeader, sizeof(IMAGE_FILE_HEADER));
  OutFileHeader->PointerToSymbolTable = 0;
  OutFileHeader->NumberOfSymbols = 0;
  OutFileHeader->Characteristics &= ~(IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_LOCAL_SYMS_STRIPPED |
                                      IMAGE_FILE_DEBUG_STRIPPED);

  OutOptHeader = (PIMAGE_OPTIONAL_HEADER)(OutFileHeader + 1);
  memcpy(OutOptHeader, InOptHeader, sizeof(IMAGE_OPTIONAL_HEADER));
  OutOptHeader->CheckSum = 0;

  OutSectionHeaders = (PIMAGE_SECTION_HEADER)((char *) OutOptHeader + OutFileHeader->SizeOfOptionalHeader);

  if (ProcessRelocations(&ProcessedRelocsLength, &ProcessedRelocs, InData, InOptHeader,
                         InFileHeader->NumberOfSections, InSectionHeaders))
    {
      return 1;
    }
  if (InOptHeader->NumberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_BASERELOC
      || 0 == InOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
    {
      InRelocSectionIndex = -1;
    }
  else
    {
      InRelocSectionIndex = FindSectionForRVA(InOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                                              InFileHeader->NumberOfSections, InSectionHeaders) - InSectionHeaders;
    }

  OutFileHeader->NumberOfSections = 0;
  CurrentSectionHeader = OutSectionHeaders;
  OutOptHeader->SizeOfImage = 0;
  RosSymOffset = 0;
  OutRelocSection = NULL;
  for (Section = 0; Section < InFileHeader->NumberOfSections; Section++)
    {
      if (0 == (InSectionHeaders[Section].Characteristics & IMAGE_SCN_LNK_REMOVE))
        {
          *CurrentSectionHeader = InSectionHeaders[Section];
          CurrentSectionHeader->PointerToLinenumbers = 0;
          CurrentSectionHeader->NumberOfLinenumbers = 0;
          if (OutOptHeader->SizeOfImage < CurrentSectionHeader->VirtualAddress +
                                          CurrentSectionHeader->Misc.VirtualSize)
            {
              OutOptHeader->SizeOfImage = ROUND_UP(CurrentSectionHeader->VirtualAddress +
                                                   CurrentSectionHeader->Misc.VirtualSize,
                                                   OutOptHeader->SectionAlignment);
            }
          if (RosSymOffset < CurrentSectionHeader->PointerToRawData + CurrentSectionHeader->SizeOfRawData)
            {
              RosSymOffset = CurrentSectionHeader->PointerToRawData + CurrentSectionHeader->SizeOfRawData;
            }
          if (Section == (ULONG)InRelocSectionIndex)
            {
              OutRelocSection = CurrentSectionHeader;
            }
          (OutFileHeader->NumberOfSections) ++;
          CurrentSectionHeader++;
        }
    }

  if (OutRelocSection == CurrentSectionHeader - 1)
    {
      OutOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = ProcessedRelocsLength;
      if (OutOptHeader->SizeOfImage == OutRelocSection->VirtualAddress +
                                       ROUND_UP(OutRelocSection->Misc.VirtualSize,
                                                OutOptHeader->SectionAlignment))
        {
          OutOptHeader->SizeOfImage = OutRelocSection->VirtualAddress +
                                      ROUND_UP(ProcessedRelocsLength,
                                               OutOptHeader->SectionAlignment);
        }
      OutRelocSection->Misc.VirtualSize = ProcessedRelocsLength;
      if (RosSymOffset == OutRelocSection->PointerToRawData
                          + OutRelocSection->SizeOfRawData)
        {
          RosSymOffset = OutRelocSection->PointerToRawData +
                         ROUND_UP(ProcessedRelocsLength, OutOptHeader->FileAlignment);
        }
      OutRelocSection->SizeOfRawData = ROUND_UP(ProcessedRelocsLength,
                                                OutOptHeader->FileAlignment);
    }

  if (RosSymLength > 0)
    {
      RosSymFileLength = ROUND_UP(RosSymLength, OutOptHeader->FileAlignment);
      memcpy(CurrentSectionHeader->Name, ".rossym", 8); /* We're lucky: string is exactly 8 bytes long */
      CurrentSectionHeader->Misc.VirtualSize = RosSymLength;
      CurrentSectionHeader->VirtualAddress = OutOptHeader->SizeOfImage;
      CurrentSectionHeader->SizeOfRawData = RosSymFileLength;
      CurrentSectionHeader->PointerToRawData = RosSymOffset;
      CurrentSectionHeader->PointerToRelocations = 0;
      CurrentSectionHeader->PointerToLinenumbers = 0;
      CurrentSectionHeader->NumberOfRelocations = 0;
      CurrentSectionHeader->NumberOfLinenumbers = 0;
      CurrentSectionHeader->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE
                                              | IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_TYPE_NOLOAD;
      OutOptHeader->SizeOfImage = ROUND_UP(CurrentSectionHeader->VirtualAddress +
                                           CurrentSectionHeader->Misc.VirtualSize,
                                       OutOptHeader->SectionAlignment);
      (OutFileHeader->NumberOfSections)++;

      PaddedRosSym = malloc(RosSymFileLength);
      if (NULL == PaddedRosSym)
        {
          fprintf(stderr, "Failed to allocate %lu bytes for padded .rossym\n", RosSymFileLength);
          return 1;
        }
      memcpy(PaddedRosSym, RosSymSection, RosSymLength);
      memset((char *) PaddedRosSym + RosSymLength, '\0', RosSymFileLength - RosSymLength);
    }
  else
    {
      PaddedRosSym = NULL;
    }
  CheckSum = 0;
  for (i = 0; i < StartOfRawData / 2; i++)
   {
      CheckSum += ((unsigned short*) OutHeader)[i];
      CheckSum = 0xffff & (CheckSum + (CheckSum >> 16));
   }
  Length = StartOfRawData;
  for (Section = 0; Section < OutFileHeader->NumberOfSections; Section++)
    {
      DWORD SizeOfRawData;
      if (OutRelocSection == OutSectionHeaders + Section)
        {
          Data = (void *) ProcessedRelocs;
	  SizeOfRawData = ProcessedRelocsLength;
        }
      else if (RosSymLength > 0 && Section + 1 == OutFileHeader->NumberOfSections)
        {
          Data = (void *) PaddedRosSym;
	  SizeOfRawData = OutSectionHeaders[Section].SizeOfRawData;
        }
      else
        {
          Data = (void *) ((char *) InData + OutSectionHeaders[Section].PointerToRawData);
	  SizeOfRawData = OutSectionHeaders[Section].SizeOfRawData;
        }
      for (i = 0; i < SizeOfRawData / 2; i++)
        {
          CheckSum += ((unsigned short*) Data)[i];
          CheckSum = 0xffff & (CheckSum + (CheckSum >> 16));
        }
      Length += OutSectionHeaders[Section].SizeOfRawData;
    }
  CheckSum += Length;
  OutOptHeader->CheckSum = CheckSum;

  if (fwrite(OutHeader, 1, StartOfRawData, OutFile) != StartOfRawData)
    {
      perror("Error writing output header\n");
      free(OutHeader);
      return 1;
    }

  for (Section = 0; Section < OutFileHeader->NumberOfSections; Section++)
    {
      if (0 != OutSectionHeaders[Section].SizeOfRawData)
        {
	  DWORD SizeOfRawData;
          fseek(OutFile, OutSectionHeaders[Section].PointerToRawData, SEEK_SET);
          if (OutRelocSection == OutSectionHeaders + Section)
            {
              Data = (void *) ProcessedRelocs;
	      SizeOfRawData = ProcessedRelocsLength;
            }
          else if (RosSymLength > 0 && Section + 1 == OutFileHeader->NumberOfSections)
            {
              Data = (void *) PaddedRosSym;
	      SizeOfRawData = OutSectionHeaders[Section].SizeOfRawData;
            }
          else
            {
              Data = (void *) ((char *) InData + OutSectionHeaders[Section].PointerToRawData);
	      SizeOfRawData = OutSectionHeaders[Section].SizeOfRawData;
            }
          if (fwrite(Data, 1, SizeOfRawData, OutFile) != SizeOfRawData)
            {
              perror("Error writing section data\n");
              free(PaddedRosSym);
              free(OutHeader);
              return 1;
            }
        }
   }

  if (PaddedRosSym)
    {
      free(PaddedRosSym);
    }
  free(OutHeader);

  return 0;
}

int main(int argc, char* argv[])
{
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  ULONG ImageBase;
  void *StabBase;
  ULONG StabsLength;
  void *StabStringBase;
  ULONG StabStringsLength;
  void *CoffBase = NULL;
  ULONG CoffsLength;
  void *CoffStringBase = NULL;
  ULONG CoffStringsLength;
  char* path1;
  char* path2;
  FILE* out;
  void *StringBase;
  ULONG StringsLength;
  ULONG StabSymbolsCount;
  PROSSYM_ENTRY StabSymbols;
  ULONG CoffSymbolsCount;
  PROSSYM_ENTRY CoffSymbols;
  ULONG MergedSymbolsCount;
  PROSSYM_ENTRY MergedSymbols;
  size_t FileSize;
  void *FileData;
  ULONG RosSymLength;
  void *RosSymSection;
  char elfhdr[4] = { '\177', 'E', 'L', 'F' };

  if (3 != argc)
    {
      fprintf(stderr, "Usage: rsym <exefile> <symfile>\n");
      exit(1);
    }

  path1 = convert_path(argv[1]);
  path2 = convert_path(argv[2]);

  FileData = load_file ( path1, &FileSize );
  if ( !FileData )
  {
    fprintf ( stderr, "An error occured loading '%s'\n", path1 );
    exit(1);
  }

  /* Check if MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) FileData;
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
    {
      /* Ignore elf */
      if (!memcmp(PEDosHeader, elfhdr, sizeof(elfhdr)))
	exit(0);
      perror("Input file is not a PE image.\n");
      free(FileData);
      exit(1);
    }

  /* Locate PE file header  */
  /* sizeof(ULONG) = sizeof(MAGIC) */
  PEFileHeader = (PIMAGE_FILE_HEADER)((char *) FileData + PEDosHeader->e_lfanew + sizeof(ULONG));

  /* Locate optional header */
  assert(sizeof(ULONG) == 4);
  PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
  ImageBase = PEOptHeader->ImageBase;

  /* Locate PE section headers  */
  PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *) PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

  if (GetStabInfo(FileData, PEFileHeader, PESectionHeaders, &StabsLength, &StabBase,
                  &StabStringsLength, &StabStringBase))
    {
      free(FileData);
      exit(1);
    }

  if (GetCoffInfo(FileData, PEFileHeader, PESectionHeaders, &CoffsLength, &CoffBase,
                  &CoffStringsLength, &CoffStringBase))
    {
      free(FileData);
      exit(1);
    }

  StringBase = malloc(1 + StabStringsLength + CoffStringsLength +
                      (CoffsLength / sizeof(ROSSYM_ENTRY)) * (E_SYMNMLEN + 1));
  if (NULL == StringBase)
    {
      free(FileData);
      fprintf(stderr, "Failed to allocate memory for strings table\n");
      exit(1);
    }
  /* Make offset 0 into an empty string */
  *((char *) StringBase) = '\0';
  StringsLength = 1;

  if (ConvertStabs(&StabSymbolsCount, &StabSymbols, &StringsLength, StringBase,
                   StabsLength, StabBase, StabStringsLength, StabStringBase,
                   ImageBase, PEFileHeader, PESectionHeaders))
    {
      free(StringBase);
      free(FileData);
      fprintf(stderr, "Failed to allocate memory for strings table\n");
      exit(1);
    }

  if (ConvertCoffs(&CoffSymbolsCount, &CoffSymbols, &StringsLength, StringBase,
                   CoffsLength, CoffBase, CoffStringsLength, CoffStringBase,
                   ImageBase, PEFileHeader, PESectionHeaders))
    {
      if (StabSymbols)
        {
          free(StabSymbols);
        }
      free(StringBase);
      free(FileData);
      exit(1);
    }

  if (MergeStabsAndCoffs(&MergedSymbolsCount, &MergedSymbols,
                         StabSymbolsCount, StabSymbols,
                         CoffSymbolsCount, CoffSymbols))
    {
      if (CoffSymbols)
        {
         free(CoffSymbols);
        }
      if (StabSymbols)
        {
          free(StabSymbols);
        }
      free(StringBase);
      free(FileData);
      exit(1);
    }

  if (CoffSymbols)
    {
      free(CoffSymbols);
    }
  if (StabSymbols)
    {
      free(StabSymbols);
    }
  if (MergedSymbolsCount == 0)
    {
      RosSymLength = 0;
      RosSymSection = NULL;
    }
  else
    {
      RosSymLength = sizeof(SYMBOLFILE_HEADER) + MergedSymbolsCount * sizeof(ROSSYM_ENTRY)
                            + StringsLength;
      RosSymSection = malloc(RosSymLength);
      if (NULL == RosSymSection)
        {
          free(MergedSymbols);
          free(StringBase);
          free(FileData);
          fprintf(stderr, "Unable to allocate memory for .rossym section\n");
          exit(1);
        }
      memset(RosSymSection, '\0', RosSymLength);

      SymbolFileHeader = (PSYMBOLFILE_HEADER) RosSymSection;
      SymbolFileHeader->SymbolsOffset = sizeof(SYMBOLFILE_HEADER);
      SymbolFileHeader->SymbolsLength = MergedSymbolsCount * sizeof(ROSSYM_ENTRY);
      SymbolFileHeader->StringsOffset = SymbolFileHeader->SymbolsOffset + SymbolFileHeader->SymbolsLength;
      SymbolFileHeader->StringsLength = StringsLength;

      memcpy((char *) RosSymSection + SymbolFileHeader->SymbolsOffset, MergedSymbols,
             SymbolFileHeader->SymbolsLength);
      memcpy((char *) RosSymSection + SymbolFileHeader->StringsOffset, StringBase,
             SymbolFileHeader->StringsLength);

      free(MergedSymbols);
    }
  free(StringBase);
  out = fopen(path2, "wb");
  if (out == NULL)
    {
      perror("Cannot open output file");
      free(RosSymSection);
      free(FileData);
      exit(1);
    }

  if (CreateOutputFile(out, FileData, PEDosHeader, PEFileHeader, PEOptHeader,
                       PESectionHeaders, RosSymLength, RosSymSection))
    {
      fclose(out);
      if (RosSymSection)
        {
          free(RosSymSection);
        }
      free(FileData);
      exit(1);
    }

  fclose(out);
  if (RosSymSection)
    {
      free(RosSymSection);
    }
  free(FileData);

  return 0;
}

/* EOF */
