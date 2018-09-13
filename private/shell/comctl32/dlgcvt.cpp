//
// This module provides the following functions:
//
//      CvtDlgToDlgEx - Converts a DLGTEMPLATE to a DLGTEMPLATEEX
//
//
#include "ctlspriv.h"


#include "dlgcvt.h"

//
// Define the amount (bytes) the stream buffer grows when required.
// It will grow enough to satisfy the required write PLUS this
// amount.
//
#ifdef DEBUG
#   define STREAM_GROW_BYTES 32     // Exercise stream growth.
#else
#   define STREAM_GROW_BYTES 512
#endif

//
// Simple MIN/MAX inline helpers.
//

#if (defined UNIX && defined ux10)
//IEUNIX: hp's version of "/usr/local/include/sys/param.h defines MAX and MIN
//macro's and breaks hp's build
#undef MAX(a,b)
#undef MIN(a,b)
#endif //UNIX && ux10

template <class T>
inline const T& MIN(const T& a, const T& b)
{
    return a < b ? a : b;
}

template <class T>
inline const T& MAX(const T& a, const T& b)
{
    return a > b ? a : b;
}

//
// This class implements a simple dynamic stream that grows as you
// add data to it.  It's modeled after the strstream class provided
// by the C++ std lib.  Unlike the std lib implementation, this one
// doesn't require C++ EH to be enabled.  If comctl32 compiled with
// C++ EH enabled, I would have used strstream instead.
// [brianau - 10/5/98]
// 
class CByteStream
{
    public:
        explicit CByteStream(int cbDefGrow = 512);
        ~CByteStream(void);

        //
        // Used as argument to AlignXXXX member functions.
        //
        enum AlignType { eAlignWrite, eAlignRead };
        //
        // Basic read/write functions.
        //
        int Read(LPVOID pb, int cb);
        int Write(const VOID *pb, int cb);
        //
        // Determine if there was an error when reading or 
        // writing to the stream.
        //
        bool ReadError(void) const
            { return m_bReadErr; }

        bool WriteError(void) const
            { return m_bWriteErr; }
        //
        // Reset the stream read or write pointer.
        //
        void ResetRead(void)
            { m_pbRead = m_pbBuf; m_bReadErr = false; }

        void ResetWrite(void)
            { m_pbWrite = m_pbBuf; m_bWriteErr = false; }
        //
        // Reset the stream.
        //
        void Reset(void);
        //
        // These functions align the read and write stream pointers.
        //
        void AlignReadWord(void)
            { Align(eAlignRead, sizeof(WORD)); }

        void AlignReadDword(void)
            { Align(eAlignRead, sizeof(DWORD)); }

        void AlignReadQword(void)
            { Align(eAlignRead, sizeof(ULONGLONG)); }

        void AlignWriteWord(void)
            { Align(eAlignWrite, sizeof(WORD)); }

        void AlignWriteDword(void)
            { Align(eAlignWrite, sizeof(DWORD)); }

        void AlignWriteQword(void)
            { Align(eAlignWrite, sizeof(ULONGLONG)); }

        //
        // GetBuffer returns the address of the stream buffer in memory.
        // The buffer is "frozen" so it will not be released if the stream
        // object is destroyed.  At this point, you own the buffer.
        // If bPermanent is false, you can call ReleaseBuffer to return 
        // control of the buffer to the stream object.
        //
        LPBYTE GetBuffer(bool bPermanent = false);
        //
        // ReleaseBuffer returns control of the buffer obtained with GetBuffer
        // to the stream object.
        //
        bool ReleaseBuffer(LPBYTE pbBuf);
        //
        // Overload the insertion and extraction operators so we can
        // work like a normal std lib stream class.
        //
        template <class T>
        CByteStream& operator >> (T& x)
            { Read(&x, sizeof(x)); return *this; }

        template <class T>
        CByteStream& operator << (const T& x)
            { Write(&x, sizeof(x)); return *this; }

