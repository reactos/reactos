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


static unsigned char
get_base_type(unsigned char type)
{

    switch (type)
    {
    case RPC_FC_USHORT:
        type = RPC_FC_SHORT;
        break;

    case RPC_FC_ULONG:
        type = RPC_FC_LONG;
        break;
    }

  return type;
}


static int get_type_size(type_t *type, int alignment)
{
    int size;
    var_t *field;

    switch(type->type)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
        size = 1;
        size = ((size + alignment - 1) & ~(alignment -1));
        break;

    case RPC_FC_WCHAR:
    case RPC_FC_USHORT:
    case RPC_FC_SHORT:
        size = 2;
        size = ((size + alignment - 1) & ~(alignment -1));
        break;

    case RPC_FC_ULONG:
    case RPC_FC_LONG:
    case RPC_FC_FLOAT:
        size = 4;
        size = ((size + alignment - 1) & ~(alignment -1));
        break;

    case RPC_FC_HYPER:
    case RPC_FC_DOUBLE:
        size = 8;
        size = ((size + alignment - 1) & ~(alignment -1));
        break;

    case RPC_FC_IGNORE:
        size = 0;
        break;

    case RPC_FC_STRUCT:
        field = type->fields;
        size = 0;
        while (NEXT_LINK(field)) field = NEXT_LINK(field);
        while (field)
        {
          size += get_type_size(field->type, alignment);
          field = PREV_LINK(field);
        }
        break;

    default:
        error("%s:%d Unknown/unsupported type 0x%x\n",
              __FUNCTION__,__LINE__, type->type);
        return 0;
    }

    return size;
}


