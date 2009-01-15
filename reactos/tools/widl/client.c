/*
 * IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
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
 
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#include "widltypes.h"
#include "typegen.h"
#include "expr.h"

static FILE* client;
static int indent = 0;

static void print_client( const char *format, ... )
{
    va_list va;
    va_start(va, format);
    print(client, indent, format, va);
    va_end(va);
}


static void check_pointers(const func_t *func)
{
    const var_t *var;

    if (!func->args)
        return;

    LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
    {
        if (is_var_ptr(var) && cant_be_null(var))
        {
            print_client("if (!%s)\n", var->name);
            print_client("{\n");
            indent++;
            print_client("RpcRaiseException(RPC_X_NULL_REF_POINTER);\n");
            indent--;
            print_client("}\n\n");
        }
    }
}

static void write_function_stubs(type_t *iface, unsigned int *proc_offset)
{
    const func_t *func;
    const char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
    const var_t *var;
    int method_count = 0;

    if (!implicit_handle)
        print_client("static RPC_BINDING_HANDLE %s__MIDL_AutoBindHandle;\n\n", iface->name);

    if (iface->funcs) LIST_FOR_EACH_ENTRY( func, iface->funcs, const func_t, entry )
    {
        const var_t *def = func->def;
        const var_t* explicit_handle_var;
        const var_t* explicit_generic_handle_var = NULL;
        const var_t* context_handle_var = NULL;
        int has_full_pointer = is_full_pointer_function(func);
        const char *callconv = get_attrp(def->type->attrs, ATTR_CALLCONV);

        /* check for a defined binding handle */
        explicit_handle_var = get_explicit_handle_var(func);
        if (!explicit_handle_var)
        {
            explicit_generic_handle_var = get_explicit_generic_handle_var(func);
            if (!explicit_generic_handle_var)
                context_handle_var = get_context_handle_var(func);
        }

        print_client( "struct __frame_%s%s\n{\n", prefix_client, get_name(def) );
        indent++;
        print_client( "__DECL_EXCEPTION_FRAME\n" );
        print_client("MIDL_STUB_MESSAGE _StubMsg;\n");
        if (implicit_handle || explicit_handle_var || explicit_generic_handle_var || context_handle_var)
        {
            if (!implicit_handle && explicit_generic_handle_var)
                print_client("%s %s;\n",
                             get_explicit_generic_handle_type(explicit_generic_handle_var)->name,
                             explicit_generic_handle_var->name );
            print_client("RPC_BINDING_HANDLE _Handle;\n");
        }

        if (!is_void(get_func_return_type(func)) && decl_indirect(get_func_return_type(func)))
        {
            print_client("void *_p_%s;\n", "_RetVal" );
        }
        indent--;
        print_client( "};\n\n" );

        print_client( "static void __finally_%s%s(", prefix_client, get_name(def) );
        print_client( " struct __frame_%s%s *__frame )\n{\n", prefix_client, get_name(def) );
        indent++;

        /* FIXME: emit client finally code */

        if (has_full_pointer)
            write_full_pointer_free(client, indent, func);

        print_client("NdrFreeBuffer(&__frame->_StubMsg);\n");

        if (!implicit_handle && explicit_generic_handle_var)
        {
            fprintf(client, "\n");
            print_client("if (__frame->_Handle)\n");
            indent++;
            print_client("%s_unbind(__frame->%s, __frame->_Handle);\n",
                get_explicit_generic_handle_type(explicit_generic_handle_var)->name,
                explicit_generic_handle_var->name);
            indent--;
        }
        indent--;
        print_client( "}\n\n" );

        write_type_decl_left(client, get_func_return_type(func));
        if (needs_space_after(get_func_return_type(func)))
          fprintf(client, " ");
        if (callconv) fprintf(client, "%s ", callconv);
        fprintf(client, "%s%s(\n", prefix_client, get_name(def));
        indent++;
        if (func->args)
            write_args(client, func->args, iface->name, 0, TRUE);
        else
            print_client("void");
        fprintf(client, ")\n");
        indent--;

        /* write the functions body */
        fprintf(client, "{\n");
        indent++;
        print_client( "struct __frame_%s%s __f, * const __frame = &__f;\n", prefix_client, get_name(def) );

        /* declare return value '_RetVal' */
        if (!is_void(get_func_return_type(func)))
        {
            print_client("");
            write_type_decl_left(client, get_func_return_type(func));
            fprintf(client, " _RetVal;\n");
        }
        print_client("RPC_MESSAGE _RpcMessage;\n");

        if (implicit_handle || explicit_handle_var || explicit_generic_handle_var || context_handle_var)
        {
            print_client( "__frame->_Handle = 0;\n" );
            if (!implicit_handle && explicit_generic_handle_var)
                print_client("__frame->%s = %s;\n",
                             explicit_generic_handle_var->name, explicit_generic_handle_var->name );
        }
        if (!is_void(get_func_return_type(func)) && decl_indirect(get_func_return_type(func)))
        {
            print_client("__frame->_p_%s = &%s;\n",
                         "_RetVal", "_RetVal");
        }
        fprintf(client, "\n");

        print_client( "RpcExceptionInit( 0, __finally_%s%s );\n", prefix_client, get_name(def) );

        if (has_full_pointer)
            write_full_pointer_init(client, indent, func, FALSE);

        /* check pointers */
        check_pointers(func);

        print_client("RpcTryFinally\n");
        print_client("{\n");
        indent++;

        print_client("NdrClientInitializeNew(&_RpcMessage, &__frame->_StubMsg, &%s_StubDesc, %d);\n",
                     iface->name, method_count);

        if (is_attr(def->attrs, ATTR_IDEMPOTENT) || is_attr(def->attrs, ATTR_BROADCAST))
        {
            print_client("_RpcMessage.RpcFlags = ( RPC_NCA_FLAGS_DEFAULT ");
            if (is_attr(def->attrs, ATTR_IDEMPOTENT))
                fprintf(client, "| RPC_NCA_FLAGS_IDEMPOTENT ");
            if (is_attr(def->attrs, ATTR_BROADCAST))
                fprintf(client, "| RPC_NCA_FLAGS_BROADCAST ");
            fprintf(client, ");\n\n");
        }

        if (explicit_handle_var)
        {
            print_client("__frame->_Handle = %s;\n", explicit_handle_var->name);
            fprintf(client, "\n");
        }
        else if (explicit_generic_handle_var)
        {
            print_client("__frame->_Handle = %s_bind(%s);\n",
                get_explicit_generic_handle_type(explicit_generic_handle_var)->name,
                explicit_generic_handle_var->name);
            fprintf(client, "\n");
        }
        else if (context_handle_var)
        {
            /* if the context_handle attribute appears in the chain of types
             * without pointers being followed, then the context handle must
             * be direct, otherwise it is a pointer */
            int is_ch_ptr = is_aliaschain_attr(context_handle_var->type, ATTR_CONTEXTHANDLE) ? FALSE : TRUE;
            print_client("if (%s%s != 0)\n", is_ch_ptr ? "*" : "", context_handle_var->name);
            indent++;
            print_client("__frame->_Handle = NDRCContextBinding(%s%s);\n",
                         is_ch_ptr ? "*" : "", context_handle_var->name);
            indent--;
            if (is_attr(context_handle_var->attrs, ATTR_IN) &&
                !is_attr(context_handle_var->attrs, ATTR_OUT))
            {
                print_client("else\n");
                indent++;
                print_client("RpcRaiseException(RPC_X_SS_IN_NULL_CONTEXT);\n");
                indent--;
            }
            fprintf(client, "\n");
        }
        else if (implicit_handle)
        {
            print_client("__frame->_Handle = %s;\n", implicit_handle);
            fprintf(client, "\n");
        }

        write_remoting_arguments(client, indent, func, "", PASS_IN, PHASE_BUFFERSIZE);

        print_client("NdrGetBuffer(&__frame->_StubMsg, __frame->_StubMsg.BufferLength, ");
        if (implicit_handle || explicit_handle_var || explicit_generic_handle_var || context_handle_var)
            fprintf(client, "__frame->_Handle);\n\n");
        else
            fprintf(client,"%s__MIDL_AutoBindHandle);\n\n", iface->name);

        /* marshal arguments */
        write_remoting_arguments(client, indent, func, "", PASS_IN, PHASE_MARSHAL);

        /* send/receive message */
        /* print_client("NdrNsSendReceive(\n"); */
        /* print_client("(unsigned char *)__frame->_StubMsg.Buffer,\n"); */
        /* print_client("(RPC_BINDING_HANDLE *) &%s__MIDL_AutoBindHandle);\n", iface->name); */
        print_client("NdrSendReceive(&__frame->_StubMsg, __frame->_StubMsg.Buffer);\n\n");

        print_client("__frame->_StubMsg.BufferStart = _RpcMessage.Buffer;\n");
        print_client("__frame->_StubMsg.BufferEnd = __frame->_StubMsg.BufferStart + _RpcMessage.BufferLength;\n");

        if (has_out_arg_or_return(func))
        {
            fprintf(client, "\n");

            print_client("if ((_RpcMessage.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_client("NdrConvert(&__frame->_StubMsg, (PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n",
                         *proc_offset);
            indent--;
        }

        /* unmarshall arguments */
        fprintf(client, "\n");
        write_remoting_arguments(client, indent, func, "", PASS_OUT, PHASE_UNMARSHAL);

        /* unmarshal return value */
        if (!is_void(get_func_return_type(func)))
        {
            if (decl_indirect(get_func_return_type(func)))
                print_client("MIDL_memset(&%s, 0, sizeof(%s));\n", "_RetVal", "_RetVal");
            else if (is_ptr(get_func_return_type(func)) || is_array(get_func_return_type(func)))
                print_client("%s = 0;\n", "_RetVal");
            write_remoting_arguments(client, indent, func, "", PASS_RETURN, PHASE_UNMARSHAL);
        }

        /* update proc_offset */
        if (func->args)
        {
            LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
                *proc_offset += get_size_procformatstring_type(var->name, var->type, var->attrs);
        }
        if (!is_void(get_func_return_type(func)))
            *proc_offset += get_size_procformatstring_type("return value", get_func_return_type(func), NULL);
        else
            *proc_offset += 2; /* FC_END and FC_PAD */

        indent--;
        print_client("}\n");
        print_client("RpcFinally\n");
        print_client("{\n");
        indent++;
        print_client( "__finally_%s%s( __frame );\n", prefix_client, get_name(def) );
        indent--;
        print_client("}\n");
        print_client("RpcEndFinally\n");


        /* emit return code */
        if (!is_void(get_func_return_type(func)))
        {
            fprintf(client, "\n");
            print_client("return _RetVal;\n");
        }

        indent--;
        fprintf(client, "}\n");
        fprintf(client, "\n");

        method_count++;
    }
}


