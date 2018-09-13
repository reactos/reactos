/*++

    Copyright (c) 1995 Intel Corporation
    
Module Name:

    nowarn.h

Abstract:

    This header contains pragmas to turn off compiler warnings that
    may be safely ignored. 
    
--*/


/* nonstandard extension 'single line comment' was used */
#pragma warning(disable: 4001)

// argument differs in indirection to slightly different base types
#pragma warning(disable: 4057)

// unreferenced formal parameter
#pragma warning(disable: 4100)

// named type definition in parentheses
#pragma warning(disable: 4115)

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

// nonstandard extension used : benign typedef redefinition
#pragma warning(disable: 4209)

// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)

// unreferenced inline function has been removed
#pragma warning(disable: 4514)

// Note: Creating precompiled header 
#pragma warning(disable: 4699)
