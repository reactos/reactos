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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WIDL_WIDLTYPES_H
#define __WIDL_WIDLTYPES_H

#include <stdarg.h>
#include "guiddef.h"
#include "wine/rpcfc.h"
#include "winglue.h"

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

#define TRUE 1
#define FALSE 0

typedef struct _attr_t attr_t;
typedef struct _expr_t expr_t;
typedef struct _type_t type_t;
typedef struct _typeref_t typeref_t;
typedef struct _var_t var_t;
typedef struct _func_t func_t;
typedef struct _ifref_t ifref_t;
typedef struct _class_t class_t;
typedef struct _typelib_entry_t typelib_entry_t;
typedef struct _typelib_t typelib_t;

#define DECL_LINK(type) \
  type *l_next; \
  type *l_prev;

#define LINK(x,y) do { x->l_next = y; if (y) y->l_prev = x; } while (0)

#define INIT_LINK(x) do { x->l_next = NULL; x->l_prev = NULL; } while (0)
#define NEXT_LINK(x) ((x)->l_next)
#define PREV_LINK(x) ((x)->l_prev)

enum attr_type
{
    ATTR_ASYNC,
    ATTR_AUTO_HANDLE,
    ATTR_CALLAS,
    ATTR_CASE,
    ATTR_CONTEXTHANDLE,
    ATTR_CONTROL,
    ATTR_DEFAULT,
    ATTR_DEFAULTVALUE_EXPR,
    ATTR_DEFAULTVALUE_STRING,
    ATTR_DLLNAME,
    ATTR_DUAL,
    ATTR_ENDPOINT,
    ATTR_ENTRY_STRING,
    ATTR_ENTRY_ORDINAL,
    ATTR_EXPLICIT_HANDLE,
    ATTR_HANDLE,
    ATTR_HELPCONTEXT,
    ATTR_HELPFILE,
    ATTR_HELPSTRING,
    ATTR_HELPSTRINGCONTEXT,
    ATTR_HELPSTRINGDLL,
    ATTR_HIDDEN,
    ATTR_ID,
    ATTR_IDEMPOTENT,
    ATTR_IIDIS,
    ATTR_IMPLICIT_HANDLE,
    ATTR_IN,
    ATTR_INPUTSYNC,
    ATTR_LENGTHIS,
    ATTR_LOCAL,
    ATTR_NONCREATABLE,
    ATTR_OBJECT,
    ATTR_ODL,
    ATTR_OLEAUTOMATION,
    ATTR_OPTIONAL,
    ATTR_OUT,
    ATTR_POINTERDEFAULT,
    ATTR_POINTERTYPE,
    ATTR_PROPGET,
    ATTR_PROPPUT,
    ATTR_PROPPUTREF,
    ATTR_PTR,
    ATTR_PUBLIC,
    ATTR_READONLY,
    ATTR_REF,
    ATTR_RESTRICTED,
    ATTR_RETVAL,
    ATTR_SIZEIS,
    ATTR_SOURCE,
    ATTR_STRING,
    ATTR_SWITCHIS,
    ATTR_SWITCHTYPE,
    ATTR_TRANSMITAS,
    ATTR_UNIQUE,
    ATTR_UUID,
    ATTR_V1ENUM,
    ATTR_VARARG,
    ATTR_VERSION,
    ATTR_WIREMARSHAL,
    ATTR_DISPINTERFACE
};

enum expr_type
{
    EXPR_VOID,
    EXPR_NUM,
    EXPR_HEXNUM,
    EXPR_IDENTIFIER,
    EXPR_NEG,
    EXPR_NOT,
    EXPR_PPTR,
    EXPR_CAST,
    EXPR_SIZEOF,
    EXPR_SHL,
    EXPR_SHR,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_AND,
    EXPR_OR,
    EXPR_COND,
};

enum type_kind
{
    TKIND_ENUM = 0,
    TKIND_RECORD,
    TKIND_MODULE,
    TKIND_INTERFACE,
    TKIND_DISPATCH,
    TKIND_COCLASS,
    TKIND_ALIAS,
    TKIND_UNION,
    TKIND_MAX
};

struct _attr_t {
  enum attr_type type;
  union {
    unsigned long ival;
    void *pval;
  } u;
  /* parser-internal */
  DECL_LINK(attr_t)
};

struct _expr_t {
  enum expr_type type;
  expr_t *ref;
  union {
    long lval;
    char *sval;
    expr_t *ext;
    typeref_t *tref;
  } u;
  expr_t *ext2;
  int is_const;
  long cval;
  /* parser-internal */
  DECL_LINK(expr_t)
};

struct _type_t {
  char *name;
  unsigned char type;
  struct _type_t *ref;
  char *rname;
  attr_t *attrs;
  func_t *funcs;
  var_t *fields;
  int ignore, is_const, sign;
  int defined, written;
  int typelib_idx;
  /* parser-internal */
  DECL_LINK(type_t)
};

struct _typeref_t {
  char *name;
  type_t *ref;
  int uniq;
};

struct _var_t {
  char *name;
  int ptr_level;
  expr_t *array;
  type_t *type;
  var_t *args;  /* for function pointers */
  char *tname;
  attr_t *attrs;
  expr_t *eval;
  long lval;

  /* parser-internal */
  DECL_LINK(var_t)
};

struct _func_t {
  var_t *def;
  var_t *args;
  int ignore, idx;

  /* parser-internal */
  DECL_LINK(func_t)
};

struct _ifref_t {
  type_t *iface;
  attr_t *attrs;

  /* parser-internal */
  DECL_LINK(ifref_t)
};

struct _class_t {
  char *name;
  attr_t *attrs;
  ifref_t *ifaces;

  /* parser-internal */
  DECL_LINK(class_t)
};

struct _typelib_entry_t {
    enum type_kind kind;
    union {
        class_t *class;
        type_t *interface;
        type_t *module;
        type_t *structure;
        type_t *enumeration;
        var_t *tdef;
    } u;
    DECL_LINK(typelib_entry_t)
};

struct _typelib_t {
    char *name;
    char *filename;
    attr_t *attrs;
    typelib_entry_t *entry;
};

#endif
