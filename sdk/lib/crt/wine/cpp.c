/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2003, 2004 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "cppexcept.h"
#include "mtdll.h"
#include "cxx.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

struct __type_info_node
{
    void *memPtr;
    struct __type_info_node* next;
};

typedef exception bad_cast;
typedef exception bad_typeid;
typedef exception __non_rtti_object;

extern const vtable_ptr MSVCRT_exception_vtable;
extern const vtable_ptr MSVCRT_bad_typeid_vtable;
extern const vtable_ptr MSVCRT_bad_cast_vtable;
extern const vtable_ptr MSVCRT___non_rtti_object_vtable;
extern const vtable_ptr MSVCRT_type_info_vtable;

/* get the vtable pointer for a C++ object */
static inline const vtable_ptr *get_vtable( void *obj )
{
    return *(const vtable_ptr **)obj;
}

static inline const rtti_object_locator *get_obj_locator( void *cppobj )
{
    const vtable_ptr *vtable = get_vtable( cppobj );
    return (const rtti_object_locator *)vtable[-1];
}

#ifndef __x86_64__
static void dump_obj_locator( const rtti_object_locator *ptr )
{
    int i;
    const rtti_object_hierarchy *h = ptr->type_hierarchy;

    TRACE( "%p: sig=%08x base_offset=%08x flags=%08x type=%p %s hierarchy=%p\n",
           ptr, ptr->signature, ptr->base_class_offset, ptr->flags,
           ptr->type_descriptor, dbgstr_type_info(ptr->type_descriptor), ptr->type_hierarchy );
    TRACE( "  hierarchy: sig=%08x attr=%08x len=%d base classes=%p\n",
           h->signature, h->attributes, h->array_len, h->base_classes );
    for (i = 0; i < h->array_len; i++)
    {
        TRACE( "    base class %p: num %d off %d,%d,%d attr %08x type %p %s\n",
               h->base_classes->bases[i],
               h->base_classes->bases[i]->num_base_classes,
               h->base_classes->bases[i]->offsets.this_offset,
               h->base_classes->bases[i]->offsets.vbase_descr,
               h->base_classes->bases[i]->offsets.vbase_offset,
               h->base_classes->bases[i]->attributes,
               h->base_classes->bases[i]->type_descriptor,
               dbgstr_type_info(h->base_classes->bases[i]->type_descriptor) );
    }
}

#else

static void dump_obj_locator( const rtti_object_locator *ptr )
{
    int i;
    char *base = ptr->signature == 0 ? RtlPcToFileHeader((void*)ptr, (void**)&base) : (char*)ptr - ptr->object_locator;
    const rtti_object_hierarchy *h = (const rtti_object_hierarchy*)(base + ptr->type_hierarchy);
    const type_info *type_descriptor = (const type_info*)(base + ptr->type_descriptor);

    TRACE( "%p: sig=%08x base_offset=%08x flags=%08x type=%p %s hierarchy=%p\n",
            ptr, ptr->signature, ptr->base_class_offset, ptr->flags,
            type_descriptor, dbgstr_type_info(type_descriptor), h );
    TRACE( "  hierarchy: sig=%08x attr=%08x len=%d base classes=%p\n",
            h->signature, h->attributes, h->array_len, base + h->base_classes );
    for (i = 0; i < h->array_len; i++)
    {
        const rtti_base_descriptor *bases = (rtti_base_descriptor*)(base +
                ((const rtti_base_array*)(base + h->base_classes))->bases[i]);

        TRACE( "    base class %p: num %d off %d,%d,%d attr %08x type %p %s\n",
                bases,
                bases->num_base_classes,
                bases->offsets.this_offset,
                bases->offsets.vbase_descr,
                bases->offsets.vbase_offset,
                bases->attributes,
                base + bases->type_descriptor,
                dbgstr_type_info((const type_info*)(base + bases->type_descriptor)) );
    }
}
#endif

/* Internal common ctor for exception */
static void EXCEPTION_ctor(exception *_this, const char** name)
{
  _this->vtable = &MSVCRT_exception_vtable;
  if (*name)
  {
    unsigned int name_len = strlen(*name) + 1;
    _this->name = MSVCRT_malloc(name_len);
    memcpy(_this->name, *name, name_len);
    _this->do_free = TRUE;
  }
  else
  {
    _this->name = NULL;
    _this->do_free = FALSE;
  }
}

#ifdef __REACTOS__
#include <internal/wine_msc.h>
#endif /* __REACTOS__ */