    private:
        int    m_cbDefGrow;  // Default amount (bytes) to grow when expanding buffer.
        LPBYTE m_pbBuf;      // Addr of allocated buffer.
        LPBYTE m_pbRead;     // Addr for next read.
        LPBYTE m_pbWrite;    // Addr for next write.
        LPBYTE m_pbEnd;      // Addr of byte following last byte in buffer.
        bool   m_bWriteErr;  // Any read errors?
        bool   m_bReadErr;   // Any write errors?
        bool   m_bOwnsBuf;   // true == delete buffer in dtor.

        //
        // Expand the buffer as needed.
        //
        bool GrowBuffer(int cb = 0);
        //
        // Align the read or write buffer pointer.
        // Used internally by the AlignXXXXX member functions.
        //
        void Align(AlignType a, size_t n);
        //
        // Internal consistency checks for debug builds.
        //
        void Validate(void) const;
        //
        // Prevent copy.
        //
        CByteStream(const CByteStream& rhs);
        CByteStream& operator = (const CByteStream& rhs);
};


//
// Class for converting in-memory dialog templates between the two
// structures DLGTEMPLATE <-> DLGTEMPLATEEX.
//
// Currently, the object only converts from DLGTEMPLATE -> DLGTEMPLATEEX.
// It would be simple to create the code for the inverse conversion.  However,
// it's currently not needed so I didn't create it.
//
class CDlgTemplateConverter
{
    public:
        explicit CDlgTemplateConverter(int iCharSet = DEFAULT_CHARSET)
            : m_iCharset(iCharSet),
              m_stm(STREAM_GROW_BYTES) { }

        ~CDlgTemplateConverter(void) { }

        HRESULT DlgToDlgEx(LPDLGTEMPLATE pTemplateIn, LPDLGTEMPLATEEX *ppTemplateOut);

        HRESULT DlgExToDlg(LPDLGTEMPLATEEX pTemplateIn, LPDLGTEMPLATE *ppTemplateOut)
            { return E_NOTIMPL; }

    private:
        int         m_iCharset;
        CByteStream m_stm;       // For converted template.

#ifndef UNIX
        HRESULT DlgHdrToDlgEx(CByteStream& s, LPWORD *ppw);
        HRESULT DlgItemToDlgEx(CByteStream& s, LPWORD *ppw);
#else
        HRESULT DlgHdrToDlgEx(CByteStream& s, LPDWORD *ppw);
        HRESULT DlgItemToDlgEx(CByteStream& s, LPDWORD *ppw);
#endif

        HRESULT DlgExHdrToDlg(CByteStream& s, LPWORD *ppw)
            { return E_NOTIMPL; }
        HRESULT DlgExItemToDlg(CByteStream& s, LPWORD *ppw)
            { return E_NOTIMPL; }
        //
        // Copy a string from pszW into a CByteStream object.
        // Copies at most cch chars.  If cch is -1, assumes the string is 
        // nul-terminated and will copy all chars in string including
        // terminating NULL.
        //
        int CopyStringW(CByteStream& stm, LPWSTR pszW, int cch = -1);
        //
        // Prevent copy.
        //
        CDlgTemplateConverter(const CDlgTemplateConverter& rhs);
        CDlgTemplateConverter& operator = (const CDlgTemplateConverter& rhs);
};


//
// Generic alignment function.
// Give it an address and an alignment size and it returns
// the address adjusted for the requested alignment.
//
// n :  2 = 16-bit
//      4 = 32-bit
//      8 = 64-bit
//
LPVOID Align(LPVOID pv, size_t n)
{
    const ULONG_PTR x = static_cast<ULONG_PTR>(n) - 1;
    return reinterpret_cast<LPVOID>((reinterpret_cast<ULONG_PTR>(pv) + x) & ~x);
}

inline LPVOID AlignWord(LPVOID pv)
{
    return ::Align(pv, sizeof(WORD));
}

inline LPVOID AlignDWord(LPVOID pv)
{
    return ::Align(pv, sizeof(DWORD));
}

