/*
 * File pe_module.c - handle PE module information
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2007, Eric Pouech.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/******************************************************************
 *		pe_locate_with_coff_symbol_table
 *
 * Use the COFF symbol table (if any) from the IMAGE_FILE_HEADER to set the absolute address
 * of global symbols.
 * Mingw32 requires this for stabs debug information as address for global variables isn't filled in
 * (this is similar to what is done in elf_module.c when using the .symtab ELF section)
 */
static BOOL pe_locate_with_coff_symbol_table(struct module* module, IMAGE_NT_HEADERS* nth, void* mapping)
{
    const IMAGE_SYMBOL* isym;
    int                 i, numsym, naux;
    const char*         strtable;
    char                tmp[9];
    const char*         name;
    struct hash_table_iter      hti;
    void*               ptr;
    struct symt_data*   sym;
    const IMAGE_SECTION_HEADER* sect;

    numsym = nth->FileHeader.NumberOfSymbols;
    if (!nth->FileHeader.PointerToSymbolTable || !numsym)
        return TRUE;
    isym = (const IMAGE_SYMBOL*)((char*)mapping + nth->FileHeader.PointerToSymbolTable);
    /* FIXME: no way to get strtable size */
    strtable = (const char*)&isym[numsym];
    sect = IMAGE_FIRST_SECTION(nth);

    for (i = 0; i < numsym; i+= naux, isym += naux)
    {
        if (isym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            isym->SectionNumber > 0 && isym->SectionNumber <= nth->FileHeader.NumberOfSections)
        {
            if (isym->N.Name.Short)
            {
                name = memcpy(tmp, isym->N.ShortName, 8);
                tmp[8] = '\0';
            }
            else name = strtable + isym->N.Name.Long;
            if (name[0] == '_') name++;
            hash_table_iter_init(&module->ht_symbols, &hti, name);
            while ((ptr = hash_table_iter_up(&hti)))
            {
                sym = GET_ENTRY(ptr, struct symt_data, hash_elt);
                if (sym->symt.tag == SymTagData &&
                    (sym->kind == DataIsGlobal || sym->kind == DataIsFileStatic) &&
                    !strcmp(sym->hash_elt.name, name))
                {
                    TRACE("Changing absolute address for %d.%s: %lx -> %s\n",
                          isym->SectionNumber, name, sym->u.var.offset,
                          wine_dbgstr_longlong(module->module.BaseOfImage +
                                               sect[isym->SectionNumber - 1].VirtualAddress + isym->Value));
                    sym->u.var.offset = module->module.BaseOfImage +
                        sect[isym->SectionNumber - 1].VirtualAddress + isym->Value;
                    break;
                }
            }
        }
        naux = isym->NumberOfAuxSymbols + 1;
    }
    return TRUE;
}

/******************************************************************
 *		pe_load_coff_symbol_table
 *
 * Load public symbols out of the COFF symbol table (if any).
 */
static BOOL pe_load_coff_symbol_table(struct module* module, IMAGE_NT_HEADERS* nth, void* mapping)
{
    const IMAGE_SYMBOL* isym;
    int                 i, numsym, naux;
    const char*         strtable;
    char                tmp[9];
    const char*         name;
    const char*         lastfilename = NULL;
    struct symt_compiland*   compiland = NULL;
    const IMAGE_SECTION_HEADER* sect;

    numsym = nth->FileHeader.NumberOfSymbols;
    if (!nth->FileHeader.PointerToSymbolTable || !numsym)
        return TRUE;
    isym = (const IMAGE_SYMBOL*)((char*)mapping + nth->FileHeader.PointerToSymbolTable);
    /* FIXME: no way to get strtable size */
    strtable = (const char*)&isym[numsym];
    sect = IMAGE_FIRST_SECTION(nth);

    for (i = 0; i < numsym; i+= naux, isym += naux)
    {
        if (isym->StorageClass == IMAGE_SYM_CLASS_FILE)
        {
            lastfilename = (const char*)(isym + 1);
            compiland = NULL;
        }
        if (isym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            isym->SectionNumber > 0 && isym->SectionNumber <= nth->FileHeader.NumberOfSections)
        {
            if (isym->N.Name.Short)
            {
                name = memcpy(tmp, isym->N.ShortName, 8);
                tmp[8] = '\0';
            }
            else name = strtable + isym->N.Name.Long;
            if (name[0] == '_') name++;

            if (!compiland && lastfilename)
                compiland = symt_new_compiland(module, 0,
                                               source_new(module, NULL, lastfilename));
            symt_new_public(module, compiland, name,
                            module->module.BaseOfImage + sect[isym->SectionNumber - 1].VirtualAddress + isym->Value,
                            1);
        }
        naux = isym->NumberOfAuxSymbols + 1;
    }
    module->module.SymType = SymCoff;
    module->module.LineNumbers = FALSE;
    module->module.GlobalSymbols = FALSE;
    module->module.TypeInfo = FALSE;
    module->module.SourceIndexed = FALSE;
    module->module.Publics = TRUE;

    return TRUE;
}

