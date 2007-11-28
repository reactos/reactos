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

static int indentation = 0;

static void indent(FILE *h, int delta)
{
  int c;
  if (delta < 0) indentation += delta;
  for (c=0; c<indentation; c++) fprintf(h, "    ");
  if (delta > 0) indentation += delta;
}

int is_attr(const attr_t *a, enum attr_type t)
{
  while (a) {
    if (a->type == t) return 1;
    a = NEXT_LINK(a);
  }
  return 0;
}

void *get_attrp(const attr_t *a, enum attr_type t)
{
  while (a) {
    if (a->type == t) return a->u.pval;
    a = NEXT_LINK(a);
  }
  return NULL;
}

unsigned long get_attrv(const attr_t *a, enum attr_type t)
{
  while (a) {
    if (a->type == t) return a->u.ival;
    a = NEXT_LINK(a);
  }
  return 0;
}

int is_void(const type_t *t, const var_t *v)
{
  if (v && v->ptr_level) return 0;
  if (!t->type && !t->ref) return 1;
  return 0;
}

static void write_guid(const char *guid_prefix, const char *name, const UUID *uuid)
{
  if (!uuid) return;
  fprintf(header, "DEFINE_GUID(%s_%s, 0x%08lx, 0x%04x, 0x%04x, 0x%02x,0x%02x, 0x%02x,"
        "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x);\n",
        guid_prefix, name, uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0],
        uuid->Data4[1], uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5],
        uuid->Data4[6], uuid->Data4[7]);
}

static void write_pident(FILE *h, const var_t *v)
{
  int c;
  for (c=0; c<v->ptr_level; c++) {
    fprintf(h, "*");
  }
  if (v->name) fprintf(h, "%s", v->name);
}

void write_name(FILE *h, const var_t *v)
{
  if (is_attr( v->attrs, ATTR_PROPGET ))
    fprintf(h, "get_" );
  else if (is_attr( v->attrs, ATTR_PROPPUT ))
    fprintf(h, "put_" );
  else if (is_attr( v->attrs, ATTR_PROPPUTREF ))
    fprintf(h, "putref_" );
  fprintf(h, "%s", v->name);
}

const char* get_name(const var_t *v)
{
  return v->name;
}

void write_array(FILE *h, const expr_t *v, int field)
{
  if (!v) return;
  while (NEXT_LINK(v)) v = NEXT_LINK(v);
  fprintf(h, "[");
  while (v) {
    if (v->is_const)
      fprintf(h, "%ld", v->cval); /* statically sized array */
    else
      if (field) fprintf(h, "1"); /* dynamically sized array */
    if (PREV_LINK(v))
      fprintf(h, ", ");
    v = PREV_LINK(v);
  }
  fprintf(h, "]");
}

static void write_field(FILE *h, var_t *v)
{
  if (!v) return;
  if (v->type) {
    indent(h, 0);
    write_type(h, v->type, NULL, v->tname);
    if (get_name(v)) {
      fprintf(h, " ");
      write_pident(h, v);
    }
    else {
      /* not all C/C++ compilers support anonymous structs and unions */
      switch (v->type->type) {
      case RPC_FC_STRUCT:
      case RPC_FC_CVSTRUCT:
      case RPC_FC_CPSTRUCT:
      case RPC_FC_CSTRUCT:
      case RPC_FC_PSTRUCT:
      case RPC_FC_BOGUS_STRUCT:
      case RPC_FC_ENCAPSULATED_UNION:
        fprintf(h, " DUMMYSTRUCTNAME");
        break;
      case RPC_FC_NON_ENCAPSULATED_UNION:
        fprintf(h, " DUMMYUNIONNAME");
        break;
      default:
        /* ? */
        break;
      }
    }
    write_array(h, v->array, 1);
    fprintf(h, ";\n");
  }
}

static void write_fields(FILE *h, var_t *v)
{
  var_t *first = v;
  if (!v) return;
  while (NEXT_LINK(v)) v = NEXT_LINK(v);
  while (v) {
    write_field(h, v);
    if (v == first) break;
    v = PREV_LINK(v);
  }
}

