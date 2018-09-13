//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       textctx.cxx
//
//  Contents:   Various text parse contexts to deal with space-collapsing
//              and other issues
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

MtDefine(CHtmCrlfParseCtx, CHtmParseCtx, "CHtmCrlfParseCtx")
MtDefine(CHtmCrlfParseCtx_AddText_pchTemp, CHtmCrlfParseCtx, "CHtmCrlfParseCtx::AddText/pchTemp")

//+---------------------------------------------------------------------------
//
//  Method:     CHtmCrlfParseCtx::AddText
//
//  Synposis:   Calls AddNonspaces and AddSpaces methods
//
//              Eliminates CRLFs from space runs (converting \n and
//              \r\n to \r) before calling AddSpaces and AddNonspaces
//
//----------------------------------------------------------------------------
            
HRESULT
CHtmCrlfParseCtx::AddText(CTreeNode *pNode, TCHAR *pchIn, ULONG cchIn, BOOL fAscii)
{
    HRESULT hr      = S_OK;
    TCHAR *pchLast  = pchIn + cchIn;
    TCHAR ach[64];
    TCHAR *pchTemp  = ach;
    ULONG  cchTemp  = ARRAY_SIZE(ach);
    TCHAR *pch;
    TCHAR *pchTo;
    ULONG cch;
    TCHAR *pchWord;
    ULONG cchWord;

    while (pchIn < pchLast)
    {
        // eat strings of spaces
        if (ISSPACE(*pchIn))
        {
            pchWord = pchIn++;

            while (pchIn < pchLast && ISSPACE(*pchIn))
                pchIn++;

            cchWord = pchIn - pchWord;
            cchIn -= cchWord;

            // step 1: scan to see if there are an \n at all

            for (pch = pchWord, cch = cchWord; cch; pch++, cch--)
                if (*pch == _T('\n'))
                    break;

            // step 2: if there are \n's, convert them appropriately

            if (!cch)
            {
                pchTo = pchWord;
                cch = cchWord;
            }
            else
            {
                // Allocate memory only if space string is longer than ach's size

                if (cchWord > cchTemp)
                {
                    if (pchTemp == ach)
                        pchTemp = NULL;

                    hr = MemRealloc(Mt(CHtmCrlfParseCtx_AddText_pchTemp), (void **)&pchTemp, cchWord * sizeof(TCHAR));
                    if (hr)
                        goto Cleanup;
                }

                // Skip over non \n chars, and \n if it is the first one and _fLastCr

                pchTo = pchTemp;

                if (pch > pchWord)
                {
                    memcpy(pchTemp, pchWord, (cchWord - cch) * sizeof(TCHAR));
                    pchTo += cchWord - cch;
                }
                else
                {
                    if (_fLastCr)
                    {
                        pch++;
                        cch--;
                    }
                }

                // Skip over any \n that follow \r, and convert other \n to \r

                for (; cch; cch--)
                {
                    if (*pch != _T('\n'))
                        *pchTo++ = *pch++;
                    else
                    {
                        if (*(pch-1) != _T('\r'))
                            *pchTo++ = _T('\r');

                        pch++;
                    }
                }

                cch = pchTo - pchTemp;
                pchTo = pchTemp;
            }

			if (cch)
			{
				// step 3: pass the string on
				hr = THR(AddSpaces(pNode, pchTo, cch));
				if (hr)
					goto Cleanup;

			}

            _fLastCr = (pchWord[cchWord-1] == _T('\r'));
		}
        else
        {
            pchWord = pchIn++;

            for (;;)
            {
                while (pchIn < pchLast && ISNONSP(*pchIn))
                    pchIn++;

                if (pchIn+1 < pchLast && *pchIn == _T(' ') && ISNONSP(pchIn[1]))
                {
                    pchIn += 2;
                }
                else
                {
                    break;
                }
            }

            cchWord = pchIn - pchWord;
            cchIn -= cchWord;

            _fLastCr = FALSE;

            if (cchWord)
            {
                hr = THR(AddNonspaces(pNode, pchWord, cchWord, fAscii));
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    if (pchTemp != ach)
        MemFree(pchTemp);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CHtmCrlfParseCtx::Finish
//
//  Synopsis:   Would be needed if we were converting \r's to \n's instead
//              of \n's to \r's. For the time being, it does nothing.
//
//----------------------------------------------------------------------------
HRESULT CHtmCrlfParseCtx::Finish()
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Class:      CHtmSpaceParseCtx
//
//  Synopsis:   1. Collapses multiple spaces down to one space
//              2. Can eat space to the left or to the right of "fill"
//                 markers, or transfer space from the left to the right
//                 of a fill marker.
//              3. Will eat single linefeeds between two adjacent chinese
//                 characters
//
//----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   HanguelRange
//
//  Synopsis:   Detects Korean Hangeul characters
//
//-----------------------------------------------------------------------------

static inline BOOL
HanguelRange ( TCHAR ch )
{
    return ch > 0x10ff &&
           (InRange(ch, 0x1100, 0x11f9) ||
            InRange(ch, 0x3130, 0x318f) ||
            InRange(ch, 0xac00, 0xd7a3) ||
            InRange(ch, 0xffa1, 0xffdc));
}

//+----------------------------------------------------------------------------
//
//  Function:   TwoFarEastNonHanguelChars
//
//  Synopsis:   Determines if a CR between two characters should be
//              ignored (e.g., so Chinese chars on separate lines
//              of HTML are adjacent).
//
//-----------------------------------------------------------------------------

static inline BOOL
TwoFarEastNonHanguelChars ( TCHAR chPrev, TCHAR chAfter )
{
    if (chPrev < 0x3000 || chAfter < 0x3000)
        return FALSE;

    return ! HanguelRange( chPrev ) && ! HanguelRange( chAfter );
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmSpaceParseCtx::Destructor
//
//  Synopsis:   Releases any unreleased pointers
//
//-----------------------------------------------------------------------------

CHtmSpaceParseCtx::~CHtmSpaceParseCtx()
{
#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(_pNodeSpace);
#endif
}


//+----------------------------------------------------------------------------
//
//  Function:   CHtmSpaceParseCtx::AddNonspaces
//
//  Synopsis:   Adds any deferred space
//
//-----------------------------------------------------------------------------
HRESULT
CHtmSpaceParseCtx::AddNonspaces(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    Assert(cch && *pch);
    Assert(!_fEatSpace || !_pNodeSpace);
    Assert(!_pNodeSpace || _pNodeSpace == pNode);
    Assert((DWORD_PTR)_pNodeSpace != 1);

    HRESULT hr;
    CTreeNode *pNodeSpace;

    // 1. If space is needed, add it now

    if (_pNodeSpace)
    {
        // reentrant code
        pNodeSpace = _pNodeSpace;
        _pNodeSpace = NULL;
        
        if (!_chLast || !_fOneLine || !TwoFarEastNonHanguelChars(_chLast, *pch))
        {
            hr = THR(AddSpace(pNodeSpace));
            if (hr)
                goto Cleanup;
        }
        
#ifdef NOPARSEADDREF
        pNodeSpace->NodeRelease();
#endif
    }

    // 2. Add words

    hr = THR(AddWord(pNode, pch, cch, fAscii));
    if (hr)
        goto Cleanup;

    // 3. Stop eating space and note last (possibly chinese) char
    
    _fEatSpace = FALSE;
    _chLast = pch[cch - 1];
    _fOneLine = FALSE;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmSpaceParseCtx::AddSpaces
//
//  Synopsis:   Makes a note of any space that may need to be inserted later
//
//-----------------------------------------------------------------------------
HRESULT
CHtmSpaceParseCtx::AddSpaces(CTreeNode *pNode, TCHAR *pch, ULONG cch)
{
    Assert(cch && *pch); // more than zero spaces
    Assert(!_fEatSpace || !_pNodeSpace);
    Assert(!_pNodeSpace || _pNodeSpace == pNode);
    Assert((DWORD_PTR)_pNodeSpace != 1);

    // 1. Note space if we're not eating space

    if (!_fEatSpace && !_pNodeSpace)
    {
#ifdef NOPARSEADDREF
        pNode->NodeAddRef();
#endif
        _pNodeSpace = pNode;
    }

    // 2. If last char was nonspace (possibly chinese), note single line
    
    if (_chLast)
    {
        if (cch == 1 && *pch == _T('\r') && !_fOneLine)
            _fOneLine = TRUE;
        else
            _chLast = _T('\0');
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmSpaceParseCtx::LFill
//
//  Synopsis:   Flushes any deferred space
//
//-----------------------------------------------------------------------------
HRESULT
CHtmSpaceParseCtx::LFill(UINT fillcode)
{
    Assert(!_fEatSpace || !_pNodeSpace);
    Assert((DWORD_PTR)_pNodeSpace != 1);

    HRESULT hr = S_OK;
    
    // 1. Now last char was not chinese
    
    _chLast = _T('\0');
    
    // 2. Output, eat, or transfer space from the left

    if (_pNodeSpace)
    {
        if (fillcode == FILL_PUT)
        {
            hr = THR(AddSpace(_pNodeSpace));
            if (hr)
                goto Cleanup;

            _fEatSpace = TRUE;
        }

#ifdef NOPARSEADDREF
        _pNodeSpace->NodeRelease();
#endif
        _pNodeSpace = NULL;

        if (fillcode == FILL_NUL)
            _pNodeSpace = (CTreeNode*)1L;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CHtmSpaceParseCtx::RFill
//
//  Synopsis:   Sets up state for absorbing or accepting space
//
//-----------------------------------------------------------------------------
HRESULT
CHtmSpaceParseCtx::RFill(UINT fillcode, CTreeNode *pNode)
{
    Assert(!_fEatSpace || !_pNodeSpace);
    Assert(!_chLast);
    Assert(!_pNodeSpace || (DWORD_PTR)_pNodeSpace == 1);

    // 1. Reject space to the right if EAT

    if (fillcode == FILL_EAT)
    {
        _pNodeSpace = NULL;
        _fEatSpace = TRUE;
    }

    // 2. Accept space to the right if PUT

    if (fillcode == FILL_PUT)
    {
        _fEatSpace = FALSE;
    }
    
    // 3. Transfer any existing space

    if (_pNodeSpace)
    {
#ifdef NOPARSEADDREF
        pNode->NodeAddRef();
#endif
        _pNodeSpace = pNode;
    }

    return S_OK;
}
