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


#ifdef __APPLE__
# define __ASM_SKIP ".space"
#else
# define __ASM_SKIP ".skip"
#endif

static int string_compare( const void *ptr1, const void *ptr2 )
{
    const char * const *str1 = ptr1;
    const char * const *str2 = ptr2;
    return strcmp( *str1, *str2 );
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
 *         declare_weak_function
 *
 * Output a prototype for a weak function.
 */
static void declare_weak_function( FILE *outfile, const char *ret_type, const char *name, const char *params)
{
    fprintf( outfile, "#ifdef __GNUC__\n" );
    fprintf( outfile, "# ifdef __APPLE__\n" );
    fprintf( outfile, "extern %s %s(%s) __attribute__((weak_import));\n", ret_type, name, params );
    fprintf( outfile, "static %s (*__wine_spec_weak_%s)(%s) = %s;\n", ret_type, name, params, name );
    fprintf( outfile, "#define %s __wine_spec_weak_%s\n", name, name );
    fprintf( outfile, "# else\n" );
    fprintf( outfile, "extern %s %s(%s) __attribute__((weak));\n", ret_type, name, params );
    fprintf( outfile, "# endif\n" );
    fprintf( outfile, "#else\n" );
    fprintf( outfile, "extern %s %s(%s);\n", ret_type, name, params );
    fprintf( outfile, "static void __asm__dummy_%s(void)", name );
    fprintf( outfile, " { asm(\".weak " __ASM_NAME("%s") "\"); }\n", name );
    fprintf( outfile, "#endif\n\n" );
}


/*******************************************************************
 *         output_debug
 *
 * Output the debug channels.
 */
static int output_debug( FILE *outfile )
{
    int i;

    if (!nb_debug_channels) return 0;
    qsort( debug_channels, nb_debug_channels, sizeof(debug_channels[0]), string_compare );

    for (i = 0; i < nb_debug_channels; i++)
        fprintf( outfile, "char __wine_dbch_%s[] = \"\\003%s\";\n",
                 debug_channels[i], debug_channels[i] );

    fprintf( outfile, "\nstatic char * const debug_channels[%d] =\n{\n", nb_debug_channels );
    for (i = 0; i < nb_debug_channels; i++)
    {
        fprintf( outfile, "    __wine_dbch_%s", debug_channels[i] );
        if (i < nb_debug_channels - 1) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );
    fprintf( outfile, "static void *debug_registration;\n\n" );

    return nb_debug_channels;
}


/*******************************************************************
 *         output_exports
 *
 * Output the export table for a Win32 module.
 */
static int output_exports( FILE *outfile, int nr_exports, DLLSPEC *spec )
{
    int i, fwd_size = 0, total_size = 0;

    if (!nr_exports) return 0;

    fprintf( outfile, "asm(\".data\\n\"\n" );
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
    fprintf( outfile, "    \"" __ASM_NAME("__wine_spec_exports") ":\\n\"\n" );

    /* export directory header */

    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* Characteristics */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* TimeDateStamp */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* MajorVersion/MinorVersion */
    fprintf( outfile, "    \"\\t.long __wine_spec_exp_names\\n\"\n" ); /* Name */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", spec->base );        /* Base */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", nr_exports );        /* NumberOfFunctions */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", spec->nb_names );    /* NumberOfNames */
    fprintf( outfile, "    \"\\t.long __wine_spec_exports_funcs\\n\"\n" ); /* AddressOfFunctions */
    if (spec->nb_names)
    {
        fprintf( outfile, "    \"\\t.long __wine_spec_exp_name_ptrs\\n\"\n" );     /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long __wine_spec_exp_ordinals\\n\"\n" );  /* AddressOfNameOrdinals */
    }
    else
    {
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNameOrdinals */
    }
    total_size += 10 * sizeof(int);

    /* output the function pointers */

    fprintf( outfile, "    \"__wine_spec_exports_funcs:\\n\"\n" );
    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) fprintf( outfile, "    \"\\t.long 0\\n\"\n" );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            if (!(odp->flags & FLAG_FORWARD))
            {
                fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") "\\n\"\n",
                         (odp->flags & FLAG_REGISTER) ? make_internal_name( odp, spec, "regs" )
                         : odp->link_name );
            }
            else
            {
                fprintf( outfile, "    \"\\t.long __wine_spec_forwards+%d\\n\"\n", fwd_size );
                fwd_size += strlen(odp->link_name) + 1;
            }
            break;
        case TYPE_STUB:
            fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") "\\n\"\n",
                     make_internal_name( odp, spec, "stub" ) );
            break;
        default:
            assert(0);
        }
    }
    total_size += (spec->limit - spec->base + 1) * sizeof(int);

    if (spec->nb_names)
    {
        /* output the function name pointers */

        int namepos = strlen(spec->file_name) + 1;

        fprintf( outfile, "    \"__wine_spec_exp_name_ptrs:\\n\"\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "    \"\\t.long __wine_spec_exp_names+%d\\n\"\n", namepos );
            namepos += strlen(spec->names[i]->name) + 1;
        }
        total_size += spec->nb_names * sizeof(int);
    }

    /* output the function names */

    fprintf( outfile, "    \"\\t.text\\n\"\n" );
    fprintf( outfile, "    \"__wine_spec_exp_names:\\n\"\n" );
    fprintf( outfile, "    \"\\t" __ASM_STRING " \\\"%s\\\"\\n\"\n", spec->file_name );
    for (i = 0; i < spec->nb_names; i++)
        fprintf( outfile, "    \"\\t" __ASM_STRING " \\\"%s\\\"\\n\"\n", spec->names[i]->name );
    fprintf( outfile, "    \"\\t.data\\n\"\n" );

    if (spec->nb_names)
    {
        /* output the function ordinals */

        fprintf( outfile, "    \"__wine_spec_exp_ordinals:\\n\"\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "    \"\\t" __ASM_SHORT " %d\\n\"\n",
                     spec->names[i]->ordinal - spec->base );
        }
        total_size += spec->nb_names * sizeof(short);
        if (spec->nb_names % 2)
        {
            fprintf( outfile, "    \"\\t" __ASM_SHORT " 0\\n\"\n" );
            total_size += sizeof(short);
        }
    }

    /* output forward strings */

    if (fwd_size)
    {
        fprintf( outfile, "    \"__wine_spec_forwards:\\n\"\n" );
        for (i = spec->base; i <= spec->limit; i++)
        {
            ORDDEF *odp = spec->ordinals[i];
            if (odp && (odp->flags & FLAG_FORWARD))
                fprintf( outfile, "    \"\\t" __ASM_STRING " \\\"%s\\\"\\n\"\n", odp->link_name );
        }
        fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
        total_size += (fwd_size + 3) & ~3;
    }

    /* output relays */

    if (debugging)
    {
        for (i = spec->base; i <= spec->limit; i++)
        {
            ORDDEF *odp = spec->ordinals[i];
            unsigned int j, args, mask = 0;
            const char *name;

            /* skip non-existent entry points */
            if (!odp) goto ignore;
            /* skip non-functions */
            if ((odp->type != TYPE_STDCALL) && (odp->type != TYPE_CDECL)) goto ignore;
            /* skip norelay and forward entry points */
            if (odp->flags & (FLAG_NORELAY|FLAG_FORWARD)) goto ignore;

            for (j = 0; odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
            }
            if ((odp->flags & FLAG_RET64) && (j < 16)) mask |= 0x80000000;

            name = odp->link_name;
            args = strlen(odp->u.func.arg_types) * sizeof(int);
            if (odp->flags & FLAG_REGISTER) name = make_internal_name( odp, spec, "regs" );

            switch(odp->type)
            {
            case TYPE_STDCALL:
                fprintf( outfile, "    \"\\tjmp " __ASM_NAME("%s") "\\n\"\n", name );
                fprintf( outfile, "    \"\\tret $%d\\n\"\n", args );
                fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") ",0x%08x\\n\"\n", name, mask );
                break;
            case TYPE_CDECL:
                fprintf( outfile, "    \"\\tjmp " __ASM_NAME("%s") "\\n\"\n", name );
                fprintf( outfile, "    \"\\tret\\n\"\n" );
                fprintf( outfile, "    \"\\t" __ASM_SHORT " %d\\n\"\n", args );
                fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") ",0x%08x\\n\"\n", name, mask );
                break;
            default:
                assert(0);
            }
            continue;

        ignore:
            fprintf( outfile, "    \"\\t.long 0,0,0,0\\n\"\n" );
        }
    }

    fprintf( outfile, "    \"\\t.text\\n\"\n" );
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
    fprintf( outfile, ");\n\n" );

    return total_size;
}


