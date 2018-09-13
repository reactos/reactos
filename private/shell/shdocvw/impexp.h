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

#ifndef _IMPEXP_H
#define _IMPEXP_H


//////////////////////////////////////////////////////////////////
//	Exported Functions
//////////////////////////////////////////////////////////////////
BOOL    GetVersionFromFile(PTSTR pszFileName, PDWORD pdwMSVer, PDWORD pdwLSVer);
void    DoImportOrExport(BOOL fImport, LPCWSTR pwszPath, LPCWSTR pwszImpExpDestPath, BOOL fConfirm);

BOOL    ImportBookmarks(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks, HWND hwnd);			//  Import Netscape Bookmarks to IE Favorites
BOOL    ExportFavorites(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks);			//  Export IE Favorites to Netscape Bookmarks

#ifdef UNIX
BOOL        GetNavBkMkDir( LPTSTR lpszDir, int isize);
BOOL        GetPathFromRegistry(LPTSTR szPath, UINT cbPath, HKEY theHKEY, LPTSTR szKey, LPTSTR szVName);
BOOL        VerifyBookmarksFile(HANDLE hFile);
BOOL    ImportBookmarks(TCHAR *pszPathToFavorites, TCHAR *pszPathToBookmarks, HWND hwnd);//  Import Netscape Bookmarks to IE Favorites
#endif
#endif // _IMPEXP_H