static inline void* pe_get_sect(IMAGE_NT_HEADERS* nth, void* mapping,
                                IMAGE_SECTION_HEADER* sect)
{
    return (sect) ? RtlImageRvaToVa(nth, mapping, sect->VirtualAddress, NULL) : NULL;
}

static inline DWORD pe_get_sect_size(IMAGE_SECTION_HEADER* sect)
{
    return (sect) ? sect->SizeOfRawData : 0;
}

/******************************************************************
 *		pe_load_stabs
 *
 * look for stabs information in PE header (it's how the mingw compiler provides 
 * its debugging information)
 */
static BOOL pe_load_stabs(const struct process* pcs, struct module* module, 
                          void* mapping, IMAGE_NT_HEADERS* nth)
{
    IMAGE_SECTION_HEADER*       section;
    IMAGE_SECTION_HEADER*       sect_stabs = NULL;
    IMAGE_SECTION_HEADER*       sect_stabstr = NULL;
    int                         i;
    BOOL                        ret = FALSE;

    section = (IMAGE_SECTION_HEADER*)
        ((char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++)
    {
        if (!strcasecmp((const char*)section->Name, ".stab"))              sect_stabs = section;
        else if (!strncasecmp((const char*)section->Name, ".stabstr", 8))  sect_stabstr = section;
    }
    if (sect_stabs && sect_stabstr)
    {
        ret = stabs_parse(module,
                          module->module.BaseOfImage - nth->OptionalHeader.ImageBase,
                          pe_get_sect(nth, mapping, sect_stabs),   pe_get_sect_size(sect_stabs),
                          pe_get_sect(nth, mapping, sect_stabstr), pe_get_sect_size(sect_stabstr),
                          NULL, NULL);
        if (ret) pe_locate_with_coff_symbol_table(module, nth, mapping);
    }
    TRACE("%s the STABS debug info\n", ret ? "successfully loaded" : "failed to load");

    return ret;
}

/******************************************************************
 *		pe_load_dwarf
 *
 * look for dwarf information in PE header (it's also a way for the mingw compiler
 * to provide its debugging information)
 */
static BOOL pe_load_dwarf(const struct process* pcs, struct module* module,
                          void* mapping, IMAGE_NT_HEADERS* nth)
{
    IMAGE_SECTION_HEADER*       section;
    IMAGE_SECTION_HEADER*       sect_debuginfo = NULL;
    IMAGE_SECTION_HEADER*       sect_debugstr = NULL;
    IMAGE_SECTION_HEADER*       sect_debugabbrev = NULL;
    IMAGE_SECTION_HEADER*       sect_debugline = NULL;
    IMAGE_SECTION_HEADER*       sect_debugloc = NULL;
    int                         i;
    const char*                 strtable;
    const char*                 sectname;
    BOOL                        ret = FALSE;

    if (nth->FileHeader.PointerToSymbolTable && nth->FileHeader.NumberOfSymbols)
        /* FIXME: no way to get strtable size */
        strtable = (const char*)mapping + nth->FileHeader.PointerToSymbolTable +
             nth->FileHeader.NumberOfSymbols * sizeof(IMAGE_SYMBOL);
    else strtable = NULL;

    section = (IMAGE_SECTION_HEADER*)
        ((char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++)
    {
        sectname = (const char*)section->Name;
        /* long section names start with a '/' (at least on MinGW32) */
        if (*sectname == '/' && strtable)
            sectname = strtable + atoi(sectname + 1);
        if (!strcasecmp(sectname, ".debug_info"))        sect_debuginfo = section;
        else if (!strcasecmp(sectname, ".debug_str"))    sect_debugstr = section;
        else if (!strcasecmp(sectname, ".debug_abbrev")) sect_debugabbrev = section;
        else if (!strcasecmp(sectname, ".debug_line"))   sect_debugline = section;
        else if (!strcasecmp(sectname, ".debug_loc"))    sect_debugloc = section;
    }
    if (sect_debuginfo)
    {
        ret = dwarf2_parse(module,
                           module->module.BaseOfImage - nth->OptionalHeader.ImageBase,
                           NULL, /* FIXME: some thunks to deal with ? */
                           pe_get_sect(nth, mapping, sect_debuginfo),   pe_get_sect_size(sect_debuginfo),
                           pe_get_sect(nth, mapping, sect_debugabbrev), pe_get_sect_size(sect_debugabbrev),
                           pe_get_sect(nth, mapping, sect_debugstr),    pe_get_sect_size(sect_debugstr),
                           pe_get_sect(nth, mapping, sect_debugline),   pe_get_sect_size(sect_debugline),
                           pe_get_sect(nth, mapping, sect_debugloc),    pe_get_sect_size(sect_debugloc));
    }
    TRACE("%s the DWARF debug info\n", ret ? "successfully loaded" : "failed to load");

    return ret;
}

/******************************************************************
 *		pe_load_dbg_file
 *
 * loads a .dbg file
 */
static BOOL pe_load_dbg_file(const struct process* pcs, struct module* module,
                             const char* dbg_name, DWORD timestamp)
{
    char                                tmp[MAX_PATH];
    HANDLE                              hFile = INVALID_HANDLE_VALUE, hMap = 0;
    const BYTE*                         dbg_mapping = NULL;
    BOOL                                ret = FALSE;

    TRACE("Processing DBG file %s\n", debugstr_a(dbg_name));

    if (path_find_symbol_file(pcs, dbg_name, NULL, timestamp, 0, tmp, &module->module.DbgUnmatched) &&
        (hFile = CreateFileA(tmp, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE &&
        ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0) &&
        ((dbg_mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        const IMAGE_SEPARATE_DEBUG_HEADER*      hdr;
        const IMAGE_SECTION_HEADER*             sectp;
        const IMAGE_DEBUG_DIRECTORY*            dbg;

        hdr = (const IMAGE_SEPARATE_DEBUG_HEADER*)dbg_mapping;
        /* section headers come immediately after debug header */
        sectp = (const IMAGE_SECTION_HEADER*)(hdr + 1);
        /* and after that and the exported names comes the debug directory */
        dbg = (const IMAGE_DEBUG_DIRECTORY*)
            (dbg_mapping + sizeof(*hdr) +
             hdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
             hdr->ExportedNamesSize);

        ret = pe_load_debug_directory(pcs, module, dbg_mapping, sectp,
                                      hdr->NumberOfSections, dbg,
                                      hdr->DebugDirectorySize / sizeof(*dbg));
    }
    else
        ERR("Couldn't find .DBG file %s (%s)\n", debugstr_a(dbg_name), debugstr_a(tmp));

    if (dbg_mapping) UnmapViewOfFile(dbg_mapping);
    if (hMap) CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    return ret;
}

/******************************************************************
 *		pe_load_msc_debug_info
 *
 * Process MSC debug information in PE file.
 */
static BOOL pe_load_msc_debug_info(const struct process* pcs, 
                                   struct module* module,
                                   void* mapping, const IMAGE_NT_HEADERS* nth)
{
    BOOL                        ret = FALSE;
    const IMAGE_DATA_DIRECTORY* dir;
    const IMAGE_DEBUG_DIRECTORY*dbg = NULL;
    int                         nDbg;

    /* Read in debug directory */
    dir = nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;
    nDbg = dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (!nDbg) return FALSE;

    dbg = RtlImageRvaToVa(nth, mapping, dir->VirtualAddress, NULL);

    /* Parse debug directory */
    if (nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        /* Debug info is stripped to .DBG file */
        const IMAGE_DEBUG_MISC* misc = (const IMAGE_DEBUG_MISC*)
            ((const char*)mapping + dbg->PointerToRawData);

        if (nDbg != 1 || dbg->Type != IMAGE_DEBUG_TYPE_MISC ||
            misc->DataType != IMAGE_DEBUG_MISC_EXENAME)
        {
            WINE_ERR("-Debug info stripped, but no .DBG file in module %s\n",
                     debugstr_w(module->module.ModuleName));
        }
        else
        {
            ret = pe_load_dbg_file(pcs, module, (const char*)misc->Data, nth->FileHeader.TimeDateStamp);
        }
    }
    else
    {
        const IMAGE_SECTION_HEADER *sectp = (const IMAGE_SECTION_HEADER*)((const char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
        /* Debug info is embedded into PE module */
        ret = pe_load_debug_directory(pcs, module, mapping, sectp,
            nth->FileHeader.NumberOfSections, dbg, nDbg);
    }

    return ret;
}

/***********************************************************************
 *			pe_load_export_debug_info
 */
static BOOL pe_load_export_debug_info(const struct process* pcs, 
                                      struct module* module, 
                                      void* mapping, const IMAGE_NT_HEADERS* nth)
{
    unsigned int 		        i;
    const IMAGE_EXPORT_DIRECTORY* 	exports;
    DWORD			        base = module->module.BaseOfImage;
    DWORD                               size;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

#if 0
    /* Add start of DLL (better use the (yet unimplemented) Exe SymTag for this) */
    /* FIXME: module.ModuleName isn't correctly set yet if it's passed in SymLoadModule */
    symt_new_public(module, NULL, module->module.ModuleName, base, 1);
#endif
    
    /* Add entry point */
    symt_new_public(module, NULL, "EntryPoint", 
                    base + nth->OptionalHeader.AddressOfEntryPoint, 1);
#if 0
    /* FIXME: we'd better store addresses linked to sections rather than 
       absolute values */
    IMAGE_SECTION_HEADER*       section;
    /* Add start of sections */
    section = (IMAGE_SECTION_HEADER*)
        ((char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++) 
    {
	symt_new_public(module, NULL, section->Name, 
                        RtlImageRvaToVa(nth, mapping, section->VirtualAddress, NULL), 1);
    }
#endif

    /* Add exported functions */
    if ((exports = RtlImageDirectoryEntryToData(mapping, FALSE,
                                                IMAGE_DIRECTORY_ENTRY_EXPORT, &size)))
    {
        const WORD*             ordinals = NULL;
        const DWORD_PTR*	functions = NULL;
        const DWORD*		names = NULL;
        unsigned int		j;
        char			buffer[16];

        functions = RtlImageRvaToVa(nth, mapping, exports->AddressOfFunctions, NULL);
        ordinals  = RtlImageRvaToVa(nth, mapping, exports->AddressOfNameOrdinals, NULL);
        names     = RtlImageRvaToVa(nth, mapping, exports->AddressOfNames, NULL);

        if (functions && ordinals && names)
        {
            for (i = 0; i < exports->NumberOfNames; i++)
            {
                if (!names[i]) continue;
                symt_new_public(module, NULL,
                                RtlImageRvaToVa(nth, mapping, names[i], NULL),
                                base + functions[ordinals[i]], 1);
            }

            for (i = 0; i < exports->NumberOfFunctions; i++)
            {
                if (!functions[i]) continue;
                /* Check if we already added it with a name */
                for (j = 0; j < exports->NumberOfNames; j++)
                    if ((ordinals[j] == i) && names[j]) break;
                if (j < exports->NumberOfNames) continue;
                snprintf(buffer, sizeof(buffer), "%d", i + exports->Base);
                symt_new_public(module, NULL, buffer, base + (DWORD)functions[i], 1);
            }
        }
    }
    /* no real debug info, only entry points */
    if (module->module.SymType == SymDeferred)
        module->module.SymType = SymExport;
    return TRUE;
}

/******************************************************************
 *		pe_load_debug_info
 *
 */
BOOL pe_load_debug_info(const struct process* pcs, struct module* module)
{
    BOOL                ret = FALSE;
    HANDLE              hFile;
    HANDLE              hMap;
    void*               mapping;
    IMAGE_NT_HEADERS*   nth;

    hFile = CreateFileW(module->module.LoadedImageName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return ret;
    if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0)
    {
        if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
        {
            nth = RtlImageNtHeader(mapping);

            if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
            {
                ret = pe_load_stabs(pcs, module, mapping, nth) ||
                    pe_load_dwarf(pcs, module, mapping, nth) ||
                    pe_load_msc_debug_info(pcs, module, mapping, nth) ||
                    pe_load_coff_symbol_table(module, nth, mapping);
                /* if we still have no debug info (we could only get SymExport at this
                 * point), then do the SymExport except if we have an ELF container, 
                 * in which case we'll rely on the export's on the ELF side
                 */
            }
/* FIXME shouldn't we check that? if (!module_get_debug(pcs, module))l */
            if (pe_load_export_debug_info(pcs, module, mapping, nth) && !ret)
                ret = TRUE;
            UnmapViewOfFile(mapping);
        }
        CloseHandle(hMap);
    }
    CloseHandle(hFile);

    return ret;
}

/******************************************************************
 *		pe_load_native_module
 *
 */
struct module* pe_load_native_module(struct process* pcs, const WCHAR* name,
                                     HANDLE hFile, DWORD base, DWORD size)
{
    struct module*      module = NULL;
    BOOL                opened = FALSE;
    HANDLE              hMap;
    WCHAR               loaded_name[MAX_PATH];

    loaded_name[0] = '\0';
    if (!hFile)
    {

        assert(name);

        if ((hFile = FindExecutableImageExW(name, pcs->search_path, loaded_name, NULL, NULL)) == NULL)
            return NULL;
        opened = TRUE;
    }
    else if (name) strcpyW(loaded_name, name);
    else if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
        FIXME("Trouble ahead (no module name passed in deferred mode)\n");

    if ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
    {
        void*   mapping;

        if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
        {
            IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(mapping);

            if (nth)
            {
                if (!base) base = nth->OptionalHeader.ImageBase;
                if (!size) size = nth->OptionalHeader.SizeOfImage;

                module = module_new(pcs, loaded_name, DMT_PE, FALSE, base, size,
                                    nth->FileHeader.TimeDateStamp,
                                    nth->OptionalHeader.CheckSum);
                if (module)
                {
                    if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
                        module->module.SymType = SymDeferred;
                    else
                        pe_load_debug_info(pcs, module);
                }
                else
                    ERR("could not load the module '%s'\n", debugstr_w(loaded_name));
            }
            UnmapViewOfFile(mapping);
        }
        CloseHandle(hMap);
    }
    if (opened) CloseHandle(hFile);

    return module;
}

/******************************************************************
 *		pe_load_nt_header
 *
 */
BOOL pe_load_nt_header(HANDLE hProc, DWORD64 base, IMAGE_NT_HEADERS* nth)
{
    IMAGE_DOS_HEADER    dos;

    return ReadProcessMemory(hProc, (char*)(DWORD_PTR)base, &dos, sizeof(dos), NULL) &&
        dos.e_magic == IMAGE_DOS_SIGNATURE &&
        ReadProcessMemory(hProc, (char*)(DWORD_PTR)(base + dos.e_lfanew),
                          nth, sizeof(*nth), NULL) &&
        nth->Signature == IMAGE_NT_SIGNATURE;
}

/******************************************************************
 *		pe_load_builtin_module
 *
 */
struct module* pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                      DWORD64 base, DWORD64 size)
{
    struct module*      module = NULL;

    if (base && pcs->dbg_hdr_addr)
    {
        IMAGE_NT_HEADERS    nth;

        if (pe_load_nt_header(pcs->handle, base, &nth))
        {
            if (!size) size = nth.OptionalHeader.SizeOfImage;
            module = module_new(pcs, name, DMT_PE, FALSE, base, size,
                                nth.FileHeader.TimeDateStamp,
                                nth.OptionalHeader.CheckSum);
        }
    }
    return module;
}

/***********************************************************************
 *           ImageDirectoryEntryToDataEx (DBGHELP.@)
 *
 * Search for specified directory in PE image
 *
 * PARAMS
 *
 *   base    [in]  Image base address
 *   image   [in]  TRUE - image has been loaded by loader, FALSE - raw file image
 *   dir     [in]  Target directory index
 *   size    [out] Receives directory size
 *   section [out] Receives pointer to section header of section containing directory data
 *
 * RETURNS
 *   Success: pointer to directory data
 *   Failure: NULL
 *
 */
PVOID WINAPI ImageDirectoryEntryToDataEx( PVOID base, BOOLEAN image, USHORT dir, PULONG size, PIMAGE_SECTION_HEADER *section )
{
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    *size = 0;
    if (section) *section = NULL;

    if (!(nt = RtlImageNtHeader( base ))) return NULL;
    if (dir >= nt->OptionalHeader.NumberOfRvaAndSizes) return NULL;
    if (!(addr = nt->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;

    *size = nt->OptionalHeader.DataDirectory[dir].Size;
    if (image || addr < nt->OptionalHeader.SizeOfHeaders) return (char *)base + addr;

    return RtlImageRvaToVa( nt, base, addr, section );
}

/***********************************************************************
 *         ImageDirectoryEntryToData   (DBGHELP.@)
 *
 * NOTES
 *   See ImageDirectoryEntryToDataEx
 */
PVOID WINAPI ImageDirectoryEntryToData( PVOID base, BOOLEAN image, USHORT dir, PULONG size )
{
    return ImageDirectoryEntryToDataEx( base, image, dir, size, NULL );
}
