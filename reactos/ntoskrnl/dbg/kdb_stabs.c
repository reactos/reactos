/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/*
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/i386/exp.c
 * PURPOSE:              Handling exceptions
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              ??/??/??: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/module.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/trap.h>
#include <ntdll/ldr.h>
#include <internal/safe.h>
#include <internal/kd.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef struct _SYMBOLFILE_HEADER {
  unsigned long StabsOffset;
  unsigned long StabsLength;
  unsigned long StabstrOffset;
  unsigned long StabstrLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _IMAGE_SYMBOL_INFO_CACHE {
  LIST_ENTRY ListEntry;
  UNICODE_STRING FullName;
  PVOID FileBuffer;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
} IMAGE_SYMBOL_INFO_CACHE, *PIMAGE_SYMBOL_INFO_CACHE;


typedef struct _STAB_ENTRY {
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} _STAB_ENTRY, *PSTAB_ENTRY;

/*
 * Desc - Line number
 * Value - Relative virtual address
 */
#define N_FUN 0x24

/*
 * Desc - Line number
 * Value - Relative virtual address
 */
#define N_SLINE 0x44

/*
 * String - First containing a '/' is the compillation directory (CD)
 *          Not containing a '/' is a source file relative to CD
 */
#define N_SO 0x64

static LIST_ENTRY SymbolListHead;
static KSPIN_LOCK SymbolListLock;

NTSTATUS
LdrGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG LineNumber,
  OUT PCH FileName  OPTIONAL,
  OUT PCH FunctionName  OPTIONAL);

VOID
KdbLdrUnloadModuleSymbols(PIMAGE_SYMBOL_INFO SymbolInfo);

/* FUNCTIONS ****************************************************************/

