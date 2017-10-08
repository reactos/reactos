#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for select(). */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg5 for select(). */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the mktime function.  */
#define HAVE_MKTIME 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* #define HAVE_STRCASECMP 1 */

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

#define HAVE_LONG_LONG 1
#define SCANF_LONG_LONG "%I64d"
#define PRINTF_LONG_LONG "%I64d"
#define PRINTF_LONG_LONG_I64D 1
#define SCANF_LONG_LONG_I64D 1
