   /************************************************************\
    FILE: convert.c

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

\************************************************************/
#include "priv.h"
#include "advpub.h"
#ifndef UNIX
#include "sdsutils.h"
#include "utils.h"
#else
#include "resource.h"
#define ANSIStrStr strstr
#define ANSIStrChr strchr
#endif /* UNIX */
#include "convert.h"
#include <regstr.h>
#include "impexp.h" // for GetPathFromRegistry (a function that should be nuked)


//////////////////////////////////////////////////////////////////
//  TYPES:
//////////////////////////////////////////////////////////////////

//typedef enum MYENTRYTYPE MyEntryType;

extern HINSTANCE g_hinst;

//////////////////////////////////////////////////////////////////
//  Constants:
//////////////////////////////////////////////////////////////////
#define MAX_URL 2048
#define FILE_EXT 4          // For ".url" at the end of favorite filenames
#define REASONABLE_NAME_LEN     100

#define BEGIN_DIR_TOKEN         "<DT><H"
#define MID_DIR_TOKEN           "\">"
#define END_DIR_TOKEN           "</H"
#define BEGIN_EXITDIR_TOKEN     "</DL><p>"
#define BEGIN_URL_TOKEN         "<DT><A HREF=\""
#define END_URL_TOKEN           "\" A"
#define BEGIN_BOOKMARK_TOKEN    ">"
#define END_BOOKMARK_TOKEN      "</A>"

#define VALIDATION_STR "<!DOCTYPE NETSCAPE-Bookmark-file-"

//////////////////////////////////////////////////////////////////
//  GLOBALS:
//////////////////////////////////////////////////////////////////
#ifndef UNIX
char    * szNetscapeBMRegSub        = "SOFTWARE\\Netscape\\Netscape Navigator\\Bookmark List";
#else
char    * szNetscapeBMRegSub        = "SOFTWARE\\Microsoft\\Internet Explorer\\unix\\nsbookmarks";
#endif /* UNIX */
char    * szNetscapeBMRegKey        = "File Location";
char    * szIEFavoritesRegSub       = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
char    * szIEFavoritesRegKey       = "Favorites";
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

