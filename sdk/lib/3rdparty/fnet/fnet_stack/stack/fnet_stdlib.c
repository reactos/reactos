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
*  Standard functions implementation.
*
***************************************************************************/

#include "fnet.h"

static fnet_uint32_t fnet_rand_seed;  /* Used by fnet_rand()*/

#if !FNET_CFG_OVERLOAD_MEMCPY
/************************************************************************
* DESCRIPTION:
*************************************************************************/
#if 0
/* Slowest && Smallest */
void fnet_memcpy( FNET_COMP_PACKED_VAR void *dest, FNET_COMP_PACKED_VAR const void *src, fnet_size_t n )
{
    const fnet_uint8_t  *p = (fnet_uint8_t *)src;
    fnet_uint8_t        *q = (fnet_uint8_t *)dest;

    for (n++; --n; )
    {
        *q++ = *p++;
    }
}
#elif 0 /* Faster. */
void fnet_memcpy (FNET_COMP_PACKED_VAR void *dest, FNET_COMP_PACKED_VAR const void *src, fnet_size_t n)
{
    fnet_size_t                         longs;
    fnet_size_t                         bytes;
    FNET_COMP_PACKED_VAR fnet_uint32_t  *dpl = (fnet_uint32_t *)dest;
    FNET_COMP_PACKED_VAR fnet_uint32_t  *spl = (fnet_uint32_t *)src;
    fnet_uint8_t                        *dpb, *spb;

    bytes = (n & 0x3);
    longs = ((n - bytes) >> 2);

    for (longs++; --longs;)
    {
        *dpl++ = *spl++;
    }

    dpb = (fnet_uint8_t *)dpl;
    spb = (fnet_uint8_t *)spl;

    for (bytes++; --bytes;)
    {
        *dpb++ = *spb++;
    }
}
#else /* Fastest & Biggest. */
void fnet_memcpy(FNET_COMP_PACKED_VAR void *to_ptr, FNET_COMP_PACKED_VAR const void *from_ptr, fnet_size_t number_of_bytes)
{
    const fnet_uint8_t                          *from8_ptr = (const fnet_uint8_t *) from_ptr;
    fnet_uint8_t                                *to8_ptr = (fnet_uint8_t *) to_ptr;
    FNET_COMP_PACKED_VAR const fnet_uint16_t    *from16_ptr = (const fnet_uint16_t *) from_ptr;
    FNET_COMP_PACKED_VAR fnet_uint16_t          *to16_ptr = (fnet_uint16_t *) to_ptr;
    FNET_COMP_PACKED_VAR const fnet_uint32_t    *from32_ptr = (const fnet_uint32_t *) from_ptr;
    FNET_COMP_PACKED_VAR fnet_uint32_t          *to32_ptr = (fnet_uint32_t *) to_ptr;
    fnet_index_t                                loops;

    /*
    * The copying is optimized to avoid alignment problems, and attempts
    *               to copy 32bit numbers optimally.
    */
    if (number_of_bytes > 3u)
    {
        /* Try to align source on word */
        if (((fnet_uint32_t)from_ptr & 1u) != 0u)
        {
            from8_ptr = (const fnet_uint8_t *) from_ptr;
            to8_ptr = (fnet_uint8_t *) to_ptr;

            *to8_ptr++ = *from8_ptr++;

            from_ptr = from8_ptr;
            to_ptr = to8_ptr;
            --number_of_bytes;
        }

        /* Try to align source on longword */
        if ((((fnet_uint32_t)from_ptr) & 2u) != 0u)
        {
            from16_ptr = (const fnet_uint16_t *) from_ptr;
            to16_ptr = (fnet_uint16_t *) to_ptr;

            *to16_ptr++ = *from16_ptr++;

            from_ptr = from16_ptr;
            to_ptr = to16_ptr;
            number_of_bytes -= 2u;
        }

        from32_ptr = (const fnet_uint32_t *) from_ptr;
        to32_ptr = (fnet_uint32_t *) to_ptr;

        /*
        ** To increase performance a bit, we will copy 64 bytes (16 * longwords) sequentially
        ** This gets less instruction cycles.
        */
        for (loops = (number_of_bytes >> 6u); loops > 0u; loops--)
        {
            /* copy 16 longwords */
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
            *to32_ptr++ = *from32_ptr++;
        }

        /* Now, write the rest of bytes */
        for (loops = ((number_of_bytes >> 2) & 0xFu); loops > 0u; loops--)
        {
            *to32_ptr++ = *from32_ptr++;
        }

        from_ptr = from32_ptr;
        to_ptr = to32_ptr;
    }

    /* Copy all remaining bytes */
    if ((number_of_bytes & 2u) != 0u)
    {
        from16_ptr = (const fnet_uint16_t *) from_ptr;
        to16_ptr = (fnet_uint16_t *) to_ptr;

        *to16_ptr++ = *from16_ptr++;

        from_ptr = from16_ptr;
        to_ptr = to16_ptr;
    }

    if ((number_of_bytes & 1u) != 0u)
    {
        * (fnet_uint8_t *) to_ptr = * (const fnet_uint8_t *) from_ptr;
    }
}
#endif

