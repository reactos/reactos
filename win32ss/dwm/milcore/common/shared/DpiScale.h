// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains DpiScale declaration
//
//-----------------------------------------------------------------------------

#pragma once
#include <limits>
#include "scopeguard.h"
#include "DpiUtil.h"

/// <summary>
/// Stores DPI/PPI information
/// </summary>
/// <remarks>
/// Similar to the managed <see cref="System.Windows.DpiScale"/> structure, and 
/// incorporates pieces of <see cref="MS.Internal.PresentationCore.DpiUtil"/>. 
/// 
/// Also provides some additional functionality exposed 
/// </remarks>
struct DpiScale
{
    /// <summary>
    /// The DPI scale on the X axis. When the DPI is 96, this value is 1.
    /// </summary>
    /// <remarks>On Windows Desktop, this value is the same as <see cref="DpiScaleY"/></remarks>
    float DpiScaleX;

    /// <summary>
    /// The DPI scale on the Y axis. When the DPI is 96, this value is 1. 
    /// </summary>
    /// <remarks>On Windows Desktop, this value is the same as <see cref="DpiScaleX"/></remarks>
    float DpiScaleY;

    /// <summary>
    /// Constructor
    /// </summary>
    /// <remarks>
    /// Works for any type parameters that can be converted 
    /// to float
    /// </remarks>
    template<
        typename T1, typename T2 = T1, 
        typename = typename std::enable_if<std::is_convertible<T1, float>::value>::type,
        typename = typename std::enable_if<std::is_convertible<T2, float>::value>::type
    >
    DpiScale(const T1& dpiScaleX, const T2& dpiScaleY)
        : DpiScaleX(static_cast<float>(dpiScaleX)), DpiScaleY(static_cast<float>(dpiScaleY))
    {}

    /// <summary>
    /// Default constructor
    /// </summary>
    /// <remarks>
    /// This represents an invalid DPI scale value.
    /// </remarks>
    inline DpiScale() :
        DpiScale(0.0f, 0.0f)
    {}

    /// <summary>
    /// Copy Constructor
    /// </summary>
    inline DpiScale(const DpiScale&) = default;

    /// <summary>
    /// Instantiates <see cref="DpiScale"/> from PPI values
    /// </summary>
    template<
        typename T1, 
        typename T2= T1, 
        typename = typename std::enable_if<std::is_convertible<T1, float>::value>::type,
        typename = typename std::enable_if<std::is_convertible<T2, float>::value>::type
    >
    inline static DpiScale FromPixelsPerInch(const T1& ppiX, const T2& ppiY)
    {
        return 
            DpiScale(
                static_cast<float>(ppiX) / DpiScale::DefaultPixelsPerInch(), 
                static_cast<float>(ppiY) / DpiScale::DefaultPixelsPerInch());
    }

    /// <summary>
    /// Copy-assignment operator
    /// </summary>
    inline DpiScale& operator=(const DpiScale&) = default;


    /// <summary>
    /// Assignment from a tuple
    /// </summary>
    template<
        typename T1,
        typename T2 = T1,
        typename = typename std::enable_if<std::is_convertible<T1, float>::value>::type,
        typename = typename std::enable_if<std::is_convertible<T2, float>::value>::type
    >
        inline DpiScale& operator=(const std::tuple<T1, T2>& val)
    {
        DpiScaleX = static_cast<float>(std::get<0>(val));
        DpiScaleY = static_cast<float>(std::get<1>(val));

        return *this;
    }

    /// <summary>
    /// Two DPI scale values are equal if they are equal after
    /// rounding up to hundredths place.
    /// </summary>
    /// <remarks>
    /// Common PPI values are:
    ///     96  (100% : 1.00)
    ///     120 (125% : 1.25)
    ///     144 (150% : 1.50)
    ///     192 (200% : 2.00)
    /// </remarks>
    inline bool operator==(const DpiScale& other) const
    {
        return
            essentially_equals(DpiScaleX, other.DpiScaleX) &&
            essentially_equals(DpiScaleY, other.DpiScaleY);
    }

