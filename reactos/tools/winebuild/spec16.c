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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>

#include "wine/exception.h"
#include "wine/winbase16.h"

#include "build.h"


/*******************************************************************
 *         get_cs
 */
#ifdef __i386__
static inline unsigned short get_cs(void)
{
    unsigned short res;
#ifdef __GNUC__
    __asm__("movw %%cs,%w0" : "=r"(res));
#elif defined(_MSC_VER)
    __asm { mov res, cs }
#else
    res = 0;
#endif
    return res;
}
#endif /* __i386__ */


/*******************************************************************
 *         output_file_header
 *
 * Output a file header with the common declarations we need.
 */
static void output_file_header( FILE *outfile )
{
    output_standard_file_header( outfile );
    fprintf( outfile, "extern struct\n{\n" );
    fprintf( outfile, "  void *base[8192];\n" );
    fprintf( outfile, "  unsigned long limit[8192];\n" );
    fprintf( outfile, "  unsigned char flags[8192];\n" );
    fprintf( outfile, "} wine_ldt_copy;\n\n" );
#ifdef __i386__
    fprintf( outfile, "#define __stdcall __attribute__((__stdcall__))\n\n" );
#else
    fprintf( outfile, "#define __stdcall\n\n" );
#endif
}


/*******************************************************************
 *         output_entry_table
 */
static int output_entry_table( unsigned char **ret_buff, DLLSPEC *spec )
{
    int i, prev = 0, prev_sel = -1;
    unsigned char *pstr, *buffer;
    unsigned char *bundle = NULL;

    buffer = xmalloc( spec->limit * 5 );  /* we use at most 5 bytes per entry-point */
    pstr = buffer;

    for (i = 1; i <= spec->limit; i++)
    {
        int selector = 0;
        WORD offset;
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

        if (!bundle || prev + 1 != i || prev_sel != selector || *bundle == 255)
        {
            /* need to start a new bundle */

            if (prev + 1 != i)
            {
                int skip = i - (prev + 1);
                while (skip > 255)
                {
                    *pstr++ = 255;
                    *pstr++ = 0;
                    skip -= 255;
                }
                *pstr++ = skip;
                *pstr++ = 0;
            }

            bundle = pstr;
            *pstr++ = 0;
            *pstr++ = selector;
            prev_sel = selector;
        }
        /* output the entry */
        *pstr++ = 3;  /* flags: exported & public data */
        offset = odp->offset;
        memcpy( pstr, &offset, sizeof(WORD) );
        pstr += sizeof(WORD);
        (*bundle)++;  /* increment bundle entry count */
        prev = i;
    }
    *pstr++ = 0;
    if ((pstr - buffer) & 1) *pstr++ = 0;
    *ret_buff = xrealloc( buffer, pstr - buffer );
    return pstr - buffer;
}


/*******************************************************************
 *         output_bytes
 */
static void output_bytes( FILE *outfile, const void *buffer, unsigned int size )
{
    unsigned int i;
    const unsigned char *ptr = buffer;

    fprintf( outfile, "  {" );
    for (i = 0; i < size; i++)
    {
        if (!(i & 7)) fprintf( outfile, "\n   " );
        fprintf( outfile, " 0x%02x", *ptr++ );
        if (i < size - 1) fprintf( outfile, "," );
    }
    fprintf( outfile, "\n  },\n" );
}