static void write_stubdescdecl(type_t *iface)
{
    print_client("static const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(client, "\n");
}


static void write_stubdescriptor(type_t *iface, int expr_eval_routines)
{
    const char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    print_client("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_client("{\n");
    indent++;
    print_client("(void *)& %s___RpcClientInterface,\n", iface->name);
    print_client("MIDL_user_allocate,\n");
    print_client("MIDL_user_free,\n");
    print_client("{\n");
    indent++;
    if (implicit_handle)
        print_client("&%s,\n", implicit_handle);
    else
        print_client("&%s__MIDL_AutoBindHandle,\n", iface->name);
    indent--;
    print_client("},\n");
    print_client("0,\n");
    print_client("0,\n");
    if (expr_eval_routines)
        print_client("ExprEvalRoutines,\n");
    else
        print_client("0,\n");
    print_client("0,\n");
    print_client("__MIDL_TypeFormatString.Format,\n");
    print_client("1, /* -error bounds_check flag */\n");
    print_client("0x10001, /* Ndr library version */\n");
    print_client("0,\n");
    print_client("0x50100a4, /* MIDL Version 5.1.164 */\n");
    print_client("0,\n");
    print_client("%s,\n", list_empty(&user_type_list) ? "0" : "UserMarshalRoutines");
    print_client("0,  /* notify & notify_flag routine table */\n");
    print_client("1,  /* Flags */\n");
    print_client("0,  /* Reserved3 */\n");
    print_client("0,  /* Reserved4 */\n");
    print_client("0   /* Reserved5 */\n");
    indent--;
    print_client("};\n");
    fprintf(client, "\n");
}


static void write_clientinterfacedecl(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const str_list_t *endpoints = get_attrp(iface->attrs, ATTR_ENDPOINT);

    if (endpoints) write_endpoints( client, iface->name, endpoints );

    print_client("static const RPC_CLIENT_INTERFACE %s___RpcClientInterface =\n", iface->name );
    print_client("{\n");
    indent++;
    print_client("sizeof(RPC_CLIENT_INTERFACE),\n");
    print_client("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], MAJORVERSION(ver), MINORVERSION(ver));
    print_client("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_client("0,\n");
    if (endpoints)
    {
        print_client("%u,\n", list_count(endpoints));
        print_client("(PRPC_PROTSEQ_ENDPOINT)%s__RpcProtseqEndpoint,\n", iface->name);
    }
    else
    {
        print_client("0,\n");
        print_client("0,\n");
    }
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    indent--;
    print_client("};\n");
    if (old_names)
        print_client("RPC_IF_HANDLE %s_ClientIfHandle DECLSPEC_HIDDEN = (RPC_IF_HANDLE)& %s___RpcClientInterface;\n",
                     iface->name, iface->name);
    else
        print_client("RPC_IF_HANDLE %s%s_v%d_%d_c_ifspec DECLSPEC_HIDDEN = (RPC_IF_HANDLE)& %s___RpcClientInterface;\n",
                     prefix_client, iface->name, MAJORVERSION(ver), MINORVERSION(ver), iface->name);
    fprintf(client, "\n");
}


static void write_implicithandledecl(type_t *iface)
{
    const char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    if (implicit_handle)
    {
        fprintf(client, "handle_t %s;\n", implicit_handle);
        fprintf(client, "\n");
    }
}


static void init_client(void)
{
    if (client) return;
    if (!(client = fopen(client_name, "w")))
        error("Could not open %s for output\n", client_name);

    print_client("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
    print_client("#include <string.h>\n");
    print_client("#ifdef _ALPHA_\n");
    print_client("#include <stdarg.h>\n");
    print_client("#endif\n");
    fprintf(client, "\n");
    print_client("#include \"%s\"\n", header_name);
    print_client( "\n");
    print_client( "#ifndef DECLSPEC_HIDDEN\n");
    print_client( "#define DECLSPEC_HIDDEN\n");
    print_client( "#endif\n");
    print_client( "\n");
    write_exceptions( client );
    print_client( "\n");
}


static void write_client_ifaces(const statement_list_t *stmts, int expr_eval_routines, unsigned int *proc_offset)
{
    const statement_t *stmt;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP)
        {
            type_t *iface = stmt->u.type;
            if (!need_stub(iface))
                return;

            fprintf(client, "/*****************************************************************************\n");
            fprintf(client, " * %s interface\n", iface->name);
            fprintf(client, " */\n");
            fprintf(client, "\n");

            if (iface->funcs)
            {
                write_implicithandledecl(iface);

                write_clientinterfacedecl(iface);
                write_stubdescdecl(iface);
                write_function_stubs(iface, proc_offset);

                print_client("#if !defined(__RPC_WIN%u__)\n", pointer_size == 8 ? 64 : 32);
                print_client("#error  Invalid build platform for this stub.\n");
                print_client("#endif\n");

                fprintf(client, "\n");
                write_stubdescriptor(iface, expr_eval_routines);
            }
        }
        else if (stmt->type == STMT_LIBRARY)
            write_client_ifaces(stmt->u.lib->stmts, expr_eval_routines, proc_offset);
    }
}

static void write_client_routines(const statement_list_t *stmts)
{
    unsigned int proc_offset = 0;
    int expr_eval_routines;

    write_formatstringsdecl(client, indent, stmts, need_stub);
    expr_eval_routines = write_expr_eval_routines(client, client_token);
    if (expr_eval_routines)
        write_expr_eval_routine_list(client, client_token);
    write_user_quad_list(client);

    write_client_ifaces(stmts, expr_eval_routines, &proc_offset);

    fprintf(client, "\n");

    write_procformatstring(client, stmts, need_stub);
    write_typeformatstring(client, stmts, need_stub);
}

void write_client(const statement_list_t *stmts)
{
    if (!do_client)
        return;
    if (do_everything && !need_stub_files(stmts))
        return;

    init_client();
    if (!client)
        return;

    if (do_win32 && do_win64)
    {
        fprintf(client, "\n#ifndef _WIN64\n\n");
        pointer_size = 4;
        write_client_routines( stmts );
        fprintf(client, "\n#else /* _WIN64 */\n\n");
        pointer_size = 8;
        write_client_routines( stmts );
        fprintf(client, "\n#endif /* _WIN64 */\n");
    }
    else if (do_win32)
    {
        pointer_size = 4;
        write_client_routines( stmts );
    }
    else if (do_win64)
    {
        pointer_size = 8;
        write_client_routines( stmts );
    }

    fclose(client);
}
