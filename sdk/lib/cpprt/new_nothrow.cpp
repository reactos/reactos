/*
 * PROJECT:         ReactOS C++ runtime library
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         nothrow version of the new operators
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

#include <new>

void* operator new (std::size_t) throw(std::bad_alloc);
void* operator new[] (std::size_t) throw(std::bad_alloc);

const std::nothrow_t std::nothrow;

void* operator new (std::size_t size, const std::nothrow_t& nothrow_constant) throw()
{
    try
    {
        return operator new (size);
    }
    catch (std::bad_alloc)
    {
        return NULL;
    }
}

void* operator new[] (std::size_t size, const std::nothrow_t& nothrow_constant) throw()
{
    try
    {
        return operator new[] (size);
    }
    catch (std::bad_alloc)
    {
        return NULL;
    }
}
