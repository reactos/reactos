#include <precomp.h>
#include <msvcrt/errno.h>
#include "../../msvcrt/stdlib/doserrmap.h"

#undef errno
int errno;

#undef _doserrno
int _doserrno;

#undef _fpecode
int fpecode;

/*
 * @implemented
 */
int *_errno(void)
{
	return &errno;
}

int __set_errno (int error)
{
	errno = error;
	return error;
}

/*
 * @implemented
 */
int * __fpecode ( void )
{
        return &fpecode;
}

/*
 * @implemented
 */
int*	__doserrno(void)
{
	_doserrno = GetLastError();
	return &_doserrno;
}

/*
 * This function sets both doserrno to the passed in OS error code
 * and also maps this to an appropriate errno code.  The mapping
 * has been deduced automagically by running this functions, which
 * exists in MSVCRT but is undocumented, on all the error codes in
 * winerror.h.
 */
void _dosmaperr(unsigned long oserror)
{
	int pos, base, lim;
	
	SetLastError(oserror);
	
	/* Use binary chop to find the corresponding errno code */
	for (base=0, lim=sizeof(doserrmap)/sizeof(doserrmap[0]); lim; lim >>= 1) {
		pos = base+(lim >> 1);
		if (doserrmap[pos].winerr == oserror) {
			__set_errno(doserrmap[pos].en);
			return;
		} else if (doserrmap[pos].winerr > oserror) {
			base = pos + 1;
			--lim;
		}
	}
	/* EINVAL appears to be the default */
	__set_errno(EINVAL);
}
