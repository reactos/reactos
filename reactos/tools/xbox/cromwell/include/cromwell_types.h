#ifndef cromwell_types_h
#define cromwell_types_h

/////////////////////////////////
// some typedefs to make for easy sizing

typedef unsigned long ULONG;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#ifndef bool_already_defined_
	typedef int bool;
#endif
typedef unsigned long RGBA; // LSB=R -> MSB = A
typedef long long __int64;

#define guint int
#define guint8 unsigned char

#define true 1
#define false 0

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* #ifndef cromwell_types_h */
