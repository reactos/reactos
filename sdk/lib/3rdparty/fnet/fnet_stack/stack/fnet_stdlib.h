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
* Standard functions API.
*
***************************************************************************/

#ifndef _FNET_STDLIB_H_

#define _FNET_STDLIB_H_

/*! @addtogroup fnet_stdlib
* The standard library defines general purpose functions, including string converting, searching and other data manipulations.
*/

/*! @{ */

#include <stdarg.h> /* For va_list etc.*/
#include <string.h> /* In case of alternative functions */

/**************************************************************************/ /*!
 * @def FNET_NULL
 * @brief NULL pointer.
 * @showinitializer
 ******************************************************************************/
#define FNET_NULL       ((void *)0)

/**************************************************************************/ /*!
 * @def FNET_RAND_MAX
 * @brief The maximum number returned by @ref fnet_rand().
 * @showinitializer
 ******************************************************************************/
#define FNET_RAND_MAX (32767u)

/**************************************************************************/ /*!
 * @brief Unsigned integer type representing the size in bytes.
 ******************************************************************************/
typedef unsigned long fnet_size_t;

/**************************************************************************/ /*!
 * @brief Signed integer type representing the size in bytes.
 ******************************************************************************/
typedef long fnet_ssize_t;

/**************************************************************************/ /*!
 * @brief Unsigned integer type representing the bit flag.
 ******************************************************************************/
typedef unsigned int fnet_flag_t;

/**************************************************************************/ /*!
 * @brief Unsigned integer type representing the index.
 ******************************************************************************/
typedef unsigned int fnet_index_t;

/**************************************************************************/ /*!
 * @brief Type representing the charecter.
 ******************************************************************************/
typedef char fnet_char_t;

/**************************************************************************/ /*!
 * @brief Boolean type.
 ******************************************************************************/
typedef enum
{
    FNET_FALSE = 0, /**< FALSE Boolean value.*/
    FNET_TRUE  = 1  /**< TRUE Boolean value.*/
} fnet_bool_t;

/**************************************************************************/ /*!
 * @brief General return codes, used by most of API functions.
 ******************************************************************************/
typedef enum
{
    FNET_OK  = (0), /**< No error.*/
    FNET_ERR = (-1) /**< There is error.*/
} fnet_return_t;


#define FNET_ALIGN_DIV(x, div)     ((fnet_uint32_t)((volatile fnet_uint8_t *)(x) + ((div)-1U)) & (~((div)-1U)))

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Copies bytes in memory.
 *
 * @param to_ptr      Pointer to the memory location to copy to.
 *
 * @param from_ptr       Pointer to the memory location to copy from.
 *
 * @param number_of_bytes         Number of bytes to copy.
 *
 * @see FNET_CFG_OVERLOAD_MEMCPY
 ******************************************************************************
 *
 * This function copies @c number_of_bytes bytes from
 * memory area pointed by @c from_ptr to memory area pointed by @c to_ptr.@n
 * You may overload it by own implementation, using @ref FNET_CFG_OVERLOAD_MEMCPY.
 *
 ******************************************************************************/
#if !FNET_CFG_OVERLOAD_MEMCPY
void fnet_memcpy(FNET_COMP_PACKED_VAR void *to_ptr, FNET_COMP_PACKED_VAR const void *from_ptr, fnet_size_t number_of_bytes);
#endif

/***************************************************************************/ /*!
 *
 * @brief    Relocates function in memory.
 *
 * @param to_buf_ptr      Pointer to the memory location to copy to.
 *
 * @param from_func_ptr       Pointer to the function to be relocated
 *
 * @param to_buf_size         Number of bytes to copy.
 *
 * @return          This function returns new pointer to the relocated function.
 ******************************************************************************
 *
 * This function copies @c to_buf_size bytes from
 * memory area pointed by @c from_func_ptr to memory area pointed by @c to_buf_ptr.@n
 * It is used for a function relocation from Flash to RAM.
 *
 ******************************************************************************/
void *fnet_memcpy_func(void *to_buf_ptr, const void *from_func_ptr, fnet_size_t to_buf_size);

/***************************************************************************/ /*!
 *
 * @brief           Sets bytes in memory.
 *
 * @param dest      Pointer to the buffer to be set.
 *
 * @param value     Value to be set. @n
 *                  The value is passed as an @c int, but the function converts
 *                  it to @c fnet_uint8_t.
 *
 * @param n         Number of bytes to be set.
 *
 * @see fnet_memset_zero()
 *
 ******************************************************************************
 *
 * This function sets the first @c n bytes of the memory area pointed to by
 * @c dest with the constant byte @c value.
 *
 ******************************************************************************/
