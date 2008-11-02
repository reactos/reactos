#ifndef _DDRAWTEST_H
#define _DDRAWTEST_H

#include <stdio.h>
#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>

#define TEST(x) \
	if (x)\
	{\
		(*passed)++;\
	} else {\
		(*failed)++;\
		printf("Test failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
	};


/* The type definitions */
typedef BOOL (*TESTPROC)(INT*, INT*);

typedef struct tagTEST
{
	CHAR* Test;
	TESTPROC Proc;
} TEST, *PTEST;


extern TEST TestList[];

#endif /* _DDRAWTEST_H */

/* EOF */
