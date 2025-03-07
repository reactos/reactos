//
// __scrt_uninitialize_crt.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of __scrt_uninitialize_crt.
//
// SPDX-License-Identifier: MIT
//

extern "C"
bool
__cdecl
__scrt_uninitialize_crt(bool is_terminating, bool from_exit)
{
    return true;
}
