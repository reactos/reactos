/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exboosts.h

Abstract:

    This file contains all of the Priority boots numbers used by the NT
    executive.

Author:

    Steve Wood (stevewo) 03-Jun-1989

Revision History:

--*/

// begin_ntddk begin_wdm begin_ntifs
//
// Priority increment definitions.  The comment for each definition gives
// the names of the system services that use the definition when satisfying
// a wait.
//

//
// Priority increment used when satisfying a wait on an executive event
// (NtPulseEvent and NtSetEvent)
//

#define EVENT_INCREMENT                 1

// end_ntddk end_wdm end_ntifs
//
// Priority increment used when satisfying a wait on an executive event pair
//

#define EVENT_PAIR_INCREMENT            1

//
// Priority increment used when satisfying a wait on a semaphore used for
// LPC communication.
//

#define LPC_RELEASE_WAIT_INCREMENT      1

// begin_ntddk begin_wdm begin_ntifs
//
// Priority increment when no I/O has been done.  This is used by device
// and file system drivers when completing an IRP (IoCompleteRequest).
//

#define IO_NO_INCREMENT                 0

//
// Priority increment for completing CD-ROM I/O.  This is used by CD-ROM device
// and file system drivers when completing an IRP (IoCompleteRequest)
//

#define IO_CD_ROM_INCREMENT             1

//
// Priority increment for completing disk I/O.  This is used by disk device
// and file system drivers when completing an IRP (IoCompleteRequest)
//

#define IO_DISK_INCREMENT               1

// end_ntifs
//
// Priority increment for completing keyboard I/O.  This is used by keyboard
// device drivers when completing an IRP (IoCompleteRequest)
//

#define IO_KEYBOARD_INCREMENT           6

// begin_ntifs
//
// Priority increment for completing mailslot I/O.  This is used by the mail-
// slot file system driver when completing an IRP (IoCompleteRequest).
//

#define IO_MAILSLOT_INCREMENT           2

// end_ntifs
//
// Priority increment for completing mouse I/O.  This is used by mouse device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_MOUSE_INCREMENT              6

// begin_ntifs
//
// Priority increment for completing named pipe I/O.  This is used by the
// named pipe file system driver when completing an IRP (IoCompleteRequest).
//

#define IO_NAMED_PIPE_INCREMENT         2

//
// Priority increment for completing network I/O.  This is used by network
// device and network file system drivers when completing an IRP
// (IoCompleteRequest).
//

#define IO_NETWORK_INCREMENT            2

// end_ntifs
//
// Priority increment for completing parallel I/O.  This is used by parallel
// device drivers when completing an IRP (IoCompleteRequest)
//

#define IO_PARALLEL_INCREMENT           1

//
// Priority increment for completing serial I/O.  This is used by serial device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_SERIAL_INCREMENT             2

//
// Priority increment for completing sound I/O.  This is used by sound device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_SOUND_INCREMENT              8

//
// Priority increment for completing video I/O.  This is used by video device
// drivers when completing an IRP (IoCompleteRequest)
//

#define IO_VIDEO_INCREMENT              1

// end_ntddk end_wdm
//
// Priority increment used when satisfying a wait on an executive mutant
// (NtReleaseMutant)
//

#define MUTANT_INCREMENT                1

// begin_ntddk begin_wdm begin_ntifs
//
// Priority increment used when satisfying a wait on an executive semaphore
// (NtReleaseSemaphore)
//

#define SEMAPHORE_INCREMENT             1

// end_ntddk end_wdm end_ntifs
//
// Priority increment used when queuing an APC for an executive timer.
//

#define TIMER_APC_INCREMENT             0

//
// Priority increment used to get slow exclusive eresource holders
// moving again.
//

#define ERESOURCE_INCREMENT             4

