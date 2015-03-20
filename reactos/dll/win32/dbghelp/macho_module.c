/*
 * File macho_module.c - processing of Mach-O files
 *      Originally based on elf_module.c
 *
 * Copyright (C) 1996, Eric Youngdale.
 *               1999-2007 Eric Pouech
 *               2009 Ken Thomases, CodeWeavers Inc.
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

#include "dbghelp_private.h"

#ifdef HAVE_MACH_O_LOADER_H

#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#ifdef HAVE_MACH_O_DYLD_IMAGES_H
#include <mach-o/dyld_images.h>
#else
struct dyld_image_info {
    const struct mach_header *imageLoadAddress;
    const char               *imageFilePath;
    uintptr_t                 imageFileModDate;
};

struct dyld_all_image_infos {
    uint32_t                      version;
    uint32_t                      infoArrayCount;
    const struct dyld_image_info *infoArray;
    void*                         notification;
    int                           processDetachedFromSharedRegion;
};
#endif

#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

#ifdef WORDS_BIGENDIAN
#define swap_ulong_be_to_host(n) (n)
#else
#define swap_ulong_be_to_host(n) (RtlUlongByteSwap(n))
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_macho);


struct macho_module_info
{
    unsigned long               load_addr;
    unsigned short              in_use : 1,
                                is_loader : 1;
};

#define MACHO_INFO_DEBUG_HEADER   0x0001
#define MACHO_INFO_MODULE         0x0002
#define MACHO_INFO_NAME           0x0004

struct macho_info
{
    unsigned                    flags;          /* IN  one (or several) of the MACHO_INFO constants */
    unsigned long               dbg_hdr_addr;   /* OUT address of debug header (if MACHO_INFO_DEBUG_HEADER is set) */
    struct module*              module;         /* OUT loaded module (if MACHO_INFO_MODULE is set) */
    const WCHAR*                module_name;    /* OUT found module name (if MACHO_INFO_NAME is set) */
};

/* structure holding information while handling a Mach-O image */
#define BITS_PER_ULONG (sizeof(ULONG) * 8)
#define ULONGS_FOR_BITS(nbits) (((nbits) + BITS_PER_ULONG - 1) / BITS_PER_ULONG)
struct macho_file_map
{
    /* A copy of the Mach-O header for an individual architecture. */
    struct mach_header          mach_header;

    /* The mapped load commands. */
    const struct load_command*  load_commands;

    /* The portion of the file which is this architecture.  mach_header was
     * read from arch_offset. */
    unsigned                    arch_offset;
    unsigned                    arch_size;

    /* The range of address space covered by all segments. */
    size_t                      segs_start;
    size_t                      segs_size;

    /* Map of which sections contain code.  Sections are accessed using 1-based
     * index.  Bit 0 of this bitset indicates if the bitset has been initialized. */
    RTL_BITMAP                  sect_is_code;
    ULONG                       sect_is_code_buff[ULONGS_FOR_BITS(MAX_SECT + 1)];

    /* The file. */
    int                         fd;
};

static void macho_unmap_file(struct macho_file_map* fmap);

/******************************************************************
 *              macho_calc_range
 *
 * For a range (offset & length) of a single architecture within
 * a Mach-O file, calculate the page-aligned range of the whole file
 * that encompasses it.  For a fat binary, the architecture will
 * itself be offset within the file, so take that into account.
 */
static void macho_calc_range(const struct macho_file_map* fmap, unsigned offset,
                             unsigned len, unsigned* out_aligned_offset,
                             unsigned* out_aligned_end, unsigned* out_aligned_len,
                             unsigned* out_misalign)
{
    unsigned pagemask = sysconf( _SC_PAGESIZE ) - 1;
    unsigned file_offset, misalign;

    file_offset = fmap->arch_offset + offset;
    misalign = file_offset & pagemask;
    *out_aligned_offset = file_offset - misalign;
    *out_aligned_end = (file_offset + len + pagemask) & ~pagemask;
    if (out_aligned_len)
        *out_aligned_len = *out_aligned_end - *out_aligned_offset;
    if (out_misalign)
        *out_misalign = misalign;
}

/******************************************************************
 *              macho_map_range
 *
 * Maps a range (offset, length in bytes) from a Mach-O file into memory
 */
static const char* macho_map_range(const struct macho_file_map* fmap, unsigned offset, unsigned len)
{
    unsigned    misalign, aligned_offset, aligned_map_end, map_size;
    const void* aligned_ptr;

    TRACE("(%p/%d, 0x%08x, 0x%08x)\n", fmap, fmap->fd, offset, len);

    macho_calc_range(fmap, offset, len, &aligned_offset, &aligned_map_end,
                     &map_size, &misalign);

    aligned_ptr = mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fmap->fd, aligned_offset);

    TRACE("Mapped (0x%08x - 0x%08x) to %p\n", aligned_offset, aligned_map_end, aligned_ptr);

    if (aligned_ptr == MAP_FAILED) return MACHO_NO_MAP;
    return (const char*)aligned_ptr + misalign;
}

/******************************************************************
 *              macho_unmap_range
 *
 * Unmaps a range (offset, length in bytes) of a Mach-O file from memory
 */
static void macho_unmap_range(const void** mapped, const struct macho_file_map* fmap,
                              unsigned offset, unsigned len)
{
    TRACE("(%p, %p/%d, 0x%08x, 0x%08x)\n", mapped, fmap, fmap->fd, offset, len);

    if (mapped && *mapped != MACHO_NO_MAP)
    {
        unsigned    misalign, aligned_offset, aligned_map_end, map_size;
        void*       aligned_ptr;

        macho_calc_range(fmap, offset, len, &aligned_offset, &aligned_map_end,
                         &map_size, &misalign);

        aligned_ptr = (char*)*mapped - misalign;
        if (munmap(aligned_ptr, map_size) < 0)
            WARN("Couldn't unmap the range\n");
        TRACE("Unmapped (0x%08x - 0x%08x) from %p - %p\n", aligned_offset, aligned_map_end, aligned_ptr, (char*)aligned_ptr + map_size);
        *mapped = MACHO_NO_MAP;
    }
}

/******************************************************************
 *              macho_map_ranges
 *
 * Maps two ranges (offset, length in bytes) from a Mach-O file
 * into memory.  If the two ranges overlap, use one mmap so that
 * the munmap doesn't fragment the mapping.
 */
