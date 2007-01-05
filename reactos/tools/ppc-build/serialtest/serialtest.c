#include "serial.h"

int main() {
    int i;
    char *iobase = (char *)0x80000000;
    char *serport = iobase + 0x3f8;

    setup( serport, 9600 );

    for( i = ' '; i <= '~'; i++ ) {
        send(serport, i);
    }
    send(serport, 0xa);

    return 0;
}

