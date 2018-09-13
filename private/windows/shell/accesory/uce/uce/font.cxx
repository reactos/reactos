/****************************************************************************

    font.c

    PURPOSE: manage fonts for UCE

    Copyright (c) 1997-1999 Microsoft Corporation.
****************************************************************************/

#include "windows.h"
#include "commctrl.h"

#include "UCE.h"
#include "stdlib.h"
#include "tchar.h"
#include "stdio.h"
#include "winuser.h"
#include "string.h"
#include "search.h"
#include "getuname.h"

#include "winnls.h"
#include "wingdi.h"

#define tag_CharToIndexMap      0x636d6170        /* 'cmap' */

#define FS_2BYTE(p)  ( ((unsigned short)((p)[0]) << 8) |  (p)[1])
#define FS_4BYTE(p)  ( FS_2BYTE((p)+2) | ( (FS_2BYTE(p)+0L) << 16) )

#define SWAPW(a)        ((short) FS_2BYTE( (unsigned char *)(&a) ))
#define SWAPL(a)        ((long) FS_4BYTE( (unsigned char *)(&a) ))

FONTINFO *Font_pList  = NULL;
INT       Font_nCount = 0;

//
// kd.c
//

/****************************************************************************

  Font_EnumProc

****************************************************************************/
INT APIENTRY
Font_EnumProc(
    LPLOGFONT lpLogFont,
    NEWTEXTMETRICEX *lpTextMetric,
    int    nFontType,
    LPARAM hWnd)
{
    DWORD   ntmFlags = lpTextMetric->ntmTm.ntmFlags;
    DWORD   FontType = 0;
    URANGE *pUniRange;
    DWORD   dwSize = 0;

    // excludes FE @ font
    if(lpLogFont->lfFaceName[0] == L'@') return (1);

//OEM FONT, BITMAP FONT will be treated as SYMBOL FONT
/* 
    if(lpLogFont->lfCharSet == OEM_CHARSET)
    {
        if(lstrcmpi(lpLogFont->lfFaceName, L"Terminal") == 0)
        {
            FontType = OEM_FONTTYPE;
        }
    }
*/

    if (ntmFlags & NTM_PS_OPENTYPE)
    {
        FontType = PS_OPENTYPE_FONTTYPE;
    }
    else if (ntmFlags & NTM_TYPE1)
    {
        FontType = TYPE1_FONTTYPE;
    }
    else if (nFontType & TRUETYPE_FONT)
    {
        if (ntmFlags & NTM_TT_OPENTYPE)
            FontType = TT_OPENTYPE_FONTTYPE;
        else
            FontType = TRUETYPE_FONT;
    }
    else
    {
        FontType |= SYMBOL_FONTTYPE;
    }

    if(lpLogFont->lfCharSet == SYMBOL_CHARSET)
    {
        FontType |= SYMBOL_FONTTYPE;
    }

//
//SYMBOL/OEM/BITMAP FONT get the ANSI code points, we have to
//use ConvertAnsifontToUnicode later when we display them.
//
    if(FontType & SYMBOL_FONTTYPE)
    {
        pUniRange = (URANGE *) malloc (sizeof(URANGE));
        if (pUniRange)
        {
            pUniRange[0].wcFrom = 0x0021;
            pUniRange[0].wcTo   = 0x00FF;
            dwSize = 1;
        }
        else
        {
            //
            // error message, unicode range error.
            //
        }

    }
/* treat OEM/BITMAP as SYMBOL  
    else if(FontType & OEM_FONTTYPE)
    {
        dwSize = URanges(CP_OEMCP, NULL);
        pUniRange = (URANGE *) malloc (sizeof(URANGE) * dwSize);
        if (pUniRange)
        {
            URanges(CP_OEMCP, pUniRange);
        }
        else
        {
            //
            // error message, unicode range error.
            //
        }
    }
    else if(FontType == 0)                                 // BitMap font
    {
        UINT cp = 0;

        if(lpLogFont->lfCharSet != OEM_CHARSET)
        {
           cp = CharSetToCodePage(lpLogFont->lfCharSet);
        }

        dwSize = URanges(cp, NULL);
        pUniRange = (URANGE *) malloc (sizeof(URANGE) * dwSize);
        if (pUniRange)
        {
            URanges(cp, pUniRange);
        }
        else
        {
            //
            // error message, unicode range error.
            //
        }
    }
*/
    else if((FontType &  PS_OPENTYPE_FONTTYPE) ||
            (FontType &  TYPE1_FONTTYPE))
    {
        pUniRange = (URANGE *) malloc (sizeof(URANGE));
        if (pUniRange)
        {
            pUniRange[0].wcFrom = 0x0021;
            pUniRange[0].wcTo   = 0xFFFF;
            dwSize = 1;
        }
        else
        {
            //
            // error message, unicode range error.
            //
        }
    }
    else    // TT_OPENTYPE_FONTTYPE || TRUETYPE_FONT
    {
        HFONT  hFontOld;
        LPVOID lpvBuffer = NULL;
        LPBYTE lp, lp1, lp2;
        DWORD  cbData;
        DWORD  dwTag = tag_CharToIndexMap;
        WORD   Num, i;
        HDC    hdc = GetWindowDC((HWND)hWnd);

        HFONT hFont = CreateFontIndirect(lpLogFont);
        hFontOld = (HFONT) SelectObject(hdc, hFont);
        dwTag = SWAPL(dwTag);
        if(!(cbData = GetFontData(hdc, dwTag, 0, NULL, 0)))
        {
            goto Close;
        }
        if(!(lpvBuffer = (LPVOID) malloc (cbData)))
        {
            goto Close;
        }
        GetFontData(hdc, dwTag, 0, lpvBuffer, cbData);

        Num = TWO_BYTE_NUM(((CMAP_HEAD*)lpvBuffer)->NumTables);
        lp1 = (BYTE*) ((BYTE*)lpvBuffer + sizeof(CMAP_HEAD));

        while(Num >0)
        {
          if(TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Platform) == MICROSOFT_PLATFORM)
//           TWO_BYTE_NUM(((CMAP_TABLE*)lp1)->Encoding) == UNICODE_INDEXING
          {
             lp = (BYTE*)
                  lpvBuffer
                  + FOUR_BYTE_NUM(((CMAP_TABLE*)lp1)->Offset);

             if(TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->Format) == CMAP_FORMAT_FOUR)
             {
                break;
             }
          }
          Num--;
          lp1 += sizeof(CMAP_TABLE);
        }

        if(Num == 0)                   // can't find Platform:3/Encoding:1 (Unicode)
        {
            goto Close;
        }

        Num  = (TWO_BYTE_NUM(((CMAP_FORMAT*)lp)->SegCountX2));
        dwSize = Num>>1;
        pUniRange = (URANGE *) malloc (sizeof(URANGE)*dwSize);
        if (!pUniRange)
        {
            goto Close;
        }

        lp2  = lp  + sizeof(CMAP_FORMAT);        // lp2 -> first WCHAR of wcTo
        lp1  = lp2 + Num + 2;                    // lp1 -> first WCHAR of wcFrom
        for(i = 0; i < Num; i++, i++)
        {
            pUniRange[i>>1].wcFrom = TWO_BYTE_NUM((lp1+i));
            pUniRange[i>>1].wcTo   = TWO_BYTE_NUM((lp2+i));
        }

        //
        // Some fonts put glyphs in U+F000-F0FF,
        // It should be marked as a Symbol font as well.
        // For example, Guttman Adi (a Hebrew font from Office).
        //

        if( dwSize <= 2 &&
           (pUniRange[0].wcFrom >= 0xF000) &&
           (pUniRange[0].wcFrom <= 0xF0FF) )
        {
            if (pUniRange[0].wcFrom  < 0xF021)
            {
                pUniRange[0].wcFrom  = 0x0021;
            }
            else
            {
                pUniRange[0].wcFrom &= 0x00FF;
            }

            pUniRange[0].wcFrom &= 0x00FF;
            pUniRange[0].wcTo   &= 0x00FF;
            if(pUniRange[0].wcFrom < 0x0021)
               pUniRange[0].wcFrom = 0x0021;
            FontType |= SYMBOL_FONTTYPE;
        }

