/* $$$ Modified by ReactOS config tool on Sun Nov 28 18:18:50 2004
 */
#ifndef __INCLUDE_NTOSKRNL_CONFIG_H
#define __INCLUDE_NTOSKRNL_CONFIG_H

/* Enable strict checking of the nonpaged pool on every allocation */
#undef ENABLE_VALIDATE_POOL

/* Enable tracking of statistics about the tagged blocks in the pool */
#undef TAG_STATISTICS_TRACKING

/*
 * Put each block in its own range of pages and position the block at the
 * end of the range so any accesses beyond the end of block are to invalid
 * memory locations.
 */
#undef WHOLE_PAGE_ALLOCATIONS

#endif /* __INCLUDE_NTOSKRNL_CONFIG_H */

