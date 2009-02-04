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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <host/typedefs.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "widltypes.h"
#include "typelib_struct.h"
#include "typetree.h"

static typelib_t *typelib;

int is_ptr(const type_t *t)
{
    return type_get_type(t) == TYPE_POINTER;
}

int is_array(const type_t *t)
{
    return type_get_type(t) == TYPE_ARRAY;
}

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
    unsigned char fc;
    if (is_array(t))
      fc = type_array_get_element(t)->type;
    else
      fc = type_pointer_get_ref(t)->type;
    switch (fc)
      {
      case RPC_FC_CHAR: return VT_LPSTR;
      case RPC_FC_WCHAR: return VT_LPWSTR;
      default: break;
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
    if (match(t->name, "int")) return VT_INT;
    return VT_I4;
  case RPC_FC_ULONG:
    if (match(t->name, "int")) return VT_UINT;
    return VT_UI4;
  case RPC_FC_HYPER:
    if (t->sign < 0) return VT_UI8;
    if (match(t->name, "MIDL_uhyper")) return VT_UI8;
    return VT_I8;
  case RPC_FC_FLOAT:
    return VT_R4;
  case RPC_FC_DOUBLE:
    return VT_R8;
  case RPC_FC_RP:
  case RPC_FC_UP:
  case RPC_FC_OP:
  case RPC_FC_FP:
  case RPC_FC_SMFARRAY:
  case RPC_FC_LGFARRAY:
  case RPC_FC_SMVARRAY:
  case RPC_FC_LGVARRAY:
  case RPC_FC_CARRAY:
  case RPC_FC_CVARRAY:
  case RPC_FC_BOGUS_ARRAY:
    if(t->ref)
    {
      if (match(t->ref->name, "SAFEARRAY"))
        return VT_SAFEARRAY;
      return VT_PTR;
    }

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
  case RPC_FC_COCLASS:
  case RPC_FC_MODULE:
    return VT_USERDEFINED;
  case 0:
    return VT_VOID;
  default:
    error("get_type_vt: unknown type: 0x%02x\n", t->type);
  }
  return 0;
}

void start_typelib(typelib_t *typelib_type)
{
    if (!do_typelib) return;

    typelib = typelib_type;
    typelib->filename = xstrdup(typelib_name);
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
