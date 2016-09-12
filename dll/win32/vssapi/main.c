/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "vss.h"
#include "vswriter.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( vssapi );

#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define THISCALL_NAME(func) __ASM_NAME("__thiscall_" #func)
#undef __thiscall
#define __thiscall __stdcall
#ifdef _MSC_VER
#define DEFINE_THISCALL_WRAPPER(func,args) \
    void __declspec(naked) __thiscall_ ##func(void) \
    { \
        __asm { pop eax } \
        __asm { push ecx } \
        __asm { push eax } \
        __asm { jmp func } \
    }
#else
#define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void THISCALL(func)(void); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                      "popl %eax\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )
#endif
#else /* __i386__ */

#define THISCALL(func) func
#define THISCALL_NAME(func) __ASM_NAME(#func)
#define __thiscall __cdecl
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

struct CVssWriter
{
    void **vtable;
};

/******************************************************************
 *  ??0CVssWriter@@QAE@XZ (VSSAPI.@)
 */
struct CVssWriter * __thiscall VSSAPI_CVssWriter_default_ctor( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
    writer->vtable = NULL;
    return writer;
}
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_default_ctor, 4 )

/******************************************************************
 *  ??1CVssWriter@@UAE@XZ (VSSAPI.@)
 */
void __thiscall VSSAPI_CVssWriter_dtor( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
}
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_dtor, 4 )

/******************************************************************
 *  ?Initialize@CVssWriter@@QAGJU_GUID@@PBGW4VSS_USAGE_TYPE@@W4VSS_SOURCE_TYPE@@W4_VSS_APPLICATION_LEVEL@@KW4VSS_ALTERNATE_WRITER_STATE@@_N@Z
 */
HRESULT __thiscall VSSAPI_CVssWriter_Initialize( struct CVssWriter *writer, VSS_ID id,
    LPCWSTR name, VSS_USAGE_TYPE usage_type, VSS_SOURCE_TYPE source_type,
    VSS_APPLICATION_LEVEL level, DWORD timeout, VSS_ALTERNATE_WRITER_STATE alt_writer_state,
    BOOL throttle, LPCWSTR instance )
{
    FIXME( "%p, %s, %s, %u, %u, %u, %u, %u, %d, %s\n", writer, debugstr_guid(&id),
           debugstr_w(name), usage_type, source_type, level, timeout, alt_writer_state,
           throttle, debugstr_w(instance) );
    return S_OK;
}
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Initialize, 52 )

/******************************************************************
 *  ?Subscribe@CVssWriter@@QAGJK@Z
 */
HRESULT __thiscall VSSAPI_CVssWriter_Subscribe( struct CVssWriter *writer, DWORD flags )
{
    FIXME( "%p, %x\n", writer, flags );
    return S_OK;
}
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Subscribe, 8 )

/******************************************************************
 *  ?Unsubscribe@CVssWriter@@QAGJXZ
 */
HRESULT __thiscall VSSAPI_CVssWriter_Unsubscribe( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
    return S_OK;
}
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Unsubscribe, 4 )