BOOL ImportBookmarks(HINSTANCE hInstWithStr)
{
    char    szFavoritesDir[MAX_PATH];
    char    szBookmarksDir[MAX_PATH];
    HANDLE  hBookmarksFile        = INVALID_HANDLE_VALUE;
    BOOL    fSuccess                = FALSE;
    

    // Initialize Variables
    szFavoritesDir[0] = '\0';
    szBookmarksDir[0] = '\0';

    // Get Bookmarks Dir
    if (TRUE == GetNavBkMkDir( szBookmarksDir, sizeof(szBookmarksDir) ) )
    {
        if ((NULL != szBookmarksDir) && (szBookmarksDir[0] != '\0'))
        {
            hBookmarksFile = CreateFile(szBookmarksDir, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            
            if ( hBookmarksFile != INVALID_HANDLE_VALUE ) 
            {
                // Get Favorites Dir
                if (TRUE == GetPathFromRegistry(szFavoritesDir, MAX_PATH, HKEY_CURRENT_USER,
                    szIEFavoritesRegSub, szIEFavoritesRegKey))
                {
                    if ((NULL != szFavoritesDir) && (szFavoritesDir[0] != '\0'))
                    {
                        // Verify it's a valid Bookmarks file
                        if (TRUE == VerifyBookmarksFile( hBookmarksFile ))
                        {
                            // Do the importing...
                            fSuccess = ConvertBookmarks(szFavoritesDir, hBookmarksFile, hInstWithStr);
                        }
                    }
                }
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hBookmarksFile)
    {
        CloseHandle(hBookmarksFile);
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

BOOL ConvertBookmarks(char * szFavoritesDir, HANDLE hFile, HINSTANCE hInstWithStr)
{
    BOOL    fDone       = FALSE;
    BOOL    fSuccess    = TRUE;
    BOOL    fIsEmpty    = TRUE;
    BOOL    fCreated    = FALSE;
    char    * szData    = NULL;
    char    * szCurrent = NULL;
    char    * szToken   = NULL;
    char    szSubDir[MAX_PATH];
// PORT QSY  Need to reset the current working dir back after going into
//	     Favorite dir
#ifdef UNIX
    char    szOldDir[MAX_PATH];
    DWORD   fGoodDir;

    szOldDir[0] = '\0';
    fGoodDir = GetCurrentDirectory(MAX_PATH,  szOldDir);
#endif
// PORT 

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

    // We don't want to install Other Popular Browser's bookmarks on our top level of our
    // favorites, so we create a sub director to put them in.
    if (0 != LoadString(hInstWithStr, IDS_NS_BOOKMARKS_DIR, szSubDir, sizeof(szSubDir)))
    {
        lstrcat(szFavoritesDir, szSubDir);

        if ((TRUE == fSuccess) && (!SetCurrentDirectory(szFavoritesDir)))
        {
            // If the directory doesn't exist, make it...
            if (!CreateDirectory(szFavoritesDir, NULL))
                fSuccess = FALSE;
            else {
                fCreated = TRUE;
                if (!SetCurrentDirectory(szFavoritesDir))
                    fSuccess = FALSE;
            }
        }
    }
    else
        fSuccess = FALSE;

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

    if ( fIsEmpty  && fCreated )
    {
        // nothing to import, delete the dir created earlier
#ifndef UNIX
        DelNode(szFavoritesDir, 0);
#else
        /* 
         * If we did not create the directory this time,
         * we should not delete it
         */
           RemoveDirectory(szFavoritesDir);
#endif /* UNIX */
    }

    if (NULL != szData)
    {
        LocalFree(szData);
        szData = NULL;
        szCurrent = NULL;       // szCurrent no longer points to valid data.
        szToken = NULL;     // szCurrent no longer points to valid data.
    }
   
// PORT QSY	See comments above
#ifdef UNIX
    if (fGoodDir && szOldDir[0] != '\0')
	SetCurrentDirectory(szOldDir);
#endif

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
            if ((NULL == (pszTemp = ANSIStrStr(pCurrentToken, MID_DIR_TOKEN))) ||
                (NULL == ANSIStrStr(pszTemp, END_DIR_TOKEN)))
            {
                returnVal = ET_ERROR;       // We can't find all the tokens needed.
            }
            else
            {
                // This function has to set *ppToken to the name of the directory to create
                *ppToken =  ANSIStrStr(pCurrentToken, MID_DIR_TOKEN) + sizeof(MID_DIR_TOKEN)-1;
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
        // Check for begin dir token
        if ((ET_ERROR != returnVal) &&
            (NULL != (pCurrentToken = ANSIStrStr(*ppStr, BEGIN_URL_TOKEN))))
        {
            // Bookmark token found
            // Verify that other needed tokens exist or it's an error
            if ((NULL == (pszTemp = ANSIStrStr(pCurrentToken, END_URL_TOKEN))) ||
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
                *ppStr = ANSIStrStr(pTheToken, MID_DIR_TOKEN) + sizeof(MID_DIR_TOKEN);
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
        pBuf = CharNext(pBuf);
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
    if (NULL != pstrEndOfStr)
    {
        lStrLen = pstrEndOfStr-pBookmarkName;
        if (MAX_URL < lStrLen)
        lStrLen = MAX_URL-1;

        // Create the name of the Bookmark
        lstrcpyn(szURL, pBookmarkName, MAX_URL);
        szURL[lStrLen] = '\0';

        pstrBeginOfName = ANSIStrStr(pstrEndOfStr, BEGIN_BOOKMARK_TOKEN);
        if (NULL != pstrBeginOfName)
        {
        pstrBeginOfName += sizeof(BEGIN_BOOKMARK_TOKEN) - 1;            // Start at beginning of Name

        pstrEndOfStr = ANSIStrStr(pstrBeginOfName, END_BOOKMARK_TOKEN); // Find end of name
        if (NULL != pstrEndOfStr)
        {
            lStrLen = pstrEndOfStr-pstrBeginOfName;
            if (REASONABLE_NAME_LEN-FILE_EXT < lStrLen)
            lStrLen = REASONABLE_NAME_LEN-FILE_EXT-1;

            // Generate the URL
            lstrcpyn(szNameOfBM, pstrBeginOfName, lStrLen+1);
            //szNameOfBM[lStrLen] = '\0';
            lstrcat(szNameOfBM, ".url");
            RemoveInvalidFileNameChars(szNameOfBM);


            // Check to see if Favorite w/same name exists
            if (INVALID_HANDLE_VALUE != (hFile = CreateFile(szNameOfBM, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                                 CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL )))
            {
                WriteFile(hFile, "[InternetShortcut]\n", lstrlen( "[InternetShortcut]\n" ), &dwSize, NULL);
                wnsprintfA( szBuf, ARRAYSIZE(szBuf), "URL=%s\n", szURL);
                WriteFile(hFile, szBuf, lstrlen(szBuf), &dwSize, NULL );
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
            lStrLen = pstrEndOfName-pDirName;
            if (REASONABLE_NAME_LEN < lStrLen)
            lStrLen = REASONABLE_NAME_LEN-1;

            lstrcpyn(szNameOfDir, pDirName, lStrLen+1);
            //szNameOfDir[lStrLen] = '\0';
            RemoveInvalidFileNameChars(szNameOfDir);

            // BUGBUG : Try to CD into existing dir first
            if ( !SetCurrentDirectory(szNameOfDir) )
            {
                if ( CreateDirectory(szNameOfDir, NULL) )
                {
                    if ( SetCurrentDirectory(szNameOfDir) )
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
    return( SetCurrentDirectory("..") );
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
        szFileHeader[sizeof(VALIDATION_STR)] = '\0';            // Terminate String.
        if (0 == lstrcmp(szFileHeader, VALIDATION_STR))          // See if header is the same as the Validation string.
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


BOOL GetNavBkMkDir( LPSTR lpszDir, int isize )
{
    char    szDir[MAX_PATH];
    HKEY    hKey;
    HKEY    hKeyUser;
    char    szUser[MAX_PATH];
    DWORD   dwSize;
    BOOL    bDirFound = FALSE;

#ifndef UNIX
    lstrcpy( szUser, REGSTR_PATH_APPPATHS );
    AddPath( szUser, "NetScape.exe" );
    if ( GetPathFromRegistry( szDir, MAX_PATH, HKEY_LOCAL_MACHINE, szUser, "" ) &&
         lstrlen(szDir) )
    {
        DWORD dwMV, dwLV;

        if ( SUCCEEDED(GetVersionFromFile( szDir, &dwMV, &dwLV, TRUE)) )
        {
            if ( dwMV < 0x00040000 )
                bDirFound = GetPathFromRegistry( lpszDir, isize, HKEY_CURRENT_USER,
                                     szNetscapeBMRegSub, szNetscapeBMRegKey);
            else
            {
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Netscape\\Netscape Navigator\\Users", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szUser);
                    if (RegQueryValueEx(hKey, "CurrentUser", NULL, NULL, (LPBYTE)szUser, &dwSize) == ERROR_SUCCESS)
                    {
                        if (RegOpenKeyEx(hKey, szUser, 0, KEY_READ, &hKeyUser) == ERROR_SUCCESS)
                        {
                            dwSize = sizeof(szDir);
                            if (RegQueryValueEx(hKeyUser, "DirRoot", NULL, NULL, (LPBYTE)szDir, &dwSize) == ERROR_SUCCESS)
                            {
                                // Found the directory for the current user.
                                lstrcpy( lpszDir, szDir);
                                AddPath( lpszDir, "bookmark.htm" );
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
#endif /* UNIX */
        bDirFound = GetPathFromRegistry( lpszDir, isize, HKEY_CURRENT_USER,
                                         szNetscapeBMRegSub, szNetscapeBMRegKey);
    return bDirFound;
}
