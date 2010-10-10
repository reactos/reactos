/*
 * 16-bit spec files
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>

#include "build.h"

#define NE_FFLAGS_SINGLEDATA 0x0001
#define NE_FFLAGS_LIBMODULE  0x8000

/* argument type flags for relay debugging */
enum arg_types
{
    ARG_NONE = 0, /* indicates end of arg list */
    ARG_WORD,     /* unsigned word */
    ARG_SWORD,    /* signed word */
    ARG_LONG,     /* long or segmented pointer */
    ARG_PTR,      /* linear pointer */
    ARG_STR,      /* linear pointer to null-terminated string */
    ARG_SEGSTR,   /* segmented pointer to null-terminated string */
    ARG_VARARG    /* start of varargs */
};

/* sequences of nops to fill a certain number of words */
static const char * const nop_sequence[4] =
{
    ".byte 0x89,0xf6",  /* mov %esi,%esi */
    ".byte 0x8d,0x74,0x26,0x00",  /* lea 0x00(%esi),%esi */
    ".byte 0x8d,0xb6,0x00,0x00,0x00,0x00",  /* lea 0x00000000(%esi),%esi */
    ".byte 0x8d,0x74,0x26,0x00,0x8d,0x74,0x26,0x00" /* lea 0x00(%esi),%esi; lea 0x00(%esi),%esi */
};

static inline int is_function( const ORDDEF *odp )
{
    return (odp->type == TYPE_CDECL ||
            odp->type == TYPE_PASCAL ||
            odp->type == TYPE_VARARGS ||
            odp->type == TYPE_STUB);
}

static void init_dll_name( DLLSPEC *spec )
{
    if (!spec->file_name)
    {
        char *p;
        spec->file_name = xstrdup( output_file_name );
        if ((p = strrchr( spec->file_name, '.' ))) *p = 0;
    }
    if (!spec->dll_name)  /* set default name from file name */
    {
        char *p;
        spec->dll_name = xstrdup( spec->file_name );
        if ((p = strrchr( spec->dll_name, '.' ))) *p = 0;
    }
}

/*******************************************************************
 *         output_entries
 *
 * Output entries for individual symbols in the entry table.
 */
static void output_entries( DLLSPEC *spec, int first, int count )
{
    int i;

    for (i = 0; i < count; i++)
    {
        ORDDEF *odp = spec->ordinals[first + i];
        output( "\t.byte 0x03\n" );  /* flags: exported & public data */
        switch (odp->type)
        {
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
        case TYPE_STUB:
            output( "\t%s .L__wine_%s_%u-.L__wine_spec_code_segment\n",
                     get_asm_short_keyword(),
                     make_c_identifier(spec->dll_name), first + i );
            break;
        case TYPE_VARIABLE:
            output( "\t%s .L__wine_%s_%u-.L__wine_spec_data_segment\n",
                     get_asm_short_keyword(),
                     make_c_identifier(spec->dll_name), first + i );
            break;
        case TYPE_ABS:
            output( "\t%s 0x%04x  /* %s */\n",
                     get_asm_short_keyword(), odp->u.abs.value, odp->name );
            break;
        default:
            assert(0);
        }
    }
}


/*******************************************************************
 *         output_entry_table
 */
static void output_entry_table( DLLSPEC *spec )
{
    int i, prev = 0, prev_sel = -1, bundle_count = 0;

    for (i = 1; i <= spec->limit; i++)
    {
        int selector = 0;
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;

        switch (odp->type)
        {
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
        case TYPE_STUB:
            selector = 1;  /* Code selector */
            break;
        case TYPE_VARIABLE:
            selector = 2;  /* Data selector */
            break;
        case TYPE_ABS:
            selector = 0xfe;  /* Constant selector */
            break;
        default:
            continue;
        }

        if (prev + 1 != i || prev_sel != selector || bundle_count == 255)
        {
            /* need to start a new bundle */

            /* flush previous bundle */
            if (bundle_count)
            {
                output( "\t/* %s.%d - %s.%d */\n",
                         spec->dll_name, prev - bundle_count + 1, spec->dll_name, prev );
                output( "\t.byte 0x%02x,0x%02x\n", bundle_count, prev_sel );
                output_entries( spec, prev - bundle_count + 1, bundle_count );
            }

            if (prev + 1 != i)
            {
                int skip = i - (prev + 1);
                while (skip > 255)
                {
                    output( "\t.byte 0xff,0x00\n" );
                    skip -= 255;
                }
                output( "\t.byte 0x%02x,0x00\n", skip );
            }

            bundle_count = 0;
            prev_sel = selector;
        }
        bundle_count++;
        prev = i;
    }

    /* flush last bundle */
    if (bundle_count)
    {
        output( "\t.byte 0x%02x,0x%02x\n", bundle_count, prev_sel );
        output_entries( spec, prev - bundle_count + 1, bundle_count );
    }
    output( "\t.byte 0x00\n" );
}


