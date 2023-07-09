/*
 * project.h - Project header file for LinkInfo DLL.
 */


/* System Headers
 *****************/

#define BUILDDLL              /* for windows.h */
#define STRICT                /* for windows.h (robustedness) */

#ifdef WINNT
#if DBG
#define DEBUG 1
#endif
#endif

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

#pragma warning(default:4218) /* "must specify at least a storage class or type" warning */
#pragma warning(default:4214) /* "bit field types other than int" warning */
#pragma warning(default:4209) /* "benign typedef redefinition" warning */
#pragma warning(default:4201) /* "nameless struct/union" warning */
#pragma warning(default:4115) /* "named type definition in parentheses" warning */
#pragma warning(default:4001) /* "single line comment" warning */

#ifdef WINNT
#define ReinitializeCriticalSection NoThunkReinitializeCriticalSection
VOID WINAPI NoThunkReinitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    );

#define ALIGN_CNT(x,y)    (((x)+(y)-1) & ~((y)-1))
#else
#define ALIGN_CNT(x,y)    (x)
#endif

#define ALIGN_PTR(x,y)      ALIGN_CNT((DWORD)(x),(y))
#define ALIGN_DWORD_CNT(x)  ALIGN_CNT((x),SIZEOF(DWORD))
#define ALIGN_DWORD_PTR(x)  ALIGN_PTR((x),SIZEOF(DWORD))
#define ALIGN_WORD_CNT(x)   ALIGN_CNT((x),SIZEOF(WORD))
#define ALIGN_WORD_PTR(x)   ALIGN_PTR((x),SIZEOF(WORD))

#include <limits.h>

#define _LINKINFO_            /* for linkinfo.h */
#include <linkinfo.h>


/* Constants
 ************/

#ifdef DEBUG

#define INDENT_STRING         "    "

#endif


/* Project Headers
 ******************/

/* The order of the following include files is significant. */

#include "..\core\stock.h"
#include "..\core\serial.h"

#ifdef DEBUG

#include "..\core\inifile.h"
#include "..\core\resstr.h"

#endif

#include "..\core\debug.h"
#include "..\core\valid.h"
#include "..\core\memmgr.h"
#include "..\core\comc.h"

#include "util.h"
#include "canon.h"


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
