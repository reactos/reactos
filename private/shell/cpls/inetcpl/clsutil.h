//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  CLSUTIL.H - header file for utility C++ classes
//

//  HISTORY:
//  
//  12/07/94    jeremys     Borrowed from WNET common library
//

#ifndef _CLSUTIL_H_
#define _CLSUTIL_H_

/*************************************************************************

    NAME:       BUFFER_BASE

    SYNOPSIS:   Base class for transient buffer classes

    INTERFACE:  BUFFER_BASE()
                    Construct with optional size of buffer to allocate.

                Resize()
                    Resize buffer to specified size.  Returns TRUE if
                    successful.

                QuerySize()
                    Return the current size of the buffer in bytes.

                QueryPtr()
                    Return a pointer to the buffer.

    PARENT:     None

    USES:       None

    CAVEATS:    This is an abstract class, which unifies the interface
                of BUFFER, GLOBAL_BUFFER, etc.

    NOTES:      In standard OOP fashion, the buffer is deallocated in
                the destructor.

    HISTORY:
        03/24/93    gregj   Created base class

**************************************************************************/

class BUFFER_BASE
{
protected:
    UINT _cb;

    virtual BOOL Alloc( UINT cbBuffer ) = 0;
    virtual BOOL Realloc( UINT cbBuffer ) = 0;

public:
    BUFFER_BASE()
        { _cb = 0; }    // buffer not allocated yet
    ~BUFFER_BASE()
        { _cb = 0; }    // buffer size no longer valid
    BOOL Resize( UINT cbNew );
    UINT QuerySize() const { return _cb; };
};

#define GLOBAL_BUFFER   BUFFER

/*************************************************************************

    NAME:       BUFFER

    SYNOPSIS:   Wrapper class for new and delete

    INTERFACE:  BUFFER()
                    Construct with optional size of buffer to allocate.

                Resize()
                    Resize buffer to specified size.  Only works if the
                    buffer hasn't been allocated yet.

                QuerySize()
                    Return the current size of the buffer in bytes.

                QueryPtr()
                    Return a pointer to the buffer.

    PARENT:     BUFFER_BASE

    USES:       operator new, operator delete

    CAVEATS:

    NOTES:      In standard OOP fashion, the buffer is deallocated in
                the destructor.

    HISTORY:
        03/24/93    gregj   Created

**************************************************************************/

class BUFFER : public BUFFER_BASE
{
protected:
    TCHAR *_lpBuffer;

    virtual BOOL Alloc( UINT cbBuffer );
    virtual BOOL Realloc( UINT cbBuffer );

public:
    BUFFER( UINT cbInitial=0 );
    ~BUFFER();
    BOOL Resize( UINT cbNew );
    TCHAR * QueryPtr() const { return (TCHAR *)_lpBuffer; }
    operator TCHAR *() const { return (TCHAR *)_lpBuffer; }
};

class RegEntry
{
    public:
        RegEntry(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER, REGSAM regsam = KEY_READ|KEY_WRITE);
        ~RegEntry();
        
        long    GetError()  { return _error; }
        long    SetValue(const TCHAR *pszValue, const TCHAR *string);
        long    SetValue(const TCHAR *pszValue, unsigned long dwNumber);
        TCHAR * GetString(const TCHAR *pszValue, TCHAR *string, unsigned long length);
        long    GetNumber(const TCHAR *pszValue, long dwDefault = 0);
        long    DeleteValue(const TCHAR *pszValue);
        long    FlushKey();
        long    MoveToSubKey(const TCHAR *pszSubKeyName);
        HKEY    GetKey()    { return _hkey; }

    private:
        HKEY    _hkey;
        long    _error;
        BOOL    bhkeyValid;
};

class RegEnumValues
{
    public:
        RegEnumValues(RegEntry *pRegEntry);
        ~RegEnumValues();
        long    Next();
        TCHAR * GetName()       {return pchName;}
        DWORD   GetType()       {return dwType;}
        LPBYTE  GetData()       {return pbValue;}
        DWORD   GetDataLength() {return dwDataLength;}
        long    GetError()  { return _error; }