static BOOL macho_map_ranges(const struct macho_file_map* fmap,
                             unsigned offset1, unsigned len1,
                             unsigned offset2, unsigned len2,
                             const void** mapped1, const void** mapped2)
{
    unsigned aligned_offset1, aligned_map_end1;
    unsigned aligned_offset2, aligned_map_end2;

    TRACE("(%p/%d, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p, %p)\n", fmap, fmap->fd,
            offset1, len1, offset2, len2, mapped1, mapped2);

    macho_calc_range(fmap, offset1, len1, &aligned_offset1, &aligned_map_end1, NULL, NULL);
    macho_calc_range(fmap, offset2, len2, &aligned_offset2, &aligned_map_end2, NULL, NULL);

    if (aligned_map_end1 < aligned_offset2 || aligned_map_end2 < aligned_offset1)
    {
        *mapped1 = macho_map_range(fmap, offset1, len1);
        if (*mapped1 != MACHO_NO_MAP)
        {
            *mapped2 = macho_map_range(fmap, offset2, len2);
            if (*mapped2 == MACHO_NO_MAP)
                macho_unmap_range(mapped1, fmap, offset1, len1);
        }
    }
    else
    {
        if (offset1 < offset2)
        {
            *mapped1 = macho_map_range(fmap, offset1, offset2 + len2 - offset1);
            if (*mapped1 != MACHO_NO_MAP)
                *mapped2 = (const char*)*mapped1 + offset2 - offset1;
        }
        else
        {
            *mapped2 = macho_map_range(fmap, offset2, offset1 + len1 - offset2);
            if (*mapped2 != MACHO_NO_MAP)
                *mapped1 = (const char*)*mapped2 + offset1 - offset2;
        }
    }

    TRACE(" => %p, %p\n", *mapped1, *mapped2);

    return (*mapped1 != MACHO_NO_MAP) && (*mapped2 != MACHO_NO_MAP);
}

/******************************************************************
 *              macho_unmap_ranges
 *
 * Unmaps two ranges (offset, length in bytes) of a Mach-O file
 * from memory.  Use for ranges which were mapped by
 * macho_map_ranges.
 */
static void macho_unmap_ranges(const struct macho_file_map* fmap,
                               unsigned offset1, unsigned len1,
                               unsigned offset2, unsigned len2,
                               const void** mapped1, const void** mapped2)
{
    unsigned    aligned_offset1, aligned_map_end1;
    unsigned    aligned_offset2, aligned_map_end2;

    TRACE("(%p/%d, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p/%p, %p/%p)\n", fmap, fmap->fd,
            offset1, len1, offset2, len2, mapped1, *mapped1, mapped2, *mapped2);

    macho_calc_range(fmap, offset1, len1, &aligned_offset1, &aligned_map_end1, NULL, NULL);
    macho_calc_range(fmap, offset2, len2, &aligned_offset2, &aligned_map_end2, NULL, NULL);

    if (aligned_map_end1 < aligned_offset2 || aligned_map_end2 < aligned_offset1)
    {
        macho_unmap_range(mapped1, fmap, offset1, len1);
        macho_unmap_range(mapped2, fmap, offset2, len2);
    }
    else
    {
        if (offset1 < offset2)
        {
            macho_unmap_range(mapped1, fmap, offset1, offset2 + len2 - offset1);
            *mapped2 = MACHO_NO_MAP;
        }
        else
        {
            macho_unmap_range(mapped2, fmap, offset2, offset1 + len1 - offset2);
            *mapped1 = MACHO_NO_MAP;
        }
    }
}

/******************************************************************
 *              macho_map_load_commands
 *
 * Maps the load commands from a Mach-O file into memory
 */
static const struct load_command* macho_map_load_commands(struct macho_file_map* fmap)
{
    if (fmap->load_commands == MACHO_NO_MAP)
    {
        fmap->load_commands = (const struct load_command*) macho_map_range(
                fmap, sizeof(fmap->mach_header), fmap->mach_header.sizeofcmds);
        TRACE("Mapped load commands: %p\n", fmap->load_commands);
    }

    return fmap->load_commands;
}

/******************************************************************
 *              macho_unmap_load_commands
 *
 * Unmaps the load commands of a Mach-O file from memory
 */
static void macho_unmap_load_commands(struct macho_file_map* fmap)
{
    if (fmap->load_commands != MACHO_NO_MAP)
    {
        TRACE("Unmapping load commands: %p\n", fmap->load_commands);
        macho_unmap_range((const void**)&fmap->load_commands, fmap,
                    sizeof(fmap->mach_header), fmap->mach_header.sizeofcmds);
    }
}

/******************************************************************
 *              macho_next_load_command
 *
 * Advance to the next load command
 */
static const struct load_command* macho_next_load_command(const struct load_command* lc)
{
    return (const struct load_command*)((const char*)lc + lc->cmdsize);
}

/******************************************************************
 *              macho_enum_load_commands
 *
 * Enumerates the load commands for a Mach-O file, selecting by
 * the command type, calling a callback for each.  If the callback
 * returns <0, that indicates an error.  If it returns >0, that means
 * it's not interested in getting any more load commands.
 * If this function returns <0, that's an error produced by the
 * callback.  If >=0, that's the count of load commands successfully
 * processed.
 */
static int macho_enum_load_commands(struct macho_file_map* fmap, unsigned cmd,
                                    int (*cb)(struct macho_file_map*, const struct load_command*, void*),
                                    void* user)
{
    const struct load_command* lc;
    int i;
    int count = 0;

    TRACE("(%p/%d, %u, %p, %p)\n", fmap, fmap->fd, cmd, cb, user);

    if ((lc = macho_map_load_commands(fmap)) == MACHO_NO_MAP) return -1;

    TRACE("%d total commands\n", fmap->mach_header.ncmds);

    for (i = 0; i < fmap->mach_header.ncmds; i++, lc = macho_next_load_command(lc))
    {
        int result;

        if (cmd && cmd != lc->cmd) continue;
        count++;

        result = cb(fmap, lc, user);
        TRACE("load_command[%d] (%p), cmd %u; callback => %d\n", i, lc, lc->cmd, result);
        if (result) return (result < 0) ? result : count;
    }

    return count;
}

/******************************************************************
 *              macho_accum_segs_range
 *
 * Callback for macho_enum_load_commands.  Accumulates the address
 * range covered by the segments of a Mach-O file.  All commands
 * are expected to be of LC_SEGMENT type.
 */
