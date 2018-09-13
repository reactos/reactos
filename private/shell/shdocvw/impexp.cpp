//
// IMPEXP.CPP - Browser Import and Export Code
//
// Imports and Exports Favorites in various formats
//
// julianj  2/16/98
//

//
// *** IMPORT FAVORITES CODE ***
//

   /************************************************************\
    FILE: impext.cpp

    DATE: April 1, 1996

    AUTHOR(S):  Bryan Starbuck (bryanst)

    DESCRIPTION:
    This file contains functions that can be used to upgrade
    settings from the Microsoft Internet Explorer v2.0 to v3.0,
    and some features to import Netscape features into Internet
    Explorer.

    This file will handle the logic to convert Netscape
    bookmarks to Microsoft Internet Explorer favorites.  This
    will happen by finding the location of the Netscape bookmarks
    file and the Microsoft Internet Explorer favorites directory
    from the registry.  Then it will parse the bookmarks file to
    extract the URLs, which will finally be added to the favorites
    directory.

    USAGE:
    This code is designed to be called when the user may
    want Netscape bookmarks imported into system level Favorites
    usable by programs such as Internet Explorer.  External
    users should call ImportBookmarks().  If this is done during
    setup, it should be done after setup specifies the Favorites
    registry entry and directory.  If Netscape is not installed,
    then the ImportBookmarks() is just a big no-op.

  NOTE:
    If this file is being compiled into something other
    than infnist.exe, it will be necessary to include the
    following String Resource:

    #define     IDS_NS_BOOKMARKS_DIR    137
    STRINGTABLE DISCARDABLE
    BEGIN
    ...
    IDS_NS_BOOKMARKS_DIR    "\\Imported Bookmarks"
    END


  UPDATES:  I adopted this file to allow IE4.0 having the abilities
    to upgrade from NetScape's setting.  Two CustomActions will be added
    to call in functions in this file. (inateeg)

    8/14/98: added functions to import or export via an URL,
    8/19/98: added UI to allow user to import/export via browser's File 
            menu/"Import and Exporting..."
\************************************************************/
#include "priv.h"
#include "impexp.h"
#include <regstr.h>
#include "resource.h"

#include <mluisupp.h>

//
// Information about the Netscape Bookmark file format that is shared between
// the import and export code
// 

#define BEGIN_DIR_TOKEN         "<DT><H"
#ifdef UNIX
#define MID_DIR_TOKEN0          "3>"
#endif
#define MID_DIR_TOKEN           "\">"
#define END_DIR_TOKEN           "</H"
#define BEGIN_EXITDIR_TOKEN     "</DL><p>"
#define BEGIN_URL_TOKEN         "<DT><A HREF=\""
#define END_URL_TOKEN           "\" A"
#ifdef UNIX
#define END_URL_TOKEN2          "\">"
#endif
#define BEGIN_BOOKMARK_TOKEN    ">"
#define END_BOOKMARK_TOKEN      "</A>"

#define VALIDATION_STR "<!DOCTYPE NETSCAPE-Bookmark-file-"

//
// Use by export code
// 
#define COMMENT_STR "<!-- This is an automatically generated file.\r\nIt will be read and overwritten.\r\nDo Not Edit! -->"
#define TITLE     "<TITLE>Bookmarks</TITLE>\r\n<H1>Bookmarks</H1>"

// ItemType is going to be the type of entry found in the bookmarks
// file.
typedef enum MYENTRYTYPE
{
    ET_OPEN_DIR     = 531,  // New level in heirarchy
    ET_CLOSE_DIR,           // Close level in heirarchy
    ET_BOOKMARK,            // Bookmark entry.
    ET_NONE,                // End of File
    ET_ERROR                // Bail, we encountered an error
} MyEntryType;

//////////////////////////////////////////////////////////////////
//  Internal Functions
//////////////////////////////////////////////////////////////////
BOOL    ImportNetscapeProxy(void);		// Import Netscape Proxy Setting
BOOL    UpdateHomePage(void);			// Upgrade IE v1.0 Home URL to v3.0
BOOL    ImportBookmarks(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks, HWND hwnd);			//  Import Netscape Bookmarks to IE Favorites
BOOL    ExportFavorites(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks, HWND hwnd);			//  Export IE Favorites to Netscape Bookmarks
BOOL    RegStrValueEmpty(HKEY hTheKey, char * szPath, char * szKey);
BOOL    GetNSProxyValue(char * szProxyValue, DWORD * pdwSize);

BOOL        VerifyBookmarksFile(HANDLE hFile);
BOOL        ConvertBookmarks(TCHAR * szFavoritesDir, HANDLE hFile);
MyEntryType   NextFileEntry(char ** ppStr, char ** ppToken);
BOOL        GetData(char ** ppData, HANDLE hFile);
void        RemoveInvalidFileNameChars(char * pBuf);
BOOL        CreateDir(char *pDirName);
BOOL        CloseDir(void);
BOOL        CreateBookmark(char *pBookmarkName);
BOOL        GetPathFromRegistry(LPTSTR szPath, UINT cbPath, HKEY theHKEY, LPTSTR szKey, LPTSTR szVName);
BOOL        GetNavBkMkDir( LPTSTR lpszDir, int isize);
BOOL        GetTargetFavoritesPath(LPTSTR szPath, UINT cbPath);

BOOL    PostFavorites(TCHAR *pszPathToBookmarks, TCHAR* pszPathToPost);
void    CALLBACK StatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwStatus,
            LPVOID lpvInfo, DWORD dwInfoLength);

//////////////////////////////////////////////////////////////////
//  TYPES:
//////////////////////////////////////////////////////////////////

//typedef enum MYENTRYTYPE MyEntryType;

//////////////////////////////////////////////////////////////////
//  Constants:
//////////////////////////////////////////////////////////////////
#define MAX_URL 2048
#define FILE_EXT 4          // For ".url" at the end of favorite filenames
#define REASONABLE_NAME_LEN     100


#define ANSIStrStr(p, q) StrStrIA(p, q)
#define ANSIStrChr(p, q) StrChrIA(p, q)

//////////////////////////////////////////////////////////////////
//  GLOBALS:
//////////////////////////////////////////////////////////////////
#ifndef UNIX
TCHAR   * szNetscapeBMRegSub        = TEXT("SOFTWARE\\Netscape\\Netscape Navigator\\Bookmark List");
#else
TCHAR   * szNetscapeBMRegSub        = TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\unix\\nsbookmarks");
#endif

TCHAR   * szNetscapeBMRegKey        = TEXT("File Location");
TCHAR   * szIEFavoritesRegSub       = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
TCHAR   * szIEFavoritesRegKey       = TEXT("Favorites");
char    * szInvalidFolderCharacters = "\\/:*?\"<>|";

BOOL    gfValidNetscapeFile = FALSE;
BOOL    gfValidIEDirFile = FALSE;



#define _FAT_   1
#define _HPFS_  0
#define _NTFS_  0
#define _WILD_  0
#define _OFS_   0
#define _OLE_   0

#define AnsiMaxChar     128                 // The array below only indicates the lower 7 bits of the byte.

