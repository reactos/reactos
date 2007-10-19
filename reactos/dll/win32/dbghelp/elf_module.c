/*
 * File elf.c - processing of ELF files
 *
 * Copyright (C) 1996, Eric Youngdale.
 *		 1999-2004 Eric Pouech
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
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include "dbghelp_private.h"

#if defined(__svr4__) || defined(__sun)
#define __ELF__
#endif

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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

struct elf_module_info
{
    unsigned long               elf_addr;
    unsigned short	        elf_mark : 1,
                                elf_loader : 1;
};

#ifdef __ELF__

#define ELF_INFO_DEBUG_HEADER   0x0001
#define ELF_INFO_MODULE         0x0002

struct elf_info
{
    unsigned                    flags;          /* IN  one (or several) of the ELF_INFO constants */
    unsigned long               dbg_hdr_addr;   /* OUT address of debug header (if ELF_INFO_DEBUG_HEADER is set) */
    struct module*              module;         /* OUT loaded module (if ELF_INFO_MODULE is set) */
};

struct symtab_elt
{
    struct hash_table_elt       ht_elt;
    const Elf32_Sym*            symp;
    const char*                 filename;
    unsigned                    used;
};

struct thunk_area
{
    const char*                 symname;
    THUNK_ORDINAL               ordinal;
    unsigned long               rva_start;
    unsigned long               rva_end;
};

/******************************************************************
 *		elf_hash_symtab
 *
 * creating an internal hash table to ease use ELF symtab information lookup
 */
static void elf_hash_symtab(const struct module* module, struct pool* pool,
                            struct hash_table* ht_symtab, const char* map_addr,
                            const Elf32_Shdr* symtab, const Elf32_Shdr* strtab,
                            unsigned num_areas, struct thunk_area* thunks)
{
    int		                i, j, nsym;
    const char*                 strp;
    const char*                 symname;
    const char*                 filename = NULL;
    const char*                 ptr;
    const Elf32_Sym*            symp;
    struct symtab_elt*          ste;

    symp = (const Elf32_Sym*)(map_addr + symtab->sh_offset);
    nsym = symtab->sh_size / sizeof(*symp);
    strp = (const char*)(map_addr + strtab->sh_offset);

    for (j = 0; j < num_areas; j++)
        thunks[j].rva_start = thunks[j].rva_end = 0;

    for (i = 0; i < nsym; i++, symp++)
    {
        /* Ignore certain types of entries which really aren't of that much
         * interest.
         */
        if (ELF32_ST_TYPE(symp->st_info) == STT_SECTION || symp->st_shndx == SHN_UNDEF)
        {
            continue;
        }

        symname = strp + symp->st_name;

        if (ELF32_ST_TYPE(symp->st_info) == STT_FILE)
        {
            filename = symname;
            continue;
        }
        for (j = 0; j < num_areas; j++)
        {
            if (!strcmp(symname, thunks[j].symname))
            {
                thunks[j].rva_start = symp->st_value;
                thunks[j].rva_end   = symp->st_value + symp->st_size;
                break;
            }
        }
        if (j < num_areas) continue;

        /* FIXME: we don't need to handle them (GCC internals)
         * Moreover, they screw up our symbol lookup :-/
         */
        if (symname[0] == '.' && symname[1] == 'L' && isdigit(symname[2]))
            continue;

        ste = pool_alloc(pool, sizeof(*ste));
        /* GCC seems to emit, in some cases, a .<digit>+ suffix.
         * This is used for static variable inside functions, so
         * that we can have several such variables with same name in
         * the same compilation unit
         * We simply ignore that suffix when present (we also get rid
         * of it in stabs parsing)
         */
        ptr = symname + strlen(symname) - 1;
        ste->ht_elt.name = symname;
        if (isdigit(*ptr))
        {
            while (*ptr >= '0' && *ptr <= '9' && ptr >= symname) ptr--;
            if (ptr > symname && *ptr == '.')
            {
                char* n = pool_alloc(pool, ptr - symname + 1);
                memcpy(n, symname, ptr - symname + 1);
                n[ptr - symname] = '\0';
                ste->ht_elt.name = n;
            }
        }
        ste->symp        = symp;
        ste->filename    = filename;
        ste->used        = 0;
        hash_table_add(ht_symtab, &ste->ht_elt);
    }
}

