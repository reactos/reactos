/*
 * PROJECT:         ReactOS c++ runtime library
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Exception-handling vector ctor/dtor iterator implementation
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

#include <stddef.h>

void __stdcall MSVCRTEX_eh_vector_constructor_iterator(void *pMem, size_t sizeOfItem, int nItems, void (__thiscall *ctor)(void *), void (__thiscall *dtor)(void *))
{
    char *pEnd = static_cast<char *>(pMem) + nItems * sizeOfItem;
    for (char *pItem = static_cast<char *>(pMem);
         pItem < pEnd;
         pItem += sizeOfItem)
    {
        try
        {
            ctor(pItem);
        }
        catch (...)
        {
            for (pItem -= sizeOfItem; pItem >= pMem; pItem -= sizeOfItem)
                dtor(pItem);
            throw;
        }
    }
}

void __stdcall MSVCRTEX_eh_vector_destructor_iterator(void *pMem, size_t sizeOfItem, int nItems, void (__thiscall *dtor)(void *))
{
    char *pEnd = static_cast<char *>(pMem) + nItems * sizeOfItem;
    for (char *pItem = pEnd - sizeOfItem;
         pItem >= pMem;
         pItem -= sizeOfItem)
    {
        try
        {
            dtor(pItem);
        }
        catch (...)
        {
            for (pItem -= sizeOfItem; pItem >= pMem; pItem -= sizeOfItem)
                dtor(pItem);
            throw;
        }
    }
}
