/*
 * IDL Compiler
 *
 * Copyright 2005 Eric Kohl
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

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#define END_OF_LIST(list)       \
  do {                          \
    if (list) {                 \
      while (NEXT_LINK(list))   \
        list = NEXT_LINK(list); \
    }                           \
  } while(0)

static FILE* server;
static int indent = 0;


static int print_server(const char *format, ...)
{
    va_list va;
    int i, r;

    va_start(va, format);
    for (i = 0; i < indent; i++)
        fprintf(server, "    ");
    r = vfprintf(server, format, va);
    va_end(va);
    return r;
}


static void write_procformatstring(type_t *iface)
{
    func_t *cur = iface->funcs;

    print_server("static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    print_server("{\n");
    indent++;

    while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
    while (cur)
    {
        var_t *def = cur->def;

        if (is_void(def->type, NULL))
        {
            print_server("0x5b,    /* FC_END */\n");
            print_server("0x5c,    /* FC_PAD */\n");
        }
        else
        {
            print_server("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
            print_server("0x%02x,    /* <type> */\n", def->type->type);
        }

        cur = PREV_LINK(cur);
    }

    print_server("0x0\n");
    indent--;
    print_server("}\n");
    indent--;
    print_server("};\n");
    print_server("\n");
}


static void write_typeformatstring(void)
{
    print_server("static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    print_server("{\n");
    indent++;
    print_server("NdrFcShort(0x0),\n");
    print_server("0x0\n");
    indent--;
    print_server("}\n");
    indent--;
    print_server("};\n");
    print_server("\n");
}


unsigned int get_required_stack_size(type_t *type)
{
    switch(type->type)
    {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_ULONG:
        case RPC_FC_LONG:
            return 4;

      case RPC_FC_HYPER:
            return 8;

        default:
            error("Unknown/unsupported type: %s\n", type->name);
    }
}


static void write_function_stubs(type_t *iface)
{
    func_t *cur = iface->funcs;
    while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
    while (cur)
    {
        var_t *def = cur->def;

        write_type(server, def->type, def, def->tname);
        fprintf(server, " __RPC_STUB\n");
        fprintf(server, "%s_", iface->name);
        write_name(server, def);
        fprintf(server, "(\n");
        indent++;
        print_server("PRPC_MESSAGE _pRpcMessage)\n");
        indent--;

        /* write the functions body */
        fprintf(server, "{\n");
        indent++;

        /* declare return value '_RetVal' */
        if (!is_void(def->type, NULL))
        {
            print_server("");
            write_type(server, def->type, def, def->tname);
            fprintf(server, " _RetVal;\n");
        }

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

        print_server("RpcTryFinally\n");
        print_server("{\n");
        indent++;
        print_server("RpcTryExcept\n");
        print_server("{\n");
        indent++;
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


        /* Call the real server function */
        if (!is_void(def->type, NULL))
            print_server("_RetVal = ");
        else
            print_server("");
        write_name(server, def);

        /* FIXME: handle argument list */
        fprintf(server, "();\n");

        /* FIXME: Marshall the return value */
        if (!is_void(def->type, NULL))
        {
            fprintf(server, "\n");
            print_server("_StubMsg.BufferLength = %uU;\n", get_required_stack_size(def->type));
            print_server("_pRpcMessage->BufferLength = _StubMsg.BufferLength;\n");
            fprintf(server, "\n");
            print_server("_Status = I_RpcGetBuffer(_pRpcMessage);\n");
            print_server("if (_Status)\n");
            indent++;
            print_server("RpcRaiseException(_Status);\n");
            indent--;
            fprintf(server, "\n");
            print_server("_StubMsg.Buffer = (unsigned char __RPC_FAR *)_pRpcMessage->Buffer;\n");
            fprintf(server, "\n");

            print_server("*((");
            write_type(server, def->type, def, def->tname);
            fprintf(server, " __RPC_FAR *)_StubMsg.Buffer)++ = _RetVal;\n");
        }

        indent--;
        print_server("}\n");
        print_server("RpcFinally\n");
        print_server("{\n");
        print_server("}\n");
        print_server("RpcEndFinally\n");

        /* calculate buffer length */
        fprintf(server, "\n");
        print_server("_pRpcMessage->BufferLength =\n");
        indent++;
        print_server("(unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);\n");
        indent--;
        indent--;
        fprintf(server, "}\n");
        fprintf(server, "\n");

        cur = PREV_LINK(cur);
    }
}


static void write_dispatchtable(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    unsigned long method_count = 0;
    func_t *cur = iface->funcs;

    print_server("static RPC_DISPATCH_FUNCTION %s_table[] =\n", iface->name);
    print_server("{\n");
    indent++;
    while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
    while (cur)
    {
        var_t *def = cur->def;

        print_server("%s_", iface->name);
        write_name(server, def);
        fprintf(server, ",\n");

        method_count++;
        cur = PREV_LINK(cur);
    }
    print_server("0\n");
    indent--;
    print_server("};\n");
    print_server("RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable =\n", iface->name, LOWORD(ver), HIWORD(ver));
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
    print_server("extern const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(server, "\n");
}


static void write_stubdescriptor(type_t *iface)
{
    print_server("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_server("{\n");
    indent++;
    print_server("(void __RPC_FAR *)& %s___RpcServerInterface,\n", iface->name);
    print_server("MIDL_user_allocate,\n");
    print_server("MIDL_user_free,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("__MIDL_TypeFormatString.Format,\n");
    print_server("1, /* -error bounds_check flag */\n");
    print_server("0x10001, /* Ndr library version */\n");
    print_server("0,\n");
    print_server("0x50100a4, /* MIDL Version 5.1.164 */\n");
    print_server("0,\n");
    print_server("0,\n");
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

    print_server("extern RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable;\n", iface->name, LOWORD(ver), HIWORD(ver));
    fprintf(server, "\n");
    print_server("static const RPC_SERVER_INTERFACE %s___RpcServerInterface =\n", iface->name );
    print_server("{\n");
    indent++;
    print_server("sizeof(RPC_SERVER_INTERFACE),\n");
    print_server("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], LOWORD(ver), HIWORD(ver));
    print_server("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_server("&%s_v%d_%d_DispatchTable,\n", iface->name, LOWORD(ver), HIWORD(ver));
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    indent--;
    print_server("};\n");
    print_server("RPC_IF_HANDLE %s_v%d_%d_s_ifspec = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                 iface->name, LOWORD(ver), HIWORD(ver), iface->name);
    fprintf(server, "\n");
}

static void write_formatdesc( const char *str )
{
    print_server("typedef struct _MIDL_%s_FORMAT_STRING\n", str );
    print_server("{\n");
    indent++;
    print_server("short Pad;\n");
    print_server("unsigned char Format[%s_FORMAT_STRING_SIZE];\n", str);
    indent--;
    print_server("} MIDL_%s_FORMAT_STRING;\n", str);
    print_server("\n");
}


static void write_formatstringsdecl(type_t *iface)
{
    func_t *cur;
    int byte_count = 1;

    print_server("#define TYPE_FORMAT_STRING_SIZE %d\n", 3); /* FIXME */

    /* determine the proc format string size */
    cur = iface->funcs;
    while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
    while (cur)
    {
        byte_count += 2; /* FIXME: determine real size */
        cur = PREV_LINK(cur);
    }
    print_server("#define PROC_FORMAT_STRING_SIZE %d\n", byte_count);

    fprintf(server, "\n");
    write_formatdesc("TYPE");
    write_formatdesc("PROC");
    fprintf(server, "\n");
    print_server("extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;\n");
    print_server("extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;\n");
    print_server("\n");
}


static void init_server(void)
{
    if (server)
        return;
    if (!(server = fopen(server_name, "w")))
        error("Could not open %s for output\n", server_name);

    print_server("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", WIDL_FULLVERSION, input_name);
    print_server("#include<string.h>\n");
    fprintf(server, "\n");
    print_server("#include\"%s\"\n", header_name);
    fprintf(server, "\n");
}


void write_server(ifref_t *ifaces)
{
    ifref_t *lcur = ifaces;
    char *file_id = server_token;
    int c;

    if (!do_server)
        return;
    if (!lcur)
        return;
    END_OF_LIST(lcur);

    init_server();
    if (!server)
        return;

    write_formatstringsdecl(lcur->iface);
    write_serverinterfacedecl(lcur->iface);
    write_stubdescdecl(lcur->iface);

    write_function_stubs(lcur->iface);

    write_stubdescriptor(lcur->iface);
    write_dispatchtable(lcur->iface);

    print_server("#if !defined(__RPC_WIN32__)\n");
    print_server("#error  Invalid build platform for this stub.\n");
    print_server("#endif\n");
    fprintf(server, "\n");

    write_procformatstring(lcur->iface);
    write_typeformatstring();

    fprintf(server, "\n");

    fclose(server);
}
