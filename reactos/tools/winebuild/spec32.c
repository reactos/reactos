/*
 * 32-bit spec files
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
#include <stdarg.h>
#include <string.h>

#include "winglue.h"

#define EXCEPTION_WINE_STUB       0x80000100  /* stub entry point called */
#define EH_NONCONTINUABLE   0x01

#include "build.h"


/* check if entry point needs a relay thunk */
static inline int needs_relay( const ORDDEF *odp )
{
    /* skip nonexistent entry points */
    if (!odp) return 0;
    /* skip non-functions */
    if ((odp->type != TYPE_STDCALL) && (odp->type != TYPE_CDECL)) return 0;
    /* skip norelay and forward entry points */
    if (odp->flags & (FLAG_NORELAY|FLAG_FORWARD)) return 0;
    return 1;
}

/* check if dll will output relay thunks */
int has_relays( DLLSPEC *spec )
{
    unsigned int i;

    if (target_cpu != CPU_x86) return 0;

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (needs_relay( odp )) return 1;
    }
    return 0;
}

/*******************************************************************
 *         make_internal_name
 *
 * Generate an internal name for an entry point. Used for stubs etc.
 */
static const char *make_internal_name( const ORDDEF *odp, DLLSPEC *spec, const char *prefix )
{
    static char buffer[256];
    if (odp->name || odp->export_name)
    {
        char *p;
        sprintf( buffer, "__wine_%s_%s_%s", prefix, spec->file_name,
                 odp->name ? odp->name : odp->export_name );
        /* make sure name is a legal C identifier */
        for (p = buffer; *p; p++) if (!isalnum(*p) && *p != '_') break;
        if (!*p) return buffer;
    }
    sprintf( buffer, "__wine_%s_%s_%d", prefix, make_c_identifier(spec->file_name), odp->ordinal );
    return buffer;
}


/*******************************************************************
 *         output_relay_debug
 *
 * Output entry points for relay debugging
 */
static void output_relay_debug( FILE *outfile, DLLSPEC *spec )
{
    unsigned int i, j, args, flags;

    /* first the table of entry point offsets */

    fprintf( outfile, "\t%s\n", get_asm_rodata_section() );
    fprintf( outfile, "\t.align %d\n", get_alignment(4) );
    fprintf( outfile, ".L__wine_spec_relay_entry_point_offsets:\n" );

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];

        if (needs_relay( odp ))
            fprintf( outfile, "\t.long .L__wine_spec_relay_entry_point_%d-__wine_spec_relay_entry_points\n", i );
        else
            fprintf( outfile, "\t.long 0\n" );
    }

    /* then the table of argument types */

    fprintf( outfile, "\t.align %d\n", get_alignment(4) );
    fprintf( outfile, ".L__wine_spec_relay_arg_types:\n" );

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        unsigned int mask = 0;

        if (needs_relay( odp ))
        {
            for (j = 0; j < 16 && odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
            }
        }
        fprintf( outfile, "\t.long 0x%08x\n", mask );
    }

    /* then the relay thunks */

    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "__wine_spec_relay_entry_points:\n" );
    fprintf( outfile, "\tnop\n" );  /* to avoid 0 offset */

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];

        if (!needs_relay( odp )) continue;

        fprintf( outfile, "\t.align %d\n", get_alignment(4) );
        fprintf( outfile, ".L__wine_spec_relay_entry_point_%d:\n", i );

        if (odp->flags & FLAG_REGISTER)
            fprintf( outfile, "\tpushl %%eax\n" );
        else
            fprintf( outfile, "\tpushl %%esp\n" );

        args = strlen(odp->u.func.arg_types);
        flags = 0;
        if (odp->flags & FLAG_RET64) flags |= 1;
        if (odp->type == TYPE_STDCALL) flags |= 2;
        fprintf( outfile, "\tpushl $%u\n", (flags << 24) | (args << 16) | (i - spec->base) );

        if (UsePIC)
        {
            fprintf( outfile, "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
            fprintf( outfile, "1:\tleal .L__wine_spec_relay_descr-1b(%%eax),%%eax\n" );
        }
        else fprintf( outfile, "\tmovl $.L__wine_spec_relay_descr,%%eax\n" );
        fprintf( outfile, "\tpushl %%eax\n" );

        if (odp->flags & FLAG_REGISTER)
        {
            fprintf( outfile, "\tcall *8(%%eax)\n" );
        }
        else
        {
            fprintf( outfile, "\tcall *4(%%eax)\n" );
            if (odp->type == TYPE_STDCALL)
                fprintf( outfile, "\tret $%u\n", args * get_ptr_size() );
            else
                fprintf( outfile, "\tret\n" );
        }
    }
}

