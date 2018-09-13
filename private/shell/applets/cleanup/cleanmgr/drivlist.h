#ifndef DRIVLIST_H
#define DRIVLIST_H

#ifndef DISKUTIL_H
	#include "diskutil.h"
#endif

/*
 * DEFINITIONS ________________________________________________________________
 *
 */

typedef enum	// Per-window extra bytes for DriveList
{
	DL_COMBOWND = 0,	// Far pointer to a ComboBox HWND.
	DL_COMBOPROC = 4,	// Far pointer to original comboproc
	DL_UPDATES = 8	// ==0 if paints are OK
} DriveWindLongs;


#define szDriveListCLASS  "DRIVELISTCLASS"

#define DLN_SELCHANGE   (WM_USER +110)	// Sends WP=drive letter chosen

#define DL_UPDATESBAD   (WM_USER +111)	// Don't refresh until later...
#define DL_UPDATESOKAY  (WM_USER +112)	// ...later's here.

#define STYLE_LISTBOX   0x000080000


/*
 * PROTOTYPES _________________________________________________________________
 *
 */

BOOL   RegisterDriveList   (HANDLE hInst);
void   ExitDriveList       (void);

#endif

