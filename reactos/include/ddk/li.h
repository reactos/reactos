
#ifndef __DDK_LI_H
#define __DDK_LI_H

#define QUAD_PART(LI)  (*(LONGLONG *)(&LI))

#ifdef COMPILER_LARGE_INTEGERS

#define GET_LARGE_INTEGER_HIGH_PART(LI) ( ( (LI) >> 32) )
#define GET_LARGE_INTEGER_LOW_PART(LI) ( ((LI) & 0xFFFFFFFF) )
#define SET_LARGE_INTEGER_HIGH_PART(LI, HP)  \
  ( (LI) = ((LI) & 0xFFFFFFFFL) | ( ((LARGE_INTEGER)(HP)) << 32 ) )
#define SET_LARGE_INTEGER_LOW_PART(LI, LP) \
  ( (LI) = ((LI) & 0xFFFFFFFF00000000L) | (LP) )
#define LARGE_INTEGER_QUAD_PART(LI) (LI)
#define INITIALIZE_LARGE_INTEGER (0)

typedef long long int LONGLONG, *PLONGLONG;
typedef unsigned long long int ULONGLONG, *PULONGLONG;

#else

#define GET_LARGE_INTEGER_HIGH_PART(LargeInteger) ( (LargeInteger).HighPart )
#define GET_LARGE_INTEGER_LOW_PART(LargeInteger) ( (LargeInteger).LowPart )
#define SET_LARGE_INTEGER_HIGH_PART(LargeInteger,Signed_Long)  \
  ((LargeInteger).HighPart = (Signed_Long))
#define SET_LARGE_INTEGER_LOW_PART(LargeInteger,Unsigned_Long)  \
  ((LargeInteger).LowPart = (Unsigned_Long))
#define LARGE_INTEGER_QUAD_PART(LI)  (*(LONGLONG *)(&(LI)))
#define INITIALIZE_LARGE_INTEGER ({0,0})

typedef double LONGLONG, *PLONGLONG;
typedef double ULONGLONG, *PULONGLONG;

#endif

#endif

