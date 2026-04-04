/*
 * Copyright (C) 2016 Austin English
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __BTHSDPDEF_H__
#define __BTHSDPDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDP_LARGE_INTEGER_16 {
    ULONGLONG LowPart;
    LONGLONG HighPart;
} SDP_LARGE_INTEGER_16, *PSDP_LARGE_INTEGER_16, *LPSDP_LARGE_INTEGER_16;

typedef struct SDP_ULARGE_INTEGER_16 {
    ULONGLONG LowPart;
    ULONGLONG HighPart;
} SDP_ULARGE_INTEGER_16, *PSDP_ULARGE_INTEGER_16, *LPSDP_ULARGE_INTEGER_16;

typedef enum NodeContainerType {
    NodeContainerTypeSequence,
    NodeContainerTypeAlternative
} NodeContainerType;

typedef USHORT SDP_ERROR, *PSDP_ERROR;

typedef enum SDP_TYPE {
    SDP_TYPE_NIL =  0x00,
    SDP_TYPE_UINT = 0x01,
    SDP_TYPE_INT = 0x02,
    SDP_TYPE_UUID = 0x03,
    SDP_TYPE_STRING = 0x04,
    SDP_TYPE_BOOLEAN = 0x05,
    SDP_TYPE_SEQUENCE = 0x06,
    SDP_TYPE_ALTERNATIVE = 0x07,
    SDP_TYPE_URL = 0x08,
    SDP_TYPE_CONTAINER = 0x20
} SDP_TYPE;

typedef enum SDP_SPECIFICTYPE {
    SDP_ST_NONE = 0x0000,
    SDP_ST_UINT8 = 0x0010,
    SDP_ST_UINT16 = 0x0110,
    SDP_ST_UINT32 = 0x0210,
    SDP_ST_UINT64 = 0x0310,
    SDP_ST_UINT128 = 0x0410,
    SDP_ST_INT8 = 0x0020,
    SDP_ST_INT16 = 0x0120,
    SDP_ST_INT32 = 0x0220,
    SDP_ST_INT64 = 0x0320,
    SDP_ST_INT128 = 0x0420,
    SDP_ST_UUID16 = 0x0130,
    SDP_ST_UUID32 = 0x0220,
    SDP_ST_UUID128 = 0x0430
} SDP_SPECIFICTYPE;

typedef struct _SdpAttributeRange {
    USHORT minAttribute;
    USHORT maxAttribute;
} SdpAttributeRange;

typedef union SdpQueryUuidUnion {
    GUID uuid128;
    ULONG uuid32;
    USHORT uuid16;
} SdpQueryUuidUnion;

typedef struct _SdpQueryUuid {
    SdpQueryUuidUnion u;
    USHORT uuidType;
} SdpQueryUuid;

#ifdef __cplusplus
}
#endif

#endif /* __BTHSDPDEF_H__ */
