/* ldns/config.h.  Generated from config.h.in by configure.  */
/* ldns/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Whether the C compiler accepts the "format" attribute */
#define HAVE_ATTR_FORMAT 1

/* Whether the C compiler accepts the "unused" attribute */
#define HAVE_ATTR_UNUSED 1

/* Define to 1 if you have the `b32_ntop' function. */
/* #undef HAVE_B32_NTOP */

/* Define to 1 if you have the `b32_pton' function. */
/* #undef HAVE_B32_PTON */

/* Define to 1 if you have the `b64_ntop' function. */
/* #undef HAVE_B64_NTOP */

/* Define to 1 if you have the `b64_pton' function. */
/* #undef HAVE_B64_PTON */

/* Define to 1 if you have the `bzero' function. */
/* #undef HAVE_BZERO */

/* Define to 1 if you have the `calloc' function. */
#define HAVE_CALLOC 1

/* Define to 1 if you have the `CONF_modules_unload' function. */
/* #undef HAVE_CONF_MODULES_UNLOAD */

/* Define to 1 if you have the `CRYPTO_cleanup_all_ex_data' function. */
/* #undef HAVE_CRYPTO_CLEANUP_ALL_EX_DATA */

/* Define to 1 if you have the `CRYPTO_memcmp' function. */
/* #undef HAVE_CRYPTO_MEMCMP */

/* Define to 1 if you have the `ctime_r' function. */
/* #undef HAVE_CTIME_R */

/* Is a CAFILE given at configure time */
#define HAVE_DANE_CA_FILE 0

/* Is a CAPATH given at configure time */
#define HAVE_DANE_CA_PATH 0

/* Define to 1 if you have the declaration of `EVP_PKEY_base_id', and to 0 if
   you don't. */
#define HAVE_DECL_EVP_PKEY_BASE_ID 1

/* Define to 1 if you have the declaration of `NID_ED25519', and to 0 if you
   don't. */
/* #undef HAVE_DECL_NID_ED25519 */

/* Define to 1 if you have the declaration of `NID_ED448', and to 0 if you
   don't. */
/* #undef HAVE_DECL_NID_ED448 */

/* Define to 1 if you have the declaration of `NID_secp384r1', and to 0 if you
   don't. */
/* #undef HAVE_DECL_NID_SECP384R1 */

/* Define to 1 if you have the declaration of `NID_X9_62_prime256v1', and to 0
   if you don't. */
/* #undef HAVE_DECL_NID_X9_62_PRIME256V1 */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `DSA_get0_key' function. */
/* #undef HAVE_DSA_GET0_KEY */

/* Define to 1 if you have the `DSA_get0_pqg' function. */
/* #undef HAVE_DSA_GET0_PQG */

/* Define to 1 if you have the `DSA_SIG_get0' function. */
/* #undef HAVE_DSA_SIG_GET0 */

/* Define to 1 if you have the `DSA_SIG_set0' function. */
/* #undef HAVE_DSA_SIG_SET0 */

/* Define to 1 if you have the `ECDSA_SIG_get0' function. */
/* #undef HAVE_ECDSA_SIG_GET0 */

/* Define to 1 if you have the `endprotoent' function. */
/* #undef HAVE_ENDPROTOENT */

/* Define to 1 if you have the `endservent' function. */
/* #undef HAVE_ENDSERVENT */

/* Define to 1 if you have the `ENGINE_cleanup' function. */
/* #undef HAVE_ENGINE_CLEANUP */

/* Define to 1 if you have the `ENGINE_free' function. */
/* #undef HAVE_ENGINE_FREE */

/* Define to 1 if you have the `ERR_free_strings' function. */
/* #undef HAVE_ERR_FREE_STRINGS */

/* Define to 1 if you have the `ERR_load_crypto_strings' function. */
/* #undef HAVE_ERR_LOAD_CRYPTO_STRINGS */

/* Define to 1 if you have the `EVP_cleanup' function. */
/* #undef HAVE_EVP_CLEANUP */

/* Define to 1 if you have the `EVP_dss1' function. */
/* #undef HAVE_EVP_DSS1 */

/* Define to 1 if you have the `EVP_MD_CTX_new' function. */
/* #undef HAVE_EVP_MD_CTX_NEW */

