#ifndef __WIN32K_MDEVOBJ_H
#define __WIN32K_MDEVOBJ_H

/* Type definitions ***********************************************************/

typedef struct _PDEVOBJ *PPDEVOBJ;

typedef struct _MDEVOBJ
{
    PPDEVOBJ ppdevGlobal;
} MDEVOBJ, *PMDEVOBJ;

/* Globals ********************************************************************/

extern PMDEVOBJ gpmdev; /* FIXME: should be stored in gpDispInfo->pmdev */

#endif /* !__WIN32K_MDEVOBJ_H */
