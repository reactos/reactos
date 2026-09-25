// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

// Shared data structures and exception type definitions
// used by the DX abstraction layer

#include <memory>

#include <Windows.h>

#if !defined(TESTUSE_NOSTACKCAPTURE)
#include "AvalonDebugP.h"
#include "instrumentation.h"
#endif



namespace dxlayer
{
    enum class dxapi { d3dx9, xmath };

    // Forward declarations
    template <dxapi> class vector2_t;
    template <dxapi> class vector3_t;
    template <dxapi> class vector4_t;
    template <dxapi> class quaternion_t;
    template <dxapi> class matrix_t;

    template<dxapi> struct vector3pair_t;

    template<dxapi> struct color_t;

    // Shared types

    // Dummy type used as a placeholder
    // Template specializations for basetypes<dxapi>
    // will replace this with the actual platform types.
    struct dummy_t
    {
    };

    // These will always be types that are size and 
    // layout compatible with D3DX9.
    template <dxapi apiset>
    struct basetypes
    {
        typedef dummy_t vector2_base_t;
        typedef dummy_t vector3_base_t;
        typedef dummy_t vector4_base_t;
        typedef dummy_t quaternion_base_t;
        typedef dummy_t matrix_base_t;
        typedef dummy_t color_base_t;
    };

    // Enum describing 2D axes
    enum class axis_2d {X, Y};
    // Enum describing 3D axes
    enum class axis_3d {X, Y, Z};
    // Enum describing 4D axes
    enum class axis_4d {X, Y, Z, W};


    // Pair of 3D vectors
    template<dxapi apiset>
    struct vector3pair_t: public std::pair<vector3_t<apiset>, vector3_t<apiset>>
    {
        inline vector3pair_t(const vector3_t<apiset>& first, const vector3_t<apiset>& second)
            : std::pair<vector3_t<apiset>, vector3_t<apiset>>(first, second)
        {
            // empty
        }

        template <typename... Args>
        vector3pair_t(Args&&...args)
            : std::pair<vector3_t<apiset>, vector3_t<apiset>>(std::forward<Args>(args)...)
        {
        }

        // move constructor
        inline vector3pair_t(vector3pair_t<apiset>&& source)
            : std::pair<vector3_t<apiset>, vector3_t<apiset>>(source)
        {
            // empty
        }

        inline vector3pair_t<apiset>& operator=(const vector3pair_t<apiset>& source)
        {
            first = source.first;
            second = source.second;

            return *this;
        }
    };

    // Represents a generic Win32 error
    // Base class for dxerror, hresult
    class winerror
    {
    public:
        virtual ~winerror() {}
    };

    // Generic dxlayer error
    class dxerror : public winerror
    {
    public:
        virtual ~dxerror() {}
    };

    // Represents a HRESULT based error
    class hresult : public winerror
    {
        HRESULT hr;
    public:
        inline hresult(HRESULT hr) : hr(hr) 
        {
#if !defined(TESTUSE_NOSTACKCAPTURE)
            DoStackCapture(1, hr, __LINE__);
#endif
        }

        inline hresult() : hresult(E_FAIL) {}

        inline HRESULT get_hr() const { return hr; }

        virtual ~hresult() {}
    };

    // Represents an error-code (GetLastError()) based error
    class errcode : public winerror
    {
        DWORD err_code;
    public:
        inline errcode(DWORD err) : err_code(err) 
        {
#if !defined(TESTUSE_NOSTACKCAPTURE)
            DoStackCapture(1, HRESULT_FROM_WIN32(err), __LINE__);
#endif
        }

        inline errcode() : errcode(static_cast<DWORD>(-1)) {}

        inline DWORD get_err() { return err_code; }

        virtual ~errcode() {}
    };

    // Exception encapsulating a winerror - i.e., a hresult, errcode or 
    // a non-specific dxerror
    class dxlayer_exception: public std::exception
    {
    private:
        winerror error;
    public:
        inline dxlayer_exception(const winerror& error) 
            : error(error), std::exception() {}

        inline dxlayer_exception()
            : error(dxerror()), std::exception() {}

        inline dxlayer_exception(const char* message)
            : error(dxerror()), std::exception(message) {}

        inline const winerror& get_error() const
        {
            return error;
        }

    };

    // Represents an assertion failure in a dxlayer type
    // This type is accessed via the static method 
    //    dxlayer_assert::check(bool expr)
    // If the assertion fails, check() will call
    // std::terminate to end the program
    class dxlayer_assert
    {
    public:
        template <typename...Args>
        static void check(bool expr)
        {
            if (!expr)
            {
                std::terminate();
            }
        }
    };

    class invalid_index_assert : public dxlayer_assert
    {
    public:
        inline static void __declspec(noreturn) terminate()
        {
            check(false);
        }
    };

    // Definition for floating point equality comparisons
    // This is based on The Art of Computer Programming by Donald Knuth, 
    // Vol 2. Section 4.2.2.A. 
    template<typename Floating, typename = std::enable_if_t<std::is_floating_point<Floating>::value>>
    struct comparer
    {
        inline bool operator()(const Floating& a, const Floating& b) const
        {
            return is_essentially_equal_to(a, b);
        }

        static inline bool is_essentially_equal_to(const Floating& a, const Floating& b)
        {
            auto abs_a = std::abs(a);
            auto abs_b = std::abs(b);
            auto abs_diff = std::abs(a - b);

            if (abs_a > abs_b)
            {
                return (abs_diff <= (abs_b * std::numeric_limits<Floating>::epsilon()));
            }
            else
            {
                return (abs_diff <= (abs_a * std::numeric_limits<Floating>::epsilon()));
            }
        }
    };
}