static int macho_accum_segs_range(struct macho_file_map* fmap,
                                  const struct load_command* lc, void* user)
{
    const struct segment_command*   sc = (const struct segment_command*)lc;
    unsigned                        tmp, page_mask = sysconf( _SC_PAGESIZE ) - 1;

    TRACE("(%p/%d, %p, %p) before: 0x%08x - 0x%08x\n", fmap, fmap->fd, lc, user,
            (unsigned)fmap->segs_start, (unsigned)fmap->segs_size);
    TRACE("Segment command vm: 0x%08x - 0x%08x\n", (unsigned)sc->vmaddr,
            (unsigned)sc->vmaddr + sc->vmsize);

    if (!strncmp(sc->segname, "WINE_", 5))
    {
        TRACE("Ignoring special Wine segment %s\n", debugstr_an(sc->segname, sizeof(sc->segname)));
        return 0;
    }
    if (!strncmp(sc->segname, "__PAGEZERO", 10))
    {
        TRACE("Ignoring __PAGEZERO segment\n");
        return 0;
    }

    /* If this segment starts before previously-known earliest, record
     * new earliest. */
    if (sc->vmaddr < fmap->segs_start)
        fmap->segs_start = sc->vmaddr;

    /* If this segment extends beyond previously-known furthest, record
     * new furthest. */
    tmp = (sc->vmaddr + sc->vmsize + page_mask) & ~page_mask;
    if (fmap->segs_size < tmp) fmap->segs_size = tmp;

    TRACE("after: 0x%08x - 0x%08x\n", (unsigned)fmap->segs_start, (unsigned)fmap->segs_size);

    return 0;
}

/******************************************************************
 *              macho_map_file
 *
 * Maps a Mach-O file into memory (and checks it's a real Mach-O file)
 */
static BOOL macho_map_file(const WCHAR* filenameW, struct macho_file_map* fmap)
{
    struct fat_header   fat_header;
    struct stat         statbuf;
    int                 i;
    char*               filename;
    unsigned            len;
    BOOL                ret = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(filenameW), fmap);

    fmap->fd = -1;
    fmap->load_commands = MACHO_NO_MAP;
    RtlInitializeBitMap(&fmap->sect_is_code, fmap->sect_is_code_buff, MAX_SECT + 1);

    len = WideCharToMultiByte(CP_UNIXCP, 0, filenameW, -1, NULL, 0, NULL, NULL);
    if (!(filename = HeapAlloc(GetProcessHeap(), 0, len)))
    {
        WARN("failed to allocate filename buffer\n");
        return FALSE;
    }
    WideCharToMultiByte(CP_UNIXCP, 0, filenameW, -1, filename, len, NULL, NULL);

    /* check that the file exists */
    if (stat(filename, &statbuf) == -1 || S_ISDIR(statbuf.st_mode))
    {
        TRACE("stat() failed or %s is directory: %s\n", debugstr_a(filename), strerror(errno));
        goto done;
    }

    /* Now open the file, so that we can mmap() it. */
    if ((fmap->fd = open(filename, O_RDONLY)) == -1)
    {
        TRACE("failed to open file %s: %d\n", debugstr_a(filename), errno);
        goto done;
    }

    if (read(fmap->fd, &fat_header, sizeof(fat_header)) != sizeof(fat_header))
    {
        TRACE("failed to read fat header: %d\n", errno);
        goto done;
    }
    TRACE("... got possible fat header\n");

    /* Fat header is always in big-endian order. */
    if (swap_ulong_be_to_host(fat_header.magic) == FAT_MAGIC)
    {
        int narch = swap_ulong_be_to_host(fat_header.nfat_arch);
        for (i = 0; i < narch; i++)
        {
            struct fat_arch fat_arch;
            if (read(fmap->fd, &fat_arch, sizeof(fat_arch)) != sizeof(fat_arch))
                goto done;
            if (swap_ulong_be_to_host(fat_arch.cputype) == CPU_TYPE_X86)
            {
                fmap->arch_offset = swap_ulong_be_to_host(fat_arch.offset);
                fmap->arch_size = swap_ulong_be_to_host(fat_arch.size);
                break;
            }
        }
        if (i >= narch) goto done;
        TRACE("... found x86 arch\n");
    }
    else
    {
        fmap->arch_offset = 0;
        fmap->arch_size = statbuf.st_size;
        TRACE("... not a fat header\n");
    }

    /* Individual architecture (standalone or within a fat file) is in its native byte order. */
    lseek(fmap->fd, fmap->arch_offset, SEEK_SET);
    if (read(fmap->fd, &fmap->mach_header, sizeof(fmap->mach_header)) != sizeof(fmap->mach_header))
        goto done;
    TRACE("... got possible Mach header\n");
    /* and check for a Mach-O header */
    if (fmap->mach_header.magic != MH_MAGIC ||
        fmap->mach_header.cputype != CPU_TYPE_X86) goto done;
    /* Make sure the file type is one of the ones we expect. */
    switch (fmap->mach_header.filetype)
    {
        case MH_EXECUTE:
        case MH_DYLIB:
        case MH_DYLINKER:
        case MH_BUNDLE:
            break;
        default:
            goto done;
    }
    TRACE("... verified Mach x86 header\n");

    fmap->segs_size = 0;
    fmap->segs_start = ~0L;

    if (macho_enum_load_commands(fmap, LC_SEGMENT, macho_accum_segs_range, NULL) < 0)
        goto done;

    fmap->segs_size -= fmap->segs_start;
    TRACE("segs_start: 0x%08x, segs_size: 0x%08x\n", (unsigned)fmap->segs_start,
            (unsigned)fmap->segs_size);

    ret = TRUE;
done:
    if (!ret)
        macho_unmap_file(fmap);
    HeapFree(GetProcessHeap(), 0, filename);
    return ret;
}

/******************************************************************
 *              macho_unmap_file
 *
 * Unmaps a Mach-O file from memory (previously mapped with macho_map_file)
 */
static void macho_unmap_file(struct macho_file_map* fmap)
{
    TRACE("(%p/%d)\n", fmap, fmap->fd);
    if (fmap->fd != -1)
    {
        macho_unmap_load_commands(fmap);
        close(fmap->fd);
        fmap->fd = -1;
    }
}

/******************************************************************
 *              macho_fill_sect_is_code
 *
 * Callback for macho_enum_load_commands.  Determines which segments
 * of a Mach-O file contain code.  All commands are expected to be
 * of LC_SEGMENT type.
 */
