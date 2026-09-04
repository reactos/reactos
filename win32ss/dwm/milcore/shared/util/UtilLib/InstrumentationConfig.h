// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Contents:  Macros and flags used to configure the behavior of the MIL
//             debug instrumentation.  The instrumentation can be 
//             configured to do nothing, capture the current stack, or
//             break into the debugger upon failure within block, function,
//             class, or global scope.
//
//------------------------------------------------------------------------

//
// Instrumentation configuration macros
//

//+----------------------------------------------------------------------------
//
// Macro:   SET_MILINSTRUMENTATION_FLAGS and
//          SET_CONDITIONAL_MILINSTRUMENTATION_FLAGS
//
// Synopsis: Configures the instrumentation's behavior within whatever scope it
//           is used in.  If flags are dependent on runtime state use
//           SET_CONDITIONAL_MILINSTRUMENTATION_FLAGS.
//
// Example Usage:  To configure the instrumentation to break when a failure
//                 occurs:
//
//  SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_BREAKONFAIL);
//
// Example Usage:  To configure the instrumentation to break when a failure
//                 occurs, but only when fCaseA is true:
//
//  SET_MILINSTRUMENTATION_FLAGS( fCaseA ?
//                                MILINSTRUMENTATIONFLAGS_BREAKONFAIL :
//                                MILINSTRUMENTATIONFLAGS_DONOTHING);
//
//-----------------------------------------------------------------------------
#define SET_MILINSTRUMENTATION_FLAGS(f) \
    static const DWORD MILINSTRUMENTATIONFLAGS = (f)
#define SET_CONDITIONAL_MILINSTRUMENTATION_FLAGS(f) \
    const DWORD MILINSTRUMENTATIONFLAGS = (f)

//+--------------------------------------------------------------------------------------
//
// Macro:  (BEGIN|END)_MILINSTRUMENTATION_HRESULT_LIST
//
// Synopsis: Declares an HRESULT list within whatever scope it is used in
//
// Example Usage (comments assume MILINSTRUMENTATIONFLAGS_BREAKEXCLUDELIST is set):
//
//          BEGIN_MILINSTRUMENTATION_HRESULT_LIST
//              // Don't break on E_OUTOFMEMORY
//              E_OUTOFMEMORY,   
//              // Don't break on ERROR_NOT_ENOUGH_MEMORY
//              HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
//              // Don't break on E_POINTER
//              E_POINTER
//          END_MILINSTRUMENTATION_HRESULT_LIST  
//
//---------------------------------------------------------------------------------------
#define BEGIN_MILINSTRUMENTATION_HRESULT_LIST \
    static const HRESULT MILINSTRUMENTATIONHRESULTLIST[] = {
#define END_MILINSTRUMENTATION_HRESULT_LIST \
    }; \
    static const UINT MILINSTRUMENTATIONHRESULTLISTSIZE = ARRAY_SIZE(MILINSTRUMENTATIONHRESULTLIST);

