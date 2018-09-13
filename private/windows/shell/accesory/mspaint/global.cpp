
#include "stdafx.h"


#include <direct.h>

#include <tchar.h>
#include "global.h"

#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif // _DEBUG

#include "memtrace.h"

/***************************************************************************/

//
// This function returns a pointer to a monochrome GDI brush with
// alternating "black" and "white" pixels.  This brush should NOT
// be deleted!
//
CBrush* GetHalftoneBrush()
    {
    static CBrush NEAR halftoneBrush;

    if (halftoneBrush.m_hObject == NULL)
        {
        static WORD NEAR rgwHalftone [] =
            {
            0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555
            };

        CBitmap bitmap;

        if (!bitmap.CreateBitmap(8, 8, 1, 1, rgwHalftone))
            return NULL;

        if (!halftoneBrush.CreatePatternBrush(&bitmap))
            return NULL;
        }

    return &halftoneBrush;
    }

/////////////////////////////////////////////////////////////////////////////
//
// The following code manages a cache of GDI brushes that correspond to
// the system defined colors.  The cache is flushed when the user changes
// any of the system colors using the control panel.  Using GetSysBrush()
// to get a system colored brush will be more efficient than creating the
// brush yourself.
//
void ResetSysBrushes()
    {
    //NOTE: we don't include our extensions to the "system" brushes, because
    //  often the brush handle is used as hbrBackground for a Window class!
    for (UINT nBrush = 0; nBrush < nSysBrushes + nOurBrushes; nBrush++)
        if (theApp.m_pbrSysColors[nBrush])
            {
            delete theApp.m_pbrSysColors[nBrush];
            theApp.m_pbrSysColors[nBrush] = NULL;
            }
    }

COLORREF MyGetSysColor(UINT nSysColor)
    {
    if (nSysColor < nSysBrushes)
        return ::GetSysColor( nSysColor );

    static COLORREF NEAR rgColors[nOurBrushes] =
        {
        CMP_RGB_HILITE, CMP_RGB_LTGRAY, CMP_RGB_DKGRAY, CMP_RGB_BLACK,
        };

    ASSERT(nSysColor - CMP_COLOR_HILITE >= 0);
    ASSERT(nSysColor - CMP_COLOR_HILITE < nOurBrushes);

    return rgColors[nSysColor - CMP_COLOR_HILITE];
    }

CBrush* GetSysBrush(UINT nSysColor)
    {
    ASSERT(nSysColor < nSysBrushes + nOurBrushes);

    if (! theApp.m_pbrSysColors[nSysColor])
        {
        COLORREF cr = MyGetSysColor(nSysColor);

        theApp.m_pbrSysColors[nSysColor] = new CBrush;

        if (theApp.m_pbrSysColors[nSysColor])
            {
            if (! theApp.m_pbrSysColors[nSysColor]->CreateSolidBrush( cr ))
                {
                TRACE( TEXT("GetSysBrush failed!\n") );
                theApp.SetGdiEmergency();

                delete theApp.m_pbrSysColors[nSysColor];
                theApp.m_pbrSysColors[nSysColor] = NULL;
                }
            }
        else
            theApp.SetMemoryEmergency();
        }

    return theApp.m_pbrSysColors[nSysColor];
    }


//
//      PreTerminateList
//              Helper function for deleting all objects in a list, and then
//              truncating the list.  Help stop leaks by using this, so your
//              objects don't get left in memory.
//

void PreTerminateList( CObList* pList )
    {
    if (pList == NULL || pList->IsEmpty())
        return;

    while (! pList->IsEmpty())
        {
        CObject* pObj = pList->RemoveHead();
        delete pObj;
        }
    }

/////////////////////////////////////////////////////////////////////////////


void MySplitPath (const TCHAR *szPath, TCHAR *szDrive, TCHAR *szDir, TCHAR *szName, TCHAR *szExt)
    {
       // Found this in tchar.h
       _tsplitpath (szPath, szDrive, szDir, szName, szExt);
    }

// Remove the drive and directory from a file name...
//
CString StripPath(const TCHAR* szFilePath)
    {
    TCHAR szName [_MAX_FNAME + _MAX_EXT];
    TCHAR szExt [_MAX_EXT];
    MySplitPath(szFilePath, NULL, NULL, szName, szExt);
    lstrcat(szName, szExt);
    return CString(szName);
    }

// Remove the name part of a file path.  Return just the drive and directory.
//
CString StripName(const TCHAR* szFilePath)
    {
    TCHAR szPath [_MAX_DRIVE + _MAX_DIR];
    TCHAR szDir [_MAX_DIR];
    MySplitPath(szFilePath, szPath, szDir, NULL, NULL);
    lstrcat(szPath, szDir);
    return CString(szPath);
    }

// Remove the name part of a file path.  Return just the drive and directory, and name.
//
CString StripExtension(const TCHAR* szFilePath)
    {
    TCHAR szPath [_MAX_DRIVE + _MAX_DIR + _MAX_FNAME];
    TCHAR szDir [_MAX_DIR];
    TCHAR szName [_MAX_FNAME];
    MySplitPath(szFilePath, szPath, szDir, szName, NULL);
    lstrcat(szPath, szDir);
    lstrcat(szPath, szName);
    return CString(szPath);
    }

// Get the extension of a file path.
//
CString GetExtension(const TCHAR* szFilePath)
    {
    TCHAR szExt [_MAX_EXT];
    MySplitPath(szFilePath, NULL, NULL, NULL, szExt);
    return CString(szExt);
    }

// Get the name of a file path.
//
CString GetName(const TCHAR* szFilePath)
    {
    TCHAR szName [_MAX_FNAME];
    MySplitPath(szFilePath, NULL, NULL, szName, NULL);
    return CString(szName);
    }