static void write_procformatstring(type_t *iface)
{
    func_t *func = iface->funcs;
    var_t *var;
    unsigned int type_offset = 2;
    int in_attr, out_attr;

    print_server("static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    print_server("{\n");
    indent++;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* emit argument data */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                out_attr = is_attr(var->attrs, ATTR_OUT);
                in_attr = is_attr(var->attrs, ATTR_IN);

                /* set 'in' attribute if neither 'in' nor 'out' is set */
                if (!out_attr && !in_attr)
                    in_attr = 1;

                if (var->ptr_level == 0)
                {
                    if (is_base_type(var->type))
                    {
                        print_server("0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                        print_server("0x%02x,    /* FC_<type> */\n", get_base_type(var->type->type));
                    }
                    else if (var->type->type == RPC_FC_RP)
                    {
                        var_t *field = var->type->ref->ref->fields;
                        int size;

                        if (in_attr & !out_attr)
                            print_server("0x4d,    /* FC_IN_PARAM */\n");
                        else if (!in_attr & out_attr)
                            print_server("0x51,    /* FC_OUT_PARAM */\n");
                        else if (in_attr & out_attr)
                            print_server("0x50,    /* FC_IN_OUT_PARAM */\n");
                        fprintf(server, "#ifndef _ALPHA_\n");
                        print_server("0x01,\n");
                        fprintf(server, "#else\n");
                        print_server("0x02,\n");
                        fprintf(server, "#endif\n");
                        print_server("NdrFcShort(0x%x),\n", type_offset);

                        size = 9;
                        while (NEXT_LINK(field)) field = NEXT_LINK(field);
                        while (field)
                        {
                            size++;
                            field = PREV_LINK(field);
                        }
                        if (size % 2)
                            size++;
                        type_offset += size;
                    }
                    else
                    {
                        error("%s:%d Unknown/unsupported type 0x%x\n",
                              __FUNCTION__,__LINE__, var->type->type);
                        return;
                    }
                }
                else if (var->ptr_level == 1)
                {
//                    if (is_base_type(var->type))
//                    {
                        if (in_attr & !out_attr)
                            print_server("0x4d,    /* FC_IN_PARAM */\n");
                        else if (!in_attr & out_attr)
                            print_server("0x51,    /* FC_OUT_PARAM */\n");
                        else if (in_attr & out_attr)
                            print_server("0x50,    /* FC_IN_OUT_PARAM */\n");
                        fprintf(server, "#ifndef _ALPHA_\n");
                        print_server("0x01,\n");
                        fprintf(server, "#else\n");
                        print_server("0x02,\n");
                        fprintf(server, "#endif\n");
                        print_server("NdrFcShort(0x%x),\n", type_offset);
                        type_offset += 4;
//                    }
//                    else
//                    {
//                        error("%s:%d Unknown/unsupported type 0x%x\n",
//                              __FUNCTION__,__LINE__, var->type->type);
//                        return;
//                    }
                }

                var = PREV_LINK(var);
            }
        }

        /* emit return value data */
        var = func->def;
        if (is_void(var->type, NULL))
        {
            print_server("0x5b,    /* FC_END */\n");
            print_server("0x5c,    /* FC_PAD */\n");
        }
        else if (is_base_type(var->type))
        {
            print_server("0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
            print_server("0x%02x,    /* FC_<type> */\n", get_base_type(var->type->type));
        }
        else
        {
            error("%s:%d Unknown/unsupported type 0x%x\n",
                  __FUNCTION__,__LINE__, var->type->type);
            return;
        }

        func = PREV_LINK(func);
    }

    print_server("0x0\n");
    indent--;
    print_server("}\n");
    indent--;
    print_server("};\n");
    print_server("\n");
}


static void write_typeformatstring(type_t *iface)
{
    func_t *func = iface->funcs;
    var_t *var;
    int in_attr, out_attr;
    int string_attr;
    int ptr_attr, ref_attr, unique_attr;

    print_server("static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    print_server("{\n");
    indent++;
    print_server("NdrFcShort(0x0),\n");

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                in_attr = is_attr(var->attrs, ATTR_IN);
                out_attr = is_attr(var->attrs, ATTR_OUT);
                string_attr = is_attr(var->attrs, ATTR_STRING);

                if (var->ptr_level > 1)
                {
                    error("Function '%s' argument '%s': Pointer level %d not supported!\n",
                          func->def->name, var->name, var->ptr_level);
                    return;
                }

                if (var->ptr_level == 0)
                {
                    if (!is_base_type(var->type))
                    {
                        if (var->type->type == RPC_FC_RP)
                        {
                            var_t *field;
                            int tsize = 9;
                            unsigned char flags = 0;

                            if (!in_attr & out_attr)
                                flags |= RPC_FC_P_ONSTACK;

                            print_server("0x11, 0x%02X,    /* FC_RP, [flags] */\n", flags);
                            print_server("NdrFcShort(0x%02X),\n", 0x02);
                            print_server("0x%02X,\n", var->type->ref->ref->type);
                            print_server("0x%02X,\n", 3); /* alignment -1 */
                            print_server("NdrFcShort(0x%02X),\n", get_type_size(var->type->ref->ref, 4));

                            field = var->type->ref->ref->fields;
                            while (NEXT_LINK(field)) field = NEXT_LINK(field);
                            while (field)
                            {
                                print_server("0x%02X,\n", get_base_type(field->type->type));
                                tsize++;
                                field = PREV_LINK(field);
                            }
                            if (tsize % 2)
                            {
                                print_server("0x5c,          /* FC_PAD */\n");
                                tsize++;
                            }
                            print_server("0x5b,          /* FC_END */\n");
                        }
                        else
                        {

                            error("%s:%d Unknown/unsupported type 0x%x\n",
                                  __FUNCTION__,__LINE__, var->type->type);
                            return;
                        }
                    }
                }
                else if (var->ptr_level == 1)
                {
                    ptr_attr = is_attr(var->attrs, ATTR_PTR);
                    ref_attr = is_attr(var->attrs, ATTR_REF);
                    unique_attr = is_attr(var->attrs, ATTR_UNIQUE);

                    if (ptr_attr + ref_attr + unique_attr == 0)
                        ref_attr = 1;

                    if (is_base_type(var->type))
                    {
                        if (out_attr)
                            print_server("0x11, 0x0c,    /* FC_RP [allocated_on_stack] [simple_pointer] */\n");
                        else
                        {
                            if (ptr_attr)
                                print_server("0x14, 0x08,    /* FC_FP [simple_pointer] */\n");
                            else if (ref_attr)
                                print_server("0x11, 0x08,    /* FC_RP [simple_pointer] */\n");
                            else if (unique_attr)
                                print_server("0x12, 0x08,    /* FC_UP [simple_pointer] */\n");
                        }

                        if (string_attr)
                        {
                            if (var->type->type == RPC_FC_CHAR)
                                print_server("0x%02x,          /* FC_C_CSTRING */\n", RPC_FC_C_CSTRING);
                            else if (var->type->type == RPC_FC_WCHAR)
                                print_server("0x%02x,          /* FC_C_WSTRING */\n", RPC_FC_C_WSTRING);
                            else
                            {
                                error("%s: Invalid type!\n", __FUNCTION__);
                                return;
                            }
                        }
                        else
                            print_server("0x%02x,          /* FC_<type> */\n", get_base_type(var->type->type));
                        print_server("0x5c,          /* FC_PAD */\n");
                    }
                }

                var = PREV_LINK(var);
            }
        }



        func = PREV_LINK(func);
    }

    print_server("0x0\n");
    indent--;
    print_server("}\n");
    indent--;
    print_server("};\n");
    print_server("\n");
}