static void write_enums(FILE *h, var_t *v)
{
  if (!v) return;
  while (NEXT_LINK(v)) v = NEXT_LINK(v);
  while (v) {
    if (get_name(v)) {
      indent(h, 0);
      write_name(h, v);
      if (v->eval) {
        fprintf(h, " = ");
        write_expr(h, v->eval, 0);
      }
    }
    if (PREV_LINK(v))
      fprintf(h, ",\n");
    v = PREV_LINK(v);
  }
  fprintf(h, "\n");
}

void write_type(FILE *h, type_t *t, const var_t *v, const char *n)
{
  int c;

  if (n) fprintf(h, "%s", n);
  else {
    if (t->is_const) fprintf(h, "const ");
    if (t->type) {
      if (t->sign > 0) fprintf(h, "signed ");
      else if (t->sign < 0) fprintf(h, "unsigned ");
      switch (t->type) {
      case RPC_FC_BYTE:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "byte");
        break;
      case RPC_FC_CHAR:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "char");
        break;
      case RPC_FC_WCHAR:
        fprintf(h, "WCHAR");
        break;
      case RPC_FC_USMALL:
      case RPC_FC_SMALL:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "small");
        break;
      case RPC_FC_USHORT:
      case RPC_FC_SHORT:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "short");
        break;
      case RPC_FC_ULONG:
      case RPC_FC_LONG:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "long");
        break;
      case RPC_FC_HYPER:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "hyper");
        break;
      case RPC_FC_FLOAT:
        fprintf(h, "float");
        break;
      case RPC_FC_DOUBLE:
        fprintf(h, "double");
        break;
      case RPC_FC_ENUM16:
      case RPC_FC_ENUM32:
        if (t->defined && !t->written && !t->ignore) {
          if (t->name) fprintf(h, "enum %s {\n", t->name);
          else fprintf(h, "enum {\n");
          t->written = TRUE;
          indentation++;
          write_enums(h, t->fields);
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "enum %s", t->name);
        break;
      case RPC_FC_ERROR_STATUS_T:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "error_status_t");
        break;
      case RPC_FC_BIND_PRIMITIVE:
        if (t->ref) fprintf(h, t->ref->name);
        else fprintf(h, "handle_t");
        break;
      case RPC_FC_STRUCT:
      case RPC_FC_CVSTRUCT:
      case RPC_FC_CPSTRUCT:
      case RPC_FC_CSTRUCT:
      case RPC_FC_PSTRUCT:
      case RPC_FC_BOGUS_STRUCT:
      case RPC_FC_ENCAPSULATED_UNION:
        if (t->defined && !t->written && !t->ignore) {
          if (t->name) fprintf(h, "struct %s {\n", t->name);
          else fprintf(h, "struct {\n");
          t->written = TRUE;
          indentation++;
          write_fields(h, t->fields);
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "struct %s", t->name);
        break;
      case RPC_FC_NON_ENCAPSULATED_UNION:
        if (t->defined && !t->written && !t->ignore) {
          if (t->name) fprintf(h, "union %s {\n", t->name);
          else fprintf(h, "union {\n");
          t->written = TRUE;
          indentation++;
          write_fields(h, t->fields);
          indent(h, -1);
          fprintf(h, "}");
        }
        else fprintf(h, "union %s", t->name);
        break;
      case RPC_FC_FP:
        if (t->ref) write_type(h, t->ref, NULL, t->name);
        fprintf(h, "*");
        break;
      default:
        fprintf(h, "(unknown-type:%d)", t->type);
      }
    }
    else {
      if (t->ref) {
        write_type(h, t->ref, NULL, t->name);
      }
      else fprintf(h, "void");
    }
  }
  if (v) {
    for (c=0; c<v->ptr_level; c++) {
      fprintf(h, "*");
    }
  }
}


struct user_type
{
    struct user_type *next;
    char name[1];
};

static struct user_type *user_type_list;

static int user_type_registered(const char *name)
{
  struct user_type *ut;
  for (ut = user_type_list; ut; ut = ut->next)
    if (!strcmp(name, ut->name))
        return 1;
  return 0;
}