inline LPVOID AlignQWord(LPVOID pv)
{
    return ::Align(pv, sizeof(ULONGLONG));
}



CByteStream::CByteStream(
    int cbDefGrow
    ) : m_cbDefGrow(MAX(cbDefGrow, 1)),
        m_pbBuf(NULL),
        m_pbRead(NULL),
        m_pbWrite(NULL),
        m_pbEnd(NULL),
        m_bWriteErr(false),
        m_bReadErr(false),
        m_bOwnsBuf(true) 
{ 

}


CByteStream::~CByteStream(
    void
    )
{
    if (m_bOwnsBuf && NULL != m_pbBuf)
    {
        LocalFree(m_pbBuf);
    }
}

//
// Simple checks to validate stream state.
// In non-debug builds, this will be a no-op.
// Use ASSERT_VALIDSTREAM macro.
//
void
CByteStream::Validate(
    void
    ) const
{
    ASSERT(m_pbEnd >= m_pbBuf);
    ASSERT(m_pbWrite >= m_pbBuf);
    ASSERT(m_pbRead >= m_pbBuf);
    ASSERT(m_pbWrite <= m_pbEnd);
    ASSERT(m_pbRead <= m_pbEnd);
}

#ifdef DEBUG
#   define ASSERT_VALIDSTREAM(ps)  ps->Validate()
#else
#   define ASSERT_VALIDSTREAM(ps)
#endif

//
// Read "cb" bytes from the stream and write them to 
// the location specified in "pb".  Return number
// of bytes read.  Note that if we don't "own" the
// buffer (i.e. the client has called GetBuffer but
// not ReleaseBuffer), no read will occur.
//
int 
CByteStream::Read(
    LPVOID pb,
    int cb
    )
{
    ASSERT_VALIDSTREAM(this);

    int cbRead = 0;
    if (m_bOwnsBuf)
    {
        cbRead = MIN(static_cast<int>(m_pbEnd - m_pbRead), cb);
        CopyMemory(pb, m_pbRead, cbRead);
        m_pbRead += cbRead;
        if (cb != cbRead)
            m_bReadErr = true;
    }

    ASSERT_VALIDSTREAM(this);

    return cbRead;
}


//
// Write "cb" bytes from location "pb" into the stream.
// Return number of bytes written.  Note that if we don't "own" the
// buffer (i.e. the client has called GetBuffer but
// not ReleaseBuffer), no write will occur.
//
int 
CByteStream::Write(
    const VOID *pb,
    int cb
    )
{
    ASSERT_VALIDSTREAM(this);

    int cbWritten = 0;
    if (m_bOwnsBuf)
    {
        if (m_pbWrite + cb < m_pbEnd || 
            GrowBuffer(static_cast<int>(m_pbEnd - m_pbBuf) + cb + m_cbDefGrow))
        {
            CopyMemory(m_pbWrite, pb, cb);
            m_pbWrite += cb;
            cbWritten = cb;
        }
        else
            m_bWriteErr = true;
    }

    ASSERT_VALIDSTREAM(this);

    return cbWritten;
}

//
// Reallocate the buffer by cb or m_cbDefGrow.
// Copy existing contents to new buffer.  All internal
// pointers are updated.
//
bool 
CByteStream::GrowBuffer(
    int cb               // optional.  Default is 0 causing us to use m_cbDefGrow.
    )
{
    bool bResult         = false;
    int cbGrow           = 0 < cb ? cb : m_cbDefGrow;
    ULONG_PTR ulReadOfs  = m_pbRead - m_pbBuf;
    ULONG_PTR ulWriteOfs = m_pbWrite - m_pbBuf;
    ULONG_PTR cbAlloc    = m_pbEnd - m_pbBuf;
    LPBYTE pNew = static_cast<LPBYTE>(LocalAlloc(LPTR, cbAlloc + cbGrow));
    if (NULL != pNew)
    {
        if (NULL != m_pbBuf)
        {
            CopyMemory(pNew, m_pbBuf, cbAlloc);
            LocalFree(m_pbBuf);
        }
        m_pbBuf   = pNew;
        m_pbRead  = m_pbBuf + ulReadOfs;
        m_pbWrite = m_pbBuf + ulWriteOfs;
        m_pbEnd   = m_pbBuf + cbAlloc + cbGrow;
        bResult   = true;
    }

    ASSERT_VALIDSTREAM(this);
    return bResult;
}

