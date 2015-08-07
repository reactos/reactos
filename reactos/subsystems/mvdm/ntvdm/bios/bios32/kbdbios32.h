/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            kbdbios32.h
 * PURPOSE:         VDM 32-bit PS/2 Keyboard BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _KBDBIOS32_H_
#define _KBDBIOS32_H_

/* DEFINES ********************************************************************/

// #define BIOS_KBD_INTERRUPT      0x16

#define BIOS_KBD_BUFFER_SIZE    16

#define BDA_KBDFLAG_RSHIFT      (1 << 0)
#define BDA_KBDFLAG_LSHIFT      (1 << 1)
#define BDA_KBDFLAG_CTRL        (1 << 2)
#define BDA_KBDFLAG_RCTRL       (1 << 2)    // Used for KeybdStatusFlags
#define BDA_KBDFLAG_ALT         (1 << 3)
#define BDA_KBDFLAG_RALT        (1 << 3)    // Used for KeybdStatusFlags
#define BDA_KBDFLAG_SCROLL_ON   (1 << 4)
#define BDA_KBDFLAG_NUMLOCK_ON  (1 << 5)
#define BDA_KBDFLAG_CAPSLOCK_ON (1 << 6)
#define BDA_KBDFLAG_INSERT_ON   (1 << 7)
#define BDA_KBDFLAG_LCTRL       (1 << 8)
#define BDA_KBDFLAG_LALT        (1 << 9)
#define BDA_KBDFLAG_SYSRQ       (1 << 10)
#define BDA_KBDFLAG_PAUSE       (1 << 11)
#define BDA_KBDFLAG_SCROLL      (1 << 12)
#define BDA_KBDFLAG_NUMLOCK     (1 << 13)
#define BDA_KBDFLAG_CAPSLOCK    (1 << 14)
#define BDA_KBDFLAG_INSERT      (1 << 15)

/* FUNCTIONS ******************************************************************/

VOID KbdBios32Post(VOID);

#endif /* _KBDBIOS32_H_ */

/* EOF */
