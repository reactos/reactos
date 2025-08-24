/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "expr.h"
#include "typetree.h"
#include "typelib.h"

static int indentation = 0;
static int is_object_interface = 0;
user_type_list_t user_type_list = LIST_INIT(user_type_list);
context_handle_list_t context_handle_list = LIST_INIT(context_handle_list);
generic_handle_list_t generic_handle_list = LIST_INIT(generic_handle_list);

static void write_type_v(FILE *f, const decl_spec_t *t, int is_field, bool define, const char *name, enum name_type name_type);

static void write_apicontract_guard_start(FILE *header, const expr_t *expr);
static void write_apicontract_guard_end(FILE *header, const expr_t *expr);

static void write_widl_using_macros(FILE *header, type_t *iface);

static void indent(FILE *h, int delta)
{
  int c;
  if (delta < 0) indentation += delta;
  for (c=0; c<indentation; c++) fprintf(h, "    ");
  if (delta > 0) indentation += delta;
}

static void write_line(FILE *f, int delta, const char *fmt, ...)
{
    va_list ap;
    indent(f, delta);
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fprintf(f, "\n");
}

static char *format_parameterized_type_args(const type_t *type, const char *prefix, const char *suffix)
{
    typeref_list_t *params;
    typeref_t *ref;
    size_t len = 0, pos = 0;
    char *buf = NULL;

    params = type->details.parameterized.params;
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        assert(ref->type->type_type != TYPE_POINTER);
        pos += strappend(&buf, &len, pos, "%s%s%s", prefix, ref->type->name, suffix);
        if (list_next(params, &ref->entry)) pos += strappend(&buf, &len, pos, ", ");
    }

    if (!buf) return xstrdup("");
    return buf;
}

static void write_guid(FILE *f, const char *guid_prefix, const char *name, const struct uuid *uuid)
{
  if (!uuid) return;
  fprintf(f, "DEFINE_GUID(%s_%s, 0x%08x, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x);\n",
        guid_prefix, name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0],
        uuid->Data4[1], uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5],
        uuid->Data4[6], uuid->Data4[7]);
}

static void write_uuid_decl(FILE *f, type_t *type, const struct uuid *uuid)
{
  fprintf(f, "#ifdef __CRT_UUID_DECL\n");
  fprintf(f, "__CRT_UUID_DECL(%s, 0x%08x, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x)\n",
        type->c_name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
        uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
        uuid->Data4[7]);
  fprintf(f, "#endif\n");
}

static const char *uuid_string(const struct uuid *uuid)
{
  static char buf[37];

  snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1], uuid->Data4[2],
        uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6], uuid->Data4[7]);

  return buf;
}

static void write_namespace_start(FILE *header, struct namespace *namespace)
{
    if(is_global_namespace(namespace)) {
        if(use_abi_namespace)
            write_line(header, 1, "namespace ABI {");
        return;
    }

    write_namespace_start(header, namespace->parent);
    write_line(header, 1, "namespace %s {", namespace->name);
}

static void write_namespace_end(FILE *header, struct namespace *namespace)
{
    if(is_global_namespace(namespace)) {
        if(use_abi_namespace)
            write_line(header, -1, "}");
        return;
    }

    write_line(header, -1, "}");
    write_namespace_end(header, namespace->parent);
}

const char *get_name(const var_t *v)
{
    static char *buffer;
    free( buffer );
    if (is_attr( v->attrs, ATTR_EVENTADD ))
        return buffer = strmake( "add_%s", v->name );
    if (is_attr( v->attrs, ATTR_EVENTREMOVE ))
        return buffer = strmake( "remove_%s", v->name );
    if (is_attr( v->attrs, ATTR_PROPGET ))
        return buffer = strmake( "get_%s", v->name );
    if (is_attr( v->attrs, ATTR_PROPPUT ))
        return buffer = strmake( "put_%s", v->name );
    if (is_attr( v->attrs, ATTR_PROPPUTREF ))
        return buffer = strmake( "putref_%s", v->name );
    buffer = NULL;
    return v->name;
}

static void write_fields(FILE *h, var_list_t *fields, enum name_type name_type)
{
    unsigned nameless_struct_cnt = 0, nameless_struct_i = 0, nameless_union_cnt = 0, nameless_union_i = 0;
    const char *name;
    char buf[32];
    var_t *v;

    if (!fields) return;

    LIST_FOR_EACH_ENTRY( v, fields, var_t, entry ) {
        if (!v->declspec.type) continue;

        switch(type_get_type_detect_alias(v->declspec.type)) {
        case TYPE_STRUCT:
        case TYPE_ENCAPSULATED_UNION:
            nameless_struct_cnt++;
            break;
        case TYPE_UNION:
            nameless_union_cnt++;
            break;
        default:
            ;
        }
    }

    LIST_FOR_EACH_ENTRY( v, fields, var_t, entry ) {
        expr_t *contract = get_attrp(v->attrs, ATTR_CONTRACT);
        if (!v->declspec.type) continue;
        if (contract) write_apicontract_guard_start(h, contract);

        indent(h, 0);
        name = v->name;

        switch(type_get_type_detect_alias(v->declspec.type)) {
        case TYPE_STRUCT:
        case TYPE_ENCAPSULATED_UNION:
            if(!v->name) {
                fprintf(h, "__C89_NAMELESS ");
                if(nameless_struct_cnt == 1) {
                    name = "__C89_NAMELESSSTRUCTNAME";
                }else if(nameless_struct_i < 5 /* # of supporting macros */) {
                    snprintf(buf, sizeof(buf), "__C89_NAMELESSSTRUCTNAME%d", ++nameless_struct_i);
                    name = buf;
                }
            }
            break;
        case TYPE_UNION:
            if(!v->name) {
                fprintf(h, "__C89_NAMELESS ");
                if(nameless_union_cnt == 1) {
                    name = "__C89_NAMELESSUNIONNAME";
                }else if(nameless_union_i < 8 /* # of supporting macros */ ) {
                    snprintf(buf, sizeof(buf), "__C89_NAMELESSUNIONNAME%d", ++nameless_union_i);
                    name = buf;
                }
            }
            break;
        default:
            ;
        }
        write_type_v(h, &v->declspec, TRUE, v->is_defined, name, name_type);
        fprintf(h, ";\n");
        if (contract) write_apicontract_guard_end(h, contract);
    }
}

static void write_enums(FILE *h, var_list_t *enums, const char *enum_name)
{
  var_t *v;
  if (!enums) return;
  LIST_FOR_EACH_ENTRY( v, enums, var_t, entry )
  {
    expr_t *contract = get_attrp(v->attrs, ATTR_CONTRACT);
    if (contract) write_apicontract_guard_start(h, contract);
    if (v->name) {
      indent(h, 0);
      if(!enum_name)
          fprintf(h, "%s", get_name(v));
      else
          fprintf(h, "%s_%s", enum_name, get_name(v));
      if (v->eval) {
        fprintf(h, " = ");
        write_expr(h, v->eval, 0, 1, NULL, NULL, "");
      }
    }
    if (list_next( enums, &v->entry )) fprintf(h, ",\n");
    else fprintf(h, "\n");
    if (contract) write_apicontract_guard_end(h, contract);
  }
}

int needs_space_after(type_t *t)
{
  return (type_is_alias(t) ||
          (!is_ptr(t) && (!is_array(t) || !type_array_is_decl_as_ptr(t) || t->name)));
}

static int decl_needs_parens(const type_t *t)
{
    if (type_is_alias(t))
        return FALSE;
    if (is_array(t) && !type_array_is_decl_as_ptr(t))
        return TRUE;
    return is_func(t);
}

static void write_pointer_left(FILE *h, type_t *ref)
{
    if (needs_space_after(ref))
        fprintf(h, " ");
    if (decl_needs_parens(ref))
        fprintf(h, "(");
    if (type_get_type_detect_alias(ref) == TYPE_FUNCTION)
    {
        const char *callconv = get_attrp(ref->attrs, ATTR_CALLCONV);
        if (!callconv && is_object_interface) callconv = "STDMETHODCALLTYPE";
        if (callconv) fprintf(h, "%s ", callconv);
    }
    fprintf(h, "*");
}