    private:
        RegEntry * pRegEntry;
        DWORD   iEnum;
        DWORD   cEntries;
        TCHAR *  pchName;
        LPBYTE  pbValue;
        DWORD   dwType;
        DWORD   dwDataLength;
        DWORD   cMaxValueName;
        DWORD   cMaxData;
        LONG    _error;
};

/*************************************************************************

    NAME:       WAITCURSOR

    SYNOPSIS:   Sets the cursor to an hourclass until object is destructed

**************************************************************************/
class WAITCURSOR
{
private:
    HCURSOR m_curOld;
    HCURSOR m_curNew;

public:
    WAITCURSOR() { m_curNew = ::LoadCursor( NULL, IDC_WAIT ); m_curOld = ::SetCursor( m_curNew ); }
    ~WAITCURSOR() { ::SetCursor( m_curOld ); }
};





/*************************************************************************

    NAME:       CAccessibleWrapper

    SYNOPSIS:   Sets the cursor to an hourclass until object is destructed

**************************************************************************/

// Generic CAccessibleWrapper class - just calls through on all methods.
// Add overriding behavior in classes derived from this.

class CAccessibleWrapper: public IAccessible,
                         public IOleWindow,
                         public IEnumVARIANT
{
        // We need to do our own refcounting for this wrapper object
        ULONG          m_ref;

        // Need ptr to the IAccessible - also keep around ptrs to EnumVar and
        // OleWindow as part of this object, so we can filter those interfaces
        // and trap their QI's...
        // (We leave pEnumVar and OleWin as NULL until we need them)
        IAccessible *  m_pAcc;
        IEnumVARIANT * m_pEnumVar;
        IOleWindow *   m_pOleWin;
public:
        CAccessibleWrapper( IAccessible * pAcc );
        virtual ~CAccessibleWrapper();

        // IUnknown
        // (We do our own ref counting)
        virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);
        virtual STDMETHODIMP_(ULONG)    AddRef();
        virtual STDMETHODIMP_(ULONG)    Release();

        // IDispatch
        virtual STDMETHODIMP            GetTypeInfoCount(UINT* pctinfo);
        virtual STDMETHODIMP            GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        virtual STDMETHODIMP            GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid);
        virtual STDMETHODIMP            Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
            UINT* puArgErr);

        // IAccessible
        virtual STDMETHODIMP            get_accParent(IDispatch ** ppdispParent);
        virtual STDMETHODIMP            get_accChildCount(long* pChildCount);
        virtual STDMETHODIMP            get_accChild(VARIANT varChild, IDispatch ** ppdispChild);

        virtual STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName);
        virtual STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        virtual STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR* pszDescription);
        virtual STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole);
        virtual STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState);
        virtual STDMETHODIMP            get_accHelp(VARIANT varChild, BSTR* pszHelp);
        virtual STDMETHODIMP            get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
        virtual STDMETHODIMP            get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
        virtual STDMETHODIMP            get_accFocus(VARIANT * pvarFocusChild);
        virtual STDMETHODIMP            get_accSelection(VARIANT * pvarSelectedChildren);
        virtual STDMETHODIMP            get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        virtual STDMETHODIMP            accSelect(long flagsSel, VARIANT varChild);
        virtual STDMETHODIMP            accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        virtual STDMETHODIMP            accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
        virtual STDMETHODIMP            accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
        virtual STDMETHODIMP            accDoDefaultAction(VARIANT varChild);

        virtual STDMETHODIMP            put_accName(VARIANT varChild, BSTR szName);
        virtual STDMETHODIMP            put_accValue(VARIANT varChild, BSTR pszValue);

        // IEnumVARIANT
        virtual STDMETHODIMP            Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP            Skip(ULONG celt);
        virtual STDMETHODIMP            Reset(void);
        virtual STDMETHODIMP            Clone(IEnumVARIANT ** ppenum);

        // IOleWindow
        virtual STDMETHODIMP            GetWindow(HWND* phwnd);
        virtual STDMETHODIMP            ContextSensitiveHelp(BOOL fEnterMode);
};



#endif  // _CLSUTIL_H_