/*******************************************************************
 *         output_exports
 *
 * Output the export table for a Win32 module.
 */
static void output_exports( FILE *outfile, DLLSPEC *spec )
{
    int i, fwd_size = 0;
    int nr_exports = spec->base <= spec->limit ? spec->limit - spec->base + 1 : 0;

    if (!nr_exports) return;

    fprintf( outfile, "\n/* export table */\n\n" );
    fprintf( outfile, "\t.data\n" );
    fprintf( outfile, "\t.align %d\n", get_alignment(4) );
    fprintf( outfile, ".L__wine_spec_exports:\n" );

    /* export directory header */

    fprintf( outfile, "\t.long 0\n" );                       /* Characteristics */
    fprintf( outfile, "\t.long 0\n" );                       /* TimeDateStamp */
    fprintf( outfile, "\t.long 0\n" );                       /* MajorVersion/MinorVersion */
    fprintf( outfile, "\t.long .L__wine_spec_exp_names-.L__wine_spec_rva_base\n" ); /* Name */
    fprintf( outfile, "\t.long %u\n", spec->base );          /* Base */
    fprintf( outfile, "\t.long %u\n", nr_exports );          /* NumberOfFunctions */
    fprintf( outfile, "\t.long %u\n", spec->nb_names );      /* NumberOfNames */
    fprintf( outfile, "\t.long .L__wine_spec_exports_funcs-.L__wine_spec_rva_base\n" ); /* AddressOfFunctions */
    if (spec->nb_names)
    {
        fprintf( outfile, "\t.long .L__wine_spec_exp_name_ptrs-.L__wine_spec_rva_base\n" ); /* AddressOfNames */
        fprintf( outfile, "\t.long .L__wine_spec_exp_ordinals-.L__wine_spec_rva_base\n" );  /* AddressOfNameOrdinals */
    }
    else
    {
        fprintf( outfile, "\t.long 0\n" );  /* AddressOfNames */
        fprintf( outfile, "\t.long 0\n" );  /* AddressOfNameOrdinals */
    }

    /* output the function pointers */

    fprintf( outfile, "\n.L__wine_spec_exports_funcs:\n" );
    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) fprintf( outfile, "\t.long 0\n" );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            if (odp->flags & FLAG_FORWARD)
            {
                fprintf( outfile, "\t%s .L__wine_spec_forwards+%u\n", get_asm_ptr_keyword(), fwd_size );
                fwd_size += strlen(odp->link_name) + 1;
            }
            else if (odp->flags & FLAG_EXT_LINK)
            {
                fprintf( outfile, "\t%s %s_%s\n",
                         get_asm_ptr_keyword(), asm_name("__wine_spec_ext_link"), odp->link_name );
            }
            else
            {
                fprintf( outfile, "\t%s %s\n", get_asm_ptr_keyword(), asm_name(odp->link_name) );
            }
            break;
        case TYPE_STUB:
            fprintf( outfile, "\t%s %s\n", get_asm_ptr_keyword(),
                     asm_name( get_stub_name( odp, spec )) );
            break;
        default:
            assert(0);
        }
    }

    if (spec->nb_names)
    {
        /* output the function name pointers */

        int namepos = strlen(spec->file_name) + 1;

        fprintf( outfile, "\n.L__wine_spec_exp_name_ptrs:\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "\t.long .L__wine_spec_exp_names+%u-.L__wine_spec_rva_base\n", namepos );
            namepos += strlen(spec->names[i]->name) + 1;
        }

        /* output the function ordinals */

        fprintf( outfile, "\n.L__wine_spec_exp_ordinals:\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "\t%s %d\n",
                     get_asm_short_keyword(), spec->names[i]->ordinal - spec->base );
        }
        if (spec->nb_names % 2)
        {
            fprintf( outfile, "\t%s 0\n", get_asm_short_keyword() );
        }
    }

    /* output the export name strings */

    fprintf( outfile, "\n.L__wine_spec_exp_names:\n" );
    fprintf( outfile, "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );
    for (i = 0; i < spec->nb_names; i++)
        fprintf( outfile, "\t%s \"%s\"\n",
                 get_asm_string_keyword(), spec->names[i]->name );

    /* output forward strings */

    if (fwd_size)
    {
        fprintf( outfile, "\n.L__wine_spec_forwards:\n" );
        for (i = spec->base; i <= spec->limit; i++)
        {
            ORDDEF *odp = spec->ordinals[i];
            if (odp && (odp->flags & FLAG_FORWARD))
                fprintf( outfile, "\t%s \"%s\"\n", get_asm_string_keyword(), odp->link_name );
        }
    }
    fprintf( outfile, "\t.align %d\n", get_alignment(get_ptr_size()) );
    fprintf( outfile, ".L__wine_spec_exports_end:\n" );

    /* output relays */

    /* we only support relay debugging on i386 */
    if (target_cpu != CPU_x86)
    {
        fprintf( outfile, "\t%s 0\n", get_asm_ptr_keyword() );
        return;
    }

    fprintf( outfile, ".L__wine_spec_relay_descr:\n" );
    fprintf( outfile, "\t%s 0xdeb90001\n", get_asm_ptr_keyword() );  /* magic */
    fprintf( outfile, "\t%s 0,0\n", get_asm_ptr_keyword() );         /* relay funcs */
    fprintf( outfile, "\t%s 0\n", get_asm_ptr_keyword() );           /* private data */
    fprintf( outfile, "\t%s __wine_spec_relay_entry_points\n", get_asm_ptr_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_relay_entry_point_offsets\n", get_asm_ptr_keyword() );
    fprintf( outfile, "\t%s .L__wine_spec_relay_arg_types\n", get_asm_ptr_keyword() );

    output_relay_debug( outfile, spec );
}


/*******************************************************************
 *         output_stub_funcs
 *
 * Output the functions for stub entry points
 */
static void output_stub_funcs( FILE *outfile, DLLSPEC *spec )
{
    int i;

#if 0
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "extern void __wine_spec_unimplemented_stub( const char *module, const char *func ) __attribute__((noreturn));\n" );
        fprintf( outfile, "#else\n" );
        fprintf( outfile, "extern void __wine_spec_unimplemented_stub( const char *module, const char *func );\n" );
        fprintf( outfile, "#endif\n\n" );
        break;
    }
