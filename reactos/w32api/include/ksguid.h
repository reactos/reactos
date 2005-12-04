
#ifndef __KSGUID__
#define __KSGUID__

#define INITGUID
#include <guiddef.h>

#if defined( DEFINE_GUIDEX )
             #undef DEFINE_GUIDEX
#endif

#if !defined( DEFINE_WAVEFORMATEX_GUID )
              #define DEFINE_WAVEFORMATEX_GUID(guid_id) (USHORT)(guid_id), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#endif

#ifndef STATICGUIDOF
                    #define STATICGUIDOF(guids) STATIC_##guids
#endif 

#define DEFINE_GUIDEX(Name) EXTERN_C const CDECL GUID __declspec(selectany) Name = { STATIC_##Name }

#endif
