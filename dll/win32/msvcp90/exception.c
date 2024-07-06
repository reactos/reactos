/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <errno.h>
#include <stdarg.h>

#include "msvcp90.h"
#include "windef.h"
#include "winbase.h"
#ifndef __REACTOS__
#include "winternl.h"
#include "rtlsupportapi.h"
#endif
#include "wine/debug.h"

#ifdef __REACTOS__
    #undef RTTI_USE_RVA
#endif

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#define CXX_FRAME_MAGIC_VC6 0x19930520

#ifdef RTTI_USE_RVA
#define CXX_EXCEPTION_PARAMS 4
#else
#define CXX_EXCEPTION_PARAMS 3
#endif

CREATE_TYPE_INFO_VTABLE

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

int* __cdecl __processing_throw(void);

#if _MSVCP_VER >= 70 || defined(_MSVCIRT)
typedef const char **exception_name;
#define EXCEPTION_STR(name) (*name)
#define EXCEPTION_NAME(str) (&str)
#else
typedef const char *exception_name;
#define EXCEPTION_STR(name) (name)
#define EXCEPTION_NAME(str) (str)
#endif

void (CDECL *_Raise_handler)(const exception*) = NULL;

/* vtables */
extern const vtable_ptr exception_vtable;
/* ??_7bad_alloc@std@@6B@ */
extern const vtable_ptr bad_alloc_vtable;
/* ??_7logic_error@std@@6B@ */
extern const vtable_ptr logic_error_vtable;
/* ??_7length_error@std@@6B@ */
extern const vtable_ptr length_error_vtable;
/* ??_7out_of_range@std@@6B@ */
extern const vtable_ptr out_of_range_vtable;
extern const vtable_ptr invalid_argument_vtable;
/* ??_7runtime_error@std@@6B@ */
extern const vtable_ptr runtime_error_vtable;
extern const vtable_ptr _System_error_vtable;
extern const vtable_ptr system_error_vtable;
extern const vtable_ptr failure_vtable;
/* ??_7bad_cast@std@@6B@ */
extern const vtable_ptr bad_cast_vtable;
/* ??_7range_error@std@@6B@ */
extern const vtable_ptr range_error_vtable;
/* ??_7bad_function_call@std@@6B@ */
extern const vtable_ptr bad_function_call_vtable;

/* ??0exception@@QAE@ABQBD@Z */
/* ??0exception@@QEAA@AEBQEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_exception_ctor,8)
exception* __thiscall MSVCP_exception_ctor(exception *this, exception_name name)
{
    TRACE("(%p %s)\n", this, EXCEPTION_STR(name));

    this->vtable = &exception_vtable;
    if(EXCEPTION_STR(name)) {
        unsigned int name_len = strlen(EXCEPTION_STR(name)) + 1;
        this->name = malloc(name_len);
        memcpy(this->name, EXCEPTION_STR(name), name_len);
        this->do_free = TRUE;
    } else {
        this->name = NULL;
        this->do_free = FALSE;
    }
    return this;
}

DEFINE_THISCALL_WRAPPER(exception_copy_ctor,8)
exception* __thiscall exception_copy_ctor(exception *this, const exception *rhs)
{
    TRACE("(%p,%p)\n", this, rhs);

    if(!rhs->do_free) {
        this->vtable = &exception_vtable;
        this->name = rhs->name;
        this->do_free = FALSE;
    } else
        MSVCP_exception_ctor(this, (exception_name)EXCEPTION_NAME(rhs->name));
    TRACE("name = %s\n", this->name);
    return this;
}

/* ??0exception@@QAE@XZ */
/* ??0exception@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_exception_default_ctor,4)
exception* __thiscall MSVCP_exception_default_ctor(exception *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &exception_vtable;
    this->name = NULL;
    this->do_free = FALSE;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_dtor,4)
void __thiscall MSVCP_exception_dtor(exception *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &exception_vtable;
    if(this->do_free)
        free(this->name);
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_vector_dtor, 8)
void * __thiscall MSVCP_exception_vector_dtor(exception *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_exception_dtor(this+i);
        operator_delete(ptr);
    } else {
        MSVCP_exception_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??_Gexception@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_exception_scalar_dtor, 8)
void * __thiscall MSVCP_exception_scalar_dtor(exception *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    MSVCP_exception_dtor(this);
    if (flags & 1) operator_delete(this);
    return this;
}

/* ??4exception@@QAEAAV0@ABV0@@Z */
/* ??4exception@@QEAAAEAV0@AEBV0@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_exception_assign, 8)
exception* __thiscall MSVCP_exception_assign(exception *this, const exception *assign)
{
    MSVCP_exception_dtor(this);
    return exception_copy_ctor(this, assign);
}

/* ?_Doraise@bad_alloc@std@@MBEXXZ */
/* ?_Doraise@bad_alloc@std@@MEBAXXZ */
/* ?_Doraise@logic_error@std@@MBEXXZ */
/* ?_Doraise@logic_error@std@@MEBAXXZ */
/* ?_Doraise@length_error@std@@MBEXXZ */
/* ?_Doraise@length_error@std@@MEBAXXZ */
/* ?_Doraise@out_of_range@std@@MBEXXZ */
/* ?_Doraise@out_of_range@std@@MEBAXXZ */
/* ?_Doraise@runtime_error@std@@MBEXXZ */
/* ?_Doraise@runtime_error@std@@MEBAXXZ */
/* ?_Doraise@bad_cast@std@@MBEXXZ */
/* ?_Doraise@bad_cast@std@@MEBAXXZ */
/* ?_Doraise@range_error@std@@MBEXXZ */
/* ?_Doraise@range_error@std@@MEBAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_exception__Doraise, 4)
void __thiscall MSVCP_exception__Doraise(exception *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_exception_what,4)
const char* __thiscall MSVCP_exception_what(exception * this)
{
    TRACE("(%p) returning %s\n", this, this->name);
    return this->name ? this->name : "Unknown exception";
}

#if _MSVCP_VER >= 80
DEFINE_RTTI_DATA0(exception, 0, ".?AVexception@std@@")
#else
DEFINE_RTTI_DATA0(exception, 0, ".?AVexception@@")
#endif
DEFINE_CXX_DATA0(exception, MSVCP_exception_dtor)

/* bad_alloc class data */
typedef exception bad_alloc;

