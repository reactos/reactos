// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains declarations for generic render utility routines.
//
//------------------------------------------------------------------------------

#pragma once 

//+------------------------------------------------------------------------
//
//  Function:  ClampValueMin
//
//  Synopsis:  Template method that clamps a value to a min value.
//
//  Notes:     This method is written in manner such that NaNs
//             are clamped to the minValue.
//
//-------------------------------------------------------------------------
template <class T>
inline T
ClampValueMin(T value, T minValue)
{
    if (value >= minValue)
    {
        return value;
    }
    else
    {
        return minValue;
    }
}

//+------------------------------------------------------------------------
//
//  Function:  ClampMinDouble
//
//  Synopsis:  Clamps a double to to a min value.
//
//-------------------------------------------------------------------------
inline DOUBLE
ClampMinDouble(DOUBLE value, DOUBLE rMin)
{
    return ClampValueMin<DOUBLE>(value, rMin);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampMinFloat
//
//  Synopsis:  Clamps a float to to a min value.
//
//-------------------------------------------------------------------------
inline float
ClampMinFloat(float value, float rMin)
{
    return ClampValueMin<float>(value, rMin);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampValue
//
//  Synopsis:  Template method that clamps a value to a range
//
//  Notes:     This method is written in manner such that NaNs
//             are clamped to the minValue.
//
//-------------------------------------------------------------------------
template <class T>
inline T
ClampValue(T value, T minValue, T maxValue)
{
    Assert(minValue <= maxValue);

    if (value > maxValue)
    {
        return maxValue;
    }
    else if (value >= minValue)
    {
        return value;
    }
    else
    {
        return minValue;
    }
}

//+------------------------------------------------------------------------
//
//  Function:  ClampInteger
//
//  Synopsis:  Clamps an integer to the specified range.
//
//-------------------------------------------------------------------------
inline INT
ClampInteger(INT value, INT nMin, INT nMax)
{
    return ClampValue<INT>(value, nMin, nMax);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampReal
//
//  Synopsis:  Clamps a float to the specified range.
//
//-------------------------------------------------------------------------
inline REAL
ClampReal(REAL value, REAL rMin, REAL rMax)
{
    return ClampValue<REAL>(value, rMin, rMax);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampDouble
//
//  Synopsis:  Clamps a double to the specified range.
//
//-------------------------------------------------------------------------
inline DOUBLE
ClampDouble(DOUBLE value, DOUBLE rMin, DOUBLE rMax)
{
    return ClampValue<DOUBLE>(value, rMin, rMax);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampAlpha (REAL)
//
//  Synopsis:  Clamps a float alpha value
//
//-------------------------------------------------------------------------
inline REAL
ClampAlpha(
    REAL alpha
    )
{
    return ClampReal(alpha, 0.0f, 1.0f);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampAlpha (DOUBLE)
//
//  Synopsis:  Clamps a double alpha value
//
//-------------------------------------------------------------------------
inline DOUBLE
ClampAlpha(
    DOUBLE alpha
    )
{
    return ClampDouble(alpha, 0.0, 1.0);
}

//+------------------------------------------------------------------------
//
//  Function:  ClampValueUnordered
//
//  Synopsis:  Template method that clamps a value to an unordered range
//
//  Notes:     This method is written in manner such that NaNs
//             are clamped to the minimum of the range.
//
//-------------------------------------------------------------------------
template <class T>
inline T
ClampValueUnordered(T value, T extrema1, T extrema2)
{
    if (extrema1 < extrema2)
    {
        return ClampValue(value, extrema1, extrema2);
    }
    else
    {
        return ClampValue(value, extrema2, extrema1);
    }
}


//+----------------------------------------------------------------------------
//
//  Template:  AssertOrderedDiffValid
//
//  Synopsis:  Assert that difference of start and end don't overflow type's
//             supported range
//

template <typename>
class TypeProperties {};

template <>
class TypeProperties<float>
{
    void DefinitionFor_finite();
};

template <>
class TypeProperties<double>
{
    void DefinitionFor_finite();
};

template<class TBase>
void
AssertOrderedDiffValid(
    TBase start,
    TBase end
    )
{
#if DBG
    typedef TypeProperties<TBase> TBaseTraitMap;

    // This assert is only for overflow and not underflow; so assert that
    // underflow isn't even possible.
    Assert(start <= end);

    //
    // Test for overflow
    //
    // There is one situation that can overflow with an ordered difference:
    //
    //  positive - negative > MAX
    //
    // Using the observation that start must be negative to have overflow we
    // can simplify the overflow check to just see that the result is finite
    // and greater than end.
    //
    // First check start range
    //
    if (start < 0)
    {
        TBase difference = end - start;

        //
        // Check for a definition of _finite for this type which indicates the
        // type has an infinite value - floating point types.  We expect the
        // result to be infinite for these types rather than to wrap below end.
        //

        __if_exists(TBaseTraitMap::DefinitionFor_finite)
        {
            //
            // Check to see if we have a finite difference.
            //

            Assert(_finite(difference));
        }

        __if_not_exists(TBaseTraitMap::DefinitionFor_finite)
        {
            //
            // There is no concept of infinite so as long as the resulting
            // value is greater than the original we didn't overflow.
            //
            // Note we do not allow difference to equal end.  That could happen
            // for floating point types, but the _finite check should be used
            // for those types.
            //

            Assert(difference > end);
        }
    }

#endif
}


/*=========================================================================*\
    Common code for all entry points:
\*=========================================================================*/

ExternTag(tagMILApiCalls)
ExternTag(tagMILApiCallWarnings)

#if DBG
void DbgCheckAPI(HRESULT hr);
#endif

#if DBG

    #define API_ENTRY_NOFPU(a) \
        TraceTag((tagMILApiCalls, #a));

    #define API_ENTRY(a) \
        TraceTag((tagMILApiCalls, #a)); \
        CFloatFPU fps

#else

    #define API_ENTRY_NOFPU(a)

    #define API_ENTRY(a) \
        CFloatFPU fps

#endif

#if DBG

    #define API_CHECK(hr) \
      { \
        /* Use a local HRESULT in case hr is not a variable */ \
        HRESULT __hr = (hr); \
        if (FAILED(__hr)) \
            TraceTag((tagMILApiCallWarnings, "API failure code %x", __hr)); \
        MIL_CHECKHR_ADDFLAGS(__hr, MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL); \
        DbgCheckAPI(__hr); \
      }
#else

    #define API_CHECK(hr) \
      { \
        /* Use a local HRESULT in case hr is not a variable */ \
        HRESULT __hr = (hr); \
        MIL_CHECKHR_ADDFLAGS(__hr, MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL); \
      }

#endif


#define API_CALLBACK_BEGIN \
        __try \
        { \
            FPUStateSandbox fps;

#define API_CALLBACK_END \
        } \
        __except(EXCEPTION_EXECUTE_HANDLER) \
        { \
            hr = E_FAIL; \
        }

inline void APIError(const char *pszMessage)
{
    // for now, this simply emits a WARNING, however
    // WARNING has a lot of extraneous spew (filename, line number, etc. )
    // which we probably don't want from an API message.

    TraceTag((tagMILApiCallWarnings, "MIL Error: %s", pszMessage));
    pszMessage;
}

//+------------------------------------------------------------------------
//
//  Function:  ReleaseHandle(HANDLE&)
//
//  Synopsis:  Releases a handle if not NULL, then nulls it out.
//
//-------------------------------------------------------------------------
static inline void ReleaseHandle(HANDLE& handle)
{
    if (handle != NULL)
    {
        CloseHandle(handle);
        handle = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:  InterlockedIncrementULONG, InterlockedDecrementULONG
//
//  Synopsis:  Thunk to LONG verion of InterlockedIncrement/Decrement
//

MIL_FORCEINLINE
ULONG
InterlockedIncrementULONG(
    ULONG volatile *pAddend
    )
{
    return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG volatile *>(pAddend)));
}

MIL_FORCEINLINE
ULONG
InterlockedDecrementULONG(
    ULONG volatile *pAddend
    )
{
    return static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG volatile *>(pAddend)));
}


inline int RectWidth(__in const RECT &rc)
{
    return max(0L, rc.right - rc.left);
}

inline int RectHeight(__in const RECT &rc)
{
    return max(0L, rc.bottom - rc.top);
}


inline bool IsRectEmpty(__in const RECT &rc)
{
    return ((rc.right <= rc.left) || (rc.bottom <= rc.top));
}

//+-----------------------------------------------------------------------
//  Synopsis:  compare if twp LUIDs are equal
//
//  Returns: 
//
//------------------------------------------------------------------------
inline bool operator==(const LUID &l1, const LUID &l2)
{
    return ((l1.LowPart == l2.LowPart) && 
            (l1.HighPart == l2.HighPart));
}

inline bool operator!=(const LUID &l1, const LUID &l2)
{
    return !(l1 == l2);
}

//+-----------------------------------------------------------------------------
// 
//    Function:
//        WrapHandleInUInt64
// 
//    Synopsis:
//        A helper function used by the marshaler, converts a Win32 handle to UINT64.
// 
//------------------------------------------------------------------------------

inline UINT64 WrapHandleInUInt64(HANDLE handle)
{
    // Note: this conversion is always safe by the assertion below
    C_ASSERT(sizeof(HANDLE) <= sizeof(UINT64));

    return static_cast<UINT64>(reinterpret_cast<UINT_PTR>(handle));
}


//+-----------------------------------------------------------------------------
// 
//    Function:
//        UnwrapHandleFromUInt64
// 
//    Synopsis:
//        A helper function used by the marshaler, converts UINT64 to a Win32 handle.
// 
//------------------------------------------------------------------------------

inline HANDLE UnwrapHandleFromUInt64(UINT64 handle)
{
    // Note: Win32 handles take 32-bit values only. Convert larger values to INVALID_HANDLE_VALUE...
    C_ASSERT((sizeof(HANDLE) == sizeof(UINT32)) || (sizeof(HANDLE) == sizeof(UINT64)));

    return (handle <= UINT_MAX)
        ? reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(handle))
        : INVALID_HANDLE_VALUE;
}






