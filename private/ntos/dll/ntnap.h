/*++
Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntnap.h

Abstract:

    This module contains the data structure decalrations for
    profiling of NT API's.

Author:

    Russ Blake (russbl) 22-Apr-1991

Revision History:

--*/

//
// Initialization of minimum times
//

#define MAX_LONG  0x7FFFFFFFL
#define MAX_ULONG 0xFFFFFFFFL

//
// Timer calibration
//

#define NUM_ITERATIONS		      2000  // Number of iterations
#define MIN_ACCEPTABLEOVERHEAD		10  // Minimum overhead allowed
#define NAP_CALIBRATION_SERVICE_NUMBER	-1L // Calibration Routine Number

typedef struct _NAPCONTROL {

    //
    // The following assures the area will be initialized only once.
    // Note: Assumes the section is given to us zero filled.
    //

    BOOLEAN Initialized;

    //
    // Profiling control.  If NapPaused > 0, NapRecordInfo will
    // collect no data.  Used to keep initialization and dumping
    // of data itself from being profiled.
    //

    BOOLEAN Paused;

} NAPCONTROL, *PNAPCONTROL;



extern PCHAR NapNames[];

//
// Called internally
//

VOID	 NapDllInit		(VOID);
VOID	 NapRecordInfo		(IN ULONG, IN LARGE_INTEGER[]);
NTSTATUS NapCreateDataSection	(PNAPCONTROL *);

//
// Called by us
//

extern VOID NapCalibrate	(VOID);
extern VOID NapSpinOnSpinLock  (ULONG *);
extern VOID NapReleaseSpinLock	(ULONG *);