#ifdef __i386__
/*******************************************************************
 *         BuildCallFrom16Func
 *
 * Build a 16-bit-to-Wine callback glue function.
 *
 * The generated routines are intended to be used as argument conversion
 * routines to be called by the CallFrom16... core. Thus, the prototypes of
 * the generated routines are (see also CallFrom16):
 *
 *  extern WORD WINAPI PREFIX_CallFrom16_C_word_xxx( FARPROC func, LPBYTE args );
 *  extern LONG WINAPI PREFIX_CallFrom16_C_long_xxx( FARPROC func, LPBYTE args );
 *  extern void WINAPI PREFIX_CallFrom16_C_regs_xxx( FARPROC func, LPBYTE args,
 *                                                   CONTEXT86 *context );
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
static void BuildCallFrom16Func( FILE *outfile, const char *profile, const char *prefix )
{
    int i, pos, argsize = 0;
    int short_ret = 0;
    int reg_func = 0;
    int usecdecl = 0;
    int varargs = 0;
    const char *args = profile + 7;
    const char *ret_type;

    /* Parse function type */

    if (!strncmp( "c_", profile, 2 )) usecdecl = 1;
    else if (!strncmp( "v_", profile, 2 )) varargs = usecdecl = 1;
    else if (strncmp( "p_", profile, 2 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    if (!strncmp( "word_", profile + 2, 5 )) short_ret = 1;
    else if (!strncmp( "regs_", profile + 2, 5 )) reg_func = 1;
    else if (strncmp( "long_", profile + 2, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    for ( i = 0; args[i]; i++ )
        switch ( args[i] )
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
        }

    ret_type = reg_func? "void" : short_ret ? "unsigned short" : "unsigned int";

    fprintf( outfile, "typedef %s (%s*proc_%s_t)( ",
             ret_type, usecdecl ? "" : "__stdcall ", profile );
    args = profile + 7;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ", " );
        switch (args[i])
        {
        case 'w':           fprintf( outfile, "unsigned short" ); break;
        case 's':           fprintf( outfile, "short" ); break;
        case 'l': case 'T': fprintf( outfile, "unsigned int" ); break;
        case 'p': case 't': fprintf( outfile, "void *" ); break;
        }
    }
    if (reg_func || varargs)
        fprintf( outfile, "%svoid *", i? ", " : "" );
    else if ( !i )
        fprintf( outfile, "void" );
    fprintf( outfile, " );\n" );

    fprintf( outfile, "static %s __stdcall __wine_%s_CallFrom16_%s( proc_%s_t proc, unsigned char *args%s )\n",
             ret_type, make_c_identifier(prefix), profile, profile,
             reg_func? ", void *context" : "" );

    fprintf( outfile, "{\n    %sproc(\n", reg_func ? "" : "return " );
    args = profile + 7;
    pos = !usecdecl? argsize : 0;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ",\n" );
        fprintf( outfile, "        " );
        switch (args[i])
        {
        case 'w':  /* word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(unsigned short *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 's':  /* s_word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(short *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "*(unsigned int *)(args+%d)", pos );
            if (  usecdecl ) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "((char*)wine_ldt_copy.base[*(unsigned short*)(args+%d) >> 3] + *(unsigned short*)(args+%d))",
                     pos + 2, pos );
            if (  usecdecl ) pos += 4;
            break;

        default:
            fprintf( stderr, "Unknown arg type '%c'\n", args[i] );
        }
    }
    if ( reg_func )
        fprintf( outfile, "%s        context", i? ",\n" : "" );
    else if (varargs)
        fprintf( outfile, "%s        args + %d", i? ",\n" : "", argsize );
    fprintf( outfile, " );\n}\n\n" );
}
#endif


/*******************************************************************
 *         get_function_name
 */
static const char *get_function_name( const ORDDEF *odp )
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
 *         Spec16TypeCompare
 */
static int Spec16TypeCompare( const void *e1, const void *e2 )
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
 *         output_stub_funcs
 *
 * Output the functions for stub entry points
*/
static void output_stub_funcs( FILE *outfile, const DLLSPEC *spec )
{
    int i;
    char *p;

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || odp->type != TYPE_STUB) continue;
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func ) __attribute__((noreturn));\n" );
        fprintf( outfile, "#endif\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func )\n{\n" );
        fprintf( outfile, "  extern void __stdcall RaiseException( unsigned int, unsigned int, unsigned int, const void ** );\n" );
        fprintf( outfile, "  const void *args[2];\n" );
        fprintf( outfile, "  args[0] = \"%s\";\n", spec->file_name );
        fprintf( outfile, "  args[1] = func;\n" );
        fprintf( outfile, "  for (;;) RaiseException( 0x%08x, %d, 2, args );\n}\n\n",
                 EXCEPTION_WINE_STUB, EH_NONCONTINUABLE );
        break;
    }
    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || odp->type != TYPE_STUB) continue;
        odp->link_name = xrealloc( odp->link_name, strlen(odp->name) + 13 );
        strcpy( odp->link_name, "__wine_stub_" );
        strcat( odp->link_name, odp->name );
        for (p = odp->link_name; *p; p++) if (!isalnum(*p)) *p = '_';
        fprintf( outfile, "static void %s(void) { __wine_unimplemented(\"%s\"); }\n",
                 odp->link_name, odp->name );
    }
}


