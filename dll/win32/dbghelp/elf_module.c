/*
 * File elf.c - processing of ELF files
 *
 * Copyright (C) 1996, Eric Youngdale.
 *		 1999-2007 Eric Pouech
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
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dbghelp_private.h"
#include "image_private.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/heap.h"

#define ELF_INFO_DEBUG_HEADER   0x0001
#define ELF_INFO_MODULE         0x0002
#define ELF_INFO_NAME           0x0004

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

struct elf_info
{
    unsigned                    flags;          /* IN  one (or several) of the ELF_INFO constants */
    DWORD_PTR                   dbg_hdr_addr;   /* OUT address of debug header (if ELF_INFO_DEBUG_HEADER is set) */
    struct module*              module;         /* OUT loaded module (if ELF_INFO_MODULE is set) */
    const WCHAR*                module_name;    /* OUT found module name (if ELF_INFO_NAME is set) */
};

struct elf_sym32
{
    UINT32                      st_name;   /* Symbol name (string tbl index) */
    UINT32                      st_value;  /* Symbol value */
    UINT32                      st_size;   /* Symbol size */
    UINT8                       st_info;   /* Symbol type and binding */
    UINT8                       st_other;  /* Symbol visibility */
    UINT16                      st_shndx;  /* Section index */
};

struct elf_sym
{
    UINT32                      st_name;   /* Symbol name (string tbl index) */
    UINT8                       st_info;   /* Symbol type and binding */
    UINT8                       st_other;  /* Symbol visibility */
    UINT16                      st_shndx;  /* Section index */
    UINT64                      st_value;  /* Symbol value */
    UINT64                      st_size;   /* Symbol size */
};

struct symtab_elt
{
    struct hash_table_elt       ht_elt;
    struct elf_sym              sym;
    struct symt_compiland*      compiland;
    unsigned                    used;
};

struct elf_thunk_area
{
    const char*                 symname;
    THUNK_ORDINAL               ordinal;
    ULONG_PTR                   rva_start;
    ULONG_PTR                   rva_end;
};

struct elf_module_info
{
    ULONG_PTR                   elf_addr;
    unsigned short	        elf_mark : 1,
                                elf_loader : 1;
    struct image_file_map       file_map;
};

/* Legal values for sh_type (section type).  */
#define ELF_SHT_NULL            0    /* Section header table entry unused */
#define ELF_SHT_PROGBITS        1    /* Program data */
#define ELF_SHT_SYMTAB          2    /* Symbol table */
#define ELF_SHT_STRTAB          3    /* String table */
#define ELF_SHT_RELA            4    /* Relocation entries with addends */
#define ELF_SHT_HASH            5    /* Symbol hash table */
#define ELF_SHT_DYNAMIC         6    /* Dynamic linking information */
#define ELF_SHT_NOTE            7    /* Notes */
#define ELF_SHT_NOBITS          8    /* Program space with no data (bss) */
#define ELF_SHT_REL             9    /* Relocation entries, no addends */
#define ELF_SHT_SHLIB          10    /* Reserved */
#define ELF_SHT_DYNSYM         11    /* Dynamic linker symbol table */
#define ELF_SHT_INIT_ARRAY     14    /* Array of constructors */
#define ELF_SHT_FINI_ARRAY     15    /* Array of destructors */
#define ELF_SHT_PREINIT_ARRAY  16    /* Array of pre-constructors */
#define ELF_SHT_GROUP          17    /* Section group */
#define ELF_SHT_SYMTAB_SHNDX   18    /* Extended section indices */
#define ELF_SHT_NUM            19    /* Number of defined types.  */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */
#define ELF_STT_NOTYPE          0    /* Symbol type is unspecified */
#define ELF_STT_OBJECT          1    /* Symbol is a data object */
#define ELF_STT_FUNC            2    /* Symbol is a code object */
#define ELF_STT_SECTION         3    /* Symbol associated with a section */
#define ELF_STT_FILE            4    /* Symbol's name is file name */

#define ELF_PT_LOAD             1    /* Loadable program segment */

#define ELF_DT_DEBUG           21    /* For debugging; unspecified */

#define ELF_AT_SYSINFO_EHDR    33

/******************************************************************
 *		elf_map_section
 *
 * Maps a single section into memory from an ELF file
 */
static const char* elf_map_section(struct image_section_map* ism)
{
    struct elf_file_map*        fmap = &ism->fmap->u.elf;
    SIZE_T ofst, size;
    HANDLE mapping;

    assert(ism->fmap->modtype == DMT_ELF);
    if (ism->sidx < 0 || ism->sidx >= ism->fmap->u.elf.elfhdr.e_shnum ||
        fmap->sect[ism->sidx].shdr.sh_type == ELF_SHT_NOBITS)
        return IMAGE_NO_MAP;

    if (fmap->target_copy)
    {
        return fmap->target_copy + fmap->sect[ism->sidx].shdr.sh_offset;
    }

    /* align required information on allocation granularity */
    ofst = fmap->sect[ism->sidx].shdr.sh_offset & ~(sysinfo.dwAllocationGranularity - 1);
    size = fmap->sect[ism->sidx].shdr.sh_offset + fmap->sect[ism->sidx].shdr.sh_size - ofst;
    if (!(mapping = CreateFileMappingW(fmap->handle, NULL, PAGE_READONLY, 0, ofst + size, NULL)))
    {
        ERR("map creation %p failed %u offset %lu %lu size %lu\n", fmap->handle, GetLastError(), ofst, ofst % 4096, size);
        return IMAGE_NO_MAP;
    }
    fmap->sect[ism->sidx].mapped = MapViewOfFile(mapping, FILE_MAP_READ, 0, ofst, size);
    CloseHandle(mapping);
    if (!fmap->sect[ism->sidx].mapped)
    {
        ERR("map %p failed %u offset %lu %lu size %lu\n", fmap->handle, GetLastError(), ofst, ofst % 4096, size);
        return IMAGE_NO_MAP;
    }
    return fmap->sect[ism->sidx].mapped + (fmap->sect[ism->sidx].shdr.sh_offset & (sysinfo.dwAllocationGranularity - 1));
}

/******************************************************************
 *		elf_find_section
 *
 * Finds a section by name (and type) into memory from an ELF file
 * or its alternate if any
 */