/******************************************************************
 *		??0exception@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_ctor,8)
exception * __thiscall MSVCRT_exception_ctor(exception * _this, const char ** name)
{
  TRACE("(%p,%s)\n", _this, *name);
  EXCEPTION_ctor(_this, name);
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@ABQBDH@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_ctor_noalloc,12)
exception * __thiscall MSVCRT_exception_ctor_noalloc(exception * _this, char ** name, int noalloc)
{
  TRACE("(%p,%s)\n", _this, *name);
  _this->vtable = &MSVCRT_exception_vtable;
  _this->name = *name;
  _this->do_free = FALSE;
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_copy_ctor,8)
exception * __thiscall MSVCRT_exception_copy_ctor(exception * _this, const exception * rhs)
{
  TRACE("(%p,%p)\n", _this, rhs);

  if (!rhs->do_free)
  {
    _this->vtable = &MSVCRT_exception_vtable;
    _this->name = rhs->name;
    _this->do_free = FALSE;
  }
  else
    EXCEPTION_ctor(_this, (const char**)&rhs->name);
  TRACE("name = %s\n", _this->name);
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_default_ctor,4)
exception * __thiscall MSVCRT_exception_default_ctor(exception * _this)
{
  static const char* empty = NULL;

  TRACE("(%p)\n", _this);
  EXCEPTION_ctor(_this, &empty);
  return _this;
}

/******************************************************************
 *		??1exception@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_dtor,4)
void __thiscall MSVCRT_exception_dtor(exception * _this)
{
  TRACE("(%p)\n", _this);
  _this->vtable = &MSVCRT_exception_vtable;
  if (_this->do_free) MSVCRT_free(_this->name);
}

/******************************************************************
 *		??4exception@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_opequals,8)
exception * __thiscall MSVCRT_exception_opequals(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  if (_this != rhs)
  {
      MSVCRT_exception_dtor(_this);
      MSVCRT_exception_copy_ctor(_this, rhs);
  }
  TRACE("name = %s\n", _this->name);
  return _this;
}

/******************************************************************
 *		??_Eexception@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_vector_dtor,8)
void * __thiscall MSVCRT_exception_vector_dtor(exception * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_exception_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_exception_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gexception@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_scalar_dtor,8)
void * __thiscall MSVCRT_exception_scalar_dtor(exception * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    MSVCRT_exception_dtor(_this);
    if (flags & 1) MSVCRT_operator_delete(_this);
    return _this;
}

/******************************************************************
 *		?what@exception@@UBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_what_exception,4)
const char * __thiscall MSVCRT_what_exception(exception * _this)
{
  TRACE("(%p) returning %s\n", _this, _this->name);
  return _this->name ? _this->name : "Unknown exception";
}

/******************************************************************
 *		??0bad_typeid@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_copy_ctor,8)
bad_typeid * __thiscall MSVCRT_bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_exception_copy_ctor(_this, rhs);
  _this->vtable = &MSVCRT_bad_typeid_vtable;
  return _this;
}

/******************************************************************
 *		??0bad_typeid@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_ctor,8)
bad_typeid * __thiscall MSVCRT_bad_typeid_ctor(bad_typeid * _this, const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  EXCEPTION_ctor(_this, &name);
  _this->vtable = &MSVCRT_bad_typeid_vtable;
  return _this;
}

/******************************************************************
 *		??_Fbad_typeid@@QAEXXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_default_ctor,4)
bad_typeid * __thiscall MSVCRT_bad_typeid_default_ctor(bad_typeid * _this)
{
  return MSVCRT_bad_typeid_ctor( _this, "bad typeid" );
}

/******************************************************************
 *		??1bad_typeid@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_dtor,4)
void __thiscall MSVCRT_bad_typeid_dtor(bad_typeid * _this)
{
  TRACE("(%p)\n", _this);
  MSVCRT_exception_dtor(_this);
}

/******************************************************************
 *		??4bad_typeid@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_opequals,8)
bad_typeid * __thiscall MSVCRT_bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_exception_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *              ??_Ebad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_vector_dtor,8)
void * __thiscall MSVCRT_bad_typeid_vector_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_bad_typeid_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_bad_typeid_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_scalar_dtor,8)
void * __thiscall MSVCRT_bad_typeid_scalar_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    MSVCRT_bad_typeid_dtor(_this);
    if (flags & 1) MSVCRT_operator_delete(_this);
    return _this;
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_copy_ctor,8)
__non_rtti_object * __thiscall MSVCRT___non_rtti_object_copy_ctor(__non_rtti_object * _this,
                                                                 const __non_rtti_object * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_bad_typeid_copy_ctor(_this, rhs);
  _this->vtable = &MSVCRT___non_rtti_object_vtable;
  return _this;
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_ctor,8)
__non_rtti_object * __thiscall MSVCRT___non_rtti_object_ctor(__non_rtti_object * _this,
                                                            const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  EXCEPTION_ctor(_this, &name);
  _this->vtable = &MSVCRT___non_rtti_object_vtable;
  return _this;
}

/******************************************************************
 *		??1__non_rtti_object@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_dtor,4)
void __thiscall MSVCRT___non_rtti_object_dtor(__non_rtti_object * _this)
{
  TRACE("(%p)\n", _this);
  MSVCRT_bad_typeid_dtor(_this);
}

/******************************************************************
 *		??4__non_rtti_object@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_opequals,8)
__non_rtti_object * __thiscall MSVCRT___non_rtti_object_opequals(__non_rtti_object * _this,
                                                                const __non_rtti_object *rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_bad_typeid_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *		??_E__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_vector_dtor,8)
void * __thiscall MSVCRT___non_rtti_object_vector_dtor(__non_rtti_object * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT___non_rtti_object_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT___non_rtti_object_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_G__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_scalar_dtor,8)
void * __thiscall MSVCRT___non_rtti_object_scalar_dtor(__non_rtti_object * _this, unsigned int flags)
{
  TRACE("(%p %x)\n", _this, flags);
  MSVCRT___non_rtti_object_dtor(_this);
  if (flags & 1) MSVCRT_operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??0bad_cast@@AAE@PBQBD@Z (MSVCRT.@)
 *		??0bad_cast@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_ctor,8)
bad_cast * __thiscall MSVCRT_bad_cast_ctor(bad_cast * _this, const char ** name)
{
  TRACE("(%p %s)\n", _this, *name);
  EXCEPTION_ctor(_this, name);
  _this->vtable = &MSVCRT_bad_cast_vtable;
  return _this;
}

/******************************************************************
 *		??0bad_cast@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_copy_ctor,8)
bad_cast * __thiscall MSVCRT_bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_exception_copy_ctor(_this, rhs);
  _this->vtable = &MSVCRT_bad_cast_vtable;
  return _this;
}

/******************************************************************
 *		??0bad_cast@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_ctor_charptr,8)
bad_cast * __thiscall MSVCRT_bad_cast_ctor_charptr(bad_cast * _this, const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  EXCEPTION_ctor(_this, &name);
  _this->vtable = &MSVCRT_bad_cast_vtable;
  return _this;
}

/******************************************************************
 *		??_Fbad_cast@@QAEXXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_default_ctor,4)
bad_cast * __thiscall MSVCRT_bad_cast_default_ctor(bad_cast * _this)
{
  return MSVCRT_bad_cast_ctor_charptr( _this, "bad cast" );
}

/******************************************************************
 *		??1bad_cast@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_dtor,4)
void __thiscall MSVCRT_bad_cast_dtor(bad_cast * _this)
{
  TRACE("(%p)\n", _this);
  MSVCRT_exception_dtor(_this);
}

/******************************************************************
 *		??4bad_cast@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_opequals,8)
bad_cast * __thiscall MSVCRT_bad_cast_opequals(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  MSVCRT_exception_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *              ??_Ebad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_vector_dtor,8)
void * __thiscall MSVCRT_bad_cast_vector_dtor(bad_cast * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_bad_cast_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_bad_cast_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_scalar_dtor,8)
void * __thiscall MSVCRT_bad_cast_scalar_dtor(bad_cast * _this, unsigned int flags)
{
  TRACE("(%p %x)\n", _this, flags);
  MSVCRT_bad_cast_dtor(_this);
  if (flags & 1) MSVCRT_operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??8type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_opequals_equals,8)
int __thiscall MSVCRT_type_info_opequals_equals(type_info * _this, const type_info * rhs)
{
    int ret = !strcmp(_this->mangled + 1, rhs->mangled + 1);
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		??9type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_opnot_equals,8)
int __thiscall MSVCRT_type_info_opnot_equals(type_info * _this, const type_info * rhs)
{
    int ret = !!strcmp(_this->mangled + 1, rhs->mangled + 1);
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		?before@type_info@@QBEHABV1@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_before,8)
int __thiscall MSVCRT_type_info_before(type_info * _this, const type_info * rhs)
{
    int ret = strcmp(_this->mangled + 1, rhs->mangled + 1) < 0;
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		??1type_info@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_dtor,4)
void __thiscall MSVCRT_type_info_dtor(type_info * _this)
{
  TRACE("(%p)\n", _this);
  MSVCRT_free(_this->name);
}

/******************************************************************
 *		?name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_name,4)
const char * __thiscall MSVCRT_type_info_name(type_info * _this)
{
  if (!_this->name)
  {
    /* Create and set the demangled name */
    /* Note: mangled name in type_info struct always starts with a '.', while
     * it isn't valid for mangled name.
     * Is this '.' really part of the mangled name, or has it some other meaning ?
     */
    char* name = __unDName(0, _this->mangled + 1, 0,
                           MSVCRT_malloc, MSVCRT_free, UNDNAME_NO_ARGUMENTS | UNDNAME_32_BIT_DECODE);
    if (name)
    {
      unsigned int len = strlen(name);

      /* It seems _unDName may leave blanks at the end of the demangled name */
      while (len && name[--len] == ' ')
        name[len] = '\0';

      if (InterlockedCompareExchangePointer((void**)&_this->name, name, NULL))
      {
        /* Another thread set this member since we checked above - use it */
        MSVCRT_free(name);
      }
    }
  }
  TRACE("(%p) returning %s\n", _this, _this->name);
  return _this->name;
}