static int macho_fill_sect_is_code(struct macho_file_map* fmap,
                                   const struct load_command* lc, void* user)
{
    const struct segment_command*   sc = (const struct segment_command*)lc;
    const struct section*           sections;
    int*                            cursect = user;
    int                             i;

    TRACE("(%p/%d, %p, %p/%d) scanning %u sections\n", fmap, fmap->fd, lc,
            cursect, *cursect, sc->nsects);

    sections = (const struct section*)(sc + 1);
    for (i = 0; i < sc->nsects; i++)
    {
        if (*cursect > MAX_SECT) return -1;
        (*cursect)++;

        if (!(sections[i].flags & SECTION_TYPE) &&
            (sections[i].flags & (S_ATTR_PURE_INSTRUCTIONS|S_ATTR_SOME_INSTRUCTIONS)))
            RtlSetBits(&fmap->sect_is_code, *cursect, 1);
        else
            RtlClearBits(&fmap->sect_is_code, *cursect, 1);
        TRACE("Section %d (%d of this segment) is%s code\n", *cursect, i,
                (RtlAreBitsSet(&fmap->sect_is_code, *cursect, 1) ? "" : " not"));
    }

    return 0;
}

/******************************************************************
 *              macho_sect_is_code
 *
 * Checks if a section, identified by sectidx which is a 1-based
 * index into the sections of all segments, in order of load
 * commands, contains code.
 */
static BOOL macho_sect_is_code(struct macho_file_map* fmap, unsigned char sectidx)
{
    TRACE("(%p/%d, %u)\n", fmap, fmap->fd, sectidx);

    if (!RtlAreBitsSet(&fmap->sect_is_code, 0, 1))
    {
        int cursect = 0;
        if (macho_enum_load_commands(fmap, LC_SEGMENT, macho_fill_sect_is_code, &cursect) < 0)
            WARN("Couldn't load sect_is_code map\n");
        RtlSetBits(&fmap->sect_is_code, 0, 1);
    }

    return RtlAreBitsSet(&fmap->sect_is_code, sectidx, 1);
}

struct symtab_elt
{
    struct hash_table_elt       ht_elt;
    struct symt_compiland*      compiland;
    unsigned long               addr;
    unsigned char               is_code:1,
                                is_public:1,
                                is_global:1,
                                used:1;
};

struct macho_debug_info
{
    struct macho_file_map*      fmap;
    struct module*              module;
    struct pool                 pool;
    struct hash_table           ht_symtab;
};

/******************************************************************
 *              macho_stabs_def_cb
 *
 * Callback for stabs_parse.  Collect symbol definitions.
 */
static void macho_stabs_def_cb(struct module* module, unsigned long load_offset,
                               const char* name, unsigned long offset,
                               BOOL is_public, BOOL is_global, unsigned char sectidx,
                               struct symt_compiland* compiland, void* user)
{
    struct macho_debug_info*    mdi = user;
    struct symtab_elt*          ste;

    TRACE("(%p, 0x%08lx, %s, 0x%08lx, %d, %d, %u, %p, %p/%p/%d)\n", module, load_offset,
            debugstr_a(name), offset, is_public, is_global, sectidx,
            compiland, mdi, mdi->fmap, mdi->fmap->fd);

    /* Defer the creation of new non-debugging symbols until after we've
     * finished parsing the stabs. */
    ste                 = pool_alloc(&mdi->pool, sizeof(*ste));
    ste->ht_elt.name    = pool_strdup(&mdi->pool, name);
    ste->compiland      = compiland;
    ste->addr           = load_offset + offset;
    ste->is_code        = !!macho_sect_is_code(mdi->fmap, sectidx);
    ste->is_public      = !!is_public;
    ste->is_global      = !!is_global;
    ste->used           = 0;
    hash_table_add(&mdi->ht_symtab, &ste->ht_elt);
}

/******************************************************************
 *              macho_parse_symtab
 *
 * Callback for macho_enum_load_commands.  Processes the LC_SYMTAB
 * load commands from the Mach-O file.
 */
static int macho_parse_symtab(struct macho_file_map* fmap,
                              const struct load_command* lc, void* user)
{
    const struct symtab_command*    sc = (const struct symtab_command*)lc;
    struct macho_debug_info*        mdi = user;
    const struct nlist*             stab;
    const char*                     stabstr;
    int                             ret = 0;

    TRACE("(%p/%d, %p, %p) %u syms at 0x%08x, strings 0x%08x - 0x%08x\n", fmap, fmap->fd, lc,
            user, sc->nsyms, sc->symoff, sc->stroff, sc->stroff + sc->strsize);

    if (!macho_map_ranges(fmap, sc->symoff, sc->nsyms * sizeof(struct nlist),
            sc->stroff, sc->strsize, (const void**)&stab, (const void**)&stabstr))
        return 0;

    if (!stabs_parse(mdi->module,
                     mdi->module->format_info[DFI_MACHO]->u.macho_info->load_addr - fmap->segs_start,
                     stab, sc->nsyms * sizeof(struct nlist),
                     stabstr, sc->strsize, macho_stabs_def_cb, mdi))
        ret = -1;

    macho_unmap_ranges(fmap, sc->symoff, sc->nsyms * sizeof(struct nlist),
            sc->stroff, sc->strsize, (const void**)&stab, (const void**)&stabstr);

    return ret;
}

/******************************************************************
 *              macho_finish_stabs
 *
 * Integrate the non-debugging symbols we've gathered into the
 * symbols that were generated during stabs parsing.
 */