/*******************************************************************
 *         BuildSpec16File
 *
 * Build a Win16 assembly file from a spec file.
 */
void BuildSpec16File( FILE *outfile, DLLSPEC *spec )
{
    ORDDEF **type, **typelist;
    int i, nFuncs, nTypes;
    unsigned char *resdir_buffer, *resdata_buffer, *et_buffer, *data_buffer;
    unsigned char string[256];
    unsigned int ne_offset, segtable_offset, impnames_offset;
    unsigned int entrypoint_size, callfrom_size;
    unsigned int code_size, code_offset;
    unsigned int data_size, data_offset;
    unsigned int resnames_size, resnames_offset;
    unsigned int resdir_size, resdir_offset;
    unsigned int resdata_size, resdata_offset, resdata_align;
    unsigned int et_size, et_offset;

    char constructor[100], destructor[100];
#ifdef __i386__
    unsigned short code_selector = get_cs();
#endif

    /* File header */

    output_file_header( outfile );
    fprintf( outfile, "extern unsigned short __wine_call_from_16_word();\n" );
    fprintf( outfile, "extern unsigned int __wine_call_from_16_long();\n" );
    fprintf( outfile, "extern void __wine_call_from_16_regs();\n" );
    fprintf( outfile, "extern void __wine_call_from_16_thunk();\n" );

    data_buffer = xmalloc( 0x10000 );
    memset( data_buffer, 0, 16 );
    data_size = 16;

    if (!spec->dll_name)  /* set default name from file name */
    {
        char *p;
        spec->dll_name = xstrdup( spec->file_name );
        if ((p = strrchr( spec->dll_name, '.' ))) *p = 0;
    }

    output_stub_funcs( outfile, spec );

    /* Build sorted list of all argument types, without duplicates */

    typelist = (ORDDEF **)calloc( spec->limit+1, sizeof(ORDDEF *) );

    for (i = nFuncs = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_VARARGS:
          case TYPE_STUB:
            typelist[nFuncs++] = odp;

          default:
            break;
        }
    }

    qsort( typelist, nFuncs, sizeof(ORDDEF *), Spec16TypeCompare );

    i = nTypes = 0;
    while ( i < nFuncs )
    {
        typelist[nTypes++] = typelist[i++];
        while ( i < nFuncs && Spec16TypeCompare( typelist + i, typelist + nTypes-1 ) == 0 )
            i++;
    }

    /* Output CallFrom16 routines needed by this .spec file */
#ifdef __i386__
    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101];

        strcpy( profile, get_function_name( typelist[i] ));
        BuildCallFrom16Func( outfile, profile, spec->file_name );
    }
#endif

    /* compute code and data sizes, set offsets, and output prototypes */

#ifdef __i386__
    entrypoint_size = 2 + 5 + 4;    /* pushw bp + pushl target + call */
    callfrom_size = 5 + 7 + 4 + 8;  /* pushl relay + lcall cs:glue + lret n + args */
#else
    entrypoint_size = 4 + 4;  /* target + call */
    callfrom_size = 4 + 8;    /* lret n + args */
