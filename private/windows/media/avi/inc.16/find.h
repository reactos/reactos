/**********************************************************************
//	FIND.H
//
//		Copyright (c) 1992 - Microsoft Corp.
//		All rights reserved.
//		Microsoft Confidential
//
//	Include file with #defines and prototypes for qgrep functions used
//	by the Jaguar search engine.
**********************************************************************/

#include	<setjmp.h>

//*********************************************************************
//	Constant definitions.
//*********************************************************************

#if 0
typedef long (far pascal *FIND_CALLBACK)( int Func, unsigned uArg0,\
									 	 				void far *pArg1, void far *pArg2,\
										 	 			void far *pArg3 );

#define	FLG_FIND_NOMATCH	0x0001		// Display non-matching lines
#define	FLG_FIND_COUNT		0x0002		// Display on count of matching lines
#define	FLG_FIND_LINENO	0x0004		// Display line numbers on output
#endif

#define	FIND_FILE			0x0100		// Only searching for a file (EXCLUSIVE)
#define	FIND_NOT				0x0200		// Searching for /NOT strings


#define BUFLEN		256						// Temporary buffer length
#define PATMAX		512						// Maximum parsed pattern length

#define	MAXSTRLEN	128					// Maximum search string length
#define	TRTABLEN		256					// Translation table length
													// Total bytes in StringList array
#define	STRING_LST_LEN		((TRTABLEN + 1) * sizeof(char *))

// #ifndef NOASM
//		#define match	exprmatch
// #endif

//*********************************************************************
//	Pattern token types.
//*********************************************************************

#define T_END		0							// End of expression
#define T_STRING	1							// String to match
#define T_SINGLE	2							// Single character to match
#define T_CLASS	3 							// Class to match
#define T_ANY		4							// Match any character
#define T_STAR		5							// *-expr

//*********************************************************************
//*********************************************************************

typedef struct stringnode
{
	struct stringnode *s_alt;				// List of alternates
	struct stringnode *s_suf;				// List of suffixes
	int s_must; 								// Length of portion that must match
}
STRINGNODE; 									// String node
													// Text field access macro

#define s_text(x) ( ((char *)(x)) + (sizeof( STRINGNODE ) + ((x)->s_must & 1)) )

//*********************************************************************
//	Type definitions.
//*********************************************************************

typedef struct exprnode
{
	struct exprnode	*ex_next;			// Next node in list
	struct exprnode	*ex_dummy;			// Needed by freenode()
	char					*ex_pattern; 		// Pointer to pattern to match
}
EXPR; 											// Expression node

//*********************************************************************
//	QGREP function prototypes
//*********************************************************************

extern void (*addstr)( char *s, int n );
extern char	*(*find)	( char *buffer, char *bufend );
extern void	(*flush1)( void );
extern int	(*grep)	( char *startbuf, char *endbuf, char *name, int *first );
extern void	(*write1)( char *buffer, int buflen );

int			InitGrepInfo( char *pStrLst, char *pNotList, unsigned uOpts );
int			FreeGrepBufs( void );
int			InitGrepBufs( void );
int			FileFindGrep( int fHandle, unsigned fFlags,
								  long (far pascal *AppCb)( int Func,
																	 unsigned uArg0,
																	 void far *pArg1,
																	 unsigned long ulArg2 ) );
void			addexpr		( char *e, int n );
void 			addstring	( char *s, int n );
int			addstrings	( char *buffer, char *bufend, char *seplist, int *first );
void			addtoken		( char *e, int n );
char			*alloc		( unsigned size );
void			bitset		( unsigned char *bitvec, int first, int last, int bitval );
int			enumlist		( struct stringnode *node, int cchprev );
int			enumstrings	( void );
int			exprmatch	( char *s, char *p );
char			*exprparse	( char *p, int *NewBufLen );
char			*findall		( char *buffer, char *bufend );
char			*findlist	( char *buffer, char *bufend );
char			*findone		( char *buffer, char *bufend );
void			freenode		( struct stringnode *x );
char			*get1stcharset( char *e, unsigned char *bitvec );
int			isexpr		( char *s, int n );
int			istoken		( char *s, int n );
void			maketd1		( unsigned char *pch, int cch, int cchstart );
int			match			( char *s, char *p );
void			matchstrings( char *s1, char *s2, int len, int *nmatched,
								  int *leg );
STRINGNODE	*newnode		( char *s, int n );
static int	newstring	( unsigned char *s, int n );
char			*NextEol		( char *pchChar, char *EndBuf );
int			preveol		( char *s );

