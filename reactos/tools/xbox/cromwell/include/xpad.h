/* XPAD data */

struct xpad_data
{
	int timestamp;
	short stick_left_x;
	short stick_left_y;
	short stick_right_x;
	short stick_right_y;
	short trig_left;
	short trig_right;
	char pad; /* 1 up 2 down 4 left 8 right */
	char state; /* 1 start 2 back 4 stick_left 8 stick_right */
	unsigned char keys[6]; /* A B X Y Black White */
	
};

int risefall_xpad_BUTTON(unsigned char selected_Button);

#define XPAD_PAD_UP 1
#define XPAD_PAD_DOWN 2
#define XPAD_PAD_LEFT 4
#define XPAD_PAD_RIGHT 8

#define XPAD_STATE_START 1
#define XPAD_STATE_BACK 2
#define XPAD_STATE_LEFT 4
#define XPAD_STATE_RIGHT 8
     
#define TRIGGER_XPAD_KEY_A   		0
#define TRIGGER_XPAD_KEY_B		1
#define TRIGGER_XPAD_KEY_C		2
#define TRIGGER_XPAD_KEY_D        	3
#define TRIGGER_XPAD_KEY_START    	4
#define TRIGGER_XPAD_KEY_BACK     	5

#define TRIGGER_XPAD_PAD_UP 		6
#define TRIGGER_XPAD_PAD_DOWN 		7
#define TRIGGER_XPAD_PAD_LEFT 		8 	
#define TRIGGER_XPAD_PAD_RIGHT 		9

#define TRIGGER_XPAD_PAD_KEYSTROKE	10

