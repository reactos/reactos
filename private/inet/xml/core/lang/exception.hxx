/*
 * @(#)Exception.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_LANG_EXCEPTION
#define _CORE_LANG_EXCEPTION

class String;
typedef _reference<String> RString;
typedef unsigned long ResourceID;

DEFINE_CLASS(Exception);

#define TRY { __try 

#define CATCH __except(Exception::fillException(GetExceptionInformation()))
#define CATCHE __except(EXCEPTION_EXECUTE_HANDLER)
#define GETEXCEPTION() Exception::getException()

#ifdef UNIX
#   define ENDTRY _endexcept } 
#else
#   define ENDTRY } 
#endif


#define ERETURN return GETEXCEPTION()->getHRESULT();
#define ERESULT GETEXCEPTION()->getHRESULT();
#define ERESULTINFO   (_dispatchImpl::setErrorInfo(GETEXCEPTION()))->getHRESULT()

/**
 */
class DLLEXPORT Exception : public Base
{
    DECLARE_CLASS_MEMBERS(Exception, Base);
    DECLARE_CLASS_INSTANCE(Exception, Base);

    enum NewNoException { NewNoExceptionEnum };

public: void * __cdecl operator new(size_t cb, NewNoException);

    // we need this because we throw an exception as part of out-of-mem situations...
public: static void classInit();
public: static void classExit();

public: 

    // protected so it cannot be invoked from normal code !
    protected: Exception(String * s = null);

    public: static Exception * newException(String * s = null);

    public: static Exception * newException(HRESULT hr,String * s = null);

    public: static Exception * newException(HRESULT hresult, ResourceID resid, String* first, ...);

    /**
     * Constructor with detailed message
     */
    public: static void throwE(String * s, HRESULT hr = 0);

    /**
     * Constructor with detailed message
     */
    public: static void throwE(const TCHAR * c);

    /**
     * Constructor with HRESULT
     */
    public: static void throwE(HRESULT hresult);

    /**
     * Constructor with detailed message
     */
    public: static void throwE(HRESULT hresult, ResourceID resid, String* first, ...);

    /**
     * Parser exceptions
     */
    public: static void throwE(HRESULT hresult, String* msg, int line, int col);

    /**
     * Constructor with Win32 LastError
     */
    public: static void throwLastError();

    /**
     * Constructor which returns static Out-of-memory object
     */
    public: static void throwEOutOfMemory();

    /**
     * Re-throw exception.
     */
    public: static void throwAgain();

    /**
     * Throw this exception
     */
    public: void throwE();

    //
    // Public methods
    //

    /**
     * Gets detailed message
     */
    public: virtual String * getMessage();

    /**
     * Gets exception string
     */
    public: virtual String * toString(); 

    /**
     * Gets HRESULT
     */
    public: HRESULT getHRESULT();

    protected: virtual void finalize();

    public: static int fillException(EXCEPTION_POINTERS * ep);
    public: static Exception * getException();
    protected: static void raiseException(Exception * e);

    public: void addDetail(String* moreDetail);

    public: String * getSourceText() { return _pSourceText; }
    public: void setSourceText(String * pSourceText);

    public: String * getUrl() { return _pUrl; }
    public: void setUrl(String * pUrl);

protected:

    /**
     * HRESULT
     */
    HRESULT hr;

    /**
     * Detailed exception message
     */
    RString detailMessage;
    RString _pUrl;
    RString _pSourceText;

public:
    int _nLine;
    int _nCol;
    int _nFilePosition;
};


#endif _CORE_LANG_EXCEPTION
