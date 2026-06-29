/***
*safeint.h - SafeInt class and free-standing functions used to prevent arithmetic overflows
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*       The SafeInt class is designed to have as low an overhead as possible
*       while still ensuring that all integer operations are conducted safely.
*       Nearly every operator has been overloaded, with a very few exceptions.
*
*       A usability-safety trade-off has been made to help ensure safety. This
*       requires that every operation return either a SafeInt or a bool. If we
*       allowed an operator to return a base integer type T, then the following
*       can happen:
*
*       char i = SafeInt<char>(32) * 2 + SafeInt<char>(16) * 4;
*
*       The * operators take precedence, get overloaded, return a char, and then
*       you have:
*
*       char i = (char)64 + (char)64; //overflow!
*
*       This situation would mean that safety would depend on usage, which isn't
*       acceptable.
*
*       One key operator that is missing is an implicit cast to type T. The reason for
*       this is that if there is an implicit cast operator, then we end up with
*       an ambiguous compile-time precedence. Because of this amiguity, there
*       are two methods that are provided:
*
*       Casting operators for every native integer type
*
*       SafeInt::Ptr()   - returns the address of the internal integer
*
*       The SafeInt class should be used in any circumstances where ensuring
*       integrity of the calculations is more important than performance. See Performance
*       Notes below for additional information.
*
*       Many of the conditionals will optimize out or be inlined for a release
*       build (especially with /Ox), but it does have significantly more overhead,
*       especially for signed numbers. If you do not _require_ negative numbers, use
*       unsigned integer types - certain types of problems cannot occur, and this class
*       performs most efficiently.
*
*       Here's an example of when the class should ideally be used -
*
*       void* AllocateMemForStructs(int StructSize, int HowMany)
*       {
*          SafeInt<unsigned long> s(StructSize);
*
*          s *= HowMany;
*
*          return malloc(s);
*
*       }
*
*       Here's when it should NOT be used:
*
*       void foo()
*       {
*         int i;
*
*         for(i = 0; i < 0xffff; i++)
*           ....
*       }
*
*       Error handling - a SafeInt class will throw exceptions if something
*       objectionable happens. The exceptions are SafeIntException classes,
*       which contain an enum as a code.
*
*       Typical usage might be:
*
*       bool foo()
*       {
*         SafeInt<unsigned long> s; //note that s == 0 unless set
*
*         try{
*           s *= 23;
*           ....
*         }
*         catch(SafeIntException err)
*         {
*            //handle errors here
*         }
*       }
*
*       SafeInt accepts an error policy as an optional template parameter.
*       We provide two error policy along with SafeInt: SafeIntErrorPolicy_SafeIntException, which
*       throws SafeIntException in case of error, and SafeIntErrorPolicy_InvalidParameter, which
*       calls _invalid_parameter to terminate the program.
*
*       You can replace the error policy class with any class you like. This is accomplished by:
*       1) Create a class that has the following interface:
*
*         struct YourSafeIntErrorPolicy
*         {
*             static __declspec(noreturn) void __stdcall SafeIntOnOverflow()
*             {
*                 throw YourException( YourSafeIntArithmeticOverflowError );
*                 // or do something else which will terminate the program
*             }
*
*             static __declspec(noreturn) void __stdcall SafeIntOnDivZero()
*             {
*                 throw YourException( YourSafeIntDivideByZeroError );
*                 // or do something else which will terminate the program
*             }
*         };
*
*       Note that you don't have to throw C++ exceptions, you can throw Win32 exceptions, or do
*       anything you like, just don't return from the call back into the code.
*
*       2) Either explicitly declare SafeInts like so:
*          SafeInt< int, YourSafeIntErrorPolicy > si;
*       or, before including SafeInt:
*          #define _SAFEINT_DEFAULT_ERROR_POLICY ::YourSafeIntErrorPolicy
*
*       Performance:
*
*       Due to the highly nested nature of this class, you can expect relatively poor
*       performance in unoptimized code. In tests of optimized code vs. correct inline checks
*       in native code, this class has been found to take approximately 8% more CPU time (this varies),
*       most of which is due to exception handling.
*
*       Binary Operators:
*
*       All of the binary operators have certain assumptions built into the class design.
*       This is to ensure correctness. Notes on each class of operator follow:
*
*       Arithmetic Operators (*,/,+,-,%)
*       There are three possible variants:
*       SafeInt< T, E > op SafeInt< T, E >
*       SafeInt< T, E > op U
*       U op SafeInt< T, E >
*
*       The SafeInt< T, E > op SafeInt< U, E > variant is explicitly not supported, and if you try to do
*       this the compiler with throw the following error:
*
*       error C2593: 'operator *' is ambiguous
*
*       This is because the arithmetic operators are required to return a SafeInt of some type.
*       The compiler cannot know whether you'd prefer to get a type T or a type U returned. If
*       you need to do this, you need to extract the value contained within one of the two using
*       the casting operator. For example:
*
*       SafeInt< T, E > t, result;
*       SafeInt< U, E > u;
*
*       result = t * (U)u;
*
*       Comparison Operators:
*
*       Because each of these operators return type bool, mixing SafeInts of differing types is
*       allowed.
*
*       Shift Operators:
*
*       Shift operators always return the type on the left hand side of the operator. Mixed type
*       operations are allowed because the return type is always known.
*
*       Boolean Operators:
*
*       Like comparison operators, these overloads always return type bool, and mixed-type SafeInts
*       are allowed. Additionally, specific overloads exist for type bool on both sides of the
*       operator.
*
*       Binary Operators:
*
*       Mixed-type operations are discouraged, however some provision has been made in order to
*       enable things like:
*
*       SafeInt<char> c = 2;
*
*       if(c & 0x02)
*         ...
*
*       The "0x02" is actually an int, and it needs to work.
*       In the case of binary operations on integers smaller than 32-bit, or of mixed type, corner
*       cases do exist where you could get unexpected results. In any case where SafeInt returns a different
*       result than the underlying operator, it will call _ASSERTE(). You should examine your code and cast things
*       properly so that you are not programming with side effects.
*
*       Comparison Operators and ANSI Conversions:
*
*       The comparison operator behavior in this class varies from the ANSI definition.
*       As an example, consider the following:
*
*       unsigned int l = 0xffffffff;
*       char c = -1;
*
*       if(c == l)
*         printf("Why is -1 equal to 4 billion???\n");
*
*       The problem here is that c gets cast to an int, now has a value of 0xffffffff, and then gets
*       cast again to an unsigned int, losing the true value. This behavior is despite the fact that
*       an __int64 exists, and the following code will yield a different (and intuitively correct)
*       answer:
*
*       if((__int64)c == (__int64)l))
*         printf("Why is -1 equal to 4 billion???\n");
*       else
*         printf("Why doesn't the compiler upcast to 64-bits when needed?\n");
*
*       Note that combinations with smaller integers won't display the problem - if you
*       changed "unsigned int" above to "unsigned short", you'd get the right answer.
*
*       If you prefer to retain the ANSI standard behavior insert, before including safeint.h:
*
*       #define _SAFEINT_ANSI_CONVERSIONS 1
*
*       into your source. Behavior differences occur in the following cases:
*       8, 16, and 32-bit signed int, unsigned 32-bit int
*       any signed int, unsigned 64-bit int
*       Note - the signed int must be negative to show the problem
*
****/

