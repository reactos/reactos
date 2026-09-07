// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Defines macros to define delay loading function pointer implementation.
//
//-----------------------------------------------------------------------------

#pragma once

// Macro: STRINGIZE - returns quoted string of text given a parameter.
#ifndef STRINGIZE
#define STRINGIZE_IMPL(x) #x
#define STRINGIZE(x) STRINGIZE_IMPL(x)
#endif


//+----------------------------------------------------------------------------
//
//  Macros:
//      DELAY_LOAD_PROC / DELAY_LOAD_PROC_EX
//
//  Synopsis:
//      Defines function pointer for a function in a delay loaded module,
//      convenience wrapper method of the same name, and provides load on first
//      use logic.  A STUB_<ProcName> method must be defined in case the
//      procedure fails to load.  A link error will result if the stub method
//      is not defined.
//
//      The load implementation will call LoadProcAddress on specified <Module>
//      object given.  <Module> may be a CDelayLoadedModule or other object
//      that has LoadProcAddress method.  When using DELAY_LOAD_PROC <Module>
//      is specified by defining DLP_MODULE_VARIABLE before using
//      DELAY_LOAD_PROC.  DELAY_LOAD_PROC_EX takes <Module> as first argument.
//
//      The macro also allows a prefix for the function pointer variable it
//      will define.  When using DELAY_LOAD_PROC <PFNPrefix> is specified by
//      defining DLP_PFN_VARIABLE_PREFIX before using DELAY_LOAD_PROC. 
//      DELAY_LOAD_PROC_EX takes <PFNPrefix> as second argument.
//
//  Arguments:
//
//      ReturnType      export's return-type
//      ProcName        export's name
//      ParameterList   export's parameter-type-list
//      ArgumentList    export's arguments (parameter name list without types)
//
//      DLP_MODULE_VARIABLE / Module
//          CDelayLoadedModule variable name (or object that has
//          LoadProcAddress method
//
//      DLP_PFN_VARIABLE_PREFIX / PFNPrefix
//          Prefix given to function pointer variable this code will define.
//
//-----------------------------------------------------------------------------

#define DELAY_LOAD_PROC(ReturnType, ProcName, ParameterList, ArgumentList)  \
    DELAY_LOAD_PROC_EX(DLP_MODULE_VARIABLE, DLP_PFN_VARIABLE_PREFIX, ReturnType, ProcName, ParameterList, ArgumentList)

#define DELAY_LOAD_PROC_EX(Module, PFNPrefix, ReturnType, ProcName, ParameterList, ArgumentList)  \
    DELAY_LOAD_PROC_IMPL(Module, PFNPrefix, ReturnType, ProcName, ParameterList, ArgumentList)


#define DELAY_LOAD_PROC_IMPL(Module, PFNPrefix, ReturnType, ProcName, ParameterList, ArgumentList)  \
    typedef ReturnType (WINAPI *PFN##ProcName) ParameterList;                   \
\
    extern PFN##ProcName volatile PFNPrefix##PFN##ProcName;                     \
\
    ReturnType WINAPI STUB_##ProcName ParameterList ;                           \
\
    ReturnType WINAPI LOAD_##ProcName ParameterList                             \
    {                                                                           \
        PFN##ProcName pfn =                                                     \
            reinterpret_cast<PFN##ProcName>                                     \
                (Module.LoadProcAddress(STRINGIZE(ProcName)));                  \
\
        /* If we couldn't get the process address then use a stub */            \
        if (pfn == NULL)                                                        \
        {                                                                       \
            pfn = STUB_##ProcName;                                              \
        }                                                                       \
\
        PFNPrefix##PFN##ProcName = pfn;                                         \
\
        return PFNPrefix##PFN##ProcName ArgumentList;                           \
    }                                                                           \
\
    PFN##ProcName volatile PFNPrefix##PFN##ProcName = LOAD_##ProcName;          \
\
    ReturnType ProcName ParameterList                                           \
    {                                                                           \
        return PFNPrefix##PFN##ProcName ArgumentList;                           \
    }                                                                           \



