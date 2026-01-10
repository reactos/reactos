#ifndef _KSANITIZE_H
#define _KSANITIZE_H

#if defined(__GNUC__) && defined(__SANITIZE_UB__)
#define NO_SANITIZE __attribute__ ((no_sanitize("undefined")))
#else
#define NO_SANITIZE
#endif

#endif
