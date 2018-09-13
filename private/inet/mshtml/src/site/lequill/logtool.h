#ifndef I_LOGTOOL_H_
#define I_LOGTOOL_H_


// theory of operation
//
// The library contains a CLogBuffer object which implements fast reading and
// writing contiguous stream data.  The CLogBufferPoolManager is intended to
// implement reuse of allocated blocks.  (currently it does nothing).
// By initializing CLogBuffer with a pool mgr, you create an output (write)
// buffer.  By initializing with a block of memory, you create an
// input (read) buffer.
//
// There are a set of classes for each type of log record, which can be
// used to either read or write the log records.
//
// If initialized with
// a buffer, they read a record from the buffer and initialize their
// internal fields with it.  Such a class is in read mode and can be
// queried for field values, but not updated.
// IMPORTANT:  string attributes are direct pointers into the buffer,
// so the buffer block must continue to exist as long as any read-mode
// record objects initialized from it continue to exist.
// IMPORTANT(2): when initializing a buffer from a stream, the buffer
// assumes that both the LEADING-LENGTH field and the OPCODE field have
// already been read from the buffer.  This assumption is made because
// most code scanning a buffer will be doing this in order to select
// a branch of the switch statement.
//
// If initialized with no arguments, the object is a write-record.
// the attributes can be updated.  When the Write()
// method is called, the data is written to the buffer (which must be
// an output buffer).  After a write has been performed, the object
// is useless and should be discarded.
//
// To facilitate log transformations, a read record object may be tranformed
// into a write record object by calling MutateToWrite().  This allows
// code to scan a buffer, reading records, turning them into write records,
// modify their fields, and turn around and write them to another stream.
// (internally, the object copies all string attributes so they no longer
// depend on underlying buffers and can be changed).


// forwards
class CLogBuffer;
class CLogBufferPoolManager;


// tree operations
typedef enum tagTreeOps
{
    synclogInsertText  = 1,  // cp, cch, unicode text          NB: this is text within a run only
    synclogInsertElement = 3,// cpStart, cpFinish, tagid, cchAtrs, szAttrs
    synclogDeleteElement = 4,// cpStart, cpFinish, tagid, cchAtrs, szAttrs
                             // NB: cpStart and cpFinish must be the actual begin/end of the element
    synclogInsertTree  = 5,  // cpStart, cpFinish, cchHTML, szHTML   NB: HTML must be well formed
                             // NB: cpStart and cpFinish must the be actual begin/end of the subtree.
    synclogChangeAttr  = 7,  // cpElementStart, cchName, szName, dwBits, cchValOld, szValOld, cchValNew, szValNew
                             // dwBits:  0x01 = old isn't NULL  |  0x02 = new isn't null
    synclogCutCopyMove = 8,  // cpStart, cpFinish, cpTarget (-1 if not in tree), fRemove
} TreeOps;


// bit fields for synclogChangeAttr
#define SYNCLOG_CHANGEATTR_OLDNOTNULL ((DWORD)0x01)
#define SYNCLOG_CHANGEATTR_NEWNOTNULL ((DWORD)0x02)


// manages store of large allocated buffer blocks
class CLogBufferPoolManager
{
public:

    CLogBufferPoolManager();
    ~CLogBufferPoolManager();
};


// supply buffer read/write capability
class CLogBuffer
{
public:

    CLogBuffer();
    ~CLogBuffer();

    // initialize new buffer for writing
    HRESULT Init(CLogBufferPoolManager *ppool);

    // initialize buffer for reading
    void Init(void *pvBuf, DWORD dwBufLen);

    // shut down the buffer
    void Shutdown();

    HRESULT AppendOp(TreeOps op);
    HRESULT AppendDWORD(DWORD dw);
    HRESULT AppendString(WCHAR *rgwch, DWORD cwch);

    HRESULT ReserveDWORD(DWORD *pdwIndexOut);
    HRESULT UpdateDWORD(DWORD dwIndex, DWORD dwValue);

