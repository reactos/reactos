/*++

Copyright (c) Microsoft Corporation

ModuleName:

    DbgMacros.h

Abstract:

    This file contains debug macros
    to make sure that an object is intialized

    This is useful in mode agnostic primitives
    where initialization is important in user mode
    but not in kernel mode (e.g. for a lock)

Author:



Revision History:



--*/

#pragma once

#if DBG_WDF
#define DECLARE_DBGFLAG_INITIALIZED \
    protected: \
        BOOLEAN m_DbgFlagIsInitialized;

#define ASSERT_DBGFLAG_INITIALIZED \
{ \
    ASSERT(m_DbgFlagIsInitialized == TRUE); \
}

#define SET_DBGFLAG_INITIALIZED \
{ \
    m_DbgFlagIsInitialized = TRUE; \
}

#define CLEAR_DBGFLAG_INITIALIZED \
{ \
    m_DbgFlagIsInitialized = FALSE; \
}

#define ASSERT_DBGFLAG_NOT_INITIALIZED \
{ \
    ASSERT(m_DbgFlagIsInitialized == FALSE); \
}

#else

#define DECLARE_DBGFLAG_INITIALIZED
#define ASSERT_DBGFLAG_INITIALIZED
#define SET_DBGFLAG_INITIALIZED
#define CLEAR_DBGFLAG_INITIALIZED
#define ASSERT_DBGFLAG_NOT_INITIALIZED

#endif
