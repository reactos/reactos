//  KB_US.H     KEY DEFINITIONS FOR US EXTENDED KEYBOARD (101 KEYS)


#include "KBKEYDEF.H"

	/* the position of the key is given in relative units to the
		comence of the drawing.  That means that an X or Y position of
		350 means that the x or y edge is drawing begining in the position
		p + 350. In this case the 'p' value is the offset from the edge.*/

typedef	struct KBkeyRec
		{
		LPTSTR  textL;       // text in key lower
		LPTSTR  textC;       // text in key capital
		LPTSTR	skLow;       // What has to be printed low letter
		LPTSTR  skCap;   	 // What has to be printed cap letter
		int 	name;		 // BITMAP, LSHIFT, RSHIF...
		short	posY;		 // See explanation above
		short	posX;		 // same as above
		short	ksizeY;		 // key size in conventional units
		short	ksizeX;		 // same as above
		BOOL 	smallF;		 // TRUE = text has to be written in smaller font
		int  	ktype;		 // 1 - normal, 2 - modifier, 3 - dead
		int		smallKb;	 // SMALL, LARGE, BOTH, NOTSHOW
        BOOL    Caps_Redraw; // Redraw the window for shift, caps
		int 	print;	     //1 - print use ToAscii(), 2 - print the text provided by the header file   
		UINT	scancode[4]; // key scan-code
		}KBkeyRec;

extern struct KBkeyRec KBkey[];