//+--------------------------------------------------------------------------------------
//
// Macro:  BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
//
// Synopsis: Begins an HRESULT list within whatever scope it is used in that includes
//              the default exclude HRESULTs (OUTOFMEMORY)
//
// Example Usage:  To define a list with E_POINTER and OUTOFMEMORY HRESULTs:
//
//          BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
//              // Don't break on E_POINTER
//              E_POINTER
//          END_MILINSTRUMENTATION_HRESULT_LIST
//
//---------------------------------------------------------------------------------------
#define BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS \
    static const HRESULT MILINSTRUMENTATIONHRESULTLIST[] = { MILINSTRUMENTATION_DEFAULTEXCLUDEHRS

//+--------------------------------------------------------------------------------------
//
// Macro:       DECLARE_CLASS_MILINSTRUMENTATION_HRESULT_LIST
//
// Synopsis:    Declares an HRESULT list that is scoped to a specific class
//              by declaring static constants as members of that class.
//              All instrumented macros within the class's methods will use 
//              this HRESULT list.
//              This macro is to be used within the class definition (.H file)
//
// Example Usage: To declare that a class has a class-specific HRESULT list (the 
//                list must also be defined in the .CPP file using 
//                BEGIN/END_CLASS_MILINSTRUMENTATION_HRESULT_LIST):
//
//      class CFoo {
//      ...
//      private:
//                DECLARE_CLASS_MILINSTRUMENTATION_HRESULT_LIST;
//      };
//
//
//---------------------------------------------------------------------------------------
#define DECLARE_CLASS_MILINSTRUMENTATION_HRESULT_LIST \
    static const HRESULT MILINSTRUMENTATIONHRESULTLIST[]; \
    static const UINT MILINSTRUMENTATIONHRESULTLISTSIZE

//+--------------------------------------------------------------------------------------
//
// Macro: (BEGIN|END)_CLASS_MILINSTRUMENTATION_HRESULT_LIST
//
// Synopsis:    Defines an HRESULT list that is scoped to a specific class
//              by declaring static constants as members of that class
//              All instrumented macros within the class's methods will use 
//              this HRESULT list.
//              This macro is to be used in the class implementation file (.CPP file)
//
// Example Usage:  To define a HRESULT for list for class 'CFoo' (must also declare the
//                 HRESULT list using DECLARE_CLASS_MILINSTRUMENTATION_HRESULT_LIST):
//
//          BEGIN_CLASS_MILINSTRUMENTATION_HRESULT_LIST(CFoo)
//              // Don't break on E_OUTOFMEMORY
//              E_OUTOFMEMORY,   
//              // Don't break on ERROR_NOT_ENOUGH_MEMORY
//              HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
//              // Don't break on E_POINTER
//              E_POINTER
//          END_CLASS_MILINSTRUMENTATION_HRESULT_LIST(CFoo)
//
//---------------------------------------------------------------------------------------
#define BEGIN_CLASS_MILINSTRUMENTATION_HRESULT_LIST(_class_) \
    const HRESULT _class_::MILINSTRUMENTATIONHRESULTLIST[] = {
#define END_CLASS_MILINSTRUMENTATION_HRESULT_LIST(_class_) \
    }; \
    const UINT _class_::MILINSTRUMENTATIONHRESULTLISTSIZE = ARRAY_SIZE(_class_::MILINSTRUMENTATIONHRESULTLIST);

//+--------------------------------------------------------------------------------------
//
// Macro: BEGIN_CLASS_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
//
// Synopsis:    Begins an HRESULT list that is scoped to a specific class
//              by declaring static constants as members of that class, and includes
//              the default exclude HRESULTs (OUTOFMEMORY)
//              All instrumented macros within the class's methods will use 
//              this HRESULT list.
//              This macro is to be used in the class implementation file (.CPP file)
//
// Example Usage:  To define a exlude HRESULT for list for class 'CFoo' (must also declare the
//                 HRESULT list using DECLARE_CLASS_MILINSTRUMENTATION_HRESULT_LIST):
//
//          BEGIN_CLASS_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS(CFoo)
//              // Don't break on E_POINTER
//              E_POINTER
//          END_CLASS_MILINSTRUMENTATION_HRESULT_LIST(CFoo)
//
//---------------------------------------------------------------------------------------
#define BEGIN_CLASS_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS(_class_) \
    const HRESULT _class_::MILINSTRUMENTATIONHRESULTLIST[] = { MILINSTRUMENTATION_DEFAULTEXCLUDEHRS

//+--------------------------------------------------------------------------------------
//
// #DEFINE FLAG: OVERRIDE_GLOBAL_MILINSTRUMENTATION_FLAGS
//
// Synopsis:    Used to redefine behavior of the debug instrumentation within a
//              limited global scope (i.e., global scope within a particular subset 
//              of files). This is often used in precomp.h to override flags
//              for a specific directory.  
//              This flag must be defined before this header is included.
//
// Example Usage:  To configure the instrumentation to break on failure within
//                 a limited global scope:
//
//  #define OVERRIDE_GLOBAL_MILINSTRUMENTATION_FLAGS MILINSTRUMENTATIONFLAGS_BREAKONFAIL;
//
//---------------------------------------------------------------------------------------
#ifndef OVERRIDE_GLOBAL_MILINSTRUMENTATION_FLAGS 
// Define the default instrumentation flags here within global scope.
SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DEFAULT);     
#else
// Use flag override
SET_MILINSTRUMENTATION_FLAGS(OVERRIDE_GLOBAL_MILINSTRUMENTATION_FLAGS);
#endif


