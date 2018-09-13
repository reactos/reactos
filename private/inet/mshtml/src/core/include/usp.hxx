#ifndef I_USP_HXX_
#define I_USP_HXX_
#pragma INCMSG("--- Beg 'usp.hxx'")

//+------------------------------------------------------------------------
//
// wrapper function declarations for usp.cxx
// these functions wrap the growth of CDataAry parameters
//
//-------------------------------------------------------------------------

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#pragma INCMSG("--- Beg <lsdefs.h>")
#include <lsdefs.h>  // dependency created when some of our objects are defined
#pragma INCMSG("--- End <lsdefs.h>")
#endif

#ifndef X_USP10_H_
#define X_USP10_H_
#pragma INCMSG("--- Beg <usp10.h>")
#include <usp10.h>
#pragma INCMSG("--- End <usp10.h>")
#endif

#define STACK_ALLOC_RUNS 16
#define USP10_NOT_FOUND  0x80070002

HRESULT WINAPI ScriptItemize(
    PCWSTR                  pwcInChars,     // In   Unicode string to be itemized
    int                     cInChars,       // In   Character count to itemize
    int                     cItemGrow,      // In   Items to grow by if paryItems is too small
    const SCRIPT_CONTROL   *psControl,      // In   Analysis control (optional)
    const SCRIPT_STATE     *psState,        // In   Initial bidi algorithm state (optional)
    CDataAry<SCRIPT_ITEM>  *paryItems,      // Out  Array to receive itemization
    PINT                    pcItems );      // Out  Count of items processed    HRESULT hr;

const SCRIPT_PROPERTIES * GetScriptProperties(WORD eScript);
const WORD GetNumericScript(DWORD lang);

inline BOOL IsComplexScript(
    WORD eScript)
{
    return GetScriptProperties(eScript)->fComplex;
}

inline BOOL IsNumericScript(
    WORD eScript)
{
    return GetScriptProperties(eScript)->fNumeric;
}

inline BOOL NeedWordBreak(
    WORD eScript)
{
    return GetScriptProperties(eScript)->fNeedsWordBreaking;
}

inline BOOL GetScriptCharSet(
    WORD eScript)
{
    return GetScriptProperties(eScript)->bCharSet;
}

#pragma INCMSG("--- End 'usp.hxx'")
#else
#pragma INCMSG("*** Dup 'usp.hxx'")
#endif
