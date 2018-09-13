//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       syncbuf.cxx
//
//  Contents:   Tree Syncronization Opcode/Operand Buffer stuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "logtool.h"




// implementation of CLogBufferPoolManager


CLogBufferPoolManager::CLogBufferPoolManager()
{
}


CLogBufferPoolManager::~CLogBufferPoolManager()
{
}




// implementation of CLogBuffer


// enter the undefined state
CLogBuffer::CLogBuffer()
{
    _fZombie = true;
}


// clean up
CLogBuffer::~CLogBuffer()
{
    Shutdown();
}


// initialize buffer for writing
HRESULT
CLogBuffer::Init(CLogBufferPoolManager *ppool)
{
    Assert(_fZombie);

    _ppool = ppool;
    _dwBufLen = 4096;
    _prgbBuf = new BYTE[_dwBufLen];
    if (_prgbBuf == NULL)
    {
        _dwBufLen = 0;
    }
    _dwBufIndex = 0;
    _fWriteMode = true;
    _fZombie = false;

    return S_OK;
}


// initialize buffer for reading
void
CLogBuffer::Init(void *pvBuf, DWORD dwBufLen)
{
    Assert(_fZombie);

    _ppool = NULL;
    _dwBufLen = dwBufLen;
    _prgbBuf = (BYTE*)pvBuf;
    _dwBufIndex = 0;
    _fWriteMode = false;
    _fZombie = false;
}


// shut down the buffer -- return to zombie state
void
CLogBuffer::Shutdown()
{
    if (!_fZombie)
    {
        if (_fWriteMode)
        {
            delete [] _prgbBuf;
        }
        _fZombie = true;
    }
}


HRESULT
CLogBuffer::AppendOp(TreeOps op)
{
    Assert(!_fZombie);
    return AppendDWORD(op);
}


HRESULT
CLogBuffer::AppendDWORD(DWORD dw)
{
    Assert(!_fZombie);
    Assert(_fWriteMode);

    HRESULT hr = EnsureSize(_dwBufIndex + sizeof(DWORD));
    if (hr != S_OK)
    {
        return hr;
    }

    *((DWORD*)(_prgbBuf + _dwBufIndex)) = dw;
    _dwBufIndex += sizeof(DWORD);

    Assert(_dwBufIndex <= _dwBufLen);

    return S_OK;
}


HRESULT
CLogBuffer::AppendString(WCHAR *rgwch, DWORD cwch)
{
    Assert(!_fZombie);
    Assert(_fWriteMode);

    HRESULT hr = EnsureSize(_dwBufIndex + sizeof(DWORD) + (cwch + 1) * sizeof(WCHAR));
    if (hr != S_OK)
    {
        return hr;
    }

    *((DWORD*)(_prgbBuf + _dwBufIndex)) = cwch;
    _dwBufIndex += sizeof(DWORD);

    memcpy(_prgbBuf + _dwBufIndex,(BYTE*)rgwch,cwch * sizeof(WCHAR));
    _dwBufIndex += cwch * sizeof(WCHAR);

    *((WCHAR*)(_prgbBuf + _dwBufIndex)) = 0;
    _dwBufIndex += sizeof(WCHAR);

    Assert(_dwBufIndex <= _dwBufLen);

    return S_OK;

}


HRESULT
CLogBuffer::ReserveDWORD(DWORD *pdwIndexOut)
{
    Assert(!_fZombie);
    Assert(_fWriteMode);

    HRESULT hr = EnsureSize(_dwBufIndex + sizeof(DWORD));
    if (hr != S_OK)
    {
        return hr;
    }

#if DBG
    *((DWORD*)(_prgbBuf + _dwBufIndex)) = 0xDEADBEEF;
#endif
    *pdwIndexOut = _dwBufIndex;
    _dwBufIndex += sizeof(DWORD);

    Assert(_dwBufIndex <= _dwBufLen);

    return S_OK;
}


HRESULT
CLogBuffer::UpdateDWORD(DWORD dwIndex, DWORD dwValue)
{
    Assert(!_fZombie);
    Assert(_fWriteMode);

    Assert(dwIndex < _dwBufIndex);

    *((DWORD*)(_prgbBuf + dwIndex)) = dwValue;

    return S_OK;
}