/*******************************************************************
 *         output_resident_name
 */
static void output_resident_name( const char *string, int ordinal )
{
    unsigned int i, len = strlen(string);

    output( "\t.byte 0x%02x", len );
    for (i = 0; i < len; i++) output( ",0x%02x", (unsigned char)toupper(string[i]) );
    output( " /* %s */\n", string );
    output( "\t%s %u\n", get_asm_short_keyword(), ordinal );
}


/*******************************************************************
 *         get_callfrom16_name
 */
static const char *get_callfrom16_name( const ORDDEF *odp )
{
    static char buffer[80];

    sprintf( buffer, "%s_%s_%s",
             (odp->type == TYPE_PASCAL) ? "p" :
             (odp->type == TYPE_VARARGS) ? "v" : "c",
             (odp->flags & FLAG_REGISTER) ? "regs" :
             (odp->flags & FLAG_RET16) ? "word" : "long",
             odp->u.func.arg_types );
    return buffer;
}


/*******************************************************************
 *         get_relay_name
 */
static const char *get_relay_name( const ORDDEF *odp )
{
    static char buffer[80];
    char *p;

    switch(odp->type)
    {
    case TYPE_PASCAL:
        strcpy( buffer, "p_" );
        break;
    case TYPE_VARARGS:
        strcpy( buffer, "v_" );
        break;
    case TYPE_CDECL:
    case TYPE_STUB:
        strcpy( buffer, "c_" );
        break;
    default:
        assert(0);
    }
    strcat( buffer, odp->u.func.arg_types );
    for (p = buffer + 2; *p; p++)
    {
        /* map string types to the corresponding plain pointer type */
        if (*p == 't') *p = 'p';
        else if (*p == 'T') *p = 'l';
    }
    if (odp->flags & FLAG_REGISTER) strcat( buffer, "_regs" );
    return buffer;
}


/*******************************************************************
 *         get_function_argsize
 */
static int get_function_argsize( const ORDDEF *odp )
{
    const char *args;
    int argsize = 0;

    for (args = odp->u.func.arg_types; *args; args++)
    {
        switch (*args)
        {
        case 'w':  /* word */
        case 's':  /* s_word */
            argsize += 2;
            break;
        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            argsize += 4;
            break;
        default:
            assert(0);
        }
    }
    return argsize;
}


/*******************************************************************
 *         output_call16_function
 *
 * Build a 16-bit-to-Wine callback glue function.
 *
 * The generated routines are intended to be used as argument conversion
 * routines to be called by the CallFrom16... core. Thus, the prototypes of
 * the generated routines are (see also CallFrom16):
 *
 *  extern WORD WINAPI __wine_spec_call16_C_xxx( FARPROC func, LPBYTE args );
 *  extern LONG WINAPI __wine_spec_call16_C_xxx( FARPROC func, LPBYTE args );
 *  extern void WINAPI __wine_spec_call16_C_xxx_regs( FARPROC func, LPBYTE args, CONTEXT86 *context );
 *
 * where 'C' is the calling convention ('p' for pascal or 'c' for cdecl),
 * and each 'x' is an argument  ('w'=word, 's'=signed word, 'l'=long,
 * 'p'=linear pointer, 't'=linear pointer to null-terminated string,
 * 'T'=segmented pointer to null-terminated string).
 *
 * The generated routines fetch the arguments from the 16-bit stack (pointed
 * to by 'args'); the offsets of the single argument values are computed
 * according to the calling convention and the argument types.  Then, the
 * 32-bit entry point is called with these arguments.
 *
 * For register functions, the arguments (if present) are converted just
 * the same as for normal functions, but in addition the CONTEXT86 pointer
 * filled with the current register values is passed to the 32-bit routine.
 */
