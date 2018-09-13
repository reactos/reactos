/*
 * project.h - Project header file for object synchronization engine.
 */


/* System Headers
 *****************/

#define BUILDDLL              /* for windows.h */
#define STRICT                /* for windows.h (robustedness) */
#define _OLE32_               /* for objbase.h - HACKHACK: Remove DECLSPEC_IMPORT from WINOLEAPI. */
#define INC_OLE2              /* for windows.h */
#define CONST_VTABLE          /* for objbase.h */

/*
 * RAIDRAID: (16282) Get rid of warnings about stupid unused Int64 inline
 * functions in winnt.h for all modules.  Emasculate other warnings only for
 * windows.h.
 */

#pragma warning(disable:4514) /* "unreferenced inline function" warning */

#pragma warning(disable:4001) /* "single line comment" warning */
#pragma warning(disable:4115) /* "named type definition in parentheses" warning */
#pragma warning(disable:4201) /* "nameless struct/union" warning */
#pragma warning(disable:4209) /* "benign typedef redefinition" warning */
#pragma warning(disable:4214) /* "bit field types other than int" warning */
#pragma warning(disable:4218) /* "must specify at least a storage class or type" warning */

#include <windows.h>
#pragma warning(disable:4001) /* "single line comment" warning - windows.h enabled it */
#include <shlobj.h>           /* for ShellChangeNotify(), etc. */
#include <shlapip.h>
#include <shlwapi.h>

#pragma warning(default:4218) /* "must specify at least a storage class or type" warning */
#pragma warning(default:4214) /* "bit field types other than int" warning */
#pragma warning(default:4209) /* "benign typedef redefinition" warning */
#pragma warning(default:4201) /* "nameless struct/union" warning */
#pragma warning(default:4115) /* "named type definition in parentheses" warning */
#pragma warning(default:4001) /* "single line comment" warning */

#include <limits.h>
#include <string.h>

#include <linkinfo.h>
#include <reconcil.h>

#define _SYNCENG_             /* for synceng.h */
#include <synceng.h>


/* Project Headers
 ******************/

/* The order of the following include files is significant. */

#include "stock.h"
#include "olestock.h"

#ifdef DEBUG

#include "inifile.h"
#include "resstr.h"

#endif

#include "debug.h"
#include "valid.h"
#include "olevalid.h"
#include "memmgr.h"
#include "ptrarray.h"
#include "list.h"
#include "hndtrans.h"
#include "string2.h"
#include "comc.h"
#include "util.h"
#include "path.h"
#include "fcache.h"
#include "brfcase.h"
#include "storage.h"
#include "clsiface.h"
#include "twin.h"
#include "foldtwin.h"
#include "expandft.h"
#include "twinlist.h"
#include "reclist.h"
#include "copy.h"
#include "merge.h"
#include "recon.h"
#include "db.h"
#include "serial.h"

/* RAIDRAID: (16283) Remove the OLE pig module hack if possible. */

#include "olepig.h"


/* Constants
 ************/

/*
 * constants to be used with #pragma data_seg()
 *
 * These section names must be given the associated attributes in the project's
 * module definition file.
 */

#define DATA_SEG_READ_ONLY       ".text"
#define DATA_SEG_PER_INSTANCE    ".instanc"
#define DATA_SEG_SHARED          ".data"

