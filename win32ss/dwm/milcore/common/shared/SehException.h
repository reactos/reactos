// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once
#include <exception>
#include <winnt.h>
#include <eh.h>


namespace wpf
{
    namespace util
    {
        namespace exceptions
        {
            /// <summary>
            /// Represents an SEH exception as a C++ exception
            /// </summary>
            /// <remarks>
            /// Only the excpetion code is preserved, the <see cref="_EXCEPTION_POINTERS"/>
            /// information is not carried in this class.
            /// </remarks>
            class SehException : public std::exception
            {
            private:
                unsigned int m_code;

            public:
                inline SehException(unsigned int code)
                    : m_code(code)
                {}

                inline int code() const
                {
                    return m_code;
                }
            };

            /// <summary>
            /// Translates an SEH exception into a C++ <see cref="SehException"/> 
            /// </summary>
            /// <remarks>
            /// Requires that the module be compile with /Ehsa for <see cref="_set_se_translator"/> 
            /// etc. to function.
            /// 
            /// Usage: 
            /// 
            /// <code><![CDATA[
            /// {
            ///     ....
            ///     // All SEH exceptions will be translated into 
            ///     // SehException from here onwards
            ///     SehGuard guard; 
            ///     try
            ///     {
            ///         
            ///     }
            ///     catch (const SehException& sehex)
            ///     {
            ///         std::cout << sehex.code();
            ///     }
            ///     ...
            /// }   // SehGuard stops being effective at the end of the enclosing block
            /// ]]></code>
            /// </remarks>
            class SehGuard
            {
            private:
                _se_translator_function m_prev;

            public:
                SehGuard()
                {
                    m_prev = _set_se_translator([](unsigned int code, struct _EXCEPTION_POINTERS*)
                    {
                        throw SehException(code);
                    });
                }

                ~SehGuard()
                {
                    _set_se_translator(m_prev);
                }
            };
        }
    }
}