/*******************************************************************
 *         output_stub_funcs
 *
 * Output the functions for stub entry points
*/
static void output_stub_funcs( FILE *outfile, DLLSPEC *spec )
{
    int i;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func ) __attribute__((noreturn));\n" );
        fprintf( outfile, "#endif\n\n" );
        fprintf( outfile, "struct exc_record {\n" );
        fprintf( outfile, "  unsigned int code, flags;\n" );
        fprintf( outfile, "  void *rec, *addr;\n" );
        fprintf( outfile, "  unsigned int params;\n" );
        fprintf( outfile, "  const void *info[15];\n" );
        fprintf( outfile, "};\n\n" );
        fprintf( outfile, "extern void __stdcall RtlRaiseException( struct exc_record * );\n\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func )\n{\n" );
        fprintf( outfile, "  struct exc_record rec;\n" );
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

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "void %s(void) ", make_internal_name( odp, spec, "stub" ) );
        if (odp->name)
            fprintf( outfile, "{ __wine_unimplemented(\"%s\"); }\n", odp->name );
        else if (odp->export_name)
            fprintf( outfile, "{ __wine_unimplemented(\"%s\"); }\n", odp->export_name );
        else
            fprintf( outfile, "{ __wine_unimplemented(\"%d\"); }\n", odp->ordinal );
    }
}