HRESULT
CLogBuffer::ReadOp(TreeOps *popOut)
{
    DWORD dw;
    HRESULT hr;

    Assert(!_fZombie);
    Assert(!_fWriteMode);

    hr = ReadDWORD(&dw);
    Assert(_dwBufIndex <= _dwBufLen);
    if (hr != S_OK)
    {
        return hr;
    }

    *popOut = (TreeOps)dw;

    return S_OK;
}


HRESULT
CLogBuffer::ReadDWORD(DWORD *pdwOut)
{
    Assert(!_fZombie);
    Assert(!_fWriteMode);

    Assert(_dwBufIndex + sizeof(DWORD) <= _dwBufLen);
    if (!(_dwBufIndex + sizeof(DWORD) <= _dwBufLen))
    {
        return E_UNEXPECTED;
    }

    if (pdwOut != NULL)
    {
        *pdwOut = *((DWORD*)(_prgbBuf + _dwBufIndex));
    }
    _dwBufIndex += sizeof(DWORD);

    Assert(_dwBufIndex <= _dwBufLen);

    return S_OK;
}


HRESULT
CLogBuffer::ReadString(WCHAR **prgwch, DWORD *pcwch)
{
    DWORD cwch;

    Assert(!_fZombie);
    Assert(!_fWriteMode);

    Assert(_dwBufIndex + sizeof(DWORD) <= _dwBufLen);
    if (!(_dwBufIndex + sizeof(DWORD) <= _dwBufLen))
    {
        return E_UNEXPECTED;
    }

    cwch = *((DWORD*)(_prgbBuf + _dwBufIndex));
    _dwBufIndex += sizeof(DWORD);

    Assert(_dwBufIndex + (cwch + 1) * sizeof(WCHAR) <= _dwBufLen);
    if (!(_dwBufIndex + (cwch + 1) * sizeof(WCHAR) <= _dwBufLen))
    {
        return E_UNEXPECTED;
    }

    *prgwch = (WCHAR*)(_prgbBuf + _dwBufIndex);
    _dwBufIndex += (cwch + 1) * sizeof(WCHAR);

    *pcwch = cwch;

    Assert(_dwBufIndex <= _dwBufLen);

    return S_OK;
}


HRESULT
CLogBuffer::BackReadPrevDWORD(DWORD *pdw)
{
    Assert(!_fZombie);
    Assert(!_fWriteMode);

    Assert(_dwBufIndex > sizeof(DWORD));
    if (!(_dwBufIndex > sizeof(DWORD)))
    {
        return E_UNEXPECTED;
    }

    _dwBufIndex -= sizeof(DWORD);
    *pdw = *((DWORD*)(_prgbBuf + _dwBufIndex));

    return S_OK;
}


HRESULT
CLogBuffer::EnsureSize(DWORD dwNewIndex)
{
    Assert(!_fZombie);
    Assert(_fWriteMode);

    if (dwNewIndex > _dwBufLen)
    {
        DWORD dwLenNew;
        BYTE *prgbNew;

        dwLenNew = _dwBufLen * 2;
        if (dwNewIndex > dwLenNew)
        {
            dwLenNew = dwNewIndex;
        }
        prgbNew = new BYTE[dwLenNew];
        if (prgbNew == NULL)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(prgbNew,_prgbBuf,_dwBufLen);

        delete [] _prgbBuf;

        _prgbBuf = prgbNew;
        _dwBufLen = dwLenNew;
    }

    return S_OK;
}




// implementation for CLogRecord