static void macho_finish_stabs(struct module* module, struct hash_table* ht_symtab)
{
    struct hash_table_iter      hti_ours;
    struct symtab_elt*          ste;
    BOOL                        adjusted = FALSE;

    TRACE("(%p, %p)\n", module, ht_symtab);

    /* For each of our non-debugging symbols, see if it can provide some
     * missing details to one of the module's known symbols. */
    hash_table_iter_init(ht_symtab, &hti_ours, NULL);
    while ((ste = hash_table_iter_up(&hti_ours)))
    {
        struct hash_table_iter  hti_modules;
        void*                   ptr;
        struct symt_ht*         sym;
        struct symt_function*   func;
        struct symt_data*       data;

        hash_table_iter_init(&module->ht_symbols, &hti_modules, ste->ht_elt.name);
        while ((ptr = hash_table_iter_up(&hti_modules)))
        {
            sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);

            if (strcmp(sym->hash_elt.name, ste->ht_elt.name))
                continue;

            switch (sym->symt.tag)
            {
            case SymTagFunction:
                func = (struct symt_function*)sym;
                if (func->address == module->format_info[DFI_MACHO]->u.macho_info->load_addr)
                {
                    TRACE("Adjusting function %p/%s!%s from 0x%08lx to 0x%08lx\n", func,
                          debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                          func->address, ste->addr);
                    func->address = ste->addr;
                    adjusted = TRUE;
                }
                if (func->address == ste->addr)
                    ste->used = 1;
                break;
            case SymTagData:
                data = (struct symt_data*)sym;
                switch (data->kind)
                {
                case DataIsGlobal:
                case DataIsFileStatic:
                    if (data->u.var.offset == module->format_info[DFI_MACHO]->u.macho_info->load_addr)
                    {
                        TRACE("Adjusting data symbol %p/%s!%s from 0x%08lx to 0x%08lx\n",
                              data, debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                              data->u.var.offset, ste->addr);
                        data->u.var.offset = ste->addr;
                        adjusted = TRUE;
                    }
                    if (data->u.var.offset == ste->addr)
                    {
                        enum DataKind new_kind;

                        new_kind = ste->is_global ? DataIsGlobal : DataIsFileStatic;
                        if (data->kind != new_kind)
                        {
                            WARN("Changing kind for %p/%s!%s from %d to %d\n", sym,
                                 debugstr_w(module->module.ModuleName), sym->hash_elt.name,
                                 (int)data->kind, (int)new_kind);
                            data->kind = new_kind;
                            adjusted = TRUE;
                        }
                        ste->used = 1;
                    }
                    break;
                default:;
                }
                break;
            default:
                TRACE("Ignoring tag %u\n", sym->symt.tag);
                break;
            }
        }
    }

    if (adjusted)
    {
        /* since we may have changed some addresses, mark the module to be resorted */
        module->sortlist_valid = FALSE;
    }

    /* Mark any of our non-debugging symbols which fall on an already-used
     * address as "used".  This allows us to skip them in the next loop,
     * below.  We do this in separate loops because symt_new_* marks the
     * list as needing sorting and symt_find_nearest sorts if needed,
     * causing thrashing. */
    if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        hash_table_iter_init(ht_symtab, &hti_ours, NULL);
        while ((ste = hash_table_iter_up(&hti_ours)))
        {
            struct symt_ht* sym;
            ULONG64         addr;

            if (ste->used) continue;

            sym = symt_find_nearest(module, ste->addr);
            if (sym)
                symt_get_address(&sym->symt, &addr);
            if (sym && ste->addr == addr)
            {
                ULONG64 size = 0;
                DWORD   kind = -1;

                ste->used = 1;

                /* If neither symbol has a correct size (ours never does), we
                 * consider them both to be markers.  No warning is needed in
                 * that case.
                 * Also, we check that we don't have two symbols, one local, the other
                 * global, which is legal.
                 */
                symt_get_info(module, &sym->symt, TI_GET_LENGTH,   &size);
                symt_get_info(module, &sym->symt, TI_GET_DATAKIND, &kind);
                if (size && kind == (ste->is_global ? DataIsGlobal : DataIsFileStatic))
                    FIXME("Duplicate in %s: %s<%08lx> %s<%s-%s>\n",
                          debugstr_w(module->module.ModuleName),
                          ste->ht_elt.name, ste->addr,
                          sym->hash_elt.name,
                          wine_dbgstr_longlong(addr), wine_dbgstr_longlong(size));
            }
        }
    }

    /* For any of our remaining non-debugging symbols which have no match
     * among the module's known symbols, add them as new symbols. */
    hash_table_iter_init(ht_symtab, &hti_ours, NULL);
    while ((ste = hash_table_iter_up(&hti_ours)))
    {
        if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY) && !ste->used)
        {
            if (ste->is_code)
            {
                symt_new_function(module, ste->compiland, ste->ht_elt.name,
                    ste->addr, 0, NULL);
            }
            else
            {
                struct location loc;

                loc.kind = loc_absolute;
                loc.reg = 0;
                loc.offset = ste->addr;
                symt_new_global_variable(module, ste->compiland, ste->ht_elt.name,
                                         !ste->is_global, loc, 0, NULL);
            }

            ste->used = 1;
        }

        if (ste->is_public && !(dbghelp_options & SYMOPT_NO_PUBLICS))
        {
            symt_new_public(module, ste->compiland, ste->ht_elt.name, ste->addr, 0);
        }
    }
}

/******************************************************************
 *              macho_load_debug_info_from_map
 *
 * Loads the symbolic information from a Mach-O module.
 * Returns
 *      FALSE if the file doesn't contain symbolic info (or this info
 *              cannot be read or parsed)
 *      TRUE on success
 */
static BOOL macho_load_debug_info_from_map(struct module* module,
                                           struct macho_file_map* fmap)
{
    BOOL                    ret = FALSE;
    struct macho_debug_info mdi;
    int                     result;

    TRACE("(%p, %p/%d)\n", module, fmap, fmap->fd);

    module->module.SymType = SymExport;

    mdi.fmap = fmap;
    mdi.module = module;
    pool_init(&mdi.pool, 65536);
    hash_table_init(&mdi.pool, &mdi.ht_symtab, 256);
    result = macho_enum_load_commands(fmap, LC_SYMTAB, macho_parse_symtab, &mdi);
    if (result > 0)
        ret = TRUE;
    else if (result < 0)
        WARN("Couldn't correctly read stabs\n");

    macho_finish_stabs(module, &mdi.ht_symtab);

    pool_destroy(&mdi.pool);
    return ret;
}

/******************************************************************
 *              macho_load_debug_info
 *
 * Loads Mach-O debugging information from the module image file.
 */
BOOL macho_load_debug_info(struct module* module, struct macho_file_map* fmap)
{
    BOOL                    ret = TRUE;
    struct macho_file_map   my_fmap;

    TRACE("(%p, %p/%d)\n", module, fmap, fmap ? fmap->fd : -1);

    if (module->type != DMT_MACHO || !module->format_info[DFI_MACHO]->u.macho_info)
    {
        ERR("Bad Mach-O module '%s'\n", debugstr_w(module->module.LoadedImageName));
        return FALSE;
    }

    if (!fmap)
    {
        fmap = &my_fmap;
        ret = macho_map_file(module->module.LoadedImageName, fmap);
    }
    if (ret)
        ret = macho_load_debug_info_from_map(module, fmap);

    if (fmap == &my_fmap) macho_unmap_file(fmap);
    return ret;
}

/******************************************************************
 *              macho_fetch_file_info
 *
 * Gathers some more information for a Mach-O module from a given file
 */
