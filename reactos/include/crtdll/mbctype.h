#ifndef _MBCTYPE_H
#define _MBCTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#define _MS	0x01
#define _MP	0x02
#define _M1	0x04
#define _M2	0x08

#define _MBC_SINGLE	 0	
#define _MBC_LEAD	 1	
#define _MBC_TRAIL	 2		
#define _MBC_ILLEGAL	-1		

#define _MB_CP_SBCS      0
#define _MB_CP_OEM      -2
#define _MB_CP_ANSI     -3
#define _MB_CP_LOCALE   -4

#ifdef __cplusplus
}
#endif

#endif