static BOOL elf_find_section(struct image_file_map* _fmap, const char* name, struct image_section_map* ism)
{
    struct elf_file_map*        fmap = &_fmap->u.elf;
    unsigned i;

    if (fmap->shstrtab == IMAGE_NO_MAP)
    {
        struct image_section_map  hdr_ism = {_fmap, fmap->elfhdr.e_shstrndx};
        if ((fmap->shstrtab = elf_map_section(&hdr_ism)) == IMAGE_NO_MAP) return FALSE;
    }
    for (i = 0; i < fmap->elfhdr.e_shnum; i++)
    {
        if (strcmp(fmap->shstrtab + fmap->sect[i].shdr.sh_name, name) == 0)
        {
            ism->fmap = _fmap;
            ism->sidx = i;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL elf_find_section_type(struct image_file_map* _fmap, const char* name, unsigned sht, struct image_section_map* ism)
{
    struct elf_file_map*        fmap;
    unsigned i;

    while (_fmap)
    {
        if (_fmap->modtype != DMT_ELF) break;
        fmap = &_fmap->u.elf;
        if (fmap->shstrtab == IMAGE_NO_MAP)
        {
            struct image_section_map  hdr_ism = {_fmap, fmap->elfhdr.e_shstrndx};
            if ((fmap->shstrtab = elf_map_section(&hdr_ism)) == IMAGE_NO_MAP) break;
        }
        for (i = 0; i < fmap->elfhdr.e_shnum; i++)
        {
            if (strcmp(fmap->shstrtab + fmap->sect[i].shdr.sh_name, name) == 0 && sht == fmap->sect[i].shdr.sh_type)
            {
                ism->fmap = _fmap;
                ism->sidx = i;
                return TRUE;
            }
        }
        _fmap = _fmap->alternate;
    }
    ism->fmap = NULL;
    ism->sidx = -1;
    return FALSE;
}

/******************************************************************
 *		elf_unmap_section
 *
 * Unmaps a single section from memory
 */
static void elf_unmap_section(struct image_section_map* ism)
{
    struct elf_file_map*        fmap = &ism->fmap->u.elf;

    if (ism->sidx >= 0 && ism->sidx < fmap->elfhdr.e_shnum && !fmap->target_copy &&
        fmap->sect[ism->sidx].mapped)
    {
        if (!UnmapViewOfFile(fmap->sect[ism->sidx].mapped))
            WARN("Couldn't unmap the section\n");
        fmap->sect[ism->sidx].mapped = NULL;
    }
}

static void elf_end_find(struct image_file_map* fmap)
{
    struct image_section_map      ism;

    while (fmap && fmap->modtype == DMT_ELF)
    {
        ism.fmap = fmap;
        ism.sidx = fmap->u.elf.elfhdr.e_shstrndx;
        elf_unmap_section(&ism);
        fmap->u.elf.shstrtab = IMAGE_NO_MAP;
        fmap = fmap->alternate;
    }
}

/******************************************************************
 *		elf_get_map_rva
 *
 * Get the RVA of an ELF section
 */
static DWORD_PTR elf_get_map_rva(const struct image_section_map* ism)
{
    if (ism->sidx < 0 || ism->sidx >= ism->fmap->u.elf.elfhdr.e_shnum)
        return 0;
    return ism->fmap->u.elf.sect[ism->sidx].shdr.sh_addr - ism->fmap->u.elf.elf_start;
}

/******************************************************************
 *		elf_get_map_size
 *
 * Get the size of an ELF section
 */
static unsigned elf_get_map_size(const struct image_section_map* ism)
{
    if (ism->sidx < 0 || ism->sidx >= ism->fmap->u.elf.elfhdr.e_shnum)
        return 0;
    return ism->fmap->u.elf.sect[ism->sidx].shdr.sh_size;
}

/******************************************************************
 *		elf_unmap_file
 *
 * Unmaps an ELF file from memory (previously mapped with elf_map_file)
 */
static void elf_unmap_file(struct image_file_map* fmap)
{
    if (fmap->u.elf.handle != INVALID_HANDLE_VALUE)
    {
        struct image_section_map  ism;
        ism.fmap = fmap;
        for (ism.sidx = 0; ism.sidx < fmap->u.elf.elfhdr.e_shnum; ism.sidx++)
        {
            elf_unmap_section(&ism);
        }
        HeapFree(GetProcessHeap(), 0, fmap->u.elf.sect);
        CloseHandle(fmap->u.elf.handle);
    }
    HeapFree(GetProcessHeap(), 0, fmap->u.elf.target_copy);
}

static const struct image_file_map_ops elf_file_map_ops =
{
    elf_map_section,
    elf_unmap_section,
    elf_find_section,
    elf_get_map_rva,
    elf_get_map_size,
    elf_unmap_file,
};

static inline void elf_reset_file_map(struct image_file_map* fmap)
{
    fmap->ops = &elf_file_map_ops;
    fmap->alternate = NULL;
    fmap->u.elf.handle = INVALID_HANDLE_VALUE;
    fmap->u.elf.shstrtab = IMAGE_NO_MAP;
    fmap->u.elf.target_copy = NULL;
}

struct elf_map_file_data
{
    enum {from_file, from_process, from_handle}      kind;
    union
    {
        struct
        {
            const WCHAR* filename;
        } file;
        struct
        {
            HANDLE      handle;
            void*       load_addr;
        } process;
        HANDLE handle;
    } u;
};

static BOOL elf_map_file_read(struct image_file_map* fmap, struct elf_map_file_data* emfd,
                              void* buf, size_t len, size_t off)
{
    LARGE_INTEGER li;
    DWORD bytes_read;
    SIZE_T dw;

    switch (emfd->kind)
    {
    case from_file:
    case from_handle:
        li.QuadPart = off;
        if (!SetFilePointerEx(fmap->u.elf.handle, li, NULL, FILE_BEGIN)) return FALSE;
        return ReadFile(fmap->u.elf.handle, buf, len, &bytes_read, NULL);
    case from_process:
        return ReadProcessMemory(emfd->u.process.handle,
                                 (void*)((ULONG_PTR)emfd->u.process.load_addr + (ULONG_PTR)off),
                                 buf, len, &dw) && dw == len;
    default:
        assert(0);
        return FALSE;
    }
}

static BOOL elf_map_shdr(struct elf_map_file_data* emfd, struct image_file_map* fmap, unsigned int i)
{
    if (fmap->addr_size == 32)
    {
        struct
        {
            UINT32  sh_name;       /* Section name (string tbl index) */
            UINT32  sh_type;       /* Section type */
            UINT32  sh_flags;      /* Section flags */
            UINT32  sh_addr;       /* Section virtual addr at execution */
            UINT32  sh_offset;     /* Section file offset */
            UINT32  sh_size;       /* Section size in bytes */
            UINT32  sh_link;       /* Link to another section */
            UINT32  sh_info;       /* Additional section information */
            UINT32  sh_addralign;  /* Section alignment */
            UINT32  sh_entsize;    /* Entry size if section holds table */
        } shdr32;

        if (!elf_map_file_read(fmap, emfd, &shdr32, sizeof(shdr32),
                               fmap->u.elf.elfhdr.e_shoff + i * sizeof(shdr32)))
            return FALSE;

        fmap->u.elf.sect[i].shdr.sh_name      = shdr32.sh_name;
        fmap->u.elf.sect[i].shdr.sh_type      = shdr32.sh_type;
        fmap->u.elf.sect[i].shdr.sh_flags     = shdr32.sh_flags;
        fmap->u.elf.sect[i].shdr.sh_addr      = shdr32.sh_addr;
        fmap->u.elf.sect[i].shdr.sh_offset    = shdr32.sh_offset;
        fmap->u.elf.sect[i].shdr.sh_size      = shdr32.sh_size;
        fmap->u.elf.sect[i].shdr.sh_link      = shdr32.sh_link;
        fmap->u.elf.sect[i].shdr.sh_info      = shdr32.sh_info;
        fmap->u.elf.sect[i].shdr.sh_addralign = shdr32.sh_addralign;
        fmap->u.elf.sect[i].shdr.sh_entsize   = shdr32.sh_entsize;
    }
    else
    {
        if (!elf_map_file_read(fmap, emfd, &fmap->u.elf.sect[i].shdr, sizeof(fmap->u.elf.sect[i].shdr),
                               fmap->u.elf.elfhdr.e_shoff + i * sizeof(fmap->u.elf.sect[i].shdr)))
            return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		elf_map_file
 *
 * Maps an ELF file into memory (and checks it's a real ELF file)
 */
static BOOL elf_map_file(struct elf_map_file_data* emfd, struct image_file_map* fmap)
{
    unsigned int        i;
    size_t              tmp, page_mask = sysinfo.dwPageSize - 1;
    WCHAR              *dos_path;
    unsigned char e_ident[ARRAY_SIZE(fmap->u.elf.elfhdr.e_ident)];

    elf_reset_file_map(fmap);

    fmap->modtype = DMT_ELF;
    fmap->u.elf.handle = INVALID_HANDLE_VALUE;
    fmap->u.elf.target_copy = NULL;

    switch (emfd->kind)
    {
    case from_file:
        if (!(dos_path = get_dos_file_name(emfd->u.file.filename))) return FALSE;
        fmap->u.elf.handle = CreateFileW(dos_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        heap_free(dos_path);
        if (fmap->u.elf.handle == INVALID_HANDLE_VALUE) return FALSE;
        break;
    case from_handle:
        if (!DuplicateHandle(GetCurrentProcess(), emfd->u.handle, GetCurrentProcess(), &fmap->u.elf.handle, GENERIC_READ, FALSE, 0))
            return FALSE;
        break;
    case from_process:
        break;
    }

    if (!elf_map_file_read(fmap, emfd, e_ident, sizeof(e_ident), 0))
        return FALSE;

    /* and check for an ELF header */
    if (memcmp(e_ident, "\177ELF", 4))
        return FALSE;

    fmap->addr_size = e_ident[4] == 2 /* ELFCLASS64 */ ? 64 : 32;

    if (fmap->addr_size == 32)
    {
        struct
        {
            UINT8   e_ident[16];  /* Magic number and other info */
            UINT16  e_type;       /* Object file type */
            UINT16  e_machine;    /* Architecture */
            UINT32  e_version;    /* Object file version */
            UINT32  e_entry;      /* Entry point virtual address */
            UINT32  e_phoff;      /* Program header table file offset */
            UINT32  e_shoff;      /* Section header table file offset */
            UINT32  e_flags;      /* Processor-specific flags */
            UINT16  e_ehsize;     /* ELF header size in bytes */
            UINT16  e_phentsize;  /* Program header table entry size */
            UINT16  e_phnum;      /* Program header table entry count */
            UINT16  e_shentsize;  /* Section header table entry size */
            UINT16  e_shnum;      /* Section header table entry count */
            UINT16  e_shstrndx;   /* Section header string table index */
        } elfhdr32;

        if (!elf_map_file_read(fmap, emfd, &elfhdr32, sizeof(elfhdr32), 0))
            return FALSE;

        memcpy(fmap->u.elf.elfhdr.e_ident, elfhdr32.e_ident, sizeof(e_ident));
        fmap->u.elf.elfhdr.e_type      = elfhdr32.e_type;
        fmap->u.elf.elfhdr.e_machine   = elfhdr32.e_machine;
        fmap->u.elf.elfhdr.e_version   = elfhdr32.e_version;
        fmap->u.elf.elfhdr.e_entry     = elfhdr32.e_entry;
        fmap->u.elf.elfhdr.e_phoff     = elfhdr32.e_phoff;
        fmap->u.elf.elfhdr.e_shoff     = elfhdr32.e_shoff;
        fmap->u.elf.elfhdr.e_flags     = elfhdr32.e_flags;
        fmap->u.elf.elfhdr.e_ehsize    = elfhdr32.e_ehsize;
        fmap->u.elf.elfhdr.e_phentsize = elfhdr32.e_phentsize;
        fmap->u.elf.elfhdr.e_phnum     = elfhdr32.e_phnum;
        fmap->u.elf.elfhdr.e_shentsize = elfhdr32.e_shentsize;
        fmap->u.elf.elfhdr.e_shnum     = elfhdr32.e_shnum;
        fmap->u.elf.elfhdr.e_shstrndx  = elfhdr32.e_shstrndx;
    }
    else
    {
        if (!elf_map_file_read(fmap, emfd, &fmap->u.elf.elfhdr, sizeof(fmap->u.elf.elfhdr), 0))
            return FALSE;
    }

    fmap->u.elf.sect = HeapAlloc(GetProcessHeap(), 0,
                                 fmap->u.elf.elfhdr.e_shnum * sizeof(fmap->u.elf.sect[0]));
    if (!fmap->u.elf.sect) return FALSE;

    for (i = 0; i < fmap->u.elf.elfhdr.e_shnum; i++)
    {
        if (!elf_map_shdr(emfd, fmap, i))
        {
            HeapFree(GetProcessHeap(), 0, fmap->u.elf.sect);
            fmap->u.elf.sect = NULL;
            return FALSE;
        }
        fmap->u.elf.sect[i].mapped = NULL;
    }

    /* grab size of module once loaded in memory */
    fmap->u.elf.elf_size = 0;
    fmap->u.elf.elf_start = ~0L;
    for (i = 0; i < fmap->u.elf.elfhdr.e_phnum; i++)
    {
        if (fmap->addr_size == 32)
        {
            struct
            {
                UINT32  p_type;    /* Segment type */
                UINT32  p_offset;  /* Segment file offset */
                UINT32  p_vaddr;   /* Segment virtual address */
                UINT32  p_paddr;   /* Segment physical address */
                UINT32  p_filesz;  /* Segment size in file */
                UINT32  p_memsz;   /* Segment size in memory */
                UINT32  p_flags;   /* Segment flags */
                UINT32  p_align;   /* Segment alignment */
            } phdr;

            if (elf_map_file_read(fmap, emfd, &phdr, sizeof(phdr),
                                  fmap->u.elf.elfhdr.e_phoff + i * sizeof(phdr)) &&
                phdr.p_type == ELF_PT_LOAD)
            {
                tmp = (phdr.p_vaddr + phdr.p_memsz + page_mask) & ~page_mask;
                if (fmap->u.elf.elf_size < tmp) fmap->u.elf.elf_size = tmp;
                if (phdr.p_vaddr < fmap->u.elf.elf_start) fmap->u.elf.elf_start = phdr.p_vaddr;
            }
        }
        else
        {
            struct
            {
                UINT32  p_type;    /* Segment type */
                UINT32  p_flags;   /* Segment flags */
                UINT64  p_offset;  /* Segment file offset */
                UINT64  p_vaddr;   /* Segment virtual address */
                UINT64  p_paddr;   /* Segment physical address */
                UINT64  p_filesz;  /* Segment size in file */
                UINT64  p_memsz;   /* Segment size in memory */
                UINT64  p_align;   /* Segment alignment */
            } phdr;

            if (elf_map_file_read(fmap, emfd, &phdr, sizeof(phdr),
                                  fmap->u.elf.elfhdr.e_phoff + i * sizeof(phdr)) &&
                phdr.p_type == ELF_PT_LOAD)
            {
                tmp = (phdr.p_vaddr + phdr.p_memsz + page_mask) & ~page_mask;
                if (fmap->u.elf.elf_size < tmp) fmap->u.elf.elf_size = tmp;
                if (phdr.p_vaddr < fmap->u.elf.elf_start) fmap->u.elf.elf_start = phdr.p_vaddr;
            }
        }
    }
    /* if non relocatable ELF, then remove fixed address from computation
     * otherwise, all addresses are zero based and start has no effect
     */
    fmap->u.elf.elf_size -= fmap->u.elf.elf_start;

    switch (emfd->kind)
    {
    case from_handle:
    case from_file: break;
    case from_process:
        if (!(fmap->u.elf.target_copy = HeapAlloc(GetProcessHeap(), 0, fmap->u.elf.elf_size)))
        {
            HeapFree(GetProcessHeap(), 0, fmap->u.elf.sect);
            return FALSE;
        }
        if (!ReadProcessMemory(emfd->u.process.handle, emfd->u.process.load_addr, fmap->u.elf.target_copy,
                               fmap->u.elf.elf_size, NULL))
        {
            HeapFree(GetProcessHeap(), 0, fmap->u.elf.target_copy);
            HeapFree(GetProcessHeap(), 0, fmap->u.elf.sect);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

BOOL elf_map_handle(HANDLE handle, struct image_file_map* fmap)
{
    struct elf_map_file_data emfd;
    emfd.kind = from_handle;
    emfd.u.handle = handle;
    return elf_map_file(&emfd, fmap);
}

static void elf_module_remove(struct process* pcs, struct module_format* modfmt)
{
    image_unmap_file(&modfmt->u.elf_info->file_map);
    HeapFree(GetProcessHeap(), 0, modfmt);
}

/******************************************************************
 *		elf_is_in_thunk_area
 *
 * Check whether an address lies within one of the thunk area we
 * know of.
 */
int elf_is_in_thunk_area(ULONG_PTR addr,
                         const struct elf_thunk_area* thunks)
{
    unsigned i;

    if (thunks) for (i = 0; thunks[i].symname; i++)
    {
        if (addr >= thunks[i].rva_start && addr < thunks[i].rva_end)
            return i;
    }
    return -1;
}

/******************************************************************
 *		elf_hash_symtab
 *
 * creating an internal hash table to ease use ELF symtab information lookup
 */
static void elf_hash_symtab(struct module* module, struct pool* pool,
                            struct hash_table* ht_symtab, struct image_file_map* fmap,
                            struct elf_thunk_area* thunks)
{
    int		                i, j, nsym;
    const char*                 strp;
    const char*                 symname;
    struct symt_compiland*      compiland = NULL;
    const char*                 ptr;
    struct symtab_elt*          ste;
    struct image_section_map    ism, ism_str;
    const char *symtab;

    if (!elf_find_section_type(fmap, ".symtab", ELF_SHT_SYMTAB, &ism) &&
        !elf_find_section_type(fmap, ".dynsym", ELF_SHT_DYNSYM, &ism)) return;
    if ((symtab = image_map_section(&ism)) == IMAGE_NO_MAP) return;
    ism_str.fmap = ism.fmap;
    ism_str.sidx = fmap->u.elf.sect[ism.sidx].shdr.sh_link;
    if ((strp = image_map_section(&ism_str)) == IMAGE_NO_MAP)
    {
        image_unmap_section(&ism);
        return;
    }

    nsym = image_get_map_size(&ism) /
           (fmap->addr_size == 32 ? sizeof(struct elf_sym32) : sizeof(struct elf_sym));

    for (j = 0; thunks[j].symname; j++)
        thunks[j].rva_start = thunks[j].rva_end = 0;

    for (i = 0; i < nsym; i++)
    {
        struct elf_sym sym;
        unsigned int type;

        if (fmap->addr_size == 32)
        {
            struct elf_sym32 *sym32 = &((struct elf_sym32 *)symtab)[i];

            sym.st_name  = sym32->st_name;
            sym.st_value = sym32->st_value;
            sym.st_size  = sym32->st_size;
            sym.st_info  = sym32->st_info;
            sym.st_other = sym32->st_other;
            sym.st_shndx = sym32->st_shndx;
        }
        else
            sym = ((struct elf_sym *)symtab)[i];

        type = sym.st_info & 0xf;

        /* Ignore certain types of entries which really aren't of that much
         * interest.
         */
        if ((type != ELF_STT_NOTYPE && type != ELF_STT_FILE && type != ELF_STT_OBJECT && type != ELF_STT_FUNC)
            || !sym.st_shndx)
        {
            continue;
        }

        symname = strp + sym.st_name;

        /* handle some specific symtab (that we'll throw away when done) */
        switch (type)
        {
        case ELF_STT_FILE:
            if (symname)
                compiland = symt_new_compiland(module, sym.st_value,
                                               source_new(module, NULL, symname));
            else
                compiland = NULL;
            continue;
        case ELF_STT_NOTYPE:
            /* we are only interested in wine markers inserted by winebuild */
            for (j = 0; thunks[j].symname; j++)
            {
                if (!strcmp(symname, thunks[j].symname))
                {
                    thunks[j].rva_start = sym.st_value;
                    thunks[j].rva_end   = sym.st_value + sym.st_size;
                    break;
                }
            }
            continue;
        }

        /* FIXME: we don't need to handle them (GCC internals)
         * Moreover, they screw up our symbol lookup :-/
         */
        if (symname[0] == '.' && symname[1] == 'L' && isdigit(symname[2]))
            continue;

        ste = pool_alloc(pool, sizeof(*ste));
        ste->ht_elt.name = symname;
        /* GCC emits, in some cases, a .<digit>+ suffix.
         * This is used for static variable inside functions, so
         * that we can have several such variables with same name in
         * the same compilation unit
         * We simply ignore that suffix when present (we also get rid
         * of it in stabs parsing)
         */
        ptr = symname + strlen(symname) - 1;
        if (isdigit(*ptr))
        {
            while (isdigit(*ptr) && ptr >= symname) ptr--;
            if (ptr > symname && *ptr == '.')
            {
                char* n = pool_alloc(pool, ptr - symname + 1);
                memcpy(n, symname, ptr - symname + 1);
                n[ptr - symname] = '\0';
                ste->ht_elt.name = n;
            }
        }
        ste->sym         = sym;
        ste->compiland   = compiland;
        ste->used        = 0;
        hash_table_add(ht_symtab, &ste->ht_elt);
    }
    /* as we added in the ht_symtab pointers to the symbols themselves,
     * we cannot unmap yet the sections, it will be done when we're over
     * with this ELF file
     */
}

/******************************************************************
 *		elf_lookup_symtab
 *
 * lookup a symbol by name in our internal hash table for the symtab
 */
static const struct elf_sym *elf_lookup_symtab(const struct module* module,
                                               const struct hash_table* ht_symtab,
                                               const char* name, const struct symt* compiland)
{
    struct symtab_elt*          weak_result = NULL; /* without compiland name */
    struct symtab_elt*          result = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;
    const char*                 compiland_name;
    const char*                 compiland_basename;
    const char*                 base;

    /* we need weak match up (at least) when symbols of same name, 
     * defined several times in different compilation units,
     * are merged in a single one (hence a different filename for c.u.)
     */
    if (compiland)
    {
        compiland_name = source_get(module,
                                    ((const struct symt_compiland*)compiland)->source);
        compiland_basename = file_nameA(compiland_name);
    }
    else compiland_name = compiland_basename = NULL;
    
    hash_table_iter_init(ht_symtab, &hti, name);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used || strcmp(ste->ht_elt.name, name)) continue;

        weak_result = ste;
        if ((ste->compiland && !compiland_name) || (!ste->compiland && compiland_name))
            continue;
        if (ste->compiland && compiland_name)
        {
            const char* filename = source_get(module, ste->compiland->source);
            if (strcmp(filename, compiland_name))
            {
                base = file_nameA(filename);
                if (strcmp(base, compiland_basename)) continue;
            }
        }
        if (result)
        {
            FIXME("Already found symbol %s (%s) in symtab %s @%08x and %s @%08x\n",
                  name, compiland_name,
                  source_get(module, result->compiland->source), (unsigned int)result->sym.st_value,
                  source_get(module, ste->compiland->source), (unsigned int)ste->sym.st_value);
        }
        else
        {
            result = ste;
            ste->used = 1;
        }
    }
    if (!result && !(result = weak_result))
    {
        FIXME("Couldn't find symbol %s!%s in symtab\n",
              debugstr_w(module->module.ModuleName), name);
        return NULL;
    }
    return &result->sym;
}

static BOOL elf_is_local_symbol(unsigned int info)
{
    return !(info >> 4);
}

/******************************************************************
 *		elf_finish_stabs_info
 *
 * - get any relevant information (address & size) from the bits we got from the
 *   stabs debugging information
 */
static void elf_finish_stabs_info(struct module* module, const struct hash_table* symtab)
{
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym;
    const struct elf_sym*       symp;
    struct elf_module_info*     elf_info = module->format_info[DFI_ELF]->u.elf_info;

    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);
        switch (sym->symt.tag)
        {
        case SymTagFunction:
            if (((struct symt_function*)sym)->address != elf_info->elf_addr &&
                ((struct symt_function*)sym)->size)
            {
                break;
            }
            symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name, 
                                     ((struct symt_function*)sym)->container);
            if (symp)
            {
                if (((struct symt_function*)sym)->address != elf_info->elf_addr &&
                    ((struct symt_function*)sym)->address != elf_info->elf_addr + symp->st_value)
                    FIXME("Changing address for %p/%s!%s from %08lx to %s\n",
                          sym, debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                          ((struct symt_function*)sym)->address,
                          wine_dbgstr_longlong(elf_info->elf_addr + symp->st_value));
                if (((struct symt_function*)sym)->size && ((struct symt_function*)sym)->size != symp->st_size)
                    FIXME("Changing size for %p/%s!%s from %08lx to %08x\n",
                          sym, debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                          ((struct symt_function*)sym)->size, (unsigned int)symp->st_size);

                ((struct symt_function*)sym)->address = elf_info->elf_addr + symp->st_value;
                ((struct symt_function*)sym)->size    = symp->st_size;
            } else
                FIXME("Couldn't find %s!%s\n",
                      debugstr_w(module->module.ModuleName), sym->hash_elt.name);
            break;
        case SymTagData:
            switch (((struct symt_data*)sym)->kind)
            {
            case DataIsGlobal:
            case DataIsFileStatic:
                if (((struct symt_data*)sym)->u.var.kind != loc_absolute ||
                    ((struct symt_data*)sym)->u.var.offset != elf_info->elf_addr)
                    break;
                symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name, 
                                         ((struct symt_data*)sym)->container);
                if (symp)
                {
                    if (((struct symt_data*)sym)->u.var.offset != elf_info->elf_addr &&
                        ((struct symt_data*)sym)->u.var.offset != elf_info->elf_addr + symp->st_value)
                        FIXME("Changing address for %p/%s!%s from %08lx to %s\n",
                              sym, debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                              ((struct symt_function*)sym)->address,
                              wine_dbgstr_longlong(elf_info->elf_addr + symp->st_value));
                    ((struct symt_data*)sym)->u.var.offset = elf_info->elf_addr + symp->st_value;
                    ((struct symt_data*)sym)->kind = elf_is_local_symbol(symp->st_info) ?
                        DataIsFileStatic : DataIsGlobal;
                } else
                    FIXME("Couldn't find %s!%s\n",
                          debugstr_w(module->module.ModuleName), sym->hash_elt.name);
                break;
            default:;
            }
            break;
        default:
            FIXME("Unsupported tag %u\n", sym->symt.tag);
            break;
        }
    }
    /* since we may have changed some addresses & sizes, mark the module to be resorted */
    module->sortlist_valid = FALSE;
}

/******************************************************************
 *		elf_load_wine_thunks
 *
 * creating the thunk objects for a wine native DLL
 */
static int elf_new_wine_thunks(struct module* module, const struct hash_table* ht_symtab,
                               const struct elf_thunk_area* thunks)
{
    int		                j;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;
    DWORD_PTR                   addr;
    struct symt_ht*             symt;

    hash_table_iter_init(ht_symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used) continue;

        addr = module->reloc_delta + ste->sym.st_value;

        j = elf_is_in_thunk_area(ste->sym.st_value, thunks);
        if (j >= 0) /* thunk found */
        {
            symt_new_thunk(module, ste->compiland, ste->ht_elt.name, thunks[j].ordinal,
                           addr, ste->sym.st_size);
        }
        else
        {
            ULONG64     ref_addr;
            struct location loc;

            symt = symt_find_nearest(module, addr);
            if (symt && !symt_get_address(&symt->symt, &ref_addr))
                ref_addr = addr;
            if (!symt || addr != ref_addr)
            {
                /* creating public symbols for all the ELF symbols which haven't been
                 * used yet (ie we have no debug information on them)
                 * That's the case, for example, of the .spec.c files
                 */
                switch (ste->sym.st_info & 0xf)
                {
                case ELF_STT_FUNC:
                    symt_new_function(module, ste->compiland, ste->ht_elt.name,
                                      addr, ste->sym.st_size, NULL);
                    break;
                case ELF_STT_OBJECT:
                    loc.kind = loc_absolute;
                    loc.reg = 0;
                    loc.offset = addr;
                    symt_new_global_variable(module, ste->compiland, ste->ht_elt.name,
                                             elf_is_local_symbol(ste->sym.st_info),
                                             loc, ste->sym.st_size, NULL);
                    break;
                default:
                    FIXME("Shouldn't happen\n");
                    break;
                }
                /* FIXME: this is a hack !!!
                 * we are adding new symbols, but as we're parsing a symbol table
                 * (hopefully without duplicate symbols) we delay rebuilding the sorted
                 * module table until we're done with the symbol table
                 * Otherwise, as we intertwine symbols' add and lookup, performance
                 * is rather bad
                 */
                module->sortlist_valid = TRUE;
            }
        }
    }
    /* see comment above */
    module->sortlist_valid = FALSE;
    return TRUE;
}

/******************************************************************
 *		elf_new_public_symbols
 *
 * Creates a set of public symbols from an ELF symtab
 */
static int elf_new_public_symbols(struct module* module, const struct hash_table* symtab)
{
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

    /* FIXME: we're missing the ELF entry point here */

    hash_table_iter_init(symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        symt_new_public(module, ste->compiland, ste->ht_elt.name,
                        FALSE,
                        module->reloc_delta + ste->sym.st_value,
                        ste->sym.st_size);
    }
    return TRUE;
}

/******************************************************************
 *		elf_load_debug_info_from_map
 *
 * Loads the symbolic information from ELF module which mapping is described
 * in fmap
 * the module has been loaded at 'load_offset' address, so symbols' address
 * relocation is performed.
 * CRC is checked if fmap->with_crc is TRUE
 * returns
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
static BOOL elf_load_debug_info_from_map(struct module* module,
                                         struct image_file_map* fmap,
                                         struct pool* pool,
                                         struct hash_table* ht_symtab)
{
    BOOL                ret = FALSE, lret;
    struct elf_thunk_area thunks[] = 
    {
        {"__wine_spec_import_thunks",           THUNK_ORDINAL_NOTYPE, 0, 0},    /* inter DLL calls */
        {"__wine_spec_delayed_import_loaders",  THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_delayed_import_thunks",   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_delay_load",                   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_thunk_text_16",           -16,                  0, 0},    /* 16 => 32 thunks */
        {"__wine_spec_thunk_text_32",           -32,                  0, 0},    /* 32 => 16 thunks */
        {NULL,                                  0,                    0, 0}
    };

    module->module.SymType = SymExport;

    /* create a hash table for the symtab */
    elf_hash_symtab(module, pool, ht_symtab, fmap, thunks);

    if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        struct image_section_map stab_sect, stabstr_sect;

        /* check if we need an alternate file (from debuglink or build-id) */
        ret = image_check_alternate(fmap, module);

        if (image_find_section(fmap, ".stab", &stab_sect) &&
            image_find_section(fmap, ".stabstr", &stabstr_sect))
        {
            const char* stab;
            const char* stabstr;

            stab = image_map_section(&stab_sect);
            stabstr = image_map_section(&stabstr_sect);
            if (stab != IMAGE_NO_MAP && stabstr != IMAGE_NO_MAP)
            {
                /* OK, now just parse all of the stabs. */
                lret = stabs_parse(module, module->format_info[DFI_ELF]->u.elf_info->elf_addr,
                                   stab, image_get_map_size(&stab_sect) / sizeof(struct stab_nlist), sizeof(struct stab_nlist),
                                   stabstr, image_get_map_size(&stabstr_sect),
                                   NULL, NULL);
                if (lret)
                    /* and fill in the missing information for stabs */
                    elf_finish_stabs_info(module, ht_symtab);
                else
                    WARN("Couldn't correctly read stabs\n");
                ret = ret || lret;
            }
            image_unmap_section(&stab_sect);
            image_unmap_section(&stabstr_sect);
        }
        lret = dwarf2_parse(module, module->reloc_delta, thunks, fmap);
        ret = ret || lret;
    }
    if (wcsstr(module->module.ModuleName, S_ElfW) ||
        !wcscmp(module->module.ModuleName, S_WineLoaderW))
    {
        /* add the thunks for native libraries */
        if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
            elf_new_wine_thunks(module, ht_symtab, thunks);
    }
    /* add all the public symbols from symtab */
    if (elf_new_public_symbols(module, ht_symtab) && !ret) ret = TRUE;

    return ret;
}

/******************************************************************
 *		elf_load_debug_info
 *
 * Loads ELF debugging information from the module image file.
 */
static BOOL elf_load_debug_info(struct process* process, struct module* module)
{
    BOOL                        ret = TRUE;
    struct pool                 pool;
    struct hash_table           ht_symtab;
    struct module_format*       modfmt;

    if (module->type != DMT_ELF || !(modfmt = module->format_info[DFI_ELF]) || !modfmt->u.elf_info)
    {
	ERR("Bad elf module '%s'\n", debugstr_w(module->module.LoadedImageName));
	return FALSE;
    }

    pool_init(&pool, 65536);
    hash_table_init(&pool, &ht_symtab, 256);

    ret = elf_load_debug_info_from_map(module, &modfmt->u.elf_info->file_map, &pool, &ht_symtab);

    pool_destroy(&pool);
    return ret;
}

/******************************************************************
 *		elf_fetch_file_info
 *
 * Gathers some more information for an ELF module from a given file
 */
static BOOL elf_fetch_file_info(struct process* process, const WCHAR* name, ULONG_PTR load_addr, DWORD_PTR* base, DWORD* size, DWORD* checksum)
{
    struct image_file_map fmap;

    struct elf_map_file_data    emfd;

    emfd.kind = from_file;
    emfd.u.file.filename = name;
    if (!elf_map_file(&emfd, &fmap)) return FALSE;
    if (base) *base = fmap.u.elf.elf_start;
    *size = fmap.u.elf.elf_size;
    *checksum = calc_crc32(fmap.u.elf.handle);
    image_unmap_file(&fmap);
    return TRUE;
}

static BOOL elf_load_file_from_fmap(struct process* pcs, const WCHAR* filename,
                                    struct image_file_map* fmap, ULONG_PTR load_offset,
                                    ULONG_PTR dyn_addr, struct elf_info* elf_info)
{
    BOOL        ret = FALSE;

    if (elf_info->flags & ELF_INFO_DEBUG_HEADER)
    {
        struct image_section_map        ism;

        if (elf_find_section_type(fmap, ".dynamic", ELF_SHT_DYNAMIC, &ism))
        {
            char*           ptr = (char*)(ULONG_PTR)fmap->u.elf.sect[ism.sidx].shdr.sh_addr;
            ULONG_PTR       len;

            if (load_offset) ptr += load_offset - fmap->u.elf.elf_start;

            if (fmap->addr_size == 32)
            {
                struct
                {
                    INT32  d_tag;    /* Dynamic entry type */
                    UINT32 d_val;    /* Integer or address value */
                } dyn;

                do
                {
                    if (!ReadProcessMemory(pcs->handle, ptr, &dyn, sizeof(dyn), &len) ||
                        len != sizeof(dyn))
                        return ret;
                    if (dyn.d_tag == ELF_DT_DEBUG)
                    {
                        elf_info->dbg_hdr_addr = dyn.d_val;
                        if (load_offset == 0 && dyn_addr == 0) /* likely the case */
                            /* Assume this module (the Wine loader) has been
                             * loaded at its preferred address */
                            dyn_addr = ism.fmap->u.elf.sect[ism.sidx].shdr.sh_addr;
                        break;
                    }
                    ptr += sizeof(dyn);
                } while (dyn.d_tag);
                if (!dyn.d_tag) return ret;
            }
            else
            {
                struct
                {
                    INT64  d_tag;    /* Dynamic entry type */
                    UINT64 d_val;    /* Integer or address value */
                } dyn;

                do
                {
                    if (!ReadProcessMemory(pcs->handle, ptr, &dyn, sizeof(dyn), &len) ||
                        len != sizeof(dyn))
                        return ret;
                    if (dyn.d_tag == ELF_DT_DEBUG)
                    {
                        elf_info->dbg_hdr_addr = dyn.d_val;
                        if (load_offset == 0 && dyn_addr == 0) /* likely the case */
                            /* Assume this module (the Wine loader) has been
                             * loaded at its preferred address */
                            dyn_addr = ism.fmap->u.elf.sect[ism.sidx].shdr.sh_addr;
                        break;
                    }
                    ptr += sizeof(dyn);
                } while (dyn.d_tag);
                if (!dyn.d_tag) return ret;
            }
        }
        elf_end_find(fmap);
    }

    if (elf_info->flags & ELF_INFO_MODULE)
    {
        struct elf_module_info *elf_module_info;
        struct module_format*   modfmt;
        struct image_section_map ism;
        ULONG_PTR               modbase = load_offset;

        if (elf_find_section_type(fmap, ".dynamic", ELF_SHT_DYNAMIC, &ism))
        {
            ULONG_PTR rva_dyn = elf_get_map_rva(&ism);

            TRACE("For module %s, got ELF (start=%lx dyn=%lx), link_map (start=%lx dyn=%lx)\n",
                  debugstr_w(filename), (ULONG_PTR)fmap->u.elf.elf_start, rva_dyn,
                  load_offset, dyn_addr);
            if (dyn_addr && load_offset + rva_dyn != dyn_addr)
            {
                WARN("\thave to relocate: %lx\n", dyn_addr - rva_dyn);
                modbase = dyn_addr - rva_dyn;
            }
	} else WARN("For module %s, no .dynamic section\n", debugstr_w(filename));
        elf_end_find(fmap);

        modfmt = HeapAlloc(GetProcessHeap(), 0,
                          sizeof(struct module_format) + sizeof(struct elf_module_info));
        if (!modfmt) return FALSE;
        elf_info->module = module_new(pcs, filename, DMT_ELF, FALSE, modbase,
                                      fmap->u.elf.elf_size, 0, calc_crc32(fmap->u.elf.handle));
        if (!elf_info->module)
        {
            HeapFree(GetProcessHeap(), 0, modfmt);
            return FALSE;
        }
        elf_info->module->reloc_delta = elf_info->module->module.BaseOfImage - fmap->u.elf.elf_start;
        elf_module_info = (void*)(modfmt + 1);
        elf_info->module->format_info[DFI_ELF] = modfmt;
        modfmt->module      = elf_info->module;
        modfmt->remove      = elf_module_remove;
        modfmt->loc_compute = NULL;
        modfmt->u.elf_info  = elf_module_info;

        elf_module_info->elf_addr = load_offset;

        elf_module_info->file_map = *fmap;
        elf_reset_file_map(fmap);
        if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
        {
            elf_info->module->module.SymType = SymDeferred;
            ret = TRUE;
        }
        else ret = elf_load_debug_info(pcs, elf_info->module);

        elf_module_info->elf_mark = 1;
        elf_module_info->elf_loader = 0;
    } else ret = TRUE;

    if (elf_info->flags & ELF_INFO_NAME)
    {
        WCHAR*  ptr;
        ptr = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(filename) + 1) * sizeof(WCHAR));
        if (ptr)
        {
            lstrcpyW(ptr, filename);
            elf_info->module_name = ptr;
        }
        else ret = FALSE;
    }

    return ret;
}

