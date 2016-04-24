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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "expr.h"
#include "typetree.h"

static int indentation = 0;
static int is_object_interface = 0;
user_type_list_t user_type_list = LIST_INIT(user_type_list);
context_handle_list_t context_handle_list = LIST_INIT(context_handle_list);
generic_handle_list_t generic_handle_list = LIST_INIT(generic_handle_list);

static void write_type_def_or_decl(FILE *f, type_t *t, int field, const char *name);

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

int is_ptrchain_attr(const var_t *var, enum attr_type t)
{
    if (is_attr(var->attrs, t))
        return 1;
    else
    {
        type_t *type = var->type;
        for (;;)
        {
            if (is_attr(type->attrs, t))
                return 1;
            else if (type_is_alias(type))
                type = type_alias_get_aliasee(type);
            else if (is_ptr(type))
                type = type_pointer_get_ref(type);
            else return 0;
        }
    }
}

int is_aliaschain_attr(const type_t *type, enum attr_type attr)
{
    const type_t *t = type;
    for (;;)
    {
        if (is_attr(t->attrs, attr))
            return 1;
        else if (type_is_alias(t))
            t = type_alias_get_aliasee(t);
        else return 0;
    }
}

int is_attr(const attr_list_t *list, enum attr_type t)
{
    const attr_t *attr;
    if (list) LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == t) return 1;
    return 0;
}

void *get_attrp(const attr_list_t *list, enum attr_type t)
{
    const attr_t *attr;
    if (list) LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == t) return attr->u.pval;
    return NULL;
}

unsigned int get_attrv(const attr_list_t *list, enum attr_type t)
{
    const attr_t *attr;
    if (list) LIST_FOR_EACH_ENTRY( attr, list, const attr_t, entry )
        if (attr->type == t) return attr->u.ival;
    return 0;
}

static void write_guid(FILE *f, const char *guid_prefix, const char *name, const UUID *uuid)
{
  if (!uuid) return;
  fprintf(f, "DEFINE_GUID(%s_%s, 0x%08x, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x);\n",
        guid_prefix, name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0],
        uuid->Data4[1], uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5],
        uuid->Data4[6], uuid->Data4[7]);
}

static void write_uuid_decl(FILE *f, type_t *type, const UUID *uuid)
{
  char *name = format_namespace(type->namespace, "", "::", type->name);
  fprintf(f, "#ifdef __CRT_UUID_DECL\n");
  fprintf(f, "__CRT_UUID_DECL(%s, 0x%08x, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x)\n",
        name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
        uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
        uuid->Data4[7]);
  fprintf(f, "#endif\n");
  free(name);
}

static const char *uuid_string(const UUID *uuid)
{
  static char buf[37];

  sprintf(buf, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
            write_line(header, -1, "}", namespace->name);
        return;
    }

    write_line(header, -1, "}", namespace->name);
    write_namespace_end(header, namespace->parent);
}

const char *get_name(const var_t *v)
{
    static char buffer[256];

    if (is_attr( v->attrs, ATTR_PROPGET ))
        strcpy( buffer, "get_" );
    else if (is_attr( v->attrs, ATTR_PROPPUT ))
        strcpy( buffer, "put_" );
    else if (is_attr( v->attrs, ATTR_PROPPUTREF ))
        strcpy( buffer, "putref_" );
    else
        buffer[0] = 0;
    strcat( buffer, v->name );
    return buffer;
}