BOOLEAN 
KdbPrintUserAddress(PVOID address)
{
   PLIST_ENTRY current_entry;
   PLDR_MODULE current;
   PEPROCESS CurrentProcess;
   PPEB Peb = NULL;
   ULONG_PTR RelativeAddress;
   NTSTATUS Status;
   ULONG LineNumber;
   CHAR FileName[256];
   CHAR FunctionName[256];

   CurrentProcess = PsGetCurrentProcess();
   if (NULL != CurrentProcess)
    {
      Peb = CurrentProcess->Peb;
    }

   if (NULL == Peb)
	   {
       DbgPrint("<%x>", address);
       return(TRUE);
     }

   current_entry = Peb->Ldr->InLoadOrderModuleList.Flink;
   
   while (current_entry != &Peb->Ldr->InLoadOrderModuleList &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, LDR_MODULE, InLoadOrderModuleList);
	
	if (address >= (PVOID)current->BaseAddress &&
	    address < (PVOID)(current->BaseAddress + current->SizeOfImage))
	  {
            RelativeAddress = (ULONG_PTR) address - (ULONG_PTR)current->BaseAddress;
            Status = LdrGetAddressInformation(&current->SymbolInfo,
              RelativeAddress,
              &LineNumber,
              FileName,
              FunctionName);
            if (NT_SUCCESS(Status))
              {
                DbgPrint("<%wZ: %x (%s:%d (%s))>",
                  &current->BaseDllName, RelativeAddress, FileName, LineNumber, FunctionName);
              }
            else
             {
               DbgPrint("<%wZ: %x>", &current->BaseDllName, RelativeAddress);
             }
	     return(TRUE);
	  }

	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

BOOLEAN 
KdbPrintAddress(PVOID address)
{
   PLIST_ENTRY current_entry;
   MODULE_TEXT_SECTION* current;
   extern LIST_ENTRY ModuleTextListHead;
   ULONG_PTR RelativeAddress;
   NTSTATUS Status;
   ULONG LineNumber;
   CHAR FileName[256];
   CHAR FunctionName[256];

   current_entry = ModuleTextListHead.Flink;
   
   while (current_entry != &ModuleTextListHead &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);

	if (address >= (PVOID)current->Base &&
	    address < (PVOID)(current->Base + current->Length))
	  {
            RelativeAddress = (ULONG_PTR) address - current->Base;
            Status = LdrGetAddressInformation(&current->SymbolInfo,
              RelativeAddress,
              &LineNumber,
              FileName,
              FunctionName);
            if (NT_SUCCESS(Status))
              {
                DbgPrint("<%ws: %x (%s:%d (%s))>",
                  current->Name, RelativeAddress, FileName, LineNumber, FunctionName);
              }
            else
             {
               DbgPrint("<%ws: %x>", current->Name, RelativeAddress);
             }
	     return(TRUE);
	  }
	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

VOID
KdbFreeSymbolsProcess(PEPROCESS Process)
{
  PLIST_ENTRY CurrentEntry;
  PLDR_MODULE Current;
  PIMAGE_SYMBOL_INFO SymbolInfo;
  PEPROCESS CurrentProcess;
  PPEB Peb;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
  {
    KeAttachProcess(Process);
  }
  Peb = Process->Peb;
  assert (Peb);
  assert (Peb->Ldr);

  CurrentEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
  while (CurrentEntry != &Peb->Ldr->InLoadOrderModuleList && 
	 CurrentEntry != NULL)
    {
      Current = CONTAINING_RECORD(CurrentEntry, LDR_MODULE, 
				  InLoadOrderModuleList);

      SymbolInfo = &Current->SymbolInfo;
      KdbLdrUnloadModuleSymbols(SymbolInfo);

      CurrentEntry = CurrentEntry->Flink;
    }
  if (CurrentProcess != Process)
  {
    KeDetachProcess();
  }
}

VOID
KdbLdrInit(MODULE_TEXT_SECTION* NtoskrnlTextSection,
	   MODULE_TEXT_SECTION* LdrHalTextSection)
{
  RtlZeroMemory(&NtoskrnlTextSection->SymbolInfo, 
		sizeof(NtoskrnlTextSection->SymbolInfo));
  NtoskrnlTextSection->SymbolInfo.ImageBase = 
    NtoskrnlTextSection->OptionalHeader->ImageBase;
  NtoskrnlTextSection->SymbolInfo.ImageSize = NtoskrnlTextSection->Length;

  RtlZeroMemory(&LdrHalTextSection->SymbolInfo, 
		sizeof(LdrHalTextSection->SymbolInfo));
  LdrHalTextSection->SymbolInfo.ImageBase = 
    LdrHalTextSection->OptionalHeader->ImageBase;
  LdrHalTextSection->SymbolInfo.ImageSize = LdrHalTextSection->Length;

  InitializeListHead(&SymbolListHead);
  KeInitializeSpinLock(&SymbolListLock);
}

VOID
LdrpParseImageSymbols(PIMAGE_SYMBOL_INFO SymbolInfo)
/* Note: It is important that the symbol strings buffer not be released after
   this function is called because the strings are still referenced */
{
  PSYMBOL CurrentFileNameSymbol;
  PSYMBOL CurrentFunctionSymbol;
  PSYMBOL CurrentLineNumberSymbol;
  PSYMBOL Symbol;
  PSTAB_ENTRY StabEntry;
  PVOID StabsEnd;
  PCHAR String;
  ULONG_PTR FunRelativeAddress;
  ULONG FunLineNumber;
  ULONG_PTR ImageBase;

  assert(SymbolInfo);

  DPRINT("Parsing symbols.\n");

  SymbolInfo->FileNameSymbols.SymbolCount = 0;
  SymbolInfo->FileNameSymbols.Symbols = NULL;
  SymbolInfo->FunctionSymbols.SymbolCount = 0;
  SymbolInfo->FunctionSymbols.Symbols = NULL;
  SymbolInfo->LineNumberSymbols.SymbolCount = 0;
  SymbolInfo->LineNumberSymbols.Symbols = NULL;
  StabsEnd = SymbolInfo->SymbolsBase + SymbolInfo->SymbolsLength;
  StabEntry = (PSTAB_ENTRY) SymbolInfo->SymbolsBase;
  ImageBase = SymbolInfo->ImageBase;
  FunRelativeAddress = 0;
  FunLineNumber = 0;
  CurrentFileNameSymbol = NULL;
  CurrentFunctionSymbol = NULL;
  CurrentLineNumberSymbol = NULL;
  while ((ULONG_PTR) StabEntry < (ULONG_PTR) StabsEnd)
    {
      Symbol = NULL;

      if (StabEntry->n_type == N_FUN)
        {
          if (StabEntry->n_desc > 0)
            {
              assert(StabEntry->n_value >= ImageBase);

              FunRelativeAddress = StabEntry->n_value - ImageBase;
              FunLineNumber = StabEntry->n_desc;

              Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
              assert(Symbol);
              Symbol->Next = NULL;
              Symbol->SymbolType = ST_FUNCTION;
              Symbol->RelativeAddress = FunRelativeAddress;
              Symbol->LineNumber = FunLineNumber;
              String = (PCHAR)SymbolInfo->SymbolStringsBase + StabEntry->n_strx;
              RtlInitAnsiString(&Symbol->Name, String);

              DPRINT("FUN found. '%s' %d @ %x\n",
                Symbol->Name.Buffer, FunLineNumber, FunRelativeAddress);
            }
        }
      else if (StabEntry->n_type == N_SLINE)
        {
          Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
          assert(Symbol);
          Symbol->Next = NULL;
          Symbol->SymbolType = ST_LINENUMBER;
          Symbol->RelativeAddress = FunRelativeAddress + StabEntry->n_value;
          Symbol->LineNumber = StabEntry->n_desc;

          DPRINT("SLINE found. %d @ %x\n",
            Symbol->LineNumber, Symbol->RelativeAddress);
        }
      else if (StabEntry->n_type == N_SO)
        {
          Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
          assert(Symbol);
          Symbol->Next = NULL;
          Symbol->SymbolType = ST_FILENAME;
          Symbol->RelativeAddress = StabEntry->n_value - ImageBase;
          Symbol->LineNumber = 0;
          String = (PCHAR)SymbolInfo->SymbolStringsBase + StabEntry->n_strx;
          RtlInitAnsiString(&Symbol->Name, String);

          DPRINT("SO found. '%s' @ %x\n",
            Symbol->Name.Buffer, Symbol->RelativeAddress);
        }

      if (Symbol != NULL)
        {
          switch (Symbol->SymbolType)
          {
            case ST_FILENAME:
              if (SymbolInfo->FileNameSymbols.Symbols == NULL)
                SymbolInfo->FileNameSymbols.Symbols = Symbol;
              else
                CurrentFileNameSymbol->Next = Symbol;

              CurrentFileNameSymbol = Symbol;

              SymbolInfo->FileNameSymbols.SymbolCount++;
              break;
            case ST_FUNCTION:
              if (SymbolInfo->FunctionSymbols.Symbols == NULL)
                SymbolInfo->FunctionSymbols.Symbols = Symbol;
              else
                CurrentFunctionSymbol->Next = Symbol;

              CurrentFunctionSymbol = Symbol;

              SymbolInfo->FunctionSymbols.SymbolCount++;
              break;
            case ST_LINENUMBER:
              if (SymbolInfo->LineNumberSymbols.Symbols == NULL)
                SymbolInfo->LineNumberSymbols.Symbols = Symbol;
              else
                CurrentLineNumberSymbol->Next = Symbol;

              CurrentLineNumberSymbol = Symbol;

              SymbolInfo->LineNumberSymbols.SymbolCount++;
              break;
          }
        }

      StabEntry++;
    }
}

static NTSTATUS
LdrpGetFileName(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PCH  FileName)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->FileNameSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("FN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_FILENAME) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          DPRINT("FN found\n");
          strcpy(FileName, Symbol->Name.Buffer);
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("FN not found\n");

  return STATUS_UNSUCCESSFUL;
}

static NTSTATUS
LdrpGetFunctionName(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PCH  FunctionName)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->FunctionSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("FUN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_FUNCTION) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          PCHAR ExtraInfo;
          ULONG Length;

          DPRINT("FUN found\n");

          /* Remove the extra information from the function name */
          ExtraInfo = strchr(Symbol->Name.Buffer, ':');
          if (ExtraInfo != NULL)
            Length = ExtraInfo - Symbol->Name.Buffer;
          else
            Length = strlen(Symbol->Name.Buffer);

          strncpy(FunctionName, Symbol->Name.Buffer, Length);
	  FunctionName[Length]=0;
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("FUN not found\n");

  return STATUS_UNSUCCESSFUL;
}

static NTSTATUS
LdrpGetLineNumber(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG  LineNumber)
{
  PSYMBOL NextSymbol;
  ULONG_PTR NextAddress;
  PSYMBOL Symbol;

  Symbol = SymbolInfo->LineNumberSymbols.Symbols;
  while (Symbol != NULL)
    {
      NextSymbol = Symbol->Next;
      if (NextSymbol != NULL)
        NextAddress = NextSymbol->RelativeAddress;
      else
        NextAddress = SymbolInfo->ImageSize;

      DPRINT("LN SEARCH: Type %d  RelativeAddress %x >= Symbol->RelativeAddress %x  < NextAddress %x\n",
        Symbol->SymbolType, RelativeAddress, Symbol->RelativeAddress, NextAddress);

      if ((Symbol->SymbolType == ST_LINENUMBER) &&
        (RelativeAddress >= Symbol->RelativeAddress) &&
        (RelativeAddress < NextAddress))
        {
          DPRINT("LN found\n");
          *LineNumber = Symbol->LineNumber;
          return STATUS_SUCCESS;
        }
      Symbol = NextSymbol;
    }

  DPRINT("LN not found\n");

  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
LdrGetAddressInformation(IN PIMAGE_SYMBOL_INFO  SymbolInfo,
  IN ULONG_PTR  RelativeAddress,
  OUT PULONG LineNumber,
  OUT PCH FileName  OPTIONAL,
  OUT PCH FunctionName  OPTIONAL)
{
  NTSTATUS Status;

  *LineNumber = 0;

  DPRINT("RelativeAddress %p\n", RelativeAddress);

  if (RelativeAddress >= SymbolInfo->ImageSize)
    {
      DPRINT("Address is not within .text section. RelativeAddress %p  Length 0x%x\n",
        RelativeAddress, SymbolInfo->ImageSize);
      return STATUS_UNSUCCESSFUL;
    }

  if (!AreSymbolsParsed(SymbolInfo))
    {
      LdrpParseImageSymbols(SymbolInfo);
    }

  Status = LdrpGetLineNumber(SymbolInfo, RelativeAddress, LineNumber);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (FileName)
   {
     Status = LdrpGetFileName(SymbolInfo, RelativeAddress, FileName);
     if (!NT_SUCCESS(Status))
       {
         strcpy(FileName, "");
       }
   }

  if (FunctionName)
   {
     Status = LdrpGetFunctionName(SymbolInfo, RelativeAddress, FunctionName);
     if (!NT_SUCCESS(Status))
       {
         strcpy(FunctionName, "");
       }
   }

  return STATUS_SUCCESS;
}

VOID
LdrpLoadModuleSymbols(PUNICODE_STRING FileName,
		      PIMAGE_SYMBOL_INFO SymbolInfo)
{
  FILE_STANDARD_INFORMATION FileStdInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR TmpFileName[MAX_PATH];
  UNICODE_STRING SymFileName;
  LPWSTR Start, Ext;
  HANDLE FileHandle;
  PVOID FileBuffer;
  NTSTATUS Status;
  ULONG Length;
  IO_STATUS_BLOCK IoStatusBlock;
  PSYMBOLFILE_HEADER SymbolFileHeader;

  /*  Get the path to the symbol store  */
  wcscpy(TmpFileName, L"\\SystemRoot\\symbols\\");

  /*  Get the symbol filename from the module name  */
  Start = wcsrchr(FileName->Buffer, L'\\');
  if (Start == NULL)
    Start = FileName->Buffer;
  else
    Start++;

  Ext = wcsrchr(FileName->Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncat(TmpFileName, Start, Length);
  wcscat(TmpFileName, L".sym");
  RtlInitUnicodeString(&SymFileName, TmpFileName);

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             &SymFileName,
                             0,
                             NULL,
                             NULL);

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open symbol file: %wZ\n", &SymFileName);
      return;
    }

  CPRINT("Loading symbols from %wZ...\n", &SymFileName);

  /*  Get the size of the file  */
  Status = ZwQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not get file size\n");
      ZwClose(FileHandle);
      return;
    }

  /*  Allocate nonpageable memory for symbol file  */
  FileBuffer = ExAllocatePool(NonPagedPool,
                              FileStdInfo.EndOfFile.u.LowPart);

  if (FileBuffer == NULL)
    {
      DPRINT("Could not allocate memory for symbol file\n");
      ZwClose(FileHandle);
      return;
    }
   
  /*  Load file into memory chunk  */
  Status = ZwReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      FileBuffer,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not read symbol file into memory (Status 0x%x)\n", Status);
      ExFreePool(FileBuffer);
      ZwClose(FileHandle);
      return;
    }

  ZwClose(FileHandle);

  SymbolFileHeader = (PSYMBOLFILE_HEADER) FileBuffer;
  SymbolInfo->FileBuffer = FileBuffer;
  SymbolInfo->SymbolsBase = FileBuffer + SymbolFileHeader->StabsOffset;
  SymbolInfo->SymbolsLength = SymbolFileHeader->StabsLength;
  SymbolInfo->SymbolStringsBase = FileBuffer + SymbolFileHeader->StabstrOffset;
  SymbolInfo->SymbolStringsLength = SymbolFileHeader->StabstrLength;
}