#endif
    code_size = nTypes * callfrom_size;

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
        case TYPE_ABS:
            odp->offset = LOWORD(odp->u.abs.value);
            break;
        case TYPE_VARIABLE:
            odp->offset = data_size;
            memcpy( data_buffer + data_size, odp->u.var.values, odp->u.var.n_values * sizeof(int) );
            data_size += odp->u.var.n_values * sizeof(int);
            break;
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
            fprintf( outfile, "extern void %s();\n", odp->link_name );
            /* fall through */
        case TYPE_STUB:
            odp->offset = code_size;
            code_size += entrypoint_size;
            break;
        default:
            assert(0);
            break;
        }
    }
    data_buffer = xrealloc( data_buffer, data_size );  /* free unneeded data */

    /* Output the module structure */

    /* DOS header */

    fprintf( outfile, "\n#include \"pshpack1.h\"\n" );
    fprintf( outfile, "static const struct module_data\n{\n" );
    fprintf( outfile, "  struct\n  {\n" );
    fprintf( outfile, "    unsigned short e_magic;\n" );
    fprintf( outfile, "    unsigned short e_cblp;\n" );
    fprintf( outfile, "    unsigned short e_cp;\n" );
    fprintf( outfile, "    unsigned short e_crlc;\n" );
    fprintf( outfile, "    unsigned short e_cparhdr;\n" );
    fprintf( outfile, "    unsigned short e_minalloc;\n" );
    fprintf( outfile, "    unsigned short e_maxalloc;\n" );
    fprintf( outfile, "    unsigned short e_ss;\n" );
    fprintf( outfile, "    unsigned short e_sp;\n" );
    fprintf( outfile, "    unsigned short e_csum;\n" );
    fprintf( outfile, "    unsigned short e_ip;\n" );
    fprintf( outfile, "    unsigned short e_cs;\n" );
    fprintf( outfile, "    unsigned short e_lfarlc;\n" );
    fprintf( outfile, "    unsigned short e_ovno;\n" );
    fprintf( outfile, "    unsigned short e_res[4];\n" );
    fprintf( outfile, "    unsigned short e_oemid;\n" );
    fprintf( outfile, "    unsigned short e_oeminfo;\n" );
    fprintf( outfile, "    unsigned short e_res2[10];\n" );
    fprintf( outfile, "    unsigned int   e_lfanew;\n" );
    fprintf( outfile, "  } dos_header;\n" );

    /* NE header */

    ne_offset = 64;
    fprintf( outfile, "  struct\n  {\n" );
    fprintf( outfile, "    unsigned short  ne_magic;\n" );
    fprintf( outfile, "    unsigned char   ne_ver;\n" );
    fprintf( outfile, "    unsigned char   ne_rev;\n" );
    fprintf( outfile, "    unsigned short  ne_enttab;\n" );
    fprintf( outfile, "    unsigned short  ne_cbenttab;\n" );
    fprintf( outfile, "    int             ne_crc;\n" );
    fprintf( outfile, "    unsigned short  ne_flags;\n" );
    fprintf( outfile, "    unsigned short  ne_autodata;\n" );
    fprintf( outfile, "    unsigned short  ne_heap;\n" );
    fprintf( outfile, "    unsigned short  ne_stack;\n" );
    fprintf( outfile, "    unsigned int    ne_csip;\n" );
    fprintf( outfile, "    unsigned int    ne_sssp;\n" );
    fprintf( outfile, "    unsigned short  ne_cseg;\n" );
    fprintf( outfile, "    unsigned short  ne_cmod;\n" );
    fprintf( outfile, "    unsigned short  ne_cbnrestab;\n" );
    fprintf( outfile, "    unsigned short  ne_segtab;\n" );
    fprintf( outfile, "    unsigned short  ne_rsrctab;\n" );
    fprintf( outfile, "    unsigned short  ne_restab;\n" );
    fprintf( outfile, "    unsigned short  ne_modtab;\n" );
    fprintf( outfile, "    unsigned short  ne_imptab;\n" );
    fprintf( outfile, "    unsigned int    ne_nrestab;\n" );
    fprintf( outfile, "    unsigned short  ne_cmovent;\n" );
    fprintf( outfile, "    unsigned short  ne_align;\n" );
    fprintf( outfile, "    unsigned short  ne_cres;\n" );
    fprintf( outfile, "    unsigned char   ne_exetyp;\n" );
    fprintf( outfile, "    unsigned char   ne_flagsothers;\n" );
    fprintf( outfile, "    unsigned short  ne_pretthunks;\n" );
    fprintf( outfile, "    unsigned short  ne_psegrefbytes;\n" );
    fprintf( outfile, "    unsigned short  ne_swaparea;\n" );
    fprintf( outfile, "    unsigned short  ne_expver;\n" );
    fprintf( outfile, "  } os2_header;\n" );

    /* segment table */

    segtable_offset = 64;
    fprintf( outfile, "  struct\n  {\n" );
    fprintf( outfile, "    unsigned short filepos;\n" );
    fprintf( outfile, "    unsigned short size;\n" );
    fprintf( outfile, "    unsigned short flags;\n" );
    fprintf( outfile, "    unsigned short minsize;\n" );
    fprintf( outfile, "  } segtable[2];\n" );

    /* resource directory */

    resdir_offset = segtable_offset + 2 * 8;
    resdir_size = get_res16_directory_size( spec );
    fprintf( outfile, "  unsigned char resdir[%d];\n", resdir_size );

    /* resident names table */

    resnames_offset = resdir_offset + resdir_size;
    fprintf( outfile, "  struct\n  {\n" );
    fprintf( outfile, "    struct { unsigned char len; char name[%d]; unsigned short ord; } name_0;\n",
             strlen( spec->dll_name ) );
    resnames_size = 3 + strlen( spec->dll_name );
    for (i = 1; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !odp->name[0]) continue;
        fprintf( outfile, "    struct { unsigned char len; char name[%d]; unsigned short ord; } name_%d;\n",
                 strlen(odp->name), i );
        resnames_size += 3 + strlen( odp->name );
    }
    fprintf( outfile, "    unsigned char name_last[%d];\n", 2 - (resnames_size & 1) );
    resnames_size = (resnames_size + 2) & ~1;
    fprintf( outfile, "  } resnames;\n" );

    /* imported names table */

    impnames_offset = resnames_offset + resnames_size;
    fprintf( outfile, "  unsigned char impnames[2];\n" );

    /* entry table */

    et_offset = impnames_offset + 2;
    et_size = output_entry_table( &et_buffer, spec );
    fprintf( outfile, "  unsigned char entry_table[%d];\n", et_size );

    /* code segment */

    code_offset = et_offset + et_size;
    fprintf( outfile, "  struct {\n" );
