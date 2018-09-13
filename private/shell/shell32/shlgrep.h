//
// Extracts from search.h and find.h
//
// *********************************************************************
// Macros for creating buffers based boundaried on sector sizes
// *********************************************************************


#define SECTORLEN   (512)                   // Sector length
#define LG2SECTOR   10                      // Log 2 sector length
#define FILBUFLEN   (SECTORLEN*8)       // File buffer length (4096 bytes)

#define SectorBased( x )        ((unsigned)(x) & (0xffff << LG2SECTOR))

// *********************************************************************
//  FIND.H
// *********************************************************************

// *********************************************************************
//  Constant definitions.
// *********************************************************************


#define FIND_FILE   0x0100      // Only searching for a file (EXCLUSIVE)
#define FIND_NOT    0x0200      // Searching for /NOT strings


#define BUFLEN      256         // Temporary buffer length
#define PATMAX      512         // Maximum parsed pattern length

#define MAXSTRLEN   128         // Maximum search string length
#define TRTABLEN    256         // Translation table length
                                // Total bytes in StringList array
#define STRING_LST_LEN      ((TRTABLEN + 1) * SIZEOF(TCHAR *))

#define FLG_NOT                 0x0001  // Apply inverse to next switch (/NOT)
#define FLG_DEFAULT             0x0002  // Processing default cmd  string
#define FLG_REGULAR             0x0004  // Use regular expressions      (/R)
#define FLG_CASE                0x0008  //      Case sensitive searches      (/C)
#define FLG_NOTSRCH             0x0010  // Match file not containing string
#define FLG_SRCHSUB             0x0020  // Search subdirectories                  (/S)
#define FLG_NOT_SRCHSUB         0x0040  //      Explicid don't search subs (/NOT /S)
#define FLG_NOT_REGULAR         0x0080  //      Expl. not on regular exprs (/NOT /RE)
#define FLG_NOT_CASE            0x0100  // Expl. not on case sense (/NOT /CS)

#define FLG_DIR_ENTER           0x1000  // Do callback on leaving subdirectory
#define FLG_DIR_LEAVE           0x2000  // Do callback on leaving subdirectory
#define FLG_GET_ACCESS          0x4000  // Always get access on matching files

// *********************************************************************
//  Pattern token types.
// *********************************************************************

#define T_END       0                           // End of expression
#define T_STRING    1                           // String to match
#define T_SINGLE    2                           // Single character to match
#define T_CLASS 3                           // Class to match
#define T_ANY       4                           // Match any character
#define T_STAR      5                           // *-expr

#define OK 0

// *********************************************************************
// *********************************************************************

typedef struct stringnode
{
    struct stringnode *s_alt;               // List of alternates
    struct stringnode *s_suf;               // List of suffixes
    int s_must;                                 // Length of portion that must match
}
STRINGNODE;                                     // String node
typedef STRINGNODE * LPSTRINGNODE;
                                                // Text field access macro

#define s_text(x) ( ((LPSTR)(x)) + (SIZEOF( STRINGNODE ) + ((x)->s_must & 1)) )

// *********************************************************************
//  Type definitions.
// *********************************************************************

typedef struct exprnode
{
    struct exprnode *ex_next;           // Next node in list
    struct exprnode *ex_dummy;          // Needed by freenode()
    LPSTR ex_pattern;    // Pointer to pattern to match
}
EXPR;                                       // Expression node

typedef struct grepelements                 // Bad name, but...
{
    // Variables for grep strings.
    int         TblEntriesUsed;         // Number of transtab entries used
    int         ExprEntriesUsed;        // Number of expression strings used.
    int         StrCount;               // String count
    int         TargetLen;              // Length of last string added
    int         MaxChar;                // Max char value in search string
    int         MinChar;                // Min  char value in search string
    int         ShortStrLen;            // Minimum string length added to list
    LPSTR       ExprStrList[TRTABLEN+1]; // Array of ptrs to search expressions
    LPSTRINGNODE StringList[TRTABLEN+1]; // Array of ptrs to search strings
    char        td1[TRTABLEN];           // TD1 shift table
    char        TransTable[TRTABLEN];    // Allocated in grepmain()
} GREPELEMENTS;

typedef struct grepinfo
{
    char        ReadBuf[FILBUFLEN];     // Ptr to buffer for file reads

    // strings.
    char        Target[MAXSTRLEN];      // Buffer to hold Max Strings.

    // Variables for grep strings.
    GREPELEMENTS ge;                    // Main Grep elements.


    // Storage for /NOT "string" search trees.
    GREPELEMENTS geNot;

    // More values to control the search
    int         CaseSen;                // Assume case-sensitivity
    int         Flags;                      // Global search flags
    long        FileLen;                    // Number of bytes in file
    signed long LineNum;                  // Current line number
    signed long MatchCnt;                 // Count of matching lines
                                          // MatchCnt initialized in Qgrep()
    void        (*addstr)   (struct grepinfo *, LPSTR , int);
    LPSTR       (*find)     (struct grepinfo *, LPSTR , LPSTR );
    int         (*ncmp)     ( LPCSTR , LPCSTR , int);

}
GREPINFO, * LPGREPINFO;