static void write_fields(FILE *h, var_list_t *fields)
{
    unsigned nameless_struct_cnt = 0, nameless_struct_i = 0, nameless_union_cnt = 0, nameless_union_i = 0;
    const char *name;
    char buf[32];
    var_t *v;

    if (!fields) return;

    LIST_FOR_EACH_ENTRY( v, fields, var_t, entry ) {
        if (!v || !v->type) continue;

        switch(type_get_type_detect_alias(v->type)) {
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
        if (!v || !v->type) continue;

        indent(h, 0);
        name = v->name;

        switch(type_get_type_detect_alias(v->type)) {
        case TYPE_STRUCT:
        case TYPE_ENCAPSULATED_UNION:
            if(!v->name) {
                fprintf(h, "__C89_NAMELESS ");
                if(nameless_struct_cnt == 1) {
                    name = "__C89_NAMELESSSTRUCTNAME";
                }else if(nameless_struct_i < 5 /* # of supporting macros */) {
                    sprintf(buf, "__C89_NAMELESSSTRUCTNAME%d", ++nameless_struct_i);
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
                    sprintf(buf, "__C89_NAMELESSUNIONNAME%d", ++nameless_union_i);
                    name = buf;
                }
            }
            break;
        default:
            ;
        }
        write_type_def_or_decl(h, v->type, TRUE, name);
        fprintf(h, ";\n");
    }
}

static void write_enums(FILE *h, var_list_t *enums, const char *enum_name)
{
  var_t *v;
  if (!enums) return;
  LIST_FOR_EACH_ENTRY( v, enums, var_t, entry )
  {
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
  }
  fprintf(h, "\n");
}

int needs_space_after(type_t *t)
{
  return (type_is_alias(t) ||
          (!is_ptr(t) && (!is_array(t) || !type_array_is_decl_as_ptr(t) || t->name)));
}

void write_type_left(FILE *h, type_t *t, enum name_type name_type, int declonly)
{
  const char *name;

  if (!h) return;

  name = type_get_name(t, name_type);

  if (is_attr(t->attrs, ATTR_CONST) &&
      (type_is_alias(t) || !is_ptr(t)))
    fprintf(h, "const ");

  if (type_is_alias(t)) fprintf(h, "%s", t->name);
  else {
    switch (type_get_type_detect_alias(t)) {
      case TYPE_ENUM:
        if (!declonly && t->defined && !t->written) {
          if (name) fprintf(h, "enum %s {\n", name);
          else fprintf(h, "enum {\n");
          t->written = TRUE;
          indentation++;
          write_enums(h, type_enum_get_values(t), is_global_namespace(t->namespace) ? NULL : t->name);
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "enum %s", name ? name : "");
        break;
      case TYPE_STRUCT:
      case TYPE_ENCAPSULATED_UNION:
        if (!declonly && t->defined && !t->written) {
          if (name) fprintf(h, "struct %s {\n", name);
          else fprintf(h, "struct {\n");
          t->written = TRUE;
          indentation++;
          if (type_get_type(t) != TYPE_STRUCT)
            write_fields(h, type_encapsulated_union_get_fields(t));
          else
            write_fields(h, type_struct_get_fields(t));
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "struct %s", name ? name : "");
        break;
      case TYPE_UNION:
        if (!declonly && t->defined && !t->written) {
          if (t->name) fprintf(h, "union %s {\n", t->name);
          else fprintf(h, "union {\n");
          t->written = TRUE;
          indentation++;
          write_fields(h, type_union_get_cases(t));
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "union %s", t->name ? t->name : "");
        break;
      case TYPE_POINTER:
        write_type_left(h, type_pointer_get_ref(t), name_type, declonly);
        fprintf(h, "%s*", needs_space_after(type_pointer_get_ref(t)) ? " " : "");
        if (is_attr(t->attrs, ATTR_CONST)) fprintf(h, "const ");
        break;
      case TYPE_ARRAY:
        if (t->name && type_array_is_decl_as_ptr(t))
          fprintf(h, "%s", t->name);
        else
        {
          write_type_left(h, type_array_get_element(t), name_type, declonly);
          if (type_array_is_decl_as_ptr(t))
            fprintf(h, "%s*", needs_space_after(type_array_get_element(t)) ? " " : "");
        }
        break;
      case TYPE_BASIC:
        if (type_basic_get_type(t) != TYPE_BASIC_INT32 &&
            type_basic_get_type(t) != TYPE_BASIC_INT64 &&
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
        fprintf(h, "%s", t->name);
        break;
      case TYPE_VOID:
        fprintf(h, "void");
        break;
      case TYPE_BITFIELD:
        write_type_left(h, type_bitfield_get_field(t), name_type, declonly);
        break;
      case TYPE_ALIAS:
      case TYPE_FUNCTION:
        /* handled elsewhere */
        assert(0);
        break;
    }
  }
}

void write_type_right(FILE *h, type_t *t, int is_field)
{
  if (!h) return;

  switch (type_get_type(t))
  {
  case TYPE_ARRAY:
    if (!type_array_is_decl_as_ptr(t))
    {
      if (is_conformant_array(t))
      {
        fprintf(h, "[%s]", is_field ? "1" : "");
        t = type_array_get_element(t);
      }
      for ( ;
           type_get_type(t) == TYPE_ARRAY && !type_array_is_decl_as_ptr(t);
           t = type_array_get_element(t))
        fprintf(h, "[%u]", type_array_get_dim(t));
    }
    break;
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
  case TYPE_FUNCTION:
  case TYPE_INTERFACE:
  case TYPE_POINTER:
    break;
  }
}

static void write_type_v(FILE *h, type_t *t, int is_field, int declonly, const char *name)
{
  type_t *pt = NULL;
  int ptr_level = 0;

  if (!h) return;

  if (t) {
    for (pt = t; is_ptr(pt); pt = type_pointer_get_ref(pt), ptr_level++)
      ;

    if (type_get_type_detect_alias(pt) == TYPE_FUNCTION) {
      int i;
      const char *callconv = get_attrp(pt->attrs, ATTR_CALLCONV);
      if (!callconv && is_object_interface) callconv = "STDMETHODCALLTYPE";
      if (is_attr(pt->attrs, ATTR_INLINE)) fprintf(h, "inline ");
      write_type_left(h, type_function_get_rettype(pt), NAME_DEFAULT, declonly);
      fputc(' ', h);
      if (ptr_level) fputc('(', h);
      if (callconv) fprintf(h, "%s ", callconv);
      for (i = 0; i < ptr_level; i++)
        fputc('*', h);
    } else
      write_type_left(h, t, NAME_DEFAULT, declonly);
  }

  if (name) fprintf(h, "%s%s", !t || needs_space_after(t) ? " " : "", name );

  if (t) {
    if (type_get_type_detect_alias(pt) == TYPE_FUNCTION) {
      const var_list_t *args = type_function_get_args(pt);

      if (ptr_level) fputc(')', h);
      fputc('(', h);
      if (args)
          write_args(h, args, NULL, 0, FALSE);
      else
          fprintf(h, "void");
      fputc(')', h);
    } else
      write_type_right(h, t, is_field);
  }
}

static void write_type_def_or_decl(FILE *f, type_t *t, int field, const char *name)
{
  write_type_v(f, t, field, FALSE, name);
}

static void write_type_definition(FILE *f, type_t *t)
{
    int in_namespace = t->namespace && !is_global_namespace(t->namespace);
    int save_written = t->written;

    if(in_namespace) {
        fprintf(f, "#ifdef __cplusplus\n");
        fprintf(f, "} /* extern \"C\" */\n");
        write_namespace_start(f, t->namespace);
    }
    indent(f, 0);
    write_type_left(f, t, NAME_DEFAULT, FALSE);
    fprintf(f, ";\n");
    if(in_namespace) {
        t->written = save_written;
        write_namespace_end(f, t->namespace);
        fprintf(f, "extern \"C\" {\n");
        fprintf(f, "#else\n");
        write_type_left(f, t, NAME_C, FALSE);
        fprintf(f, ";\n");
        fprintf(f, "#endif\n\n");
    }
}

void write_type_decl(FILE *f, type_t *t, const char *name)
{
  write_type_v(f, t, FALSE, TRUE, name);
}

void write_type_decl_left(FILE *f, type_t *t)
{
  write_type_left(f, t, NAME_DEFAULT, TRUE);
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
        if (type_is_alias( type )) type = type_alias_get_aliasee( type );
        else if (is_ptr( type )) type = type_pointer_get_ref( type );
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
        if (type_is_alias( type )) type = type_alias_get_aliasee( type );
        else if (is_ptr( type )) type = type_pointer_get_ref( type );
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
void check_for_additional_prototype_types(const var_list_t *list)
{
  const var_t *v;

  if (!list) return;
  LIST_FOR_EACH_ENTRY( v, list, const var_t, entry )
  {
    type_t *type = v->type;
    if (!type) continue;
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
        check_for_additional_prototype_types(vars);
      }

      if (type_is_alias(type))
        type = type_alias_get_aliasee(type);
      else if (is_ptr(type))
        type = type_pointer_get_ref(type);
      else if (is_array(type))
        type = type_array_get_element(type);
      else
        break;
    }
  }
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

static void write_typedef(FILE *header, type_t *type)
{
  fprintf(header, "typedef ");
  write_type_def_or_decl(header, type_alias_get_aliasee(type), FALSE, type->name);
  fprintf(header, ";\n");
}

int is_const_decl(const var_t *var)
{
  const type_t *t;
  /* strangely, MIDL accepts a const attribute on any pointer in the
  * declaration to mean that data isn't being instantiated. this appears
  * to be a bug, but there is no benefit to being incompatible with MIDL,
  * so we'll do the same thing */
  for (t = var->type; ; )
  {
    if (is_attr(t->attrs, ATTR_CONST))
      return TRUE;
    else if (is_ptr(t))
      t = type_pointer_get_ref(t);
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
    switch (v->stgclass)
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
    write_type_def_or_decl(header, v->type, FALSE, v->name);
    fprintf(header, ";\n\n");
  }
}

static void write_library(FILE *header, const typelib_t *typelib)
{
  const UUID *uuid = get_attrp(typelib->attrs, ATTR_UUID);
  fprintf(header, "\n");
  write_guid(header, "LIBID", typelib->name, uuid);
  fprintf(header, "\n");
}


const type_t* get_explicit_generic_handle_type(const var_t* var)
{
    const type_t *t;
    for (t = var->type;
         is_ptr(t) || type_is_alias(t);
         t = type_is_alias(t) ? type_alias_get_aliasee(t) : type_pointer_get_ref(t))
        if ((type_get_type_detect_alias(t) != TYPE_BASIC || type_basic_get_type(t) != TYPE_BASIC_HANDLE) &&
            is_attr(t->attrs, ATTR_HANDLE))
            return t;
    return NULL;
}

const var_t *get_func_handle_var( const type_t *iface, const var_t *func,
                                  unsigned char *explicit_fc, unsigned char *implicit_fc )
{
    const var_t *var;
    const var_list_t *args = type_get_function_args( func->type );

    *explicit_fc = *implicit_fc = 0;
    if (args) LIST_FOR_EACH_ENTRY( var, args, const var_t, entry )
    {
        if (!is_attr( var->attrs, ATTR_IN ) && is_attr( var->attrs, ATTR_OUT )) continue;
        if (type_get_type( var->type ) == TYPE_BASIC && type_basic_get_type( var->type ) == TYPE_BASIC_HANDLE)
        {
            *explicit_fc = RPC_FC_BIND_PRIMITIVE;
            return var;
        }
        if (get_explicit_generic_handle_type( var ))
        {
            *explicit_fc = RPC_FC_BIND_GENERIC;
            return var;
        }
        if (is_context_handle( var->type ))
        {
            *explicit_fc = RPC_FC_BIND_CONTEXT;
            return var;
        }
    }

    if ((var = get_attrp( iface->attrs, ATTR_IMPLICIT_HANDLE )))
    {
        if (type_get_type( var->type ) == TYPE_BASIC &&
            type_basic_get_type( var->type ) == TYPE_BASIC_HANDLE)
            *implicit_fc = RPC_FC_BIND_PRIMITIVE;
        else
            *implicit_fc = RPC_FC_BIND_GENERIC;
        return var;
    }

    *implicit_fc = RPC_FC_AUTO_HANDLE;
    return NULL;
}

int has_out_arg_or_return(const var_t *func)
{
    const var_t *var;

    if (!is_void(type_function_get_rettype(func->type)))
        return 1;

    if (!type_get_function_args(func->type))
        return 0;

    LIST_FOR_EACH_ENTRY( var, type_get_function_args(func->type), const var_t, entry )
        if (is_attr(var->attrs, ATTR_OUT))
            return 1;

    return 0;
}


/********** INTERFACES **********/

int is_object(const type_t *iface)
{
    const attr_t *attr;
    if (type_is_defined(iface) && type_iface_get_inherit(iface))
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
  enum type_type type = type_get_type(type_function_get_rettype(func->type));
  return type == TYPE_STRUCT || type == TYPE_UNION ||
         type == TYPE_COCLASS || type == TYPE_INTERFACE;
}

static char *get_vtbl_entry_name(const type_t *iface, const var_t *func)
{
  static char buff[255];
  if (is_inherited_method(iface, func))
    sprintf(buff, "%s_%s", iface->name, get_name(func));
  else
    sprintf(buff, "%s", get_name(func));
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

    if (!is_callas(func->attrs) && !is_aggregate_return(func)) {
      const var_t *arg;

      fprintf(header, "#define %s_%s(This", name, get_name(func));
      if (type_get_function_args(func->type))
          LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), const var_t, entry )
              fprintf(header, ",%s", arg->name);
      fprintf(header, ") ");

      fprintf(header, "(This)->lpVtbl->%s(This", get_vtbl_entry_name(iface, func));
      if (type_get_function_args(func->type))
          LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), const var_t, entry )
              fprintf(header, ",%s", arg->name);
      fprintf(header, ")\n");
    }
  }
}

