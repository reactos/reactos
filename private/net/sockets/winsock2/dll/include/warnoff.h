/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    warnoff.h

Abstract:

    This header contains pragmas to turn off compiler warnings that
    may be safely ignored.

Author:

    Dirk Brandewie dirk@mink.intel.com  29-Jun-1995

Revision History:

--*/


/*#ifndef _WARNOFF_
  #define _WARNOFF_
  */
/* nonstandard extension 'single line comment' was used */
#pragma warning(disable: 4001)

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)

// Note: Creating precompiled header
#pragma warning(disable: 4699)

// unreferenced inline function has been removed
#pragma warning(disable: 4514)

// unreferenced formal parameter
//#pragma warning(disable: 4100)

// 'type' differs in indirection to slightly different base
// types from 'other type'
#pragma warning(disable: 4057)

// named type definition in parentheses
#pragma warning(disable: 4115)

// nonstandard extension used : benign typedef redefinition
#pragma warning(disable: 4209)

/*
#endif // _WARNOFF_
*/