static void check_for_user_types(const var_t *v)
{
  while (v) {
    type_t *type = v->type;
    const char *name = v->tname;
    for (type = v->type; type; type = type->ref) {
      if (type->user_types_registered) continue;
      type->user_types_registered = 1;
      if (is_attr(type->attrs, ATTR_WIREMARSHAL)) {
        if (!user_type_registered(name))
        {
          struct user_type *ut = xmalloc(sizeof(struct user_type) + strlen(name));
          strcpy(ut->name, name);
          ut->next = user_type_list;
          user_type_list = ut;
        }
        /* don't carry on parsing fields within this type as we are already
         * using a wire marshaled type */
        break;
      }
      else if (type->fields)
      {
        const var_t *fields = type->fields;
        while (NEXT_LINK(fields)) fields = NEXT_LINK(fields);
        check_for_user_types(fields);
      }
      /* the wire_marshal attribute is always at least one reference away
       * from the name of the type, so update it after the rest of the
       * processing above */
      if (type->name) name = type->name;
    }
    v = PREV_LINK(v);
  }
}

void write_user_types(void)
{
  struct user_type *ut;
  for (ut = user_type_list; ut; ut = ut->next)
  {
    const char *name = ut->name;
    fprintf(header, "unsigned long   __RPC_USER %s_UserSize     (unsigned long *, unsigned long,   %s *);\n", name, name);
    fprintf(header, "unsigned char * __RPC_USER %s_UserMarshal  (unsigned long *, unsigned char *, %s *);\n", name, name);
    fprintf(header, "unsigned char * __RPC_USER %s_UserUnmarshal(unsigned long *, unsigned char *, %s *);\n", name, name);
    fprintf(header, "void            __RPC_USER %s_UserFree     (unsigned long *, %s *);\n", name, name);
  }
}

void write_typedef(type_t *type, const var_t *names)
{
  const char *tname = names->tname;
  const var_t *lname;
  while (NEXT_LINK(names)) names = NEXT_LINK(names);
  lname = names;
  fprintf(header, "typedef ");
  write_type(header, type, NULL, tname);
  fprintf(header, " ");
  while (names) {
    write_pident(header, names);
    if (PREV_LINK(names))
      fprintf(header, ", ");
    names = PREV_LINK(names);
  }
  fprintf(header, ";\n");
}

void write_expr(FILE *h, const expr_t *e, int brackets)
{
  switch (e->type) {
  case EXPR_VOID:
    break;
  case EXPR_NUM:
    fprintf(h, "%ld", e->u.lval);
    break;
  case EXPR_HEXNUM:
    fprintf(h, "0x%lx", e->u.lval);
    break;
  case EXPR_TRUEFALSE:
    if (e->u.lval == 0)
      fprintf(h, "FALSE");
    else
      fprintf(h, "TRUE");
    break;
  case EXPR_IDENTIFIER:
    fprintf(h, "%s", e->u.sval);
    break;
  case EXPR_NEG:
    fprintf(h, "-");
    write_expr(h, e->ref, 1);
    break;
  case EXPR_NOT:
    fprintf(h, "~");
    write_expr(h, e->ref, 1);
    break;
  case EXPR_PPTR:
    fprintf(h, "*");
    write_expr(h, e->ref, 1);
    break;
  case EXPR_CAST:
    fprintf(h, "(");
    write_type(h, e->u.tref->ref, NULL, e->u.tref->name);
    fprintf(h, ")");
    write_expr(h, e->ref, 1);
    break;
  case EXPR_SIZEOF:
    fprintf(h, "sizeof(");
    write_type(h, e->u.tref->ref, NULL, e->u.tref->name);
    fprintf(h, ")");
    break;
  case EXPR_SHL:
  case EXPR_SHR:
  case EXPR_MUL:
  case EXPR_DIV:
  case EXPR_ADD:
  case EXPR_SUB:
  case EXPR_AND:
  case EXPR_OR:
    if (brackets) fprintf(h, "(");
    write_expr(h, e->ref, 1);
    switch (e->type) {
    case EXPR_SHL: fprintf(h, " << "); break;
    case EXPR_SHR: fprintf(h, " >> "); break;
    case EXPR_MUL: fprintf(h, " * "); break;
    case EXPR_DIV: fprintf(h, " / "); break;
    case EXPR_ADD: fprintf(h, " + "); break;
    case EXPR_SUB: fprintf(h, " - "); break;
    case EXPR_AND: fprintf(h, " & "); break;
    case EXPR_OR:  fprintf(h, " | "); break;
    default: break;
    }
    write_expr(h, e->u.ext, 1);
    if (brackets) fprintf(h, ")");
    break;
  case EXPR_COND:
    if (brackets) fprintf(h, "(");
    write_expr(h, e->ref, 1);
    fprintf(h, " ? ");
    write_expr(h, e->u.ext, 1);
    fprintf(h, " : ");
    write_expr(h, e->ext2, 1);
    if (brackets) fprintf(h, ")");
    break;
  }
}