void write_type_left(FILE *h, const decl_spec_t *ds, enum name_type name_type, bool define, int write_callconv)
{
  type_t *t = ds->type;
  const char *decl_name, *name;
  char *args;

  if (!h) return;

  decl_name = type_get_decl_name(t, name_type);
  name = type_get_name(t, name_type);

  if (ds->func_specifier & FUNCTION_SPECIFIER_INLINE)
    fprintf(h, "inline ");

  if ((ds->qualifier & TYPE_QUALIFIER_CONST) && (type_is_alias(t) || !is_ptr(t)))
    fprintf(h, "const ");

  if (type_is_alias(t)) fprintf(h, "%s", name);
  else {
    switch (type_get_type_detect_alias(t)) {
      case TYPE_ENUM:
        if (!define) fprintf(h, "enum %s", decl_name ? decl_name : "");
        else if (!t->written) {
          assert(t->defined);
          if (decl_name) fprintf(h, "enum %s {\n", decl_name);
          else fprintf(h, "enum {\n");
          t->written = TRUE;
          indentation++;
          write_enums(h, type_enum_get_values(t), is_global_namespace(t->namespace) ? NULL : t->name);
          indent(h, -1);
          fprintf(h, "}");
        }
        else if (winrt_mode && name_type == NAME_DEFAULT && name) fprintf(h, "%s", name);
        else fprintf(h, "enum %s", name ? name : "");
        break;
      case TYPE_STRUCT:
      case TYPE_ENCAPSULATED_UNION:
        if (!define) fprintf(h, "struct %s", decl_name ? decl_name : "");
        else if (!t->written) {
          assert(t->defined);
          if (decl_name) fprintf(h, "struct %s {\n", decl_name);
          else fprintf(h, "struct {\n");
          t->written = TRUE;
          indentation++;
          if (type_get_type(t) != TYPE_STRUCT)
            write_fields(h, type_encapsulated_union_get_fields(t), name_type);
          else
            write_fields(h, type_struct_get_fields(t), name_type);
          indent(h, -1);
          fprintf(h, "}");
        }
        else if (winrt_mode && name_type == NAME_DEFAULT && name) fprintf(h, "%s", name);
        else fprintf(h, "struct %s", name ? name : "");
        break;
      case TYPE_UNION:
        if (!define) fprintf(h, "union %s", decl_name ? decl_name : "");
        else if (!t->written) {
          assert(t->defined);
          if (decl_name) fprintf(h, "union %s {\n", decl_name);
          else fprintf(h, "union {\n");
          t->written = TRUE;
          indentation++;
          write_fields(h, type_union_get_cases(t), name_type);
          indent(h, -1);
          fprintf(h, "}");
        }
        else if (winrt_mode && name_type == NAME_DEFAULT && name) fprintf(h, "%s", name);
        else fprintf(h, "union %s", name ? name : "");
        break;
      case TYPE_POINTER:
      {
        write_type_left(h, type_pointer_get_ref(t), name_type, define, FALSE);
        write_pointer_left(h, type_pointer_get_ref_type(t));
        if (ds->qualifier & TYPE_QUALIFIER_CONST) fprintf(h, "const ");
        break;
      }
      case TYPE_ARRAY:
        if (t->name && type_array_is_decl_as_ptr(t))
          fprintf(h, "%s", t->name);
        else
        {
          write_type_left(h, type_array_get_element(t), name_type, define, !type_array_is_decl_as_ptr(t));
          if (type_array_is_decl_as_ptr(t))
            write_pointer_left(h, type_array_get_element_type(t));
        }
        break;
      case TYPE_FUNCTION:
      {
        write_type_left(h, type_function_get_ret(t), name_type, define, TRUE);

        /* A pointer to a function has to write the calling convention inside
         * the parentheses. There's no way to handle that here, so we have to
         * use an extra parameter to tell us whether to write the calling
         * convention or not. */
        if (write_callconv)
        {
            const char *callconv = get_attrp(t->attrs, ATTR_CALLCONV);
            if (!callconv && is_object_interface) callconv = "STDMETHODCALLTYPE";
            if (callconv) fprintf(h, " %s ", callconv);
        }
        break;
      }
      case TYPE_BASIC:
        if (type_basic_get_type(t) != TYPE_BASIC_INT32 &&
            type_basic_get_type(t) != TYPE_BASIC_INT64 &&
            type_basic_get_type(t) != TYPE_BASIC_LONG &&
            type_basic_get_type(t) != TYPE_BASIC_HYPER)
        {
          if (type_basic_get_sign(t) < 0) fprintf(h, "signed ");
          else if (type_basic_get_sign(t) > 0) fprintf(h, "unsigned ");
        }
        switch (type_basic_get_type(t))
        {
        case TYPE_BASIC_INT8: fprintf(h, "small"); break;
        case TYPE_BASIC_INT16: fprintf(h, "short"); break;
        case TYPE_BASIC_INT: fprintf(h, "int"); break;
        case TYPE_BASIC_INT3264: fprintf(h, "__int3264"); break;
        case TYPE_BASIC_BYTE: fprintf(h, "byte"); break;
        case TYPE_BASIC_CHAR: fprintf(h, "char"); break;
        case TYPE_BASIC_WCHAR: fprintf(h, "wchar_t"); break;
        case TYPE_BASIC_FLOAT: fprintf(h, "float"); break;
        case TYPE_BASIC_DOUBLE: fprintf(h, "double"); break;
        case TYPE_BASIC_ERROR_STATUS_T: fprintf(h, "error_status_t"); break;
        case TYPE_BASIC_HANDLE: fprintf(h, "handle_t"); break;
        case TYPE_BASIC_INT32:
          if (type_basic_get_sign(t) > 0)
            fprintf(h, "UINT32");
          else
            fprintf(h, "INT32");
          break;
        case TYPE_BASIC_LONG:
          if (type_basic_get_sign(t) > 0)
            fprintf(h, "ULONG");
          else
            fprintf(h, "LONG");
          break;
        case TYPE_BASIC_INT64:
          if (type_basic_get_sign(t) > 0)
            fprintf(h, "UINT64");
          else
            fprintf(h, "INT64");
          break;
        case TYPE_BASIC_HYPER:
          if (type_basic_get_sign(t) > 0)
            fprintf(h, "MIDL_uhyper");
          else
            fprintf(h, "hyper");
          break;
        }
        break;
      case TYPE_INTERFACE:
      case TYPE_MODULE:
      case TYPE_COCLASS:
        fprintf(h, "%s", type_get_name(t, name_type));
        break;
      case TYPE_RUNTIMECLASS:
        fprintf(h, "%s", type_get_name(type_runtimeclass_get_default_iface(t, TRUE), name_type));
        break;
      case TYPE_DELEGATE:
        fprintf(h, "%s", type_get_name(type_delegate_get_iface(t), name_type));
        break;
      case TYPE_VOID:
        fprintf(h, "void");
        break;
      case TYPE_BITFIELD:
      {
        const decl_spec_t ds = {.type = type_bitfield_get_field(t)};
        write_type_left(h, &ds, name_type, define, TRUE);
        break;
      }
      case TYPE_ALIAS:
        /* handled elsewhere */
        assert(0);
        break;
      case TYPE_PARAMETERIZED_TYPE:
      {
        type_t *iface = type_parameterized_type_get_real_type(t);
        if (type_get_type(iface) == TYPE_DELEGATE) iface = type_delegate_get_iface(iface);
        args = format_parameterized_type_args(t, "", "_logical");
        fprintf(h, "%s<%s>", iface->name, args);
        free(args);
        break;
      }
      case TYPE_PARAMETER:
        fprintf(h, "%s_abi", t->name);
        break;
      case TYPE_APICONTRACT:
        /* shouldn't be here */
        assert(0);
        break;
    }
  }
}

void write_type_right(FILE *h, type_t *t, int is_field)
{
  if (!h) return;
  if (type_is_alias(t)) return;

  switch (type_get_type(t))
  {
  case TYPE_ARRAY:
  {
    type_t *elem = type_array_get_element_type(t);
    if (type_array_is_decl_as_ptr(t))
    {
      if (decl_needs_parens(elem))
        fprintf(h, ")");
    }
    else
    {
      if (is_conformant_array(t))
        fprintf(h, "[%s]", is_field ? "1" : "");
      else
        fprintf(h, "[%u]", type_array_get_dim(t));
    }
    write_type_right(h, elem, FALSE);
    break;
  }
  case TYPE_FUNCTION:
  {
    const var_list_t *args = type_function_get_args(t);
    fputc('(', h);
    if (args) write_args(h, args, NULL, 0, FALSE, NAME_DEFAULT);
    else
      fprintf(h, "void");
    fputc(')', h);
    write_type_right(h, type_function_get_rettype(t), FALSE);
    break;
  }
  case TYPE_POINTER:
  {
    type_t *ref = type_pointer_get_ref_type(t);
    if (decl_needs_parens(ref))
      fprintf(h, ")");
    write_type_right(h, ref, FALSE);
    break;
  }
  case TYPE_BITFIELD:
    fprintf(h, " : %u", type_bitfield_get_bits(t)->cval);
    break;
  case TYPE_VOID:
  case TYPE_BASIC:
  case TYPE_ENUM:
  case TYPE_STRUCT:
  case TYPE_ENCAPSULATED_UNION:
  case TYPE_UNION:
  case TYPE_ALIAS:
  case TYPE_MODULE:
  case TYPE_COCLASS:
  case TYPE_INTERFACE:
  case TYPE_RUNTIMECLASS:
  case TYPE_DELEGATE:
  case TYPE_PARAMETERIZED_TYPE:
  case TYPE_PARAMETER:
    break;
  case TYPE_APICONTRACT:
    /* not supposed to be here */
    assert(0);
    break;
  }
}

static void write_type_v(FILE *h, const decl_spec_t *ds, int is_field, bool define, const char *name, enum name_type name_type)
{
    type_t *t = ds->type;

    if (!h) return;

    if (t) write_type_left(h, ds, name_type, define, TRUE);

    if (name) fprintf(h, "%s%s", !t || needs_space_after(t) ? " " : "", name );

    if (t)
        write_type_right(h, t, is_field);
}

static void write_type_definition(FILE *f, type_t *t, bool define)
{
    int in_namespace = t->namespace && !is_global_namespace(t->namespace);
    int save_written = t->written;
    decl_spec_t ds = {.type = t};
    expr_t *contract = get_attrp(t->attrs, ATTR_CONTRACT);

    if (contract) write_apicontract_guard_start(f, contract);
    if(in_namespace) {
        fprintf(f, "#ifdef __cplusplus\n");
        fprintf(f, "} /* extern \"C\" */\n");
        write_namespace_start(f, t->namespace);
    }
    indent(f, 0);
    write_type_left(f, &ds, NAME_DEFAULT, define, TRUE);
    fprintf(f, ";\n");
    if(in_namespace) {
        t->written = save_written;
        write_namespace_end(f, t->namespace);
        fprintf(f, "extern \"C\" {\n");
        fprintf(f, "#else\n");
        write_type_left(f, &ds, NAME_C, define, TRUE);
        fprintf(f, ";\n");
        if (winrt_mode) write_widl_using_macros(f, t);
        fprintf(f, "#endif\n\n");
    }
    if (contract) write_apicontract_guard_end(f, contract);
}

