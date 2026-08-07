// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       MemUtilsInternal.h
//------------------------------------------------------------------------------
#ifndef UtilLib__MemUtilsInternal_h__INCLUDED
#define UtilLib__MemUtilsInternal_h__INCLUDED

// Debug Support ---------------------------------------------------------------
#if DBG != 1 && RETAILDEBUGLIB != 1

    #define DbgPreAlloc(cb)             cb
    #define DbgPostAlloc(pv)            pv
    #define DbgPreFree(pv)              pv
    #define DbgPostFree()
    #define DbgPreRealloc(pv, cb, ppv)  cb
    #define DbgPostRealloc(pv)          pv
    #define DbgPreGetSize(pv)           pv
    #define DbgPostGetSize(cb)          cb
    #define DbgPreDidAlloc(pv)          pv
    #define DbgPostDidAlloc(pv, fAct)   fAct

    #define DbgMemoryTrackDisable(fb)
    #define DbgCoMemoryTrackDisable(fb)
    #define DbgMemoryBlockTrackDisable(pv)

    #define CHECK_HEAP()

#else

    #define DbgPreAlloc                 DbgExPreAlloc
    #define DbgPostAlloc                DbgExPostAlloc
    #define DbgPreFree                  DbgExPreFree
    #define DbgPostFree                 DbgExPostFree
    #define DbgPreRealloc               DbgExPreRealloc
    #define DbgPostRealloc              DbgExPostRealloc
    #define DbgPreGetSize               DbgExPreGetSize
    #define DbgPostGetSize              DbgExPostGetSize
    #define DbgPreDidAlloc              DbgExPreDidAlloc
    #define DbgPostDidAlloc             DbgExPostDidAlloc

    #define DbgMemoryTrackDisable       DbgExMemoryTrackDisable
    #define DbgCoMemoryTrackDisable     DbgExCoMemoryTrackDisable
    #define DbgMemoryBlockTrackDisable  DbgExMemoryBlockTrackDisable

    //
    // Use the CHECK_HEAP macro to do thorough heap validation.
    //
    BOOL CheckSmallBlockHeap();
    void WINAPI DbgExCheckHeap();
    #define CHECK_HEAP()                DbgExCheckHeap();

#endif

#endif // UtilLib__MemUtilsInternal_h__INCLUDED


