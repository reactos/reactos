/*
 * IDL Compiler
 *
 * Copyright 2004 Ove Kaaven
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include "wine/wpp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include <typedefs.h>
#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "widltypes.h"
#include "typelib_struct.h"
#include "typetree.h"

static typelib_t *typelib;

/* List of oleauto types that should be recognized by name.
 * (most of) these seem to be intrinsic types in mktyplib.
 * This table MUST be alphabetically sorted on the kw field.
 */
static const struct oatype {
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

static unsigned short builtin_vt(const type_t *t)
{
  const char *kw = t->name;
  struct oatype key;
  const struct oatype *kwp;
  key.kw = kw;
#ifdef KW_BSEARCH
  kwp = bsearch(&key, oatypes, NTYPES, sizeof(oatypes[0]), kw_cmp_func);
#else
  {
    unsigned int i;
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
  if (is_string_type (t->attrs, t))
  {
    const type_t *elem_type;
    if (is_array(t))
      elem_type = type_array_get_element(t);
    else
      elem_type = type_pointer_get_ref(t);
    if (type_get_type(elem_type) == TYPE_BASIC)
    {
      switch (type_basic_get_type(elem_type))
      {
      case TYPE_BASIC_CHAR: return VT_LPSTR;
      case TYPE_BASIC_WCHAR: return VT_LPWSTR;
      default: break;
      }
    }
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
    vt = builtin_vt(t);
    if (vt) return vt;
  }

  if (type_is_alias(t) && is_attr(t->attrs, ATTR_PUBLIC))
    return VT_USERDEFINED;

  switch (type_get_type(t)) {
  case TYPE_BASIC:
    switch (type_basic_get_type(t)) {
    case TYPE_BASIC_BYTE:
      return VT_UI1;
    case TYPE_BASIC_CHAR:
    case TYPE_BASIC_INT8:
      if (type_basic_get_sign(t) > 0)
        return VT_UI1;
      else
        return VT_I1;
    case TYPE_BASIC_WCHAR:
      return VT_I2; /* mktyplib seems to parse wchar_t as short */
    case TYPE_BASIC_INT16:
      if (type_basic_get_sign(t) > 0)
        return VT_UI2;
      else
        return VT_I2;
    case TYPE_BASIC_INT:
      if (type_basic_get_sign(t) > 0)
        return VT_UINT;
      else
        return VT_INT;
    case TYPE_BASIC_INT32:
    case TYPE_BASIC_ERROR_STATUS_T:
      if (type_basic_get_sign(t) > 0)
        return VT_UI4;
      else
        return VT_I4;
    case TYPE_BASIC_INT64:
    case TYPE_BASIC_HYPER:
      if (type_basic_get_sign(t) > 0)
        return VT_UI8;
      else
        return VT_I8;
    case TYPE_BASIC_INT3264:
      if (typelib_kind == SYS_WIN64)
      {
        if (type_basic_get_sign(t) > 0)
          return VT_UI8;
        else
          return VT_I8;
      }
      else
      {
        if (type_basic_get_sign(t) > 0)
          return VT_UI4;
        else
          return VT_I4;
      }
    case TYPE_BASIC_FLOAT:
      return VT_R4;
    case TYPE_BASIC_DOUBLE:
      return VT_R8;
    case TYPE_BASIC_HANDLE:
      error("handles can't be used in typelibs\n");
    }
    break;

  case TYPE_POINTER:
    return VT_PTR;

  case TYPE_ARRAY:
    if (type_array_is_decl_as_ptr(t))
    {
      if (match(type_array_get_element(t)->name, "SAFEARRAY"))
        return VT_SAFEARRAY;
    }
    else
      error("get_type_vt: array types not supported\n");
    return VT_PTR;

  case TYPE_INTERFACE:
    if(match(t->name, "IUnknown"))
      return VT_UNKNOWN;
    if(match(t->name, "IDispatch"))
      return VT_DISPATCH;
    return VT_USERDEFINED;

  case TYPE_ENUM:
  case TYPE_STRUCT:
  case TYPE_COCLASS:
  case TYPE_MODULE:
  case TYPE_UNION:
  case TYPE_ENCAPSULATED_UNION:
    return VT_USERDEFINED;

  case TYPE_VOID:
    return VT_VOID;

  case TYPE_ALIAS:
    /* aliases should be filtered out by the type_get_type call above */
    assert(0);
    break;

  case TYPE_FUNCTION:
    error("get_type_vt: functions not supported\n");
    break;

  case TYPE_BITFIELD:
    error("get_type_vt: bitfields not supported\n");
    break;
  }
  return 0;
}

void start_typelib(typelib_t *typelib_type)
{
    if (!do_typelib) return;
    typelib = typelib_type;
}

void end_typelib(void)
{
    if (!typelib) return;

    create_msft_typelib(typelib);
}

static void tlb_read(int fd, void *buf, int count)
{
    if(read(fd, buf, count) < count)
        error("error while reading importlib.\n");
}

static void tlb_lseek(int fd, off_t offset)
{
    if(lseek(fd, offset, SEEK_SET) == -1)
        error("lseek failed\n");
}

static void msft_read_guid(int fd, MSFT_SegDir *segdir, int offset, GUID *guid)
{
    tlb_lseek(fd, segdir->pGuidTab.offset+offset);
    tlb_read(fd, guid, sizeof(GUID));
}

static void read_msft_importlib(importlib_t *importlib, int fd)
{
    MSFT_Header header;
    MSFT_SegDir segdir;
    int *typeinfo_offs;
    int i;

    importlib->allocated = 0;

    tlb_lseek(fd, 0);
    tlb_read(fd, &header, sizeof(header));

    importlib->version = header.version;

    typeinfo_offs = xmalloc(header.nrtypeinfos*sizeof(INT));
    tlb_read(fd, typeinfo_offs, header.nrtypeinfos*sizeof(INT));
    tlb_read(fd, &segdir, sizeof(segdir));

    msft_read_guid(fd, &segdir, header.posguid, &importlib->guid);

    importlib->ntypeinfos = header.nrtypeinfos;
    importlib->importinfos = xmalloc(importlib->ntypeinfos*sizeof(importinfo_t));

    for(i=0; i < importlib->ntypeinfos; i++) {
        MSFT_TypeInfoBase base;
        MSFT_NameIntro nameintro;
        int len;

        tlb_lseek(fd, sizeof(MSFT_Header) + header.nrtypeinfos*sizeof(INT) + sizeof(MSFT_SegDir)
                 + typeinfo_offs[i]);
        tlb_read(fd, &base, sizeof(base));

        importlib->importinfos[i].importlib = importlib;
        importlib->importinfos[i].flags = (base.typekind&0xf)<<24;
        importlib->importinfos[i].offset = -1;
        importlib->importinfos[i].id = i;

        if(base.posguid != -1) {
            importlib->importinfos[i].flags |= MSFT_IMPINFO_OFFSET_IS_GUID;
            msft_read_guid(fd, &segdir, base.posguid, &importlib->importinfos[i].guid);
        }
        else memset( &importlib->importinfos[i].guid, 0, sizeof(importlib->importinfos[i].guid));

        tlb_lseek(fd, segdir.pNametab.offset + base.NameOffset);
        tlb_read(fd, &nameintro, sizeof(nameintro));

        len = nameintro.namelen & 0xff;

        importlib->importinfos[i].name = xmalloc(len+1);
        tlb_read(fd, importlib->importinfos[i].name, len);
        importlib->importinfos[i].name[len] = 0;
    }

    free(typeinfo_offs);
}

static void read_importlib(importlib_t *importlib)
{
    int fd;
    INT magic;
    char *file_name;

    file_name = wpp_find_include(importlib->name, NULL);
    if(file_name) {
        fd = open(file_name, O_RDONLY | O_BINARY );
        free(file_name);
    }else {
        fd = open(importlib->name, O_RDONLY | O_BINARY );
    }

    if(fd < 0)
        error("Could not open importlib %s.\n", importlib->name);

    tlb_read(fd, &magic, sizeof(magic));

    switch(magic) {
    case MSFT_MAGIC:
        read_msft_importlib(importlib, fd);
        break;
    default:
        error("Wrong or unsupported typelib magic %x\n", magic);
    };

    close(fd);
}

void add_importlib(const char *name)
{
    importlib_t *importlib;

    if(!typelib) return;

    LIST_FOR_EACH_ENTRY( importlib, &typelib->importlibs, importlib_t, entry )
        if(!strcmp(name, importlib->name))
            return;

    chat("add_importlib: %s\n", name);

    importlib = xmalloc(sizeof(*importlib));
    memset( importlib, 0, sizeof(*importlib) );
    importlib->name = xstrdup(name);

    read_importlib(importlib);
    list_add_head( &typelib->importlibs, &importlib->entry );
}