void write_type_decl(FILE *f, const decl_spec_t *t, const char *name)
{
    write_type_v(f, t, FALSE, false, name, NAME_DEFAULT);
}

void write_type_decl_left(FILE *f, const decl_spec_t *ds)
{
  write_type_left(f, ds, NAME_DEFAULT, false, TRUE);
}

static int user_type_registered(const char *name)
{
  user_type_t *ut;
  LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
    if (!strcmp(name, ut->name))
      return 1;
  return 0;
}

static int context_handle_registered(const char *name)
{
  context_handle_t *ch;
  LIST_FOR_EACH_ENTRY(ch, &context_handle_list, context_handle_t, entry)
    if (!strcmp(name, ch->name))
      return 1;
  return 0;
}

static int generic_handle_registered(const char *name)
{
  generic_handle_t *gh;
  LIST_FOR_EACH_ENTRY(gh, &generic_handle_list, generic_handle_t, entry)
    if (!strcmp(name, gh->name))
      return 1;
  return 0;
}

unsigned int get_context_handle_offset( const type_t *type )
{
    context_handle_t *ch;
    unsigned int index = 0;

    while (!is_attr( type->attrs, ATTR_CONTEXTHANDLE ))
    {
        if (type_is_alias( type )) type = type_alias_get_aliasee_type( type );
        else if (is_ptr( type )) type = type_pointer_get_ref_type( type );
        else error( "internal error: %s is not a context handle\n", type->name );
    }
    LIST_FOR_EACH_ENTRY( ch, &context_handle_list, context_handle_t, entry )
    {
        if (!strcmp( type->name, ch->name )) return index;
        index++;
    }
    error( "internal error: %s is not registered as a context handle\n", type->name );
    return index;
}

unsigned int get_generic_handle_offset( const type_t *type )
{
    generic_handle_t *gh;
    unsigned int index = 0;

    while (!is_attr( type->attrs, ATTR_HANDLE ))
    {
        if (type_is_alias( type )) type = type_alias_get_aliasee_type( type );
        else if (is_ptr( type )) type = type_pointer_get_ref_type( type );
        else error( "internal error: %s is not a generic handle\n", type->name );
    }
    LIST_FOR_EACH_ENTRY( gh, &generic_handle_list, generic_handle_t, entry )
    {
        if (!strcmp( type->name, gh->name )) return index;
        index++;
    }
    error( "internal error: %s is not registered as a generic handle\n", type->name );
    return index;
}

/* check for types which require additional prototypes to be generated in the
 * header */
void check_for_additional_prototype_types(type_t *type)
{
  if (!type) return;
  for (;;) {
    const char *name = type->name;
    if (type->user_types_registered) break;
    type->user_types_registered = 1;
    if (is_attr(type->attrs, ATTR_CONTEXTHANDLE)) {
      if (!context_handle_registered(name))
      {
        context_handle_t *ch = xmalloc(sizeof(*ch));
        ch->name = xstrdup(name);
        list_add_tail(&context_handle_list, &ch->entry);
      }
      /* don't carry on parsing fields within this type */
      break;
    }
    if ((type_get_type(type) != TYPE_BASIC ||
         type_basic_get_type(type) != TYPE_BASIC_HANDLE) &&
        is_attr(type->attrs, ATTR_HANDLE)) {
      if (!generic_handle_registered(name))
      {
        generic_handle_t *gh = xmalloc(sizeof(*gh));
        gh->name = xstrdup(name);
        list_add_tail(&generic_handle_list, &gh->entry);
      }
      /* don't carry on parsing fields within this type */
      break;
    }
    if (is_attr(type->attrs, ATTR_WIREMARSHAL)) {
      if (!user_type_registered(name))
      {
        user_type_t *ut = xmalloc(sizeof *ut);
        ut->name = xstrdup(name);
        list_add_tail(&user_type_list, &ut->entry);
      }
      /* don't carry on parsing fields within this type as we are already
       * using a wire marshaled type */
      break;
    }
    else if (type_is_complete(type))
    {
      var_list_t *vars;
      const var_t *v;
      switch (type_get_type_detect_alias(type))
      {
      case TYPE_ENUM:
        vars = type_enum_get_values(type);
        break;
      case TYPE_STRUCT:
        vars = type_struct_get_fields(type);
        break;
      case TYPE_UNION:
        vars = type_union_get_cases(type);
        break;
      default:
        vars = NULL;
        break;
      }
      if (vars) LIST_FOR_EACH_ENTRY( v, vars, const var_t, entry )
        check_for_additional_prototype_types(v->declspec.type);
    }

    if (type_is_alias(type))
      type = type_alias_get_aliasee_type(type);
    else if (is_ptr(type))
      type = type_pointer_get_ref_type(type);
    else if (is_array(type))
      type = type_array_get_element_type(type);
    else
      break;
  }
}

static int write_serialize_function_decl(FILE *header, const type_t *type)
{
    write_serialize_functions(header, type, NULL);
    return 1;
}

static int serializable_exists(FILE *header, const type_t *type)
{
    return 0;
}

static int for_each_serializable(const statement_list_t *stmts, FILE *header,
                                 int (*proc)(FILE*, const type_t*))
{
    statement_t *stmt, *iface_stmt;
    statement_list_t *iface_stmts;
    typeref_t *ref;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, statement_t, entry )
    {
        if (stmt->type != STMT_TYPE || type_get_type(stmt->u.type) != TYPE_INTERFACE)
            continue;

        iface_stmts = type_iface_get_stmts(stmt->u.type);
        if (iface_stmts) LIST_FOR_EACH_ENTRY( iface_stmt, iface_stmts, statement_t, entry )
        {
            if (iface_stmt->type != STMT_TYPEDEF) continue;
            if (iface_stmt->u.type_list) LIST_FOR_EACH_ENTRY(ref, iface_stmt->u.type_list, typeref_t, entry)
            {
                if (!is_attr(ref->type->attrs, ATTR_ENCODE)
                    && !is_attr(ref->type->attrs, ATTR_DECODE))
                    continue;
                if (!proc(header, ref->type))
                    return 0;
            }
        }
    }

    return 1;
}

static void write_user_types(FILE *header)
{
  user_type_t *ut;
  LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
  {
    const char *name = ut->name;
    fprintf(header, "ULONG           __RPC_USER %s_UserSize     (ULONG *, ULONG, %s *);\n", name, name);
    fprintf(header, "unsigned char * __RPC_USER %s_UserMarshal  (ULONG *, unsigned char *, %s *);\n", name, name);
    fprintf(header, "unsigned char * __RPC_USER %s_UserUnmarshal(ULONG *, unsigned char *, %s *);\n", name, name);
    fprintf(header, "void            __RPC_USER %s_UserFree     (ULONG *, %s *);\n", name, name);
  }
}

static void write_context_handle_rundowns(FILE *header)
{
  context_handle_t *ch;
  LIST_FOR_EACH_ENTRY(ch, &context_handle_list, context_handle_t, entry)
  {
    const char *name = ch->name;
    fprintf(header, "void __RPC_USER %s_rundown(%s);\n", name, name);
  }
}

static void write_generic_handle_routines(FILE *header)
{
  generic_handle_t *gh;
  LIST_FOR_EACH_ENTRY(gh, &generic_handle_list, generic_handle_t, entry)
  {
    const char *name = gh->name;
    fprintf(header, "handle_t __RPC_USER %s_bind(%s);\n", name, name);
    fprintf(header, "void __RPC_USER %s_unbind(%s, handle_t);\n", name, name);
  }
}

static void write_typedef(FILE *header, type_t *type, bool define)
{
    type_t *t = type_alias_get_aliasee_type(type), *root = type_pointer_get_root_type(t);
    if (winrt_mode && root->namespace && !is_global_namespace(root->namespace))
    {
        fprintf(header, "#ifndef __cplusplus\n");
        fprintf(header, "typedef ");
        write_type_v(header, type_alias_get_aliasee(type), FALSE, define, type->c_name, NAME_C);
        fprintf(header, ";\n");
        if (type_get_type_detect_alias(t) != TYPE_ENUM)
        {
            fprintf(header, "#else /* __cplusplus */\n");
            if (t->namespace && !is_global_namespace(t->namespace)) write_namespace_start(header, t->namespace);
            indent(header, 0);
            fprintf(header, "typedef ");
            write_type_v(header, type_alias_get_aliasee(type), FALSE, false, type->name, NAME_DEFAULT);
            fprintf(header, ";\n");
            if (t->namespace && !is_global_namespace(t->namespace)) write_namespace_end(header, t->namespace);
        }
        fprintf(header, "#endif /* __cplusplus */\n\n");
    }
    else
    {
        fprintf(header, "typedef ");
        write_type_v(header, type_alias_get_aliasee(type), FALSE, define, type->name, NAME_DEFAULT);
        fprintf(header, ";\n");
    }
}

int is_const_decl(const var_t *var)
{
  const decl_spec_t *t;
  /* strangely, MIDL accepts a const attribute on any pointer in the
  * declaration to mean that data isn't being instantiated. this appears
  * to be a bug, but there is no benefit to being incompatible with MIDL,
  * so we'll do the same thing */
  for (t = &var->declspec; ; )
  {
    if (t->qualifier & TYPE_QUALIFIER_CONST)
      return TRUE;
    else if (is_ptr(t->type))
      t = type_pointer_get_ref(t->type);
    else break;
  }
  return FALSE;
}

