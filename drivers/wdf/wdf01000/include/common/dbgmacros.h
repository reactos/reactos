#ifndef _DBGMACROS_H_
#define _DBGMACROS_H_

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

#endif//_DBGMACROS_H_