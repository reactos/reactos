/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    parse.c

Abstract:

    This module contains the parse routines for the Win32 common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include "fileopen.h"



//
//  Global Variables.
//
extern TCHAR szCaption[];
extern TCHAR szWarning[];




////////////////////////////////////////////////////////////////////////////
//
//  ParseFileNew
//
//  On the return, pnExtOffset is the offset to the dot.
//
////////////////////////////////////////////////////////////////////////////

int ParseFileNew(
    LPTSTR pszPath,
    int *pnExtOffset,
    BOOL bWowApp,
    BOOL bNewStyle)
{
    int lRet = ParseFile(pszPath, TRUE, bWowApp, bNewStyle);

    if (pnExtOffset)
    {
        int nExt;

        nExt = (int)(SHORT)HIWORD(lRet);
        *pnExtOffset = ((nExt) && *(pszPath + nExt)) ? nExt : 0;
    }

    return ((int)(SHORT)LOWORD(lRet));
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseFileOld
//
//  On return, pnExtOffset is the offset to the the dot and
//  pnOldExt is the offset to the character following the dot.
//
////////////////////////////////////////////////////////////////////////////

int ParseFileOld(
    LPTSTR pszPath,
    int *pnExtOffset,
    int *pnOldExt,
    BOOL bWowApp,
    BOOL bNewStyle)
{
    int lRet = ParseFile(pszPath, TRUE, bWowApp, bNewStyle);

    int nExt = (int)(SHORT)HIWORD(lRet);
    *pnExtOffset = nExt;
    *pnOldExt = ((nExt) && *(pszPath + nExt)) ? nExt + 1 : 0;

    return ((int)(SHORT)LOWORD(lRet));
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseFile
//
//  Determines if the filename is a legal dos name.
//
//  Circumstances checked:
//      1) Valid as directory name, but not as file name
//      2) Empty String
//      3) Illegal Drive label
//      4) Period in invalid location (in extension, 1st in file name)
//      5) Missing directory character
//      6) Illegal character
//      7) Wildcard in directory name
//      8) Double slash beyond 1st 2 characters
//      9) Space character in the middle of the name (trailing spaces OK)
//         -->> no longer applies : spaces are allowed in LFN
//      10) Filename greater than 8 characters : NOT APPLICABLE TO LONG FILE NAMES
//      11) Extension greater than 3 characters: NOT APPLICABLE TO LONG FILE NAMES
//
//  lpstrFileName - ptr to a single file name
//
//  Returns:
//      LONG - LOWORD = char offset to filename,
//             HIWORD = char offset to extension (dot),
//      LONG - LOWORD is error code (<0), HIWORD is approx. place of problem
//
////////////////////////////////////////////////////////////////////////////

DWORD ParseFile(
    LPTSTR lpstrFileName,
    BOOL bLFNFileSystem,
    BOOL bWowApp,
    BOOL bNewStyle)
{
    SHORT nFile, nExt, nFileOffset, nExtOffset = 0;
    BOOL bExt;
    BOOL bWildcard;
    SHORT nNetwork = 0;
    BOOL bUNCPath = FALSE;
    LPTSTR lpstr = lpstrFileName;

    //Check if the string is empty
    if (!*lpstr)
    {
        nFileOffset = PARSE_EMPTYSTRING;
        goto ParseFile_Failure;
    }

    //Check if the string is of form  c:\foo1\foo2
    if (*(lpstr + 1) == CHAR_COLON)
    {
        //Yes. Get the drive letter
        TCHAR cDrive = (TCHAR)CharLower((LPTSTR)(DWORD)*lpstr);

        //
        //  Test to see if the drive is legal.
        //
        //  Note: Does not test that drive exists.
        //
        if ((cDrive < CHAR_A) || (cDrive > CHAR_Z))
        {
            nFileOffset = PARSE_INVALIDDRIVE;
            goto ParseFile_Failure;
        }

        //Move string past drive letter and ':'
        lpstr = CharNext(CharNext(lpstr));
    }

    if ((*lpstr == CHAR_BSLASH) || (*lpstr == CHAR_SLASH && !bNewStyle))
    {
        //
        //  Cannot have "c:\."
        //
        if (*++lpstr == CHAR_DOT)
        {
            //
            //  Unless it's stupid.
            //
            if ((*++lpstr != CHAR_BSLASH) && (*lpstr != CHAR_SLASH || bNewStyle))
            {
                //
                //  It's the root directory.
                //
                if (!*lpstr)
                {
                    goto MustBeDir;
                }
                else
                {
                    lpstr--;
                }
            }
            else
            {
                //
                //  It's saying top dir (once again), thus allowed.
                //
                ++lpstr;
            }
        }
        else if ((*lpstr == CHAR_BSLASH) && (*(lpstr - 1) == CHAR_BSLASH))
        {
            //
            //  It seems that for a full network path, whether a drive is
            //  declared or not is insignificant, though if a drive is given,
            //  it must be valid (hence the code above should remain there).
            //

            //
            //  ...since it's the first slash, 2 are allowed.
            //
            ++lpstr;

            //
            //  Must receive server and share to be real.
            //
            nNetwork = -1;

            //
            //  No wildcards allowed if UNC name.
            //
            bUNCPath = TRUE;
        }
        else if (*lpstr == CHAR_SLASH && !bNewStyle)
        {
            nFileOffset = PARSE_INVALIDDIRCHAR;
            goto ParseFile_Failure;
        }
    }
    else if (*lpstr == CHAR_DOT)
    {
        //
        //  Up one directory.
        //
        if (*++lpstr == CHAR_DOT)
        {
            ++lpstr;
        }

        if (!*lpstr)
        {
            goto MustBeDir;
        }
        if ((*lpstr != CHAR_BSLASH) && (*lpstr != CHAR_SLASH || bNewStyle))
        {
            //
            //  Jumping to Failure here will skip the parsing that causes
            //  ".xxx.txt" to return with nFileOffset = 2.
            //
            nFileOffset = 0;
            goto ParseFile_Failure;
        }
        else
        {
            //
            //  Allow directory.
            //
            ++lpstr;
        }
    }

    if (!*lpstr)
    {
        goto MustBeDir;
    }

    //
    //  Should point to first char in filename by now.
    //
    nFileOffset = nExtOffset = nFile = nExt = 0;
    bWildcard = bExt = FALSE;
    while (*lpstr)
    {
        //
        //  Anything below the "Space" character is invalid.
        //
#ifdef UNICODE
        if (*lpstr < CHAR_SPACE)
#else
        if (((UCHAR)*lpstr) < CHAR_SPACE)
#endif
        {
            nFileOffset = PARSE_INVALIDCHAR;
            goto ParseFile_Failure;
        }
        switch (*lpstr)
        {
            case ( CHAR_COLON ) :
            case ( CHAR_BAR ) :
            case ( CHAR_LTHAN ) :
            case ( CHAR_QUOTE ) :
            {
                //
                //  Invalid characters for all file systems.
                //
                nFileOffset = PARSE_INVALIDCHAR;
                goto ParseFile_Failure;
            }
            case ( CHAR_SEMICOLON ) :
            case ( CHAR_COMMA ) :
            case ( CHAR_PLUS ) :
            case ( CHAR_LBRACKET ) :
            case ( CHAR_RBRACKET ) :
            case ( CHAR_EQUAL ) :
            {
                if (!bLFNFileSystem)
                {
                    nFileOffset = PARSE_INVALIDCHAR;
                    goto ParseFile_Failure;
                }
                else
                {
                    goto RegularCharacter;
                }
            }
            case ( CHAR_SLASH ) :
            {
                if (bNewStyle)
                {
                    nFileOffset = PARSE_INVALIDCHAR;
                    goto ParseFile_Failure;
                }

                // fall thru...
            }
            case ( CHAR_BSLASH ) :
            {
                //
                //  Subdir indicators.
                //
                nNetwork++;
                if (bWildcard)
                {
                    nFileOffset = PARSE_WILDCARDINDIR;
                    goto ParseFile_Failure;
                }

                //
                // if nFile==0 means that we are seeing this backslash right next to a backslash  
                // which is not allowed.
                if (nFile == 0)
                {
                    nFileOffset = PARSE_INVALIDDIRCHAR;
                    goto ParseFile_Failure;
                }
                else
                {
                    //Move over the BSLASH/SLASH character.
                    ++lpstr;

                    //Check if the path is valid network path name
                    if (!nNetwork && !*lpstr)
                    {
                        nFileOffset = PARSE_INVALIDNETPATH;
                        goto ParseFile_Failure;
                    }

                    
                    //We assume that the  characters we are seeing are filename characters. This BSLASH/SLASH
                    //character tells that characters we have seen so far specifies the name of a directory in the
                    //path. Reset flags so that we can start looking for filename again.
                    nFile = nExt = 0;
                    nExtOffset = 0;
                    bExt = FALSE;
                }
                break;
            }
            case ( CHAR_SPACE ) :
            {
                LPTSTR lpSpace = lpstr;

                if (bLFNFileSystem)
                {
                    // In Long file name file system space characters  are O.K
                    goto RegularCharacter;
                }



                //We are not interested in the trailing spaces so null terminate it.
                *lpSpace = CHAR_NULL;


                // In non long file name file systems, space characters are OK at the end of file
                // name. Check to see if all the characters that follows are spaces. if thats the case
                // then its valid. if we have any non space character after the first space then its a 
                // invalid file name.
                
                while (*++lpSpace)
                {
                    if (*lpSpace != CHAR_SPACE)
                    {
                        *lpstr = CHAR_SPACE;
                        nFileOffset = PARSE_INVALIDSPACE;
                        goto ParseFile_Failure;
                    }
                }

                break;
            }
            
            case ( CHAR_DOT ) :
            {

                // In newstyle nExtOffset points to the dot and not to the first character of extension.
                if (bNewStyle)
                {
                    nExtOffset = (SHORT)(lpstr - lpstrFileName);
                    goto RegularCharacter;
                }
                
                if (nFile == 0)
                {
                    nFileOffset = (SHORT)(lpstr - lpstrFileName);
                    if (*++lpstr == CHAR_DOT)
                    {
                        ++lpstr;
                    }
                    if (!*lpstr)
                    {
                        goto MustBeDir;
                    }

                    //
                    //  Flags already set.
                    //
                    nFile++;
                    ++lpstr;
                }
                else
                {
                    nExtOffset = 0;
                    ++lpstr;
                    bExt = TRUE;
                }
                break;
            }
            case ( CHAR_STAR ) :
            case ( CHAR_QMARK ) :
            {
                bWildcard = TRUE;

                //  Fall thru...
            }
            default :
            {
RegularCharacter:

                //Are we in extension part ?
                if (bExt)
                {
                    //Is this first character in extension part
                    if (++nExt == 1)
                    {
                        //Yes, then get the Extension offset
                        nExtOffset = (SHORT)(lpstr - lpstrFileName);
                    }
                }

                //We are still in file name part. 
                //Is this the first character in filename part ?
                else if (++nFile == 1)
                {
                    //Yes. Get the filename offset
                    nFileOffset = (SHORT)(lpstr - lpstrFileName);
                }

                //Move to the next character
                lpstr = CharNext(lpstr);
                break;
            }
        }
    }

    if (nNetwork == -1)
    {
        nFileOffset = PARSE_INVALIDNETPATH;
        goto ParseFile_Failure;
    }
    else if (bUNCPath)
    {
        if (!nNetwork)
        {
            //
            //  Server and share only.(e.g \\server\foo)
            //
            *lpstr = CHAR_NULL;
            nFileOffset = PARSE_DIRECTORYNAME;
            goto ParseFile_Failure;
        }
        else if ((nNetwork == 1) && !nFile)
        {
            //
            //  Server and share root.(e.g \\server\foo\)
            //
            *lpstr = CHAR_NULL;
            nFileOffset = PARSE_DIRECTORYNAME;
            goto ParseFile_Failure;
        }
    }

    if (!nFile)
    {
MustBeDir:
        nFileOffset = PARSE_DIRECTORYNAME;
        goto ParseFile_Failure;
    }

    //
    //  If bNewStyle is true, no ext. wanted.
    //
    if (!bNewStyle)
    {
        if ((bWowApp) &&
            (*(lpstr - 1) == CHAR_DOT) &&
            (*CharNext(lpstr - 2) == CHAR_DOT))
        {
            //
            //  Remove terminating period.
            //
            *(lpstr - 1) = CHAR_NULL;
        }
        else if (!nExt)
        {
ParseFile_Failure:
            //
            //  Need to recheck bNewStyle since we can jump here.
            //
            if (!bNewStyle)
            {
                nExtOffset = (SHORT)(lpstr - lpstrFileName);
            }
        }
    }

    return (MAKELONG(nFileOffset, nExtOffset));
}


////////////////////////////////////////////////////////////////////////////
//
//  PathRemoveBslash
//
//  Removes a trailing backslash from the given path.
//
//  Returns:
//      Pointer to NULL that replaced the backslash   OR
//      Pointer to the last character if it isn't a backslash
//
////////////////////////////////////////////////////////////////////////////

LPTSTR PathRemoveBslash(
    LPTSTR lpszPath)
{
    int len = lstrlen(lpszPath) - 1;

#ifndef UNICODE
    if (IsDBCSLeadByte(*CharPrev(lpszPath, lpszPath + len + 1)))
    {
        len--;
    }
#endif

    if (!PathIsRoot(lpszPath) && (lpszPath[len] == CHAR_BSLASH))
    {
        lpszPath[len] = CHAR_NULL;
    }

    return (lpszPath + len);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsWild
//
////////////////////////////////////////////////////////////////////////////

BOOL IsWild(
    LPCTSTR lpsz)
{
    return (StrChr(lpsz, CHAR_STAR) || StrChr(lpsz, CHAR_QMARK));
}


////////////////////////////////////////////////////////////////////////////
//
//  AppendExt
//
//  Appends default extension onto path name.
//  It assumes the current path name doesn't already have an extension.
//  lpExtension does not need to be null terminated.
//
////////////////////////////////////////////////////////////////////////////

VOID AppendExt(
    LPTSTR lpszPath,
    LPCTSTR lpExtension,
    BOOL bWildcard)
{
    WORD wOffset;
    SHORT i;
    TCHAR szExt[MAX_PATH + 1];

    if (lpExtension && *lpExtension)
    {
        wOffset = (WORD)lstrlen(lpszPath);
        if (bWildcard)
        {
            *(lpszPath + wOffset++) = CHAR_STAR;
        }

        //
        //  Add a period.
        //
        *(lpszPath + wOffset++) = CHAR_DOT;
        for (i = 0; *(lpExtension + i) && i < MAX_PATH; i++)
        {
            szExt[i] = *(lpExtension + i);
        }
        szExt[i] = 0;

        //
        //  Remove leading / trailing blanks in the extension.
        //
        PathRemoveBlanks(szExt);

        //
        //  Add the rest.
        //
        lstrcpy(lpszPath + wOffset, szExt);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsUNC
//
//  Determines if the given path is a UNC path.
//
//  Returns:
//      TRUE    if path starts with "\\" or "X:\\"
//      FALSE   otherwise
//
////////////////////////////////////////////////////////////////////////////

BOOL IsUNC(
    LPCTSTR lpszPath)
{
    return ( DBL_BSLASH(lpszPath) ||
             ((lpszPath[1] == CHAR_COLON) && DBL_BSLASH(lpszPath + 2)) );
}


////////////////////////////////////////////////////////////////////////////
//
//  PortName
//
////////////////////////////////////////////////////////////////////////////

#define PORTARRAY 14

BOOL PortName(
    LPTSTR lpszFileName)
{
    static TCHAR *szPorts[PORTARRAY] = { TEXT("LPT1"),
                                         TEXT("LPT2"),
                                         TEXT("LPT3"),
                                         TEXT("LPT4"),
                                         TEXT("COM1"),
                                         TEXT("COM2"),
                                         TEXT("COM3"),
                                         TEXT("COM4"),
                                         TEXT("EPT"),
                                         TEXT("NUL"),
                                         TEXT("PRN"),
                                         TEXT("CLOCK$"),
                                         TEXT("CON"),
                                         TEXT("AUX"),
                                       };
    short i;
    TCHAR cSave, cSave2;


    cSave = *(lpszFileName + 4);
    if (cSave == CHAR_DOT)
    {
        *(lpszFileName + 4) = CHAR_NULL;
    }

    //
    //  For "EPT".
    //
    cSave2 = *(lpszFileName + 3);
    if (cSave2 == CHAR_DOT)
    {
      *(lpszFileName + 3) = CHAR_NULL;
    }

    for (i = 0; i < PORTARRAY; i++)
    {
        if (!lstrcmpi(szPorts[i], lpszFileName))
        {
            break;
        }
    }
    *(lpszFileName + 4) = cSave;
    *(lpszFileName + 3) = cSave2;

    return (i != PORTARRAY);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsDirectory
//
////////////////////////////////////////////////////////////////////////////

BOOL IsDirectory(
    LPTSTR pszPath)
{
    DWORD dwAttributes;

    //
    //  Clean up for GetFileAttributes.
    //
    PathRemoveBslash(pszPath);

    dwAttributes = GetFileAttributes(pszPath);
    return ( (dwAttributes != (DWORD)(-1)) &&
             (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) );
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteProtectedDirCheck
//
//  This function takes a full filename, strips the path, and creates
//  a temp file in that directory.  If it can't, the directory is probably
//  write protected.
//
//  Returns:
//      error code if writeprotected
//      0 if successful creation of file.
//
//  Assumptions:
//    Full Path name on input with space for full filename appended.
//
//  Note: Do NOT use this on a floppy, it's too slow!
//
////////////////////////////////////////////////////////////////////////////

int WriteProtectedDirCheck(
    LPTSTR lpszFile)
{
    SHORT nFileOffset;
    TCHAR szFile[MAX_PATH + 1];
    TCHAR szBuf[MAX_PATH + 1];

    lstrcpyn(szFile, lpszFile, MAX_PATH + 1);
    nFileOffset = (SHORT)(int)LOWORD(ParseFile(szFile, TRUE, FALSE, TRUE));

    szFile[nFileOffset - 1] = CHAR_NULL;
    if (!GetTempFileName(szFile, TEXT("TMP"), 0, szBuf))
    {
        return (GetLastError());
    }
    else
    {
        DeleteFile(szBuf);
        return (0);               // success
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FOkToWriteOver
//
//  Verifies that the user really does want to destroy the file,
//  replacing its contents with new stuff.
//
////////////////////////////////////////////////////////////////////////////

BOOL FOkToWriteOver(
    HWND hDlg,
    LPTSTR szFileName)
{
    if (!CDLoadString( g_hinst,
                     iszOverwriteQuestion,
                     szCaption,
                     WARNINGMSGLENGTH - 1 ))
    {
        return (FALSE);
    }

    //
    //  Since we're passed in a valid filename, if the 3rd & 4th characters
    //  are both slashes, weve got a dummy drive as the 1st two characters.
    //
    if (DBL_BSLASH(szFileName + 2))
    {
        szFileName = szFileName + 2;
    }

    wsprintf(szWarning, szCaption, szFileName);

    GetWindowText(hDlg, szCaption, cbCaption);
    return (MessageBox( hDlg,
                        szWarning,
                        szCaption,
                        MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ) == IDYES);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateFileDlg
//
////////////////////////////////////////////////////////////////////////////

int CreateFileDlg(
    HWND hDlg,
    LPTSTR szPath)
{
    //
    //  Since we're passed in a valid filename, if the 3rd & 4th
    //  characters are both slashes, we've got a dummy drive as the
    //  1st two characters.
    //
    if (DBL_BSLASH(szPath + 2))
    {
        szPath = szPath + 2;
    }

    if (!CDLoadString(g_hinst, iszCreatePrompt, szCaption, TOOLONGLIMIT))
    {
        return (IDNO);
    }
    if (lstrlen(szPath) > TOOLONGLIMIT)
    {
        *(szPath + TOOLONGLIMIT) = CHAR_NULL;
    }

    wsprintf(szWarning, szCaption, szPath);

    GetWindowText(hDlg, szCaption, TOOLONGLIMIT);

    return (MessageBox( hDlg,
                        szWarning,
                        szCaption,
                        MB_YESNO | MB_ICONQUESTION ));
}


#ifndef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  EliminateString
//
//  Chops the string by the specified length.  If a DBCS lead byte is
//  left as the last char, then it is removed as well.
//
//  NOTE: For non-Unicode strings only.
//
////////////////////////////////////////////////////////////////////////////

VOID EliminateString(
    LPSTR lpStr,
    int nLen)
{
    LPSTR lpChar;
    BOOL bFix = FALSE;

    *(lpStr + nLen) = CHAR_NULL;
    for (lpChar = lpStr + nLen - 1; lpChar >= lpStr; lpChar--)
    {
        if (!IsDBCSLeadByte(*lpChar))
        {
            break;
        }
        bFix = !bFix;
    }
    if (bFix)
    {
        *(lpStr + nLen - 1) = CHAR_NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsBackSlash
//
//  Decides whether a character is a '\' or a DBCS trail byte with the same
//  code point value.
//
//  NOTE: For non-Unicode strings only.
//
////////////////////////////////////////////////////////////////////////////

BOOL IsBackSlash(
    LPSTR lpStart,
    LPSTR lpChar)
{
    if (*lpChar == CHAR_BSLASH)
    {
        BOOL bRet = TRUE;

        while (--lpChar >= lpStart)
        {
            if (!IsDBCSLeadByte(*lpChar))
            {
                break;
            }
            bRet = !bRet;
        }
        return (bRet);
    }
    return (FALSE);
}

#endif