static void write_declaration(FILE *header, const var_t *v)
{
  if (is_const_decl(v) && v->eval)
  {
    fprintf(header, "#define %s (", v->name);
    write_expr(header, v->eval, 0, 1, NULL, NULL, "");
    fprintf(header, ")\n\n");
  }
  else
  {
    switch (v->declspec.stgclass)
    {
      case STG_NONE:
      case STG_REGISTER: /* ignored */
        break;
      case STG_STATIC:
        fprintf(header, "static ");
        break;
      case STG_EXTERN:
        fprintf(header, "extern ");
        break;
    }
    write_type_v(header, &v->declspec, FALSE, v->is_defined, v->name, NAME_DEFAULT);
    fprintf(header, ";\n\n");
  }
}

static void write_library(FILE *header, const typelib_t *typelib)
{
  const struct uuid *uuid = get_attrp(typelib->attrs, ATTR_UUID);
  fprintf(header, "\n");
  write_guid(header, "LIBID", typelib->name, uuid);
  fprintf(header, "\n");
}


const type_t* get_explicit_generic_handle_type(const var_t* var)
{
    const type_t *t;
    for (t = var->declspec.type;
         is_ptr(t) || type_is_alias(t);
         t = type_is_alias(t) ? type_alias_get_aliasee_type(t) : type_pointer_get_ref_type(t))
        if ((type_get_type_detect_alias(t) != TYPE_BASIC || type_basic_get_type(t) != TYPE_BASIC_HANDLE) &&
            is_attr(t->attrs, ATTR_HANDLE))
            return t;
    return NULL;
}

const var_t *get_func_handle_var( const type_t *iface, const var_t *func,
                                  unsigned char *explicit_fc, unsigned char *implicit_fc )
{
    const var_t *var;
    const var_list_t *args = type_function_get_args( func->declspec.type );

    *explicit_fc = *implicit_fc = 0;
    if (args) LIST_FOR_EACH_ENTRY( var, args, const var_t, entry )
    {
        if (!is_attr( var->attrs, ATTR_IN ) && is_attr( var->attrs, ATTR_OUT )) continue;
        if (type_get_type( var->declspec.type ) == TYPE_BASIC && type_basic_get_type( var->declspec.type ) == TYPE_BASIC_HANDLE)
        {
            *explicit_fc = FC_BIND_PRIMITIVE;
            return var;
        }
        if (get_explicit_generic_handle_type( var ))
        {
            *explicit_fc = FC_BIND_GENERIC;
            return var;
        }
        if (is_context_handle( var->declspec.type ))
        {
            *explicit_fc = FC_BIND_CONTEXT;
            return var;
        }
    }

    if ((var = get_attrp( iface->attrs, ATTR_IMPLICIT_HANDLE )))
    {
        if (type_get_type( var->declspec.type ) == TYPE_BASIC &&
            type_basic_get_type( var->declspec.type ) == TYPE_BASIC_HANDLE)
            *implicit_fc = FC_BIND_PRIMITIVE;
        else
            *implicit_fc = FC_BIND_GENERIC;
        return var;
    }

    *implicit_fc = FC_AUTO_HANDLE;
    return NULL;
}

int has_out_arg_or_return(const var_t *func)
{
    const var_t *var;

    if (!is_void(type_function_get_rettype(func->declspec.type)))
        return 1;

    if (!type_function_get_args(func->declspec.type))
        return 0;

    LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        if (is_attr(var->attrs, ATTR_OUT))
            return 1;

    return 0;
}


/********** INTERFACES **********/

int is_object(const type_t *iface)
{
    const attr_t *attr;
    if (type_is_defined(iface) && (type_get_type(iface) == TYPE_DELEGATE || type_iface_get_inherit(iface)))
        return 1;
    if (iface->attrs) LIST_FOR_EACH_ENTRY( attr, iface->attrs, const attr_t, entry )
        if (attr->type == ATTR_OBJECT || attr->type == ATTR_ODL) return 1;
    return 0;
}

int is_local(const attr_list_t *a)
{
  return is_attr(a, ATTR_LOCAL);
}

const var_t *is_callas(const attr_list_t *a)
{
  return get_attrp(a, ATTR_CALLAS);
}

static int is_inherited_method(const type_t *iface, const var_t *func)
{
  while ((iface = type_iface_get_inherit(iface)))
  {
    const statement_t *stmt;
    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
    {
      const var_t *funccmp = stmt->u.var;

      if (!is_callas(func->attrs))
      {
         char inherit_name[256];
         /* compare full name including property prefix */
         strcpy(inherit_name, get_name(funccmp));
         if (!strcmp(inherit_name, get_name(func))) return 1;
      }
    }
  }

  return 0;
}

static int is_override_method(const type_t *iface, const type_t *child, const var_t *func)
{
  if (iface == child)
    return 0;

  do
  {
    const statement_t *stmt;
    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(child))
    {
      const var_t *funccmp = stmt->u.var;

      if (!is_callas(func->attrs))
      {
         char inherit_name[256];
         /* compare full name including property prefix */
         strcpy(inherit_name, get_name(funccmp));
         if (!strcmp(inherit_name, get_name(func))) return 1;
      }
    }
  }
  while ((child = type_iface_get_inherit(child)) && child != iface);

  return 0;
}

static int is_aggregate_return(const var_t *func)
{
  enum type_type type = type_get_type(type_function_get_rettype(func->declspec.type));
  return type == TYPE_STRUCT || type == TYPE_UNION ||
         type == TYPE_COCLASS || type == TYPE_INTERFACE ||
         type == TYPE_RUNTIMECLASS;
}

static char *get_vtbl_entry_name(const type_t *iface, const var_t *func)
{
  static char buff[255];
  if (is_inherited_method(iface, func))
    snprintf(buff, sizeof(buff), "%s_%s", iface->name, get_name(func));
  else
    snprintf(buff, sizeof(buff), "%s", get_name(func));
  return buff;
}

static void write_method_macro(FILE *header, const type_t *iface, const type_t *child, const char *name)
{
  const statement_t *stmt;
  int first_iface = 1;

  if (type_iface_get_inherit(iface))
    write_method_macro(header, type_iface_get_inherit(iface), child, name);

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;

    if (first_iface)
    {
      fprintf(header, "/*** %s methods ***/\n", iface->name);
      first_iface = 0;
    }

    if (is_override_method(iface, child, func))
      continue;

    if (!is_callas(func->attrs)) {
      const var_t *arg;

      fprintf(header, "#define %s_%s(This", name, get_name(func));
      if (type_function_get_args(func->declspec.type))
          LIST_FOR_EACH_ENTRY( arg, type_function_get_args(func->declspec.type), const var_t, entry )
              fprintf(header, ",%s", arg->name);
      fprintf(header, ") ");

      if (is_aggregate_return(func))
      {
        fprintf(header, "%s_%s_define_WIDL_C_INLINE_WRAPPERS_for_aggregate_return_support\n", name, get_name(func));
        continue;
      }

      fprintf(header, "(This)->lpVtbl->%s(This", get_vtbl_entry_name(iface, func));
      if (type_function_get_args(func->declspec.type))
          LIST_FOR_EACH_ENTRY( arg, type_function_get_args(func->declspec.type), const var_t, entry )
              fprintf(header, ",%s", arg->name);
      fprintf(header, ")\n");
    }
  }
}

void write_args(FILE *h, const var_list_t *args, const char *name, int method, int do_indent, enum name_type name_type)
{
  const var_t *arg;
  int count = 0;

  if (do_indent)
  {
      indentation++;
      indent(h, 0);
  }
  if (method == 1) {
    fprintf(h, "%s* This", name);
    count++;
  }
  if (args) LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry ) {
    if (count) {
        if (do_indent)
        {
            fprintf(h, ",\n");
            indent(h, 0);
        }
        else fprintf(h, ",");
    }
    /* In theory we should be writing the definition using write_type_v(..., arg->define),
     * but that causes redefinition in e.g. proxy files. In fact MIDL disallows
     * defining UDTs inside of an argument list. */
    write_type_v(h, &arg->declspec, FALSE, false, arg->name, name_type);
    if (method == 2) {
        const expr_t *expr = get_attrp(arg->attrs, ATTR_DEFAULTVALUE);
        if (expr) {
            const var_t *tail_arg;

            /* Output default value only if all following arguments also have default value. */
            LIST_FOR_EACH_ENTRY_REV( tail_arg, args, const var_t, entry ) {
                if(tail_arg == arg) {
                    expr_t bstr;

                    /* Fixup the expression type for a BSTR like midl does. */
                    if (get_type_vt(arg->declspec.type) == VT_BSTR && expr->type == EXPR_STRLIT)
                    {
                        bstr = *expr;
                        bstr.type = EXPR_WSTRLIT;
                        expr = &bstr;
                    }

                    fprintf(h, " = ");
                    write_expr( h, expr, 0, 1, NULL, NULL, "" );
                    break;
                }
                if(!get_attrp(tail_arg->attrs, ATTR_DEFAULTVALUE))
                    break;
            }
        }
    }
    count++;
  }
  if (do_indent) indentation--;
}

