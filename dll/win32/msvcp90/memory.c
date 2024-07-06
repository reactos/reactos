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

#include <stdarg.h>
#include <limits.h>

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"


/* ?address@?$allocator@D@std@@QBEPADAAD@Z */
/* ?address@?$allocator@D@std@@QEBAPEADAEAD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_address, 8)
char* __thiscall MSVCP_allocator_char_address(void *this, char *ptr)
{
    return ptr;
}

/* ?address@?$allocator@D@std@@QBEPBDABD@Z */
/* ?address@?$allocator@D@std@@QEBAPEBDAEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_const_address, 8)
const char* __thiscall MSVCP_allocator_char_const_address(void *this, const char *ptr)
{
    return ptr;
}

/* ??0?$allocator@D@std@@QAE@XZ */
/* ??0?$allocator@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_ctor, 4)
void* __thiscall MSVCP_allocator_char_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@D@std@@QAE@ABV01@@Z */
/* ??0?$allocator@D@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_copy_ctor, 8)
void* __thiscall MSVCP_allocator_char_copy_ctor(void *this, const void *copy)
{
    return this;
}

/* ??4?$allocator@D@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@D@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_assign, 8)
void* __thiscall MSVCP_allocator_char_assign(void *this, const void *assign)
{
    return this;
}

/* ?deallocate@?$allocator@D@std@@QAEXPADI@Z */
/* ?deallocate@?$allocator@D@std@@QEAAXPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_deallocate, 12)
void __thiscall MSVCP_allocator_char_deallocate(void *this, char *ptr, size_t size)
{
    operator_delete(ptr);
}

/* ?allocate@?$allocator@D@std@@QAEPADI@Z */
/* ?allocate@?$allocator@D@std@@QEAAPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_allocate, 8)
char* __thiscall MSVCP_allocator_char_allocate(void *this, size_t count)
{
    return operator_new(count);
}

/* ?allocate@?$allocator@D@std@@QAEPADIPBX@Z */
/* ?allocate@?$allocator@D@std@@QEAAPEAD_KPEBX@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_allocate_hint, 12)
char* __thiscall MSVCP_allocator_char_allocate_hint(void *this,
        size_t count, const void *hint)
{
    /* Native ignores hint */
    return MSVCP_allocator_char_allocate(this, count);
}

