#ifndef __INCLUDE_NTOSKRNL_CONFIG_H
#define __INCLUDE_NTOSKRNL_CONFIG_H

/********** dbg/print.c **********/

/* Enable serialization of debug messages printed with DbgPrint
 *
 * If this is enabled DbgPrint will queue messages if another thread is already
 * printing a message, and immediately returns. The other thread will print
 * queued messages before it returns.
 * It could happen that some messages are lost if the processor is halted before
 * the message queue was flushed.
 */
#undef SERIALIZE_DBGPRINT

/********** mm/ppool.c **********/

/* Disable Debugging Features */
#if !DBG
    /* Enable strict checking of the nonpaged pool on every allocation */
    #undef ENABLE_VALIDATE_POOL

    /* Enable tracking of statistics about the tagged blocks in the pool */
    #undef TAG_STATISTICS_TRACKING

    /* Enable Memory Debugging Features/Helpers */
    #undef POOL_DEBUG_APIS

    /* Enable Redzone */
    #define R_RZ 0

    /* Enable Allocator Stack */
    #define R_STACK 0

    /*
     * Put each block in its own range of pages and position the block at the
     * end of the range so any accesses beyond the end of block are to invalid
     * memory locations.
     */
    #undef WHOLE_PAGE_ALLOCATIONS
#endif

#endif /* __INCLUDE_NTOSKRNL_CONFIG_H */