#endif

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "void %s(void) ", make_internal_name( odp, spec, "stub" ) );
        if (odp->name)
            fprintf( outfile, "{ __wine_spec_unimplemented_stub(__wine_spec_file_name, \"%s\"); }\n", odp->name );
        else if (odp->export_name)
            fprintf( outfile, "{ __wine_spec_unimplemented_stub(__wine_spec_file_name, \"%s\"); }\n", odp->export_name );
        else
            fprintf( outfile, "{ __wine_spec_unimplemented_stub(__wine_spec_file_name, \"%d\"); }\n", odp->ordinal );
    }
}


/*******************************************************************
 *         output_asm_constructor
 *
 * Output code for calling a dll constructor.
 */
static void output_asm_constructor( FILE *outfile, const char *constructor )
{
    if (target_platform == PLATFORM_APPLE)
    {
        /* Mach-O doesn't have an init section */
        fprintf( outfile, "\n\t.mod_init_func\n" );
        fprintf( outfile, "\t.align %d\n", get_alignment(4) );
        fprintf( outfile, "\t.long %s\n", asm_name(constructor) );
    }
    else
    {
        fprintf( outfile, "\n\t.section \".init\",\"ax\"\n" );
        switch(target_cpu)
        {
        case CPU_x86:
        case CPU_x86_64:
            fprintf( outfile, "\tcall %s\n", asm_name(constructor) );
            break;
        case CPU_SPARC:
            fprintf( outfile, "\tcall %s\n", asm_name(constructor) );
            fprintf( outfile, "\tnop\n" );
            break;
        case CPU_ALPHA:
            fprintf( outfile, "\tjsr $26,%s\n", asm_name(constructor) );
            break;
        case CPU_POWERPC:
            fprintf( outfile, "\tbl %s\n", asm_name(constructor) );
            break;
        }
    }
}


