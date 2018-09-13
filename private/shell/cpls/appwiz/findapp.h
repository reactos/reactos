#ifndef __FINDAPP_H_
#define __FINDAPP_H_

// Match Levels
#define MATCH_LEVEL_NOMATCH 0
#define MATCH_LEVEL_LOW     1
#define MATCH_LEVEL_NORMAL  2
#define MATCH_LEVEL_HIGH    3

// Parse a string to find the possible path in it
BOOL ParseInfoString(LPCTSTR pszInfo, LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszOut);

// Match the app folder or exe name
int MatchAppName(LPCTSTR pszName, LPCTSTR pszAppFullName, LPCTSTR pszAppShortName, BOOL bStrict);

// Find the best match for an app folder give a path name
int FindBestMatch(LPCTSTR pszFolder, LPCTSTR pszAppFullName, LPCTSTR pszAppShortName, BOOL bStrict, LPTSTR pszResult);

// Find a sub word
LPCTSTR FindSubWord(LPCTSTR pszStr, LPCTSTR pszSrch);

// is the path a setup path, cStripLevel is the maximum level we go up in the
// directory chain
BOOL PathIsSetup(LPCTSTR pszFolder, int cStripLevel);

BOOL PathIsCommonFiles(LPCTSTR pszPath);

BOOL PathIsUnderWindows(LPCTSTR pszPath);
#endif // _FINDAPP_H_