static void output_call16_function( ORDDEF *odp )
{
    char name[256];
    int i, pos, stack_words;
    const char *args = odp->u.func.arg_types;
    int argsize = get_function_argsize( odp );
    int needs_ldt = strchr( args, 'p' ) || strchr( args, 't' );

    sprintf( name, ".L__wine_spec_call16_%s", get_relay_name(odp) );

    output( "\t.align %d\n", get_alignment(4) );
    output( "\t%s\n", func_declaration(name) );
    output( "%s:\n", name );
    output( "\tpushl %%ebp\n" );
    output( "\tmovl %%esp,%%ebp\n" );
    stack_words = 2;
    if (needs_ldt)
    {
        output( "\tpushl %%esi\n" );
        stack_words++;
        if (UsePIC)
        {
            output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
            output( "1:\tmovl wine_ldt_copy_ptr-1b(%%eax),%%esi\n" );
        }
        else
            output( "\tmovl $%s,%%esi\n", asm_name("wine_ldt_copy") );
    }

    /* preserve 16-byte stack alignment */
    stack_words += strlen(args);
    if ((odp->flags & FLAG_REGISTER) || (odp->type == TYPE_VARARGS)) stack_words++;
    if (stack_words % 4) output( "\tsubl $%d,%%esp\n", 16 - 4 * (stack_words % 4) );

    if (args[0] || odp->type == TYPE_VARARGS)
        output( "\tmovl 12(%%ebp),%%ecx\n" );  /* args */

    if (odp->flags & FLAG_REGISTER)
    {
        output( "\tpushl 16(%%ebp)\n" );  /* context */
    }
    else if (odp->type == TYPE_VARARGS)
    {
        output( "\tleal %d(%%ecx),%%eax\n", argsize );
        output( "\tpushl %%eax\n" );  /* va_list16 */
    }

    pos = (odp->type == TYPE_PASCAL) ? 0 : argsize;
    for (i = strlen(args) - 1; i >= 0; i--)
    {
        switch (args[i])
        {
        case 'w':  /* word */
            if (odp->type != TYPE_PASCAL) pos -= 2;
            output( "\tmovzwl %d(%%ecx),%%eax\n", pos );
            output( "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 2;
            break;

        case 's':  /* s_word */
            if (odp->type != TYPE_PASCAL) pos -= 2;
            output( "\tmovswl %d(%%ecx),%%eax\n", pos );
            output( "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if (odp->type != TYPE_PASCAL) pos -= 4;
            output( "\tpushl %d(%%ecx)\n", pos );
            if (odp->type == TYPE_PASCAL) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if (odp->type != TYPE_PASCAL) pos -= 4;
            output( "\tmovzwl %d(%%ecx),%%edx\n", pos + 2 ); /* sel */
            output( "\tshr $3,%%edx\n" );
            output( "\tmovzwl %d(%%ecx),%%eax\n", pos ); /* offset */
            output( "\taddl (%%esi,%%edx,4),%%eax\n" );
            output( "\tpushl %%eax\n" );
            if (odp->type == TYPE_PASCAL) pos += 4;
            break;

        default:
            assert(0);
        }
    }

    output( "\tcall *8(%%ebp)\n" );

    if (needs_ldt) output( "\tmovl -4(%%ebp),%%esi\n" );

    output( "\tleave\n" );
    output( "\tret\n" );
    output_function_size( name );
}


/*******************************************************************
 *         callfrom16_type_compare
 *
 * Compare two callfrom16 sequences.
 */
static int callfrom16_type_compare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)e1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)e2;
    int retval;
    int type1 = odp1->type;
    int type2 = odp2->type;

    if (type1 == TYPE_STUB) type1 = TYPE_CDECL;
    if (type2 == TYPE_STUB) type2 = TYPE_CDECL;

    if ((retval = type1 - type2) != 0) return retval;

    type1 = odp1->flags & (FLAG_RET16|FLAG_REGISTER);
    type2 = odp2->flags & (FLAG_RET16|FLAG_REGISTER);

    if ((retval = type1 - type2) != 0) return retval;

    return strcmp( odp1->u.func.arg_types, odp2->u.func.arg_types );
}


/*******************************************************************
 *         relay_type_compare
 *
 * Same as callfrom16_type_compare but ignores differences that don't affect the resulting relay function.
 */
static int relay_type_compare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)e1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)e2;
    char name1[80];

    strcpy( name1, get_relay_name(odp1) );
    return strcmp( name1, get_relay_name(odp2) );
}


/*******************************************************************
 *         sort_func_list
 *
 * Sort a list of functions, removing duplicates.
 */
