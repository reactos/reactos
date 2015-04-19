/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ex/arm/ioport.s
* PURPOSE:         Low level port communication functions for ARM
* PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
*/

/* INCLUDES *****************************************************************/

#include <ksarm.h>

#define __assertfail DCD 0xDEFC

    TEXTAREA

    LEAF_ENTRY READ_REGISTER_UCHAR
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_UCHAR

    LEAF_ENTRY READ_REGISTER_USHORT
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_USHORT

    LEAF_ENTRY READ_REGISTER_ULONG
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_ULONG

    LEAF_ENTRY WRITE_REGISTER_UCHAR
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_UCHAR

    LEAF_ENTRY WRITE_REGISTER_USHORT
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_USHORT

    LEAF_ENTRY WRITE_REGISTER_ULONG
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_ULONG


    LEAF_ENTRY READ_REGISTER_BUFFER_UCHAR
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_BUFFER_UCHAR

    LEAF_ENTRY READ_REGISTER_BUFFER_USHORT
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_BUFFER_USHORT

    LEAF_ENTRY READ_REGISTER_BUFFER_ULONG
    __assertfail
    bx lr
    LEAF_END READ_REGISTER_BUFFER_ULONG

    LEAF_ENTRY WRITE_REGISTER_BUFFER_UCHAR
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_BUFFER_UCHAR

    LEAF_ENTRY WRITE_REGISTER_BUFFER_USHORT
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_BUFFER_USHORT

    LEAF_ENTRY WRITE_REGISTER_BUFFER_ULONG
    __assertfail
    bx lr
    LEAF_END WRITE_REGISTER_BUFFER_ULONG

    END
/* EOF */
