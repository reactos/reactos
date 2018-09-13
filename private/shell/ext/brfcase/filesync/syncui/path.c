//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: path.c
//
//  This files contains the path whacking code.
//
// History:
//  01-31-94 ScottH     Moved from shellext.c
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers
#include "res.h"


/*----------------------------------------------------------
Purpose: Removes the trailing backslash from a path.

         A:\            -->     A:\
         C:\foo\        -->     C:\foo
         \\Pyrex\User\  -->     \\Pyrex\User

Returns: pointer to NULL that replaced the backslash or
         the pointer to the last character if it isn't 
         a backslash

Cond:    pimped this code from the shell
*/
LPTSTR PUBLIC MyPathRemoveBackslash(
    LPTSTR lpszPath)
    {
    int len = lstrlen(lpszPath)-1;
    if (IsDBCSLeadByte((BYTE)*CharPrev(lpszPath,lpszPath+len+1)))
        len--;

    if (!PathIsRoot(lpszPath) && lpszPath[len] == TEXT('\\'))
        lpszPath[len] = TEXT('\0');

    return lpszPath + len;
    }


#ifdef NOTUSED
/*----------------------------------------------------------
Purpose: copies the path without the extension into the buffer

Returns: new path
Cond:    --
*/
LPTSTR PUBLIC PathRemoveExt(
    LPCTSTR pszPath,
    LPTSTR pszBuf)
    {
    LPTSTR psz;
    LPTSTR pszMark = NULL;

    ASSERT(pszPath);
    ASSERT(pszBuf);

    psz = pszBuf;
    while (*pszPath)
        {
        *psz = *pszPath;
        pszPath = CharNext(pszPath);
        if (TEXT('.') == *psz)
            pszMark = psz;
        else if (TEXT('\\') == *psz)
            pszMark = NULL;
        psz = CharNext(psz);
        }
    *psz = TEXT('\0');

    if (pszMark)
        *pszMark = TEXT('\0');

    return pszBuf;
    }
#endif


/*----------------------------------------------------------
Purpose: Convert a file spec to make it look a bit better
         if it is all upper case chars.

Returns: --
Cond:    --
*/
BOOL PRIVATE PathMakeComponentPretty(LPTSTR lpPath)
{
    LPTSTR lp;

    // REVIEW: INTL need to deal with lower case chars in (>127) range?

    // check for all uppercase
    for (lp = lpPath; *lp; lp = CharNext(lp)) {
        if ((*lp >= TEXT('a')) && (*lp <= TEXT('z')))
            return FALSE;       // this is a LFN, dont mess with it
    }

    CharLower(lpPath);
    CharUpperBuff(lpPath, 1);
    return TRUE;        // did the conversion
}


//---------------------------------------------------------------------------
// Given a pointer to a point in a path - return a ptr the start of the
// next path component. Path components are delimted by slashes or the
// null at the end.
// There's special handling for UNC names.
// This returns NULL if you pass in a pointer to a NULL ie if you're about
// to go off the end of the  path.
LPTSTR PUBLIC PathFindNextComponentI(LPCTSTR lpszPath)
{
    LPTSTR lpszLastSlash;

    // Are we at the end of a path.
    if (!*lpszPath)
    {
        // Yep, quit.
        return NULL;
    }
    // Find the next slash.
    // REVIEW UNDONE - can slashes be quoted?
    lpszLastSlash = StrChr(lpszPath, TEXT('\\'));
    // Is there a slash?
    if (!lpszLastSlash)
    {
        // No - Return a ptr to the NULL.
        return (LPTSTR) (lpszPath+lstrlen(lpszPath));
    }
    else
    {
        // Is it a UNC style name?
        if (TEXT('\\') == *(lpszLastSlash+1))
        {
            // Yep, skip over the second slash.
            return lpszLastSlash+2;
        }
        else
        {
            // Nope. just skip over one slash.
            return lpszLastSlash+1;
        }
    }
}