/******************************************************************
 *		elf_load_file
 *
 * Loads the information for ELF module stored in 'filename'
 * the module has been loaded at 'load_offset' address
 * returns
 *	-1 if the file cannot be found/opened
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
static BOOL elf_load_file(struct process* pcs, const WCHAR* filename,
                          ULONG_PTR load_offset, ULONG_PTR dyn_addr,
                          struct elf_info* elf_info)
{
    BOOL                        ret = FALSE;
    struct image_file_map       fmap;
    struct elf_map_file_data    emfd;

    TRACE("Processing elf file '%s' at %08lx\n", debugstr_w(filename), load_offset);

    emfd.kind = from_file;
    emfd.u.file.filename = filename;
    if (!elf_map_file(&emfd, &fmap)) return ret;

    /* Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    if (!fmap.u.elf.elf_start && !load_offset)
        ERR("Relocatable ELF %s, but no load address. Loading at 0x0000000\n",
            debugstr_w(filename));

    ret = elf_load_file_from_fmap(pcs, filename, &fmap, load_offset, dyn_addr, elf_info);

    image_unmap_file(&fmap);

    return ret;
}

struct elf_load_file_params
{
    struct process  *process;
    ULONG_PTR        load_offset;
    ULONG_PTR        dyn_addr;
    struct elf_info *elf_info;
};

static BOOL elf_load_file_cb(void *param, HANDLE handle, const WCHAR *filename)
{
    struct elf_load_file_params *load_file = param;
    return elf_load_file(load_file->process, filename, load_file->load_offset, load_file->dyn_addr, load_file->elf_info);
}

/******************************************************************
 *		elf_search_auxv
 *
 * locate some a value from the debuggee auxiliary vector
 */