BOOL macho_fetch_file_info(const WCHAR* name, DWORD_PTR* base,
                           DWORD* size, DWORD* checksum)
{
    struct macho_file_map fmap;

    TRACE("(%s, %p, %p, %p)\n", debugstr_w(name), base, size, checksum);

    if (!macho_map_file(name, &fmap)) return FALSE;
    if (base) *base = fmap.segs_start;
    *size = fmap.segs_size;
    *checksum = calc_crc32(fmap.fd);
    macho_unmap_file(&fmap);
    return TRUE;
}

/******************************************************************
 *              macho_load_file
 *
 * Loads the information for Mach-O module stored in 'filename'.
 * The module has been loaded at 'load_addr' address.
 * returns
 *      FALSE if the file cannot be found/opened or if the file doesn't
 *              contain symbolic info (or this info cannot be read or parsed)
 *      TRUE on success
 */
static BOOL macho_load_file(struct process* pcs, const WCHAR* filename,
                            unsigned long load_addr, struct macho_info* macho_info)
{
    BOOL                    ret = TRUE;
    struct macho_file_map   fmap;

    TRACE("(%p/%p, %s, 0x%08lx, %p/0x%08x)\n", pcs, pcs->handle, debugstr_w(filename),
            load_addr, macho_info, macho_info->flags);

    if (!macho_map_file(filename, &fmap)) return FALSE;

    /* Find the dynamic loader's table of images loaded into the process.
     */
    if (macho_info->flags & MACHO_INFO_DEBUG_HEADER)
    {
        PROCESS_BASIC_INFORMATION pbi;
        NTSTATUS status;

        ret = FALSE;

        /* Get address of PEB */
        status = NtQueryInformationProcess(pcs->handle, ProcessBasicInformation,
                                           &pbi, sizeof(pbi), NULL);
        if (status == STATUS_SUCCESS)
        {
            ULONG_PTR dyld_image_info;

            /* Read dyld image info address from PEB */
            if (ReadProcessMemory(pcs->handle, &pbi.PebBaseAddress->Reserved[0],
                                  &dyld_image_info, sizeof(dyld_image_info), NULL))
            {
                TRACE("got dyld_image_info 0x%08x from PEB %p MacDyldImageInfo %p\n",
                      dyld_image_info, pbi.PebBaseAddress, &pbi.PebBaseAddress->Reserved);
                macho_info->dbg_hdr_addr = dyld_image_info;
                ret = TRUE;
            }
        }

#ifndef __LP64__ /* No reading the symtab with nlist(3) in LP64 */
        if (!ret)
        {
            static void* dyld_all_image_infos_addr;

            /* Our next best guess is that dyld was loaded at its base address
               and we can find the dyld image infos address by looking up its symbol. */
            if (!dyld_all_image_infos_addr)
            {
                struct nlist nl[2];
                memset(nl, 0, sizeof(nl));
                nl[0].n_un.n_name = (char*)"_dyld_all_image_infos";
                if (!nlist("/usr/lib/dyld", nl))
                    dyld_all_image_infos_addr = (void*)nl[0].n_value;
            }

            if (dyld_all_image_infos_addr)
            {
                TRACE("got dyld_image_info %p from /usr/lib/dyld symbol table\n",
                      dyld_all_image_infos_addr);
                macho_info->dbg_hdr_addr = (unsigned long)dyld_all_image_infos_addr;
                ret = TRUE;
            }
        }
#endif
    }

    if (macho_info->flags & MACHO_INFO_MODULE)
    {
        struct macho_module_info *macho_module_info;
        struct module_format*   modfmt =
            HeapAlloc(GetProcessHeap(), 0, sizeof(struct module_format) + sizeof(struct macho_module_info));
        if (!modfmt) goto leave;
        if (!load_addr)
            load_addr = fmap.segs_start;
        macho_info->module = module_new(pcs, filename, DMT_MACHO, FALSE, load_addr,
                                        fmap.segs_size, 0, calc_crc32(fmap.fd));
        if (!macho_info->module)
        {
            HeapFree(GetProcessHeap(), 0, modfmt);
            goto leave;
        }
        macho_module_info = (void*)(modfmt + 1);
        macho_info->module->format_info[DFI_MACHO] = modfmt;

        modfmt->module       = macho_info->module;
        modfmt->remove       = NULL;
        modfmt->loc_compute  = NULL;
        modfmt->u.macho_info = macho_module_info;

        macho_module_info->load_addr = load_addr;

        if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
            macho_info->module->module.SymType = SymDeferred;
        else if (!macho_load_debug_info(macho_info->module, &fmap))
            ret = FALSE;

        macho_info->module->format_info[DFI_MACHO]->u.macho_info->in_use = 1;
        macho_info->module->format_info[DFI_MACHO]->u.macho_info->is_loader = 0;
        TRACE("module = %p\n", macho_info->module);
    }

    if (macho_info->flags & MACHO_INFO_NAME)
    {
        WCHAR*  ptr;
        ptr = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(filename) + 1) * sizeof(WCHAR));
        if (ptr)
        {
            strcpyW(ptr, filename);
            macho_info->module_name = ptr;
        }
        else ret = FALSE;
        TRACE("module_name = %p %s\n", macho_info->module_name, debugstr_w(macho_info->module_name));
    }
leave:
    macho_unmap_file(&fmap);

    TRACE(" => %d\n", ret);
    return ret;
}

/******************************************************************
 *              macho_load_file_from_path
 * Tries to load a Mach-O file from a set of paths (separated by ':')
 */
static BOOL macho_load_file_from_path(struct process* pcs,
                                      const WCHAR* filename,
                                      unsigned long load_addr,
                                      const char* path,
                                      struct macho_info* macho_info)
{
    BOOL                ret = FALSE;
    WCHAR               *s, *t, *fn;
    WCHAR*              pathW = NULL;
    unsigned            len;

    TRACE("(%p/%p, %s, 0x%08lx, %s, %p)\n", pcs, pcs->handle, debugstr_w(filename), load_addr,
            debugstr_a(path), macho_info);

    if (!path) return FALSE;

    len = MultiByteToWideChar(CP_UNIXCP, 0, path, -1, NULL, 0);
    pathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!pathW) return FALSE;
    MultiByteToWideChar(CP_UNIXCP, 0, path, -1, pathW, len);

    for (s = pathW; s && *s; s = (t) ? (t+1) : NULL)
    {
        t = strchrW(s, ':');
        if (t) *t = '\0';
        fn = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(filename) + 1 + lstrlenW(s) + 1) * sizeof(WCHAR));
        if (!fn) break;
        strcpyW(fn, s);
        strcatW(fn, S_SlashW);
        strcatW(fn, filename);
        ret = macho_load_file(pcs, fn, load_addr, macho_info);
        HeapFree(GetProcessHeap(), 0, fn);
        if (ret) break;
        s = (t) ? (t+1) : NULL;
    }

    TRACE(" => %d\n", ret);
    HeapFree(GetProcessHeap(), 0, pathW);
    return ret;
}

