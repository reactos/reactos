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
#include "stackframe.h"
#include "builtin16.h"
#include "module.h"

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
 *         StoreVariableCode
 *
 * Store a list of ints into a byte array.
 */
static int StoreVariableCode( unsigned char *buffer, int size, ORDDEF *odp )
{
    int i;

    switch(size)
    {
    case 1:
        for (i = 0; i < odp->u.var.n_values; i++)
            buffer[i] = odp->u.var.values[i];
        break;
    case 2:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned short *)buffer)[i] = odp->u.var.values[i];
        break;
    case 4:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned int *)buffer)[i] = odp->u.var.values[i];
        break;
    }
    return odp->u.var.n_values * size;
}


/*******************************************************************
 *         BuildModule16
 *
 * Build the in-memory representation of a 16-bit NE module, and dump it
 * as a byte stream into the assembly code.
 */
static int BuildModule16( FILE *outfile, int max_code_offset,
                          int max_data_offset, DLLSPEC *spec )
{
    int i;
    char *buffer;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    OFSTRUCT *pFileInfo;
    BYTE *pstr;
    ET_BUNDLE *bundle = 0;
    ET_ENTRY entry;

    /*   Module layout:
     * NE_MODULE       Module
     * OFSTRUCT        File information
     * SEGTABLEENTRY   Segment 1 (code)
     * SEGTABLEENTRY   Segment 2 (data)
     * WORD[2]         Resource table (empty)
     * BYTE[2]         Imported names (empty)
     * BYTE[n]         Resident names table
     * BYTE[n]         Entry table
     */

    buffer = xmalloc( 0x10000 );
    memset( buffer, 0, 0x10000 );

    pModule = (NE_MODULE *)buffer;
    pModule->magic = IMAGE_OS2_SIGNATURE;
    pModule->count = 1;
    pModule->next = 0;
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_BUILTIN | NE_FFLAGS_LIBMODULE;
    pModule->dgroup = 2;
    pModule->heap_size = spec->heap_size;
    pModule->stack_size = 0;
    pModule->ip = 0;
    pModule->cs = 0;
    pModule->sp = 0;
    pModule->ss = 0;
    pModule->seg_count = 2;
    pModule->modref_count = 0;
    pModule->nrname_size = 0;
    pModule->modref_table = 0;
    pModule->nrname_fpos = 0;
    pModule->moveable_entries = 0;
    pModule->alignment = 0;
    pModule->truetype = 0;
    pModule->os_flags = NE_OSFLAGS_WINDOWS;
    pModule->misc_flags = 0;
    pModule->dlls_to_init  = 0;
    pModule->nrname_handle = 0;
    pModule->min_swap_area = 0;
    pModule->expected_version = 0;
    pModule->module32 = 0;
    pModule->self = 0;
    pModule->self_loading_sel = 0;

      /* File information */

    pFileInfo = (OFSTRUCT *)(pModule + 1);
    pModule->fileinfo = (int)pFileInfo - (int)pModule;
    memset( pFileInfo, 0, sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName) );
    pFileInfo->cBytes = sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName)
                        + strlen(spec->file_name);
    strcpy( pFileInfo->szPathName, spec->file_name );
    pstr = (char *)pFileInfo + pFileInfo->cBytes + 1;

      /* Segment table */

    pstr = (char *)(((long)pstr + 3) & ~3);
    pSegment = (SEGTABLEENTRY *)pstr;
    pModule->seg_table = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_code_offset;
    pSegment->flags = 0;
    pSegment->minsize = max_code_offset;
    pSegment->hSeg = 0;
    pSegment++;

    pModule->dgroup_entry = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_data_offset;
    pSegment->flags = NE_SEGFLAGS_DATA;
    pSegment->minsize = max_data_offset;
    pSegment->hSeg = 0;
    pSegment++;

      /* Resource table */

    pstr = (char *)pSegment;
    pstr = (char *)(((long)pstr + 3) & ~3);
    pModule->res_table = (int)pstr - (int)pModule;
    pstr += output_res16_directory( pstr, spec );

      /* Imported names table */

    pstr = (char *)(((long)pstr + 3) & ~3);
    pModule->import_table = (int)pstr - (int)pModule;
    *pstr++ = 0;
    *pstr++ = 0;

      /* Resident names table */

    pstr = (char *)(((long)pstr + 3) & ~3);
    pModule->name_table = (int)pstr - (int)pModule;
    /* First entry is module name */
    *pstr = strlen( spec->dll_name );
    strcpy( pstr + 1, spec->dll_name );
    strupper( pstr + 1 );
    pstr += *pstr + 1;
    *pstr++ = 0;
    *pstr++ = 0;
    /* Store all ordinals */
    for (i = 1; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        WORD ord = i;
        if (!odp || !odp->name[0]) continue;
        *pstr = strlen( odp->name );
        strcpy( pstr + 1, odp->name );
        strupper( pstr + 1 );
        pstr += *pstr + 1;
        memcpy( pstr, &ord, sizeof(WORD) );
        pstr += sizeof(WORD);
    }
    *pstr++ = 0;

      /* Entry table */

    pstr = (char *)(((long)pstr + 3) & ~3);
    pModule->entry_table = (int)pstr - (int)pModule;
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
            selector = 0;  /* Invalid selector */
            break;
        }

        if ( !selector )
           continue;

        if ( bundle && bundle->last+1 == i )
            bundle->last++;
        else
        {
            pstr = (char *)(((long)pstr + 1) & ~1);
            if ( bundle )
                bundle->next = (char *)pstr - (char *)pModule;

            bundle = (ET_BUNDLE *)pstr;
            bundle->first = i-1;
            bundle->last = i;
            bundle->next = 0;
            pstr += sizeof(ET_BUNDLE);
        }

	/* FIXME: is this really correct ?? */
	entry.type = 0xff;  /* movable */
	entry.flags = 3; /* exported & public data */
	entry.segnum = selector;
	entry.offs = odp->offset;
        memcpy( pstr, &entry, sizeof(ET_ENTRY) );
	pstr += sizeof(ET_ENTRY);
    }
    *pstr++ = 0;

      /* Dump the module content */

    pstr = (char *)(((long)pstr + 3) & ~3);
    dump_bytes( outfile, (char *)pModule, (int)pstr - (int)pModule, "Module", 0 );
    return (int)pstr - (int)pModule;
}


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
        fprintf( outfile, "  struct exc_record {\n" );
        fprintf( outfile, "    unsigned int code, flags;\n" );
        fprintf( outfile, "    void *rec, *addr;\n" );
        fprintf( outfile, "    unsigned int params;\n" );
        fprintf( outfile, "    const void *info[15];\n" );
        fprintf( outfile, "  } rec;\n\n" );
        fprintf( outfile, "  extern void __stdcall RtlRaiseException( struct exc_record * );\n\n" );
        fprintf( outfile, "  rec.code    = 0x%08x;\n", EXCEPTION_WINE_STUB );
        fprintf( outfile, "  rec.flags   = %d;\n", EH_NONCONTINUABLE );
        fprintf( outfile, "  rec.rec     = 0;\n" );
        fprintf( outfile, "  rec.params  = 2;\n" );
        fprintf( outfile, "  rec.info[0] = \"%s\";\n", spec->file_name );
        fprintf( outfile, "  rec.info[1] = func;\n" );
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "  rec.addr = __builtin_return_address(1);\n" );
        fprintf( outfile, "#else\n" );
        fprintf( outfile, "  rec.addr = 0;\n" );
        fprintf( outfile, "#endif\n" );
        fprintf( outfile, "  for (;;) RtlRaiseException( &rec );\n}\n\n" );
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
    int code_offset, data_offset, module_size, res_size;
    unsigned char *data;
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

    data = (unsigned char *)xmalloc( 0x10000 );
    memset( data, 0, 16 );
    data_offset = 16;

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

    /* Output the DLL functions prototypes */

    for (i = 0; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) continue;
        switch(odp->type)
        {
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_VARARGS:
            fprintf( outfile, "extern void %s();\n", odp->link_name );
            break;
        default:
            break;
        }
    }

    /* Output code segment */

    fprintf( outfile, "\n#include \"pshpack1.h\"\n" );
    fprintf( outfile, "\nstatic struct code_segment\n{\n" );
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
    fprintf( outfile, "} code_segment =\n{\n  {\n" );

    code_offset = 0;

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
        code_offset += sizeof(CALLFROM16);
    }
    fprintf( outfile, "  },\n  {\n" );

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
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 4, odp);
            break;

          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_VARARGS:
          case TYPE_STUB:
            type = bsearch( &odp, typelist, nTypes, sizeof(ORDDEF *), Spec16TypeCompare );
            assert( type );

            fprintf( outfile, "    /* %s.%d */ ", spec->dll_name, i );