// return an allocated temporary string
HRESULT
CLogRecord::TempStoreString(WCHAR *rgwch, DWORD cwch, WCHAR **prgwch, DWORD *pcwch)
{
    Assert(*prgwch == NULL);

    *prgwch = NULL;
    *pcwch = 0;

    WCHAR *ptemp = new WCHAR[cwch + 1];
    if (ptemp == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *pcwch = cwch;
    *prgwch = ptemp;

    memcpy(ptemp,rgwch,sizeof(WCHAR) * cwch);
    ptemp[cwch] = 0;

    return S_OK;
}



// free the string allocated with TempStoreString
void
CLogRecord::FreeTempString(WCHAR *rgwch, DWORD cwch)
{
    delete [] rgwch;
}


// get info needed to put length of log record into log
HRESULT
CLogRecord::DelimitLengthSetup(CLogBuffer *pbuf, DWORD *pdwLeadingLengthLocation, DWORD *pdwInitialIndex)
{
    *pdwInitialIndex = pbuf->GetIndex();
    return pbuf->ReserveDWORD(pdwLeadingLengthLocation);
}


// finish setting up the log record length
HRESULT
CLogRecord::DelimitLengthFinish(CLogBuffer *pbuf, DWORD dwLeadingLengthLocatin, DWORD dwInitialIndex)
{
    DWORD dwSize = sizeof(DWORD) + (pbuf->GetIndex() - dwInitialIndex);
    pbuf->UpdateDWORD(dwLeadingLengthLocatin,dwSize); // leading length delimiter
    return pbuf->AppendDWORD(dwSize); // trailing length delimiter
}




// implementation for CInsertTextLogRecord


// create insert text record by reading from buffer
CInsertTextLogRecord::CInsertTextLogRecord(CLogBuffer *pbuf, HRESULT *phr)
{
    HRESULT hr;

#if DBG
    _fValid_dbg = false;
    _fWritten_dbg = false;
#endif
    _fWrite = false;

    _rec._cch = 0;
    _rec._rgwch = NULL;

    // assume leading length and opcode already scanned

    hr = pbuf->ReadDWORD(&_rec._cp);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._rgwch,&_rec._cch);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->SkipDWORD(); // trailing length
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

#if DBG
    _fValid_dbg = true;
#endif

    *phr = S_OK;
    return;
}


CInsertTextLogRecord::CInsertTextLogRecord(INSERTTEXTREC *prec)
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif

    _rec = *prec;

    _fWrite = false;
}


// create insert text record for writing to buffer
CInsertTextLogRecord::CInsertTextLogRecord()
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif
    _fWrite = true;

    _rec._cch = 0;
    _rec._rgwch = NULL;
}


CInsertTextLogRecord::~CInsertTextLogRecord()
{
    if (_fWrite)
    {
        FreeTempString(_rec._rgwch,_rec._cch);
    }
}


HRESULT
CInsertTextLogRecord::SetText(WCHAR *psz, DWORD cch)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._rgwch,_rec._cch);
    return TempStoreString(psz,cch,&_rec._rgwch,&_rec._cch);
}


HRESULT
CInsertTextLogRecord::Write(CLogBuffer *pbuf)
{
    HRESULT hr;
    DWORD dwLeadLenLoc;
    DWORD dwInitIndx;

    Assert(_fValid_dbg && !_fWritten_dbg);

#if DBG
    _fWritten_dbg = true;
#endif

    hr = DelimitLengthSetup(pbuf,&dwLeadLenLoc,&dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendOp(synclogInsertText);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cp);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._rgwch,_rec._cch);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = DelimitLengthFinish(pbuf,dwLeadLenLoc,dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    return hr;
}


HRESULT
CInsertTextLogRecord::MutateToWrite()
{
    Assert(_fValid_dbg && !_fWritten_dbg && !_fWrite);

    DWORD cch = _rec._cch;
    WCHAR *rgwch = _rec._rgwch;

    _rec._cch = 0;
    _rec._rgwch = NULL;

    _fWrite = true;

    HRESULT hr = TempStoreString(rgwch,cch,&_rec._rgwch,&_rec._cch);
    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}




// implementation for CInsertTextLogRecord


// create insert element by reading from buffer
CInsertDeleteElementLogRecord::CInsertDeleteElementLogRecord(CLogBuffer *pbuf, HRESULT *phr)
{
    HRESULT hr;

#if DBG
    _fValid_dbg = false;
    _fWritten_dbg = false;
#endif
    _fWrite = false;

    _rec._cchAttrs = 0;
    _rec._pszAttrs = NULL;

    // assume leading length and opcode already scanned

    hr = pbuf->ReadDWORD(&_rec._cpStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._tagid);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszAttrs,&_rec._cchAttrs);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->SkipDWORD(); // trailing length
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

#if DBG
    _fValid_dbg = true;
#endif

    *phr = S_OK;
    return;
}


CInsertDeleteElementLogRecord::CInsertDeleteElementLogRecord(INSDELELEMENTREC *prec)
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif

    _rec = *prec;

    _fWrite = false;
}


// create insert element for writing to buffer
CInsertDeleteElementLogRecord::CInsertDeleteElementLogRecord()
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif
    _fWrite = true;

    _rec._cchAttrs = 0;
    _rec._pszAttrs = NULL;
}