Close:
        DeleteObject(hFont);
        if(lpvBuffer) free(lpvBuffer);
        SelectObject(hdc, hFontOld);
        ReleaseDC((HWND)hWnd, hdc);
        if(dwSize == 0) return (1);
    }

    if (!Font_AddToList(lpLogFont, FontType, pUniRange, dwSize))
    {
        //
        // error message, add data error
        //
    }

    return (1);
}

/****************************************************************************

  EUDC_Fonts

****************************************************************************/
BOOL
EUDC_Fonts(LPLOGFONT lpLogFont)
{
    TCHAR EudcReg[] = TEXT("EUDC");
    TCHAR szFaceName[LF_EUDCFACESIZE];
    DWORD dwValue;
    TCHAR szFontFile[MAX_PATH];
    TCHAR szExpandedFontFile[MAX_PATH];
    DWORD dwData;
    DWORD dwType;
    LONG  Success;
    HKEY  hKey;
    int   i;

    DWORD    dwSize;
    URANGE  *pUniRange;

    HKEY  hKeyEUDC;
//    DWORD dwIndex;
    BOOL  bEUDC_TTE = FALSE;          // include EUDC.TTE only once

    Success = RegOpenKeyEx(
                  HKEY_CURRENT_USER,
                  EudcReg,
                  0,
                  KEY_READ,
                  &hKeyEUDC);
    if(Success != ERROR_SUCCESS) return FALSE;

// only display fonts of the current codepage.
//    for (dwIndex=0; ;dwIndex++)
    {
        TCHAR    szName[8];
        DWORD    cbName = sizeof(szName);
        wsprintf(szName,TEXT("%d"),GetACP());
        /*
        Success = RegEnumKeyEx( hKeyEUDC, dwIndex,
                                szName, &cbName,
                                NULL, NULL, NULL,
                                &ft );
        if(Success != ERROR_SUCCESS)
        {
          RegCloseKey(hKeyEUDC);
          return TRUE;
          //  break;
        }
        */ 
        Success = RegOpenKeyEx( hKeyEUDC,
                                szName,
                                0,
                                KEY_READ,
                                &hKey );
        if(Success != ERROR_SUCCESS)
        {
          RegCloseKey(hKeyEUDC);
          return TRUE;
          //  break;
        }

        for (i=0; ;i++)
        {
            dwValue = sizeof(szFaceName);
            dwData  = sizeof(szFontFile);

            Success = RegEnumValue(
                          hKey,
                          i,
                          szFaceName,
                          &dwValue,
                          NULL,
                          &dwType,
                          (LPBYTE)szFontFile,
                          &dwData
                      );
            if(Success != ERROR_SUCCESS)
            {
                break;
            }

            if(lstrcmpi(szFaceName, L"SystemDefaultEUDCFont") == 0)
            {
                if(bEUDC_TTE) continue;
                bEUDC_TTE = TRUE;
            }

            ExpandEnvironmentStrings(szFontFile, szExpandedFontFile, MAX_PATH); 
            pUniRange = EUDC_Range(szExpandedFontFile, &dwSize);

            if (!pUniRange) 
            {
              if((lstrcmpi(szFaceName, L"SystemDefaultEUDCFont") == 0) &&
                  bEUDC_TTE)
              bEUDC_TTE = false;
              continue;
            }

            lstrcpy(lpLogFont->lfFaceName, szFaceName);
            if (!Font_AddToList(lpLogFont, EUDC_FONTTYPE | TRUETYPE_FONT,
                                pUniRange, dwSize))
            {
                //
                // error message, add data error
                //
            }
        }
        RegCloseKey(hKey);
    }

    RegCloseKey(hKeyEUDC);
    return TRUE;
}