/******************************************************************
 *		elf_lookup_symtab
 *
 * lookup a symbol by name in our internal hash table for the symtab
 */
static const Elf32_Sym* elf_lookup_symtab(const struct module* module,
                                          const struct hash_table* ht_symtab,
                                          const char* name, struct symt* compiland)
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
                                    ((struct symt_compiland*)compiland)->source);
        compiland_basename = strrchr(compiland_name, '/');
        if (!compiland_basename++) compiland_basename = compiland_name;
    }
    else compiland_name = compiland_basename = NULL;

    hash_table_iter_init(ht_symtab, &hti, name);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used || strcmp(ste->ht_elt.name, name)) continue;

        weak_result = ste;
        if ((ste->filename && !compiland_name) || (!ste->filename && compiland_name))
            continue;
        if (ste->filename && compiland_name)
        {
            if (strcmp(ste->filename, compiland_name))
            {
                base = strrchr(ste->filename, '/');
                if (!base++) base = ste->filename;
                if (strcmp(base, compiland_basename)) continue;
            }
        }
        if (result)
        {
            FIXME("Already found symbol %s (%s) in symtab %s @%08x and %s @%08x\n",
                  name, compiland_name, result->filename, result->symp->st_value,
                  ste->filename, ste->symp->st_value);
        }
        else
        {
            result = ste;
            ste->used = 1;
        }
    }
    if (!result && !(result = weak_result))
    {
        FIXME("Couldn't find symbol %s.%s in symtab\n",
              module->module.ModuleName, name);
        return NULL;
    }
    return result->symp;
}

/******************************************************************
 *		elf_finish_stabs_info
 *
 * - get any relevant information (address & size) from the bits we got from the
 *   stabs debugging information
 */
