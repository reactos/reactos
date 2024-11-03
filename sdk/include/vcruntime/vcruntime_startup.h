
#pragma once

typedef enum _crt_argv_mode
{
    _crt_argv_no_arguments,
    _crt_argv_unexpanded_arguments,
    _crt_argv_expanded_arguments,
} _crt_argv_mode;

typedef enum _crt_exit_cleanup_mode
{
    _crt_exit_full_cleanup,
    _crt_exit_quick_cleanup,
    _crt_exit_no_cleanup
} _crt_exit_cleanup_mode;

typedef enum _crt_exit_return_mode
{
    _crt_exit_terminate_process,
    _crt_exit_return_to_caller
} _crt_exit_return_mode;

__vcrt_bool __cdecl __vcrt_uninitialize(_In_ __vcrt_bool _Terminating);
