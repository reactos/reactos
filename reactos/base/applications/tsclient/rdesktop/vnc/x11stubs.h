#ifndef NOXKEYMAP_H
#define NOXKEYMAP_H

/* Fake a few X11 calls */

#define XK_MISCELLANY
#include <rfb/rfb.h>
#include <rfb/keysym.h>

#define NoSymbol 0L
#define ShiftMask               (1<<0)
#define LockMask                (1<<1)
#define ControlMask             (1<<2)
#define Mod1Mask                (1<<3)
#define Mod2Mask                (1<<4)
#define Mod3Mask                (1<<5)
#define Mod4Mask                (1<<6)
#define Mod5Mask                (1<<7)
#define Button1                 1
#define Button2                 2
#define Button3                 3
#define Button4                 4
#define Button5                 5

typedef int Display;
typedef int Window;
typedef rfbKeySym KeySym;

KeySym XStringToKeysym(const char *str);
const char *XKeysymToString(KeySym keysym);
void XDisplayKeycodes(Display * display, int *min_keycode, int *max_keycode);

#endif