static void elf_finish_stabs_info(struct module* module, struct hash_table* symtab)
{
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym;
    const Elf32_Sym*            symp;

    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        switch (sym->symt.tag)
        {
        case SymTagFunction:
            if (((struct symt_function*)sym)->address != module->elf_info->elf_addr &&
                ((struct symt_function*)sym)->size)
            {
                break;
            }
            symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name,
                                     ((struct symt_function*)sym)->container);
            if (symp)
            {
                ((struct symt_function*)sym)->address = module->elf_info->elf_addr +
                                                        symp->st_value;
                ((struct symt_function*)sym)->size    = symp->st_size;
            } else FIXME("Couldn't find %s\n", sym->hash_elt.name);
            break;
        case SymTagData:
            switch (((struct symt_data*)sym)->kind)
            {
            case DataIsGlobal:
            case DataIsFileStatic:
                if (((struct symt_data*)sym)->u.address != module->elf_info->elf_addr)
                    break;
                symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name,
                                         ((struct symt_data*)sym)->container);
                if (symp)
                {
                    ((struct symt_data*)sym)->u.address = module->elf_info->elf_addr +
                                                          symp->st_value;
                    ((struct symt_data*)sym)->kind = (ELF32_ST_BIND(symp->st_info) == STB_LOCAL) ?
                        DataIsFileStatic : DataIsGlobal;
                }
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
static int elf_new_wine_thunks(struct module* module, struct hash_table* ht_symtab,
                               unsigned num_areas, struct thunk_area* thunks)
{
    int		                j;
    struct symt_compiland*      compiland = NULL;
    const char*                 compiland_name = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;
    DWORD                       addr;
    int                         idx;

    hash_table_iter_init(ht_symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used) continue;

        /* FIXME: this is not a good idea anyway... we are creating several
         * compiland objects for a same compilation unit
         * We try to cache the last compiland used, but it's not enough
         * (we should here only create compilands if they are not yet
         *  defined)
         */
        if (!compiland_name || compiland_name != ste->filename)
            compiland = symt_new_compiland(module,
                                           compiland_name = ste->filename);

        addr = module->elf_info->elf_addr + ste->symp->st_value;

        for (j = 0; j < num_areas; j++)
        {
            if (ste->symp->st_value >= thunks[j].rva_start &&
                ste->symp->st_value < thunks[j].rva_end)
                break;
        }
        if (j < num_areas) /* thunk found */
        {
            symt_new_thunk(module, compiland, ste->ht_elt.name, thunks[j].ordinal,
                           addr, ste->symp->st_size);
        }
        else
        {
            ULONG64     ref_addr;

            idx = symt_find_nearest(module, addr);
            if (idx != -1)
                symt_get_info(&module->addr_sorttab[idx]->symt,
                              TI_GET_ADDRESS, &ref_addr);
            if (idx == -1 || addr != ref_addr)
            {
                /* creating public symbols for all the ELF symbols which haven't been
                 * used yet (ie we have no debug information on them)
                 * That's the case, for example, of the .spec.c files
                 */
                if (ELF32_ST_TYPE(ste->symp->st_info) == STT_FUNC)
                {
                    symt_new_function(module, compiland, ste->ht_elt.name,
                                      addr, ste->symp->st_size, NULL);
                }
                else
                {
                    symt_new_global_variable(module, compiland, ste->ht_elt.name,
                                             ELF32_ST_BIND(ste->symp->st_info) == STB_LOCAL,
                                             addr, ste->symp->st_size, NULL);
                }
                /* FIXME: this is a hack !!!
                 * we are adding new symbols, but as we're parsing a symbol table
                 * (hopefully without duplicate symbols) we delay rebuilding the sorted
                 * module table until we're done with the symbol table
                 * Otherwise, as we intertwine symbols's add and lookup, performance
                 * is rather bad
                 */
                module->sortlist_valid = TRUE;
            }
            else if (strcmp(ste->ht_elt.name, module->addr_sorttab[idx]->hash_elt.name))
            {
                ULONG64 xaddr = 0;
                DWORD   xsize = 0, kind = -1;

                symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_ADDRESS,  &xaddr);
                symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_LENGTH,   &xsize);
                symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_DATAKIND, &kind);

                /* If none of symbols has a correct size, we consider they are both markers
                 * Hence, we can silence this warning
                 * Also, we check that we don't have two symbols, one local, the other
                 * global which is legal
                 */
                if ((xsize || ste->symp->st_size) &&
                    (kind == (ELF32_ST_BIND(ste->symp->st_info) == STB_LOCAL) ? DataIsFileStatic : DataIsGlobal))
                    FIXME("Duplicate in %s: %s<%08lx-%08x> %s<%s-%08lx>\n",
                          module->module.ModuleName,
                          ste->ht_elt.name, addr, ste->symp->st_size,
                          module->addr_sorttab[idx]->hash_elt.name,
                          wine_dbgstr_longlong(xaddr), xsize);
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
static int elf_new_public_symbols(struct module* module, struct hash_table* symtab)
{
    struct symt_compiland*      compiland = NULL;
    const char*                 compiland_name = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

    /* FIXME: we're missing the ELF entry point here */

    hash_table_iter_init(symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        /* FIXME: this is not a good idea anyway... we are creating several
         * compiland objects for a same compilation unit
         * We try to cache the last compiland used, but it's not enough
         * (we should here only create compilands if they are not yet
         *  defined)
         */
        if (!compiland_name || compiland_name != ste->filename)
            compiland = symt_new_compiland(module,
                                           compiland_name = ste->filename);

        symt_new_public(module, compiland, ste->ht_elt.name,
                        module->elf_info->elf_addr + ste->symp->st_value,
                        ste->symp->st_size, TRUE /* FIXME */,
                        ELF32_ST_TYPE(ste->symp->st_info) == STT_FUNC);
    }
    return TRUE;
}

/* Copyright (C) 1986 Gary S. Brown. Modified by Robert Shearman. You may use
   the following calc_crc32 code or tables extracted from it, as desired without
   restriction. */

/**********************************************************************\
|* Demonstration program to compute the 32-bit CRC used as the frame  *|
|* check sequence in ADCCP (ANSI X3.66, also known as FIPS PUB 71     *|
|* and FED-STD-1003, the U.S. versions of CCITT's X.25 link-level     *|
|* protocol).  The 32-bit FCS was added via the Federal Register,     *|
|* 1 June 1982, p.23798.  I presume but don't know for certain that   *|
|* this polynomial is or will be included in CCITT V.41, which        *|
|* defines the 16-bit CRC (often called CRC-CCITT) polynomial.  FIPS  *|
|* PUB 78 says that the 32-bit FCS reduces otherwise undetected       *|
|* errors by a factor of 10^-5 over 16-bit FCS.                       *|
\**********************************************************************/

/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is                                                       */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1.                                                    */

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.  UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.                  */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*                                                                     */
/*  1. The table can be generated at runtime if desired; code to do so */
/*     is shown later.  It might not be obvious, but the feedback      */
/*     terms simply represent the results of eight shift/xor opera-    */
/*     tions for all combinations of data and CRC register values.     */
/*                                                                     */
/*  2. The CRC accumulation logic is the same for all CRC polynomials, */
/*     be they sixteen or thirty-two bits wide.  You simply choose the */
/*     appropriate table.  Alternatively, because the table can be     */
/*     generated at runtime, you can start by generating the table for */
/*     the polynomial in question and use exactly the same "updcrc",   */
/*     if your application needn't simultaneously handle two CRC       */
/*     polynomials.  (Note, however, that XMODEM is strange.)          */
/*                                                                     */
/*  3. For 16-bit CRCs, the table entries need be only 16 bits wide;   */
/*     of course, 32-bit entries work OK if the high 16 bits are zero. */
/*                                                                     */
/*  4. The values must be right-shifted by eight bits by the "updcrc"  */
/*     logic; the shift must be unsigned (bring in zeroes).  On some   */
/*     hardware you could probably optimize the shift in assembler by  */
/*     using byte-swap instructions.                                   */


static DWORD calc_crc32(const unsigned char *buf, size_t len)
{
#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))
    static const DWORD crc_32_tab[] =
    { /* CRC polynomial 0xedb88320 */
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    size_t i;
    DWORD crc = ~0;
    for(i = 0; i < len; i++)
        crc = UPDC32(buf[i], crc);
    return ~crc;
#undef UPDC32
}


/******************************************************************
 *		elf_load_debug_info_from_file
 *
 * Loads the symbolic information from ELF module stored in 'file'
 * the module has been loaded at 'load_offset' address, so symbols' address
 * relocation is performed. crc optionally points to the CRC of the debug file
 * to load.
 * returns
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
static BOOL elf_load_debug_info_from_file(
    struct module* module, const char* file, struct pool* pool,
    struct hash_table* ht_symtab, const DWORD *crc)
{
    BOOL                ret = FALSE;
    char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const char*	        shstrtab;
    int	       	        i;
    int                 symtab_sect, dynsym_sect, stab_sect, stabstr_sect, debug_sect, debuglink_sect;
    struct thunk_area   thunks[] =
    {
        {"__wine_spec_import_thunks",           THUNK_ORDINAL_NOTYPE, 0, 0},    /* inter DLL calls */
        {"__wine_spec_delayed_import_loaders",  THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_delayed_import_thunks",   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_delay_load",                   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_thunk_text_16",           -16,                  0, 0},    /* 16 => 32 thunks */
        {"__wine_spec_thunk_data_16",           -16,                  0, 0},    /* 16 => 32 thunks */
        {"__wine_spec_thunk_text_32",           -32,                  0, 0},    /* 32 => 16 thunks */
        {"__wine_spec_thunk_data_32",           -32,                  0, 0},    /* 32 => 16 thunks */
    };

    if (module->type != DMT_ELF || !module->elf_info)
    {
	ERR("Bad elf module '%s'\n", module->module.LoadedImageName);
	return FALSE;
    }

    TRACE("%s\n", file);
    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(file, &statbuf) == -1) goto leave;
    if (S_ISDIR(statbuf.st_mode)) goto leave;

    /*
     * Now open the file, so that we can mmap() it.
     */
    if ((fd = open(file, O_RDONLY)) == -1) goto leave;

    /*
     * Now mmap() the file.
     */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)0xffffffff) goto leave;

    if (crc && (*crc != calc_crc32(addr, statbuf.st_size)))
    {
        ERR("Bad CRC for file %s\n", file);
        /* we don't tolerate mis-matched files */
        goto leave;
    }

    /*
     * Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    ehptr = (Elf32_Ehdr*)addr;
    spnt = (Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    symtab_sect = dynsym_sect = stab_sect = stabstr_sect = debug_sect = debuglink_sect = -1;

    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if (strcmp(shstrtab + spnt[i].sh_name, ".stab") == 0)
	    stab_sect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".stabstr") == 0)
	    stabstr_sect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".debug_info") == 0)
	    debug_sect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".gnu_debuglink") == 0)
	    debuglink_sect = i;
	if ((strcmp(shstrtab + spnt[i].sh_name, ".symtab") == 0) &&
	    (spnt[i].sh_type == SHT_SYMTAB))
            symtab_sect = i;
        if ((strcmp(shstrtab + spnt[i].sh_name, ".dynsym") == 0) &&
            (spnt[i].sh_type == SHT_DYNSYM))
            dynsym_sect = i;
    }

    if (symtab_sect == -1)
    {
        /* if we don't have a symtab but a dynsym, process the dynsym
         * section instead. It'll contain less (relevant) information,
         * but it'll be better than nothing
         */
        if (dynsym_sect == -1) goto leave;
        symtab_sect = dynsym_sect;
    }

    module->module.SymType = SymExport;

    /* create a hash table for the symtab */
    elf_hash_symtab(module, pool, ht_symtab, addr,
                    spnt + symtab_sect, spnt + spnt[symtab_sect].sh_link,
                    sizeof(thunks) / sizeof(thunks[0]), thunks);

    if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        if (stab_sect != -1 && stabstr_sect != -1)
        {
            /* OK, now just parse all of the stabs. */
            ret = stabs_parse(module, module->elf_info->elf_addr,
                              addr + spnt[stab_sect].sh_offset,
                              spnt[stab_sect].sh_size,
                              addr + spnt[stabstr_sect].sh_offset,
                              spnt[stabstr_sect].sh_size);
            if (!ret)
            {
                WARN("Couldn't read correctly read stabs\n");
                goto leave;
            }
            /* and fill in the missing information for stabs */
            elf_finish_stabs_info(module, ht_symtab);
        }
        else if (debug_sect != -1)
        {
            /* Dwarf 2 debug information */
            FIXME("Unsupported Dwarf2 information for %s\n", module->module.ModuleName);
        }
        else if (debuglink_sect != -1)
        {
            DWORD crc;
            const char * file = (const char *)(addr + spnt[debuglink_sect].sh_offset);
            /* crc is stored after the null terminated file string rounded
             * up to the next 4 byte boundary */
            crc = *(const DWORD *)(file + ((DWORD_PTR)(strlen(file) + 4) & ~3));
            ret = elf_load_debug_info_from_file(module, file, pool, ht_symtab, &crc);
            if (!ret)
                WARN("Couldn't load linked debug file %s\n", file);
        }
    }
    if (strstr(module->module.ModuleName, "<elf>") ||
        !strcmp(module->module.ModuleName, "<wine-loader>"))
    {
        /* add the thunks for native libraries */
        if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
            elf_new_wine_thunks(module, ht_symtab,
                                sizeof(thunks) / sizeof(thunks[0]), thunks);
    }
    /* add all the public symbols from symtab */
    if (elf_new_public_symbols(module, ht_symtab) && !ret) ret = TRUE;

