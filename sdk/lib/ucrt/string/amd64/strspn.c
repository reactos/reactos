/***
*strspn.c - find length of initial substring of chars from a control string
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strspn() - finds the length of the initial substring of
*       a string consisting entirely of characters from a control string.
*
*       defines strcspn()- finds the length of the initial substring of
*       a string consisting entirely of characters not in a control string.
*
*       defines strpbrk()- finds the index of the first character in a string
*       that is not in a control string
*
*******************************************************************************/

#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 Prefast doesn't understand reading past buffer but staying on same page.
#pragma warning(disable:__WARNING_RETURNING_BAD_RESULT) // 28196

/* Determine which routine we're compiling for (default to STRSPN) */
#define _STRSPN         1
#define _STRCSPN        2
#define _STRPBRK        3

#if defined (SSTRCSPN)
    #define ROUTINE _STRCSPN
#elif defined (SSTRPBRK)
    #define ROUTINE _STRPBRK
#else
    #define ROUTINE _STRSPN
    #define STRSPN_USE_SSE2
#endif

/***
*int strspn(string, control) - find init substring of control chars
*
*Purpose:
*       Finds the index of the first character in string that does belong
*       to the set of characters specified by control.  This is
*       equivalent to the length of the initial substring of string that
*       consists entirely of characters from control.  The '\0' character
*       that terminates control is not considered in the matching process.
*
*Entry:
*       char *string - string to search
*       char *control - string containing characters not to search for
*
*Exit:
*       returns index of first char in string not in control
*
*Exceptions:
*
*******************************************************************************/

/***
*int strcspn(string, control) - search for init substring w/o control chars
*
*Purpose:
*       returns the index of the first character in string that belongs
*       to the set of characters specified by control.  This is equivalent
*       to the length of the length of the initial substring of string
*       composed entirely of characters not in control.  Null chars not
*       considered.
*
*Entry:
*       char *string - string to search
*       char *control - set of characters not allowed in init substring
*
*Exit:
*       returns the index of the first char in string
*       that is in the set of characters specified by control.
*
*Exceptions:
*
*******************************************************************************/

/***
*char *strpbrk(string, control) - scans string for a character from control
*
*Purpose:
*       Finds the first occurence in string of any character from
*       the control string.
*
*Entry:
*       char *string - string to search in
*       char *control - string containing characters to search for
*
*Exit:
*       returns a pointer to the first character from control found
*       in string.
*       returns NULL if string and control have no characters in common.
*
*Exceptions:
*
*******************************************************************************/



