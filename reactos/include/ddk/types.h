#ifndef __INCLUDE_DDK_TYPES_H
#define __INCLUDE_DDK_TYPES_H

// these should be moved to a file like ntdef.h



typedef ULONG KAFFINITY, *PKAFFINITY;





/*
 * Various other types (all quite pointless)
 */
typedef ULONG KPROCESSOR_MODE;
typedef UCHAR KIRQL;
typedef KIRQL* PKIRQL;
typedef ULONG IO_ALLOCATION_ACTION;
typedef ULONG POOL_TYPE;
typedef ULONG TIMER_TYPE;
typedef ULONG MM_SYSTEM_SIZE;
typedef ULONG LOCK_OPERATION;

typedef LARGE_INTEGER PHYSICAL_ADDRESS;
typedef PHYSICAL_ADDRESS* PPHYSICAL_ADDRESS;




#endif /* __INCLUDE_DDK_TYPES_H */