static int sort_func_list( ORDDEF **list, int count,
                           int (*compare)(const void *, const void *) )
{
    int i, j;

    if (!count) return 0;
    qsort( list, count, sizeof(*list), compare );

    for (i = j = 0; i < count; i++)
    {
        if (compare( &list[j], &list[i] )) list[++j] = list[i];
    }
    return j + 1;
}


/*******************************************************************
 *         output_init_code
 *
 * Output the dll initialization code.
 */
static void output_init_code( const DLLSPEC *spec )
{
    char name[80];

    sprintf( name, ".L__wine_spec_%s_init", make_c_identifier(spec->dll_name) );

    output( "\n/* dll initialization code */\n\n" );
    output( "\t.text\n" );
    output( "\t.align 4\n" );
    output( "\t%s\n", func_declaration(name) );
    output( "%s:\n", name );
    output( "\tsubl $4,%%esp\n" );
    if (UsePIC)
    {
        output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
        output( "1:\tleal .L__wine_spec_file_name-1b(%%eax),%%ecx\n" );
        output( "\tpushl %%ecx\n" );
        output( "\tleal .L__wine_spec_dos_header-1b(%%eax),%%ecx\n" );
        output( "\tpushl %%ecx\n" );
    }
    else
    {
        output( "\tpushl $.L__wine_spec_file_name\n" );
        output( "\tpushl $.L__wine_spec_dos_header\n" );
    }
    output( "\tcall %s\n", asm_name("__wine_dll_register_16") );
    output( "\taddl $12,%%esp\n" );
    output( "\tret\n" );
    output_function_size( name );

    sprintf( name, ".L__wine_spec_%s_fini", make_c_identifier(spec->dll_name) );

    output( "\t.align 4\n" );
    output( "\t%s\n", func_declaration(name) );
    output( "%s:\n", name );
    output( "\tsubl $8,%%esp\n" );
    if (UsePIC)
    {
        output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
        output( "1:\tleal .L__wine_spec_dos_header-1b(%%eax),%%ecx\n" );
        output( "\tpushl %%ecx\n" );
    }
    else
    {
        output( "\tpushl $.L__wine_spec_dos_header\n" );
    }
    output( "\tcall %s\n", asm_name("__wine_dll_unregister_16") );
    output( "\taddl $12,%%esp\n" );
    output( "\tret\n" );
    output_function_size( name );

    if (target_platform == PLATFORM_APPLE)
    {
        output( "\t.mod_init_func\n" );
        output( "\t.align %d\n", get_alignment(4) );
        output( "\t.long .L__wine_spec_%s_init\n", make_c_identifier(spec->dll_name) );
        output( "\t.mod_term_func\n" );
        output( "\t.align %d\n", get_alignment(4) );
        output( "\t.long .L__wine_spec_%s_fini\n", make_c_identifier(spec->dll_name) );
    }
    else
    {
        output( "\t.section \".init\",\"ax\"\n" );
        output( "\tcall .L__wine_spec_%s_init\n", make_c_identifier(spec->dll_name) );
        output( "\t.section \".fini\",\"ax\"\n" );
        output( "\tcall .L__wine_spec_%s_fini\n", make_c_identifier(spec->dll_name) );
    }
}


/*******************************************************************
 *         output_module16
 *
 * Output code for a 16-bit module.
 */
