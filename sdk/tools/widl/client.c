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

static void print_client( const char *format, ... ) __attribute__((format (printf, 1, 2)));
static void print_client( const char *format, ... )
{
    va_list va;
    va_start(va, format);
    print(client, indent, format, va);
    va_end(va);
}

static void write_client_func_decl( const type_t *iface, const var_t *func )
{
    const char *callconv = get_attrp(func->type->attrs, ATTR_CALLCONV);
    const var_list_t *args = type_get_function_args(func->type);
    type_t *rettype = type_function_get_rettype(func->type);

    if (!callconv) callconv = "__cdecl";
    write_type_decl_left(client, rettype);
    fprintf(client, " %s ", callconv);
    fprintf(client, "%s%s(\n", prefix_client, get_name(func));
    indent++;
    if (args)
        write_args(client, args, iface->name, 0, TRUE);
    else
        print_client("void");
    fprintf(client, ")\n");
    indent--;
}

static void write_function_stub( const type_t *iface, const var_t *func,
                                 int method_count, unsigned int proc_offset )
{
    unsigned char explicit_fc, implicit_fc;
    int has_full_pointer = is_full_pointer_function(func);
    var_t *retval = type_function_get_retval(func->type);
    const var_t *handle_var = get_func_handle_var( iface, func, &explicit_fc, &implicit_fc );
    int has_ret = !is_void(retval->type);

    if (is_interpreted_func( iface, func ))
    {
        write_client_func_decl( iface, func );
        write_client_call_routine( client, iface, func, iface->name, proc_offset );
        return;
    }

    print_client( "struct __frame_%s%s\n{\n", prefix_client, get_name(func) );
    indent++;
    print_client( "__DECL_EXCEPTION_FRAME\n" );
    print_client("MIDL_STUB_MESSAGE _StubMsg;\n");
    if (handle_var)
    {
        if (explicit_fc == FC_BIND_GENERIC)
            print_client("%s %s;\n",
                         get_explicit_generic_handle_type(handle_var)->name, handle_var->name );
        print_client("RPC_BINDING_HANDLE _Handle;\n");
    }

    if (has_ret && decl_indirect(retval->type))
    {
        print_client("void *_p_%s;\n", retval->name);
    }
    indent--;
    print_client( "};\n\n" );

    print_client( "static void __finally_%s%s(", prefix_client, get_name(func) );
    print_client( " struct __frame_%s%s *__frame )\n{\n", prefix_client, get_name(func) );
    indent++;

    if (has_full_pointer)
        write_full_pointer_free(client, indent, func);

    print_client("NdrFreeBuffer(&__frame->_StubMsg);\n");

    if (explicit_fc == FC_BIND_GENERIC)
    {
        fprintf(client, "\n");
        print_client("if (__frame->_Handle)\n");
        indent++;
        print_client("%s_unbind(__frame->%s, __frame->_Handle);\n",
                     get_explicit_generic_handle_type(handle_var)->name, handle_var->name);
        indent--;
    }
    indent--;
    print_client( "}\n\n" );

    write_client_func_decl( iface, func );

    /* write the functions body */
    fprintf(client, "{\n");
    indent++;
    print_client( "struct __frame_%s%s __f, * const __frame = &__f;\n", prefix_client, get_name(func) );

    /* declare return value */
    if (has_ret)
    {
        print_client("%s", "");
        write_type_decl(client, retval->type, retval->name);
        fprintf(client, ";\n");
    }
    print_client("RPC_MESSAGE _RpcMessage;\n");

    if (handle_var)
    {
        print_client( "__frame->_Handle = 0;\n" );
        if (explicit_fc == FC_BIND_GENERIC)
            print_client("__frame->%s = %s;\n", handle_var->name, handle_var->name );
    }
    if (has_ret && decl_indirect(retval->type))
    {
        print_client("__frame->_p_%s = &%s;\n", retval->name, retval->name);
    }
    fprintf(client, "\n");

    print_client( "RpcExceptionInit( 0, __finally_%s%s );\n", prefix_client, get_name(func) );

    if (has_full_pointer)
        write_full_pointer_init(client, indent, func, FALSE);

    /* check pointers */
    write_pointer_checks( client, indent, func );

    print_client("RpcTryFinally\n");
    print_client("{\n");
    indent++;

    print_client("NdrClientInitializeNew(&_RpcMessage, &__frame->_StubMsg, &%s_StubDesc, %d);\n",
                 iface->name, method_count);

    if (is_attr(func->attrs, ATTR_IDEMPOTENT) || is_attr(func->attrs, ATTR_BROADCAST))
    {
        print_client("_RpcMessage.RpcFlags = ( RPC_NCA_FLAGS_DEFAULT ");
        if (is_attr(func->attrs, ATTR_IDEMPOTENT))
            fprintf(client, "| RPC_NCA_FLAGS_IDEMPOTENT ");
        if (is_attr(func->attrs, ATTR_BROADCAST))
            fprintf(client, "| RPC_NCA_FLAGS_BROADCAST ");
        fprintf(client, ");\n\n");
    }

    switch (explicit_fc)
    {
    case FC_BIND_PRIMITIVE:
        print_client("__frame->_Handle = %s;\n", handle_var->name);
        fprintf(client, "\n");
        break;
    case FC_BIND_GENERIC:
        print_client("__frame->_Handle = %s_bind(%s);\n",
                     get_explicit_generic_handle_type(handle_var)->name, handle_var->name);
        fprintf(client, "\n");
        break;
    case FC_BIND_CONTEXT:
    {
        /* if the context_handle attribute appears in the chain of types
         * without pointers being followed, then the context handle must
         * be direct, otherwise it is a pointer */
        int is_ch_ptr = !is_aliaschain_attr(handle_var->type, ATTR_CONTEXTHANDLE);
        print_client("if (%s%s != 0)\n", is_ch_ptr ? "*" : "", handle_var->name);
        indent++;
        print_client("__frame->_Handle = NDRCContextBinding(%s%s);\n",
                     is_ch_ptr ? "*" : "", handle_var->name);
        indent--;
        if (is_attr(handle_var->attrs, ATTR_IN) && !is_attr(handle_var->attrs, ATTR_OUT))
        {
            print_client("else\n");
            indent++;
            print_client("RpcRaiseException(RPC_X_SS_IN_NULL_CONTEXT);\n");
            indent--;
        }
        fprintf(client, "\n");
        break;
    }
    case 0:  /* implicit handle */
        if (handle_var)
        {
            print_client("__frame->_Handle = %s;\n", handle_var->name);
            fprintf(client, "\n");
        }
        break;
    }

    write_remoting_arguments(client, indent, func, "", PASS_IN, PHASE_BUFFERSIZE);

    print_client("NdrGetBuffer(&__frame->_StubMsg, __frame->_StubMsg.BufferLength, ");
    if (handle_var)
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
                     proc_offset);
        indent--;
    }

    /* unmarshall arguments */
    fprintf(client, "\n");
    write_remoting_arguments(client, indent, func, "", PASS_OUT, PHASE_UNMARSHAL);

    /* unmarshal return value */
    if (has_ret)
    {
        if (decl_indirect(retval->type))
            print_client("MIDL_memset(&%s, 0, sizeof(%s));\n", retval->name, retval->name);
        else if (is_ptr(retval->type) || is_array(retval->type))
            print_client("%s = 0;\n", retval->name);
        write_remoting_arguments(client, indent, func, "", PASS_RETURN, PHASE_UNMARSHAL);
    }

    indent--;
    print_client("}\n");
    print_client("RpcFinally\n");
    print_client("{\n");
    indent++;
    print_client( "__finally_%s%s( __frame );\n", prefix_client, get_name(func) );
    indent--;
    print_client("}\n");
    print_client("RpcEndFinally\n");


    /* emit return code */
    if (has_ret)
    {
        fprintf(client, "\n");
        print_client("return %s;\n", retval->name);
    }

    indent--;
    fprintf(client, "}\n");
    fprintf(client, "\n");
}