#ifdef __i386__
    fprintf( outfile, "    unsigned char pushl;\n" );      /* pushl $relay */
    fprintf( outfile, "    void *relay;\n" );
    fprintf( outfile, "    unsigned char lcall;\n" );      /* lcall __FLATCS__:glue */
    fprintf( outfile, "    void *glue;\n" );
    fprintf( outfile, "    unsigned short flatcs;\n" );
#endif
    fprintf( outfile, "    unsigned short lret;\n" );      /* lret $args */
    fprintf( outfile, "    unsigned short args;\n" );
    fprintf( outfile, "    unsigned int arg_types[2];\n" );
    fprintf( outfile, "  } call[%d];\n", nTypes );
    fprintf( outfile, "  struct {\n" );
#ifdef __i386__
    fprintf( outfile, "    unsigned short pushw_bp;\n" );  /* pushw %bp */
    fprintf( outfile, "    unsigned char pushl;\n" );      /* pushl $target */
#endif
    fprintf( outfile, "    void (*target)();\n" );
    fprintf( outfile, "    unsigned short call;\n" );      /* call CALLFROM16 */
    fprintf( outfile, "    short callfrom16;\n" );
    fprintf( outfile, "  } entry[%d];\n", nFuncs );

    /* data segment */

    data_offset = code_offset + code_size;
    fprintf( outfile, "  unsigned char data_segment[%d];\n", data_size );
    if (data_offset + data_size >= 0x10000)
        fatal_error( "Not supported yet: 16-bit module data larger than 64K\n" );

    /* resource data */

    resdata_offset = ne_offset + data_offset + data_size;
    for (resdata_align = 0; resdata_align < 16; resdata_align++)
    {
        unsigned int size = get_res16_data_size( spec, resdata_offset, resdata_align );
        if ((resdata_offset + size) >> resdata_align <= 0xffff) break;
    }
    output_res16_directory( &resdir_buffer, spec, resdata_offset, resdata_align );
    resdata_size = output_res16_data( &resdata_buffer, spec, resdata_offset, resdata_align );
    if (resdata_size) fprintf( outfile, "  unsigned char resources[%d];\n", resdata_size );

    /* Output the module data */

    /* DOS header */

    fprintf( outfile, "} module =\n{\n  {\n" );
    fprintf( outfile, "    0x%04x,\n", IMAGE_DOS_SIGNATURE );  /* e_magic */
    fprintf( outfile, "    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\n" );
    fprintf( outfile, "    { 0, 0, 0, 0, }, 0, 0,\n" );
    fprintf( outfile, "    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },\n" );
    fprintf( outfile, "    sizeof(module.dos_header)\n" );  /* e_lfanew */

    /* NE header */

    fprintf( outfile, "  },\n  {\n" );
    fprintf( outfile, "    0x%04x,\n", IMAGE_OS2_SIGNATURE ); /* ne_magic */
    fprintf( outfile, "    0, 0,\n" );
    fprintf( outfile, "    %d,\n", et_offset );               /* ne_enttab */
    fprintf( outfile, "    sizeof(module.entry_table),\n" );  /* ne_cbenttab */
    fprintf( outfile, "    0,\n" );                           /* ne_crc */
    fprintf( outfile, "    0x%04x,\n",                        /* ne_flags */
             NE_FFLAGS_SINGLEDATA | NE_FFLAGS_LIBMODULE );
    fprintf( outfile, "    2,\n" );                           /* ne_autodata */
    fprintf( outfile, "    %d,\n", spec->heap_size );         /* ne_heap */
    fprintf( outfile, "    0, 0, 0,\n" );
    fprintf( outfile, "    2,\n" );                           /* ne_cseg */
    fprintf( outfile, "    0,\n" );                           /* ne_cmod */
    fprintf( outfile, "    0,\n" );                           /* ne_cbnrestab */
    fprintf( outfile, "    %d,\n", segtable_offset );         /* ne_segtab */
    fprintf( outfile, "    %d,\n", resdir_offset );           /* ne_rsrctab */
    fprintf( outfile, "    %d,\n", resnames_offset );         /* ne_restab */
    fprintf( outfile, "    %d,\n", impnames_offset );         /* ne_modtab */
    fprintf( outfile, "    %d,\n", impnames_offset );         /* ne_imptab */
    fprintf( outfile, "    0,\n" );                           /* ne_nrestab */
    fprintf( outfile, "    0,\n" );                           /* ne_cmovent */
    fprintf( outfile, "    0,\n" );                           /* ne_align */
    fprintf( outfile, "    0,\n" );                           /* ne_cres */
    fprintf( outfile, "    0x%04x,\n", NE_OSFLAGS_WINDOWS );  /* ne_exetyp */
    fprintf( outfile, "    0x%04x,\n", NE_AFLAGS_FASTLOAD );  /* ne_flagsothers */
    fprintf( outfile, "    0,\n" );                           /* ne_pretthunks */
    fprintf( outfile, "    0,\n" );                           /* ne_psegrefbytes */
    fprintf( outfile, "    0,\n" );                           /* ne_swaparea */
    fprintf( outfile, "    0\n" );                            /* ne_expver */
    fprintf( outfile, "  },\n" );

    /* segment table */

    fprintf( outfile, "  {\n" );
    fprintf( outfile, "    { %d, %d, 0x%04x, %d },\n",
             ne_offset + code_offset, code_size, NE_SEGFLAGS_32BIT, code_size );
    fprintf( outfile, "    { %d, %d, 0x%04x, %d },\n",
             ne_offset + data_offset, data_size, NE_SEGFLAGS_DATA, data_size );
    fprintf( outfile, "  },\n" );

    /* resource directory */

    output_bytes( outfile, resdir_buffer, resdir_size );
    free( resdir_buffer );

    /* resident names table */

    fprintf( outfile, "  {\n" );
    strcpy( string, spec->dll_name );
    fprintf( outfile, "    { %d, \"%s\", 0 },\n", strlen(string), strupper(string) );
    for (i = 1; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !odp->name[0]) continue;
        strcpy( string, odp->name );
        fprintf( outfile, "    { %d, \"%s\", %d },\n", strlen(string), strupper(string), i );
    }
    fprintf( outfile, "    { 0 }\n  },\n" );

    /* imported names table */

    fprintf( outfile, "  { 0, 0 },\n" );

    /* entry table */

    output_bytes( outfile, et_buffer, et_size );
    free( et_buffer );

    /* code segment */

    fprintf( outfile, "  {\n" );
    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101], *arg;
        unsigned int arg_types[2];
        int j, argsize = 0;

        strcpy( profile, get_function_name( typelist[i] ));
        if ( typelist[i]->type == TYPE_PASCAL )
            for ( arg = typelist[i]->u.func.arg_types; *arg; arg++ )
                switch ( *arg )
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
                }

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
        if (typelist[i]->flags & FLAG_REGISTER) arg_types[0] |= ARG_REGISTER;
        if (typelist[i]->flags & FLAG_RET16) arg_types[0] |= ARG_RET16;