//
// Align the read or write pointer on the stream.
// The write pointer is aligned by padding skipped bytes with 0.
//
void
CByteStream::Align(
    CByteStream::AlignType a,
    size_t n
    )
{
    static const BYTE fill[8] = {0};
    if (m_bOwnsBuf)
    {
        switch(a)
        {
            case eAlignWrite:
                Write(fill, static_cast<int>(reinterpret_cast<LPBYTE>(::Align(m_pbWrite, n)) - m_pbWrite));
                break;

            case eAlignRead:
                m_pbRead = reinterpret_cast<LPBYTE>(::Align(m_pbRead, n));
                if (m_pbRead >= m_pbEnd)
                    m_bReadErr = true;
                break;

            default:
                break;
        }
    }
    ASSERT_VALIDSTREAM(this);
}


//
// Caller takes ownership of the buffer.
//
LPBYTE 
CByteStream::GetBuffer(
    bool bPermanent       // optional.  Default is false.
    )
{ 
    LPBYTE pbRet = m_pbBuf;
    if (bPermanent)
    {
        //
        // Caller now permanently owns the buffer.
        // Can't return it through ReleaseBuffer().
        // Reset the internal stream control values.
        //
        m_pbBuf = m_pbWrite = m_pbRead = m_pbEnd = NULL;
        m_bWriteErr = m_bReadErr = false;
        m_bOwnsBuf = true;
    }
    else
    {
        //
        // Caller now owns the buffer but it can be returned
        // through ReleaseBuffer().
        //
        m_bOwnsBuf = false; 
    }
    return pbRet; 
}


//
// Take back ownership of the buffer.
// Returns:  
//
//      true   = CByteStream object took back ownership.
//      false  = CByteStream object couldn't take ownership.
//
bool 
CByteStream::ReleaseBuffer(
    LPBYTE pbBuf
    )
{
    if (pbBuf == m_pbBuf)
    {
        m_bOwnsBuf = true;
        return true;
    }
    return false;
}
     

//
// Reset the stream.
//
void 
CByteStream::Reset(
    void
    )
{
    if (NULL != m_pbBuf)
    {
        LocalFree(m_pbBuf);
    }
    m_pbBuf = m_pbWrite = m_pbRead = m_pbEnd = NULL;
    m_bWriteErr = m_bReadErr = false;
    m_bOwnsBuf = true;
}


//
// Copy one or more WORDs from the location provided in "pszW" into
// the stream.  If cch is -1, it's assumed that the string is nul-terminated.
// Returns the number of WCHARs written.
//    
int 
CDlgTemplateConverter::CopyStringW(
    CByteStream& stm,
    LPWSTR pszW,
    int cch
    )
{
    if (-1 == cch)
        cch = lstrlenW(pszW) + 1;
    return stm.Write(pszW, cch * sizeof(WCHAR)) / sizeof(WCHAR);
}

