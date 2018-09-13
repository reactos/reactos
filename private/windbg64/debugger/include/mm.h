/*** mm.h - Memory Manager public data and routines
*
*   Copyright <C> 1989, Microsoft Corporation
*
* Purpose:  handle the near and far memory requests of cw and help systems.
*
*
*************************************************************************/
//
//  MB (Memory Block) memory management structures
//

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(1)     // pack on byte boundary

typedef struct _MBHI {          // memory block handle information
    unsigned char   index;
    unsigned char   flags   : 4;
    unsigned char   mbNum   : 4;
    } MBHI;

typedef union _MBH {            // memory block handle
    HDEP            hmem;
    MBHI            mbhi;
    } MBH;

typedef struct _MB {            // memory blocks
    unsigned char   flags;
    short           swLocked;
    unsigned short  cb;
    void FAR *      lpvBlock;
    } MB;


typedef MB FAR *    PMB;

//
// Based Heap Memory manager data structures and defines.
//
enum HEAPPRIORITY {
        MMEMERGENCYHEAP,
        MMCOWHEAP,
        MMDLLHEAP,
        MMHELPHEAP
};

enum _HANDLEFLAGS {
    MALLOCED        = 0x1,  // memory was allocated with near malloc
    FMALLOCED       = 0x2,  // memory was allocated with far malloc
    CVMALLOCED      = 0x4   // memory was allocated from cv far memory pool
    };


#define MBUNDEFINED (0) // undefined handle value.

#pragma pack()     // packing off


#ifdef __cplusplus
} // extern "C" {
#endif