static void print_message_buffer_size(func_t *func)
{
    unsigned int alignment = 0;
    int size = 0;
    int last_size = -1;
    int in_attr;
    int out_attr;
    int string_attr;
    int empty_line;
    var_t *var;

    fprintf(server, "\n");
    print_server("_StubMsg.BufferLength =");
    if (func->args)
    {
        var = func->args;
        while (NEXT_LINK(var)) var = NEXT_LINK(var);
        for (; var; var = PREV_LINK(var))
        {
            out_attr = is_attr(var->attrs, ATTR_OUT);
            if (out_attr)
            {
                if (is_base_type(var->type))
                {
                    if (size != 0 && last_size != 0)
                    {
                        print_server("_StubMsg.BufferLength +=");
                    }

                    alignment = 0;
                    switch (var->type->type)
                    {
                    case RPC_FC_BYTE:
                    case RPC_FC_CHAR:
                    case RPC_FC_SMALL:
                        size = 1;
                        alignment = 0;
                        break;

                    case RPC_FC_WCHAR:
                    case RPC_FC_USHORT:
                    case RPC_FC_SHORT:
                        size = 2;
                        if (last_size > 0 && last_size < 2)
                            alignment += (2 - last_size);
                        break;

                    case RPC_FC_ULONG:
                    case RPC_FC_LONG:
                    case RPC_FC_FLOAT:
                        size = 4;
                        if (last_size > 0 && last_size < 4)
                            alignment += (4 - last_size);
                        break;

                    case RPC_FC_HYPER:
                    case RPC_FC_DOUBLE:
                        size = 8;
                        if (last_size > 0 && last_size < 4)
                            alignment += (4 - last_size);
                        break;

                    default:
                        error("%s:%d Unknown/unsupported type 0x%x\n",
                              __FUNCTION__,__LINE__, var->type->type);
                        return;
                    }

                    if (last_size != -1)
                        fprintf(server, " +");
                    fprintf(server, " %dU", (size == 0) ? 0 : size + alignment);

                    last_size = size;
                }
                else if (var->type->type == RPC_FC_RP)
                {
                    if (size == 0)
                    {
                      fprintf(server, " 0;\n");
                    }
                    else if (last_size != 0)
                    {
                      fprintf(server, ";\n");
                      last_size = 0;
                    }

                    fprintf(server,"\n");
                    print_server("NdrSimpleStructBufferSize(\n");
                    indent++;
                    print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
                    print_server("(unsigned char __RPC_FAR *)%s,\n", var->name);
                    print_server("(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u]);\n",
                                 0); /* FIXME */
                    indent--;
                    fprintf(server,"\n");
                }
            }
        }
    }

    /* return value size */
    if (!is_void(func->def->type, NULL))
    {
        switch(func->def->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_SMALL:
        case RPC_FC_CHAR:
        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
            size = 4;
            if (last_size > 0 && last_size < 4)
                alignment += (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size > 0 && last_size < 4)
                alignment += (4 - last_size);
            break;

        default:
            error("%s:%d Unknown/unsupported type 0x%x\n",
                  __FUNCTION__,__LINE__, func->def->type->type);
            return;
        }

        if (last_size != -1)
            fprintf(server, " +");

        fprintf(server, " %dU", (size == 0) ? 0 : size + alignment);
    }
    fprintf(server, ";\n");

    /* get string size */
    if (func->args)
    {
        empty_line = 0;
        var = func->args;
        while (NEXT_LINK(var)) var = NEXT_LINK(var);
        for (; var; var = PREV_LINK(var))
        {
            out_attr = is_attr(var->attrs, ATTR_OUT);
            in_attr = is_attr(var->attrs, ATTR_IN);
            string_attr = is_attr(var->attrs, ATTR_STRING);

            /* set 'in' attribute if neither 'in' nor 'out' is found */
            if (!out_attr && !in_attr)
                in_attr = 1;

            if (var->ptr_level == 1 &&
                string_attr &&
                (var->type->type == RPC_FC_CHAR || var->type->type == RPC_FC_WCHAR))
            {
                print_server("_StubMsg.BufferLength += 16;\n");
                empty_line = 1;
            }
        }

        if (empty_line)
            fprintf(server, "\n");
    }
}