#ifdef __i386__
            fprintf( outfile, "{ 0x5566, 0x68, %s, 0xe866, %d  /* %s */ },\n",
#else
            fprintf( outfile, "{ %s, 0xe866, %d, /* %s */ },\n",
#endif
                     odp->link_name,
                     (type-typelist)*sizeof(CALLFROM16) -
                     (code_offset + sizeof(ENTRYPOINT16)),
                     get_function_name( odp ) );

            odp->offset = code_offset;
            code_offset += sizeof(ENTRYPOINT16);
            break;

          default:
            fprintf(stderr,"build: function type %d not available for Win16\n",
                    odp->type);
            exit(1);
        }
    }

    fprintf( outfile, "    }\n};\n" );

    /* Output data segment */

    dump_bytes( outfile, data, data_offset, "Data_Segment", 0 );

    /* Build the module */

    module_size = BuildModule16( outfile, code_offset, data_offset, spec );
    res_size = output_res16_data( outfile, spec );

    /* Output the DLL descriptor */

    fprintf( outfile, "#include \"poppack.h\"\n\n" );

    fprintf( outfile, "static const struct dll_descriptor\n{\n" );
    fprintf( outfile, "    unsigned char *module_start;\n" );
    fprintf( outfile, "    int module_size;\n" );
    fprintf( outfile, "    struct code_segment *code_start;\n" );
    fprintf( outfile, "    unsigned char *data_start;\n" );
    fprintf( outfile, "    const char *owner;\n" );
    fprintf( outfile, "    const unsigned char *rsrc;\n" );
    fprintf( outfile, "} descriptor =\n{\n" );
    fprintf( outfile, "    Module,\n" );
    fprintf( outfile, "    sizeof(Module),\n" );
    fprintf( outfile, "    &code_segment,\n" );
    fprintf( outfile, "    Data_Segment,\n" );
    fprintf( outfile, "    \"%s\",\n", spec->owner_name );
    fprintf( outfile, "    %s\n", res_size ? "resource_data" : "0" );
    fprintf( outfile, "};\n" );

    /* Output the DLL constructor */

    sprintf( constructor, "__wine_spec_%s_init", make_c_identifier(spec->file_name) );
    sprintf( destructor, "__wine_spec_%s_fini", make_c_identifier(spec->file_name) );
    output_dll_init( outfile, constructor, destructor );

    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void __wine_register_dll_16( const struct dll_descriptor *descr );\n"
             "    __wine_register_dll_16( &descriptor );\n"
             "}\n", constructor );
    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void __wine_unregister_dll_16( const struct dll_descriptor *descr );\n"
             "    __wine_unregister_dll_16( &descriptor );\n"
             "}\n", destructor );
}
