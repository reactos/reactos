/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * 
 */

#include <nt.h>
#include <ntrtl.h>      // which requires all of these header files...
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>

#include <shlobj.h>
#include <shlobjp.h>
#include <shellp.h>
#include <sfview.h>
#include <shsemip.h>
#include <shlwapi.h>
#include <shguidp.h>
#include <ccstock.h>

#include <urlmon.h>
#include <mshtmhst.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <dllload.h>

#include "tlist.h"
#include "dll\thext.h"
#include "resource.h"
#include "thsmgd.h"
#include "dragdrop.h"
#include "storage.h"
#include "imgcache.h"
#include "thextsup.h"
#include "extract.h"
#include "thumbs.h"
#include "tngen\ctngen.h"

#include "html.h"
#include "bmp.h"
#include "lnk.h"
#include "docfile.h"
#include "thumpriv.h"
