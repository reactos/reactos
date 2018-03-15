
#ifndef __WINE_MSI_PRECOMP__
#define __WINE_MSI_PRECOMP__

#include <wine/config.h>

#include <assert.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "msipriv.h"
#include "query.h"

#include <winreg.h>
#include <wincon.h>
#include <msiserver.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sddl.h>

#include <wine/unicode.h>

#include "resource.h"

#endif /* !__WINE_MSI_PRECOMP__ */
