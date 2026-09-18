// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//------------------------------------------------------------------------------
#pragma once

//+------------------------------------------------------------------------
//
// DYNCAST macro
//
// Use to cast objects from one class type to another. This should be used
// rather than using standard casts.
//
// Example:
//         CBodyElement *pBody = (CBodyElement*)_pElement;
//
//      is replaced by:
//
//         CBodyElement *pBody = DYNCAST(CBodyElement, _pElement);
//
// The dyncast macro will assert if _pElement is not really a CBodyElement.
//
// For ship builds the DYNCAST macro expands to a standard cast.
//
//-------------------------------------------------------------------------

#if !defined(_PREFAST_) && (!DBG || defined(NO_RTTI))

#define DYNCAST(Dest_type, Source_Value) (static_cast<Dest_type*>(Source_Value))

#else // PREfast || DBG

#if defined(_PREFAST_) || defined(NO_RTTI)

template <class TS, class TD>
__success(source != NULL) __out_ecount(1) TD * DYNCAST_IMPL(__in_ecount_opt(1) TS * source, __in_opt TD &, __in PCSTR pszType)
{
    return static_cast <TD *> (source);
}

#else // !PREfast && RTTI

#pragma warning(push)
#include <typeinfo>
#pragma warning(pop)

#include "UtilMisc.h"   // For ARRAY_COMMA_ECOUNT
#include <strsafe.h>    // For StringCchPrintfA

template <class TS, class TD>
__success(source != NULL) __out_ecount(1) TD * DYNCAST_IMPL(__in_ecount_opt(1) TS * source, __in_opt TD &, __in PCSTR pszType)
{
    if (!source) return NULL;

    char achDynCastMsg[256];

    TD * dest  = dynamic_cast <TD *> (source);
    TD * dest2 = static_cast <TD *> (source);
    if (!dest)
    {
        StringCchPrintfA(ARRAY_COMMA_ELEM_COUNT(achDynCastMsg),
                         "Invalid Static Cast -- Attempt to cast object "
                         "of type %s to type %s.",
                         typeid(*source).name(), pszType);
        AssertMsgA(FALSE, achDynCastMsg);
    }
    else if (dest != dest2)
    {
        StringCchPrintfA(ARRAY_COMMA_ELEM_COUNT(achDynCastMsg),
                         "Dynamic Cast Attempted ---  "
                         "Attempt to cast between two base classes of %s. "
                         "The cast was to class %s from some other base class "
                         "pointer. This cast will not succeed in a retail build.",
                         typeid(*source).name(), pszType);
        AssertMsgA(FALSE, achDynCastMsg);
    }

    return dest2;
}

#endif // PREfast || NO_RTTI

#define DYNCAST(Dest_type, Source_value) \
    DYNCAST_IMPL(Source_value,(Dest_type &)*(Dest_type*)NULL, #Dest_type)

#endif // PREfast || DBG