void write_constdef(const var_t *v)
{
  fprintf(header, "#define %s (", get_name(v));
  write_expr(header, v->eval, 0);
  fprintf(header, ")\n\n");
}

void write_externdef(const var_t *v)
{
  fprintf(header, "extern const ");
  write_type(header, v->type, NULL, v->tname);
  if (get_name(v)) {
    fprintf(header, " ");
    write_pident(header, v);
  }
  fprintf(header, ";\n\n");
}

void write_library(const char *name, const attr_t *attr) {
  const UUID *uuid = get_attrp(attr, ATTR_UUID);
  fprintf(header, "\n");
  write_guid("LIBID", name, uuid);
  fprintf(header, "\n");
}


const var_t* get_explicit_handle_var(const func_t* func)
{
    const var_t* var;

    if (!func->args)
        return NULL;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        if (var->type->type == RPC_FC_BIND_PRIMITIVE)
            return var;

        var = PREV_LINK(var);
    }

    return NULL;
}

int has_out_arg_or_return(const func_t *func)
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


/********** INTERFACES **********/

int is_object(const attr_t *a)
{
  while (a) {
    if (a->type == ATTR_OBJECT || a->type == ATTR_ODL) return 1;
    a = NEXT_LINK(a);
  }
  return 0;
}

int is_local(const attr_t *a)
{
  return is_attr(a, ATTR_LOCAL);
}

const var_t *is_callas(const attr_t *a)
{
  return get_attrp(a, ATTR_CALLAS);
}

static int write_method_macro(const type_t *iface, const char *name)
{
  int idx;
  func_t *cur = iface->funcs;

  if (iface->ref) idx = write_method_macro(iface->ref, name);
  else idx = 0;

  if (!cur) return idx;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);

  fprintf(header, "/*** %s methods ***/\n", iface->name);
  while (cur) {
    var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      var_t *arg = cur->args;
      int argc = 0;
      int c;
      while (arg) {
	arg = NEXT_LINK(arg);
	argc++;
      }

      fprintf(header, "#define %s_", name);
      write_name(header,def);
      fprintf(header, "(p");
      for (c=0; c<argc; c++)
	fprintf(header, ",%c", c+'a');
      fprintf(header, ") ");

      fprintf(header, "(p)->lpVtbl->");
      write_name(header, def);
      fprintf(header, "(p");
      for (c=0; c<argc; c++)
	fprintf(header, ",%c", c+'a');
      fprintf(header, ")\n");
      if (cur->idx == -1) cur->idx = idx;
      else if (cur->idx != idx) yyerror("BUG: method index mismatch in write_method_macro");
      idx++;
    }
    cur = PREV_LINK(cur);
  }
  return idx;
}

void write_args(FILE *h, var_t *arg, const char *name, int method, int do_indent)
{
  int count = 0;
  if (arg) {
    while (NEXT_LINK(arg))
      arg = NEXT_LINK(arg);
  }
  if (do_indent)
  {
      indentation++;
      indent(h, 0);
  }
  if (method == 1) {
    fprintf(h, "%s* This", name);
    count++;
  }
  while (arg) {
    if (count) {
        if (do_indent)
        {
            fprintf(h, ",\n");
            indent(h, 0);
        }
        else fprintf(h, ",");
    }
    write_type(h, arg->type, arg, arg->tname);
    if (arg->args)
    {
      fprintf(h, " (STDMETHODCALLTYPE *");
      write_name(h,arg);
      fprintf(h, ")(");
      write_args(h, arg->args, NULL, 0, FALSE);
      fprintf(h, ")");
    }
    else
    {
      fprintf(h, " ");
      write_name(h, arg);
    }
    write_array(h, arg->array, 0);
    arg = PREV_LINK(arg);
    count++;
  }
  if (do_indent) indentation--;
}

