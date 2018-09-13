/*
**  This file contains the set of internal data structures
**  for the break point engine.
*/

/*
**  This defines what a parsed but unbound break point line looks
**  like.
*/

typedef struct BPP {

    //
    //  Mark flags
    //
    ULONG   fNewMark        :1;     // Newly Marked
    ULONG   fMarkAdd        :1;     // Mark for addition
    ULONG   fMarkDelete     :1;     // Mark for deletion
    ULONG   fMarkDisable    :1;     // Mark for disable
    ULONG   fMarkEnable     :1;     // Mark for enable
    ULONG   fMarkChange     :1;     // Mark for change
    ULONG   fReplacement    :1;     // Replaces a changed BP

    //
    //  Flags to determine breakpoint type
    //
    ULONG   fWndProc        :1;     // Window Proc BP:     Requires fCodeAddr
    ULONG   fCodeAddr       :1;     // Code Addr. BP
    ULONG   fExpression     :1;     // Expression BP
    ULONG   fMemory         :1;     // Memory BP
    ULONG   fMsg            :1;     // Msg BP
    ULONG   fMsgClass       :1;     // Msg is Msg class (i.e. mask)
    ULONG   fMsgParsed      :1;     // Msg has been parsed
    ULONG   fTemporary      :1;     // Temp BP, for gountil

    //
    //  Other flags
    //
    ULONG   fHighlight      :1;     // Is the highlight currently marked?
    ULONG   fQuiet          :1;     // Quiet defer
    ULONG   fProcess        :1;     // Process set
    ULONG   fThread         :1;     // Thread set

    UINT    iBreakpointNum;         // Breakpoint Number
    BPSTATE bpstate;                // BP state
    HTM     hTm;                    // Handle to expression parse tree

    //
    //  Breakpoint Address
    //
    ADDR    addr;                   // CS:IP Address
    HPID    hPid;                   // PID for the breakpoint
    HTID    hTid;                   // TID for the breakpoint
    CXF     Cxf;                    // Breakpoint context

    UINT    iPid;                   // Process number
    UINT    iTid;                   // Thread number

    DWORD64 qwNotify;               // Tag to bind w/ em/dm's BP


    //
    //  Highlight info
    //
    ATOM    atmHighlightFile;       // File where the hightlight currently is
    UINT    iHighlightLine;         // Line where the hightlight currently is

    //
    //  Pass count stuff
    //
    UINT    iPassCount;             // Number of hits left to pass
    UINT    macPassCount;           // Original number of hits left to pass

    //
    //  Memory Breakpoint stuff
    //
    UINT    cbDataSize;             // Number of bytes to check
    ADDR    AddrMem;                // Memory address to check
#if 0
    PBYTE   Mem;                    // Previous memory contents
#endif

    //
    // Flags to indicate read, readwrite, execute, or code
    //
    DWORD   BpType;

    //
    //  Message breakpoint stuff
    //
    DWORD        Msg;               // Msg or Msg class
    LPMESSAGEMAP MsgMap;            // Msg Map

    //
    //  Change handle if marked for change
    //
    HBPT    ChangeHbpt;

    //
    //  BP options in string & atom form
    //
    ATOM    atmAddrCxt;             // Address Context
    char   *szAddrExpr;             // CS:IP Address
    ATOM    atmCxtDef;              // BP Cxt.  i.e {,file,exe}
    ATOM    atmPassCount;           // BP pass count
    char   *szCmdLine;              // Commands to execute
    ATOM    atmExpr;                // Exp. for Expression BPs
    ATOM    atmData;                // Data BP address expression
    ATOM    atmRangeSize;           // Data BP data size - see cbDataSize
    ATOM    atmMsg;                 // Message string

} BPP;

typedef struct BPP FAR *    LPBPP;

