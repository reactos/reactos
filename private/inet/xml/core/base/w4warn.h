/*----------------------------------------------------------------------------
*  Note that we don't want to use a single line comment before the warning is
*   disabled.
*
*   Microsoft Windows
* Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*
*   File:       w4warn.h
*
*   Contents:   #pragmas to adjust warning levels.
*
*---------------------------------------------------------------------------*/

/*
 *   Level 4 warnings to suppress.
 */

#ifdef WIN16
#pragma warning 442 9
#pragma warning 604 9
#pragma warning 583 9
#pragma warning 594 9

#pragma warning 379 9    // 'delete' expression will invoke a non-virtual destructor
#pragma warning 387 9   // expression is useful only for its side effects
#pragma warning 354    4    // unsigned or pointer expression is always >= 0
#pragma warning 389 4    // integral value may be truncated during assignment
#pragma warning 4    4    // base class XXX does not have a virtual destructor
#pragma warning 13    4    // unreachable code
#pragma warning 628 4    // expression is not meaningful
#pragma warning 627 9   // text following pre-processor directive (comment after endif)

#pragma warning 188    5    // base class is inherited with private access. basically means base
                        // class access hasn't been specified on the class definiton.

#pragma off(unreferenced)

#else
#pragma warning(disable:4001) /* nonstandard extension 'extension' was used                            */
#pragma warning(disable:4097) /* typedef name 'type' used as synonym for class 'class'                 */
#pragma warning(disable:4100) /* unreferenced formal parameter                                         */
#pragma warning(disable:4115) /* named type definition in parentheses                                  */
#pragma warning(disable:4134) /* conversion between pointers to members of same class                  */
#pragma warning(disable:4152) /* nonstandard extension, function/data pointer conversion in expression */
#pragma warning(disable:4200) /* nonstandard extension used : zero-sized array in struct/union         */
#pragma warning(disable:4201) /* nonstandard extension used : nameless struct/union                    */
#pragma warning(disable:4204) /* nonstandard extension used : non-constant aggregate initializer       */
#pragma warning(disable:4209) /* nonstandard extension used : benign typedef redefinition              */
#pragma warning(disable:4214) /* nonstandard extension used : bit field types other than int           */
#pragma warning(disable:4505) /* unreferenced local function has been removed                          */
#pragma warning(disable:4511) /* 'class' : copy constructor could not be generated                     */
#pragma warning(disable:4512) /* 'class': assignment operator could not be generated                   */
#pragma warning(disable:4514) /* 'function' : unreferenced inline function has been removed            */
#pragma warning(disable:4705) /* statement has no effect                                               */
#pragma warning(disable:4710) /* function call not expanded                                            */
/*
 * Warning 4244 is benign more often than not, so if you don't want it then
 * uncomment this to filter out those errors.
 *
 */
#pragma warning(disable:4244) /* conversion from 'type' to 'type', possible loss of data               */

/*
 * This helps to track down "Illegal attempt to instantiate abstract class" messages
 */

#pragma warning(error:4259) /* pure virtual function not defined                                       */

/*
 *   Level 4 warnings that we want treated as level 3 warnings.
 */

#pragma warning(3:4127) /* conditional expression is constant                                          */
#pragma warning(3:4702) /* unreachable code                                                            */
#pragma warning(3:4706) /* assignment within conditional expression                                    */

#pragma warning(disable:4041) /* compiler limit reached: terminating browser output                    */

#ifdef _MAC
#pragma warning(disable:4229) /* anachronism used : modifiers on data are ignored                      */
#pragma warning(disable:4798) /* pcode: constructors and destructors have native code generated        */
#endif

#ifdef _M_PPC
#pragma warning(disable:4701) /* local variable  may be used without having been initialized           */
#endif
#endif 