/* Define to 1 if you have the EVP_PKEY_base_id function or macro. */
#define HAVE_EVP_PKEY_BASE_ID 1

/* Define to 1 if you have the `EVP_PKEY_get_base_id' function. */
/* #undef HAVE_EVP_PKEY_GET_BASE_ID */

/* Define to 1 if you have the `EVP_PKEY_keygen' function. */
/* #undef HAVE_EVP_PKEY_KEYGEN */

/* Define to 1 if you have the `EVP_sha256' function. */
/* #undef HAVE_EVP_SHA256 */

/* Define to 1 if you have the `EVP_sha384' function. */
/* #undef HAVE_EVP_SHA384 */

/* Define to 1 if you have the `EVP_sha512' function. */
/* #undef HAVE_EVP_SHA512 */

/* Define to 1 if you have the `fcntl' function. */
/* #undef HAVE_FCNTL */

/* Define to 1 if you have the `fork' function. */
/* #undef HAVE_FORK */

/* if fork is available for compile */
#define HAVE_FORK_AVAILABLE 1

/* Whether getaddrinfo is available */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `gmtime_r' function. */
/* #undef HAVE_GMTIME_R */

/* Define to 1 if you have the `inet_aton' function. */
/* #undef HAVE_INET_ATON */

/* Define to 1 if you have the `inet_ntop' function. */
/* #undef HAVE_INET_NTOP */

/* Define to 1 if you have the `inet_pton' function. */
/* #undef HAVE_INET_PTON */

/* define if you have inttypes.h */
#define HAVE_INTTYPES_H 1

/* if the function 'ioctlsocket' is available */
#define HAVE_IOCTLSOCKET 1

/* Define to 1 if you have the `isascii' function. */
#define HAVE_ISASCII 1

/* Define to 1 if you have the `isblank' function. */
#define HAVE_ISBLANK 1

/* Define to 1 if you have the `pcap' library (-lpcap). */
/* #undef HAVE_LIBPCAP */

/* Define if we have LibreSSL */
/* #undef HAVE_LIBRESSL */

/* Define to 1 if you have the `localtime_r' function. */
/* #undef HAVE_LOCALTIME_R */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
/* #undef HAVE_NETINET_IF_ETHER_H */

/* Define to 1 if you have the <netinet/igmp.h> header file. */
/* #undef HAVE_NETINET_IGMP_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
/* #undef HAVE_NETINET_IN_SYSTM_H */

/* Define to 1 if you have the <netinet/ip6.h> header file. */
/* #undef HAVE_NETINET_IP6_H */

/* Define to 1 if you have the <netinet/ip_compat.h> header file. */
/* #undef HAVE_NETINET_IP_COMPAT_H */

/* Define to 1 if you have the <netinet/ip.h> header file. */
/* #undef HAVE_NETINET_IP_H */

/* Define to 1 if you have the <netinet/udp.h> header file. */
/* #undef HAVE_NETINET_UDP_H */

/* Define to 1 if you have the <net/ethernet.h> header file. */
/* #undef HAVE_NET_ETHERNET_H */

/* Define to 1 if you have the <net/if.h> header file. */
/* #undef HAVE_NET_IF_H */

/* Define to 1 if you have the <openssl/conf.h> header file. */
#define HAVE_OPENSSL_CONF_H 1

/* Define to 1 if you have the <openssl/engine.h> header file. */
#define HAVE_OPENSSL_ENGINE_H 1

/* Define to 1 if you have the <openssl/err.h> header file. */
#define HAVE_OPENSSL_ERR_H 1

/* Define to 1 if you have the <openssl/evp.h> header file. */
#define HAVE_OPENSSL_EVP_H 1

/* Define to 1 if you have the `OPENSSL_init_crypto' function. */
/* #undef HAVE_OPENSSL_INIT_CRYPTO */

/* Define to 1 if you have the `OPENSSL_init_ssl' function. */
/* #undef HAVE_OPENSSL_INIT_SSL */

/* Define to 1 if you have the <openssl/rand.h> header file. */
#define HAVE_OPENSSL_RAND_H 1

/* Define to 1 if you have the <openssl/ssl.h> header file. */
#define HAVE_OPENSSL_SSL_H 1

/* Define to 1 if you have the <pcap.h> header file. */
/* #undef HAVE_PCAP_H */

