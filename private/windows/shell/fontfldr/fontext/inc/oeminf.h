/*
** oeminf.h - Public interface for oeminf.c.
*/

// Constants
/////////////

#define MAX_INF_COMP_LEN      0xffff   // Must not overrun a segment.
#define MAX_INF_READ_SIZE     0xffff   // INT_MAX  // Must not overrun an int.

#define INF_PREFIX            TEXT( "oem" )
#define INF_EXTENSION         TEXT( "inf" )

#define MAX_5_DEC_DIGITS      99999

// Buffer size for Get...Directory() calls.
#define MAX_NET_PATH          MAX_PATH

// .inf file specification and length.
#define OEM_STAR_DOT_INF      TEXT( "\\oem*.inf" )
#define OEM_STAR_DOT_INF_LEN  9


// Macros
//////////

#define IS_PATH_SEPARATOR(c)  ((c) == TEXT( '\\' ) || (c) == TEXT( '/' ) || (c) == TEXT( ':' ))
#define IS_SLASH(c)           ((c) == TEXT( '\\' ) || (c) == TEXT( '/' ))

BOOL   FAR PASCAL RunningFromNet( void );
HANDLE FAR PASCAL ReadFileIntoBuffer( int doshSource );
int    FAR PASCAL FilesMatch( HANDLE h1, HANDLE h2, unsigned uLength );
LPTSTR FAR PASCAL TruncateFileName( LPTSTR lpszPathSpec );
int    FAR PASCAL OpenFileAndGetLength( LPTSTR pszSourceFile, LPLONG plFileLength );
int    FAR PASCAL IsNewFile( LPTSTR lpszSourceFile, LPTSTR lpszSearchSpec );
LPTSTR FAR PASCAL MakeUniqueFilename( LPTSTR pszDirName, LPTSTR pszPrefix, LPTSTR pszExtension );
BOOL   FAR PASCAL CopyNewOEMInfFile( LPTSTR pszOEMInfPath );

// Macros
//////////

#define FILEMAX            14          // 8.3 + null terminator

// #define FOPEN(sz)          _lopen(sz, OF_READ)
// #define FCLOSE(fh)         _lclose(fh)
// #define FCREATE(sz)        _lcreat(sz, 0)


