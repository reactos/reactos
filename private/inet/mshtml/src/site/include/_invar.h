
/*
 *
 *
 *  _INVAR.H
 *  
 *  Purpose:
 *      Template class designed to call parameterized object's Invariant().
 *
 *  Overview (see also, usage):
 *      1)  declare and define a public const function BOOL Invariant( void ) in your class, with #ifdef DEBUG.
 *      2)  in the source: #define DEBUG_CLASSNAME to be the name of the class you're debugging.
 *      3)  followed by #include "_invar.h"
 *      4)  For every method you wish to check Invariants,
 *          insert the _TEST_INVARIANT_ macro once, usually at the beginning of a routine.
 *          OPTIONAL: You may optionally use the _TEST_INVARIANT_ON (x) to call x's Invariant directly.
 *
 *  Notes:
 *      Invariants are designed to be called at the beginning and upon exit of a routine, 
 *      testing the consistent properties of an object which remain invariant--always the same.
 *
 *      Functions may temporarily make an object inconsistent during their execution.
 *      A generalized invariant test should not be called during these inconsistent times;
 *      if there is a need for a function, which checks invariants, to be called during
 *      an inconsistent object state, a solution will need to be designed--the current design
 *      does not facilitate this.
 *
 *      Because it is entirely possible for an Invariant() function to recurse on itself
 *      causing a stack overflow, the template explicitly prevents this from happening.
 *      The template also prevents invariant-checking during the processing of Assert(),
 *      preventing another type of recursion. Assert() recursion is avoided by checking
 *      a global flag, fInAssert.
 *
 *      Currently Invariant() returns a BOOL, as I think this allows for it to be called
 *      from the QuickWatch window under VC++2.0. TRUE indicates that the invariant executed
 *      normally.
 *
 *  Usage:
 *      -the _invariant.h header should only be included in source files. An error will occur
 *          if included in another header file. This is to prevent multiple #define DEBUG_CLASSNAME.
 *      -Typical #include into a source file looks like this:
            #define DEBUG_CLASSNAME ClassName
            #include "_invar.h"
 *      -Typical definition of a class' Invariant() method looks like this:
            #ifdef DEBUG
                public:
                BOOL Invariant( void ) const;
                protected:
            #endif  // DEBUG
 *      -Typical declaration of Invariant() looks like this:
            #ifdef DEBUG

            BOOL
            ClassName::Invariant( void ) const
            {
                static LONG numTests = 0;
                numTests++;             // how many times we've been called.

                // do mega-assert checking here.

                return TRUE;
            }

            #endif  // DEBUG
 *
 *
 *      
 *  
 *  Author:
 *      Jon Matousek (jonmat) 5/04/1995
 *
 *      Any problems? Please let me know.
 */

#ifndef I__INVAR_H_
#define I__INVAR_H_
#pragma INCMSG("--- Beg '_invar.h'")

#ifndef DEBUG_CLASSNAME
prior to including _invariant.h file, you must define DEBUG_CLASSNAME
to be the name of the class for which you are making Invariant() calls.
#endif


#ifdef DEBUG

template < class T >
class InvariantDebug
{
    public:
    InvariantDebug  ( const T & t) : _t(t)
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


    // code that should be at the start and end of all Invariant() methods.

#pragma INCMSG("--- End '_invar.h'")
#else
#pragma INCMSG("*** Dup '_invar.h'")
#endif
