/************************************************************\
	FILE: convert.h

	DATE: Apr 1, 1996

	AUTHOR: Bryan Starbuck (bryanst)

	DESCRIPTION:
		This file will handle the logic to convert Netscape
	bookmarks to Microsoft Internet Explorer favorites.  This 
	will happen by finding the location of the Netscape bookmarks
	file and the Microsoft Internet Explorer favorites directory
	from the registry.  Then it will parse the bookmarks file to
	extract the URLs, which will finally be added to the favorites
	directory.

  NOTES:
	This was developed with Netscape 2.0 and IE 2.0.  Future notes
	will be made about compatibility with different versions of
	these browsers.
	
\************************************************************/

#ifndef _CONVERT_H
#define _CONVERT_H



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
//	Exprted Functions
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//  Internal Functions
//////////////////////////////////////////////////////////////////
BOOL    ImportNetscapeProxy(void);		// Import Netscape Proxy Setting
BOOL    UpdateHomePage(void);			// Upgrade IE v1.0 Home URL to v3.0
BOOL    ImportBookmarks(HINSTANCE hInstWithStr);			//  Import Netscape Bookmarks to IE Favorites

BOOL    RegStrValueEmpty(HKEY hTheKey, char * szPath, char * szKey);
BOOL    GetNSProxyValue(char * szProxyValue, DWORD * pdwSize);

BOOL        VerifyBookmarksFile(HANDLE hFile);
BOOL        ConvertBookmarks(char * szFavoritesDir, HANDLE hFile, HINSTANCE hInstWithStr);
MyEntryType   NextFileEntry(char ** ppStr, char ** ppToken);
BOOL        GetData(char ** ppData, HANDLE hFile);
void        RemoveInvalidFileNameChars(char * pBuf);
BOOL        CreateDir(char *pDirName);
BOOL        CloseDir(void);
BOOL        CreateBookmark(char *pBookmarkName);
BOOL        GetNavBkMkDir( LPSTR lpszDir, int isize );


#endif // _CONVERT_H