/*----------------------------------------------------------
Purpose: Takes the path and makes it presentable.

         The rules are:
            If the LFN name is simply the short name (all caps),
             then convert to lowercase with first letter capitalized

Returns: --
Cond:    --
*/
void PUBLIC PathMakePresentable(
    LPTSTR pszPath)
    {
    LPTSTR pszComp;          // pointers to begining and
    LPTSTR pszEnd;           //  end of path component
    LPTSTR pch;
    int cComponent = 0;
    BOOL bUNCPath;
    TCHAR ch;

    bUNCPath = PathIsUNC(pszPath);

    pszComp = pszPath;
    while (pszEnd = PathFindNextComponentI(pszComp))
        {
        // pszEnd may be pointing to the right of the backslash
        //  beyond the path component, so back up one
        //
        ch = *pszEnd;
        *pszEnd = 0;        // temporary null

        // pszComp points to the path component
        //
        pch = CharNext(pszComp);
        if (TEXT(':') == *pch)
            {
            // Simply capitalize the drive-portion of the path
            //
            CharUpper(pszComp);
            }
        else if (bUNCPath && cComponent++ < 3)
            {
            // Network server or share name
            //      BUGBUG: handle LFN network names
            //
            CharUpper(pszComp);
            PathMakeComponentPretty(pszComp);
            }
        else
            {
            // Normal path component
            //
            PathMakeComponentPretty(pszComp);
            }

        *pszEnd = ch;
        pszComp = pszEnd;
        }
    }