    /// <summary>
    /// Inequality operator
    /// </summary>
    inline bool operator!=(const DpiScale& other) const
    {
        return !(!this == other);
    }

    /// <summary>
    /// Goodness test
    /// On windows, we expect the scale factor on both axes to the equal, 
    /// and non-zero. 
    /// </summary>
    inline operator bool() const
    {
        return
            essentially_equals(DpiScaleX, DpiScaleY) &&
            DpiScaleX > 0;
    }

    /// <summary>
    /// Scalar multiplication-assignment operator
    /// </summary>
    inline DpiScale& operator*=(float factor)
    {
        DpiScaleX *= factor;
        DpiScaleY *= factor;

        return *this;
    }

    /// <summary>
    /// Vector multiplication-assignment operator
    /// </summary>
    inline DpiScale& operator*=(const DpiScale& other)
    {
        DpiScaleX *= other.DpiScaleX;
        DpiScaleY *= other.DpiScaleY;

        return *this;
    }

    /// <summary>
    /// Scalar multiplication operator
    /// </summary>
    inline DpiScale operator*(float factor) const
    {
        DpiScale dpi(*this);
        dpi *= factor;

        return dpi;
    }

    /// <summary>
    /// Scalar multiplication operator 
    /// </summary>
    inline friend DpiScale operator*(float factor, DpiScale dpi)
    {
        return dpi * factor;
    }

    /// <summary>
    /// Vector multiplication operator
    /// </summary>
    inline DpiScale operator* (const DpiScale other) const
    {
        DpiScale dpi(*this);
        dpi *= other;

        return dpi;
    }

    /// <summary>
    /// Scalar division-assignment operator
    /// </summary>
    /// <remarks>Caller is responsible for avoiding divide-by-zero error</remarks>
    inline DpiScale& operator/= (float divisor)
    {
        DpiScaleX /= divisor;
        DpiScaleY /= divisor;

        return *this;
    }

    /// <summary>
    /// Vector division-assignment operator
    /// </summary>
    /// <remarks>Caller is responsible for avoiding divide-by-zero error</remarks>
    inline DpiScale& operator/=(const DpiScale& other)
    {
        DpiScaleX /= other.DpiScaleX;
        DpiScaleY /= other.DpiScaleY;

        return *this;
    }

    /// <summary>
    /// Scalar division operator
    /// </summary>
    /// <remarks>Caller is responsible for avoiding divide-by-zero error</remarks>
    inline DpiScale operator/(float divisor) const
    {
        DpiScale dpi(*this);
        dpi /= divisor;

        return dpi;
    }

    /// <summary>
    /// Scalar division operator
    /// </summary>
    /// <remarks>Caller is responsible for avoiding divide-by-zero error</remarks>
    friend inline DpiScale operator/(float numerator, DpiScale dpi)
    {
        return DpiScale(numerator / dpi.DpiScaleX, numerator / dpi.DpiScaleY);
    }

    /// <summary>
    /// Vector division operator
    /// </summary>
    /// <remarks>Caller is responsible for avoiding divide-by-zero error</remarks>
    inline DpiScale operator/(DpiScale other) const
    {
        DpiScale dpi(*this);
        dpi /= other;

        return dpi;
    }

    /// <summary>
    /// Pixels per DIP ~= <see cref="DpiScaleY"/>
    /// </summary>
    inline float PixelsPerDip() const
    {
        return DpiScaleY;
    }

    /// <summary>
    /// PPI along X-axis
    /// </summary>
    /// <remarks>
    /// On Windows Desktop, this value is the same as <see cref="PixelsPerInchY()"/>
    /// </remarks>
    inline float PixelsPerInchX() const
    {
        return DefaultPixelsPerInch() * DpiScaleX;
    }

