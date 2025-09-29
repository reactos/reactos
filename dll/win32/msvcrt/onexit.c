/*
 * msvcrt onexit functions
 *
 * Copyright 2016 Nikolay Sivov
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

/* these functions are part of the import lib for compatibility with the Mingw runtime */
#if 0
#pragma makedep implib
#endif

#include <process.h>
#include "msvcrt.h"
#include "mtdll.h"


/*********************************************************************
 *		_initialize_onexit_table (UCRTBASE.@)
 */
int __cdecl _initialize_onexit_table(_onexit_table_t *table)
{
    if (!table)
        return -1;

    if (table->_first == table->_end)
        table->_last = table->_end = table->_first = NULL;
    return 0;
}


/*********************************************************************
 *		_register_onexit_function (UCRTBASE.@)
 */
int __cdecl _register_onexit_function(_onexit_table_t *table, _onexit_t func)
{
    if (!table)
        return -1;

    _lock(_EXIT_LOCK1);
    if (!table->_first)
    {
        table->_first = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 32 * sizeof(void *));
        if (!table->_first)
        {
            _unlock(_EXIT_LOCK1);
            return -1;
        }
        table->_last = table->_first;
        table->_end = table->_first + 32;
    }

    /* grow if full */
    if (table->_last == table->_end)
    {
        int len = table->_end - table->_first;
        _PVFV *tmp = HeapReAlloc(GetProcessHeap(), 0, table->_first, 2 * len * sizeof(void *));
        if (!tmp)
        {
            _unlock(_EXIT_LOCK1);
            return -1;
        }
        table->_first = tmp;
        table->_end = table->_first + 2 * len;
        table->_last = table->_first + len;
    }

    *table->_last = (_PVFV)func;
    table->_last++;
    _unlock(_EXIT_LOCK1);
    return 0;
}


/*********************************************************************
 *		_execute_onexit_table (UCRTBASE.@)
 */
int __cdecl _execute_onexit_table(_onexit_table_t *table)
{
    _PVFV *func;
    _onexit_table_t copy;

    if (!table)
        return -1;

    _lock(_EXIT_LOCK1);
    if (!table->_first || table->_first >= table->_last)
    {
        _unlock(_EXIT_LOCK1);
        return 0;
    }
    copy._first = table->_first;
    copy._last  = table->_last;
    copy._end   = table->_end;
    memset(table, 0, sizeof(*table));
    _initialize_onexit_table(table);
    _unlock(_EXIT_LOCK1);

    for (func = copy._last - 1; func >= copy._first; func--)
    {
        if (*func)
           (*func)();
    }

    HeapFree(GetProcessHeap(), 0, copy._first);
    return 0;
}