static void init_pointers (func_t *func)
{
    var_t *var;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        if (var->ptr_level == 0)
        {
            if (var->type->type == RPC_FC_RP)
            {
                print_server("(");
                write_type(server, var->type, NULL, var->tname);
                fprintf(server, ")%s = 0;\n", var->name);
            }
        }
        else if (var->ptr_level == 1)
        {
            print_server("(");
            write_type(server, var->type, NULL, var->tname);
            fprintf(server, " __RPC_FAR *)%s = 0;\n", var->name);
        }
        else if (var->ptr_level > 1)
        {
            error("Pointer level %d not supported!\n", var->ptr_level);
            return;
        }

        var = PREV_LINK(var);
    }
    fprintf(server, "\n");
}


static void unmarshall_in_arguments(func_t *func, unsigned int *type_offset)
{
    unsigned int alignment;
    unsigned int size;
    unsigned int last_size = 0;
    var_t *var;
    int in_attr, out_attr;
    int string_attr;
    int ptr_attr, ref_attr, unique_attr;
    unsigned int local_type_offset = *type_offset;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    for (; var; var = PREV_LINK(var))
    {
        out_attr = is_attr(var->attrs, ATTR_OUT);
        in_attr = is_attr(var->attrs, ATTR_IN);
        string_attr = is_attr(var->attrs, ATTR_STRING);

        /* set 'in' attribute if neither 'in' nor 'out' is set */
        if (!out_attr && !in_attr)
            in_attr = 1;

        if (in_attr)
        {
            if (var->ptr_level == 1)
            {
                ptr_attr = is_attr(var->attrs, ATTR_PTR);
                ref_attr = is_attr(var->attrs, ATTR_REF);
                unique_attr = is_attr(var->attrs, ATTR_UNIQUE);
                if (ptr_attr + ref_attr + unique_attr == 0)
                    ref_attr = 1;

                if (ref_attr)
                {
                    if (string_attr &&
                        (var->type->type == RPC_FC_CHAR || var->type->type == RPC_FC_WCHAR))
                    {
                        print_server("NdrConformantStringUnmarshall(\n");
                        indent++;
                        print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
                        print_server("(unsigned char __RPC_FAR * __RPC_FAR *)&%s,\n", var->name);
                        print_server("(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u],\n",
                                     local_type_offset + 2);
                        print_server("(unsigned char)0);\n");
                        indent--;
                        fprintf(server, "\n");
                    }
                    else
                    {
                        alignment = 0;
                        switch (var->type->type)
                        {
                        case RPC_FC_BYTE:
                        case RPC_FC_CHAR:
                        case RPC_FC_SMALL:
                            size = 1;
                            alignment = 0;
                            break;

                        case RPC_FC_WCHAR:
                        case RPC_FC_USHORT:
                        case RPC_FC_SHORT:
                            size = 2;
                            if (last_size != 0 && last_size < 2)
                                alignment = (2 - last_size);
                            break;

                        case RPC_FC_ULONG:
                        case RPC_FC_LONG:
                        case RPC_FC_FLOAT:
                            size = 4;
                            if (last_size != 0 && last_size < 4)
                                alignment = (4 - last_size);
                            break;

                        case RPC_FC_HYPER:
                        case RPC_FC_DOUBLE:
                            size = 8;
                            if (last_size != 0 && last_size < 4)
                                alignment = (4 - last_size);
                            break;

                        case RPC_FC_IGNORE:
                            size = 0;
                            break;

                        default:
                            error("%s:%d Unknown/unsupported type 0x%x\n",
                                  __FUNCTION__,__LINE__, var->type->type);
                            return;
                        }

                        if (size != 0)
                        {
                            if (alignment != 0)
                                print_server("_StubMsg.Buffer += %u;\n", alignment);

                            print_server("");
                            write_name(server, var);
                            fprintf(server, " = (");
                            write_type(server, var->type, NULL, var->tname);
                            fprintf(server, " __RPC_FAR*)_StubMsg.Buffer;\n");
                            print_server("_StubMsg.Buffer += sizeof(");
                            write_type(server, var->type, NULL, var->tname);
                            fprintf(server, ");\n");
                            fprintf(server, "\n");

                            last_size = size;
                        }
                    }
                }
                else if (unique_attr)
                {
                    print_server("NdrPointerUnmarshall(\n");
                    indent++;
                    print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
                    print_server("(unsigned char __RPC_FAR * __RPC_FAR *)&%s,\n", var->name);
                    print_server("(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u],\n",
                                 local_type_offset);
                    print_server("(unsigned char)0);\n");
                    indent--;
                    fprintf(server, "\n");
                }

            }
            else
            {
                if (is_base_type(var->type))
                {
                    alignment = 0;
                    switch (var->type->type)
                    {
                    case RPC_FC_BYTE:
                    case RPC_FC_CHAR:
                    case RPC_FC_SMALL:
                        size = 1;
                        alignment = 0;
                        break;

                    case RPC_FC_WCHAR:
                    case RPC_FC_USHORT:
                    case RPC_FC_SHORT:
                        size = 2;
                        if (last_size != 0 && last_size < 2)
                            alignment = (2 - last_size);
                        break;

                    case RPC_FC_ULONG:
                    case RPC_FC_LONG:
                    case RPC_FC_FLOAT:
                        size = 4;
                        if (last_size != 0 && last_size < 4)
                            alignment = (4 - last_size);
                        break;

                    case RPC_FC_HYPER:
                    case RPC_FC_DOUBLE:
                        size = 8;
                        if (last_size != 0 && last_size < 4)
                            alignment = (4 - last_size);
                        break;

                    case RPC_FC_IGNORE:
                        size = 0;
                        break;

                    default:
                        error("%s:%d Unknown/unsupported type 0x%x\n",
                              __FUNCTION__,__LINE__, var->type->type);
                        return;
                    }

                    if (size != 0)
                    {
                        if (alignment != 0)
                            print_server("_StubMsg.Buffer += %u;\n", alignment);

                        print_server("");
                        write_name(server, var);
                        fprintf(server, " = *((");
                        write_type(server, var->type, NULL, var->tname);
                        fprintf(server, " __RPC_FAR*)_StubMsg.Buffer);\n");
                        print_server("_StubMsg.Buffer += sizeof(");
                        write_type(server, var->type, NULL, var->tname);
                        fprintf(server, ");\n");
                        fprintf(server, "\n");

                        last_size = size;
                    }
                }
                else if (var->type->type == RPC_FC_RP)
                {
                    if (var->type->ref->ref->type == RPC_FC_STRUCT)
                    {
                        print_server("NdrSimpleStructUnmarshall(\n");
                        indent++;
                        print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
                        print_server("(unsigned char __RPC_FAR * __RPC_FAR *)&%s,\n", var->name);
                        print_server("(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u],\n",
                                     local_type_offset + 4);
                        print_server("(unsigned char)0);\n");
                        indent--;
                        fprintf(server, "\n");
                    }
                }
            }
        }

        /* calculate the next type offset */
        if (var->ptr_level == 0)
        {
            if ((var->type->type == RPC_FC_RP) &&
                (var->type->ref->ref->type == RPC_FC_STRUCT))
            {
                var_t *field = var->type->ref->ref->fields;
                int tsize = 9;

                while (NEXT_LINK(field)) field = NEXT_LINK(field);
                while (field)
                {
                    tsize++;
                    field = PREV_LINK(field);
                }
                if (tsize % 2)
                    tsize++;

                local_type_offset += tsize;
            }
        }
        else if (var->ptr_level == 1)
        {
            local_type_offset += 4;
        }
    }
}