/* ?construct@?$allocator@D@std@@QAEXPADABD@Z */
/* ?construct@?$allocator@D@std@@QEAAXPEADAEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_construct, 12)
void __thiscall MSVCP_allocator_char_construct(void *this, char *ptr, const char *val)
{
    *ptr = *val;
}

/* ?destroy@?$allocator@D@std@@QAEXPAD@Z */
/* ?destroy@?$allocator@D@std@@QEAAXPEAD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_destroy, 8)
void __thiscall MSVCP_allocator_char_destroy(void *this, char *ptr)
{
}

/* ?max_size@?$allocator@D@std@@QBEIXZ */
/* ?max_size@?$allocator@D@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_char_max_size, 4)
size_t __thiscall MSVCP_allocator_char_max_size(const void *this)
{
    return UINT_MAX/sizeof(char);
}


/* allocator<wchar_t> */
/* ?address@?$allocator@_W@std@@QBEPA_WAA_W@Z */
/* ?address@?$allocator@_W@std@@QEBAPEA_WAEA_W@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_address, 8)
wchar_t* __thiscall MSVCP_allocator_wchar_address(void *this, wchar_t *ptr)
{
    return ptr;
}

/* ?address@?$allocator@_W@std@@QBEPB_WAB_W@Z */
/* ?address@?$allocator@_W@std@@QEBAPEB_WAEB_W@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_const_address, 8)
const wchar_t* __thiscall MSVCP_allocator_wchar_const_address(void *this, const wchar_t *ptr)
{
    return ptr;
}

/* ??0?$allocator@_W@std@@QAE@XZ */
/* ??0?$allocator@_W@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_ctor, 4)
void* __thiscall MSVCP_allocator_wchar_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@_W@std@@QAE@ABV01@@Z */
/* ??0?$allocator@_W@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_copy_ctor, 8)
void* __thiscall MSVCP_allocator_wchar_copy_ctor(void *this, void *copy)
{
    return this;
}

/* ??4?$allocator@_W@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@_W@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_assign, 8)
void* __thiscall MSVCP_allocator_wchar_assign(void *this, void *assign)
{
    return this;
}

/* ?deallocate@?$allocator@_W@std@@QAEXPA_WI@Z */
/* ?deallocate@?$allocator@_W@std@@QEAAXPEA_W_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_deallocate, 12)
void __thiscall MSVCP_allocator_wchar_deallocate(void *this,
        wchar_t *ptr, size_t size)
{
    operator_delete(ptr);
}

/* ?allocate@?$allocator@_W@std@@QAEPA_WI@Z */
/* ?allocate@?$allocator@_W@std@@QEAAPEA_W_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_allocate, 8)
wchar_t* __thiscall MSVCP_allocator_wchar_allocate(void *this, size_t count)
{
    if(UINT_MAX/count < sizeof(wchar_t)) _Xmem();
    return operator_new(count * sizeof(wchar_t));
}

/* ?allocate@?$allocator@_W@std@@QAEPA_WIPBX@Z */
/* ?allocate@?$allocator@_W@std@@QEAAPEA_W_KPEBX@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_allocate_hint, 12)
wchar_t* __thiscall MSVCP_allocator_wchar_allocate_hint(void *this,
        size_t count, const void *hint)
{
    return MSVCP_allocator_wchar_allocate(this, count);
}

/* ?construct@?$allocator@_W@std@@QAEXPA_WAB_W@Z */
/* ?construct@?$allocator@_W@std@@QEAAXPEA_WAEB_W@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_construct, 12)
void __thiscall MSVCP_allocator_wchar_construct(void *this,
        wchar_t *ptr, const wchar_t *val)
{
    *ptr = *val;
}

/* ?destroy@?$allocator@_W@std@@QAEXPA_W@Z */
/* ?destroy@?$allocator@_W@std@@QEAAXPEA_W@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_destroy, 8)
void __thiscall MSVCP_allocator_wchar_destroy(void *this, char *ptr)
{
}

/* ?max_size@?$allocator@_W@std@@QBEIXZ */
/* ?max_size@?$allocator@_W@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_wchar_max_size, 4)
size_t __thiscall MSVCP_allocator_wchar_max_size(const void *this)
{
    return UINT_MAX/sizeof(wchar_t);
}

/* allocator<unsigned short> */
/* ?address@?$allocator@G@std@@QBEPAGAAG@Z */
/* ?address@?$allocator@G@std@@QEBAPEAGAEAG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_address, 8)
unsigned short* __thiscall MSVCP_allocator_short_address(
        void *this, unsigned short *ptr)
{
    return ptr;
}

/* ?address@?$allocator@G@std@@QBEPBGABG@Z */
/* ?address@?$allocator@G@std@@QEBAPEBGAEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_const_address, 8)
const unsigned short* __thiscall MSVCP_allocator_short_const_address(
        void *this, const unsigned short *ptr)
{
    return ptr;
}

/* ??0?$allocator@G@std@@QAE@XZ */
/* ??0?$allocator@G@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_ctor, 4)
void* __thiscall MSVCP_allocator_short_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@G@std@@QAE@ABV01@@Z */
/* ??0?$allocator@G@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_copy_ctor, 8)
void* __thiscall MSVCP_allocator_short_copy_ctor(void *this, void *copy)
{
    return this;
}

/* ??4?$allocator@G@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@G@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_assign, 8)
void* __thiscall MSVCP_allocator_short_assign(void *this, void *assign)
{
    return this;
}

/* ?deallocate@?$allocator@G@std@@QAEXPAGI@Z */
/* ?deallocate@?$allocator@G@std@@QEAAXPEAG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_deallocate, 12)
void __thiscall MSVCP_allocator_short_deallocate(void *this,
        unsigned short *ptr, size_t size)
{
    operator_delete(ptr);
}

/* ?allocate@?$allocator@G@std@@QAEPAGI@Z */
/* ?allocate@?$allocator@G@std@@QEAAPEAG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_allocate, 8)
unsigned short* __thiscall MSVCP_allocator_short_allocate(
        void *this, size_t count)
{
    if(UINT_MAX/count < sizeof(unsigned short)) _Xmem();
    return operator_new(count * sizeof(unsigned short));
}

/* ?allocate@?$allocator@G@std@@QAEPAGIPBX@Z */
/* ?allocate@?$allocator@G@std@@QEAAPEAG_KPEBX@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_allocate_hint, 12)
unsigned short* __thiscall MSVCP_allocator_short_allocate_hint(
        void *this, size_t count, const void *hint)
{
    return MSVCP_allocator_short_allocate(this, count);
}

/* ?construct@?$allocator@G@std@@QAEXPAGABG@Z */
/* ?construct@?$allocator@G@std@@QEAAXPEAGAEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_construct, 12)
void __thiscall MSVCP_allocator_short_construct(void *this,
        unsigned short *ptr, unsigned short *val)
{
    *ptr = *val;
}

/* ?destroy@?$allocator@G@std@@QAEXPAG@Z */
/* ?destroy@?$allocator@G@std@@QEAAXPEAG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_destroy, 8)
void __thiscall MSVCP_allocator_short_destroy(void *this, size_t *ptr)
{
}

/* ?max_size@?$allocator@G@std@@QBEIXZ */
/* ?max_size@?$allocator@G@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_short_max_size, 4)
size_t __thiscall MSVCP_allocator_short_max_size(void *this)
{
    return UINT_MAX/sizeof(unsigned short);
}

/* allocator<void> */
/* ??0?$allocator@X@std@@QAE@XZ */
/* ??0?$allocator@X@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_ctor, 4)
void* __thiscall MSVCP_allocator_void_ctor(void *this)
{
    return this;
}

/* ??0?$allocator@X@std@@QAE@ABV01@@Z */
/* ??0?$allocator@X@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_copy_ctor, 8)
void* __thiscall MSVCP_allocator_void_copy_ctor(void *this, void *copy)
{
    return this;
}

/* ??4?$allocator@X@std@@QAEAAV01@ABV01@@Z */
/* ??4?$allocator@X@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_allocator_void_assign, 8)
void* __thiscall MSVCP_allocator_void_assign(void *this, void *assign)
{
    return this;
}
