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
*  Address-conversion functions.
*
***************************************************************************/

#include "fnet.h"

/************************************************************************
*     Function Prototypes
*************************************************************************/
static fnet_char_t *_fnet_inet_ntop_ip4 ( const fnet_ip4_addr_t *addr, fnet_char_t *str, fnet_size_t str_len);
static fnet_return_t _fnet_inet_pton_ip4(const fnet_char_t *str, fnet_ip4_addr_t *addr);

#if FNET_CFG_IP6
    static fnet_char_t *_fnet_inet_ntop_ip6 (const fnet_ip6_addr_t *addr, fnet_char_t *str, fnet_size_t str_len);
    static fnet_return_t _fnet_inet_pton_ip6(const fnet_char_t *str, fnet_ip6_addr_t *addr);
#endif

/************************************************************************
* DESCRIPTION:The function converts an (IPv4) Internet network address
*             into a string in Internet standard dotted format.
*************************************************************************/
fnet_char_t *fnet_inet_ntoa( struct fnet_in_addr addr, fnet_char_t *res_str )
{
    return  _fnet_inet_ntop_ip4(&addr.s_addr, res_str, FNET_IP4_ADDR_STR_SIZE);
}

/************************************************************************
* DESCRIPTION:The function converts a string containing an (IPv4)
*             Internet Protocol dotted address into a suitable binary
*             representation of the Internet address.
*************************************************************************/
fnet_return_t fnet_inet_aton( fnet_char_t *cp, struct fnet_in_addr *addr )
{
    return _fnet_inet_pton_ip4(cp, &addr->s_addr);
}

/************************************************************************
* DESCRIPTION:The function converts network format IPv4 and IPv6 address
*               to presentation/text format (string).
*************************************************************************/
fnet_char_t *fnet_inet_ntop(fnet_address_family_t family, const void *addr, fnet_char_t *str, fnet_size_t str_len)
{
    fnet_char_t *result;

    switch (family)
    {
#if FNET_CFG_IP4
        case AF_INET:
            result = _fnet_inet_ntop_ip4((const fnet_ip4_addr_t *)addr, str, str_len);
            break;
#endif
#if FNET_CFG_IP6
        case AF_INET6:
            result = _fnet_inet_ntop_ip6((const fnet_ip6_addr_t *)addr, str, str_len);
            break;
#endif
        default:
            result = FNET_NULL;
            break;
    }

    return result;
}

/************************************************************************
* DESCRIPTION:The function converts from presentation format (string)
*	        to network format.
*************************************************************************/
fnet_return_t fnet_inet_pton(fnet_address_family_t family, const fnet_char_t *str, void *addr, fnet_size_t addr_len)
{
    fnet_return_t result;

    switch (family)
    {
#if FNET_CFG_IP4
        case AF_INET:
            if(addr_len < sizeof(fnet_ip4_addr_t))
            {
                result = FNET_ERR;
            }
            else
            {
                result = _fnet_inet_pton_ip4(str, (fnet_ip4_addr_t *)addr);
            }
            break;
#endif /* FNET_CFG_IP4 */
#if FNET_CFG_IP6
        case AF_INET6:
            if(addr_len < sizeof(fnet_ip6_addr_t))
            {
                result = FNET_ERR;
            }
            else
            {
                result = _fnet_inet_pton_ip6(str, (fnet_ip6_addr_t *)addr);
            }
            break;
#endif /* FNET_CFG_IP6 */
        default:
            result = FNET_ERR;
            break;
    }
    return result;
}