static void write_cpp_method_def(const type_t *iface)
{
  func_t *cur = iface->funcs;

  if (!cur) return;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  while (cur) {
    var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      indent(header, 0);
      fprintf(header, "virtual ");
      write_type(header, def->type, def, def->tname);
      fprintf(header, " STDMETHODCALLTYPE ");
      write_name(header, def);
      fprintf(header, "(\n");
      write_args(header, cur->args, iface->name, 2, TRUE);
      fprintf(header, ") = 0;\n");
      fprintf(header, "\n");
    }
    cur = PREV_LINK(cur);
  }
}

static void do_write_c_method_def(const type_t *iface, const char *name)
{
  const func_t *cur = iface->funcs;

  if (iface->ref) do_write_c_method_def(iface->ref, name);

  if (!cur) return;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  indent(header, 0);
  fprintf(header, "/*** %s methods ***/\n", iface->name);
  while (cur) {
    const var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      indent(header, 0);
      write_type(header, def->type, def, def->tname);
      fprintf(header, " (STDMETHODCALLTYPE *");
      write_name(header, def);
      fprintf(header, ")(\n");
      write_args(header, cur->args, name, 1, TRUE);
      fprintf(header, ");\n");
      fprintf(header, "\n");
    }
    cur = PREV_LINK(cur);
  }
}

static void write_c_method_def(const type_t *iface)
{
  do_write_c_method_def(iface, iface->name);
}

static void write_c_disp_method_def(const type_t *iface)
{
  do_write_c_method_def(iface->ref, iface->name);
}

static void write_method_proto(const type_t *iface)
{
  const func_t *cur = iface->funcs;

  if (!cur) return;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  while (cur) {
    const var_t *def = cur->def;
    const var_t *cas = is_callas(def->attrs);
    const var_t *args;
    if (!is_local(def->attrs)) {
      /* proxy prototype */
      write_type(header, def->type, def, def->tname);
      fprintf(header, " CALLBACK %s_", iface->name);
      write_name(header, def);
      fprintf(header, "_Proxy(\n");
      write_args(header, cur->args, iface->name, 1, TRUE);
      fprintf(header, ");\n");
      /* stub prototype */
      fprintf(header, "void __RPC_STUB %s_", iface->name);
      write_name(header,def);
      fprintf(header, "_Stub(\n");
      fprintf(header, "    IRpcStubBuffer* This,\n");
      fprintf(header, "    IRpcChannelBuffer* pRpcChannelBuffer,\n");
      fprintf(header, "    PRPC_MESSAGE pRpcMessage,\n");
      fprintf(header, "    DWORD* pdwStubPhase);\n");

      args = cur->args;
      if (args) {
        while (NEXT_LINK(args))
          args = NEXT_LINK(args);
      }
      check_for_user_types(args);
    }
    if (cas) {
      const func_t *m = iface->funcs;
      while (m && strcmp(get_name(m->def), cas->name))
        m = NEXT_LINK(m);
      if (m) {
        const var_t *mdef = m->def;
        /* proxy prototype - use local prototype */
        write_type(header, mdef->type, mdef, mdef->tname);
        fprintf(header, " CALLBACK %s_", iface->name);
        write_name(header, mdef);
        fprintf(header, "_Proxy(\n");
        write_args(header, m->args, iface->name, 1, TRUE);
        fprintf(header, ");\n");
        /* stub prototype - use remotable prototype */
        write_type(header, def->type, def, def->tname);
        fprintf(header, " __RPC_STUB %s_", iface->name);
        write_name(header, mdef);
        fprintf(header, "_Stub(\n");
        write_args(header, cur->args, iface->name, 1, TRUE);
        fprintf(header, ");\n");
      }
      else {
        yywarning("invalid call_as attribute (%s -> %s)\n", get_name(def), cas->name);
      }
    }

    cur = PREV_LINK(cur);
  }
}