/* ??0bad_alloc@std@@QAE@PBD@Z */
/* ??0bad_alloc@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_ctor, 8)
bad_alloc* __thiscall MSVCP_bad_alloc_ctor(bad_alloc *this, exception_name name)
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_exception_ctor(this, name);
    this->vtable = &bad_alloc_vtable;
    return this;
}

/* ??0bad_alloc@std@@QAE@XZ */
/* ??0bad_alloc@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_default_ctor, 4)
bad_alloc* __thiscall MSVCP_bad_alloc_default_ctor(bad_alloc *this)
{
    static const char *name = "bad allocation";
    return MSVCP_bad_alloc_ctor(this, EXCEPTION_NAME(name));
}

/* ??0bad_alloc@std@@QAE@ABV01@@Z */
/* ??0bad_alloc@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(bad_alloc_copy_ctor, 8)
bad_alloc* __thiscall bad_alloc_copy_ctor(bad_alloc *this, const bad_alloc *rhs)
{
    TRACE("%p %p\n", this, rhs);
    exception_copy_ctor(this, rhs);
    this->vtable = &bad_alloc_vtable;
    return this;
}

/* ??1bad_alloc@std@@UAE@XZ */
/* ??1bad_alloc@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_dtor, 4)
void __thiscall MSVCP_bad_alloc_dtor(bad_alloc *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_vector_dtor, 8)
void * __thiscall MSVCP_bad_alloc_vector_dtor(bad_alloc *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_bad_alloc_dtor(this+i);
        operator_delete(ptr);
    } else {
        MSVCP_bad_alloc_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??4bad_alloc@std@@QAEAAV01@ABV01@@Z */
/* ??4bad_alloc@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_alloc_assign, 8)
bad_alloc* __thiscall MSVCP_bad_alloc_assign(bad_alloc *this, const bad_alloc *assign)
{
    MSVCP_bad_alloc_dtor(this);
    return bad_alloc_copy_ctor(this, assign);
}

DEFINE_RTTI_DATA1(bad_alloc, 0, &exception_rtti_base_descriptor, ".?AVbad_alloc@std@@")
DEFINE_CXX_DATA1(bad_alloc, &exception_cxx_type_info, MSVCP_bad_alloc_dtor)

/* logic_error class data */
typedef struct {
    exception e;
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    basic_string_char str;
#endif
} logic_error;

/* ??0logic_error@@QAE@ABQBD@Z */
/* ??0logic_error@@QEAA@AEBQEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_ctor, 8)
logic_error* __thiscall MSVCP_logic_error_ctor( logic_error *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
#if _MSVCP_VER == 60
    MSVCP_exception_ctor(&this->e, "");
#else
    MSVCP_exception_default_ctor(&this->e);
#endif
    MSVCP_basic_string_char_ctor_cstr(&this->str, EXCEPTION_STR(name));
#else
    MSVCP_exception_ctor(&this->e, name);
#endif
    this->e.vtable = &logic_error_vtable;
    return this;
}

/* ??0logic_error@std@@QAE@ABV01@@Z */
/* ??0logic_error@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(logic_error_copy_ctor, 8)
logic_error* __thiscall logic_error_copy_ctor(
        logic_error *this, const logic_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    exception_copy_ctor(&this->e, &rhs->e);
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    MSVCP_basic_string_char_copy_ctor(&this->str, &rhs->str);
#endif
    this->e.vtable = &logic_error_vtable;
    return this;
}

/* ??0logic_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0logic_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
#ifndef _MSVCIRT
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_ctor_bstr, 8)
logic_error* __thiscall MSVCP_logic_error_ctor_bstr(logic_error *this, const basic_string_char *str)
{
    const char *name = MSVCP_basic_string_char_c_str(str);
    TRACE("(%p %p %s)\n", this, str, name);
    return MSVCP_logic_error_ctor(this, EXCEPTION_NAME(name));
}
#endif

/* ??1logic_error@std@@UAE@XZ */
/* ??1logic_error@std@@UEAA@XZ */
/* ??1length_error@std@@UAE@XZ */
/* ??1length_error@std@@UEAA@XZ */
/* ??1out_of_range@std@@UAE@XZ */
/* ??1out_of_range@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_dtor, 4)
void __thiscall MSVCP_logic_error_dtor(logic_error *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(&this->e);
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    MSVCP_basic_string_char_dtor(&this->str);
#endif
}

DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_vector_dtor, 8)
void* __thiscall MSVCP_logic_error_vector_dtor(
        logic_error *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_logic_error_dtor(this+i);
        operator_delete(ptr);
    } else {
        MSVCP_logic_error_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??_Glogic_error@@UAEPAXI@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_scalar_dtor, 8)
void * __thiscall MSVCP_logic_error_scalar_dtor(logic_error *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    MSVCP_logic_error_dtor(this);
    if (flags & 1) operator_delete(this);
    return this;
}

/* ??4logic_error@std@@QAEAAV01@ABV01@@Z */
/* ??4logic_error@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_assign, 8)
logic_error* __thiscall MSVCP_logic_error_assign(logic_error *this, const logic_error *assign)
{
    MSVCP_logic_error_dtor(this);
    return logic_error_copy_ctor(this, assign);
}

/* ?what@logic_error@std@@UBEPBDXZ */
/* ?what@logic_error@std@@UEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_logic_error_what, 4)
const char* __thiscall MSVCP_logic_error_what(logic_error *this)
{
    TRACE("%p\n", this);
#if _MSVCP_VER > 90 || defined _MSVCIRT
    return MSVCP_exception_what( &this->e );
#else
    return MSVCP_basic_string_char_c_str(&this->str);
#endif
}

#if _MSVCP_VER >= 80
DEFINE_RTTI_DATA1(logic_error, 0, &exception_rtti_base_descriptor, ".?AVlogic_error@std@@")
#else
DEFINE_RTTI_DATA1(logic_error, 0, &exception_rtti_base_descriptor, ".?AVlogic_error@@")
#endif
DEFINE_CXX_TYPE_INFO(logic_error)

/* length_error class data */
typedef logic_error length_error;

