#ifndef SERIAL_H
#define SERIAL_H

#define RCV 0
#define THR 0
#define BAUDLOW 0
#define BAUDHIGH 1
#define IER 1
#define FCR 2
#define ISR 2
#define LCR 3
#define MCR 4
#define LSR 5
#define MSR 6
#define SPR 7

extern void send(char *serport, char c);
extern char recv(char *serport);
extern void setup(char *serport, int baud);

#endif//SERIAL_H
