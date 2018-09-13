/*
** buffers.h - Function prototypes and global variables used to manipulate
** buffers used in I/O and expansion.
**
** Author:  DavidDi
*/


// Constants
/////////////

// N.b., rgbyteInBuf[] allocated with one extra byte for UnreadByte().
#define MAX_IN_BUF_SIZE    32768U   // maximum size of input buffer
#define MAX_OUT_BUF_SIZE   32768U   // maximum size of output buffer

#define IN_BUF_STEP        1024U    // decrement sizes used in I/O buffer
#define OUT_BUF_STEP       1024U    // allocation in InitBuffers()

#define FLUSH_BYTE         ((BYTE) 'F')   // dummy character used to flush
                                          // rgbyteOutBuf[] to output file

#define END_OF_INPUT       500      // ReadInBuf() EOF flag for input file

// DOS file handle flag indicating that the compression savings should be
// computed, but no output file written.
#define NO_DOSH            (-2)

#define READ_IT            TRUE     // GetIOHandle() bRead flag values
#define WRITE_IT           FALSE


// Macros
//////////

// Read a byte (buffered) from input file.  Stores byte read in argument.
// Returns TRUE if successful, or one of ReadInBuf()'s error codes if
// unsuccessful.
//-protect-
#define ReadByte(byte)         ((pLZI->pbyteInBuf < pLZI->pbyteInBufEnd) ? \
                               ((byte = *pLZI->pbyteInBuf++), TRUE) : \
                               ReadInBuf((BYTE ARG_PTR *)&byte, doshSource, pLZI))

// Put at most one byte back into the buffered input.  N.b., may be used at
// most (pbyteInBuf - &rgbyteInBuf[1]) times.  E.g., may be used only once at
// beginning of buffer.  Return value is always TRUE.
//-protect-
#define UnreadByte()          ((pLZI->pbyteInBuf == &pLZI->rgbyteInBuf[1]) ? \
                               (pLZI->bLastUsed = TRUE) : \
                               (--pLZI->pbyteInBuf, TRUE))

// Write a byte (buffered) to output file.  Returns TRUE if successful, or
// one of WriteOutBuf()'s error codes if unsuccessful.  ALWAYS increments
// cblOutSize.
#define WriteByte(byte)        ((pLZI->pbyteOutBuf < pLZI->pbyteOutBufEnd) ? \
                               ((*pLZI->pbyteOutBuf++ = byte), pLZI->cblOutSize++, TRUE) : \
                               (pLZI->cblOutSize++, WriteOutBuf(byte, doshDest, pLZI)))

// Flush output buffer.  DOES NOT increment cblOutSize.  N.b., you cannot
// perform a valid UnreadByte() immediately after FlushOutputBuffer() because
// the byte kept will be the bogus FLUSH_BYTE.
#define FlushOutputBuffer(dosh, pLZI)  WriteOutBuf(FLUSH_BYTE, dosh, pLZI)

// Reset buffer pointers to empty buffer state.
//-protect-
#define ResetBuffers()        {  pLZI->pbyteInBufEnd = &pLZI->rgbyteInBuf[1] + pLZI->ucbInBufLen; \
                                 pLZI->pbyteInBuf = &pLZI->rgbyteInBuf[1] + pLZI->ucbInBufLen; \
                                 pLZI->bLastUsed = FALSE; \
                                 pLZI->pbyteOutBufEnd = pLZI->rgbyteOutBuf + pLZI->ucbOutBufLen; \
                                 pLZI->pbyteOutBuf = pLZI->rgbyteOutBuf; \
                                 pLZI->cblOutSize = 0L; \
                              }

// The buffer pointers are initialized to NULL to indicate the buffers have
// not yet been allocated.  init.c!InitGlobalBuffers() allocates the buffers
// and sets the buffers' base pointers.  buffers.h!ResetBufferPointers() sets
// the buffers' current position and end position pointers.

// Prototypes
//////////////

// buffers.c
extern INT ReadInBuf(BYTE ARG_PTR *pbyte, INT doshSource, PLZINFO pLZI);
extern INT WriteOutBuf(BYTE byteNext, INT doshDest, PLZINFO pLZI);

// init.c
extern PLZINFO InitGlobalBuffers(DWORD dwOutBufSize, DWORD dwRingBufSize, DWORD dwInBufSize);
extern PLZINFO InitGlobalBuffersEx();
extern VOID FreeGlobalBuffers(PLZINFO);

extern INT GetIOHandle(CHAR ARG_PTR *pszFileName, BOOL bRead, INT ARG_PTR *pdosh,
   LONG *pcblInSize);