void write_args(FILE *h, const var_list_t *args, const char *name, int method, int do_indent)
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
    write_type_decl(h, arg->type, arg->name);
    if (method == 2) {
        const expr_t *expr = get_attrp(arg->attrs, ATTR_DEFAULTVALUE);
        if (expr) {
            const var_t *tail_arg;

            /* Output default value only if all following arguments also have default value. */
            LIST_FOR_EACH_ENTRY_REV( tail_arg, args, const var_t, entry ) {
                if(tail_arg == arg) {
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
      const char *callconv = get_attrp(func->type->attrs, ATTR_CALLCONV);
      if (!callconv) callconv = "STDMETHODCALLTYPE";
      indent(header, 0);
      fprintf(header, "virtual ");
      write_type_decl_left(header, type_function_get_rettype(func->type));
      fprintf(header, " %s %s(\n", callconv, get_name(func));
      write_args(header, type_get_function_args(func->type), iface->name, 2, TRUE);
      fprintf(header, ") = 0;\n");
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

      fprintf(header, "FORCEINLINE ");
      write_type_decl_left(header, type_function_get_rettype(func->type));
      fprintf(header, " %s_%s(", name, get_name(func));
      write_args(header, type_get_function_args(func->type), name, 1, FALSE);
      fprintf(header, ") {\n");
      ++indentation;
      if (!is_aggregate_return(func)) {
        indent(header, 0);
        fprintf(header, "%sThis->lpVtbl->%s(This",
                is_void(type_function_get_rettype(func->type)) ? "" : "return ",
                get_vtbl_entry_name(iface, func));
      } else {
        indent(header, 0);
        write_type_decl_left(header, type_function_get_rettype(func->type));
        fprintf(header, " __ret;\n");
        indent(header, 0);
        fprintf(header, "return *This->lpVtbl->%s(This,&__ret", get_vtbl_entry_name(iface, func));
      }
      if (type_get_function_args(func->type))
          LIST_FOR_EACH_ENTRY( arg, type_get_function_args(func->type), const var_t, entry )
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
  else if (type_iface_get_stmts(iface) == NULL)
  {
    fprintf(header, "#ifndef __cplusplus\n");
    indent(header, 0);
    fprintf(header, "char dummy;\n");
    fprintf(header, "#endif\n");
    fprintf(header, "\n");
    return;
  }

  STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
  {
    const var_t *func = stmt->u.var;
    if (first_iface) {
      indent(header, 0);
      fprintf(header, "/*** %s methods ***/\n", iface->name);
      first_iface = 0;
    }
    if (!is_callas(func->attrs)) {
      const char *callconv = get_attrp(func->type->attrs, ATTR_CALLCONV);
      if (!callconv) callconv = "STDMETHODCALLTYPE";
      indent(header, 0);
      write_type_decl_left(header, type_function_get_rettype(func->type));
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
        write_type_decl_left(header, type_function_get_rettype(func->type));
        fprintf(header, " *__ret");
      }
      --indentation;
      if (type_get_function_args(func->type)) {
        fprintf(header, ",\n");
        write_args(header, type_get_function_args(func->type), name, 0, TRUE);
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

    if (!is_local(func->attrs)) {
      const char *callconv = get_attrp(func->type->attrs, ATTR_CALLCONV);
      if (!callconv) callconv = "STDMETHODCALLTYPE";
      /* proxy prototype */
      write_type_decl_left(header, type_function_get_rettype(func->type));
      fprintf(header, " %s %s_%s_Proxy(\n", callconv, iface->name, get_name(func));
      write_args(header, type_get_function_args(func->type), iface->name, 1, TRUE);
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
        if (!strcmp(stmt2->u.var->name, cas->name))
          break;
      if (&stmt2->entry != type_iface_get_stmts(iface)) {
        const var_t *m = stmt2->u.var;
        /* proxy prototype - use local prototype */
        write_type_decl_left(fp, type_function_get_rettype(m->type));
        fprintf(fp, " CALLBACK %s_%s_Proxy(\n", iface->name, get_name(m));
        write_args(fp, type_get_function_args(m->type), iface->name, 1, TRUE);
        fprintf(fp, ")");
        if (body) {
          type_t *rt = type_function_get_rettype(m->type);
          fprintf(fp, "\n{\n");
          fprintf(fp, "    %s\n", comment);
          if (rt->name && strcmp(rt->name, "HRESULT") == 0)
            fprintf(fp, "    return E_NOTIMPL;\n");
          else if (type_get_type(rt) != TYPE_VOID) {
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
        write_type_decl_left(fp, type_function_get_rettype(func->type));
        fprintf(fp, " __RPC_STUB %s_%s_Stub(\n", iface->name, get_name(m));
        write_args(fp, type_get_function_args(func->type), iface->name, 1, TRUE);
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
  const char *callconv = get_attrp(fun->type->attrs, ATTR_CALLCONV);

  if (!callconv) callconv = "__cdecl";
  /* FIXME: do we need to handle call_as? */
  write_type_decl_left(header, type_function_get_rettype(fun->type));
  fprintf(header, " %s ", callconv);
  fprintf(header, "%s%s(\n", prefix, get_name(fun));
  if (type_get_function_args(fun->type))
    write_args(header, type_get_function_args(fun->type), iface->name, 0, TRUE);
  else
    fprintf(header, "    void");
  fprintf(header, ");\n\n");
}

static void write_forward(FILE *header, type_t *iface)
{
  fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", iface->c_name);
  fprintf(header, "#define __%s_FWD_DEFINED__\n", iface->c_name);
  fprintf(header, "typedef interface %s %s;\n", iface->c_name, iface->c_name);
  fprintf(header, "#ifdef __cplusplus\n");
  write_namespace_start(header, iface->namespace);
  write_line(header, 0, "interface %s;", iface->name);
  write_namespace_end(header, iface->namespace);
  fprintf(header, "#endif /* __cplusplus */\n");
  fprintf(header, "#endif\n\n" );
}

static void write_com_interface_start(FILE *header, const type_t *iface)
{
  int dispinterface = is_attr(iface->attrs, ATTR_DISPINTERFACE);
  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s %sinterface\n", iface->name, dispinterface ? "disp" : "");
  fprintf(header, " */\n");
  fprintf(header,"#ifndef __%s_%sINTERFACE_DEFINED__\n", iface->c_name, dispinterface ? "DISP" : "");
  fprintf(header,"#define __%s_%sINTERFACE_DEFINED__\n\n", iface->c_name, dispinterface ? "DISP" : "");
}

static void write_com_interface_end(FILE *header, type_t *iface)
{
  int dispinterface = is_attr(iface->attrs, ATTR_DISPINTERFACE);
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
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
      write_line(header, 0, "MIDL_INTERFACE(\"%s\")", uuid_string(uuid));
      indent(header, 0);
  }else {
      indent(header, 0);
      fprintf(header, "interface ");
  }
  if (type_iface_get_inherit(iface))
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
  if (!dispinterface)
    write_cpp_method_def(header, iface);
  if (!type_iface_get_inherit(iface))
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
  fprintf(header,"#endif  /* __%s_%sINTERFACE_DEFINED__ */\n\n", iface->c_name, dispinterface ? "DISP" : "");
}

static void write_rpc_interface_start(FILE *header, const type_t *iface)
{
  unsigned int ver = get_attrv(iface->attrs, ATTR_VERSION);
  const var_t *var = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s interface (v%d.%d)\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
  fprintf(header, " */\n");
  fprintf(header,"#ifndef __%s_INTERFACE_DEFINED__\n", iface->name);
  fprintf(header,"#define __%s_INTERFACE_DEFINED__\n\n", iface->name);
  if (var)
  {
      fprintf(header, "extern ");
      write_type_decl( header, var->type, var->name );
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
  fprintf(header,"\n#endif  /* __%s_INTERFACE_DEFINED__ */\n\n", iface->name);
}

static void write_coclass(FILE *header, type_t *cocl)
{
  const UUID *uuid = get_attrp(cocl->attrs, ATTR_UUID);

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

static void write_import(FILE *header, const char *fname)
{
  char *hname, *p;

  hname = dup_basename(fname, ".idl");
  p = hname + strlen(hname) - 2;
  if (p <= hname || strcmp( p, ".h" )) strcat(hname, ".h");

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
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE)
        {
          if (is_object(stmt->u.type) || is_attr(stmt->u.type->attrs, ATTR_DISPINTERFACE))
            write_forward(header, stmt->u.type);
        }
        else if (type_get_type(stmt->u.type) == TYPE_COCLASS)
          write_coclass_forward(header, stmt->u.type);
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
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE)
        {
          type_t *iface = stmt->u.type;
          if (is_object(iface)) is_object_interface++;
          if (is_attr(stmt->u.type->attrs, ATTR_DISPINTERFACE) || is_object(stmt->u.type))
          {
            write_com_interface_start(header, iface);
            write_header_stmts(header, type_iface_get_stmts(iface), stmt->u.type, TRUE);
            write_com_interface_end(header, iface);
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
        else
        {
          write_type_definition(header, stmt->u.type);
        }
        break;
      case STMT_TYPEREF:
        /* FIXME: shouldn't write out forward declarations for undefined
        * interfaces but a number of our IDL files depend on this */
        if (type_get_type(stmt->u.type) == TYPE_INTERFACE && !stmt->u.type->written)
          write_forward(header, stmt->u.type);
        break;
      case STMT_IMPORTLIB:
      case STMT_MODULE:
      case STMT_PRAGMA:
        /* not included in header */
        break;
      case STMT_IMPORT:
        /* not processed here */
        break;
      case STMT_TYPEDEF:
      {
        const type_list_t *type_entry = stmt->u.type_list;
        for (; type_entry; type_entry = type_entry->next)
	  write_typedef(header, type_entry->type);
        break;
      }
      case STMT_LIBRARY:
        write_library(header, stmt->u.lib);
        write_header_stmts(header, stmt->u.lib->stmts, NULL, FALSE);
        break;
      case STMT_CPPQUOTE:
        fprintf(header, "%s\n", stmt->u.str);
        break;
      case STMT_DECLARATION:
        if (iface && type_get_type(stmt->u.var->type) == TYPE_FUNCTION)
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

  fprintf(header, "#ifndef __REQUIRED_RPCNDR_H_VERSION__\n");
  fprintf(header, "#define __REQUIRED_RPCNDR_H_VERSION__ 475\n");
  fprintf(header, "#endif\n\n");

  fprintf(header, "#ifdef __REACTOS__\n");
  fprintf(header, "#define WIN32_LEAN_AND_MEAN\n");
  fprintf(header, "#endif\n\n");

  fprintf(header, "#include <rpc.h>\n" );
  fprintf(header, "#include <rpcndr.h>\n\n" );

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
