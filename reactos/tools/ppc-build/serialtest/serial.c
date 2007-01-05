#include "serial.h"

void sync() {
    __asm__("eieio\n\t"
	    "sync");
}

void send(char *serport, char c) {
	/* Wait for Clear to Send */
    while( !(serport[LSR] & 0x20) ) sync();
    
    serport[THR] = c;
    sync();
}

char recv(char *serport) {
    char c;

    while( !(serport[LSR] & 1) ) sync();
    
    c = serport[RCV];
    sync();
}

void setup(char *serport, int baud) {
	int x = 115200 / baud;
	serport[LCR] = 128;
	sync();
	serport[BAUDLOW] = x & 255;
	sync();
	serport[BAUDHIGH] = x >> 8;
	sync();
	serport[LCR] = 3;
	sync();
	serport[IER] = 0;
	sync();
}