static void write_serialize_function(FILE *file, const type_t *type, const type_t *iface,
                                     const char *func_name, const char *ret_type)
{
    enum stub_mode mode = get_stub_mode();
    static int emited_pickling_info;

    if (iface && !type->typestring_offset)
    {
        /* FIXME: Those are mostly basic types. They should be implemented
         * using NdrMesSimpleType* functions */
        if (ret_type) warning("Serialization of type %s is not supported\n", type->name);
        return;
    }

    if (!emited_pickling_info && iface && mode != MODE_Os)
    {
        fprintf(file, "static const MIDL_TYPE_PICKLING_INFO __MIDL_TypePicklingInfo =\n");
        fprintf(file, "{\n");
        fprintf(file, "    0x33205054,\n");
        fprintf(file, "    0x3,\n");
        fprintf(file, "    0,\n");
        fprintf(file, "    0,\n");
        fprintf(file, "    0\n");
        fprintf(file, "};\n");
        fprintf(file, "\n");
        emited_pickling_info = 1;
    }

    /* FIXME: Assuming explicit handle */

    fprintf(file, "%s __cdecl %s_%s(handle_t IDL_handle, %s *IDL_type)%s\n",
            ret_type ? ret_type : "void", type->name, func_name, type->name, iface ? "" : ";");
    if (!iface) return; /* declaration only */

    fprintf(file, "{\n");
    fprintf(file, "    %sNdrMesType%s%s(\n", ret_type ? "return " : "", func_name,
            mode != MODE_Os ? "2" : "");
    fprintf(file, "        IDL_handle,\n");
    if (mode != MODE_Os)
        fprintf(file, "        (MIDL_TYPE_PICKLING_INFO*)&__MIDL_TypePicklingInfo,\n");
    fprintf(file, "        &%s_StubDesc,\n", iface->name);
    fprintf(file, "        (PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u],\n",
            type->typestring_offset);
    fprintf(file, "        IDL_type);\n");
    fprintf(file, "}\n");
    fprintf(file, "\n");
}

