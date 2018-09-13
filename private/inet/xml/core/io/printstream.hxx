/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_PRINTSTREAM
#define _CORE_IO_PRINTSTREAM

#ifndef _CORE_IO_OUTPUTSTREAM
#include "core/io/outputstream.hxx"
#endif

#ifndef _CORE_IO_STREAM
#include "core/io/stream.hxx"
#endif

DEFINE_CLASS(PrintStream);

/**
 */

class PrintStream : public OutputStream,
                    public Stream
{

    DECLARE_CLASS_MEMBERS_I1(PrintStream, OutputStream, Stream);

    private: bool autoFlush;
    private: bool trouble;
    private: ROutputStream out;

    protected: PrintStream() {}

    /**
     */
    public: static PrintStream * newPrintStream(OutputStream * out, bool autoFlush = false);

    /** Check to make sure that the stream has not been closed */
    private: virtual void ensureOpen(); //throws IOException 

    public: virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);

    /**
     */
    public: virtual void flush();

    /**
     */
    public: virtual void close();

    /**
     */
    public: virtual bool checkError();

    /** Indicate that an error has occurred. */
    protected: virtual void setError();

    /**
     */
    public: virtual void write(int b);

    /**
     */
    public: virtual void write(abyte * buf, int off, int len);

    /*
     */

    private: virtual void write(ATCHAR * buf);

    private: virtual void write(String * s);

    private: virtual void newLine();

    /* Methods that do not terminate lines */

    /**
     */
    public: virtual void print(bool b);

    /**
     */
    public: virtual void print(TCHAR c);

    /**
     */
    public: virtual void print(int i);

#if 0 // these aren't used at the moment
    /**
     */
    public: virtual void print(int64 l);

    /**
     */
    public: virtual void print(float f);

    /**
     */
    public: virtual void print(double d);
#endif

    /**
     */
    public: virtual void print(ATCHAR * s);

    /**
     */
    public: virtual void print(String * s);
    public: virtual void print(TCHAR * s);

    /**
     */
    public: virtual void print(Object * obj);

    /* Methods that do terminate lines */

    /**
     */
    public: virtual void println();

    /**
     */
    public: virtual void println(bool x);

    /**
     */
    public: virtual void println(TCHAR x);

    /**
     */
    public: virtual void println(int x);

#if 0 // these aren't used at the moment
    /**
     */
    public: virtual void println(int64 x);

    /**
     */
    public: virtual void println(float x);

    /**
     */
    public: virtual void println(double x);
#endif

    /**
     */
    public: virtual void println(ATCHAR * x);

    /**
     */
    public: virtual void println(String * x);
    public: virtual void println(TCHAR * x);

    /**
     */
    public: virtual void println(Object * x);

    protected: virtual void finalize()
    {
        out = null;
    }
};


#endif _CORE_IO_PRINTSTREAM