CInsertDeleteElementLogRecord::~CInsertDeleteElementLogRecord()
{
    if (_fWrite)
    {
        FreeTempString(_rec._pszAttrs,_rec._cchAttrs);
    }
}


HRESULT
CInsertDeleteElementLogRecord::SetAttrs(WCHAR *pszAttrs, DWORD cchAttrs)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszAttrs,_rec._cchAttrs);
    return TempStoreString(pszAttrs,cchAttrs,&_rec._pszAttrs,&_rec._cchAttrs);
}


HRESULT
CInsertDeleteElementLogRecord::Write(CLogBuffer *pbuf)
{
    HRESULT hr;
    DWORD dwLeadLenLoc;
    DWORD dwInitIndx;

    Assert(_fValid_dbg && !_fWritten_dbg);

#if DBG
    _fWritten_dbg = true;
#endif

    hr = DelimitLengthSetup(pbuf,&dwLeadLenLoc,&dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendOp(GetOpcode());
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._tagid);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszAttrs,_rec._cchAttrs);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = DelimitLengthFinish(pbuf,dwLeadLenLoc,dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    return hr;
}


HRESULT
CInsertDeleteElementLogRecord::MutateToWrite()
{
    Assert(_fValid_dbg && !_fWritten_dbg && !_fWrite);

    DWORD cchAttrs = _rec._cchAttrs;
    WCHAR *rgwchAttrs = _rec._pszAttrs;

    _rec._cchAttrs = 0;
    _rec._pszAttrs = NULL;

    _fWrite = true;

    HRESULT hr = TempStoreString(rgwchAttrs,cchAttrs,&_rec._pszAttrs,&_rec._cchAttrs);
    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}


TreeOps
CInsertElementLogRecord::GetOpcode()
{
    return synclogInsertElement;
}


TreeOps
CDeleteElementLogRecord::GetOpcode()
{
    return synclogDeleteElement;
}




// implementation for CInsertTreeLogRecord


// create insert tree by reading from buffer
CInsertTreeLogRecord::CInsertTreeLogRecord(CLogBuffer *pbuf, HRESULT *phr)
{
    HRESULT hr;

#if DBG
    _fValid_dbg = false;
    _fWritten_dbg = false;
#endif
    _fWrite = false;

    _rec._cchHTML = 0;
    _rec._pszHTML = NULL;

    // assume leading length and opcode already scanned

    hr = pbuf->ReadDWORD(&_rec._cpStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpTarget);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszHTML,&_rec._cchHTML);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->SkipDWORD(); // trailing length
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

#if DBG
    _fValid_dbg = true;
#endif

    *phr = S_OK;
    return;
}


CInsertTreeLogRecord::CInsertTreeLogRecord(INSERTTREEREC *prec)
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif

    _rec = *prec;

    _fWrite = false;
}


// create insert element for writing to buffer
CInsertTreeLogRecord::CInsertTreeLogRecord()
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif
    _fWrite = true;

    _rec._cchHTML = 0;
    _rec._pszHTML = NULL;
}


CInsertTreeLogRecord::~CInsertTreeLogRecord()
{
    if (_fWrite)
    {
        FreeTempString(_rec._pszHTML,_rec._cchHTML);
    }
}


HRESULT
CInsertTreeLogRecord::SetHTML(WCHAR *pszHTML, DWORD cchHTML)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszHTML,_rec._cchHTML);
    return TempStoreString(pszHTML,cchHTML,&_rec._pszHTML,&_rec._cchHTML);
}


HRESULT
CInsertTreeLogRecord::Write(CLogBuffer *pbuf)
{
    HRESULT hr;
    DWORD dwLeadLenLoc;
    DWORD dwInitIndx;

    Assert(_fValid_dbg && !_fWritten_dbg);

#if DBG
    _fWritten_dbg = true;
#endif

    hr = DelimitLengthSetup(pbuf,&dwLeadLenLoc,&dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendOp(synclogInsertTree);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpTarget);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszHTML,_rec._cchHTML);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = DelimitLengthFinish(pbuf,dwLeadLenLoc,dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    return hr;
}


