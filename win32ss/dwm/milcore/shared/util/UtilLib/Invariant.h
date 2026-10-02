// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       Invariant.h
//------------------------------------------------------------------------------
#ifndef UtilLib__Invariant_h__INCLUDED
#define UtilLib__Invariant_h__INCLUDED

//------------------------------------------------------------------------------
//  Purpose:    Template class designed to call parameterized object's
//              Invariant().
//
//  Overview (see also, usage):
//      1:  declare and define a public const function BOOL Invariant( void ) in
//          your class, with #ifdef DEBUG.
//      2:  in the source: #define DEBUG_CLASSNAME to be the name of the class
//          you're debugging.
//      3:  followed by #include "Invariant.h"
//      4:  For every method you wish to check Invariants, insert the
//          _TEST_INVARIANT_ macro once, usually at the beginning of a routine.
//          OPTIONAL: You may optionally use the _TEST_INVARIANT_ON (x) to call x's Invariant directly.
//
//  Notes:  Invariants are designed to be called at the beginning and upon exit
//          of a routine, testing the consistent properties of an object which
//          remain invariant--always the same.
//
//          Functions may temporarily make an object inconsistent during their
//          execution. A generalized invariant test should not be called during
//          these inconsistent times; if there is a need for a function, which
//          checks invariants, to be called during an inconsistent object state,
//          a solution will need to be designed--the current design does not
//          facilitate this.
//
//          Because it is entirely possible for an Invariant() function to
//          recurse on itself causing a stack overflow, the template explicitly
//          prevents this from happening. The template also prevents
//          invariant-checking during the processing of Assert(), preventing
//          another type of recursion. Assert() recursion is avoided by checking
//          a global flag, fInAssert.
//
//  Usage:
//      *  the Invariant.h header should only be included in source files. An
//         error will occur if included in another header file. This is to
//         prevent multiple #define DEBUG_CLASSNAME.
//      *  Typical #include into a source file looks like this:
//              #define DEBUG_CLASSNAME ClassName
//              #include "_invar.h"
//      *  Typical definition of a class' Invariant() method looks like this:
//              #ifdef DEBUG
//                  public:
//                  BOOL Invariant( void ) const;
//                  protected:
//              #endif  // DEBUG
//      *  Typical declaration of Invariant() looks like this:
//              #ifdef DEBUG
//              BOOL
//              ClassName::Invariant( void ) const
//              {
//                  static LONG numTests = 0;
//                  numTests++;             // how many times we've been called.
//                   // do mega-assert checking here.
//                   return TRUE;
//              }
//              #endif  // DEBUG
//  
//------------------------------------------------------------------------------

#ifndef DEBUG_CLASSNAME
#pragma error(prior to including Invariant.h file, you must define DEBUG_CLASSNAME to be the name of the class for which you are making Invariant() calls.)
#endif

#ifdef DEBUG
//------------------------------------------------------------------------------
//  Member:     InvariantDebug
//------------------------------------------------------------------------------
template < class T >
class InvariantDebug
{
public:
    InvariantDebug  (__in_ecount(1) const T & t) : _t(t)
    {
        static volatile BOOL fRecurse = FALSE;

        if ( fRecurse ) return;     /* Don't allow recursion.*/
        
        fRecurse = TRUE;

        _t.Invariant();

        fRecurse = FALSE;
    }

    ~InvariantDebug ()
    {
        static volatile BOOL fRecurse = FALSE;

        if ( fRecurse ) return;     /* Don't allow recursion.*/
        
        fRecurse = TRUE;

        _t.Invariant();

        fRecurse = FALSE;
    }

private:
     const T &_t;
};

typedef InvariantDebug<DEBUG_CLASSNAME> DoInvariant;

#define _TEST_INVARIANT_ DoInvariant __invariant_tester( *this );
#define _TEST_INVARIANT_ON(x) \
                    {\
                        static volatile BOOL fRecurse = FALSE;\
                        if ( FALSE == fRecurse )\
                        {\
                            fRecurse = TRUE;\
                            (x).Invariant();\
                            fRecurse = FALSE;\
                        }\
                    }

#else   // DEBUG

#define _TEST_INVARIANT_
#define _TEST_INVARIANT_ON(x)

#endif  // DEBUG


#endif // UtilLib__Invariant_h__INCLUDED



