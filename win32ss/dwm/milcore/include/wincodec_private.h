// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      MIL <-> WIC Interop Interfaces and Enums
//


#include "wincodec.h"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI WICCreateImagingFactory_Proxy(
    __in UINT SDKVersion,
    __deref_out_ecount(1) IWICImagingFactory **ppIWICImagingFactory
    );

#ifdef __cplusplus
}
#endif

#ifndef __WINCODEC_PRIVATE_H__
#define __WINCODEC_PRIVATE_H__
#pragma once

#ifndef BEGIN_MILENUM

#define BEGIN_MILENUM(type)                     \
    namespace type {                            \
            enum Enum {	                        \

#define END_MILENUM                             \
            FORCE_DWORD = 0xffffffff            \
        };                                      \
    }

#define MILENUM(type) type::Enum

#endif //BEGIN_MILENUM

#ifndef BEGIN_MILFLAGENUM

#define BEGIN_MILFLAGENUM(type)                 \
    namespace type {                            \
        enum FlagsEnum {                        \

#define END_MILFLAGENUM                         \
           FORCE_DWORD = 0xffffffff             \
        };                                      \
                                                \
        typedef TMILFlagsEnum<FlagsEnum> Flags; \
    }
    
#define MILFLAGENUM(type) type::Flags

#define WINCODEC_SDK_VERSION_WPF 0x0236

DEFINE_GUID(CLSID_WICImagingFactoryWPF, 0xcacaf262, 0x9370, 0x4615, 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0xa);

template<class E>
struct TMILFlagsEnum
{
    E flags;

    TMILFlagsEnum() { }
    TMILFlagsEnum(const E &_Right) { flags = _Right; }
    TMILFlagsEnum(const int &_Right) { flags = static_cast<E>(_Right); }

    operator const E &() const { return flags; }

    TMILFlagsEnum &operator|=(const int &_Right)
    {
        flags = static_cast<E>(flags | _Right);
        return *this;
    }

    TMILFlagsEnum &operator&=(const int &_Right)
    {
        flags = static_cast<E>(flags & _Right);
        return *this;
    }

    TMILFlagsEnum &operator^=(const int &_Right)
    {
        flags = static_cast<E>(flags ^ _Right);
        return *this;
    }
};

#endif //BEGIN_MILFLAGENUM

#include "wincodec_private_generated.h"

/*=========================================================================*\
    WICPixelFormat Stuff Interop
\*=========================================================================*/

inline bool WicPfIsMil(REFWICPixelFormatGUID wicPf)
{
    // (radum): this check is an unfortunate legacy side effect. While most extended pixel formats
    // are stored in GUIDS, there are still a number of areas in the code which expect them in enums,
    // and for time reasons they could not be changed. (Task# 53609)
    if (static_cast<MilPixelFormat::Enum>(wicPf.Data4[7]) > MilPixelFormat::CMYK32bpp)
    {
        return false;
    }

    if (0 == memcmp(&wicPf, &GUID_WICPixelFormatDontCare, sizeof(WICPixelFormatGUID) - 1))
    {
        return true;
    }
    else
    {
        return false;
    }
}

inline HRESULT WicPfToMil(REFWICPixelFormatGUID wicPf, MilPixelFormat::Enum *milPf)
{
    HRESULT hr = S_OK;

    if (WicPfIsMil(wicPf))
    {
        *milPf = static_cast<MilPixelFormat::Enum>(wicPf.Data4[7]);
    }
    else
    {
        hr = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        *milPf = MilPixelFormat::Undefined;
    }

    return hr;
}

inline WICPixelFormatGUID MilPfToWic(MilPixelFormat::Enum milPf)
{
    WICPixelFormatGUID wicPf = GUID_WICPixelFormatDontCare;
    wicPf.Data4[7] = static_cast<BYTE>(milPf);

    return wicPf;
}

inline bool WicPfEqualsMil(REFWICPixelFormatGUID wicPf, MilPixelFormat::Enum milPf)
{
    return WicPfIsMil(wicPf) ? wicPf.Data4[7] == milPf : false;
}

//
// Interface for managed stream interop
//
DECLARE_INTERFACE_(IManagedStream, IStream)
{
    STDMETHOD(CanWrite)(
        __out_ecount(1) BOOL *pfCanWrite
        ) PURE;

    STDMETHOD(CanSeek)(
        __out_ecount(1) BOOL *pfCanSeek
        ) PURE;
};
#endif