/******************************************************************
 *              macho_load_file_from_dll_path
 *
 * Tries to load a Mach-O file from the dll path
 */
static BOOL macho_load_file_from_dll_path(struct process* pcs,
                                          const WCHAR* filename,
                                          unsigned long load_addr,
                                          struct macho_info* macho_info)
{
    BOOL ret = FALSE;
    unsigned int index = 0;
    const char *path;

    TRACE("(%p/%p, %s, 0x%08lx, %p)\n", pcs, pcs->handle, debugstr_w(filename), load_addr,
            macho_info);

    while (!ret && (path = wine_dll_enum_load_path( index++ )))
    {
        WCHAR *name;
        unsigned len;

        len = MultiByteToWideChar(CP_UNIXCP, 0, path, -1, NULL, 0);

        name = HeapAlloc( GetProcessHeap(), 0,
                          (len + lstrlenW(filename) + 2) * sizeof(WCHAR) );

        if (!name) break;
        MultiByteToWideChar(CP_UNIXCP, 0, path, -1, name, len);
        strcatW( name, S_SlashW );
        strcatW( name, filename );
        ret = macho_load_file(pcs, name, load_addr, macho_info);
        HeapFree( GetProcessHeap(), 0, name );
    }
    TRACE(" => %d\n", ret);
    return ret;
}

/******************************************************************
 *              macho_search_and_load_file
 *
 * Lookup a file in standard Mach-O locations, and if found, load it
 */
static BOOL macho_search_and_load_file(struct process* pcs, const WCHAR* filename,
                                       unsigned long load_addr,
                                       struct macho_info* macho_info)
{
    BOOL                ret = FALSE;
    struct module*      module;
    static const WCHAR  S_libstdcPPW[] = {'l','i','b','s','t','d','c','+','+','\0'};
    const WCHAR*        p;

    TRACE("(%p/%p, %s, 0x%08lx, %p)\n", pcs, pcs->handle, debugstr_w(filename), load_addr,
            macho_info);

    if (filename == NULL || *filename == '\0') return FALSE;
    if ((module = module_is_already_loaded(pcs, filename)))
    {
        macho_info->module = module;
        module->format_info[DFI_MACHO]->u.macho_info->in_use = 1;
        return module->module.SymType;
    }

    if (strstrW(filename, S_libstdcPPW)) return FALSE; /* We know we can't do it */

    /* If has no directories, try LD_LIBRARY_PATH first. */
    if (!strchrW(filename, '/'))
    {
        ret = macho_load_file_from_path(pcs, filename, load_addr,
                                      getenv("PATH"), macho_info);
    }
    /* Try DYLD_LIBRARY_PATH, with just the filename (no directories). */
    if (!ret)
    {
        if ((p = strrchrW(filename, '/'))) p++;
        else p = filename;
        ret = macho_load_file_from_path(pcs, p, load_addr,
                                      getenv("DYLD_LIBRARY_PATH"), macho_info);
    }
    /* Try the path as given. */
    if (!ret)
        ret = macho_load_file(pcs, filename, load_addr, macho_info);
    /* Try DYLD_FALLBACK_LIBRARY_PATH, with just the filename (no directories). */
    if (!ret)
    {
        ret = macho_load_file_from_path(pcs, p, load_addr,
                                      getenv("DYLD_FALLBACK_LIBRARY_PATH"), macho_info);
    }
    if (!ret && !strchrW(filename, '/'))
        ret = macho_load_file_from_dll_path(pcs, filename, load_addr, macho_info);

    return ret;
}

/******************************************************************
 *              macho_enum_modules_internal
 *
 * Enumerate Mach-O modules from a running process
 */
static BOOL macho_enum_modules_internal(const struct process* pcs,
                                        const WCHAR* main_name,
                                        enum_modules_cb cb, void* user)
{
    struct dyld_all_image_infos image_infos;
    struct dyld_image_info*     info_array = NULL;
    unsigned long               len;
    int                         i;
    char                        bufstr[256];
    WCHAR                       bufstrW[MAX_PATH];
    BOOL                        ret = FALSE;

    TRACE("(%p/%p, %s, %p, %p)\n", pcs, pcs->handle, debugstr_w(main_name), cb,
            user);

    if (!pcs->dbg_hdr_addr ||
        !ReadProcessMemory(pcs->handle, (void*)pcs->dbg_hdr_addr,
                           &image_infos, sizeof(image_infos), NULL) ||
        !image_infos.infoArray)
        goto done;
    TRACE("Process has %u image infos at %p\n", image_infos.infoArrayCount, image_infos.infoArray);

    len = image_infos.infoArrayCount * sizeof(info_array[0]);
    info_array = HeapAlloc(GetProcessHeap(), 0, len);
    if (!info_array ||
        !ReadProcessMemory(pcs->handle, image_infos.infoArray,
                           info_array, len, NULL))
        goto done;
    TRACE("... read image infos\n");

    for (i = 0; i < image_infos.infoArrayCount; i++)
    {
        if (info_array[i].imageFilePath != NULL &&
            ReadProcessMemory(pcs->handle, info_array[i].imageFilePath, bufstr, sizeof(bufstr), NULL))
        {
            bufstr[sizeof(bufstr) - 1] = '\0';
            TRACE("[%d] image file %s\n", i, debugstr_a(bufstr));
            MultiByteToWideChar(CP_UNIXCP, 0, bufstr, -1, bufstrW, sizeof(bufstrW) / sizeof(WCHAR));
            if (main_name && !bufstrW[0]) strcpyW(bufstrW, main_name);
            if (!cb(bufstrW, (unsigned long)info_array[i].imageLoadAddress, user)) break;
        }
    }

    ret = TRUE;
done:
    HeapFree(GetProcessHeap(), 0, info_array);
    return ret;
}

struct macho_sync
{
    struct process*     pcs;
    struct macho_info   macho_info;
};

static BOOL macho_enum_sync_cb(const WCHAR* name, unsigned long addr, void* user)
{
    struct macho_sync*  ms = user;

    TRACE("(%s, 0x%08lx, %p)\n", debugstr_w(name), addr, user);
    macho_search_and_load_file(ms->pcs, name, addr, &ms->macho_info);
    return TRUE;
}