/*******************************************************************
 *         output_register_funcs
 *
 * Output the functions for register entry points
 */
static void output_register_funcs( FILE *outfile, DLLSPEC *spec )
{
    const char *name;
    int i;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STDCALL && odp->type != TYPE_CDECL) continue;
        if (!(odp->flags & FLAG_REGISTER)) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        name = make_internal_name( odp, spec, "regs" );
        fprintf( outfile,
                 "asm(\".align %d\\n\\t\"\n"
                 "    \"" __ASM_FUNC("%s") "\\n\\t\"\n"
                 "    \"" __ASM_NAME("%s") ":\\n\\t\"\n"
                 "    \"call " __ASM_NAME("__wine_call_from_32_regs") "\\n\\t\"\n"
                 "    \".long " __ASM_NAME("%s") "\\n\\t\"\n"
                 "    \".byte %d,%d\");\n",
                 get_alignment(4),
                 name, name, odp->link_name,
                 strlen(odp->u.func.arg_types) * sizeof(int),
                 (odp->type == TYPE_CDECL) ? 0 : (strlen(odp->u.func.arg_types) * sizeof(int)) );
    }
}


/*******************************************************************
 *         output_dll_init
 *
 * Output code for calling a dll constructor and destructor.
 */
void output_dll_init( FILE *outfile, const char *constructor, const char *destructor )
{
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy_dll_init(void) {\n" );
    fprintf( outfile, "#endif\n" );

#if defined(__i386__)
    if (constructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tcall " __ASM_NAME("%s") "\\n\"\n", constructor );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
    if (destructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tcall " __ASM_NAME("%s") "\\n\"\n", destructor );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
#elif defined(__sparc__)
    if (constructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tcall " __ASM_NAME("%s") "\\n\"\n", constructor );
        fprintf( outfile, "    \"\\tnop\\n\"\n" );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
    if (destructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tcall " __ASM_NAME("%s") "\\n\"\n", destructor );
        fprintf( outfile, "    \"\\tnop\\n\"\n" );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
#elif defined(__powerpc__)
# ifdef __APPLE__
/* Mach-O doesn't have an init section */
    if (constructor)
    {
        fprintf( outfile, "asm(\"\\t.mod_init_func\\n\"\n" );
        fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
        fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") "\\n\"\n", constructor );
        fprintf( outfile, "    \"\\t.text\\n\");\n" );
    }
    if (destructor)
    {
        fprintf( outfile, "asm(\"\\t.mod_term_func\\n\"\n" );
        fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
        fprintf( outfile, "    \"\\t.long " __ASM_NAME("%s") "\\n\"\n", destructor );
        fprintf( outfile, "    \"\\t.text\\n\");\n" );
    }
# else /* __APPLE__ */
    if (constructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tbl " __ASM_NAME("%s") "\\n\"\n", constructor );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
    if (destructor)
    {
        fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tbl " __ASM_NAME("%s") "\\n\"\n", destructor );
        fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
    }
# endif /* __APPLE__ */
#else
#error You need to define the DLL constructor for your architecture
#endif
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif\n" );
}