    HRESULT ReadOp(TreeOps *popOut);
    HRESULT ReadDWORD(DWORD *pdwOut);
    // IMPORTANT: this method returns a pointer to the embedded string.
    // the classes just hold onto the pointer.  This is much more efficient
    // than copying the string, but it means the log buffer must stay
    // intact until the object has been destroyed.
    HRESULT ReadString(WCHAR **prgwch, DWORD *pcwch);

    HRESULT SkipDWORD()
    {
        return ReadDWORD(NULL);
    }

    HRESULT BackReadPrevDWORD(DWORD *pdw);

    inline DWORD GetIndex()
    {
        return _dwBufIndex;
    }

    inline void SetIndex(DWORD dwBufIndex)
    {
        Assert(!_fWriteMode);
        Assert(dwBufIndex <= _dwBufLen);
        _dwBufIndex = dwBufIndex;
    }

    inline void *GetBuffer()
    {
        return _prgbBuf;
    }

    inline bool IsEndOfBuffer()
    {
        Assert(!_fWriteMode);
        return (_dwBufIndex >= _dwBufLen);
    }

private:

    HRESULT EnsureSize(DWORD dwNewIndex);

    CLogBufferPoolManager  *_ppool;
    BYTE        *_prgbBuf;
    DWORD       _dwBufLen;
    DWORD       _dwBufIndex;
    bool        _fWriteMode;
    bool        _fZombie;
};


// base class for records, which includes string mem-var utils
class CLogRecord
{
public:

    // no constructor or destructor

protected:

    // return an allocated temporary string
    HRESULT TempStoreString(WCHAR *rgwch, DWORD cwch, WCHAR **prgwch, DWORD *pcwch);

    // free the string allocated with TempStoreString
    void FreeTempString(WCHAR *rgwch, DWORD cwch);

    // get info needed to put length of log record into log
    HRESULT DelimitLengthSetup(CLogBuffer *pbuf, DWORD *pdwLeadingLengthLocation, DWORD *pdwInitialIndex);

    // finish setting up the log record length
    HRESULT DelimitLengthFinish(CLogBuffer *pbuf, DWORD dwLeadingLengthLocatin, DWORD dwInitialIndex);
};


// insert text struct
typedef struct tagINSERTTEXTREC
{
    DWORD       _cp;
    DWORD       _cch;
    WCHAR *     _rgwch;
} INSERTTEXTREC;

// insert text
class CInsertTextLogRecord : CLogRecord
{
public:

    // create insert text record by reading from buffer
    CInsertTextLogRecord(CLogBuffer *pbuf, HRESULT *phr);
    CInsertTextLogRecord(INSERTTEXTREC *prec);

    // create insert text record for writing to buffer
    CInsertTextLogRecord();

    ~CInsertTextLogRecord();

    DWORD GetCp()                       {Assert(_fValid_dbg); return _rec._cp;}
    DWORD GetCch()                      {Assert(_fValid_dbg); return _rec._cch;}
    WCHAR *GetText()                    {Assert(_fValid_dbg); return _rec._rgwch;}

    void SetCp(DWORD cp)                {Assert(!_fWritten_dbg && _fWrite); _rec._cp = cp;}
    HRESULT SetText(WCHAR *psz, DWORD cch);

    HRESULT Write(CLogBuffer *pbuf);

    HRESULT MutateToWrite();

    INSERTTEXTREC *GetRec()             {Assert(_fValid_dbg); return &_rec;}

private:

    INSERTTEXTREC   _rec;
#if DBG
    bool            _fValid_dbg;
    bool            _fWritten_dbg;
#endif
    bool            _fWrite;
};


// insert/delete element struct
typedef struct tagINSDELELEMENTREC
{
    DWORD       _cpStart;
    DWORD       _cpFinish;
    DWORD       _tagid;
    DWORD       _cchAttrs;
    WCHAR *     _pszAttrs;
} INSDELELEMENTREC;

// insert/delete element base class
class CInsertDeleteElementLogRecord : CLogRecord
{
public:

    // create insert element by reading from buffer
    CInsertDeleteElementLogRecord(CLogBuffer *pbuf, HRESULT *phr);
    CInsertDeleteElementLogRecord(INSDELELEMENTREC *prec);

