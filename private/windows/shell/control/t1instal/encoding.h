/***
 **
 **   Module: Encoding
 **
 **   Description:
 **      This is a module of the T1 to TT font converter. The module
 **      contains interface functions for the global encoding table,
 **      i.e. this is an abstract data type.
 **
 **   Author: Michael Jansson
 **
 **   Created: 6/13/93
 **
 ***/


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif


#define ENC_UNICODE   (USHORT)0   /* Unicode */
#define ENC_MSWINDOWS (USHORT)1   /* Microsoft Windows UGL sub-set encoding. */
#define ENC_STANDARD  (USHORT)2   /* Postscript Standard Encoding */
#define ENC_MACCODES  (USHORT)3   /* Mac encoding. */
#define ENC_MAXCODES  (USHORT)4

#define NOTDEFCODE (USHORT)0xffff /* 0xfffff is not a vaild code point so use
												 it for the .notdef character. */
#define NOTDEFINIT	0xffffffffL	 /* Used to init encoding arrays. */
#define NOTDEFGLYPH	(USHORT)0	 /* Glyph zero must be the notdef glyph. */
#define NULLGLYPH		(USHORT)1	 /* Glyph one must be the null glyph. */

/***
** Function: LookupNotDef
**
** Description:
**   look up a the .notdef character
***/
const struct encoding   *LookupNotDef        _ARGS((void));


/***
** Function: LookupPSName
**
** Description:
**   Do a binary search for a postscript name, and return
**   a handle that can be used to look up a the character
**   code for a specific encoding schema.
***/
struct encoding   *LookupPSName        _ARGS((IN      struct encoding *table,
                                              INOUT   USHORT size,
                                              IN      char *name));


/***
** Function: LookupCharName
**
** Description:
**   look up a the character name for a
**   specific encoding scheme.
***/
const char        *LookupCharName      _ARGS((IN      struct encoding *enc));


/***
** Function: LookupCharCode
**
** Description:
**   look up a the character code for a
**   specific encoding scheme.
***/
USHORT            LookupCharCode       _ARGS((IN      struct encoding *enc,
                                              IN      USHORT type));

/***
** Function: DecodeChar
**
** Description:
**   look up an encoding record for a character code in some
**   known encoding.
***/
const struct encoding   *DecodeChar    _ARGS((IN   struct encoding *table,
                                              IN      USHORT size,
                                              IN      USHORT type,
                                              IN      USHORT code));
/***
** Function: AllocEncodingTable
**
** Description:
**   Create a new encoding ADT.
***/
struct encoding   *AllocEncodingTable  _ARGS((IN      USHORT num));


/***
** Function: SetEncodingEntry
**
** Description:
**   Set the mapping from a glyph name to character
**   codes for various platforms.
***/
void              SetEncodingEntry     _ARGS((INOUT   struct encoding *, 
                                              IN      USHORT entry,
                                              IN      char *name,
                                              IN      USHORT max,
                                              IN      USHORT *codes));
/***
** Function: RehashEncodingTable
**
** Description:
**   Prepare an encoding ADT so that entries can be
**   located in it.
***/
void              RehashEncodingTable  _ARGS((INOUT   struct encoding *, 
                                              IN      USHORT num));


/***
** Function: FreeEncoding
**
** Description:
**   Deallocate memory associated to the encoding array.
***/
void              FreeEncoding         _ARGS((INOUT   struct encoding *,
                                              IN      USHORT num));

/***
** Function: LookupFirstEnc
**
** Description:
**   Locate the first encoding for a given glyph.
***/
const struct encoding *LookupFirstEnc(const struct encoding *encRoot,
												  const int encSize,
												  const struct encoding *encItem);


/***
** Function: LookupNextEnc
**
** Description:
**   Locate the first encoding for a given glyph.
***/
const struct encoding *LookupNextEnc(const struct encoding *encRoot,
												 const int encSize,
												 const struct encoding *encItem);