/******************************************************************
 *              macho_synchronize_module_list
 *
 * Rescans the debuggee's modules list and synchronizes it with
 * the one from 'pcs', ie:
 * - if a module is in debuggee and not in pcs, it's loaded into pcs
 * - if a module is in pcs and not in debuggee, it's unloaded from pcs
 */
BOOL    macho_synchronize_module_list(struct process* pcs)
{
    struct module*      module;
    struct macho_sync     ms;

    TRACE("(%p/%p)\n", pcs, pcs->handle);

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_MACHO && !module->is_virtual)
            module->format_info[DFI_MACHO]->u.macho_info->in_use = 0;
    }

    ms.pcs = pcs;
    ms.macho_info.flags = MACHO_INFO_MODULE;
    if (!macho_enum_modules_internal(pcs, NULL, macho_enum_sync_cb, &ms))
        return FALSE;

    module = pcs->lmodules;
    while (module)
    {
        if (module->type == DMT_MACHO && !module->is_virtual &&
            !module->format_info[DFI_MACHO]->u.macho_info->in_use &&
            !module->format_info[DFI_MACHO]->u.macho_info->is_loader)
        {
            module_remove(pcs, module);
            /* restart all over */
            module = pcs->lmodules;
        }
        else module = module->next;
    }
    return TRUE;
}

/******************************************************************
 *              macho_search_loader
 *
 * Lookup in a running Mach-O process the loader, and sets its Mach-O link
 * address (for accessing the list of loaded images) in pcs.
 * If flags is MACHO_INFO_MODULE, the module for the loader is also
 * added as a module into pcs.
 */
static BOOL macho_search_loader(struct process* pcs, struct macho_info* macho_info)
{
    return macho_search_and_load_file(pcs, get_wine_loader_name(), 0, macho_info);
}

/******************************************************************
 *              macho_read_wine_loader_dbg_info
 *
 * Try to find a decent wine executable which could have loaded the debuggee
 */
BOOL macho_read_wine_loader_dbg_info(struct process* pcs)
{
    struct macho_info     macho_info;

    TRACE("(%p/%p)\n", pcs, pcs->handle);
    macho_info.flags = MACHO_INFO_DEBUG_HEADER | MACHO_INFO_MODULE;
    if (!macho_search_loader(pcs, &macho_info)) return FALSE;
    macho_info.module->format_info[DFI_MACHO]->u.macho_info->is_loader = 1;
    module_set_module(macho_info.module, S_WineLoaderW);
    return (pcs->dbg_hdr_addr = macho_info.dbg_hdr_addr) != 0;
}

/******************************************************************
 *              macho_enum_modules
 *
 * Enumerates the Mach-O loaded modules from a running target (hProc)
 * This function doesn't require that someone has called SymInitialize
 * on this very process.
 */
BOOL macho_enum_modules(HANDLE hProc, enum_modules_cb cb, void* user)
{
    struct process      pcs;
    struct macho_info   macho_info;
    BOOL                ret;

    TRACE("(%p, %p, %p)\n", hProc, cb, user);
    memset(&pcs, 0, sizeof(pcs));
    pcs.handle = hProc;
    macho_info.flags = MACHO_INFO_DEBUG_HEADER | MACHO_INFO_NAME;
    if (!macho_search_loader(&pcs, &macho_info)) return FALSE;
    pcs.dbg_hdr_addr = macho_info.dbg_hdr_addr;
    ret = macho_enum_modules_internal(&pcs, macho_info.module_name, cb, user);
    HeapFree(GetProcessHeap(), 0, (char*)macho_info.module_name);
    return ret;
}

struct macho_load
{
    struct process*     pcs;
    struct macho_info   macho_info;
    const WCHAR*        name;
    BOOL                ret;
};

/******************************************************************
 *              macho_load_cb
 *
 * Callback for macho_load_module, used to walk the list of loaded
 * modules.
 */
static BOOL macho_load_cb(const WCHAR* name, unsigned long addr, void* user)
{
    struct macho_load*  ml = user;
    const WCHAR*        p;

    TRACE("(%s, 0x%08lx, %p)\n", debugstr_w(name), addr, user);

    /* memcmp is needed for matches when bufstr contains also version information
     * ml->name: libc.so, name: libc.so.6.0
     */
    p = strrchrW(name, '/');
    if (!p++) p = name;
    if (!memcmp(p, ml->name, lstrlenW(ml->name) * sizeof(WCHAR)))
    {
        ml->ret = macho_search_and_load_file(ml->pcs, name, addr, &ml->macho_info);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *              macho_load_module
 *
 * Loads a Mach-O module and stores it in process' module list.
 * Also, find module real name and load address from
 * the real loaded modules list in pcs address space.
 */
struct module*  macho_load_module(struct process* pcs, const WCHAR* name, unsigned long addr)
{
    struct macho_load   ml;

    TRACE("(%p/%p, %s, 0x%08lx)\n", pcs, pcs->handle, debugstr_w(name), addr);

    ml.macho_info.flags = MACHO_INFO_MODULE;
    ml.ret = FALSE;

    if (pcs->dbg_hdr_addr) /* we're debugging a live target */
    {
        ml.pcs = pcs;
        /* do only the lookup from the filename, not the path (as we lookup module
         * name in the process' loaded module list)
         */
        ml.name = strrchrW(name, '/');
        if (!ml.name++) ml.name = name;
        ml.ret = FALSE;

        if (!macho_enum_modules_internal(pcs, NULL, macho_load_cb, &ml))
            return NULL;
    }
    else if (addr)
    {
        ml.name = name;
        ml.ret = macho_search_and_load_file(pcs, ml.name, addr, &ml.macho_info);
    }
    if (!ml.ret) return NULL;
    assert(ml.macho_info.module);
    return ml.macho_info.module;
}

#else  /* HAVE_MACH_O_LOADER_H */

BOOL    macho_synchronize_module_list(struct process* pcs)
{
    return FALSE;
}

BOOL macho_fetch_file_info(const WCHAR* name, DWORD_PTR* base,
                           DWORD* size, DWORD* checksum)
{
    return FALSE;
}

BOOL macho_read_wine_loader_dbg_info(struct process* pcs)
{
    return FALSE;
}

BOOL macho_enum_modules(HANDLE hProc, enum_modules_cb cb, void* user)
{
    return FALSE;
}

struct module*  macho_load_module(struct process* pcs, const WCHAR* name, unsigned long addr)
{
    return NULL;
}

BOOL macho_load_debug_info(struct module* module, struct macho_file_map* fmap)
{
    return FALSE;
}
#endif  /* HAVE_MACH_O_LOADER_H */