static void write_cpp_method_def(FILE *header, const type_t *iface)
{
  const statement_t *stmt;

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;
    if (!is_callas(func->attrs)) {
      const decl_spec_t *ret = type_function_get_ret(func->declspec.type);
      const char *callconv = get_attrp(func->declspec.type->attrs, ATTR_CALLCONV);
      const var_list_t *args = type_function_get_args(func->declspec.type);
      const var_t *arg;

      if (!callconv) callconv = "STDMETHODCALLTYPE";

      if (is_aggregate_return(func)) {
        fprintf(header, "#ifdef WIDL_EXPLICIT_AGGREGATE_RETURNS\n");

        indent(header, 0);
        fprintf(header, "virtual ");
        write_type_decl_left(header, ret);
        fprintf(header, "* %s %s(\n", callconv, get_name(func));
        ++indentation;
        indent(header, 0);
        write_type_decl_left(header, ret);
        fprintf(header, " *__ret");
        --indentation;
        if (args) {
          fprintf(header, ",\n");
          write_args(header, args, iface->name, 2, TRUE, NAME_DEFAULT);
        }
        fprintf(header, ") = 0;\n");

        indent(header, 0);
        write_type_decl_left(header, ret);
        fprintf(header, " %s %s(\n", callconv, get_name(func));
        write_args(header, args, iface->name, 2, TRUE, NAME_DEFAULT);
        fprintf(header, ")\n");
        indent(header, 0);
        fprintf(header, "{\n");
        ++indentation;
        indent(header, 0);
        write_type_decl_left(header, ret);
        fprintf(header, " __ret;\n");
        indent(header, 0);
        fprintf(header, "return *%s(&__ret", get_name(func));
        if (args)
            LIST_FOR_EACH_ENTRY(arg, args, const var_t, entry)
                fprintf(header, ", %s", arg->name);
        fprintf(header, ");\n");
        --indentation;
        indent(header, 0);
        fprintf(header, "}\n");

        fprintf(header, "#else\n");
      }

      indent(header, 0);
      fprintf(header, "virtual ");
      write_type_decl_left(header, ret);
      fprintf(header, " %s %s(\n", callconv, get_name(func));
      write_args(header, args, iface->name, 2, TRUE, NAME_DEFAULT);
      fprintf(header, ") = 0;\n");

      if (is_aggregate_return(func))
        fprintf(header, "#endif\n");
      fprintf(header, "\n");
    }
  }
}

static void write_inline_wrappers(FILE *header, const type_t *iface, const type_t *child, const char *name)
{
  const statement_t *stmt;
  int first_iface = 1;

  if (type_iface_get_inherit(iface))
    write_inline_wrappers(header, type_iface_get_inherit(iface), child, name);

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;

    if (first_iface)
    {
      fprintf(header, "/*** %s methods ***/\n", iface->name);
      first_iface = 0;
    }

    if (is_override_method(iface, child, func))
      continue;

    if (!is_callas(func->attrs)) {
      const var_t *arg;

      fprintf(header, "static inline ");
      write_type_decl_left(header, type_function_get_ret(func->declspec.type));
      fprintf(header, " %s_%s(", name, get_name(func));
      write_args(header, type_function_get_args(func->declspec.type), name, 1, FALSE, NAME_C);
      fprintf(header, ") {\n");
      ++indentation;
      if (!is_aggregate_return(func)) {
        indent(header, 0);
        fprintf(header, "%sThis->lpVtbl->%s(This",
                is_void(type_function_get_rettype(func->declspec.type)) ? "" : "return ",
                get_vtbl_entry_name(iface, func));
      } else {
        indent(header, 0);
        write_type_decl_left(header, type_function_get_ret(func->declspec.type));
        fprintf(header, " __ret;\n");
        indent(header, 0);
        fprintf(header, "return *This->lpVtbl->%s(This,&__ret", get_vtbl_entry_name(iface, func));
      }
      if (type_function_get_args(func->declspec.type))
          LIST_FOR_EACH_ENTRY( arg, type_function_get_args(func->declspec.type), const var_t, entry )
              fprintf(header, ",%s", arg->name);
      fprintf(header, ");\n");
      --indentation;
      fprintf(header, "}\n");
    }
  }
}

static void do_write_c_method_def(FILE *header, const type_t *iface, const char *name)
{
  const statement_t *stmt;
  int first_iface = 1;

  if (type_iface_get_inherit(iface))
    do_write_c_method_def(header, type_iface_get_inherit(iface), name);
#ifdef __REACTOS__ /* r59312 / 3ab1571 */
  else if (type_iface_get_stmts(iface) == NULL)
  {
    fprintf(header, "#ifndef __cplusplus\n");
    indent(header, 0);
    fprintf(header, "char dummy;\n");
    fprintf(header, "#endif\n");
    fprintf(header, "\n");
    return;
  }
#endif

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;
    if (first_iface) {
      indent(header, 0);
      fprintf(header, "/*** %s methods ***/\n", iface->name);
      first_iface = 0;
    }
    if (!is_callas(func->attrs)) {
      const char *callconv = get_attrp(func->declspec.type->attrs, ATTR_CALLCONV);
      if (!callconv) callconv = "STDMETHODCALLTYPE";
      indent(header, 0);
      write_type_decl_left(header, type_function_get_ret(func->declspec.type));
      if (is_aggregate_return(func))
        fprintf(header, " *");
      if (is_inherited_method(iface, func))
        fprintf(header, " (%s *%s_%s)(\n", callconv, iface->name, func->name);
      else
        fprintf(header, " (%s *%s)(\n", callconv, get_name(func));
      ++indentation;
      indent(header, 0);
      fprintf(header, "%s *This", name);
      if (is_aggregate_return(func)) {
        fprintf(header, ",\n");
        indent(header, 0);
        write_type_decl_left(header, type_function_get_ret(func->declspec.type));
        fprintf(header, " *__ret");
      }
      --indentation;
      if (type_function_get_args(func->declspec.type)) {
        fprintf(header, ",\n");
        write_args(header, type_function_get_args(func->declspec.type), name, 0, TRUE, NAME_C);
      }
      fprintf(header, ");\n");
      fprintf(header, "\n");
    }
  }
}

static void write_c_method_def(FILE *header, const type_t *iface)
{
  do_write_c_method_def(header, iface, iface->c_name);
}

static void write_c_disp_method_def(FILE *header, const type_t *iface)
{
  do_write_c_method_def(header, type_iface_get_inherit(iface), iface->c_name);
}

static void write_method_proto(FILE *header, const type_t *iface)
{
  const statement_t *stmt;

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;

    if (is_callas(func->attrs)) {
      const char *callconv = get_attrp(func->declspec.type->attrs, ATTR_CALLCONV);
      if (!callconv) callconv = "STDMETHODCALLTYPE";
      /* proxy prototype */
      write_type_decl_left(header, type_function_get_ret(func->declspec.type));
      fprintf(header, " %s %s_%s_Proxy(\n", callconv, iface->name, get_name(func));
      write_args(header, type_function_get_args(func->declspec.type), iface->name, 1, TRUE, NAME_DEFAULT);
      fprintf(header, ");\n");
      /* stub prototype */
      fprintf(header, "void __RPC_STUB %s_%s_Stub(\n", iface->name, get_name(func));
      fprintf(header, "    IRpcStubBuffer* This,\n");
      fprintf(header, "    IRpcChannelBuffer* pRpcChannelBuffer,\n");
      fprintf(header, "    PRPC_MESSAGE pRpcMessage,\n");
      fprintf(header, "    DWORD* pdwStubPhase);\n");
    }
  }
}

static void write_locals(FILE *fp, const type_t *iface, int body)
{
  static const char comment[]
    = "/* WIDL-generated stub.  You must provide an implementation for this.  */";
  const statement_t *stmt;

  if (!is_object(iface))
    return;

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface)) {
    const var_t *func = stmt->u.var;
    const var_t *cas = is_callas(func->attrs);

    if (cas) {
      const statement_t *stmt2 = NULL;
      STATEMENTS_FOR_EACH_FUNC(stmt2, type_iface_get_stmts(iface))
        if (!strcmp(get_name(stmt2->u.var), cas->name))
          break;
      if (&stmt2->entry != type_iface_get_stmts(iface)) {
        const var_t *m = stmt2->u.var;
        /* proxy prototype - use local prototype */
        write_type_decl_left(fp, type_function_get_ret(m->declspec.type));
        fprintf(fp, " CALLBACK %s_%s_Proxy(\n", iface->name, get_name(m));
        write_args(fp, type_function_get_args(m->declspec.type), iface->name, 1, TRUE, NAME_DEFAULT);
        fprintf(fp, ")");
        if (body) {
          const decl_spec_t *rt = type_function_get_ret(m->declspec.type);
          fprintf(fp, "\n{\n");
          fprintf(fp, "    %s\n", comment);
          if (rt->type->name && strcmp(rt->type->name, "HRESULT") == 0)
            fprintf(fp, "    return E_NOTIMPL;\n");
          else if (type_get_type(rt->type) != TYPE_VOID) {
            fprintf(fp, "    ");
            write_type_decl(fp, rt, "rv");
            fprintf(fp, ";\n");
            fprintf(fp, "    memset(&rv, 0, sizeof rv);\n");
            fprintf(fp, "    return rv;\n");
          }
          fprintf(fp, "}\n\n");
        }
        else
          fprintf(fp, ";\n");
        /* stub prototype - use remotable prototype */
        write_type_decl_left(fp, type_function_get_ret(func->declspec.type));
        fprintf(fp, " __RPC_STUB %s_%s_Stub(\n", iface->name, get_name(m));
        write_args(fp, type_function_get_args(func->declspec.type), iface->name, 1, TRUE, NAME_DEFAULT);
        fprintf(fp, ")");
        if (body)
          /* Remotable methods must all return HRESULTs.  */
          fprintf(fp, "\n{\n    %s\n    return E_NOTIMPL;\n}\n\n", comment);
        else
          fprintf(fp, ";\n");
      }
      else
        error_loc("invalid call_as attribute (%s -> %s)\n", func->name, cas->name);
    }
  }
}

