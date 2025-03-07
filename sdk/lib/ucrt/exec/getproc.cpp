/***
*getproc.c - Get the address of a procedure in a DLL.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _getdllprocadd() - gets a procedure address by name or
*       ordinal
*
*******************************************************************************/

#include <corecrt_internal.h>

#define _CRT_ENABLE_OBSOLETE_LOADLIBRARY_FUNCTIONS

#include <process.h>

/***
*int (*)() _getdllprocaddr(handle, name, ordinal) - Get the address of a
*       DLL procedure specified by name or ordinal
*
*Purpose:
*
*Entry:
*       int handle - a DLL handle from _loaddll
*       char * name - Name of the procedure, or nullptr to get by ordinal
*       int ordinal - Ordinal of the procedure, or -1 to get by name
*
*
*Exit:
*       returns a pointer to the procedure if found
*       returns nullptr if not found
*
*Exceptions:
*
*******************************************************************************/
typedef int (__cdecl* proc_address_type)();

extern "C" 
DECLSPEC_GUARD_SUPPRESS
proc_address_type __cdecl _getdllprocaddr(
    intptr_t const module_handle_value,
    char*    const procedure_name,
    intptr_t const ordinal
    )
{
    HMODULE const module_handle = reinterpret_cast<HMODULE>(module_handle_value);
    if (procedure_name == nullptr)
    {
        if (ordinal <= 65535)
        {
            char* const ordinal_as_string = reinterpret_cast<char*>(ordinal);
            return reinterpret_cast<proc_address_type>(
                GetProcAddress(module_handle, ordinal_as_string));
        }
    }
    else
    {
        if (ordinal == static_cast<intptr_t>(-1))
        {
            return reinterpret_cast<proc_address_type>(
                GetProcAddress(module_handle, procedure_name));
        }
    }

    return nullptr;
}
