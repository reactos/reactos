/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_sys_types_h_
#define __dj_include_sys_types_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#include <sys/djtypes.h>
  
typedef int		dev_t;
typedef int		ino_t;
typedef int		mode_t;
typedef int		nlink_t;

__DJ_gid_t
#undef __DJ_gid_t
#define __DJ_gid_t
__DJ_off_t
#undef __DJ_off_t
#define __DJ_off_t
__DJ_pid_t
#undef __DJ_pid_t
#define __DJ_pid_t
//__DJ_size_t
#undef __DJ_size_t
#define __DJ_size_t
__DJ_ssize_t
#undef __DJ_ssize_t
#define __DJ_ssize_t
__DJ_uid_t
#undef __DJ_uid_t
#define __DJ_uid_t

#ifndef _POSIX_SOURCE

/* Allow including program to override.  */
#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif

typedef struct fd_set {
  unsigned char fd_bits [((FD_SETSIZE) + 7) / 8];
} fd_set;

#define FD_SET(n, p)    ((p)->fd_bits[(n) / 8] |= (1 << ((n) & 7)))
#define FD_CLR(n, p)	((p)->fd_bits[(n) / 8] &= ~(1 << ((n) & 7)))
#define FD_ISSET(n, p)	((p)->fd_bits[(n) / 8] & (1 << ((n) & 7)))
#define FD_ZERO(p)	memset ((void *)(p), 0, sizeof (*(p)))

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_sys_types_h_ */
