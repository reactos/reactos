/* ----------- keys.h ------------ */

#ifndef KEYS_H
#define KEYS_H

#define DF_OFFSET 0x1000

#define DF_RUBOUT        8			/* BACHSPACE KEY */
#define DF_BELL          7
#define DF_ESC          27
#define DF_ALT_BS      (197+DF_OFFSET)
#define DF_ALT_DEL     (184+DF_OFFSET)
#define DF_SHIFT_DEL   (198+DF_OFFSET)
#define DF_CTRL_INS    (186+DF_OFFSET)
#define DF_SHIFT_INS   (185+DF_OFFSET)
#define DF_SHIFT_F8    (219+DF_OFFSET)
#define DF_F1          (187+DF_OFFSET)
#define DF_F2          (188+DF_OFFSET)
#define DF_F3          (189+DF_OFFSET)
#define DF_F4          (190+DF_OFFSET)
#define DF_F5          (191+DF_OFFSET)
#define DF_F6          (192+DF_OFFSET)
#define DF_F7          (193+DF_OFFSET)
#define DF_F8          (194+DF_OFFSET)
#define DF_F9          (195+DF_OFFSET)
#define DF_F10         (196+DF_OFFSET)
#define DF_CTRL_F1     (222+DF_OFFSET)
#define DF_CTRL_F2     (223+DF_OFFSET)
#define DF_CTRL_F3     (224+DF_OFFSET)
#define DF_CTRL_F4     (225+DF_OFFSET)
#define DF_CTRL_F5     (226+DF_OFFSET)
#define DF_CTRL_F6     (227+DF_OFFSET)
#define DF_CTRL_F7     (228+DF_OFFSET)
#define DF_CTRL_F8     (229+DF_OFFSET)
#define DF_CTRL_F9     (230+DF_OFFSET)
#define DF_CTRL_F10    (231+DF_OFFSET)
#define DF_ALT_F1      (232+DF_OFFSET)
#define DF_ALT_F2      (233+DF_OFFSET)
#define DF_ALT_F3      (234+DF_OFFSET)
#define DF_ALT_F4      (235+DF_OFFSET)
#define DF_ALT_F5      (236+DF_OFFSET)
#define DF_ALT_F6      (237+DF_OFFSET)
#define DF_ALT_F7      (238+DF_OFFSET)
#define DF_ALT_F8      (239+DF_OFFSET)
#define DF_ALT_F9      (240+DF_OFFSET)
#define DF_ALT_F10     (241+DF_OFFSET)
#define DF_HOME        (199+DF_OFFSET)
#define DF_UP          (200+DF_OFFSET)
#define DF_PGUP        (201+DF_OFFSET)
#define DF_BS          (203+DF_OFFSET)	/* CURSOR LEFT KEY */
#define DF_FWD         (205+DF_OFFSET)	/* CURSOR RIGHT KEY */
#define DF_END         (207+DF_OFFSET)
#define DF_DN          (208+DF_OFFSET)
#define DF_PGDN        (209+DF_OFFSET)
#define DF_INS         (210+DF_OFFSET)
#define DF_DEL         (211+DF_OFFSET)
#define DF_CTRL_HOME   (247+DF_OFFSET)
#define DF_CTRL_PGUP   (132+DF_OFFSET)
#define DF_CTRL_BS     (243+DF_OFFSET)
#define DF_CTRL_FIVE   (143+DF_OFFSET)
#define DF_CTRL_FWD    (244+DF_OFFSET)
#define DF_CTRL_END    (245+DF_OFFSET)
#define DF_CTRL_PGDN   (246+DF_OFFSET)
#define DF_SHIFT_HT    (143+DF_OFFSET)
#define DF_ALT_A       (158+DF_OFFSET)
#define DF_ALT_B       (176+DF_OFFSET)
#define DF_ALT_C       (174+DF_OFFSET)
#define DF_ALT_D       (160+DF_OFFSET)
#define DF_ALT_E       (146+DF_OFFSET)
#define DF_ALT_F       (161+DF_OFFSET)
#define DF_ALT_G       (162+DF_OFFSET)
#define DF_ALT_H       (163+DF_OFFSET)
#define DF_ALT_I       (151+DF_OFFSET)
#define DF_ALT_J       (164+DF_OFFSET)
#define DF_ALT_K       (165+DF_OFFSET)
#define DF_ALT_L       (166+DF_OFFSET)
#define DF_ALT_M       (178+DF_OFFSET)
#define DF_ALT_N       (177+DF_OFFSET)
#define DF_ALT_O       (152+DF_OFFSET)
#define DF_ALT_P       (153+DF_OFFSET)
#define DF_ALT_Q       (144+DF_OFFSET)
#define DF_ALT_R       (147+DF_OFFSET)
#define DF_ALT_S       (159+DF_OFFSET)
#define DF_ALT_T       (148+DF_OFFSET)
#define DF_ALT_U       (150+DF_OFFSET)
#define DF_ALT_V       (175+DF_OFFSET)
#define DF_ALT_W       (145+DF_OFFSET)
#define DF_ALT_X       (173+DF_OFFSET)
#define DF_ALT_Y       (149+DF_OFFSET)
#define DF_ALT_Z       (172+DF_OFFSET)
#define DF_ALT_1      (0xf8+DF_OFFSET)
#define DF_ALT_2      (0xf9+DF_OFFSET)
#define DF_ALT_3      (0xfa+DF_OFFSET)
#define DF_ALT_4      (0xfb+DF_OFFSET)
#define DF_ALT_5      (0xfc+DF_OFFSET)
#define DF_ALT_6      (0xfd+DF_OFFSET)
#define DF_ALT_7      (0xfe+DF_OFFSET)
#define DF_ALT_8      (0xff+DF_OFFSET)
#define DF_ALT_9      (0x80+DF_OFFSET)
#define DF_ALT_0      (0x81+DF_OFFSET)
#define DF_ALT_HYPHEN  (130+DF_OFFSET)

#define DF_RIGHTSHIFT 0x01
#define DF_LEFTSHIFT  0x02
#define DF_CTRLKEY    0x04
#define DF_ALTKEY     0x08
#define DF_SCROLLLOCK 0x10
#define DF_NUMLOCK    0x20
#define DF_CAPSLOCK   0x40
#define DF_INSERTKEY  0x80

struct DfKeys {
    int keycode;
    char *keylabel;
};
extern struct DfKeys keys[];

#endif

