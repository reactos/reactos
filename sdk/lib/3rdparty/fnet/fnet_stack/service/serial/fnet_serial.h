/**************************************************************************
*
* Copyright 2008-2016 by Andrey Butok. FNET Community.
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
*  FNET Serial Input and Output Library API.
*
***************************************************************************/
#ifndef _FNET_SERIAL_H_

#define _FNET_SERIAL_H_

/*! @addtogroup fnet_serial
* The Serial Input/Output (I/O) library provides commonly used I/O functions
* like printf(), putchar() and getchar().@n
* It uses stream abstraction to allow reading and writing data in an uniform way.
* There are three standard streams allocated automatically that are
* associated with physical serial ports (one per UART module). @n
* For the serial library usage example, refer to FNET demo application source code. @n
* @n
* NOTE: The HW serial ports associated with I/O streams are not initialized by the serial
* library and should be initialized by the application. It can be inialized by
* the @ref fnet_cpu_serial_init() function. @n
* @n
* Configuration parameters:
* - @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
*
*/
/*! @{ */

/******************************************************************************
* Escape sequences for termianl that suports vt100.
*******************************************************************************/
#define FNET_ATTR_RESET     0
#define FNET_ATTR_BOLD      1
#define FNET_ATTR_BLINK     5
#define FNET_ATTR_UNDERLINE 4
#define FNET_ATTR_REVERSE   7

#define FNET_COLOR_BLACK    0
#define FNET_COLOR_RED      1
#define FNET_COLOR_GREEN    2
#define FNET_COLOR_YELLOW   3
#define FNET_COLOR_BLUE     4
#define FNET_COLOR_MAGENTA  5
#define FNET_COLOR_CYAN     6
#define	FNET_COLOR_WHITE    7

/* Foreground color.*/
#define FNET_SERIAL_ESC_FG_BLACK    "\33[30m"
#define FNET_SERIAL_ESC_FG_RED      "\33[31m"
#define FNET_SERIAL_ESC_FG_GREEN    "\33[32m"
#define FNET_SERIAL_ESC_FG_YELLOW   "\33[33m"
#define FNET_SERIAL_ESC_FG_BLUE     "\33[34m"
#define FNET_SERIAL_ESC_FG_MAGENTA  "\33[35m"
#define FNET_SERIAL_ESC_FG_CYAN     "\33[36m"
#define	FNET_SERIAL_ESC_FG_WHITE    "\33[37m"

/* Background color.*/
#define FNET_SERIAL_ESC_BG_BLACK    "\33[40m"
#define FNET_SERIAL_ESC_BG_RED      "\33[41m"
#define FNET_SERIAL_ESC_BG_GREEN    "\33[42m"
#define FNET_SERIAL_ESC_BG_YELLOW   "\33[43m"
#define FNET_SERIAL_ESC_BG_BLUE     "\33[44m"
#define FNET_SERIAL_ESC_BG_MAGENTA  "\33[45m"
#define FNET_SERIAL_ESC_BG_CYAN     "\33[46m"
#define	FNET_SERIAL_ESC_BG_WHITE    "\33[47m"

/* SGR -- Select Graphic Rendition.*/
#define FNET_SERIAL_ESC_ATTR_RESET     "\33[0m"
#define FNET_SERIAL_ESC_ATTR_BOLD      "\33[1m"
#define FNET_SERIAL_ESC_ATTR_UNDERLINE "\33[4m"
#define FNET_SERIAL_ESC_ATTR_BLINK     "\33[5m"
#define FNET_SERIAL_ESC_ATTR_REVERSE   "\33[7m"

/**************************************************************************/ /*!
 * @brief Stream control structure.
 *
 * This structure defines stream-specific parameters. All streams have similar
 * properties independently of the individual characteristics of
 * the media they are associated with.
 *
 * @see fnet_serial_stream_t
 ******************************************************************************/
