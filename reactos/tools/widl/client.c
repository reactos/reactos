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
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "windef.h"

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#include "widltypes.h"
#include "typelib.h"
#include "typelib_struct.h"
#include "typegen.h"

static FILE* client;
static int indent = 0;

static int print_client( const char *format, ... )
{
    va_list va;
    int i, r;

    va_start(va, format);
    for (i = 0; i < indent; i++)
        fprintf(client, "    ");
    r = vfprintf(client, format, va);
    va_end(va);
    return r;
}


static void print_message_buffer_size(const func_t *func)
{
    unsigned int total_size = 0;

    if (func->args)
    {
        const var_t *var = func->args;
        while (NEXT_LINK(var)) var = NEXT_LINK(var);
        while (var)
        {
            unsigned int alignment;

            total_size += get_required_buffer_size(var, &alignment, PASS_IN);
            total_size += alignment;

            var = PREV_LINK(var);
        }
    }
    fprintf(client, " %u", total_size);
}

static void check_pointers(const func_t *func)
{
    var_t *var;
    int pointer_type;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        pointer_type = get_attrv(var->attrs, ATTR_POINTERTYPE);
        if (!pointer_type)
            pointer_type = RPC_FC_RP;

        if (pointer_type == RPC_FC_RP)
        {
            if (var->ptr_level >= 1)
            {
                print_client("if (!%s)\n", var->name);
                print_client("{\n");
                indent++;
                print_client("RpcRaiseException(RPC_X_NULL_REF_POINTER);\n");
                indent--;
                print_client("}\n\n");
            }
        }

        var = PREV_LINK(var);
    }
}

static void write_function_stubs(type_t *iface, unsigned int *proc_offset, unsigned int *type_offset)
{
    const func_t *func = iface->funcs;
    const char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
    int explicit_handle = is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE);
    var_t *var;
    int method_count = 0;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        const var_t *def = func->def;
        const var_t* explicit_handle_var;
        unsigned int type_offset_func;

        /* check for a defined binding handle */
        explicit_handle_var = get_explicit_handle_var(func);
        if (explicit_handle)
        {
            if (!explicit_handle_var)
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

        write_type(client, def->type, def, def->tname);
        fprintf(client, " ");
        write_name(client, def);
        fprintf(client, "(\n");
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

        /* declare return value '_RetVal' */
        if (!is_void(def->type, NULL))
        {
            print_client("");
            write_type(client, def->type, def, def->tname);
            fprintf(client, " _RetVal;\n");
        }

        if (implicit_handle || explicit_handle_var)
            print_client("RPC_BINDING_HANDLE _Handle = 0;\n");

        print_client("RPC_MESSAGE _RpcMessage;\n");
        print_client("MIDL_STUB_MESSAGE _StubMsg;\n");
        fprintf(client, "\n");

        /* check pointers */
        check_pointers(func);

        print_client("RpcTryFinally\n");
        print_client("{\n");
        indent++;

        print_client("NdrClientInitializeNew(\n");
        indent++;
        print_client("(PRPC_MESSAGE)&_RpcMessage,\n");
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("(PMIDL_STUB_DESC)&%s_StubDesc,\n", iface->name);
        print_client("%d);\n", method_count);
        indent--;
        fprintf(client, "\n");

        if (implicit_handle)
        {
            print_client("_Handle = %s;\n", implicit_handle);
            fprintf(client, "\n");
        }
        else if (explicit_handle_var)
        {
            print_client("_Handle = %s;\n", explicit_handle_var->name);
            fprintf(client, "\n");
        }

        /* emit the message buffer size */
        print_client("_StubMsg.BufferLength =");
        print_message_buffer_size(func);
        fprintf(client, ";\n");

        type_offset_func = *type_offset;
        write_remoting_arguments(client, indent, func, &type_offset_func, PASS_IN, PHASE_BUFFERSIZE);

        print_client("NdrGetBuffer(\n");
        indent++;
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("_StubMsg.BufferLength,\n");
        if (implicit_handle || explicit_handle_var)
            print_client("_Handle);\n");
        else
            print_client("%s__MIDL_AutoBindHandle);\n", iface->name);
        indent--;
        fprintf(client, "\n");


        /* make a copy so we don't increment the type offset twice */
        type_offset_func = *type_offset;

        /* marshal arguments */
        write_remoting_arguments(client, indent, func, &type_offset_func, PASS_IN, PHASE_MARSHAL);

        /* send/receive message */
        /* print_client("NdrNsSendReceive(\n"); */
        /* print_client("(unsigned char *)_StubMsg.Buffer,\n"); */
        /* print_client("(RPC_BINDING_HANDLE *) &%s__MIDL_AutoBindHandle);\n", iface->name); */
        print_client("NdrSendReceive(\n");
        indent++;
        print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
        print_client("(unsigned char *)_StubMsg.Buffer);\n\n");
        indent--;

        print_client("_StubMsg.BufferStart = (unsigned char *)_RpcMessage.Buffer;\n");
        print_client("_StubMsg.BufferEnd = _StubMsg.BufferStart + _RpcMessage.BufferLength;\n");

        if (has_out_arg_or_return(func))
        {
            fprintf(client, "\n");

            print_client("if ((_RpcMessage.DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_client("NdrConvert(\n");
            indent++;
            print_client("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
            print_client("(PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n", *proc_offset);
            indent -= 2;
        }

        /* unmarshall arguments */
        fprintf(client, "\n");
        write_remoting_arguments(client, indent, func, type_offset, PASS_OUT, PHASE_UNMARSHAL);

        /* unmarshal return value */
        if (!is_void(def->type, NULL))
            print_phase_basetype(client, indent, PHASE_UNMARSHAL, PASS_RETURN, def, "_RetVal");

        /* update proc_offset */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                *proc_offset += get_size_procformatstring_var(var);
                var = PREV_LINK(var);
            }
        }
        if (!is_void(def->type, NULL))
            *proc_offset += get_size_procformatstring_var(def);
        else
            *proc_offset += 2; /* FC_END and FC_PAD */

        indent--;
        print_client("}\n");
        print_client("RpcFinally\n");
        print_client("{\n");
        indent++;


        /* FIXME: emit client finally code */

        print_client("NdrFreeBuffer((PMIDL_STUB_MESSAGE)&_StubMsg);\n");

        indent--;
        print_client("}\n");
        print_client("RpcEndFinally\n");


        /* emit return code */
        if (!is_void(def->type, NULL))
        {
            fprintf(client, "\n");
            print_client("return _RetVal;\n");
        }

        indent--;
        fprintf(client, "}\n");
        fprintf(client, "\n");

        method_count++;
        func = PREV_LINK(func);
    }
}