void write_serialize_functions(FILE *file, const type_t *type, const type_t *iface)
{
    if (is_attr(type->attrs, ATTR_ENCODE))
    {
        write_serialize_function(file, type, iface, "AlignSize", "SIZE_T");
        write_serialize_function(file, type, iface, "Encode", NULL);
    }
    if (is_attr(type->attrs, ATTR_DECODE))
    {
        write_serialize_function(file, type, iface, "Decode", NULL);
        write_serialize_function(file, type, iface, "Free", NULL);
    }
}

static void write_function_stubs(type_t *iface, unsigned int *proc_offset)
{
    const statement_t *stmt;
    const var_t *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
    int method_count = 0;

    if (!implicit_handle)
        print_client("static RPC_BINDING_HANDLE %s__MIDL_AutoBindHandle;\n\n", iface->name);

    LIST_FOR_EACH_ENTRY( stmt, type_iface_get_stmts(iface), const statement_t, entry )
    {
        switch (stmt->type)
        {
        case STMT_DECLARATION:
        {
            const var_t *func = stmt->u.var;
            if (stmt->u.var->stgclass != STG_NONE
                || type_get_type_detect_alias(stmt->u.var->type) != TYPE_FUNCTION)
                continue;
            write_function_stub( iface, func, method_count++, *proc_offset );
            *proc_offset += get_size_procformatstring_func( iface, func );
            break;
        }
        case STMT_TYPEDEF:
        {
            const type_list_t *type_entry;
            for (type_entry = stmt->u.type_list; type_entry; type_entry = type_entry->next)
                write_serialize_functions(client, type_entry->type, iface);
            break;
        }
        default:
            break;
        }
    }
}


