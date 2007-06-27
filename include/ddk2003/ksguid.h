
#define INITGUID
#include <guiddef.h>

#if defined( DEFINE_GUIDEX )
    #undef DEFINE_GUIDEX
#endif

#define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID __declspec(selectany) name = { STATICGUIDOF(name) }

#ifndef STATICGUIDOF
    #define STATICGUIDOF(guid) STATIC_##guid
#endif

#if !defined( DEFINE_WAVEFORMATEX_GUID )
#define DEFINE_WAVEFORMATEX_GUID(x) (USHORT)(x), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#endif