    // create insert element for writing to buffer
    CInsertDeleteElementLogRecord();

    ~CInsertDeleteElementLogRecord();

    DWORD GetCpStart()                  {Assert(_fValid_dbg); return _rec._cpStart;}
    DWORD GetCpFinish()                 {Assert(_fValid_dbg); return _rec._cpFinish;}
    DWORD GetTagId()                    {Assert(_fValid_dbg); return _rec._tagid;}
    DWORD GetCchAttrs()                 {Assert(_fValid_dbg); return _rec._cchAttrs;}
    WCHAR *GetAttrs()                   {Assert(_fValid_dbg); return _rec._pszAttrs;}

    void SetCpStart(DWORD cpStart)      {Assert(!_fWritten_dbg && _fWrite); _rec._cpStart = cpStart;}
    void SetCpFinish(DWORD cpFinish)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpFinish = cpFinish;}
    void SetTagId(DWORD tagid)          {Assert(!_fWritten_dbg && _fWrite); _rec._tagid = tagid;}
    HRESULT SetAttrs(WCHAR *pszAttrs, DWORD cchAttrs);

    HRESULT Write(CLogBuffer *pbuf);

    HRESULT MutateToWrite();

    INSDELELEMENTREC *GetRec()          {Assert(_fValid_dbg); return &_rec;}

protected:

    virtual TreeOps GetOpcode() = 0;

private:

    INSDELELEMENTREC    _rec;
#if DBG
    bool                _fValid_dbg;
    bool                _fWritten_dbg;
#endif
    bool                _fWrite;
};

class CInsertElementLogRecord : public CInsertDeleteElementLogRecord
{
public:

    CInsertElementLogRecord(CLogBuffer *pbuf, HRESULT *phr)
        : CInsertDeleteElementLogRecord(pbuf,phr)
    {
    }

    CInsertElementLogRecord(INSDELELEMENTREC *prec)
        : CInsertDeleteElementLogRecord(prec)
    {
    }

    CInsertElementLogRecord()
        : CInsertDeleteElementLogRecord()
    {
    }

protected:

    virtual TreeOps GetOpcode();
};

class CDeleteElementLogRecord : public CInsertDeleteElementLogRecord
{
public:

    CDeleteElementLogRecord(CLogBuffer *pbuf, HRESULT *phr)
        : CInsertDeleteElementLogRecord(pbuf,phr)
    {
    }

    CDeleteElementLogRecord(INSDELELEMENTREC *prec)
        : CInsertDeleteElementLogRecord(prec)
    {
    }

    CDeleteElementLogRecord()
        : CInsertDeleteElementLogRecord()
    {
    }

protected:

    virtual TreeOps GetOpcode();
};


// insert tree struct
typedef struct tagINSERTTREEREC
{
    DWORD       _cpStart;
    DWORD       _cpFinish;
    DWORD       _cpTarget;
    DWORD       _cchHTML;
    WCHAR *     _pszHTML;
} INSERTTREEREC;

// insert tree
class CInsertTreeLogRecord : CLogRecord
{
public:

    // create insert tree by reading from buffer
    CInsertTreeLogRecord(CLogBuffer *pbuf, HRESULT *phr);
    CInsertTreeLogRecord(INSERTTREEREC *prec);

    // create insert element for writing to buffer
    CInsertTreeLogRecord();

    ~CInsertTreeLogRecord();

    DWORD GetCpStart()                  {Assert(_fValid_dbg); return _rec._cpStart;}
    DWORD GetCpFinish()                 {Assert(_fValid_dbg); return _rec._cpFinish;}
    DWORD GetCpTarget()                 {Assert(_fValid_dbg); return _rec._cpTarget;}
    DWORD GetCchHTML()                  {Assert(_fValid_dbg); return _rec._cchHTML;}
    WCHAR *GetHTML()                    {Assert(_fValid_dbg); return _rec._pszHTML;}

