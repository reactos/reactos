#ifndef _INC_DSKQUOTA_EXCEPT_H
#define _INC_DSKQUOTA_EXCEPT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: except.h

    Description: Basic exception class hierarchy.

         I shamelessly based this on the MFC exception hierarchy.

         CException
              CMemoryException        - i.e. "Out of memory", "invalid index"
              CFileException          - i.e. "device IO error"
              CSyncException          - i.e. "mutext abandoned"
              CResourceException      - i.e. "resource not found in image"
              COleException           - i.e. "some critical OLE error"
              CNotSupportedException  - i.e. "function not supported"


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif


//
// Very naive string implementation.  Just need something simple to hold
// strings in exception objects when necessary.
// Ensures proper cleanup on dtor.
// Can't use CString here (I wanted to) because that creates a circular
// reference condition between strclass.h and except.h
//
class CExceptionString 
{
    public:
        explicit CExceptionString(LPCTSTR pszText = TEXT(""));
        ~CExceptionString(void)
            { delete[] m_pszText; }

        CExceptionString(const CExceptionString& rhs);
        CExceptionString& operator = (const CExceptionString& rhs);

        const TCHAR * const GetTextPtr(void) const
            { return m_pszText; }

    private:
        LPTSTR m_pszText;

        LPTSTR Dup(LPCTSTR psz);

};

//
// Base class for all exceptions.
//
class CException
{
    public:
        enum reason { none };
        explicit CException(DWORD r) : m_reason(r) { }

        DWORD Reason(void) const { return m_reason; }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CException"); }

        virtual LPCTSTR ReasonText(void) const
            { return TEXT("Unknown"); }
#endif // DBG

    private:
        DWORD m_reason;
};

//
// Exception class representing different "bad" things associated
// with memory use.
//
class CMemoryException : public CException
{
    public:
        enum reason { alloc,        // Memory allocation failure.
                      overflow,     // Memory overflow.
                      index,        // Bad index value.
                      range,        // Value overrange for data type.
                      pointer,      // Bad pointer (i.e. NULL).
                      num_reasons
                    };
        explicit CMemoryException(reason r) : CException((DWORD)r) { }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CMemoryException"); }

        virtual LPCTSTR ReasonText(void) const
            { return m_pszReasons[Reason()]; }
    private:
        static LPCTSTR m_pszReasons[num_reasons];

#endif // DBG
};

class CAllocException : private CMemoryException
{
    public:
        CAllocException(void) : CMemoryException(CMemoryException::alloc) { }
        
#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CAllocException"); }

        virtual LPCTSTR ReasonText(void) const
            { return CMemoryException::ReasonText(); }

#endif // DBG
};

//
// Exception class representing file I/O errors.
//
class CFileException : public CException
{
    public:
        enum reason { create,       // Can't create file.
                      read,         // Can't read file.
                      write,        // Can't write file.
                      diskfull,     // Disk is full.
                      access,       // No access.
                      device,       // Device write error.
                      num_reasons
                    };

        CFileException(reason r, LPCTSTR pszFile, DWORD dwIoError) 
            : CException((DWORD)r),
              m_strFile(pszFile),
              m_dwIoError(dwIoError) { }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CFileException"); }

        virtual LPCTSTR ReasonText(void) const
            { return m_pszReasons[Reason()]; }
#endif // DBG

        const TCHAR * const FileName(void) const { return m_strFile.GetTextPtr(); }
        DWORD IoError(void) const { return m_dwIoError; }

    private:
        DWORD            m_dwIoError;
        CExceptionString m_strFile;
#if DBG
        static LPCTSTR m_pszReasons[num_reasons];
#endif // DBG
};

//
// Thread synchronization object exception.
//
class CSyncException : public CException
{
    public:
        enum object { mutex, critsect, semaphore, event, thread, process, num_objects };
        enum reason { create, timeout, abandoned, num_reasons };
        CSyncException(object obj, reason r)
            : CException(r),
              m_object(obj) { }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CSyncException"); }

        virtual LPCTSTR ReasonText(void) const
            { return m_pszReasons[Reason()]; }

        virtual LPCTSTR ObjectText(void) const
            { return m_pszObjects[Object()]; }
#endif // DBG

        object Object(void) const { return m_object; }

    private:
        object m_object; 

#if DBG
        static LPCTSTR m_pszReasons[num_reasons];
        static LPCTSTR m_pszObjects[num_objects];
#endif // DBG
};


//
// Windows resource exception.
//
class CResourceException : public CException
{
    public:
        enum type { accelerator,
                    anicursor,
                    aniicon,
                    bitmap,
                    cursor,
                    dialog,
                    font,
                    fontdir,
                    group_cursor,
                    group_icon,
                    icon,
                    menu,
                    messagetable,
                    rcdata,
                    string,
                    version,
                    num_reasons };

        CResourceException(type t, HINSTANCE hInstance, UINT uResId) 
            : CException(CException::none),
              m_type(t),
              m_uResId(uResId),
              m_hInstance(hInstance) { }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CResourceException"); }

        virtual LPCTSTR ReasonText(void) const;

#endif // DBG

        HINSTANCE Module(void) const { return m_hInstance; }
        enum type Type(void) const { return m_type; }

    private:
        enum type  m_type;
        UINT       m_uResId;
        HINSTANCE  m_hInstance;

#if DBG
        static LPCTSTR m_pszReasons[num_reasons];
#endif // DBG
};


class COleException : public CException
{
    public:
        explicit COleException(HRESULT hr) 
            : CException(CException::none),
              m_hr(hr) { }

        HRESULT Result(void) const { return m_hr; }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("COleException"); }
        virtual LPCTSTR ReasonText(void) const
            { return TEXT("not applicable"); }
#endif // DBG

    private:
        HRESULT m_hr;
};


//
// Some requested operation is not supported.
//
class CNotSupportedException : public CException
{
    public:
        CNotSupportedException(void) : CException(CException::none) { }

#if DBG
        virtual LPCTSTR NameText(void) const
            { return TEXT("CNotSupportedException"); }
        virtual LPCTSTR ReasonText(void) const
            { return TEXT("not applicable"); }
#endif // DBG
};



#endif // _INC_DSKQUOTA_EXCEPT_H
