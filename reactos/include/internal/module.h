
#ifndef __MODULE_H
#define __MODULE_H

#include <ddk/ntddk.h>
#include <coff.h>
#include <pe.h>

/* FIXME: replace this struct with struct below in all code  */
typedef struct _module
{
   unsigned int nsyms;
        unsigned int text_base;
        unsigned int data_base;
        unsigned int bss_base;
        SCNHDR* scn_list;
        char* str_tab;
        SYMENT* sym_list;
        unsigned int size;

        /*
         * Base address of the module in memory
         */
        unsigned int base;

        /*
         * Offset of the raw data in memory
         */
        unsigned int raw_data_off;
} module;

typedef SCNHDR COFF_SECTION_HEADER, *PCOFF_SECTION_HEADER;

typedef struct _MODULE_OBJECT
{
  CSHORT  ObjectType;
  CSHORT  ObjectSize;
  PVOID  Base;
  unsigned int  Flags;
  PVOID  EntryPoint;
  union
    { 
      struct
        {
          unsigned int NumberOfSyms;
          PVOID TextBase;
          PVOID DataBase;
          PVOID BSSBase;
          SCNHDR *SectionList;
          char *StringTable;
          SYMENT *SymbolList;
        } COFF;
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