STRINGNODE	*reallocnode(  register STRINGNODE *node, char *s, int n );
char			*simpleprefix( char *s, char **pp );
int			strncspn		( char *s, char *t, int n );
int			strnspn		( char *s, char *t, int n );
char			*strnupr		( char *pch, int cch );
void			SwapSrchTables( void );
int			cmpicase		( char * buf1, char * buf2, unsigned int count );

char			*findexpr	( char *buffer, char *bufend );

//*********************************************************************
//	Bit flag definitions
//*********************************************************************

#define	SHOWNAME		0x001					// Print filename
#define	NAMEONLY		0x002					// Print filename only
#define	LINENOS		0x004					// Print line numbers
#define	BEGLINE		0x008					// Match at beginning of line
#define	ENDLINE		0x010					// Match at end of line
#define	DEBUG			0x020					// Print debugging output
#define	TIMER			0x040					// Time execution
#define	SEEKOFF		0x080					// Print seek offsets
#define	ALLLINES		0x100					// Print all lines before/after match
#define	COLNOS		0x200					// Show column numbers (if LINENOS)
#define	CNTMATCH		0x400					// Show count of matching lines
#define	NEWDISP		0x800

#ifndef TRUE
	#define	TRUE 1
#endif

#ifndef	FALSE
	#define FALSE 0
#endif

//*********************************************************************
//	Miscellaneous constants.
//*********************************************************************

#define	EOS				('\r')			// End of string character

//*********************************************************************
//	Data shared among source files.
//*********************************************************************

extern char	*Target;								// Buffer for srch string being added
extern int	CaseSen;								// Case-sensitivity flag
extern int	Flags;								// Flags
extern int	StrCount;							// String count
extern jmp_buf	ErrorJmp;						// Storage location for setjmp()

	// All of the data below is located in DATA.ASM to allow	swapping
	// blocks of search data with a single memmove() call.

#define	SWAP_LEN		((sizeof( int ) * 8) + (sizeof( char * ) * 4))
#define	INIT_LEN		(sizeof( int ) * 8)

	// Storage for "string" search trees.
extern int			DummyFirst;
extern int			TblEntriesUsed;			// Number of transtab entries used
extern int			ExprEntriesUsed;			// Number of expression strings used
extern int			StrCount;					// String count
extern int			TargetLen; 					// Length of last string added
extern int			MaxChar;						// Max char value in srch string
extern int			MinChar;						// Min char value in srch string
extern int			ShortStrLen;				// Min string length added to list
extern char				**ExprStrList;			// Array of ptrs to srch expressions
extern STRINGNODE		**StringList;			// Array of ptrs to srch strings
extern unsigned char	*td1;						// Ptr to TD1 shift table
extern unsigned char	*TransTable;			// Allocated in grepmain()

extern int			nDummyFirst;
extern int			nTblEntriesUsed;			// Number of transtab entries used
extern int			nExprEntriesUsed;			// Number of expression strings used
extern int			nStrCount;					// String count
extern int			nTargetLen; 				// Length of last string added
extern unsigned	nMaxChar;					// Max char value in search string
extern unsigned 	nMinChar;					// Min char value in srch string
extern int			nShortStrLen;				// Min string length added to list
extern char				**nExprStrList;		// Array of ptrs to srch expressions
extern STRINGNODE		**nStringList;			// Array of ptrs to srch strings
extern unsigned char	*ntd1;					// Ptr to TD1 shift table
extern unsigned char	*nTransTable;			// Allocated in grepmain()


extern unsigned InitialSearchData;			// First word in area containing
														// initial search values.

extern char			*ReadBuf;					// Ptr to buffer for file reads
extern char			*Target;						// Tmp buf for string being added

extern unsigned char	*achcol;					// Ptr to collate table

//*********************************************************************
//	Added for purposes of integrating the message subsysstem
//*********************************************************************

struct sublistx
{
	unsigned char	size;	       			// sublist size			      
	unsigned char	reserved;      		// reserved for future growth	      
	unsigned far	*value;	      		// pointer to replaceable parm	      
	unsigned char	id;	       			// type of replaceable parm	      
	unsigned char	Flags;	      		// how parm is to be displayed	      
	unsigned char	max_width;     		// max width of replaceable field      
	unsigned char	min_width;     		// min width of replaceable field      
	unsigned char	pad_char;      		// pad character for replaceable field 
};