leave:
    if (addr != (char*)0xffffffff) munmap(addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return ret;
}

/******************************************************************
 *		elf_load_debug_info
 *
 * Loads ELF debugging information from the module image file.
 */
BOOL elf_load_debug_info(struct module* module)
{
    BOOL              ret;
    struct pool       pool;
    struct hash_table ht_symtab;

    pool_init(&pool, 65536);
    hash_table_init(&pool, &ht_symtab, 256);

    ret = elf_load_debug_info_from_file(module,
        module->module.LoadedImageName, &pool, &ht_symtab, NULL);

    pool_destroy(&pool);

    return ret;
}


/******************************************************************
 *		is_dt_flag_valid
 * returns true iff the section tag is valid
 */
static unsigned is_dt_flag_valid(unsigned d_tag)
{
#ifndef DT_PROCNUM
#define DT_PROCNUM 0
#endif
#ifndef DT_EXTRANUM
#define DT_EXTRANUM 0
#endif
    return (d_tag >= 0 && d_tag < DT_NUM + DT_PROCNUM + DT_EXTRANUM)
#if defined(DT_LOOS) && defined(DT_HIOS)
        || (d_tag >= DT_LOOS && d_tag < DT_HIOS)
#endif
#if defined(DT_LOPROC) && defined(DT_HIPROC)
        || (d_tag >= DT_LOPROC && d_tag < DT_HIPROC)
#endif
        ;
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
static BOOL elf_load_file(struct process* pcs, const char* filename,
                          unsigned long load_offset, struct elf_info* elf_info)
{
    static const BYTE   elf_signature[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
    BOOL                ret = FALSE;
    const char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const Elf32_Phdr*	ppnt;
    const char*         shstrtab;
    int	       	        i;
    DWORD	        size, start;
    unsigned            tmp, page_mask = getpagesize() - 1;

    TRACE("Processing elf file '%s' at %08lx\n", filename, load_offset);

    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(filename, &statbuf) == -1) goto leave;

    /* Now open the file, so that we can mmap() it. */
    if ((fd = open(filename, O_RDONLY)) == -1) goto leave;

    /* Now mmap() the file. */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)-1) goto leave;

    /* Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    ehptr = (const Elf32_Ehdr*)addr;
    if (memcmp(ehptr->e_ident, elf_signature, sizeof(elf_signature))) goto leave;

    spnt = (const Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    /* grab size of module once loaded in memory */
    ppnt = (const Elf32_Phdr*)(addr + ehptr->e_phoff);
    size = 0; start = ~0L;

    for (i = 0; i < ehptr->e_phnum; i++)
    {
        if (ppnt[i].p_type == PT_LOAD)
        {
            tmp = (ppnt[i].p_vaddr + ppnt[i].p_memsz + page_mask) & ~page_mask;
            if (size < tmp) size = tmp;
            if (ppnt[i].p_vaddr < start) start = ppnt[i].p_vaddr;
        }
    }

    /* if non relocatable ELF, then remove fixed address from computation
     * otherwise, all addresses are zero based and start has no effect
     */
    size -= start;
    if (!start && !load_offset)
        ERR("Relocatable ELF %s, but no load address. Loading at 0x0000000\n",
            filename);
    if (start && load_offset)
    {
        WARN("Non-relocatable ELF %s, but load address of 0x%08lx supplied. "
             "Assuming load address is corrupt\n", filename, load_offset);
        load_offset = 0;
    }

    if (elf_info->flags & ELF_INFO_DEBUG_HEADER)
    {
        for (i = 0; i < ehptr->e_shnum; i++)
        {
            if (strcmp(shstrtab + spnt[i].sh_name, ".dynamic") == 0 &&
                spnt[i].sh_type == SHT_DYNAMIC)
            {
                Elf32_Dyn       dyn;
                char*           ptr = (char*)spnt[i].sh_addr;
                unsigned long   len;

                do
                {
                    if (!ReadProcessMemory(pcs->handle, ptr, &dyn, sizeof(dyn), &len) ||
                        len != sizeof(dyn) || !is_dt_flag_valid(dyn.d_tag))
                        dyn.d_tag = DT_NULL;
                    ptr += sizeof(dyn);
                } while (dyn.d_tag != DT_DEBUG && dyn.d_tag != DT_NULL);
                if (dyn.d_tag == DT_NULL) goto leave;
                elf_info->dbg_hdr_addr = dyn.d_un.d_ptr;
            }
	}
    }

    if (elf_info->flags & ELF_INFO_MODULE)
    {
        struct elf_module_info *elf_module_info =
            HeapAlloc(GetProcessHeap(), 0, sizeof(struct elf_module_info));
        if (!elf_module_info) goto leave;
        elf_info->module = module_new(pcs, filename, DMT_ELF,
                                        (load_offset) ? load_offset : start,
                                        size, 0, 0);
        if (!elf_info->module)
        {
            HeapFree(GetProcessHeap(), 0, elf_module_info);
            goto leave;
        }
        elf_info->module->elf_info = elf_module_info;
        elf_info->module->elf_info->elf_addr = load_offset;

        if (dbghelp_options & SYMOPT_DEFERRED_LOADS)
        {
            elf_info->module->module.SymType = SymDeferred;
            ret = TRUE;
        }
        else ret = elf_load_debug_info(elf_info->module);

        elf_info->module->elf_info->elf_mark = 1;
        elf_info->module->elf_info->elf_loader = 0;
    } else ret = TRUE;

