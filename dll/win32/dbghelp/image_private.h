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

#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_SYS_ELF32_H
# include <sys/elf32.h>
#endif
#ifdef HAVE_SYS_EXEC_ELF_H
# include <sys/exec_elf.h>
#endif
#if !defined(DT_NUM)
# if defined(DT_COUNT)
#  define DT_NUM DT_COUNT
# else
/* this seems to be a satisfactory value on Solaris, which doesn't support this AFAICT */
#  define DT_NUM 24
# endif
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#ifdef HAVE_SYS_LINK_H
# include <sys/link.h>
#endif
#ifdef HAVE_MACH_O_LOADER_H
#include <mach-o/loader.h>
#endif

#define IMAGE_NO_MAP  ((void*)-1)

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
#if defined(__ELF__) && !defined(DBGHELP_STATIC_LIB)
            Elf64_Ehdr                  elfhdr;
            struct
            {
                Elf64_Shdr                      shdr;
                const char*                     mapped;
            }*                          sect;
#endif
        } elf;
        struct macho_file_map
        {
            size_t                      segs_size;
            size_t                      segs_start;
            HANDLE                      handle;
            struct image_file_map*      dsym;   /* the debug symbols file associated with this one */

#ifdef HAVE_MACH_O_LOADER_H
            struct mach_header          mach_header;
            size_t                      header_size; /* size of real header in file */
            const struct load_command*  load_commands;
            const struct uuid_command*  uuid;

            /* The offset in the file which is this architecture.  mach_header was
             * read from arch_offset. */
            unsigned                    arch_offset;

            int                         num_sections;
            struct
            {
                struct section_64               section;
                const char*                     mapped;
                unsigned int                    ignored : 1;
            }*                          sect;
#endif
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
