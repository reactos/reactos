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
 *		pe_load_stabs
 *
 * look for stabs information in PE header (it's how the mingw compiler provides 
 * its debugging information)
 */
static BOOL pe_load_stabs(const struct process* pcs, struct module* module, 
                          void* mapping, IMAGE_NT_HEADERS* nth)
{
    IMAGE_SECTION_HEADER*       section;
    int                         i, stabsize = 0, stabstrsize = 0;
    unsigned int                stabs = 0, stabstr = 0;
    BOOL                        ret = FALSE;

    section = (IMAGE_SECTION_HEADER*)
        ((char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++)
    {
        if (!strcasecmp((const char*)section->Name, ".stab"))
        {
            stabs = section->VirtualAddress;
            stabsize = section->SizeOfRawData;
        }
        else if (!strncasecmp((const char*)section->Name, ".stabstr", 8))
        {
            stabstr = section->VirtualAddress;
            stabstrsize = section->SizeOfRawData;
        }
    }

    if (stabstrsize && stabsize)
    {
        ret = stabs_parse(module, 
                          module->module.BaseOfImage - nth->OptionalHeader.ImageBase, 
                          RtlImageRvaToVa(nth, mapping, stabs, NULL),
                          stabsize,
                          RtlImageRvaToVa(nth, mapping, stabstr, NULL),
                          stabstrsize);
    }

    TRACE("%s the STABS debug info\n", ret ? "successfully loaded" : "failed to load");
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
                                   void* mapping, IMAGE_NT_HEADERS* nth)
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
                                      void* mapping, IMAGE_NT_HEADERS* nth)
{
    unsigned int 		        i;
    const IMAGE_EXPORT_DIRECTORY* 	exports;
    DWORD			        base = module->module.BaseOfImage;
    DWORD                               size;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

#if 0
    /* Add start of DLL (better use the (yet unimplemented) Exe SymTag for this) */
    /* FIXME: module.ModuleName isn't correctly set yet if it's passed in SymLoadModule */
    symt_new_public(module, NULL, module->module.ModuleName, base, 1,
                    TRUE /* FIXME */, TRUE /* FIXME */);
#endif
    
    /* Add entry point */
    symt_new_public(module, NULL, "EntryPoint", 
                    base + nth->OptionalHeader.AddressOfEntryPoint, 1,
                    TRUE, TRUE);
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
                        RtlImageRvaToVa(nth, mapping, section->VirtualAddress, NULL), 
                        1, TRUE /* FIXME */, TRUE /* FIXME */);
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
                                base + functions[ordinals[i]],
                                1, TRUE /* FIXME */, TRUE /* FIXME */);
            }

            for (i = 0; i < exports->NumberOfFunctions; i++)
            {
                if (!functions[i]) continue;
                /* Check if we already added it with a name */
                for (j = 0; j < exports->NumberOfNames; j++)
                    if ((ordinals[j] == i) && names[j]) break;
                if (j < exports->NumberOfNames) continue;
                snprintf(buffer, sizeof(buffer), "%d", i + exports->Base);
                symt_new_public(module, NULL, buffer, base + (DWORD)functions[i], 1,
                                TRUE /* FIXME */, TRUE /* FIXME */);
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
                    pe_load_msc_debug_info(pcs, module, mapping, nth);
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
BOOL pe_load_nt_header(HANDLE hProc, DWORD base, IMAGE_NT_HEADERS* nth)
{
    IMAGE_DOS_HEADER    dos;

    return ReadProcessMemory(hProc, (char*)base, &dos, sizeof(dos), NULL) &&
        dos.e_magic == IMAGE_DOS_SIGNATURE &&
        ReadProcessMemory(hProc, (char*)(base + dos.e_lfanew), 
                          nth, sizeof(*nth), NULL) &&
        nth->Signature == IMAGE_NT_SIGNATURE;
}

/******************************************************************
 *		pe_load_builtin_module
 *
 */
struct module* pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                      DWORD base, DWORD size)
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

    return RtlImageRvaToVa( nt, (HMODULE)base, addr, section );
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
