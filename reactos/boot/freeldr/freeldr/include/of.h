#ifndef FREELDR_OF_H
#define FREELDR_OF_H

#define OF_FAILED 0
#define ERR_NOT_FOUND 0xc0000010

#define REV(x) (((x)>>24)&0xff)|(((x)>>8)&0xff00)|(((x)<<24)&0xff000000)|(((x)<<8)&0xff0000)

#include "of_call.h"
#include <string.h>

typedef int (*of_proxy)
    ( int table_off, void *arg1, void *arg2, void *arg3, void *arg4 );
typedef long jmp_buf[100];
extern of_proxy ofproxy;
extern void le_swap( void *begin, void *end, void *dest );

int setjmp( jmp_buf buf );
int longjmp( jmp_buf buf, int retval );

#endif/*FREELDR_OF_H*/
