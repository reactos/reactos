/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  Internet checksum implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_checksum.h"
#include "fnet_checksum_prv.h"
#include "fnet_ip4.h"

static fnet_uint32_t _fnet_checksum32_nb(fnet_netbuf_t *nb, fnet_size_t length);

#if !FNET_CFG_OVERLOAD_CHECKSUM_LOW
    static fnet_uint32_t _fnet_checksum32_low(fnet_uint32_t sum, fnet_size_t length, const fnet_uint16_t *d_ptr);
#endif

/*RFC:
    The checksum field is the 16 bit one's complement of the one's
    complement sum of all 16 bit words in the header and text.  If a
    segment contains an odd number of header and text octets to be
    checksummed, the last octet is padded on the right with zeros to
    form a 16 bit word for checksum purposes.  The pad is not
    transmitted as part of the segment.  While computing the checksum,
    the checksum field itself is replaced with zeros.
*/

/************************************************************************
* DESCRIPTION: Calculates Internet checksum of nb chain.
*************************************************************************/
#if !FNET_CFG_OVERLOAD_CHECKSUM_LOW

static fnet_uint32_t _fnet_checksum32_low(fnet_uint32_t sum, fnet_size_t length, const fnet_uint16_t *d_ptr)
{
    fnet_uint16_t   p_byte1;
    fnet_int32_t   current_length = (fnet_int32_t)length;

    while((current_length -= 32) >= 0)
    {
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
    }
    current_length += 32;

    while((current_length -= 8) >= 0)
    {
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
        sum += *d_ptr++;
    }
    current_length += 8;

    while((current_length -= 2) >= 0)
    {
        sum += *d_ptr++;
    }

    current_length += 2;
    if(current_length)
    {
        p_byte1 = (fnet_uint16_t)((*((const fnet_uint16_t *)d_ptr)) & FNET_NTOHS(0xFF00u));
        sum += (fnet_uint16_t)p_byte1;
    }
    return sum;
}
#else

extern fnet_uint32_t _fnet_checksum32_low(fnet_uint32_t sum, fnet_size_t length, const fnet_uint16_t *d_ptr);

#endif

static fnet_uint32_t _fnet_checksum32_nb(fnet_netbuf_t *nb, fnet_size_t length)
{
    fnet_netbuf_t   *tmp_nb;
    fnet_uint16_t   *d_ptr;
    fnet_uint16_t   p_byte2;
    fnet_uint32_t   sum = 0U;
    fnet_int32_t    current_length;
    fnet_int32_t    len = (fnet_int32_t)length;

    tmp_nb = nb;

    d_ptr = (fnet_uint16_t *)tmp_nb->data_ptr;               /* Store the data pointer of the 1st net_buf.*/

    if(nb->length > (fnet_size_t)len)
    {
        current_length = len;          /* If no more net_bufs to proceed.*/
    }
    else
    {
        current_length = (fnet_int32_t)nb->length;   /* Or full first net_buf.*/
    }

    while(len)
    {
        len -= current_length;       /* current_length bytes should be proceeded.*/

        sum = _fnet_checksum32_low(sum, (fnet_size_t)current_length, d_ptr);

        if(len)
        {
            tmp_nb = tmp_nb->next;
            d_ptr = (fnet_uint16_t *)tmp_nb->data_ptr;

            if(((fnet_size_t)current_length & 1u) != 0u)
            {
                /* If previous fragment was odd, add in first byte in lower 8 bits. */
                p_byte2 = fnet_ntohs(((fnet_uint16_t)(*((fnet_uint8_t *)d_ptr)) & 0x00FFU));
                d_ptr = (fnet_uint16_t *)((fnet_uint8_t *)d_ptr + 1);

                sum += (fnet_uint16_t)p_byte2;
                len--; /* len still > 0, no check is needed */
                current_length = -1;
            }
            else
            {
                current_length = 0;
            }


            if(tmp_nb->length > (fnet_size_t)len)
            {
                current_length = len;
            }
            else
            {
                current_length += (fnet_int32_t)tmp_nb->length;
            }
        }
    }

    return sum;
}

