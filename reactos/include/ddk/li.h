
#ifndef __DDK_LI_H
#define __DDK_LI_H

#define QUAD_PART(LI)  (*(LONGLONG *)(&LI))

#ifdef COMPILER_LARGE_INTEGERS

#define GET_LARGE_INTEGER_HIGH_PART(LargeInteger) ( ( LargeInteger >> 32) )
#define GET_LARGE_INTEGER_LOW_PART(LargeInteger) ( (LargeInteger & 0xFFFFFFFF) )
#define SET_LARGE_INTEGER_HIGH_PART(LargeInteger,Signed_Long)  \
  ( LargeInteger |= ( ((LARGE_INTEGER)Signed_Long) << 32 ) )
#define SET_LARGE_INTEGER_LOW_PART(LargeInteger,Unsigned_Long) \
  ( LargeInteger |= Unsigned_Long )
#define LARGE_INTEGER_QUAD_PART(LargeInteger) (LargeInteger)

#else

#define GET_LARGE_INTEGER_HIGH_PART(LargeInteger) ( (LargeInteger).HighPart )
#define GET_LARGE_INTEGER_LOW_PART(LargeInteger) ( (LargeInteger).LowPart )
#define SET_LARGE_INTEGER_HIGH_PART(LargeInteger,Signed_Long)  \
  ((LargeInteger).HighPart = (Signed_Long))
#define SET_LARGE_INTEGER_LOW_PART(LargeInteger,Unsigned_Long)  \
  ((LargeInteger).LowPart = (Unsigned_Long))
#define LARGE_INTEGER_QUAD_PART(LI)  (*(LONGLONG *)(&(LI)))

#endif

#endif