    void SetCpStart(DWORD cpStart)      {Assert(!_fWritten_dbg && _fWrite); _rec._cpStart = cpStart;}
    void SetCpFinish(DWORD cpFinish)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpFinish = cpFinish;}
    void SetCpTarget(DWORD cpTarget)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpTarget = cpTarget;}
    HRESULT SetHTML(WCHAR *pszHTML, DWORD cchHTML);

    HRESULT Write(CLogBuffer *pbuf);

    HRESULT MutateToWrite();

    INSERTTREEREC *GetRec()             {Assert(_fValid_dbg); return &_rec;}

private:

    INSERTTREEREC   _rec;
#if DBG
    bool            _fValid_dbg;
    bool            _fWritten_dbg;
#endif
    bool            _fWrite;
};


// cut-copy-move struct
typedef struct tagCUTCOPYMOVEREC
{
    DWORD       _cpStart;
    DWORD       _cpFinish;
    DWORD       _cpTarget;
    DWORD       _fRemove;
    DWORD       _cchOldHTML;
    WCHAR *     _pszOldHTML;
    DWORD       _cpOldHTMLStart;
    DWORD       _cpOldHTMLFinish;
    DWORD       _cpPostOpSrcStart;
    DWORD       _cpPostOpSrcFinish;
    DWORD       _cpPostOpTgtStart;
    DWORD       _cpPostOpTgtFinish;
} CUTCOPYMOVEREC;

// cut-copy-move
class CCutCopyMoveTreeLogRecord : CLogRecord
{
public:

    // create insert tree by reading from buffer
    CCutCopyMoveTreeLogRecord(CLogBuffer *pbuf, HRESULT *phr);
    CCutCopyMoveTreeLogRecord(CUTCOPYMOVEREC *prec);

    // create insert element for writing to buffer
    CCutCopyMoveTreeLogRecord();
    ~CCutCopyMoveTreeLogRecord();

    DWORD GetCpStart()                  {Assert(_fValid_dbg); return _rec._cpStart;}
    DWORD GetCpFinish()                 {Assert(_fValid_dbg); return _rec._cpFinish;}
    DWORD GetCpTarget()                 {Assert(_fValid_dbg); return _rec._cpTarget;}
    BOOL GetFRemove()                   {Assert(_fValid_dbg); return (_rec._fRemove != 0);}
    DWORD GetCchOldHTML()               {Assert(_fValid_dbg); return _rec._cchOldHTML;}
    WCHAR *GetOldHTML()                 {Assert(_fValid_dbg); return _rec._pszOldHTML;}
    DWORD GetCpOldHTMLStart()           {Assert(_fValid_dbg); return _rec._cpOldHTMLStart;}
    DWORD GetCpOldHTMLFinish()          {Assert(_fValid_dbg); return _rec._cpOldHTMLFinish;}
    DWORD GetCpPostOpSrcStart()         {Assert(_fValid_dbg); return _rec._cpPostOpSrcStart;}
    DWORD GetCpPostOpSrcFinish()        {Assert(_fValid_dbg); return _rec._cpPostOpSrcFinish;}
    DWORD GetCpPostOpTgtStart()         {Assert(_fValid_dbg); return _rec._cpPostOpTgtStart;}
    DWORD GetCpPostOpTgtFinish()        {Assert(_fValid_dbg); return _rec._cpPostOpTgtFinish;}