static void output_module16( DLLSPEC *spec )
{
    ORDDEF **typelist;
    ORDDEF *entry_point = NULL;
    int i, j, nb_funcs;

    /* store the main entry point as ordinal 0 */

    if (!spec->ordinals)
    {
        assert(spec->limit == 0);
        spec->ordinals = xmalloc( sizeof(spec->ordinals[0]) );
        spec->ordinals[0] = NULL;
    }
    if (spec->init_func && !(spec->characteristics & IMAGE_FILE_DLL))
    {
        entry_point = xmalloc( sizeof(*entry_point) );
        entry_point->type = TYPE_PASCAL;
        entry_point->ordinal = 0;
        entry_point->lineno = 0;
        entry_point->flags = FLAG_REGISTER;
        entry_point->name = NULL;
        entry_point->link_name = xstrdup( spec->init_func );
        entry_point->export_name = NULL;
        entry_point->u.func.arg_types[0] = 0;
        assert( !spec->ordinals[0] );
        spec->ordinals[0] = entry_point;
    }

    /* Build sorted list of all argument types, without duplicates */

    typelist = xmalloc( (spec->limit + 1) * sizeof(*typelist) );

    for (i = nb_funcs = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        if (is_function( odp )) typelist[nb_funcs++] = odp;
    }

    nb_funcs = sort_func_list( typelist, nb_funcs, callfrom16_type_compare );

    /* Output the module structure */

    output( "\n/* module data */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(4) );
    output( ".L__wine_spec_dos_header:\n" );
    output( "\t%s 0x5a4d\n", get_asm_short_keyword() );                    /* e_magic */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_cblp */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_cp */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_crlc */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_cparhdr */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_minalloc */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_maxalloc */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_ss */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_sp */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_csum */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_ip */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_cs */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_lfarlc */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_ovno */
    output( "\t%s 0,0,0,0\n", get_asm_short_keyword() );                   /* e_res */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_oemid */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* e_oeminfo */
    output( "\t%s 0,0,0,0,0,0,0,0,0,0\n", get_asm_short_keyword() );       /* e_res2 */
    output( "\t.long .L__wine_spec_ne_header-.L__wine_spec_dos_header\n" );/* e_lfanew */

    output( ".L__wine_spec_ne_header:\n" );
    output( "\t%s 0x454e\n", get_asm_short_keyword() );                    /* ne_magic */
    output( "\t.byte 0\n" );                                               /* ne_ver */
    output( "\t.byte 0\n" );                                               /* ne_rev */
    output( "\t%s .L__wine_spec_ne_enttab-.L__wine_spec_ne_header\n",      /* ne_enttab */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_ne_enttab_end-.L__wine_spec_ne_enttab\n",  /* ne_cbenttab */
             get_asm_short_keyword() );
    output( "\t.long 0\n" );                                               /* ne_crc */
    output( "\t%s 0x%04x\n", get_asm_short_keyword(),                      /* ne_flags */
             NE_FFLAGS_SINGLEDATA |
             ((spec->characteristics & IMAGE_FILE_DLL) ? NE_FFLAGS_LIBMODULE : 0) );
    output( "\t%s 2\n", get_asm_short_keyword() );                         /* ne_autodata */
    output( "\t%s %u\n", get_asm_short_keyword(), spec->heap_size );       /* ne_heap */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* ne_stack */
    if (!entry_point) output( "\t.long 0\n" );                             /* ne_csip */
    else output( "\t%s .L__wine_%s_0-.L__wine_spec_code_segment,1\n",
                 get_asm_short_keyword(), make_c_identifier(spec->dll_name) );
    output( "\t%s 0,2\n", get_asm_short_keyword() );                       /* ne_sssp */
    output( "\t%s 2\n", get_asm_short_keyword() );                         /* ne_cseg */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* ne_cmod */
    output( "\t%s 0\n", get_asm_short_keyword() );                         /* ne_cbnrestab */
    output( "\t%s .L__wine_spec_ne_segtab-.L__wine_spec_ne_header\n",      /* ne_segtab */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_ne_rsrctab-.L__wine_spec_ne_header\n",     /* ne_rsrctab */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_ne_restab-.L__wine_spec_ne_header\n",      /* ne_restab */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_ne_modtab-.L__wine_spec_ne_header\n",      /* ne_modtab */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_ne_imptab-.L__wine_spec_ne_header\n",      /* ne_imptab */
             get_asm_short_keyword() );
    output( "\t.long 0\n" );                                   /* ne_nrestab */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_cmovent */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_align */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_cres */
    output( "\t.byte 0x02\n" );                                /* ne_exetyp = NE_OSFLAGS_WINDOWS */
    output( "\t.byte 0x08\n" );                                /* ne_flagsothers = NE_AFLAGS_FASTLOAD */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_pretthunks */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_psegrefbytes */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_swaparea */
    output( "\t%s 0\n", get_asm_short_keyword() );             /* ne_expver */

    /* segment table */

    output( "\n.L__wine_spec_ne_segtab:\n" );

    /* code segment entry */

    output( "\t%s .L__wine_spec_code_segment-.L__wine_spec_dos_header\n",  /* filepos */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_code_segment_end-.L__wine_spec_code_segment\n", /* size */
             get_asm_short_keyword() );
    output( "\t%s 0x2000\n", get_asm_short_keyword() ); /* flags = NE_SEGFLAGS_32BIT */
    output( "\t%s .L__wine_spec_code_segment_end-.L__wine_spec_code_segment\n", /* minsize */
             get_asm_short_keyword() );

    /* data segment entry */

    output( "\t%s .L__wine_spec_data_segment-.L__wine_spec_dos_header\n",  /* filepos */
             get_asm_short_keyword() );
    output( "\t%s .L__wine_spec_data_segment_end-.L__wine_spec_data_segment\n", /* size */
             get_asm_short_keyword() );
    output( "\t%s 0x0001\n", get_asm_short_keyword() ); /* flags = NE_SEGFLAGS_DATA */
    output( "\t%s .L__wine_spec_data_segment_end-.L__wine_spec_data_segment\n", /* minsize */
             get_asm_short_keyword() );

    /* resource directory */

    output_res16_directory( spec );

    /* resident names table */

    output( "\n\t.align %d\n", get_alignment(2) );
    output( ".L__wine_spec_ne_restab:\n" );
    output_resident_name( spec->dll_name, 0 );
    for (i = 1; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !odp->name[0]) continue;
        output_resident_name( odp->name, i );
    }
    output( "\t.byte 0\n" );

    /* imported names table */

    output( "\n\t.align %d\n", get_alignment(2) );
    output( ".L__wine_spec_ne_modtab:\n" );
    output( ".L__wine_spec_ne_imptab:\n" );
    output( "\t.byte 0,0\n" );

    /* entry table */

    output( "\n.L__wine_spec_ne_enttab:\n" );
    output_entry_table( spec );
    output( ".L__wine_spec_ne_enttab_end:\n" );

    /* code segment */

    output( "\n\t.align %d\n", get_alignment(2) );
    output( ".L__wine_spec_code_segment:\n" );

    for ( i = 0; i < nb_funcs; i++ )
    {
        unsigned int arg_types[2];
        int nop_words, argsize = 0;

        if ( typelist[i]->type == TYPE_PASCAL )
            argsize = get_function_argsize( typelist[i] );

        /* build the arg types bit fields */
        arg_types[0] = arg_types[1] = 0;
        for (j = 0; typelist[i]->u.func.arg_types[j]; j++)
        {
            int type = 0;
            switch(typelist[i]->u.func.arg_types[j])
            {
            case 'w': type = ARG_WORD; break;
            case 's': type = ARG_SWORD; break;
            case 'l': type = ARG_LONG; break;
            case 'p': type = ARG_PTR; break;
            case 't': type = ARG_STR; break;
            case 'T': type = ARG_SEGSTR; break;
            }
            arg_types[j / 10] |= type << (3 * (j % 10));
        }
        if (typelist[i]->type == TYPE_VARARGS) arg_types[j / 10] |= ARG_VARARG << (3 * (j % 10));

        output( ".L__wine_spec_callfrom16_%s:\n", get_callfrom16_name(typelist[i]) );
        output( "\tpushl $.L__wine_spec_call16_%s\n", get_relay_name(typelist[i]) );
        output( "\tlcall $0,$0\n" );

        if (typelist[i]->flags & FLAG_REGISTER)
        {
            nop_words = 4;
        }
        else if (typelist[i]->flags & FLAG_RET16)
        {
            output( "\torw %%ax,%%ax\n" );
            output( "\tnop\n" );  /* so that the lretw is aligned */
            nop_words = 2;
        }
        else
        {
            output( "\tshld $16,%%eax,%%edx\n" );
            output( "\torl %%eax,%%eax\n" );
            nop_words = 1;
        }

        if (argsize)
        {
            output( "\tlretw $%u\n", argsize );
            nop_words--;
        }
        else output( "\tlretw\n" );

        if (nop_words) output( "\t%s\n", nop_sequence[nop_words-1] );

        /* the movl is here so that the code contains only valid instructions, */
        /* it's never actually executed, we only care about the arg_types[] values */
        output( "\t%s 0x86c7\n", get_asm_short_keyword() );
        output( "\t.long 0x%08x,0x%08x\n", arg_types[0], arg_types[1] );
    }

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !is_function( odp )) continue;
        output( ".L__wine_%s_%u:\n", make_c_identifier(spec->dll_name), i );
        output( "\tpushw %%bp\n" );
        output( "\tpushl $%s\n",
                 asm_name( odp->type == TYPE_STUB ? get_stub_name( odp, spec ) : odp->link_name ));
        output( "\tcallw .L__wine_spec_callfrom16_%s\n", get_callfrom16_name( odp ) );
    }
    output( ".L__wine_spec_code_segment_end:\n" );

    /* data segment */

    output( "\n.L__wine_spec_data_segment:\n" );
    output( "\t.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n" );  /* instance data */
    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || odp->type != TYPE_VARIABLE) continue;
        output( ".L__wine_%s_%u:\n", make_c_identifier(spec->dll_name), i );
        output( "\t.long " );
        for (j = 0; j < odp->u.var.n_values-1; j++)
            output( "0x%08x,", odp->u.var.values[j] );
        output( "0x%08x\n", odp->u.var.values[j] );
    }
    output( ".L__wine_spec_data_segment_end:\n" );

    /* resource data */

    if (spec->nb_resources)
    {
        output( "\n.L__wine_spec_resource_data:\n" );
        output_res16_data( spec );
    }

    output( "\t.byte 0\n" );  /* make sure the last symbol points to something */

    /* relay functions */

    nb_funcs = sort_func_list( typelist, nb_funcs, relay_type_compare );
    if (nb_funcs)
    {
        output( "\n/* relay functions */\n\n" );
        output( "\t.text\n" );
        for ( i = 0; i < nb_funcs; i++ ) output_call16_function( typelist[i] );
        output( "\t.data\n" );
        output( "wine_ldt_copy_ptr:\n" );
        output( "\t.long %s\n", asm_name("wine_ldt_copy") );
    }

    free( typelist );
}