static UCHAR LocalLegalAnsiCharacterArray[AnsiMaxChar] = {

    0,                                                // 0x00 ^@
                          _OLE_,  // 0x01 ^A
                          _OLE_,  // 0x02 ^B
                          _OLE_,  // 0x03 ^C
                          _OLE_,  // 0x04 ^D
                          _OLE_,  // 0x05 ^E
                          _OLE_,  // 0x06 ^F
                          _OLE_,  // 0x07 ^G
                          _OLE_,  // 0x08 ^H
                          _OLE_,  // 0x09 ^I
                          _OLE_,  // 0x0A ^J
                          _OLE_,  // 0x0B ^K
                          _OLE_,  // 0x0C ^L
                          _OLE_,  // 0x0D ^M
                          _OLE_,  // 0x0E ^N
                          _OLE_,  // 0x0F ^O
                          _OLE_,  // 0x10 ^P
                          _OLE_,  // 0x11 ^Q
                          _OLE_,  // 0x12 ^R
                          _OLE_,  // 0x13 ^S
                          _OLE_,  // 0x14 ^T
                          _OLE_,  // 0x15 ^U
                          _OLE_,  // 0x16 ^V
                          _OLE_,  // 0x17 ^W
                          _OLE_,  // 0x18 ^X
                          _OLE_,  // 0x19 ^Y
                          _OLE_,  // 0x1A ^Z
                          _OLE_,  // 0x1B ESC
                          _OLE_,  // 0x1C FS
                          _OLE_,  // 0x1D GS
                          _OLE_,  // 0x1E RS
                          _OLE_,  // 0x1F US
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x20 space
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_,          // 0x21 !
                  _WILD_,                 // 0x22 "
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x23 #
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x24 $
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x25 %
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x26 &
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x27 '
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x28 (
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x29 )
                  _WILD_,                 // 0x2A *
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x2B +
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x2C ,
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x2D -
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x2E .
    0,                                                // 0x2F /
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x30 0
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x31 1
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x32 2
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x33 3
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x34 4
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x35 5
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x36 6
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x37 7
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x38 8
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x39 9
             _NTFS_ |         _OFS_,          // 0x3A :
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x3B ;
                  _WILD_,                 // 0x3C <
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x3D =
                  _WILD_,                 // 0x3E >
                  _WILD_,                 // 0x3F ?
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x40 @
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x41 A
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x42 B
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x43 C
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x44 D
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x45 E
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x46 F
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x47 G
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x48 H
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x49 I
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4A J
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4B K
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4C L
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4D M
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4E N
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x4F O
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x50 P
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x51 Q
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x52 R
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x53 S
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x54 T
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x55 U
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x56 V
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x57 W
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x58 X
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x59 Y
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x5A Z
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x5B [
    0,                                                // 0x5C backslash
        _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x5D ]
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x5E ^
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x5F _
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x60 `
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x61 a
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x62 b
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x63 c
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x64 d
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x65 e
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x66 f
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x67 g
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x68 h
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x69 i
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6A j
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6B k
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6C l
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6D m
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6E n
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x6F o
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x70 p
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x71 q
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x72 r
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x73 s
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x74 t
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x75 u
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x76 v
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x77 w
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x78 x
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x79 y
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x7A z
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x7B {
                          _OLE_,  // 0x7C |
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x7D }
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x7E ~
    _FAT_ | _HPFS_ | _NTFS_ |         _OFS_ | _OLE_,  // 0x7F 
};



// Returns the location of the favorites folder in which to import the netscape favorites
BOOL GetTargetFavoritesPath(LPTSTR szPath, UINT cbPath)
{
    if (GetPathFromRegistry(szPath, cbPath, HKEY_CURRENT_USER, szIEFavoritesRegSub, szIEFavoritesRegKey))
    {
        //MLLoadString(IDS_NS_BOOKMARKS_DIR, szSubDir, sizeof(szSubDir))
        //lstrcat(szPath, "\\Imported Netscape Favorites");
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////
//  Import Netscape Bookmarks to Microsoft
//  Internet Explorer's Favorites
///////////////////////////////////////////////////////

/************************************************************\
    FUNCTION: ImportBookmarks

    PARAMETERS:
    HINSTANCE hInstWithStr - Location of String Resources.
    BOOL return - If an error occurs importing the bookmarks, FALSE is returned.

    DESCRIPTION:
    This function will see if it can find a IE Favorite's
    registry entry and a Netscape bookmarks registry entry.  If
    both are found, then the conversion can happen.  It will
    attempt to open the verify that the bookmarks file is
    valid and then convert the entries to favorite entries.
    If an error occures, ImportBookmarks() will return FALSE,
    otherwise it will return TRUE.
\*************************************************************/

BOOL ImportBookmarks(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks, HWND hwnd)
{
    HANDLE  hBookmarksFile        = INVALID_HANDLE_VALUE;
    BOOL    fSuccess              = FALSE;

    // Prompt the user to insert floppy, format floppy or drive, remount mapped partition,
    // or any create sub directories so pszPathToBookmarks becomes valid.
    if (FAILED(SHPathPrepareForWriteWrap(hwnd, NULL, pszPathToBookmarks, FO_COPY, (SHPPFW_DEFAULT | SHPPFW_IGNOREFILENAME))))
        return FALSE;

    if (pszPathToFavorites==NULL || *pszPathToFavorites == TEXT('\0') ||
        pszPathToBookmarks==NULL || *pszPathToBookmarks == TEXT('\0'))
    {
        return FALSE;
    }
    
    hBookmarksFile = CreateFile(pszPathToBookmarks, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    
    if ( hBookmarksFile != INVALID_HANDLE_VALUE ) 
    {
        //
        // Verify it's a valid Bookmarks file
        //
        if (VerifyBookmarksFile( hBookmarksFile ))
        {
            //
            // Do the importing...
            //
            fSuccess = ConvertBookmarks(pszPathToFavorites, hBookmarksFile);

            if (hwnd && !fSuccess)
            {
                MLShellMessageBox(
                    hwnd,
                    MAKEINTRESOURCE(IDS_IMPORTCONVERTERROR),
                    MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV),
                    MB_OK);
            }
        }
        else
        {
            if (hwnd)
            {
                MLShellMessageBox(
                    hwnd,
                    MAKEINTRESOURCE(IDS_NOTVALIDBOOKMARKS),
                    MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV),
                    MB_OK);
            }
        }
        CloseHandle(hBookmarksFile);
    }
    else
    {
        if (hwnd)
        {
            MLShellMessageBox(
                hwnd,
                MAKEINTRESOURCE(IDS_COULDNTOPENBOOKMARKS),
                MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV),
                MB_OK);
        }
    }
    return(fSuccess);
}


/************************************************************\
    FUNCTION: ConvertBookmarks

    PARAMETERS:
    char * szFavoritesDir - String containing the path to
            the IE Favorites directory
    BOOL return - If an error occurs importing the bookmarks, FALSE is returned.

    DESCRIPTION:
    This function will continue in a loop converting each
    entry in the bookmark file.  There are three types of
    entries in the bookmark file, 1) a bookmark, 2) start of
    new level in heirarchy, 3) end of current level in heirarchy.
    The function NextFileEntry() will return these values until
    the file is empty, at which point, this function will end.

    NOTE:
    In order to prevent an infinite loop, it's assumed
    that NextFileEntry() will eventually return ET_NONE or ET_ERROR.
\************************************************************/

BOOL ConvertBookmarks(TCHAR * szFavoritesDir, HANDLE hFile)
{
    BOOL    fDone       = FALSE;
    BOOL    fSuccess    = TRUE;
    BOOL    fIsEmpty    = TRUE;
    char    * szData    = NULL;
    char    * szCurrent = NULL;
    char    * szToken   = NULL;

    fSuccess = GetData(&szData, hFile);
    if (NULL == szData)
        fSuccess = FALSE;

    szCurrent = szData;

    // Verify directory exists or that we can make it.
    if ((TRUE == fSuccess) && ( !SetCurrentDirectory(szFavoritesDir)))
    {
        // If the directory doesn't exist, make it...
        if ( !CreateDirectory(szFavoritesDir, NULL))
            fSuccess = FALSE;
        else
            if (!SetCurrentDirectory(szFavoritesDir))
                fSuccess = FALSE;
    }

   
    while ((FALSE == fDone) && (TRUE == fSuccess))
    {
        switch(NextFileEntry(&szCurrent, &szToken))
        {
            case ET_OPEN_DIR:
                fSuccess = CreateDir(szToken);
                break;
            case ET_CLOSE_DIR:
                fSuccess = CloseDir();
                break;
            case ET_BOOKMARK:
                fSuccess = CreateBookmark(szToken);
                fIsEmpty = FALSE;
                break;
            case ET_ERROR:
                fSuccess = FALSE;
                break;
            case ET_NONE:            
            default:
                fDone = TRUE;
                break;
        }
    }

    if ( fIsEmpty )
    {
        // nothing to import, delete the dir created earlier
        RemoveDirectory(szFavoritesDir);
    }

    if (NULL != szData)
    {
        LocalFree(szData);
        szData = NULL;
        szCurrent = NULL;       // szCurrent no longer points to valid data.
        szToken = NULL;     // szCurrent no longer points to valid data.
    }

    return(fSuccess);
}

/************************************************************\
    FUNCTION: NextFileEntry

    PARAMETERS:
    char ** ppStr   - The data to parse.
    char ** ppToken - The token pointer.
    EntryType return- See below.

    DESCRIPTION:
    This function will look for the next entry in the
    bookmark file to create or act on.  The return value
    will indicate this response:
    ET_OPEN_DIR             Create a new level in heirarchy
    ET_CLOSE_DIR,           Close level in heirarchy
    ET_BOOKMARK,            Create Bookmark entry.
    ET_NONE,                End of File
    ET_ERROR                Error encountered

    Errors will be detected by finding the start of a token,
    but in not finding other parts of the token that are needed
    to parse the data.
\************************************************************/

MyEntryType NextFileEntry(char ** ppStr, char ** ppToken)
{
    MyEntryType   returnVal       = ET_NONE;
    char *      pCurrentToken   = NULL;         // The current token to check if valid.
    char *      pTheToken       = NULL;         // The next valid token.
    char *      pszTemp         = NULL;
#ifdef UNIX
    char        szMidDirToken[8];
#endif

    //ASSERTSZ(NULL != ppStr, "It's an error to pass NULL for ppStr");
    //ASSERTSZ(NULL != *ppStr, "It's an error to pass NULL for *ppStr");
    //ASSERTSZ(NULL != ppToken, "It's an error to pass NULL for ppToken");

    if ((NULL != ppStr) && (NULL != *ppStr) && (NULL != ppToken))
    {
        // Check for begin dir token
        if (NULL != (pCurrentToken = ANSIStrStr(*ppStr, BEGIN_DIR_TOKEN)))
        {
            // Begin dir token found
            // Verify that other needed tokens exist or it's an error
#ifndef UNIX
            if ((NULL == (pszTemp = ANSIStrStr(pCurrentToken, MID_DIR_TOKEN))) ||
#else
	    if (pCurrentToken[7] == ' ')
	        StrCpyNA(szMidDirToken, MID_DIR_TOKEN, lstrlenA(MID_DIR_TOKEN) + 1);
	    else
	        StrCpyNA(szMidDirToken, MID_DIR_TOKEN0, lstrlenA(MID_DIR_TOKEN0) + 1);
            if ((NULL == (pszTemp = ANSIStrStr(pCurrentToken, szMidDirToken))) ||
#endif
                (NULL == ANSIStrStr(pszTemp, END_DIR_TOKEN)))
            {
                returnVal = ET_ERROR;       // We can't find all the tokens needed.
            }
            else
            {
                // This function has to set *ppToken to the name of the directory to create
#ifndef UNIX
                *ppToken =  ANSIStrStr(pCurrentToken, MID_DIR_TOKEN) + sizeof(MID_DIR_TOKEN)-1;
#else
                *ppToken =  ANSIStrStr(pCurrentToken, szMidDirToken) + lstrlenA(szMidDirToken);
#endif
                pTheToken = pCurrentToken;
                returnVal = ET_OPEN_DIR;
            }
        }
        // Check for exit dir token
        if ((ET_ERROR != returnVal) &&
            (NULL != (pCurrentToken = ANSIStrStr(*ppStr, BEGIN_EXITDIR_TOKEN))))
        {
            // Exit dir token found
            // See if this token comes before TheToken.
            if ((NULL == pTheToken) || (pCurrentToken < pTheToken))
            {
                // ppToken is not used for Exit Dir
                *ppToken = NULL;
                pTheToken = pCurrentToken;
                returnVal = ET_CLOSE_DIR;
            }
        }
        // Check for begin url token
        if ((ET_ERROR != returnVal) &&
            (NULL != (pCurrentToken = ANSIStrStr(*ppStr, BEGIN_URL_TOKEN))))
        {
            // Bookmark token found
            // Verify that other needed tokens exist or it's an error
#ifndef UNIX
            if ((NULL == (pszTemp = ANSIStrStr(pCurrentToken, END_URL_TOKEN))) ||
#else
            if (((NULL == (pszTemp = ANSIStrStr(pCurrentToken, END_URL_TOKEN))) && 
		 (NULL == (pszTemp = ANSIStrStr(pCurrentToken, END_URL_TOKEN2)))) ||
#endif
                (NULL == (pszTemp = ANSIStrStr(pszTemp, BEGIN_BOOKMARK_TOKEN))) ||
                (NULL == ANSIStrStr(pszTemp, END_BOOKMARK_TOKEN)))
            {
                returnVal = ET_ERROR;       // We can't find all the tokens needed.
            }
            else
            {
                // See if this token comes before TheToken.
                if ((NULL == pTheToken) || (pCurrentToken < pTheToken))
                {
                    // This function has to set *ppToken to the name of the bookmark
                    *ppToken =  pCurrentToken + sizeof(BEGIN_URL_TOKEN)-1;
                    pTheToken = pCurrentToken;
                    returnVal = ET_BOOKMARK;
                }
            }
        }
    }
    else
        returnVal = ET_ERROR;               // We should never get here.

    if (NULL == pTheToken)
        returnVal = ET_NONE;
    else
    {
        // Next time we will start parsing where we left off.
        switch(returnVal)
        {
            case ET_OPEN_DIR:
#ifndef UNIX
                *ppStr = ANSIStrStr(pTheToken, MID_DIR_TOKEN) + sizeof(MID_DIR_TOKEN);
#else
                *ppStr = ANSIStrStr(pTheToken, szMidDirToken) + lstrlenA(szMidDirToken) + 1;
#endif
                break;
            case ET_CLOSE_DIR:
                *ppStr = pTheToken + sizeof(BEGIN_EXITDIR_TOKEN);
                break;
            case ET_BOOKMARK:
                *ppStr = ANSIStrStr(pTheToken, END_BOOKMARK_TOKEN) + sizeof(END_BOOKMARK_TOKEN);
                break;
            default:
                break;
    }
    }

    return(returnVal);
}


/************************************************************\
    FUNCTION: GetPathFromRegistry

    PARAMETERS:
    LPSTR szPath    - The value found in the registry. (Result of function)
    UINT cbPath     - Size of szPath.
    HKEY theHKEY    - The HKEY to look into (HKEY_CURRENT_USER)
    LPSTR szKey     - Path in Registry (Software\...\Explore\Shell Folders)
    LPSTR szVName   - Value to query (Favorites)
    BOOL return     - TRUE if succeeded, FALSE if Error.
    EXAMPLE:
    HKEY_CURRENT_USER\Software\Microsoft\CurrentVersion\Explore\Shell Folders
    Favorites = "C:\WINDOWS\Favorites"

    DESCRIPTION:
    This function will look in the registry for the value
    to look up.  The caller specifies the HKEY, subkey (szKey),
    value to query (szVName).  The caller also sets a side memory
    for the result and passes a pointer to that memory in szPath
    with it's size in cbPath.  The BOOL return value will indicate
    success or failure of this function.
\************************************************************/

BOOL GetPathFromRegistry(LPTSTR szPath, UINT cbPath, HKEY theHKEY,
                LPTSTR szKey, LPTSTR szVName)
{
    DWORD   dwType;
    DWORD   dwSize;

    /*
     * Get Path to program
     *      from the registry
     */
    dwSize = cbPath;
    return (ERROR_SUCCESS == SHGetValue(theHKEY, szKey, szVName, &dwType, (LPBYTE) szPath, &dwSize)
            && (dwType == REG_EXPAND_SZ || dwType == REG_SZ));
}


/************************************************************\
    FUNCTION: RemoveInvalidFileNameChars

    PARAMETERS:
    char * pBuf     - The data to search.

    DESCRIPTION:
    This function will search pBuf until it encounters
    a character that is not allowed in a file name.  It will
    then replace that character with a SPACE and continue looking
    for more invalid chars until they have all been removed.
\************************************************************/

void RemoveInvalidFileNameChars(char * pBuf)
{
    //ASSERTSZ(NULL != pBuf, "Invalid function parameter");

    // Go through the array of chars, replacing offending characters with a space
    if (NULL != pBuf)
    {
    if (REASONABLE_NAME_LEN < strlen(pBuf))
        pBuf[REASONABLE_NAME_LEN] = '\0';   // String too long. Terminate it.

    while ('\0' != *pBuf)
    {
        // Check if the character is invalid
        if (!IsDBCSLeadByte(*pBuf))
        {
        if  (ANSIStrChr(szInvalidFolderCharacters, *pBuf) != NULL)
            *pBuf = '_';
        }
#if 0
// Old code
        // We look in the array to see if the character is supported by FAT.
        // The array only includes the first 128 values, so we need to fail
        // on the other 128 values that have the high bit set.
        if (((AnsiMaxChar <= *pBuf) && (FALSE == IsDBCSLeadByte(*pBuf))) ||
        (0 == LocalLegalAnsiCharacterArray[*pBuf]))
        *pBuf = '$';
#endif
        pBuf = CharNextA(pBuf);
    }
    }
}



/************************************************************\
    FUNCTION: CreateBookmark

    PARAMETERS:
    char * pBookmarkName- This is a pointer that contains
              the name of the bookmark to create.
              Note that it is not NULL terminated.
    BOOL return     - Return TRUE if successful.

    DESCRIPTION:
    This function will take the data that is passed to
    it and extract the name of the bookmark and it's value to create.
    If the name is too long, it will be truncated.  Then,
    the directory will be created.  Any errors encountered
    will cause the function to return FALSE to indicate
    failure.
\************************************************************/

BOOL CreateBookmark(char *pBookmarkName)
{
    BOOL    fSuccess                = FALSE;
    char    szNameOfBM[REASONABLE_NAME_LEN];
    char    szURL[MAX_URL];
    char    * pstrEndOfStr          = NULL;
    char    * pstrBeginOfName       = NULL;
    long    lStrLen                 = 0;
    HANDLE  hFile                   = NULL;
    DWORD   dwSize;
    char    szBuf[MAX_URL];

    //ASSERTSZ(NULL != pBookmarkName, "Bad input parameter");
    if (NULL != pBookmarkName)
    {

    pstrEndOfStr = ANSIStrStr(pBookmarkName, END_URL_TOKEN);
#ifdef UNIX
    if (!pstrEndOfStr)
        pstrEndOfStr = ANSIStrStr(pBookmarkName, END_URL_TOKEN2);
#endif
    if (NULL != pstrEndOfStr)
    {
        lStrLen = (int) (pstrEndOfStr-pBookmarkName);
        if (MAX_URL < lStrLen)
        lStrLen = MAX_URL-1;

        // Create the name of the Bookmark
        StrCpyNA(szURL, pBookmarkName, ARRAYSIZE(szURL));
        szURL[lStrLen] = '\0';

        // filter out file links, we won't create a bookmark to a file link
        // but remove the link silently and continue
        if (IsFileUrl(szURL))
            return TRUE;

        pstrBeginOfName = ANSIStrStr(pstrEndOfStr, BEGIN_BOOKMARK_TOKEN);
        if (NULL != pstrBeginOfName)
        {
        pstrBeginOfName += sizeof(BEGIN_BOOKMARK_TOKEN) - 1;            // Start at beginning of Name

        pstrEndOfStr = ANSIStrStr(pstrBeginOfName, END_BOOKMARK_TOKEN); // Find end of name
        if (NULL != pstrEndOfStr)
        {
            lStrLen = (int) (pstrEndOfStr-pstrBeginOfName);
            if (REASONABLE_NAME_LEN-FILE_EXT < lStrLen)
            lStrLen = REASONABLE_NAME_LEN-FILE_EXT-1;

            // Generate the URL
            StrCpyNA(szNameOfBM, pstrBeginOfName, lStrLen+1);
            //szNameOfBM[lStrLen] = '\0';
            StrCatBuffA(szNameOfBM, ".url", ARRAYSIZE(szNameOfBM));
            RemoveInvalidFileNameChars(szNameOfBM);

            // Check to see if Favorite w/same name exists
            if (INVALID_HANDLE_VALUE != (hFile = CreateFileA(szNameOfBM, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                                 CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL )))
            {
                WriteFile(hFile, "[InternetShortcut]\n", lstrlenA( "[InternetShortcut]\n" ), &dwSize, NULL);
                wnsprintfA( szBuf, ARRAYSIZE(szBuf), "URL=%s\n", szURL);
                WriteFile(hFile, szBuf, lstrlenA(szBuf), &dwSize, NULL );
                fSuccess = TRUE;
            }
            else
            {
                fSuccess = TRUE;
            }

            if (NULL != hFile)
            {
                CloseHandle( hFile );
                hFile = NULL;
            }

        }
        }
    }
    }

    return(fSuccess);
}


/************************************************************\
    FUNCTION: CreateDir

    PARAMETERS:
    char * pDirName - This is a pointer that contains
              the name of the directory to create.
              Note that it is not NULL terminated.
    BOOL return     - Return TRUE if successful.

    DESCRIPTION:
    This function will take the data that is passed to
    it and extract the name of the directory to create.
    If the name is too long, it will be truncated.  Then,
    the directory will be created.  Any errors encountered
    will cause the function to return FALSE to indicate
    failure.
\************************************************************/
BOOL CreateDir(char *pDirName)
{
    BOOL    fSuccess                = FALSE;
    char    szNameOfDir[REASONABLE_NAME_LEN];
    char    * pstrEndOfName         = NULL;
    long    lStrLen                 = 0;

    //ASSERTSZ(NULL != pDirName, "Bad input parameter");
    if (NULL != pDirName)
    {
        pstrEndOfName = ANSIStrStr(pDirName, END_DIR_TOKEN);
        if (NULL != pstrEndOfName)
        {
            lStrLen = (int) (pstrEndOfName-pDirName);
            if (REASONABLE_NAME_LEN < lStrLen)
            lStrLen = REASONABLE_NAME_LEN-1;

            StrCpyNA(szNameOfDir, pDirName, lStrLen+1);
            //szNameOfDir[lStrLen] = '\0';
            RemoveInvalidFileNameChars(szNameOfDir);

            // BUGBUG : Try to CD into existing dir first
            if ( !SetCurrentDirectoryA(szNameOfDir) )
            {
                if ( CreateDirectoryA(szNameOfDir, NULL) )
                {
                    if ( SetCurrentDirectoryA(szNameOfDir) )
                    {
                        fSuccess = TRUE;// It didn't exist, but now it does.
                    }
                }
            }
            else
                fSuccess = TRUE;        // It exists already.
        }
    }

    return(fSuccess);
}


/************************************************************\
    FUNCTION: CloseDir

    PARAMETERS:
    BOOL return     - Return TRUE if successful.

    DESCRIPTION:
    This function will back out of the current directory.
\************************************************************/
BOOL CloseDir(void)
{
    return( SetCurrentDirectoryA("..") );
}


/************************************************************\
    FUNCTION: VerifyBookmarksFile

    PARAMETERS:
    FILE * pFile    - Pointer to Netscape Bookmarks file.
    BOOL return     - TRUE if No Error and Valid Bookmark file

    DESCRIPTION:
    This function needs to be passed with a valid pointer
    that points to an open file.  Upon return, the file will
    still be open and is guarenteed to have the file pointer
    point to the beginning of the file.
    This function will return TRUE if the file contains
    text that indicates it's a valid Netscape bookmarks file.
\************************************************************/

BOOL VerifyBookmarksFile(HANDLE hFile)
{
    BOOL    fSuccess            = FALSE;
    char    szFileHeader[sizeof(VALIDATION_STR)+1] = "";
    DWORD   dwSize;

    //ASSERTSZ(NULL != pFile, "You can't pass me a NULL File Pointer");
    if (INVALID_HANDLE_VALUE == hFile)
        return(FALSE);

    // Reading the first part of the file.  If the file isn't this long, then
    // it can't possibly be a Bookmarks file.    
    if ( ReadFile( hFile, szFileHeader, sizeof(VALIDATION_STR)-1, &dwSize, NULL ) && (dwSize == sizeof(VALIDATION_STR)-1) )
    {
#ifndef UNIX
        szFileHeader[sizeof(VALIDATION_STR)] = '\0';            // Terminate String.
#else
        // BUGBUG : The above statement doesn;t serve the purpose on UNIX.
        // I think we should change for NT also.
        // IEUNIX : NULL character after the buffer read
        szFileHeader[sizeof(VALIDATION_STR)-1] = '\0';          // Terminate String.
#endif

        if (0 == StrCmpA(szFileHeader, VALIDATION_STR))          // See if header is the same as the Validation string.
            fSuccess = TRUE;
    }

    // Reset the point to point to the beginning of the file.
    dwSize = SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
    if ( dwSize == 0xFFFFFFFF )
         fSuccess = FALSE;

    return(fSuccess);
}



/************************************************************\
    FUNCTION: GetData

    PARAMETERS:
    char ** ppData  - Where to put the data
    FILE * pFile    - Pointer to Netscape Bookmarks file.
    BOOL return     - Return TRUE is successful.

    DESCRIPTION:
    This function will find the size of the bookmarks file,
    malloc that much memory, and put the file's contents in
    that buffer.  ppData will be invalid when the function
    is called and will return with malloced memory that
    needs to be freed by the falling function.
\************************************************************/

BOOL GetData(char ** ppData, HANDLE hFile)
{
    DWORD  dwlength, dwRead;
    BOOL   fSuccess = FALSE;

    //ASSERTSZ(NULL != ppData, "Invalid input parameter");

    if (NULL != ppData)
    {
        *ppData = NULL;

        // Find the size of the data
        if ( dwlength = GetFileSize(hFile, NULL))
        {
            *ppData = (PSTR)LocalAlloc(LPTR, dwlength+1 );
            if (NULL != *ppData)
            {                
                if ( ReadFile( hFile, *ppData, dwlength+1, &dwRead, NULL ) &&
                     ( dwlength == dwRead ) )
                {
                    fSuccess = TRUE;
                }

                (*ppData)[dwlength] = '\0';
            }
        }
    }

    return(fSuccess);
}

//
// AddPath - added by julianj when porting from setup code to stand alone
//
void PASCAL AddPath(LPTSTR pszPath, LPCTSTR pszName, int cchPath )
{
    LPTSTR pszTmp;
    int    cchTmp;

    // Find end of the string
    cchTmp = lstrlen(pszPath);
    pszTmp = pszPath + cchTmp;
    cchTmp = cchPath - cchTmp;

        // If no trailing backslash then add one
    if ( pszTmp > pszPath && *(CharPrev( pszPath, pszTmp )) != FILENAME_SEPARATOR )
    {
        *(pszTmp++) = FILENAME_SEPARATOR;
        cchTmp--;
    }

        // Add new name to existing path string
    while ( *pszName == TEXT(' ') ) pszName++;
    StrCpyN( pszTmp, pszName, cchTmp );
}

//
// GetVersionFromFile - added by julianj when porting from setup code to stand alone
//
BOOL GetVersionFromFile(PTSTR pszFileName, PDWORD pdwMSVer, PDWORD pdwLSVer)
{
    DWORD dwVerInfoSize, dwHandle;
    LPVOID lpVerInfo;
    VS_FIXEDFILEINFO *pvsVSFixedFileInfo;
    UINT uSize;

    HRESULT hr = E_FAIL;

    *pdwMSVer = *pdwLSVer = 0;

    if ((dwVerInfoSize = GetFileVersionInfoSize(pszFileName, &dwHandle)))
    {
        if ((lpVerInfo = (LPVOID) LocalAlloc(LPTR, dwVerInfoSize)) != NULL)
        {
            if (GetFileVersionInfo(pszFileName, dwHandle, dwVerInfoSize, lpVerInfo))
            {
                if (VerQueryValue(lpVerInfo, TEXT("\\"), (LPVOID *) &pvsVSFixedFileInfo, &uSize))
                {
                    *pdwMSVer = pvsVSFixedFileInfo->dwFileVersionMS;
                    *pdwLSVer = pvsVSFixedFileInfo->dwFileVersionLS;
                    hr = S_OK;
                }
            }
            LocalFree(lpVerInfo);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

BOOL GetNavBkMkDir( LPTSTR lpszDir, int isize)
{
    BOOL    bDirFound = FALSE;
#ifndef UNIX
    TCHAR   szDir[MAX_PATH];
    HKEY    hKey;
    HKEY    hKeyUser;
    TCHAR   szUser[MAX_PATH];
    DWORD   dwSize;

    StrCpyN( szUser, REGSTR_PATH_APPPATHS, ARRAYSIZE(szUser) );
    AddPath( szUser, TEXT("NetScape.exe"), ARRAYSIZE(szUser) );
    if ( GetPathFromRegistry( szDir, MAX_PATH, HKEY_LOCAL_MACHINE, szUser, TEXT("") ) &&
         lstrlen(szDir) )
    {
        DWORD dwMV, dwLV;

        if ( SUCCEEDED(GetVersionFromFile( szDir, &dwMV, &dwLV )) )
        {
            if ( dwMV < 0x00040000 )
                bDirFound = GetPathFromRegistry( lpszDir, isize, HKEY_CURRENT_USER,
                                     szNetscapeBMRegSub, szNetscapeBMRegKey);
            else
            {
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Netscape\\Netscape Navigator\\Users"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szUser);
                    if (RegQueryValueEx(hKey, TEXT("CurrentUser"), NULL, NULL, (LPBYTE)szUser, &dwSize) == ERROR_SUCCESS)
                    {
                        if (RegOpenKeyEx(hKey, szUser, 0, KEY_READ, &hKeyUser) == ERROR_SUCCESS)
                        {
                            dwSize = sizeof(szDir);
                            if (RegQueryValueEx(hKeyUser, TEXT("DirRoot"), NULL, NULL, (LPBYTE)szDir, &dwSize) == ERROR_SUCCESS)
                            {
                                // Found the directory for the current user.
                                StrCpyN( lpszDir, szDir, isize);
                                AddPath( lpszDir, TEXT("bookmark.htm"), isize );
                                bDirFound = TRUE;
                            }
                            RegCloseKey(hKeyUser);
                        }
                    }
                    RegCloseKey(hKey);
                }
            }
        }
    }
    else
#endif
        bDirFound = GetPathFromRegistry( lpszDir, isize, HKEY_CURRENT_USER,
                                         szNetscapeBMRegSub, szNetscapeBMRegKey);
 
    return bDirFound;
}


//
// *** EXPORT FAVORITES CODE ***
//

// REVIEW REMOVE THESE
#include <windows.h>
//#include <stdio.h>
#include <shlobj.h>
#include <shlwapi.h>

//
// Generate HTML from favorites
//

#define INDENT_AMOUNT 4

int Indent = 0;

HANDLE g_hOutputStream = INVALID_HANDLE_VALUE;
 
void Output(const char *format, ...)
{
    DWORD dwSize;
    char buf[MAX_URL];

    va_list argptr;

    va_start(argptr, format);

    for (int i=0; i<Indent*INDENT_AMOUNT; i++)
    {
        WriteFile(g_hOutputStream, " ", 1, &dwSize, NULL);
    }

    wvnsprintfA(buf, ARRAYSIZE(buf), format, argptr);
    WriteFile(g_hOutputStream, buf, lstrlenA(buf), &dwSize, NULL);
}

void OutputLn(const char *format, ...)
{
    DWORD dwSize;
    char buf[MAX_URL];

    va_list argptr;

    va_start(argptr, format);

    for (int i=0; i<Indent*INDENT_AMOUNT; i++)
    {
        WriteFile(g_hOutputStream, " ", 1, &dwSize, NULL);
    }

    wvnsprintfA(buf, ARRAYSIZE(buf), format, argptr);
    WriteFile(g_hOutputStream, buf, lstrlenA(buf), &dwSize, NULL);
    WriteFile(g_hOutputStream, "\r\n", 2, &dwSize, NULL);
}

#define CREATION_TIME 0
#define ACCESS_TIME   1
#define MODIFY_TIME   2

//
// This nasty looking macro converts a FILETIME structure
// (100-nanosecond intervals since Jan 1st 1601) to a
// unix time_t value (seconds since Jan 1st 1970).
//
// The numbers come from knowledgebase article Q167296
//
#define FILETIME_TO_UNIXTIME(ft) (UINT)((*(LONGLONG*)&ft-116444736000000000)/10000000)

UINT GetUnixFileTime(LPTSTR pszFileName, int mode)
{

    WIN32_FIND_DATA wfd;
    HANDLE hFind;

    hFind = FindFirstFile(pszFileName,&wfd);

    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    FindClose(hFind);

    switch (mode)
    {

    case CREATION_TIME:
        return FILETIME_TO_UNIXTIME(wfd.ftCreationTime);

    case ACCESS_TIME:
        return FILETIME_TO_UNIXTIME(wfd.ftLastAccessTime);

    case MODIFY_TIME:
        return FILETIME_TO_UNIXTIME(wfd.ftLastWriteTime);

    default:
        ASSERT(0);
        return 0;
        
    }
    
}

void WalkTree(TCHAR * szDir)
{
    WIN32_FIND_DATA findFileData;
    TCHAR buf[MAX_PATH];
    HANDLE hFind;

    Indent++;

    //
    // First iterate through all directories
    //
    wnsprintf(buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("*"), szDir);
    hFind = FindFirstFile(buf, &findFileData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if ((StrCmp(findFileData.cFileName, TEXT(".")) != 0  &&
                     StrCmp(findFileData.cFileName, TEXT("..")) != 0 &&
                     StrCmp(findFileData.cFileName, TEXT("History")) != 0 && // REVIEW just for JJ. Should check for system bit on folders
                     StrCmp(findFileData.cFileName, TEXT("Software Updates")) != 0 && // don't export software updates
                     StrCmp(findFileData.cFileName, TEXT("Channels")) != 0))         // don't export channels for now!
                {
                    char thisFile[MAX_PATH];
                    wnsprintf(buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("%s"), szDir, findFileData.cFileName);

                    if (!(GetFileAttributes(buf)&FILE_ATTRIBUTE_SYSTEM))
                    {
                        SHTCharToAnsi(findFileData.cFileName, thisFile, MAX_PATH);
                        OutputLn("<DT><H3 FOLDED ADD_DATE=\"%u\">%s</H3>", GetUnixFileTime(buf,CREATION_TIME), thisFile);
                        OutputLn("<DL><p>");
                        WalkTree(buf);
                        OutputLn(BEGIN_EXITDIR_TOKEN);
                    }

                }
                else
                {
                    ; // ignore . and ..
                }
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
    }

    //
    // Next iterate through all files
    //
    wnsprintf(buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("*"), szDir);
    hFind = FindFirstFile(buf, &findFileData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                wnsprintf(buf, ARRAYSIZE(buf), TEXT("%s") TEXT(FILENAME_SEPARATOR_STR) TEXT("%s"), szDir, findFileData.cFileName);

                //
                // Read the url from the .url file
                //
                TCHAR szUrl[MAX_PATH];

                SHGetIniString(
                    TEXT("InternetShortcut"),
                    TEXT("URL"),
                    szUrl,       // returns url
                    MAX_PATH,
                    buf);        // full path to .url file

                if (*szUrl != 0)
                {
                    //
                    // create a copy of the filename without the extension
                    // note PathFindExtension returns a ptr to the NULL at 
                    // end if '.' not found so its ok to just blast *pch with 0
                    //
                    TCHAR szFileName[MAX_PATH];
                    StrCpyN(szFileName, findFileData.cFileName, ARRAYSIZE(szFileName));
                    TCHAR *pch = PathFindExtension(szFileName);
                    *pch = TEXT('\0'); // 
                    char  szUrlAnsi[MAX_PATH], szFileNameAnsi[MAX_PATH];
                    SHTCharToAnsi(szUrl, szUrlAnsi, MAX_PATH);
                    SHTCharToAnsi(szFileName, szFileNameAnsi, MAX_PATH);
                    OutputLn("<DT><A HREF=\"%s\" ADD_DATE=\"%u\" LAST_VISIT=\"%u\" LAST_MODIFIED=\"%u\">%s</A>", 
						szUrlAnsi, 
						GetUnixFileTime(buf,CREATION_TIME),
						GetUnixFileTime(buf,ACCESS_TIME),
						GetUnixFileTime(buf,MODIFY_TIME),
						szFileNameAnsi);
                }
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
    }

    Indent--;
}

BOOL ExportFavorites(TCHAR * pszPathToFavorites, TCHAR * pszPathToBookmarks, HWND hwnd)
{
    // Prompt the user to insert floppy, format floppy or drive, remount mapped partition,
    // or any create sub directories so pszPathToBookmarks becomes valid.
    if (FAILED(SHPathPrepareForWriteWrap(hwnd, NULL, pszPathToBookmarks, FO_COPY, (SHPPFW_DEFAULT | SHPPFW_IGNOREFILENAME))))
        return FALSE;

    // Open output file REVIEW redo to use Win32 file apis
    g_hOutputStream = CreateFile(
        pszPathToBookmarks,
        GENERIC_WRITE,
        0, // no sharing,
        NULL, // no security attribs
        CREATE_ALWAYS, // overwrite if present
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (g_hOutputStream == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Output bookmark file header stuff
    //
    Output(VALIDATION_STR);
    OutputLn("1>");
    OutputLn(COMMENT_STR);
    OutputLn(TITLE); // REVIEW put/persist users name in Title???

    //
    // Do the walk
    //
    OutputLn("<DL><p>");
    WalkTree(pszPathToFavorites);
    OutputLn(BEGIN_EXITDIR_TOKEN);

    //
    // Close output file handle
    //
    CloseHandle(g_hOutputStream); // REVIEW

    return TRUE;
}

//
// Import/Export User interface dialog routines
//

//
// Standalone app for importing the Netscape Favorites into IE.
//
// julianj 3/9/98
//

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#ifdef _WIN32_WINDOWS
#undef _WIN32_WINDOWS
#endif
#ifdef WINVER
#undef WINVER
#endif
#define _WIN32_WINDOWS      0x0400
#define _WIN32_WINNT        0x0400
#define WINVER              0x0400



TCHAR g_szPathToFavorites[MAX_PATH+1];
TCHAR g_szPathToBookmarks[MAX_PATH+1];
LPITEMIDLIST g_pidlFavorites = NULL;

enum DIALOG_TYPE {FILE_OPEN_DIALOG, FILE_SAVE_DIALOG};

BOOL BrowseForBookmarks(TCHAR *pszPathToBookmarks, int cchPathToBookmarks, HWND hwnd, DIALOG_TYPE dialogType)
{
    TCHAR szFile[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR *pszFileName = PathFindFileName(pszPathToBookmarks);
    TCHAR szDialogTitle[MAX_PATH];
    
    //
    // Now copy the filename into the buffer for use with OpenFile
    // and then copy szDir from path to bookmarks and truncate it at filename 
    // so it contains the initial working directory for the dialog
    //
    StrCpyN(szFile, pszFileName, ARRAYSIZE(szFile));
    StrCpyN(szDir,  pszPathToBookmarks, ARRAYSIZE(szDir));
    szDir[pszFileName-pszPathToBookmarks] = TEXT('\0');

    //
    // Use common dialog code to get path to folder
    //
    TCHAR filter[] = TEXT("HTML File\0*.HTM\0All Files\0*.*\0");
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = HINST_THISDLL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = szDir;
    ofn.lpstrDefExt = TEXT("htm");

    if (dialogType == FILE_SAVE_DIALOG)
    {
        MLLoadString(IDS_EXPORTDIALOGTITLE, szDialogTitle, ARRAYSIZE(szDialogTitle));

        ofn.lpstrTitle = szDialogTitle;
        ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        if (GetSaveFileName(&ofn))
        {
            StrCpyN(pszPathToBookmarks, szFile, cchPathToBookmarks);
            return TRUE;
        }
    }
    else
    {
        MLLoadString(IDS_IMPORTDIALOGTITLE, szDialogTitle, ARRAYSIZE(szDialogTitle));

        ofn.lpstrTitle = szDialogTitle;
        ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileName(&ofn))
        {
            StrCpyN(pszPathToBookmarks, szFile, cchPathToBookmarks);
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT CreateILFromPath(LPCTSTR pszPath, LPITEMIDLIST* ppidl)
{
    // ASSERT(pszPath);
    // ASSERT(ppidl);

    HRESULT hr;

    IShellFolder* pIShellFolder;

    hr = SHGetDesktopFolder(&pIShellFolder);

    if (SUCCEEDED(hr))
    {
        // ASSERT(pIShellFolder);

        WCHAR wszPath[MAX_PATH];

        if (SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath)))
        {
            ULONG ucch;

            hr = pIShellFolder->ParseDisplayName(NULL, NULL, wszPath, &ucch,
                                                 ppidl, NULL);
        }
        else
        {
            hr = E_FAIL;
        }
        pIShellFolder->Release();
    }
    return hr;
}

int CALLBACK BrowseForFavoritesCallBack(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        SendMessage(hwnd, BFFM_SETSELECTIONA, (WPARAM)TRUE, lpData);
        break;

    default:
        break;
    }
    return 0;
}


void BrowseForFavorites(char *pszPathToFavorites, HWND hwnd)
{
    //
    // Use SHBrowseForFolder to get path to folder
    //
    char szDisplayName[MAX_PATH];
    char szTitle[MAX_PATH];

    MLLoadStringA(IDS_IMPORTEXPORTTITLE, szTitle, ARRAYSIZE(szTitle));

    BROWSEINFOA bi = {0};
    bi.hwndOwner = hwnd;
    bi.pidlRoot = g_pidlFavorites;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle = szTitle;
    // bi.ulFlags = BIF_EDITBOX; // do we want this?
    bi.ulFlags = BIF_USENEWUI;
    bi.lpfn = BrowseForFavoritesCallBack;
    bi.lParam = (LPARAM)pszPathToFavorites;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

    if (pidl)
    {
        SHGetPathFromIDListA(pidl, pszPathToFavorites);
        ILFree(pidl);
    };
}

#define REG_STR_IMPEXP          TEXT("Software\\Microsoft\\Internet Explorer\\Main")
#define REG_STR_PATHTOFAVORITES TEXT("FavoritesImportFolder")
#define REG_STR_PATHTOBOOKMARKS TEXT("FavoritesExportFile")
#define REG_STR_DESKTOP         TEXT("Desktop")
#define REG_STR_SHELLFOLDERS    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")

#ifndef UNIX
#define STR_BOOKMARK_FILE       TEXT("\\bookmark.htm")
#else
#define STR_BOOKMARK_FILE       TEXT("/bookmark.html")
#endif

//
// InitializePaths
//
void InitializePaths()
{
    //
    // Read the Netscape users bookmark file location and the
    // current users favorite path from registry
    //
    if (!GetNavBkMkDir(g_szPathToBookmarks, MAX_PATH))
    {
        //
        // If Nav isn't installed then use the desktop
        //
        GetPathFromRegistry(g_szPathToBookmarks, MAX_PATH, HKEY_CURRENT_USER,
            REG_STR_SHELLFOLDERS, REG_STR_DESKTOP);
        StrCatBuff(g_szPathToBookmarks, STR_BOOKMARK_FILE, ARRAYSIZE(g_szPathToBookmarks));
    }

    GetTargetFavoritesPath(g_szPathToFavorites, MAX_PATH);
    
    if (FAILED(CreateILFromPath(g_szPathToFavorites, &g_pidlFavorites)))
        g_pidlFavorites = NULL;

    //
    // Now override these values with values stored in the registry just for
    // this tool, so if the user consistently wants to save their favorites
    // out to a separate .HTM file its easy to do
    //
    HKEY hKey;
    DWORD dwSize;
    DWORD dwType;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_STR_IMPEXP, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = MAX_PATH;
        dwType = REG_SZ;
        RegQueryValueEx(hKey, REG_STR_PATHTOBOOKMARKS, 0, &dwType, (LPBYTE)g_szPathToBookmarks, &dwSize);

        dwSize = MAX_PATH;
        dwType = REG_SZ;
        RegQueryValueEx(hKey, REG_STR_PATHTOFAVORITES, 0, &dwType, (LPBYTE)g_szPathToFavorites, &dwSize);

        RegCloseKey(hKey);
    }
}

void PersistPaths()
{
    HKEY hKey;
    DWORD dwDisp;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_STR_IMPEXP, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisp) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, REG_STR_PATHTOBOOKMARKS, 0, REG_SZ, (LPBYTE)g_szPathToBookmarks, (lstrlen(g_szPathToBookmarks)+1)*sizeof(TCHAR));
        RegSetValueEx(hKey, REG_STR_PATHTOFAVORITES, 0, REG_SZ, (LPBYTE)g_szPathToFavorites, (lstrlen(g_szPathToFavorites)+1)*sizeof(TCHAR));
        RegCloseKey(hKey);
    }
}

#define REG_STR_IE_POLICIES          TEXT("Software\\Policies\\Microsoft\\Internet Explorer")
#define REG_STR_IMPEXP_POLICIES      TEXT("DisableImportExportFavorites")

BOOL IsImportExportDisabled(void)
{
    HKEY  hKey;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    DWORD value = 0;
    BOOL  bret = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_STR_IE_POLICIES, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, REG_STR_IMPEXP_POLICIES, 0, &dwType, (PBYTE)&value, &dwSize) == ERROR_SUCCESS &&
                   (dwType == REG_BINARY || dwType == REG_DWORD))
            bret = (value) ? TRUE : FALSE;

        RegCloseKey(hKey);
    }

    return bret;
}

void DoImportOrExport(BOOL fImport, LPCWSTR pwszPath, LPCWSTR pwszImpExpPath, BOOL fConfirm)
{
    BOOL fRemote = FALSE;
    HWND hwnd = NULL;
    TCHAR szImpExpPath[INTERNET_MAX_URL_LENGTH];

    //
    // REVIEW should this be passed in...
    //
    hwnd = GetActiveWindow();

    // Decide if import/export is allowed here
    if (IsImportExportDisabled())
    {
        MLShellMessageBox(
                        hwnd, 
                        (fImport) ? MAKEINTRESOURCE(IDS_IMPORT_DISABLED) :
                                    MAKEINTRESOURCE(IDS_EXPORT_DISABLED),
                        (fImport) ? MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV) :
                                    MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                        MB_OK);
        return;
    }

 
    InitializePaths();

    //
    // Overwrite path to favorites with passed in one if present
    //
    if (pwszPath && *pwszPath != 0)
    {
        SHUnicodeToTChar(pwszPath, g_szPathToFavorites, ARRAYSIZE(g_szPathToFavorites));
    }

    //
    // Decide if we export/import to/from an URL? or a file
    //   (we expect pwszImpExpPath an absolute path)
    // if it's not a valid URL or filename, we give error message and bail out
    //
    if (pwszImpExpPath && *pwszImpExpPath != 0)
    {
        SHUnicodeToTChar(pwszImpExpPath, szImpExpPath, ARRAYSIZE(szImpExpPath));

        if (PathIsURL(pwszImpExpPath))
        {
            
            TCHAR szDialogTitle[MAX_PATH];
            TCHAR szfmt[MAX_PATH], szmsg[MAX_PATH+INTERNET_MAX_URL_LENGTH];
            fRemote = TRUE;
            
            if (fImport)
            {
                if (fConfirm)
                {
                    //
                    // Show confirmation UI when importing over internet
                    //
                    MLLoadShellLangString(IDS_CONFIRM_IMPTTL_FAV, szDialogTitle, ARRAYSIZE(szDialogTitle));
                    MLLoadShellLangString(IDS_CONFIRM_IMPORT, szfmt, ARRAYSIZE(szfmt));
                    wnsprintf(szmsg, ARRAYSIZE(szmsg), szfmt, szImpExpPath);
                    if (MLShellMessageBox(hwnd, szmsg, szDialogTitle,
                                              MB_YESNO | MB_ICONQUESTION) == IDNO)
                        return;
                }
                // download imported file to cache

                if ( (IsGlobalOffline() && !InternetGoOnline(g_szPathToBookmarks,hwnd,0)) ||
                      FAILED(URLDownloadToCacheFile(NULL, szImpExpPath, g_szPathToBookmarks, MAX_PATH, 0, NULL)))
                {
                    MLShellMessageBox(
                        hwnd, 
                        MAKEINTRESOURCE(IDS_IMPORTFAILURE_FAV), 
                        MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV), 
                        MB_OK);
                    return;
                }
            }
            else
            {
                if (fConfirm)
                {
                    //
                    // Show confirmation UI when exporting over internet
                    //
                    MLLoadShellLangString(IDS_CONFIRM_EXPTTL_FAV, szDialogTitle, ARRAYSIZE(szDialogTitle));
                    MLLoadShellLangString(IDS_CONFIRM_EXPORT, szfmt, ARRAYSIZE(szfmt));
                    wnsprintf(szmsg, ARRAYSIZE(szmsg), szfmt, szImpExpPath);
                    if (MLShellMessageBox(hwnd, szmsg, szDialogTitle,
                                              MB_YESNO | MB_ICONQUESTION) == IDNO)
                        return;
                }
                
                //
                // Create bookmark file name from bookmark directory with favorite name so we can export
                // favorites to local file before posting to URL
                //
                TCHAR *pszFav = PathFindFileName(g_szPathToFavorites);
                TCHAR *pszBMD = PathFindFileName(g_szPathToBookmarks);
                if (pszFav && pszBMD)
                {
                    StrCpyN(pszBMD, pszFav, ARRAYSIZE(g_szPathToBookmarks) - ((int)(pszFav - g_szPathToBookmarks)));
                    StrCatBuff(pszBMD, TEXT(".htm"), ARRAYSIZE(g_szPathToBookmarks) - ((int)(pszFav - g_szPathToBookmarks)));
                }
                
            }
        }
        else
        {

            if (fConfirm)
            {
                TCHAR szDialogTitle[MAX_PATH];
                TCHAR szfmt[MAX_PATH], szmsg[MAX_PATH+INTERNET_MAX_URL_LENGTH];

                if (fImport)
                {
                    //
                    // Show confirmation UI when importing
                    //
                    MLLoadShellLangString(IDS_CONFIRM_IMPTTL_FAV, szDialogTitle, ARRAYSIZE(szDialogTitle));
                    MLLoadShellLangString(IDS_CONFIRM_IMPORT, szfmt, ARRAYSIZE(szfmt));
                    wnsprintf(szmsg, ARRAYSIZE(szmsg), szfmt, szImpExpPath);
                    if (MLShellMessageBox(hwnd, szmsg, szDialogTitle,
                                              MB_YESNO | MB_ICONQUESTION) == IDNO)
                        return;
                }
                else
                {
                    //
                    // Show confirmation UI when exporting.
                    //
                    MLLoadShellLangString(IDS_CONFIRM_EXPTTL_FAV, szDialogTitle, ARRAYSIZE(szDialogTitle));
                    MLLoadShellLangString(IDS_CONFIRM_EXPORT, szfmt, ARRAYSIZE(szfmt));
                    wnsprintf(szmsg, ARRAYSIZE(szmsg), szfmt, szImpExpPath);
                    if (MLShellMessageBox(hwnd, szmsg, szDialogTitle,
                                              MB_YESNO | MB_ICONQUESTION) == IDNO)
                        return;
                }
            }
                
            if (PathFindFileName(szImpExpPath) != szImpExpPath)
            {
            
                //override path to bookmarks with passed in one
                StrCpyN(g_szPathToBookmarks, szImpExpPath, ARRAYSIZE(g_szPathToBookmarks));

            }
            else
            {
                MLShellMessageBox(
                    hwnd, 
                    MAKEINTRESOURCE(IDS_IMPORTFAILURE_FAV), 
                    MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV), 
                    MB_OK);
                return;
            }

        }
    }
    else
    {
        if (fImport)
        {
            //
            // Do Import Favorites UI
            //
            if (!BrowseForBookmarks(g_szPathToBookmarks, ARRAYSIZE(g_szPathToBookmarks), hwnd, FILE_OPEN_DIALOG))
                return;
        }
        else
        {
            //
            // Do Export Favorites UI
            //
            if (!BrowseForBookmarks(g_szPathToBookmarks, ARRAYSIZE(g_szPathToBookmarks), hwnd, FILE_SAVE_DIALOG))
                return;
        }
    }
    
    if (fImport)
    {
        if (ImportBookmarks(g_szPathToFavorites, g_szPathToBookmarks, hwnd))
        {
            MLShellMessageBox(
                            hwnd, 
                            MAKEINTRESOURCE(IDS_IMPORTSUCCESS_FAV), 
                            MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV), 
                            MB_OK);
#ifdef UNIX
	    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, g_szPathToFavorites, 0);
#endif
            if (!fRemote)
                PersistPaths();
        }
        else
        {
            ; // ImportBookmarks will report errors
        }
    }
    else  
    {
        if (ExportFavorites(g_szPathToFavorites, g_szPathToBookmarks, hwnd))
        {
            if (fRemote)
            {
                if ( (!IsGlobalOffline() || InternetGoOnline(g_szPathToBookmarks,hwnd,0)) &&
                       PostFavorites(g_szPathToBookmarks, szImpExpPath))
                {
                    MLShellMessageBox(
                                hwnd, 
                                MAKEINTRESOURCE(IDS_EXPORTSUCCESS_FAV), 
                                MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                                MB_OK);
                }
                else
                    MLShellMessageBox(
                                hwnd, 
                                MAKEINTRESOURCE(IDS_EXPORTFAILURE_FAV), 
                                MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                                MB_OK);

                //Remove temp file on local disk
                DeleteFile(g_szPathToBookmarks);
            }
            else
            {
                MLShellMessageBox(
                                hwnd, 
                                MAKEINTRESOURCE(IDS_EXPORTSUCCESS_FAV), 
                                MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                                MB_OK);
                PersistPaths();
            }
        }
        else
        {
            MLShellMessageBox(
                            hwnd, 
                            MAKEINTRESOURCE(IDS_EXPORTFAILURE_FAV), 
                            MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                            MB_OK);
        }
    }
}


//
//  *** POST FAVORITES HTML FILE ***
//
HINTERNET g_hInternet = 0;
HINTERNET g_hConnect = 0;
HINTERNET g_hHttpRequest = 0;

HANDLE    g_hEvent = NULL;

typedef struct AsyncRes
{
    DWORD_PTR   Result;
    DWORD_PTR   Error;
} ASYNCRES;

#define STR_USERAGENT          "PostFavorites"

void CloseRequest(void)
{
    if (g_hHttpRequest)
        InternetCloseHandle(g_hHttpRequest);
    if (g_hConnect)
        InternetCloseHandle(g_hConnect);
    if (g_hInternet)
        InternetCloseHandle(g_hInternet);

    g_hInternet = g_hConnect = g_hHttpRequest = 0;

}

HRESULT InitRequest(LPSTR pszPostURL, BOOL bAsync, ASYNCRES *pasyncres)
{
    char    hostName[INTERNET_MAX_HOST_NAME_LENGTH+1];
    char    userName[INTERNET_MAX_USER_NAME_LENGTH+1];
    char    password[INTERNET_MAX_PASSWORD_LENGTH+1];
    char    urlPath[INTERNET_MAX_PATH_LENGTH+1];
    URL_COMPONENTSA     uc;

    memset(&uc, 0, sizeof(URL_COMPONENTS));
    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.lpszHostName = hostName;
    uc.dwHostNameLength = sizeof(hostName);
    uc.nPort = INTERNET_INVALID_PORT_NUMBER;
    uc.lpszUserName = userName;
    uc.dwUserNameLength = sizeof(userName);
    uc.lpszPassword = password;
    uc.dwPasswordLength = sizeof(password);
    uc.lpszUrlPath = urlPath;
    uc.dwUrlPathLength = sizeof(urlPath);
    
    if (!InternetCrackUrlA(pszPostURL,lstrlenA(pszPostURL),ICU_DECODE, &uc))
    {
        return E_FAIL;
    }

    if (bAsync)
    {
        // Create an auto-reset event
        g_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if (g_hEvent == NULL)
            bAsync = FALSE;
    }

    g_hInternet = InternetOpenA(STR_USERAGENT,               // used in User-Agent: header 
                            INTERNET_OPEN_TYPE_PRECONFIG,  //INTERNET_OPEN_TYPE_DIRECT, 
                            NULL,
                            NULL, 
                            (bAsync) ? INTERNET_FLAG_ASYNC : 0
                            );

    if ( !g_hInternet )
    {
        return E_FAIL;
    }

    if (bAsync)
    {
        if (INTERNET_INVALID_STATUS_CALLBACK == InternetSetStatusCallbackA(g_hInternet, StatusCallback))
            return E_FAIL;
    }

    // Connect to host
    g_hConnect = InternetConnectA(g_hInternet, 
                                    uc.lpszHostName,
                                    uc.nPort,           //INTERNET_INVALID_PORT_NUMBER,
                                    uc.lpszUserName, 
                                    uc.lpszPassword,
                                    INTERNET_SERVICE_HTTP, 
                                    0,                  //INTERNET_FLAG_KEEP_CONNECTION, 
                                    (bAsync)? (DWORD_PTR) pasyncres : 0); 

    if ( !g_hConnect )
    {
        if (bAsync && GetLastError() == ERROR_IO_PENDING)
        {
            WaitForSingleObject(g_hEvent, INFINITE);
            if (pasyncres->Result == 0)
                return E_FAIL;

            g_hConnect = (HINTERNET)pasyncres->Result;
        }
        else
            return E_FAIL;
    }                                    
    
    // Create request.
    g_hHttpRequest = HttpOpenRequestA
        (
            g_hConnect, 
            "POST", 
            uc.lpszUrlPath,
            HTTP_VERSIONA, 
            NULL,                     //lpszReferer
            NULL,                     //lpszAcceptTypes
            INTERNET_FLAG_RELOAD
            | INTERNET_FLAG_KEEP_CONNECTION
            | SECURITY_INTERNET_MASK, // ignore SSL warnings 
            (bAsync)? (DWORD_PTR) pasyncres : 0);
                            

    if ( !g_hHttpRequest )
    {
        if (bAsync && GetLastError() == ERROR_IO_PENDING)
        {
            WaitForSingleObject(g_hEvent, INFINITE);
            if (pasyncres->Result == 0)
                return E_FAIL;

            g_hHttpRequest = (HINTERNET)pasyncres->Result;
        }
        else
            return E_FAIL;
    }
    
    return S_OK;
    
}                                                                

const char c_szHeaders[] = "Content-Type: application/x-www-form-urlencoded\r\n";
#define c_ccHearders  (ARRAYSIZE(c_szHeaders) - 1)

BOOL AddRequestHeaders
(
    LPCSTR     lpszHeaders,
    DWORD      dwHeadersLength,
    DWORD      dwAddFlag,
    BOOL       bAsync,
    ASYNCRES   *pasyncres
)
{
    BOOL bRet = FALSE;

    bRet = HttpAddRequestHeadersA(g_hHttpRequest, 
                           lpszHeaders, 
                           dwHeadersLength, 
                           HTTP_ADDREQ_FLAG_ADD | dwAddFlag);

    if (bAsync && !bRet && GetLastError() == ERROR_IO_PENDING) 
    {
        WaitForSingleObject(g_hEvent, INFINITE);
        bRet = (BOOL)pasyncres->Result;
    }

    return bRet;
}

HRESULT SendRequest
(
    LPCSTR     lpszHeaders,
    DWORD      dwHeadersLength,
    LPCSTR     lpszOption,
    DWORD      dwOptionLength,
    BOOL       bAsync,
    ASYNCRES   *pasyncres
)
{
    BOOL bRet=FALSE;

    bRet = AddRequestHeaders((LPCSTR)c_szHeaders, (DWORD)-1L, 0, bAsync, pasyncres);

    if (lpszHeaders && *lpszHeaders)        // don't bother if it's empty
    {

        bRet = AddRequestHeaders( 
                          (LPCSTR)lpszHeaders, 
                          dwHeadersLength, 
                          HTTP_ADDREQ_FLAG_REPLACE,
                          bAsync,
                          pasyncres);
        if ( !bRet )
        {
            return E_FAIL;
        }
    }

    pasyncres->Result = 0;

    bRet = HttpSendRequestA(g_hHttpRequest, 
                          NULL,                            //HEADER_ENCTYPE, 
                          0,                               //sizeof(HEADER_ENCTYPE), 
                          (LPVOID)lpszOption, 
                          dwOptionLength);

    if ( !bRet )
    {
        DWORD_PTR dwLastError = GetLastError();
        if (bAsync && dwLastError == ERROR_IO_PENDING)
        {
            WaitForSingleObject(g_hEvent, INFINITE);
            dwLastError = pasyncres->Error;
            bRet = (BOOL)pasyncres->Result;
            if (!bRet)
            {
                TraceMsg(DM_ERROR, "Async HttpSendRequest returned FALSE");
                if (dwLastError != ERROR_SUCCESS)
                {
                    TraceMsg(DM_ERROR, "Async HttpSendRequest failed: Error = %lx", dwLastError);
                    return E_FAIL;
                }
            }

        }
        else
        {
            TraceMsg(DM_ERROR, "HttpSendRequest failed: Error = %lx", dwLastError);
            return E_FAIL;
        }
    }

    //
    //verify request response here
    //
    DWORD dwBuffLen;
    TCHAR buff[10];

    dwBuffLen = sizeof(buff);

    bRet = HttpQueryInfo(g_hHttpRequest,
                        HTTP_QUERY_STATUS_CODE,   //HTTP_QUERY_RAW_HEADERS,
                        buff,
                        &dwBuffLen,
                        NULL);

    int iretcode = StrToInt(buff);
    TraceMsg(DM_TRACE, "HttpQueryInfo returned %d", iretcode);
    return (iretcode == HTTP_STATUS_OK) ? 
        S_OK : E_FAIL;

}                                                                

DWORD ReadFavoritesFile(LPCTSTR lpFile, LPSTR* lplpbuf)
{
    HANDLE  hFile = NULL;
    DWORD   cbFile = 0;
    DWORD   cbRead;

    hFile = CreateFile(lpFile, 
                GENERIC_READ,
                0,                              //no sharing
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (hFile == INVALID_HANDLE_VALUE) 
        return 0;

    cbFile = GetFileSize(hFile, NULL);
    if (cbFile == 0xFFFFFFFF)
    {
        CloseHandle(hFile);
        return 0;
    }
        
    *lplpbuf = (LPSTR)GlobalAlloc(LPTR, (cbFile + 2) * sizeof(CHAR));
    cbRead = 0;
    if (!ReadFile(hFile, *lplpbuf, cbFile, &cbRead, NULL))
    {
        cbRead = 0;
    }    
        
    ASSERT((cbRead == cbFile));
    CloseHandle(hFile);
    return cbRead;
}


BOOL PostFavorites(TCHAR *pszPathToBookmarks, TCHAR* pszPathToPost)
{
    DWORD cbRead = 0;
    LPSTR lpbuf = NULL;
    BOOL  bret = FALSE;
    BOOL  bAsync = TRUE;
    CHAR  szPathToPost[INTERNET_MAX_URL_LENGTH];
    ASYNCRES asyncres = {0, 0};

    cbRead = ReadFavoritesFile(pszPathToBookmarks, &lpbuf);
    if (cbRead == 0)
        return TRUE;
    SHTCharToAnsi(pszPathToPost, szPathToPost, ARRAYSIZE(szPathToPost));
    if (SUCCEEDED(InitRequest(szPathToPost, bAsync, &asyncres)))
    {
        bret = (SUCCEEDED(SendRequest(NULL, lstrlenA(""), lpbuf, cbRead, bAsync, &asyncres)));
    }

    CloseRequest();

    if (lpbuf)
    {
        GlobalFree( lpbuf );
        lpbuf = NULL;
    }

    return bret;
}

//
// Callback function for Asynchronous HTTP POST request
//
void CALLBACK StatusCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwStatus,
    LPVOID lpvInfo,
    DWORD dwInfoLength
    )
{
    switch (dwStatus)
    {

    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        ASYNCRES *pasyncres = (ASYNCRES *)dwContext;

        pasyncres->Result = ((LPINTERNET_ASYNC_RESULT)lpvInfo)->dwResult;
        pasyncres->Error = ((LPINTERNET_ASYNC_RESULT)lpvInfo)->dwError;

        SetEvent(g_hEvent);
    }
        break;

    default:
        break;
    }
}