//
// Convert a DLGTEMPLATE structure to a DLGTEMPLATEEX structure.
// pti is the address of the DLGTEMPLATE to be converted.
// ppto points to a LPDLGTEMPLATEEX ptr to receive the address of the
// converted template structure.  Caller is responsible for freeing
// this buffer with LocalFree.
//
// Returns:  E_OUTOFMEMORY, NOERROR
//
HRESULT
CDlgTemplateConverter::DlgToDlgEx(
    LPDLGTEMPLATE pti,
    LPDLGTEMPLATEEX *ppto
    )
{
    HRESULT hr = NOERROR;
#ifndef UNIX
    LPWORD pw = reinterpret_cast<LPWORD>(pti);
#else
    LPDWORD pw = reinterpret_cast<LPDWORD>(pti);
#endif

    *ppto = NULL;

    //
    // Reset the stream.
    //
    m_stm.Reset();
    //
    // Convert DLGTEMPLATE -> DLGTEMPLATEEX
    //
    hr = DlgHdrToDlgEx(m_stm, &pw);
    //
    // Convert each DLGITEMTEMPLATE -> DLGITEMTEMPLATEEX
    //
    for (int i = 0; i < pti->cdit && SUCCEEDED(hr); i++)
    {
#ifndef UNIX
        pw = reinterpret_cast<LPWORD>(::AlignDWord(pw));
#else
        pw = reinterpret_cast<LPDWORD>(::AlignDWord(pw));
#endif
        m_stm.AlignWriteDword();
        hr = DlgItemToDlgEx(m_stm, &pw);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Return the buffer to the caller.  Buffer is permanently
        // detached from the stream object so the stream's dtor
        // won't free it.
        //
        *ppto = reinterpret_cast<LPDLGTEMPLATEEX>(m_stm.GetBuffer(true));    
    }
    return hr;
};


//
// Convert DLGTEMPLATE -> DLGTEMPLATEEX
//
// s   = Stream to hold converted template.
// ppw = Address of current read pointer into the template being converted.
//       On exit, the referenced pointer is updated with the current read location.
//
// Returns:  E_OUTOFMEMORY, NOERROR
//
HRESULT
CDlgTemplateConverter::DlgHdrToDlgEx(
    CByteStream& s,
#ifndef UNIX
    LPWORD *ppw
#else
    LPDWORD *ppw
#endif
    )
{
#ifndef UNIX
    LPWORD pw = *ppw;
#else
    LPDWORD pw = *ppw;
#endif
    LPDLGTEMPLATE pt = reinterpret_cast<LPDLGTEMPLATE>(pw);

    //
    // Convert the fixed-length stuff.
    //
    s << static_cast<WORD>(1)                        // wDlgVer
      << static_cast<WORD>(0xFFFF)                   // wSignature
      << static_cast<DWORD>(0)                       // dwHelpID
      << static_cast<DWORD>(pt->dwExtendedStyle)
      << static_cast<DWORD>(pt->style)
      << static_cast<WORD>(pt->cdit)
      << static_cast<short>(pt->x)
      << static_cast<short>(pt->y)
      << static_cast<short>(pt->cx)
      << static_cast<short>(pt->cy);

    //
    // Arrays are always WORD aligned.
    //
#ifndef UNIX
    pw = reinterpret_cast<LPWORD>(::AlignWord(reinterpret_cast<LPBYTE>(pw) + sizeof(DLGTEMPLATE)));
    s.AlignWriteWord();
#else
    pw = reinterpret_cast<LPDWORD>(::AlignDWord(reinterpret_cast<LPBYTE>(pw) + sizeof(DLGTEMPLATE)));
    s.AlignWriteDword();
#endif

    //
    // Copy the menu array.
    //
    switch(*pw)
    {
        case 0xFFFF:
            s << *pw++;
            //
            // Fall through...
            //
        case 0x0000:
            s << *pw++;
            break;
                        
        default:
#ifndef UNIX
            pw += CopyStringW(s, pw);
#else
            pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
            break;
    };
    //
    // Copy the class array.
    //
    switch(*pw)
    {
        case 0xFFFF:
            s << *pw++;
            //
            // Fall through...
            //
        case 0x0000:
            s << *pw++;
            break;
            
        default:
#ifndef UNIX
            pw += CopyStringW(s, pw);
#else
            pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
            break;
    };
    //
    // Copy the title array.
    //
    switch(*pw)
    {
        case 0x0000:
            s << *pw++;
            break;

        default:
#ifndef UNIX
            pw += CopyStringW(s, pw);
#else
            pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
            break;
    };
    //
    // Copy font information if it's present.
    //
    if (DS_SETFONT & pt->style)
    {
        s << *pw++;                              // pt size
        s << static_cast<WORD>(FW_NORMAL);       // weight (default, not in DLGTEMPLATE)
        s << static_cast<BYTE>(FALSE);           // italic (default, not in DLGTEMPLATE)
        s << static_cast<BYTE>(m_iCharset);        // charset (default if not given, 
                                                 //          not in DLGTEMPLATE)
#ifndef UNIX
        pw += CopyStringW(s, pw);
#else
        pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
    }

    *ppw = pw;

    return s.WriteError() ? E_OUTOFMEMORY : NOERROR;
}


