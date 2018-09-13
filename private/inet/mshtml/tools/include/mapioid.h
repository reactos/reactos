/*
 *  M A P I O I D . H
 *
 *  MAPI OID definition header file
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _MAPIOID_
#define _MAPIOID_

/*
 *  MAPI 1.0 Object Identifiers (OID's)
 *
 *  All MAPI 1.0 OIDs are prefixed by the segment
 *
 *      {iso(1) ansi(2) usa(840) microsoft(113556) mapi(3)}
 *
 *  All MAPI 1.0 tags are also include the addistion segment
 *
 *      {tags(10)}
 *
 *  All MAPI 1.0 encodings are also include the addistion segment
 *
 *      {encodeings(11)}
 *
 *  The set of defined tags are as follows
 *
 *   {{mapiprefix} {tags} {tnef(1)}}                        MAPI 1.0 TNEF encapsulation tag
 *
 *   {{mapiprefix} {tags} {ole(3)}}                         MAPI 1.0 OLE prefix
 *   {{mapiprefix} {tags} {ole(3)} {v1(1)}}                 MAPI 1.0 OLE 1.0 prefix
 *   {{mapiprefix} {tags} {ole(3)} {v1(1)} {storage(1)}}    MAPI 1.0 OLE 1.0 OLESTREAM
 *   {{mapiprefix} {tags} {ole(3)} {v2(2)}}                 MAPI 1.0 OLE 2.0 prefix
 *   {{mapiprefix} {tags} {ole(3)} {v2(2)} {storage(1)}}    MAPI 1.0 OLE 2.0 IStorage
 *
 *  The set of defined encodings are as follows
 *
 *   {{mapiprefix} {encodings} {MacBinary(1)}}              MAPI 1.0 MacBinary
 */

#define OID_TAG         0x0A
#define OID_ENCODING    0x0B

#define DEFINE_OID_1(name, b0, b1) \
    EXTERN_C const BYTE FAR * name

#define DEFINE_OID_2(name, b0, b1, b2) \
    EXTERN_C const BYTE FAR * name

#define DEFINE_OID_3(name, b0, b1, b2, b3) \
    EXTERN_C const BYTE FAR * name

#define DEFINE_OID_4(name, b0, b1, b2, b3, b4) \
    EXTERN_C const BYTE FAR * name

#define CB_OID_1        9
#define CB_OID_2        10
#define CB_OID_3        11
#define CB_OID_4        12

#ifdef INITOID
#include <initoid.h>
#endif

#ifdef  USES_OID_TNEF
DEFINE_OID_1(OID_TNEF, OID_TAG, 0x01);
#define CB_OID_TNEF CB_OID_1
#endif

#ifdef  USES_OID_OLE
DEFINE_OID_1(OID_OLE, OID_TAG, 0x03);
#define CB_OID_OLE CB_OID_1
#endif

#ifdef  USES_OID_OLE1
DEFINE_OID_2(OID_OLE1, OID_TAG, 0x03, 0x01);
#define CB_OID_OLE1 CB_OID_2
#endif

#ifdef  USES_OID_OLE1_STORAGE
DEFINE_OID_3(OID_OLE1_STORAGE, OID_TAG, 0x03, 0x01, 0x01);
#define CB_OID_OLE1_STORAGE CB_OID_3
#endif

#ifdef  USES_OID_OLE2
DEFINE_OID_2(OID_OLE2, OID_TAG, 0x03, 0x02);
#define CB_OID_OLE2 CB_OID_2
#endif

#ifdef  USES_OID_OLE2_STORAGE
DEFINE_OID_3(OID_OLE2_STORAGE, OID_TAG, 0x03, 0x02, 0x01);
#define CB_OID_OLE2_STORAGE CB_OID_3
#endif

#ifdef  USES_OID_MAC_BINARY
DEFINE_OID_1(OID_MAC_BINARY, OID_ENCODING, 0x01);
#define CB_OID_MAC_BINARY CB_OID_1
#endif

#ifdef  USES_OID_MIMETAG
DEFINE_OID_1(OID_MIMETAG, OID_TAG, 0x04);
#define CB_OID_MIMETAG CB_OID_1
#endif

#endif