static length_error* MSVCP_length_error_ctor( length_error *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &length_error_vtable;
    return this;
}

/* ??0length_error@std@@QAE@ABV01@@Z */
/* ??0length_error@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(length_error_copy_ctor, 8)
length_error* __thiscall length_error_copy_ctor(
        length_error *this, const length_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    logic_error_copy_ctor(this, rhs);
    this->e.vtable = &length_error_vtable;
    return this;
}

/* ??0length_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0length_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
#ifndef _MSVCIRT
DEFINE_THISCALL_WRAPPER(MSVCP_length_error_ctor_bstr, 8)
length_error* __thiscall MSVCP_length_error_ctor_bstr(length_error *this, const basic_string_char *str)
{
    const char *name = MSVCP_basic_string_char_c_str(str);
    TRACE("(%p %p %s)\n", this, str, name);
    return MSVCP_length_error_ctor(this, EXCEPTION_NAME(name));
}
#endif

/* ??4length_error@std@@QAEAAV01@ABV01@@Z */
/* ??4length_error@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_length_error_assign, 8)
length_error* __thiscall MSVCP_length_error_assign(length_error *this, const length_error *assign)
{
    MSVCP_logic_error_dtor(this);
    return length_error_copy_ctor(this, assign);
}

DEFINE_RTTI_DATA2(length_error, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVlength_error@std@@")
DEFINE_CXX_DATA2(length_error, &logic_error_cxx_type_info, &exception_cxx_type_info, MSVCP_logic_error_dtor)

/* out_of_range class data */
typedef logic_error out_of_range;

static out_of_range* MSVCP_out_of_range_ctor( out_of_range *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &out_of_range_vtable;
    return this;
}

/* ??0out_of_range@std@@QAE@ABV01@@Z */
/* ??0out_of_range@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(out_of_range_copy_ctor, 8)
out_of_range* __thiscall out_of_range_copy_ctor(
        out_of_range *this, const out_of_range *rhs)
{
    TRACE("%p %p\n", this, rhs);
    logic_error_copy_ctor(this, rhs);
    this->e.vtable = &out_of_range_vtable;
    return this;
}

/* ??0out_of_range@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0out_of_range@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
#ifndef _MSVCIRT
DEFINE_THISCALL_WRAPPER(MSVCP_out_of_range_ctor_bstr, 8)
out_of_range* __thiscall MSVCP_out_of_range_ctor_bstr(out_of_range *this, const basic_string_char *str)
{
    const char *name = MSVCP_basic_string_char_c_str(str);
    TRACE("(%p %p %s)\n", this, str, name);
    return MSVCP_out_of_range_ctor(this, EXCEPTION_NAME(name));
}
#endif

/* ??4out_of_range@std@@QAEAAV01@ABV01@@Z */
/* ??4out_of_range@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_out_of_range_assign, 8)
out_of_range* __thiscall MSVCP_out_of_range_assign(out_of_range *this, const out_of_range *assign)
{
    MSVCP_logic_error_dtor(this);
    return out_of_range_copy_ctor(this, assign);
}

DEFINE_RTTI_DATA2(out_of_range, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVout_of_range@std@@")
DEFINE_CXX_DATA2(out_of_range, &logic_error_cxx_type_info, &exception_cxx_type_info, MSVCP_logic_error_dtor)

/* invalid_argument class data */
typedef logic_error invalid_argument;