//
// Convert DLGITEMTEMPLATE -> DLGITEMTEMPLATEEX
//
// s   = Stream to hold converted template.
// ppw = Address of current read pointer into the template being converted.
//       On exit, the referenced pointer is updated with the current read location.
//
// Returns:  E_OUTOFMEMORY, NOERROR
//
HRESULT
CDlgTemplateConverter::DlgItemToDlgEx(
    CByteStream& s,
#ifndef UNIX
    LPWORD *ppw
#else
    LPDWORD *ppw
#endif
    )
{
#ifndef UNIX
    LPWORD pw = *ppw;
#else
    LPDWORD pw = *ppw;
#endif
    LPDLGITEMTEMPLATE pit = reinterpret_cast<LPDLGITEMTEMPLATE>(pw);

    //
    // Convert the fixed-length stuff.
    //
    s << static_cast<DWORD>(0)                     // dwHelpID
      << static_cast<DWORD>(pit->dwExtendedStyle)
      << static_cast<DWORD>(pit->style)
      << static_cast<short>(pit->x)
      << static_cast<short>(pit->y)
      << static_cast<short>(pit->cx)
      << static_cast<short>(pit->cy)
      << static_cast<DWORD>(pit->id);

    //
    // Arrays are always word aligned.
    //
#ifndef UNIX
    pw = reinterpret_cast<LPWORD>(::AlignWord(reinterpret_cast<LPBYTE>(pw) + sizeof(DLGITEMTEMPLATE)));
    s.AlignWriteWord();
#else
    pw = reinterpret_cast<LPDWORD>(::AlignWord(reinterpret_cast<LPBYTE>(pw) + sizeof(DLGITEMTEMPLATE)));
    s.AlignWriteDword();
#endif
    //
    // Copy the class array.
    //
    switch(*pw)
    {
        case 0xFFFF:
            s << *pw++;
            s << *pw++;   // Class code.
            break;
            
        default:
#ifndef UNIX
            pw += CopyStringW(s, pw);
#else
            pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
            break;
    };
    //
    // Copy the title array.
    //
    switch(*pw)
    {
        case 0xFFFF:
            s << *pw++;
            s << *pw++;   // Resource ordinal value.
            break;
            
        default:
#ifndef UNIX
            pw += CopyStringW(s, pw);
#else
            pw += CopyStringW (s, reinterpret_cast<LPWSTR>(pw));
#endif
            break;
    };
    //
    // Copy the creation data.
    // *pw is either 0 or the number of bytes of creation data,
    // including *pw.
    //
    switch(*pw)
    {
        case 0x0000:
            s << *pw++;
            break;

        default:
#ifndef UNIX
            pw += s.Write(pw, *pw) / sizeof(WORD);
#else
            pw += s.Write(pw, *pw) / sizeof(DWORD);
#endif
            break;
    };

    *ppw = pw;

    return s.WriteError() ? E_OUTOFMEMORY : NOERROR;
}


//
// This is the public function for converting a DLGTEMPLATE to
// a DLGTEMPLATEEX.
//
// Returns:  E_OUTOFMEMORY, NOERROR
//
HRESULT 
CvtDlgToDlgEx(
    LPDLGTEMPLATE pTemplate, 
    LPDLGTEMPLATEEX *ppTemplateExOut,
    int iCharset
    )
{
    CDlgTemplateConverter dtc(iCharset);
    return dtc.DlgToDlgEx(pTemplate, ppTemplateExOut);
}