static BOOL elf_search_auxv(const struct process* pcs, unsigned type, ULONG_PTR* val)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    BYTE*       addr;
    BYTE*       str;
    BYTE*       str_max;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromName(pcs->handle, "libwine.so.1!__wine_main_environ", si) ||
        !(addr = (void*)(DWORD_PTR)si->Address) ||
        !ReadProcessMemory(pcs->handle, addr, &addr, sizeof(addr), NULL) ||
        !addr)
    {
        FIXME("can't find symbol in module\n");
        return FALSE;
    }
    /* walk through envp[] */
    /* envp[] strings are located after the auxiliary vector, so protect the walk */
    str_max = (void*)(DWORD_PTR)~0L;
    while (ReadProcessMemory(pcs->handle, addr, &str, sizeof(str), NULL) &&
           (addr = (void*)((DWORD_PTR)addr + sizeof(str))) != NULL && str != NULL)
        str_max = min(str_max, str);

    /* Walk through the end of envp[] array.
     * Actually, there can be several NULLs at the end of envp[]. This happens when an env variable is
     * deleted, the last entry is replaced by an extra NULL.
     */
    while (addr < str_max && ReadProcessMemory(pcs->handle, addr, &str, sizeof(str), NULL) && str == NULL)
        addr = (void*)((DWORD_PTR)addr + sizeof(str));

    if (pcs->is_64bit)
    {
        struct
        {
            UINT64 a_type;
            UINT64 a_val;
        } auxv;

        while (ReadProcessMemory(pcs->handle, addr, &auxv, sizeof(auxv), NULL) && auxv.a_type)
        {
            if (auxv.a_type == type)
            {
                *val = auxv.a_val;
                return TRUE;
            }
            addr += sizeof(auxv);
        }
    }
    else
    {
        struct
        {
            UINT32 a_type;
            UINT32 a_val;
        } auxv;

        while (ReadProcessMemory(pcs->handle, addr, &auxv, sizeof(auxv), NULL) && auxv.a_type)
        {
            if (auxv.a_type == type)
            {
                *val = auxv.a_val;
                return TRUE;
            }
            addr += sizeof(auxv);
        }
    }

    return FALSE;
}

