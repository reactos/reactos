/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/* 7.8 Format conversion of integer types <inttypes.h> */

#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#include <corecrt.h>
#include <stdint.h>
#include <stddef.h>

#ifdef	__cplusplus
extern	"C"	{
#endif

typedef struct {
	intmax_t quot;
	intmax_t rem;
	} imaxdiv_t;

/* 7.8.1 Macros for format specifiers */
#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIo64 "llo"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"

/* fprintf macros for signed types */
#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "d"

#define PRIdLEAST8 "d"
#define PRIdLEAST16 "d"
#define PRIdLEAST32 "d"
#define PRIdLEAST64 PRId64

#define PRIdFAST8 "d"
#define PRIdFAST16 "d"
#define PRIdFAST32 "d"
#define PRIdFAST64 PRId64

#define PRIdMAX PRId64

#define PRIi8 "i"
#define PRIi16 "i"
#define PRIi32 "i"

#define PRIiLEAST8 "i"
#define PRIiLEAST16 "i"
#define PRIiLEAST32 "i"
#define PRIiLEAST64 PRIi64

#define PRIiFAST8 "i"
#define PRIiFAST16 "i"
#define PRIiFAST32 "i"
#define PRIiFAST64 PRIi64

#define PRIiMAX PRIi64

#define PRIo8 "o"
#define PRIo16 "o"
#define PRIo32 "o"

#define PRIoLEAST8 "o"
#define PRIoLEAST16 "o"
#define PRIoLEAST32 "o"
#define PRIoLEAST64 PRIo64

#define PRIoFAST8 "o"
#define PRIoFAST16 "o"
#define PRIoFAST32 "o"
#define PRIoFAST64 PRIo64

#define PRIoMAX PRIo64

/* fprintf macros for unsigned types */
#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "u"


#define PRIuLEAST8 "u"
#define PRIuLEAST16 "u"
#define PRIuLEAST32 "u"
#define PRIuLEAST64 PRIu64

#define PRIuFAST8 "u"
#define PRIuFAST16 "u"
#define PRIuFAST32 "u"
#define PRIuFAST64 PRIu64

#define PRIuMAX PRIu64

#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "x"

#define PRIxLEAST8 "x"
#define PRIxLEAST16 "x"
#define PRIxLEAST32 "x"
#define PRIxLEAST64 PRIx64

#define PRIxFAST8 "x"
#define PRIxFAST16 "x"
#define PRIxFAST32 "x"
#define PRIxFAST64 PRIx64

#define PRIxMAX PRIx64

#define PRIX8 "X"
#define PRIX16 "X"
#define PRIX32 "X"

#define PRIXLEAST8 "X"
#define PRIXLEAST16 "X"
#define PRIXLEAST32 "X"
#define PRIXLEAST64 PRIX64

#define PRIXFAST8 "X"
#define PRIXFAST16 "X"
#define PRIXFAST32 "X"
#define PRIXFAST64 PRIX64

#define PRIXMAX PRIX64

/*
 *   fscanf macros for signed int types
 *   NOTE: if 32-bit int is used for int_fast8_t and int_fast16_t
 *   (see stdint.h, 7.18.1.3), FAST8 and FAST16 should have
 *   no length identifiers
 */

#define SCNd16 "hd"
#define SCNd32 "d"
#define SCNd64 PRId64

#define SCNdLEAST16 "hd"
#define SCNdLEAST32 "d"
#define SCNdLEAST64 PRId64

#define SCNdFAST16 "hd"
#define SCNdFAST32 "d"
#define SCNdFAST64 PRId64

#define SCNdMAX PRId64

#define SCNi16 "hi"
#define SCNi32 "i"
#define SCNi64 PRIi64

#define SCNiLEAST16 "hi"
#define SCNiLEAST32 "i"
#define SCNiLEAST64 PRIi64

#define SCNiFAST16 "hi"
#define SCNiFAST32 "i"
#define SCNiFAST64 PRIi64

#define SCNiMAX PRIi64

#define SCNo16 "ho"
#define SCNo32 "o"
#define SCNo64 PRIo64

#define SCNoLEAST16 "ho"
#define SCNoLEAST32 "o"
#define SCNoLEAST64 PRIo64

#define SCNoFAST16 "ho"
#define SCNoFAST32 "o"
#define SCNoFAST64 PRIo64

#define SCNoMAX PRIo64

#define SCNx16 "hx"
#define SCNx32 "x"
#define SCNx64 PRIx64

#define SCNxLEAST16 "hx"
#define SCNxLEAST32 "x"
#define SCNxLEAST64 PRIx64

#define SCNxFAST16 "hx"
#define SCNxFAST32 "x"
#define SCNxFAST64 PRIx64

#define SCNxMAX PRIx64

/* fscanf macros for unsigned int types */

#define SCNu16 "hu"
#define SCNu32 "u"
#define SCNu64 PRIu64

#define SCNuLEAST16 "hu"
#define SCNuLEAST32 "u"
#define SCNuLEAST64 PRIu64

#define SCNuFAST16 "hu"
#define SCNuFAST32 "u"
#define SCNuFAST64 PRIu64

#define SCNuMAX PRIu64

#ifdef _WIN64
#define PRIdPTR PRId64
#define PRIiPTR PRIi64
#define PRIoPTR PRIo64
#define PRIuPTR PRIu64
#define PRIxPTR PRIx64
#define PRIXPTR PRIX64
#define SCNdPTR PRId64
#define SCNiPTR PRIi64
#define SCNoPTR PRIo64
#define SCNxPTR PRIx64
#define SCNuPTR PRIu64
#else
#define PRIdPTR "d"
#define PRIiPTR "i"
#define PRIoPTR "o"
#define PRIuPTR "u"
#define PRIxPTR "x"
#define PRIXPTR "X"
#define SCNdPTR "d"
#define SCNiPTR "i"
#define SCNoPTR "o"
#define SCNxPTR "x"
#define SCNuPTR "u"
#endif

_ACRTIMP __int64            __cdecl strtoimax(const char*,char**,int);
_ACRTIMP __int64            __cdecl strtoimax_l(const char*,char**,int,_locale_t);
_ACRTIMP unsigned __int64   __cdecl strtoumax(const char*,char**,int);
_ACRTIMP unsigned __int64   __cdecl strtoumax_l(const char*,char**,int,_locale_t);

#ifdef	__cplusplus
}
#endif

#endif /* ndef _INTTYPES_H */
