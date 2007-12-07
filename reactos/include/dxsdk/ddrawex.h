#ifndef __DDRAWEXH__
#define __DDRAWEXH__

#ifdef __cplusplus
extern "C" {
#endif

#include <ddraw.h>
DEFINE_GUID(CLSID_DirectDrawFactory,            0x4FD2A832, 0x86C8, 0x11D0, 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);
DEFINE_GUID(IID_IDirectDrawFactory,             0x4FD2A833, 0x86C8, 0x11D0, 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);
DEFINE_GUID(IID_IDirectDraw3,                   0x618F8AD4, 0x8B7A, 0x11D0, 0x8F, 0xCC, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D);

#define DDSCAPS_DATAEXCHANGE                    (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)
#define DDERR_LOADFAILED                        MAKE_DDHRESULT( 901 )
#define DDERR_BADVERSIONINFO                    MAKE_DDHRESULT( 902 )
#define DDERR_BADPROCADDRESS                    MAKE_DDHRESULT( 903 )
#define DDERR_LEGACYUSAGE                       MAKE_DDHRESULT( 904 )


#ifdef __cplusplus
}
#endif
#endif
