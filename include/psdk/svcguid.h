#ifndef _SVCGUID_H
#define _SVCGUID_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SVCID_NETWARE(_SapId) \
	{ (0x000B << 16) | (_SapId), 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } }

#define SAPID_FROM_SVCID_NETWARE(_g) \
	((WORD)(_g->Data1 & 0xFFFF))

#define SET_NETWARE_SVCID(_g,_SapId) { \
	(_g)->Data1 = (0x000B << 16 ) | (_SapId); \
	(_g)->Data2 = 0; \
	(_g)->Data3 = 0; \
	(_g)->Data4[0] = 0xC0; \
	(_g)->Data4[1] = 0x0; \
	(_g)->Data4[2] = 0x0; \
	(_g)->Data4[3] = 0x0; \
	(_g)->Data4[4] = 0x0; \
	(_g)->Data4[5] = 0x0; \
	(_g)->Data4[6] = 0x0; \
	(_g)->Data4[7] = 0x46; }

#ifdef __cplusplus
}
#endif
#endif
