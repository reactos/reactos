
#include <ksarm.h>


    TEXTAREA

    LEAF_ENTRY KeSwitchKernelStack
    __assertfail
    bx lr
    LEAF_END KeSwitchKernelStack

    LEAF_ENTRY KiPassiveRelease
    __assertfail
    bx lr
    LEAF_END KiPassiveRelease

    END
/* EOF */