/*******************************************************************
 *         BuildSpec32File
 *
 * Build a Win32 C file from a spec file.
 */
void BuildSpec32File( FILE *outfile, DLLSPEC *spec )
{
    int machine = 0;
    unsigned int page_size = get_page_size();

    resolve_imports( spec );
    output_standard_file_header( outfile );

    /* Reserve some space for the PE header */

    fprintf( outfile, "\t.text\n" );
    fprintf( outfile, "\t.align %d\n", get_alignment(page_size) );
    fprintf( outfile, "__wine_spec_pe_header:\n" );
    if (target_platform == PLATFORM_APPLE)
        fprintf( outfile, "\t.space 65536\n" );
    else
        fprintf( outfile, "\t.skip 65536\n" );

    /* Output the NT header */

    fprintf( outfile, "\n\t.data\n" );
    fprintf( outfile, "\t.align %d\n", get_alignment(get_ptr_size()) );
    fprintf( outfile, "%s\n", asm_globl("__wine_spec_nt_header") );
    fprintf( outfile, ".L__wine_spec_rva_base:\n" );

    fprintf( outfile, "\t.long 0x%04x\n", IMAGE_NT_SIGNATURE );    /* Signature */
    switch(target_cpu)
    {
    case CPU_x86:     machine = IMAGE_FILE_MACHINE_I386; break;
    case CPU_x86_64:  machine = IMAGE_FILE_MACHINE_AMD64; break;
    case CPU_POWERPC: machine = IMAGE_FILE_MACHINE_POWERPC; break;
    case CPU_ALPHA:   machine = IMAGE_FILE_MACHINE_ALPHA; break;
    case CPU_SPARC:   machine = IMAGE_FILE_MACHINE_UNKNOWN; break;
    }
    fprintf( outfile, "\t%s 0x%04x\n",              /* Machine */
             get_asm_short_keyword(), machine );
    fprintf( outfile, "\t%s 0\n",                   /* NumberOfSections */
             get_asm_short_keyword() );
    fprintf( outfile, "\t.long 0\n" );              /* TimeDateStamp */
    fprintf( outfile, "\t.long 0\n" );              /* PointerToSymbolTable */
    fprintf( outfile, "\t.long 0\n" );              /* NumberOfSymbols */
    fprintf( outfile, "\t%s %d\n",                  /* SizeOfOptionalHeader */
             get_asm_short_keyword(),
             get_ptr_size() == 8 ? IMAGE_SIZEOF_NT_OPTIONAL64_HEADER : IMAGE_SIZEOF_NT_OPTIONAL32_HEADER );
    fprintf( outfile, "\t%s 0x%04x\n",              /* Characteristics */
             get_asm_short_keyword(), spec->characteristics );
    fprintf( outfile, "\t%s 0x%04x\n",              /* Magic */
             get_asm_short_keyword(),
             get_ptr_size() == 8 ? IMAGE_NT_OPTIONAL_HDR64_MAGIC : IMAGE_NT_OPTIONAL_HDR32_MAGIC );
    fprintf( outfile, "\t.byte 0\n" );              /* MajorLinkerVersion */
    fprintf( outfile, "\t.byte 0\n" );              /* MinorLinkerVersion */
    fprintf( outfile, "\t.long 0\n" );              /* SizeOfCode */
    fprintf( outfile, "\t.long 0\n" );              /* SizeOfInitializedData */
    fprintf( outfile, "\t.long 0\n" );              /* SizeOfUninitializedData */
    /* note: we expand the AddressOfEntryPoint field on 64-bit by overwriting the BaseOfCode field */
    fprintf( outfile, "\t%s %s\n",                  /* AddressOfEntryPoint */
             get_asm_ptr_keyword(), asm_name(spec->init_func) );
    if (get_ptr_size() == 4)
    {
        fprintf( outfile, "\t.long 0\n" );          /* BaseOfCode */
        fprintf( outfile, "\t.long 0\n" );          /* BaseOfData */
    }
    fprintf( outfile, "\t%s __wine_spec_pe_header\n",         /* ImageBase */
             get_asm_ptr_keyword() );
    fprintf( outfile, "\t.long %u\n", page_size );  /* SectionAlignment */
    fprintf( outfile, "\t.long %u\n", page_size );  /* FileAlignment */
    fprintf( outfile, "\t%s 1,0\n",                 /* Major/MinorOperatingSystemVersion */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s 0,0\n",                 /* Major/MinorImageVersion */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s %u,%u\n",               /* Major/MinorSubsystemVersion */
             get_asm_short_keyword(), spec->subsystem_major, spec->subsystem_minor );
    fprintf( outfile, "\t.long 0\n" );                          /* Win32VersionValue */
    fprintf( outfile, "\t.long %s-.L__wine_spec_rva_base\n",    /* SizeOfImage */
             asm_name("_end") );
    fprintf( outfile, "\t.long %u\n", page_size );  /* SizeOfHeaders */
    fprintf( outfile, "\t.long 0\n" );              /* CheckSum */
    fprintf( outfile, "\t%s 0x%04x\n",              /* Subsystem */
             get_asm_short_keyword(), spec->subsystem );
    fprintf( outfile, "\t%s 0\n",                   /* DllCharacteristics */
             get_asm_short_keyword() );
    fprintf( outfile, "\t%s %u,%u\n",               /* SizeOfStackReserve/Commit */
             get_asm_ptr_keyword(), (spec->stack_size ? spec->stack_size : 1024) * 1024, page_size );
    fprintf( outfile, "\t%s %u,%u\n",               /* SizeOfHeapReserve/Commit */
             get_asm_ptr_keyword(), (spec->heap_size ? spec->heap_size : 1024) * 1024, page_size );
    fprintf( outfile, "\t.long 0\n" );              /* LoaderFlags */
    fprintf( outfile, "\t.long 16\n" );             /* NumberOfRvaAndSizes */

    if (spec->base <= spec->limit)   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] */
        fprintf( outfile, "\t.long .L__wine_spec_exports-.L__wine_spec_rva_base,"
                 ".L__wine_spec_exports_end-.L__wine_spec_exports\n" );
    else
        fprintf( outfile, "\t.long 0,0\n" );

    if (has_imports())   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] */
        fprintf( outfile, "\t.long .L__wine_spec_imports-.L__wine_spec_rva_base,"
                 ".L__wine_spec_imports_end-.L__wine_spec_imports\n" );
    else
        fprintf( outfile, "\t.long 0,0\n" );

    if (spec->nb_resources)   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] */
        fprintf( outfile, "\t.long .L__wine_spec_resources-.L__wine_spec_rva_base,"
                 ".L__wine_spec_resources_end-.L__wine_spec_resources\n" );
    else
        fprintf( outfile, "\t.long 0,0\n" );

    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[3] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[4] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[5] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[6] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[7] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[8] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[9] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[10] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[11] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[12] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[13] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[14] */
    fprintf( outfile, "\t.long 0,0\n" );  /* DataDirectory[15] */

    fprintf( outfile, "\n\t%s\n", get_asm_string_section() );
    fprintf( outfile, "%s\n", asm_globl("__wine_spec_file_name") );
    fprintf( outfile, ".L__wine_spec_file_name:\n" );
    fprintf( outfile, "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );
    if (target_platform == PLATFORM_APPLE)
        fprintf( outfile, "\t.lcomm %s,4\n", asm_name("_end") );

    output_stubs( outfile, spec );
    output_exports( outfile, spec );
    output_imports( outfile, spec );
    output_resources( outfile, spec );
    output_asm_constructor( outfile, "__wine_spec_init_ctor" );
}