HRESULT
CInsertTreeLogRecord::MutateToWrite()
{
    Assert(_fValid_dbg && !_fWritten_dbg && !_fWrite);

    DWORD cchHTML = _rec._cchHTML;
    WCHAR *rgwchHTML = _rec._pszHTML;

    _rec._cchHTML = 0;
    _rec._pszHTML = NULL;

    _fWrite = true;

    HRESULT hr = TempStoreString(rgwchHTML,cchHTML,&_rec._pszHTML,&_rec._cchHTML);
    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}




// implementation for CCutCopyMoveTreeLogRecord


// create insert tree by reading from buffer
CCutCopyMoveTreeLogRecord::CCutCopyMoveTreeLogRecord(CLogBuffer *pbuf, HRESULT *phr)
{
    HRESULT hr;

#if DBG
    _fValid_dbg = false;
    _fWritten_dbg = false;
#endif
    _fWrite = false;

    _rec._cchOldHTML = 0;
    _rec._pszOldHTML = NULL;

    // assume leading length and opcode already scanned

    hr = pbuf->ReadDWORD(&_rec._cpStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpTarget);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._fRemove);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszOldHTML,&_rec._cchOldHTML);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpOldHTMLStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpOldHTMLFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpPostOpSrcStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpPostOpSrcFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpPostOpTgtStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._cpPostOpTgtFinish);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->SkipDWORD(); // trailing length
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

#if DBG
    _fValid_dbg = true;
#endif

    *phr = S_OK;
    return;
}


CCutCopyMoveTreeLogRecord::CCutCopyMoveTreeLogRecord(CUTCOPYMOVEREC *prec)
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif

    _rec = *prec;

    _fWrite = false;
}


// create insert element for writing to buffer
CCutCopyMoveTreeLogRecord::CCutCopyMoveTreeLogRecord()
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif
    _fWrite = true;

    _rec._cchOldHTML = 0;
    _rec._pszOldHTML = NULL;
}


CCutCopyMoveTreeLogRecord::~CCutCopyMoveTreeLogRecord()
{
    if (_fWrite)
    {
        FreeTempString(_rec._pszOldHTML,_rec._cchOldHTML);
    }
}


HRESULT
CCutCopyMoveTreeLogRecord::SetOldHTML(WCHAR *psz, DWORD cch)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszOldHTML,_rec._cchOldHTML);
    return TempStoreString(psz,cch,&_rec._pszOldHTML,&_rec._cchOldHTML);
}


HRESULT
CCutCopyMoveTreeLogRecord::Write(CLogBuffer *pbuf)
{
    HRESULT hr;
    DWORD dwLeadLenLoc;
    DWORD dwInitIndx;

    Assert(_fValid_dbg && !_fWritten_dbg);

#if DBG
    _fWritten_dbg = true;
#endif

    hr = DelimitLengthSetup(pbuf,&dwLeadLenLoc,&dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendOp(synclogCutCopyMove);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpTarget);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._fRemove);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszOldHTML,_rec._cchOldHTML);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpOldHTMLStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpOldHTMLFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpPostOpSrcStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpPostOpSrcFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpPostOpTgtStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpPostOpTgtFinish);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = DelimitLengthFinish(pbuf,dwLeadLenLoc,dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    return hr;
}


HRESULT
CCutCopyMoveTreeLogRecord::MutateToWrite()
{
    Assert(_fValid_dbg && !_fWritten_dbg && !_fWrite);

    DWORD cchOldHTML = _rec._cchOldHTML;
    WCHAR *rgwchOldHTML = _rec._pszOldHTML;

    _rec._cchOldHTML = 0;
    _rec._pszOldHTML = NULL;

    _fWrite = true;

    HRESULT hr = TempStoreString(rgwchOldHTML,cchOldHTML,&_rec._pszOldHTML,&_rec._cchOldHTML);
    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}




// implementation for CCutCopyMoveTreeLogRecord