void fnet_memset(void *dest, fnet_uint8_t value, fnet_size_t n);

/***************************************************************************/ /*!
 *
 * @brief           Sets bytes in memory to zeros.
 *
 * @param dest      Pointer to the buffer to be set.
 *
 * @param n         Number of bytes to be set.
 *
 * @see fnet_memset()
 *
 ******************************************************************************
 *
 * This function sets the first @c n bytes of the memory area pointed to by
 * @c dest with zeros.
 *
 ******************************************************************************/
void fnet_memset_zero(void *dest, fnet_size_t n );

/***************************************************************************/ /*!
 *
 * @brief           Compares memory areas.
 *
 * @param src1      Pointer to the memory buffer to compare.
 *
 * @param src2      Pointer to the memory buffer to compare.
 *
 * @param count     Number of bytes to compare.
 *
 * @return          - Zero if two buffers are identical.
 *                  - Difference between the first differing bytes.
 *
 ******************************************************************************
 *
 * This function compares the first @c n bytes of the memory buffer pointed
 * by @c src1 to the first @c count bytes pointed by @c src2, returning zero if
 * they all match, difference between the first differing bytes.
 *
 ******************************************************************************/
fnet_int32_t fnet_memcmp(const void *src1, const void *src2, fnet_size_t count );

/***************************************************************************/ /*!
 *
 * @brief           Calculates the length of a string.
 *
 * @param str       Pointer to the null-terminated string to be examined.
 *
 * @return          This function returns the number of characters in @c str.
 *
 ******************************************************************************
 *
 * This function computes the number of bytes in the string to which @c str
 * points, not including the terminating null character.
 *
 ******************************************************************************/
fnet_size_t fnet_strlen (const fnet_char_t *str);

/***************************************************************************/ /*!
 *
 * @brief           Calculate the length of a fixed-size string.
 *
 * @param str       Pointer to the null-terminated string to be examined.
 * @param max_len   Maximum number of characters to examine.
 *
 * @return          This function returns the number of characters in the
 *                  @c str string or max_len if the null character was not found.
 *
 ******************************************************************************
 *
 * This function computes the number of bytes in the string to which @c str
 * points, not including the terminating null character, but at most max_len.
 * The function never examines more than @c max_len bytes of the string.
 *
 ******************************************************************************/
fnet_size_t fnet_strnlen (const fnet_char_t *str, fnet_size_t max_len);

/***************************************************************************/ /*!
 *
 * @brief           Concatenates two strings.
 *
 * @param dest      Pointer to the null-terminated string to append to.
 *
 * @param src       Pointer to the null-terminated string to copy to
 *                  the end of @c dest.
 *
 ******************************************************************************
 *
 * This function appends a copy of the string pointed to by @c src to the end
 * of the string pointed to by  @c dest. @n
 * The resulting string is null-terminated.
 *
 ******************************************************************************/
void fnet_strcat (fnet_char_t *dest, const fnet_char_t *src);

/***************************************************************************/ /*!
 *
 * @brief           Concatenates a string with part of another.
 *
 * @param dest      Pointer to the null-terminated string to append to.
 *
 * @param src       Pointer to the null-terminated string to copy to
 *                  the end of @c dest.
 *
 * @param n         Maximum number of characters to be appended.
 *
 ******************************************************************************
 *
 * This function appends a copy of the string pointed to by @c src to the end
 * of the string pointed to by  @c dest. If the length of the @c src string
 * is less than @c n, only the content up to the terminating null-character
 * is copied. @n
 * The resulting string is null-terminated.
 *
 ******************************************************************************/
void fnet_strlcat (fnet_char_t *dest, const fnet_char_t *src, fnet_size_t n);

/***************************************************************************/ /*!
 *
 * @brief           Copies a string.
 *
 * @param dest      Pointer to the destination buffer where the content is
 *                  to be copied.
 *
 * @param src       Pointer to the null-terminated string to be copied.
 *
 ******************************************************************************
 *
 * This function copies the string pointed by @c src into the buffer pointed
 * by @c dest, including the terminating null character.
 *
 ******************************************************************************/
void fnet_strcpy (fnet_char_t *dest, const fnet_char_t *src);

