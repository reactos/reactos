
#ifndef __MODULE_H
#define __MODULE_H

#include <ddk/ntddk.h>
#include <pe.h>

typedef struct _MODULE_OBJECT
{
  CSHORT  ObjectType;
  CSHORT  ObjectSize;
  PVOID  Base;
  ULONG Length;
  ULONG Flags;
  PVOID  EntryPoint;
  LIST_ENTRY ListEntry;
  UNICODE_STRING Name;
  union
    {
      struct
        {
          PIMAGE_FILE_HEADER FileHeader;
          PIMAGE_OPTIONAL_HEADER OptionalHeader;
          PIMAGE_SECTION_HEADER SectionList;
        } PE;
    } Image;
} MODULE_OBJECT, *PMODULE_OBJECT;

typedef MODULE_OBJECT MODULE, *PMODULE;

#define MODULE_FLAG_BIN  0x0001
#define MODULE_FLAG_MZ   0x0002
#define MODULE_FLAG_NE   0x0004
#define MODULE_FLAG_PE   0x0008
#define MODULE_FLAG_COFF 0x0010

typedef struct _INSTANCE
{
  HANDLE ModuleHandle;
} INSTANCE, *PINSTANCE;

BOOLEAN process_boot_module(unsigned int start);

#endif

