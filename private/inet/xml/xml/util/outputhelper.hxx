/*
 * @(#)OutputHelper.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */


#ifndef _XML_OUTPUTHELPER
#define _XML_OUTPUTHELPER

DEFINE_CLASS(OutputHelper)
DEFINE_CLASS(Name);

typedef _reference<IStream> RStream;

class OutputHelper : public Base
{
    DECLARE_CLASS_MEMBERS(OutputHelper, Base);

protected:
    OutputHelper() { };
    static const int BUFFERSIZE;

public:

    enum
	{
        DEFAULT = 0, // default
	    PRETTY = 1, // pretty output
	    COMPACT = 2, // no new lines or tabs.
	};

    // encoding is needed for error messages about un-encodable characters.
    static OutputHelper* newOutputHelper(IStream* o, int style, String * encoding = null);

    /**
     * Write the Unicode character.
     */
    void write(WCHAR c);

    /**
     * Write the given string.
     */
    void write(const String * str);
    void write(const WCHAR * c);
    void write(const WCHAR * c, int len);
    void write(ATCHAR *str)
    {
        if (str)
        {
            write(str->getData(), str->length());
        }
    }


    /**
     * Write the name
     */
    void write(NameDef * n);

	/**
     * Write out the string and escape quote '"' in it
     */
    void writeString(const String * str, bool fIsPCData = false);
    void writeString(const WCHAR * c, int len = -1, bool fIsPCData = false);
    void writeString(ATCHAR *str)
    {
        if (str)
        {
            writeString(str->getData(), str->length(), true);
        }
    }

    /**
     * Write a new line or do nothing if output style is COMPACT.
     */
    void writeNewLine();

    /**
     * Write the appropriate indent - given current indent level,
     * or do nothing if output style is COMPACT.
     */
    void writeIndent();

    void addIndent(int offset) { _iIndent += offset; }
    int getOutputStyle() { return _iStyle; }
    void setOutputStyle(int s) { _iStyle = s; }

    void flush()
    {
        // BUGBUG - because QueryInterface for IID_IStream doesn't know how
        // the stream is going to be used (read versus write), we have to 
        // at least call write once - even if there's no bytes to write to 
        // tell the stream that this was a save operation.  This way the stream
        // can generate errors if zero bytes is not valid (like in the 
        // xmldoc.save(doc2) case. (bug 62865).
        _hardWrite();
    }

    void close();

    bool _fEncoding; // whether we are honoring XML encoding

protected:

    void actuallyWriteIndent();

    void _write( WCHAR c)
    {
        _pchBuf[_iUsed++] = c;
        _chLast = c;
        if (_iUsed == BUFFERSIZE)
            _hardWrite();
    }

    void _hardWrite();
    virtual void finalize();

private:

    bool        _fHoldingIndent;
    int         _iIndent;
    WCHAR       _chLast;
    int         _iStyle;
    int         _iUsed;
    WCHAR*      _pchBuf;
    bool        _fWriteError;
    RStream     _pStm;
    RString     _pEncoding;
};

#endif _XML_OUTPUTHELPER