/*******************************************************************
 *         BuildSpec16File
 *
 * Build a Win16 assembly file from a spec file.
 */
void BuildSpec16File( DLLSPEC *spec )
{
    init_dll_name( spec );
    output_standard_file_header();
    output_module16( spec );
    output_init_code( spec );

    output( "\n\t%s\n", get_asm_string_section() );
    output( ".L__wine_spec_file_name:\n" );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );

    output_stubs( spec );
    output_get_pc_thunk();
    output_gnu_stack_note();
}


/*******************************************************************
 *         output_spec16_file
 *
 * Output the complete data for a spec 16-bit file.
 */
void output_spec16_file( DLLSPEC *spec16 )
{
    DLLSPEC *spec32 = alloc_dll_spec();

    init_dll_name( spec16 );
    spec32->file_name = xstrdup( spec16->file_name );

    if (spec16->characteristics & IMAGE_FILE_DLL)
    {
        spec32->characteristics = IMAGE_FILE_DLL;
        spec32->init_func = xstrdup( "__wine_spec_dll_entry" );
    }

    resolve_imports( spec16 );
    add_16bit_exports( spec32, spec16 );

    output_standard_file_header();
    output_module( spec32 );
    output_module16( spec16 );
    output_stubs( spec16 );
    output_exports( spec32 );
    output_imports( spec16 );
    if (spec16->main_module)
    {
        output( "\n\t%s\n", get_asm_string_section() );
        output( ".L__wine_spec_main_module:\n" );
        output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec16->main_module );
    }
    output_gnu_stack_note();
    free_dll_spec( spec32 );
}

