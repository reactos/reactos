/*
 * @(#)EncodingStream.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _ENCODINGSTREAM_HXX
#define _ENCODINGSTREAM_HXX

#include "charencoder.hxx"
#include <objbase.h>

typedef _reference<IStream> RStream;

/**
 */

class EncodingStream : public _unknown<IStream, &IID_IStream>
{
protected:

    EncodingStream(IStream * stream);
    ~EncodingStream();

public:
    ///////////////////////////////////////////////////////////
    // Constructor functions
    //

    /**
     * Builds the EncodingStream for input
     * Reads the first four bytes of the InputStream * in order to make a guess
     * as to the character encoding of the file.
     * Assumes that the document is following the XML standard and that
     * any non-UTF-8 file will begin with a <?XML> tag.
     */
    static IStream * newEncodingStream(IStream * stream);

    /**
     * Builds the EncodingStream for output
     * It is created as a standard UTF-8 output stream if <code> encoding </> is null.
     */
    static IStream * newEncodingStream(IStream * stream, Encoding * encoding);

    ///////////////////////////////////////////////////////////
    // IStream Interface
    //
    HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead);

    HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
    {
        return E_NOTIMPL;
    } 
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
    {
        return stream->Commit(grfCommitFlags);
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }

    ///////////////////////////////////////////////////////////
    // public methods
    //

    /**
     * Defines the character encoding of the input stream.  
     * The new character encoding must agree with the encoding determined by the constructer.  
     * setEncoding is used to clarify between encodings that are not fully determinable 
     * through the first four bytes in a stream and not to change the encoding.
     * This method must be called within BUFFERSIZE reads() after construction.
     */
    HRESULT switchEncodingAt(Encoding * newEncoding, int newPosition);

    /**
     * Gets encoding
     */
    Encoding * getEncoding();

    // For Read EncodingStreams, this method can be used to push raw data
    // which is an alternate approach to providing another IStream.
    HRESULT AppendData(const BYTE* buffer, ULONG length, BOOL lastBuffer);

    HRESULT BufferData();

    void setReadStream(bool flag) { _fReadStream = flag; }

    void SetMimeType(const WCHAR * pwszMimeType, int length);
    void SetCharset(const WCHAR * pwszCharset, int length);

private:

    /**
     * Buffer size
     */
    static const int BUFFERSIZE;  

    HRESULT autoDetect();

    HRESULT prepareForInput(ULONG minlen);
    
private:

    /**
     * Character encoding variables
     */ 
    CODEPAGE codepage;   // code page number
    Encoding * encoding; // encoding
    bool  _fTextXML;     // MIME type, true: "text/xml", false: "application/xml"
    bool  _fSetCharset;  // Whether the charset has been set from outside. e.g, when mime type text/xml or application/xml
                         // has charset parameter

    /**
     * Multibyte buffer
     */
    BYTE * buf;        // storage for multibyte bytes
    ULONG bufsize;
    UINT bnext;        // point to next available byte in the rawbuffer
    ULONG btotal;      // total number of bytes in the rawbuffer
    int startAt;       // where the buffer starts at in the input stream 

    /**
     * Function pointer to convert from multibyte to unicode
     */
    WideCharFromMultiByteFunc * pfnWideCharFromMultiByte;

    /**
     * Function pointer to convert from unicode to multibytes
     */
    WideCharToMultiByteFunc * pfnWideCharToMultiByte;

    UINT maxCharSize;   // maximum number of bytes of a wide char

    RStream stream;
    bool isInput;
    bool lastBuffer;
    bool _fEOF;
    bool _fUTF8BOM;
    bool _fReadStream; // lets Read() method call Read() on wrapped stream object.

    DWORD _dwMode; // MLANG context.
};

typedef _reference<EncodingStream> REncodingStream;

#endif _ENCODINGSTREAM_HXX
