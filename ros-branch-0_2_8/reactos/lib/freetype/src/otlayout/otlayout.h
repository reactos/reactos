#ifndef __OT_LAYOUT_H__
#define __OT_LAYOUT_H__


#include "otlconf.h"

OTL_BEGIN_HEADER

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                       BASE DATA TYPES                        *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  typedef unsigned char     OTL_Byte;
  typedef const OTL_Byte*   OTL_Bytes;

  typedef int               OTL_Error;

  typedef void*             OTL_Pointer;

  typedef int               OTL_Int;
  typedef unsigned int      OTL_UInt;

  typedef long              OTL_Long;
  typedef unsigned long     OTL_ULong;

  typedef short             OTL_Int16;
  typedef unsigned short    OTL_UInt16;


#if OTL_SIZEOF_INT == 4

  typedef int               OTL_Int32;
  typedef unsigned int      OTL_UInt32;

#elif OTL_SIZEOF_LONG == 4

  typedef long              OTL_Int32;
  typedef unsigned long     OTL_UInt32;

#else
#  error "no 32-bits type found"
#endif

  typedef OTL_UInt32        OTL_Tag;

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                       ERROR CODES                            *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  typedef enum
  {
    OTL_Err_Ok = 0,
    OTL_Err_InvalidArgument,
    OTL_Err_InvalidFormat,
    OTL_Err_InvalidOffset,

    OTL_Err_Max

  } OTL_Error;


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                     MEMORY MANAGEMENT                        *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  typedef OTL_Pointer  (*OTL_AllocFunc)( OTL_ULong    size,
                                         OTL_Pointer  data );

  typedef OTL_Pointer  (*OTL_ReallocFunc)( OTL_Pointer   block,
                                           OTL_ULong     size,
                                           OTL_Pointer   data );

  typedef void         (*OTL_FreeFunc)( OTL_Pointer  block,
                                        OTL_Pointer  data );

  typedef struct OTL_MemoryRec_
  {
    OTL_Pointer      mem_data;
    OTL_AllocFunc    mem_alloc;
    OTL_ReallocFunc  mem_realloc;
    OTL_FreeFunc     mem_free;

  } OTL_MemoryRec, *OTL_Memory;

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                        ENUMERATIONS                          *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

/* re-define OTL_MAKE_TAG to something different if you're not */
/* using an ASCII-based character set (Vaxes anyone ?)         */
#ifndef  OTL_MAKE_TAG
#define  OTL_MAKE_TAG(c1,c2,c3,c4)         \
           ( ( (OTL_UInt32)(c1) << 24 ) |  \
               (OTL_UInt32)(c2) << 16 ) |  \
               (OTL_UInt32)(c3) <<  8 ) |  \
               (OTL_UInt32)(c4)         )
#endif

  typedef enum OTL_ScriptTag_
  {
    OTL_SCRIPT_NONE = 0,

#define OTL_SCRIPT_TAG(c1,c2,c3,c4,s,n)  OTL_SCRIPT_TAG_ ## n = OTL_MAKE_TAG(c1,c2,c3,c4),
#include "otltags.h"

    OTL_SCRIPT_MAX

  } OTL_ScriptTag;


  typedef enum OTL_LangTag_
  {
    OTL_LANG_DEFAULT = 0,

#define OTL_LANG_TAG(c1,c2,c3,c4,s,n)  OTL_LANG_TAG_ ## n = OTL_MAKE_TAG(c1,c2,c3,c4),
#include "otltags.h"

    OTL_LANG_MAX

  } OTL_LangTag;


 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                       MEMORY READS                           *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

#define  OTL_PEEK_USHORT(p)  ( ((OTL_UInt)((p)[0]) << 8) |  \
                               ((OTL_UInt)((p)[1])     ) )

#define  OTL_PEEK_ULONG(p)   ( ((OTL_UInt32)((p)[0]) << 24) |  \
                               ((OTL_UInt32)((p)[1]) << 16) |  \
                               ((OTL_UInt32)((p)[2]) <<  8) |  \
                               ((OTL_UInt32)((p)[3])      ) )

#define  OTL_PEEK_SHORT(p)     ((OTL_Int16)OTL_PEEK_USHORT(p))

#define  OTL_PEEK_LONG(p)      ((OTL_Int32)OTL_PEEK_ULONG(p))

#define  OTL_NEXT_USHORT(p)  ( (p) += 2, OTL_PEEK_USHORT((p)-2) )
#define  OTL_NEXT_ULONG(p)   ( (p) += 4, OTL_PEEK_ULONG((p)-4) )

#define  OTL_NEXT_SHORT(p)   ((OTL_Int16)OTL_NEXT_USHORT(p))
#define  OTL_NEXT_LONG(p)    ((OTL_Int32)OTL_NEXT_ULONG(p))

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                        VALIDATION                            *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

  typedef struct OTL_ValidatorRec_*  OTL_Validator;

  typedef struct OTL_ValidatorRec_
  {
    OTL_Bytes    limit;
    OTL_Bytes    base;
    OTL_Error    error;
    OTL_jmp_buf  jump_buffer;

  } OTL_ValidatorRec;

  typedef void  (*OTL_ValidateFunc)( OTL_Bytes  table,
                                     OTL_Valid  valid );

  OTL_API( void )
  otl_validator_error( OTL_Validator  validator,
                       OTL_Error      error );

#define  OTL_INVALID(e)  otl_validator_error( valid, e )

#define  OTL_INVALID_TOO_SHORT  OTL_INVALID( OTL_Err_InvalidOffset )
#define  OTL_INVALID_DATA       OTL_INVALID( OTL_Err_InvalidFormat )

#define  OTL_CHECK(_count)   OTL_BEGIN_STMNT                       \
                               if ( p + (_count) > valid->limit )  \
                                 OTL_INVALID_TOO_SHORT;            \
                             OTL_END_STMNT

 /* */

OTL_END_HEADER

#endif /* __OPENTYPE_LAYOUT_H__ */
