//
// txtmode.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the global _fmode variable and sets the global file mode to text.
// This is the default behavior.
//
#include <corecrt_internal.h>
#include <fcntl.h>
#include <stdlib.h>

// Clients of the static CRT can choose to access _fmode directly as a global variable; they do so as if it was declared as an int.
// Because state separation is disabled in the static CRT, the dual_state_global<int> has the same representation as an int, so this is okay, if a bit messy.
__crt_state_management::dual_state_global<int> _fmode;  // This is automatically initialized to zero by the compiler

extern "C" int* __cdecl __p__fmode()
{
    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    return &_fmode.value();
    _END_SECURE_CRT_DEPRECATION_DISABLE
}

// Initializer for fmode global correction.
_CRT_LINKER_FORCE_INCLUDE(__acrt_fmode_initializer);

extern "C" int __cdecl __acrt_initialize_fmode()
{
    // The initial mode for _fmode is initialized during pre_c_initializer() in vcstartup, so that it can be overridden with a linkopt.
    // The default value is provided by _get_startup_file_mode(), but this value is not used to initialize both _fmode states.
    // (ex: if exe is Prog-Mode, OS-Mode _fmode is uninitialized, if exe is OS-Mode, Prog-Mode _fmode is uninitialized)

    // To fix this, we adjust zero-value _fmode to _O_TEXT, since that is the default.
    // Only the exe can use the linkopt, so the state that is not being initialized could not have requested _O_BINARY.

    int * const fmode_states = _fmode.dangerous_get_state_array();
    for (unsigned int i = 0; i != __crt_state_management::state_index_count; ++i)
    {
        int& fmode_state = fmode_states[i];
        if (fmode_state == 0)
        {
            fmode_state = _O_TEXT;
        }
    }

    return 0;
}