/*******************************************************************
 *         BuildDef32File
 *
 * Build a Win32 def file from a spec file.
 */
void BuildDef32File( FILE *outfile, DLLSPEC *spec )
{
    const char *name;
    int i, total;

    if (spec_file_name)
        fprintf( outfile, "; File generated automatically from %s; do not edit!\n\n",
                 spec_file_name );
    else
        fprintf( outfile, "; File generated automatically; do not edit!\n\n" );

    fprintf(outfile, "LIBRARY %s\n\n", spec->file_name);

    fprintf(outfile, "EXPORTS\n");

    /* Output the exports and relay entry points */

    for (i = total = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        int is_data = 0;

        if (!odp) continue;

        if (odp->name) name = odp->name;
        else if (odp->type == TYPE_STUB) name = make_internal_name( odp, spec, "stub" );
        else if (odp->export_name) name = odp->export_name;
        else name = make_internal_name( odp, spec, "noname_export" );

        if (!(odp->flags & FLAG_PRIVATE)) total++;

        fprintf(outfile, "  %s", name);

        switch(odp->type)
        {
        case TYPE_EXTERN:
            is_data = 1;
            /* fall through */
        case TYPE_VARARGS:
        case TYPE_CDECL:
            /* try to reduce output */
            if(strcmp(name, odp->link_name) || (odp->flags & FLAG_FORWARD))
                fprintf(outfile, "=%s", odp->link_name);
            break;
        case TYPE_STDCALL:
        {
            int at_param = strlen(odp->u.func.arg_types) * get_ptr_size();
            if (!kill_at) fprintf(outfile, "@%d", at_param);
            if  (odp->flags & FLAG_FORWARD)
            {
                fprintf(outfile, "=%s", odp->link_name);
            }
            else if (strcmp(name, odp->link_name)) /* try to reduce output */
            {
                fprintf(outfile, "=%s", odp->link_name);
                if (!kill_at) fprintf(outfile, "@%d", at_param);
            }
            break;
        }
        case TYPE_STUB:
        {
            if (!kill_at)
            {
                const char *check = name + strlen(name);
                while (name != check &&
                       '0' <= check[-1] && check[-1] <= '9')
                {
                    check--;
                }
                if (name != check && check != name + strlen(name) &&
                    '@' == check[-1])
                {
                    fprintf(outfile, "%s", check - 1);
                }
            }
            if (NULL != odp->name)
            {
                fprintf(outfile, "=%s", make_internal_name( odp, spec, "stub" ));
            }
            break;
        }
        default:
            assert(0);
        }
        fprintf( outfile, " @%d", odp->ordinal );
#if 0 /* MinGW binutils cannot handle this correctly */
        if (!odp->name) fprintf( outfile, " NONAME" );
#else
        if (!odp->name && (odp->type == TYPE_STUB || odp->export_name)) fprintf( outfile, " NONAME" );
#endif
        if (is_data) fprintf( outfile, " DATA" );
#if 0
        /* MinGW binutils cannot handle this correctly */
        if (odp->flags & FLAG_PRIVATE) fprintf( outfile, " PRIVATE" );
#endif
        fprintf( outfile, "\n" );
    }
    if (!total) warning( "%s: Import library doesn't export anything\n", spec->file_name );
}