/************************************************************************
* DESCRIPTION: Calculates one's complement (Internet) checksum.
*************************************************************************/
fnet_uint16_t _fnet_checksum_netbuf(fnet_netbuf_t *nb, fnet_size_t len)
{
    fnet_uint32_t sum = _fnet_checksum32_nb(nb, len);

    /* Add potential carries - no branches. */

    sum += 0xffffU; /* + 0xffff acording to RFC1624*/

    /* Add accumulated carries */
    while ((sum >> 16) != 0u)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    return (fnet_uint16_t)(0xffffu & ~sum);
}

/************************************************************************
* DESCRIPTION: Calculates one's complement (Internet) checksum
*              for a buffer.
*************************************************************************/
fnet_uint16_t fnet_checksum(fnet_uint8_t *buf, fnet_size_t buf_len)
{
    fnet_uint32_t sum = _fnet_checksum32_low(0U, buf_len, (fnet_uint16_t *)buf);

    sum += 0xffffU; /* + 0xffff acording to RFC1624*/

    /* Add accumulated carries */
    while ((sum >> 16) != 0u)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    return (fnet_uint16_t)(0xffffu & ~sum);
}

/************************************************************************
* DESCRIPTION: Calculates one's complement (Internet) checksum of
*              the IP pseudo header, for a buffer.
*************************************************************************/
fnet_uint16_t fnet_checksum_pseudo(fnet_uint8_t *buf, fnet_uint16_t buf_len, fnet_uint16_t protocol, const fnet_uint8_t *ip_src, const fnet_uint8_t *ip_dest, fnet_size_t addr_size )
{
    fnet_uint32_t sum = _fnet_checksum32_low(0U, (fnet_size_t)buf_len, (fnet_uint16_t *)buf);

    sum += (fnet_uint32_t)protocol + (fnet_uint32_t)fnet_htons(buf_len);
    sum = _fnet_checksum32_low(sum, addr_size, (const fnet_uint16_t *)ip_src);
    sum = _fnet_checksum32_low(sum, addr_size, (const fnet_uint16_t *)ip_dest);

    /* Add accumulated carries */
    while ((sum >> 16) != 0u)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    return (fnet_uint16_t)(0xffffu & ~sum);
}

/************************************************************************
* DESCRIPTION: Calculates  one's complement (Internet) checksum of
*              the IP pseudo header
*************************************************************************/
fnet_uint16_t _fnet_checksum_pseudo_netbuf_start( fnet_netbuf_t *nb, fnet_uint16_t protocol, fnet_uint16_t protocol_len )
{
    fnet_uint32_t sum = _fnet_checksum32_nb(nb, (fnet_size_t)protocol_len);

    sum += (fnet_uint32_t)protocol + (fnet_uint32_t)fnet_htons(protocol_len);

    sum += 0xffffu; /*  + 0xffff acording to RFC1624*/

    /* Add in accumulated carries */
    while ( (sum >> 16) != 0u)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }
    return (fnet_uint16_t)(sum);
}

/************************************************************************
* DESCRIPTION: Calculates  one's complement (Internet) checksum of
*              the IP pseudo header
*************************************************************************/
fnet_uint16_t _fnet_checksum_pseudo_netbuf_end( fnet_uint16_t sum_s, const fnet_uint8_t *ip_src, const fnet_uint8_t *ip_dest, fnet_size_t addr_size )
{
    fnet_uint32_t sum = 0U;

    sum = sum_s;

    sum = _fnet_checksum32_low(sum, addr_size, (const fnet_uint16_t *)ip_src);
    sum = _fnet_checksum32_low(sum, addr_size, (const fnet_uint16_t *)ip_dest);

    sum += 0xffffU; /* Add in accumulated carries + 0xffff acording to RFC1624*/

    /* Add accumulated carries */
    while ( (sum >> 16) != 0u)
    {
        sum = (sum & 0xffffu) + (sum >> 16);
    }

    return (fnet_uint16_t)(0xffffu & ~sum);
}
