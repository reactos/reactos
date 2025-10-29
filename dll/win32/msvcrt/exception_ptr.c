/*
 * std::exception_ptr helper functions
 *
 * Copyright 2022 Torge Matthies for CodeWeavers
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

#include <stdarg.h>
#include <stdbool.h>

#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "cppexcept.h"

/* call a copy constructor */
#ifdef __ASM_USE_THISCALL_WRAPPER
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
__ASM_GLOBAL_FUNC( call_dtor,
                   "movl 8(%esp),%ecx\n\t"
                   "call *4(%esp)\n\t"
                   "ret" )
#endif

#if _MSVCR_VER >= 100

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

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

#ifndef _CONCRT

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

#endif

/*********************************************************************
 * ?__ExceptionPtrRethrow@@YAXPBX@Z
 * ?__ExceptionPtrRethrow@@YAXPEBX@Z
 */
void __cdecl __ExceptionPtrRethrow(const exception_ptr *ep)
{
    TRACE("(%p)\n", ep);

    if (!ep->rec)
    {
        throw_exception("bad exception");
        return;
    }

    RaiseException(ep->rec->ExceptionCode, ep->rec->ExceptionFlags & ~EXCEPTION_UNWINDING,
            ep->rec->NumberParameters, ep->rec->ExceptionInformation);
}

void exception_ptr_from_record(exception_ptr *ep, EXCEPTION_RECORD *rec)
{
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

#ifndef _CONCRT

/*********************************************************************
 * ?__ExceptionPtrCurrentException@@YAXPAX@Z
 * ?__ExceptionPtrCurrentException@@YAXPEAX@Z
 */
void __cdecl __ExceptionPtrCurrentException(exception_ptr *ep)
{
    TRACE("(%p)\n", ep);
    exception_ptr_from_record(ep, msvcrt_get_thread_data()->exc_record);
}

#if _MSVCR_VER >= 110
/*********************************************************************
 * ?__ExceptionPtrToBool@@YA_NPBX@Z
 * ?__ExceptionPtrToBool@@YA_NPEBX@Z
 */
bool __cdecl __ExceptionPtrToBool(exception_ptr *ep)
{
    return !!ep->rec;
}
#endif

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

#endif /* _MSVCR_VER >= 100 */