/* This platform supports poll(7). */
/* #undef HAVE_POLL */

/* If available, contains the Python version number currently in use. */
/* #undef HAVE_PYTHON */

/* Define to 1 if you have the `random' function. */
/* #undef HAVE_RANDOM */

/* Define to 1 if you have the `sleep' function. */
#define HAVE_SLEEP 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define if you have the SSL libraries installed. */
/* #undef HAVE_SSL */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcpy' function. */
/* #undef HAVE_STRLCPY */

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define if you have SWIG libraries and header files. */
/* #undef HAVE_SWIG */

/* Define to 1 if you have the <sys/mount.h> header file. */
/* #undef HAVE_SYS_MOUNT_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* define if you have sys/socket.h */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* define if you have sys/types.h */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <TargetConditionals.h> header file. */
/* #undef HAVE_TARGETCONDITIONALS_H */

/* Define to 1 if you have the `timegm' function. */
/* #undef HAVE_TIMEGM */

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* define if you have unistd.h */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
/* #undef HAVE_VFORK */

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#define HAVE_WINSOCK2_H 1

/* Define to 1 if `fork' works. */
/* #undef HAVE_WORKING_FORK */

/* Define to 1 if `vfork' works. */
/* #undef HAVE_WORKING_VFORK */

/* Define to 1 if you have the <ws2tcpip.h> header file. */
#define HAVE_WS2TCPIP_H 1

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Is a CAFILE given at configure time */
/* #undef LDNS_DANE_CA_FILE */

/* Is a CAPATH given at configure time */
/* #undef LDNS_DANE_CA_PATH */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "libdns@nlnetlabs.nl"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ldns"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "ldns 1.8.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libdns"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.8.3"

/* Define this to enable RR type AMTRELAY. */
/* #undef RRTYPE_AMTRELAY */

/* Define this to enable RR type AVC. */
/* #undef RRTYPE_AVC */

/* Define this to enable RR type DOA. */
/* #undef RRTYPE_DOA */

/* Define this to enable RR type NINFO. */
/* #undef RRTYPE_NINFO */

/* Define this to enable RR type OPENPGPKEY. */
#define RRTYPE_OPENPGPKEY /**/

/* Define this to enable RR type RKEY. */
/* #undef RRTYPE_RKEY */

/* Define this to enable RR types SVCB and HTTPS. */
#define RRTYPE_SVCB_HTTPS /**/

/* Define this to enable RR type TA. */
/* #undef RRTYPE_TA */

/* The size of `time_t', as computed by sizeof. */
#define SIZEOF_TIME_T 4

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Define this to enable messages to stderr. */
/* #undef STDERR_MSGS */

/* System configuration dir */
#define SYSCONFDIR sysconfdir

/* Define this to enable DANE support. */
/* #undef USE_DANE */

/* Define this to enable DANE-TA usage type support. */
/* #undef USE_DANE_TA_USAGE */

/* Define this to enable DANE verify support. */
/* #undef USE_DANE_VERIFY */

/* Define this to enable DSA support. */
/* #undef USE_DSA */

/* Define this to enable ECDSA support. */
/* #undef USE_ECDSA */

/* Define this to enable ED25519 support. */
/* #undef USE_ED25519 */

/* Define this to enable ED448 support. */
/* #undef USE_ED448 */

/* Define this to enable GOST support. */
/* #undef USE_GOST */

/* Define this to enable SHA256 and SHA512 support. */
/* #undef USE_SHA2 */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# define _DARWIN_C_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
# define _HPUX_ALT_XOPEN_SOCKET_API 1
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
# define _NETBSD_SOURCE 1
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
# define _OPENBSD_SOURCE 1
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# define __STDC_WANT_IEC_60559_ATTRIBS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# define __STDC_WANT_IEC_60559_DFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# define __STDC_WANT_IEC_60559_FUNCS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# define __STDC_WANT_LIB_EXT2__ 1
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# define __STDC_WANT_MATH_SPEC_FUNCS__ 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* Whether the windows socket API is used */
#define USE_WINSOCK 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Enable for compile on Minix */
#define _NETBSD_SOURCE 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* in_addr_t */
#define in_addr_t uint32_t

