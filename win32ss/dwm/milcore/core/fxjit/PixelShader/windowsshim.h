// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------
//

//
//  Shim windows.h.  This file is included in Jolt and other clients
//  should include windows.h instead
//
//------------------------------------------------------------------------

#ifndef XRESULT 
#define XRESULT long
#endif

#ifndef XUINT16
#define XUINT16 unsigned short
#endif

#define __INC_XCPUNKNWN__     // so that we don't get IUknown thunks from typedefs.h

#pragma warning(disable:4201) // nameless struct warning
#include "typedefs.h" 

extern "C"
{
    _Post_equal_to_(_Dst)
        _At_buffer_(
        (unsigned char*)_Dst,
            _Iter_,
            _Size,
            _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == _Val)
        )
        void* __cdecl memset(
            _Out_writes_bytes_all_(_Size) void* _Dst,
            _In_                          int    _Val,
            _In_                          size_t _Size
        );

    _Post_equal_to_(_Dst)
        _At_buffer_(
        (unsigned char*)_Dst,
            _Iter_,
            _Size,
            _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_])
        )
        void* __cdecl memcpy(
            _Out_writes_bytes_all_(_Size) void* _Dst,
            _In_reads_bytes_(_Size)       void const* _Src,
            _In_                          size_t      _Size
        ); 
}


