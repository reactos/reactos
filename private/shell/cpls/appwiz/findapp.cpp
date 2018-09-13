//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: findapp.cpp
//
// Implements hueristics to find the folder of an application 
//            
// History:
//         2-17-98  by dli implemented FindAppFolder
//         5-01-98  added lots of little functioins 
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "appwiz.h"
#include "appsize.h"
#include "findapp.h"
#include "util.h"


// Things to do:
// 1. Move special strings into the RC file


/*-------------------------------------------------------------------------
Purpose: This function searches and returns the sub word (if one is found).
         pszStr is the big string, pszSrch is the candidate substring used
         in the search.

         Returns NULL if no subword is found.
*/
LPCTSTR FindSubWord(LPCTSTR pszStr, LPCTSTR pszSrch)
{
    LPCTSTR pszRet = NULL;

    LPCTSTR pszBegin = pszStr;
    // Search for the sub string from the beginning
    LPCTSTR pszSub;
    while (NULL != (pszSub = StrStrI(pszBegin, pszSrch)))
    {
        LPCTSTR pszPrev;
        LPCTSTR pszEnd = pszSub + lstrlen(pszSrch);
        
        // Is the previous character alphanumeric?
        if (pszSub != pszBegin)
        {
            ASSERT(pszSub > pszBegin);
            pszPrev = CharPrev(pszBegin, pszSub);
            ASSERT(pszPrev >= pszBegin);
            if (IsCharAlphaNumeric(*pszPrev))
            {
                // yes, go on searching
                pszBegin = pszEnd;
                continue;
            }
        }

        // Is the character after the sub string we found
        // alpha numeric? 
        if (IsCharAlphaNumeric(*pszEnd))
        {
            // yes, go on searching
            pszBegin = pszEnd;
            continue;
        }

        // No to both questions above, it is a sub word!!
        pszRet = pszSub;
        break;
    }

    return pszRet;
}

int MatchMultipleSubWords(LPCTSTR pszStr, LPCTSTR pszSubWords)
{
    if (!StrChrI(pszSubWords, TEXT(' ')))
        return 0;

    TCHAR szSubWords[MAX_PATH];
    lstrcpy(szSubWords, pszSubWords);
    LPTSTR pszStart = szSubWords;

    LPTSTR pszSpace;
    int iNumMatches = 0;
    while (pszSpace = StrChrI(pszStart, TEXT(' ')))
    {
        *pszSpace = 0;
        if (FindSubWord(pszStr, pszStart))
            iNumMatches++;
        pszStart = ++pszSpace;
    }

    if (FindSubWord(pszStr, pszStart))
        iNumMatches++;
    
    return iNumMatches;
}




/*-------------------------------------------------------------------------
Purpose: Removes the spaces from pszPath, including spaces in the middle
         of the folder or filespec.  The resulting string is placed in
         pszBuf.

         Example:
         
         (before)
         "C:\Program Files\Microsoft Office\Word.exe"
         (after)
         "C:\ProgramFiles\MicrosoftOffice\Word.exe"
         
*/
void PathRemoveSpaces(LPCTSTR pszPath, LPTSTR pszBuf)
{
    ASSERT(IS_VALID_STRING_PTR(pszPath, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, MAX_PATH));

    TCHAR szTemp[MAX_PATH];
    lstrcpyn(szTemp, pszPath, SIZECHARS(szTemp));
    
    LPTSTR psz = szTemp;
    LPTSTR pszOutT = pszBuf;
    LPTSTR pszSpace;
    while (pszSpace = StrChrI(psz, TEXT(' ')))
    {
        *pszSpace = 0;
        lstrcpy(pszOutT, psz);
        pszOutT += lstrlen(psz);
        psz = ++pszSpace;
    }
    lstrcpy(pszOutT, psz);  // Copy remaining portion of string
}


// Returns TRUE if all chars in pszCharGroup is in pszString
BOOL AllCharsInString(LPCTSTR pszString, LPCTSTR pszCharGroup)
{
    if (!pszCharGroup || !pszCharGroup[0])
        return FALSE;

    LPCTSTR pszT = pszCharGroup;
    while (*pszT && StrChrI(pszString, *pszT))
        pszT++;

    return (*pszT == 0) ? TRUE : FALSE;
}