#ifdef NOTUSED
/*----------------------------------------------------------
Purpose: Takes the path and pretties up each component of
         the path.

         The rules are:
            Use the LFN name of the component
            If the LFN name is simply the short name (all caps),
             then convert to lowercase with first letter capitalized

Returns: --
Cond:    --
*/
void PRIVATE PathGetCompleteLFN(
    LPCTSTR pszPath,
    LPTSTR pszLong,
    int cbLong)
    {
    TCHAR sz[MAX_PATH];
    TCHAR szPath[MAX_PATH+1];
    LPTSTR pszComp;         // pointers to begining and end of path component
    LPTSTR pszEnd;
    int cbPath;
    int cb;
    BOOL bAtEnd = FALSE;
    int cComponent = 0;
    BOOL bUNCPath;
    TCHAR ch;

    // BUGBUG: this is broken for double-byte characters for sure

    // For each component in string, get the LFN and add it to
    //  the pszLong buffer.
    //

    cbPath = lstrlen(pszPath) * sizeof(TCHAR);
    ASSERT(cbPath+1 <= sizeof(szPath));
    lstrcpy(szPath, pszPath);

    bUNCPath = PathIsUNC(szPath);

    *pszLong = NULL_CHAR;
    cb = 0;

    pszComp = szPath;
    while (pszEnd = PathFindNextComponentI(pszComp))
        {
        // pszEnd may be pointing to the right of the backslash beyond the
        //  path component, so back up one
        //
        if (0 == *pszEnd)
            bAtEnd = TRUE;
        else
            {
            if (!bUNCPath || cComponent > 0)
                pszEnd--;       // not the server or share portions of a UNC path
            ch = *pszEnd;
            *pszEnd = 0;        // temporary null
            }

        // pszComp points to the path component now
        //
        if (TEXT(':') == *(pszEnd-1) || TEXT(':') == *(pszEnd-2))
            {
            // Simply capitalize the drive-portion of the path
            //
            CharUpper(szPath);
            }
        else if (bUNCPath && cComponent++ < 3)
            {
            // Network server or share name
            //      BUGBUG: handle LFN network names
            //
            CharUpper(pszComp);
            PathMakeComponentPretty(pszComp);
            }
        else
            {
            int ib;

            // Try to get the LFN
            //
            *sz = NULL_CHAR;
            PathGetLongName(szPath, sz, ARRAYSIZE(sz));

            // If an LFN does not exist, keep the path component
            //  as it is. (Sometimes the path component can be
            //  something like "Link to Foo.txt")
            //
            if (*sz)
                {
                // Make pszComp point to the same offset in sz now
                //  (the components in each are the same offsets)
                //
                ib = pszComp - (LPTSTR)szPath;
                pszComp = &sz[ib];
                }
            PathMakeComponentPretty(pszComp);
            }

        // Save new LFN-ized component to buffer
        //
        cb += lstrlen(pszComp) * sizeof(TCHAR);
        if (cbLong <= cb)
            break;      // reached end of pszLong buffer
        lstrcat(pszLong, pszComp);
        if (!bAtEnd)
            {
            PathAddBackslash(pszLong);
            *pszEnd = ch;
            if (bUNCPath && 1 == cComponent)
                pszComp = pszEnd;   // pointing to share portion of path
            else
                pszComp = pszEnd+1; // Move component pointer to next part
            }
        else
            pszComp = pszEnd;
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Returns TRUE if the combined path of pszFolder and
         pszName is greater than MAX_PATH.

Returns: see above
Cond:    --
*/
BOOL PUBLIC PathsTooLong(
    LPCTSTR pszFolder,
    LPCTSTR pszName)
    {
    // +1 for possible '\' between the two path components
    return lstrlen(pszFolder) + lstrlen(pszName) + 1 >= MAX_PATH;
    }


/*----------------------------------------------------------
Purpose: Fully qualifies a path
Returns: --
Cond:    --
*/
void PUBLIC BrfPathCanonicalize(
    LPCTSTR pszPath,
    LPTSTR pszBuf)           // Must be sizeof(MAX_PATH)
    {
    DWORD dwcPathLen;

    dwcPathLen = GetFullPathName(pszPath, MAX_PATH, pszBuf, NULL);

    if (! dwcPathLen || dwcPathLen >= MAX_PATH)
        lstrcpy(pszBuf, pszPath);

    // If pszBuf won't cover losslessly to ANSI, use the short name instead

    #if defined(UNICODE) 
    {
        CHAR szAnsi[MAX_PATH];
        WCHAR szUnicode[MAX_PATH];
        szUnicode[0] = L'\0';
 
        WideCharToMultiByte(CP_ACP, 0, pszBuf, -1, szAnsi, ARRAYSIZE(szAnsi), NULL, NULL);
        MultiByteToWideChar(CP_ACP, 0, szAnsi,   -1, szUnicode, ARRAYSIZE(szUnicode));
        if (lstrcmp(szUnicode, pszBuf))
        {
            // Cannot convert losslessly from Unicode -> Ansi, so get the short path

            lstrcpy(szUnicode, pszBuf);
            SheShortenPath(szUnicode, TRUE);
            lstrcpy(pszBuf, szUnicode);
        }
   }
   #endif

    PathMakePresentable(pszBuf);

    ASSERT(lstrlen(pszBuf) < MAX_PATH);
    }


/*----------------------------------------------------------
Purpose: Gets the displayable filename of the path.  The filename 
         is placed in the provided buffer.  
         
Returns: pointer to buffer
Cond:    --
*/
LPTSTR PUBLIC PathGetDisplayName(
    LPCTSTR pszPath,
    LPTSTR pszBuf)
    {
    SHFILEINFO sfi;

    if (SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME))
        lstrcpy(pszBuf, sfi.szDisplayName);
    else
        lstrcpy(pszBuf, PathFindFileName(pszPath));

    return pszBuf;
    }


/*----------------------------------------------------------
Purpose: Checks if the attributes of the path.  If it is a
         directory and has the system bit set, and if the brfcase.dat
         file exists in the directory, then return TRUE.

         Worst case: performs two GetFileAttributes.

Returns: see above
Cond:    --
*/
BOOL PUBLIC PathCheckForBriefcase(
    LPCTSTR pszPath,
    DWORD dwAttrib)     // if -1, then function gets the attributes
    {
    ASSERT(pszPath);

    if (0xFFFFFFFF == dwAttrib)
        {
        dwAttrib = GetFileAttributes(pszPath);
        if (0xFFFFFFFF == dwAttrib)
            return FALSE;
        }

    if (IsFlagSet(dwAttrib, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY) ||
        IsFlagSet(dwAttrib, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM))
        {
        TCHAR szT[MAX_PATH];
        LPCTSTR pszDBName;

        // Check for the existence of the brfcase.dat file.
        //
        if (IsLFNDrive(pszPath))
            pszDBName = g_szDBName;
        else
            pszDBName = g_szDBNameShort;

        if (PathsTooLong(pszPath, pszDBName))
            return FALSE;
        else
            {
            PathCombine(szT, pszPath, pszDBName);
            return PathExists(szT);
            }
        }

    return FALSE;
    }


/*----------------------------------------------------------
Purpose: Returns TRUE if the path is to a briefcase root.

         This function may hit the file-system to achieve
         its goal.

         Worst case: performs two GetFileAttributes.

Returns: TRUE if the path refers to a briefcase root.
Cond:    --
*/
BOOL PUBLIC PathIsBriefcase(
    LPCTSTR pszPath)
    {
    UINT uRet;

    ASSERT(pszPath);

    // We perform our search by first looking in our cache
    // of known briefcase paths (CPATH).  If we don't find
    // anything, then we proceed to iterate thru each
    // component of the path, checking for these two things:
    //
    //   1) A directory with the system attribute
    //   2) The existence of a brfcase.dat file in the directory.
    //
    uRet = CPATH_GetLocality(pszPath, NULL);
    if (PL_FALSE == uRet)
        {
        uRet = PathCheckForBriefcase(pszPath, (DWORD)-1) ? PL_ROOT : PL_FALSE;

        if (PL_ROOT == uRet)
            {
            int atom;

            // Add this path to the briefcase path cache.
            //
            atom = Atom_Add(pszPath);
            if (ATOM_ERR != atom)
                CPATH_Replace(atom);
            }
        }

    return PL_ROOT == uRet;
    }


/*----------------------------------------------------------
Purpose: Gets the locality of the path, relative to any
         briefcase.  If PL_ROOT or PL_INSIDE is returned,
         pszBuf will contain the path to the root of the
         briefcase.

         This function may hit the file-system to achieve
         its goal.

         Worst case: performs 2*n GetFileAttributes, where
         n is the number of components in pszPath.

Returns: Path locality (PL_FALSE, PL_ROOT, PL_INSIDE)

Cond:    --
*/
UINT PUBLIC PathGetLocality(
    LPCTSTR pszPath,
    LPTSTR pszBuf)       // Buffer for root path
    {
    UINT uRet;

    ASSERT(pszPath);
    ASSERT(pszBuf);

    *pszBuf = NULL_CHAR;

    // pszPath may be:
    //  1) a path to the briefcase folder itself
    //  2) a path to a file or folder beneath the briefcase
    //  3) a path to something unrelated to a briefcase

    // We perform our search by first looking in our cache
    // of known briefcase paths (CPATH).  If we don't find
    // anything, then we proceed to iterate thru each
    // component of the path, checking for these two things:
    //
    //   1) A directory with the system attribute
    //   2) The existence of a brfcase.dat file in the directory.
    //
    uRet = CPATH_GetLocality(pszPath, pszBuf);
    if (PL_FALSE == uRet)
        {
        int cnt = 0;

        lstrcpy(pszBuf, pszPath);
        do
            {
            if (PathCheckForBriefcase(pszBuf, (DWORD)-1))
                {
                int atom;

                uRet = cnt > 0 ? PL_INSIDE : PL_ROOT;

                // Add this briefcase path to our cache
                //
                atom = Atom_Add(pszBuf);
                if (ATOM_ERR != atom)
                    CPATH_Replace(atom);

                break;      // Done
                }

            cnt++;

            } while (PathRemoveFileSpec(pszBuf));

        if (PL_FALSE == uRet)
            *pszBuf = NULL_CHAR;
        }

    return uRet;
    }


/*----------------------------------------------------------
Purpose: Returns TRUE if the file/directory exists.

Returns: see above
Cond:    --
*/
BOOL PUBLIC PathExists(
    LPCTSTR pszPath)
    {
    return GetFileAttributes(pszPath) != 0xFFFFFFFF;
    }


/*----------------------------------------------------------
Purpose: Finds the end of the root specification in a path.

           input path                    output string
           ----------                    -------------
           c:                            <empty string>
           c:\                           <empty string>
           c:\foo                        foo
           c:\foo\bar                    foo\bar
           \\pyrex\user                  <empty string>
           \\pyrex\user\                 <empty string>
           \\pyrex\user\foo              foo
           \\pyrex\user\foo\bar          foo\bar

Returns: pointer to first character after end of root spec.

Cond:    --
*/
LPCTSTR PUBLIC PathFindEndOfRoot(
    LPCTSTR pszPath)
    {
    LPCTSTR psz;

    ASSERT(pszPath);

    if (TEXT(':') == pszPath[1])
        {
        if (TEXT('\\') == pszPath[2])
            psz = &pszPath[3];
        else
            psz = &pszPath[2];
        }
    else if (PathIsUNC(pszPath))
        {
        psz = PathFindNextComponentI(pszPath);  // hop double-slash
        psz = PathFindNextComponentI(psz);      // hop server name
        if (psz)
            psz = PathFindNextComponentI(psz);  // hop share name

        if (!psz)
            {
            ASSERT(0);      // There is no share name
            psz = pszPath;
            }
        }
    else
        {
        ASSERT(0);
        psz = pszPath;
        }

    return psz;
    }


/*----------------------------------------------------------
Purpose: Sends a notify message to the shell regarding a file-status
         change.
Returns: --
Cond:    --
*/
void PUBLIC PathNotifyShell(
    LPCTSTR pszPath,
    NOTIFYSHELLEVENT nse,
    BOOL bDoNow)        // TRUE: force the event to be processed right away
    {
#pragma data_seg(DATASEG_READONLY)

    static LONG const rgShEvents[] = 
      { SHCNE_CREATE, SHCNE_MKDIR, SHCNE_UPDATEITEM, SHCNE_UPDATEDIR };

#pragma data_seg()

    ASSERT(pszPath);
    ASSERT(nse < ARRAYSIZE(rgShEvents));

    SHChangeNotify(rgShEvents[nse], SHCNF_PATH, pszPath, NULL);

    if (bDoNow)
        {
        SHChangeNotify(0, SHCNF_FLUSHNOWAIT, NULL, NULL);
        }
    }

