#include "../usb_wrapper.h"
#include "config.h"
#include "xremote.h"

// This is for the Xpad
extern unsigned char xpad_button_history[7];

// This is for the Keyboard
extern unsigned int current_keyboard_key;

int risefall_xpad_BUTTON(unsigned char selected_Button) {
	
      	int xpad_id; 
	int match;
	extern int xpad_num;
	
	// USB keyboard section 
	
	match=0;
	if (current_keyboard_key!=0) {
		switch (selected_Button) {
			case TRIGGER_XPAD_KEY_A :
		   		if (current_keyboard_key == 0x28) match=1;
				break;
			case TRIGGER_XPAD_KEY_B :
		   		if (current_keyboard_key == 0x29) match=1;
				break;
			case TRIGGER_XPAD_PAD_UP :
				if (current_keyboard_key == 0x52) match=1;
				break;
			case TRIGGER_XPAD_PAD_DOWN :
		   		if (current_keyboard_key == 0x51) match=1;
				break;
			case TRIGGER_XPAD_PAD_LEFT :
		   		if (current_keyboard_key == 0x50) match=1;
				break;
			case TRIGGER_XPAD_PAD_RIGHT :
		   		if (current_keyboard_key == 0x4f) match=1;
				break;
		}

		if (match) {
			//A match occurred, so the event has now been processed
			//Clear it, and return success
			current_keyboard_key=0;
			return 1;
		}
	}
	
	// Xbox IR remote section
	
	match=0;
	if (!remotekeyIsRepeat) {
		/* We only grab the key event when the button is first pressed.
		 * If it's being held down, we ignore the multiple events this 
		 * generates */
		
		switch (selected_Button) {
			case TRIGGER_XPAD_KEY_A:
		   		if (current_remote_key == RC_KEY_SELECT) match=1;
				break;
			case TRIGGER_XPAD_PAD_UP:
				if (current_remote_key == RC_KEY_UP) match=1;
				break;
			case TRIGGER_XPAD_PAD_DOWN:
				if (current_remote_key == RC_KEY_DOWN) match=1;
				break;
			case TRIGGER_XPAD_PAD_LEFT:
				if (current_remote_key == RC_KEY_LEFT) match=1;
				break;
			case TRIGGER_XPAD_PAD_RIGHT:
				if (current_remote_key == RC_KEY_RIGHT) match=1;
				break;
			case TRIGGER_XPAD_KEY_BACK:
				if (current_remote_key == RC_KEY_BACK) match=1;
				break;
		}
		if (match) {
			//A match occurred, so the event has now been processed
			//Clear it, and return success
			current_remote_key=0;
			remotekeyIsRepeat=0;
			return 1;
		}
	}
       	
	// Xbox controller section
	if (selected_Button < 6) {
       	
       		unsigned char Button;
       	
       		Button = XPAD_current[0].keys[selected_Button];
	
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
		if (((XPAD_current[0].pad&Buttonmask) != 0) & ((xpad_button_history[6]&Buttonmask) == 0)) {
			xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
			return 1;
		}				
		// Falling Edge
		if (((XPAD_current[0].pad&Buttonmask) == 0) & ((xpad_button_history[6]&Buttonmask) != 0)) {
			xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
			return -1;
 		}
	}
	return 0;
}