VOID
KdbLdrUnloadModuleSymbols(PIMAGE_SYMBOL_INFO SymbolInfo)
{
  PSYMBOL NextSymbol;
  PSYMBOL Symbol;

  DPRINT("Unloading symbols\n");

  if (SymbolInfo != NULL)
    {
      Symbol = SymbolInfo->FileNameSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->FileNameSymbols.SymbolCount = 0;
      SymbolInfo->FileNameSymbols.Symbols = NULL;

      Symbol = SymbolInfo->FunctionSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->FunctionSymbols.SymbolCount = 0;
      SymbolInfo->FunctionSymbols.Symbols = NULL;

      Symbol = SymbolInfo->LineNumberSymbols.Symbols;
      while (Symbol != NULL)
	{
	  NextSymbol = Symbol->Next;
	  RtlFreeAnsiString(&Symbol->Name);
	  ExFreePool(Symbol);
	  Symbol = NextSymbol;
	}

      SymbolInfo->LineNumberSymbols.SymbolCount = 0;
      SymbolInfo->LineNumberSymbols.Symbols = NULL;
#if 0
      /* Don't free buffers because we cache symbol buffers
         (eg. they are shared across processes) */
      /* FIXME: We can free them if we do reference counting */
      if (SymbolInfo->FileBuffer != NULL)
        {
          ExFreePool(SymbolInfo->FileBuffer);
          SymbolInfo->FileBuffer = NULL;
          SymbolInfo->SymbolsBase = NULL;
          SymbolInfo->SymbolsLength = 0;
        }
#endif
    }
}


