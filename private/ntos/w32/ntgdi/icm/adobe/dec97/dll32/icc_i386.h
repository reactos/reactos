#ifndef ICC_I386_H
#define ICC_I386_H
#include "windows.h"

#ifdef  __cplusplus
extern "C"
{
#endif

// ColorProfile errors - can be later added to global Errors
//
typedef enum
{
    CP_NULL_POINTER_ERR = 10000,
    CP_MEMORY_ALLOC_ERR,
    CP_FILE_OPEN_ERR,
    CP_FILE_READ_ERR,
    CP_FORMAT_ERR,
    CP_OUT_OF_RANGE_ERR,
    CP_NO_MEMORY_ERR,
    CP_NOT_FOUND_ERR,
    CP_POSTSCRIPT_ERR
} cpError;


/*------------------------------------------------------------------------*/
//          Application specific data
//
//  We use here the same concept that is used in WINSOCK 1.1 specifications.
//  All the data that come from the ColorProfile are in "network" format,
//   and we deal with that data using "host" format. The "host" byte order can
//   be either big- or little-endian, the size of the integers and floats
//   may vary too, we only require that all "host" type data must have
//   the precision  equal or higher of the "network" data.
//  The platfornm-specific implementation must provide macros or functions
//   to convert from  "network" to "host" representation.
//  Currently we do not write data into profile, so no conversion
//   from "host" to "network" is  required.
//
//  The basic types that should be defined to work with ColorProfiles are:
//
//      BOOL    -   any size variable that can hold either TRUE or FALSE
//      BYTES   -   exactly 8-bit 
//      SINT    -   signed int capable of holding at least 32 bits
//      SFLOAT  -   signed float capable of holding at least 32 bits
//      CSIG    -   entity that can hold at least 32 bits of signature
//                      in host format
//      ATTRIB  -   entity that can hold at least 64 bits of attributes
//                      in host format
//      MEMPTR  -   Pointer to the memory block which can refernce more
//                      than 64K of BYTES
//      
//
/*------------------------------------------------------------------------*/

#ifndef     BOOL
typedef     int             BOOL;       // Any variable that can hold
#endif                                  //  a boolean TRUE/FALSE

typedef     unsigned char   BYTES;      // Exactly 8 bits

typedef     short           SHORT;      // Exactly 16 bits signed int

typedef     unsigned short  USHORT;     // Exactly 16 bits unsigned signed int
                                       
typedef     signed long     SINT;       // The signed integer type that can
                                        //  hold more than 32768( > 16bits)

typedef     float           SFLOAT;     // The type that can hold a floating
                                        //  or fixed point variable
                                        //  at least with 5 digits after point, 
                                        //  both positive and negtive

typedef     unsigned long   CSIG;       // Type that can hold a Signature -
                                        //  32-bits or more

typedef     unsigned long   ATTRIB[2];  // Type that can hold attributes,
                                        // it's more than or equal to 64 bits

typedef     BYTES __huge    *MEMPTR;    // Pointer to the memory that can
                                        // access more than 64K BYTES
typedef     MEMPTR __huge   *PMEMPTR;   

typedef     SHORT __huge    *PSHORT;
typedef     USHORT __huge   *PUSHORT;  
#define     EXTERN          __loadds __far __pascal  


/*------------------------------------------------------------------------*/
//  Pointers to the platform-specific data types.
//
/*------------------------------------------------------------------------*/

// Those pointers are platform and OS specific. For all 16-bit apps
//   we have to explicitly declare those pointers as __far for them
//   to be able to point to the data in case DS !=SS.
// Pointers that will be used to sequentially access the
//   data (like anypointer[i]  and so on) must be declared  __huge
//   in order to provide correct passing of 64K selector boundary.
// For NT apps and other 32-bit apps all pointers are 32-bits, so no
//   __far and __huge declaration is necessary.

typedef     BYTES   __far   *LPBYTES;
typedef     SINT    __far   *LPSINT;
typedef     SFLOAT  __far   *LPSFLOAT;
typedef     CSIG    __far   *LPCSIG;
typedef     ATTRIB  __far   *LPATTRIB;
typedef     MEMPTR  __far   *LPMEMPTR;

typedef     icProfile       __huge *lpcpProfile;
typedef     icHeader        __huge *lpcpHeader;
typedef     icTagList       __huge *lpcpTagList;
typedef     icTag           __huge *lpcpTag;
typedef     icTagBase       __huge *lpcpTagBase;

typedef     icCurveType     __huge *lpcpCurveType;
typedef     icLut16Type     __huge *lpcpLut16Type;
typedef     icLut8Type      __huge *lpcpLut8Type;
typedef     icXYZType       __huge *lpcpXYZType;


//===========================================================================
//    Macros to convert from Color Profile to Platform-specific
//          data representation.
//  We assume here that all data in ColorProfile are accessed through
//      pointer to BYTES. The only valid way to use those macros
//      is to call them this way:
//          MEMPTR lpMem;
//          ...
//          lpMem = ..........;
//          z = XtoY(lpMem);

#define     ui8toSINT(a)    ((SINT) (a))
#define     ui16toSINT(a)   ((SINT) ( ( ((a)[0]<<8) & 0x00FF00) | \
                                      (  (a)[1] & 0x00FF)))
#define     ui32toSINT(a)   ((SINT) ( ( (((SINT)(a)[0])<<24) & 0x00FF000000) | \
                                      ( (((SINT)(a)[1])<<16) & 0x00FF0000  ) | \
                                      ( (       (a)[2] <<8)  & 0x00FF00    ) | \
                                      (         (a)[3]       & 0x00FF      )))

#define     si8toSINT(a)    ((SINT) (a))
#define     si16toSINT(a)   ((SINT) (   ((SINT)(a)[0])<<8 | \
                                      (        (a)[1] & 0x00FF)))
#define     si32toSINT(a)   ((SINT) (   ((SINT)(a)[0])<<24                | \
                                    (  (((SINT)(a)[1])<<16) & 0x00FF0000) | \
                                    (  (       (a)[2] <<8)  & 0x00FF00  ) | \
                                    (          (a)[3] & 0x00FF          )))

#define     ui8f8toSFLOAT(a)   ((SFLOAT)(  (a)[0] + (a)[1]/256.0) )
#define     ui16f16toSFLOAT(a) ((SFLOAT)(  (a)[0]*256.0         + \
                                           (a)[1]               + \
                                         ( (a)[2]               + \
                                           (a)[3]/256.0 ) /256.0 ))


#define     si16f16toSFLOAT(a) ( ((SFLOAT)( ((SINT)(a)[0])<<24                | \
                                          ((((SINT)(a)[1])<<16) & 0x00FF0000) | \
                                          (       ((a)[2] <<8)  & 0x00FF00  ) | \
                                          (        (a)[3]        & 0x00FF   )   \
                                 )) /65536.0)


#define     SigtoCSIG(a)    ((CSIG) ( ( (((SINT)(a)[0])<<24) & 0x00FF000000) | \
                                      ( (((SINT)(a)[1])<<16) & 0x00FF0000  ) | \
                                      ( (       (a)[2] <<8)  & 0x00FF00    ) | \
                                      (         (a)[3]       & 0x00FF      )  ) )


#ifdef  __cplusplus
}
#endif

#endif  //  ICC_I386_H