#pragma once

#if !defined(RC_INVOKED)

#include <corecrt.h>
#include <crtdbg.h>

// Disable warnings hit under /Wall:
// C4514: unreferenced inline function has been removed (/Wall)
// C4710: function not inlined (/Wall)
#pragma warning(push)
#pragma warning(disable: 4514)
#pragma warning(disable: 4710)

#if !defined (_SAFEINT_DEFAULT_ERROR_POLICY)
#define _SAFEINT_DEFAULT_ERROR_POLICY SafeIntErrorPolicy_SafeIntException
#endif  /* !defined (_SAFEINT_DEFAULT_ERROR_POLICY) */

#if !defined (_SAFEINT_SHIFT_ASSERT)
#define _SAFEINT_SHIFT_ASSERT(x) _ASSERTE(x)
#endif  /* !defined (_SAFEINT_SHIFT_ASSERT) */

#if !defined (_SAFEINT_BINARY_ASSERT)
#define _SAFEINT_BINARY_ASSERT(x) _ASSERTE(x)
#endif  /* !defined (_SAFEINT_BINARY_ASSERT) */

#if !defined (_SAFEINT_EXCEPTION_ASSERT)
#define _SAFEINT_EXCEPTION_ASSERT()
#endif  /* !defined (_SAFEINT_EXCEPTION_ASSERT) */

// by default, SafeInt will accept negation of an unsigned int;
// if you wish to disable it or assert, you can define the following
// macro to be a static assert or a runtime assert
#if !defined (_SAFEINT_UNSIGNED_NEGATION_BEHAVIOR)
#define _SAFEINT_UNSIGNED_NEGATION_BEHAVIOR()
#endif  /* !defined (_SAFEINT_UNSIGNED_NEGATION_BEHAVIOR) */

// See above "Comparison Operators and ANSI Conversions" for an explanation
// of _SAFEINT_USE_ANSI_CONVERSIONS
#if !defined (_SAFEINT_USE_ANSI_CONVERSIONS)
#define _SAFEINT_USE_ANSI_CONVERSIONS 0
#endif  /* !defined (_SAFEINT_USE_ANSI_CONVERSIONS) */

#pragma pack(push, _CRT_PACKING)

namespace msl
{

namespace utilities
{

enum SafeIntError
{
    SafeIntNoError = 0,
    SafeIntArithmeticOverflow,
    SafeIntDivideByZero
};

} // namespace utilities

} // namespace msl

#include "safeint_internal.h"

namespace msl
{

namespace utilities
{

class SafeIntException
{
public:
    SafeIntException() { m_code = SafeIntNoError; }
    SafeIntException( SafeIntError code )
    {
        m_code = code;
    }
    SafeIntError m_code;
};

struct SafeIntErrorPolicy_SafeIntException
{
    static __declspec(noreturn) void SafeIntOnOverflow()
    {
        _SAFEINT_EXCEPTION_ASSERT();
        throw SafeIntException( SafeIntArithmeticOverflow );
    }

    static __declspec(noreturn) void SafeIntOnDivZero()
    {
        _SAFEINT_EXCEPTION_ASSERT();
        throw SafeIntException( SafeIntDivideByZero );
    }
};

struct SafeIntErrorPolicy_InvalidParameter
{
    static __declspec(noreturn) void SafeIntOnOverflow()
    {
        _SAFEINT_EXCEPTION_ASSERT();
        _CRT_SECURE_INVALID_PARAMETER("SafeInt Arithmetic Overflow");
    }