PIMAGE_SYMBOL_INFO_CACHE
LdrpLookupUserSymbolInfo(PLDR_MODULE LdrModule)
{
  PIMAGE_SYMBOL_INFO_CACHE Current;
  PLIST_ENTRY CurrentEntry;
  KIRQL Irql;

  DPRINT("Searching symbols for %S\n", LdrModule->FullDllName.Buffer);

  KeAcquireSpinLock(&SymbolListLock, &Irql);

  CurrentEntry = SymbolListHead.Flink;
  while (CurrentEntry != (&SymbolListHead))
    {
      Current = CONTAINING_RECORD(CurrentEntry, IMAGE_SYMBOL_INFO_CACHE, ListEntry);

      if (RtlEqualUnicodeString(&Current->FullName, &LdrModule->FullDllName, TRUE))
        {
          KeReleaseSpinLock(&SymbolListLock, Irql);
          return Current;
        }

      CurrentEntry = CurrentEntry->Flink;
    }

  KeReleaseSpinLock(&SymbolListLock, Irql);

  return(NULL);
}

VOID
KdbLdrLoadUserModuleSymbols(PLDR_MODULE LdrModule)
{
  PIMAGE_SYMBOL_INFO_CACHE CacheEntry;

  DPRINT("LdrModule %p\n", LdrModule);

  RtlZeroMemory(&LdrModule->SymbolInfo, sizeof(LdrModule->SymbolInfo));
  LdrModule->SymbolInfo.ImageBase = (ULONG_PTR) LdrModule->BaseAddress;
  LdrModule->SymbolInfo.ImageSize = LdrModule->SizeOfImage;

  CacheEntry = LdrpLookupUserSymbolInfo(LdrModule);
  if (CacheEntry != NULL)
    {
      DPRINT("Symbol cache hit for %S\n", CacheEntry->FullName.Buffer);
   
      LdrModule->SymbolInfo.FileBuffer = CacheEntry->FileBuffer;
      LdrModule->SymbolInfo.SymbolsBase = CacheEntry->SymbolsBase;
      LdrModule->SymbolInfo.SymbolsLength = CacheEntry->SymbolsLength;
      LdrModule->SymbolInfo.SymbolStringsBase = CacheEntry->SymbolStringsBase;
      LdrModule->SymbolInfo.SymbolStringsLength = CacheEntry->SymbolStringsLength;
    }
  else
    {
      CacheEntry = ExAllocatePool(NonPagedPool, sizeof(IMAGE_SYMBOL_INFO_CACHE));
      assert(CacheEntry);
      RtlZeroMemory(CacheEntry, sizeof(IMAGE_SYMBOL_INFO_CACHE));

      RtlCreateUnicodeString(&CacheEntry->FullName, LdrModule->FullDllName.Buffer);
      assert(CacheEntry->FullName.Buffer);
      LdrpLoadModuleSymbols(&LdrModule->FullDllName, &LdrModule->SymbolInfo);
      CacheEntry->FileBuffer = LdrModule->SymbolInfo.FileBuffer;
      CacheEntry->SymbolsBase = LdrModule->SymbolInfo.SymbolsBase;
      CacheEntry->SymbolsLength = LdrModule->SymbolInfo.SymbolsLength;
      CacheEntry->SymbolStringsBase = LdrModule->SymbolInfo.SymbolStringsBase;
      CacheEntry->SymbolStringsLength = LdrModule->SymbolInfo.SymbolStringsLength;
      InsertTailList(&SymbolListHead, &CacheEntry->ListEntry);
    }
}