#ifdef __i386__
        fprintf( outfile, "    { 0x68, __wine_%s_CallFrom16_%s, 0x9a, __wine_call_from_16_%s,\n",
                 make_c_identifier(spec->file_name), profile,
                 (typelist[i]->flags & FLAG_REGISTER) ? "regs":
                 (typelist[i]->flags & FLAG_RET16) ? "word" : "long" );
        if (argsize)
            fprintf( outfile, "      0x%04x, 0xca66, %d, { 0x%08x, 0x%08x } },\n",
                     code_selector, argsize, arg_types[0], arg_types[1] );
        else
            fprintf( outfile, "      0x%04x, 0xcb66, 0x9090, { 0x%08x, 0x%08x } },\n",
                     code_selector, arg_types[0], arg_types[1] );
#else
        if (argsize)
            fprintf( outfile, "    { 0xca66, %d, { 0x%08x, 0x%08x } },\n",
                     argsize, arg_types[0], arg_types[1] );
        else
            fprintf( outfile, "     { 0xcb66, 0x9090, { 0x%08x, 0x%08x } },\n",
                     arg_types[0], arg_types[1] );
#endif
    }
    fprintf( outfile, "  },\n  {\n" );

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_VARARGS:
          case TYPE_STUB:
            type = bsearch( &odp, typelist, nTypes, sizeof(ORDDEF *), Spec16TypeCompare );
            assert( type );

            fprintf( outfile, "    /* %s.%d */ ", spec->dll_name, i );
            fprintf( outfile,
#ifdef __i386__
                     "{ 0x5566, 0x68, %s, 0xe866, %d  /* %s */ },\n",
#else
                     "{ %s, 0xe866, %d, /* %s */ },\n",
#endif
                     odp->link_name,
                     (type - typelist) * callfrom_size - (odp->offset + entrypoint_size),
                     get_function_name( odp ) );
            break;
        default:
            break;
        }
    }
    fprintf( outfile, "  },\n" );


    /* data_segment */

    output_bytes( outfile, data_buffer, data_size );
    free( data_buffer );

    /* resource data */

    if (resdata_size)
    {
        output_bytes( outfile, resdata_buffer, resdata_size );
        free( resdata_buffer );
    }

    fprintf( outfile, "};\n" );
    fprintf( outfile, "#include \"poppack.h\"\n\n" );

    /* Output the DLL constructor */

    sprintf( constructor, "__wine_spec_%s_init", make_c_identifier(spec->file_name) );
    sprintf( destructor, "__wine_spec_%s_fini", make_c_identifier(spec->file_name) );
    output_dll_init( outfile, constructor, destructor );

    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void __wine_dll_register_16( const struct module_data *, const char * );\n"
             "    __wine_dll_register_16( &module, \"%s\" );\n"
             "}\n", constructor, spec->file_name );
    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void __wine_dll_unregister_16( const struct module_data * );\n"
             "    __wine_dll_unregister_16( &module );\n"
             "}\n", destructor );
}
