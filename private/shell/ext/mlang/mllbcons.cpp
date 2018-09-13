// MLLBCons.cpp : Implementation of CMLLBCons
#include "private.h"
#include "mllbcons.h"
#ifdef ASTRIMPL
#include "mlswalk.h"
#endif
#include "mlstrbuf.h"

/////////////////////////////////////////////////////////////////////////////
// Line Break Character Table

const WCHAR awchNonBreakingAtLineEnd[] = {
    0x0028, // LEFT PARENTHESIS
    0x005B, // LEFT SQUARE BRACKET
    0x007B, // LEFT CURLY BRACKET
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x2045, // LEFT SQUARE BRACKET WITH QUILL
    0x207D, // SUPERSCRIPT LEFT PARENTHESIS
    0x208D, // SUBSCRIPT LEFT PARENTHESIS
    0x226A, // MUCH LESS THAN
    0x3008, // LEFT ANGLE BRACKET
    0x300A, // LEFT DOUBLE ANGLE BRACKET
    0x300C, // LEFT CORNER BRACKET
    0x300E, // LEFT WHITE CORNER BRACKET
    0x3010, // LEFT BLACK LENTICULAR BRACKET
    0x3014, // LEFT TORTOISE SHELL BRACKET
    0x3016, // LEFT WHITE LENTICULAR BRACKET
    0x3018, // LEFT WHITE TORTOISE SHELL BRACKET
    0x301A, // LEFT WHITE SQUARE BRACKET
    0x301D, // REVERSED DOUBLE PRIME QUOTATION MARK
    0xFD3E, // ORNATE LEFT PARENTHESIS
    0xFE35, // PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS
    0xFE37, // PRESENTATION FORM FOR VERTICAL LEFT CURLY BRACKET
    0xFE39, // PRESENTATION FORM FOR VERTICAL LEFT TORTOISE SHELL BRACKET
    0xFE3B, // PRESENTATION FORM FOR VERTICAL LEFT BLACK LENTICULAR BRACKET
    0xFE3D, // PRESENTATION FORM FOR VERTICAL LEFT DOUBLE ANGLE BRACKET
    0xFE3F, // PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET
    0xFE41, // PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET
    0xFE43, // PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET
    0xFE59, // SMALL LEFT PARENTHESIS
    0xFE5B, // SMALL LEFT CURLY BRACKET
    0xFE5D, // SMALL LEFT TORTOISE SHELL BRACKET
    0xFF08, // FULLWIDTH LEFT PARENTHESIS
    0xFF1C, // FULLWIDTH LESS-THAN SIGN
    0xFF3B, // FULLWIDTH LEFT SQUARE BRACKET
    0xFF5B, // FULLWIDTH LEFT CURLY BRACKET
    0xFF62, // HALFWIDTH LEFT CORNER BRACKET
    0xFFE9  // HALFWIDTH LEFTWARDS ARROW
};

