/*
** header.h - Common information used in compressed file header manipulation.
**
** Author:  DavidDi
*/


// Constants
/////////////

// compressed file signature: "SZDDˆð'3"
#define COMP_SIG        "SZDD\x88\xf0\x27\x33"

#define COMP_SIG_LEN    8              // length of signature (bytes)
                                       // (no '\0' terminator)

#define ALG_FIRST       ((BYTE) 'A')   // first version algorithm label for
                                       // Lempel-Ziv
#define ALG_LZ          ((BYTE) 'B')   // new Lempel-Ziv algorithm label
#define ALG_LZA         ((BYTE) 'C')   // Lempel-Ziv with arithmetic encoding
                                       // algorithm label

// length of entire compressed file header (used as offset to start of
// compressed data)
#define HEADER_LEN      14
// (14 == cbCompSigLength + algorithm + extension character
//        + uncompressed length)

#define BYTE_MASK       0xff           // mask used to isolate low-order byte


// Types
/////////

// Declare compressed file header information structure.  N.b., the
// compressed file header does not contain the file size of the compressed
// file since this is readily obtainable through filelength() or lseek().
// The file info structure, however, does contain the compressed file size,
// which is used when expanding the file.
typedef struct tagFH
{
   BYTE rgbyteMagic[COMP_SIG_LEN];  // array of compressed file signature
                                    // (magic bytes)

   BYTE byteAlgorithm;              // algorithm label
   BYTE byteExtensionChar;          // last extension character
                                    // (always 0 for ALG_FIRST)

   // The file sizes are unsigned longs instead of signed longs for backward
   // compatibilty with version 1.00.
   DWORD cbulUncompSize;    // uncompressed file size
   DWORD cbulCompSize;      // compressed file size (not stored in
                                    // header)
} FH;
typedef struct tagFH *PFH;


// Macros
//////////

#if 0
#define RecognizeCompAlg(chAlg)  ((chAlg) == ALG_FIRST || \
                                  (chAlg) == ALG_LZ    || \
                                  (chAlg) == ALG_LZA)
#else
#define RecognizeCompAlg(chAlg)  ((chAlg) == ALG_FIRST)
#endif


// Prototypes
//////////////

// header.c
extern INT WriteHdr(PFH pFH, INT doshDest, PLZINFO pLZI);
extern INT GetHdr(PFH pFH, INT doshSource, LONG * pcblInSize);
extern BOOL IsCompressed(PFH pFHIn);
extern VOID MakeHeader(PFH pFHBlank, BYTE byteAlgorithm, BYTE byteExtensionChar,
   PLZINFO pLZI);

