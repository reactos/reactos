// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once 
#include <system_error>
#include <Windows.h>

namespace wpf
{
    namespace util
    {
        namespace exceptions
        {
            /// <summary>
            /// Returns an <see cref="std::system_error"/> instance that represents the last 
            /// Win32 error returned by <see cref="GetLastError()"/>
            /// </summary>
            inline std::system_error win32error()
            {
                return std::system_error(std::error_code(GetLastError(), std::system_category()));
            }
        }
    }
}
