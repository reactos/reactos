#include "windows.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

void __cdecl main(int argc,char *argv[]) {

    DCB NewDCB;
    COMMTIMEOUTS To = {0};
    char firstString[] = "1200,n,8,1";
    char secondString[] = "COM1 1200,n,8,1";
    char thirdString[] = "HOST1 1200,8,N,1";
    char fourthString[] = "COM1:1200,n,8,1";
    char fifthString[] = "COM1: baud=9600 TO=ON";

    printf("About to do %s\n",&firstString[0]);
    if (!BuildCommDCB(
             &firstString[0],
             &NewDCB
             )) {

        printf("Bad BuildDCB: %d\n",GetLastError());

    }
    printf("About to do %s\n",&secondString[0]);
    if (!BuildCommDCB(
             &secondString[0],
             &NewDCB
             )) {

        printf("Bad BuildDCB: %d\n",GetLastError());

    }
    printf("About to do %s\n",&thirdString[0]);
    if (!BuildCommDCB(
             &thirdString[0],
             &NewDCB
             )) {

        printf("Bad BuildDCB: %d\n",GetLastError());

    }
    printf("About to do %s\n",&fourthString[0]);
    if (!BuildCommDCB(
             &fourthString[0],
             &NewDCB
             )) {

        printf("Bad BuildDCB: %d\n",GetLastError());

    }
    printf("About to do %s\n",&fifthString[0]);
    if (!BuildCommDCBAndTimeouts(
             &fifthString[0],
             &NewDCB,
             &To
             )) {

        printf("Bad BuildDCB: %d\n",GetLastError());

    }

}
