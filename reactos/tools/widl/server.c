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

#include "typegen.h"

static FILE* server;
static int indent = 0;


static void print_server(const char *format, ...)
{
    va_list va;
    va_start(va, format);
    print(server, indent, format, va);
    va_end(va);
}

static void write_function_stubs(type_t *iface, unsigned int *proc_offset)
{
    char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
    int explicit_handle = is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE);
    const func_t *func;
    const var_t *var;
    const var_t* explicit_handle_var;

    if (!iface->funcs) return;
    LIST_FOR_EACH_ENTRY( func, iface->funcs, const func_t, entry )
    {
        const var_t *def = func->def;
        const var_t* context_handle_var = NULL;
        const var_t* explicit_generic_handle_var = NULL;
        int has_full_pointer = is_full_pointer_function(func);

        /* check for a defined binding handle */
        explicit_handle_var = get_explicit_handle_var(func);
        if (!explicit_handle_var)
        {
            explicit_generic_handle_var = get_explicit_generic_handle_var(func);
            if (!explicit_generic_handle_var)
                context_handle_var = get_context_handle_var(func);
        }
        if (explicit_handle)
        {
            if (!explicit_handle_var && !explicit_generic_handle_var && !context_handle_var)
            {
                error("%s() does not define an explicit binding handle!\n", def->name);
                return;
            }
        }
        else if (implicit_handle)
        {
            if (explicit_handle_var)
            {
                error("%s() must not define a binding handle!\n", def->name);
                return;
            }
        }

        fprintf(server, "void __RPC_STUB\n");
        fprintf(server, "%s_", iface->name);
        write_name(server, def);
        fprintf(server, "(\n");
        indent++;
        print_server("PRPC_MESSAGE _pRpcMessage)\n");
        indent--;

        /* write the functions body */
        fprintf(server, "{\n");
        indent++;

        /* Declare arguments */
        declare_stub_args(server, indent, func);

        print_server("MIDL_STUB_MESSAGE _StubMsg;\n");
        print_server("RPC_STATUS _Status;\n");
        fprintf(server, "\n");


        print_server("((void)(_Status));\n");
        print_server("NdrServerInitializeNew(\n");
        indent++;
        print_server("_pRpcMessage,\n");
        print_server("&_StubMsg,\n");
        print_server("&%s_StubDesc);\n", iface->name);
        indent--;
        fprintf(server, "\n");

        write_parameters_init(server, indent, func);

        if (explicit_handle_var)
        {
            print_server("%s = _pRpcMessage->Handle;\n", explicit_handle_var->name);
            fprintf(server, "\n");
        }

        print_server("RpcTryFinally\n");
        print_server("{\n");
        indent++;
        print_server("RpcTryExcept\n");
        print_server("{\n");
        indent++;

        if (has_full_pointer)
            write_full_pointer_init(server, indent, func, TRUE);

        if (func->args)
        {
            print_server("if ((_pRpcMessage->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_server("NdrConvert(\n");
            indent++;
            print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
            print_server("(PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n", *proc_offset);
            indent -= 2;
            fprintf(server, "\n");

            /* unmarshall arguments */
            write_remoting_arguments(server, indent, func, PASS_IN, PHASE_UNMARSHAL);
        }

        print_server("if (_StubMsg.Buffer > _StubMsg.BufferEnd)\n");
        print_server("{\n");
        indent++;
        print_server("RpcRaiseException(RPC_X_BAD_STUB_DATA);\n");
        indent--;
        print_server("}\n");
        indent--;
        print_server("}\n");
        print_server("RpcExcept(RPC_BAD_STUB_DATA_EXCEPTION_FILTER)\n");
        print_server("{\n");
        indent++;
        print_server("RpcRaiseException(RPC_X_BAD_STUB_DATA);\n");
        indent--;
        print_server("}\n");
        print_server("RpcEndExcept\n");
        fprintf(server, "\n");

        /* Assign 'out' arguments */
        assign_stub_out_args(server, indent, func);

        /* Call the real server function */
        if (!is_void(get_func_return_type(func)))
            print_server("_RetVal = ");
        else
            print_server("");
        write_prefix_name(server, prefix_server, def);

        if (func->args)
        {
            int first_arg = 1;

            fprintf(server, "(\n");
            indent++;
            LIST_FOR_EACH_ENTRY( var, func->args, const var_t, entry )
            {
                if (first_arg)
                    first_arg = 0;
                else
                    fprintf(server, ",\n");
                if (is_context_handle(var->type))
                {
                    /* if the context_handle attribute appears in the chain of types
                     * without pointers being followed, then the context handle must
                     * be direct, otherwise it is a pointer */
                    int is_ch_ptr = is_aliaschain_attr(var->type, ATTR_CONTEXTHANDLE) ? FALSE : TRUE;
                    print_server("(");
                    write_type_decl_left(server, var->type);
                    fprintf(server, ")%sNDRSContextValue(%s)", is_ch_ptr ? "" : "*", var->name);
                }
                else
                {
                    print_server("");
                    if (var->type->declarray)
                        fprintf(server, "*");
                    write_name(server, var);
                }
            }
            fprintf(server, ");\n");
            indent--;
        }
        else
        {
            fprintf(server, "();\n");
        }

        if (has_out_arg_or_return(func))
        {
            write_remoting_arguments(server, indent, func, PASS_OUT, PHASE_BUFFERSIZE);

            if (!is_void(get_func_return_type(func)))
                write_remoting_arguments(server, indent, func, PASS_RETURN, PHASE_BUFFERSIZE);

            print_server("_pRpcMessage->BufferLength = _StubMsg.BufferLength;\n");
            fprintf(server, "\n");
            print_server("_Status = I_RpcGetBuffer(_pRpcMessage);\n");
            print_server("if (_Status)\n");
            indent++;
            print_server("RpcRaiseException(_Status);\n");
            indent--;
            fprintf(server, "\n");
            print_server("_StubMsg.Buffer = (unsigned char *)_pRpcMessage->Buffer;\n");
            fprintf(server, "\n");
        }

        /* marshall arguments */
        write_remoting_arguments(server, indent, func, PASS_OUT, PHASE_MARSHAL);

        /* marshall the return value */
        if (!is_void(get_func_return_type(func)))
            write_remoting_arguments(server, indent, func, PASS_RETURN, PHASE_MARSHAL);

        indent--;
        print_server("}\n");
        print_server("RpcFinally\n");
        print_server("{\n");
        indent++;

        write_remoting_arguments(server, indent, func, PASS_OUT, PHASE_FREE);

        if (has_full_pointer)
            write_full_pointer_free(server, indent, func);

        indent--;
        print_server("}\n");
        print_server("RpcEndFinally\n");

        /* calculate buffer length */
        fprintf(server, "\n");
        print_server("_pRpcMessage->BufferLength =\n");
        indent++;
        print_server("(unsigned int)(_StubMsg.Buffer - (unsigned char *)_pRpcMessage->Buffer);\n");
        indent--;
        indent--;
        fprintf(server, "}\n");
        fprintf(server, "\n");

        /* update proc_offset */
        *proc_offset += get_size_procformatstring_func( func );
    }
}


static void write_dispatchtable(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    unsigned long method_count = 0;
    const func_t *func;

    print_server("static RPC_DISPATCH_FUNCTION %s_table[] =\n", iface->name);
    print_server("{\n");
    indent++;

    if (iface->funcs) LIST_FOR_EACH_ENTRY( func, iface->funcs, const func_t, entry )
    {
        var_t *def = func->def;

        print_server("%s_", iface->name);
        write_name(server, def);
        fprintf(server, ",\n");

        method_count++;
    }
    print_server("0\n");
    indent--;
    print_server("};\n");
    print_server("RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable =\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    print_server("{\n");
    indent++;
    print_server("%u,\n", method_count);
    print_server("%s_table\n", iface->name);
    indent--;
    print_server("};\n");
    fprintf(server, "\n");
}


static void write_stubdescdecl(type_t *iface)
{
    print_server("static const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(server, "\n");
}


static void write_stubdescriptor(type_t *iface, int expr_eval_routines)
{
    print_server("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_server("{\n");
    indent++;
    print_server("(void *)& %s___RpcServerInterface,\n", iface->name);
    print_server("MIDL_user_allocate,\n");
    print_server("MIDL_user_free,\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    indent--;
    print_server("},\n");
    print_server("0,\n");
    print_server("0,\n");
    if (expr_eval_routines)
        print_server("ExprEvalRoutines,\n");
    else
        print_server("0,\n");
    print_server("0,\n");
    print_server("__MIDL_TypeFormatString.Format,\n");
    print_server("1, /* -error bounds_check flag */\n");
    print_server("0x10001, /* Ndr library version */\n");
    print_server("0,\n");
    print_server("0x50100a4, /* MIDL Version 5.1.164 */\n");
    print_server("0,\n");
    print_server("%s,\n", list_empty(&user_type_list) ? "0" : "UserMarshalRoutines");
    print_server("0,  /* notify & notify_flag routine table */\n");
    print_server("1,  /* Flags */\n");
    print_server("0,  /* Reserved3 */\n");
    print_server("0,  /* Reserved4 */\n");
    print_server("0   /* Reserved5 */\n");
    indent--;
    print_server("};\n");
    fprintf(server, "\n");
}


static void write_serverinterfacedecl(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const str_list_t *endpoints = get_attrp(iface->attrs, ATTR_ENDPOINT);

    if (endpoints) write_endpoints( server, iface->name, endpoints );

    print_server("extern RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable;\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    fprintf(server, "\n");
    print_server("static const RPC_SERVER_INTERFACE %s___RpcServerInterface =\n", iface->name );
    print_server("{\n");
    indent++;
    print_server("sizeof(RPC_SERVER_INTERFACE),\n");
    print_server("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], MAJORVERSION(ver), MINORVERSION(ver));
    print_server("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_server("&%s_v%d_%d_DispatchTable,\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    if (endpoints)
    {
        print_server("%u,\n", list_count(endpoints));
        print_server("(PRPC_PROTSEQ_ENDPOINT)%s__RpcProtseqEndpoint,\n", iface->name);
    }
    else
    {
        print_server("0,\n");
        print_server("0,\n");
    }
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    indent--;
    print_server("};\n");
    if (old_names)
        print_server("RPC_IF_HANDLE %s_ServerIfHandle = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                     iface->name, iface->name);
    else
        print_server("RPC_IF_HANDLE %s%s_v%d_%d_s_ifspec = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                     prefix_server, iface->name, MAJORVERSION(ver), MINORVERSION(ver), iface->name);
    fprintf(server, "\n");
}


static void init_server(void)
{
    if (server)
        return;
    if (!(server = fopen(server_name, "w")))
        error("Could not open %s for output\n", server_name);

    print_server("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
    print_server("#include <string.h>\n");
    fprintf(server, "\n");
    print_server("#define _SEH_NO_NATIVE_NLG\n");
    print_server("#include \"%s\"\n", header_name);
    fprintf(server, "\n");
}


void write_server(ifref_list_t *ifaces)
{
    unsigned int proc_offset = 0;
    int expr_eval_routines;
    ifref_t *iface;

    if (!do_server)
        return;
    if (do_everything && !need_stub_files(ifaces))
        return;

    init_server();
    if (!server)
        return;

    write_formatstringsdecl(server, indent, ifaces, need_stub);
    expr_eval_routines = write_expr_eval_routines(server, server_token);
    if (expr_eval_routines)
        write_expr_eval_routine_list(server, server_token);
    write_user_quad_list(server);

    if (ifaces) LIST_FOR_EACH_ENTRY( iface, ifaces, ifref_t, entry )
    {
        if (!need_stub(iface->iface))
            continue;

        fprintf(server, "/*****************************************************************************\n");
        fprintf(server, " * %s interface\n", iface->iface->name);
        fprintf(server, " */\n");
        fprintf(server, "\n");

        if (iface->iface->funcs)
        {
            write_serverinterfacedecl(iface->iface);
            write_stubdescdecl(iface->iface);
    
            write_function_stubs(iface->iface, &proc_offset);
    
            print_server("#if !defined(__RPC_WIN32__)\n");
            print_server("#error  Invalid build platform for this stub.\n");
            print_server("#endif\n");

            fprintf(server, "\n");
            write_stubdescriptor(iface->iface, expr_eval_routines);
            write_dispatchtable(iface->iface);
        }
    }

    fprintf(server, "\n");

    write_procformatstring(server, ifaces, need_stub);
    write_typeformatstring(server, ifaces, need_stub);

    fclose(server);
}
