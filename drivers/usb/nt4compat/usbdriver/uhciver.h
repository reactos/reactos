#ifndef __UHCIVER_H__
#define __UHCIVER_H__

//#define _MULTI_UHCI
//#define _TIANSHENG_DRIVER

#ifdef _MULTI_UHCI
#define UHCI_VER_STR "m564.a\0"
#else
#define UHCI_VER_STR "0564.d\0"
#endif

#endif