    /// <summary>
    /// PPI along Y-axis
    /// </summary>
    /// <remarks>
    /// On Windows Desktop, this value is the same as <see cref="PixelsPerInchX()"/>
    /// </remarks>
    inline float PixelsPerInchY() const
    {
        return DefaultPixelsPerInch() * DpiScaleY;
    }

    /// <summary>
    /// Default PPI = 96
    /// Historically, most display devices were 96 PPI devices
    /// </summary>
    inline static float DefaultPixelsPerInch()
    {
        return 96.0f;
    }

    inline static const DpiScale& PrimaryDisplayDpi()
    {
        static bool initialized = false; 
        static DpiScale primaryDisplayDpi; 

        if (!initialized)
        {
            // user32!GetDpiForSystem is only supported on 
            // Windows 10 v1607+
            //
            // GetDpiForSystem is more efficient and more reliable than 
            // calling GetDC+GetDeviceCaps.MSDN has the following explanation: 
            // 
            // Any component that could be running 
            // in an application that uses sub-process DPI awareness should not 
            // assume that the system DPI is static during the life cycle of the process. 
            // For example, if a thread that is running under DPI_AWARENESS_CONTEXT_UNAWARE 
            // awareness context queries the system DPI, the answer will be 96. However, if that 
            // same thread switched to DPI_AWARENESS_CONTEXT_SYSTEM awareness context and queried 
            // the system DPI again, the answer could be different. To avoid the use of a 
            // cached system-DPI value being used in an incorrect thread DPI_AWARENESS_CONTEXT, use
            // GetDpiForSystem to retrieve the system DPI relative to the DPI awareness mode of the
            // calling thread.
            // 
            // Though this API is intended to support dynamic querying, WPF's current design caches the system
            // DPI nevertheless. This design exists for historical reasons, and this cache lives in the UI thread
            // in Visual.cs. WPF uses this cache carefully taking into consideration the thread's DPI_AWARENESS_CONTEXT.
            auto systemDpi = wpf::util::DpiUtil::GetDpiForSystem();

            if (systemDpi > 0)
            {
                primaryDisplayDpi = DpiScale::FromPixelsPerInch(systemDpi, systemDpi);
                initialized = true;
            }


            if (!initialized)
            {
                // GetDpiForSystem failed, try GetDC + GetDeviceCaps
                // CreateIC is a lightweight alternative to GetDC 
                wpf::util::scopeguard<HDC> hDesktopDC(
                    []() -> HDC {return CreateICW(L"DISPLAY", nullptr, nullptr, nullptr); },    // acquire HDC
                    [](HDC& hDC) ->void {DeleteDC(hDC); },                                      // release HDC
                    [](const HDC& hDC) ->bool {return hDC != nullptr; });                       // goodness test

                if (hDesktopDC.valid())
                {
                    primaryDisplayDpi = 
                        DpiScale::FromPixelsPerInch(
                            GetDeviceCaps(hDesktopDC, LOGPIXELSX), 
                            GetDeviceCaps(hDesktopDC, LOGPIXELSY));
                }

                if (primaryDisplayDpi.DpiScaleX <= 0 || primaryDisplayDpi.DpiScaleY <= 0)
                {
                    TraceTag((tagMILWarning, "GetDeviceCaps failed"));

                    primaryDisplayDpi = 
                        DpiScale::FromPixelsPerInch(
                            DefaultPixelsPerInch(), 
                            DefaultPixelsPerInch());
                }

                initialized = true;
            }
        }

        return primaryDisplayDpi;
    }

private:

    /// <summary>
    /// Based on TAOCP Vol 2. Section 4.2.2.A.
    /// </summary>
    inline static bool essentially_equals(float x, float y)
    {
        const auto epsilon = std::numeric_limits<float>::epsilon();

        auto abs_x = std::abs(x);
        auto abs_y = std::abs(y);
        auto abs_diff = std::abs(abs_x - abs_y);

        return
            abs_x > abs_y
            ? abs_diff <= abs_y * epsilon
            : abs_diff <= abs_x * epsilon;
    }
};

