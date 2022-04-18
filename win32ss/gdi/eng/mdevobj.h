#ifndef __WIN32K_MDEVOBJ_H
#define __WIN32K_MDEVOBJ_H

/* Type definitions ***********************************************************/

typedef struct _PDEVOBJ *PPDEVOBJ;

typedef struct _MDEVDISPLAY
{
    PPDEVOBJ ppdev;
} MDEVDISPLAY, *PMDEVDISPLAY;

typedef struct _MDEVOBJ
{
    ULONG cDev;
    PPDEVOBJ ppdevGlobal;
    MDEVDISPLAY dev[0];
} MDEVOBJ, *PMDEVOBJ;

/* Globals ********************************************************************/

extern PMDEVOBJ gpmdev; /* FIXME: should be stored in gpDispInfo->pmdev */

/* Function prototypes ********************************************************/

VOID
MDEVOBJ_vEnable(
    _Inout_ PMDEVOBJ pmdev);

BOOL
MDEVOBJ_bDisable(
    _Inout_ PMDEVOBJ pmdev);

/* Create a new MDEV:
 * - pustrDeviceName: name of the device to put in MDEV. If NULL, will put all graphics devices in MDEV
 * - pdm: settings associated to pustrDeviceName. Unused if pustrDeviceName is NULL.
 * Return value: the new MDEV (or NULL in case of error)
 */
PMDEVOBJ
MDEVOBJ_Create(
    _In_opt_ PUNICODE_STRING pustrDeviceName,
    _In_opt_ PDEVMODEW pdm);

VOID
MDEVOBJ_vDestroy(
    _Inout_ PMDEVOBJ pmdev);

#endif /* !__WIN32K_MDEVOBJ_H */
