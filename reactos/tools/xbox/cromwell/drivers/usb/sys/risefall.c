#include "../usb_wrapper.h"
#include "config.h"

// this is for the XPAD
extern unsigned char xpad_button_history[7];

// This is for the IR Remote control
extern unsigned int current_remote_key;
extern unsigned int current_remote_keydir;


// this is for the Keyboard
extern unsigned int current_keyboard_key;


unsigned char accepted_xremote [] = { 0x0b, 0xa6, 0xa7, 0xa9, 0xa8 ,0xd5,
				     0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf};
				     
int risefall_xpad_BUTTON(unsigned char selected_Button) {
	
	int temp;
      	int xpad_id; 
	extern int xpad_num;
	// Section Keyboard	
	
	if (current_keyboard_key!=0) {
		switch (selected_Button) {
			case TRIGGER_XPAD_KEY_A :
		   		if (current_keyboard_key == 0x28) {
					current_keyboard_key=0;
					return 1;
				}
			case TRIGGER_XPAD_KEY_B :
		   		if (current_keyboard_key == 0x29) {
					current_keyboard_key=0;
					return 1;
				}
			case TRIGGER_XPAD_PAD_UP :
		   		if (current_keyboard_key == 0x52) {
					current_keyboard_key=0;
					return 1;
				}
			case TRIGGER_XPAD_PAD_DOWN :
		   		if (current_keyboard_key == 0x51) {
					current_keyboard_key=0;
					return 1;
				}
			case TRIGGER_XPAD_PAD_LEFT :
		   		if (current_keyboard_key == 0x50) {
					current_keyboard_key=0;
					return 1;
				}
			case TRIGGER_XPAD_PAD_RIGHT :
		   		if (current_keyboard_key == 0x4f) {
					current_keyboard_key=0;
					return 1;
				}
		}
	}
	// Section for IR Driver   

       temp = current_remote_keydir;
        
       if ((temp&0x100)==0x100) {
       		int counter;
		for (counter = 0; counter < (sizeof(accepted_xremote)) ; counter++) {
			if (accepted_xremote[counter] == current_remote_key) {
                		temp &= 0xff;
	                       	current_remote_key &=0xff;
				switch (selected_Button) {
					case TRIGGER_XPAD_KEY_A :
				   		if (current_remote_key ==  0xb) return 1; 
					case TRIGGER_XPAD_PAD_UP :
						if (current_remote_key == 0xa6) return 1; 
					case TRIGGER_XPAD_PAD_DOWN :
						if (current_remote_key == 0xa7) return 1; 
					case TRIGGER_XPAD_PAD_LEFT :
						if (current_remote_key == 0xa9) return 1; 
					case TRIGGER_XPAD_PAD_RIGHT :
						if (current_remote_key == 0xa8) return 1; 
					case TRIGGER_XPAD_PAD_KEYSTROKE :
						if (current_remote_key == 0xd5) return 0xd5; 
						if ((current_remote_key>0xc5)&(current_remote_key<0xd0)) return current_remote_key;
				}
				return 0;
			}
		}
	}
        
	for (xpad_id=0;xpad_id<xpad_num; xpad_id++) {
        	// We continue with normal XPAD operations
        	if (selected_Button < 6) {
        	
        		unsigned char Button;
        	
        		Button = XPAD_current[xpad_id].keys[selected_Button];
		
			if ((Button>0x30)&&(xpad_button_history[selected_Button]==0)) {
				// Button Rising Edge
				xpad_button_history[selected_Button] = 1;		
				return 1;
			}	
			
			if ((Button==0x00)&&(xpad_button_history[selected_Button]==1)) {
				// Button Falling Edge
				xpad_button_history[selected_Button] = 0;		
				return -1;
			}	
		}
 		
 		if ((selected_Button > 5) & (selected_Button < 10) ) {
		
			unsigned char Buttonmask;
        	      
			switch (selected_Button) {
				case TRIGGER_XPAD_PAD_UP :
							   Buttonmask = XPAD_PAD_UP; 
							   break;
				case TRIGGER_XPAD_PAD_DOWN :
							   Buttonmask = XPAD_PAD_DOWN;
							   break;
				case TRIGGER_XPAD_PAD_LEFT :
							   Buttonmask = XPAD_PAD_LEFT;
							   break;
				case TRIGGER_XPAD_PAD_RIGHT :
							   Buttonmask = XPAD_PAD_RIGHT;
							   break;
			}		
        	       
			// Rising Edge
			if (((XPAD_current[xpad_id].pad&Buttonmask) != 0) & ((xpad_button_history[6]&Buttonmask) == 0)) {
				xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
				return 1;
			}				
			// Falling Edge
			if (((XPAD_current[xpad_id].pad&Buttonmask) == 0) & ((xpad_button_history[6]&Buttonmask) != 0)) {
				xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
				return -1;
 			}
	
		}
	}
	return 0;
}