VOID
KdbLoadDriver(PUNICODE_STRING Filename, PMODULE_OBJECT Module)
{
  /* Load symbols for the image if available */
  LdrpLoadModuleSymbols(Filename, &Module->TextSection->SymbolInfo);
}

VOID
KdbUnloadDriver(PMODULE_OBJECT ModuleObject)
{
  /* Unload symbols for module if available */
  KdbLdrUnloadModuleSymbols(&ModuleObject->TextSection->SymbolInfo);
}

VOID
KdbProcessSymbolFile(PVOID ModuleLoadBase, PCHAR FileName, ULONG Length)
{
  PMODULE_OBJECT ModuleObject;
  UNICODE_STRING ModuleName;
  CHAR TmpBaseName[MAX_PATH];
  CHAR TmpFileName[MAX_PATH];
  PSYMBOLFILE_HEADER SymbolFileHeader;
  PIMAGE_SYMBOL_INFO SymbolInfo;
  ANSI_STRING AnsiString;

  DPRINT("Module %s is a symbol file\n", FileName);

  strncpy(TmpBaseName, FileName, Length);
  TmpBaseName[Length] = '\0';
  
  DPRINT("base: %s (Length %d)\n", TmpBaseName, Length);
  
  strcpy(TmpFileName, TmpBaseName);
  strcat(TmpFileName, ".sys");
  RtlInitAnsiString(&AnsiString, TmpFileName);
  
  RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
  ModuleObject = LdrGetModuleObject(&ModuleName);
  RtlFreeUnicodeString(&ModuleName);
  if (ModuleObject == NULL)
    {
      strcpy(TmpFileName, TmpBaseName);
      strcat(TmpFileName, ".exe");
      RtlInitAnsiString(&AnsiString, TmpFileName);
      RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
      ModuleObject = LdrGetModuleObject(&ModuleName);
      RtlFreeUnicodeString(&ModuleName);
    }
  if (ModuleObject != NULL)
    {
      SymbolInfo = (PIMAGE_SYMBOL_INFO) &ModuleObject->TextSection->SymbolInfo;
      SymbolFileHeader = (PSYMBOLFILE_HEADER) ModuleLoadBase;
      SymbolInfo->FileBuffer = ModuleLoadBase;
      SymbolInfo->SymbolsBase = ModuleLoadBase + SymbolFileHeader->StabsOffset;
      SymbolInfo->SymbolsLength = SymbolFileHeader->StabsLength;
      SymbolInfo->SymbolStringsBase = ModuleLoadBase + SymbolFileHeader->StabstrOffset;
      SymbolInfo->SymbolStringsLength = SymbolFileHeader->StabstrLength;
    } 
}