// create insert tree by reading from buffer
CChangeAttrLogRecord::CChangeAttrLogRecord(CLogBuffer *pbuf, HRESULT *phr)
{
    HRESULT hr;

#if DBG
    _fValid_dbg = false;
    _fWritten_dbg = false;
#endif
    _fWrite = false;

    _rec._cchName = 0;
    _rec._pszName = NULL;

    _rec._cchValOld = 0;
    _rec._pszValOld = NULL;

    _rec._cchValNew = 0;
    _rec._pszValNew = NULL;

    // assume leading length and opcode already scanned

    hr = pbuf->ReadDWORD(&_rec._cpStart);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszName,&_rec._cchName);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadDWORD(&_rec._dwBits);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszValOld,&_rec._cchValOld);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->ReadString(&_rec._pszValNew,&_rec._cchValNew);
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

    hr = pbuf->SkipDWORD(); // trailing length
    Assert(SUCCEEDED(hr));
    if (hr != S_OK)
    {
        *phr = hr;
        return;
    }

#if DBG
    _fValid_dbg = true;
#endif

    *phr = S_OK;
    return;
}


CChangeAttrLogRecord::CChangeAttrLogRecord(CHANGEATTRREC *prec)
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif

    _rec = *prec;

    _fWrite = false;
}


// create insert element for writing to buffer
CChangeAttrLogRecord::CChangeAttrLogRecord()
{
#if DBG
    _fValid_dbg = true;
    _fWritten_dbg = false;
#endif
    _fWrite = true;

    _rec._cchName = 0;
    _rec._pszName = NULL;

    _rec._cchValOld = 0;
    _rec._pszValOld = NULL;

    _rec._cchValNew = 0;
    _rec._pszValNew = NULL;
}


CChangeAttrLogRecord::~CChangeAttrLogRecord()
{
    if (_fWrite)
    {
        FreeTempString(_rec._pszName,_rec._cchName);
        FreeTempString(_rec._pszValOld,_rec._cchValOld);
        FreeTempString(_rec._pszValNew,_rec._cchValNew);
    }
}


HRESULT
CChangeAttrLogRecord::SetName(WCHAR *psz, DWORD cch)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszName,_rec._cchName);
    return TempStoreString(psz,cch,&_rec._pszName,&_rec._cchName);
}


HRESULT
CChangeAttrLogRecord::SetSzValOld(WCHAR *psz, DWORD cch)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszValOld,_rec._cchValOld);
    return TempStoreString(psz,cch,&_rec._pszValOld,&_rec._cchValOld);
}


HRESULT
CChangeAttrLogRecord::SetSzValNew(WCHAR *psz, DWORD cch)
{
    Assert(!_fWritten_dbg && _fWrite);

    FreeTempString(_rec._pszValNew,_rec._cchValNew);
    return TempStoreString(psz,cch,&_rec._pszValNew,&_rec._cchValNew);
}

HRESULT
CChangeAttrLogRecord::Write(CLogBuffer *pbuf)
{
    HRESULT hr;
    DWORD dwLeadLenLoc;
    DWORD dwInitIndx;

    Assert(_fValid_dbg && !_fWritten_dbg);

#if DBG
    _fWritten_dbg = true;
#endif

    hr = DelimitLengthSetup(pbuf,&dwLeadLenLoc,&dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendOp(synclogChangeAttr);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._cpStart);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszName,_rec._cchName);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendDWORD(_rec._dwBits);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszValOld,_rec._cchValOld);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = pbuf->AppendString(_rec._pszValNew,_rec._cchValNew);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = DelimitLengthFinish(pbuf,dwLeadLenLoc,dwInitIndx);
    if (hr != S_OK)
    {
        return hr;
    }

    return hr;
}


HRESULT
CChangeAttrLogRecord::MutateToWrite()
{
    Assert(_fValid_dbg && !_fWritten_dbg && !_fWrite);


    DWORD cchName = _rec._cchName;
    WCHAR *rgwchName = _rec._pszName;

    _rec._cchName = 0;
    _rec._pszName = NULL;


    DWORD cchValOld = _rec._cchValOld;
    WCHAR *rgwchValOld = _rec._pszValOld;

    _rec._cchValOld = 0;
    _rec._pszValOld = NULL;


    DWORD cchValNew = _rec._cchValNew;
    WCHAR *rgwchValNew = _rec._pszValNew;

    _rec._cchValNew = 0;
    _rec._pszValNew = NULL;


    _fWrite = true;

    HRESULT hr = TempStoreString(rgwchName,cchName,&_rec._pszName,&_rec._cchName);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = TempStoreString(rgwchValOld,cchValOld,&_rec._pszValOld,&_rec._cchValOld);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = TempStoreString(rgwchValNew,cchValNew,&_rec._pszValNew,&_rec._cchValNew);
    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}
