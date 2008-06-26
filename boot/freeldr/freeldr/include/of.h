#ifndef FREELDR_OF_H
#define FREELDR_OF_H

#define OF_FAILED 0
#define ERR_NOT_FOUND 0xc0000010

#include "of_call.h"
#include <string.h>

typedef int (*of_proxy)
    ( int table_off, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6 );
typedef long jmp_buf[100];
extern of_proxy ofproxy;

int setjmp( jmp_buf buf );
int longjmp( jmp_buf buf, int retval );
int ofw_callmethod_ret(const char *method, int handle, int nargs, int *args, int ret);

#endif/*FREELDR_OF_H*/
