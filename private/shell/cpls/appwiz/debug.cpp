#include "priv.h"

// These have to come before dump.h otherwise dump.h won't declare
// some of the prototypes.  Failure to do this will cause the compiler
// to decorate some of these functions with C++ mangling.

#include "datasrc.h"        // for LOAD_STATE
#include "simpdata.h"       // for OSPRW


#include "dump.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "appwiz.ini"
#define SZ_DEBUGSECTION     "appwiz"
#define SZ_MODULE           "APPWIZ"
#define DECLARE_DEBUG
#include <debug.h>


#ifdef DEBUG

LPCTSTR Dbg_GetReadyState(LONG state)
{
    LPCTSTR pcsz = TEXT("<Unknown READYSTATE>");
    
    switch (state)
    {
    STRING_CASE(READYSTATE_UNINITIALIZED);
    STRING_CASE(READYSTATE_LOADING);
    STRING_CASE(READYSTATE_LOADED);
    STRING_CASE(READYSTATE_INTERACTIVE);
    STRING_CASE(READYSTATE_COMPLETE);
    }

    ASSERT(pcsz);

    return pcsz;
}


// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM
LPCTSTR Dbg_GetGuid(REFGUID rguid, LPTSTR pszBuf, int cch)
{
    SHStringFromGUID(rguid, pszBuf, cch);
    return pszBuf;
}
#endif //DOWNLEVEL_PLATFORM

LPCTSTR Dbg_GetBool(BOOL bVal)
{
    return bVal ? TEXT("TRUE") : TEXT("FALSE");
}


LPCTSTR Dbg_GetOSPRW(OSPRW state)
{
    LPCTSTR pcsz = TEXT("<Unknown OSPRW>");
    
    switch (state)
    {
    STRING_CASE(OSPRW_MIXED);
    STRING_CASE(OSPRW_READONLY);
    STRING_CASE(OSPRW_READWRITE);
    }

    ASSERT(pcsz);

    return pcsz;
}


LPCTSTR Dbg_GetLS(LOAD_STATE state)
{
    LPCTSTR pcsz = TEXT("<Unknown LOAD_STATE>");
    
    switch (state)
    {
    STRING_CASE(LS_NOTSTARTED);
    STRING_CASE(LS_LOADING_SLOWINFO);
    STRING_CASE(LS_DONE);
    }

    ASSERT(pcsz);

    return pcsz;
}


LPCTSTR Dbg_GetAppCmd(APPCMD appcmd)
{
    LPCTSTR pcsz = TEXT("<Unknown APPCMD>");
    
    switch (appcmd)
    {
    STRING_CASE(APPCMD_UNKNOWN);
    STRING_CASE(APPCMD_INSTALL);
    STRING_CASE(APPCMD_UNINSTALL);
    STRING_CASE(APPCMD_REPAIR);
    STRING_CASE(APPCMD_UPGRADE);
    STRING_CASE(APPCMD_MODIFY);
    STRING_CASE(APPCMD_GENERICINSTALL);
    }

    ASSERT(pcsz);

    return pcsz;
}

#endif // DEBUG
