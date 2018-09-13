//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       htmltag.cxx
//
//  Contents:   CHtmTag
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagHtmTagStm, "Dwn", "Trace CHtmTagStm")

// Performance Meters ---------------------------------------------------------

MtDefine(CHtmTagStm, Dwn, "CHtmTagStm")
MtDefine(CHtmTagStm_ptextbuf, CHtmTagStm, "CHtmTagStm::_ptextbuf")
MtDefine(CHtmTagStm_ptagbuf, CHtmTagStm, "CHtmTagStm::_ptagbuf")
MtDefine(CHtmTagQueue, Dwn, "CHtmTagQueue")

//+------------------------------------------------------------------------
//
//  Member:     CHtmTag::AttrFromName
//
//  Synopsis:   name->CAttr*
//
//-------------------------------------------------------------------------

CHtmTag::CAttr *
CHtmTag::AttrFromName(const TCHAR * pchName)
{
    Assert(pchName);

    int i = _cAttr;

    // optimize for zero attrs
    if (i && pchName)
    {
        CAttr *pattr = _aAttr;
        for (; i--; pattr++)
        {
            if (!StrCmpIC(pattr->_pchName, pchName))
            {
                return pattr;
            }
        }
    }
    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTag::ValFromName
//
//  Synopsis:   name->val
//              Returns TRUE if attribute named by pchName is present.
//              Returns pointer to value string in *ppchVal (NULL if no
//              value is present)
//
//-------------------------------------------------------------------------
BOOL
CHtmTag::ValFromName(const TCHAR * pchName, TCHAR **ppchVal)
{
    CAttr * pattr = AttrFromName(pchName);
    if (pattr)
    {
        *ppchVal = pattr->_pchVal;
        return TRUE;
    }
    else
    {
        *ppchVal = NULL;
        return FALSE;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTag::GetXmlNamespace
//
//  Synopsis:   enumerator of "xmlns:foo" declarations
//
//-------------------------------------------------------------------------

LPTSTR
CHtmTag::GetXmlNamespace(int * pIdx)
{
    Assert (pIdx);

    if (_cAttr)
    {
        CAttr * pAttr;

        for (pAttr = &(_aAttr[*pIdx]); (*pIdx) < _cAttr; (*pIdx)++, pAttr++)
        {
            if (_tcsnipre(_T("xmlns:"), 6, pAttr->_pchName, -1))
                return pAttr->_pchName + 6;
        }
    }
    return NULL;
}

#if 0

//+------------------------------------------------------------------------
//
//  Member:     CHtmTag::SaveAsText
//
//  Synopsis:   Creates a string containing the HTML of the tag.
//
//              The tag is recreated
//
//-------------------------------------------------------------------------
HRESULT
CHtmTag::SaveAsText(CBufferedStr *pbufstrOut)
{
    HRESULT hr;
    int     i;
    CAttr   * pattr;

    if (_etag == ETAG_RAW_COMMENT)
    {
        hr = THR(pbufstrOut->QuickAppend(_pch));
        goto Cleanup;
    }

    // In 5.0 the "/" does not exist in the _pch of an unknown tag

    if (IsEnd())
        hr = THR(pbufstrOut->QuickAppend(_T("</")));
    else
        hr = THR(pbufstrOut->QuickAppend(_T("<")));

    if (hr)
        goto Cleanup;

    if (_etag == ETAG_UNKNOWN || _etag == ETAG_GENERIC || _etag == ETAG_GENERIC_LITERAL)
        hr = THR(pbufstrOut->QuickAppend(_pch));
    else
        hr = THR(pbufstrOut->QuickAppend(NameFromEtag((ELEMENT_TAG)_etag)));

    if (hr)
        goto Cleanup;

    for (i = _cAttr, pattr = _aAttr; i--; ++pattr)
    {
        hr = THR(pbufstrOut->QuickAppend(_T(" ")));
        if (hr)
            goto Cleanup;

        hr = THR(pbufstrOut->QuickAppend(pattr->_pchName));
        if (hr)
            goto Cleanup;

        if (pattr->_pchVal)
        {
            hr = THR(pbufstrOut->QuickAppend(_T("=\"")));
            if (hr)
                goto Cleanup;

            hr = THR(pbufstrOut->QuickAppend(pattr->_pchVal));
            if (hr)
                goto Cleanup;

            hr = THR(pbufstrOut->QuickAppend(_T("\"")));
            if (hr)
                goto Cleanup;
        }
    }

    if (IsEmpty())
        hr = THR(pbufstrOut->QuickAppend(_T(" />")));
    else
        hr = THR(pbufstrOut->QuickAppend(_T(">")));

Cleanup:
    RRETURN(hr);
}

#endif

// CHtmTagStm -----------------------------------------------------------------

CHtmTagStm::~CHtmTagStm()
{
    TEXTBUF *   ptextbuf;
    TAGBUF *    ptagbuf;

    while ((ptextbuf = _ptextbufHead) != NULL)
    {
        _ptextbufHead = ptextbuf->ptextbufNext;
        MemFree(ptextbuf);
    }

    while ((ptagbuf = _ptagbufHead) != NULL)
    {
        _ptagbufHead = ptagbuf->ptagbufNext;
        MemFree(ptagbuf);
    }

    if (_pdsSource)
    {
        _pdsSource->Release();
    }

    MemFree(_ptextbufWrite);
}

HRESULT
CHtmTagStm::AllocTextBuffer(UINT cch, TCHAR ** ppch)
{
    PerfDbgLog1(tagHtmTagStm, this, "CHtmTagStm::AllocBuffer (cch=%ld)", cch);

    TEXTBUF * ptextbuf = (TEXTBUF *)MemAlloc(Mt(CHtmTagStm_ptextbuf), offsetof(TEXTBUF, ach) + cch * sizeof(TCHAR));
    HRESULT hr;

    if (ptextbuf == NULL)
    {
        *ppch = NULL;
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (_ptextbufWrite)
    {
        _ptextbufWrite->ptextbufNext = NULL;

        g_csDwnStm.Enter();

        if (_ptextbufTail == NULL)
        {
            _ptextbufHead = _ptextbufWrite;
            _ptextbufTail = _ptextbufWrite;
        }
        else
        {
            _ptextbufTail->ptextbufNext = _ptextbufWrite;
            _ptextbufTail = _ptextbufWrite;
        }

        _ptextbufWrite = NULL;

        g_csDwnStm.Leave();

        _fNextBuffer = TRUE;
    }

    _ptextbufWrite = ptextbuf;

    *ppch = ptextbuf->ach;
    hr = S_OK;

Cleanup:
    PerfDbgLog1(tagHtmTagStm, this, "CHtmTagStm::AllocBuffer (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmTagStm::GrowTextBuffer(UINT cch, TCHAR ** ppch)
{
    PerfDbgLog1(tagHtmTagStm, this, "CHtmTagStm::GrowBuffer (cch=%ld)", cch);

    Assert(_ptextbufWrite != NULL);

    HRESULT hr;

    hr = MemRealloc(Mt(CHtmTagStm_ptextbuf), (void **)&_ptextbufWrite, offsetof(TEXTBUF, ach) + cch * sizeof(TCHAR));

    if (hr == S_OK)
    {
        *ppch = _ptextbufWrite->ach;
    }

    PerfDbgLog1(tagHtmTagStm, this, "CHtmTagStm::GrowBuffer (hr=%lX)", hr);
    RRETURN(hr);
}

void
CHtmTagStm::DequeueTextBuffer()
{
    TEXTBUF * ptextbuf = _ptextbufHead;

    g_csDwnStm.Enter();

    _ptextbufHead = ptextbuf->ptextbufNext;

    if (_ptextbufHead == NULL)
        _ptextbufTail = NULL;

    g_csDwnStm.Leave();

    MemFree(ptextbuf);
}

HRESULT
CHtmTagStm::AllocTagBuffer(UINT cbNeed, void * pvCopy, UINT cbCopy)
{
    PerfDbgLog2(tagHtmTagStm, this, "+CHtmTagStm::AllocTagBuffer (cbNeed=%ld,cbCopy=%ld)", cbNeed, cbCopy);

    TAGBUF *    ptagbuf = _ptagbufTail;
    TAGBUF **   pptagbuf;
    HRESULT     hr = S_OK;

    if (_cbTagBuffer == 0)
        _cbTagBuffer = 256;
    else if (_cbTagBuffer < 4096)
        _cbTagBuffer <<= 1;

    cbNeed = (cbNeed + (_cbTagBuffer - 1)) & ~(_cbTagBuffer - 1);

    if (ptagbuf && ptagbuf->phtWrite == (CHtmTag *)ptagbuf->ab)
    {
        // Realloc the current buffer to make the entire tag fit

        hr = THR(MemRealloc(Mt(CHtmTagStm_ptagbuf), (void **)&ptagbuf, offsetof(TAGBUF, ab) + cbNeed));
        if (hr)
            goto Cleanup;

        // Update the queue pointers to point at the new tag buffer if it moved

        if (ptagbuf != _ptagbufTail)
        {
            g_csDwnStm.Enter();

            for (pptagbuf = &_ptagbufHead; *pptagbuf != _ptagbufTail; pptagbuf = &(*pptagbuf)->ptagbufNext) ;
            *pptagbuf = ptagbuf;

            _ptagbufTail = ptagbuf;

            g_csDwnStm.Leave();
        }
    }
    else
    {
        ptagbuf = (TAGBUF *)MemAlloc(Mt(CHtmTagStm_ptagbuf), offsetof(TAGBUF, ab) + cbNeed);

        if (ptagbuf == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        ptagbuf->ptagbufNext = NULL;

        if (cbCopy)
        {
            memcpy(ptagbuf->ab, pvCopy, cbCopy);
        }

        g_csDwnStm.Enter();

        if (_ptagbufTail == NULL)
            _ptagbufHead = ptagbuf;
        else
            _ptagbufTail->ptagbufNext = ptagbuf;

        _ptagbufTail = ptagbuf;

        g_csDwnStm.Leave();
    }

    ptagbuf->phtWrite = (CHtmTag *)ptagbuf->ab;
    _cbLeft = cbNeed;

Cleanup:
    PerfDbgLog1(tagHtmTagStm, this, "-CHtmTagStm::AllocTagBuffer (hr=%lX)", hr);
    RRETURN(hr);
}

CHtmTag *
CHtmTagStm::DequeueTagBuffer()
{
    TAGBUF * ptagbuf = _ptagbufHead;

    g_csDwnStm.Enter();

    _ptagbufHead = ptagbuf->ptagbufNext;

    Assert(_ptagbufHead);

    g_csDwnStm.Leave();

    MemFree(ptagbuf);

    return((CHtmTag *)_ptagbufHead->ab);
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

HRESULT
CHtmTagStm::WriteTag(ELEMENT_TAG etag, TCHAR * pch, ULONG cch, BOOL fAscii)
{
    if (_cbLeft < CHtmTag::ComputeSize(FALSE, 0))
    {
        HRESULT hr = THR(AllocTagBuffer(CHtmTag::ComputeSize(FALSE, 0), NULL, 0));
        if (hr)
            RRETURN(hr);
    }

    CHtmTag * pht = _ptagbufTail->phtWrite;
    _ptagbufTail->phtWrite = (CHtmTag *)((BYTE *)pht + CHtmTag::ComputeSize(FALSE, 0));
    _cbLeft -= CHtmTag::ComputeSize(FALSE, 0);

    pht->Reset();
    pht->SetTag(etag);
    pht->SetPch(pch);
    pht->SetCch(cch);

    if (fAscii)
        pht->SetAscii();

    if (_fNextBuffer)
    {
        _fNextBuffer = FALSE;
        pht->SetNextBuf();
    }

    _chtWrite += 1;
    _fSignal = TRUE;

    return(S_OK);
}

HRESULT
CHtmTagStm::WriteTag(ELEMENT_TAG etag, ULONG ul1, ULONG ul2)
{
    if (_cbLeft < CHtmTag::ComputeSize(FALSE, 0))
    {
        HRESULT hr = THR(AllocTagBuffer(CHtmTag::ComputeSize(FALSE, 0), NULL, 0));
        if (hr)
            RRETURN(hr);
    }

    CHtmTag * pht = _ptagbufTail->phtWrite;
    _ptagbufTail->phtWrite = (CHtmTag *)((BYTE *)pht + CHtmTag::ComputeSize(FALSE, 0));
    _cbLeft -= CHtmTag::ComputeSize(FALSE, 0);

    pht->Reset();
    pht->SetTag(etag);
    pht->SetLine(ul1);
    pht->SetOffset(ul2);

    if (_fNextBuffer)
    {
        _fNextBuffer = FALSE;
        pht->SetNextBuf();
    }

    _chtWrite += 1;
    _fSignal = TRUE;

    return(S_OK);
}

HRESULT
CHtmTagStm::WriteTagBeg(ELEMENT_TAG etag, CHtmTag ** ppht)
{
    if (_cbLeft < CHtmTag::ComputeSize(FALSE, 0))
    {
        HRESULT hr = THR(AllocTagBuffer(CHtmTag::ComputeSize(FALSE, 0), NULL, 0));
        if (hr)
            RRETURN(hr);
    }

    CHtmTag * pht = _ptagbufTail->phtWrite;

    pht->Reset();
    pht->SetTag(etag);

    *ppht = pht;

    return(S_OK);
}

HRESULT
CHtmTagStm::WriteTagGrow(CHtmTag ** ppht, CHtmTag::CAttr ** ppAttr)
{
    CHtmTag *   pht     = *ppht;
    UINT        cAttr   = pht->GetAttrCount();
    UINT        cbGrow  = CHtmTag::ComputeSize(FALSE, cAttr + 1);

    Assert(_ptagbufTail);
    Assert(_ptagbufTail->phtWrite == pht);
    Assert(!pht->IsTiny());

    if (_cbLeft < cbGrow)
    {
        HRESULT hr = THR(AllocTagBuffer(cbGrow, pht, cbGrow - sizeof(CHtmTag::CAttr)));
        if (hr)
            RRETURN(hr);

        *ppht = pht = _ptagbufTail->phtWrite;
    }

    pht->SetAttrCount(cAttr + 1);

    *ppAttr = pht->GetAttr(cAttr);

    return(S_OK);
}

void
CHtmTagStm::WriteTagEnd()
{
    Assert(_ptagbufTail);

    CHtmTag *   pht     = _ptagbufTail->phtWrite;
    UINT        cbTag   = pht->ComputeSize();

    Assert(_cbLeft >= cbTag);

    _ptagbufTail->phtWrite = (CHtmTag *)((BYTE *)pht + cbTag);
    _cbLeft -= cbTag;

    if (_fNextBuffer)
    {
        _fNextBuffer = FALSE;
        pht->SetNextBuf();
    }

    _chtWrite += 1;
    _fSignal = TRUE;
}

void
CHtmTagStm::WriteEof(HRESULT hrEof)
{
    PerfDbgLog1(tagHtmTagStm, this, "+CHtmTagStm::WriteEof (hrEof=%lX)", hrEof);

    if (!_fEof || hrEof)
    {
        _hrEof   = hrEof;
        _fEof    = TRUE;
        _fSignal = TRUE;
        Signal();
    }

    PerfDbgLog(tagHtmTagStm, this, "-CHtmTagStm::WriteEof");
}

HRESULT
CHtmTagStm::WriteSource(TCHAR *pch, ULONG cch)
{
    HRESULT hr = S_OK;
    void *pvTo;
    ULONG cbTo;
    ULONG cchCopy;

    if (!_pdsSource)
    {
        _pdsSource = new CDwnStm();
        if (!_pdsSource)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    while (cch)
    {
        hr = THR(_pdsSource->WriteBeg(&pvTo, &cbTo));
        if (hr)
            goto Cleanup;

        cchCopy = min(cch, (ULONG)(cbTo / sizeof(TCHAR)));

        Assert(cchCopy);
        if (!cchCopy)
            return E_FAIL;

        memcpy(pvTo, pch, cchCopy * sizeof(TCHAR));

        _pdsSource->WriteEnd(cchCopy * sizeof(TCHAR));

        cch -= cchCopy;
        pch += cchCopy;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTagStm::ReadSource(CBuffer2 *pBuffer, ULONG cch)
{
    HRESULT hr = S_OK;
    void *pvFrom;
    ULONG cbFrom;
    ULONG cchCopy;

    Assert(!cch || _pdsSource);
    if (cch && !_pdsSource)
        return E_FAIL;

    while (cch)
    {
        hr = THR(_pdsSource->ReadBeg(&pvFrom, &cbFrom));
        if (hr)
            goto Cleanup;

        cchCopy = min(cch, (ULONG)(cbFrom / sizeof(TCHAR)));

        Assert(cchCopy);
        if (!cchCopy)
            return E_FAIL;

        hr = THR(pBuffer->Append((TCHAR *)pvFrom, cchCopy));
        if (hr)
            goto Cleanup;

        _pdsSource->ReadEnd(cchCopy * sizeof(TCHAR));

        cch -= cchCopy;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmTagStm::SkipSource(ULONG cch)
{
    HRESULT hr = S_OK;
    void *pvFrom;
    ULONG cbFrom;
    ULONG cchCopy;

    Assert(!cch || _pdsSource);
    if (cch && !_pdsSource)
        return E_FAIL;

    while (cch)
    {
        hr = THR(_pdsSource->ReadBeg(&pvFrom, &cbFrom));
        if (hr)
            goto Cleanup;

        cchCopy = min(cch, (ULONG)(cbFrom / sizeof(TCHAR)));

        Assert(cchCopy);
        if (!cchCopy)
            return E_FAIL;

        _pdsSource->ReadEnd(cchCopy * sizeof(TCHAR));

        cch -= cchCopy;
    }

Cleanup:
    RRETURN(hr);
}

void
CHtmTagStm::Signal()
{
    if (_fSignal)
    {
        _fSignal = FALSE;
        super::Signal();
    }
}

CHtmTag *
CHtmTagStm::ReadTag(CHtmTag * pht)
{
    Assert(_chtRead <= _chtWrite);

    if (_hrEof || _chtRead == _chtWrite)
    {
        return(NULL);
    }

    Assert(_ptagbufHead);

    if (pht == NULL)
    {
        pht = _phtRead;

        if (pht == NULL)
        {
            pht = (CHtmTag *)_ptagbufHead->ab;
            goto gottag;
        }
    }

    Assert(pht);
    Assert(pht == _phtRead);
    Assert(pht >= (CHtmTag *)_ptagbufHead->ab);
    Assert(pht < _ptagbufHead->phtWrite);

    pht = (CHtmTag *)((BYTE *)pht + pht->ComputeSize());

    Assert(pht <= _ptagbufHead->phtWrite);

    if (pht == _ptagbufHead->phtWrite)
    {
        pht = DequeueTagBuffer();
    }

    if (pht->IsNextBuf())
    {
        DequeueTextBuffer();
    }

gottag:

    _phtRead = pht;
    _chtRead += 1;

    return(pht);
}

HRESULT
CHtmTagQueue::EnqueueTag(CHtmTag *pht)
{
    TCHAR *pch;
    ULONG cch;
    ULONG c;
    CHtmTag *phtTo;
    CHtmTag::CAttr *pAttr;
    CHtmTag::CAttr *pAttrTo;
    HRESULT hr = S_OK;

    Assert(!pht->IsSource()); // not needed, not handled

    // Step 1: compute the amount of text needed to copy tag

    cch = 0;

    if (pht->Is(ETAG_RAW_TEXT) ||
        pht->Is(ETAG_UNKNOWN) ||
        pht->Is(ETAG_GENERIC) ||
        pht->Is(ETAG_GENERIC_LITERAL) ||
        pht->Is(ETAG_GENERIC_BUILTIN))
    {
        cch += pht->GetCch() + 1;
    }

    c = pht->GetAttrCount();

    if (c)
    {
        for (pAttr = pht->GetAttr(0); c; c--, pAttr++)
        {
            cch += pAttr->_cchName + 1;

            if (pAttr->_pchVal)
                cch += pAttr->_cchVal + 1;
        }
    }

    // Step 2: allocate tag, copy static data

    hr = THR(WriteTagBeg(pht->GetTag(), &phtTo));
    if (hr)
        goto Cleanup;

    phtTo->SetPch(NULL);  // will be fixed up in step 3
    phtTo->SetCch(0);

    if (pht->Is(ETAG_SCRIPT) ||
        pht->Is(ETAG_RAW_CODEPAGE) ||
        pht->Is(ETAG_RAW_DOCSIZE))
    {
        phtTo->SetLine(pht->GetLine()); // also copies codepage, docsize
        phtTo->SetOffset(pht->GetOffset());
    }

    if (pht->IsTiny())
    {
        Assert(pht->GetTag() > ETAG_UNKNOWN && pht->GetTag() < ETAG_GENERIC
            || pht->Is(ETAG_RAW_BEGINSEL) || pht->Is(ETAG_RAW_ENDSEL));
        Assert(pht->GetAttrCount() == 0);
        Assert(!cch);
        phtTo->SetTiny();
    }

    if (pht->IsRestart())
        phtTo->SetRestart();

    if (pht->IsEmpty())
        phtTo->SetEmpty();

    if (pht->IsEnd())
        phtTo->SetEnd();

    if (pht->IsAscii())
        phtTo->SetAscii();

    // Step 3: allocate and copy text/attr/val

    if (cch)
    {
        hr = THR(AllocTextBuffer(cch, &pch));
        if (hr)
            goto Cleanup;

        if (pht->Is(ETAG_RAW_TEXT) ||
            pht->Is(ETAG_UNKNOWN) ||
            pht->Is(ETAG_GENERIC) ||
            pht->Is(ETAG_GENERIC_LITERAL) ||
            pht->Is(ETAG_GENERIC_BUILTIN))
        {
            phtTo->SetPch(pch);
            memcpy(pch, pht->GetPch(), pht->GetCch() * sizeof(TCHAR));
            pch += pht->GetCch();
            *pch = _T('\0');
            pch++;
        }

        c = pht->GetAttrCount();

        if (c)
        {
            for (pAttr = pht->GetAttr(0); c; c--, pAttr++)
            {
                hr = THR(WriteTagGrow(&phtTo, &pAttrTo));
                if (hr)
                    goto Cleanup;

                // Copy all fields
                *pAttrTo = *pAttr;

                // Fixup pointers
                pAttrTo->_pchName = pch;
                memcpy(pch, pAttr->_pchName, pAttr->_cchName * sizeof(TCHAR));
                pch += pAttr->_cchName;
                *pch = _T('\0');
                pch++;

                if (pAttr->_pchVal)
                {
                    pAttrTo->_pchVal = pch;
                    memcpy(pch, pAttr->_pchVal, pAttr->_cchVal * sizeof(TCHAR));
                    pch += pAttr->_cchVal;
                    *pch = _T('\0');
                    pch++;
                }
            }
        }
    }

    WriteTagEnd();

    _cEnqueued++;

Cleanup:
    RRETURN(hr);
}

CHtmTag *
CHtmTagQueue::DequeueTag()
{
    CHtmTag *pht;

    Assert(_cEnqueued);

    _cEnqueued--;

    pht = ReadTag(NULL);

    Assert(pht);

    return pht;
}

HRESULT
CHtmTagQueue::ParseAndEnqueueTag(TCHAR *pch, ULONG cch)
{
    TCHAR *pchCopy;
    TCHAR ch;
    HRESULT hr;

    if (cch < 2)
        return E_FAIL;

    if (*pch != _T('<') || *(pch + cch - 1) != _T('>'))
        return E_FAIL;

    // Don't deal with end-tags, etc for now

    ch = *(pch + 1);

    if (!ISNAMCH(ch))
        return E_FAIL;

    hr = THR(AllocTextBuffer(cch, &pchCopy));
    if (hr)
        goto Cleanup;

    memcpy(pchCopy, pch, cch * sizeof(TCHAR));

    hr = THR(CHtmPre::DoTokenizeOneTag(pchCopy, cch, this, NULL, 0, 0, FALSE, NULL));
    if (hr)
        goto Cleanup;

    _cEnqueued += 1;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CreateElement
//
//  Note:       If the etag is ETAG_NULL, the the string contains the name
//              of the tag to create (like "h1") or it can contain an
//              actual tag (like "<h1 foo=bar>").
//
//              If etag is not ETAG_NULL, then the string contains the
//              arguments for the new element.
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::CreateElement (
    ELEMENT_TAG etag, CElement * * ppElementNew, TCHAR * pch, long cch )
{
    HRESULT hr = S_OK;
    CHtmTag ht, * pht = NULL;
    TCHAR * pchTag = NULL;
    long    cchTag = 0;
    CStr    strTag;     // In case we have to build "<name attrs>"
    CHtmTagQueue * phtq = NULL;

    ht.Reset();

    Assert( ppElementNew );

    *ppElementNew = NULL;

    //
    // Here we check for the various kinds of input to this function.
    //
    // After this checking, we will either have pht != NULL which is
    // ready for use to pass to create element, or we will have a string
    // (pchTag, cchTag) which is appropriate for passing to the 'parser'
    // for creating an element.
    //

    if (etag != ETAG_NULL)
    {
        if (!pch || cch <= 0)
        {
            pht = & ht;
        }
        else
        {
            const TCHAR * pchName = NameFromEtag( etag );

            if (!pchName || !*pchName)
            {
                pht = & ht;
            }
            else
            {
                hr = THR( strTag.Append( _T("<"), 1 ) );

                if (hr)
                    goto Cleanup;

                hr = THR( strTag.Append( pchName ) );

                if (hr)
                    goto Cleanup;

                hr = THR( strTag.Append( _T(" "), 1 ) );

                if (hr)
                    goto Cleanup;

                hr = THR( strTag.Append( pch, cch ) );

                if (hr)
                    goto Cleanup;

                hr = THR( strTag.Append( _T(">"), 1 ) );

                if (hr)
                    goto Cleanup;

                pchTag = strTag;
                cchTag = strTag.Length();
            }
        }

        if (pht)
        {
            Assert( pht = & ht );
            ht.Reset();
            ht.SetTag( etag );
        }
    }
    else
    {
        if (pch && cch > 2 && *pch == _T('<'))
        {
            pchTag = pch;
            cchTag = cch;
        }
        else
        {
            pht = & ht;
            pht->Reset();
            ht.SetTag( EtagFromName( pch, cch ) );
        }
    }

    //
    //
    //

    if (!pht && pchTag && cchTag)
    {
        Assert( *pchTag == _T('<') );
        Assert( long( _tcslen( pchTag ) ) == cchTag );

        phtq = new CHtmTagQueue;

        if (!phtq)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( phtq->ParseAndEnqueueTag( pchTag, cchTag ) );

        if (hr)
            goto Cleanup;

        pht = phtq->DequeueTag();
    }

    if (!pht)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (pht->GetTag() == ETAG_NULL)
    {
        // generic tag?

#ifdef VSTUDIO7
        BOOL fDerived = FALSE;
        pht->SetTag( IsGenericElement( pch, /* fAllSprinklesGeneric = */ TRUE, &fDerived ) );
        if (fDerived)
            pht->SetDerivedTag();
#else
        pht->SetTag( IsGenericElement( pch, /* fAllSprinklesGeneric = */ TRUE ) );
#endif //VSTUDIO7

        ht.SetPch( pch );
        ht.SetCch( cch );
    }

    pht->SetScriptCreated();

    hr = THR( ::CreateElement( pht, ppElementNew, Doc(), this, NULL, INIT2FLAG_EXECUTE ) );

    if (hr)
        goto Cleanup;

    if (!(*ppElementNew)->IsNoScope())
        (*ppElementNew)->_fExplicitEndTag = TRUE;

Cleanup:

    if (phtq)
        phtq->Release();

    RRETURN( hr );
}


#if DBG!=1
#pragma optimize("", on)
#endif