/*-------------------------------------------------------------------------
Purpose: Given the full name (and sometimes the short name) of the app,
         this function determines whether the given pszName is a match.
         If bStrict is TRUE, the heuristic skips the slinky checks.

         Returns a ranking of the accuracy of the match:
            MATCH_LEVEL_NOMATCH - pszName does not match whatsoever
            MATCH_LEVEL_LOW     - pszName somewhat matches
            MATCH_LEVEL_NORMAL  - pszName matches pretty good
            MATCH_LEVEL_HIGH    - pszName definitely matches
*/
int MatchAppNameExact(
    LPCTSTR pszName, 
    LPCTSTR pszAppFullName, 
    LPCTSTR pszAppShortName, 
    BOOL bStrict)
{
    TraceMsg(TF_FINDAPP, "MatchAppName ---- %s | %s | %s ", pszName, pszAppShortName, pszAppFullName);

    ASSERT(IS_VALID_STRING_PTR(pszName, -1));

    // In the heuristic below, we never degrade from a better match
    // to a lower match.
    int iMatch = MATCH_LEVEL_NOMATCH;

    // Since the long fullname has the most accuracy, check that first.
    if (pszAppFullName && *pszAppFullName)
    {
        // Is pszName equivalent to the full name of the app?
        if (!lstrcmpi(pszAppFullName, pszName))
            iMatch = MATCH_LEVEL_HIGH;        // Yes, definitely a high match
        else
        {
            // No, okay let's see if there are multiple (> 1) number of sub 
            // words from pszName that match the subwords in the app's full name 
            int iSubMatches = MatchMultipleSubWords(pszAppFullName, pszName);

            // More than three matches, definitely high match
            // NOTE: there could be a risk here, but I have not found a 
            // counter example yet. 
            if (iSubMatches > 3)
                iMatch = MATCH_LEVEL_HIGH;

            // NOTE: there is a risk here. For example: 
            //
            // Microsoft Internet Explorer Setup Files vs. 
            // Microsoft Internet Explorer ... 
            
            else if ((iSubMatches > 1) && (!bStrict || (iSubMatches > 2)))
                iMatch = MATCH_LEVEL_NORMAL;

            // All these are turned off if we have a strict matching
            else if (!bStrict)
            {
                // If the potential folder name is a subset of the full name or 
                // if all of the characters of the potential folder name can 
                // be found in the full name, we have a low match 
                // (Counter Ex: Microsoft vs. Microsoft Office)

                // NOTE: The reason for AllCharsInString is to detect case like 
                // Ex: "PM65 vs. Adobe Page Maker 6.5"
                // There might be a risk in this, but I have not found a counter 
                // example, yet.
                if (StrStrI(pszAppFullName, pszName) || AllCharsInString(pszAppFullName, pszName))
                    iMatch = MATCH_LEVEL_LOW;
            }
        }
    }

    // Association between folder name and the reg key name(short name)
    // This is given second priority because the reg key name is unreliable (could be an ID)
    if (MATCH_LEVEL_HIGH > iMatch && pszAppShortName && *pszAppShortName)
    {
        // Does the string exactly match the app's shortname?
        if (!lstrcmpi(pszAppShortName, pszName))
            iMatch = MATCH_LEVEL_HIGH;      // yes

        // All these are turned off if we have strict matching
        else if (!bStrict)
        {
            // Does the string contain the app's shortname?
            if (iMatch < MATCH_LEVEL_NORMAL && StrStrI(pszName, pszAppShortName))
                iMatch = MATCH_LEVEL_NORMAL;        // yes

            // Or does the app's shortname contain the string?
            else if (iMatch < MATCH_LEVEL_LOW && StrStrI(pszAppShortName, pszName))
                iMatch = MATCH_LEVEL_LOW;           // yes
        }
    }
    
    return iMatch;
}


