
#ifndef _INETCOMM_PRECOMP_H_
#define _INETCOMM_PRECOMP_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
#include <mimeole.h>

#include <wine/list.h>
#include <wine/debug.h>

#include "inetcomm_private.h"

#endif /* !_INETCOMM_PRECOMP_H_ */
