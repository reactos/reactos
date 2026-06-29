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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#ifdef __REACTOS__
#include <typedefs.h>
#include <pecoff.h>
#else
#include "windef.h"
#include "winbase.h"
#endif
#include "utils.h"
#include "wpp_private.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "widltypes.h"
#include "typelib_struct.h"
#include "typetree.h"

#ifdef __REACTOS__
static typelib_t *typelib;
#endif

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
  kwp = bsearch(&key, oatypes, ARRAY_SIZE(oatypes), sizeof(oatypes[0]), kw_cmp_func);
#else
  {
    unsigned int i;
    for (kwp = NULL, i = 0; i < ARRAY_SIZE(oatypes); i++)
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
      elem_type = type_array_get_element_type(t);
    else
      elem_type = type_pointer_get_ref_type(t);
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

  if (type_is_alias(t) &&
        (is_attr(t->attrs, ATTR_PUBLIC) || is_attr(t->attrs, ATTR_WIREMARSHAL)))
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
    case TYPE_BASIC_LONG:
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
      if (pointer_size == 8)
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
      if (match(type_array_get_element_type(t)->name, "SAFEARRAY"))
        return VT_SAFEARRAY;
      return VT_PTR;
    }
    else
      return VT_CARRAY;

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
  case TYPE_RUNTIMECLASS:
  case TYPE_DELEGATE:
    return VT_USERDEFINED;

  case TYPE_VOID:
    return VT_VOID;

  case TYPE_ALIAS:
  case TYPE_APICONTRACT:
  case TYPE_PARAMETERIZED_TYPE:
  case TYPE_PARAMETER:
    /* not supposed to be here */
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

#ifdef __REACTOS__
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
#endif

static void msft_read_guid(void *data, MSFT_SegDir *segdir, int offset, struct uuid *guid)
{
    memcpy( guid, (char *)data + segdir->pGuidTab.offset + offset, sizeof(*guid) );
}

static void read_msft_importlib(importlib_t *importlib, void *data, unsigned int size)
{
    MSFT_Header *header = data;
    MSFT_SegDir *segdir;
    int *typeinfo_offs;
    int i;

    importlib->allocated = 0;
    importlib->version = header->version;

    typeinfo_offs = (int *)(header + 1);
    segdir = (MSFT_SegDir *)(typeinfo_offs + header->nrtypeinfos);

    msft_read_guid(data, segdir, header->posguid, &importlib->guid);

    importlib->ntypeinfos = header->nrtypeinfos;
    importlib->importinfos = xmalloc(importlib->ntypeinfos*sizeof(importinfo_t));

    for(i=0; i < importlib->ntypeinfos; i++) {
        MSFT_TypeInfoBase *base = (MSFT_TypeInfoBase *)((char *)(segdir + 1) + typeinfo_offs[i]);
        MSFT_NameIntro *nameintro;
        int len;

        importlib->importinfos[i].importlib = importlib;
        importlib->importinfos[i].flags = (base->typekind & 0xf)<<24;
        importlib->importinfos[i].offset = -1;
        importlib->importinfos[i].id = i;

        if(base->posguid != -1) {
            importlib->importinfos[i].flags |= MSFT_IMPINFO_OFFSET_IS_GUID;
            msft_read_guid(data, segdir, base->posguid, &importlib->importinfos[i].guid);
        }
        else memset( &importlib->importinfos[i].guid, 0, sizeof(importlib->importinfos[i].guid));

        nameintro = (MSFT_NameIntro *)((char *)data + segdir->pNametab.offset + base->NameOffset);

        len = nameintro->namelen & 0xff;
        importlib->importinfos[i].name = xmalloc(len+1);
        memcpy( importlib->importinfos[i].name, nameintro + 1, len );
        importlib->importinfos[i].name[len] = 0;
    }
}

static unsigned int rva_to_va( const IMAGE_NT_HEADERS32 *nt, unsigned int rva )
{
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION( nt );
    unsigned int i;

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        if (sec->VirtualAddress <= rva && sec->VirtualAddress + sec->Misc.VirtualSize > rva)
            return sec->PointerToRawData + (rva - sec->VirtualAddress);
    error( "no PE section found for addr %x\n", rva );
#ifdef __REACTOS__
    return 0;
#endif
}

static void read_pe_importlib(importlib_t *importlib, void *data, unsigned int size)
{
    IMAGE_DOS_HEADER *dos = data;
    IMAGE_NT_HEADERS32 *nt;
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_RESOURCE_DIRECTORY *root, *resdir;
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry;
    IMAGE_RESOURCE_DATA_ENTRY *resdata;
    void *ptr;
    unsigned int i, va;

    if (dos->e_lfanew < sizeof(*dos) || dos->e_lfanew >= size) error( "not a PE file\n" );
    nt = (IMAGE_NT_HEADERS32 *)((char *)data + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) error( "not a PE file\n" );
    if ((char *)(IMAGE_FIRST_SECTION(nt) + nt->FileHeader.NumberOfSections) > (char *)data + size)
        error( "invalid PE file\n" );
    switch (nt->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        dir = &((IMAGE_NT_HEADERS64 *)nt)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
        break;
    default:
        error( "invalid PE file\n" );
    }
    if (!dir->VirtualAddress || !dir->Size) error( "resource not found in PE file\n" );
    va = rva_to_va( nt, dir->VirtualAddress );
    if (va + dir->Size > size) error( "invalid resource data in PE file\n" );
    root = resdir = (IMAGE_RESOURCE_DIRECTORY *)((char *)data + va );
    entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries; i++, entry++)
    {
        static const WCHAR typelibW[] = {'T','Y','P','E','L','I','B'};
        WCHAR *name = (WCHAR *)((char *)root + entry->NameOffset);
        if (name[0] != ARRAY_SIZE(typelibW)) continue;
        if (!memcmp( name + 1, typelibW, sizeof(typelibW) )) break;
    }
    if (i == resdir->NumberOfNamedEntries) error( "typelib resource not found in PE file\n" );
    while (entry->DataIsDirectory)
    {
        resdir = (IMAGE_RESOURCE_DIRECTORY *)((char *)root + entry->OffsetToDirectory);
        entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    }
    resdata = (IMAGE_RESOURCE_DATA_ENTRY *)((char *)root + entry->OffsetToData);
    ptr = (char *)data + rva_to_va( nt, resdata->OffsetToData );
    if (memcmp( ptr, "MSFT", 4 )) error( "invalid typelib found in PE file\n" );
    read_msft_importlib( importlib, ptr, resdata->Size );
}

static void read_importlib(importlib_t *importlib)
{
    int fd, size;
    void *data;

    fd = open_typelib(importlib->name);

    /* widl extension: if importlib name has no .tlb extension, try using .tlb */
    if (fd < 0 && !strendswith( importlib->name, ".tlb" ))
        fd = open_typelib( strmake( "%s.tlb", importlib->name ));

    if(fd < 0)
        error("Could not find importlib %s.\n", importlib->name);

    size = lseek( fd, 0, SEEK_END );
    data = xmalloc( size );
    lseek( fd, 0, SEEK_SET );
    if (read( fd, data, size) < size) error("error while reading importlib.\n");
    close( fd );

    if (!memcmp( data, "MSFT", 4 ))
        read_msft_importlib(importlib, data, size);
    else if (!memcmp( data, "MZ", 2 ))
        read_pe_importlib(importlib, data, size);
    else
        error("Wrong or unsupported typelib\n");

    free( data );
}

#ifdef __REACTOS__
void add_importlib(const char *name)
#else
void add_importlib(const char *name, typelib_t *typelib)
#endif
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