/*******************************************************************
 *         BuildSpec32File
 *
 * Build a Win32 C file from a spec file.
 */
void BuildSpec32File( FILE *outfile, DLLSPEC *spec )
{
    int exports_size = 0;
    int nr_exports, nr_imports;
    DWORD page_size;
    const char *init_func = spec->init_func;

#ifdef HAVE_GETPAGESIZE
    page_size = getpagesize();
#elif defined(__svr4__)
    page_size = sysconf(_SC_PAGESIZE);
#elif defined(_WINDOWS)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
    }
#else
#   error Cannot get the page size on this platform
#endif

    nr_exports = spec->base <= spec->limit ? spec->limit - spec->base + 1 : 0;
    resolve_imports( spec );
    output_standard_file_header( outfile );

    /* Reserve some space for the PE header */

    fprintf( outfile, "extern char __wine_spec_pe_header[];\n" );
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy_header(void) {\n" );
    fprintf( outfile, "#endif\n" );
    fprintf( outfile, "asm(\".text\\n\\t\"\n" );
    fprintf( outfile, "    \".align %d\\n\"\n", get_alignment(page_size) );
    fprintf( outfile, "    \"" __ASM_NAME("__wine_spec_pe_header") ":\\t" __ASM_SKIP " 65536\\n\\t\"\n" );
    fprintf( outfile, "    \".data\\n\\t\"\n" );
    fprintf( outfile, "    \".align %d\\n\"\n", get_alignment(4) );
    fprintf( outfile, "    \"" __ASM_NAME("__wine_spec_data_start") ":\\t.long 1\");\n" );
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif\n" );

#ifdef __APPLE__
    fprintf( outfile, "static char _end[4];\n" );
#else
    fprintf( outfile, "extern char _end[];\n" );
#endif
    
    fprintf( outfile, "extern int __wine_spec_data_start[], __wine_spec_exports[];\n\n" );

#ifdef __i386__
    fprintf( outfile, "#define __stdcall __attribute__((__stdcall__))\n\n" );
#else
    fprintf( outfile, "#define __stdcall\n\n" );
#endif

    if (nr_exports)
    {
        /* Output the stub functions */

        output_stub_funcs( outfile, spec );

        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "static void __asm__dummy(void) {\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );

        /* Output code for all register functions */

        output_register_funcs( outfile, spec );

        /* Output the exports and relay entry points */

        exports_size = output_exports( outfile, nr_exports, spec );

        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "}\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
    }

    /* Output the DLL imports */

    nr_imports = output_imports( outfile, spec );

    /* Output the resources */

    output_resources( outfile, spec );

    /* Output the entry point function */

    fprintf( outfile, "static int __wine_spec_init_state;\n" );
    fprintf( outfile, "extern int __wine_main_argc;\n" );
    fprintf( outfile, "extern char **__wine_main_argv;\n" );
    fprintf( outfile, "extern char **__wine_main_environ;\n" );
    fprintf( outfile, "extern unsigned short **__wine_main_wargv;\n" );
#ifdef __APPLE__
    fprintf( outfile, "extern _dyld_func_lookup(char *, void *);" );
    fprintf( outfile, "static void __wine_spec_hidden_init(int argc, char** argv, char** envp)\n" );
    fprintf( outfile, "{\n" );
    fprintf( outfile, "    void (*init)(void);\n" );
    fprintf( outfile, "    _dyld_func_lookup(\"__dyld_make_delayed_module_initializer_calls\", (unsigned long *)&init);\n" );
    fprintf( outfile, "    init();\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "static void __wine_spec_hidden_fini()\n" );
    fprintf( outfile, "{\n" );
    fprintf( outfile, "    void (*fini)(void);\n" );
    fprintf( outfile, "    _dyld_func_lookup(\"__dyld_mod_term_funcs\", (unsigned long *)&fini);\n" );
    fprintf( outfile, "    fini();\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#define _init __wine_spec_hidden_init\n" );
    fprintf( outfile, "#define _fini __wine_spec_hidden_fini\n" );
#else
    fprintf( outfile, "extern void _init(int, char**, char**);\n" );
    fprintf( outfile, "extern void _fini();\n" );
#endif

    if (spec->characteristics & IMAGE_FILE_DLL)
    {
        if (init_func)
            fprintf( outfile, "extern int __stdcall %s( void*, unsigned int, void* );\n\n", init_func );
        else
        {
            declare_weak_function( outfile, "int __stdcall", "DllMain", "void*, unsigned int, void*" );
            init_func = "DllMain";
        }
        fprintf( outfile,
                 "static int __stdcall __wine_dll_main( void *inst, unsigned int reason, void *reserved )\n"
                 "{\n"
                 "    int ret;\n"
                 "    if (reason == %d && __wine_spec_init_state == 1)\n"
                 "        _init( __wine_main_argc, __wine_main_argv, __wine_main_environ );\n"
                 "    ret = %s ? %s( inst, reason, reserved ) : 1;\n"
                 "    if (reason == %d && __wine_spec_init_state == 1) _fini();\n"
                 "    return ret;\n"
                 "}\n",
                 DLL_PROCESS_ATTACH, init_func, init_func, DLL_PROCESS_DETACH );
        init_func = "__wine_dll_main";
    }
    else switch(spec->subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE:
        if (init_func)
            fprintf( outfile, "extern int __stdcall %s( void*, void* );\n\n", init_func );
        else
        {
            declare_weak_function( outfile, "int __stdcall", "DriverEntry", "void*, void*");
            init_func = "DriverEntry";
        }
        fprintf( outfile,
                 "static int __stdcall __wine_driver_entry( void *obj, void *path )\n"
                 "{\n"
                 "    int ret;\n"
                 "    if (__wine_spec_init_state == 1)\n"
                 "        _init( __wine_main_argc, __wine_main_argv, __wine_main_environ );\n"
                 "    ret = %s ? %s( obj, path ) : 0;\n"
                 "    if (__wine_spec_init_state == 1) _fini();\n"
                 "    return ret;\n"
                 "}\n",
                 init_func, init_func );
        init_func = "__wine_driver_entry";
        break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        if (init_func)
            fprintf( outfile, "extern int %s( int argc, char *argv[] );\n", init_func );
        else
        {
            declare_weak_function( outfile, "int", "main", "int argc, char *argv[]" );
            declare_weak_function( outfile, "int", "wmain", "int argc, unsigned short *argv[]" );
            declare_weak_function( outfile, "int __stdcall", "WinMain", "void *,void *,char *,int" );
        }
        fprintf( outfile,
                 "\ntypedef struct {\n"
                 "    unsigned int cb;\n"
                 "    char *lpReserved, *lpDesktop, *lpTitle;\n"
                 "    unsigned int dwX, dwY, dwXSize, dwYSize;\n"
                 "    unsigned int dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;\n"
                 "    unsigned short wShowWindow, cbReserved2;\n"
                 "    char *lpReserved2;\n"
                 "    void *hStdInput, *hStdOutput, *hStdError;\n"
                 "} STARTUPINFOA;\n"
                 "extern char * __stdcall GetCommandLineA(void);\n"
                 "extern void * __stdcall GetModuleHandleA(char *);\n"
                 "extern void __stdcall GetStartupInfoA(STARTUPINFOA *);\n"
                 "extern void __stdcall ExitProcess(unsigned int);\n"
                 "static void __wine_exe_main(void)\n"
                 "{\n"
                 "    int ret;\n"
                 "    if (__wine_spec_init_state == 1)\n"
                 "        _init( __wine_main_argc, __wine_main_argv, __wine_main_environ );\n" );
        if (init_func)
            fprintf( outfile,
                     "    ret = %s( __wine_main_argc, __wine_main_argv );\n", init_func );
        else
            fprintf( outfile,
                     "    if (WinMain) {\n"
                     "        STARTUPINFOA info;\n"
                     "        char *cmdline = GetCommandLineA();\n"
                     "        int bcount=0, in_quotes=0;\n"
                     "        while (*cmdline) {\n"
                     "            if ((*cmdline=='\\t' || *cmdline==' ') && !in_quotes) break;\n"
                     "            else if (*cmdline=='\\\\') bcount++;\n"
                     "            else if (*cmdline=='\\\"') {\n"
                     "                if ((bcount & 1)==0) in_quotes=!in_quotes;\n"
                     "                bcount=0;\n"
                     "            }\n"
                     "            else bcount=0;\n"
                     "            cmdline++;\n"
                     "        }\n"
                     "        while (*cmdline=='\\t' || *cmdline==' ') cmdline++;\n"
                     "        GetStartupInfoA( &info );\n"
                     "        if (!(info.dwFlags & 1)) info.wShowWindow = 1;\n"
                     "        ret = WinMain( GetModuleHandleA(0), 0, cmdline, info.wShowWindow );\n"
                     "    }\n"
                     "    else if (wmain) ret = wmain( __wine_main_argc, __wine_main_wargv );\n"
                     "    else ret = main( __wine_main_argc, __wine_main_argv );\n" );
        fprintf( outfile,
                 "    if (__wine_spec_init_state == 1) _fini();\n"
                 "    ExitProcess( ret );\n"
                 "}\n\n" );
        init_func = "__wine_exe_main";
        break;
    }

    /* Output the NT header */

    /* this is the IMAGE_NT_HEADERS structure, but we cannot include winnt.h here */
    fprintf( outfile, "static const struct image_nt_headers\n{\n" );
    fprintf( outfile, "  int Signature;\n" );
    fprintf( outfile, "  struct file_header {\n" );
    fprintf( outfile, "    short Machine;\n" );
    fprintf( outfile, "    short NumberOfSections;\n" );
    fprintf( outfile, "    int   TimeDateStamp;\n" );
    fprintf( outfile, "    void *PointerToSymbolTable;\n" );
    fprintf( outfile, "    int   NumberOfSymbols;\n" );
    fprintf( outfile, "    short SizeOfOptionalHeader;\n" );
    fprintf( outfile, "    short Characteristics;\n" );
    fprintf( outfile, "  } FileHeader;\n" );
    fprintf( outfile, "  struct opt_header {\n" );
    fprintf( outfile, "    short Magic;\n" );
    fprintf( outfile, "    char  MajorLinkerVersion, MinorLinkerVersion;\n" );
    fprintf( outfile, "    int   SizeOfCode;\n" );
    fprintf( outfile, "    int   SizeOfInitializedData;\n" );
    fprintf( outfile, "    int   SizeOfUninitializedData;\n" );
    fprintf( outfile, "    void *AddressOfEntryPoint;\n" );
    fprintf( outfile, "    void *BaseOfCode;\n" );
    fprintf( outfile, "    void *BaseOfData;\n" );
    fprintf( outfile, "    void *ImageBase;\n" );
    fprintf( outfile, "    int   SectionAlignment;\n" );
    fprintf( outfile, "    int   FileAlignment;\n" );
    fprintf( outfile, "    short MajorOperatingSystemVersion;\n" );
    fprintf( outfile, "    short MinorOperatingSystemVersion;\n" );
    fprintf( outfile, "    short MajorImageVersion;\n" );
    fprintf( outfile, "    short MinorImageVersion;\n" );
    fprintf( outfile, "    short MajorSubsystemVersion;\n" );
    fprintf( outfile, "    short MinorSubsystemVersion;\n" );
    fprintf( outfile, "    int   Win32VersionValue;\n" );
    fprintf( outfile, "    void *SizeOfImage;\n" );
    fprintf( outfile, "    int   SizeOfHeaders;\n" );
    fprintf( outfile, "    int   CheckSum;\n" );
    fprintf( outfile, "    short Subsystem;\n" );
    fprintf( outfile, "    short DllCharacteristics;\n" );
    fprintf( outfile, "    int   SizeOfStackReserve;\n" );
    fprintf( outfile, "    int   SizeOfStackCommit;\n" );
    fprintf( outfile, "    int   SizeOfHeapReserve;\n" );
    fprintf( outfile, "    int   SizeOfHeapCommit;\n" );
    fprintf( outfile, "    int   LoaderFlags;\n" );
    fprintf( outfile, "    int   NumberOfRvaAndSizes;\n" );
    fprintf( outfile, "    struct { const void *VirtualAddress; int Size; } DataDirectory[%d];\n",
             IMAGE_NUMBEROF_DIRECTORY_ENTRIES );
    fprintf( outfile, "  } OptionalHeader;\n" );
    fprintf( outfile, "} nt_header = {\n" );
    fprintf( outfile, "  0x%04x,\n", IMAGE_NT_SIGNATURE );   /* Signature */
#ifdef __i386__
    fprintf( outfile, "  { 0x%04x,\n", IMAGE_FILE_MACHINE_I386 );  /* Machine */
#elif defined(__powerpc__)
    fprintf( outfile, "  { 0x%04x,\n", IMAGE_FILE_MACHINE_POWERPC ); /* Machine */
#else
    fprintf( outfile, "  { 0x%04x,\n", IMAGE_FILE_MACHINE_UNKNOWN );  /* Machine */
#endif
    fprintf( outfile, "    0, 0, 0, 0,\n" );
    fprintf( outfile, "    sizeof(nt_header.OptionalHeader),\n" ); /* SizeOfOptionalHeader */
    fprintf( outfile, "    0x%04x },\n", spec->characteristics );  /* Characteristics */

    fprintf( outfile, "  { 0x%04x,\n", IMAGE_NT_OPTIONAL_HDR_MAGIC );  /* Magic */
    fprintf( outfile, "    0, 0,\n" );                   /* Major/MinorLinkerVersion */
    fprintf( outfile, "    0, 0, 0,\n" );                /* SizeOfCode/Data */
    fprintf( outfile, "    %s,\n", init_func );          /* AddressOfEntryPoint */
    fprintf( outfile, "    0, __wine_spec_data_start,\n" );              /* BaseOfCode/Data */
    fprintf( outfile, "    __wine_spec_pe_header,\n" );  /* ImageBase */
    fprintf( outfile, "    %ld,\n", page_size );         /* SectionAlignment */
    fprintf( outfile, "    %ld,\n", page_size );         /* FileAlignment */
    fprintf( outfile, "    1, 0,\n" );                   /* Major/MinorOperatingSystemVersion */
    fprintf( outfile, "    0, 0,\n" );                   /* Major/MinorImageVersion */
    fprintf( outfile, "    %d,\n", spec->subsystem_major );             /* MajorSubsystemVersion */
    fprintf( outfile, "    %d,\n", spec->subsystem_minor );             /* MinorSubsystemVersion */
    fprintf( outfile, "    0,\n" );                      /* Win32VersionValue */
    fprintf( outfile, "    _end,\n" );                   /* SizeOfImage */
    fprintf( outfile, "    %ld,\n", page_size );         /* SizeOfHeaders */
    fprintf( outfile, "    0,\n" );                      /* CheckSum */
    fprintf( outfile, "    0x%04x,\n", spec->subsystem );/* Subsystem */
    fprintf( outfile, "    0,\n" );                      /* DllCharacteristics */
    fprintf( outfile, "    %d, %ld,\n",                  /* SizeOfStackReserve/Commit */
             (spec->stack_size ? spec->stack_size : 1024) * 1024, page_size );
    fprintf( outfile, "    %d, %ld,\n",                  /* SizeOfHeapReserve/Commit */
             (spec->heap_size ? spec->heap_size : 1024) * 1024, page_size );
    fprintf( outfile, "    0,\n" );                      /* LoaderFlags */
    fprintf( outfile, "    %d,\n", IMAGE_NUMBEROF_DIRECTORY_ENTRIES );  /* NumberOfRvaAndSizes */
    fprintf( outfile, "    {\n" );
    fprintf( outfile, "      { %s, %d },\n",  /* IMAGE_DIRECTORY_ENTRY_EXPORT */
             exports_size ? "__wine_spec_exports" : "0", exports_size );
    fprintf( outfile, "      { %s, %s },\n",  /* IMAGE_DIRECTORY_ENTRY_IMPORT */
             nr_imports ? "&imports" : "0", nr_imports ? "sizeof(imports)" : "0" );
    fprintf( outfile, "      { %s, %s },\n",   /* IMAGE_DIRECTORY_ENTRY_RESOURCE */
             spec->nb_resources ? "&resources" : "0",
             spec->nb_resources ? "sizeof(resources)" : "0" );
    fprintf( outfile, "    }\n  }\n};\n\n" );

    /* Output the DLL constructor */

    fprintf( outfile,
             "void __wine_spec_init(void)\n"
             "{\n"
             "    extern void __wine_dll_register( const struct image_nt_headers *, const char * );\n"
             "    __wine_spec_init_state = 1;\n"
             "    __wine_dll_register( &nt_header, \"%s\" );\n"
             "}\n\n",
             spec->file_name );

    output_dll_init( outfile, "__wine_spec_init_ctor", NULL );
    fprintf( outfile,
             "void __wine_spec_init_ctor(void)\n"
             "{\n"
             "    if (__wine_spec_init_state) return;\n"
             "    __wine_spec_init();\n"
             "    __wine_spec_init_state = 2;\n"
             "}\n" );
}


/*******************************************************************
 *         BuildDef32File
 *
 * Build a Win32 def file from a spec file.
 */
void BuildDef32File( FILE *outfile, DLLSPEC *spec )
{
    const char *name;
    int i;

    fprintf(outfile, "; File generated automatically from %s; do not edit!\n\n",
            input_file_name );

    fprintf(outfile, "LIBRARY %s\n\n", spec->file_name);

    fprintf(outfile, "EXPORTS\n");

    /* Output the exports and relay entry points */

    for(i = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        int is_data = 0;

        if (!odp) continue;
        if (odp->name) name = odp->name;
        else if (odp->type == TYPE_STUB) name = make_internal_name( odp, spec, "stub" );
        else if (odp->export_name) name = odp->export_name;
        else name = make_internal_name( odp, spec, "noname_export" );

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
            int at_param = strlen(odp->u.func.arg_types) * sizeof(int);
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
}


/*******************************************************************
 *         BuildDebugFile
 *
 * Build the debugging channels source file.
 */
void BuildDebugFile( FILE *outfile, const char *srcdir, char **argv )
{
    int nr_debug;
    char *prefix, *p;

    while (*argv)
    {
        if (!parse_debug_channels( srcdir, *argv++ )) exit(1);
    }

    output_standard_file_header( outfile );
    nr_debug = output_debug( outfile );
    if (!nr_debug)
    {
        fprintf( outfile, "/* no debug channels found for this module */\n" );
        return;
    }

    if (output_file_name)
    {
        if ((p = strrchr( output_file_name, '/' ))) p++;
        prefix = xstrdup( p ? p : output_file_name );
        if ((p = strchr( prefix, '.' ))) *p = 0;
        strcpy( p, make_c_identifier(p) );
    }
    else prefix = xstrdup( "_" );

    /* Output the DLL constructor */

    fprintf( outfile,
             "#ifdef __GNUC__\n"
             "void __wine_dbg_%s_init(void) __attribute__((constructor));\n"
             "void __wine_dbg_%s_fini(void) __attribute__((destructor));\n"
             "#else\n"
             "static void __asm__dummy_dll_init(void) {\n",
             prefix, prefix );

#if defined(__i386__)
    fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall " __ASM_NAME("__wine_dbg_%s_init") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall " __ASM_NAME("__wine_dbg_%s_fini") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
#elif defined(__sparc__)
    fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall " __ASM_NAME("__wine_dbg_%s_init") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\tnop\\n\"\n" );
    fprintf( outfile, "    \"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall " __ASM_NAME("__wine_dbg_%s_fini") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\tnop\\n\"\n" );
    fprintf( outfile, "    \"\\t.section\t\\\".text\\\"\\n\");\n" );
#elif defined(__powerpc__)
# ifdef __APPLE__
	fprintf( outfile, "asm(\"\\t.mod_init_func\\n\"\n" );
	fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
	fprintf( outfile, "    \"\\t.long " __ASM_NAME("__wine_dbg_%s_init") "\\n\"\n", prefix );
	fprintf( outfile, "    \"\\t.text\\n\");\n" );
	fprintf( outfile, "asm(\"\\t.mod_term_func\\n\"\n" );
	fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
	fprintf( outfile, "    \"\\t.long " __ASM_NAME("__wine_dbg_%s_fini") "\\n\"\n", prefix );
	fprintf( outfile, "    \"\\t.text\\n\");\n" );
# else
    fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tbl " __ASM_NAME("__wine_dbg_%s_init") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tbl " __ASM_NAME("__wine_dbg_%s_fini") "\\n\"\n", prefix );
    fprintf( outfile, "    \"\\t.text\\n\");\n" );
# endif
#else
#error You need to define the DLL constructor for your architecture
#endif
    fprintf( outfile, "}\n#endif /* defined(__GNUC__) */\n\n" );

    fprintf( outfile,
             "void __wine_dbg_%s_init(void)\n"
             "{\n"
             "    extern void *__wine_dbg_register( char * const *, int );\n"
             "    if (!debug_registration) debug_registration = __wine_dbg_register( debug_channels, %d );\n"
             "}\n\n", prefix, nr_debug );
    fprintf( outfile,
             "void __wine_dbg_%s_fini(void)\n"
             "{\n"
             "    extern void __wine_dbg_unregister( void* );\n"
             "    __wine_dbg_unregister( debug_registration );\n"
             "}\n", prefix );

    free( prefix );
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

    if (nr_exports)
    {
        /* Output the stub functions */

        output_stub_funcs( outfile, spec );
    }
}
