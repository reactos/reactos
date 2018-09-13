
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrnetb.h

Abstract:

    Contains prototypes and definitions for Vdm netbios support routines

Author:

    Colin Watson (colinw) 09-Dec-1991

Revision History:

    09-Dec-1991 colinw
        Created

--*/

//
//  Internal version of the ncb layout for mvdm.
//

#include <packon.h>

//
//  Use packing to ensure that the cu union is not forced to word alignment.
//  All elements of this structure are naturally aligned.
//

typedef struct _NCBW {
    UCHAR   ncb_command;            /* command code                   */
    volatile UCHAR   ncb_retcode;   /* return code                    */
    UCHAR   ncb_lsn;                /* local session number           */
    UCHAR   ncb_num;                /* number of our network name     */
    PUCHAR  ncb_buffer;             /* address of message buffer      */
    WORD    ncb_length;             /* size of message buffer         */
    union {
        UCHAR   ncb_callname[NCBNAMSZ];/* blank-padded name of remote */
        struct _CHAIN_SEND {
            WORD ncb_length2;
            PUCHAR ncb_buffer2;
        } ncb_chain;
    } cu;
    UCHAR   ncb_name[NCBNAMSZ];     /* our blank-padded netname       */
    UCHAR   ncb_rto;                /* rcv timeout/retry count        */
    UCHAR   ncb_sto;                /* send timeout/sys timeout       */
    void (*ncb_post)( struct _NCB * ); /* POST routine address        */
    UCHAR   ncb_lana_num;           /* lana (adapter) number          */
    volatile UCHAR   ncb_cmd_cplt;  /* 0xff => commmand pending       */

    // Make driver specific use of the reserved area of the NCB.
    WORD    ncb_reserved;           /* return to natural alignment    */
    union {
        LIST_ENTRY      ncb_next;   /* queued to worker thread        */
        IO_STATUS_BLOCK ncb_iosb;   /* used for Nt I/O interface      */
    } u;

    HANDLE          ncb_event;      /* HANDLE to Win32 event          */

    //  Extra workspace utilized by the mvdm component.

    WORD ncb_es;                    /* 16 bit address of the real NCB */
    WORD ncb_bx;

    PNCB ncb_original_ncb;          /* 32 bit address of the real NCB */
    DWORD ProtectModeNcb;           /* TRUE if NCB originated in PM   */

    } NCBW, *PNCBW;

#include <packoff.h>

VOID
VrNetbios5c(
    VOID
    );


VOID
VrNetbios5cInterrupt(
    VOID
    );

VOID
VrNetbios5cInitialize(
    VOID
    );

BOOLEAN
IsPmNcbAtQueueHead(
    VOID
    );