static void write_stubdescdecl(type_t *iface)
{
    print_client("static const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(client, "\n");
}


static void write_stubdescriptor(type_t *iface, int expr_eval_routines)
{
    const var_t *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    print_client("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_client("{\n");
    indent++;
    print_client("(void *)& %s___RpcClientInterface,\n", iface->name);
    print_client("MIDL_user_allocate,\n");
    print_client("MIDL_user_free,\n");
    print_client("{\n");
    indent++;
    if (implicit_handle)
        print_client("&%s,\n", implicit_handle->name);
    else
        print_client("&%s__MIDL_AutoBindHandle,\n", iface->name);
    indent--;
    print_client("},\n");
    print_client("0,\n");
    if (!list_empty( &generic_handle_list ))
        print_client("BindingRoutines,\n");
    else
        print_client("0,\n");
    if (expr_eval_routines)
        print_client("ExprEvalRoutines,\n");
    else
        print_client("0,\n");
    print_client("0,\n");
    print_client("__MIDL_TypeFormatString.Format,\n");
    print_client("1, /* -error bounds_check flag */\n");
    print_client("0x%x, /* Ndr library version */\n", get_stub_mode() == MODE_Oif ? 0x50002 : 0x10001);
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
    unsigned int ver = get_attrv(iface->attrs, ATTR_VERSION);
    const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const str_list_t *endpoints = get_attrp(iface->attrs, ATTR_ENDPOINT);

    if (endpoints) write_endpoints( client, iface->name, endpoints );

    print_client("static const RPC_CLIENT_INTERFACE %s___RpcClientInterface =\n", iface->name );
    print_client("{\n");
    indent++;
    print_client("sizeof(RPC_CLIENT_INTERFACE),\n");
    print_client("{{0x%08x,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
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
    const var_t *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

    if (implicit_handle)
    {
        write_type_decl( client, implicit_handle->type, implicit_handle->name );
        fprintf(client, ";\n\n");
    }
}


static void init_client(void)
{
    if (client) return;
    if (!(client = fopen(client_name, "w")))
        error("Could not open %s for output\n", client_name);

    print_client("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
    print_client("#include <string.h>\n");
    print_client( "\n");
    print_client("#include \"%s\"\n", header_name);
    print_client( "\n");
    print_client( "#ifndef DECLSPEC_HIDDEN\n");
    print_client( "#define DECLSPEC_HIDDEN\n");
    print_client( "#endif\n");
    print_client( "\n");
}


static void write_client_ifaces(const statement_list_t *stmts, int expr_eval_routines, unsigned int *proc_offset)
{
    const statement_t *stmt;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
        {
            int needs_stub = 0;
            const statement_t *stmt2;
            type_t *iface = stmt->u.type;
            if (!need_stub(iface))
                return;

            fprintf(client, "/*****************************************************************************\n");
            fprintf(client, " * %s interface\n", iface->name);
            fprintf(client, " */\n");
            fprintf(client, "\n");

            LIST_FOR_EACH_ENTRY(stmt2, type_iface_get_stmts(iface), const statement_t, entry)
            {
                if (stmt2->type == STMT_DECLARATION && stmt2->u.var->stgclass == STG_NONE &&
                    type_get_type_detect_alias(stmt2->u.var->type) == TYPE_FUNCTION)
                {
                    needs_stub = 1;
                    break;
                }
                if (stmt2->type == STMT_TYPEDEF)
                {
                    const type_list_t *type_entry;
                    for (type_entry = stmt2->u.type_list; type_entry; type_entry = type_entry->next)
                    {
                        if (is_attr(type_entry->type->attrs, ATTR_ENCODE)
                            || is_attr(type_entry->type->attrs, ATTR_DECODE))
                        {
                            needs_stub = 1;
                            break;
                        }
                    }
                    if (needs_stub)
                        break;
                }
            }
            if (needs_stub)
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
    }
}

static void write_generic_handle_routine_list(void)
{
    generic_handle_t *gh;

    if (list_empty( &generic_handle_list )) return;
    print_client( "static const GENERIC_BINDING_ROUTINE_PAIR BindingRoutines[] =\n" );
    print_client( "{\n" );
    indent++;
    LIST_FOR_EACH_ENTRY( gh, &generic_handle_list, generic_handle_t, entry )
    {
        print_client( "{ (GENERIC_BINDING_ROUTINE)%s_bind, (GENERIC_UNBIND_ROUTINE)%s_unbind },\n",
                      gh->name, gh->name );
    }
    indent--;
    print_client( "};\n\n" );
}

static void write_client_routines(const statement_list_t *stmts)
{
    unsigned int proc_offset = 0;
    int expr_eval_routines;

    if (need_inline_stubs_file( stmts ))
    {
        write_exceptions( client );
        print_client( "\n");
    }

    write_formatstringsdecl(client, indent, stmts, need_stub);
    expr_eval_routines = write_expr_eval_routines(client, client_token);
    if (expr_eval_routines)
        write_expr_eval_routine_list(client, client_token);
    write_generic_handle_routine_list();
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

    write_client_routines( stmts );
    fclose(client);
}
