/*
 * File elf_private.h - definitions for processing of ELF files
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

#pragma once

#define IMAGE_NO_MAP  ((void*)-1)

struct elf_header
{
    UINT8   e_ident[16];  /* Magic number and other info */
    UINT16  e_type;       /* Object file type */
    UINT16  e_machine;    /* Architecture */
    UINT32  e_version;    /* Object file version */
    UINT64  e_entry;      /* Entry point virtual address */
    UINT64  e_phoff;      /* Program header table file offset */
    UINT64  e_shoff;      /* Section header table file offset */
    UINT32  e_flags;      /* Processor-specific flags */
    UINT16  e_ehsize;     /* ELF header size in bytes */
    UINT16  e_phentsize;  /* Program header table entry size */
    UINT16  e_phnum;      /* Program header table entry count */
    UINT16  e_shentsize;  /* Section header table entry size */
    UINT16  e_shnum;      /* Section header table entry count */
    UINT16  e_shstrndx;   /* Section header string table index */
};

struct elf_section_header
{
    UINT32  sh_name;       /* Section name (string tbl index) */
    UINT32  sh_type;       /* Section type */
    UINT64  sh_flags;      /* Section flags */
    UINT64  sh_addr;       /* Section virtual addr at execution */
    UINT64  sh_offset;     /* Section file offset */
    UINT64  sh_size;       /* Section size in bytes */
    UINT32  sh_link;       /* Link to another section */
    UINT32  sh_info;       /* Additional section information */
    UINT64  sh_addralign;  /* Section alignment */
    UINT64  sh_entsize;    /* Entry size if section holds table */
};

struct macho_load_command
{
    UINT32  cmd;           /* type of load command */
    UINT32  cmdsize;       /* total size of command in bytes */
};

struct macho_uuid_command
{
    UINT32  cmd;           /* LC_UUID */
    UINT32  cmdsize;
    UINT8   uuid[16];
};

struct macho_section
{
    char    sectname[16];  /* name of this section */
    char    segname[16];   /* segment this section goes in */
    UINT64  addr;          /* memory address of this section */
    UINT64  size;          /* size in bytes of this section */
    UINT32  offset;        /* file offset of this section */
    UINT32  align;         /* section alignment (power of 2) */
    UINT32  reloff;        /* file offset of relocation entries */
    UINT32  nreloc;        /* number of relocation entries */
    UINT32  flags;         /* flags (section type and attributes)*/
    UINT32  reserved1;     /* reserved (for offset or index) */
    UINT32  reserved2;     /* reserved (for count or sizeof) */
    UINT32  reserved3;     /* reserved */
};

struct macho_section32
{
    char    sectname[16];  /* name of this section */
    char    segname[16];   /* segment this section goes in */
    UINT32  addr;          /* memory address of this section */
    UINT32  size;          /* size in bytes of this section */
    UINT32  offset;        /* file offset of this section */
    UINT32  align;         /* section alignment (power of 2) */
    UINT32  reloff;        /* file offset of relocation entries */
    UINT32  nreloc;        /* number of relocation entries */
    UINT32  flags;         /* flags (section type and attributes)*/
    UINT32  reserved1;     /* reserved (for offset or index) */
    UINT32  reserved2;     /* reserved (for count or sizeof) */
};

/* structure holding information while handling an ELF image
 * allows one by one section mapping for memory savings
 */
struct image_file_map
{
    enum module_type            modtype;
    const struct image_file_map_ops *ops;
    unsigned                    addr_size;      /* either 16 (not used), 32 or 64 */
    struct image_file_map*      alternate;      /* another file linked to this one */
    union
    {
        struct elf_file_map
        {
            size_t                      elf_size;
            size_t                      elf_start;
            HANDLE                      handle;
            const char*	                shstrtab;
            char*                       target_copy;
            struct elf_header           elfhdr;
            struct
            {
                struct elf_section_header       shdr;
                const char*                     mapped;
            }*                          sect;
        } elf;
        struct macho_file_map
        {
            size_t                      segs_size;
            size_t                      segs_start;
            HANDLE                      handle;
            struct image_file_map*      dsym;   /* the debug symbols file associated with this one */
            size_t                      header_size; /* size of real header in file */
            size_t                      commands_size;
            unsigned int                commands_count;

            const struct macho_load_command*    load_commands;
            const struct macho_uuid_command*    uuid;

            /* The offset in the file which is this architecture.  mach_header was
             * read from arch_offset. */
            unsigned                    arch_offset;

            int                         num_sections;
            struct
            {
                struct macho_section            section;
                const char*                     mapped;
                unsigned int                    ignored : 1;
            }*                          sect;
        } macho;
        struct pe_file_map
        {
            HANDLE                      hMap;
            IMAGE_NT_HEADERS            ntheader;
            BOOL                        builtin;
            unsigned                    full_count;
            void*                       full_map;
            struct
            {
                IMAGE_SECTION_HEADER            shdr;
                const char*                     mapped;
            }*                          sect;
            const char*	                strtable;
        } pe;
    } u;
};

struct image_section_map
{
    struct image_file_map*      fmap;
    LONG_PTR                    sidx;
};

struct stab_nlist
{
    unsigned            n_strx;
    unsigned char       n_type;
    char                n_other;
    short               n_desc;
    unsigned            n_value;
};

struct macho64_nlist
{
    unsigned            n_strx;
    unsigned char       n_type;
    char                n_other;
    short               n_desc;
    UINT64              n_value;
};

BOOL image_check_alternate(struct image_file_map* fmap, const struct module* module) DECLSPEC_HIDDEN;

BOOL elf_map_handle(HANDLE handle, struct image_file_map* fmap) DECLSPEC_HIDDEN;
BOOL pe_map_file(HANDLE file, struct image_file_map* fmap, enum module_type mt) DECLSPEC_HIDDEN;

struct image_file_map_ops
{
    const char* (*map_section)(struct image_section_map* ism);
    void  (*unmap_section)(struct image_section_map* ism);
    BOOL (*find_section)(struct image_file_map* fmap, const char* name, struct image_section_map* ism);
    DWORD_PTR (*get_map_rva)(const struct image_section_map* ism);
    unsigned (*get_map_size)(const struct image_section_map* ism);
    void (*unmap_file)(struct image_file_map *fmap);
};

static inline BOOL image_find_section(struct image_file_map* fmap, const char* name,
                                      struct image_section_map* ism)
{
    while (fmap)
    {
        if (fmap->ops->find_section(fmap, name, ism)) return TRUE;
        fmap = fmap->alternate;
    }
    ism->fmap = NULL;
    ism->sidx = -1;
    return FALSE;
}

static inline void image_unmap_file(struct image_file_map* fmap)
{
    while (fmap)
    {
        fmap->ops->unmap_file(fmap);
        fmap = fmap->alternate;
    }
}

static inline const char* image_map_section(struct image_section_map* ism)
{
    return ism->fmap ? ism->fmap->ops->map_section(ism) : NULL;
}

static inline void image_unmap_section(struct image_section_map* ism)
{
    if (ism->fmap) ism->fmap->ops->unmap_section(ism);
}

static inline DWORD_PTR image_get_map_rva(const struct image_section_map* ism)
{
    return ism->fmap ? ism->fmap->ops->get_map_rva(ism) : 0;
}

static inline unsigned image_get_map_size(const struct image_section_map* ism)
{
    return ism->fmap ? ism->fmap->ops->get_map_size(ism) : 0;
}