static void marshall_out_arguments(func_t *func, unsigned int *type_offset)
{
    unsigned int alignment = 0;
    unsigned int size = 0;
    unsigned int last_size = 0;
    var_t *var;
    var_t *def;
    int out_attr;
    unsigned int local_type_offset = *type_offset;

    def = func->def;

    /* marshall the out arguments */
    if (func->args)
    {
        var = func->args;
        while (NEXT_LINK(var)) var = NEXT_LINK(var);
        for (; var; var = PREV_LINK(var))
        {
            out_attr = is_attr(var->attrs, ATTR_OUT);
            if (out_attr)
            {
                if (is_base_type(var->type))
                {
                    alignment = 0;
                    switch (var->type->type)
                    {
                    case RPC_FC_BYTE:
                    case RPC_FC_CHAR:
                    case RPC_FC_SMALL:
                        size = 1;
                        alignment = 0;
                        break;

                    case RPC_FC_WCHAR:
                    case RPC_FC_USHORT:
                    case RPC_FC_SHORT:
                        size = 2;
                        if (last_size != 0 && last_size < 2)
                            alignment = (2 - last_size);
                        break;

                    case RPC_FC_ULONG:
                    case RPC_FC_LONG:
                    case RPC_FC_FLOAT:
                        size = 4;
                        if (last_size != 0 && last_size < 4)
                            alignment = (4 - last_size);
                        break;

                    case RPC_FC_HYPER:
                    case RPC_FC_DOUBLE:
                        size = 8;
                        if (last_size != 0 && last_size < 4)
                            alignment = (4 - last_size);
                        break;

                    case RPC_FC_IGNORE:
                        size = 0;
                        break;

                    default:
                        error("%s:%d Unknown/unsupported type 0x%x\n",
                              __FUNCTION__,__LINE__, var->type->type);
                        return;
                    }

                    if (size != 0)
                    {
                        if (alignment != 0)
                            print_server("_StubMsg.Buffer += %u;\n", alignment);

                        if (var->ptr_level == 1)
                        {
                            fprintf(server, "\n");
                            print_server("*((");
                            write_type(server, var->type, NULL, var->tname);
                            fprintf(server, " __RPC_FAR *)_StubMsg.Buffer) = *");
                            write_name(server, var);
                            fprintf(server, ";\n");

                            print_server("_StubMsg.Buffer += sizeof(");
                            write_type(server, var->type, NULL, var->tname);
                            fprintf(server, ");");
                        }
                        else
                        {
                            error("Pointer level %d is not supported!\n", var->ptr_level);
                            return;
                        }

                        last_size = size;
                    }
                }
                else if (var->type->type == RPC_FC_RP)
                {
                    fprintf(server, "\n");
                    print_server("NdrSimpleStructMarshall(\n");
                    indent++;
                    print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
                    print_server("(unsigned char __RPC_FAR *)%s,\n", var->name);
                    print_server("(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%u]);\n",
                                 local_type_offset + 4);
                    indent--;
                }
            }

            /* calculate the next type offset */
            if (var->ptr_level == 0)
            {
                if ((var->type->type == RPC_FC_RP) &&
                    (var->type->ref->ref->type == RPC_FC_STRUCT))
                {
                    var_t *field = var->type->ref->ref->fields;
                    int tsize = 9;

                    while (NEXT_LINK(field)) field = NEXT_LINK(field);
                    while (field)
                    {
                        tsize++;
                        field = PREV_LINK(field);
                    }
                    if (tsize % 2)
                        tsize++;

                    local_type_offset += tsize;
                }
            }
            else if (var->ptr_level == 1)
            {
                local_type_offset += 4;
            }
        }
    }

    /* marshall the return value */
    if (!is_void(def->type, NULL))
    {
        alignment = 0;
        switch (def->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
            size = 4;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        default:
            error("%s:%d Unknown/unsupported type 0x%x\n",
                  __FUNCTION__,__LINE__, var->type->type);
            return;
        }

        fprintf(server, "\n");
        if (alignment != 0)
            print_server("_StubMsg.Buffer += %u;\n", alignment);
        print_server("*((");
        write_type(server, def->type, def, def->tname);
        fprintf(server, " __RPC_FAR *)_StubMsg.Buffer) = _RetVal;\n");
        print_server("_StubMsg.Buffer += sizeof(");
        write_type(server, def->type, def, def->tname);
        fprintf(server, ");\n");
    }
}


