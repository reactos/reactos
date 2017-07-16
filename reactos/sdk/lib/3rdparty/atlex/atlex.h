/*
    Copyright 1991-2017 Amebis

    This file is part of atlex.

    atlex is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    atlex is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with atlex. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atlconv.h>
#include <atldef.h>
#include <atlstr.h>

#ifndef ATL_STACK_BUFFER_BYTES
///
/// Size of the stack buffer in bytes used for initial system function call
///
/// Some system functions with variable length output data fail for
/// insufficient buffer sizes, and return an exact buffer length required.
/// The function helpers use a fixed size stack buffer first. If the stack
/// buffer really prooved sufficient, the helper allocates the exact length
/// output on heap and copies the data without calling the system function
/// again. Otherwise it allocates the exact length output on heap and retries.
///
/// \note
/// Decrease this value in case of stack overflow.
///
#define ATL_STACK_BUFFER_BYTES  1024
#endif


namespace ATL
{
    ///
    /// \defgroup ATLSysHandles System Handles
    /// Simplifies work with object handles of various type
    ///
    /// @{

    ///
    /// Base abstract template class to support generic object handle keeping
    ///
    /// It provides basic operators and methods common to all descendands of this class establishing a base to ease the replacement of native object handle type with classes in object-oriented approach.
    ///
    template <class T> class CObjectWithHandleT
    {
    public:
        ///
        /// Datatype of the object handle this template class handles
        ///
        typedef T HANDLE;

        ///
        /// Initializes a new class instance with the object handle set to NULL.
        ///
        inline CObjectWithHandleT() : m_h(NULL)
        {
        }

        ///
        /// Initializes a new class instance with an available object handle.
        ///
        /// \param[in] h Initial object handle value
        ///
        inline CObjectWithHandleT(_In_opt_ HANDLE h) : m_h(h)
        {
        }

        /// \name Operators for transparent usage of this class
        /// @{

        ///
        /// Auto-typecasting operator
        ///
        /// \return Object handle
        ///
        inline operator HANDLE() const
        {
            return m_h;
        }

        ///
        /// Returns the object handle value when the object handle is a pointer to a value (class, struct, etc.).
        ///
        /// \return Object handle value
        ///
        inline HANDLE*& operator*() const
        {
            ATLENSURE(m_h != NULL);
            return *m_h;
        }

        ///
        /// Returns the object handle reference.
        /// \return Object handle reference
        ///
        inline HANDLE* operator&()
        {
            ATLASSERT(m_h == NULL);
            return &m_h;
        }

        ///
        /// Provides object handle member access when the object handle is a pointer to a class or struct.
        ///
        /// \return Object handle
        ///
        inline HANDLE operator->() const
        {
            ATLASSERT(m_h != NULL);
            return m_h;
        }

        /// @}

        /// \name Comparison operators
        /// @{

        ///
        /// Tests if the object handle is NULL.
        ///
        /// \return
        /// - Non zero when object handle is NULL;
        /// - Zero otherwise.
        ///
        inline bool operator!() const
        {
            return m_h == NULL;
        }

        ///
        /// Is handle less than?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is less than h;
        /// - Zero otherwise.
        ///
        inline bool operator<(_In_opt_ HANDLE h) const
        {
            return m_h < h;
        }

        ///
        /// Is handle less than or equal to?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is less than or equal to h;
        /// - Zero otherwise.
        ///
        inline bool operator<=(_In_opt_ HANDLE h) const
        {
            return m_h <= h;
        }

        ///
        /// Is handle greater than or equal to?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is greater than or equal to h;
        /// - Zero otherwise.
        ///
        inline bool operator>=(_In_opt_ HANDLE h) const
        {
            return m_h >= h;
        }

        ///
        /// Is handle greater than?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is greater than h;
        /// - Zero otherwise.
        ///
        inline bool operator>(_In_opt_ HANDLE h) const
        {
            return m_h > h;
        }

        ///
        /// Is handle not equal to?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is not equal to h;
        /// - Zero otherwise.
        ///
        inline bool operator!=(_In_opt_ HANDLE h) const
        {
            return !operator==(h);
        }

        ///
        /// Is handle equal to?
        ///
        /// \param[in] h Object handle to compare against
        /// \return
        /// - Non zero when object handle is equal to h;
        /// - Zero otherwise.
        ///
        inline bool operator==(_In_opt_ HANDLE h) const
        {
            return m_h == h;
        }

        /// @}

        ///
        /// Sets a new object handle for the class
        ///
        /// When the current object handle of the class is non-NULL, the object is destroyed first.
        ///
        /// \param[in] h New object handle
        ///
        inline void Attach(_In_opt_ HANDLE h)
        {
            if (m_h)
                InternalFree();
            m_h = h;
        }

        ///
        /// Dismisses the object handle from this class
        ///
        /// \return Object handle
        ///
        inline HANDLE Detach()
        {
            HANDLE h = m_h;
            m_h = NULL;
            return h;
        }

        ///
        /// Destroys the object
        ///
        inline void Free()
        {
            if (m_h) {
                InternalFree();
                m_h = NULL;
            }
        }

        /// @}

    protected:
        ///
        /// Abstract member function that must be implemented by child classes to do the actual object destruction.
        ///
        virtual void InternalFree() = 0;

    protected:
        HANDLE m_h; ///< Object handle
    };


    ///
    /// Base abstract template class to support object handle keeping for objects that support handle duplication
    ///
    template <class T>
    class CObjectWithHandleDuplT : public CObjectWithHandleT<T>
    {
    public:
        ///
        /// Duplicates and returns a new object handle.
        ///
        /// \return Duplicated object handle
        ///
        inline HANDLE GetDuplicate() const
        {
            return m_h ? InternalDuplicate(m_h) : NULL;
        }

        ///
        /// Duplicates an object handle and sets a new object handle.
        ///
        /// \param[in] h Object handle of existing object
        /// \return
        /// - TRUE when duplication succeeds;
        /// - FALSE when duplication fails. In case of failure obtaining the extended error information is object type specific (for example: `GetLastError()`).
        ///
        inline BOOL DuplicateAndAttach(_In_opt_ HANDLE h)
        {
            if (m_h)
                InternalFree();

            return h ? (m_h = InternalDuplicate(h)) != NULL : (m_h = NULL, TRUE);
        }

        //
        // Do not allow = operators. They are semantically ambigious:
        // Do they attach the class to the existing instance of object, or do they duplicate it?
        // To avoid confusion, user should use Attach() and Duplicate() methods explicitly.
        //
        //inline const CObjectWithHandleDuplT<T>& operator=(_In_ const HANDLE src)
        //{
        //    Attach(src ? InternalDuplicate(src) : NULL);
        //    return *this;
        //}

        //inline const CObjectWithHandleDuplT<T>& operator=(_In_ const CObjectWithHandleDuplT<T> &src)
        //{
        //    Attach(src.m_h ? InternalDuplicate(src.m_h) : NULL);
        //    return *this;
        //}

    protected:
        ///
        /// Abstract member function that must be implemented by child classes to do the actual object handle duplication.
        ///
        /// \param[in] h Object handle of existing object
        /// \return Duplicated object handle
        ///
        virtual HANDLE InternalDuplicate(_In_ HANDLE h) const = 0;
    };

    /// @}

    ///
    /// \defgroup ATLStrFormat String Formatting
    /// Formatted string generation
    ///
    /// \par Example
    /// \code
    /// // Please note the PCSTR typecasting invokes an operator to return
    /// // pointer to formatted buffer rather than class reference itself.
    /// cout << (PCSTR)(CStrFormatA("%i is less than %i.\n", 1, 5));
    /// \endcode
    ///
    /// @{

    ///
    /// Base template class to support string formatting using `printf()` style templates
    ///
    template<typename BaseType, class StringTraits>
    class CStrFormatT : public CStringT<BaseType, StringTraits>
    {
    public:
        /// \name Initializing string using template in memory
        /// @{

        ///
        /// Initializes a new string and formats its contents using `printf()` style template.
        ///
        /// \param[in] pszFormat String template using `printf()` style
        ///
        CStrFormatT(_In_z_ _Printf_format_string_ PCXSTR pszFormat, ...)
        {
            ATLASSERT(AtlIsValidString(pszFormat));

            va_list argList;
            va_start(argList, pszFormat);
            FormatV(pszFormat, argList);
            va_end(argList);
        }

        /// @}

        /// \name Initializing string using template in resources
        /// @{

        ///
        /// Initializes a new string and formats its contents using `printf()` style template in resources.
        ///
        /// \param[in] nFormatID Resource ID of the string template using `printf()` style
        ///
        CStrFormatT(_In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(nFormatID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatV(strFormat, argList);
            va_end(argList);
        }

        ///
        /// Initializes a new string and formats its contents using `printf()` style template in resources.
        ///
        /// \param[in] hInstance Resource module handle
        /// \param[in] nFormatID Resource ID of the string template using `printf()` style
        ///
        CStrFormatT(_In_ HINSTANCE hInstance, _In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(hInstance, nFormatID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatV(strFormat, argList);
            va_end(argList);
        }

        ///
        /// Initializes a new string and formats its contents using `printf()` style template in resources.
        ///
        /// \param[in] hInstance Resource module handle
        /// \param[in] wLanguageID Resource language
        /// \param[in] nFormatID Resource ID of the string template using `printf()` style
        ///
        CStrFormatT(_In_ HINSTANCE hInstance, _In_ WORD wLanguageID, _In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(hInstance, nFormatID, wLanguageID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatV(strFormat, argList);
            va_end(argList);
        }

        /// }@
    };

    ///
    /// Wide character implementation of a class to support string formatting using `printf()` style templates
    ///
    typedef CStrFormatT< wchar_t, StrTraitATL< wchar_t, ChTraitsCRT< wchar_t > > > CStrFormatW;

    ///
    /// Single-byte character implementation of a class to support string formatting using `printf()` style templates
    ///
    typedef CStrFormatT< char, StrTraitATL< char, ChTraitsCRT< char > > > CStrFormatA;

    ///
    /// TCHAR implementation of a class to support string formatting using `printf()` style templates
    ///
    typedef CStrFormatT< TCHAR, StrTraitATL< TCHAR, ChTraitsCRT< TCHAR > > > CStrFormat;


    ///
    /// Base template class to support string formatting using `FormatMessage()` style templates
    ///
    template<typename BaseType, class StringTraits>
    class CStrFormatMsgT : public CStringT<BaseType, StringTraits>
    {
    public:
        /// \name Initializing string using template in memory
        /// @{

        ///
        /// Initializes a new string and formats its contents using `FormatMessage()` style template.
        ///
        /// \param[in] pszFormat String template using `FormatMessage()` style
        ///
        CStrFormatMsgT(_In_z_ _FormatMessage_format_string_ PCXSTR pszFormat, ...)
        {
            ATLASSERT(AtlIsValidString(pszFormat));

            va_list argList;
            va_start(argList, pszFormat);
            FormatMessageV(pszFormat, &argList);
            va_end(argList);
        }

        /// @}

        /// \name Initializing string using template in resources
        /// @{

        ///
        /// Initializes a new string and formats its contents using `FormatMessage()` style template in resources.
        ///
        /// \param[in] nFormatID Resource ID of the string template using `FormatMessage()` style
        ///
        CStrFormatMsgT(_In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(nFormatID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatMessageV(strFormat, &argList);
            va_end(argList);
        }

        ///
        /// Initializes a new string and formats its contents using `FormatMessage()` style template in resources.
        ///
        /// \param[in] hInstance Resource module handle
        /// \param[in] nFormatID Resource ID of the string template using `FormatMessage()` style
        ///
        CStrFormatMsgT(_In_ HINSTANCE hInstance, _In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(hInstance, nFormatID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatMessageV(strFormat, &argList);
            va_end(argList);
        }

        ///
        /// Initializes a new string and formats its contents using `FormatMessage()` style template in resources.
        ///
        /// \param[in] hInstance Resource module handle
        /// \param[in] wLanguageID Resource language
        /// \param[in] nFormatID Resource ID of the string template using `FormatMessage()` style
        ///
        CStrFormatMsgT(_In_ HINSTANCE hInstance, _In_ WORD wLanguageID, _In_ UINT nFormatID, ...)
        {
            CStringT strFormat(GetManager());
            ATLENSURE(strFormat.LoadString(hInstance, nFormatID, wLanguageID));

            va_list argList;
            va_start(argList, nFormatID);
            FormatMessageV(strFormat, &argList);
            va_end(argList);
        }

        /// @}
    };

    ///
    /// Wide character implementation of a class to support string formatting using `FormatMessage()` style templates
    ///
    typedef CStrFormatMsgT< wchar_t, StrTraitATL< wchar_t, ChTraitsCRT< wchar_t > > > CStrFormatMsgW;

    ///
    /// Single-byte character implementation of a class to support string formatting using `FormatMessage()` style templates
    ///
    typedef CStrFormatMsgT< char, StrTraitATL< char, ChTraitsCRT< char > > > CStrFormatMsgA;

    ///
    /// TCHAR implementation of a class to support string formatting using `FormatMessage()` style templates
    ///
    typedef CStrFormatMsgT< TCHAR, StrTraitATL< TCHAR, ChTraitsCRT< TCHAR > > > CStrFormatMsg;

    /// @}

    /// \defgroup ATLMemSanitize Auto-sanitize Memory Management
    /// Sanitizes memory before dismissed
    ///
    /// @{

    ///
    /// A heap template that sanitizes each memory block before it is destroyed or reallocated
    ///
    /// This template is typcally used to extend one of the base ATL heap management classes.
    ///
    /// \par Example
    /// \code
    /// CParanoidHeap<CWin32Heap> myHeap;
    /// \endcode
    ///
    /// \note
    /// CParanoidHeap introduces a performance penalty. However, it provides an additional level of security.
    /// Use for security sensitive data memory storage only.
    ///
    /// \sa [Memory Management Classes](https://msdn.microsoft.com/en-us/library/44yh1z4f.aspx)
    ///
    template <class BaseHeap>
    class CParanoidHeap : public BaseHeap {
    public:
        ///
        /// Sanitizes memory before freeing it
        ///
        /// \param[in] p Pointer to heap memory block
        ///
        virtual void Free(_In_opt_ void* p)
        {
            // Sanitize then free.
            SecureZeroMemory(p, GetSize(p));
            BaseHeap::Free(p);
        }

        ///
        /// Safely reallocates the memory block by sanitizing memory at the previous location
        ///
        /// This member function always performs the following steps (regardless of the current and new size):
        /// 1. Allocates a new memory block,
        /// 2. Copies the data,
        /// 3. Sanitizes the old memory block,
        /// 4. Frees the old memory block.
        ///
        /// \param[in] p Pointer to heap memory block
        /// \param[in] nBytes New size in bytes
        /// \return Pointer to the new heap memory block
        ///
        virtual /*_Ret_opt_bytecap_(nBytes)*/ void* Reallocate(_In_opt_ void* p, _In_ size_t nBytes)
        {
            // Create a new sized copy.
            void *pNew = Allocate(nBytes);
            size_t nSizePrev = GetSize(p);
            CopyMemory(pNew, p, nSizePrev);

            // Sanitize the old data then free.
            SecureZeroMemory(p, nSizePrev);
            Free(p);

            return pNew;
        }
    };


    ///
    /// Base template class to support string conversion with memory sanitization after use
    ///
    template<int t_nBufferLength = 128>
    class CW2AParanoidEX : public CW2AEX<t_nBufferLength> {
    public:
        ///
        /// Initializes a new class instance with the string provided.
        ///
        /// \param[in] psz Pointer to wide string
        ///
        CW2AParanoidEX(_In_z_ LPCWSTR psz) throw(...) : CW2AEX<t_nBufferLength>(psz) {}

        ///
        /// Initializes a new class instance with the string provided.
        ///
        /// \param[in] psz Pointer to wide string
        /// \param[in] nCodePage Code page to use when converting to single-byte string
        ///
        CW2AParanoidEX(_In_z_ LPCWSTR psz, _In_ UINT nCodePage) throw(...) : CW2AEX<t_nBufferLength>(psz, nCodePage) {}

        ///
        /// Sanitizes string memory, then destroys it
        ///
        ~CW2AParanoidEX()
        {
            // Sanitize before free.
            if (m_psz != m_szBuffer)
                SecureZeroMemory(m_psz, _msize(m_psz));
            else
                SecureZeroMemory(m_szBuffer, sizeof(m_szBuffer));
        }
    };

    ///
    /// Support for string conversion with memory sanitization after use
    ///
    typedef CW2AParanoidEX<> CW2AParanoid;

    /// @}
}