static void write_local_stubs_stmts(FILE *local_stubs, const statement_list_t *stmts)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE)
      write_locals(local_stubs, stmt->u.type, TRUE);
  }
}

void write_local_stubs(const statement_list_t *stmts)
{
  FILE *local_stubs;

  if (!local_stubs_name) return;

  local_stubs = fopen(local_stubs_name, "w");
  if (!local_stubs) {
    error("Could not open %s for output\n", local_stubs_name);
    return;
  }
  fprintf(local_stubs, "/* call_as/local stubs for %s */\n\n", input_name);
  fprintf(local_stubs, "#include <objbase.h>\n");
  fprintf(local_stubs, "#include \"%s\"\n\n", header_name);

  write_local_stubs_stmts(local_stubs, stmts);

  fclose(local_stubs);
}

static void write_function_proto(FILE *header, const type_t *iface, const var_t *fun, const char *prefix)
{
  const char *callconv = get_attrp(fun->declspec.type->attrs, ATTR_CALLCONV);

  if (!callconv) callconv = "__cdecl";
  /* FIXME: do we need to handle call_as? */
  write_type_decl_left(header, type_function_get_ret(fun->declspec.type));
  fprintf(header, " %s ", callconv);
  fprintf(header, "%s%s(\n", prefix, get_name(fun));
  if (type_function_get_args(fun->declspec.type))
    write_args(header, type_function_get_args(fun->declspec.type), iface->name, 0, TRUE, NAME_DEFAULT);
  else
    fprintf(header, "    void");
  fprintf(header, ");\n\n");
}

static void write_parameterized_type_forward(FILE *header, type_t *type)
{
    type_t *iface = type->details.parameterized.type;
    char *args;

    if (type_get_type(iface) == TYPE_DELEGATE) iface = type_delegate_get_iface(iface);

    fprintf(header, "#if defined(__cplusplus) && !defined(CINTERFACE)\n");
    write_namespace_start(header, type->namespace);

    args = format_parameterized_type_args(type, "class ", "");
    write_line(header, 0, "template <%s>", args);
    write_line(header, 0, "struct %s_impl;\n", iface->name);

    write_line(header, 0, "template <%s>", args);
    free(args);
    args = format_parameterized_type_args(type, "", "");
    write_line(header, 0, "struct %s : %s_impl<%s> {};", iface->name, iface->name, args);
    free(args);

    write_namespace_end(header, type->namespace);
    fprintf(header, "#endif\n\n" );
}

static void write_parameterized_implementation(FILE *header, type_t *type, bool define)
{
    const statement_t *stmt;
    typeref_list_t *params = type->details.parameterized.params;
    typeref_t *ref;
    type_t *iface = type->details.parameterized.type, *base;
    char *args = NULL;

    fprintf(header, "#if defined(__cplusplus) && !defined(CINTERFACE)\n");
    write_line(header, 0, "} /* extern \"C\" */");
    write_namespace_start(header, type->namespace);

    if (type_get_type(iface) == TYPE_DELEGATE) iface = type_delegate_get_iface(iface);
    base = type_iface_get_inherit(iface);

    args = format_parameterized_type_args(type, "class ", "");
    write_line(header, 0, "template <%s>", args);
    free(args);
    write_line(header, 0, "struct %s_impl%s", iface->name, base ? strmake(" : %s", base->name) : "");
    write_line(header, 0, "{");

    write_line(header, 1, "private:");
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
    {
        write_line(header, 0, "typedef typename Windows::Foundation::Internal::GetAbiType<%s>::type     %s_abi;", ref->type->name, ref->type->name);
        write_line(header, 0, "typedef typename Windows::Foundation::Internal::GetLogicalType<%s>::type %s_logical;", ref->type->name, ref->type->name);
    }
    indentation -= 1;

    write_line(header, 1, "public:");
    if (params) LIST_FOR_EACH_ENTRY(ref, params, typeref_t, entry)
        write_line(header, 0, "typedef %s %s_complex;", ref->type->name, ref->type->name);

    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
    {
        const var_t *func = stmt->u.var;
        if (is_callas(func->attrs)) continue;
        indent(header, 1);
        fprintf(header, "virtual ");
        write_type_decl_left(header, &func->declspec);
        fprintf(header, "%s(", get_name(func));
        write_args(header, type_function_get_args(func->declspec.type), NULL, 0, 0, NAME_DEFAULT);
        fprintf(header, ") = 0;\n");
        indentation -= 1;
    }
    write_line(header, -1, "};");

    write_namespace_end(header, type->namespace);
    write_line(header, 0, "extern \"C\" {");
    write_line(header, 0, "#endif\n");
}

static void write_forward(FILE *header, type_t *iface)
{
  fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", iface->c_name);
  fprintf(header, "#define __%s_FWD_DEFINED__\n", iface->c_name);
  fprintf(header, "typedef interface %s %s;\n", iface->c_name, iface->c_name);
  fprintf(header, "#ifdef __cplusplus\n");
  if (iface->namespace && !is_global_namespace(iface->namespace))
    fprintf(header, "#define %s %s\n", iface->c_name, iface->qualified_name);
  if (!iface->impl_name)
  {
    write_namespace_start(header, iface->namespace);
    write_line(header, 0, "interface %s;", iface->name);
    write_namespace_end(header, iface->namespace);
  }
  fprintf(header, "#endif /* __cplusplus */\n");
  fprintf(header, "#endif\n\n" );
}

static char *format_apicontract_macro(const type_t *type)
{
    char *name = format_namespace(type->namespace, "", "_", type->name, NULL);
    int i;
    for (i = strlen(name); i > 0; --i) name[i - 1] = toupper(name[i - 1]);
    return name;
}

static void write_apicontract_guard_start(FILE *header, const expr_t *expr)
{
    const type_t *type;
    char *name;
    int ver;
    if (!winrt_mode) return;
    type = expr->u.tref.type;
    ver = expr->ref->u.integer.value;
    name = format_apicontract_macro(type);
    fprintf(header, "#if %s_VERSION >= %#x\n", name, ver);
    free(name);
}

static void write_apicontract_guard_end(FILE *header, const expr_t *expr)
{
    const type_t *type;
    char *name;
    int ver;
    if (!winrt_mode) return;
    type = expr->u.tref.type;
    ver = expr->ref->u.integer.value;
    name = format_apicontract_macro(type);
    fprintf(header, "#endif /* %s_VERSION >= %#x */\n", name, ver);
    free(name);
}

static void write_com_interface_start(FILE *header, const type_t *iface)
{
  int dispinterface = is_attr(iface->attrs, ATTR_DISPINTERFACE);
  expr_t *contract = get_attrp(iface->attrs, ATTR_CONTRACT);
  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s %sinterface\n", iface->name, dispinterface ? "disp" : "");
  fprintf(header, " */\n");
  if (contract) write_apicontract_guard_start(header, contract);
  fprintf(header,"#ifndef __%s_%sINTERFACE_DEFINED__\n", iface->c_name, dispinterface ? "DISP" : "");
  fprintf(header,"#define __%s_%sINTERFACE_DEFINED__\n\n", iface->c_name, dispinterface ? "DISP" : "");
}

static void write_widl_using_method_macros(FILE *header, const type_t *iface, const type_t *top_iface)
{
    const statement_t *stmt;
    const char *name = top_iface->short_name ? top_iface->short_name : top_iface->name;

    if (type_iface_get_inherit(iface)) write_widl_using_method_macros(header, type_iface_get_inherit(iface), top_iface);

    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
    {
        const var_t *func = stmt->u.var;
        const char *func_name;

        if (is_override_method(iface, top_iface, func)) continue;
        if (is_callas(func->attrs)) continue;

        func_name = get_name(func);
        fprintf(header, "#define %s_%s %s_%s\n", name, func_name, top_iface->c_name, func_name);
    }
}

static void write_widl_using_macros(FILE *header, type_t *iface)
{
    const struct uuid *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const char *name = iface->short_name ? iface->short_name : iface->name;
    char *macro;

    if (!strcmp(iface->name, iface->c_name)) return;

    macro = format_namespace(iface->namespace, "WIDL_using_", "_", NULL, NULL);
    fprintf(header, "#ifdef %s\n", macro);

    if (uuid) fprintf(header, "#define IID_%s IID_%s\n", name, iface->c_name);
    if (iface->type_type == TYPE_INTERFACE) fprintf(header, "#define %sVtbl %sVtbl\n", name, iface->c_name);
    fprintf(header, "#define %s %s\n", name, iface->c_name);

    if (iface->type_type == TYPE_INTERFACE) write_widl_using_method_macros(header, iface, iface);

    fprintf(header, "#endif /* %s */\n", macro);
    free(macro);
}