// Return the path to szFilePath relative to szDirectory.  (E.g. if szFilePath
// is "C:\FOO\BAR\CDR.CAR" and szDirectory is "C:\FOO", then "BAR\CDR.CAR"
// is returned.  This will never use '..'; if szFilePath is not in szDirectory
// or a sub-directory, then szFilePath is returned unchanged.
// If szDirectory is NULL, the current directory is used.
//
CString GetRelativeName(const TCHAR* szFilePath, const TCHAR* szDirectory /*= NULL*/)
    {
    CString strDir;

    if ( szDirectory == NULL )
        {
        GetCurrentDirectory(_MAX_DIR, strDir.GetBuffer(_MAX_DIR) );
        strDir.ReleaseBuffer();
        strDir += (TCHAR)TEXT('\\');
        szDirectory = strDir;
        }

    int cchDirectory = lstrlen(szDirectory);
    if (_tcsnicmp(szFilePath, szDirectory, cchDirectory) == 0)
        return CString(szFilePath + cchDirectory);
    else if ( szFilePath[0] == szDirectory[0] &&
              szFilePath[1] == TEXT(':') && szDirectory[1] == TEXT(':') )    // Remove drive if same.
        return CString(szFilePath + 2);

    return CString(szFilePath);
    }
#if 0
/////////////////////////////////////////////////////////////////////////////
//  Taken from windows system code.  Contains intl support.
/* Returns: 0x00 if no matching char,
 *      0x01 if menmonic char is matching,
 *      0x80 if first char is matching
 */

#define CH_PREFIX TEXT('&')

int FindMnemChar(LPTSTR lpstr, TCHAR ch, BOOL fFirst, BOOL fPrefix)

    {
    register TCHAR chc;
    register TCHAR chnext;
    TCHAR      chFirst;

    while (*lpstr == TEXT(' '))
        lpstr++;

    ch = (TCHAR)(DWORD)CharLower((LPTSTR)(DWORD)(BYTE)ch);
    chFirst = (TCHAR)(DWORD)CharLower((LPTSTR)(DWORD)(BYTE)(*lpstr));

    #ifndef DBCS
    if (fPrefix)
        {
        while (chc = *lpstr++)
            {
            if (((TCHAR)(DWORD)CharLower((LPTSTR)(DWORD)(BYTE)chc) == CH_PREFIX))
                {
                chnext = (TCHAR)(DWORD)CharLower((LPTSTR)(DWORD)(BYTE)*lpstr);

                if (chnext == CH_PREFIX)
                    lpstr++;
                  else
                      if (chnext == ch)
                          return(0x01);
                      else
                          {
                          return(0x00);
                            }
                }
            }
        }
    #else
    #ifdef JAPAN
    if (fPrefix)
        {
        WORD wvch, xvkey;

        // get OEM-dependent virtual key code
        if ((wvch = VkKeyScan((BYTE)ch)) != -1)
        wvch &= 0xFF;

        while (chc = *lpstr++)
            {
            if (IsDBCSLeadByte(chc))
                {
                lpstr++;
                continue;
                }

            if ( (chc == CH_PREFIX) ||
               (KanjiMenuMode == KMM_ENGLISH && chc == CH_ENGLISHPREFIX) ||
               (KanjiMenuMode == KMM_KANJI   && chc == CH_KANJIPREFIX))
                {
                chnext = (TCHAR)CharLower((LPTSTR)(DWORD)(BYTE)*lpstr);

                if (chnext == CH_PREFIX)
                    lpstr++;
                else
                    if (chnext == ch)
                        return(0x01);

                // Compare should be done with virtual key in Kanji menu mode
                // in order to accept Digit shortcut key and save English
                // windows applications!
                xvkey = VkKeyScan((BYTE)chnext);

                if (xvkey != 0xFFFF && (xvkey & 0xFF) == wvch)
                    return(0x01);
                else
                    return(0x00);
                }
            }
        }
    #else
    #ifdef KOREA
    if( fPrefix )
        {
        WORD  wHangeul;
        register TCHAR  chnext2;

        if( KanjiMenuMode != KMM_KANJI )
            {
            while (chc = *lpstr++)
                {
                if (IsDBCSLeadByte(chc))
                    {
                    lpstr++;
                    continue;
                    }
                if ( (chc == CH_PREFIX) ||
                     (KanjiMenuMode == KMM_ENGLISH && chc == CH_ENGLISHPREFIX))
                    {
                    chnext = (TCHAR)CharLower((LPTSTR)(DWORD)(BYTE)*lpstr);

                    if (chnext == CH_PREFIX)
                        lpstr++;
                    else
                        if (chnext == ch)
                            return(0x01);
                        else
                            return(0x00);
                    }
                }
            }
        else
            { //KMM_KANJI
            if( ch >= TEXT('0') && ch <= TEXT('9') )
                wHangeul = 0x0a3b0 | ( (BYTE)ch & 0x0f );   // junja 0 + offset
            else
                if( ch >= TEXT('a') && ch <= TEXT('z') )
                    wHangeul = TranslateHangeul( ch );
                else
                    return(0x00);

            while (chc = *lpstr++)
                {
                if (IsDBCSLeadByte(chc))
                    {
                    lpstr++;
                    continue;
                    }
                if(chc == CH_KANJIPREFIX)
                    {
                    chnext = *lpstr++;
                    chnext2 = *lpstr;

                    if(chnext == HIBYTE(wHangeul) && chnext2 == LOBYTE(wHangeul))
                        return(0x01);
                    else
                        return(0x00);
                    }
                }
            }
    #endif  //KOREA
    #endif  //JAPAN
    #endif  //!DBCS

    if (fFirst && (ch == chFirst))
        return(0x80);

    return(0x00);
    }

#endif
