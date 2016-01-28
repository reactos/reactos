
#include <ksarm.h>

    TEXTAREA

    LEAF_ENTRY LdrInitializeThunk
    __assertfail
    bx lr
    LEAF_END LdrInitializeThunk

    LEAF_ENTRY KiRaiseUserExceptionDispatcher
    __assertfail
    bx lr
    LEAF_END KiRaiseUserExceptionDispatcher

    LEAF_ENTRY KiUserApcDispatcher
    __assertfail
    bx lr
    LEAF_END KiUserApcDispatcher

    LEAF_ENTRY KiUserCallbackDispatcher
    __assertfail
    bx lr
    LEAF_END KiUserCallbackDispatcher

    LEAF_ENTRY KiUserExceptionDispatcher
    __assertfail
    bx lr
    LEAF_END KiUserExceptionDispatcher

    END
