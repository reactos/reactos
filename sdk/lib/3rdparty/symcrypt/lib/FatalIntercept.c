//
// FatalIntercept.C
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// Empty function which our test code can replace to intercept any Fatal calls.
// Used in Kernel-mode tests so that an error doesn't bugcheck the machine.
// Rather, it can kill the current thread and not take down the machine.
//
// This is in its own C file so that it is only linked in when the caller doesn't have
// a function by this name.
//

#include "precomp.h"

VOID
SYMCRYPT_CALL
SymCryptFatalIntercept( UINT32 fatalCode )
{
    UNREFERENCED_PARAMETER( fatalCode );
}
