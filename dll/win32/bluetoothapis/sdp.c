/*
 * SDP APIs
 *
 * Copyright 2024 Vibhav Pant
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
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>

#include "wine/debug.h"
#include "bthsdpdef.h"
#include "bluetoothapis.h"

WINE_DEFAULT_DEBUG_CHANNEL( bluetoothapis );

#define BTH_READ_UINT16( s ) RtlUshortByteSwap( *(USHORT *)(s) )
#define BTH_READ_UINT32( s ) RtlUlongByteSwap( *(ULONG *)(s) )
#define BTH_READ_UINT64( s ) RtlUlonglongByteSwap( *(ULONGLONG *)(s) )

#define SDP_SIZE_DESC_1_BYTE 0
#define SDP_SIZE_DESC_2_BYTES 1
#define SDP_SIZE_DESC_4_BYTES 2
#define SDP_SIZE_DESC_8_BYTES 3
#define SDP_SIZE_DESC_16_BYTES 4
#define SDP_SIZE_DESC_NEXT_UINT8 5
#define SDP_SIZE_DESC_NEXT_UINT16 6
#define SDP_SIZE_DESC_NEXT_UINT32 7

static void bth_read_uint128( BYTE *s, SDP_ULARGE_INTEGER_16 *v )
{
    v->HighPart = BTH_READ_UINT64( s );
    v->LowPart = BTH_READ_UINT64( s + 8 );
}

static BYTE data_elem_type( BYTE elem ) { return (elem & 0xf8) >> 3; }
static BYTE data_elem_size_desc( BYTE elem ) { return elem & 0x7; }

#define SDP_ELEMENT_IS_UINT16( d ) ( (d)->type == SDP_TYPE_UINT && (d)->specificType == SDP_ST_UINT16 )
#define SDP_ELEMENT_IS_ATTRID( d ) SDP_ELEMENT_IS_UINT16((d))

/* Read the data element's size/length as described by the size descriptor, starting from stream. Only
 * valid for SDP_SIZE_DESC_NEXT_* types. */
static BOOL sdp_elem_read_var_size( BYTE *stream, ULONG stream_size, SIZE_T *read, BYTE size_desc,
                                    UINT32 *size )
{
    switch (size_desc)
    {
    case SDP_SIZE_DESC_NEXT_UINT8:
        if (stream_size < sizeof( UINT8 )) return FALSE;
        *size = *stream;
        *read += sizeof( UINT8 );
        return TRUE;
    case SDP_SIZE_DESC_NEXT_UINT16:
        if (stream_size < sizeof( UINT16 )) return FALSE;
        *size = BTH_READ_UINT16( stream );
        *read += sizeof( UINT16 );
        return TRUE;
    case SDP_SIZE_DESC_NEXT_UINT32:
        if (stream_size < sizeof( UINT32 )) return FALSE;
        *size = BTH_READ_UINT32( stream );
        *read += sizeof( UINT32 );
        return TRUE;
    default:
        return FALSE;
    }
}

const static SDP_SPECIFICTYPE SDP_BASIC_TYPES[4][5] = {
    [SDP_TYPE_UINT] =
        {
            [SDP_SIZE_DESC_1_BYTE] = SDP_ST_UINT8,
            [SDP_SIZE_DESC_2_BYTES] = SDP_ST_UINT16,
            [SDP_SIZE_DESC_4_BYTES] = SDP_ST_UINT32,
            [SDP_SIZE_DESC_8_BYTES] = SDP_ST_UINT64,
            [SDP_SIZE_DESC_16_BYTES] = SDP_ST_UINT128,
        },
    [SDP_TYPE_INT] =
        {
            [SDP_SIZE_DESC_1_BYTE] = SDP_ST_INT8,
            [SDP_SIZE_DESC_2_BYTES] = SDP_ST_INT16,
            [SDP_SIZE_DESC_4_BYTES] = SDP_ST_INT32,
            [SDP_SIZE_DESC_8_BYTES] = SDP_ST_INT64,
            [SDP_SIZE_DESC_16_BYTES] = SDP_ST_INT128,
        },
    [SDP_TYPE_UUID] =
        {
            [SDP_SIZE_DESC_2_BYTES] = SDP_ST_UUID16,
            [SDP_SIZE_DESC_4_BYTES] = SDP_ST_UUID32,
            [SDP_SIZE_DESC_16_BYTES] = SDP_ST_UUID128,
        },
};

