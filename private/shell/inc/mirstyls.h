
#ifndef _MIRSTYLS_H_
#define _MIRSTYLS_H_

#define PrivateWS_EX_LAYOUTRTL         0x00400000
#define PrivateWS_EX_NOINHERITLAYOUT   0x00100000

#if (WINVER >= 0x0500)
#if WS_EX_LAYOUTRTL != PrivateWS_EX_LAYOUTRTL
#error inconsistant WS_EX_LAYOUTRTL in winuser.h
#endif
#if WS_EX_NOINHERITLAYOUT != PrivateWS_EX_NOINHERITLAYOUT
#error inconsistant WS_EX_NOINHERITLAYOUT in winuser.h
#endif
#else
#define WS_EX_LAYOUTRTL          PrivateWS_EX_LAYOUTRTL
#define WS_EX_NOINHERITLAYOUT    PrivateWS_EX_NOINHERITLAYOUT
#endif

#endif
