/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/kernel32/li.h
 * PURPOSE:         Include file for lib/kernel32 to discard large integers
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#ifdef COMPILER_LARGE_INTEGERS 
#define GET_LARGE_INTEGER_HIGH_PART(LargeInteger) ( ( LargeInteger >> 32) )
#define GET_LARGE_INTEGER_LOW_PART(LargeInteger) ( (LargeInteger & 0xFFFFFFFF) )
#define SET_LARGE_INTEGER_HIGH_PART(LargeInteger,Signed_Long) ( LargeInteger |= ( ((LARGE_INTEGER)Signed_Long) << 32 ) ) 
#define SET_LARGE_INTEGER_LOW_PART(LargeInteger,Unsigned_Long) ( LargeInteger |= Unsigned_Long  ) 
#else
#define GET_LARGE_INTEGER_HIGH_PART(LargeInteger) (  ( LargeInteger.HighPart) )
#define GET_LARGE_INTEGER_LOW_PART(LargeInteger) ( (LargeInteger.LowPart) )
#define SET_LARGE_INTEGER_HIGH_PART(LargeInteger,Signed_Long) ( LargeInteger.HighPart=  Signed_Long  ) 
#define SET_LARGE_INTEGER_LOW_PART(LargeInteger,Unsigned_Long) ( LargeInteger.LowPart = Unsigned_Long  ) 
#endif