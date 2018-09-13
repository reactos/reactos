/*
** lzpriv.h - Private information for LZEXPAND.DLL.
**
** Author:  DavidDi
*/


// Constants
/////////////

#define IN_BUF_LEN         (1 + 1024)  // size of input buffer
#define EXP_BUF_LEN        1024     // size of expanded data buffer
#define RING_BUF_LEN       4096     // size of ring buffer data area
#define MAX_RING_BUF_LEN   4224     // total size of ring buffer
#define MAX_CHAR_CODES     400      // maximum number of character codes

#define MAX_LZFILES        16       // maximum number of LZFile structs

#define LZ_TABLE_BIAS      1024     // offset of first LZFile entry in table of
                                    // handles, should be > 255
                                    //  (255 == largest possible DOS file handle)

#define STYLE_MASK         0xff0f   // wStyle mask used to determine whether
                                    // or not to set up an LZFile information
                                    // struct in LZOpenFile()
                                    // (used to ignore SHARE bits)

#define LZAPI  PASCAL


// Decoding bit flags used in LZFile.DecodeState.wFlags:

#define LZF_INITIALIZED     0x00000001 // 1 ==> buffers have been initialized
                                       // 0 ==> not initialized yet

// DOS Extended Error Codes

#define DEE_FILENOTFOUND   0x02     // File not found.  Awww...


// Types
/////////

typedef struct tagLZFile
{
   int dosh;                        /* DOS file handle of compressed file */

   BYTE byteAlgorithm;              /* compression algorithm */

   WORD wFlags;                     /* bit flags */

   unsigned long cbulUncompSize;    /* uncompressed file size */
   unsigned long cbulCompSize;      /* compressed file size */

   RTL_CRITICAL_SECTION semFile;    /* protect against >1 threads LZReading the same file all at once */

   long lCurSeekPos;                /* expanded file pointer position */

   PLZINFO pLZI;

} LZFile;


// Globals
///////////

extern HANDLE      rghLZFileTable[MAX_LZFILES];

// Prototypes
//////////////

// state.c
VOID SetGlobalBuffers(LZFile FAR *pLZFile);
VOID SaveDecodingState(LZFile FAR *pLZFile);
VOID RestoreDecodingState(LZFile FAR *pLZFile);
INT ConvertWin32FHToDos(HFILE DoshSource);
HFILE ConvertDosFHToWin32(INT DoshSource);