const WCHAR awchNonBreakingAtLineStart[] = {
    0x0029, // RIGHT PARENTHESIS
    0x002D, // HYPHEN
    0x005D, // RIGHT SQUARE BRACKET
    0x007D, // RIGHT CURLY BRACKET
    0x00AD, // OPTIONAL HYPHEN
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x02C7, // CARON
    0x02C9, // MODIFIER LETTER MACRON
    0x055D, // ARMENIAN COMMA
    0x060C, // ARABIC COMMA
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x2016, // DOUBLE VERTICAL LINE
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2022, // BULLET
    0x2025, // TWO DOT LEADER
    0x2026, // HORIZONTAL ELLIPSIS
    0x2027, // HYPHENATION POINT
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x2046, // RIGHT SQUARE BRACKET WITH QUILL
    0x207E, // SUPERSCRIPT RIGHT PARENTHESIS
    0x208E, // SUBSCRIPT RIGHT PARENTHESIS
    0x226B, // MUCH GREATER THAN
    0x2574, // BOX DRAWINGS LIGHT LEFT
    0x3001, // IDEOGRAPHIC COMMA
    0x3002, // IDEOGRAPHIC FULL STOP
    0x3003, // DITTO MARK
    0x3005, // IDEOGRAPHIC ITERATION MARK
    0x3009, // RIGHT ANGLE BRACKET
    0x300B, // RIGHT DOUBLE ANGLE BRACKET
    0x300D, // RIGHT CORNER BRACKET
    0x300F, // RIGHT WHITE CORNER BRACKET
    0x3011, // RIGHT BLACK LENTICULAR BRACKET
    0x3015, // RIGHT TORTOISE SHELL BRACKET
    0x3017, // RIGHT WHITE LENTICULAR BRACKET
    0x3019, // RIGHT WHITE TORTOISE SHELL BRACKET
    0x301B, // RIGHT WHITE SQUARE BRACKET
    0x301E, // DOUBLE PRIME QUOTATION MARK
    0x3041, // HIRAGANA LETTER SMALL A
    0x3043, // HIRAGANA LETTER SMALL I
    0x3045, // HIRAGANA LETTER SMALL U
    0x3047, // HIRAGANA LETTER SMALL E
    0x3049, // HIRAGANA LETTER SMALL O
    0x3063, // HIRAGANA LETTER SMALL TU
    0x3083, // HIRAGANA LETTER SMALL YA
    0x3085, // HIRAGANA LETTER SMALL YU
    0x3087, // HIRAGANA LETTER SMALL YO
    0x308E, // HIRAGANA LETTER SMALL WA
    0x309B, // KATAKANA-HIRAGANA VOICED SOUND MARK
    0x309C, // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
    0x309D, // HIRAGANA ITERATION MARK
    0x309E, // HIRAGANA VOICED ITERATION MARK
    0x30A1, // KATAKANA LETTER SMALL A
    0x30A3, // KATAKANA LETTER SMALL I
    0x30A5, // KATAKANA LETTER SMALL U
    0x30A7, // KATAKANA LETTER SMALL E
    0x30A9, // KATAKANA LETTER SMALL O
    0x30C3, // KATAKANA LETTER SMALL TU
    0x30E3, // KATAKANA LETTER SMALL YA
    0x30E5, // KATAKANA LETTER SMALL YU
    0x30E7, // KATAKANA LETTER SMALL YO
    0x30EE, // KATAKANA LETTER SMALL WA
    0x30F5, // KATAKANA LETTER SMALL KA
    0x30F6, // KATAKANA LETTER SMALL KE
    0x30FC, // KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0x30FD, // KATAKANA ITERATION MARK
    0x30FE, // KATAKANA VOICED ITERATION MARK
    0xFD3F, // ORNATE RIGHT PARENTHESIS
    0xFE30, // VERTICAL TWO DOT LEADER
    0xFE31, // VERTICAL EM DASH
    0xFE33, // VERTICAL LOW LINE
    0xFE34, // VERTICAL WAVY LOW LINE
    0xFE36, // PRESENTATION FORM FOR VERTICAL RIGHT PARENTHESIS
    0xFE38, // PRESENTATION FORM FOR VERTICAL RIGHT CURLY BRACKET
    0xFE3A, // PRESENTATION FORM FOR VERTICAL RIGHT TORTOISE SHELL BRACKET
    0xFE3C, // PRESENTATION FORM FOR VERTICAL RIGHT BLACK LENTICULAR BRACKET
    0xFE3E, // PRESENTATION FORM FOR VERTICAL RIGHT DOUBLE ANGLE BRACKET
    0xFE40, // PRESENTATION FORM FOR VERTICAL RIGHT ANGLE BRACKET
    0xFE42, // PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET
    0xFE44, // PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET
    0xFE4F, // WAVY LOW LINE
    0xFE50, // SMALL COMMA
    0xFE51, // SMALL IDEOGRAPHIC COMMA
    0xFE5A, // SMALL RIGHT PARENTHESIS
    0xFE5C, // SMALL RIGHT CURLY BRACKET
    0xFE5E, // SMALL RIGHT TORTOISE SHELL BRACKET
    0xFF09, // FULLWIDTH RIGHT PARENTHESIS
    0xFF0C, // FULLWIDTH COMMA
    0xFF0E, // FULLWIDTH FULL STOP
    0xFF1E, // FULLWIDTH GREATER-THAN SIGN
    0xFF3D, // FULLWIDTH RIGHT SQUARE BRACKET
    0xFF40, // FULLWIDTH GRAVE ACCENT
    0xFF5C, // FULLWIDTH VERTICAL LINE
    0xFF5D, // FULLWIDTH RIGHT CURLY BRACKET
    0xFF5E, // FULLWIDTH TILDE
    0xFF61, // HALFWIDTH IDEOGRAPHIC FULL STOP
    0xFF63, // HALFWIDTH RIGHT CORNER BRACKET
    0xFF64, // HALFWIDTH IDEOGRAPHIC COMMA
    0xFF67, // HALFWIDTH KATAKANA LETTER SMALL A
    0xFF68, // HALFWIDTH KATAKANA LETTER SMALL I
    0xFF69, // HALFWIDTH KATAKANA LETTER SMALL U
    0xFF6A, // HALFWIDTH KATAKANA LETTER SMALL E
    0xFF6B, // HALFWIDTH KATAKANA LETTER SMALL O
    0xFF6C, // HALFWIDTH KATAKANA LETTER SMALL YA
    0xFF6D, // HALFWIDTH KATAKANA LETTER SMALL YU
    0xFF6E, // HALFWIDTH KATAKANA LETTER SMALL YO
    0xFF6F, // HALFWIDTH KATAKANA LETTER SMALL TU
    0xFF70, // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0xFF9E, // HALFWIDTH KATAKANA VOICED SOUND MARK
    0xFF9F, // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
    0xFFEB  // HALFWIDTH RIGHTWARDS ARROW
};

