/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _BUFFEREDSTREAM_HXX
#define _BUFFEREDSTREAM_HXX

#include "encoder/encodingstream.hxx"

// Returned from nextChar when a new buffer is read.  This gives the
// caller some idea of download progress without having to count
// characters.  Just call nextChar again to continue on as normal.
#define E_DATA_AVAILABLE 0xC00CE600L
#define E_DATA_REALLOCATE 0xC00CE601L

//------------------------------------------------------------------------
// This class adds buffering and auto-growing semantics to an IStream
// so that a variable length chunk of an IStream can be collected
// in memory for processing using Mark() and getToken() methods.
// It also supports collapsing of newlines into 0x20 if you use 
// nextChar2 instead of nextChar.
// It also guarentees a line buffer so that a pointer to the 
// beginning of the line can be returned in error conditions.
// (for the degenerate case where there are no new lines, it
// returns the last 100 characters).
//
// Alternatively, buffers can be appended instead of
// using an IStream.  In this case the BufferedStream returns
// E_PENDING until the last buffer is appended.  Use AppendData instead
// of Load(IStream.  

class XMLStream;

class BufferedStream 
{   
public:
    BufferedStream(XMLStream *pXMLStream);
    ~BufferedStream();

    // Method 1: pass in an IStream.  The IStream must return unicode
    // characters.
    HRESULT Load( 
        /* [unique][in] */ EncodingStream  *pStm);
    
    // Method 2: append raw buffers, set lastBuffer to TRUE you are ready to
    // return E_ENDOFINPUT.  Length is number of chars in buffer.  To do unicode
    // you must provide a byte order mark (0xFFFE or OxFEFF depending
    // on whether it is bigendian or little endian).
    HRESULT AppendData( const BYTE* buffer, ULONG length, BOOL lastBuffer);

    HRESULT Reset();

    HRESULT nextChar( 
        /* [out] */ WCHAR* ch,
        /* [out] */ bool* fEOF);

    // Marks the last character read as the start of a buffer
    // that grows until Mark is called again.  You can mark backwards
    // from last character read anywhere up to last marked position
    // by passing a non-zero delta.  For example, to mark the
    // position at the 3rd last character read, call Mark(3);
    inline void Mark(long back = 0) 
    {
        _lMark = (_lCurrent > back) ? (_lCurrent - back - 1) : 0;
        if (_lLinepos != _lCurrent)
        {
            // only move the marked line position forward, if we haven't
            // marked the actual new line characters.  This ensures we
            // return useful information from getLineBuf.
            _lMarkedline = _lLine;
            _lMarkedlinepos = _lLinepos;
        }
    }

        // Returns a pointer to a contiguous block of text accumulated 
        // from the last time Mark() was called up to but not including
        // the last character read. (This allows a parser to have a
        // lookahead character that is not included in the token).
    HRESULT getToken(const WCHAR**p, long* len);

    HRESULT switchEncoding(const TCHAR * charset, ULONG len);

    // Returns Marked position.
    long getLine();
    long getLinePos();
    WCHAR* getLineBuf(ULONG* len, ULONG* startpos);
    long getInputPos(); // absolute position.

    long getTokenLength() // convenience function.
    {
        return (_lCurrent - 1 - _lMark);
    }

    inline bool isWhiteSpace(WCHAR ch)
    {
        return (_lLastWhiteSpace == _lCurrent);
    }
    inline void setWhiteSpace(bool flag = true)
    {
        _lLastWhiteSpace = flag ? _lCurrent : _lCurrent-1;
    }

    void init();

    // Lock/UnLock is another level on top of Mark/Reset that 
    // works as follows. If you Lock(), then the buffer keeps everything
    // until you UnLock at which time it resets the "Marked" position to
    // the Locked() position.  This is so that you can scan through
    // a series of tokens, but then return all of them in one chunk.
    void Lock();
    void UnLock();

    // Freezing the buffer makes the buffer always grow WITHOUT shifting
    // data around in the buffer.  This makes it valid to hold on to pointers
    // in the buffer until the buffer is unfrozen.
    HRESULT Freeze();
    HRESULT UnFreeze();

    WCHAR*  getEncoding();

    // Special XML optimization.
    HRESULT scanPCData( 
        /* [out] */ WCHAR* ch,
        /* [out] */ bool* fWhitespace);

private:
    WCHAR nextChar();

    HRESULT fillBuffer();
    HRESULT prepareForInput();
    HRESULT doSwitchEncoding();
    long    getNewStart();

    REncodingStream _pStmInput; // input stream
    WCHAR*  _pchBuffer; // buffer containing chars from input stream.
    long    _lCurrent; // current read position in buffer
    long    _lCurrent2; // used when collapsing white space.
    long    _lSize; // total size of buffer.
    long    _lMark; // start of current token.
    long    _lUsed; // amount of buffer currently used.
    WCHAR   _chLast; // last character returned.
    long    _lLine; // current line number
    long    _lLinepos; // position of start of last line.
    long    _lMarkedline; // current line number of marked position.
    long    _lMarkedlinepos; 
    long    _lStartAt; // The number of unicode characters before the current buffer
    bool    _fEof;
    bool    _fNotified;
    bool    _fFrozen;
    long    _lLockCount;
    long    _lLockedPos;
    long    _lLockedLine;
    long    _lLockedLinePos;
    long    _lLastWhiteSpace;
    long    _lMidPoint; 
    Encoding* _pPendingEncoding;
    XMLStream *_pXMLStream; // regular pointer pointing back to the XMLStream object
};

#endif // _BUFFEREDSTREAM_HXX