static void write_function_proto(const type_t *iface)
{
  const char *implicit_handle = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
  int explicit_handle = is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE);
  const var_t* explicit_handle_var;

  func_t *cur = iface->funcs;
  while (NEXT_LINK(cur)) cur = NEXT_LINK(cur);
  while (cur) {
    var_t *def = cur->def;

    /* check for a defined binding handle */
    explicit_handle_var = get_explicit_handle_var(cur);
    if (explicit_handle) {
      if (!explicit_handle_var) {
        error("%s() does not define an explicit binding handle!\n", def->name);
        return;
      }
    } else if (implicit_handle) {
      if (explicit_handle_var) {
        error("%s() must not define a binding handle!\n", def->name);
        return;
      }
    }

    /* FIXME: do we need to handle call_as? */
    write_type(header, def->type, def, def->tname);
    fprintf(header, " ");
    write_name(header, def);
    fprintf(header, "(\n");
    if (cur->args)
      write_args(header, cur->args, iface->name, 0, TRUE);
    else
      fprintf(header, "    void");
    fprintf(header, ");\n");

    cur = PREV_LINK(cur);
  }
}

void write_forward(type_t *iface)
{
  /* C/C++ forwards should only be written for object interfaces, so if we
   * have a full definition we only write one if we find [object] among the
   * attributes - however, if we don't have a full definition at this point
   * (i.e. this is an IDL forward), then we also assume that it is an object
   * interface, since non-object interfaces shouldn't need forwards */
  if ((!iface->defined || is_object(iface->attrs) || is_attr(iface->attrs, ATTR_DISPINTERFACE))
        && !iface->written) {
    fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", iface->name);
    fprintf(header, "#define __%s_FWD_DEFINED__\n", iface->name);
    fprintf(header, "typedef interface %s %s;\n", iface->name, iface->name);
    fprintf(header, "#endif\n\n" );
    iface->written = TRUE;
  }
}

static void write_iface_guid(const type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid("IID", iface->name, uuid);
}

static void write_dispiface_guid(const type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid("DIID", iface->name, uuid);
}

static void write_coclass_guid(type_t *cocl)
{
  const UUID *uuid = get_attrp(cocl->attrs, ATTR_UUID);
  write_guid("CLSID", cocl->name, uuid);
}

static void write_com_interface(type_t *iface)
{
  if (!iface->funcs && !iface->ref) {
    yywarning("%s has no methods", iface->name);
    return;
  }

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s interface\n", iface->name);
  fprintf(header, " */\n");
  fprintf(header,"#ifndef __%s_INTERFACE_DEFINED__\n", iface->name);
  fprintf(header,"#define __%s_INTERFACE_DEFINED__\n\n", iface->name);
  write_iface_guid(iface);
  write_forward(iface);
  /* C++ interface */
  fprintf(header, "#if defined(__cplusplus) && !defined(CINTERFACE)\n");
  if (iface->ref)
  {
      fprintf(header, "interface %s : public %s\n", iface->name, iface->ref->name);
      fprintf(header, "{\n");
      indentation++;
      write_cpp_method_def(iface);
      indentation--;
      fprintf(header, "};\n");
  }
  else
  {
      fprintf(header, "interface %s\n", iface->name);
      fprintf(header, "{\n");
      fprintf(header, "    BEGIN_INTERFACE\n");
      fprintf(header, "\n");
      indentation++;
      write_cpp_method_def(iface);
      indentation--;
      fprintf(header, "    END_INTERFACE\n");
      fprintf(header, "};\n");
  }
  fprintf(header, "#else\n");
  /* C interface */
  fprintf(header, "typedef struct %sVtbl {\n", iface->name);
  indentation++;
  fprintf(header, "    BEGIN_INTERFACE\n");
  fprintf(header, "\n");
  write_c_method_def(iface);
  indentation--;
  fprintf(header, "    END_INTERFACE\n");
  fprintf(header, "} %sVtbl;\n", iface->name);
  fprintf(header, "interface %s {\n", iface->name);
  fprintf(header, "    const %sVtbl* lpVtbl;\n", iface->name);
  fprintf(header, "};\n");
  fprintf(header, "\n");
  fprintf(header, "#ifdef COBJMACROS\n");
  write_method_macro(iface, iface->name);
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  write_method_proto(iface);
  fprintf(header,"\n#endif  /* __%s_INTERFACE_DEFINED__ */\n\n", iface->name);
}