const WCHAR awchRomanInterWordSpace[] = {
    0x0009, // TAB
    0x0020, // SPACE
    0x2002, // EN SPACE
    0x2003, // EM SPACE
    0x2004, // THREE-PER-EM SPACE
    0x2005, // FOUR-PER-EM SPACE
    0x2006, // SIX-PER-EM SPACE
    0x2007, // FIGURE SPACE
    0x2008, // PUNCTUATION SPACE
    0x2009, // THIN SPACE
    0x200A, // HAIR SPACE
    0x200B  // ZERO WIDTH SPACE
};

BOOL ScanWChar(const WCHAR awch[], int nArraySize, WCHAR wch)
{
    int iMin = 0;
    int iMax = nArraySize - 1;

    while (iMax - iMin >= 2)
    {
        int iTry = (iMax + iMin + 1) / 2;
        if (wch < awch[iTry])
            iMax = iTry;
        else if  (wch > awch[iTry])
            iMin = iTry;
        else
            return TRUE;
    }

    return (wch == awch[iMin] || wch == awch[iMax]);
}

#ifdef MLLBCONS_DEBUG
void TestTable(const WCHAR awch[], int nArraySize)
{
    int nDummy;

    for (int i = 0; i < nArraySize - 1; i++)
    {
        if (awch[i] >= awch[i + 1])
            nDummy = 0;
    }

    int cFound = 0;
    for (int n = 0; n < 0x10000; n++)
    {
        if (ScanWChar(awch, nArraySize, n))
        {
            cFound++;
            for (i = 0; i < nArraySize; i++)
            {
                if (awch[i] == n)
                    break;
            }
            ASSERT(i < nArraySize);
        }
    }
    ASSERT(cFound == nArraySize);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMLLBCons

STDMETHODIMP CMLLBCons::BreakLineML(IMLangString* pSrcMLStr, long lSrcPos, long lSrcLen, long cMinColumns, long cMaxColumns, long* plLineLen, long* plSkipLen)
{
#ifdef MLLBCONS_DEBUG
    TestTable(awchNonBreakingAtLineEnd, ARRAYSIZE(awchNonBreakingAtLineEnd));
    TestTable(awchNonBreakingAtLineStart, ARRAYSIZE(awchNonBreakingAtLineStart));
    TestTable(awchRomanInterWordSpace, ARRAYSIZE(awchRomanInterWordSpace));
#endif
    ASSERT_THIS;
    ASSERT_READ_PTR(pSrcMLStr);
    ASSERT_WRITE_PTR_OR_NULL(plLineLen);
    ASSERT_WRITE_PTR_OR_NULL(plSkipLen);

    HRESULT hr;
    IMLangStringWStr* pMLStrWStr;
    long lStrLen;
    long lBreakPos;
    long lSkipLen;

    if (SUCCEEDED(hr = pSrcMLStr->QueryInterface(IID_IMLangStringWStr, (void**)&pMLStrWStr)) &&
        SUCCEEDED(hr = pSrcMLStr->GetLength(&lStrLen)) &&
        SUCCEEDED(hr = ::RegularizePosLen(lStrLen, &lSrcPos, &lSrcLen)))
    {
        long cColumns = 0;
#ifndef ASTRIMPL
        long lSrcPosTemp = lSrcPos;
        long lSrcLenTemp = lSrcLen;
#endif
        long lCandPos;
        struct {
            unsigned fDone : 1;
            unsigned fInSpaces : 1;
            unsigned fFEChar : 1;
            unsigned fInFEChar : 1;
            unsigned fBreakByEndOfLine : 1;
            unsigned fNonBreakNext : 1;
            unsigned fHaveCandPos : 1;
            unsigned fSlashR : 1;
        } Flags = {0, 0, 0, 0, 0, 0, 0, 0};
#ifdef ASTRIMPL
        CCharType<CT_CTYPE3, 128> ct3;
        CMLStrWalkW StrWalk(pMLStrWStr, lSrcPos, lSrcLen);
#else

        LCID locale;
        hr = pMLStrWStr->GetLocale(0, -1, &locale, NULL, NULL);
        CCharType<CT_CTYPE3, 128> ct3(locale);
#endif

        lBreakPos = -1; // Break at cMaxColumns if no spaces and FE chars

#ifdef ASTRIMPL
        while (StrWalk.Lock(hr))
        {
                ct3.Flush();

                for (int iCh = 0; iCh < StrWalk.GetCCh(); iCh++)
                {
                    const WCHAR wch = StrWalk.GetStr()[iCh];
                    const WORD wCharType3 = ct3.GetCharType(pSrcMLStr, StrWalk.GetPos() + iCh, StrWalk.GetLen() - iCh, &hr);
                    if (FAILED(hr))
                        break;
#else
        while (lSrcLenTemp > 0 && SUCCEEDED(hr))
        {
            WCHAR* pszBuf;
            long cchBuf;
            long lLockedLen;

            ct3.Flush();

            if (SUCCEEDED(hr = pMLStrWStr->LockWStr(lSrcPosTemp, lSrcLenTemp, MLSTR_READ, 0, &pszBuf, &cchBuf, &lLockedLen)))
            {
                for (int iCh = 0; iCh < cchBuf; iCh++)
                {
                    const WCHAR wch = pszBuf[iCh];
                    const WORD wCharType3 = ct3.GetCharType(pszBuf + iCh, cchBuf - iCh);
#endif
                    const int nWidth = (wCharType3 & C3_HALFWIDTH) ? 1 : 2;

                    if (wch == L'\r' && !Flags.fSlashR)
                    {
                        Flags.fSlashR = TRUE;
                    }
                    else if (wch == L'\n' || Flags.fSlashR) // End of line
                    {
                        Flags.fDone = TRUE;
                        Flags.fBreakByEndOfLine = TRUE;
                        if (Flags.fInSpaces)
                        {
                            Flags.fHaveCandPos = FALSE;
                            lBreakPos = lCandPos;
                            lSkipLen++; // Skip spaces and line break character
                        }
                        else
                        {
#ifdef ASTRIMPL
                            lBreakPos = StrWalk.GetPos() + iCh; // Break at right before the end of line
#else
                            lBreakPos = lSrcPosTemp + iCh; // Break at right before the end of line
#endif
                            if (Flags.fSlashR)
                                lBreakPos--;

                            lSkipLen = 1; // Skip line break character
                        }
                        if (wch == L'\n' && Flags.fSlashR)
                            lSkipLen++;
                        break;
                    }
                    else if (ScanWChar(awchRomanInterWordSpace, ARRAYSIZE(awchRomanInterWordSpace), wch)) // Spaces
                    {
                        if (!Flags.fInSpaces && !Flags.fNonBreakNext)
                        {
                            Flags.fHaveCandPos = TRUE;
#ifdef ASTRIMPL
                            lCandPos = StrWalk.GetPos() + iCh; // Break at right before the spaces
#else
                            lCandPos = lSrcPosTemp + iCh; // Break at right before the spaces
#endif
                            lSkipLen = 0;
                        }
                        Flags.fInSpaces = TRUE;
                        lSkipLen++; // Skip continuous spaces after breaking
                    }
                    else // Other characters
                    {
                        Flags.fFEChar = ((wCharType3 & (C3_KATAKANA | C3_HIRAGANA | C3_FULLWIDTH | C3_IDEOGRAPH)) != 0);

                        if ((Flags.fFEChar || Flags.fInFEChar) && !Flags.fNonBreakNext && !Flags.fInSpaces)
                        {
                            Flags.fHaveCandPos = TRUE;
#ifdef ASTRIMPL
                            lCandPos = StrWalk.GetPos() + iCh; // Break at right before or after the FE char
#else
                            lCandPos = lSrcPosTemp + iCh; // Break at right before or after the FE char
#endif
                            lSkipLen = 0;
                        }
                        Flags.fInFEChar = Flags.fFEChar;
                        Flags.fInSpaces = FALSE;

                        if (Flags.fHaveCandPos)
                        {
                            Flags.fHaveCandPos = FALSE;
                            if (!ScanWChar(awchNonBreakingAtLineStart, ARRAYSIZE(awchNonBreakingAtLineStart), wch))
                                lBreakPos = lCandPos;
                        }

                        if (cColumns + nWidth > cMaxColumns)
                        {
                            Flags.fDone = TRUE;
                            break;
                        }

                        Flags.fNonBreakNext = ScanWChar(awchNonBreakingAtLineEnd, ARRAYSIZE(awchNonBreakingAtLineEnd), wch);
                    }

                    cColumns += nWidth;
                }
#ifdef ASTRIMPL
                StrWalk.Unlock(hr);

                if (Flags.fDone && SUCCEEDED(hr))
                    break;
#else
                HRESULT hrTemp = pMLStrWStr->UnlockWStr(pszBuf, 0, NULL, NULL);
                if (FAILED(hrTemp) && SUCCEEDED(hr))
                    hr = hrTemp;

                if (Flags.fDone && SUCCEEDED(hr))
                    break;

                lSrcPosTemp += lLockedLen;
                lSrcLenTemp -= lLockedLen;
            }
#endif
        }

        pMLStrWStr->Release();

        if (Flags.fHaveCandPos)
            lBreakPos = lCandPos;

        if (SUCCEEDED(hr) && !Flags.fBreakByEndOfLine && lBreakPos - lSrcPos < cMinColumns)
        {
            lBreakPos = min(lSrcLen, cMaxColumns) + lSrcPos; // Default breaking
            lSkipLen = 0;
        }

        if (SUCCEEDED(hr) && !Flags.fDone)
        {
            if (Flags.fInSpaces)
            {
                lBreakPos = lSrcLen - lSkipLen;
            }
            else
            {
                lBreakPos = lSrcLen;
                lSkipLen = 0;
            }
            if (Flags.fSlashR)
            {
                lBreakPos--;
                lSkipLen++;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (plLineLen)
            *plLineLen = lBreakPos - lSrcPos;
        if (plSkipLen)
            *plSkipLen = lSkipLen;
    }
    else
    {
        if (plLineLen)
            *plLineLen = 0;
        if (plSkipLen)
            *plSkipLen = 0;
    }

    return hr;
}

STDMETHODIMP CMLLBCons::BreakLineW(LCID locale, const WCHAR* pszSrc, long cchSrc, long lMaxColumns, long* pcchLine, long* pcchSkip)
{
    ASSERT_THIS;
	ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchLine);
    ASSERT_WRITE_PTR_OR_NULL(pcchSkip);

    HRESULT hr = S_OK;
    IMLangStringWStr* pMLStrW;

    if (SUCCEEDED(hr = PrepareMLStrClass()) &&
        SUCCEEDED(hr = m_pMLStrClass->CreateInstance(NULL, IID_IMLangStringWStr, (void**)&pMLStrW)))
    {
        CMLStrBufConstStackW StrBuf((LPWSTR)pszSrc, cchSrc);
        long lLineLen;
        long lSkipLen;
    
        hr = pMLStrW->SetStrBufW(0, -1, &StrBuf, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pMLStrW->SetLocale(0, -1, locale);

        if (SUCCEEDED(hr))
            hr = BreakLineML(pMLStrW, 0, -1, 0, lMaxColumns, (pcchLine || pcchSkip) ? &lLineLen : NULL, (pcchSkip) ? &lSkipLen : NULL);

        if (SUCCEEDED(hr) && pcchLine)
            hr = pMLStrW->GetWStr(0, lLineLen, NULL, 0, pcchLine, NULL);

        if (SUCCEEDED(hr) && pcchSkip)
            hr = pMLStrW->GetWStr(lLineLen, lSkipLen, NULL, 0, pcchSkip, NULL);

        pMLStrW->Release();
    }

    if (FAILED(hr))
    {
        if (pcchLine)
            *pcchLine = 0;
        if (pcchSkip)
            *pcchSkip = 0;
    }

    return hr;
}

STDMETHODIMP CMLLBCons::BreakLineA(LCID locale, UINT uCodePage, const CHAR* pszSrc, long cchSrc, long lMaxColumns, long* pcchLine, long* pcchSkip)
{
    ASSERT_THIS;
	ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchLine);
    ASSERT_WRITE_PTR_OR_NULL(pcchSkip);

    HRESULT hr = S_OK;
    IMLangStringAStr* pMLStrA;

    if (uCodePage == 50000)
        uCodePage = 1252;

    if (SUCCEEDED(hr = PrepareMLStrClass()) &&
        SUCCEEDED(hr = m_pMLStrClass->CreateInstance(NULL, IID_IMLangStringAStr, (void**)&pMLStrA)))
    {
        CMLStrBufConstStackA StrBuf((LPSTR)pszSrc, cchSrc);
        long lLineLen;
        long lSkipLen;
    
        hr = pMLStrA->SetStrBufA(0, -1, uCodePage, &StrBuf, NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pMLStrA->SetLocale(0, -1, locale);

        if (SUCCEEDED(hr))
            hr = BreakLineML(pMLStrA, 0, -1, 0, lMaxColumns, (pcchLine || pcchSkip) ? &lLineLen : NULL, (pcchSkip) ? &lSkipLen : NULL);

        if (SUCCEEDED(hr) && pcchLine)
            hr = pMLStrA->GetAStr(0, lLineLen, uCodePage, NULL, NULL, 0, pcchLine, NULL);

        if (SUCCEEDED(hr) && pcchSkip)
            hr = pMLStrA->GetAStr(lLineLen, lSkipLen, uCodePage, NULL, NULL, 0, pcchSkip, NULL);

        pMLStrA->Release();
    }

    if (FAILED(hr))
    {
        if (pcchLine)
            *pcchLine = 0;
        if (pcchSkip)
            *pcchSkip = 0;
    }

    return hr;
}