#endif

/************************************************************************
* DESCRIPTION: Copy function to other location.
*              Used for relocate function from Flash to RAM
*
*************************************************************************/
void *fnet_memcpy_func(void *to_buf_ptr, const void *from_func_ptr, fnet_size_t to_buf_size)
{

    fnet_memcpy(to_buf_ptr, (void *)FNET_CPU_INSTRUCTION_TO_ADDR(from_func_ptr), to_buf_size);

    return (void *)FNET_CPU_ADDR_TO_INSTRUCTION(to_buf_ptr);
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_memset( void *dest, fnet_uint8_t value, fnet_size_t n )
{
    /* Not optimized */
    fnet_uint8_t *sp = (fnet_uint8_t *)dest;

    while(n)
    {
        *sp++ = (fnet_uint8_t)value;
        n--;
    }
}

/************************************************************************
* DESCRIPTION: Same as "fnet_memset( void *s, 0, unsigned n )"
*************************************************************************/
void fnet_memset_zero( void *dest, fnet_size_t n )
{
    /* Not optimized */
    fnet_uint8_t *sp = (fnet_uint8_t *)dest;

    while(n)
    {
        *sp++ = 0u;
        n--;
    }
}

/************************************************************************
* DESCRIPTION: Compare two memory regions and return zero if they are identical,
*              non-zero otherwise.  If count is zero, return zero.
*************************************************************************/
fnet_int32_t fnet_memcmp(const void *src1, const void *src2, fnet_size_t count )
{
    const fnet_uint8_t  *p1 = (const fnet_uint8_t *)src1;
    const fnet_uint8_t  *p2 = (const fnet_uint8_t *)src2;
    fnet_int32_t        result = 0;

    while(count)
    {
        result = (fnet_int32_t)(*p1 - *p2);

        if(result != 0)
        {
            break;
        }

        count--;
        ++p1;
        ++p2;
    }

    return result;
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
fnet_int32_t fnet_strcmp( const fnet_char_t *str1, const fnet_char_t *str2 )
{
    /* No checks for 0 */
    const fnet_char_t *s1p = str1;
    const fnet_char_t *s2p = str2;

    while(*s2p != '\0')
    {
        if(*s1p != *s2p)
        {
            break;
        }

        ++s1p;
        ++s2p;
    }

    return (fnet_int32_t)(*s1p - *s2p);
}

/************************************************************************
* DESCRIPTION: Calculates the length of a string.
*************************************************************************/
fnet_size_t fnet_strlen (const fnet_char_t *str)
{
    const fnet_char_t  *s = str;
    fnet_size_t         len = 0U;

    if(s)
    {
        while (*s++ != '\0')
        {
            len++;
        }
    }
    return len;
}

/************************************************************************
* DESCRIPTION: Calculate the length of a fixed-size string.
*************************************************************************/
fnet_size_t fnet_strnlen (const fnet_char_t *str, fnet_size_t max_len)
{
    const fnet_char_t  *s = str;
    fnet_size_t         len = 0U;

    if(s)
    {
        while (*s++ != '\0')
        {
            if(max_len == len)
            {
                break;
            }
            else
            {
                len++;
            }
        }
    }

    return len;
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_strcat (fnet_char_t *dest, const fnet_char_t *src)
{
    fnet_char_t        *dp;
    const fnet_char_t  *sp = src;

    if ((dest != 0) && (src != 0))
    {
        dp = &dest[fnet_strlen(dest)];

        while (*sp != '\0')
        {
            *dp++ = *sp++;
        }
        *dp = '\0';
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_strlcat (fnet_char_t *dest, const fnet_char_t *src, fnet_size_t n)
{
    fnet_char_t        *dp;
    const fnet_char_t  *sp = src;

    if ((dest != 0) && (src != 0) && (n > 0U))
    {
        dp = &dest[fnet_strlen(dest)];

        while((*sp != '\0') && (n > 0u))
        {
            *dp++ = *sp++;
            n--;
        }
        *dp = '\0';
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_strcpy (fnet_char_t *dest, const fnet_char_t *src)
{
    fnet_char_t        *dp = dest;
    const fnet_char_t  *sp = src;

    if ((dest != 0) && (src != 0))
    {
        while (*sp != '\0')
        {
            *dp++ = *sp++;
        }
        *dp = '\0';
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
void fnet_strlcpy( fnet_char_t *dest, const fnet_char_t *src, fnet_size_t n )
{
    fnet_char_t        *dp = dest;
    const fnet_char_t  *sp = src;

    if((dest != 0) && (src != 0) && (n > 0u))
    {
        while((*sp != '\0') && (n-- > 0u))
        {
            *dp++ = *sp++;
        }

        *dp = '\0';
    }
}

/************************************************************************
* DESCRIPTION: The function fnet_strrchr() returns a pointer to the last
* occurrence of chr in str, or FNET_NULL if no match is found.
*************************************************************************/
fnet_char_t *fnet_strrchr(const fnet_char_t *str, fnet_char_t chr )
{
    const fnet_char_t  *p = str;
    const fnet_char_t  *q = 0;
    fnet_char_t        c = chr;
    fnet_char_t        ch = *p++;

    while(ch)
    {
        if(ch == c)
        {
            q = p - 1;
        }

        ch = *p++;
    }

    if(q)
    {
        return (fnet_char_t *)(q);
    }

    return (c ? FNET_NULL : (fnet_char_t *)(p - 1));
}
/************************************************************************
* DESCRIPTION: The function fnet_strchr() returns a pointer to the first
* occurence of chr in str, or 0 if chr is not found.
*************************************************************************/
fnet_char_t *fnet_strchr( const fnet_char_t *str, fnet_char_t chr )
{
    const fnet_char_t   *p = str;
    fnet_char_t         c = chr;
    fnet_char_t         ch = *p++;

    while(ch)
    {
        if(ch == c)
        {
            return (fnet_char_t *)(p - 1);
        }

        ch = *p++;
    }

    return (fnet_char_t *)(c ? FNET_NULL : (p - 1));
}

/************************************************************************
* DESCRIPTION: The function fnet_strstr() returns a pointer to the first
* occurrence of substr in str, or 0 if no match is found.
* If the length of pat is zero, then fnet_strstr() will
* simply return str.
*************************************************************************/
fnet_char_t *fnet_strstr( const fnet_char_t *str, const fnet_char_t *substr )
{
    const fnet_char_t  *s1 = str;
    const fnet_char_t  *p1 = substr;
    fnet_uint8_t        firstc, c1, c2;

    if((substr == 0) || (!((firstc = *p1++) != 0u)) )
    {
        return (fnet_char_t *)(str);
    }

    c1 = *s1++;

    while(c1)
    {
        if(c1 == firstc)
        {
            const fnet_char_t *s2 = s1;
            const fnet_char_t *p2 = p1;

            while(((c1 = *s2++) == (c2 = *p2++)) && c1)
            {}

            if(!c2)
            {
                return ((fnet_char_t *)(s1 - 1));
            }
        }

        c1 = *s1++;
    }

    return (0);
}

/************************************************************************
* DESCRIPTION: The fnet_strncmp() function compares at most count characters
* of str1 and str2.
*************************************************************************/
fnet_int32_t fnet_strncmp( const fnet_char_t *str1, const fnet_char_t *str2, fnet_size_t n )
{
    const fnet_char_t  *p1 = str1;
    const fnet_char_t  *p2 = str2;
    fnet_uint8_t        c1, c2;

    n++;

    while(--n)
    {
        if((c1 = *p1++) != (c2 = *p2++))
        {
            return (fnet_int32_t)(c1 - c2);
        }
        else if(!c1)
        {
            break;
        }
        else
        {}
    }

    return (0);
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
fnet_uint32_t fnet_strtoul (const fnet_char_t *str, fnet_char_t **ptr, fnet_size_t base)
{
    fnet_uint32_t   rvalue;
    fnet_bool_t     err;
    fnet_bool_t     neg;
    fnet_char_t     c;
    fnet_char_t     *endp;
    fnet_char_t     *startp;

    rvalue = 0u;
    err = FNET_FALSE;
    neg = FNET_FALSE;

    /* Check for invalid arguments */
    if ((str == 0) || (base == 1u) || (base > 36u))
    {
        if (ptr != 0)
        {
            *ptr = (fnet_char_t *)str;
        }
        return 0u;
    }

    /* Skip leading white spaces */
    for (startp = (fnet_char_t *)str; ((*startp == ' ') || (*startp == '\t')) ; ++startp)
    {}

    /* Check for notations */
    switch (startp[0])
    {
        case '0':
            if ((startp[1] == 'x') || (startp[1] == 'X'))
            {
                if ((base == 0u) || (base == 16u))
                {
                    base = 16u;
                    startp = &startp[2];
                }
            }
            break;
        case '-':
            neg = FNET_TRUE;
            startp = &startp[1];
            break;
        default:
            break;
    }

    if (base == 0u)
    {
        base = 10u;
    }

    /* Check for invalid chars in str */
    for ( endp = startp; (err == FNET_FALSE) && ((c = (*endp)) != '\0') && (!((*endp == ' ') || (*endp == '\t'))); ++endp)
    {
        /* Check for 0..9,Aa-Zz */
        if (!(((c >= '0') && (c <= '9')) ||
              ((c >= 'A') && (c <= 'Z')) ||
              ((c >= 'a') && (c <= 'z'))))
        {
            err = FNET_TRUE;
        }
        else
        {
            /* Convert fnet_char_t to num in 0..36 */
            if ((c >= '0') && (c <= '9')) /* Is digit.*/
            {
                c = c - '0';
            }
            else
            {
                if ((c >= 'A') && (c <= 'Z')) /* Is upper.*/
                {
                    c = c - 'A' + 10u;
                }
                else
                {
                    c = c - 'a' + 10u;
                }
            }

            /* check c against base */
            if (c >= base)
            {
                err = FNET_TRUE;
            }
            else
            {
                if (neg)
                {
                    rvalue = (rvalue * base) - (fnet_uint32_t)c;
                }
                else
                {
                    rvalue = (rvalue * base) + (fnet_uint32_t)c;
                }
            }
        }
    }

    /* Upon exit, endp points to the character at which valid info */
    /* STOPS.  No chars including and beyond endp are used.        */

    if (ptr != 0)
    {
        *ptr = endp;
    }

    if (err)
    {
        if (ptr != 0)
        {
            *ptr = (fnet_char_t *)str;
        }
        return 0u;
    }
    else
    {
        return rvalue;
    }
}

/************************************************************************
* DESCRIPTION: This function converts an uppercase letter to the corresponding
* lowercase letter.
*************************************************************************/
fnet_char_t fnet_tolower( fnet_char_t to_lower )
{
    if((to_lower >= 'A') && (to_lower <= 'Z'))
    {
        return (fnet_uint8_t)(to_lower + 0x20u);
    }

    return to_lower;
}

/************************************************************************
* DESCRIPTION: The fnet_strcasecmp() function compares the two strings s1
* and s2, ignoring the case of the characters. It returns an
* integer less than, equal to, or greater than zero if s1 is found,
* respectively, to be less than, to match, or be greater than s2.
*************************************************************************/
fnet_int32_t fnet_strcasecmp( const fnet_char_t *str1, const fnet_char_t *str2 )
{
    fnet_uint8_t c1, c2;

    while(1)
    {
        c1 = fnet_tolower(*str1++);
        c2 = fnet_tolower(*str2++);

        if(c1 < c2)
        {
            return -1;
        }

        if(c1 > c2)
        {
            return 1;
        }

        if(c1 == 0u)
        {
            return 0;
        }
    }
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
fnet_int32_t fnet_strcmp_splitter( const fnet_char_t *in_str, const fnet_char_t *name, fnet_char_t splitter)
{
    fnet_int32_t result;

    /* No checks for 0 */
    const fnet_char_t *s1p = in_str;
    const fnet_char_t *s2p = name;

    while (*s2p == ' ')
    {
        s2p++;  /* Strip leading spaces */
    }
    while (*s2p == splitter)
    {
        s2p++;	/* Strip heading slash */
    }

    while (*s1p == ' ')
    {
        s1p++;  /* Strip leading spaces */
    }

    /* Special case for root folder.*/
    if((*s2p == '\0') && (*s1p == splitter))
    {
        result = 0;
    }
    else
    {
        while (*s1p == splitter)
        {
            s1p++;	/* Strip heading slash */
        }

        while((*s2p != '\0') && (*s1p == *s2p))
        {
            ++s1p;
            ++s2p;

            if (*s1p == splitter)
            {
                break; /* next element */
            }
        }

        if(*s1p == splitter)
        {
            result = 0;
        }
        else
        {
            result = (fnet_int32_t )(*s1p - *s2p);
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Breaks a string into a sequence of tokens.
*************************************************************************/
fnet_char_t *fnet_strtok_r(fnet_char_t *str, const fnet_char_t *delimiter, fnet_char_t **last)
{
    const fnet_char_t  *spanp;
    fnet_char_t        c;
    fnet_char_t        sc;
    fnet_char_t        *tok;

    if ((str == FNET_NULL) && ((str = *last) == FNET_NULL))
    {
        return (FNET_NULL);
    }

    /*
     * Skip leading delimiters.
     */
CONT:
    c = (*str++);
    spanp = delimiter;
    while( (sc = (*spanp++)) != 0u)
    {
        if (c == sc)
        {
            goto CONT;
        }
    }

    if (c == 0u) /* No non-delimiter characters */
    {
        *last = FNET_NULL;
        return (FNET_NULL);
    }
    tok = str - 1;

    /*
     * Scan token.
     */
    for (;;)
    {
        c = (*str++);
        spanp = delimiter;
        do
        {
            if ((sc = (*spanp++)) == c)
            {
                if (c == 0u)
                {
                    str = FNET_NULL;
                }
                else
                {
                    str[-1] = 0u;
                }
                *last = str;
                return (tok);
            }
        }
        while (sc != 0u);
    }
    /* Not reached.*/
}

/************************************************************************
* DESCRIPTION: Generates a pseudo-random number.
*************************************************************************/
fnet_uint32_t fnet_rand(void)
{
#if 0 /* original */
    fnet_rand_seed = fnet_rand_seed * 1103515245u + 12345u;
    return((fnet_uint32_t)(fnet_rand_seed >> 16u) % (FNET_RAND_MAX + 1u));
#else /* Park-Miller minimum random number generator (Comm ACM Oct 1988 p1192-1201, Vol 31 Num 10) */
    fnet_uint64_t const a    = 16807;      /* 7^5 (a primitive root modulo M31) */
    fnet_uint64_t const m    = 2147483647; /* 2^31-1 (a Mersenne prime M31) */
    fnet_uint64_t tmp;

    tmp = fnet_rand_seed * a;
    fnet_rand_seed = (fnet_uint32_t)(tmp % m);
    return(fnet_rand_seed);
#endif
}

/************************************************************************
* DESCRIPTION: Initializes the pseudo-random number generator.
*************************************************************************/
void fnet_srand(fnet_uint32_t seed)
{
    fnet_rand_seed += seed;
}
