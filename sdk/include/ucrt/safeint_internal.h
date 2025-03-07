/***
*safeint_internal.h - Internal details for SafeInt (see safeint.h)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Private internal details for SafeInt.
*       The constructs and functions in Microsoft::Utilities::details are not
*       meant to be used by external code and can change at any time.
*
****/

#pragma once

#include <crtdbg.h>

#pragma pack(push, _CRT_PACKING)

namespace msl
{

namespace utilities
{

namespace details
{
#pragma warning(push)
#pragma warning(disable:4702)

template < typename T >
class DependentFalse { public: enum{ value = false }; };

template < typename T > class NumericType;

template <> class NumericType<bool>             { public: enum{ isBool = true,  isFloat = false, isInt = false }; };
template <> class NumericType<char>             { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<unsigned char>    { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<signed char>      { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<short>            { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<unsigned short>   { public: enum{ isBool = false, isFloat = false, isInt = true }; };
#ifdef _NATIVE_WCHAR_T_DEFINED
template <> class NumericType<wchar_t>          { public: enum{ isBool = false, isFloat = false, isInt = true }; };
#endif  /* _NATIVE_WCHAR_T_DEFINED */
template <> class NumericType<int>              { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<unsigned int>     { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<long>             { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<unsigned long>    { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<__int64>          { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<unsigned __int64> { public: enum{ isBool = false, isFloat = false, isInt = true }; };
template <> class NumericType<float>            { public: enum{ isBool = false, isFloat = true,  isInt = false }; };
template <> class NumericType<double>           { public: enum{ isBool = false, isFloat = true,  isInt = false }; };
template <> class NumericType<long double>      { public: enum{ isBool = false, isFloat = true,  isInt = false }; };
// Catch-all for anything not supported
template < typename T > class NumericType       { public: enum{ isBool = false, isFloat = false, isInt = false }; };


template < typename T > class IntTraits
{
public:
    static_assert( NumericType<T>::isInt || NumericType<T>::isBool, "non-integral type T" );
    enum
    {
#pragma warning(suppress:4804)
        isSigned  = ( (T)(-1) < 0 ),
        is64Bit   = ( sizeof(T) == 8 ),
        is32Bit   = ( sizeof(T) == 4 ),
        is16Bit   = ( sizeof(T) == 2 ),
        is8Bit    = ( sizeof(T) == 1 ),
        isLT32Bit = ( sizeof(T) < 4 ),
        isLT64Bit = ( sizeof(T) < 8 ),
        isInt8    = ( sizeof(T) == 1 && isSigned ),
        isUint8   = ( sizeof(T) == 1 && !isSigned ),
        isInt16   = ( sizeof(T) == 2 && isSigned ),
        isUint16  = ( sizeof(T) == 2 && !isSigned ),
        isInt32   = ( sizeof(T) == 4 && isSigned ),
        isUint32  = ( sizeof(T) == 4 && !isSigned ),
        isInt64   = ( sizeof(T) == 8 && isSigned ),
        isUint64  = ( sizeof(T) == 8 && !isSigned ),
        bitCount  = ( sizeof(T)*8 ),
#pragma warning(suppress:4804)
        isBool    = NumericType<T>::isBool
    };

#pragma warning(push)
#pragma warning(disable:4310)
#pragma warning(disable:4804) // suppress warning about '<<' being an unsafe operation when T is bool
    const static T maxInt = isSigned ? ((T)~((T)1 << (T)(bitCount-1))) : ((T)(~(T)0));
    const static T minInt = isSigned ? ((T)((T)1 << (T)(bitCount-1)))  : ((T)0);
#pragma warning(pop)
};

// this is strictly internal and not to be used as a policy in SafeInt<>
struct SafeIntErrorPolicy_NoThrow
{
    static void SafeIntOnOverflow()
    {
    }

    static void SafeIntOnDivZero()
    {
    }
};

template < typename T, typename U > class SafeIntCompare
{
public:
    enum
    {
        isBothSigned   = (IntTraits< T >::isSigned && IntTraits< U >::isSigned),
        isBothUnsigned = (!IntTraits< T >::isSigned && !IntTraits< U >::isSigned),
        isLikeSigned   = (static_cast<bool>(IntTraits< T >::isSigned) == static_cast<bool>(IntTraits< U >::isSigned)),
        isCastOK       = ((isLikeSigned && sizeof(T) >= sizeof(U)) ||
        (IntTraits< T >::isSigned && sizeof(T) > sizeof(U))),
        isBothLT32Bit  = (IntTraits< T >::isLT32Bit && IntTraits< U >::isLT32Bit),
        isBothLT64Bit  = (IntTraits< T >::isLT64Bit && IntTraits< U >::isLT64Bit)
    };
};

template < typename U > class SafeIntCompare< float, U >
{
public:
    enum
    {
        isBothSigned   = IntTraits< U >::isSigned,
        isBothUnsigned = false,
        isLikeSigned   = IntTraits< U >::isSigned,
        isCastOK       = true
    };
};

template < typename U > class SafeIntCompare< double, U >
{
public:
    enum
    {
        isBothSigned   = IntTraits< U >::isSigned,
        isBothUnsigned = false,
        isLikeSigned   = IntTraits< U >::isSigned,
        isCastOK       = true
    };
};

template < typename U > class SafeIntCompare< long double, U >
{
public:
    enum
    {
        isBothSigned   = IntTraits< U >::isSigned,
        isBothUnsigned = false,
        isLikeSigned   = IntTraits< U >::isSigned,
        isCastOK       = true
    };
};

//all of the arithmetic operators can be solved by the same code within
//each of these regions without resorting to compile-time constant conditionals
//most operators collapse the problem into less than the 22 zones, but this is used
//as the first cut
//using this also helps ensure that we handle all of the possible cases correctly

template < typename T, typename U > class IntRegion
{
public:
    enum
    {
        //unsigned-unsigned zone
        IntZone_UintLT32_UintLT32 = SafeIntCompare< T,U >::isBothUnsigned && SafeIntCompare< T,U >::isBothLT32Bit,
        IntZone_Uint32_UintLT64   = SafeIntCompare< T,U >::isBothUnsigned && IntTraits< T >::is32Bit && IntTraits< U >::isLT64Bit,
        IntZone_UintLT32_Uint32   = SafeIntCompare< T,U >::isBothUnsigned && IntTraits< T >::isLT32Bit && IntTraits< U >::is32Bit,
        IntZone_Uint64_Uint       = SafeIntCompare< T,U >::isBothUnsigned && IntTraits< T >::is64Bit,
        IntZone_UintLT64_Uint64   = SafeIntCompare< T,U >::isBothUnsigned && IntTraits< T >::isLT64Bit && IntTraits< U >::is64Bit,
        //unsigned-signed
        IntZone_UintLT32_IntLT32  = !IntTraits< T >::isSigned && IntTraits< U >::isSigned && SafeIntCompare< T,U >::isBothLT32Bit,
        IntZone_Uint32_IntLT64    = IntTraits< T >::isUint32 && IntTraits< U >::isSigned && IntTraits< U >::isLT64Bit,
        IntZone_UintLT32_Int32    = !IntTraits< T >::isSigned && IntTraits< T >::isLT32Bit && IntTraits< U >::isInt32,
        IntZone_Uint64_Int        = IntTraits< T >::isUint64 && IntTraits< U >::isSigned && IntTraits< U >::isLT64Bit,
        IntZone_UintLT64_Int64    = !IntTraits< T >::isSigned && IntTraits< T >::isLT64Bit && IntTraits< U >::isInt64,
        IntZone_Uint64_Int64      = IntTraits< T >::isUint64 && IntTraits< U >::isInt64,
        //signed-signed
        IntZone_IntLT32_IntLT32   = SafeIntCompare< T,U >::isBothSigned && SafeIntCompare< T, U >::isBothLT32Bit,
        IntZone_Int32_IntLT64     = SafeIntCompare< T,U >::isBothSigned && IntTraits< T >::is32Bit && IntTraits< U >::isLT64Bit,
        IntZone_IntLT32_Int32     = SafeIntCompare< T,U >::isBothSigned && IntTraits< T >::isLT32Bit && IntTraits< U >::is32Bit,
        IntZone_Int64_Int64       = SafeIntCompare< T,U >::isBothSigned && IntTraits< T >::isInt64 && IntTraits< U >::isInt64,
        IntZone_Int64_Int         = SafeIntCompare< T,U >::isBothSigned && IntTraits< T >::is64Bit && IntTraits< U >::isLT64Bit,
        IntZone_IntLT64_Int64     = SafeIntCompare< T,U >::isBothSigned && IntTraits< T >::isLT64Bit && IntTraits< U >::is64Bit,
        //signed-unsigned
        IntZone_IntLT32_UintLT32  = IntTraits< T >::isSigned && !IntTraits< U >::isSigned && SafeIntCompare< T,U >::isBothLT32Bit,
        IntZone_Int32_UintLT32    = IntTraits< T >::isInt32 && !IntTraits< U >::isSigned && IntTraits< U >::isLT32Bit,
        IntZone_IntLT64_Uint32    = IntTraits< T >::isSigned && IntTraits< T >::isLT64Bit && IntTraits< U >::isUint32,
        IntZone_Int64_UintLT64    = IntTraits< T >::isInt64 && !IntTraits< U >::isSigned && IntTraits< U >::isLT64Bit,
        IntZone_Int_Uint64        = IntTraits< T >::isSigned && IntTraits< U >::isUint64 && IntTraits< T >::isLT64Bit,
        IntZone_Int64_Uint64      = IntTraits< T >::isInt64 && IntTraits< U >::isUint64
    };
};

// useful function to help with getting the magnitude of a negative number
enum AbsMethod
{
    AbsMethodInt,
    AbsMethodInt64,
    AbsMethodNoop
};

template < typename T >
class GetAbsMethod
{
public:
    enum
    {
        method = IntTraits< T >::isLT64Bit && IntTraits< T >::isSigned ? AbsMethodInt :
        IntTraits< T >::isInt64 ? AbsMethodInt64 : AbsMethodNoop
    };
};

template < typename T, int Method = GetAbsMethod< T >::method > class AbsValueHelper;

template < typename T > class AbsValueHelper < T, AbsMethodInt >
{
public:
    static unsigned __int32 Abs( T t ) throw()
    {
        _ASSERTE( t < 0 );
        return (unsigned __int32)-t;
    }
};

template < typename T > class AbsValueHelper < T, AbsMethodInt64 >
{
public:
    static unsigned __int64 Abs( T t ) throw()
    {
        _ASSERTE( t < 0 );
        return (unsigned __int64)-t;
    }
};

template < typename T > class AbsValueHelper < T, AbsMethodNoop >
{
public:
    static T Abs( T t ) throw()
    {
        // Why are you calling Abs on an unsigned number ???
        _ASSERTE( ("AbsValueHelper::Abs should not be called with an unsigned integer type", 0) );
        return t;
    }
};

template < typename T, typename E, bool fSigned > class NegationHelper;

template < typename T, typename E > class NegationHelper < T, E, true > // Signed
{
public:
    static SafeIntError Negative( T t, T& ret )
    {
        // corner case
        if( t != IntTraits< T >::minInt )
        {
            // cast prevents unneeded checks in the case of small ints
            ret = -t;
            return SafeIntNoError;
        }
        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};


template < typename T, typename E > class NegationHelper < T, E, false > // unsigned
{
public:
    static SafeIntError Negative( T t, T& ret ) throw()
    {
        _SAFEINT_UNSIGNED_NEGATION_BEHAVIOR();

#pragma warning(suppress:4127)
        _ASSERTE( !IntTraits<T>::isLT32Bit );

#pragma warning(suppress:4146)
        ret = -t;
        return SafeIntNoError;
    }
};

//core logic to determine casting behavior
enum CastMethod
{
    CastOK = 0,
    CastCheckLTZero,
    CastCheckGTMax,
    CastCheckMinMaxUnsigned,
    CastCheckMinMaxSigned,
    CastFromFloat,
    CastToBool,
    CastFromBool
};

template < typename ToType, typename FromType >
class GetCastMethod
{
public:
    enum
    {
        method =  ( IntTraits< FromType >::isBool &&
        !IntTraits< ToType >::isBool )                    ? CastFromBool :

        ( !IntTraits< FromType >::isBool &&
        IntTraits< ToType >::isBool )                     ? CastToBool :
        ( NumericType< FromType >::isFloat &&
        !NumericType< ToType >::isFloat )                 ? CastFromFloat :

        ( SafeIntCompare< ToType, FromType >::isCastOK ||
        ( NumericType< ToType >::isFloat &&
        !NumericType< FromType >::isFloat ) )          ? CastOK :

        ( ( IntTraits< ToType >::isSigned &&
        !IntTraits< FromType >::isSigned &&
        sizeof( FromType ) >= sizeof( ToType ) ) ||
        ( SafeIntCompare< ToType, FromType >::isBothUnsigned &&
        sizeof( FromType ) > sizeof( ToType ) ) )      ? CastCheckGTMax :

        ( !IntTraits< ToType >::isSigned &&
        IntTraits< FromType >::isSigned &&
        sizeof( ToType ) >= sizeof( FromType ) )          ? CastCheckLTZero :

        ( !IntTraits< ToType >::isSigned )                    ? CastCheckMinMaxUnsigned
        : CastCheckMinMaxSigned
    };
};

template < typename T, typename U, typename E,
    int Method = GetCastMethod< T, U >::method > class SafeCastHelper;

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastOK >
{
public:
    static SafeIntError Cast( U u, T& t ) throw()
    {
        t = (T)u;
        return SafeIntNoError;
    }
};

// special case floats and doubles
// tolerate loss of precision
template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastFromFloat >
{
public:
    static SafeIntError Cast( U u, T& t )
    {
        if( u <= (U)IntTraits< T >::maxInt &&
            u >= (U)IntTraits< T >::minInt )
        {
            t = (T)u;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

// Match on any method where a bool is cast to type T
template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastFromBool >
{
public:
    static SafeIntError Cast( bool b, T& t ) throw()
    {
        t = (T)( b ? 1 : 0 );
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastToBool >
{
public:
    static SafeIntError Cast( T t, bool& b ) throw()
    {
        b = !!t;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastCheckLTZero >
{
public:
    static SafeIntError Cast( U u, T& t )
    {
        if( u < 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        t = (T)u;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastCheckGTMax >
{
public:
    static SafeIntError Cast( U u, T& t )
    {
        if( u > IntTraits< T >::maxInt )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        t = (T)u;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastCheckMinMaxUnsigned >
{
public:
    static SafeIntError Cast( U u, T& t )
    {
        // U is signed - T could be either signed or unsigned
        if( u > IntTraits< T >::maxInt || u < 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        t = (T)u;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SafeCastHelper < T, U, E, CastCheckMinMaxSigned >
{
public:
    static SafeIntError Cast( U u, T& t )
    {
        // T, U are signed
        if( u > IntTraits< T >::maxInt || u < IntTraits< T >::minInt )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        t = (T)u;
        return SafeIntNoError;
    }
};

//core logic to determine whether a comparison is valid, or needs special treatment
enum ComparisonMethod
{
    ComparisonMethod_Ok = 0,
    ComparisonMethod_CastInt,
    ComparisonMethod_CastInt64,
    ComparisonMethod_UnsignedT,
    ComparisonMethod_UnsignedU
};

template < typename T, typename U >
class ValidComparison
{
public:
    enum
    {
#if _SAFEINT_USE_ANSI_CONVERSIONS
        method = ComparisonMethod_Ok
#else  /* _SAFEINT_USE_ANSI_CONVERSIONS */
        method = ( ( SafeIntCompare< T, U >::isLikeSigned )                              ? ComparisonMethod_Ok :
        ( ( IntTraits< T >::isSigned && sizeof(T) < 8 && sizeof(U) < 4 ) ||
        ( IntTraits< U >::isSigned && sizeof(T) < 4 && sizeof(U) < 8 ) )  ? ComparisonMethod_CastInt :
        ( ( IntTraits< T >::isSigned && sizeof(U) < 8 ) ||
        ( IntTraits< U >::isSigned && sizeof(T) < 8 ) )                   ? ComparisonMethod_CastInt64 :
        ( !IntTraits< T >::isSigned )                                       ? ComparisonMethod_UnsignedT :
        ComparisonMethod_UnsignedU )
#endif  /* _SAFEINT_USE_ANSI_CONVERSIONS */
    };
};

template <typename T, typename U, int Method = ValidComparison< T, U >::method > class EqualityTest;

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_Ok >
{
public:
    static bool IsEquals( const T t, const U u ) throw() { return ( t == u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_CastInt >
{
public:
    static bool IsEquals( const T t, const U u ) throw() { return ( (int)t == (int)u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_CastInt64 >
{
public:
    static bool IsEquals( const T t, const U u ) throw() { return ( (__int64)t == (__int64)u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_UnsignedT >
{
public:
    static bool IsEquals( const T t, const U u ) throw()
    {
        //one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( u < 0 )
        {
            return false;
        }

        //else safe to cast to type T
        return ( t == (T)u );
    }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_UnsignedU>
{
public:
    static bool IsEquals( const T t, const U u ) throw()
    {
        //one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( t < 0 )
        {
            return false;
        }

        //else safe to cast to type U
        return ( (U)t == u );
    }
};

template <typename T, typename U, int Method = ValidComparison< T, U >::method > class GreaterThanTest;

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_Ok >
{
public:
    static bool GreaterThan( const T t, const U u ) throw() { return ( t > u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_CastInt >
{
public:
    static bool GreaterThan( const T t, const U u ) throw() { return ( (int)t > (int)u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_CastInt64 >
{
public:
    static bool GreaterThan( const T t, const U u ) throw() { return ( (__int64)t > (__int64)u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_UnsignedT >
{
public:
    static bool GreaterThan( const T t, const U u ) throw()
    {
        // one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( u < 0 )
        {
            return SafeIntNoError;
        }

        // else safe to cast to type T
        return ( t > (T)u );
    }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_UnsignedU >
{
public:
    static bool GreaterThan( const T t, const U u ) throw()
    {
        // one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( t < 0 )
        {
            return false;
        }

        // else safe to cast to type U
        return ( (U)t > u );
    }
};

// Modulus is simpler than comparison, but follows much the same logic
// using this set of functions, it can't fail except in a div 0 situation
template <typename T, typename U, typename E, int Method = ValidComparison< T, U >::method > class ModulusHelper;

template <typename T, typename U, typename E> class ModulusHelper <T, U, E, ComparisonMethod_Ok>
{
public:
    static SafeIntError Modulus( const T& t, const U& u, T& result )
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // trap corner case
#pragma warning(suppress:4127)
        if( IntTraits< U >::isSigned )
        {
            if(u == -1)
            {
                result = 0;
                return SafeIntNoError;
            }
        }

        result = (T)(t % u);
        return SafeIntNoError;
    }
};

template <typename T, typename U, typename E> class ModulusHelper <T, U, E, ComparisonMethod_CastInt>
{
public:
    static SafeIntError Modulus( const T& t, const U& u, T& result )
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // trap corner case
#pragma warning(suppress:4127)
        if( IntTraits< U >::isSigned )
        {
            if(u == -1)
            {
                result = 0;
                return SafeIntNoError;
            }
        }

        result = (T)(t % u);
        return SafeIntNoError;
    }
};

template <typename T, typename U, typename E> class ModulusHelper <T, U, E, ComparisonMethod_CastInt64>
{
public:
    static SafeIntError Modulus( const T& t, const U& u, T& result )
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

#pragma warning(suppress:4127)
        if(IntTraits< U >::isSigned && u == -1)
        {
            result = 0;
        }
        else
        {
            result = (T)((__int64)t % (__int64)u);
        }

        return SafeIntNoError;
    }
};

// T is unsigned __int64, U is any signed int
template <typename T, typename U, typename E> class ModulusHelper <T, U, E, ComparisonMethod_UnsignedT>
{
public:
    static SafeIntError Modulus( const T& t, const U& u, T& result )
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // u could be negative - if so, need to convert to positive
        // casts below are always safe due to the way modulus works
        if(u < 0)
        {
            result = (T)(t % AbsValueHelper< U >::Abs(u));
        }
        else
        {
            result = (T)(t % u);
        }

        return SafeIntNoError;
    }
};

// U is unsigned __int64, T any signed int
template <typename T, typename U, typename E> class ModulusHelper <T, U, E, ComparisonMethod_UnsignedU>
{
public:
    static SafeIntError Modulus( const T& t, const U& u, T& result )
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        //t could be negative - if so, need to convert to positive
        if(t < 0)
        {
            result = -(T)( AbsValueHelper< T >::Abs( t ) % u );
        }
        else
        {
            result = (T)((T)t % u);
        }

        return SafeIntNoError;
    }
};

//core logic to determine method to check multiplication
enum MultiplicationState
{
    MultiplicationState_CastInt = 0,  // One or both signed, smaller than 32-bit
    MultiplicationState_CastInt64,    // One or both signed, smaller than 64-bit
    MultiplicationState_CastUint,     // Both are unsigned, smaller than 32-bit
    MultiplicationState_CastUint64,   // Both are unsigned, both 32-bit or smaller
    MultiplicationState_Uint64Uint,   // Both are unsigned, lhs 64-bit, rhs 32-bit or smaller
    MultiplicationState_Uint64Uint64, // Both are unsigned int64
    MultiplicationState_Uint64Int,    // lhs is unsigned int64, rhs int32
    MultiplicationState_Uint64Int64,  // lhs is unsigned int64, rhs signed int64
    MultiplicationState_UintUint64,   // Both are unsigned, lhs 32-bit or smaller, rhs 64-bit
    MultiplicationState_UintInt64,    // lhs unsigned 32-bit or less, rhs int64
    MultiplicationState_Int64Uint,    // lhs int64, rhs unsigned int32
    MultiplicationState_Int64Int64,   // lhs int64, rhs int64
    MultiplicationState_Int64Int,     // lhs int64, rhs int32
    MultiplicationState_IntUint64,    // lhs int, rhs unsigned int64
    MultiplicationState_IntInt64,     // lhs int, rhs int64
    MultiplicationState_Int64Uint64,  // lhs int64, rhs uint64
    MultiplicationState_Error
};

template < typename T, typename U >
class MultiplicationMethod
{
public:
    enum
    {
        // unsigned-unsigned
        method = (IntRegion< T,U >::IntZone_UintLT32_UintLT32  ? MultiplicationState_CastUint :
        (IntRegion< T,U >::IntZone_Uint32_UintLT64 ||
        IntRegion< T,U >::IntZone_UintLT32_Uint32)   ? MultiplicationState_CastUint64 :
        SafeIntCompare< T,U >::isBothUnsigned &&
        IntTraits< T >::isUint64 && IntTraits< U >::isUint64 ? MultiplicationState_Uint64Uint64 :
        (IntRegion< T,U >::IntZone_Uint64_Uint)       ? MultiplicationState_Uint64Uint :
        (IntRegion< T,U >::IntZone_UintLT64_Uint64)   ? MultiplicationState_UintUint64 :
        // unsigned-signed
        (IntRegion< T,U >::IntZone_UintLT32_IntLT32)  ? MultiplicationState_CastInt :
        (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
        IntRegion< T,U >::IntZone_UintLT32_Int32)    ? MultiplicationState_CastInt64 :
        (IntRegion< T,U >::IntZone_Uint64_Int)        ? MultiplicationState_Uint64Int :
        (IntRegion< T,U >::IntZone_UintLT64_Int64)    ? MultiplicationState_UintInt64 :
        (IntRegion< T,U >::IntZone_Uint64_Int64)      ? MultiplicationState_Uint64Int64 :
        // signed-signed
        (IntRegion< T,U >::IntZone_IntLT32_IntLT32)   ? MultiplicationState_CastInt :
        (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
        IntRegion< T,U >::IntZone_IntLT32_Int32)     ? MultiplicationState_CastInt64 :
        (IntRegion< T,U >::IntZone_Int64_Int64)       ? MultiplicationState_Int64Int64 :
        (IntRegion< T,U >::IntZone_Int64_Int)         ? MultiplicationState_Int64Int :
        (IntRegion< T,U >::IntZone_IntLT64_Int64)     ? MultiplicationState_IntInt64 :
        // signed-unsigned
        (IntRegion< T,U >::IntZone_IntLT32_UintLT32)  ? MultiplicationState_CastInt :
        (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
        IntRegion< T,U >::IntZone_IntLT64_Uint32)    ? MultiplicationState_CastInt64 :
        (IntRegion< T,U >::IntZone_Int64_UintLT64)    ? MultiplicationState_Int64Uint :
        (IntRegion< T,U >::IntZone_Int_Uint64)        ? MultiplicationState_IntUint64 :
        (IntRegion< T,U >::IntZone_Int64_Uint64       ? MultiplicationState_Int64Uint64 :
        MultiplicationState_Error ) )
    };
};

template <typename T, typename U, typename E, int Method = MultiplicationMethod< T, U >::method > class MultiplicationHelper;

template < typename T, typename U, typename E > class MultiplicationHelper< T, U, E, MultiplicationState_CastInt>
{
public:
    //accepts signed, both less than 32-bit
    static SafeIntError Multiply( const T& t, const U& u, T& ret )
    {
        int tmp = t * u;

        if( tmp > IntTraits< T >::maxInt || tmp < IntTraits< T >::minInt )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        ret = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class MultiplicationHelper< T, U, E, MultiplicationState_CastUint >
{
public:
    //accepts unsigned, both less than 32-bit
    static SafeIntError Multiply( const T& t, const U& u, T& ret )
    {
        unsigned int tmp = t * u;

        if( tmp > IntTraits< T >::maxInt )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        ret = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class MultiplicationHelper< T, U, E, MultiplicationState_CastInt64>
{
public:
    //mixed signed or both signed where at least one argument is 32-bit, and both a 32-bit or less
    static SafeIntError Multiply( const T& t, const U& u, T& ret )
    {
        __int64 tmp = (__int64)t * (__int64)u;

        if(tmp > (__int64)IntTraits< T >::maxInt || tmp < (__int64)IntTraits< T >::minInt)
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        ret = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E  > class MultiplicationHelper< T, U, E, MultiplicationState_CastUint64>
{
public:
    //both unsigned where at least one argument is 32-bit, and both are 32-bit or less
    static SafeIntError Multiply( const T& t, const U& u, T& ret )
    {
        unsigned __int64 tmp = (unsigned __int64)t * (unsigned __int64)u;

        if(tmp > (unsigned __int64)IntTraits< T >::maxInt)
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        ret = (T)tmp;
        return SafeIntNoError;
    }
};

// T = left arg and return type
// U = right arg
template < typename T, typename U, typename E > class LargeIntRegMultiply;

template< typename E > class LargeIntRegMultiply< unsigned __int64, unsigned __int64, E >
{
public:
    static SafeIntError RegMultiply( const unsigned __int64& a, const unsigned __int64& b, unsigned __int64& ret )
    {
        unsigned __int32 aHigh, aLow, bHigh, bLow;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)
        // Note - same approach applies for 128 bit math on a 64-bit system

        aHigh = (unsigned __int32)(a >> 32);
        aLow  = (unsigned __int32)a;
        bHigh = (unsigned __int32)(b >> 32);
        bLow  = (unsigned __int32)b;

        ret = 0;

        if(aHigh == 0)
        {
            if(bHigh != 0)
            {
                ret = (unsigned __int64)aLow * (unsigned __int64)bHigh;
            }
        }
        else if(bHigh == 0)
        {
            if(aHigh != 0)
            {
                ret = (unsigned __int64)aHigh * (unsigned __int64)bLow;
            }
        }
        else
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        if(ret != 0)
        {
            unsigned __int64 tmp;

            if((unsigned __int32)(ret >> 32) != 0)
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }

            ret <<= 32;
            tmp = (unsigned __int64)aLow * (unsigned __int64)bLow;
            ret += tmp;

            if(ret < tmp)
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }

            return SafeIntNoError;
        }

        ret = (unsigned __int64)aLow * (unsigned __int64)bLow;
        return SafeIntNoError;
    }
};

template< typename E > class LargeIntRegMultiply< unsigned __int64, unsigned __int32, E >
{
public:
    static SafeIntError RegMultiply( const unsigned __int64& a, unsigned __int32 b, unsigned __int64& ret )
    {
        unsigned __int32 aHigh, aLow;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * b
        // => (aHigh * b * 2^32) + (aLow * b)

        aHigh = (unsigned __int32)(a >> 32);
        aLow  = (unsigned __int32)a;

        ret = 0;

        if(aHigh != 0)
        {
            ret = (unsigned __int64)aHigh * (unsigned __int64)b;

            unsigned __int64 tmp;

            if((unsigned __int32)(ret >> 32) != 0)
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }

            ret <<= 32;
            tmp = (unsigned __int64)aLow * (unsigned __int64)b;
            ret += tmp;

            if(ret < tmp)
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }

            return SafeIntNoError;
        }

        ret = (unsigned __int64)aLow * (unsigned __int64)b;
        return SafeIntNoError;
    }
};

template< typename E > class LargeIntRegMultiply< unsigned __int64, signed __int32, E >
{
public:
    static SafeIntError RegMultiply( const unsigned __int64& a, signed __int32 b, unsigned __int64& ret )
    {
        if( b < 0 && a != 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        return LargeIntRegMultiply< unsigned __int64, unsigned __int32, E >::RegMultiply(a, (unsigned __int32)b, ret);
    }
};

template< typename E > class LargeIntRegMultiply< unsigned __int64, signed __int64, E >
{
public:
    static SafeIntError RegMultiply( const unsigned __int64& a, signed __int64 b, unsigned __int64& ret )
    {
        if( b < 0 && a != 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        return LargeIntRegMultiply< unsigned __int64, unsigned __int64, E >::RegMultiply(a, (unsigned __int64)b, ret);
    }
};

template< typename E > class LargeIntRegMultiply< signed __int32, unsigned __int64, E >
{
public:
    static SafeIntError RegMultiply( signed __int32 a, const unsigned __int64& b, signed __int32& ret )
    {
        unsigned __int32 bHigh, bLow;
        bool fIsNegative = false;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)

        bHigh = (unsigned __int32)(b >> 32);
        bLow  = (unsigned __int32)b;

        ret = 0;

        if(bHigh != 0 && a != 0)
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        if( a < 0 )
        {
            a = -a;
            fIsNegative = true;
        }

        unsigned __int64 tmp = (unsigned __int32)a * (unsigned __int64)bLow;


        if( !fIsNegative )
        {
            if( tmp <= (unsigned __int64)IntTraits< signed __int32 >::maxInt )
            {
                ret = (signed __int32)tmp;
                return SafeIntNoError;
            }
        }
        else
        {
            if( tmp <= (unsigned __int64)IntTraits< signed __int32 >::maxInt+1 )
            {
                ret = -( (signed __int32)tmp );
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class LargeIntRegMultiply< unsigned __int32, unsigned __int64, E >
{
public:
    static SafeIntError RegMultiply( unsigned __int32 a, const unsigned __int64& b, unsigned __int32& ret )
    {
        // Consider that a*b can be broken up into:
        // (bHigh * 2^32 + bLow) * a
        // => (bHigh * a * 2^32) + (bLow * a)
        // In this case, the result must fit into 32-bits
        // If bHigh != 0 && a != 0, immediate error.

        if( (unsigned __int32)(b >> 32) != 0 && a != 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        unsigned __int64 tmp = b * (unsigned __int64)a;

        if( (unsigned __int32)(tmp >> 32) != 0 ) // overflow
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        ret = (unsigned __int32)tmp;
        return SafeIntNoError;
    }
};

template < typename E > class LargeIntRegMultiply< unsigned __int32, signed __int64, E >
{
public:
    static SafeIntError RegMultiply( unsigned __int32 a, const signed __int64& b, unsigned __int32& ret )
    {
        if( b < 0 && a != 0 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        return LargeIntRegMultiply< unsigned __int32, unsigned __int64, E >::RegMultiply( a, (unsigned __int64)b, ret );
    }
};

template < typename E > class LargeIntRegMultiply< signed __int64, signed __int64, E >
{
public:
    static SafeIntError RegMultiply( const signed __int64& a, const signed __int64& b, signed __int64& ret )
    {
        bool aNegative = false;
        bool bNegative = false;

        unsigned __int64 tmp;
        __int64 a1 = a;
        __int64 b1 = b;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = -a1;
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = -b1;
        }

        if( LargeIntRegMultiply< unsigned __int64, unsigned __int64, E >::
            RegMultiply( (unsigned __int64)a1, (unsigned __int64)b1, tmp ) == SafeIntNoError )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::minInt )
                {
                    ret = -(signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::maxInt )
                {
                    ret = (signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class LargeIntRegMultiply< signed __int64, unsigned __int32, E >
{
public:
    static SafeIntError RegMultiply( const signed __int64& a, unsigned __int32 b, signed __int64& ret )
    {
        bool aNegative = false;
        unsigned __int64 tmp;
        __int64 a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = -a1;
        }

        if( LargeIntRegMultiply< unsigned __int64, unsigned __int32, E >::RegMultiply( (unsigned __int64)a1, b, tmp ) == SafeIntNoError )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative )
            {
                // Result must be negative
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::minInt )
                {
                    ret = -(signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::maxInt )
                {
                    ret = (signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class LargeIntRegMultiply< signed __int64, signed __int32, E >
{
public:
    static SafeIntError RegMultiply( const signed __int64& a, signed __int32 b, signed __int64& ret )
    {
        bool aNegative = false;
        bool bNegative = false;

        unsigned __int64 tmp;
        __int64 a1 = a;
        __int64 b1 = b;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = -a1;
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = -b1;
        }

        if( LargeIntRegMultiply< unsigned __int64, unsigned __int32, E >::
            RegMultiply( (unsigned __int64)a1, (unsigned __int32)b1, tmp ) == SafeIntNoError )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::minInt )
                {
                    ret = -(signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::maxInt )
                {
                    ret = (signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class LargeIntRegMultiply< signed __int32, signed __int64, E >
{
public:
    static SafeIntError RegMultiply( signed __int32 a, const signed __int64& b, signed __int32& ret )
    {
        bool aNegative = false;
        bool bNegative = false;

        unsigned __int32 tmp;
        __int64 b1 = b;

        if( a < 0 )
        {
            aNegative = true;
            a = -a;
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = -b1;
        }

        if( LargeIntRegMultiply< unsigned __int32, unsigned __int64, E >::
            RegMultiply( (unsigned __int32)a, (unsigned __int64)b1, tmp )  == SafeIntNoError )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (unsigned __int32)IntTraits< signed __int32 >::minInt )
                {
#pragma warning(suppress:4146)
                    ret = -tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (unsigned __int32)IntTraits< signed __int32 >::maxInt )
                {
                    ret = (signed __int32)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class LargeIntRegMultiply< signed __int64, unsigned __int64, E >
{
public:
    static SafeIntError RegMultiply( const signed __int64& a, const unsigned __int64& b, signed __int64& ret )
    {
        bool aNegative = false;

        unsigned __int64 tmp;
        __int64 a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = -a1;
        }

        if( LargeIntRegMultiply< unsigned __int64, unsigned __int64, E >::
            RegMultiply( (unsigned __int64)a1, (unsigned __int64)b, tmp )  == SafeIntNoError )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative )
            {
                // Result must be negative
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::minInt )
                {
                    ret = -((signed __int64)tmp);
                    return SafeIntNoError;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (unsigned __int64)IntTraits< signed __int64 >::maxInt )
                {
                    ret = (signed __int64)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class MultiplicationHelper< unsigned __int64, unsigned __int64, E, MultiplicationState_Uint64Uint64 >
{
public:
    static SafeIntError Multiply( const unsigned __int64& t, const unsigned __int64& u, unsigned __int64& ret )
    {
        return LargeIntRegMultiply< unsigned __int64, unsigned __int64, E >::RegMultiply( t, u, ret );
    }
};

template < typename U, typename E > class MultiplicationHelper<unsigned __int64, U, E, MultiplicationState_Uint64Uint >
{
public:
    //U is any unsigned int 32-bit or less
    static SafeIntError Multiply( const unsigned __int64& t, const U& u, unsigned __int64& ret )
    {
        return LargeIntRegMultiply< unsigned __int64, unsigned __int32, E >::RegMultiply( t, (unsigned __int32)u, ret );
    }
};

// converse of the previous function
template < typename T, typename E > class MultiplicationHelper< T, unsigned __int64, E, MultiplicationState_UintUint64 >
{
public:
    // T is any unsigned int up to 32-bit
    static SafeIntError Multiply( const T& t, const unsigned __int64& u, T& ret )
    {
        unsigned __int32 tmp;

        if( LargeIntRegMultiply< unsigned __int32, unsigned __int64, E >::RegMultiply( t, u, tmp ) == SafeIntNoError &&
            SafeCastHelper< T, unsigned __int32, E >::Cast(tmp, ret) == SafeIntNoError )
        {
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename E > class MultiplicationHelper< unsigned __int64, U, E, MultiplicationState_Uint64Int >
{
public:
    //U is any signed int, up to 64-bit
    static SafeIntError Multiply(const unsigned __int64& t, const U& u, unsigned __int64& ret)
    {
        return LargeIntRegMultiply< unsigned __int64, signed __int32, E >::RegMultiply(t, (signed __int32)u, ret);
    }
};

template < typename E > class MultiplicationHelper<unsigned __int64, __int64, E, MultiplicationState_Uint64Int64 >
{
public:
    static SafeIntError Multiply(const unsigned __int64& t, const __int64& u, unsigned __int64& ret)
    {
        return LargeIntRegMultiply< unsigned __int64, __int64, E >::RegMultiply(t, u, ret);
    }
};

template < typename T, typename E > class MultiplicationHelper< T, __int64, E, MultiplicationState_UintInt64 >
{
public:
    //T is unsigned up to 32-bit
    static SafeIntError Multiply( const T& t, const __int64& u, T& ret )
    {
        unsigned __int32 tmp;

        if( LargeIntRegMultiply< unsigned __int32, __int64, E >::RegMultiply( (unsigned __int32)t, u, tmp ) == SafeIntNoError &&
            SafeCastHelper< T, unsigned __int32, E >::Cast( tmp, ret ) == SafeIntNoError )
        {
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename E > class MultiplicationHelper<__int64, U, E, MultiplicationState_Int64Uint >
{
public:
    //U is unsigned up to 32-bit
    static SafeIntError Multiply( const __int64& t, const U& u, __int64& ret )
    {
        return LargeIntRegMultiply< __int64, unsigned __int32, E >::RegMultiply( t, (unsigned __int32)u, ret );
    }
};

template < typename E > class MultiplicationHelper<__int64, __int64, E, MultiplicationState_Int64Int64 >
{
public:
    static SafeIntError Multiply( const __int64& t, const __int64& u, __int64& ret )
    {
        return LargeIntRegMultiply< __int64, __int64, E >::RegMultiply( t, u, ret );
    }
};

template < typename U, typename E > class MultiplicationHelper<__int64, U, E, MultiplicationState_Int64Int>
{
public:
    //U is signed up to 32-bit
    static SafeIntError Multiply( const __int64& t, U u, __int64& ret )
    {
        return LargeIntRegMultiply< __int64, __int32, E >::RegMultiply( t, (__int32)u, ret );
    }
};

template < typename T, typename E > class MultiplicationHelper< T, unsigned __int64, E, MultiplicationState_IntUint64 >
{
public:
    //T is signed up to 32-bit
    static SafeIntError Multiply(T t, const unsigned __int64& u, T& ret)
    {
        __int32 tmp;

        if( LargeIntRegMultiply< __int32, unsigned __int64, E >::RegMultiply( (__int32)t, u, tmp ) == SafeIntNoError &&
            SafeCastHelper< T, __int32, E >::Cast( tmp, ret ) == SafeIntNoError )
        {
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class MultiplicationHelper<__int64, unsigned __int64, E, MultiplicationState_Int64Uint64>
{
public:
    //U is signed up to 32-bit
    static SafeIntError Multiply( const __int64& t, const unsigned __int64& u, __int64& ret )
    {
        return LargeIntRegMultiply< __int64, unsigned __int64, E >::RegMultiply( t, u, ret );
    }
};

template < typename T, typename E > class MultiplicationHelper< T, __int64, E, MultiplicationState_IntInt64>
{
public:
    //T is signed, up to 32-bit
    static SafeIntError Multiply( T t, const __int64& u, T& ret )
    {
        __int32 tmp;

        if( LargeIntRegMultiply< __int32, __int64, E >::RegMultiply( (__int32)t, u, tmp ) == SafeIntNoError &&
            SafeCastHelper< T, __int32, E >::Cast( tmp, ret ) == SafeIntNoError )
        {
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

enum DivisionState
{
    DivisionState_OK,
    DivisionState_UnsignedSigned,
    DivisionState_SignedUnsigned32,
    DivisionState_SignedUnsigned64,
    DivisionState_SignedUnsigned,
    DivisionState_SignedSigned
};

template < typename T, typename U > class DivisionMethod
{
public:
    enum
    {
        method = (SafeIntCompare< T, U >::isBothUnsigned        ? DivisionState_OK :
        (!IntTraits< T >::isSigned && IntTraits< U >::isSigned) ? DivisionState_UnsignedSigned :
        (IntTraits< T >::isSigned &&
        IntTraits< U >::isUint32 &&
        IntTraits< T >::isLT64Bit)                           ? DivisionState_SignedUnsigned32 :
        (IntTraits< T >::isSigned && IntTraits< U >::isUint64)  ? DivisionState_SignedUnsigned64 :
        (IntTraits< T >::isSigned && !IntTraits< U >::isSigned) ? DivisionState_SignedUnsigned :
        DivisionState_SignedSigned)
    };
};

template < typename T, typename U, typename E, int Method = DivisionMethod< T, U >::method > class DivisionHelper;

template < typename T, typename U, typename E > class DivisionHelper< T, U, E, DivisionState_OK >
{
public:
    static SafeIntError Divide( const T& t, const U& u, T& result )
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        result = (T)( t/u );
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class DivisionHelper< T, U, E, DivisionState_UnsignedSigned>
{
public:
    static SafeIntError Divide( const T& t, const U& u, T& result )
    {
        if( u > 0 )
        {
            result = (T)( t/u );
            return SafeIntNoError;
        }

        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // it is always an error to try and divide an unsigned number by a negative signed number
        // unless u is bigger than t
        if( AbsValueHelper< U >::Abs( u ) > t )
        {
            result = 0;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class DivisionHelper< T, U, E, DivisionState_SignedUnsigned32 >
{
public:
    static SafeIntError Divide( const T& t, const U& u, T& result )
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // Test for t > 0
        // If t < 0, must explicitly upcast, or implicit upcast to ulong will cause errors
        // As it turns out, 32-bit division is about twice as fast, which justifies the extra conditional
        if( t > 0 )
            result = (T)( t/u );
        else
            result = (T)( (__int64)t/(__int64)u );

        return SafeIntNoError;
    }
};

template < typename T, typename E > class DivisionHelper< T, unsigned __int64, E, DivisionState_SignedUnsigned64 >
{
public:
    static SafeIntError Divide( const T& t, const unsigned __int64& u, T& result )
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        if( u <= (unsigned __int64)IntTraits< T >::maxInt )
        {
            // Else u can safely be cast to T
#pragma warning(suppress:4127)
            if( sizeof( T ) < sizeof( __int64 ) )
                result = (T)( (int)t/(int)u );
            else
                result = (T)((__int64)t/(__int64)u);
        }
        else // Corner case
            if( t == IntTraits< T >::minInt && u == (unsigned __int64)IntTraits< T >::minInt )
            {
                // Min int divided by its own magnitude is -1
                result = -1;
            }
            else
            {
                result = 0;
            }
            return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class DivisionHelper< T, U, E, DivisionState_SignedUnsigned>
{
public:
    // T is any signed, U is unsigned and smaller than 32-bit
    // In this case, standard operator casting is correct
    static SafeIntError Divide( const T& t, const U& u, T& result )
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        result = (T)( t/u );
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class DivisionHelper< T, U, E, DivisionState_SignedSigned>
{
public:
    static SafeIntError Divide( const T& t, const U& u, T& result )
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
            return SafeIntDivideByZero;
        }

        // Must test for corner case
        if( t == IntTraits< T >::minInt && u == -1 )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        result = (T)( t/u );
        return SafeIntNoError;
    }
};

enum AdditionState
{
    AdditionState_CastIntCheckMax,
    AdditionState_CastUintCheckOverflow,
    AdditionState_CastUintCheckOverflowMax,
    AdditionState_CastUint64CheckOverflow,
    AdditionState_CastUint64CheckOverflowMax,
    AdditionState_CastIntCheckMinMax,
    AdditionState_CastInt64CheckMinMax,
    AdditionState_CastInt64CheckMax,
    AdditionState_CastUint64CheckMinMax,
    AdditionState_CastUint64CheckMinMax2,
    AdditionState_CastInt64CheckOverflow,
    AdditionState_CastInt64CheckOverflowMinMax,
    AdditionState_CastInt64CheckOverflowMax,
    AdditionState_ManualCheckInt64Uint64,
    AdditionState_ManualCheck,
    AdditionState_Error
};

template< typename T, typename U >
class AdditionMethod
{
public:
    enum
    {
        //unsigned-unsigned
        method = (IntRegion< T,U >::IntZone_UintLT32_UintLT32  ? AdditionState_CastIntCheckMax :
        (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ? AdditionState_CastUintCheckOverflow :
        (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ? AdditionState_CastUintCheckOverflowMax :
        (IntRegion< T,U >::IntZone_Uint64_Uint)       ? AdditionState_CastUint64CheckOverflow :
        (IntRegion< T,U >::IntZone_UintLT64_Uint64)   ? AdditionState_CastUint64CheckOverflowMax :
        //unsigned-signed
        (IntRegion< T,U >::IntZone_UintLT32_IntLT32)  ? AdditionState_CastIntCheckMinMax :
        (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
        IntRegion< T,U >::IntZone_UintLT32_Int32)    ? AdditionState_CastInt64CheckMinMax :
        (IntRegion< T,U >::IntZone_Uint64_Int ||
        IntRegion< T,U >::IntZone_Uint64_Int64)      ? AdditionState_CastUint64CheckMinMax :
        (IntRegion< T,U >::IntZone_UintLT64_Int64)    ? AdditionState_CastUint64CheckMinMax2 :
        //signed-signed
        (IntRegion< T,U >::IntZone_IntLT32_IntLT32)   ? AdditionState_CastIntCheckMinMax :
        (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
        IntRegion< T,U >::IntZone_IntLT32_Int32)     ? AdditionState_CastInt64CheckMinMax :
        (IntRegion< T,U >::IntZone_Int64_Int ||
        IntRegion< T,U >::IntZone_Int64_Int64)       ? AdditionState_CastInt64CheckOverflow :
        (IntRegion< T,U >::IntZone_IntLT64_Int64)     ? AdditionState_CastInt64CheckOverflowMinMax :
        //signed-unsigned
        (IntRegion< T,U >::IntZone_IntLT32_UintLT32)  ? AdditionState_CastIntCheckMax :
        (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
        IntRegion< T,U >::IntZone_IntLT64_Uint32)    ? AdditionState_CastInt64CheckMax :
        (IntRegion< T,U >::IntZone_Int64_UintLT64)    ? AdditionState_CastInt64CheckOverflowMax :
        (IntRegion< T,U >::IntZone_Int64_Uint64)      ? AdditionState_ManualCheckInt64Uint64 :
        (IntRegion< T,U >::IntZone_Int_Uint64)        ? AdditionState_ManualCheck :
        AdditionState_Error)
    };
};

template < typename T, typename U, typename E, int Method = AdditionMethod< T, U >::method > class AdditionHelper;

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastIntCheckMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        //16-bit or less unsigned addition
        __int32 tmp = lhs + rhs;

        if( tmp <= (__int32)IntTraits< T >::maxInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUintCheckOverflow >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // 32-bit or less - both are unsigned
        unsigned __int32 tmp = (unsigned __int32)lhs + (unsigned __int32)rhs;

        //we added didn't get smaller
        if( tmp >= lhs )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }
        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUintCheckOverflowMax>
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // 32-bit or less - both are unsigned
        unsigned __int32 tmp = (unsigned __int32)lhs + (unsigned __int32)rhs;

        // We added and it didn't get smaller or exceed maxInt
        if( tmp >= lhs && tmp <= IntTraits< T >::maxInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }
        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUint64CheckOverflow>
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // lhs unsigned __int64, rhs unsigned
        unsigned __int64 tmp = (unsigned __int64)lhs + (unsigned __int64)rhs;

        // We added and it didn't get smaller
        if(tmp >= lhs)
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUint64CheckOverflowMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        //lhs unsigned __int64, rhs unsigned
        unsigned __int64 tmp = (unsigned __int64)lhs + (unsigned __int64)rhs;

        // We added and it didn't get smaller
        if( tmp >= lhs && tmp <= IntTraits< T >::maxInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastIntCheckMinMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // 16-bit or less - one or both are signed
        __int32 tmp = lhs + rhs;

        if( tmp <= (__int32)IntTraits< T >::maxInt && tmp >= (__int32)IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

#pragma warning(push)
#pragma warning(disable:4702)
template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastInt64CheckMinMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // 32-bit or less - one or both are signed
        __int64 tmp = (__int64)lhs + (__int64)rhs;

        if( tmp <= (__int64)IntTraits< T >::maxInt && tmp >= (__int64)IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
        // return E::SafeIntOnOverflow2();
    }
};
#pragma warning(pop)

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastInt64CheckMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // 32-bit or less - lhs signed, rhs unsigned
        __int64 tmp = (__int64)lhs + (__int64)rhs;

        if( tmp <= IntTraits< T >::maxInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUint64CheckMinMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // lhs is unsigned __int64, rhs signed
        unsigned __int64 tmp;

        if( rhs < 0 )
        {
            // So we're effectively subtracting
            tmp = AbsValueHelper< U >::Abs( rhs );

            if( tmp <= lhs )
            {
                result = lhs - tmp;
                return SafeIntNoError;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an unsigned __int64
            tmp = (unsigned __int64)lhs + (unsigned __int64)rhs;

            // We added and it did not become smaller
            if( tmp >= lhs )
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastUint64CheckMinMax2>
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // lhs is unsigned and < 64-bit, rhs signed __int64
        if( rhs < 0 )
        {
            if( lhs >= (unsigned __int64)( -rhs ) )//negation is safe, since rhs is 64-bit
            {
                result = (T)( lhs + rhs );
                return SafeIntNoError;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an unsigned __int64
            unsigned __int64 tmp = (unsigned __int64)lhs + (unsigned __int64)rhs;

            // special case - rhs cannot be larger than 0x7fffffffffffffff, lhs cannot be larger than 0xffffffff
            // it is not possible for the operation above to overflow, so just check max
            if( tmp <= IntTraits< T >::maxInt )
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }
        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastInt64CheckOverflow>
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // lhs is signed __int64, rhs signed
        __int64 tmp = (__int64)lhs + (__int64)rhs;

        if( lhs >= 0 )
        {
            // mixed sign cannot overflow
            if( rhs >= 0 && tmp < lhs )
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }
        }
        else
        {
            // lhs negative
            if( rhs < 0 && tmp > lhs )
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }
        }

        result = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastInt64CheckOverflowMinMax>
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        //rhs is signed __int64, lhs signed
        __int64 tmp;

        if( AdditionHelper< __int64, __int64, E, AdditionState_CastInt64CheckOverflow >::
            Addition( (__int64)lhs, (__int64)rhs, tmp ) == SafeIntNoError &&
            tmp <= IntTraits< T >::maxInt &&
            tmp >= IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_CastInt64CheckOverflowMax >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        //lhs is signed __int64, rhs unsigned < 64-bit
        __int64 tmp = lhs + (__int64)rhs;

        if( tmp >= lhs )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class AdditionHelper < __int64, unsigned __int64, E, AdditionState_ManualCheckInt64Uint64 >
{
public:
    static SafeIntError Addition( const __int64& lhs, const unsigned __int64& rhs, __int64& result )
    {
        // rhs is unsigned __int64, lhs __int64
        __int64 tmp = lhs + (__int64)rhs;

        if( tmp >= lhs )
        {
            result = tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class AdditionHelper < T, U, E, AdditionState_ManualCheck >
{
public:
    static SafeIntError Addition( const T& lhs, const U& rhs, T& result )
    {
        // rhs is unsigned __int64, lhs signed, 32-bit or less

        if( (unsigned __int32)( rhs >> 32 ) == 0 )
        {
            // Now it just happens to work out that the standard behavior does what we want
            // Adding explicit casts to show exactly what's happening here
            __int32 tmp = (__int32)( (unsigned __int32)rhs + (unsigned __int32)lhs );

            if( tmp >= lhs &&
                SafeCastHelper< T, __int32, E >::Cast( tmp, result ) == SafeIntNoError )
                return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

enum SubtractionState
{
    SubtractionState_BothUnsigned,
    SubtractionState_CastIntCheckMinMax,
    SubtractionState_CastIntCheckMin,
    SubtractionState_CastInt64CheckMinMax,
    SubtractionState_CastInt64CheckMin,
    SubtractionState_Uint64Int,
    SubtractionState_UintInt64,
    SubtractionState_Int64Int,
    SubtractionState_IntInt64,
    SubtractionState_Int64Uint,
    SubtractionState_IntUint64,
    SubtractionState_Int64Uint64,
    // states for SubtractionMethod2
    SubtractionState_BothUnsigned2,
    SubtractionState_CastIntCheckMinMax2,
    SubtractionState_CastInt64CheckMinMax2,
    SubtractionState_Uint64Int2,
    SubtractionState_UintInt642,
    SubtractionState_Int64Int2,
    SubtractionState_IntInt642,
    SubtractionState_Int64Uint2,
    SubtractionState_IntUint642,
    SubtractionState_Int64Uint642,
    SubtractionState_Error
};

template < typename T, typename U > class SubtractionMethod
{
public:
    enum
    {
        // unsigned-unsigned
        method = ((IntRegion< T,U >::IntZone_UintLT32_UintLT32 ||
        (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ||
        (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ||
        (IntRegion< T,U >::IntZone_Uint64_Uint)       ||
        (IntRegion< T,U >::IntZone_UintLT64_Uint64))      ? SubtractionState_BothUnsigned :
        // unsigned-signed
        (IntRegion< T,U >::IntZone_UintLT32_IntLT32)      ? SubtractionState_CastIntCheckMinMax :
        (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
        IntRegion< T,U >::IntZone_UintLT32_Int32)        ? SubtractionState_CastInt64CheckMinMax :
        (IntRegion< T,U >::IntZone_Uint64_Int ||
        IntRegion< T,U >::IntZone_Uint64_Int64)          ? SubtractionState_Uint64Int :
        (IntRegion< T,U >::IntZone_UintLT64_Int64)        ? SubtractionState_UintInt64 :
        // signed-signed
        (IntRegion< T,U >::IntZone_IntLT32_IntLT32)       ? SubtractionState_CastIntCheckMinMax :
        (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
        IntRegion< T,U >::IntZone_IntLT32_Int32)         ? SubtractionState_CastInt64CheckMinMax :
        (IntRegion< T,U >::IntZone_Int64_Int ||
        IntRegion< T,U >::IntZone_Int64_Int64)           ? SubtractionState_Int64Int :
        (IntRegion< T,U >::IntZone_IntLT64_Int64)         ? SubtractionState_IntInt64 :
        // signed-unsigned
        (IntRegion< T,U >::IntZone_IntLT32_UintLT32)      ? SubtractionState_CastIntCheckMin :
        (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
        IntRegion< T,U >::IntZone_IntLT64_Uint32)        ? SubtractionState_CastInt64CheckMin :
        (IntRegion< T,U >::IntZone_Int64_UintLT64)        ? SubtractionState_Int64Uint :
        (IntRegion< T,U >::IntZone_Int_Uint64)            ? SubtractionState_IntUint64 :
        (IntRegion< T,U >::IntZone_Int64_Uint64)          ? SubtractionState_Int64Uint64 :
        SubtractionState_Error)
    };
};

// this is for the case of U - SafeInt< T, E >
template < typename T, typename U > class SubtractionMethod2
{
public:
    enum
    {
        // unsigned-unsigned
        method = ((IntRegion< T,U >::IntZone_UintLT32_UintLT32 ||
        (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ||
        (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ||
        (IntRegion< T,U >::IntZone_Uint64_Uint)       ||
        (IntRegion< T,U >::IntZone_UintLT64_Uint64))     ? SubtractionState_BothUnsigned2 :
        // unsigned-signed
        (IntRegion< T,U >::IntZone_UintLT32_IntLT32)     ? SubtractionState_CastIntCheckMinMax2 :
        (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
        IntRegion< T,U >::IntZone_UintLT32_Int32)       ? SubtractionState_CastInt64CheckMinMax2 :
        (IntRegion< T,U >::IntZone_Uint64_Int ||
        IntRegion< T,U >::IntZone_Uint64_Int64)         ? SubtractionState_Uint64Int2 :
        (IntRegion< T,U >::IntZone_UintLT64_Int64)       ? SubtractionState_UintInt642 :
        // signed-signed
        (IntRegion< T,U >::IntZone_IntLT32_IntLT32)      ? SubtractionState_CastIntCheckMinMax2 :
        (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
        IntRegion< T,U >::IntZone_IntLT32_Int32)        ? SubtractionState_CastInt64CheckMinMax2 :
        (IntRegion< T,U >::IntZone_Int64_Int ||
        IntRegion< T,U >::IntZone_Int64_Int64)          ? SubtractionState_Int64Int2 :
        (IntRegion< T,U >::IntZone_IntLT64_Int64)        ? SubtractionState_IntInt642 :
        // signed-unsigned
        (IntRegion< T,U >::IntZone_IntLT32_UintLT32)     ? SubtractionState_CastIntCheckMinMax2 :
        (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
        IntRegion< T,U >::IntZone_IntLT64_Uint32)       ? SubtractionState_CastInt64CheckMinMax2 :
        (IntRegion< T,U >::IntZone_Int64_UintLT64)       ? SubtractionState_Int64Uint2 :
        (IntRegion< T,U >::IntZone_Int_Uint64)           ? SubtractionState_IntUint642 :
        (IntRegion< T,U >::IntZone_Int64_Uint64)         ? SubtractionState_Int64Uint642 :
        SubtractionState_Error)
    };
};

template < typename T, typename U, typename E, int Method = SubtractionMethod< T, U >::method > class SubtractionHelper;

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_BothUnsigned >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // both are unsigned - easy case
        if( rhs <= lhs )
        {
            result = (T)( lhs - rhs );
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_BothUnsigned2 >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, U& result )
    {
        // both are unsigned - easy case
        // Except we do have to check for overflow - lhs could be larger than result can hold
        if( rhs <= lhs )
        {
            T tmp = (T)(lhs - rhs);
            return SafeCastHelper< U, T, E>::Cast( tmp, result);
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_CastIntCheckMinMax >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        __int32 tmp = lhs - rhs;

        if( SafeCastHelper< T, __int32, E >::Cast( tmp, result ) == SafeIntNoError )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_CastIntCheckMinMax2 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        __int32 tmp = lhs - rhs;

        return SafeCastHelper< T, __int32, E >::Cast( tmp, result );
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_CastIntCheckMin >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // both values are 16-bit or less
        // rhs is unsigned - check only minimum
        __int32 tmp = lhs - rhs;

        if( tmp >= (__int32)IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_CastInt64CheckMinMax >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        __int64 tmp = (__int64)lhs - (__int64)rhs;

        return SafeCastHelper< T, __int64, E >::Cast( tmp, result );
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_CastInt64CheckMinMax2 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        __int64 tmp = (__int64)lhs - (__int64)rhs;

        return SafeCastHelper< T, __int64, E >::Cast( tmp, result );
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_CastInt64CheckMin >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // both values are 32-bit or less
        // rhs is unsigned - check only minimum
        __int64 tmp = (__int64)lhs - (__int64)rhs;

        if( tmp >= (__int64)IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_Uint64Int >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is an unsigned __int64, rhs signed
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (unsigned __int64)rhs <= lhs )
            {
                result = (T)( lhs - (unsigned __int64)rhs );
                return SafeIntNoError;
            }
        }
        else
        {
            // we're now effectively adding
            T tmp = lhs + AbsValueHelper< U >::Abs( rhs );

            if(tmp >= lhs)
            {
                result = tmp;
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_Uint64Int2 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // U is unsigned __int64, T is signed
        if( rhs < 0 )
        {
            // treat this as addition
            unsigned __int64 tmp;

            tmp = lhs + (unsigned __int64)AbsValueHelper< T >::Abs( rhs );

            // must check for addition overflow and max
            if( tmp >= lhs && tmp <= IntTraits< T >::maxInt )
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }
        else if( (unsigned __int64)rhs > lhs ) // now both are positive, so comparison always works
        {
            // result is negative
            // implies that lhs must fit into T, and result cannot overflow
            // Also allows us to drop to 32-bit math, which is faster on a 32-bit system
            result = (T)lhs - (T)rhs;
            return SafeIntNoError;
        }
        else
        {
            // result is positive
            unsigned __int64 tmp = (unsigned __int64)lhs - (unsigned __int64)rhs;

            if( tmp <= IntTraits< T >::maxInt )
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_UintInt64 >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is an unsigned int32 or smaller, rhs signed __int64
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (unsigned __int64)rhs <= lhs )
            {
                result = (T)( lhs - (T)rhs );
                return SafeIntNoError;
            }
        }
        else
        {
            // we're now effectively adding
            // since lhs is 32-bit, and rhs cannot exceed 2^63
            // this addition cannot overflow
            unsigned __int64 tmp = lhs + (unsigned __int64)( -rhs ); // negation safe

            // but we could exceed MaxInt
            if(tmp <= IntTraits< T >::maxInt)
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_UintInt642 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // U unsigned 32-bit or less, T __int64
        if( rhs >= 0 )
        {
            // overflow not possible
            result = (T)( (__int64)lhs - rhs );
            return SafeIntNoError;
        }
        else
        {
            // we effectively have an addition
            // which cannot overflow internally
            unsigned __int64 tmp = (unsigned __int64)lhs + (unsigned __int64)( -rhs );

            if( tmp <= (unsigned __int64)IntTraits< T >::maxInt )
            {
                result = (T)tmp;
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_Int64Int >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is an __int64, rhs signed (up to 64-bit)
        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible

        __int64 tmp = lhs - rhs;

        // Note - ideally, we can order these so that true conditionals
        // lead to success, which enables better pipelining
        // It isn't practical here
        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) || // condition 2
            ( rhs >= 0 && tmp > lhs ) )             // condition 3
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
        }

        result = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_Int64Int2 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // lhs __int64, rhs any signed int (including __int64)
        __int64 tmp = lhs - rhs;

        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible in tmp
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible in tmp

        if( lhs >= 0 )
        {
            // if both positive, overflow to negative not possible
            // which is why we'll explicitly check maxInt, and not call SafeCast
#pragma warning(suppress:4127)
            if( ( IntTraits< T >::isLT64Bit && tmp > IntTraits< T >::maxInt ) ||
                ( rhs < 0 && tmp < lhs ) )
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }
        }
        else
        {
            // lhs negative
#pragma warning(suppress:4127)
            if( ( IntTraits< T >::isLT64Bit && tmp < IntTraits< T >::minInt) ||
                ( rhs >=0 && tmp > lhs ) )
            {
                E::SafeIntOnOverflow();
                return SafeIntArithmeticOverflow;
            }
        }

        result = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_IntInt64 >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is a 32-bit int or less, rhs __int64
        // we have essentially 4 cases:
        //
        // lhs positive, rhs positive - rhs could be larger than lhs can represent
        // lhs positive, rhs negative - additive case - check tmp >= lhs and tmp > max int
        // lhs negative, rhs positive - check tmp <= lhs and tmp < min int
        // lhs negative, rhs negative - addition cannot internally overflow, check against max

        __int64 tmp = (__int64)lhs - rhs;

        if( lhs >= 0 )
        {
            // first case
            if( rhs >= 0 )
            {
                if( tmp >= IntTraits< T >::minInt )
                {
                    result = (T)tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // second case
                if( tmp >= lhs && tmp <= IntTraits< T >::maxInt )
                {
                    result = (T)tmp;
                    return SafeIntNoError;
                }
            }
        }
        else
        {
            // lhs < 0
            // third case
            if( rhs >= 0 )
            {
                if( tmp <= lhs && tmp >= IntTraits< T >::minInt )
                {
                    result = (T)tmp;
                    return SafeIntNoError;
                }
            }
            else
            {
                // fourth case
                if( tmp <= IntTraits< T >::maxInt )
                {
                    result = (T)tmp;
                    return SafeIntNoError;
                }
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_IntInt642 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // lhs is any signed int32 or smaller, rhs is int64
        __int64 tmp = (__int64)lhs - rhs;

        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) ||
            ( rhs > 0 && tmp > lhs ) )
        {
            E::SafeIntOnOverflow();
            return SafeIntArithmeticOverflow;
            //else OK
        }

        result = (T)tmp;
        return SafeIntNoError;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_Int64Uint >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is a 64-bit int, rhs unsigned int32 or smaller

        __int64 tmp = lhs - (__int64)rhs;

        if( tmp <= lhs )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_Int64Uint2 >
{
public:
    // lhs is __int64, rhs is unsigned 32-bit or smaller
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        __int64 tmp = lhs - (__int64)rhs;

        if( tmp <= IntTraits< T >::maxInt && tmp >= IntTraits< T >::minInt )
        {
            result = (T)tmp;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename T, typename U, typename E > class SubtractionHelper< T, U, E, SubtractionState_IntUint64 >
{
public:
    static SafeIntError Subtract( const T& lhs, const U& rhs, T& result )
    {
        // lhs is any signed int, rhs unsigned int64
        // check against available range

        // We need the absolute value of IntTraits< T >::minInt
        // This will give it to us without extraneous compiler warnings
        const unsigned __int64 AbsMinIntT = (unsigned __int64)IntTraits< T >::maxInt + 1;

        if( lhs < 0 )
        {
            if( rhs <= AbsMinIntT - AbsValueHelper< T >::Abs( lhs ) )
            {
                result = (T)( lhs - rhs );
                return SafeIntNoError;
            }
        }
        else
        {
            if( rhs <= AbsMinIntT + (unsigned __int64)lhs )
            {
                result = (T)( lhs - rhs );
                return SafeIntNoError;
            }
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename U, typename T, typename E > class SubtractionHelper< U, T, E, SubtractionState_IntUint642 >
{
public:
    static SafeIntError Subtract( const U& lhs, const T& rhs, T& result )
    {
        // We run into upcasting problems on comparison - needs 2 checks
        if( lhs >= 0 && (T)lhs >= rhs )
        {
            result = (T)((U)lhs - (U)rhs);
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class SubtractionHelper< __int64, unsigned __int64, E, SubtractionState_Int64Uint64 >
{
public:
    static SafeIntError Subtract( const __int64& lhs, const unsigned __int64& rhs, __int64& result )
    {
        // if we subtract, and it gets larger, there's a problem
        __int64 tmp = lhs - (__int64)rhs;

        if( tmp <= lhs )
        {
            result = tmp;
            return SafeIntNoError;
        }
        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

template < typename E > class SubtractionHelper< __int64, unsigned __int64, E, SubtractionState_Int64Uint642 >
{
public:
    // If lhs is negative, immediate problem - return must be positive, and subtracting only makes it
    // get smaller. If rhs > lhs, then it would also go negative, which is the other case
    static SafeIntError Subtract( const __int64& lhs, const unsigned __int64& rhs, unsigned __int64& result )
    {
        if( lhs >= 0 && (unsigned __int64)lhs >= rhs )
        {
            result = (unsigned __int64)lhs - rhs;
            return SafeIntNoError;
        }

        E::SafeIntOnOverflow();
        return SafeIntArithmeticOverflow;
    }
};

enum BinaryState
{
    BinaryState_OK,
    BinaryState_Int8,
    BinaryState_Int16,
    BinaryState_Int32
};

template < typename T, typename U > class BinaryMethod
{
public:
    enum
    {
        // If both operands are unsigned OR
        //    return type is smaller than rhs OR
        //    return type is larger and rhs is unsigned
        // Then binary operations won't produce unexpected results
        method = ( sizeof( T ) <= sizeof( U ) ||
        SafeIntCompare< T, U >::isBothUnsigned ||
        !IntTraits< U >::isSigned )          ? BinaryState_OK :
        IntTraits< U >::isInt8               ? BinaryState_Int8 :
        IntTraits< U >::isInt16              ? BinaryState_Int16
        : BinaryState_Int32
    };
};

template < typename T, typename U, int Method = BinaryMethod< T, U >::method > class BinaryAndHelper;

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_OK >
{
public:
    static T And( T lhs, U rhs ){ return (T)( lhs & rhs ); }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int8 >
{
public:
    static T And( T lhs, U rhs )
    {
        // cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs & rhs ) == ( lhs & (unsigned __int8)rhs ) );
        return (T)( lhs & (unsigned __int8)rhs );
    }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int16 >
{
public:
    static T And( T lhs, U rhs )
    {
        //cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs & rhs ) == ( lhs & (unsigned __int16)rhs ) );
        return (T)( lhs & (unsigned __int16)rhs );
    }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int32 >
{
public:
    static T And( T lhs, U rhs )
    {
        //cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs & rhs ) == ( lhs & (unsigned __int32)rhs ) );
        return (T)( lhs & (unsigned __int32)rhs );
    }
};

template < typename T, typename U, int Method = BinaryMethod< T, U >::method > class BinaryOrHelper;

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_OK >
{
public:
    static T Or( T lhs, U rhs ){ return (T)( lhs | rhs ); }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int8 >
{
public:
    static T Or( T lhs, U rhs )
    {
        //cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs | rhs ) == ( lhs | (unsigned __int8)rhs ) );
        return (T)( lhs | (unsigned __int8)rhs );
    }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int16 >
{
public:
    static T Or( T lhs, U rhs )
    {
        //cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs | rhs ) == ( lhs | (unsigned __int16)rhs ) );
        return (T)( lhs | (unsigned __int16)rhs );
    }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int32 >
{
public:
    static T Or( T lhs, U rhs )
    {
        //cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs | rhs ) == ( lhs | (unsigned __int32)rhs ) );
        return (T)( lhs | (unsigned __int32)rhs );
    }
};

template <typename T, typename U, int Method = BinaryMethod< T, U >::method > class BinaryXorHelper;

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_OK >
{
public:
    static T Xor( T lhs, U rhs ){ return (T)( lhs ^ rhs ); }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int8 >
{
public:
    static T Xor( T lhs, U rhs )
    {
        // cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs ^ rhs ) == ( lhs ^ (unsigned __int8)rhs ) );
        return (T)( lhs ^ (unsigned __int8)rhs );
    }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int16 >
{
public:
    static T Xor( T lhs, U rhs )
    {
        // cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs ^ rhs ) == ( lhs ^ (unsigned __int16)rhs ) );
        return (T)( lhs ^ (unsigned __int16)rhs );
    }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int32 >
{
public:
    static T Xor( T lhs, U rhs )
    {
        // cast forces sign extension to be zeros
        _SAFEINT_BINARY_ASSERT( ( lhs ^ rhs ) == ( lhs ^ (unsigned __int32)rhs ) );
        return (T)( lhs ^ (unsigned __int32)rhs );
    }
};

#pragma warning(pop)
} // namespace details

} // namespace utilities

} // namespace msl

#pragma pack(pop)