static invalid_argument* MSVCP_invalid_argument_ctor( invalid_argument *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_logic_error_ctor(this, name);
    this->e.vtable = &invalid_argument_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(invalid_argument_copy_ctor, 8)
invalid_argument* __thiscall invalid_argument_copy_ctor(
        invalid_argument *this, invalid_argument *rhs)
{
    TRACE("%p %p\n", this, rhs);
    logic_error_copy_ctor(this, rhs);
    this->e.vtable = &invalid_argument_vtable;
    return this;
}

DEFINE_RTTI_DATA2(invalid_argument, 0, &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVinvalid_argument@std@@")
DEFINE_CXX_DATA2(invalid_argument, &logic_error_cxx_type_info,  &exception_cxx_type_info, MSVCP_logic_error_dtor)

/* runtime_error class data */
typedef struct {
    exception e;
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    basic_string_char str;
#endif
} runtime_error;

static runtime_error* MSVCP_runtime_error_ctor( runtime_error *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
#if _MSVCP_VER == 60
    MSVCP_exception_ctor(&this->e, "");
#else
    MSVCP_exception_default_ctor(&this->e);
#endif
    MSVCP_basic_string_char_ctor_cstr(&this->str, EXCEPTION_STR(name));
#else
    MSVCP_exception_ctor(&this->e, name);
#endif
    this->e.vtable = &runtime_error_vtable;
    return this;
}

/* ??0runtime_error@std@@QAE@ABV01@@Z */
/* ??0runtime_error@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(runtime_error_copy_ctor, 8)
runtime_error* __thiscall runtime_error_copy_ctor(
        runtime_error *this, const runtime_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    exception_copy_ctor(&this->e, &rhs->e);
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    MSVCP_basic_string_char_copy_ctor(&this->str, &rhs->str);
#endif
    this->e.vtable = &runtime_error_vtable;
    return this;
}

/* ??0runtime_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0runtime_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
#ifndef _MSVCIRT
DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_ctor_bstr, 8)
runtime_error* __thiscall MSVCP_runtime_error_ctor_bstr(runtime_error *this, const basic_string_char *str)
{
    const char *name = MSVCP_basic_string_char_c_str(str);
    TRACE("(%p %p %s)\n", this, str, name);
    return MSVCP_runtime_error_ctor(this, EXCEPTION_NAME(name));
}
#endif

/* ??1runtime_error@std@@UAE@XZ */
/* ??1runtime_error@std@@UEAA@XZ */
/* ??1range_error@std@@UAE@XZ */
/* ??1range_error@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_dtor, 4)
void __thiscall MSVCP_runtime_error_dtor(runtime_error *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(&this->e);
#if _MSVCP_VER <= 90 && !defined _MSVCIRT
    MSVCP_basic_string_char_dtor(&this->str);
#endif
}

DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_vector_dtor, 8)
void* __thiscall MSVCP_runtime_error_vector_dtor(
        runtime_error *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_runtime_error_dtor(this+i);
        operator_delete(ptr);
    } else {
        MSVCP_runtime_error_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??4runtime_error@std@@QAEAAV01@ABV01@@Z */
/* ??4runtime_error@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_assign, 8)
runtime_error* __thiscall MSVCP_runtime_error_assign(runtime_error *this, const runtime_error *assign)
{
    MSVCP_runtime_error_dtor(this);
    return runtime_error_copy_ctor(this, assign);
}

/* ?what@runtime_error@std@@UBEPBDXZ */
/* ?what@runtime_error@std@@UEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_runtime_error_what, 4)
const char* __thiscall MSVCP_runtime_error_what(runtime_error *this)
{
    TRACE("%p\n", this);
#if _MSVCP_VER > 90 || defined _MSVCIRT
    return MSVCP_exception_what( &this->e );
#else
    return MSVCP_basic_string_char_c_str(&this->str);
#endif
}

DEFINE_RTTI_DATA1(runtime_error, 0, &exception_rtti_base_descriptor, ".?AVruntime_error@std@@")
DEFINE_CXX_DATA1(runtime_error, &exception_cxx_type_info, MSVCP_runtime_error_dtor)

/* failure class data */
typedef struct {
    runtime_error base;
#if _MSVCP_VER > 90
    error_code code;
#endif
} system_error;
typedef system_error _System_error;
typedef system_error failure;

static failure* MSVCP_failure_ctor( failure *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_runtime_error_ctor(&this->base, name);
#if _MSVCP_VER > 90
    this->code.code = 1;
    this->code.category = std_iostream_category();
#endif
    this->base.e.vtable = &failure_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(failure_copy_ctor, 8)
failure* __thiscall failure_copy_ctor(
        failure *this, failure *rhs)
{
    TRACE("%p %p\n", this, rhs);
    runtime_error_copy_ctor(&this->base, &rhs->base);
#if _MSVCP_VER > 90
    this->code = rhs->code;
#endif
    this->base.e.vtable = &failure_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_dtor, 4)
void __thiscall MSVCP_failure_dtor(failure *this)
{
    TRACE("%p\n", this);
    MSVCP_runtime_error_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_vector_dtor, 8)
void* __thiscall MSVCP_failure_vector_dtor(
        failure *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    return MSVCP_runtime_error_vector_dtor(&this->base, flags);
}

DEFINE_THISCALL_WRAPPER(MSVCP_failure_what, 4)
const char* __thiscall MSVCP_failure_what(failure *this)
{
    TRACE("%p\n", this);
    return MSVCP_runtime_error_what(&this->base);
}

#if _MSVCP_VER > 90
DEFINE_THISCALL_WRAPPER(system_error_copy_ctor, 8)
system_error* __thiscall system_error_copy_ctor(
        system_error *this, system_error *rhs)
{
    failure_copy_ctor(this, rhs);
    this->base.e.vtable = &system_error_vtable;
    return this;
}
#endif

#if _MSVCP_VER > 110
DEFINE_THISCALL_WRAPPER(_System_error_copy_ctor, 8)
_System_error* __thiscall _System_error_copy_ctor(
        _System_error *this, _System_error *rhs)
{
    failure_copy_ctor(this, rhs);
    this->base.e.vtable = &_System_error_vtable;
    return this;
}
#endif

#if _MSVCP_VER > 110
DEFINE_RTTI_DATA2(_System_error, 0, &runtime_error_rtti_base_descriptor,
        &exception_rtti_base_descriptor, ".?AV_System_error@std@@")
DEFINE_RTTI_DATA3(system_error, 0, &_System_error_rtti_base_descriptor,
        &runtime_error_rtti_base_descriptor, &exception_rtti_base_descriptor,
        ".?AVsystem_error@std@@")
DEFINE_RTTI_DATA4(failure, 0, &system_error_rtti_base_descriptor,
        &_System_error_rtti_base_descriptor, &runtime_error_rtti_base_descriptor,
        &exception_rtti_base_descriptor, ".?AVfailure@ios_base@std@@")
DEFINE_CXX_TYPE_INFO(_System_error)
DEFINE_CXX_DATA3(system_error, &_System_error_cxx_type_info,
        &runtime_error_cxx_type_info, &exception_cxx_type_info,
        MSVCP_runtime_error_dtor)
DEFINE_CXX_DATA4(failure, &system_error_cxx_type_info,
        &_System_error_cxx_type_info, &runtime_error_cxx_type_info,
        &exception_cxx_type_info, MSVCP_runtime_error_dtor)
#elif _MSVCP_VER > 90
DEFINE_RTTI_DATA2(system_error, 0, &runtime_error_rtti_base_descriptor,
        &exception_rtti_base_descriptor, ".?AVsystem_error@std@@")
DEFINE_RTTI_DATA3(failure, 0, &system_error_rtti_base_descriptor,
        &runtime_error_rtti_base_descriptor, &exception_rtti_base_descriptor,
        ".?AVfailure@ios_base@std@@")
#if _MSVCP_VER == 100
DEFINE_CXX_TYPE_INFO(system_error);
#else
DEFINE_CXX_DATA2(system_error, &runtime_error_cxx_type_info,
        &exception_cxx_type_info, MSVCP_runtime_error_dtor)
#endif
DEFINE_CXX_DATA3(failure, &system_error_cxx_type_info, &runtime_error_cxx_type_info,
        &exception_cxx_type_info, MSVCP_runtime_error_dtor)
#else
DEFINE_RTTI_DATA2(failure, 0, &runtime_error_rtti_base_descriptor,
        &exception_rtti_base_descriptor, ".?AVfailure@ios_base@std@@")
DEFINE_CXX_DATA2(failure, &runtime_error_cxx_type_info,
        &exception_cxx_type_info, MSVCP_runtime_error_dtor)
#endif

/* bad_cast class data */
typedef exception bad_cast;

/* ??0bad_cast@std@@QAE@PBD@Z */
/* ??0bad_cast@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_cast_ctor, 8)
bad_cast* __thiscall MSVCP_bad_cast_ctor(bad_cast *this, const char *name)
{
    TRACE("%p %s\n", this, name);
    MSVCP_exception_ctor(this, EXCEPTION_NAME(name));
    this->vtable = &bad_cast_vtable;
    return this;
}

/* ??_Fbad_cast@@QAEXXZ */
/* ??_Fbad_cast@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_cast_default_ctor,4)
bad_cast* __thiscall MSVCP_bad_cast_default_ctor(bad_cast *this)
{
    return MSVCP_bad_cast_ctor(this, "bad cast");
}

/* ??0bad_cast@std@@QAE@ABV01@@Z */
/* ??0bad_cast@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(bad_cast_copy_ctor, 8)
bad_cast* __thiscall bad_cast_copy_ctor(bad_cast *this, const bad_cast *rhs)
{
    TRACE("%p %p\n", this, rhs);
    exception_copy_ctor(this, rhs);
    this->vtable = &bad_cast_vtable;
    return this;
}

/* ??1bad_cast@@UAE@XZ */
/* ??1bad_cast@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_cast_dtor, 4)
void __thiscall MSVCP_bad_cast_dtor(bad_cast *this)
{
    TRACE("%p\n", this);
    MSVCP_exception_dtor(this);
}

DEFINE_THISCALL_WRAPPER(MSVCP_bad_cast_vector_dtor, 8)
void * __thiscall MSVCP_bad_cast_vector_dtor(bad_cast *this, unsigned int flags)
{
    TRACE("%p %x\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            MSVCP_bad_cast_dtor(this+i);
        operator_delete(ptr);
    } else {
        MSVCP_bad_cast_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??4bad_cast@std@@QAEAAV01@ABV01@@Z */
/* ??4bad_cast@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_bad_cast_opequals, 8)
bad_cast* __thiscall MSVCP_bad_cast_opequals(bad_cast *this, const bad_cast *rhs)
{
    TRACE("(%p %p)\n", this, rhs);

    if(this != rhs) {
        MSVCP_exception_dtor(this);
        exception_copy_ctor(this, rhs);
    }
    return this;
}

DEFINE_RTTI_DATA1(bad_cast, 0, &exception_rtti_base_descriptor, ".?AVbad_cast@std@@")

/* range_error class data */
typedef runtime_error range_error;

static range_error* MSVCP_range_error_ctor( range_error *this, exception_name name )
{
    TRACE("%p %s\n", this, EXCEPTION_STR(name));
    MSVCP_runtime_error_ctor(this, name);
    this->e.vtable = &range_error_vtable;
    return this;
}

/* ??0range_error@std@@QAE@ABV01@@Z */
/* ??0range_error@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(range_error_copy_ctor, 8)
range_error* __thiscall range_error_copy_ctor(
        range_error *this, const range_error *rhs)
{
    TRACE("%p %p\n", this, rhs);
    runtime_error_copy_ctor(this, rhs);
    this->e.vtable = &range_error_vtable;
    return this;
}

/* ??0range_error@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0range_error@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
#ifndef _MSVCIRT
DEFINE_THISCALL_WRAPPER(MSVCP_range_error_ctor_bstr, 8)
range_error* __thiscall MSVCP_range_error_ctor_bstr(range_error *this, const basic_string_char *str)
{
    const char *name = MSVCP_basic_string_char_c_str(str);
    TRACE("(%p %p %s)\n", this, str, name);
    return MSVCP_range_error_ctor(this, EXCEPTION_NAME(name));
}
#endif

/* ??4range_error@std@@QAEAAV01@ABV01@@Z */
/* ??4range_error@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_range_error_assign, 8)
range_error* __thiscall MSVCP_range_error_assign(range_error *this, const range_error *assign)
{
    MSVCP_runtime_error_dtor(this);
    return range_error_copy_ctor(this, assign);
}

DEFINE_RTTI_DATA2(range_error, 0, &runtime_error_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AVrange_error@std@@")
DEFINE_CXX_DATA2(range_error, &runtime_error_cxx_type_info, &exception_cxx_type_info, MSVCP_runtime_error_dtor)

#if _MSVCP_VER > 90
/* bad_function_call class data */
typedef exception bad_function_call;

static bad_function_call* MSVCP_bad_function_call_ctor(bad_function_call *this)
{
    static const char *name = "bad function call";

    TRACE("%p\n", this);
    MSVCP_exception_ctor(this, EXCEPTION_NAME(name));
    this->vtable = &bad_function_call_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(bad_function_call_copy_ctor, 8)
bad_function_call* __thiscall bad_function_call_copy_ctor(bad_function_call *this, const bad_function_call *rhs)
{
    TRACE("%p %p\n", this, rhs);
    exception_copy_ctor(this, rhs);
    this->vtable = &bad_function_call_vtable;
    return this;
}

DEFINE_RTTI_DATA1(bad_function_call, 0, &exception_rtti_base_descriptor, ".?AVbad_function_call@std@@")
DEFINE_CXX_DATA1(bad_function_call, &exception_cxx_type_info, MSVCP_exception_dtor)
#endif

/* ?_Nomemory@std@@YAXXZ */
void __cdecl DECLSPEC_NORETURN _Nomemory(void)
{
    bad_alloc e;

    TRACE("()\n");

    MSVCP_bad_alloc_default_ctor(&e);
    _CxxThrowException(&e, &bad_alloc_exception_type);
}

/* ?_Xmem@tr1@std@@YAXXZ */
void __cdecl DECLSPEC_NORETURN _Xmem(void)
{
    bad_alloc e;

    TRACE("()\n");

    MSVCP_bad_alloc_default_ctor(&e);
    _CxxThrowException(&e, &bad_alloc_exception_type);
}

/* ?_Xinvalid_argument@std@@YAXPBD@Z */
/* ?_Xinvalid_argument@std@@YAXPEBD@Z */
void __cdecl DECLSPEC_NORETURN _Xinvalid_argument(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    invalid_argument e;

    TRACE("(%s)\n", debugstr_a(str));

    MSVCP_invalid_argument_ctor(&e, name);
    _CxxThrowException(&e, &invalid_argument_exception_type);
}

/* ?_Xlength_error@std@@YAXPBD@Z */
/* ?_Xlength_error@std@@YAXPEBD@Z */
void __cdecl DECLSPEC_NORETURN _Xlength_error(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    length_error e;

    TRACE("(%s)\n", debugstr_a(str));

    MSVCP_length_error_ctor(&e, name);
    _CxxThrowException(&e, &length_error_exception_type);
}

/* ?_Xout_of_range@std@@YAXPBD@Z */
/* ?_Xout_of_range@std@@YAXPEBD@Z */
void __cdecl DECLSPEC_NORETURN _Xout_of_range(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    out_of_range e;

    TRACE("(%s)\n", debugstr_a(str));

    MSVCP_out_of_range_ctor(&e, name);
    _CxxThrowException(&e, &out_of_range_exception_type);
}

/* ?_Xruntime_error@std@@YAXPBD@Z */
/* ?_Xruntime_error@std@@YAXPEBD@Z */
void __cdecl DECLSPEC_NORETURN _Xruntime_error(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    runtime_error e;

    TRACE("(%s)\n", debugstr_a(str));

    MSVCP_runtime_error_ctor(&e, name);
    _CxxThrowException(&e, &runtime_error_exception_type);
}

#if _MSVCP_VER > 90
/* ?_Xbad_function_call@std@@YAXXZ() */
void __cdecl _Xbad_function_call(void)
{
    exception e;

    TRACE("()\n");

    MSVCP_bad_function_call_ctor(&e);
    _CxxThrowException(&e, &bad_function_call_exception_type);
}
#endif

/* ?uncaught_exception@std@@YA_NXZ */
bool __cdecl MSVCP__uncaught_exception(void)
{
    return __uncaught_exception();
}

#if _MSVCP_VER >= 140
/* ?_XGetLastError@std@@YAXXZ */
void __cdecl _XGetLastError(void)
{
    int err = GetLastError();
    system_error se;
    const char *msg;

    TRACE("() GetLastError()=%d\n", err);

    msg = _Winerror_map_str(err);
    MSVCP_runtime_error_ctor(&se.base, &msg);
    se.code.code = err;
    se.code.category = std_system_category();
    se.base.e.vtable = &system_error_vtable;

    _CxxThrowException(&se, &system_error_exception_type);
}
#endif

#if _MSVCP_VER >= 110
typedef struct
{
    logic_error base;
    error_code code;
} future_error;

extern const vtable_ptr future_error_vtable;

DEFINE_THISCALL_WRAPPER(future_error_copy_ctor, 8)
future_error* __thiscall future_error_copy_ctor(future_error *this, const future_error *rhs)
{
    logic_error_copy_ctor(&this->base, &rhs->base);
    this->code = rhs->code;
    this->base.e.vtable = &future_error_vtable;
    return this;
}

DEFINE_THISCALL_WRAPPER(MSVCP_future_error_what, 4)
const char* __thiscall MSVCP_future_error_what(future_error *this)
{
    const char *names[4] = {
        "broken promise",
        "future already retrieved",
        "promise already satisfied",
        "no state",
    };
#if _MSVCP_VER == 110
    int code = this->code.code;
#else
    int code = this->code.code-1;
#endif
    TRACE("%p\n", this);
    return code >= 0 && code < ARRAY_SIZE(names) ? names[code] : NULL;
}

DEFINE_RTTI_DATA3(future_error, 0, &future_error_rtti_base_descriptor,
        &logic_error_rtti_base_descriptor, &exception_rtti_base_descriptor,
        ".?AVfuture_error@std@@")
DEFINE_CXX_DATA3(future_error, &logic_error_cxx_type_info, &logic_error_cxx_type_info,
        &exception_cxx_type_info, MSVCP_logic_error_dtor)

/* ?_Throw_future_error@std@@YAXABVerror_code@1@@Z */
/* ?_Throw_future_error@std@@YAXAEBVerror_code@1@@Z */
void __cdecl DECLSPEC_NORETURN _Throw_future_error( const error_code *error_code )
{
    future_error e;
    const char *name = "";

    TRACE("(%p)\n", error_code);

    MSVCP_logic_error_ctor(&e.base, EXCEPTION_NAME(name));
    e.code = *error_code;
    e.base.e.vtable = &future_error_vtable;
    _CxxThrowException(&e, &future_error_exception_type);
}

typedef struct
{
    EXCEPTION_RECORD *rec;
    LONG *ref; /* not binary compatible with native */
} exception_ptr;

static void exception_ptr_rethrow(const exception_ptr *ep)
{
    TRACE("(%p)\n", ep);

    if (!ep->rec)
    {
        static const char *exception_msg = "bad exception";
        exception e;

        MSVCP_exception_ctor(&e, &exception_msg);
        _CxxThrowException(&e, &exception_exception_type);
        return;
    }

    RaiseException(ep->rec->ExceptionCode, ep->rec->ExceptionFlags & ~EXCEPTION_UNWINDING,
            ep->rec->NumberParameters, ep->rec->ExceptionInformation);
}

/* ?_Rethrow_future_exception@std@@YAXVexception_ptr@1@@Z */
void __cdecl _Rethrow_future_exception(const exception_ptr ep)
{
    exception_ptr_rethrow(&ep);
}

/* ?_Throw_C_error@std@@YAXH@Z */
void __cdecl _Throw_C_error(int code)
{
    system_error se;
    const char *msg;
    errno_t err;

    TRACE("(%d)\n", code);

    switch(code)
    {
    case 1:
    case 2:
        err = EAGAIN;
        break;
    case 3:
        err = EBUSY;
        break;
    case 4:
        err = EINVAL;
        break;
    default:
#if _MSVCP_VER >= 140
        abort();
#else
        return;
#endif
    }

    msg = strerror(err);
    MSVCP_runtime_error_ctor(&se.base, &msg);
    se.code.code = err;
    se.code.category = std_generic_category();
    se.base.e.vtable = &system_error_vtable;

    _CxxThrowException(&se, &system_error_exception_type);
}
#endif

#if _MSVCP_VER >= 140
void** CDECL __current_exception(void);

/* compute the this pointer for a base class of a given type */
static inline void *get_this_pointer( const this_ptr_offsets *off, void *object )
{
    if (!object) return NULL;

    if (off->vbase_descr >= 0)
    {
        int *offset_ptr;

        /* move this ptr to vbase descriptor */
        object = (char *)object + off->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)object + off->vbase_offset);
        object = (char *)object + *offset_ptr;
    }

    object = (char *)object + off->this_offset;
    return object;
}

#ifdef __ASM_USE_THISCALL_WRAPPER
extern void call_copy_ctor( void *func, void *this, void *src, int has_vbase );
__ASM_GLOBAL_FUNC( call_copy_ctor,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp, %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl $1\n\t"
                   "movl 12(%ebp), %ecx\n\t"
                   "pushl 16(%ebp)\n\t"
                   "call *8(%ebp)\n\t"
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
extern void call_dtor( void *func, void *this );
__ASM_GLOBAL_FUNC( call_dtor,
                   "movl 8(%esp),%ecx\n\t"
                   "call *4(%esp)\n\t"
                   "ret" )
#else
static inline void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
    TRACE( "calling copy ctor %p object %p src %p\n", func, this, src );
    if (has_vbase)
        ((void (__thiscall*)(void*, void*, BOOL))func)(this, src, 1);
    else
        ((void (__thiscall*)(void*, void*))func)(this, src);
}
static inline void call_dtor( void *func, void *this )
{
    ((void (__thiscall*)(void*))func)( this );
}
#endif

int __cdecl __uncaught_exceptions(void)
{
    return *__processing_throw();
}

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
            uintptr_t base = rtti_rva_base( type );

            if (type && type->destructor) call_dtor( rtti_rva(type->destructor, base), obj );
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

/*********************************************************************
 * ?__ExceptionPtrRethrow@@YAXPBX@Z
 * ?__ExceptionPtrRethrow@@YAXPEBX@Z
 */
void __cdecl __ExceptionPtrRethrow(const exception_ptr *ep)
{
    exception_ptr_rethrow(ep);
}

/*********************************************************************
 * ?__ExceptionPtrCurrentException@@YAXPAX@Z
 * ?__ExceptionPtrCurrentException@@YAXPEAX@Z
 */
void __cdecl __ExceptionPtrCurrentException(exception_ptr *ep)
{
    void **current_exception = __current_exception();
    EXCEPTION_RECORD *rec = current_exception ? *current_exception : NULL;

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
        void *obj = (void*)ep->rec->ExceptionInformation[1];
        const cxx_exception_type *et = (void*)ep->rec->ExceptionInformation[2];
        uintptr_t base = rtti_rva_base( et );
        const cxx_type_info_table *table = rtti_rva( et->type_info_table, base );
        const cxx_type_info *ti = rtti_rva( table->info[0], base );
        void **data = HeapAlloc(GetProcessHeap(), 0, ti->size);

        if (ti->flags & CLASS_IS_SIMPLE_TYPE)
        {
            memcpy(data, obj, ti->size);
            if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
        }
        else if (ti->copy_ctor)
        {
            call_copy_ctor(rtti_rva(ti->copy_ctor, base), data, get_this_pointer(&ti->offsets, obj),
                    ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
        }
        else
            memcpy(data, get_this_pointer(&ti->offsets, obj), ti->size);
        ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
    }
    return;
}

/*********************************************************************
 * ?__ExceptionPtrToBool@@YA_NPBX@Z
 * ?__ExceptionPtrToBool@@YA_NPEBX@Z
 */
bool __cdecl __ExceptionPtrToBool(exception_ptr *ep)
{
    return !!ep->rec;
}

/*********************************************************************
 * ?__ExceptionPtrCopyException@@YAXPAXPBX1@Z
 * ?__ExceptionPtrCopyException@@YAXPEAXPEBX1@Z
 */
void __cdecl __ExceptionPtrCopyException(exception_ptr *ep,
        exception *object, const cxx_exception_type *type)
{
    const cxx_type_info_table *table;
    const cxx_type_info *ti;
    void **data;
    uintptr_t base = rtti_rva_base( type );

    __ExceptionPtrDestroy(ep);

    ep->rec = HeapAlloc(GetProcessHeap(), 0, sizeof(EXCEPTION_RECORD));
    ep->ref = HeapAlloc(GetProcessHeap(), 0, sizeof(int));
    *ep->ref = 1;

    memset(ep->rec, 0, sizeof(EXCEPTION_RECORD));
    ep->rec->ExceptionCode = CXX_EXCEPTION;
    ep->rec->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    ep->rec->NumberParameters = CXX_EXCEPTION_PARAMS;
    ep->rec->ExceptionInformation[0] = CXX_FRAME_MAGIC_VC6;
    ep->rec->ExceptionInformation[2] = (ULONG_PTR)type;
    if (CXX_EXCEPTION_PARAMS == 4) ep->rec->ExceptionInformation[3] = base;

    table = rtti_rva( type->type_info_table, base );
    ti = rtti_rva( table->info[0], base );
    data = HeapAlloc(GetProcessHeap(), 0, ti->size);
    if (ti->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memcpy(data, object, ti->size);
        if (ti->size == sizeof(void *)) *data = get_this_pointer(&ti->offsets, *data);
    }
    else if (ti->copy_ctor)
    {
        call_copy_ctor( rtti_rva(ti->copy_ctor, base), data, get_this_pointer(&ti->offsets, object),
                ti->flags & CLASS_HAS_VIRTUAL_BASE_CLASS);
    }
    else
        memcpy(data, get_this_pointer(&ti->offsets, object), ti->size);
    ep->rec->ExceptionInformation[1] = (ULONG_PTR)data;
}

/*********************************************************************
 * ?__ExceptionPtrCompare@@YA_NPBX0@Z
 * ?__ExceptionPtrCompare@@YA_NPEBX0@Z
 */
bool __cdecl __ExceptionPtrCompare(const exception_ptr *ep1, const exception_ptr *ep2)
{
    return ep1->rec == ep2->rec;
}
#endif

#if _MSVCP_VER >= 70 || defined(_MSVCIRT)
#define EXCEPTION_VTABLE(name,funcs) __ASM_VTABLE(name,funcs)
#else
#define EXCEPTION_VTABLE(name,funcs) __ASM_VTABLE(name,funcs VTABLE_ADD_FUNC(MSVCP_exception__Doraise))
#endif

__ASM_BLOCK_BEGIN(exception_vtables)
    EXCEPTION_VTABLE(exception,
            VTABLE_ADD_FUNC(MSVCP_exception_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_exception_what));
    EXCEPTION_VTABLE(bad_alloc,
            VTABLE_ADD_FUNC(MSVCP_bad_alloc_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_exception_what));
    EXCEPTION_VTABLE(logic_error,
            VTABLE_ADD_FUNC(MSVCP_logic_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    EXCEPTION_VTABLE(length_error,
            VTABLE_ADD_FUNC(MSVCP_logic_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    EXCEPTION_VTABLE(out_of_range,
            VTABLE_ADD_FUNC(MSVCP_logic_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    EXCEPTION_VTABLE(invalid_argument,
            VTABLE_ADD_FUNC(MSVCP_logic_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_logic_error_what));
    EXCEPTION_VTABLE(runtime_error,
            VTABLE_ADD_FUNC(MSVCP_runtime_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_runtime_error_what));
#if _MSVCP_VER >= 110
    EXCEPTION_VTABLE(future_error,
            VTABLE_ADD_FUNC(MSVCP_logic_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_future_error_what));
#endif
#if _MSVCP_VER > 110
    EXCEPTION_VTABLE(_System_error,
            VTABLE_ADD_FUNC(MSVCP_failure_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_failure_what));
#endif
#if _MSVCP_VER > 90
    EXCEPTION_VTABLE(system_error,
            VTABLE_ADD_FUNC(MSVCP_failure_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_failure_what));
    EXCEPTION_VTABLE(bad_function_call,
            VTABLE_ADD_FUNC(MSVCP_exception_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_exception_what));
#endif
    EXCEPTION_VTABLE(failure,
            VTABLE_ADD_FUNC(MSVCP_failure_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_failure_what));
    EXCEPTION_VTABLE(bad_cast,
            VTABLE_ADD_FUNC(MSVCP_bad_cast_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_exception_what));
    EXCEPTION_VTABLE(range_error,
            VTABLE_ADD_FUNC(MSVCP_runtime_error_vector_dtor)
            VTABLE_ADD_FUNC(MSVCP_runtime_error_what));

__ASM_BLOCK_END

/* Internal: throws exception */
void DECLSPEC_NORETURN throw_exception(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    exception e;

    MSVCP_exception_ctor(&e, name);
    _CxxThrowException(&e, &exception_exception_type);
}

/* Internal: throws range_error exception */
void DECLSPEC_NORETURN throw_range_error(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    range_error e;

    MSVCP_range_error_ctor(&e, name);
    _CxxThrowException(&e, &range_error_exception_type);
}

/* Internal: throws failure exception */
void DECLSPEC_NORETURN throw_failure(const char *str)
{
    exception_name name = EXCEPTION_NAME(str);
    failure e;

    MSVCP_failure_ctor(&e, name);
    _CxxThrowException(&e, &failure_exception_type);
}

void init_exception(void *base)
{
#ifdef RTTI_USE_RVA
    init_type_info_rtti(base);
    init_exception_rtti(base);
    init_bad_alloc_rtti(base);
    init_logic_error_rtti(base);
    init_length_error_rtti(base);
    init_out_of_range_rtti(base);
    init_invalid_argument_rtti(base);
    init_runtime_error_rtti(base);
#if _MSVCP_VER >= 110
    init_future_error_rtti(base);
#endif
#if _MSVCP_VER > 110
    init__System_error_rtti(base);
#endif
#if _MSVCP_VER > 90
    init_system_error_rtti(base);
    init_bad_function_call_rtti(base);
#endif
    init_failure_rtti(base);
    init_bad_cast_rtti(base);
    init_range_error_rtti(base);

    init_exception_cxx(base);
    init_bad_alloc_cxx(base);
    init_logic_error_cxx_type_info(base);
    init_length_error_cxx(base);
    init_out_of_range_cxx(base);
    init_invalid_argument_cxx(base);
    init_runtime_error_cxx(base);
#if _MSVCP_VER >= 110
    init_future_error_cxx(base);
#endif
#if _MSVCP_VER > 110
    init__System_error_cxx_type_info(base);
#endif
#if _MSVCP_VER == 100
    init_system_error_cxx_type_info(base);
#elif _MSVCP_VER > 100
    init_system_error_cxx(base);
#endif
#if _MSVCP_VER > 90
    init_bad_function_call_cxx(base);
#endif
    init_failure_cxx(base);
    init_range_error_cxx(base);
#endif
}
