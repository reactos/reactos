/*
 * openas.h - MyOpenAsDialog() definitions.
 */

#include "shellp.h"

/* Types
 ********/

/* OPENASINFO flags */

typedef enum openasinfo_flags
{
    /* Allow association registration. */

    OPENASINFO_FL_ALLOW_REGISTRATION   = 0x0001,

    /* Register extension. */

    OPENASINFO_FL_REGISTER_EXT         = 0x0002,

    /* Execute file after registering association. */

    OPENASINFO_FL_EXEC                 = 0x0004,

    /* flag combinations */

    ALL_OPENASINFO_FLAGS               = (OPENASINFO_FL_ALLOW_REGISTRATION |
                                          OPENASINFO_FL_REGISTER_EXT |
                                          OPENASINFO_FL_EXEC)
}
OPENASINFO_FLAGS;

DECLARE_STANDARD_TYPES(OPENASINFO);

/* Prototypes
 *************/

/* fsassoc.c */

/*
 * Success:
 *      S_OK            user requested file type be registered
 *      S_FALSE         user requested file type not be registered (one-shot)
 *
 * Failure:
 *      E_ABORT         user cancelled
 *      E_OUTOFMEMORY   out of memory
 */
extern HRESULT MyOpenAsDialog(HWND hwnd, POPENASINFO poainfo);

/* assoc.c. */

#ifdef DEBUG

extern BOOL IsValidPCOPENASINFO(PCOPENASINFO pcoainfo);

#endif   /* DEBUG */