/* in_port_t */
#define in_port_t uint16_t

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `short' if <sys/types.h> does not define. */
/* #undef int16_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef int32_t */

/* Define to `long long' if <sys/types.h> does not define. */
/* #undef int64_t */

/* Define to `char' if <sys/types.h> does not define. */
/* #undef int8_t */

/* Define to `size_t' if <sys/types.h> does not define. */
/* #undef intptr_t */

/* Define as a signed integer type capable of holding a process identifier. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to 'int' if not defined */
/* #undef socklen_t */

/* Fallback member name for socket family in struct sockaddr_storage */
/* #undef ss_family */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ptrdiff_t */

/* Define to `unsigned short' if <sys/types.h> does not define. */
/* #undef uint16_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef uint32_t */

/* Define to `unsigned long long' if <sys/types.h> does not define. */
/* #undef uint64_t */

/* Define to `unsigned char' if <sys/types.h> does not define. */
/* #undef uint8_t */

/* Define as `fork' if `vfork' does not work. */
#define vfork fork


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#ifndef BYTE_ORDER
#ifdef WORDS_BIGENDIAN
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* WORDS_BIGENDIAN */
#endif /* BYTE_ORDER */

#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif


/* detect if we need to cast to unsigned int for FD_SET to avoid warnings */
#ifdef HAVE_WINSOCK2_H
#define FD_SET_T (u_int)
#else
#define FD_SET_T 
#endif




#ifdef __cplusplus
extern "C" {
#endif

int ldns_b64_ntop(uint8_t const *src, size_t srclength,
	 	  char *target, size_t targsize);
/**
 * calculates the size needed to store the result of b64_ntop
 */
/*@unused@*/
static inline size_t ldns_b64_ntop_calculate_size(size_t srcsize)
{
	return ((((srcsize + 2) / 3) * 4) + 1);
}
int ldns_b64_pton(char const *src, uint8_t *target, size_t targsize);
/**
 * calculates the size needed to store the result of ldns_b64_pton
 */
/*@unused@*/
static inline size_t ldns_b64_pton_calculate_size(size_t srcsize)
{
	return (((((srcsize + 3) / 4) * 3)) + 1);
}

/**
 * Given in dnssec_zone.c, also used in dnssec_sign.c:w

 */
int ldns_dname_compare_v(const void *a, const void *b);

#ifndef HAVE_SLEEP
/* use windows sleep, in millisecs, instead */
#define sleep(x) Sleep((x)*1000)
#endif

#ifndef HAVE_RANDOM
#define srandom(x) srand(x)
#define random(x) rand(x)
#endif

#ifndef HAVE_TIMEGM
#include <time.h>
time_t timegm (struct tm *tm);
#endif /* !TIMEGM */
#ifndef HAVE_GMTIME_R
struct tm *gmtime_r(const time_t *timep, struct tm *result);
#endif
//#ifndef HAVE_LOCALTIME_R
//struct tm *localtime_r(const time_t *timep, struct tm *result);
//#endif
#ifndef HAVE_ISBLANK
int isblank(int c);
#endif /* !HAVE_ISBLANK */
#ifndef HAVE_ISASCII
int isascii(int c);
#endif /* !HAVE_ISASCII */
#ifndef HAVE_SNPRINTF
#include <stdarg.h>
int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#endif /* HAVE_SNPRINTF */
#ifndef HAVE_INET_PTON
int inet_pton(int af, const char* src, void* dst);
#endif /* HAVE_INET_PTON */
#ifndef HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, size_t size);
#endif
#ifndef HAVE_INET_ATON
int inet_aton(const char *cp, struct in_addr *addr);
#endif
#ifndef HAVE_MEMMOVE
void *memmove(void *dest, const void *src, size_t n);
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifdef USE_WINSOCK
#define SOCK_INVALID INVALID_SOCKET
#define close_socket(_s) do { if (_s != SOCK_INVALID) {closesocket(_s); _s = -1;} } while(0)
#else
#define SOCK_INVALID -1
#define close_socket(_s) do { if (_s != SOCK_INVALID) {close(_s); _s = -1;} } while(0)
#endif

#ifdef __cplusplus
}
#endif
#ifndef HAVE_GETADDRINFO
#include "compat/fake-rfc2553.h"
#endif
#ifndef HAVE_STRTOUL
#define strtoul (unsigned long)strtol
#endif

