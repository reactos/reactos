/*
 * File pe_module.c - handle PE module information
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004,      Eric Pouech.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "winreg.h"
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
                          const void* mapping, IMAGE_NT_HEADERS* nth)
{
    IMAGE_SECTION_HEADER*       section;
    int                         i, stabsize = 0, stabstrsize = 0;
    unsigned int                stabs = 0, stabstr = 0;
    BOOL                        ret = FALSE;

    section = (IMAGE_SECTION_HEADER*)
        ((char*)&nth->OptionalHeader + nth->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++)
    {
        if (!strcasecmp(section->Name, ".stab"))
        {
            stabs = section->VirtualAddress;
            stabsize = section->SizeOfRawData;
        }
        else if (!strncasecmp(section->Name, ".stabstr", 8))
        {
            stabstr = section->VirtualAddress;
            stabstrsize = section->SizeOfRawData;
        }
    }

    if (stabstrsize && stabsize)
    {
        ret = stabs_parse(module, 
                          module->module.BaseOfImage - nth->OptionalHeader.ImageBase, 
                          RtlImageRvaToVa(nth, (void*)mapping, stabs, NULL),
                          stabsize,
                          RtlImageRvaToVa(nth, (void*)mapping, stabstr, NULL),
                          stabstrsize);
    }
    return ret;
}

static BOOL CALLBACK dbg_match(char* file, void* user)
{
    /* accept first file */
    return FALSE;
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
    const IMAGE_SEPARATE_DEBUG_HEADER*  hdr;
    const IMAGE_DEBUG_DIRECTORY*        dbg;
    BOOL                                ret = FALSE;

    WINE_TRACE("Processing DBG file %s\n", dbg_name);

    if (SymFindFileInPath(pcs->handle, NULL, (char*)dbg_name, 
                          NULL, 0, 0, 0,
                          tmp, dbg_match, NULL) &&
        (hFile = CreateFileA(tmp, GENERIC_READ, FILE_SHARE_READ, NULL, 
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE &&
        ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0) &&
        ((dbg_mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        hdr = (const IMAGE_SEPARATE_DEBUG_HEADER*)dbg_mapping;
        if (hdr->TimeDateStamp != timestamp)
        {
            WINE_ERR("Warning - %s has incorrect internal timestamp\n",
                     dbg_name);
            /*
             * Well, sometimes this happens to DBG files which ARE REALLY the
             * right .DBG files but nonetheless this check fails. Anyway,
             * WINDBG (debugger for Windows by Microsoft) loads debug symbols
             * which have incorrect timestamps.
             */
        }
        if (hdr->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE)
        {
            /* section headers come immediately after debug header */
            const IMAGE_SECTION_HEADER *sectp =
                (const IMAGE_SECTION_HEADER*)(hdr + 1);
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
            ERR("Wrong signature in .DBG file %s\n", debugstr_a(tmp));
    }
    else
        WINE_ERR("-Unable to peruse .DBG file %s (%s)\n", dbg_name, debugstr_a(tmp));

    if (dbg_mapping) UnmapViewOfFile((void*)dbg_mapping);
    if (hMap) CloseHandle(hMap);
    if (hFile != NULL) CloseHandle(hFile);
    return ret;
}

/******************************************************************
 *		pe_load_msc_debug_info
 *
 * Process MSC debug information in PE file.
 */
static BOOL pe_load_msc_debug_info(const struct process* pcs, 
                                   struct module* module,
                                   const void* mapping, IMAGE_NT_HEADERS* nth)
{
    BOOL                        ret = FALSE;
    const IMAGE_DATA_DIRECTORY* dir;
    const IMAGE_DEBUG_DIRECTORY*dbg = NULL;
    int                         nDbg;

    /* Read in debug directory */
    dir = nth->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_DEBUG;
    nDbg = dir->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (!nDbg) return FALSE;

    dbg = RtlImageRvaToVa(nth, (void*)mapping, dir->VirtualAddress, NULL);

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
                     module->module.ModuleName);
        }
        else
        {
            ret = pe_load_dbg_file(pcs, module, misc->Data, nth->FileHeader.TimeDateStamp);
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
                                      const void* mapping, IMAGE_NT_HEADERS* nth)
{
    unsigned int 		        i;
    const IMAGE_EXPORT_DIRECTORY* 	exports;
    DWORD			        base = module->module.BaseOfImage;
    DWORD                               size;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

#if 0
    /* Add start of DLL (better use the (yet unimplemented) Exe SymTag for this) */
    /* FIXME: module.ModuleName isn't correctly set yet if it's passed in SymLoadModule */
    symt_new_public(module, NULL, module->module.ModuleName, base, 0,
                    TRUE /* FIXME */, TRUE /* FIXME */);
#endif
    
    /* Add entry point */
    symt_new_public(module, NULL, "EntryPoint", 
                    base + nth->OptionalHeader.AddressOfEntryPoint, 0,
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
                        RtlImageRvaToVa(nth, (void*)mapping, section->VirtualAddress, NULL), 
                        0, TRUE /* FIXME */, TRUE /* FIXME */);
    }
#endif

    /* Add exported functions */
    if ((exports = RtlImageDirectoryEntryToData((void*)mapping, FALSE,
                                                IMAGE_DIRECTORY_ENTRY_EXPORT, &size)))
    {
        const WORD*             ordinals = NULL;
        const DWORD_PTR*	functions = NULL;
        const DWORD*		names = NULL;
        unsigned int		j;
        char			buffer[16];

        functions = RtlImageRvaToVa(nth, (void*)mapping, exports->AddressOfFunctions, NULL);
        ordinals  = RtlImageRvaToVa(nth, (void*)mapping, exports->AddressOfNameOrdinals, NULL);
        names     = RtlImageRvaToVa(nth, (void*)mapping, exports->AddressOfNames, NULL);

        for (i = 0; i < exports->NumberOfNames; i++) 
        {
            if (!names[i]) continue;
            symt_new_public(module, NULL, 
                            RtlImageRvaToVa(nth, (void*)mapping, names[i], NULL),
                            base + functions[ordinals[i]], 
                            0, TRUE /* FIXME */, TRUE /* FIXME */);
        }
	    
        for (i = 0; i < exports->NumberOfFunctions; i++) 
        {
            if (!functions[i]) continue;
            /* Check if we already added it with a name */
            for (j = 0; j < exports->NumberOfNames; j++)
                if ((ordinals[j] == i) && names[j]) break;
            if (j < exports->NumberOfNames) continue;
            snprintf(buffer, sizeof(buffer), "%ld", i + exports->Base);
            symt_new_public(module, NULL, buffer, base + (DWORD)functions[i], 0,
                            TRUE /* FIXME */, TRUE /* FIXME */);
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

    hFile = CreateFileA(module->module.LoadedImageName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return ret;
    if ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0)
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
 *		pe_load_module
 *
 */
struct module* pe_load_module(struct process* pcs, char* name,
                              HANDLE hFile, DWORD base, DWORD size)
{
    struct module*      module = NULL;
    BOOL                opened = FALSE;
    HANDLE              hMap;
    void*               mapping;
    char                loaded_name[MAX_PATH];

    loaded_name[0] = '\0';
    if (!hFile)
    {
        if (!name)
        {
            /* FIXME SetLastError */
            return NULL;
        }
        if ((hFile = FindExecutableImage(name, NULL, loaded_name)) == NULL)
            return NULL;
        opened = TRUE;
    }
    else if (name) strcpy(loaded_name, name);
    else if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
        FIXME("Trouble ahead (no module name passed in deferred mode)\n");
    if (!(module = module_find_by_name(pcs, loaded_name, DMT_PE)) &&
        (hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
    {
        if ((mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
        {
            IMAGE_NT_HEADERS*   nth = RtlImageNtHeader(mapping);

            if (nth)
            {
                if (!base) base = nth->OptionalHeader.ImageBase;
                if (!size) size = nth->OptionalHeader.SizeOfImage;
            
                module = module_new(pcs, loaded_name, DMT_PE, base, size,
                                    nth->FileHeader.TimeDateStamp, 
                                    nth->OptionalHeader.CheckSum);
                if (module)
                {
                    if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
                        module->module.SymType = SymDeferred;
                    else
                        pe_load_debug_info(pcs, module);
                }
            }
            UnmapViewOfFile(mapping);
        }
        CloseHandle(hMap);
    }
    if (opened) CloseHandle(hFile);

    return module;
}

/******************************************************************
 *		pe_load_module_from_pcs
 *
 */
struct module* pe_load_module_from_pcs(struct process* pcs, const char* name, 
                                       const char* mod_name, DWORD base, DWORD size)
{
    struct module*      module;
    const char*         ptr;

    if ((module = module_find_by_name(pcs, name, DMT_PE))) return module;
    if (mod_name) ptr = mod_name;
    else
    {
        for (ptr = name + strlen(name) - 1; ptr >= name; ptr--)
        {
            if (*ptr == '/' || *ptr == '\\')
            {
                ptr++;
                break;
            }
        }
    }
    if (ptr && (module = module_find_by_name(pcs, ptr, DMT_PE))) return module;
    if (base && pcs->dbg_hdr_addr)
    {
        IMAGE_DOS_HEADER    dos;
        IMAGE_NT_HEADERS    nth;

        if (ReadProcessMemory(pcs->handle, (char*)base, &dos, sizeof(dos), NULL) &&
            dos.e_magic == IMAGE_DOS_SIGNATURE &&
            ReadProcessMemory(pcs->handle, (char*)(base + dos.e_lfanew), 
                              &nth, sizeof(nth), NULL) &&
            nth.Signature == IMAGE_NT_SIGNATURE)
        {
            if (!size) size = nth.OptionalHeader.SizeOfImage;
            module = module_new(pcs, name, DMT_PE, base, size,
                                nth.FileHeader.TimeDateStamp, nth.OptionalHeader.CheckSum);
        }
    }
    return module;
}