static void write_com_interface_end(FILE *header, type_t *iface)
{
  int dispinterface = is_attr(iface->attrs, ATTR_DISPINTERFACE);
  const struct uuid *uuid = get_attrp(iface->attrs, ATTR_UUID);
  expr_t *contract = get_attrp(iface->attrs, ATTR_CONTRACT);
  type_t *type;

  if (uuid)
      write_guid(header, dispinterface ? "DIID" : "IID", iface->c_name, uuid);

  /* C++ interface */
  fprintf(header, "#if defined(__cplusplus) && !defined(CINTERFACE)\n");
  if (!is_global_namespace(iface->namespace)) {
      write_line(header, 0, "} /* extern \"C\" */");
      write_namespace_start(header, iface->namespace);
  }
  if (uuid) {
      if (strchr(iface->name, '<')) write_line(header, 0, "template<>");
      write_line(header, 0, "MIDL_INTERFACE(\"%s\")", uuid_string(uuid));
      indent(header, 0);
  }else {
      indent(header, 0);
      if (strchr(iface->name, '<')) fprintf(header, "template<> struct ");
      else fprintf(header, "interface ");
  }
  if (iface->impl_name)
  {
    fprintf(header, "%s : %s\n", iface->name, iface->impl_name);
    write_line(header, 1, "{");
  }
  else if (type_iface_get_inherit(iface))
  {
    fprintf(header, "%s : public %s\n", iface->name,
            type_iface_get_inherit(iface)->name);
    write_line(header, 1, "{");
  }
  else
  {
    fprintf(header, "%s\n", iface->name);
    write_line(header, 1, "{\n");
    write_line(header, 0, "BEGIN_INTERFACE\n");
  }
  /* dispinterfaces don't have real functions, so don't write C++ functions for
   * them */
  if (!dispinterface && !iface->impl_name)
    write_cpp_method_def(header, iface);
  if (!type_iface_get_inherit(iface) && !iface->impl_name)
    write_line(header, 0, "END_INTERFACE\n");
  write_line(header, -1, "};");
  if (!is_global_namespace(iface->namespace)) {
      write_namespace_end(header, iface->namespace);
      write_line(header, 0, "extern \"C\" {");
  }
  if (uuid)
      write_uuid_decl(header, iface, uuid);
  fprintf(header, "#else\n");
  /* C interface */
  write_line(header, 1, "typedef struct %sVtbl {", iface->c_name);
  write_line(header, 0, "BEGIN_INTERFACE\n");
  if (dispinterface)
    write_c_disp_method_def(header, iface);
  else
    write_c_method_def(header, iface);
  write_line(header, 0, "END_INTERFACE");
  write_line(header, -1, "} %sVtbl;\n", iface->c_name);
  fprintf(header, "interface %s {\n", iface->c_name);
  fprintf(header, "    CONST_VTBL %sVtbl* lpVtbl;\n", iface->c_name);
  fprintf(header, "};\n\n");
  fprintf(header, "#ifdef COBJMACROS\n");
  /* dispinterfaces don't have real functions, so don't write macros for them,
   * only for the interface this interface inherits from, i.e. IDispatch */
  fprintf(header, "#ifndef WIDL_C_INLINE_WRAPPERS\n");
  type = dispinterface ? type_iface_get_inherit(iface) : iface;
  write_method_macro(header, type, type, iface->c_name);
  fprintf(header, "#else\n");
  write_inline_wrappers(header, type, type, iface->c_name);
  fprintf(header, "#endif\n");
  if (winrt_mode) write_widl_using_macros(header, iface);
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  /* dispinterfaces don't have real functions, so don't write prototypes for
   * them */
  if (!dispinterface && !winrt_mode)
  {
    write_method_proto(header, iface);
    write_locals(header, iface, FALSE);
    fprintf(header, "\n");
  }
  fprintf(header, "#endif  /* __%s_%sINTERFACE_DEFINED__ */\n", iface->c_name, dispinterface ? "DISP" : "");
  if (contract) write_apicontract_guard_end(header, contract);
  fprintf(header, "\n");
}

static void write_rpc_interface_start(FILE *header, const type_t *iface)
{
  unsigned int ver = get_attrv(iface->attrs, ATTR_VERSION);
  const var_t *var = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
  expr_t *contract = get_attrp(iface->attrs, ATTR_CONTRACT);

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s interface (v%d.%d)\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
  fprintf(header, " */\n");
  if (contract) write_apicontract_guard_start(header, contract);
  fprintf(header,"#ifndef __%s_INTERFACE_DEFINED__\n", iface->name);
  fprintf(header,"#define __%s_INTERFACE_DEFINED__\n\n", iface->name);
  if (var)
  {
      fprintf(header, "extern ");
      write_type_decl( header, &var->declspec, var->name );
      fprintf(header, ";\n");
  }
  if (old_names)
  {
      fprintf(header, "extern RPC_IF_HANDLE %s%s_ClientIfHandle;\n", prefix_client, iface->name);
      fprintf(header, "extern RPC_IF_HANDLE %s%s_ServerIfHandle;\n", prefix_server, iface->name);
  }
  else
  {
      fprintf(header, "extern RPC_IF_HANDLE %s%s_v%d_%d_c_ifspec;\n",
              prefix_client, iface->name, MAJORVERSION(ver), MINORVERSION(ver));
      fprintf(header, "extern RPC_IF_HANDLE %s%s_v%d_%d_s_ifspec;\n",
              prefix_server, iface->name, MAJORVERSION(ver), MINORVERSION(ver));
  }
}

static void write_rpc_interface_end(FILE *header, const type_t *iface)
{
  expr_t *contract = get_attrp(iface->attrs, ATTR_CONTRACT);
  fprintf(header, "\n#endif  /* __%s_INTERFACE_DEFINED__ */\n", iface->name);
  if (contract) write_apicontract_guard_end(header, contract);
  fprintf(header, "\n");
}

static void write_coclass(FILE *header, type_t *cocl)
{
  const struct uuid *uuid = get_attrp(cocl->attrs, ATTR_UUID);

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s coclass\n", cocl->name);
  fprintf(header, " */\n\n");
  if (uuid)
      write_guid(header, "CLSID", cocl->name, uuid);
  fprintf(header, "\n#ifdef __cplusplus\n");
  if (uuid)
  {
      fprintf(header, "class DECLSPEC_UUID(\"%s\") %s;\n", uuid_string(uuid), cocl->name);
      write_uuid_decl(header, cocl, uuid);
  }
  else
  {
      fprintf(header, "class %s;\n", cocl->name);
  }
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
}

static void write_coclass_forward(FILE *header, type_t *cocl)
{
  fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", cocl->name);
  fprintf(header, "#define __%s_FWD_DEFINED__\n", cocl->name);
  fprintf(header, "#ifdef __cplusplus\n");
  fprintf(header, "typedef class %s %s;\n", cocl->name, cocl->name);
  fprintf(header, "#else\n");
  fprintf(header, "typedef struct %s %s;\n", cocl->name, cocl->name);
  fprintf(header, "#endif /* defined __cplusplus */\n");
  fprintf(header, "#endif /* defined __%s_FWD_DEFINED__ */\n\n", cocl->name );
}

static void write_apicontract(FILE *header, type_t *apicontract)
{
    char *name = format_apicontract_macro(apicontract);
    fprintf(header, "#if !defined(%s_VERSION)\n", name);
    fprintf(header, "#define %s_VERSION %#x\n", name, get_attrv(apicontract->attrs, ATTR_CONTRACTVERSION));
    fprintf(header, "#endif // defined(%s_VERSION)\n\n", name);
    free(name);
}

static void write_runtimeclass(FILE *header, type_t *runtimeclass)
{
    expr_t *contract = get_attrp(runtimeclass->attrs, ATTR_CONTRACT);
    char *name, *c_name;
    size_t i, len;
    name = format_namespace(runtimeclass->namespace, "", ".", runtimeclass->name, NULL);
    c_name = format_namespace(runtimeclass->namespace, "", "_", runtimeclass->name, NULL);
    fprintf(header, "/*\n");
    fprintf(header, " * Class %s\n", name);
    fprintf(header, " */\n");
    if (contract) write_apicontract_guard_start(header, contract);
    fprintf(header, "#ifndef RUNTIMECLASS_%s_DEFINED\n", c_name);
    fprintf(header, "#define RUNTIMECLASS_%s_DEFINED\n", c_name);
    fprintf(header, "#if !defined(_MSC_VER) && !defined(__MINGW32__)\n");
    fprintf(header, "static const WCHAR RuntimeClass_%s[] = {", c_name);
    for (i = 0, len = strlen(name); i < len; ++i) fprintf(header, "'%c',", name[i]);
    fprintf(header, "0};\n");
    fprintf(header, "#elif defined(__GNUC__) && !defined(__cplusplus)\n");
    /* FIXME: MIDL generates extern const here but GCC warns if extern is initialized */
    fprintf(header, "const DECLSPEC_SELECTANY WCHAR RuntimeClass_%s[] = L\"%s\";\n", c_name, name);
    fprintf(header, "#else\n");
    fprintf(header, "extern const DECLSPEC_SELECTANY WCHAR RuntimeClass_%s[] = {", c_name);
    for (i = 0, len = strlen(name); i < len; ++i) fprintf(header, "'%c',", name[i]);
    fprintf(header, "0};\n");
    fprintf(header, "#endif\n");
    fprintf(header, "#endif /* RUNTIMECLASS_%s_DEFINED */\n", c_name);
    free(c_name);
    free(name);
    if (contract) write_apicontract_guard_end(header, contract);
    fprintf(header, "\n");
}

static void write_runtimeclass_forward(FILE *header, type_t *runtimeclass)
{
    fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", runtimeclass->c_name);
    fprintf(header, "#define __%s_FWD_DEFINED__\n", runtimeclass->c_name);
    fprintf(header, "#ifdef __cplusplus\n");
    write_namespace_start(header, runtimeclass->namespace);
    write_line(header, 0, "class %s;", runtimeclass->name);
    write_namespace_end(header, runtimeclass->namespace);
    fprintf(header, "#else\n");
    fprintf(header, "typedef struct %s %s;\n", runtimeclass->c_name, runtimeclass->c_name);
    fprintf(header, "#endif /* defined __cplusplus */\n");
    fprintf(header, "#endif /* defined __%s_FWD_DEFINED__ */\n\n", runtimeclass->c_name);
}

