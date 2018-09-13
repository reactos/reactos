/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWKBD.H
 *  16-bit Keyboard API argument structures
 *
 *  History:
 *  Created 02-Feb-1991 by Jeff Parsons (jeffpar)
--*/


/* Keyboard API IDs
 */
#define FUN_ANSITOOEM           5   //
#define FUN_ANSITOOEMBUFF       134 //
#define FUN_DISABLE         3   // Internal
#define FUN_ENABLE          2   // Internal
#define FUN_ENABLEKBSYSREQ      136 // Internal
#define FUN_GETKBCODEPAGE       132 //
#define FUN_GETKEYBOARDTYPE     130 //
#define FUN_GETKEYNAMETEXT      133 //
#define FUN_GETTABLESEG         126 // Internal
#define FUN_INQUIRE         1   // Internal
#define FUN_KEYBOARD_WEP        0   // Export by name
#define FUN_MAPVIRTUALKEY       131 //
#define FUN_NEWTABLE            127 // Internal
#define FUN_OEMKEYSCAN          128 //
#define FUN_OEMTOANSI           6   //
#define FUN_OEMTOANSIBUFF       135 //
#define FUN_SCREENSWITCHENABLE      100 // Internal
#define FUN_SETSPEED            7   // Internal
#define FUN_TOASCII         4   //
#define FUN_VKKEYSCAN           129 //
#define FUN_GETBIOSKEYPROC      137 //


/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _ANSITOOEM16 {           /* kb5 */
    VPSTR f2;
    VPSTR f1;
} ANSITOOEM16;
typedef ANSITOOEM16 UNALIGNED *PANSITOOEM16;

typedef struct _ANSITOOEMBUFF16 {       /* kb134 */
    SHORT f3;
    VPSTR f2;
    VPSTR f1;
} ANSITOOEMBUFF16;
typedef ANSITOOEMBUFF16 UNALIGNED *PANSITOOEMBUFF16;

#ifdef NULLSTRUCT
typedef struct _GETKBCODEPAGE16 {       /* kb132 */
} GETKBCODEPAGE16;
typedef GETKBCODEPAGE16 UNALIGNED *PGETKBCODEPAGE16;
#endif

typedef struct _GETKEYBOARDTYPE16 {     /* kb130 */
    SHORT f1;
} GETKEYBOARDTYPE16;
typedef GETKEYBOARDTYPE16 UNALIGNED *PGETKEYBOARDTYPE16;

typedef struct _GETKEYNAMETEXT16 {      /* kb133 */
    SHORT f3;
    VPSTR f2;
    LONG f1;
} GETKEYNAMETEXT16;
typedef GETKEYNAMETEXT16 UNALIGNED *PGETKEYNAMETEXT16;

typedef struct _MAPVIRTUALKEY16 {       /* kb131 */
    WORD f2;
    WORD f1;
} MAPVIRTUALKEY16;
typedef MAPVIRTUALKEY16 UNALIGNED *PMAPVIRTUALKEY16;

typedef struct _OEMKEYSCAN16 {          /* kb128 */
    WORD f1;
} OEMKEYSCAN16;
typedef OEMKEYSCAN16 UNALIGNED *POEMKEYSCAN16;

typedef struct _OEMTOANSI16 {           /* kb6 */
    VPSTR f2;
    VPSTR f1;
} OEMTOANSI16;
typedef OEMTOANSI16 UNALIGNED *POEMTOANSI16;

typedef struct _OEMTOANSIBUFF16 {       /* kb135 */
    SHORT f3;
    VPSTR f2;
    VPSTR f1;
} OEMTOANSIBUFF16;
typedef OEMTOANSIBUFF16 UNALIGNED *POEMTOANSIBUFF16;

typedef struct _TOASCII16 {         /* kb4 */
    WORD f5;
    VPVOID f4;
    VPSTR f3;
    WORD f2;
    WORD f1;
} TOASCII16;
typedef TOASCII16 UNALIGNED *PTOASCII16;

typedef struct _VKKEYSCAN16 {           /* kb129 */
    WORD f1;
} VKKEYSCAN16;
typedef VKKEYSCAN16 UNALIGNED *PVKKEYSCAN16;

/* XLATOFF */
#pragma pack()
/* XLATON */
