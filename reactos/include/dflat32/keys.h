/* ----------- keys.h ------------ */

#ifndef KEYS_H
#define KEYS_H

#define OFFSET 0x1000

#define RUBOUT        8			/* BACHSPACE KEY */
#define BELL          7
#define ESC          27
#define ALT_BS      (197+OFFSET)
#define ALT_DEL     (184+OFFSET)
#define SHIFT_DEL   (198+OFFSET)
#define CTRL_INS    (186+OFFSET)
#define SHIFT_INS   (185+OFFSET)
#define SHIFT_F8    (219+OFFSET)
#define F1          (187+OFFSET)
#define F2          (188+OFFSET)
#define F3          (189+OFFSET)
#define F4          (190+OFFSET)
#define F5          (191+OFFSET)
#define F6          (192+OFFSET)
#define F7          (193+OFFSET)
#define F8          (194+OFFSET)
#define F9          (195+OFFSET)
#define F10         (196+OFFSET)
#define CTRL_F1     (222+OFFSET)
#define CTRL_F2     (223+OFFSET)
#define CTRL_F3     (224+OFFSET)
#define CTRL_F4     (225+OFFSET)
#define CTRL_F5     (226+OFFSET)
#define CTRL_F6     (227+OFFSET)
#define CTRL_F7     (228+OFFSET)
#define CTRL_F8     (229+OFFSET)
#define CTRL_F9     (230+OFFSET)
#define CTRL_F10    (231+OFFSET)
#define ALT_F1      (232+OFFSET)
#define ALT_F2      (233+OFFSET)
#define ALT_F3      (234+OFFSET)
#define ALT_F4      (235+OFFSET)
#define ALT_F5      (236+OFFSET)
#define ALT_F6      (237+OFFSET)
#define ALT_F7      (238+OFFSET)
#define ALT_F8      (239+OFFSET)
#define ALT_F9      (240+OFFSET)
#define ALT_F10     (241+OFFSET)
#define HOME        (199+OFFSET)
#define UP          (200+OFFSET)
#define PGUP        (201+OFFSET)
#define BS          (203+OFFSET)	/* CURSOR LEFT KEY */
#define FWD         (205+OFFSET)	/* CURSOR RIGHT KEY */
#define END         (207+OFFSET)
#define DN          (208+OFFSET)
#define PGDN        (209+OFFSET)
#define INS         (210+OFFSET)
#define DEL         (211+OFFSET)
#define CTRL_HOME   (247+OFFSET)
#define CTRL_PGUP   (132+OFFSET)
#define CTRL_BS     (243+OFFSET)
#define CTRL_FIVE   (143+OFFSET)
#define CTRL_FWD    (244+OFFSET)
#define CTRL_END    (245+OFFSET)
#define CTRL_PGDN   (246+OFFSET)
#define SHIFT_HT    (143+OFFSET)
#define ALT_A       (158+OFFSET)
#define ALT_B       (176+OFFSET)
#define ALT_C       (174+OFFSET)
#define ALT_D       (160+OFFSET)
#define ALT_E       (146+OFFSET)
#define ALT_F       (161+OFFSET)
#define ALT_G       (162+OFFSET)
#define ALT_H       (163+OFFSET)
#define ALT_I       (151+OFFSET)
#define ALT_J       (164+OFFSET)
#define ALT_K       (165+OFFSET)
#define ALT_L       (166+OFFSET)
#define ALT_M       (178+OFFSET)
#define ALT_N       (177+OFFSET)
#define ALT_O       (152+OFFSET)
#define ALT_P       (153+OFFSET)
#define ALT_Q       (144+OFFSET)
#define ALT_R       (147+OFFSET)
#define ALT_S       (159+OFFSET)
#define ALT_T       (148+OFFSET)
#define ALT_U       (150+OFFSET)
#define ALT_V       (175+OFFSET)
#define ALT_W       (145+OFFSET)
#define ALT_X       (173+OFFSET)
#define ALT_Y       (149+OFFSET)
#define ALT_Z       (172+OFFSET)
#define ALT_1      (0xf8+OFFSET)
#define ALT_2      (0xf9+OFFSET)
#define ALT_3      (0xfa+OFFSET)
#define ALT_4      (0xfb+OFFSET)
#define ALT_5      (0xfc+OFFSET)
#define ALT_6      (0xfd+OFFSET)
#define ALT_7      (0xfe+OFFSET)
#define ALT_8      (0xff+OFFSET)
#define ALT_9      (0x80+OFFSET)
#define ALT_0      (0x81+OFFSET)
#define ALT_HYPHEN  (130+OFFSET)

#define RIGHTSHIFT 0x01
#define LEFTSHIFT  0x02
#define CTRLKEY    0x04
#define ALTKEY     0x08
#define SCROLLLOCK 0x10
#define NUMLOCK    0x20
#define CAPSLOCK   0x40
#define INSERTKEY  0x80

struct keys {
    int keycode;
    char *keylabel;
};
extern struct keys keys[];

#endif