leave:
    if (addr != (char*)0xffffffff) munmap((void*)addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return ret;
}

/******************************************************************
 *		elf_load_file_from_path
 * tries to load an ELF file from a set of paths (separated by ':')
 */
static BOOL elf_load_file_from_path(HANDLE hProcess,
                                    const char* filename,
                                    unsigned long load_offset,
                                    const char* path,
                                    struct elf_info* elf_info)
{
    BOOL                ret = FALSE;
    char                *s, *t, *fn;
    char*	        paths = NULL;

    if (!path) return FALSE;

    paths = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(path) + 1), path);
    for (s = paths; s && *s; s = (t) ? (t+1) : NULL)
    {
	t = strchr(s, ':');
	if (t) *t = '\0';
	fn = HeapAlloc(GetProcessHeap(), 0, strlen(filename) + 1 + strlen(s) + 1);
	if (!fn) break;
	strcpy(fn, s);
	strcat(fn, "/");
	strcat(fn, filename);
	ret = elf_load_file(hProcess, fn, load_offset, elf_info);
	HeapFree(GetProcessHeap(), 0, fn);
	if (ret) break;
	s = (t) ? (t+1) : NULL;
    }

    HeapFree(GetProcessHeap(), 0, paths);
    return ret;
}

/******************************************************************
 *		elf_search_and_load_file
 *
 * lookup a file in standard ELF locations, and if found, load it
 */