    static __declspec(noreturn) void SafeIntOnDivZero()
    {
        _SAFEINT_EXCEPTION_ASSERT();
        _CRT_SECURE_INVALID_PARAMETER("SafeInt Divide By Zero");
    }
};

// Free-standing functions that can be used where you only need to check one operation
// non-class helper function so that you can check for a cast's validity
// and handle errors how you like

template < typename T, typename U >
inline bool SafeCast( const T From, U& To ) throw()
{
    return (details::SafeCastHelper< U, T,
        details::SafeIntErrorPolicy_NoThrow >::Cast( From, To ) == SafeIntNoError);
}

template < typename T, typename U >
inline bool SafeEquals( const T t, const U u ) throw()
{
    return details::EqualityTest< T, U >::IsEquals( t, u );
}

template < typename T, typename U >
inline bool SafeNotEquals( const T t, const U u ) throw()
{
    return !details::EqualityTest< T, U >::IsEquals( t, u );
}

template < typename T, typename U >
inline bool SafeGreaterThan( const T t, const U u ) throw()
{
    return details::GreaterThanTest< T, U >::GreaterThan( t, u );
}

template < typename T, typename U >
inline bool SafeGreaterThanEquals( const T t, const U u ) throw()
{
    return !details::GreaterThanTest< U, T >::GreaterThan( u, t );
}

template < typename T, typename U >
inline bool SafeLessThan( const T t, const U u ) throw()
{
    return details::GreaterThanTest< U, T >::GreaterThan( u, t );
}

template < typename T, typename U >
inline bool SafeLessThanEquals( const T t, const U u ) throw()
{
    return !details::GreaterThanTest< T, U >::GreaterThan( t, u );
}

template < typename T, typename U >
inline bool SafeModulus( const T& t, const U& u, T& result ) throw()
{
    return ( details::ModulusHelper< T, U, details::SafeIntErrorPolicy_NoThrow >::Modulus( t, u, result ) == SafeIntNoError );
}

template < typename T, typename U >
inline bool SafeMultiply( T t, U u, T& result ) throw()
{
    return ( details::MultiplicationHelper< T, U,
        details::SafeIntErrorPolicy_NoThrow >::Multiply( t, u, result ) == SafeIntNoError );
}

template < typename T, typename U >
inline bool SafeDivide( T t, U u, T& result ) throw()
{
    return ( details::DivisionHelper< T, U,
        details::SafeIntErrorPolicy_NoThrow >::Divide( t, u, result ) == SafeIntNoError );
}

template < typename T, typename U >
inline bool SafeAdd( T t, U u, T& result ) throw()
{
    return ( details::AdditionHelper< T, U,
        details::SafeIntErrorPolicy_NoThrow >::Addition( t, u, result ) == SafeIntNoError );
}

template < typename T, typename U >
inline bool SafeSubtract( T t, U u, T& result ) throw()
{
    return ( details::SubtractionHelper< T, U,
        details::SafeIntErrorPolicy_NoThrow >::Subtract( t, u, result ) == SafeIntNoError );
}

// SafeInt class
template < typename T, typename E = _SAFEINT_DEFAULT_ERROR_POLICY >
class SafeInt
{
public:
    SafeInt() throw()
    {
        static_assert( details::NumericType< T >::isInt , "SafeInt<T>: T needs to be an integer type" );
        m_int = 0;
    }

    // Having a constructor for every type of int
    // avoids having the compiler evade our checks when doing implicit casts -
    // e.g., SafeInt<char> s = 0x7fffffff;
    SafeInt( const T& i ) throw()
    {
        static_assert( details::NumericType< T >::isInt , "SafeInt<T>: T needs to be an integer type" );
        //always safe
        m_int = i;
    }

    // provide explicit boolean converter
    SafeInt( bool b ) throw()
    {
        static_assert( details::NumericType< T >::isInt , "SafeInt<T>: T needs to be an integer type" );
        m_int = b ? 1 : 0;
    }

    template < typename U >
    SafeInt(const SafeInt< U, E >& u)
    {
        static_assert( details::NumericType< T >::isInt , "SafeInt<T>: T needs to be an integer type" );
        *this = SafeInt< T, E >( (U)u );
    }

    template < typename U >
    SafeInt( const U& i )
    {
        static_assert( details::NumericType< T >::isInt , "SafeInt<T>: T needs to be an integer type" );
        // SafeCast will throw exceptions if i won't fit in type T
        details::SafeCastHelper< T, U, E >::Cast( i, m_int );
    }

    // now start overloading operators
    // assignment operator
    // constructors exist for all int types and will ensure safety

    template < typename U >
    SafeInt< T, E >& operator =( const U& rhs )
    {
        // use constructor to test size
        // constructor is optimized to do minimal checking based
        // on whether T can contain U
        // note - do not change this
        *this = SafeInt< T, E >( rhs );
        return *this;
    }

