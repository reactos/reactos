/* ----------- keys.h ------------ */

#ifndef KEYS_H
#define KEYS_H

#define FKEY 0x1000	/* offset for non-ASCII keys */

#define BELL          7 /* no scancode  */
#define BS            8 /* scancode: 14 */ /* backspace / rubout */
#define TAB           9	/* scancode: 15 */
#define ESC          27	/* scancode:  1 */

#define F1          (FKEY+0x3b) /* scancode: 0x3b */
#define F2          (FKEY+0x3c)
#define F3          (FKEY+0x3d)
#define F4          (FKEY+0x3e)
#define F5          (FKEY+0x3f)
#define F6          (FKEY+0x40)
#define F7          (FKEY+0x41)
#define F8          (FKEY+0x42)
#define F9          (FKEY+0x43)
#define F10         (FKEY+0x44)

#define CTRL_F1     (FKEY+94)
#define CTRL_F2     (FKEY+95)
#define CTRL_F3     (FKEY+96)
#define CTRL_F4     (FKEY+97)
#define CTRL_F5     (FKEY+98)
#define CTRL_F6     (FKEY+99)
#define CTRL_F7     (FKEY+100)
#define CTRL_F8     (FKEY+101)
#define CTRL_F9     (FKEY+102)
#define CTRL_F10    (FKEY+103)

#define ALT_F1      (FKEY+104)
#define ALT_F2      (FKEY+105)
#define ALT_F3      (FKEY+106)
#define ALT_F4      (FKEY+107)
#define ALT_F5      (FKEY+108)
#define ALT_F6      (FKEY+109)
#define ALT_F7      (FKEY+110)
#define ALT_F8      (FKEY+111)
#define ALT_F9      (FKEY+112)
#define ALT_F10     (FKEY+113)

#define HOME        (FKEY+0x47) /* scancode: 0x47 */
#define UP          (FKEY+0x48)
#define PGUP        (FKEY+0x49)
/* 4a: grey- 4b: left 4c: keypad5 4d: right 4e: grey+ */
#define END         (FKEY+0x4f)
#define DN          (FKEY+0x50)
#define PGDN        (FKEY+0x51)
#define INS         (FKEY+0x52)
#define DEL         (FKEY+0x53)

#define LARROW      (FKEY+0x4b)
#define FWD         (FKEY+0x4d)

/* valid in ANSI, so assuming that those are universal: */
#define CTRL_END    (FKEY+117)
#define CTRL_PGDN   (FKEY+118)
#define CTRL_HOME   (FKEY+119)
#define CTRL_PGUP   (FKEY+132)

/* #define CTRL_FIVE   (143) */ /* ctrl-numeric-keypad-5 */
#define CTRL_LARROW (FKEY+0x73)	/* ctrl-leftarrow */
#define CTRL_RARROW (FKEY+0x74)	/* ctrl-rightarrow */

#define CTRL_BS     (127)	/* yet another deletion keystroke */
#define SHIFT_HT    (FKEY+0x0f) /* scancode: 0x0f */
#define ALT_HYPHEN  (130)

#define ALT_BS      CTRL_Z /* undo block removal */
#define CTRL_INS    CTRL_C /* clipboard copy  */
#define SHIFT_DEL   CTRL_X /* clipboard cut   */
#define SHIFT_INS   CTRL_V /* clipboard paste */

/* Stupid...those depend on keyboard layout! */
#define ALT_A       (FKEY+0x1e) /* scancode 0x1e */
#define ALT_S       (FKEY+0x1f)
#define ALT_D       (FKEY+0x20)
#define ALT_F       (FKEY+0x21)
#define ALT_G       (FKEY+0x22)
#define ALT_H       (FKEY+0x23)
#define ALT_J       (FKEY+0x24)
#define ALT_K       (FKEY+0x25)
#define ALT_L       (FKEY+0x26)
#define ALT_Q       (FKEY+0x10)
#define ALT_W       (FKEY+0x11)
#define ALT_E       (FKEY+0x12)
#define ALT_R       (FKEY+0x13)
#define ALT_T       (FKEY+0x14)
#define ALT_Y       (FKEY+0x15)
#define ALT_U       (FKEY+0x16)
#define ALT_I       (FKEY+0x17)
#define ALT_O       (FKEY+0x18)
#define ALT_P       (FKEY+0x19)
#define ALT_Z       (FKEY+0x2c)
#define ALT_X       (FKEY+0x2d)
#define ALT_C       (FKEY+0x2e)
#define ALT_B       (FKEY+0x2f)
#define ALT_V       (FKEY+0x30)
#define ALT_N       (FKEY+0x31)
#define ALT_M       (FKEY+0x32)
#define ALT_1       (FKEY+0x78) /* 120 */
#define ALT_2       (FKEY+0x79)
#define ALT_3       (FKEY+0x7a)
#define ALT_4       (FKEY+0x7b)
#define ALT_5       (FKEY+0x7c)
#define ALT_6       (FKEY+0x7d)
#define ALT_7       (FKEY+0x7e)
#define ALT_8       (FKEY+0x7f)
#define ALT_9       (FKEY+0x80)
#define ALT_0       (FKEY+0x81)

/* Those are values that are at least typical for DOS: */
#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3	/* must have "ignore ^C / ^Break handler to use this */
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 10         /* (special meaning for DOS-CON readline?) */
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 6
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16	/* (causes print in DOS-CON) */
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19	/* (causes scroll-halt in DOS-CON) */
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26	/* (marks EOF in DOS-CON) */

/* Shift bit mask */
#define RIGHTSHIFT 0x01
#define LEFTSHIFT  0x02
#define CTRLKEY    0x04
#define ALTKEY     0x08
#define SCROLLLOCK 0x10
#define NUMLOCK    0x20
#define CAPSLOCK   0x40 /* caps lock BEING on */
#define INSERTKEY  0x80

/* Following is new by Eric 11/2002, but see CONSOLE.C */
#define SYSRQKEY   0x8000
#define CAPSLKEY   0x4000 /* PRESSING caps lock */
#define NUMLKEY    0x2000
#define SCROLLLKEY 0x1000
/* Especially the L/R distinction is important - Eric */
#define RALTKEY    0x800 /* treat this als AltGr, which is NOT Alt */
#define RCTRLKEY   0x400
#define LALTKEY    0x200
#define LCTRLKEY   0x100

struct keys {
    int keycode;
    char *keylabel;
};
extern struct keys keys[];

#endif