static void write_bindinghandledecl(type_t *iface)
{
    print_client("static RPC_BINDING_HANDLE %s__MIDL_AutoBindHandle;\n", iface->name);
    fprintf(client, "\n");
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
    print_client("0,\n");
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

    print_client("static const RPC_CLIENT_INTERFACE %s___RpcClientInterface =\n", iface->name );
    print_client("{\n");
    indent++;
    print_client("sizeof(RPC_CLIENT_INTERFACE),\n");
    print_client("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], LOWORD(ver), HIWORD(ver));
    print_client("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    print_client("0,\n");
    indent--;
    print_client("};\n");
    if (old_names)
        print_client("RPC_IF_HANDLE %s_ClientIfHandle = (RPC_IF_HANDLE)& %s___RpcClientInterface;\n",
                     iface->name, iface->name);
    else
        print_client("RPC_IF_HANDLE %s_v%d_%d_c_ifspec = (RPC_IF_HANDLE)& %s___RpcClientInterface;\n",
                     iface->name, LOWORD(ver), HIWORD(ver), iface->name);
    fprintf(client, "\n");
}


static void write_formatdesc( const char *str )
{
    print_client("typedef struct _MIDL_%s_FORMAT_STRING\n", str );
    print_client("{\n");
    indent++;
    print_client("short Pad;\n");
    print_client("unsigned char Format[%s_FORMAT_STRING_SIZE];\n", str);
    indent--;
    print_client("} MIDL_%s_FORMAT_STRING;\n", str);
    print_client("\n");
}


static void write_formatstringsdecl(ifref_t *ifaces)
{
    print_client("#define TYPE_FORMAT_STRING_SIZE %d\n",
                 get_size_typeformatstring(ifaces));

    print_client("#define PROC_FORMAT_STRING_SIZE %d\n",
                 get_size_procformatstring(ifaces));

    fprintf(client, "\n");
    write_formatdesc("TYPE");
    write_formatdesc("PROC");
    fprintf(client, "\n");
    print_client("static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;\n");
    print_client("static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;\n");
    print_client("\n");
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

    print_client("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", WIDL_FULLVERSION, input_name);
    print_client("#include <string.h>\n");
    print_client("#ifdef _ALPHA_\n");
    print_client("#include <stdarg.h>\n");
    print_client("#endif\n");
    fprintf(client, "\n");
    print_client("#include \"%s\"\n", header_name);
    fprintf(client, "\n");
}


void write_client(ifref_t *ifaces)
{
    unsigned int proc_offset = 0;
    unsigned int type_offset = 2;
    ifref_t *iface = ifaces;

    if (!do_client)
        return;
    if (!iface)
        return;
    END_OF_LIST(iface);

    init_client();
    if (!client)
        return;

    write_formatstringsdecl(ifaces);

    for (; iface; iface = PREV_LINK(iface))
    {
        if (is_object(iface->iface->attrs) || is_local(iface->iface->attrs))
            continue;

        fprintf(client, "/*****************************************************************************\n");
        fprintf(client, " * %s interface\n", iface->iface->name);
        fprintf(client, " */\n");
        fprintf(client, "\n");

        if (iface->iface->funcs)
        {
            int expr_eval_routines;

            write_implicithandledecl(iface->iface);

            write_clientinterfacedecl(iface->iface);
            write_stubdescdecl(iface->iface);
            write_bindinghandledecl(iface->iface);

            write_function_stubs(iface->iface, &proc_offset, &type_offset);

            print_client("#if !defined(__RPC_WIN32__)\n");
            print_client("#error  Invalid build platform for this stub.\n");
            print_client("#endif\n");

            fprintf(client, "\n");

            expr_eval_routines = write_expr_eval_routines(client, iface->iface->name);
            if (expr_eval_routines)
                write_expr_eval_routine_list(client, iface->iface->name);
            write_stubdescriptor(iface->iface, expr_eval_routines);
        }
    }

    fprintf(client, "\n");

    write_procformatstring(client, ifaces);
    write_typeformatstring(client, ifaces);

    fclose(client);
}