/***************************************************************************/ /*!
 *
 * @brief           Copies part of a string.
 *
 * @param dest      Pointer to the destination buffer where the content is
 *                  to be copied.
 *
 * @param src       Pointer to the null-terminated string to be copied.
 *
 * @param n         Maximum number of characters to be copied.
 *
 ******************************************************************************
 *
 * This function copies the first @c n characters of the string pointed
 * by @c src into the buffer pointed by @c dest.@n
 * The result string is null-terminated.
 *
 ******************************************************************************/
void fnet_strlcpy( fnet_char_t *dest, const fnet_char_t *src, fnet_size_t n );

/***************************************************************************/ /*!
 *
 * @brief           Locates last occurrence of a character in a string.
 *
 * @param str       Pointer to the null-terminated string to be analyzed.
 *
 * @param chr       Character to search for.
 *
 * @return          This function returns the pointer to the found character
 *                  in @c str, or @ref FNET_NULL if no such character is found.
 *
 ******************************************************************************
 *
 * This function finds the last occurrence of the character @n chr
 * in the string pointed to by @c str..
 *
 ******************************************************************************/
fnet_char_t *fnet_strrchr( const fnet_char_t *str, fnet_char_t chr );

/***************************************************************************/ /*!
 *
 * @brief           Locates first occurrence of a character in a string.
 *
 * @param str       Pointer to the null-terminated string to be analyzed.
 *
 * @param chr       Character to search for.
 *
 * @return          This function returns the pointer to the found character
 *                  in @c str, or @ref FNET_NULL if no such character is found.
 *
 ******************************************************************************
 *
 * This function finds the first occurrence of the character @n chr
 * in the string pointed to by @c str.
 *
 ******************************************************************************/
fnet_char_t *fnet_strchr(const fnet_char_t *str, fnet_char_t chr );

/***************************************************************************/ /*!
 *
 * @brief           Locates a substring.
 *
 * @param str       Pointer to the null-terminated string to be analyzed.
 *
 * @param substr    Pointer to the null-terminated string to search for.
 *
 * @return          This function returns pointer to the first occurrence
 *                  in @c str of the entire sequence of characters
 *                  specified in @c substr.
 *
 ******************************************************************************
 *
 * This function finds the first occurrence of the substring @c substr
 * (excluding the terminating null byte) in the string @c str.
 *
 ******************************************************************************/
fnet_char_t *fnet_strstr( const fnet_char_t *str, const fnet_char_t *substr );

/***************************************************************************/ /*!
 *
 * @brief           Compares two strings.
 *
 * @param str1      Pointer to the null-terminated string to be compared.
 *
 * @param str2      Pointer to the null-terminated string to be compared.
 *
 * @return          This function returns zero if two strings are identical,
 *                  otherwise returns the difference between the first
 *                  two differing characters.
 *
 ******************************************************************************
 *
 * This function compares the two strings @c str1 and @c str2, returning zero if
 * they all match or the difference between the first two differing characters.
 *
 ******************************************************************************/
fnet_int32_t fnet_strcmp( const fnet_char_t *str1, const fnet_char_t *str2 );

/***************************************************************************/ /*!
 *
 * @brief           Compares part of two strings.
 *
 * @param str1      Pointer to the null-terminated string to be compared.
 *
 * @param str2      Pointer to the null-terminated string to be compared.
 *
 * @param n         Maximum number of characters to compare.
 *
 * @return          This function returns zero if @c n characters of
 *                  two strings are identical,
 *                  otherwise returns the difference between the first
 *                  two differing characters.
 *
 ******************************************************************************
 *
 * This function compares not more than @c n characters (characters that
 * follow a null byte are not compared) from the two strings @c str1 and
 * @c str2, returning zero if they all match or the difference between
 * the first two differing characters.
 *
 ******************************************************************************/
fnet_int32_t fnet_strncmp( const fnet_char_t *str1, const fnet_char_t *str2, fnet_size_t n );

/***************************************************************************/ /*!
 *
 * @brief           Compares two strings ignoring case.
 *
 * @param str1      Pointer to the null-terminated string to be compared.
 *
 * @param str2      Pointer to the null-terminated string to be compared.
 *
 * @return          This function returns an integer less than, equal to,
 *                  or greater than zero if @c str1 is found,
 *                  respectively, to be less than, to match, or be greater
 *                  than @c str2.
 *
 ******************************************************************************
 *
 * This function compares the two strings @c str1
 * and @c str2, ignoring the case of the characters. It returns an
 * integer less than, equal to, or greater than zero if @c str1 is found,
 * respectively, to be less than, to match, or be greater than @c str2.
 *
 ******************************************************************************/