static int use_return_buffer(func_t *func)
{
    var_t *var;

    if (!is_void(func->def->type, NULL))
        return 1;

    if (!func->args)
        return 0;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        if (is_attr(var->attrs, ATTR_OUT))
            return 1;

        var = PREV_LINK(var);
    }

    return 0;
}


static void write_function_stubs(type_t *iface)
{
    int explicit_handle = is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE);
    func_t *func = iface->funcs;
    var_t *var;
    var_t* explicit_handle_var;
    unsigned int proc_offset = 0;
    unsigned int type_offset = 2;
    unsigned int i, sep;
    int in_attr;
    int out_attr;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        var_t *def = func->def;

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
        else
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

        /* declare return value '_RetVal' */
        if (!is_void(def->type, NULL))
        {
            print_server("");
            write_type(server, def->type, def, def->tname);
            fprintf(server, " _RetVal;\n");
        }

        /* declare arguments */
        if (func->args)
        {
            i = 0;
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                in_attr = is_attr(var->attrs, ATTR_IN);
                out_attr = is_attr(var->attrs, ATTR_OUT);
                if (!out_attr && !in_attr)
                    in_attr = 1;
                if (!in_attr)
                {
                    if (var->type->type == RPC_FC_RP)
                    {
                        print_server("struct ");
                        write_type(server, NULL, NULL, var->type->ref->ref->name);
                        fprintf(server, " _%sW;\n", var->name);
                    }
                    else
                    {
                        print_server("");
                        write_type(server, var->type, NULL, var->tname);
                        fprintf(server, " _W%u;\n", i);
                        i++;
                    }
                }

                print_server("");
                write_type(server, var->type, var, var->tname);
                fprintf(server, " ");
                write_name(server, var);
                fprintf(server, ";\n");

                var = PREV_LINK(var);
            }
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

        if (explicit_handle)
        {
            print_server("%s = _pRpcMessage->Handle;\n", explicit_handle_var->name);
            fprintf(server, "\n");
        }

        init_pointers(func);

        print_server("RpcTryFinally\n");
        print_server("{\n");
        indent++;
        print_server("RpcTryExcept\n");
        print_server("{\n");
        indent++;

        if (func->args)
        {
            print_server("if ((_pRpcMessage->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_server("NdrConvert(\n");
            indent++;
            print_server("(PMIDL_STUB_MESSAGE)&_StubMsg,\n");
            print_server("(PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n", proc_offset);
            indent -= 2;
            fprintf(server, "\n");

            unmarshall_in_arguments(func, &type_offset);
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

        /* assign out arguments */
        if (func->args)
        {
            sep = 0;
            i = 0;
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                in_attr = is_attr(var->attrs, ATTR_IN);
                out_attr = is_attr(var->attrs, ATTR_OUT);
                if (!out_attr && !in_attr)
                    in_attr = 1;
                if (!in_attr)
                {
                    if (var->type->type == RPC_FC_RP)
                    {
                        print_server("");
                        write_name(server, var);
                        fprintf(server, " = &_%sW;\n", var->name);
                        sep = 1;
                    }
                    else
                    {
                        print_server("");
                        write_name(server, var);
                        fprintf(server, " = &_W%u;\n", i);
                        i++;
                        sep = 1;
                    }
                }

                var = PREV_LINK(var);
            }

            if (sep)
                fprintf(server, "\n");
        }

        /* Call the real server function */
        if (!is_void(def->type, NULL))
            print_server("_RetVal = ");
        else
            print_server("");
        write_name(server, def);

        if (func->args)
        {
            int first_arg = 1;

            fprintf(server, "(\n");
            indent++;
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                if (first_arg)
                    first_arg = 0;
                else
                    fprintf(server, ",\n");
                print_server("");
                write_name(server, var);
                var = PREV_LINK(var);
            }
            fprintf(server, ");\n");
            indent--;
        }
        else
        {
            fprintf(server, "();\n");
        }

        /* allocate and fill the return message buffer */
        if (use_return_buffer(func))
        {
            print_message_buffer_size(func);
            print_server("_pRpcMessage->BufferLength = _StubMsg.BufferLength;\n");
            fprintf(server, "\n");
            print_server("_Status = I_RpcGetBuffer(_pRpcMessage);\n");
            print_server("if (_Status)\n");
            indent++;
            print_server("RpcRaiseException(_Status);\n");
            indent--;
            fprintf(server, "\n");
            print_server("_StubMsg.Buffer = (unsigned char __RPC_FAR *)_pRpcMessage->Buffer;\n");

            /* marshall the out arguments */
            marshall_out_arguments(func, &type_offset);
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

        /* update type_offset */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                if (var->ptr_level == 0)
                {
                    if ((var->type->type == RPC_FC_RP) &&
                        (var->type->ref->ref->type == RPC_FC_STRUCT))
                    {
                        var_t *field = var->type->ref->ref->fields;
                        int tsize = 9;

                        while (NEXT_LINK(field)) field = NEXT_LINK(field);
                        while (field)
                        {
                            tsize++;
                            field = PREV_LINK(field);
                        }
                        if (tsize % 2)
                            tsize++;

                        type_offset += tsize;
                    }
                }
                else if (var->ptr_level == 1)
                {
                    type_offset += 4;
                }
                var = PREV_LINK(var);
            }
        }

        /* update proc_offset */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                proc_offset += 2; /* FIXME */
                var = PREV_LINK(var);
            }
        }
        proc_offset += 2;  /* FIXME */

        func = PREV_LINK(func);
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
    print_server("{NULL},\n");
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
    if (old_names)
        print_server("RPC_IF_HANDLE %s_ServerIfHandle = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                     iface->name, iface->name);
    else
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

static int get_type_format_string_size(type_t *iface)
{
    int size = 3;
    func_t *func;
    var_t *var;

    /* determine the proc format string size */
    func = iface->funcs;
    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* argument list size */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                if (var->ptr_level == 0)
                {
                    if (var->type->type == RPC_FC_RP)
                    {
                        var_t *field = var->type->ref->ref->fields;
                        int tsize = 9;

                        while (NEXT_LINK(field)) field = NEXT_LINK(field);
                        while (field)
                        {
                            tsize ++;
                            field = PREV_LINK(field);
                        }
                        if (tsize % 2)
                            tsize++;
                        size += tsize;
                    }
                }
                else if (var->ptr_level == 1)
                {
                    if (is_base_type(var->type))
                        size += 4;
                }

                var = PREV_LINK(var);
            }
        }

        func = PREV_LINK(func);
    }

    return size;
}


