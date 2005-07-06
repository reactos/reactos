/*
 * IDL Compiler
 *
 * Copyright 2004 Ove Kaaven
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
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"

int in_typelib = 0;

static typelib_t *typelib;

/* List of oleauto types that should be recognized by name.
 * (most of) these seem to be intrinsic types in mktyplib. */

static struct oatype {
  const char *kw;
  unsigned short vt;
} oatypes[] = {
  {"BSTR",          VT_BSTR},
  {"CURRENCY",      VT_CY},
  {"DATE",          VT_DATE},
  {"DECIMAL",       VT_DECIMAL},
  {"HRESULT",       VT_HRESULT},
  {"LPSTR",         VT_LPSTR},
  {"LPWSTR",        VT_LPWSTR},
  {"SCODE",         VT_ERROR},
  {"VARIANT",       VT_VARIANT},
  {"VARIANT_BOOL",  VT_BOOL}
};
#define NTYPES (sizeof(oatypes)/sizeof(oatypes[0]))
#define KWP(p) ((const struct oatype *)(p))

static int kw_cmp_func(const void *s1, const void *s2)
{
        return strcmp(KWP(s1)->kw, KWP(s2)->kw);
}

static unsigned short builtin_vt(const char *kw)
{
  struct oatype key, *kwp;
  key.kw = kw;
#ifdef KW_BSEARCH
  kwp = bsearch(&key, oatypes, NTYPES, sizeof(oatypes[0]), kw_cmp_func);
#else
  {
    int i;
    for (kwp=NULL, i=0; i < NTYPES; i++)
      if (!kw_cmp_func(&key, &oatypes[i])) {
        kwp = &oatypes[i];
        break;
      }
  }
#endif
  if (kwp) {
    return kwp->vt;
  }
  return 0;
}

static int match(const char*n, const char*m)
{
  if (!n) return 0;
  return !strcmp(n, m);
}

unsigned short get_type_vt(type_t *t)
{
  unsigned short vt;

  chat("get_type_vt: %p type->name %s\n", t, t->name);
  if (t->name) {
    vt = builtin_vt(t->name);
    if (vt) return vt;
  }

  switch (t->type) {
  case RPC_FC_BYTE:
  case RPC_FC_USMALL:
    return VT_UI1;
  case RPC_FC_CHAR:
  case RPC_FC_SMALL:
    return VT_I1;
  case RPC_FC_WCHAR:
    return VT_I2; /* mktyplib seems to parse wchar_t as short */
  case RPC_FC_SHORT:
    return VT_I2;
  case RPC_FC_USHORT:
    return VT_UI2;
  case RPC_FC_LONG:
    if (t->ref && match(t->ref->name, "int")) return VT_INT;
    return VT_I4;
  case RPC_FC_ULONG:
    if (t->ref && match(t->ref->name, "int")) return VT_UINT;
    return VT_UI4;
  case RPC_FC_HYPER:
    if (t->sign < 0) return VT_UI8;
    if (t->ref && match(t->ref->name, "MIDL_uhyper")) return VT_UI8;
    return VT_I8;
  case RPC_FC_FLOAT:
    return VT_R4;
  case RPC_FC_DOUBLE:
    return VT_R8;
  case RPC_FC_RP:
  case RPC_FC_UP:
  case RPC_FC_OP:
  case RPC_FC_FP:
    if(t->ref)
      return VT_PTR;

    error("get_type_vt: unknown-deref-type: %d\n", t->ref->type);
    break;
  case RPC_FC_IP:
    if(match(t->name, "IUnknown"))
      return VT_UNKNOWN;
    if(match(t->name, "IDispatch"))
      return VT_DISPATCH;
    return VT_USERDEFINED;

  case RPC_FC_ENUM16:        
  case RPC_FC_STRUCT:
  case RPC_FC_PSTRUCT:
  case RPC_FC_CSTRUCT:
  case RPC_FC_CPSTRUCT:
  case RPC_FC_CVSTRUCT:
  case RPC_FC_BOGUS_STRUCT:

    return VT_USERDEFINED;
  case 0:
    if(t->attrs)
        return VT_USERDEFINED;
    return 0;
  default:
    error("get_type_vt: unknown type: 0x%02x\n", t->type);
  }
  return 0;
}

unsigned short get_var_vt(var_t *v)
{
  unsigned short vt;

  chat("get_var_vt: %p tname %s\n", v, v->tname);
  if (v->tname) {
    vt = builtin_vt(v->tname);
    if (vt) return vt;
  }

  return get_type_vt(v->type);
}

void start_typelib(char *name, attr_t *attrs)
{
    in_typelib++;
    if (!do_typelib) return;

    typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->filename = xstrdup(typelib_name);
    typelib->attrs = attrs;
}

void end_typelib(void)
{
    in_typelib--;
    if (!typelib) return;

    create_msft_typelib(typelib);
    return;
}

void add_interface(type_t *iface)
{
    typelib_entry_t *entry;
    if (!typelib) return;

    chat("add interface: %s\n", iface->name);
    entry = xmalloc(sizeof(*entry));
    entry->kind = TKIND_INTERFACE;
    entry->u.interface = iface;
    LINK(entry, typelib->entry);
    typelib->entry = entry;
}

void add_coclass(class_t *cls)
{
    typelib_entry_t *entry;

    if (!typelib) return;

    chat("add coclass: %s\n", cls->name);

    entry = xmalloc(sizeof(*entry));
    entry->kind = TKIND_COCLASS;
    entry->u.class = cls;
    LINK(entry, typelib->entry);
    typelib->entry = entry;
}

void add_module(type_t *module)
{
    typelib_entry_t *entry;
    if (!typelib) return;

    chat("add module: %s\n", module->name);
    entry = xmalloc(sizeof(*entry));
    entry->kind = TKIND_MODULE;
    entry->u.module = module;
    LINK(entry, typelib->entry);
    typelib->entry = entry;
}

void add_struct(type_t *structure)
{
     typelib_entry_t *entry;
     if (!typelib) return;

     chat("add struct: %s\n", structure->name);
     entry = xmalloc(sizeof(*entry));
     entry->kind = TKIND_RECORD;
     entry->u.structure = structure;
     LINK(entry, typelib->entry);
     typelib->entry = entry;
}

void add_enum(type_t *enumeration)
{
     typelib_entry_t *entry;
     if (!typelib) return;

     chat("add enum: %s\n", enumeration->name);
     entry = xmalloc(sizeof(*entry));
     entry->kind = TKIND_ENUM;
     entry->u.enumeration = enumeration;
     LINK(entry, typelib->entry);
     typelib->entry = entry;
}

void add_typedef(type_t *tdef, var_t *name)
{
     typelib_entry_t *entry;
     if (!typelib) return;

     entry = xmalloc(sizeof(*entry));
     entry->kind = TKIND_ALIAS;
     entry->u.tdef = xmalloc(sizeof(*entry->u.tdef));
     memcpy(entry->u.tdef, name, sizeof(*name));
     entry->u.tdef->type = tdef;
     entry->u.tdef->name = xstrdup(name->name);
     LINK(entry, typelib->entry);
     typelib->entry = entry;
}