/******************************************************************
 *		elf_search_and_load_file
 *
 * lookup a file in standard ELF locations, and if found, load it
 */
static BOOL elf_search_and_load_file(struct process* pcs, const WCHAR* filename,
                                     ULONG_PTR load_offset, ULONG_PTR dyn_addr,
                                     struct elf_info* elf_info)
{
    BOOL                ret = FALSE;
    struct module*      module;
    static const WCHAR  S_libstdcPPW[] = {'l','i','b','s','t','d','c','+','+','\0'};

    if (filename == NULL || *filename == '\0') return FALSE;
    if ((module = module_is_already_loaded(pcs, filename)))
    {
        elf_info->module = module;
        elf_info->module->format_info[DFI_ELF]->u.elf_info->elf_mark = 1;
        return module->module.SymType;
    }

    if (wcsstr(filename, S_libstdcPPW)) return FALSE; /* We know we can't do it */
    ret = elf_load_file(pcs, filename, load_offset, dyn_addr, elf_info);
    /* if relative pathname, try some absolute base dirs */
    if (!ret && filename == file_name(filename))
    {
        struct elf_load_file_params load_elf;
        load_elf.process     = pcs;
        load_elf.load_offset = load_offset;
        load_elf.dyn_addr    = dyn_addr;
        load_elf.elf_info    = elf_info;

        ret = search_unix_path(filename, process_getenv(pcs, L"LD_LIBRARY_PATH"), elf_load_file_cb, &load_elf)
            || search_unix_path(filename, BINDIR, elf_load_file_cb, &load_elf)
            || search_dll_path(pcs, filename, elf_load_file_cb, &load_elf);
    }