    SafeInt< T, E >& operator =( const T& rhs ) throw()
    {
        m_int = rhs;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator =( const SafeInt< U, E >& rhs )
    {
        details::SafeCastHelper< T, U, E >::Cast( rhs.Ref(), m_int );
        return *this;
    }

    SafeInt< T, E >& operator =( const SafeInt< T, E >& rhs ) throw()
    {
        m_int = rhs.m_int;
        return *this;
    }

    // Casting operators

    operator bool() const throw()
    {
        return !!m_int;
    }

    operator char() const
    {
        char val;
        details::SafeCastHelper< char, T, E >::Cast( m_int, val );
        return val;
    }

    operator signed char() const
    {
        signed char val;
        details::SafeCastHelper< signed char, T, E >::Cast( m_int, val );
        return val;
    }

    operator unsigned char() const
    {
        unsigned char val;
        details::SafeCastHelper< unsigned char, T, E >::Cast( m_int, val );
        return val;
    }

    operator __int16() const
    {
        __int16 val;
        details::SafeCastHelper< __int16, T, E >::Cast( m_int, val );
        return val;
    }

    operator unsigned __int16() const
    {
        unsigned __int16 val;
        details::SafeCastHelper< unsigned __int16, T, E >::Cast( m_int, val );
        return val;
    }

    operator __int32() const
    {
        __int32 val;
        details::SafeCastHelper< __int32, T, E >::Cast( m_int, val );
        return val;
    }

    operator unsigned __int32() const
    {
        unsigned __int32 val;
        details::SafeCastHelper< unsigned __int32, T, E >::Cast( m_int, val );
        return val;
    }

    // The compiler knows that int == __int32
    // but not that long == __int32
    operator long() const
    {
        long val;
        details::SafeCastHelper< long, T, E >::Cast( m_int, val );
        return  val;
    }

    operator unsigned long() const
    {
        unsigned long val;
        details::SafeCastHelper< unsigned long, T, E >::Cast( m_int, val );
        return val;
    }

    operator __int64() const
    {
        __int64 val;
        details::SafeCastHelper< __int64, T, E >::Cast( m_int, val );
        return val;
    }

    operator unsigned __int64() const
    {
        unsigned __int64 val;
        details::SafeCastHelper< unsigned __int64, T, E >::Cast( m_int, val );
        return val;
    }

#ifdef _NATIVE_WCHAR_T_DEFINED
    operator wchar_t() const
    {
        unsigned __int16 val;
        details::SafeCastHelper< unsigned __int16, T, E >::Cast( m_int, val );
        return val;
    }
#endif  /* _NATIVE_WCHAR_T_DEFINED */

    // If you need a pointer to the data
    // this could be dangerous, but allows you to correctly pass
    // instances of this class to APIs that take a pointer to an integer
    // also see overloaded address-of operator below
    T* Ptr() throw() { return &m_int; }
    const T* Ptr() const throw() { return &m_int; }
    const T& Ref() const throw() { return m_int; }

    // Unary operators
    bool operator !() const throw() { return (!m_int) ? true : false; }

    // operator + (unary)
    // note - normally, the '+' and '-' operators will upcast to a signed int
    // for T < 32 bits. This class changes behavior to preserve type
    const SafeInt< T, E >& operator +() const throw() { return *this; };

    //unary  -
    SafeInt< T, E > operator -() const
    {
        // Note - unsigned still performs the bitwise manipulation
        // will warn at level 2 or higher if the value is 32-bit or larger
        T tmp;
        details::NegationHelper< T, E, details::IntTraits< T >::isSigned >::Negative( m_int, tmp );
        return SafeInt< T, E >( tmp );
    }

    // prefix increment operator
    SafeInt< T, E >& operator ++()
    {
        if( m_int != details::IntTraits< T >::maxInt )
        {
            ++m_int;
            return *this;
        }
        E::SafeIntOnOverflow();
    }

    // prefix decrement operator
    SafeInt< T, E >& operator --()
    {
        if( m_int != details::IntTraits< T >::minInt )
        {
            --m_int;
            return *this;
        }
        E::SafeIntOnOverflow();
    }

    // note that postfix operators have inherently worse perf
    // characteristics

    // postfix increment operator
    SafeInt< T, E > operator ++( int ) // dummy arg to comply with spec
    {
        if( m_int != details::IntTraits< T >::maxInt )
        {
            SafeInt< T, E > tmp( m_int );

            m_int++;
            return tmp;
        }
        E::SafeIntOnOverflow();
    }

    // postfix decrement operator
    SafeInt< T, E > operator --( int ) // dummy arg to comply with spec
    {
        if( m_int != details::IntTraits< T >::minInt )
        {
            SafeInt< T, E > tmp( m_int );
            m_int--;
            return tmp;
        }
        E::SafeIntOnOverflow();
    }

    // One's complement
    // Note - this operator will normally change size to an int
    // cast in return improves perf and maintains type
    SafeInt< T, E > operator ~() const throw() { return SafeInt< T, E >( (T)~m_int ); }

    // Binary operators
    //
    // arithmetic binary operators
    // % modulus
    // * multiplication
    // / division
    // + addition
    // - subtraction
    //
    // For each of the arithmetic operators, you will need to
    // use them as follows:
    //
    // SafeInt<char> c = 2;
    // SafeInt<int>  i = 3;
    //
    // SafeInt<int> i2 = i op (char)c;
    // OR
    // SafeInt<char> i2 = (int)i op c;
    //
    // The base problem is that if the lhs and rhs inputs are different SafeInt types
    // it is not possible in this implementation to determine what type of SafeInt
    // should be returned. You have to let the class know which of the two inputs
    // need to be the return type by forcing the other value to the base integer type.
    //
    // Note - as per feedback from Scott Meyers, I'm exploring how to get around this.
    // 3.0 update - I'm still thinking about this. It can be done with template metaprogramming,
    // but it is tricky, and there's a perf vs. correctness tradeoff where the right answer
    // is situational.
    //
    // The case of:
    //
    // SafeInt< T, E > i, j, k;
    // i = j op k;
    //
    // works just fine and no unboxing is needed because the return type is not ambiguous.

    // Modulus
    // Modulus has some convenient properties -
    // first, the magnitude of the return can never be
    // larger than the lhs operand, and it must be the same sign
    // as well. It does, however, suffer from the same promotion
    // problems as comparisons, division and other operations
    template < typename U >
    SafeInt< T, E > operator %( U rhs ) const
    {
        T result;
        details::ModulusHelper< T, U, E >::Modulus( m_int, rhs, result );
        return SafeInt< T, E >( result );
    }

    SafeInt< T, E > operator %( SafeInt< T, E > rhs ) const
    {
        T result;
        details::ModulusHelper< T, T, E >::Modulus( m_int, rhs, result );
        return SafeInt< T, E >( result );
    }

    // Modulus assignment
    template < typename U >
    SafeInt< T, E >& operator %=( U rhs )
    {
        details::ModulusHelper< T, U, E >::Modulus( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator %=( SafeInt< U, E > rhs )
    {
        details::ModulusHelper< T, U, E >::Modulus( m_int, (U)rhs, m_int );
        return *this;
    }

    // Multiplication
    template < typename U >
    SafeInt< T, E > operator *( U rhs ) const
    {
        T ret( 0 );
        details::MultiplicationHelper< T, U, E >::Multiply( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    SafeInt< T, E > operator *( SafeInt< T, E > rhs ) const
    {
        T ret( 0 );
        details::MultiplicationHelper< T, T, E >::Multiply( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Multiplication assignment
    SafeInt< T, E >& operator *=( SafeInt< T, E > rhs )
    {
        details::MultiplicationHelper< T, T, E >::Multiply( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator *=( U rhs )
    {
        details::MultiplicationHelper< T, U, E >::Multiply( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator *=( SafeInt< U, E > rhs )
    {
        details::MultiplicationHelper< T, U, E >::Multiply( m_int, rhs.Ref(), m_int );
        return *this;
    }

    // Division
    template < typename U >
    SafeInt< T, E > operator /( U rhs ) const
    {
        T ret( 0 );
        details::DivisionHelper< T, U, E >::Divide( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    SafeInt< T, E > operator /( SafeInt< T, E > rhs ) const
    {
        T ret( 0 );
        details::DivisionHelper< T, T, E >::Divide( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Division assignment
    SafeInt< T, E >& operator /=( SafeInt< T, E > i )
    {
        details::DivisionHelper< T, T, E >::Divide( m_int, (T)i, m_int );
        return *this;
    }

    template < typename U > SafeInt< T, E >& operator /=( U i )
    {
        details::DivisionHelper< T, U, E >::Divide( m_int, i, m_int );
        return *this;
    }

    template < typename U > SafeInt< T, E >& operator /=( SafeInt< U, E > i )
    {
        details::DivisionHelper< T, U, E >::Divide( m_int, (U)i, m_int );
        return *this;
    }

    // For addition and subtraction

    // Addition
    SafeInt< T, E > operator +( SafeInt< T, E > rhs ) const
    {
        T ret( 0 );
        details::AdditionHelper< T, T, E >::Addition( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    template < typename U >
    SafeInt< T, E > operator +( U rhs ) const
    {
        T ret( 0 );
        details::AdditionHelper< T, U, E >::Addition( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    //addition assignment
    SafeInt< T, E >& operator +=( SafeInt< T, E > rhs )
    {
        details::AdditionHelper< T, T, E >::Addition( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator +=( U rhs )
    {
        details::AdditionHelper< T, U, E >::Addition( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator +=( SafeInt< U, E > rhs )
    {
        details::AdditionHelper< T, U, E >::Addition( m_int, (U)rhs, m_int );
        return *this;
    }

    // Subtraction
    template < typename U >
    SafeInt< T, E > operator -( U rhs ) const
    {
        T ret( 0 );
        details::SubtractionHelper< T, U, E >::Subtract( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    SafeInt< T, E > operator -(SafeInt< T, E > rhs) const
    {
        T ret( 0 );
        details::SubtractionHelper< T, T, E >::Subtract( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Subtraction assignment
    SafeInt< T, E >& operator -=( SafeInt< T, E > rhs )
    {
        details::SubtractionHelper< T, T, E >::Subtract( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator -=( U rhs )
    {
        details::SubtractionHelper< T, U, E >::Subtract( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator -=( SafeInt< U, E > rhs )
    {
        details::SubtractionHelper< T, U, E >::Subtract( m_int, (U)rhs, m_int );
        return *this;
    }

    // Comparison operators
    // Additional overloads defined outside the class
    // to allow for cases where the SafeInt is the rhs value

    // Less than
    template < typename U >
    bool operator <( U rhs ) const throw()
    {
        return details::GreaterThanTest< U, T >::GreaterThan( rhs, m_int );
    }

    bool operator <( SafeInt< T, E > rhs ) const throw()
    {
        return m_int < (T)rhs;
    }

    // Greater than or eq.
    template < typename U >
    bool operator >=( U rhs ) const throw()
    {
        return !details::GreaterThanTest< U, T >::GreaterThan( rhs, m_int );
    }

    bool operator >=( SafeInt< T, E > rhs ) const throw()
    {
        return m_int >= (T)rhs;
    }

    // Greater than
    template < typename U >
    bool operator >( U rhs ) const throw()
    {
        return details::GreaterThanTest< T, U >::GreaterThan( m_int, rhs );
    }

    bool operator >( SafeInt< T, E > rhs ) const throw()
    {
        return m_int > (T)rhs;
    }

    // Less than or eq.
    template < typename U >
    bool operator <=( U rhs ) const throw()
    {
        return !details::GreaterThanTest< T, U >::GreaterThan( m_int, rhs );
    }

    bool operator <=( SafeInt< T, E > rhs ) const throw()
    {
        return m_int <= (T)rhs;
    }

    // Equality
    template < typename U >
    bool operator ==( U rhs ) const throw()
    {
        return details::EqualityTest< T, U >::IsEquals( m_int, rhs );
    }

    // Need an explicit override for type bool
    bool operator ==( bool rhs ) const throw()
    {
        return ( m_int == 0 ? false : true ) == rhs;
    }

    bool operator ==( SafeInt< T, E > rhs ) const throw() { return m_int == (T)rhs; }

    // != operators
    template < typename U >
    bool operator !=( U rhs ) const throw()
    {
        return !details::EqualityTest< T, U >::IsEquals( m_int, rhs );
    }

    bool operator !=( bool b ) const throw()
    {
        return ( m_int == 0 ? false : true ) != b;
    }

    bool operator !=( SafeInt< T, E > rhs ) const throw() { return m_int != (T)rhs; }

    // Shift operators
    // Note - shift operators ALWAYS return the same type as the lhs
    // specific version for SafeInt< T, E > not needed -
    // code path is exactly the same as for SafeInt< U, E > as rhs

    // Left shift
    // Also, shifting > bitcount is undefined - trap in debug (check _SAFEINT_SHIFT_ASSERT)

    template < typename U >
    SafeInt< T, E > operator <<( U bits ) const throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( bits < (int)details::IntTraits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int << bits ) );
    }

    template < typename U >
    SafeInt< T, E > operator <<( SafeInt< U, E > bits ) const throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || (U)bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( (U)bits < (int)details::IntTraits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int << (U)bits ) );
    }

    // Left shift assignment

    template < typename U >
    SafeInt< T, E >& operator <<=( U bits ) throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( bits < (int)details::IntTraits< T >::bitCount );

        m_int <<= bits;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator <<=( SafeInt< U, E > bits ) throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || (U)bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( (U)bits < (int)details::IntTraits< T >::bitCount );

        m_int <<= (U)bits;
        return *this;
    }

    // Right shift
    template < typename U >
    SafeInt< T, E > operator >>( U bits ) const throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( bits < (int)details::IntTraits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int >> bits ) );
    }

    template < typename U >
    SafeInt< T, E > operator >>( SafeInt< U, E > bits ) const throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || (U)bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( bits < (int)details::IntTraits< T >::bitCount );

        return SafeInt< T, E >( (T)(m_int >> (U)bits) );
    }

    // Right shift assignment
    template < typename U >
    SafeInt< T, E >& operator >>=( U bits ) throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( bits < (int)details::IntTraits< T >::bitCount );

        m_int >>= bits;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator >>=( SafeInt< U, E > bits ) throw()
    {
        _SAFEINT_SHIFT_ASSERT( !details::IntTraits< U >::isSigned || (U)bits >= 0 );
        _SAFEINT_SHIFT_ASSERT( (U)bits < (int)details::IntTraits< T >::bitCount );

        m_int >>= (U)bits;
        return *this;
    }

    // Bitwise operators
    // This only makes sense if we're dealing with the same type and size
    // demand a type T, or something that fits into a type T

    // Bitwise &
    SafeInt< T, E > operator &( SafeInt< T, E > rhs ) const throw()
    {
        return SafeInt< T, E >( m_int & (T)rhs );
    }

    template < typename U >
    SafeInt< T, E > operator &( U rhs ) const throw()
    {
        // we want to avoid setting bits by surprise
        // consider the case of lhs = int, value = 0xffffffff
        //                      rhs = char, value = 0xff
        //
        // programmer intent is to get only the lower 8 bits
        // normal behavior is to upcast both sides to an int
        // which then sign extends rhs, setting all the bits

        // If you land in the assert, this is because the bitwise operator
        // was causing unexpected behavior. Fix is to properly cast your inputs
        // so that it works like you meant, not unexpectedly

        return SafeInt< T, E >( details::BinaryAndHelper< T, U >::And( m_int, rhs ) );
    }

    // Bitwise & assignment
    SafeInt< T, E >& operator &=( SafeInt< T, E > rhs ) throw()
    {
        m_int &= (T)rhs;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator &=( U rhs ) throw()
    {
        m_int = details::BinaryAndHelper< T, U >::And( m_int, rhs );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator &=( SafeInt< U, E > rhs ) throw()
    {
        m_int = details::BinaryAndHelper< T, U >::And( m_int, (U)rhs );
        return *this;
    }

    // XOR
    SafeInt< T, E > operator ^( SafeInt< T, E > rhs ) const throw()
    {
        return SafeInt< T, E >( (T)( m_int ^ (T)rhs ) );
    }

    template < typename U >
    SafeInt< T, E > operator ^( U rhs ) const throw()
    {
        // If you land in the assert, this is because the bitwise operator
        // was causing unexpected behavior. Fix is to properly cast your inputs
        // so that it works like you meant, not unexpectedly

        return SafeInt< T, E >( details::BinaryXorHelper< T, U >::Xor( m_int, rhs ) );
    }

    // XOR assignment
    SafeInt< T, E >& operator ^=( SafeInt< T, E > rhs ) throw()
    {
        m_int ^= (T)rhs;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator ^=( U rhs ) throw()
    {
        m_int = details::BinaryXorHelper< T, U >::Xor( m_int, rhs );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator ^=( SafeInt< U, E > rhs ) throw()
    {
        m_int = details::BinaryXorHelper< T, U >::Xor( m_int, (U)rhs );
        return *this;
    }

    // bitwise OR
    SafeInt< T, E > operator |( SafeInt< T, E > rhs ) const throw()
    {
        return SafeInt< T, E >( (T)( m_int | (T)rhs ) );
    }

    template < typename U >
    SafeInt< T, E > operator |( U rhs ) const throw()
    {
        return SafeInt< T, E >( details::BinaryOrHelper< T, U >::Or( m_int, rhs ) );
    }

    // bitwise OR assignment
    SafeInt< T, E >& operator |=( SafeInt< T, E > rhs ) throw()
    {
        m_int |= (T)rhs;
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator |=( U rhs ) throw()
    {
        m_int = details::BinaryOrHelper< T, U >::Or( m_int, rhs );
        return *this;
    }

    template < typename U >
    SafeInt< T, E >& operator |=( SafeInt< U, E > rhs ) throw()
    {
        m_int = details::BinaryOrHelper< T, U >::Or( m_int, (U)rhs );
        return *this;
    }

    // Miscellaneous helper functions
    SafeInt< T, E > Min( SafeInt< T, E > test, SafeInt< T, E > floor = SafeInt< T, E >( details::IntTraits< T >::minInt ) ) const throw()
    {
        T tmp = test < m_int ? test : m_int;
        return tmp < floor ? floor : tmp;
    }

    SafeInt< T, E > Max( SafeInt< T, E > test, SafeInt< T, E > upper = SafeInt< T, E >( details::IntTraits< T >::maxInt ) ) const throw()
    {
        T tmp = test > m_int ? test : m_int;
        return tmp > upper ? upper : tmp;
    }

    void Swap( SafeInt< T, E >& with ) throw()
    {
        T temp( m_int );
        m_int = with.m_int;
        with.m_int = temp;
    }

    template < int bits >
    const SafeInt< T, E >& Align()
    {
        // Zero is always aligned
        if( m_int == 0 )
            return *this;

        // We don't support aligning negative numbers at this time
        // Can't align unsigned numbers on bitCount (e.g., 8 bits = 256, unsigned char max = 255)
        // or signed numbers on bitCount-1 (e.g., 7 bits = 128, signed char max = 127).
        // Also makes no sense to try to align on negative or no bits.

        _SAFEINT_SHIFT_ASSERT( ( ( details::IntTraits<T>::isSigned && bits < (int)details::IntTraits< T >::bitCount - 1 )
            || ( !details::IntTraits<T>::isSigned && bits < (int)details::IntTraits< T >::bitCount ) ) &&
            bits >= 0 && ( !details::IntTraits<T>::isSigned || m_int > 0 ) );

        const T AlignValue = ( (T)1 << bits ) - 1;

        m_int = ( m_int + AlignValue ) & ~AlignValue;

        if( m_int <= 0 )
            E::SafeIntOnOverflow();

        return *this;
    }

    // Commonly needed alignments:
    const SafeInt< T, E >& Align2()  { return Align< 1 >(); }
    const SafeInt< T, E >& Align4()  { return Align< 2 >(); }
    const SafeInt< T, E >& Align8()  { return Align< 3 >(); }
    const SafeInt< T, E >& Align16() { return Align< 4 >(); }
    const SafeInt< T, E >& Align32() { return Align< 5 >(); }
    const SafeInt< T, E >& Align64() { return Align< 6 >(); }

private:
    T m_int;
};

// Externally defined functions for the case of U op SafeInt< T, E >
template < typename T, typename U, typename E >
bool operator <( U lhs, SafeInt< T, E > rhs ) throw()
{
    return details::GreaterThanTest< T, U >::GreaterThan( (T)rhs, lhs );
}

template < typename T, typename U, typename E >
bool operator <( SafeInt< U, E > lhs, SafeInt< T, E > rhs ) throw()
{
    return details::GreaterThanTest< T, U >::GreaterThan( (T)rhs, (U)lhs );
}

// Greater than
template < typename T, typename U, typename E >
bool operator >( U lhs, SafeInt< T, E > rhs ) throw()
{
    return details::GreaterThanTest< U, T >::GreaterThan( lhs, (T)rhs );
}

template < typename T, typename U, typename E >
bool operator >( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) throw()
{
    return details::GreaterThanTest< T, U >::GreaterThan( (T)lhs, (U)rhs );
}

// Greater than or equal
template < typename T, typename U, typename E >
bool operator >=( U lhs, SafeInt< T, E > rhs ) throw()
{
    return !details::GreaterThanTest< T, U >::GreaterThan( (T)rhs, lhs );
}

template < typename T, typename U, typename E >
bool operator >=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) throw()
{
    return !details::GreaterThanTest< U, T >::GreaterThan( (U)rhs, (T)lhs );
}

// Less than or equal
template < typename T, typename U, typename E >
bool operator <=( U lhs, SafeInt< T, E > rhs ) throw()
{
    return !details::GreaterThanTest< U, T >::GreaterThan( lhs, (T)rhs );
}

template < typename T, typename U, typename E >
bool operator <=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) throw()
{
    return !details::GreaterThanTest< T, U >::GreaterThan( (T)lhs, (U)rhs );
}

// equality
// explicit overload for bool
template < typename T, typename E >
bool operator ==( bool lhs, SafeInt< T, E > rhs ) throw()
{
    return lhs == ( (T)rhs == 0 ? false : true );
}

template < typename T, typename U, typename E >
bool operator ==( U lhs, SafeInt< T, E > rhs ) throw()
{
    return details::EqualityTest< T, U >::IsEquals((T)rhs, lhs);
}

template < typename T, typename U, typename E >
bool operator ==( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) throw()
{
    return details::EqualityTest< T, U >::IsEquals( (T)lhs, (U)rhs );
}

//not equals
template < typename T, typename U, typename E >
bool operator !=( U lhs, SafeInt< T, E > rhs ) throw()
{
    return !details::EqualityTest< T, U >::IsEquals( rhs, lhs );
}

template < typename T, typename E >
bool operator !=( bool lhs, SafeInt< T, E > rhs ) throw()
{
    return ( (T)rhs == 0 ? false : true ) != lhs;
}

template < typename T, typename U, typename E >
bool operator !=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) throw()
{
    return !details::EqualityTest< T, U >::IsEquals( lhs, rhs );
}

// Modulus
template < typename T, typename U, typename E >
SafeInt< T, E > operator %( U lhs, SafeInt< T, E > rhs )
{
    // Value of return depends on sign of lhs
    // This one may not be safe - bounds check in constructor
    // if lhs is negative and rhs is unsigned, this will throw an exception.

    // Fast-track the simple case
    // same size and same sign
#pragma warning(suppress:4127 6326)
    if( sizeof(T) == sizeof(U) && details::IntTraits< T >::isSigned == details::IntTraits< U >::isSigned )
    {
        if( rhs != 0 )
        {
            if( details::IntTraits< T >::isSigned && (T)rhs == -1 )
                return 0;

            return SafeInt< T, E >( (T)( lhs % (T)rhs ) );
        }

        E::SafeIntOnDivZero();
    }

    return SafeInt< T, E >( ( SafeInt< U, E >( lhs ) % (T)rhs ) );
}

// Multiplication
template < typename T, typename U, typename E >
SafeInt< T, E > operator *( U lhs, SafeInt< T, E > rhs )
{
    T ret( 0 );
    details::MultiplicationHelper< T, U, E >::Multiply( (T)rhs, lhs, ret );
    return SafeInt< T, E >(ret);
}

// Division
template < typename T, typename U, typename E > SafeInt< T, E > operator /( U lhs, SafeInt< T, E > rhs )
{
#pragma warning(push)
#pragma warning(disable: 4127 4146 4307 4310 6326)
    // Corner case - has to be handled separately
    if( details::DivisionMethod< U, T >::method ==  details::DivisionState_UnsignedSigned )
    {
        if( (T)rhs > 0 )
            return SafeInt< T, E >( lhs/(T)rhs );

        // Now rhs is either negative, or zero
        if( (T)rhs != 0 )
        {
            if( sizeof( U ) >= 4 && sizeof( T ) <= sizeof( U ) )
            {
                // Problem case - normal casting behavior changes meaning
                // flip rhs to positive
                // any operator casts now do the right thing
                U tmp;
                if( sizeof(T) == 4 )
                    tmp = lhs/(U)(unsigned __int32)( -(T)rhs );
                else
                    tmp = lhs/(U)( -(T)rhs );

                if( tmp <= details::IntTraits< T >::maxInt )
                    return SafeInt< T, E >( -( (T)tmp ) );

                // Corner case
                // Note - this warning happens because we're not using partial
                // template specialization in this case. For any real cases where
                // this block isn't optimized out, the warning won't be present.
                if( tmp == (U)details::IntTraits< T >::maxInt + 1 )
                    return SafeInt< T, E >( details::IntTraits< T >::minInt );

                E::SafeIntOnOverflow();
            }

            return SafeInt< T, E >(lhs/(T)rhs);
        }

        E::SafeIntOnDivZero();
    } // method == DivisionState_UnsignedSigned

    if( details::SafeIntCompare< T, U >::isBothSigned )
    {
        if( lhs == details::IntTraits< U >::minInt && (T)rhs == -1 )
        {
            // corner case of a corner case - lhs = min int, rhs = -1,
            // but rhs is the return type, so in essence, we can return -lhs
            // if rhs is a larger type than lhs
            if( sizeof( U ) < sizeof( T ) )
            {
                return SafeInt< T, E >( (T)( -(T)details::IntTraits< U >::minInt ) );
            }

            // If rhs is smaller or the same size int, then -minInt won't work
            E::SafeIntOnOverflow();
        }
    }

    // Otherwise normal logic works with addition of bounds check when casting from U->T
    U ret;
    details::DivisionHelper< U, T, E >::Divide( lhs, (T)rhs, ret );
    return SafeInt< T, E >( ret );
#pragma warning(pop)
}

// Addition
template < typename T, typename U, typename E >
SafeInt< T, E > operator +( U lhs, SafeInt< T, E > rhs )
{
    T ret( 0 );
    details::AdditionHelper< T, U, E >::Addition( (T)rhs, lhs, ret );
    return SafeInt< T, E >( ret );
}

// Subtraction
template < typename T, typename U, typename E >
SafeInt< T, E > operator -( U lhs, SafeInt< T, E > rhs )
{
    T ret( 0 );
    details::SubtractionHelper< U, T, E, details::SubtractionMethod2< U, T >::method >::Subtract( lhs, rhs.Ref(), ret );

    return SafeInt< T, E >( ret );
}

// Overrides designed to deal with cases where a SafeInt is assigned out
// to a normal int - this at least makes the last operation safe
// +=
template < typename T, typename U, typename E >
T& operator +=( T& lhs, SafeInt< U, E > rhs )
{
    T ret( 0 );
    details::AdditionHelper< T, U, E >::Addition( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
T& operator -=( T& lhs, SafeInt< U, E > rhs )
{
    T ret( 0 );
    details::SubtractionHelper< T, U, E >::Subtract( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
T& operator *=( T& lhs, SafeInt< U, E > rhs )
{
    T ret( 0 );
    details::MultiplicationHelper< T, U, E >::Multiply( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
T& operator /=( T& lhs, SafeInt< U, E > rhs )
{
    T ret( 0 );
    details::DivisionHelper< T, U, E >::Divide( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
T& operator %=( T& lhs, SafeInt< U, E > rhs )
{
    T ret( 0 );
    details::ModulusHelper< T, U, E >::Modulus( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
T& operator &=( T& lhs, SafeInt< U, E > rhs ) throw()
{
    lhs = details::BinaryAndHelper< T, U >::And( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
T& operator ^=( T& lhs, SafeInt< U, E > rhs ) throw()
{
    lhs = details::BinaryXorHelper< T, U >::Xor( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
T& operator |=( T& lhs, SafeInt< U, E > rhs ) throw()
{
    lhs = details::BinaryOrHelper< T, U >::Or( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
T& operator <<=( T& lhs, SafeInt< U, E > rhs ) throw()
{
    lhs = (T)( SafeInt< T, E >( lhs ) << (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
T& operator >>=( T& lhs, SafeInt< U, E > rhs ) throw()
{
    lhs = (T)( SafeInt< T, E >( lhs ) >> (U)rhs );
    return lhs;
}

// Specific pointer overrides
// Note - this function makes no attempt to ensure
// that the resulting pointer is still in the buffer, only
// that no int overflows happened on the way to getting the new pointer
template < typename T, typename U, typename E >
T*& operator +=( T*& lhs, SafeInt< U, E > rhs )
{
    // Cast the pointer to a number so we can do arithmetic
    SafeInt< uintptr_t, E > ptr_val = reinterpret_cast< uintptr_t >( lhs );
    // Check first that rhs is valid for the type of ptrdiff_t
    // and that multiplying by sizeof( T ) doesn't overflow a ptrdiff_t
    // Next, we need to add 2 SafeInts of different types, so unbox the ptr_diff
    // Finally, cast the number back to a pointer of the correct type
    lhs = reinterpret_cast< T* >( (uintptr_t)( ptr_val + (ptrdiff_t)( SafeInt< ptrdiff_t, E >( rhs ) * sizeof( T ) ) ) );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator -=( T*& lhs, SafeInt< U, E > rhs )
{
    // Cast the pointer to a number so we can do arithmetic
    SafeInt< size_t, E > ptr_val = reinterpret_cast< uintptr_t >( lhs );
    // See above for comments
    lhs = reinterpret_cast< T* >( (uintptr_t)( ptr_val - (ptrdiff_t)( SafeInt< ptrdiff_t, E >( rhs ) * sizeof( T ) ) ) );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator *=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator /=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator %=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator &=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator ^=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator |=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator <<=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator >>=( T* lhs, SafeInt< U, E >)
{
    static_assert( details::DependentFalse< T >::value, "SafeInt<T>: This operator explicitly not supported" );
    return lhs;
}

// Shift operators
// NOTE - shift operators always return the type of the lhs argument

// Left shift
template < typename T, typename U, typename E >
SafeInt< U, E > operator <<( U lhs, SafeInt< T, E > bits ) throw()
{
    _SAFEINT_SHIFT_ASSERT( !details::IntTraits< T >::isSigned || (T)bits >= 0 );
    _SAFEINT_SHIFT_ASSERT( (T)bits < (int)details::IntTraits< U >::bitCount );

    return SafeInt< U, E >( (U)( lhs << (T)bits ) );
}

// Right shift
template < typename T, typename U, typename E >
SafeInt< U, E > operator >>( U lhs, SafeInt< T, E > bits ) throw()
{
    _SAFEINT_SHIFT_ASSERT( !details::IntTraits< T >::isSigned || (T)bits >= 0 );
    _SAFEINT_SHIFT_ASSERT( (T)bits < (int)details::IntTraits< U >::bitCount );

    return SafeInt< U, E >( (U)( lhs >> (T)bits ) );
}

// Bitwise operators
// This only makes sense if we're dealing with the same type and size
// demand a type T, or something that fits into a type T.

// Bitwise &
template < typename T, typename U, typename E >
SafeInt< T, E > operator &( U lhs, SafeInt< T, E > rhs ) throw()
{
    return SafeInt< T, E >( details::BinaryAndHelper< T, U >::And( (T)rhs, lhs ) );
}

// Bitwise XOR
template < typename T, typename U, typename E >
SafeInt< T, E > operator ^( U lhs, SafeInt< T, E > rhs ) throw()
{
    return SafeInt< T, E >(details::BinaryXorHelper< T, U >::Xor( (T)rhs, lhs ) );
}

// Bitwise OR
template < typename T, typename U, typename E >
SafeInt< T, E > operator |( U lhs, SafeInt< T, E > rhs ) throw()
{
    return SafeInt< T, E >( details::BinaryOrHelper< T, U >::Or( (T)rhs, lhs ) );
}

} // namespace utilities

} // namespace msl

#pragma pack(pop)

#pragma warning(pop) // Disable /Wall warnings
#endif // RC_INVOKED