/******************************************************************
 *		?raw_name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_raw_name,4)
const char * __thiscall MSVCRT_type_info_raw_name(type_info * _this)
{
  TRACE("(%p) returning %s\n", _this, _this->mangled);
  return _this->mangled;
}

/* Unexported */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_vector_dtor,8)
void * __thiscall MSVCRT_type_info_vector_dtor(type_info * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_type_info_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_type_info_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

#if _MSVCR_VER >= 80

typedef exception bad_alloc;
extern const vtable_ptr MSVCRT_bad_alloc_vtable;

static void bad_alloc_ctor(bad_alloc *this, const char **name)
{
    MSVCRT_exception_ctor(this, name);
    this->vtable = &MSVCRT_bad_alloc_vtable;
}

/* bad_alloc class implementation */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_alloc_copy_ctor,8)
bad_alloc * __thiscall MSVCRT_bad_alloc_copy_ctor(bad_alloc * _this, const bad_alloc * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_bad_alloc_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_bad_alloc_dtor,4)
void __thiscall MSVCRT_bad_alloc_dtor(bad_alloc * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

#endif /* _MSVCR_VER >= 80 */

#if _MSVCR_VER >= 100

typedef struct {
    exception e;
    HRESULT hr;
} scheduler_resource_allocation_error;
extern const vtable_ptr MSVCRT_scheduler_resource_allocation_error_vtable;

/* ??0scheduler_resource_allocation_error@Concurrency@@QAE@PBDJ@Z */
/* ??0scheduler_resource_allocation_error@Concurrency@@QEAA@PEBDJ@Z */
DEFINE_THISCALL_WRAPPER(scheduler_resource_allocation_error_ctor_name, 12)
scheduler_resource_allocation_error* __thiscall scheduler_resource_allocation_error_ctor_name(
        scheduler_resource_allocation_error *this, const char *name, HRESULT hr)
{
    TRACE("(%p %s %x)\n", this, wine_dbgstr_a(name), hr);
    MSVCRT_exception_ctor(&this->e, &name);
    this->e.vtable = &MSVCRT_scheduler_resource_allocation_error_vtable;
    this->hr = hr;
    return this;
}

/* ??0scheduler_resource_allocation_error@Concurrency@@QAE@J@Z */
/* ??0scheduler_resource_allocation_error@Concurrency@@QEAA@J@Z */
DEFINE_THISCALL_WRAPPER(scheduler_resource_allocation_error_ctor, 8)
scheduler_resource_allocation_error* __thiscall scheduler_resource_allocation_error_ctor(
        scheduler_resource_allocation_error *this, HRESULT hr)
{
    return scheduler_resource_allocation_error_ctor_name(this, NULL, hr);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_scheduler_resource_allocation_error_copy_ctor,8)
scheduler_resource_allocation_error* __thiscall MSVCRT_scheduler_resource_allocation_error_copy_ctor(
        scheduler_resource_allocation_error *this,
        const scheduler_resource_allocation_error *rhs)
{
    TRACE("(%p,%p)\n", this, rhs);

    if (!rhs->e.do_free)
        memcpy(this, rhs, sizeof(*this));
    else
        scheduler_resource_allocation_error_ctor_name(this, rhs->e.name, rhs->hr);
    return this;
}

/* ?get_error_code@scheduler_resource_allocation_error@Concurrency@@QBEJXZ */
/* ?get_error_code@scheduler_resource_allocation_error@Concurrency@@QEBAJXZ */
DEFINE_THISCALL_WRAPPER(scheduler_resource_allocation_error_get_error_code, 4)
HRESULT __thiscall scheduler_resource_allocation_error_get_error_code(
        const scheduler_resource_allocation_error *this)
{
    TRACE("(%p)\n", this);
    return this->hr;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_scheduler_resource_allocation_error_dtor,4)
void __thiscall MSVCRT_scheduler_resource_allocation_error_dtor(
        scheduler_resource_allocation_error * this)
{
    TRACE("(%p)\n", this);
    MSVCRT_exception_dtor(&this->e);
}

typedef exception improper_lock;
extern const vtable_ptr MSVCRT_improper_lock_vtable;

/* ??0improper_lock@Concurrency@@QAE@PBD@Z */
/* ??0improper_lock@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(improper_lock_ctor_str, 8)
improper_lock* __thiscall improper_lock_ctor_str(improper_lock *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_improper_lock_vtable;
    return this;
}

/* ??0improper_lock@Concurrency@@QAE@XZ */
/* ??0improper_lock@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(improper_lock_ctor, 4)
improper_lock* __thiscall improper_lock_ctor(improper_lock *this)
{
    return improper_lock_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_lock_copy_ctor,8)
improper_lock * __thiscall MSVCRT_improper_lock_copy_ctor(improper_lock * _this, const improper_lock * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_improper_lock_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_lock_dtor,4)
void __thiscall MSVCRT_improper_lock_dtor(improper_lock * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

typedef exception invalid_scheduler_policy_key;
extern const vtable_ptr MSVCRT_invalid_scheduler_policy_key_vtable;

/* ??0invalid_scheduler_policy_key@Concurrency@@QAE@PBD@Z */
/* ??0invalid_scheduler_policy_key@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_key_ctor_str, 8)
invalid_scheduler_policy_key* __thiscall invalid_scheduler_policy_key_ctor_str(
        invalid_scheduler_policy_key *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_invalid_scheduler_policy_key_vtable;
    return this;
}

/* ??0invalid_scheduler_policy_key@Concurrency@@QAE@XZ */
/* ??0invalid_scheduler_policy_key@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_key_ctor, 4)
invalid_scheduler_policy_key* __thiscall invalid_scheduler_policy_key_ctor(
        invalid_scheduler_policy_key *this)
{
    return invalid_scheduler_policy_key_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_key_copy_ctor,8)
invalid_scheduler_policy_key * __thiscall MSVCRT_invalid_scheduler_policy_key_copy_ctor(
        invalid_scheduler_policy_key * _this, const invalid_scheduler_policy_key * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_invalid_scheduler_policy_key_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_key_dtor,4)
void __thiscall MSVCRT_invalid_scheduler_policy_key_dtor(
        invalid_scheduler_policy_key * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

typedef exception invalid_scheduler_policy_value;
extern const vtable_ptr MSVCRT_invalid_scheduler_policy_value_vtable;

/* ??0invalid_scheduler_policy_value@Concurrency@@QAE@PBD@Z */
/* ??0invalid_scheduler_policy_value@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_value_ctor_str, 8)
invalid_scheduler_policy_value* __thiscall invalid_scheduler_policy_value_ctor_str(
        invalid_scheduler_policy_value *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_invalid_scheduler_policy_value_vtable;
    return this;
}

/* ??0invalid_scheduler_policy_value@Concurrency@@QAE@XZ */
/* ??0invalid_scheduler_policy_value@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_value_ctor, 4)
invalid_scheduler_policy_value* __thiscall invalid_scheduler_policy_value_ctor(
        invalid_scheduler_policy_value *this)
{
    return invalid_scheduler_policy_value_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_value_copy_ctor,8)
invalid_scheduler_policy_value * __thiscall MSVCRT_invalid_scheduler_policy_value_copy_ctor(
        invalid_scheduler_policy_value * _this, const invalid_scheduler_policy_value * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_invalid_scheduler_policy_value_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_value_dtor,4)
void __thiscall MSVCRT_invalid_scheduler_policy_value_dtor(
        invalid_scheduler_policy_value * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

typedef exception invalid_scheduler_policy_thread_specification;
extern const vtable_ptr MSVCRT_invalid_scheduler_policy_thread_specification_vtable;

/* ??0invalid_scheduler_policy_thread_specification@Concurrency@@QAE@PBD@Z */
/* ??0invalid_scheduler_policy_thread_specification@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_thread_specification_ctor_str, 8)
invalid_scheduler_policy_thread_specification* __thiscall invalid_scheduler_policy_thread_specification_ctor_str(
        invalid_scheduler_policy_thread_specification *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_invalid_scheduler_policy_thread_specification_vtable;
    return this;
}

/* ??0invalid_scheduler_policy_thread_specification@Concurrency@@QAE@XZ */
/* ??0invalid_scheduler_policy_thread_specification@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(invalid_scheduler_policy_thread_specification_ctor, 4)
invalid_scheduler_policy_thread_specification* __thiscall invalid_scheduler_policy_thread_specification_ctor(
        invalid_scheduler_policy_thread_specification *this)
{
    return invalid_scheduler_policy_thread_specification_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_thread_specification_copy_ctor,8)
invalid_scheduler_policy_thread_specification * __thiscall MSVCRT_invalid_scheduler_policy_thread_specification_copy_ctor(
        invalid_scheduler_policy_thread_specification * _this, const invalid_scheduler_policy_thread_specification * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_invalid_scheduler_policy_thread_specification_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_invalid_scheduler_policy_thread_specification_dtor,4)
void __thiscall MSVCRT_invalid_scheduler_policy_thread_specification_dtor(
        invalid_scheduler_policy_thread_specification * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

typedef exception improper_scheduler_attach;
extern const vtable_ptr MSVCRT_improper_scheduler_attach_vtable;

/* ??0improper_scheduler_attach@Concurrency@@QAE@PBD@Z */
/* ??0improper_scheduler_attach@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(improper_scheduler_attach_ctor_str, 8)
improper_scheduler_attach* __thiscall improper_scheduler_attach_ctor_str(
        improper_scheduler_attach *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_improper_scheduler_attach_vtable;
    return this;
}

/* ??0improper_scheduler_attach@Concurrency@@QAE@XZ */
/* ??0improper_scheduler_attach@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(improper_scheduler_attach_ctor, 4)
improper_scheduler_attach* __thiscall improper_scheduler_attach_ctor(
        improper_scheduler_attach *this)
{
    return improper_scheduler_attach_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_scheduler_attach_copy_ctor,8)
improper_scheduler_attach * __thiscall MSVCRT_improper_scheduler_attach_copy_ctor(
        improper_scheduler_attach * _this, const improper_scheduler_attach * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_improper_scheduler_attach_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_scheduler_attach_dtor,4)
void __thiscall MSVCRT_improper_scheduler_attach_dtor(
        improper_scheduler_attach * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

typedef exception improper_scheduler_detach;
extern const vtable_ptr MSVCRT_improper_scheduler_detach_vtable;

/* ??0improper_scheduler_detach@Concurrency@@QAE@PBD@Z */
/* ??0improper_scheduler_detach@Concurrency@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(improper_scheduler_detach_ctor_str, 8)
improper_scheduler_detach* __thiscall improper_scheduler_detach_ctor_str(
        improper_scheduler_detach *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);
    MSVCRT_exception_ctor(this, &str);
    this->vtable = &MSVCRT_improper_scheduler_detach_vtable;
    return this;
}

/* ??0improper_scheduler_detach@Concurrency@@QAE@XZ */
/* ??0improper_scheduler_detach@Concurrency@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(improper_scheduler_detach_ctor, 4)
improper_scheduler_detach* __thiscall improper_scheduler_detach_ctor(
        improper_scheduler_detach *this)
{
    return improper_scheduler_detach_ctor_str(this, NULL);
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_scheduler_detach_copy_ctor,8)
improper_scheduler_detach * __thiscall MSVCRT_improper_scheduler_detach_copy_ctor(
        improper_scheduler_detach * _this, const improper_scheduler_detach * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    MSVCRT_exception_copy_ctor(_this, rhs);
    _this->vtable = &MSVCRT_improper_scheduler_detach_vtable;
    return _this;
}

DEFINE_THISCALL_WRAPPER(MSVCRT_improper_scheduler_detach_dtor,4)
void __thiscall MSVCRT_improper_scheduler_detach_dtor(
        improper_scheduler_detach * _this)
{
    TRACE("(%p)\n", _this);
    MSVCRT_exception_dtor(_this);
}

#endif /* _MSVCR_VER >= 100 */

#ifndef _MSC_VER
#ifndef __GNUC__
void __asm_dummy_vtables(void) {
#endif

__ASM_VTABLE(type_info,
        VTABLE_ADD_FUNC(MSVCRT_type_info_vector_dtor));
__ASM_VTABLE(exception,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
#if _MSVCR_VER >= 80
__ASM_VTABLE(exception_old,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(bad_alloc,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
#endif
__ASM_VTABLE(bad_typeid,
        VTABLE_ADD_FUNC(MSVCRT_bad_typeid_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(bad_cast,
        VTABLE_ADD_FUNC(MSVCRT_bad_cast_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(__non_rtti_object,
        VTABLE_ADD_FUNC(MSVCRT___non_rtti_object_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
#if _MSVCR_VER >= 100
__ASM_VTABLE(scheduler_resource_allocation_error,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(improper_lock,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(invalid_scheduler_policy_key,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(invalid_scheduler_policy_value,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(invalid_scheduler_policy_thread_specification,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(improper_scheduler_attach,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
__ASM_VTABLE(improper_scheduler_detach,
        VTABLE_ADD_FUNC(MSVCRT_exception_vector_dtor)
        VTABLE_ADD_FUNC(MSVCRT_what_exception));
#endif

#ifndef __GNUC__
}
#endif
#endif /* !_MSC_VER */

DEFINE_RTTI_DATA0( type_info, 0, ".?AVtype_info@@" )
#if _MSVCR_VER >= 80
DEFINE_RTTI_DATA0( exception, 0, ".?AVexception@std@@" )
DEFINE_RTTI_DATA0( exception_old, 0, ".?AVexception@@" )
DEFINE_RTTI_DATA1( bad_typeid, 0, &exception_rtti_base_descriptor, ".?AVbad_typeid@std@@" )
DEFINE_RTTI_DATA1( bad_cast, 0, &exception_rtti_base_descriptor, ".?AVbad_cast@std@@" )
DEFINE_RTTI_DATA2( __non_rtti_object, 0, &bad_typeid_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AV__non_rtti_object@std@@" )
DEFINE_RTTI_DATA1( bad_alloc, 0, &exception_rtti_base_descriptor, ".?AVbad_alloc@std@@" )
#else
DEFINE_RTTI_DATA0( exception, 0, ".?AVexception@@" )
DEFINE_RTTI_DATA1( bad_typeid, 0, &exception_rtti_base_descriptor, ".?AVbad_typeid@@" )
DEFINE_RTTI_DATA1( bad_cast, 0, &exception_rtti_base_descriptor, ".?AVbad_cast@@" )
DEFINE_RTTI_DATA2( __non_rtti_object, 0, &bad_typeid_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AV__non_rtti_object@@" )
#endif
#if _MSVCR_VER >= 100
DEFINE_RTTI_DATA1(scheduler_resource_allocation_error, 0, &exception_rtti_base_descriptor,
        ".?AVscheduler_resource_allocation_error@Concurrency@@")
DEFINE_RTTI_DATA1(improper_lock, 0, &exception_rtti_base_descriptor, ".?AVimproper_lock@Concurrency@@" )
DEFINE_RTTI_DATA1(invalid_scheduler_policy_key, 0, &exception_rtti_base_descriptor,
        ".?AVinvalid_scheduler_policy_key@Concurrency@@" )
DEFINE_RTTI_DATA1(invalid_scheduler_policy_value, 0, &exception_rtti_base_descriptor,
        ".?AVinvalid_scheduler_policy_value@Concurrency@@" )
DEFINE_RTTI_DATA1(invalid_scheduler_policy_thread_specification, 0, &exception_rtti_base_descriptor,
        ".?AVinvalid_scheduler_policy_thread_specification@Concurrency@@" )
DEFINE_RTTI_DATA1(improper_scheduler_attach, 0, &exception_rtti_base_descriptor,
        ".?AVimproper_scheduler_attach@Concurrency@@" )
DEFINE_RTTI_DATA1(improper_scheduler_detach, 0, &exception_rtti_base_descriptor,
        ".?AVimproper_scheduler_detach@Concurrency@@" )
#endif

DEFINE_EXCEPTION_TYPE_INFO( exception, 0, NULL, NULL )
DEFINE_EXCEPTION_TYPE_INFO( bad_typeid, 1, &exception_cxx_type_info, NULL )
DEFINE_EXCEPTION_TYPE_INFO( bad_cast, 1, &exception_cxx_type_info, NULL )
DEFINE_EXCEPTION_TYPE_INFO( __non_rtti_object, 2, &bad_typeid_cxx_type_info, &exception_cxx_type_info )
#if _MSVCR_VER >= 80
DEFINE_EXCEPTION_TYPE_INFO( bad_alloc, 1, &exception_cxx_type_info, NULL )
#endif
#if _MSVCR_VER >= 100
DEFINE_EXCEPTION_TYPE_INFO(scheduler_resource_allocation_error, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(improper_lock, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(invalid_scheduler_policy_key, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(invalid_scheduler_policy_value, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(invalid_scheduler_policy_thread_specification, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(improper_scheduler_attach, 1, &exception_cxx_type_info, NULL)
DEFINE_EXCEPTION_TYPE_INFO(improper_scheduler_detach, 1, &exception_cxx_type_info, NULL)
#endif

void msvcrt_init_exception(void *base)
{
#ifdef __x86_64__
    init_type_info_rtti(base);
    init_exception_rtti(base);
#if _MSVCR_VER >= 80
    init_exception_old_rtti(base);
    init_bad_alloc_rtti(base);
#endif
    init_bad_typeid_rtti(base);
    init_bad_cast_rtti(base);
    init___non_rtti_object_rtti(base);
#if _MSVCR_VER >= 100
    init_scheduler_resource_allocation_error_rtti(base);
    init_improper_lock_rtti(base);
    init_invalid_scheduler_policy_key_rtti(base);
    init_invalid_scheduler_policy_value_rtti(base);
    init_invalid_scheduler_policy_thread_specification_rtti(base);
    init_improper_scheduler_attach_rtti(base);
    init_improper_scheduler_detach_rtti(base);
#endif

    init_exception_cxx(base);
    init_bad_typeid_cxx(base);
    init_bad_cast_cxx(base);
    init___non_rtti_object_cxx(base);
#if _MSVCR_VER >= 80
    init_bad_alloc_cxx(base);
#endif
#if _MSVCR_VER >= 100
    init_scheduler_resource_allocation_error_cxx(base);
    init_improper_lock_cxx(base);
    init_invalid_scheduler_policy_key_cxx(base);
    init_invalid_scheduler_policy_value_cxx(base);
    init_invalid_scheduler_policy_thread_specification_cxx(base);
    init_improper_scheduler_attach_cxx(base);
    init_improper_scheduler_detach_cxx(base);
#endif
#endif
}

#if _MSVCR_VER >= 80
void throw_exception(exception_type et, HRESULT hr, const char *str)
{
    switch(et) {
    case EXCEPTION_BAD_ALLOC: {
        bad_alloc e;
        bad_alloc_ctor(&e, &str);
        _CxxThrowException(&e, &bad_alloc_exception_type);
    }
#if _MSVCR_VER >= 100
    case EXCEPTION_SCHEDULER_RESOURCE_ALLOCATION_ERROR: {
        scheduler_resource_allocation_error e;
        scheduler_resource_allocation_error_ctor_name(&e, str, hr);
        _CxxThrowException(&e.e, &scheduler_resource_allocation_error_exception_type);
    }
    case EXCEPTION_IMPROPER_LOCK: {
        improper_lock e;
        improper_lock_ctor_str(&e, str);
        _CxxThrowException(&e, &improper_lock_exception_type);
    }
    case EXCEPTION_INVALID_SCHEDULER_POLICY_KEY: {
        invalid_scheduler_policy_key e;
        invalid_scheduler_policy_key_ctor_str(&e, str);
        _CxxThrowException(&e, &invalid_scheduler_policy_key_exception_type);
    }
    case EXCEPTION_INVALID_SCHEDULER_POLICY_VALUE: {
        invalid_scheduler_policy_value e;
        invalid_scheduler_policy_value_ctor_str(&e, str);
        _CxxThrowException(&e, &invalid_scheduler_policy_value_exception_type);
    }
    case EXCEPTION_INVALID_SCHEDULER_POLICY_THREAD_SPECIFICATION: {
        invalid_scheduler_policy_thread_specification e;
        invalid_scheduler_policy_thread_specification_ctor_str(&e, str);
        _CxxThrowException(&e, &invalid_scheduler_policy_thread_specification_exception_type);
    }
    case EXCEPTION_IMPROPER_SCHEDULER_ATTACH: {
        improper_scheduler_attach e;
        improper_scheduler_attach_ctor_str(&e, str);
        _CxxThrowException(&e, &improper_scheduler_attach_exception_type);
    }
    case EXCEPTION_IMPROPER_SCHEDULER_DETACH: {
        improper_scheduler_detach e;
        improper_scheduler_detach_ctor_str(&e, str);
        _CxxThrowException(&e, &improper_scheduler_detach_exception_type);
    }
#endif
    }
}
#endif

/******************************************************************
 *		?set_terminate@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 *
 * Install a handler to be called when terminate() is called.
 *
 * PARAMS
 *  func [I] Handler function to install
 *
 * RETURNS
 *  The previously installed handler function, if any.
 */
MSVCRT_terminate_function CDECL MSVCRT_set_terminate(MSVCRT_terminate_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    MSVCRT_terminate_function previous = data->terminate_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->terminate_handler = func;
    return previous;
}

/******************************************************************
 *              _get_terminate (MSVCRT.@)
 */
MSVCRT_terminate_function CDECL MSVCRT__get_terminate(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    TRACE("returning %p\n", data->terminate_handler);
    return data->terminate_handler;
}

/******************************************************************
 *		?set_unexpected@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 *
 * Install a handler to be called when unexpected() is called.
 *
 * PARAMS
 *  func [I] Handler function to install
 *
 * RETURNS
 *  The previously installed handler function, if any.
 */
MSVCRT_unexpected_function CDECL MSVCRT_set_unexpected(MSVCRT_unexpected_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    MSVCRT_unexpected_function previous = data->unexpected_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->unexpected_handler = func;
    return previous;
}

/******************************************************************
 *              _get_unexpected (MSVCRT.@)
 */
MSVCRT_unexpected_function CDECL MSVCRT__get_unexpected(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    TRACE("returning %p\n", data->unexpected_handler);
    return data->unexpected_handler;
}

/******************************************************************
 *              ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z  (MSVCRT.@)
 */
MSVCRT__se_translator_function CDECL MSVCRT__set_se_translator(MSVCRT__se_translator_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    MSVCRT__se_translator_function previous = data->se_translator;
    TRACE("(%p) returning %p\n",func,previous);
    data->se_translator = func;
    return previous;
}

/******************************************************************
 *		?terminate@@YAXXZ (MSVCRT.@)
 *
 * Default handler for an unhandled exception.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  This function does not return. Either control resumes from any
 *  handler installed by calling set_terminate(), or (by default) abort()
 *  is called.
 */
void CDECL MSVCRT_terminate(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    if (data->terminate_handler) data->terminate_handler();
    MSVCRT_abort();
}

/******************************************************************
 *		?unexpected@@YAXXZ (MSVCRT.@)
 */
void CDECL MSVCRT_unexpected(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    if (data->unexpected_handler) data->unexpected_handler();
    MSVCRT_terminate();
}


/******************************************************************
 *		__RTtypeid (MSVCRT.@)
 *
 * Retrieve the Run Time Type Information (RTTI) for a C++ object.
 *
 * PARAMS
 *  cppobj [I] C++ object to get type information for.
 *
 * RETURNS
 *  Success: A type_info object describing cppobj.
 *  Failure: If the object to be cast has no RTTI, a __non_rtti_object
 *           exception is thrown. If cppobj is NULL, a bad_typeid exception
 *           is thrown. In either case, this function does not return.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
#ifndef __x86_64__
const type_info* CDECL MSVCRT___RTtypeid(void *cppobj)
{
    const type_info *ret;

    if (!cppobj)
    {
        bad_typeid e;
        MSVCRT_bad_typeid_ctor( &e, "Attempted a typeid of NULL pointer!" );
        _CxxThrowException( &e, &bad_typeid_exception_type );
        return NULL;
    }

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        ret = obj_locator->type_descriptor;
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        MSVCRT___non_rtti_object_ctor( &e, "Bad read pointer - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
        return NULL;
    }
    __ENDTRY
    return ret;
}

#else

const type_info* CDECL MSVCRT___RTtypeid(void *cppobj)
{
    const type_info *ret;

    if (!cppobj)
    {
        bad_typeid e;
        MSVCRT_bad_typeid_ctor( &e, "Attempted a typeid of NULL pointer!" );
        _CxxThrowException( &e, &bad_typeid_exception_type );
        return NULL;
    }

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        char *base;

        if(obj_locator->signature == 0)
            base = RtlPcToFileHeader((void*)obj_locator, (void**)&base);
        else
            base = (char*)obj_locator - obj_locator->object_locator;

        ret = (type_info*)(base + obj_locator->type_descriptor);
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        MSVCRT___non_rtti_object_ctor( &e, "Bad read pointer - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
        return NULL;
    }
    __ENDTRY
    return ret;
}
#endif

/******************************************************************
 *		__RTDynamicCast (MSVCRT.@)
 *
 * Dynamically cast a C++ object to one of its base classes.
 *
 * PARAMS
 *  cppobj   [I] Any C++ object to cast
 *  unknown  [I] Reserved, set to 0
 *  src      [I] type_info object describing cppobj
 *  dst      [I] type_info object describing the base class to cast to
 *  do_throw [I] TRUE = throw an exception if the cast fails, FALSE = don't
 *
 * RETURNS
 *  Success: The address of cppobj, cast to the object described by dst.
 *  Failure: NULL, If the object to be cast has no RTTI, or dst is not a
 *           valid cast for cppobj. If do_throw is TRUE, a bad_cast exception
 *           is thrown and this function does not return.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
#ifndef __x86_64__
void* CDECL MSVCRT___RTDynamicCast(void *cppobj, int unknown,
                                   type_info *src, type_info *dst,
                                   int do_throw)
{
    void *ret;

    if (!cppobj) return NULL;

    TRACE("obj: %p unknown: %d src: %p %s dst: %p %s do_throw: %d)\n",
          cppobj, unknown, src, dbgstr_type_info(src), dst, dbgstr_type_info(dst), do_throw);

    /* To cast an object at runtime:
     * 1.Find out the true type of the object from the typeinfo at vtable[-1]
     * 2.Search for the destination type in the class hierarchy
     * 3.If destination type is found, return base object address + dest offset
     *   Otherwise, fail the cast
     *
     * FIXME: the unknown parameter doesn't seem to be used for anything
     */
    __TRY
    {
        int i;
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        const rtti_object_hierarchy *obj_bases = obj_locator->type_hierarchy;
        const rtti_base_descriptor * const* base_desc = obj_bases->base_classes->bases;

        if (TRACE_ON(msvcrt)) dump_obj_locator(obj_locator);

        ret = NULL;
        for (i = 0; i < obj_bases->array_len; i++)
        {
            const type_info *typ = base_desc[i]->type_descriptor;

            if (!strcmp(typ->mangled, dst->mangled))
            {
                /* compute the correct this pointer for that base class */
                void *this_ptr = (char *)cppobj - obj_locator->base_class_offset;
                ret = get_this_pointer( &base_desc[i]->offsets, this_ptr );
                break;
            }
        }
        /* VC++ sets do_throw to 1 when the result of a dynamic_cast is assigned
         * to a reference, since references cannot be NULL.
         */
        if (!ret && do_throw)
        {
            const char *msg = "Bad dynamic_cast!";
            bad_cast e;
            MSVCRT_bad_cast_ctor( &e, &msg );
            _CxxThrowException( &e, &bad_cast_exception_type );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        MSVCRT___non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
        return NULL;
    }
    __ENDTRY
    return ret;
}

#else

void* CDECL MSVCRT___RTDynamicCast(void *cppobj, int unknown,
        type_info *src, type_info *dst,
        int do_throw)
{
    void *ret;

    if (!cppobj) return NULL;

    TRACE("obj: %p unknown: %d src: %p %s dst: %p %s do_throw: %d)\n",
            cppobj, unknown, src, dbgstr_type_info(src), dst, dbgstr_type_info(dst), do_throw);

    __TRY
    {
        int i;
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        const rtti_object_hierarchy *obj_bases;
        const rtti_base_array *base_array;
        char *base;

        if (TRACE_ON(msvcrt)) dump_obj_locator(obj_locator);

        if(obj_locator->signature == 0)
            base = RtlPcToFileHeader((void*)obj_locator, (void**)&base);
        else
            base = (char*)obj_locator - obj_locator->object_locator;

        obj_bases = (const rtti_object_hierarchy*)(base + obj_locator->type_hierarchy);
        base_array = (const rtti_base_array*)(base + obj_bases->base_classes);

        ret = NULL;
        for (i = 0; i < obj_bases->array_len; i++)
        {
            const rtti_base_descriptor *base_desc = (const rtti_base_descriptor*)(base + base_array->bases[i]);
            const type_info *typ = (const type_info*)(base + base_desc->type_descriptor);

            if (!strcmp(typ->mangled, dst->mangled))
            {
                void *this_ptr = (char *)cppobj - obj_locator->base_class_offset;
                ret = get_this_pointer( &base_desc->offsets, this_ptr );
                break;
            }
        }
        if (!ret && do_throw)
        {
            const char *msg = "Bad dynamic_cast!";
            bad_cast e;
            MSVCRT_bad_cast_ctor( &e, &msg );
            _CxxThrowException( &e, &bad_cast_exception_type );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        MSVCRT___non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
        return NULL;
    }
    __ENDTRY
    return ret;
}
#endif


/******************************************************************
 *		__RTCastToVoid (MSVCRT.@)
 *
 * Dynamically cast a C++ object to a void*.
 *
 * PARAMS
 *  cppobj [I] The C++ object to cast
 *
 * RETURNS
 *  Success: The base address of the object as a void*.
 *  Failure: NULL, if cppobj is NULL or has no RTTI.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
void* CDECL MSVCRT___RTCastToVoid(void *cppobj)
{
    void *ret;

    if (!cppobj) return NULL;

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        ret = (char *)cppobj - obj_locator->base_class_offset;
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        MSVCRT___non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
        return NULL;
    }
    __ENDTRY
    return ret;
}


/*********************************************************************
 *		_CxxThrowException (MSVCRT.@)
 */
#ifndef __x86_64__
void WINAPI _CxxThrowException( exception *object, const cxx_exception_type *type )
{
    ULONG_PTR args[3];

    args[0] = CXX_FRAME_MAGIC_VC6;
    args[1] = (ULONG_PTR)object;
    args[2] = (ULONG_PTR)type;
    RaiseException( CXX_EXCEPTION, EH_NONCONTINUABLE, 3, args );
}
#else
void WINAPI _CxxThrowException( exception *object, const cxx_exception_type *type )
{
    ULONG_PTR args[4];

    args[0] = CXX_FRAME_MAGIC_VC6;
    args[1] = (ULONG_PTR)object;
    args[2] = (ULONG_PTR)type;
    RtlPcToFileHeader( (void*)type, (void**)&args[3]);
    RaiseException( CXX_EXCEPTION, EH_NONCONTINUABLE, 4, args );
}
#endif

#if _MSVCR_VER >= 80

/*********************************************************************
 * ?_is_exception_typeof@@YAHABVtype_info@@PAU_EXCEPTION_POINTERS@@@Z
 * ?_is_exception_typeof@@YAHAEBVtype_info@@PEAU_EXCEPTION_POINTERS@@@Z
 */
#ifndef __x86_64__
int __cdecl _is_exception_typeof(const type_info *ti, EXCEPTION_POINTERS *ep)
{
    int ret = -1;

    TRACE("(%p %p)\n", ti, ep);

    __TRY
    {
        EXCEPTION_RECORD *rec = ep->ExceptionRecord;

        if (rec->ExceptionCode==CXX_EXCEPTION && rec->NumberParameters==3 &&
                (rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC6 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC7 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC8))
        {
            const cxx_type_info_table *tit = ((cxx_exception_type*)rec->ExceptionInformation[2])->type_info_table;
            int i;

            for (i=0; i<tit->count; i++) {
                if (ti==tit->info[i]->type_info || !strcmp(ti->mangled, tit->info[i]->type_info->mangled))
                {
                    ret = 1;
                    break;
                }
            }

            if (i == tit->count)
                ret = 0;
        }
    }
    __EXCEPT_PAGE_FAULT
    __ENDTRY

    if(ret == -1)
        MSVCRT_terminate();
    return ret;
}
#else
int __cdecl _is_exception_typeof(const type_info *ti, EXCEPTION_POINTERS *ep)
{
    int ret = -1;

    TRACE("(%p %p)\n", ti, ep);

    __TRY
    {
        EXCEPTION_RECORD *rec = ep->ExceptionRecord;

        if (rec->ExceptionCode==CXX_EXCEPTION && rec->NumberParameters==4 &&
                (rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC6 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC7 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC8))
        {
            const cxx_exception_type *et = (cxx_exception_type*)rec->ExceptionInformation[2];
            const cxx_type_info_table *tit = (const cxx_type_info_table*)(rec->ExceptionInformation[3]+et->type_info_table);
            int i;

            for (i=0; i<tit->count; i++) {
                const cxx_type_info *cti = (const cxx_type_info*)(rec->ExceptionInformation[3]+tit->info[i]);
                const type_info *except_ti = (const type_info*)(rec->ExceptionInformation[3]+cti->type_info);
                if (ti==except_ti || !strcmp(ti->mangled, except_ti->mangled))
                {
                    ret = 1;
                    break;
                }
            }

            if (i == tit->count)
                ret = 0;
        }
    }
    __EXCEPT_PAGE_FAULT
    __ENDTRY

    if(ret == -1)
        MSVCRT_terminate();
    return ret;
}
#endif

/*********************************************************************
 * __clean_type_info_names_internal (MSVCR80.@)
 */
void CDECL __clean_type_info_names_internal(void *p)
{
    FIXME("(%p) stub\n", p);
}

/*********************************************************************
 * ?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z (MSVCR100.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_name_internal_method,8)
const char * __thiscall type_info_name_internal_method(type_info * _this, struct __type_info_node *node)
{
    static int once;
    if (node && !once++) FIXME("type_info_node parameter ignored\n");

    return MSVCRT_type_info_name(_this);
}

#endif /* _MSVCR_VER >= 80 */

/* std::exception_ptr class helpers */
typedef struct
{
    EXCEPTION_RECORD *rec;
    int *ref; /* not binary compatible with native msvcr100 */
} exception_ptr;

#if _MSVCR_VER >= 100

/*********************************************************************
 * ?__ExceptionPtrCreate@@YAXPAX@Z
 * ?__ExceptionPtrCreate@@YAXPEAX@Z
 */
void __cdecl __ExceptionPtrCreate(exception_ptr *ep)
{
    TRACE("(%p)\n", ep);

    ep->rec = NULL;
    ep->ref = NULL;
}

#if defined(__i386__) && !defined(__MINGW32__)
extern void call_dtor(const cxx_exception_type *type, void *func, void *object);

__ASM_GLOBAL_FUNC( call_dtor,
                   "movl 12(%esp),%ecx\n\t"
                   "call *8(%esp)\n\t"
                   "ret" );
#elif __x86_64__
static inline void call_dtor(const cxx_exception_type *type, unsigned int dtor, void *object)
{
    char *base = RtlPcToFileHeader((void*)type, (void**)&base);
    void (__cdecl *func)(void*) = (void*)(base + dtor);
    func(object);
}
#else
#define call_dtor(type, func, object) ((void (__thiscall*)(void*))(func))(object)
#endif

/*********************************************************************
 * ?__ExceptionPtrDestroy@@YAXPAX@Z
 * ?__ExceptionPtrDestroy@@YAXPEAX@Z
 */
void __cdecl __ExceptionPtrDestroy(exception_ptr *ep)
{
    TRACE("(%p)\n", ep);

    if (!ep->rec)
        return;

    if (!InterlockedDecrement(ep->ref))
    {
        if (ep->rec->ExceptionCode == CXX_EXCEPTION)
        {
            const cxx_exception_type *type = (void*)ep->rec->ExceptionInformation[2];
            void *obj = (void*)ep->rec->ExceptionInformation[1];

            if (type && type->destructor) call_dtor(type, type->destructor, obj);
            HeapFree(GetProcessHeap(), 0, obj);
        }

        HeapFree(GetProcessHeap(), 0, ep->rec);
        HeapFree(GetProcessHeap(), 0, ep->ref);
    }
}

/*********************************************************************
 * ?__ExceptionPtrCopy@@YAXPAXPBX@Z
 * ?__ExceptionPtrCopy@@YAXPEAXPEBX@Z
 */
void __cdecl __ExceptionPtrCopy(exception_ptr *ep, const exception_ptr *copy)
{
    TRACE("(%p %p)\n", ep, copy);

    /* don't destroy object stored in ep */
    *ep = *copy;
    if (ep->ref)
        InterlockedIncrement(copy->ref);
}

/*********************************************************************
 * ?__ExceptionPtrAssign@@YAXPAXPBX@Z
 * ?__ExceptionPtrAssign@@YAXPEAXPEBX@Z
 */
void __cdecl __ExceptionPtrAssign(exception_ptr *ep, const exception_ptr *assign)
{
    TRACE("(%p %p)\n", ep, assign);

    /* don't destroy object stored in ep */
    if (ep->ref)
        InterlockedDecrement(ep->ref);

    *ep = *assign;
    if (ep->ref)
        InterlockedIncrement(ep->ref);
}

#endif /* _MSVCR_VER >= 100 */

/*********************************************************************
 * ?__ExceptionPtrRethrow@@YAXPBX@Z
 * ?__ExceptionPtrRethrow@@YAXPEBX@Z
 */
void __cdecl __ExceptionPtrRethrow(const exception_ptr *ep)
{
    TRACE("(%p)\n", ep);

    if (!ep->rec)
    {
        static const char *exception_msg = "bad exception";
        exception e;

        MSVCRT_exception_ctor(&e, &exception_msg);
        _CxxThrowException(&e, &exception_exception_type);
        return;
    }

    RaiseException(ep->rec->ExceptionCode, ep->rec->ExceptionFlags & (~EH_UNWINDING),
            ep->rec->NumberParameters, ep->rec->ExceptionInformation);
}

#if _MSVCR_VER >= 100

#ifdef __i386__
extern void call_copy_ctor( void *func, void *this, void *src, int has_vbase );
#else
static inline void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
    TRACE( "calling copy ctor %p object %p src %p\n", func, this, src );
    if (has_vbase)
        ((void (__cdecl*)(void*, void*, BOOL))func)(this, src, 1);
    else
        ((void (__cdecl*)(void*, void*))func)(this, src);
}
#endif

/*********************************************************************
 * ?__ExceptionPtrCurrentException@@YAXPAX@Z
 * ?__ExceptionPtrCurrentException@@YAXPEAX@Z
 */
#ifndef __x86_64__
void __cdecl __ExceptionPtrCurrentException(exception_ptr *ep)
{
    EXCEPTION_RECORD *rec = msvcrt_get_thread_data()->exc_record;

    TRACE("(%p)\n", ep);

    if (!rec)
    {
        ep->rec = NULL;
        ep->ref = NULL;
        return;
    }

    ep->rec = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCEPTION_RECORD));
    ep->ref = HeapAlloc(GetProcessHeap(), 0, sizeof(int));

    *ep->rec = *rec;
    *ep->ref = 1;

    if (ep->rec->ExceptionCode == CXX_EXCEPTION)
    {
        const cxx_exception_type *et = (void*)ep->rec->ExceptionInformation[2];
        const cxx_type_info *ti;
        void **data, *obj;

        ti = et->type_info_table->info[0];
        data = HeapAlloc(GetProcessHeap(), 0, ti->size);

        obj = (void*)ep->rec->ExceptionInformation[1];
        if (ti->flags & CLASS_IS_SIMPLE_TYPE)
        {
            memcpy(data, obj, ti->size);
            if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
        }
        else if (ti->copy_ctor)
        {
            call_copy_ctor(ti->copy_ctor, data, get_this_pointer(&ti->offsets, obj),
                    ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
        }
        else
            memcpy(data, get_this_pointer(&ti->offsets, obj), ti->size);
        ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
    }
    return;
}
#else
void __cdecl __ExceptionPtrCurrentException(exception_ptr *ep)
{
    EXCEPTION_RECORD *rec = msvcrt_get_thread_data()->exc_record;

    TRACE("(%p)\n", ep);

    if (!rec)
    {
        ep->rec = NULL;
        ep->ref = NULL;
        return;
    }

    ep->rec = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCEPTION_RECORD));
    ep->ref = HeapAlloc(GetProcessHeap(), 0, sizeof(int));

    *ep->rec = *rec;
    *ep->ref = 1;

    if (ep->rec->ExceptionCode == CXX_EXCEPTION)
    {
        const cxx_exception_type *et = (void*)ep->rec->ExceptionInformation[2];
        const cxx_type_info *ti;
        void **data, *obj;
        char *base = RtlPcToFileHeader((void*)et, (void**)&base);

        ti = (const cxx_type_info*)(base + ((const cxx_type_info_table*)(base + et->type_info_table))->info[0]);
        data = HeapAlloc(GetProcessHeap(), 0, ti->size);

        obj = (void*)ep->rec->ExceptionInformation[1];
        if (ti->flags & CLASS_IS_SIMPLE_TYPE)
        {
            memcpy(data, obj, ti->size);
            if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
        }
        else if (ti->copy_ctor)
        {
            call_copy_ctor(base + ti->copy_ctor, data, get_this_pointer(&ti->offsets, obj),
                    ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
        }
        else
            memcpy(data, get_this_pointer(&ti->offsets, obj), ti->size);
        ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
    }
    return;
}
#endif

#endif /* _MSVCR_VER >= 100 */

#if _MSVCR_VER >= 110
/*********************************************************************
 * ?__ExceptionPtrToBool@@YA_NPBX@Z
 * ?__ExceptionPtrToBool@@YA_NPEBX@Z
 */
MSVCRT_bool __cdecl __ExceptionPtrToBool(exception_ptr *ep)
{
    return !!ep->rec;
}
#endif

#if _MSVCR_VER >= 100

/*********************************************************************
 * ?__ExceptionPtrCopyException@@YAXPAXPBX1@Z
 * ?__ExceptionPtrCopyException@@YAXPEAXPEBX1@Z
 */
#ifndef __x86_64__
void __cdecl __ExceptionPtrCopyException(exception_ptr *ep,
        exception *object, const cxx_exception_type *type)
{
    const cxx_type_info *ti;
    void **data;

    __ExceptionPtrDestroy(ep);

    ep->rec = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCEPTION_RECORD));
    ep->ref = HeapAlloc(GetProcessHeap(), 0, sizeof(int));
    *ep->ref = 1;

    memset(ep->rec, 0, sizeof(EXCEPTION_RECORD));
    ep->rec->ExceptionCode = CXX_EXCEPTION;
    ep->rec->ExceptionFlags = EH_NONCONTINUABLE;
    ep->rec->NumberParameters = 3;
    ep->rec->ExceptionInformation[0] = CXX_FRAME_MAGIC_VC6;
    ep->rec->ExceptionInformation[2] = (ULONG_PTR)type;

    ti = type->type_info_table->info[0];
    data = HeapAlloc(GetProcessHeap(), 0, ti->size);
    if (ti->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memcpy(data, object, ti->size);
        if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
    }
    else if (ti->copy_ctor)
    {
        call_copy_ctor(ti->copy_ctor, data, get_this_pointer(&ti->offsets, object),
                ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
    }
    else
        memcpy(data, get_this_pointer(&ti->offsets, object), ti->size);
    ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
}
#else
void __cdecl __ExceptionPtrCopyException(exception_ptr *ep,
        exception *object, const cxx_exception_type *type)
{
    const cxx_type_info *ti;
    void **data;
    char *base;

    RtlPcToFileHeader((void*)type, (void**)&base);
    __ExceptionPtrDestroy(ep);

    ep->rec = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCEPTION_RECORD));
    ep->ref = HeapAlloc(GetProcessHeap(), 0, sizeof(int));
    *ep->ref = 1;

    memset(ep->rec, 0, sizeof(EXCEPTION_RECORD));
    ep->rec->ExceptionCode = CXX_EXCEPTION;
    ep->rec->ExceptionFlags = EH_NONCONTINUABLE;
    ep->rec->NumberParameters = 4;
    ep->rec->ExceptionInformation[0] = CXX_FRAME_MAGIC_VC6;
    ep->rec->ExceptionInformation[2] = (ULONG_PTR)type;
    ep->rec->ExceptionInformation[3] = (ULONG_PTR)base;

    ti = (const cxx_type_info*)(base + ((const cxx_type_info_table*)(base + type->type_info_table))->info[0]);
    data = HeapAlloc(GetProcessHeap(), 0, ti->size);
    if (ti->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memcpy(data, object, ti->size);
        if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
    }
    else if (ti->copy_ctor)
    {
        call_copy_ctor(base + ti->copy_ctor, data, get_this_pointer(&ti->offsets, object),
                ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
    }
    else
        memcpy(data, get_this_pointer(&ti->offsets, object), ti->size);
    ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
}
#endif

MSVCRT_bool __cdecl __ExceptionPtrCompare(const exception_ptr *ep1, const exception_ptr *ep2)
{
    return ep1->rec == ep2->rec;
}

#endif /* _MSVCR_VER >= 100 */

#if _MSVCR_VER >= 80
void* __cdecl __AdjustPointer(void *obj, const this_ptr_offsets *off)
{
    return get_this_pointer(off, obj);
}
#endif

#if _MSVCR_VER >= 140

typedef struct
{
    char *name;
    char mangled[1];
} type_info140;

typedef struct
{
    SLIST_ENTRY entry;
    char name[1];
} type_info_entry;

static void* CDECL type_info_entry_malloc(MSVCRT_size_t size)
{
    type_info_entry *ret = MSVCRT_malloc(FIELD_OFFSET(type_info_entry, name) + size);
    return ret->name;
}

static void CDECL type_info_entry_free(void *ptr)
{
    ptr = (char*)ptr - FIELD_OFFSET(type_info_entry, name);
    MSVCRT_free(ptr);
}

/******************************************************************
 *		__std_type_info_compare (UCRTBASE.@)
 */
int CDECL MSVCRT_type_info_compare(const type_info140 *l, const type_info140 *r)
{
    int ret;

    if (l == r) ret = 0;
    else ret = MSVCRT_strcmp(l->mangled + 1, r->mangled + 1);
    TRACE("(%p %p) returning %d\n", l, r, ret);
    return ret;
}

/******************************************************************
 *		__std_type_info_name (UCRTBASE.@)
 */
const char* CDECL MSVCRT_type_info_name_list(type_info140 *ti, SLIST_HEADER *header)
{
      if (!ti->name)
      {
          char* name = __unDName(0, ti->mangled + 1, 0,
                  type_info_entry_malloc, type_info_entry_free, UNDNAME_NO_ARGUMENTS | UNDNAME_32_BIT_DECODE);
          if (name)
          {
              unsigned int len = strlen(name);

              while (len && name[--len] == ' ')
                  name[len] = '\0';

              if (InterlockedCompareExchangePointer((void**)&ti->name, name, NULL))
              {
                  type_info_entry_free(name);
              }
              else
              {
                  type_info_entry *entry = (type_info_entry*)(name-FIELD_OFFSET(type_info_entry, name));
                  InterlockedPushEntrySList(header, &entry->entry);
              }
          }
      }
      TRACE("(%p) returning %s\n", ti, ti->name);
      return ti->name;
}

/******************************************************************
 *		__std_type_info_destroy_list  (UCRTBASE.@)
 */
void CDECL MSVCRT_type_info_destroy_list(SLIST_HEADER *header)
{
    SLIST_ENTRY *cur, *next;

    TRACE("(%p)\n", header);

    for(cur = InterlockedFlushSList(header); cur; cur = next)
    {
        next = cur->Next;
        MSVCRT_free(cur);
    }
}

/******************************************************************
 *              __std_type_info_hash (UCRTBASE.@)
 */
MSVCRT_size_t CDECL MSVCRT_type_info_hash(const type_info140 *ti)
{
    MSVCRT_size_t hash, fnv_prime;
    const char *p;

#ifdef _WIN64
    hash = 0xcbf29ce484222325;
    fnv_prime = 0x100000001b3;
#else
    hash = 0x811c9dc5;
    fnv_prime = 0x1000193;
#endif

    TRACE("(%p)->%s\n", ti, ti->mangled);

    for(p = ti->mangled+1; *p; p++) {
        hash ^= *p;
        hash *= fnv_prime;
    }

#ifdef _WIN64
    hash ^= hash >> 32;
#endif

    return hash;
}

#endif /* _MSVCR_VER >= 140 */

#if _MSVCR_VER >= 100

enum ConcRT_EventType
{
    CONCRT_EVENT_GENERIC,
    CONCRT_EVENT_START,
    CONCRT_EVENT_END,
    CONCRT_EVENT_BLOCK,
    CONCRT_EVENT_UNBLOCK,
    CONCRT_EVENT_YIELD,
    CONCRT_EVENT_ATTACH,
    CONCRT_EVENT_DETACH
};

/* ?_Trace_ppl_function@Concurrency@@YAXABU_GUID@@EW4ConcRT_EventType@1@@Z */
/* ?_Trace_ppl_function@Concurrency@@YAXAEBU_GUID@@EW4ConcRT_EventType@1@@Z */
void __cdecl Concurrency__Trace_ppl_function(const GUID *guid, unsigned char level, enum ConcRT_EventType type)
{
    FIXME("(%s %u %i) stub\n", debugstr_guid(guid), level, type);
}

#endif /* _MSVCR_VER >= 100 */