VOID
KdbInitializeDriver(PMODULE_TEXT_SECTION ModuleTextSection)
{
  RtlZeroMemory(&ModuleTextSection->SymbolInfo, sizeof(ModuleTextSection->SymbolInfo));
  ModuleTextSection->SymbolInfo.ImageBase = 
    ModuleTextSection->OptionalHeader->ImageBase;
  ModuleTextSection->SymbolInfo.ImageSize = ModuleTextSection->Length;
}

VOID
KdbLdrLoadAutoConfigDrivers(VOID)
{
  UNICODE_STRING ModuleName;
  PMODULE_OBJECT ModuleObject;

  /*
   * Load symbols for ntoskrnl.exe and hal.dll because \SystemRoot
   * is created after their module entries
   */

  RtlInitUnicodeStringFromLiteral(&ModuleName, KERNEL_MODULE_NAME);
  ModuleObject = LdrGetModuleObject(&ModuleName);
  if (ModuleObject != NULL)
    {
      LdrpLoadModuleSymbols(&ModuleName, 
			    &ModuleObject->TextSection->SymbolInfo);
    }

  RtlInitUnicodeStringFromLiteral(&ModuleName, HAL_MODULE_NAME);
  ModuleObject = LdrGetModuleObject(&ModuleName);
  if (ModuleObject != NULL)
    {
      LdrpLoadModuleSymbols(&ModuleName,
			    &ModuleObject->TextSection->SymbolInfo);
    }
}
