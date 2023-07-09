typedef struct tagLZI {
    BYTE *rgbyteRingBuf;    // ring buffer for expansion
    BYTE *rgbyteInBuf;      // input buffer for reads
    BYTE *pbyteInBufEnd;    // pointer past end of rgbyteInBuf[]
    BYTE *pbyteInBuf;       // pointer to next byte to read from
    BYTE *rgbyteOutBuf;     // output buffer for writes
    BYTE *pbyteOutBufEnd;   // pointer past end of rgbyteOutBuf[]
    BYTE *pbyteOutBuf;      // pointer to last byte to write from
    // Flag indicating whether or not rgbyteInBuf[0], which holds the last byte
    // from the previous input buffer, should be read as the next input byte.
    // (Only used so that at least one unReadUChar() can be done at all input
    // buffer positions.)
    BOOL bLastUsed;
    // Actually, rgbyteInBuf[] has length (ucbInBufLen + 1) since rgbyteInBuf[0]
    // is used when bLastUsed is TRUE.
    INT cbMaxMatchLen;      // longest match length for current algorithm
    LONG cblInSize,         // size in bytes of input file
         cblOutSize;        // size in bytes of output file
    DWORD ucbInBufLen,      // length of input buffer
          ucbOutBufLen;     // length of output buffer
    DWORD uFlags;           // LZ decoding description byte
    INT iCurRingBufPos;     // ring buffer offset
    INT *leftChild;         // parents and left and right
    INT *rightChild;        // children that make up the
    INT *parent;            // binary search trees

    INT iCurMatch,          // index of longest match (set by LZInsertNode())
        cbCurMatch;         // length of longest match (set by LZInsertNode())

} LZINFO;

typedef LZINFO *PLZINFO;

typedef struct _dcx {
    INT dcxDiamondLastIoError;
    HFDI dcxFdiContext;
    ERF dcxFdiError;
} DIAMOND_CONTEXT;

typedef DIAMOND_CONTEXT *PDIAMOND_CONTEXT;


extern DWORD itlsDiamondContext;

#define ITLS_ERROR          (0xFFFFFFFF)

#define GotDmdTlsSlot()     (itlsDiamondContext != ITLS_ERROR)
#define GotDmdContext()     (TlsGetValue(itlsDiamondContext) != NULL)

#define FdiContext          (((GotDmdTlsSlot() && GotDmdContext()) ? ((PDIAMOND_CONTEXT)(TlsGetValue(itlsDiamondContext)))->dcxFdiContext : NULL))
#define SetFdiContext(v)    (((PDIAMOND_CONTEXT)(TlsGetValue(itlsDiamondContext)))->dcxFdiContext = (v))
#define FdiError            (((PDIAMOND_CONTEXT)(TlsGetValue(itlsDiamondContext)))->dcxFdiError)
#define DiamondLastIoError  (((PDIAMOND_CONTEXT)(TlsGetValue(itlsDiamondContext)))->dcxDiamondLastIoError)

DWORD
InitDiamond(
    VOID
    );

VOID
TermDiamond(
    VOID
    );

BOOL
IsDiamondFile(
    IN PSTR FileName
    );

INT
ExpandDiamondFile(
    IN  PSTR       SourceFileName,  // Because LZOpen ... returns ASCII!
    IN  PTSTR      TargetFileName,
    IN  BOOL       RenameTarget,
    OUT PLZINFO    pLZI
    );
