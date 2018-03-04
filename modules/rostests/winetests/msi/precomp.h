
#ifndef _MSI_WINETEST_PRECOMP_H_
#define _MSI_WINETEST_PRECOMP_H_

#define _WIN32_MSI 300

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS

#include <wine/test.h>

#include <winuser.h>
#include <winreg.h>
#include <winnls.h>
#include <winsvc.h>
#include <winver.h>
#include <objbase.h>
#include <msiquery.h>
#include <msidefs.h>
#include <fci.h>
#include <srrestoreptapi.h>
#include <shellapi.h>

#endif /* !_MSI_WINETEST_PRECOMP_H_ */