static void write_rpc_interface(const type_t *iface)
{
  unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
  const char *var = get_attrp(iface->attrs, ATTR_IMPLICIT_HANDLE);
  static int allocate_written = 0;

  if (!allocate_written)
  {
    allocate_written = 1;
    fprintf(header, "void * __RPC_USER MIDL_user_allocate(size_t);\n");
    fprintf(header, "void __RPC_USER MIDL_user_free(void *);\n\n");
  }

  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s interface (v%d.%d)\n", iface->name, LOWORD(ver), HIWORD(ver));
  fprintf(header, " */\n");
  fprintf(header,"#ifndef __%s_INTERFACE_DEFINED__\n", iface->name);
  fprintf(header,"#define __%s_INTERFACE_DEFINED__\n\n", iface->name);
  if (iface->funcs)
  {
    write_iface_guid(iface);
    if (var) fprintf(header, "extern handle_t %s;\n", var);
    if (old_names)
    {
        fprintf(header, "extern RPC_IF_HANDLE %s_ClientIfHandle;\n", iface->name);
        fprintf(header, "extern RPC_IF_HANDLE %s_ServerIfHandle;\n", iface->name);
    }
    else
    {
        fprintf(header, "extern RPC_IF_HANDLE %s_v%d_%d_c_ifspec;\n", iface->name, LOWORD(ver), HIWORD(ver));
        fprintf(header, "extern RPC_IF_HANDLE %s_v%d_%d_s_ifspec;\n", iface->name, LOWORD(ver), HIWORD(ver));
    }
    write_function_proto(iface);
  }
  fprintf(header,"\n#endif  /* __%s_INTERFACE_DEFINED__ */\n\n", iface->name);

  /* FIXME: server/client code */
}

void write_interface(type_t *iface)
{
  if (is_object(iface->attrs))
    write_com_interface(iface);
  else
    write_rpc_interface(iface);
}

void write_dispinterface(type_t *iface)
{
  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s dispinterface\n", iface->name);
  fprintf(header, " */\n");
  fprintf(header,"#ifndef __%s_DISPINTERFACE_DEFINED__\n", iface->name);
  fprintf(header,"#define __%s_DISPINTERFACE_DEFINED__\n\n", iface->name);
  write_dispiface_guid(iface);
  write_forward(iface);
  /* C++ interface */
  fprintf(header, "#if defined(__cplusplus) && !defined(CINTERFACE)\n");
  fprintf(header, "interface %s : public %s\n", iface->name, iface->ref->name);
  fprintf(header, "{\n");
  fprintf(header, "};\n");
  fprintf(header, "#else\n");
  /* C interface */
  fprintf(header, "typedef struct %sVtbl {\n", iface->name);
  indentation++;
  fprintf(header, "    BEGIN_INTERFACE\n");
  fprintf(header, "\n");
  write_c_disp_method_def(iface);
  indentation--;
  fprintf(header, "    END_INTERFACE\n");
  fprintf(header, "} %sVtbl;\n", iface->name);
  fprintf(header, "interface %s {\n", iface->name);
  fprintf(header, "    const %sVtbl* lpVtbl;\n", iface->name);
  fprintf(header, "};\n");
  fprintf(header, "\n");
  fprintf(header, "#ifdef COBJMACROS\n");
  write_method_macro(iface->ref, iface->name);
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  fprintf(header, "#endif\n");
  fprintf(header, "\n");
  fprintf(header,"#endif  /* __%s_DISPINTERFACE_DEFINED__ */\n\n", iface->name);
}

void write_coclass(type_t *cocl)
{
  fprintf(header, "/*****************************************************************************\n");
  fprintf(header, " * %s coclass\n", cocl->name);
  fprintf(header, " */\n\n");
  write_coclass_guid(cocl);
  fprintf(header, "\n");
}

void write_coclass_forward(type_t *cocl)
{
  fprintf(header, "#ifndef __%s_FWD_DEFINED__\n", cocl->name);
  fprintf(header, "#define __%s_FWD_DEFINED__\n", cocl->name);
  fprintf(header, "typedef struct %s %s;\n", cocl->name, cocl->name);
  fprintf(header, "#endif /* defined __%s_FWD_DEFINED__ */\n\n", cocl->name );
}
