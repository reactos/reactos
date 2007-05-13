#ifndef _GDITEST_H
#define _GDITEST_H

#define WINVER 0x501

//#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#define TEST(x) \
	if (x)\
	{\
		(*passed)++;\
	} else {\
		(*failed)++;\
		printf("Test failed in %s:%d (%s)\n", __FILE__, __LINE__, #x);\
	};

#define GDI_HANDLE_INDEX_MASK 0x0000ffff // (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000

#define GDI_HANDLE_GET_INDEX(h)    \
    (((ULONG_PTR)(h)) & GDI_HANDLE_INDEX_MASK)

#define GDI_HANDLE_GET_TYPE(h)     \
    (((ULONG_PTR)(h)) & GDI_HANDLE_TYPE_MASK)

#define GDI_HANDLE_PEN_TO_BRUSH(h) \
    (HBRUSH)((((ULONG_PTR)(h)) & ~GDI_HANDLE_TYPE_MASK) | GDI_OBJECT_TYPE_PEN)


#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
#define GDI_OBJECT_TYPE_COLORSPACE  0x00090000
#define GDI_OBJECT_TYPE_METADC      0x00660000
#define GDI_OBJECT_TYPE_METAFILE    0x00260000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00460000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000


/* The type definitions */
typedef BOOL (*TESTPROC)(INT*, INT*);

typedef struct tagTEST
{
	CHAR* Test;
	TESTPROC Proc;
} TEST, *PTEST;


extern TEST TestList[];
INT NumTests(void);

#endif /* _GDITEST_H */

/* EOF */
