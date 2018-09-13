/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#include "core.hxx"
#pragma hdrstop

#include "core/io/printstream.hxx"

DEFINE_CLASS_MEMBERS(PrintStream, _T("PrintStream"), OutputStream);

/**
 */
PrintStream *
PrintStream::newPrintStream(OutputStream * out, bool autoFlush)
{
    PrintStream * p = new PrintStream();
    p->out = out;
    p->autoFlush = autoFlush;
    return p;
}

/** Check to make sure that the stream has not been closed */
void PrintStream::ensureOpen() //throws IOException 
{
    if (out == null)
        Exception::throwE(E_FAIL);
}

/**
 */
void PrintStream::flush() 
{
    SYNCHRONIZED(this);
    {
        TRY 
        {
            ensureOpen();
            out->flush();
        }
        CATCH
        {
            trouble = true;
        }
        ENDTRY

    }
}

/**
 */
void PrintStream::close() 
{
    SYNCHRONIZED(this);
    {
        TRY 
        {
            out->close();
        }
        CATCH
        {
            trouble = true;
        }
        ENDTRY

        out = null;
    }
}

/**
 */
bool PrintStream::checkError() 
{
    if (out != null)
        flush();
    return trouble;
}

/** Indicate that an error has occurred. */
void PrintStream::setError() 
{
    trouble = true;
}

/*
 */

/**
 */
void PrintStream::write(int b) 
{
    TRY {
        SYNCHRONIZED(this)
        {
            ensureOpen();
            out->write(b);
            if ((b == '\n') && autoFlush)
                out->flush();
        }
    }
// BUGBUG
//    catch (InterruptedIOException * x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
    }
    ENDTRY

}

HRESULT STDMETHODCALLTYPE PrintStream::Write(void const* pv, ULONG cb, ULONG * pcbWritten)
{
    TRY 
    {
        SYNCHRONIZED(this) 
        {
            ensureOpen();
            ATCHAR * buf = new (cb) ATCHAR;
            byte * p = (byte *)pv;
            for (UINT i = 0; i < cb; i++)
                (*buf)[i] = *(p + i);
            out->write(buf, 0, buf->length());
            if (autoFlush)
                out->flush();
            *pcbWritten = cb;
        }
    }
// BUGBUG
//    catch (InterruptedIOException x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
        return E_FAIL;
    }
    ENDTRY


    return S_OK;
}

/**
 */
void PrintStream::write(abyte * buf, int off, int len) 
{
    TRY 
    {
        SYNCHRONIZED(this) 
        {
            ensureOpen();
            out->write(buf, off, len);
            if (autoFlush)
                out->flush();
        }
    }
// BUGBUG
//    catch (InterruptedIOException x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
    }
    ENDTRY

}

/*
 */
void PrintStream::write(ATCHAR * buf) 
{
    TRY 
    {
        SYNCHRONIZED(this); 
        {
            ensureOpen();

            out->write(buf, 0, buf->length());

            if (autoFlush) 
            {
                for (int i = 0; i < buf->length(); i++)
                {
                    if ((*buf)[i] == '\n')
                    {
                        out->flush();
                        break;
                    }
                }
            }
        }
    }
// BUGBUG
//    catch (InterruptedIOException x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
    }
    ENDTRY

}

void PrintStream::write(String * s) 
{
    TRY 
    {
        SYNCHRONIZED(this); 
        {    
            ensureOpen();

            int l = s->length();
            ATCHAR * buf = new (l) ATCHAR;
            s->getChars(0, l, buf, 0);
            out->write(buf, 0, buf->length());

            if (autoFlush && (s->indexOf('\n') >= 0))
                out->flush();
        }
    }
// BUGBUG
//    catch (InterruptedIOException x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
    }
    ENDTRY

}

void PrintStream::newLine() 
{
    TRY 
    {
        SYNCHRONIZED(this); 
        {
            ensureOpen();

#ifdef WIN32
            out->write(0xd);
            out->write(0xa);
#else
            out->write('\n');
#endif

            if (autoFlush)
                out->flush();
        }
    }
// BUGBUG
//    catch (InterruptedIOException x) 
//    {
//        Thread.currentThread().interrupt();
//    }
    CATCH
    {
        trouble = true;
    }
    ENDTRY

}

/* Methods that do not terminate lines */

/**
 */
void PrintStream::print(bool b) 
{
    write(b ? String::newString(_T("true")) : String::newString(_T("false")));
}

/**
 */
void PrintStream::print(TCHAR c) 
{
    write(String::valueOf(c));
}

/**
 */
void PrintStream::print(int i) 
{
    write(String::valueOf(i));
}

#if 0 // these aren't used at the moment
/**
 */
void PrintStream::print(int64 l) 
{
    write(String::valueOf(l));
}

/**
 */
void PrintStream::print(float f) 
{
    write(String::valueOf(f));
}

/**
 */
void PrintStream::print(double d) 
{
    write(String::valueOf(d));
}
#endif

/**
 */
void PrintStream::print(ATCHAR * s) 
{
    write(s);
}

/**
 */
void PrintStream::print(String * s) 
{
    if (s == null) 
    {
        s = String::nullString();
    }
    write(s);
}

void PrintStream::print(TCHAR * c) 
{
    write(c == null ? String::nullString() : String::newString(c));
}

/**
 */
void PrintStream::print(Object * obj) 
{
    write(String::valueOf(obj));
}

/* Methods that do terminate lines */

/**
 */
void PrintStream::println() 
{
    newLine();
}

/**
 */
void PrintStream::println(bool x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

/**
 */
void PrintStream::println(TCHAR x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

/**
 */
void PrintStream::println(int x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

#if 0 // these aren't used at the moment
/**
 */
void PrintStream::println(int64 x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

/**
 */
void PrintStream::println(float x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

/**
 */
void PrintStream::println(double x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}
#endif

/**
 */
void PrintStream::println(ATCHAR * x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

/**
 */
void PrintStream::println(String * x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}

void PrintStream::println(TCHAR * x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}


/**
 */
void PrintStream::println(Object * x) 
{
    SYNCHRONIZED(this); 
    {
        print(x);
        newLine();
    }
}