/*-------------------------------------------------------------------------
Purpose: This function tries some different heuristics to see how well
         pszCandidate matches the given variations of the app name
         (short and long names).

         If bStrict is TRUE, the heuristic skips the slinky checks.

         Returns a ranking of the accuracy of the match:
            MATCH_LEVEL_NOMATCH - pszName does not match whatsoever
            MATCH_LEVEL_LOW     - pszName somewhat matches
            MATCH_LEVEL_NORMAL  - pszName matches pretty good
            MATCH_LEVEL_HIGH    - pszName definitely matches
*/
int MatchAppName(
    LPCTSTR pszCandidate, 
    LPCTSTR pszAppFullName, 
    LPCTSTR pszAppShortName,    OPTIONAL
    BOOL bStrict)
{
    int iMatch = MATCH_LEVEL_NOMATCH;
    if (pszCandidate && *pszCandidate)
    {
        // Clean up all the strings MAX_PATH+1, in this case, we only stick a
        // ' ' on 
        TCHAR szCleanFolderName[MAX_PATH+1];
        InsertSpaceBeforeVersion(pszCandidate, szCleanFolderName);
        
        // Now match the exact name
        iMatch = MatchAppNameExact(szCleanFolderName, pszAppFullName, pszAppShortName, bStrict);

        // Is there still no match, and do we have some flexibility to fudge?
        if (!bStrict)
        {
            int iNewMatch = MATCH_LEVEL_NOMATCH;
            // Yes; try finding it without the spaces in the filename and paths
            TCHAR szCandidate[MAX_PATH];
            TCHAR szFullName[MAX_PATH];
            TCHAR szShortName[MAX_PATH];
            
            PathRemoveSpaces(pszCandidate, szCandidate);
            PathRemoveSpaces(pszAppFullName, szFullName);

            if (pszAppShortName && pszAppShortName[0])
            {
                PathRemoveSpaces(pszAppShortName, szShortName);
                pszAppShortName = szShortName;
            }
            
            iNewMatch = MatchAppNameExact(szCandidate, szFullName, pszAppShortName, bStrict);

            if (iNewMatch > iMatch)
                iMatch = iNewMatch;
        }
    }

    return iMatch;
}


// This function returns a pointer to the beginning of the right most string 
// which looks like folder path.  This only looks for paths with fixed drive
// letters.
//
// NOTES: 
//  1. This funcion damages pszString 
//  2. We are really cracking the string, what happens
//     in localized versions? Are these going to be international char strings?
//
// Returns NULL if it could not find a legit-looking path.

LPTSTR GetRightMostFolderPathInString(LPTSTR pszString)
{
    // Reverse find the ':' in the path
    LPTSTR pszRoot = StrRChr(pszString, NULL, TEXT(':'));

    // Make sure what we found is not at the beginning of the whole 
    // string or the last character of the string
    if (pszRoot && (pszRoot > pszString) && (*CharNext(pszRoot) == TEXT('\\')))
    {
        // Okay, now move back one, we should be pointing to the drive letter
        pszRoot--;          // Don't have to use CharPrev since we're on a ':'
        
        TCHAR szDrive[2];
        szDrive[0] = *pszRoot;
        szDrive[1] = 0;
        CharUpper(szDrive);
        if ((szDrive[0] >= TEXT('C')) && (szDrive[0] <= TEXT('Z')))
        {
            // Yes, it is a real drive letter
            TCHAR atch[4];
            wsprintf(atch, TEXT("%c:\\"), *pszRoot);

            // We are only interested in fixed drives and let's check the path
            if (GetDriveType(atch) == DRIVE_FIXED)
            {
                // BUGBUG (scotth): why not use PathRemoveFileSpec here??
                PathRemoveFileSpec(pszRoot);
                return pszRoot;
            }
        }
    }

    return NULL;
}


// Given a full path, an app name, an app short name, finds the best match in this path
// EX: App Name: Microsoft Office  Short Name: Office
// C:\Microsoft Office\Office --> C:\Microsoft Office