static BOOL elf_search_and_load_file(struct process* pcs, const char* filename,
                                     unsigned long load_offset,
                                     struct elf_info* elf_info)
{
    BOOL                ret = FALSE;
    struct module*      module;

    if (filename == NULL || *filename == '\0') return FALSE;
    if ((module = module_find_by_name(pcs, filename, DMT_ELF)))
    {
        elf_info->module = module;
        module->elf_info->elf_mark = 1;
        return module->module.SymType;
    }

    if (strstr(filename, "libstdc++")) return FALSE; /* We know we can't do it */
    ret = elf_load_file(pcs, filename, load_offset, elf_info);
    /* if relative pathname, try some absolute base dirs */
    if (!ret && !strchr(filename, '/'))
    {
        ret = elf_load_file_from_path(pcs, filename, load_offset,
                                      getenv("PATH"), elf_info) ||
            elf_load_file_from_path(pcs, filename, load_offset,
                                    getenv("LD_LIBRARY_PATH"), elf_info) ||
            elf_load_file_from_path(pcs, filename, load_offset,
                                    getenv("WINEDLLPATH"), elf_info);
    }

    return ret;
}

/******************************************************************
 *		elf_synchronize_module_list
 *
 * this functions rescans the debuggee module's list and synchronizes it with
 * the one from 'pcs', ie:
 * - if a module is in debuggee and not in pcs, it's loaded into pcs
 * - if a module is in pcs and not in debuggee, it's unloaded from pcs
 */
