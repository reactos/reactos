/*
** lzcommon.h - Shared information for LZ modules.
**
** Author:  DavidDi
*/


// Constants
/////////////

#define RING_BUF_LEN       4096        // size of ring buffer
#define MAX_RING_BUF_LEN   4224        // size of ring buffer - from LZFile
                                       // struct declaration in lzexpand.h

#define NIL                RING_BUF_LEN   // flag index used in binary search
                                          // trees

#define BUF_CLEAR_BYTE     ((BYTE) ' ')   // rgbyteRingBuf[] initializer

#define MAX_LITERAL_LEN    2           // encode string into position and
                                       // length if match length greater than
                                       // this value (== # of bytes required
                                       // to encode position and length)

#define FIRST_MAX_MATCH_LEN   0x10     // ALG_FIRST used this length
#define LZ_MAX_MATCH_LEN      (0x10 + MAX_LITERAL_LEN)
#define LZA_MAX_MATCH_LEN     64
                                       // upper limit for match length
                                       // (n.b., assume length field implies
                                       // length += 3)

// Maximum number of bytes LZDecode() and LZADecode() will expand beyond
// position requested.
#define MAX_OVERRUN        ((long)pLZI->cbMaxMatchLen)


// Globals
///////////

extern INT iCurMatch,      // index of longest match (set by LZInsertNode())
           cbCurMatch;     // length of longest match (set by LZInsertNode())

extern DWORD uFlags;    // LZ decoding description byte

extern INT iCurRingBufPos; // ring buffer offset

// Prototypes
//////////////

// lzcommon.c
extern BOOL LZInitTree(PLZINFO pLZI);
extern VOID LZFreeTree(PLZINFO pLZI);
extern VOID LZInsertNode(INT nodeToInsert, BOOL bDoArithmeticInsert, PLZINFO pLZI);
extern VOID LZDeleteNode(INT nodeToDelete, PLZINFO pLZI);

// lzcomp.c
extern INT LZEncode(INT doshSource, INT doshDest, PLZINFO pLZI);

// lzexp.c
extern INT LZDecode(INT doshSource, INT doshDest, LONG cblExpandedLength,
                    BOOL bRestartDecoding, BOOL bFirstAlg, PLZINFO pLZI);