int FindBestMatch(
    LPCTSTR pszFolder, 
    LPCTSTR pszAppFullName, 
    LPCTSTR pszAppShortName, 
    BOOL bStrict, 
    LPTSTR pszResult)
{
    // This can't be a root directory 
    ASSERT(!PathIsRoot(pszFolder));

    int iBest = MATCH_LEVEL_NOMATCH;
    int iPre  = MATCH_LEVEL_NOMATCH;
    int iThis  = MATCH_LEVEL_NOMATCH;
    
    TCHAR szPrefix[MAX_PATH];
    lstrcpy(szPrefix, pszFolder);

    if (PathRemoveFileSpec(szPrefix) && !PathIsRoot(szPrefix))
        iPre = FindBestMatch(szPrefix, pszAppFullName, pszAppShortName, bStrict, pszResult);
    
    LPTSTR pszName = PathFindFileName(pszFolder);
    if (pszName)
        iThis = MatchAppName(pszName, pszAppFullName, pszAppShortName, bStrict);

    iBest = (iPre > iThis) ? iPre : iThis;
    
    // In case there is both match in the current folder and the previous folder
    // take this current one because:
    // 1. This folder is closer to the "Uninstall" or "Modify" string
    // 2. It costs less to walk this folder;
    if ((iThis > MATCH_LEVEL_NOMATCH) && (iThis >= iPre))
        lstrcpy(pszResult, pszFolder);
    
    return iBest;
}


/*--------------------------------------------------------------------------
Purpose: Given a file name or a folder name, compare it with our list of setup
app names.

NOTE: the comparason are done as the following: We compare the name with the first portion
and the last portion of our setup name EX:
name --> myuninst.exe or uninstall.exe
Setup Name --> uninst

should bother return TRUE 
*/
BOOL IsFileOrFolderSetup(LPTSTR pszName, LPCTSTR pszDoubleString)
{       
    ASSERT(pszName);
    ASSERT(pszDoubleString);

    BOOL bRet = FALSE;

    // Neither pszName of pszDoubleString should be NULL
    if (pszName && pszDoubleString)
    {
        PathRemoveExtension(pszName);
        int cchName = lstrlen(pszName);
        LPCTSTR pszT = pszDoubleString;
        while (*pszT)
        {
            int cch = lstrlen(pszT);
            // NOTE: we compare from the beginning and from the end
            if (!StrCmpNI(pszName, pszT, cch) ||
                ((cchName > cch) && !StrCmpNI(pszName + cchName - cch, pszT, cch)))
            {
                bRet = TRUE;
                break;
            }
            
            pszT += lstrlen(pszT) + 1;
        }
    }

    return bRet;
}
/*-------------------------------------------------------------------------
Purpose: Sniffs the pszFolder for any signs that the path refers to a setup
         program.  Paths that have foldernames or filespecs with the word
         "setup" or "install" are suspect.  Returns TRUE if it looks like
         it might be a setup app or folder.

         An example is "c:\program files\microsoft office\office\setup\outlook\olmaint.exe".
         This function will return TRUE because "setup" is one of the parent
         folder names.

         cStripLevel means how many levels we will go up the directory ladder
*/
BOOL PathIsSetup(LPCTSTR pszFolder, int cStripLevel)
{
    ASSERT(IS_VALID_STRING_PTR(pszFolder, -1));
    ASSERT(cStripLevel > 0);
            
    BOOL bRet = FALSE;
    TCHAR szPath[MAX_PATH];
    TCHAR szName[MAX_PATH];
    lstrcpy(szPath, pszFolder);

    static TCHAR s_szNames[MAX_PATH];
    static BOOL s_bNamesLoaded = FALSE;

    if (!s_bNamesLoaded)
    {
        LoadAndStrip(IDS_SETUPAPPNAMES, s_szNames, ARRAYSIZE(s_szNames));
        s_bNamesLoaded = TRUE;
    }
    
    LPTSTR pszName;
    int iStripLevel = cStripLevel;
    while ((iStripLevel-- > 0) && (NULL != (pszName = PathFindFileName(szPath))))
    {
        lstrcpy(szName, pszName);
        if (IsFileOrFolderSetup(szName, s_szNames))
        {
            bRet = TRUE;
            break;
        }
        else if (!PathRemoveFileSpec(szPath) || PathIsRoot(szPath))
            break;
    }

    return bRet;
}