static BOOL sdp_read_specific_type( BYTE *stream, ULONG stream_size, SDP_SPECIFICTYPE st,
                                    SDP_ELEMENT_DATA *data, SIZE_T *read )
{
    switch (st)
    {
    case SDP_ST_UINT8:
    case SDP_ST_INT8:
        if (stream_size < sizeof( UINT8 )) return FALSE;
        data->data.uint8 = *stream;
        *read += sizeof( UINT8 );
        break;
    case SDP_ST_UINT16:
    case SDP_ST_INT16:
    case SDP_ST_UUID16:
        if (stream_size < sizeof( UINT16 )) return FALSE;
        data->data.uint16 = BTH_READ_UINT16( stream );
        *read += sizeof( UINT16 );
        break;
    case SDP_ST_UINT32:
    case SDP_ST_INT32:
        if (stream_size < sizeof( UINT32 )) return FALSE;
        data->data.uint32 = BTH_READ_UINT32( stream );
        *read += sizeof( UINT32 );
        break;
    case SDP_ST_UINT64:
    case SDP_ST_INT64:
        if (stream_size < sizeof( UINT64 )) return FALSE;
        data->data.uint64 = BTH_READ_UINT64( stream );
        *read += sizeof( UINT64 );
        break;
    case SDP_ST_UINT128:
    case SDP_ST_INT128:
    case SDP_ST_UUID128:
        if (stream_size < sizeof( SDP_ULARGE_INTEGER_16 )) return FALSE;
        bth_read_uint128( stream, &data->data.uint128 );
        *read += sizeof( SDP_ULARGE_INTEGER_16 );
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static DWORD sdp_read_element_data( BYTE *stream, ULONG stream_size, SDP_ELEMENT_DATA *data,
                                    SIZE_T *read )
{
    BYTE type, size_desc, elem;
    SDP_SPECIFICTYPE st;

    if (stream_size < sizeof( BYTE )) return ERROR_INVALID_PARAMETER;

    elem = *stream;
    type = data_elem_type( elem );
    size_desc = data_elem_size_desc( elem );

    stream += sizeof( BYTE );
    *read += sizeof( BYTE );
    stream_size -= sizeof( BYTE );

    memset( data, 0, sizeof( *data ) );
    switch (type)
    {
    case SDP_TYPE_NIL:
        if (size_desc != 0) return ERROR_INVALID_PARAMETER;

        data->type = type;
        data->specificType = SDP_ST_NONE;
        break;
    case SDP_TYPE_UINT:
    case SDP_TYPE_INT:
    case SDP_TYPE_UUID:
        if (size_desc > SDP_SIZE_DESC_16_BYTES) return ERROR_INVALID_PARAMETER;

        st = SDP_BASIC_TYPES[type][size_desc];
        if (st == SDP_ST_NONE) return ERROR_INVALID_PARAMETER;

        if (!sdp_read_specific_type( stream, stream_size, st, data, read ))
            return ERROR_INVALID_PARAMETER;

        data->type = type;
        data->specificType = st;
        break;
    case SDP_TYPE_BOOLEAN:
        if (size_desc != SDP_SIZE_DESC_1_BYTE) return ERROR_INVALID_PARAMETER;
        if (stream_size < sizeof( BYTE )) return ERROR_INVALID_PARAMETER;

        data->type = type;
        data->specificType = SDP_ST_NONE;
        data->data.booleanVal = *stream;
        *read += sizeof( BYTE );
        break;
    case SDP_TYPE_STRING:
    case SDP_TYPE_URL:
    case SDP_TYPE_SEQUENCE:
    case SDP_TYPE_ALTERNATIVE:
    {
        UINT32 elems_size;
        SIZE_T size_read = 0;

        if (!(size_desc >= SDP_SIZE_DESC_NEXT_UINT8 && size_desc <= SDP_SIZE_DESC_NEXT_UINT32))
            return ERROR_INVALID_PARAMETER;
        if (!sdp_elem_read_var_size( stream, stream_size, &size_read, size_desc, &elems_size ))
            return ERROR_INVALID_PARAMETER;

        stream_size -= size_read;
        if (type == SDP_TYPE_STRING || type == SDP_TYPE_URL)
            stream += size_read;

        if (stream_size < elems_size) return ERROR_INVALID_PARAMETER;

        data->type = type;
        data->specificType = SDP_ST_NONE;
        if (type == SDP_TYPE_STRING || type == SDP_TYPE_URL)
        {
            data->data.string.value = stream;
            data->data.string.length = elems_size;
        }
        else
        {
            /* For sequence and alternative containers, the stream should begin at the container
             * header. */
            data->data.sequence.value = stream - sizeof( BYTE );
            data->data.sequence.length = elems_size + *read + size_read;
        }
        *read += size_read + elems_size;
        break;
    }
    default:
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

/*********************************************************************
 *  BluetoothSdpGetElementData
 */
DWORD WINAPI BluetoothSdpGetElementData( BYTE *stream, ULONG stream_size, SDP_ELEMENT_DATA *data )
{
    SIZE_T read = 0;

    TRACE( "(%p, %lu, %p)\n", stream, stream_size, data );

    if (stream == NULL || stream_size < sizeof( BYTE ) || data == NULL)
        return ERROR_INVALID_PARAMETER;

    return sdp_read_element_data( stream, stream_size, data, &read );
}

/*********************************************************************
 *  BluetoothSdpGetContainerElementData
 */
DWORD WINAPI BluetoothSdpGetContainerElementData( BYTE *stream, ULONG stream_size,
                                                  HBLUETOOTH_CONTAINER_ELEMENT *handle,
                                                  SDP_ELEMENT_DATA *data )
{
    BYTE *cursor;
    DWORD result;
    SIZE_T read = 0;

    TRACE( "(%p, %lu, %p, %p)\n", stream, stream_size, handle, data );

    if (stream == NULL || stream_size < sizeof( BYTE ) || handle == NULL || data == NULL)
        return ERROR_INVALID_PARAMETER;

    cursor = (BYTE *)(*handle);

    if (cursor == NULL)
    {
        BYTE header, type, size_desc;
        UINT32 elems_size = 0;
        SIZE_T read = 0;

        header = *stream;
        type = data_elem_type( header );
        size_desc = data_elem_size_desc( header );

        if (type != SDP_TYPE_SEQUENCE && type != SDP_TYPE_ALTERNATIVE)
            return ERROR_INVALID_PARAMETER;
        if (!(size_desc >= SDP_SIZE_DESC_NEXT_UINT8 && size_desc <= SDP_SIZE_DESC_NEXT_UINT32))
            return ERROR_INVALID_PARAMETER;

        stream++;
        if (!sdp_elem_read_var_size( stream, stream_size, &read, size_desc, &elems_size ))
            return ERROR_INVALID_PARAMETER;

        stream += read;
        stream_size -= read;
    }
    else
    {
        if (cursor < stream) return ERROR_INVALID_PARAMETER;
        if (cursor == (stream + stream_size)) return ERROR_NO_MORE_ITEMS;

        stream = cursor;
        stream_size = stream_size - (cursor - stream);
    }
    result = sdp_read_element_data( stream, stream_size, data, &read );
    if (result != ERROR_SUCCESS) return result;

    stream += read;
    TRACE( "handle=%p\n", stream );
    *handle = stream;
    return ERROR_SUCCESS;
}

/*********************************************************************
 *  BluetoothSdpEnumAttributes
 */
BOOL WINAPI BluetoothSdpEnumAttributes( BYTE *stream, ULONG stream_size,
                                        PFN_BLUETOOTH_ENUM_ATTRIBUTES_CALLBACK callback, void *param )
{
    SDP_ELEMENT_DATA data = {0};
    DWORD result;
    HBLUETOOTH_CONTAINER_ELEMENT cursor = NULL;

    TRACE( "(%p, %ld, %p, %p)\n", stream, stream_size, callback, param );

    if (stream == NULL || callback == NULL) return ERROR_INVALID_PARAMETER;

    result = BluetoothSdpGetElementData( stream, stream_size, &data );
    if (result != ERROR_SUCCESS)
    {
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    switch (data.type)
    {
    case SDP_TYPE_SEQUENCE:
    case SDP_TYPE_ALTERNATIVE:
        break;
    default:
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    for (;;)
    {
        SDP_ELEMENT_DATA attrid = {0};
        SDP_ELEMENT_DATA attr = {0};
        BYTE *raw_attr_stream;

        result = BluetoothSdpGetContainerElementData( data.data.sequence.value,
                                                      data.data.sequence.length, &cursor, &attrid );
        if (result == ERROR_NO_MORE_ITEMS) return TRUE;
        if (result || !SDP_ELEMENT_IS_ATTRID( &attrid ))
        {
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        raw_attr_stream = cursor;
        result = BluetoothSdpGetContainerElementData( data.data.sequence.value,
                                                      data.data.sequence.length, &cursor, &attr );
        if (result != ERROR_SUCCESS)
        {
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }
        if (!callback( attrid.data.uint16, raw_attr_stream, ((BYTE *)cursor - raw_attr_stream),
                       param ))
            return TRUE;
    }
}

struct get_attr_value_data
{
    USHORT attr_id;
    BYTE *attr_stream;
    ULONG stream_size;
};

static BOOL WINAPI get_attr_value_callback( ULONG attr_id, BYTE *stream, ULONG stream_size,
                                            void *params )
{
    struct get_attr_value_data *args = params;
    if (attr_id == args->attr_id)
    {
        args->attr_stream = stream;
        args->stream_size = stream_size;
        return FALSE;
    }
    return TRUE;
}

/*********************************************************************
 *  BluetoothSdpGetAttributeValue
 */
DWORD WINAPI BluetoothSdpGetAttributeValue( BYTE *stream, ULONG stream_size, USHORT attr_id,
                                            SDP_ELEMENT_DATA *data )
{
    struct get_attr_value_data args = {0};

    TRACE( "(%p %lu %u %p)\n", stream, stream_size, attr_id, data );

    if (stream == NULL || data == NULL) return ERROR_INVALID_PARAMETER;

    args.attr_id = attr_id;
    if (!BluetoothSdpEnumAttributes( stream, stream_size, get_attr_value_callback, &args ))
        return ERROR_INVALID_PARAMETER;
    if (!args.attr_stream) return ERROR_FILE_NOT_FOUND;

    return BluetoothSdpGetElementData( args.attr_stream, args.stream_size, data );
}
