#ifndef __WINE_SYS_TYPES_H
#define __WINE_SYS_TYPES_H

#include_next <sys/types.h>

#ifndef _PID_T_
#define	_PID_T_
#ifndef _WIN64
typedef int	_pid_t;
#else
typedef __int64	_pid_t;
#endif
#ifndef	NO_OLDNAMES
typedef _pid_t	pid_t;
#endif
#endif	/* Not _PID_T_ */

#endif  /* __WINE_SYS_TYPES_H */
