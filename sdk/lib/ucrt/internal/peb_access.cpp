//
// peb_access.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Wrapper functions to access fields in the PEB.
//

// Using internal headers for definitions. Only call publicly available functions.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

extern "C" bool __cdecl __acrt_app_verifier_enabled()
{
    return (NtCurrentTeb()->ProcessEnvironmentBlock->NtGlobalFlag & FLG_APPLICATION_VERIFIER) != 0;
}

extern "C" bool __cdecl __acrt_is_secure_process()
{
    return (NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->Flags & RTL_USER_PROC_SECURE_PROCESS) != 0;
}