/*******************************************************************
 *         BuildPedllFile
 *
 * Build a PE DLL C file from a spec file.
 */
void BuildPedllFile( FILE *outfile, DLLSPEC *spec )
{
    int nr_exports;

    nr_exports = spec->base <= spec->limit ? spec->limit - spec->base + 1 : 0;
    output_standard_file_header( outfile );

    fprintf( outfile, "#include <stdarg.h>\n");
    fprintf( outfile, "#include \"windef.h\"\n");
    fprintf( outfile, "#include \"winbase.h\"\n");
    fprintf( outfile, "#include \"wine/config.h\"\n");
    fprintf( outfile, "#include \"wine/exception.h\"\n\n");

    fprintf( outfile, "void __wine_spec_unimplemented_stub( const char *module, const char *function )\n");
    fprintf( outfile, "{\n");
    fprintf( outfile, "    ULONG_PTR args[2];\n");
    fprintf( outfile, "\n");
    fprintf( outfile, "    args[0] = (ULONG_PTR)module;\n");
    fprintf( outfile, "    args[1] = (ULONG_PTR)function;\n");
    fprintf( outfile, "    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args );\n");
    fprintf( outfile, "}\n\n");

    fprintf( outfile, "static const char __wine_spec_file_name[] = \"%s\";\n\n", spec->file_name );

    if (nr_exports)
    {
        /* Output the stub functions */

        output_stub_funcs( outfile, spec );
    }
}