// *********************************************************************
//  QGREP function prototypes
// *********************************************************************

LPVOID      AllocThrow  ( LPGREPINFO lpgi, long cb);
void        addexpr     ( LPGREPINFO lpgi, LPSTR e, int n );
void        addstring   ( LPGREPINFO lpgi,LPSTR s, int n );
int         addstrings  ( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend, LPSTR seplist, int *first );
void        addtoken    ( LPGREPINFO lpgi, LPSTR e, int n );
void        bitset      ( LPBYTE bitvec, int first, int last, int bitval );
int         enumlist    ( LPGREPINFO lpgi, struct stringnode *node, int cchprev );
int         enumstrings ( LPGREPINFO lpgi );
int         exprmatch   ( LPGREPINFO lpgi, LPSTR s, LPSTR p );
LPSTR       exprparse   ( LPGREPINFO lpgi, LPSTR p, int *NewBufLen );
LPSTR       findall     ( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend );
LPSTR       findlist    ( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend );
LPSTR       findone     ( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend );
void        freenode    ( LPGREPINFO lpgi, struct stringnode *x );
LPSTR       get1stcharset( LPGREPINFO lpgi, LPSTR e, LPBYTE bitvec );
int         isexpr      ( LPGREPINFO lpgi, LPSTR s, int n );
int         istoken     ( LPSTR s, int n );
void        maketd1     ( LPGREPINFO, LPBYTE pch, int cch, int cchstart );
int         match       ( LPGREPINFO lpgi, LPSTR s, LPSTR p );
void        matchstrings( LPGREPINFO lpgi, LPSTR s1, LPSTR s2,
                                  int len, int *nmatched,
                                  int *leg );
STRINGNODE FAR* newnode ( LPGREPINFO lpgi, LPSTR s, int n );
static int  newstring   ( LPGREPINFO lpgi, LPBYTE s, int n );
LPSTR       NextEol     ( LPSTR pchChar, LPSTR EndBuf );
int         preveol     ( LPSTR s );

STRINGNODE FAR* reallocnode(  register STRINGNODE *node, LPSTR s, int n );
LPSTR       simpleprefix( LPGREPINFO lpgi, LPSTR s, LPSTR *pp );
int         strncspn    ( LPSTR s, LPSTR t, int n );
int         strnspn     ( LPSTR s, LPSTR t, int n );
LPSTR       strnupr    ( LPSTR pch, int cch );
void        SwapSrchTables( LPGREPINFO lpgi );
int         cmpicase    ( LPSTR  buf1, LPSTR  buf2, unsigned int count );

LPSTR       findexpr   ( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend );

// *********************************************************************
//  Bit flag definitions
// *********************************************************************

#define SHOWNAME        0x001                   // Print filename
#define NAMEONLY        0x002                   // Print filename only
#define LINENOS     0x004                   // Print line numbers
#define BEGLINE     0x008                   // Match at beginning of line
#define ENDLINE     0x010                   // Match at end of line
#define DEBUGMSGS       0x020                   // Print debugging output
#define TIMER           0x040                   // Time execution
#define SEEKOFF     0x080                   // Print seek offsets
#define ALLLINES        0x100                   // Print all lines before/after match
#define COLNOS      0x200                   // Show column numbers (if LINENOS)
#define CNTMATCH        0x400                   // Show count of matching lines
#define NEWDISP     0x800

// *********************************************************************
//  Miscellaneous constants.
// *********************************************************************

#define EOS             ('\r')          // End of string character

// *********************************************************************
//  Data shared among source files.
// *********************************************************************


BOOL WINAPI FFileContainsString(LPTSTR pszPathName, LPSTR pszStr);
LPGREPINFO  InitGrepInfo( LPSTR pStrLst, LPSTR pNotList, unsigned uOpts );
int  FileFindGrep( LPGREPINFO lpgi, HANDLE fh, unsigned fFlags,
              long (far pascal *AppCb)( int Func,
                                        unsigned uArg0,
                                        void far *pArg1,
                                        unsigned long ulArg2 ) );

LPGREPINFO InitGrepBufs( void );
int FreeGrepBufs(LPGREPINFO lpgi);


#define ASCII_LEN           256                 // # of ASCII characters