/************************************************************************
* DESCRIPTION:The function converts from presentation format (string)
*	        to struct fnet_sockaddr.
*************************************************************************/
fnet_return_t fnet_inet_ptos (const fnet_char_t *str, struct fnet_sockaddr *addr)
{
#if FNET_CFG_IP4
    if(fnet_inet_pton(AF_INET, str, addr->sa_data, sizeof(addr->sa_data)) == FNET_OK)
    {
        addr->sa_family = AF_INET;
    }
    else
#endif /* FNET_CFG_IP4 */
#if FNET_CFG_IP6
        if(fnet_inet_pton(AF_INET6, str, addr->sa_data, sizeof(addr->sa_data)) == FNET_OK)
        {
            addr->sa_family = AF_INET6;
        }
        else
#endif /* FNET_CFG_IP6 */
        {
            return FNET_ERR;
        }

    /* Scope ID.*/
    {
        fnet_scope_id_t     scope_id;
        const fnet_char_t   *p = fnet_strrchr(str, '%'); /* Find "%<scope id>".*/

        if(p != FNET_NULL)
        {
            scope_id = fnet_strtoul((p + 1), 0, 10u);
        }
        else
        {
            scope_id = 0u; /* Default interface.*/
        }

        addr->sa_scope_id = scope_id;
    }

    addr->sa_port = 0u;

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION:The function converts from presentation format (string)
*	        to IPv4 address.
*************************************************************************/
static fnet_return_t _fnet_inet_pton_ip4( const fnet_char_t *str, fnet_ip4_addr_t *addr)
{
    fnet_bool_t         is_digit = FNET_FALSE;
    fnet_size_t         octets = 0u;
    fnet_char_t         ch;
    fnet_uint8_t        tmp[sizeof(*addr)];
    fnet_uint8_t        *tp;
    fnet_uint32_t       val;

    *(tp = tmp) = 0u;

    while ((ch = *str++) != '\0')
    {
        /* If "c" is digit. */
        if((ch >= '0') && (ch <= '9'))
        {
            val = (fnet_uint32_t)((fnet_uint32_t) * tp * 10u + ((fnet_uint32_t)ch - '0'));

            if ((is_digit == FNET_TRUE) && (*tp == 0u))
            {
                goto ERROR;
            }

            if (val > 255u)
            {
                goto ERROR;
            }

            *tp = (fnet_uint8_t)val;

            if (is_digit == FNET_FALSE)
            {
                if (++octets > 4u)
                {
                    goto ERROR;
                }

                is_digit = FNET_TRUE;
            }
        }
        else if ((ch == '.') && (is_digit == FNET_TRUE))
        {
            if (octets == 4u)
            {
                goto ERROR;
            }

            *++tp = 0u;
            is_digit = FNET_FALSE;
        }
        else
        {
            goto ERROR;
        }
    }
    if (octets < 4u)
    {
        goto ERROR;
    }

    fnet_memcpy(addr, tmp, sizeof(*addr));

    return (FNET_OK);
ERROR:
    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION:The function converts from presentation format (string)
*	        to IPv6 address.
*************************************************************************/
#if FNET_CFG_IP6
static fnet_return_t _fnet_inet_pton_ip6( const fnet_char_t *str, fnet_ip6_addr_t *addr )
{
    const fnet_char_t   xdigits_l[] = "0123456789abcdef";
    const fnet_char_t   xdigits_u[] = "0123456789ABCDEF";
    fnet_uint8_t        tmp[sizeof(*addr)];
    fnet_uint8_t        *tp;
    fnet_uint8_t        *endp;
    fnet_uint8_t        *colonp;
    const fnet_char_t   *xdigits;
    fnet_char_t         ch;
    fnet_size_t         seen_xdigits;
    fnet_uint32_t       val;

    fnet_memset_zero((tp = tmp), sizeof(*addr));
    endp = tp + sizeof(*addr);
    colonp = FNET_NULL;

    /* Leading :: */
    if ((*str == ':') && (*++str != ':'))
    {
        goto ERROR;
    }

    seen_xdigits = 0u;
    val = 0u;

    while (((ch = *str++) != '\0') && (ch != '%'))
    {
        const fnet_char_t *pch;

        if ((pch = fnet_strchr((xdigits = xdigits_l), ch)) == FNET_NULL)
        {
            pch = fnet_strchr((xdigits = xdigits_u), ch);
        }

        if (pch != FNET_NULL)
        {
            val <<= 4;
            val |= (fnet_uint32_t)(pch - xdigits);
            if (++seen_xdigits > 4u)
            {
                goto ERROR;
            }

            continue;
        }
        if (ch == ':')
        {
            if (!seen_xdigits)
            {
                if (colonp)
                {
                    goto ERROR;
                }

                colonp = tp;
                continue;
            }
            else if (*str == '\0')
            {
                goto ERROR;
            }
            else
            {}

            if (tp + 2 > endp)
            {
                goto ERROR;
            }

            *tp++ = (fnet_uint8_t) ((val >> 8) & 0xffu);
            *tp++ = (fnet_uint8_t) (val & 0xffu);
            seen_xdigits = 0u;
            val = 0u;
            continue;
        }
        goto ERROR;
    }

    if (seen_xdigits)
    {
        if (tp + 2 > endp)
        {
            goto ERROR;
        }

        *tp++ = (fnet_uint8_t) ((val >> 8) & 0xffu);
        *tp++ = (fnet_uint8_t) (val & 0xffu);
    }

    if (colonp != FNET_NULL)
    {
        /*
         *  Shift.
         */
        fnet_size_t n = (fnet_size_t)(tp - colonp);
        fnet_size_t i;

        if (tp == endp)
        {
            goto ERROR;
        }
        for (i = 1u; i <= n; i++)
        {
            endp[-((fnet_int32_t)i)] = colonp[n - i];
            colonp[n - i] = 0u;
        }
        tp = endp;
    }
    if (tp != endp)
    {
        goto ERROR;
    }

    fnet_memcpy(addr, tmp, sizeof(*addr));
    return (FNET_OK);
ERROR:
    return (FNET_ERR);
}

#endif /* FNET_CFG_IP6 */

/************************************************************************
* DESCRIPTION:The function converts IPv4 address
*               to presentation format (string).
*************************************************************************/
static fnet_char_t *_fnet_inet_ntop_ip4 ( const fnet_ip4_addr_t *addr, fnet_char_t *str, fnet_size_t str_len)
{
    fnet_char_t            tmp[FNET_IP4_ADDR_STR_SIZE];
    fnet_size_t            length;
    const fnet_uint8_t     *ptr = (const fnet_uint8_t *) addr;

    length = fnet_snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);

    if ((length == 0u) || (length >= str_len))
    {
        return (FNET_NULL);
    }
    else
    {
        fnet_strlcpy(str, tmp, str_len);

        return (str);
    }
}

/************************************************************************
* DESCRIPTION:The function converts IPv6 binary address into
*               presentation (printable) format.
*************************************************************************/
#if FNET_CFG_IP6
static fnet_char_t *_fnet_inet_ntop_ip6 (const fnet_ip6_addr_t *addr, fnet_char_t *str, fnet_size_t str_len)
{
    fnet_char_t                         tmp[FNET_IP6_ADDR_STR_SIZE];
    fnet_char_t                         *tp;
    struct
    {
        fnet_int32_t base, len;
    }  best, cur;
    fnet_uint32_t                       words[16 / 2];
    fnet_index_t                        i;

    /*
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in addr[] for :: shorthanding.
     */
    fnet_memset_zero(words, sizeof(words));
    for (i = 0u; i < 16u; i++)
    {
        words[i / 2u] |= ((fnet_uint32_t)addr->addr[i] << ((1u - (i % 2u)) << 3u));
    }

    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;

    for (i = 0u; i < (16u / 2u); i++)
    {
        if (words[i] == 0u)
        {
            if (cur.base == -1)
            {
                cur.base = (fnet_int32_t)i;
                cur.len = 1;
            }
            else
            {
                cur.len++;
            }
        }
        else
        {
            if (cur.base != -1)
            {
                if ((best.base == -1) || (cur.len > best.len))
                {
                    best = cur;
                }
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1)
    {
        if ((best.base == -1) || (cur.len > best.len))
        {
            best = cur;
        }
    }
    if ((best.base != -1) && (best.len < 2))
    {
        best.base = -1;
    }

    /* Format the result. */
    tp = tmp;
    for (i = 0u; i < (16u / 2u); i++)
    {
        /* Are we inside the best run of 0x00's? */
        if ((best.base != -1) && ((fnet_int32_t)i >= best.base) &&
            ((fnet_int32_t)i < (best.base + best.len)))
        {
            if ((fnet_int32_t)i == best.base)
            {
                *tp++ = ':';
            }
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0u)
        {
            *tp++ = ':';
        }

        tp += fnet_sprintf(tp, "%x", words[i]);
    }
    /* Was it a trailing run of 0x00's? */
    if ((best.base != -1) && ((best.base + best.len) ==  (16 / 2)))
    {
        *tp++ = ':';
    }

    *tp++ = '\0';

    /* Check for overflow, copy, and we're done. */
    if ((tp - tmp) > (fnet_int32_t)str_len)
    {
        return (FNET_NULL);
    }
    fnet_strlcpy(str, tmp, str_len);
    return (str);
}
#endif /* FNET_CFG_IP6 */
