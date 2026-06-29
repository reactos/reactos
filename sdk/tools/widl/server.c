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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#include "typegen.h"

static FILE* server;
static int indent = 0;


static void print_server(const char *format, ...) __attribute__((format (printf, 1, 2)));
static void print_server(const char *format, ...)
{
    va_list va;
    va_start(va, format);
    print(server, indent, format, va);
    va_end(va);
}

static void write_function_stub(const type_t *iface, const var_t *func, unsigned int proc_offset)
{
    const var_t *var;
    unsigned char explicit_fc, implicit_fc;
    int has_full_pointer = is_full_pointer_function(func);
    const var_t *handle_var = get_func_handle_var( iface, func, &explicit_fc, &implicit_fc );
    type_t *ret_type = type_function_get_rettype(func->declspec.type);

    if (is_interpreted_func( iface, func )) return;

    print_server("struct __frame_%s_%s\n{\n", iface->name, get_name(func));
    indent++;
    print_server("__DECL_EXCEPTION_FRAME\n");
    print_server("MIDL_STUB_MESSAGE _StubMsg;\n");

    /* Declare arguments */
    declare_stub_args(server, indent, func);

    indent--;
    print_server("};\n\n");

    print_server("static void __finally_%s_%s(", iface->name, get_name(func));
    fprintf(server," struct __frame_%s_%s *__frame )\n{\n", iface->name, get_name(func));

    if (interpreted_mode) print_server("NdrCorrelationFree(&__frame->_StubMsg);\n");

    indent++;
    write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_FREE);

    if (!is_void(ret_type))
        write_remoting_arguments(server, indent, func, "__frame->", PASS_RETURN, PHASE_FREE);

    if (has_full_pointer)
        write_full_pointer_free(server, indent, func);

    indent--;
    print_server("}\n\n");

    print_server("void __RPC_STUB %s_%s( PRPC_MESSAGE _pRpcMessage )\n", iface->name, get_name(func));

    /* write the functions body */
    fprintf(server, "{\n");
    indent++;
    print_server("struct __frame_%s_%s __f, * const __frame = &__f;\n", iface->name, get_name(func));
    if (interpreted_mode) print_server("ULONG _NdrCorrCache[256];\n");
    if (has_out_arg_or_return(func)) print_server("RPC_STATUS _Status;\n");
    fprintf(server, "\n");

    print_server("NdrServerInitializeNew(\n");
    indent++;
    print_server("_pRpcMessage,\n");
    print_server("&__frame->_StubMsg,\n");
    print_server("&%s_StubDesc);\n", iface->name);
    indent--;
    fprintf(server, "\n");
    print_server( "RpcExceptionInit( __server_filter, __finally_%s_%s );\n", iface->name, get_name(func));

    write_parameters_init(server, indent, func, "__frame->");

    if (explicit_fc == FC_BIND_PRIMITIVE)
    {
        print_server("__frame->%s = _pRpcMessage->Handle;\n", handle_var->name);
        fprintf(server, "\n");
    }

    print_server("RpcTryFinally\n");
    print_server("{\n");
    indent++;
    print_server("RpcTryExcept\n");
    print_server("{\n");
    indent++;
    if (interpreted_mode)
        print_server("NdrCorrelationInitialize(&__frame->_StubMsg, _NdrCorrCache, sizeof(_NdrCorrCache), 0);\n" );

    if (has_full_pointer)
        write_full_pointer_init(server, indent, func, TRUE);

    if (type_function_get_args(func->declspec.type))
    {
        print_server("if ((_pRpcMessage->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
        indent++;
        print_server("NdrConvert(&__frame->_StubMsg, (PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n",
                     proc_offset);
        indent--;
        fprintf(server, "\n");

        /* unmarshall arguments */
        write_remoting_arguments(server, indent, func, "__frame->", PASS_IN, PHASE_UNMARSHAL);
    }

    print_server("if (__frame->_StubMsg.Buffer > __frame->_StubMsg.BufferEnd)\n");
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
    assign_stub_out_args(server, indent, func, "__frame->");

    /* Call the real server function */
    if (is_context_handle(ret_type))
    {
        print_server("__frame->_RetVal = NDRSContextUnmarshall((char*)0, _pRpcMessage->DataRepresentation);\n");
        print_server("*((");
        write_type_decl(server, type_function_get_ret(func->declspec.type), NULL);
        fprintf(server, "*)NDRSContextValue(__frame->_RetVal)) = ");
    }
    else
        print_server("%s", is_void(ret_type) ? "" : "__frame->_RetVal = ");
    fprintf(server, "%s%s", prefix_server, get_name(func));

    if (type_function_get_args(func->declspec.type))
    {
        int first_arg = 1;

        fprintf(server, "(\n");
        indent++;
        LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        {
            if (first_arg)
                first_arg = 0;
            else
                fprintf(server, ",\n");
            if (is_context_handle(var->declspec.type))
            {
                /* if the context_handle attribute appears in the chain of types
                 * without pointers being followed, then the context handle must
                 * be direct, otherwise it is a pointer */
                const char *ch_ptr = is_aliaschain_attr(var->declspec.type, ATTR_CONTEXTHANDLE) ? "*" : "";
                print_server("(");
                write_type_decl_left(server, &var->declspec);
                fprintf(server, ")%sNDRSContextValue(__frame->%s)", ch_ptr, var->name);
            }
            else
            {
                print_server("%s__frame->%s", is_array(var->declspec.type) && !type_array_is_decl_as_ptr(var->declspec.type) ? "*" : "", var->name);
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
        write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_BUFFERSIZE);

        if (!is_void(ret_type))
            write_remoting_arguments(server, indent, func, "__frame->", PASS_RETURN, PHASE_BUFFERSIZE);

        print_server("_pRpcMessage->BufferLength = __frame->_StubMsg.BufferLength;\n");
        fprintf(server, "\n");
        print_server("_Status = I_RpcGetBuffer(_pRpcMessage);\n");
        print_server("if (_Status)\n");
        indent++;
        print_server("RpcRaiseException(_Status);\n");
        indent--;
        fprintf(server, "\n");
        print_server("__frame->_StubMsg.Buffer = _pRpcMessage->Buffer;\n");
        fprintf(server, "\n");
    }

    /* marshall arguments */
    write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_MARSHAL);

    /* marshall the return value */
    if (!is_void(ret_type))
        write_remoting_arguments(server, indent, func, "__frame->", PASS_RETURN, PHASE_MARSHAL);

    indent--;
    print_server("}\n");
    print_server("RpcFinally\n");
    print_server("{\n");
    indent++;
    print_server("__finally_%s_%s( __frame );\n", iface->name, get_name(func));
    indent--;
    print_server("}\n");
    print_server("RpcEndFinally\n");

    /* calculate buffer length */
    fprintf(server, "\n");
    print_server("_pRpcMessage->BufferLength = __frame->_StubMsg.Buffer - (unsigned char *)_pRpcMessage->Buffer;\n");
    indent--;
    fprintf(server, "}\n");
    fprintf(server, "\n");
}