BOOL	elf_synchronize_module_list(struct process* pcs)
{
    struct r_debug      dbg_hdr;
    void*               lm_addr;
    struct link_map     lm;
    char		bufstr[256];
    struct elf_info     elf_info;
    struct module*      module;

    if (!pcs->dbg_hdr_addr ||
        !ReadProcessMemory(pcs->handle, (void*)pcs->dbg_hdr_addr,
                           &dbg_hdr, sizeof(dbg_hdr), NULL))
        return FALSE;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF) module->elf_info->elf_mark = 0;
    }

    elf_info.flags = ELF_INFO_MODULE;
    /* Now walk the linked list.  In all known ELF implementations,
     * the dynamic loader maintains this linked list for us.  In some
     * cases the first entry doesn't appear with a name, in other cases it
     * does.
     */
    for (lm_addr = (void*)dbg_hdr.r_map; lm_addr; lm_addr = (void*)lm.l_next)
    {
	if (!ReadProcessMemory(pcs->handle, lm_addr, &lm, sizeof(lm), NULL))
	    return FALSE;

	if (lm.l_prev != NULL && /* skip first entry, normally debuggee itself */
	    lm.l_name != NULL &&
	    ReadProcessMemory(pcs->handle, lm.l_name, bufstr, sizeof(bufstr), NULL))
        {
	    bufstr[sizeof(bufstr) - 1] = '\0';
            elf_search_and_load_file(pcs, bufstr, (unsigned long)lm.l_addr,
                                     &elf_info);
	}
    }

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF && !module->elf_info->elf_mark &&
            !module->elf_info->elf_loader)
        {
            module_remove(pcs, module);
            /* restart all over */
            module = pcs->lmodules;
        }
    }
    return TRUE;
}

