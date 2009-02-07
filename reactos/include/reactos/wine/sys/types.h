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
#ifndef __pid_t_defined
#define __pid_t_defined
typedef _pid_t	pid_t;
#endif /* __pid_t_defined */
#endif
#endif	/* Not _PID_T_ */

#endif  /* __WINE_SYS_TYPES_H */
