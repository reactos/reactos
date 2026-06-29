//
// _onexit.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of _onexit.
//
// SPDX-License-Identifier: MIT
//

#include <stdlib.h>
#include <internal_shared.h>
#include <corecrt_startup.h>
#include <assert.h>

_onexit_table_t module_local_atexit_table;
int module_local_atexit_table_initialized = 0;

int __cdecl __scrt_initialize_onexit(void)
{
    _initialize_onexit_table(&module_local_atexit_table);
    module_local_atexit_table_initialized = 1;
    return 0;
}

// CRT startup initializer
_CRTALLOC(".CRT$XIAA") _PIFV const __scrt_onexit_initializer = __scrt_initialize_onexit;

_onexit_t __cdecl _onexit(_In_opt_ _onexit_t _Func)
{
    assert(module_local_atexit_table_initialized == 1);
    int result = _register_onexit_function(&module_local_atexit_table, _Func);
    return (result == 0) ? _Func : NULL;
}