/****************************************************************************

    Function : Delete this list.

****************************************************************************/
BOOL
Font_DeleteList()
{
    INT i;

    for (i=0; i<Font_nCount; i++)
    {
        if (Font_pList[i].pUniRange)
        {
            free(Font_pList[i].pUniRange);
            Font_pList[i].pUniRange = NULL;
        }
    }
    free(Font_pList);
    Font_pList  = NULL;
    Font_nCount = 0;
    return TRUE;
}

/****************************************************************************

    Function : Font_InitList

****************************************************************************/
BOOL
Font_InitList(HWND hWnd)
{
    LOGFONT lf;
    HDC     hDC;

    if (Font_pList)
    {
        Font_DeleteList();
    }

    Font_pList  = NULL;
    Font_nCount = 0;

    hDC = GetWindowDC(hWnd);

    lf.lfCharSet = 1;
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC) Font_EnumProc, (LPARAM) hWnd, 0);
    ReleaseDC(hWnd, hDC);

    EUDC_Fonts(&lf);

    return TRUE;
}

/****************************************************************************

    Function : Font_AddToList

****************************************************************************/
BOOL
Font_AddToList(
    LPLOGFONT lpLogFont,
    DWORD    FontType,
    URANGE  *pUniRange,
    INT     nNumofUniRange
    )
{
    if (Font_pList == NULL)
    {
        Font_pList = (FONTINFO *)  malloc(sizeof(FONTINFO));
        if (Font_pList == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        int i;

        if(!(FontType & EUDC_FONTTYPE))
        {
            for(i = 0; i < Font_nCount; i++)
            {
                if(lstrcmpi(Font_pList[i].szFaceName, lpLogFont->lfFaceName) == 0)
                {
                    if((lpLogFont->lfCharSet >  Font_pList[i].CharSet) &&      // BUGBUG
                       (lpLogFont->lfCharSet == SHIFTJIS_CHARSET    ||
                        lpLogFont->lfCharSet == CHINESEBIG5_CHARSET ||
                        lpLogFont->lfCharSet == GB2312_CHARSET      ||
                        lpLogFont->lfCharSet == HANGEUL_CHARSET       ))
                    {
                        Font_pList[i].CharSet = lpLogFont->lfCharSet;
                    }
                    return FALSE;
                }
            }
        }

        Font_pList = (FONTINFO *) realloc(Font_pList,
                                      sizeof(FONTINFO)*(Font_nCount+1));
        if (Font_pList == NULL)
        {
            return FALSE;
        }
    }

    lstrcpy(Font_pList[Font_nCount].szFaceName, lpLogFont->lfFaceName);
    Font_pList[Font_nCount].CharSet = lpLogFont->lfCharSet;
    Font_pList[Font_nCount].FontType = FontType;
    Font_pList[Font_nCount].PitchAndFamily = FF_DONTCARE;
    Font_pList[Font_nCount].pUniRange = pUniRange;
    Font_pList[Font_nCount].nNumofUniRange = nNumofUniRange;
    Font_nCount++;

    return TRUE;
}

/****************************************************************************

    Function : Font_FillToComboBox

****************************************************************************/
BOOL
Font_FillToComboBox(
    HWND hWnd,
    UINT uID
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  i;
    INT  nIndex;

    if (hCombo == NULL)
    {
        return FALSE;
    }

    SendMessage(
        hCombo,
        CB_RESETCONTENT,
        0,
        0);

    for (i=0; i < Font_nCount; i++)
    {
        WCHAR  wcBuf[LF_EUDCFACESIZE];
				//bug #234220
        WCHAR  wcBuf1[LF_EUDCFACESIZE];

        if(((Font_pList[i].FontType & TRUETYPE_FONT) ||
            (Font_pList[i].FontType & TT_OPENTYPE_FONTTYPE)) &&
            (Font_pList[i].CharSet == SHIFTJIS_CHARSET    ||
             Font_pList[i].CharSet == CHINESEBIG5_CHARSET ||
             Font_pList[i].CharSet == GB2312_CHARSET      ||
             Font_pList[i].CharSet == HANGEUL_CHARSET)      )
        {
             Font_pList[i].FontType |= DBCS_FONTTYPE;
        }

        lstrcpy(wcBuf, Font_pList[i].szFaceName);
        if(Font_pList[i].FontType & EUDC_FONTTYPE)
        {
           if(lstrcmpi(Font_pList[i].szFaceName, L"SystemDefaultEUDCFont") == 0)
           {
              LoadString(hInst, IDS_ALLFONTS, wcBuf, LF_EUDCFACESIZE);
           }

           LoadString(hInst, IDS_EUDC, wcBuf1, LF_EUDCFACESIZE);
           lstrcat(wcBuf, wcBuf1);
        }

        nIndex = (INT)SendMessage(
                     hCombo,
                     CB_ADDSTRING,
                     (WPARAM) 0,
                     (LPARAM) wcBuf);

        if (nIndex != CB_ERR)
        {
            SendMessage(
                hCombo,
                CB_SETITEMDATA,
                (WPARAM) nIndex,
                (LPARAM) i);
        }
    }
    return TRUE;
}

/****************************************************************************

    Function : Font_GetSelFontCharSet

****************************************************************************/
BYTE
Font_GetSelFontCharSet(
    HWND hWnd,
    UINT uID,
    INT  nIndex
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  i;

    if (hCombo == NULL)
    {
        return DEFAULT_CHARSET;
    }

    i = (INT)SendMessage(
            hCombo,
            CB_GETITEMDATA,
            (WPARAM) nIndex,
            (LPARAM) 0);

    if ((i >= 0) && (i < Font_nCount))
    {
        return Font_pList[i].CharSet;
    }
    else
    {
        return DEFAULT_CHARSET;
    }
}


/****************************************************************************

    Function : Font_SelectByCharSet

****************************************************************************/
BOOL
Font_SelectByCharSet(
    HWND hWnd,
    UINT uID,
    UINT CharSet
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  nIndex;
    INT  i;

    if (hCombo == NULL)
    {
        return FALSE;
    }

    //
    // do not swtich font if charset of the current font
    // is the same as CharSet
    //
    nIndex = (INT)SendMessage(
                 hCombo,
                 CB_GETCURSEL,
                 (WPARAM) 0,
                 (LPARAM) 0);

    if( CharSet == Font_GetSelFontCharSet(hWnd, uID, nIndex))
    {
        return FALSE;
    }

    for (i = 0; i < Font_nCount; i++)
    {
        if ((Font_pList[i].CharSet == CharSet)       &&
            (Font_pList[i].FontType & DBCS_FONTTYPE)   )
        {
            break;
        }
    }
    if (i < Font_nCount)
    {
        nIndex = (INT)SendMessage(
                     hCombo,
                     CB_FINDSTRING,
                     (WPARAM) -1,
                     (LPARAM) Font_pList[i].szFaceName);

        if (nIndex == CB_ERR)
        {
            return FALSE;
        }

        nIndex = (INT)SendMessage(
                     hCombo,
                     CB_SETCURSEL,
                     (WPARAM) nIndex,
                     (LPARAM) 0);

        if (nIndex != CB_ERR)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

/****************************************************************************

    Function : Font_IsWithinCodePage

****************************************************************************/
BOOL
Font_IsWithinCodePage(
    UINT   uCodePage,
    WCHAR  wc,
    WORD   *pAnsi
    )
{

    WORD   ctype;

    BOOL bSkip = FALSE;

    GetStringTypeW(CT_CTYPE1, &wc, 1, &ctype);

    if ((!ctype)            ||
        ( ctype & C1_CNTRL) ||
        ( ctype & C1_SPACE) ||
        ( ctype & C1_BLANK)   )
    {
        return FALSE;
    }

    if (uCodePage == UNICODE_CODEPAGE)
    {
        *pAnsi = 0;
        return TRUE;
    }
    else
    {
        WCHAR wc2;

        WideCharToMultiByte(
            uCodePage,
            0,
            &(wc), 1,
            (char*) pAnsi, 2,
            NULL, NULL);

        MultiByteToWideChar(
            uCodePage,
            MB_PRECOMPOSED,
            (char*) pAnsi, 2,
            &wc2, 1);

        return (wc == wc2);
    }
}

/****************************************************************************

    Function : Font_GetCurCMapTable

****************************************************************************/
BOOL
Font_GetCurCMapTable(
    HWND   hWnd,
    UINT   uID,
    URANGE **ppCodeList,
    UINT   *puNum
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  nItemIndex;
    INT  nIndex;

    if (hCombo == NULL)
    {
        return FALSE;
    }

    nIndex = (INT)SendMessage(
                 hCombo,
                 CB_GETCURSEL,
                 (WPARAM) 0,
                 (LPARAM) 0L);

    if (nIndex == CB_ERR)
    {
        return FALSE;
    }

    nItemIndex = (INT)SendMessage(
                     hCombo,
                     CB_GETITEMDATA,
                     (WPARAM) nIndex,
                     (LPARAM) 0);

    if ((nItemIndex < 0) || (nItemIndex >= Font_nCount))
    {
        return FALSE;
    }

    *ppCodeList = Font_pList[nItemIndex].pUniRange;
    *puNum      = Font_pList[nItemIndex].nNumofUniRange;

    return TRUE;
}

/****************************************************************************

    Function : Font_GetCharWidth32

****************************************************************************/
BOOL
Font_GetCharWidth32(
    HDC    hDC,
    UINT   chSymFirst,
    UINT   chSymLast,
    LPINT  lpdxp,
    LPWSTR pCode
    )
{
    UINT i;
    INT  nWidth;

    if (pCode == NULL)
    {
        return FALSE;
    }

    for (i=chSymFirst; i<=chSymLast; i++)
    {
        nWidth = 0;
        GetCharWidth32(
            hDC,
            (pCode[i]),
            (pCode[i]),
            &nWidth
        );
        *lpdxp = nWidth;
        lpdxp++;
    }
    return TRUE;
}

/****************************************************************************

    Function : Font_Avail

****************************************************************************/
BOOL
Font_Avail(
    UINT CharSet
    )
{
    int i;

    for (i = 0; i < Font_nCount; i++)
    {
        if (Font_pList[i].CharSet == CharSet)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/****************************************************************************

    Function : return the first DBCS charset, 0 if none

****************************************************************************/
UINT
Font_DBCS_CharSet()
{
    int i;

    for (i = 0; i < Font_nCount; i++)
    {
        if (Font_pList[i].FontType & DBCS_FONTTYPE)
        {
            return Font_pList[i].CharSet;
        }
    }

    return 0;
}

/****************************************************************************

    Function : Simulate CMAP Unicode ranges for non-TTF fonts

****************************************************************************/
DWORD
URanges(UINT CodePage, URANGE *pUR)
{
    WCHAR wcBuf[128];
    WCHAR wcTmp;
    BYTE  cTmp, cBeg;
    int   nCount, i, j;
    DWORD nUR = 0;

    if(pUR != NULL)
    {
        pUR[nUR].wcFrom = (WCHAR) ASCII_BEG;
        pUR[nUR].wcTo   = (WCHAR) ASCII_END;
    }
    nUR++;

    cBeg   = HIANSI_BEG;
    if(CodePage != CP_OEMCP)
    {
        cBeg += 32;
    }

    nCount = 0;
    for(cTmp = cBeg; cTmp != 0; cTmp++)
    {
        if(MultiByteToWideChar(CodePage, 0, (char*)&cTmp, 1, &wcTmp, 1) == 0 ||
           wcTmp <= 0x009F ||
           (wcTmp >= 0xE000 && wcTmp <= 0xF8FF))
           continue;

        for(i = 0; i < nCount; i++)
        {
            if(wcBuf[i] >= wcTmp)
                break;
        }

        if(i < nCount)
        {
            for(j = nCount; j > i; j--)
            {
                wcBuf[j] = wcBuf[j-1];
            }
        }

        wcBuf[i] = wcTmp;
        nCount++;
    }

    if(nCount > 0)
    {
        i = 0;
        while(1)
        {
            if(i == (nCount-1))
            {
                if(pUR != NULL)
                {
                    pUR[nUR].wcFrom = wcBuf[i];
                    pUR[nUR].wcTo   = wcBuf[i];
                }
                nUR++;
                break;
            }

            for(j = i; j < nCount-1; j++)
            {
                if((wcBuf[j+1] - wcBuf[j]) != 1) break;
            }

            if(pUR != NULL)
            {
                pUR[nUR].wcFrom = wcBuf[i];
                pUR[nUR].wcTo   = wcBuf[j];
            }
            nUR++;

            if(j == (nCount-1))
            {
                break;
            }

            i = j + 1;
        }
    }

    return nUR;
}

/****************************************************************************

    Function : convert CharSet to CodePage

****************************************************************************/
UINT
CharSetToCodePage(BYTE cs)
{
    switch(cs)
    {
        case (OEM_CHARSET):
        case (ANSI_CHARSET):
            return 1252;

        case (SHIFTJIS_CHARSET):
            return 932;

        case (CHINESEBIG5_CHARSET):
            return 950;

        case (HANGEUL_CHARSET):
            return 949;

        case (GB2312_CHARSET):
            return 936;

        case (EASTEUROPE_CHARSET):
            return 1250;

        case (GREEK_CHARSET):
            return 1253;

        case (RUSSIAN_CHARSET):
            return 1251;

        case (TURKISH_CHARSET):
            return 1254;

        case (BALTIC_CHARSET):
            return 1257;

        case (VIETNAMESE_CHARSET):
            return 1258;

        case (THAI_CHARSET):
            return 874;

        case (ARABIC_CHARSET):
            return 1255;

        case (HEBREW_CHARSET):
            return 1256;
    }
    return 0;
}
