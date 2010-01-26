#ifndef _SECURITY_H
#define _SECURITY_H

#include <sspi.h>

#if defined(SECURITY_WIN32) || defined(SECURITY_KERNEL)
#include <secext.h>
#endif

#endif /* _SECURITY_H */
