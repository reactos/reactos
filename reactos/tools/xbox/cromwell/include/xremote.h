#ifndef _XREMOTE_H_
#define _XREMOTE_H_

#define RC_KEY_SELECT			0x0b
#define RC_KEY_UP				0xa6
#define RC_KEY_DOWN				0xa7
#define RC_KEY_RIGHT			0xa8
#define RC_KEY_LEFT				0xa9
#define RC_KEY_INFO				0xc3
#define RC_KEY_9				0xc6
#define RC_KEY_8				0xc7
#define RC_KEY_7				0xc8
#define RC_KEY_6				0xc9
#define RC_KEY_5				0xca
#define RC_KEY_4				0xcb
#define RC_KEY_3				0xcc
#define RC_KEY_2				0xcd
#define RC_KEY_1				0xce
#define RC_KEY_0				0xcf
#define RC_KEY_DISPLAY			0xd5
#define RC_KEY_BACK				0xd8
#define RC_KEY_SKIPF			0xdd
#define RC_KEY_SKIPB			0xdf
#define RC_KEY_STOP				0xe0
#define RC_KEY_REW				0xe2
#define RC_KEY_FWD				0xe3
#define RC_KEY_TITLE			0xe5
#define RC_KEY_PAUSE			0xe6
#define RC_KEY_PLAY				0xea
#define RC_KEY_MENU				0xf7

int isRemoteKeyEvent(unsigned int button);

#endif