fnet_int32_t fnet_strcasecmp( const fnet_char_t *str1, const fnet_char_t *str2 );

/***************************************************************************/ /*!
 *
 * @brief           Compares two strings, with additional terminator.
 *
 * @param in_str    Pointer to the null or @c splitter terminated string to be compared.
 *
 * @param name      Pointer to the null-terminated string to be compared.
 *
 * @param splitter  Additional terminator that can be used in @c str.
 *
 * @return          This function returns zero if two strings are identical,
 *                  otherwise returns the difference between the first
 *                  two differing characters.
 *
 ******************************************************************************
 *
 * This function compares the two strings @c in_str and @c name, returning zero if
 * they all match or the difference between the first two differing characters.@n
 * The @c name string can be terminated by null or @c splitter character.
 *
 ******************************************************************************/
fnet_int32_t fnet_strcmp_splitter( const fnet_char_t *in_str, const fnet_char_t *name, fnet_char_t splitter);

/***************************************************************************/ /*!
 *
 * @brief           Converts string to unsigned long integer.
 *
 * @param str       Pointer to the null-terminated string to be interpreted .
 *
 * @param ptr       Pointer to a pointer to character that will be set to
 *                  the character immediately following the unsigned long
 *                  integer in the string.  If no integer can be formed,
 *                  it points to the first wrong character.
 *
 * @param base      Base of the interpreted integer value. If it equals to 0,
 *                  it will be set to the default value 10.
 *
 * @return          This function returns integer value corresponding to the
 *                  contents of @c str on success. If no conversion can be
 *                  performed, @c 0 is returned.
 *
 ******************************************************************************
 *
 * This function converts a string to an unsigned long integer.@n
 * The string to be converted can contain the digits '0' to '9'.
 * Depending on the base, the string can also contain letters representing
 * digits greater than 9.
 *
 ******************************************************************************/
fnet_uint32_t fnet_strtoul (const fnet_char_t *str, fnet_char_t **ptr, fnet_size_t base);

/***************************************************************************/ /*!
 *
 * @brief    Breaks a string into a sequence of tokens.
 *
 * @param str           The string from which to extract tokens (null-terminated).
 *
 * @param delimiter     The string that contains a set of delimiter
 *                      characters (null-terminated).
 *
 * @param last          Return parameter used by @ref fnet_strtok_r() to record
 *                      its progress through @c str. @n
 *                      This pointer, must be the same while parsing the
 *                      same string.
 *
 * @return       This function returns a pointer to the next token in @c str.
 *               If there are no remaining tokens, it returns a @ref FNET_NULL
 *               pointer.
 *
 ******************************************************************************
 *
 * This function parses the string  @c str  into  tokens. @n
 * The  first function call should have @c str as its first argument. Subse-
 * quent calls should have the first  argument  set  to  @ref FNET_NULL.  Each  call
 * returns  a  pointer  to the next token, or @ref FNET_NULL when no more tokens are
 * found. @n
 * The function also updates the @c last parameter with the starting address
 * of the token following the first occurrence of the @c delimiter parameter. @n
 * The @c delimiter string may be different for each call.
 *
 ******************************************************************************/
fnet_char_t *fnet_strtok_r(fnet_char_t *str, const fnet_char_t *delimiter, fnet_char_t **last);

/***************************************************************************/ /*!
 *
 * @brief           Converts letter to lower case.
 *
 * @param to_lower   Letter to be converted to lower case, if possible.
 *
 * @return          This function returns converted letter.
 *
 ******************************************************************************
 *
 *  This function converts an uppercase letter to the corresponding
 * lowercase letter.
 *
 ******************************************************************************/
fnet_char_t fnet_tolower( fnet_char_t to_lower );

/***************************************************************************/ /*!
 *
 * @brief           Generates a pseudo-random number
 *
 * @return          Pseudo-random number in the range betwenn 0 to @ref FNET_RAND_MAX
 *
 ******************************************************************************
 *
 *  This function generates a pseudo-random number in the range betwenn 0 to @ref FNET_RAND_MAX
 *
 ******************************************************************************/
fnet_uint32_t fnet_rand( void );

/***************************************************************************/ /*!
 *
 * @brief           Initializes the pseudo-random number generator
 *
 * @param seed      Seed for the pseudo-random number generator
 *
 ******************************************************************************
 *
 *  This function initializes the pseudo-random generator with the seed argument
 *
 ******************************************************************************/
void fnet_srand( fnet_uint32_t seed );

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_STDLIB_H_ */
