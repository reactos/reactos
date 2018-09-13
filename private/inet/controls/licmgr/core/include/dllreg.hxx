//+----------------------------------------------------------------------------
//  File:       dllreg.hxx
//
//  Synopsis:   This file contains the routines for managing the registry
//
//-----------------------------------------------------------------------------


#ifndef _DLLREG_HXX
#define _DLLREG_HXX

// Types ----------------------------------------------------------------------
#define szMODULE_PATH       _T("<m>")
#define szDEFAULT_SECTION   _T("D")
#define chDEFAULT_SECTION   _T('D')
#define szVALUES_SECTION    _T("V")
#define chVALUES_SECTION    _T('V')
#define szSUBKEY_SECTION    _T("K")
#define chSUBKEY_SECTION    _T('K')
#define szEND_SECTION       _T("\0")
#define chEND_SECTION       _T('\0')

#define DEFINE_REGISTRY_SECKEY(name, section, key)  \
extern const TCHAR g_sz##name##RegistryKey[] =      \
                {                   \
                _T(#section)        \
                _T("\0")            \
                _T(#key)            \
                _T("\0")

#define DEFINE_REGISTRY_KEY(name, key)          \
extern const TCHAR g_sz##name##RegistryKey[] =  \
                {                   \
                _T("\0")            \
                _T(#key)            \
                _T("\0")

#define DEFAULT_VALUE(value)        \
                szDEFAULT_SECTION   \
                _T(#value)          \
                _T("\0")

#define BEGIN_NAMED_VALUES          \
                szVALUES_SECTION

#define NAMED_VALUE(name, value)    \
                _T(#name)           \
                _T("\0")            \
                _T(#value)          \
                _T("\0")

#define END_NAMED_VALUES            \
                szEND_SECTION

#define BEGIN_SUBKEY(key)           \
                szSUBKEY_SECTION    \
                _T(#key)            \
                _T("\0")

#define END_SUBKEY                  \
                szEND_SECTION

#define END_REGISTRY_KEY            \
                szEND_SECTION       \
                };

#define BEGIN_REGISTRY_KEYS         \
extern const TCHAR * g_aszKeys[] =  \
                {

#define REGISTRY_KEY(key)           \
                g_sz##key##RegistryKey,

#define END_REGISTRY_KEYS           \
                NULL                \
                };

extern const TCHAR * g_aszKeys[];


#endif  // _DLLREG_HXX