static void write_import(FILE *header, const char *fname)
{
  char *hname = replace_extension( get_basename(fname), ".idl", "" );

  if (!strendswith( hname, ".h" )) hname = strmake( "%s.h", hname );
  fprintf(header, "#include <%s>\n", hname);
  free(hname);
}

static void write_imports(FILE *header, const statement_list_t *stmts)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    switch (stmt->type)
    {
      case STMT_TYPE:
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE)
          write_imports(header, type_iface_get_stmts(stmt->u.type));
        break;
      case STMT_TYPEREF:
      case STMT_IMPORTLIB:
        /* not included in header */
        break;
      case STMT_IMPORT:
        write_import(header, stmt->u.str);
        break;
      case STMT_TYPEDEF:
      case STMT_MODULE:
      case STMT_CPPQUOTE:
      case STMT_PRAGMA:
      case STMT_DECLARATION:
        /* not processed here */
        break;
      case STMT_LIBRARY:
        write_imports(header, stmt->u.lib->stmts);
        break;
    }
  }
}

static void write_forward_decls(FILE *header, const statement_list_t *stmts)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    switch (stmt->type)
    {
      case STMT_TYPE:
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE || type_get_type(stmt->u.type) == TYPE_DELEGATE)
        {
          type_t *iface = stmt->u.type;
          if (type_get_type(iface) == TYPE_DELEGATE) iface = type_delegate_get_iface(iface);
          if (is_object(iface) || is_attr(iface->attrs, ATTR_DISPINTERFACE))
          {
            write_forward(header, iface);
            if (type_iface_get_async_iface(iface))
              write_forward(header, type_iface_get_async_iface(iface));
          }
        }
        else if (type_get_type(stmt->u.type) == TYPE_COCLASS)
          write_coclass_forward(header, stmt->u.type);
        else if (type_get_type(stmt->u.type) == TYPE_RUNTIMECLASS)
          write_runtimeclass_forward(header, stmt->u.type);
        else if (type_get_type(stmt->u.type) == TYPE_PARAMETERIZED_TYPE)
          write_parameterized_type_forward(header, stmt->u.type);
        break;
      case STMT_TYPEREF:
      case STMT_IMPORTLIB:
        /* not included in header */
        break;
      case STMT_IMPORT:
      case STMT_TYPEDEF:
      case STMT_MODULE:
      case STMT_CPPQUOTE:
      case STMT_PRAGMA:
      case STMT_DECLARATION:
        /* not processed here */
        break;
      case STMT_LIBRARY:
        write_forward_decls(header, stmt->u.lib->stmts);
        break;
    }
  }
}

static void write_header_stmts(FILE *header, const statement_list_t *stmts, const type_t *iface, int ignore_funcs)
{
  const statement_t *stmt;
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
  {
    switch (stmt->type)
    {
      case STMT_TYPE:
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE || type_get_type(stmt->u.type) == TYPE_DELEGATE)
        {
          type_t *iface = stmt->u.type, *async_iface;
          if (type_get_type(stmt->u.type) == TYPE_DELEGATE) iface = type_delegate_get_iface(iface);
          async_iface = type_iface_get_async_iface(iface);
          if (is_object(iface)) is_object_interface++;
          if (is_attr(stmt->u.type->attrs, ATTR_DISPINTERFACE) || is_object(stmt->u.type))
          {
            write_com_interface_start(header, iface);
            write_header_stmts(header, type_iface_get_stmts(iface), stmt->u.type, TRUE);
            write_com_interface_end(header, iface);
            if (async_iface)
            {
              write_com_interface_start(header, async_iface);
              write_com_interface_end(header, async_iface);
            }
          }
          else
          {
            write_rpc_interface_start(header, iface);
            write_header_stmts(header, type_iface_get_stmts(iface), iface, FALSE);
            write_rpc_interface_end(header, iface);
          }
          if (is_object(iface)) is_object_interface--;
        }
        else if (type_get_type(stmt->u.type) == TYPE_COCLASS)
          write_coclass(header, stmt->u.type);
        else if (type_get_type(stmt->u.type) == TYPE_APICONTRACT)
          write_apicontract(header, stmt->u.type);
        else if (type_get_type(stmt->u.type) == TYPE_RUNTIMECLASS)
          write_runtimeclass(header, stmt->u.type);
        else if (type_get_type(stmt->u.type) != TYPE_PARAMETERIZED_TYPE)
          write_type_definition(header, stmt->u.type, stmt->is_defined);
        else
        {
          is_object_interface++;
          write_parameterized_implementation(header, stmt->u.type, stmt->is_defined);
          is_object_interface--;
        }
        break;
      case STMT_TYPEREF:
        /* FIXME: shouldn't write out forward declarations for undefined
        * interfaces but a number of our IDL files depend on this */
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE && !stmt->u.type->written)
          write_forward(header, stmt->u.type);
        break;
      case STMT_IMPORTLIB:
      case STMT_PRAGMA:
        /* not included in header */
        break;
      case STMT_IMPORT:
        /* not processed here */
        break;
      case STMT_TYPEDEF:
      {
        typeref_t *ref;
        if (stmt->u.type_list) LIST_FOR_EACH_ENTRY(ref, stmt->u.type_list, typeref_t, entry)
          write_typedef(header, ref->type, stmt->is_defined);
        break;
      }
      case STMT_LIBRARY:
        fprintf(header, "#ifndef __%s_LIBRARY_DEFINED__\n", stmt->u.lib->name);
        fprintf(header, "#define __%s_LIBRARY_DEFINED__\n", stmt->u.lib->name);
        write_library(header, stmt->u.lib);
        write_header_stmts(header, stmt->u.lib->stmts, NULL, FALSE);
        fprintf(header, "#endif /* __%s_LIBRARY_DEFINED__ */\n", stmt->u.lib->name);
        break;
      case STMT_MODULE:
        fprintf(header, "#ifndef __%s_MODULE_DEFINED__\n", stmt->u.type->name);
        fprintf(header, "#define __%s_MODULE_DEFINED__\n", stmt->u.type->name);
        write_header_stmts(header, stmt->u.type->details.module->stmts, stmt->u.type, FALSE);
        fprintf(header, "#endif /* __%s_MODULE_DEFINED__ */\n", stmt->u.type->name);
        break;
      case STMT_CPPQUOTE:
        fprintf(header, "%s\n", stmt->u.str);
        break;
      case STMT_DECLARATION:
        if (iface && type_get_type(stmt->u.var->declspec.type) == TYPE_FUNCTION)
        {
          if (!ignore_funcs)
          {
            int prefixes_differ = strcmp(prefix_client, prefix_server);

            if (prefixes_differ)
            {
              fprintf(header, "/* client prototype */\n");
              write_function_proto(header, iface, stmt->u.var, prefix_client);
              fprintf(header, "/* server prototype */\n");
            }
            write_function_proto(header, iface, stmt->u.var, prefix_server);
          }
        }
        else
          write_declaration(header, stmt->u.var);
        break;
    }
  }
}

void write_header(const statement_list_t *stmts)
{
  FILE *header;

  if (!do_header) return;

  if(!(header = fopen(header_name, "w"))) {
    error("Could not open %s for output\n", header_name);
    return;
  }
  fprintf(header, "/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n\n", PACKAGE_VERSION, input_name);

  fprintf(header, "#ifdef _WIN32\n");
  fprintf(header, "#ifndef __REQUIRED_RPCNDR_H_VERSION__\n");
  fprintf(header, "#define __REQUIRED_RPCNDR_H_VERSION__ 475\n");
#ifdef __REACTOS__
  fprintf(header, "#endif\n\n");

  fprintf(header, "#ifdef __REACTOS__\n");
  fprintf(header, "#define WIN32_LEAN_AND_MEAN\n");
  fprintf(header, "#endif\n\n");
#else
  fprintf(header, "#endif\n");
#endif
  fprintf(header, "#include <rpc.h>\n" );
  fprintf(header, "#include <rpcndr.h>\n" );
  if (!for_each_serializable(stmts, NULL, serializable_exists))
    fprintf(header, "#include <midles.h>\n" );
  fprintf(header, "#endif\n\n");

  fprintf(header, "#ifndef COM_NO_WINDOWS_H\n");
  fprintf(header, "#include <windows.h>\n");
  fprintf(header, "#include <ole2.h>\n");
  fprintf(header, "#endif\n\n");

  fprintf(header, "#ifndef __%s__\n", header_token);
  fprintf(header, "#define __%s__\n\n", header_token);

  fprintf(header, "/* Forward declarations */\n\n");
  write_forward_decls(header, stmts);

  fprintf(header, "/* Headers for imported files */\n\n");
  write_imports(header, stmts);
  fprintf(header, "\n");
  start_cplusplus_guard(header);

  write_header_stmts(header, stmts, NULL, FALSE);

  fprintf(header, "/* Begin additional prototypes for all interfaces */\n");
  fprintf(header, "\n");
  for_each_serializable(stmts, header, write_serialize_function_decl);
  write_user_types(header);
  write_generic_handle_routines(header);
  write_context_handle_rundowns(header);
  fprintf(header, "\n");
  fprintf(header, "/* End additional prototypes */\n");
  fprintf(header, "\n");

  end_cplusplus_guard(header);
  fprintf(header, "#endif /* __%s__ */\n", header_token);

  fclose(header);
}