BOOL PathIsCommonFiles(LPCTSTR pszPath)
{
    TCHAR szCommonFiles[MAX_PATH];
    TCHAR szShortCommonFiles[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTR(pszPath, -1));
    
    // This definitely need to be put in the RC file
    wsprintf(szCommonFiles, TEXT("%c:\\Program Files\\Common Files"), pszPath[0]);

    BOOL bShort = GetShortPathName(szCommonFiles, szShortCommonFiles, ARRAYSIZE(szShortCommonFiles));
    if (bShort)
    {
        ASSERT(szShortCommonFiles[0] == szCommonFiles[0]);
    }
    
    return PathIsPrefix(szCommonFiles, pszPath) || (bShort && PathIsPrefix(szShortCommonFiles, pszPath));
}


// returns TRUE if windows directory is the prefix of pszPath
BOOL PathIsUnderWindows(LPCTSTR pszPath)
{
    TCHAR szWindows[MAX_PATH];

    GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));

    // Is this path somewhere below the windows directory?
    return PathIsPrefix(szWindows, pszPath);
}

/*-------------------------------------------------------------------------
Purpose: This function looks for a valid-looking path in the given pszInfo
         string that may indicate where the app is installed.  This attempts
         to weed out suspect paths like references to setup programs in 
         other folders.

         Returns TRUE if a useful path was found.  pszOut will contain the
         path.
*/
BOOL ParseInfoString(LPCTSTR pszInfo, LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszOut)
{
    ASSERT(IS_VALID_STRING_PTR(pszInfo, -1));
    ASSERT(IS_VALID_STRING_PTR(pszFullName, -1));
    ASSERT(pszOut);

    *pszOut = 0;
    
    // if it starts with rundll, forget it!
    if (!StrCmpNI(pszInfo, TEXT("rundll"), SIZECHARS(TEXT("rundll"))))
        return FALSE;

    // more strings we bail on ...
    
    TCHAR szInfoT[MAX_INFO_STRING];
    lstrcpyn(szInfoT, pszInfo, SIZECHARS(szInfoT));

    // The algorithm: we crack the string, and go from the right most path inside the string
    // to the left most one by one and guess which one is a more reasonable
    LPTSTR pszFolder;
    while (NULL != (pszFolder = GetRightMostFolderPathInString(szInfoT)))
    {
        TCHAR szFullPath[MAX_PATH];
        // BUGBUG: GetLongPathName does not work on Win 95
        if (StrChrI(pszFolder, TEXT('\\')) && GetLongPathName(pszFolder, szFullPath, ARRAYSIZE(szFullPath)))
        {
            // Make sure this actually is a path and not a root drive
            if (PathIsDirectory(szFullPath) && !PathIsRoot(szFullPath) && !PathIsUnderWindows(szFullPath))
            {
                // No; then we'll consider it

                LPTSTR pszFolderName;
                BOOL bStop = FALSE;
                // Find out the last folder name
                // If it is "setup" or "install", move up until it's not or we can't move up any more
                // BUGBUG: These strings should be loaded from the RC file
                while(NULL != (pszFolderName = PathFindFileName(szFullPath)) &&
                      PathIsSetup(pszFolderName, 1))
                {
                    // Have we reached the root of the path?
                    if (!PathRemoveFileSpec(szFullPath) || PathIsRoot(szFullPath))
                    {
                        // Yes; don't go any further
                        bStop = TRUE;
                        break;
                    }
                }

                // We still reject those strings with "setup" or "install" in the middle,
                // or those under the program files common files
                if (!bStop && !PathIsRoot(szFullPath) && 
                    !PathIsSetup(szFullPath, 3) && !PathIsCommonFiles(szFullPath))
                {
                    if (MATCH_LEVEL_NOMATCH < FindBestMatch(szFullPath, pszFullName, pszShortName, FALSE, pszOut))
                        return TRUE;
                }
            }
        }
        
        *pszFolder = 0;
        continue;
    }

    return FALSE;
}

#endif //DOWNLEVEL_PLATFORM