/******************************************************************
 *		elf_read_wine_loader_dbg_info
 *
 * Try to find a decent wine executable which could have loaded the debuggee
 */
BOOL elf_read_wine_loader_dbg_info(struct process* pcs)
{
    const char*         ptr;
    struct elf_info     elf_info;
    BOOL                ret;

    elf_info.flags = ELF_INFO_DEBUG_HEADER | ELF_INFO_MODULE;
    /* All binaries are loaded with WINELOADER (if run from tree) or by the
     * main executable (either wine-kthread or wine-pthread)
     * Note: the heuristic use to know whether we need to load wine-pthread or
     * wine-kthread is not 100% safe
     */
    if ((ptr = getenv("WINELOADER")))
        ret = elf_search_and_load_file(pcs, ptr, 0, &elf_info);
    else
    {
        ret = elf_search_and_load_file(pcs, "wine-kthread", 0, &elf_info) ||
            elf_search_and_load_file(pcs, "wine-pthread", 0, &elf_info);
    }
    if (!ret) return FALSE;
    elf_info.module->elf_info->elf_loader = 1;
    strcpy(elf_info.module->module.ModuleName, "<wine-loader>");
    return (pcs->dbg_hdr_addr = elf_info.dbg_hdr_addr) != 0;
}

/******************************************************************
 *		elf_load_module
 *
 * loads an ELF module and stores it in process' module list
 * Also, find module real name and load address from
 * the real loaded modules list in pcs address space
 */
struct module*  elf_load_module(struct process* pcs, const char* name)
{
    struct elf_info     elf_info;
    BOOL                ret = FALSE;
    const char*         p;
    const char*         xname;
    struct r_debug      dbg_hdr;
    void*               lm_addr;
    struct link_map     lm;
    char                bufstr[256];

    TRACE("(%p %s)\n", pcs, name);

    elf_info.flags = ELF_INFO_MODULE;

    /* do only the lookup from the filename, not the path (as we lookup module name
     * in the process' loaded module list)
     */
    xname = strrchr(name, '/');
    if (!xname++) xname = name;

    if (!ReadProcessMemory(pcs->handle, (void*)pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr), NULL))
        return NULL;

    for (lm_addr = (void*)dbg_hdr.r_map; lm_addr; lm_addr = (void*)lm.l_next)
    {
        if (!ReadProcessMemory(pcs->handle, lm_addr, &lm, sizeof(lm), NULL))
            return NULL;

        if (lm.l_prev != NULL && /* skip first entry, normally debuggee itself */
            lm.l_name != NULL &&
            ReadProcessMemory(pcs->handle, lm.l_name, bufstr, sizeof(bufstr), NULL))
        {
            bufstr[sizeof(bufstr) - 1] = '\0';
            /* memcmp is needed for matches when bufstr contains also version information
             * name: libc.so, bufstr: libc.so.6.0
             */
            p = strrchr(bufstr, '/');
            if (!p++) p = bufstr;
            if (!memcmp(p, xname, strlen(xname)))
            {
                ret = elf_search_and_load_file(pcs, bufstr,
                                               (unsigned long)lm.l_addr, &elf_info);
                break;
            }
        }
    }
    if (!lm_addr || !ret) return NULL;
    assert(elf_info.module);
    return elf_info.module;
}

#else	/* !__ELF__ */

BOOL	elf_synchronize_module_list(struct process* pcs)
{
    return FALSE;
}

BOOL elf_read_wine_loader_dbg_info(struct process* pcs)
{
    return FALSE;
}

struct module*  elf_load_module(struct process* pcs, const char* name)
{
    return NULL;
}

BOOL elf_load_debug_info(struct module* module)
{
    return FALSE;
}
#endif  /* __ELF__ */