/* Routine prototype */
#if ROUTINE == _STRSPN
#if defined(STRSPN_USE_SSE2)
__declspec(noinline)
static size_t __cdecl fallbackMethod(
#else
size_t __cdecl strspn (
#endif
#elif ROUTINE == _STRCSPN
#if defined(STRSPN_USE_SSE2)
__declspec(noinline)
static size_t __cdecl fallbackMethod(
#else
size_t __cdecl strcspn (
#endif
#else  /* ROUTINE == _STRCSPN */
#if defined(STRSPN_USE_SSE2)
__declspec(noinline)
static char * __cdecl fallbackMethod(
#else
char * __cdecl strpbrk (
#endif
#endif  /* ROUTINE == _STRCSPN */
        const char * string,
        const char * control
        )
{
        const unsigned char *str = (unsigned char const*)string;
        const unsigned char *ctrl = (unsigned char const*)control;

        unsigned char map[32];
        int count;

        /* Clear out bit map */
        for (count=0; count<32; count++)
                map[count] = 0;

        /* Set bits in control map */
        while (*ctrl)
        {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
                ctrl++;
        }

#if ROUTINE == _STRSPN

        /* 1st char NOT in control map stops search */
        if (*str)
        {
                count=0;
                while (map[*str >> 3] & (1 << (*str & 7)))
                {
                        count++;
                        str++;
                }
                return(count);
        }
        return(0);

#elif ROUTINE == _STRCSPN

        /* 1st char in control map stops search */
        count=0;
        map[0] |= 1;    /* null chars not considered */
        while (!(map[*str >> 3] & (1 << (*str & 7))))
        {
                count++;
                str++;
        }
        return(count);

#else  /* ROUTINE == _STRCSPN */

        /* 1st char in control map stops search */
        while (*str)
        {
                if (map[*str >> 3] & (1 << (*str & 7)))
                        return((char *)str);
                str++;
        }
        return(NULL);

#endif  /* ROUTINE == _STRCSPN */

}

#if defined(STRSPN_USE_SSE2)
#include <emmintrin.h>
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma optimize("t", on)

/* Routine prototype */
#if ROUTINE == _STRSPN
size_t __cdecl strspn (
#elif ROUTINE == _STRCSPN
size_t __cdecl strcspn (
#else  /* ROUTINE == _STRCSPN */
char * __cdecl strpbrk (

#endif  /* ROUTINE == _STRCSPN */
const char * string, const char * control)
{
    unsigned long long shift = (unsigned long long) control & 0xf;

    // Mask off the lower bits of the address to get a 16-byte aligned pointer
    char * alignedControl = (char *) control - shift;

    // Load 16 bytes.  This will not cross a page boundary but will have spurious data
    __m128i search = _mm_loadu_si128((__m128i *) alignedControl);
    __m128i zero = _mm_xor_si128(zero, zero);
    __m128i temp, tempMask, smearedChar;
    unsigned int mask, terminatorSeen = 0;
    unsigned long bitCount;

    // Sse2 provides immediate shifts of the full 128 bit register.
    // Shift out the spurious data on the right and shift in zeros.
    switch (shift)
    {
        case 1:
            search = _mm_srli_si128(search, 1);
            break;
        case 2:
            search = _mm_srli_si128(search, 2);
            break;
        case 3:
            search = _mm_srli_si128(search, 3);
            break;
        case 4:
            search = _mm_srli_si128(search, 4);
            break;
        case 5:
            search = _mm_srli_si128(search, 5);
            break;
        case 6:
            search = _mm_srli_si128(search, 6);
            break;
        case 7:
            search = _mm_srli_si128(search, 7);
            break;
        case 8:
            search = _mm_srli_si128(search, 8);
            break;
        case 9:
            search = _mm_srli_si128(search, 9);
            break;
        case 10:
            search = _mm_srli_si128(search, 10);
            break;
        case 11:
            search = _mm_srli_si128(search, 11);
            break;
        case 12:
            search = _mm_srli_si128(search, 12);
            break;
        case 13:
            search = _mm_srli_si128(search, 13);
            break;
        case 14:
            search = _mm_srli_si128(search, 14);
            break;
        case 15:
            search = _mm_srli_si128(search, 15);
            break;
        case 0:
        default:
            break;
    }

    // Search for zero bytes in this initial string
    temp = _mm_cmpeq_epi8(search, zero);
    mask = _mm_movemask_epi8(temp);

    if (mask)
    {
        // We found zeros and now need to mask away the spurious data that may follow
       (void) _BitScanForward(&bitCount, mask);

        terminatorSeen = (shift == 0) ? 1 : (bitCount < (16 - shift));

        switch (16 - bitCount)
        {
            case 1:
               search = _mm_slli_si128(search, 1);
               search = _mm_srli_si128(search, 1);
               break;
            case 2:
               search = _mm_slli_si128(search, 2);
               search = _mm_srli_si128(search, 2);
               break;
            case 3:
               search = _mm_slli_si128(search, 3);
               search = _mm_srli_si128(search, 3);
               break;
            case 4:
               search = _mm_slli_si128(search, 4);
               search = _mm_srli_si128(search, 4);
               break;
            case 5:
               search = _mm_slli_si128(search, 5);
               search = _mm_srli_si128(search, 5);
               break;
            case 6:
               search = _mm_slli_si128(search, 6);
               search = _mm_srli_si128(search, 6);
               break;
            case 7:
               search = _mm_slli_si128(search, 7);
               search = _mm_srli_si128(search, 7);
               break;
            case 8:
               search = _mm_slli_si128(search, 8);
               search = _mm_srli_si128(search, 8);
               break;
            case 9:
               search = _mm_slli_si128(search, 9);
               search = _mm_srli_si128(search, 9);
               break;
            case 10:
               search = _mm_slli_si128(search, 10);
               search = _mm_srli_si128(search, 10);
               break;
            case 11:
               search = _mm_slli_si128(search, 11);
               search = _mm_srli_si128(search, 11);
               break;
            case 12:
               search = _mm_slli_si128(search, 12);
               search = _mm_srli_si128(search, 12);
               break;
            case 13:
               search = _mm_slli_si128(search, 13);
               search = _mm_srli_si128(search, 13);
               break;
            case 14:
               search = _mm_slli_si128(search, 14);
               search = _mm_srli_si128(search, 14);
               break;
            case 15:
               search = _mm_slli_si128(search, 15);
               search = _mm_srli_si128(search, 15);
               break;
            case 16:
                search = zero;
                break;
       }
    }
    else if (shift == 0)
    {
        // We loaded 16 bytes and found no zero.  Check if the first byte in
        // the next 16-byte aligned block is zero.  This load will not page fault
        // on a correct program since we have not see the terminator.
        if (*(alignedControl + 1) == 0)
        {
            terminatorSeen = 1;
        }
        else
        {
            // We already have 16 bytes and have not seen the terminator.  Fallback to
            // non-Sse2 version.
            return fallbackMethod(string, control);
        }
    }

    // If was have not seen the string terminator, attempt to piece-together a search
    // mask of bytes from those in the next 16-byte aligned group that follows.
    // We will allow at most 16 bytes in the search string.
    if (!terminatorSeen)
    {
        // Go get the next 16 bytes, again this will not page fault.
        alignedControl += 16;
        temp = _mm_loadu_si128((__m128i *) alignedControl);

        tempMask = _mm_cmpeq_epi8(temp, zero);
        terminatorSeen = _mm_movemask_epi8(tempMask);

        if (!terminatorSeen)
        {
            return fallbackMethod(string, control);
        }

        (void) _BitScanForward(&bitCount, terminatorSeen);

        if ((16 - shift + bitCount) > 16)
        {
            return fallbackMethod(string, control);
        }

        // Shift the 2nd part of the search mask into place with the 16 bytes
        switch (16 - bitCount)
        {
           case 1:
              temp = _mm_slli_si128(temp, 1);
              break;
           case 2:
              temp = _mm_slli_si128(temp, 2);
              break;
           case 3:
              temp = _mm_slli_si128(temp, 3);
              break;
           case 4:
              temp = _mm_slli_si128(temp, 4);
              break;
           case 5:
              temp = _mm_slli_si128(temp, 5);
              break;
           case 6:
              temp = _mm_slli_si128(temp, 6);
              break;
           case 7:
              temp = _mm_slli_si128(temp, 7);
              break;
           case 8:
              temp = _mm_slli_si128(temp, 8);
              break;
           case 9:
              temp = _mm_slli_si128(temp, 9);
              break;
           case 10:
              temp = _mm_slli_si128(temp, 10);
              break;
           case 11:
              temp = _mm_slli_si128(temp, 11);
              break;
           case 12:
              temp = _mm_slli_si128(temp, 12);
              break;
           case 13:
              temp = _mm_slli_si128(temp, 13);
              break;
           case 14:
              temp = _mm_slli_si128(temp, 14);
              break;
           case 15:
              temp = _mm_slli_si128(temp, 15);
              break;
           case 16:
              temp = zero;
              break;
      
 }

       // Or the two parts together to obtain a single up-to 16-byte search mask
        search = _mm_or_si128(search, temp);
    }

    // Now loop through the string do a 16-compare with our search mask
#if (ROUTINE == _STRSPN) || (ROUTINE == _STRCSPN)
    size_t count = 0;
#endif
    while (*string)
    {
        int smear = (int) *string;
        // Get the byte in a register
        smearedChar = _mm_cvtsi32_si128(smear);

        // The following 3 instructions smear this one character through each byte of the 16-byte register
        smearedChar = _mm_unpacklo_epi8(smearedChar, smearedChar);
        smearedChar = _mm_unpacklo_epi8(smearedChar, smearedChar);
        smearedChar = _mm_shuffle_epi32(smearedChar, 0);

        // Look for a match
        temp = _mm_cmpeq_epi8(search, smearedChar);
        mask = _mm_movemask_epi8(temp);

#if (ROUTINE == _STRSPN)
        // Break if no match
        if (!mask)
        {
           break;
        }

        string++;
        count++;
#elif (ROUTINE == _STRCSPN)
      // Break on first match
      if (mask)
      {
         break;
      }

      string++;
      count++;
#else // strpbrk case
        // Return current string location on a match
        if (mask)
        {
            return (char *) string;
        }
     
        string++;
#endif
    }

#if (ROUTINE == _STRSPN) || (ROUTINE == _STRCSPN)
    return count;
#else
    return NULL;
#endif
}
#endif
