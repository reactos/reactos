//+------------------------------------------------------------------------
//
//  File:       hook.cxx
//
//  Contents:   Hook interfaces used for debugging
//
//  History:    09-Jul-97   JohnV Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HOOK_HXX_
#define X_HOOK_HXX_
#include "hook.hxx"
#endif

#ifndef NO_DEBUG_HOOK

MtDefine(CHook, Utilities, "CHook")

CHook *
CreateHook()
{
    return new CHook;
}

#endif