    return ret;
}

typedef BOOL (*enum_elf_modules_cb)(const WCHAR*, ULONG_PTR load_addr,
                                    ULONG_PTR dyn_addr, BOOL is_system, void* user);

/******************************************************************
 *		elf_enum_modules_internal
 *
 * Enumerate ELF modules from a running process
 */
static BOOL elf_enum_modules_internal(const struct process* pcs,
                                      const WCHAR* main_name,
                                      enum_elf_modules_cb cb, void* user)
{
    WCHAR bufstrW[MAX_PATH];
    char bufstr[256];
    ULONG_PTR lm_addr;

    if (pcs->is_64bit)
    {
        struct
        {
            UINT32 r_version;
            UINT64 r_map;
            UINT64 r_brk;
            UINT32 r_state;
            UINT64 r_ldbase;
        } dbg_hdr;
        struct
        {
            UINT64 l_addr;
            UINT64 l_name;
            UINT64 l_ld;
            UINT64 l_next, l_prev;
        } lm;

        if (!pcs->dbg_hdr_addr || !read_process_memory(pcs, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
            return FALSE;

        /* Now walk the linked list.  In all known ELF implementations,
         * the dynamic loader maintains this linked list for us.  In some
         * cases the first entry doesn't appear with a name, in other cases it
         * does.
         */
        for (lm_addr = dbg_hdr.r_map; lm_addr; lm_addr = lm.l_next)
        {
            if (!read_process_memory(pcs, lm_addr, &lm, sizeof(lm)))
                return FALSE;

            if (lm.l_prev && /* skip first entry, normally debuggee itself */
                lm.l_name && read_process_memory(pcs, lm.l_name, bufstr, sizeof(bufstr)))
            {
                bufstr[sizeof(bufstr) - 1] = '\0';
                MultiByteToWideChar(CP_UNIXCP, 0, bufstr, -1, bufstrW, ARRAY_SIZE(bufstrW));
                if (main_name && !bufstrW[0]) lstrcpyW(bufstrW, main_name);
                if (!cb(bufstrW, (ULONG_PTR)lm.l_addr, (ULONG_PTR)lm.l_ld, FALSE, user))
                    break;
            }
        }
    }
    else
    {
        struct
        {
            UINT32 r_version;
            UINT32 r_map;
            UINT32 r_brk;
            UINT32 r_state;
            UINT32 r_ldbase;
        } dbg_hdr;
        struct
        {
            UINT32 l_addr;
            UINT32 l_name;
            UINT32 l_ld;
            UINT32 l_next, l_prev;
        } lm;

        if (!pcs->dbg_hdr_addr || !read_process_memory(pcs, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
            return FALSE;

        /* Now walk the linked list.  In all known ELF implementations,
         * the dynamic loader maintains this linked list for us.  In some
         * cases the first entry doesn't appear with a name, in other cases it
         * does.
         */
        for (lm_addr = dbg_hdr.r_map; lm_addr; lm_addr = lm.l_next)
        {
            if (!read_process_memory(pcs, lm_addr, &lm, sizeof(lm)))
                return FALSE;

            if (lm.l_prev && /* skip first entry, normally debuggee itself */
                lm.l_name && read_process_memory(pcs, lm.l_name, bufstr, sizeof(bufstr)))
            {
                bufstr[sizeof(bufstr) - 1] = '\0';
                MultiByteToWideChar(CP_UNIXCP, 0, bufstr, -1, bufstrW, ARRAY_SIZE(bufstrW));
                if (main_name && !bufstrW[0]) lstrcpyW(bufstrW, main_name);
                if (!cb(bufstrW, (ULONG_PTR)lm.l_addr, (ULONG_PTR)lm.l_ld, FALSE, user))
                    break;
            }
        }
    }

    if (!lm_addr)
    {
        ULONG_PTR ehdr_addr;

        if (elf_search_auxv(pcs, ELF_AT_SYSINFO_EHDR, &ehdr_addr))
        {
            static const WCHAR vdsoW[] = {'[','v','d','s','o',']','.','s','o',0};
            cb(vdsoW, ehdr_addr, 0, TRUE, user);
        }
    }
    return TRUE;
}

struct elf_enum_user
{
    enum_modules_cb     cb;
    void*               user;
};

static BOOL elf_enum_modules_translate(const WCHAR* name, ULONG_PTR load_addr,
                                       ULONG_PTR dyn_addr, BOOL is_system, void* user)
{
    struct elf_enum_user*       eeu = user;
    return eeu->cb(name, load_addr, eeu->user);
}

/******************************************************************
 *		elf_enum_modules
 *
 * Enumerates the ELF loaded modules from a running target (hProc)
 * This function doesn't require that someone has called SymInitialize
 * on this very process.
 */
static BOOL elf_enum_modules(struct process* process, enum_modules_cb cb, void* user)
{
    struct elf_info     elf_info;
    BOOL                ret;
    struct elf_enum_user eeu;

    elf_info.flags = ELF_INFO_DEBUG_HEADER | ELF_INFO_NAME;
    elf_info.module_name = NULL;
    eeu.cb = cb;
    eeu.user = user;
    ret = elf_enum_modules_internal(process, elf_info.module_name, elf_enum_modules_translate, &eeu);
    HeapFree(GetProcessHeap(), 0, (char*)elf_info.module_name);
    return ret;
}

struct elf_load
{
    struct process*     pcs;
    struct elf_info     elf_info;
    const WCHAR*        name;
    BOOL                ret;
};

/******************************************************************
 *		elf_load_cb
 *
 * Callback for elf_load_module, used to walk the list of loaded
 * modules.
 */
static BOOL elf_load_cb(const WCHAR* name, ULONG_PTR load_addr,
                        ULONG_PTR dyn_addr, BOOL is_system, void* user)
{
    struct elf_load*    el = user;
    BOOL                ret = TRUE;
    const WCHAR*        p;

    if (is_system) /* virtual ELF module, created by system. handle it from memory */
    {
        struct module*                  module;
        struct elf_map_file_data        emfd;
        struct image_file_map           fmap;

        if ((module = module_is_already_loaded(el->pcs, name)))
        {
            el->elf_info.module = module;
            el->elf_info.module->format_info[DFI_ELF]->u.elf_info->elf_mark = 1;
            return module->module.SymType;
        }

        emfd.kind = from_process;
        emfd.u.process.handle = el->pcs->handle;
        emfd.u.process.load_addr = (void*)load_addr;

        if (elf_map_file(&emfd, &fmap))
            el->ret = elf_load_file_from_fmap(el->pcs, name, &fmap, load_addr, 0, &el->elf_info);
        return TRUE;
    }
    if (el->name)
    {
        /* memcmp is needed for matches when bufstr contains also version information
         * el->name: libc.so, name: libc.so.6.0
         */
        p = file_name(name);
    }

    if (!el->name || !memcmp(p, el->name, lstrlenW(el->name) * sizeof(WCHAR)))
    {
        el->ret = elf_search_and_load_file(el->pcs, name, load_addr, dyn_addr, &el->elf_info);
        if (el->name) ret = FALSE;
    }

    return ret;
}

/******************************************************************
 *		elf_load_module
 *
 * loads an ELF module and stores it in process' module list
 * Also, find module real name and load address from
 * the real loaded modules list in pcs address space
 */
static struct module* elf_load_module(struct process* pcs, const WCHAR* name, ULONG_PTR addr)
{
    struct elf_load     el;

    TRACE("(%p %s %08lx)\n", pcs, debugstr_w(name), addr);

    el.elf_info.flags = ELF_INFO_MODULE;
    el.ret = FALSE;

    if (pcs->dbg_hdr_addr) /* we're debugging a life target */
    {
        el.pcs = pcs;
        /* do only the lookup from the filename, not the path (as we lookup module
         * name in the process' loaded module list)
         */
        el.name = file_name(name);
        el.ret = FALSE;

        if (!elf_enum_modules_internal(pcs, NULL, elf_load_cb, &el))
            return NULL;
    }
    else if (addr)
    {
        el.name = name;
        el.ret = elf_search_and_load_file(pcs, el.name, addr, 0, &el.elf_info);
    }
    if (!el.ret) return NULL;
    assert(el.elf_info.module);
    return el.elf_info.module;
}

/******************************************************************
 *		elf_synchronize_module_list
 *
 * this function rescans the debuggee module's list and synchronizes it with
 * the one from 'pcs', i.e.:
 * - if a module is in debuggee and not in pcs, it's loaded into pcs
 * - if a module is in pcs and not in debuggee, it's unloaded from pcs
 */
static BOOL elf_synchronize_module_list(struct process* pcs)
{
    struct module*      module;
    struct elf_load     el;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF && !module->is_virtual)
            module->format_info[DFI_ELF]->u.elf_info->elf_mark = 0;
    }

    el.pcs = pcs;
    el.elf_info.flags = ELF_INFO_MODULE;
    el.ret = FALSE;
    el.name = NULL; /* fetch all modules */

    if (!elf_enum_modules_internal(pcs, NULL, elf_load_cb, &el))
        return FALSE;

    module = pcs->lmodules;
    while (module)
    {
        if (module->type == DMT_ELF && !module->is_virtual)
        {
            struct elf_module_info* elf_info = module->format_info[DFI_ELF]->u.elf_info;

            if (!elf_info->elf_mark && !elf_info->elf_loader)
            {
                module_remove(pcs, module);
                /* restart all over */
                module = pcs->lmodules;
                continue;
            }
        }
        module = module->next;
    }
    return TRUE;
}

static const struct loader_ops elf_loader_ops =
{
    elf_synchronize_module_list,
    elf_load_module,
    elf_load_debug_info,
    elf_enum_modules,
    elf_fetch_file_info,
};

/******************************************************************
 *		elf_read_wine_loader_dbg_info
 *
 * Try to find a decent wine executable which could have loaded the debuggee
 */
BOOL elf_read_wine_loader_dbg_info(struct process* pcs, ULONG_PTR addr)
{
    struct elf_info     elf_info;
    WCHAR *loader;
    BOOL ret;

    elf_info.flags = ELF_INFO_DEBUG_HEADER | ELF_INFO_MODULE;
    loader = get_wine_loader_name(pcs);
    ret = elf_search_and_load_file(pcs, loader, addr, 0, &elf_info);
    heap_free(loader);
    if (!ret || !elf_info.dbg_hdr_addr) return FALSE;

    TRACE("Found ELF debug header %#lx\n", elf_info.dbg_hdr_addr);
    elf_info.module->format_info[DFI_ELF]->u.elf_info->elf_loader = 1;
    module_set_module(elf_info.module, S_WineLoaderW);
    pcs->dbg_hdr_addr = elf_info.dbg_hdr_addr;
    pcs->loader = &elf_loader_ops;
    return TRUE;
}