struct fnet_serial_stream
{
    fnet_index_t id;            /**< @brief  The @c id parameter provides a way for a stream
                                 * driver to identify a particular device. @n
                                 * For example it can be used as serial port number
                                 * or pointer to a stream private structure.@n
                                 * This parameter is passed to
                                 * @c fnet_serial_stream.putchar() and to
                                 * @c fnet_serial_stream.getchar() as the first parameter.
                                 */
    void (*putchar)(fnet_index_t stream_id, fnet_char_t character);/**< @brief Callback function used
                                                    * for writing the @c character to the stream.
                                                    */
    fnet_int32_t (*getchar)(fnet_index_t stream_id);   /**< @brief Callback function used for reading
                                        * a character from the stream.
                                        */
    void (*flush)(fnet_index_t stream_id);     /**< @brief Callback function used for
                                        * immediate data sending from internal stream buffer
                                        * to the steam client.@n
                                        * This function is optional and can be set to zero.@n
                                        * The function only has meaning for buffered streams.
                                        * UART stream does not have internal buffer and does
                                        * not use this flush function.
                                        */
};

/**************************************************************************/ /*!
 * @brief Serial stream descriptor.
 ******************************************************************************/
typedef const struct fnet_serial_stream *fnet_serial_stream_t;


extern const struct fnet_serial_stream fnet_serial_stream_port0;
extern const struct fnet_serial_stream fnet_serial_stream_port1;
extern const struct fnet_serial_stream fnet_serial_stream_port2;
extern const struct fnet_serial_stream fnet_serial_stream_port3;
extern const struct fnet_serial_stream fnet_serial_stream_port4;
extern const struct fnet_serial_stream fnet_serial_stream_port5;

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 0.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT0    ((fnet_serial_stream_t)(&fnet_serial_stream_port0))

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 1.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT1    ((fnet_serial_stream_t)(&fnet_serial_stream_port1))

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 2.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT2    ((fnet_serial_stream_t)(&fnet_serial_stream_port2))

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 3.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT3    ((fnet_serial_stream_t)(&fnet_serial_stream_port3))

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 4.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT4    ((fnet_serial_stream_t)(&fnet_serial_stream_port4))

/**************************************************************************/ /*!
 * @brief Stream descriptor associated with the serial port 5.
 * @see FNET_SERIAL_STREAM_DEFAULT
 ******************************************************************************/
#define FNET_SERIAL_STREAM_PORT5    ((fnet_serial_stream_t)(&fnet_serial_stream_port5))


/**************************************************************************/ /*!
 * @def FNET_SERIAL_STREAM_DEFAULT
 * @brief Descriptor of the default stream. It's assigned to the default
 * serial port defined by @ref FNET_CFG_CPU_SERIAL_PORT_DEFAULT
 * (usually the serial port 0).
 ******************************************************************************/
#if (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 0u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT0
#elif (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 1u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT1
#elif (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 2u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT2
#elif (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 3u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT3
#elif (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 4u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT4
#elif (FNET_CFG_CPU_SERIAL_PORT_DEFAULT == 5u)
    #define FNET_SERIAL_STREAM_DEFAULT     FNET_SERIAL_STREAM_PORT5
#else
    #error "The serial library defines only 6 ports."
#endif

