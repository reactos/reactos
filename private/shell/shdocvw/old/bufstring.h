//=--------------------------------------------------------------------------=
// 
// A small class to set and get strings. Keeps small strings in a local array,
// large strings are allocated. TODO: make the size of the local array
// a property of the class instead of a #define...
//
#define BUFFEREDSTRING_ARRAY 20 // "*.exe" is pretty small
class CBufferedString {
public:
    CBufferedString() { m_pBuf = m_szTmpBuf;
    }
    ~CBufferedString() { if (m_fFree) LocalFree(m_pBuf); }

    HRESULT SetBSTR(BSTR bstr);

    LPTSTR GetTSTR() { return m_fSet ? m_pBuf : NULL; }

private:
    LPTSTR m_pBuf;
    TCHAR  m_szTmpBuf[BUFFEREDSTRING_ARRAY];
    BOOL   m_fFree:1;
    BOOL   m_fSet:1;
};