    void SetCpStart(DWORD cpStart)      {Assert(!_fWritten_dbg && _fWrite); _rec._cpStart = cpStart;}
    void SetCpFinish(DWORD cpFinish)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpFinish = cpFinish;}
    void SetCpTarget(DWORD cpTarget)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpTarget = cpTarget;}
    void SetFRemove(BOOL fRemove)       {Assert(!_fWritten_dbg && _fWrite); _rec._fRemove = (fRemove != 0);}
    HRESULT SetOldHTML(WCHAR *psz, DWORD cch);
    void SetCpOldHTMLStart(DWORD cp)    {Assert(!_fWritten_dbg && _fWrite); _rec._cpOldHTMLStart = cp;}
    void SetCpOldHTMLFinish(DWORD cp)   {Assert(!_fWritten_dbg && _fWrite); _rec._cpOldHTMLFinish = cp;}
    void SetCpPostOpSrcStart(DWORD cp)  {Assert(!_fWritten_dbg && _fWrite); _rec._cpPostOpSrcStart = cp;}
    void SetCpPostOpSrcFinish(DWORD cp) {Assert(!_fWritten_dbg && _fWrite); _rec._cpPostOpSrcFinish = cp;}
    void SetCpPostOpTgtStart(DWORD cp)  {Assert(!_fWritten_dbg && _fWrite); _rec._cpPostOpTgtStart = cp;}
    void SetCpPostOpTgtFinish(DWORD cp) {Assert(!_fWritten_dbg && _fWrite); _rec._cpPostOpTgtFinish = cp;}

    HRESULT Write(CLogBuffer *pbuf);

    HRESULT MutateToWrite();

    CUTCOPYMOVEREC *GetRec()            {Assert(_fValid_dbg); return &_rec;}

private:

    CUTCOPYMOVEREC  _rec;
#if DBG
    bool            _fValid_dbg;
    bool            _fWritten_dbg;
#endif
    bool            _fWrite;
};



// change attribute struct
typedef struct tagCHANGEATTRREC
{
    DWORD       _cpStart;
    DWORD       _cchName;
    WCHAR *     _pszName;
    DWORD       _dwBits;
    DWORD       _cchValOld;
    WCHAR *     _pszValOld;
    DWORD       _cchValNew;
    WCHAR *     _pszValNew;
} CHANGEATTRREC;

// change attribute
class CChangeAttrLogRecord : CLogRecord
{
public:

    // create insert tree by reading from buffer
    CChangeAttrLogRecord(CLogBuffer *pbuf, HRESULT *phr);
    CChangeAttrLogRecord(CHANGEATTRREC *prec);

    // create insert element for writing to buffer
    CChangeAttrLogRecord();

    ~CChangeAttrLogRecord();

    DWORD GetCpElementStart()           {Assert(_fValid_dbg); return _rec._cpStart;}
    DWORD GetCchName()                  {Assert(_fValid_dbg); return _rec._cchName;}
    WCHAR *GetName()                    {Assert(_fValid_dbg); return _rec._pszName;}
    DWORD GetDWBits()                   {Assert(_fValid_dbg); return _rec._dwBits;}
    DWORD GetCchValOld()                {Assert(_fValid_dbg); return _rec._cchValOld;}
    WCHAR *GetSzValOld()                {Assert(_fValid_dbg); return _rec._pszValOld;}
    DWORD GetCchValNew()                {Assert(_fValid_dbg); return _rec._cchValNew;}
    WCHAR *GetSzValNew()                {Assert(_fValid_dbg); return _rec._pszValNew;}

    void SetCpElementStart(DWORD cpStart)   {Assert(!_fWritten_dbg && _fWrite); _rec._cpStart = cpStart;}
    HRESULT SetName(WCHAR *psz, DWORD cch);
    void SetDWBits(DWORD dwBits)            {Assert(!_fWritten_dbg && _fWrite); _rec._dwBits = dwBits;}
    HRESULT SetSzValOld(WCHAR *psz, DWORD cch);
    HRESULT SetSzValNew(WCHAR *psz, DWORD cch);

    HRESULT Write(CLogBuffer *pbuf);

    HRESULT MutateToWrite();

    CHANGEATTRREC *GetRec()             {Assert(_fValid_dbg); return &_rec;}

private:

    CHANGEATTRREC   _rec;
#if DBG
    bool            _fValid_dbg;
    bool            _fWritten_dbg;
#endif
    bool            _fWrite;
};


// handy union thing
typedef union tagALLLOGUNION
{
    INSERTTEXTREC       _inserttext;
    INSDELELEMENTREC    _insdelelement;
    INSERTTREEREC       _inserttree;
    CUTCOPYMOVEREC      _cutcopymove;
    CHANGEATTRREC       _changeattr;
} ALLLOGUNION;


#endif