static int get_proc_format_string_size(type_t *iface)
{
    int size = 1;
    func_t *func;
    var_t *var;

    /* determine the proc format string size */
    func = iface->funcs;
    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* argument list size */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                switch (var->ptr_level)
                {
                case 0:
                    if (is_base_type(var->type))
                        size += 2;
                    else if (var->type->type == RPC_FC_RP)
                        size += 4;
                    break;

                case 1:
                    if (is_base_type(var->type))
                        size += 4;
                    break;
                }

                var = PREV_LINK(var);
            }
        }

        /* return value size */
        size += 2;
        func = PREV_LINK(func);
    }

    return size;
}


static void write_formatstringsdecl(type_t *iface)
{
    print_server("#define TYPE_FORMAT_STRING_SIZE %d\n",
                 get_type_format_string_size(iface));

    print_server("#define PROC_FORMAT_STRING_SIZE %d\n",
                 get_proc_format_string_size(iface));

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
    ifref_t *iface = ifaces;

    if (!do_server)
        return;
    if (!iface)
        return;
    END_OF_LIST(iface);

    init_server();
    if (!server)
        return;

    while (iface)
    {
        fprintf(server, "/*****************************************************************************\n");
        fprintf(server, " * %s interface\n", iface->iface->name);
        fprintf(server, " */\n");
        fprintf(server, "\n");

        write_formatstringsdecl(iface->iface);
        write_serverinterfacedecl(iface->iface);
        write_stubdescdecl(iface->iface);

        write_function_stubs(iface->iface);

        write_stubdescriptor(iface->iface);
        write_dispatchtable(iface->iface);

        print_server("#if !defined(__RPC_WIN32__)\n");
        print_server("#error  Invalid build platform for this stub.\n");
        print_server("#endif\n");
        fprintf(server, "\n");

        write_procformatstring(iface->iface);
        write_typeformatstring(iface->iface);

        fprintf(server, "\n");

        iface = PREV_LINK(iface);
    }

    fclose(server);
}