static void write_function_stubs(type_t *iface, unsigned int *proc_offset)
{
    const statement_t *stmt;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;

        write_function_stub( iface, func, *proc_offset );

        /* update proc_offset */
        func->procstring_offset = *proc_offset;
        *proc_offset += get_size_procformatstring_func( iface, func );
    }
}


static void write_dispatchtable(type_t *iface)
{
    unsigned int ver = get_attrv(iface->attrs, ATTR_VERSION);
    unsigned int method_count = 0;
    const statement_t *stmt;

    print_server("static RPC_DISPATCH_FUNCTION %s_table[] =\n", iface->name);
    print_server("{\n");
    indent++;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        if (is_interpreted_func( iface, func ))
            print_server("NdrServerCall2,\n");
        else
            print_server("%s_%s,\n", iface->name, get_name(func));
        method_count++;
    }
    print_server("0\n");
    indent--;
    print_server("};\n");
    print_server("static RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable =\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    print_server("{\n");
    indent++;
    print_server("%u,\n", method_count);
    print_server("%s_table\n", iface->name);
    indent--;
    print_server("};\n");
    fprintf(server, "\n");
}


static void write_routinetable(type_t *iface)
{
    const statement_t *stmt;

    print_server( "static const SERVER_ROUTINE %s_ServerRoutineTable[] =\n", iface->name );
    print_server( "{\n" );
    indent++;
    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        if (is_local( func->attrs )) continue;
        print_server( "(void *)%s%s,\n", prefix_server, get_name(func));
    }
    indent--;
    print_server( "};\n\n" );
}


static void write_rundown_routines(void)
{
    context_handle_t *ch;
    int count = list_count( &context_handle_list );

    if (!count) return;
    print_server( "static const NDR_RUNDOWN RundownRoutines[] =\n" );
    print_server( "{\n" );
    indent++;
    LIST_FOR_EACH_ENTRY( ch, &context_handle_list, context_handle_t, entry )
    {
        print_server( "%s_rundown", ch->name );
        if (--count) fputc( ',', server );
        fputc( '\n', server );
    }
    indent--;
    print_server( "};\n\n" );
}


