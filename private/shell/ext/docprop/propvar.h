
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
//  File:       propvar.h
//
//  Purpose:    Prototypes, constants, and macros relating to
//              PropVariants in the Office code.
//
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


#ifndef _PROPVAR_H_
#define _PROPVAR_H_


//  ----------
//  Prototypes
//  ----------

BOOL FPropVarLoad (LPPROPVARIANT lppropvar, VARTYPE vt, LPVOID const lpv );
void VPropVarMove (LPPROPVARIANT lppropvarDest, LPPROPVARIANT const lppropvarSource);
BOOL FCoStrToWStr (LPWSTR * lplpwstr, const LPSTR lpstr, UINT  uCodePage);
BOOL FCoWStrToStr (LPSTR * lplpstr, const LPWSTR lpwstr, UINT uCodePage);
BOOL FPropVarConvertString (LPPROPVARIANT lppropvarDest, const LPPROPVARIANT lppropvarSource, UINT uCodePage);

BOOL FPropVarCopyToBuf (LPPROPVARIANT const lppropvar, DWORD cbMax, LPVOID lpBuf);

//  ---------
//  Constants
//  ---------

// Default size of PropVariant/PropSpec arrays.

#define DEFAULT_IPROPERTY_COUNT         10

//  ------
//  Macros
//  ------

// Macro to see if a PropVariant is some kind of string.

#define IS_PROPVAR_STRING( lppropvar )                        \
        ( ( (lppropvar)->vt & ~VT_VECTOR ) == VT_LPSTR      \
          ||                                                \
          ( (lppropvar)->vt & ~VT_VECTOR ) == VT_LPWSTR )   \

// Macro to see if a VT is valid in the context
// of the User-Defined property set.

#define ISUDTYPE(vt)        \
        ( vt == VT_LPSTR    \
          ||                \
          vt == VT_LPWSTR   \
          ||                \
          vt == VT_I4       \
          ||                \
          vt == VT_R8       \
          ||                \
          vt == VT_FILETIME \
          ||                \
          vt == VT_BOOL )

// Macro to determine if a string is represented
// differently (in terms of the code-page)
// in memory than it is in the Property
// set.  The codepage parameter is that of the
// property set.

#ifdef UNICODE

#define PROPVAR_STRING_CONVERSION_REQUIRED(lppropvar,codepage)  \
            ( IS_PROPVAR_STRING( lppropvar )                    \
              &&                                                \
              ((codepage) != CP_WINUNICODE)                     \
            )

#else // UNICODE

#define PROPVAR_STRING_CONVERSION_REQUIRED(lppropvar,codepage)  \
            ( IS_PROPVAR_STRING( lppropvar )                    \
              &&                                                \
              ((codepage) == CP_WINUNICODE)                     \
            )

#endif // UNICODE
    
#endif _PROPVAR_H_