/*******************************************************************
 *         output_fake_module16
 *
 * Create a fake 16-bit binary module.
 */
void output_fake_module16( DLLSPEC *spec )
{
    static const unsigned char code_segment[] = { 0x90, 0xc3 };
    static const unsigned char data_segment[16] = { 0 };
    static const char fakedll_signature[] = "Wine placeholder DLL";
    const unsigned int cseg = 2;
    const unsigned int lfanew = (0x40 + sizeof(fakedll_signature) + 15) & ~15;
    const unsigned int segtab = lfanew + 0x40;

    unsigned int i, rsrctab, restab, namelen, modtab, imptab, enttab, cbenttab, codeseg, dataseg, rsrcdata;

    init_dll_name( spec );
    init_output_buffer();

    rsrctab = lfanew;
    restab = segtab + 8 * cseg;
    if (spec->nb_resources)
    {
        output_bin_res16_directory( spec, 0 );
        align_output( 2 );
        rsrctab = restab;
        restab += output_buffer_pos;
        free( output_buffer );
        init_output_buffer();
    }

    namelen  = strlen( spec->dll_name );
    modtab   = restab + ((namelen + 3) & ~1);
    imptab   = modtab;
    enttab   = modtab + 2;
    cbenttab = 1;
    codeseg  = (enttab + cbenttab + 1) & ~1;
    dataseg  = codeseg + sizeof(code_segment);
    rsrcdata = dataseg + sizeof(data_segment);

    init_output_buffer();

    put_word( 0x5a4d );       /* e_magic */
    put_word( 0x40 );         /* e_cblp */
    put_word( 0x01 );         /* e_cp */
    put_word( 0 );            /* e_crlc */
    put_word( lfanew / 16 );  /* e_cparhdr */
    put_word( 0x0000 );       /* e_minalloc */
    put_word( 0xffff );       /* e_maxalloc */
    put_word( 0x0000 );       /* e_ss */
    put_word( 0x00b8 );       /* e_sp */
    put_word( 0 );            /* e_csum */
    put_word( 0 );            /* e_ip */
    put_word( 0 );            /* e_cs */
    put_word( lfanew );       /* e_lfarlc */
    put_word( 0 );            /* e_ovno */
    put_dword( 0 );           /* e_res */
    put_dword( 0 );
    put_word( 0 );            /* e_oemid */
    put_word( 0 );            /* e_oeminfo */
    put_dword( 0 );           /* e_res2 */
    put_dword( 0 );
    put_dword( 0 );
    put_dword( 0 );
    put_dword( 0 );
    put_dword( lfanew );

    put_data( fakedll_signature, sizeof(fakedll_signature) );
    align_output( 16 );

    put_word( 0x454e );                    /* ne_magic */
    put_byte( 0 );                         /* ne_ver */
    put_byte( 0 );                         /* ne_rev */
    put_word( enttab - lfanew );           /* ne_enttab */
    put_word( cbenttab );                  /* ne_cbenttab */
    put_dword( 0 );                        /* ne_crc */
    put_word( NE_FFLAGS_SINGLEDATA |       /* ne_flags */
              ((spec->characteristics & IMAGE_FILE_DLL) ? NE_FFLAGS_LIBMODULE : 0) );
    put_word( 2 );                         /* ne_autodata */
    put_word( spec->heap_size );           /* ne_heap */
    put_word( 0 );                         /* ne_stack */
    put_word( 0 ); put_word( 0 );          /* ne_csip */
    put_word( 0 ); put_word( 2 );          /* ne_sssp */
    put_word( cseg );                      /* ne_cseg */
    put_word( 0 );                         /* ne_cmod */
    put_word( 0 );                         /* ne_cbnrestab */
    put_word( segtab - lfanew );           /* ne_segtab */
    put_word( rsrctab - lfanew );          /* ne_rsrctab */
    put_word( restab - lfanew );           /* ne_restab */
    put_word( modtab - lfanew );           /* ne_modtab */
    put_word( imptab - lfanew );           /* ne_imptab */
    put_dword( 0 );                        /* ne_nrestab */
    put_word( 0 );                         /* ne_cmovent */
    put_word( 0 );                         /* ne_align */
    put_word( 0 );                         /* ne_cres */
    put_byte( 2 /*NE_OSFLAGS_WINDOWS*/ );  /* ne_exetyp */
    put_byte( 8 /*NE_AFLAGS_FASTLOAD*/ );  /* ne_flagsothers */
    put_word( 0 );                         /* ne_pretthunks */
    put_word( 0 );                         /* ne_psegrefbytes */
    put_word( 0 );                         /* ne_swaparea */
    put_word( 0 );                         /* ne_expver */

    /* segment table */
    put_word( codeseg );
    put_word( sizeof(code_segment) );
    put_word( 0x2000 /* NE_SEGFLAGS_32BIT */ );
    put_word( sizeof(code_segment) );
    put_word( dataseg );
    put_word( sizeof(data_segment) );
    put_word( 0x0001 /* NE_SEGFLAGS_DATA */ );
    put_word( sizeof(data_segment) );

    /* resource directory */
    if (spec->nb_resources)
    {
        output_bin_res16_directory( spec, rsrcdata );
        align_output( 2 );
    }

    /* resident names table */
    put_byte( namelen );
    for (i = 0; i < namelen; i++) put_byte( toupper(spec->dll_name[i]) );
    put_byte( 0 );
    align_output( 2 );

    /* imported names table */
    put_word( 0 );

    /* entry table */
    put_byte( 0 );
    align_output( 2 );

    /* code segment */
    put_data( code_segment, sizeof(code_segment) );

    /* data segment */
    put_data( data_segment, sizeof(data_segment) );

    /* resource data */
    output_bin_res16_data( spec );

    flush_output_buffer();
}