static void write_serverinfo(type_t *iface)
{
    print_server( "static const MIDL_SERVER_INFO %s_ServerInfo =\n", iface->name );
    print_server( "{\n" );
    indent++;
    print_server( "&%s_StubDesc,\n", iface->name );
    print_server( "%s_ServerRoutineTable,\n", iface->name );
    print_server( "__MIDL_ProcFormatString.Format,\n" );
    print_server( "%s_FormatStringOffsetTable,\n", iface->name );
    print_server( "0,\n" );
    print_server( "0,\n" );
    print_server( "0,\n" );
    print_server( "0\n" );
    indent--;
    print_server( "};\n\n" );
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
    if (!list_empty( &context_handle_list ))
        print_server("RundownRoutines,\n");
    else
        print_server("0,\n");
    print_server("0,\n");
    if (expr_eval_routines)
        print_server("ExprEvalRoutines,\n");
    else
        print_server("0,\n");
    print_server("0,\n");
    print_server("__MIDL_TypeFormatString.Format,\n");
    print_server("1, /* -error bounds_check flag */\n");
    print_server("0x%x, /* Ndr library version */\n", interpreted_mode ? 0x50002 : 0x10001);
    print_server("0,\n");
    print_server("0x50200ca, /* MIDL Version 5.2.202 */\n");
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
    unsigned int ver = get_attrv(iface->attrs, ATTR_VERSION);
    struct uuid *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const str_list_t *endpoints = get_attrp(iface->attrs, ATTR_ENDPOINT);

    if (endpoints) write_endpoints( server, iface->name, endpoints );

    print_server("static RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable;\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    print_server( "static const MIDL_SERVER_INFO %s_ServerInfo;\n", iface->name );
    fprintf(server, "\n");
    print_server("static const RPC_SERVER_INTERFACE %s___RpcServerInterface =\n", iface->name );
    print_server("{\n");
    indent++;
    print_server("sizeof(RPC_SERVER_INTERFACE),\n");
    print_server("{{0x%08x,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
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
    print_server("&%s_ServerInfo,\n", iface->name);
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
    print_server("#include \"%s\"\n", header_name);
    print_server( "\n");
}


static void write_server_stmts(const statement_list_t *stmts, int expr_eval_routines, unsigned int *proc_offset)
{
    const statement_t *stmt;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
        {
            type_t *iface = stmt->u.type;
            if (!need_stub(iface))
                continue;

            fprintf(server, "/*****************************************************************************\n");
            fprintf(server, " * %s interface\n", iface->name);
            fprintf(server, " */\n");
            fprintf(server, "\n");

            if (statements_has_func(type_iface_get_stmts(iface)))
            {
                write_serverinterfacedecl(iface);
                write_stubdescdecl(iface);

                write_function_stubs(iface, proc_offset);

                print_server("#if !defined(__RPC_WIN%u__)\n", pointer_size == 8 ? 64 : 32);
                print_server("#error  Invalid build platform for this stub.\n");
                print_server("#endif\n");

                fprintf(server, "\n");
                write_procformatstring_offsets( server, iface );
                write_stubdescriptor(iface, expr_eval_routines);
                write_dispatchtable(iface);
                write_routinetable(iface);
                write_serverinfo(iface);
            }
        }
    }
}

static void write_server_routines(const statement_list_t *stmts)
{
    unsigned int proc_offset = 0;
    int expr_eval_routines;

    if (need_inline_stubs_file( stmts ))
    {
        write_exceptions( server );
        print_server("\n");
        print_server("struct __server_frame\n");
        print_server("{\n");
        print_server("    __DECL_EXCEPTION_FRAME\n");
        print_server("    MIDL_STUB_MESSAGE _StubMsg;\n");
        print_server("};\n");
        print_server("\n");
        print_server("static int __server_filter( struct __server_frame *__frame )\n");
        print_server( "{\n");
        print_server( "    return (__frame->code == STATUS_ACCESS_VIOLATION) ||\n");
        print_server( "           (__frame->code == STATUS_DATATYPE_MISALIGNMENT) ||\n");
        print_server( "           (__frame->code == RPC_X_BAD_STUB_DATA) ||\n");
        print_server( "           (__frame->code == RPC_S_INVALID_BOUND);\n");
        print_server( "}\n");
        print_server( "\n");
    }

    write_formatstringsdecl(server, indent, stmts, need_stub);
    expr_eval_routines = write_expr_eval_routines(server, server_token);
    if (expr_eval_routines)
        write_expr_eval_routine_list(server, server_token);
    write_user_quad_list(server);
    write_rundown_routines();

    write_server_stmts(stmts, expr_eval_routines, &proc_offset);

    write_procformatstring(server, stmts, need_stub);
    write_typeformatstring(server, stmts, need_stub);
}

void write_server(const statement_list_t *stmts)
{
    if (!do_server)
        return;
    if (do_everything && !need_stub_files(stmts))
        return;

    init_server();
    if (!server)
        return;

    write_server_routines( stmts );
    fclose(server);
}