#if defined(__cplusplus)
extern "C" {
#endif


/***************************************************************************/ /*!
 *
 * @brief    Write character to the stream.
 *
 * @param stream        Stream descriptor.
 *
 * @param character     Character to be written.
 *
 * @see fnet_serial_getchar()
 *
 ******************************************************************************
 *
 * This function writes a single @c character to the stream.
 *
 ******************************************************************************/
void fnet_serial_putchar(fnet_serial_stream_t stream, fnet_char_t character);

/***************************************************************************/ /*!
 *
 * @brief    Read character from the stream.
 *
 * @param stream          Stream descriptor.
 *
 * @return This function returns:
 *          - character from the stream.
 *          - FNET_ERR if no character is available in the stream.
 *
 * @see fnet_serial_putchar()
 *
 ******************************************************************************
 *
 * This function reads a single character from the stream. @n
 * If no character is available in the stream the FNET_ERR is returned.
 *
 ******************************************************************************/
fnet_int32_t fnet_serial_getchar(fnet_serial_stream_t stream);

/***************************************************************************/ /*!
 *
 * @brief    Send data from the internal stream buffer to the stream client.
 *
 * @param stream          Stream descriptor.
 *
 ******************************************************************************
 *
 * This function immediately sends data from the internal stream
 * buffer to the stream client. @n
 * If you do not explicitly call the flush function, stream sends data to
 * the client after the internal buffer is full.@n
 * The function only has meaning for buffered streams.@n
 * UART stream does not have internal buffer and does not use
 * this flush function.
 *
 ******************************************************************************/
void fnet_serial_flush(fnet_serial_stream_t stream);

/***************************************************************************/ /*!
 *
 * @brief    Write character to the default stream.
 *
 * @param character     Character to be written.
 *
 * @see fnet_getchar()
 *
 ******************************************************************************
 *
 * This function writes a single @c character to the default stream.
 *
 ******************************************************************************/
void fnet_putchar(fnet_char_t character);

/***************************************************************************/ /*!
 *
 * @brief    Read character from the default stream.
 *
 * @return This function returns:
 *          - character from the default stream.
 *          - FNET_ERR if no character is available in the default stream.
 *
 * @see fnet_putchar()
 *
 ******************************************************************************
 *
 * This function reads a single character from the default stream. @n
 * If no character is available in the stream the FNET_ERR is returned.
 *
 ******************************************************************************/
fnet_int32_t fnet_getchar(void);

/***************************************************************************/ /*!
 *
 * @brief    Print formatted text to the stream.
 *
 * @param stream      Stream descriptor.
 *
 * @param format      Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_serial_putchar(), fnet_serial_vprintf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the @c stream. @n
 * The function takes one or more arguments. The first argument is a string parameter
 * called the "format string".
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified placeholders.
 * The number of arguments following the @c format parameters should at
 * least be as much as the number of format placeholders. @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_serial_printf(fnet_serial_stream_t stream, const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 *
 * @brief    Print formatted variable argument list to the stream.
 *
 * @param stream      Stream descriptor.
 *
 * @param format      Format string.
 *
 * @param arg         Variable arguments list. It shall have been initialized by @c va_start() macro
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_serial_printf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the @c stream, expanding the format
 * placeholders with the value of the argument list @c arg. @n
 * This function behaves exactly as @c printf except that the variable argument
 * list is passed as a @c va_list instead of a succession of arguments,
 * which becomes specially useful when the argument list to be passed comes
 * itself from a variable argument list in the calling function. @n
 * @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_serial_vprintf(fnet_serial_stream_t stream, const fnet_char_t *format, va_list arg );

/***************************************************************************/ /*!
 *
 * @brief    Print formatted text to the default stream.
 *
 * @param format      Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_sprintf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the default stream, its descriptor is
 * defined by @ref FNET_SERIAL_STREAM_DEFAULT. @n
 * The function takes one or more arguments. The first argument is a string parameter
 * called the "format string".
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified placeholders.
 * The number of arguments following the @c format parameters should at
 * least be as much as the number of format placeholders. @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n"); @n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_printf( const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 *
 * @brief    Print formatted variable argument list to the default stream.
 *
 * @param format      Format string.
 *
 * @param arg         Variable arguments list. It shall have been initialized by @c va_start() macro
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_printf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the default stream, expanding the format
 * placeholders with the value of the argument list @c arg. @n
 * This function behaves exactly as @c printf except that the variable argument
 * list is passed as a @c va_list instead of a succession of arguments,
 * which becomes specially useful when the argument list to be passed comes
 * itself from a variable argument list in the calling function. @n
 * @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_vprintf(const fnet_char_t *format, va_list arg );

/***************************************************************************/ /*!
 * @brief    Print formatted text to the default stream and terminates the
 *           printed text by the line separator string.

 * @param format      Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_sprintf()
 *
 ******************************************************************************
 *
 * This function outputs formatted text to the default stream and terminates the
 * printed text by the line separator string. Its descriptor is
 * defined by @ref FNET_SERIAL_STREAM_DEFAULT. @n
 * The function takes one or more arguments. The first argument is a string parameter
 * called the "format string".
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified placeholders.
 * The number of arguments following the @c format parameters should at
 * least be as much as the number of format placeholders. @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n"); @n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_println(const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 * @brief    Print formatted text to the buffer.
 *
 * @param str         Pointer to buffer where the resulting string is stored.
 *
 * @param format      Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_snprintf()
 *
 ******************************************************************************
 *
 * The @c format string contains the text to be written to the @c str buffer.
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified placeholders.
 * The number of arguments following the @c format parameters should at
 * least be as much as the number of format placeholders. @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * The @ref fnet_sprintf() function is just like @ref fnet_printf(), except
 * that the output is sent to buffer.@n
 * @n
 * This function does not check the bounds of the buffer and therefore
 * creates the risk of a buffer overflow. It is recommended to use @ref fnet_snprintf()
 * that doesn't suffer from buffer overruns.
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_sprintf( fnet_char_t *str, const fnet_char_t *format, ... );

/***************************************************************************/ /*!
 *
 * @brief    Print formatted text to the buffer.
 *           The length of the buffer is given, that prevents the buffer overflows.
 *
 * @param str         Pointer to buffer where the resulting string is stored.
 *
 * @param size        Maximum number of characters to produce.
 *                    The trailing null character is counted towards this limit
 *
 * @param format      Format string.
 *
 * @return This function returns the number of characters that were
 *         successfully written, excluding the trailing null.
 *
 * @see fnet_sprintf()
 *
 ******************************************************************************
 *
 * This function is just like fnet_sprintf(), except that
 * the length of the buffer is given. This prevents buffer overflows.@n
 * @n
 * The @c format string contains the text to be written to the @c str buffer.
 * The optional arguments following @c format are items
 * (integers, characters or strings) that are to be converted to character strings
 * and inserted into the output of @c format at specified placeholders.
 * The number of arguments following the @c format parameters should at
 * least be as much as the number of format placeholders. @n
 * The syntax for a format placeholder is @a "%[Flags][Width][Length]Type".
 * - @a Flags can be omitted or be any of:
 *    - @c - : Left justify.
 *    - @c + : Right justify.
 *    - @c 0 : Pad with leading zeros.
 *    - @a space  : Print space if no sign.
 * - @a Width is minimum field width. It can be omitted.
 * - @a Length is conversion character. It can be omitted or by any of:
 *    - @c h : Short integer.
 *    - @c l : Long integer.
 * - @a Type can by any of:
 *    - @c d, @c i : Integer.
 *    - @c u : Unsigned.
 *    - @c x, @c X : Hexadecimal.
 *    - @c o : Octal.
 *    - @c b : Binary.
 *    - @c p : Pointer.
 *    - @c c : Single char.
 *    - @c s : Char string.
 *    - @c n : Nothing.
 *
 * @note
 * To save some bytes from all the hard coded strings the fnet_(s)printf() functions will
 * expand all line feeds ("\n") inside the format string to CR LF ("\r\n").
 * So do not use "\r\n" in	the format string - it will be expanded to
 * "\r\r\n". It is save to add it via a parameter though, e.g.
 * fnet_printf("%s", "\r\n");@n
 * This feature can be disable/enabled by the @ref FNET_CFG_SERIAL_PRINTF_N_TO_RN
 * configuration parameter.
 ******************************************************************************/
fnet_size_t fnet_snprintf( fnet_char_t *str, fnet_size_t size, const fnet_char_t *format, ... );

#if defined(__cplusplus)
}
#endif


/*! @} */

#endif
