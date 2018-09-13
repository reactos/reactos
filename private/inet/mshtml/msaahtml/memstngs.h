// Setings for debug the memory manager
#if !defined(MEMSTNGS_H_INCLUDED)
#define MEMSTNGS_H_INCLUDED

#if defined(_DEBUG)

// Number of bytes to add to the beginning and end of the block
#define NUMPADBYTES     32
// Number of bytes to dump
#define MAXBYTESTODUMP  30

// The amount of memory allocated that will trigger release of memory
#define COMPACTMEMORYTHRESHOLD  5000000

#define PADVALUE        0x9E
#define FREEVALUE       0xCC
#define FILLVALUE       0x55


// TRUE if memory allocation are being traced
#define TRACEMEMALLOCS  FALSE
// TRUE if we fill the memory with randome values before returning
#define FILLMEMORY      TRUE
// If true we will fill the memory with randome values, if false with 0
#define RANDOMFILL      TRUE
// TRUE if we keep the released memory blocks around to check for usage
#define KEEPRELEASEDMEMORY TRUE


#endif
#endif
