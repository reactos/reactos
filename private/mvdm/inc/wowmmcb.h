/* --------------------------------------------------------------------
** Module       : wowmmcb.h
**
** Description  : Interrupt callback stuff for Multi-Media.
**
** History:     : Created 09-Nov-1992 by StephenE
**
** --------------------------------------------------------------------
*/
#ifndef WOWMMCB_H
#define WOWMMCB_H

/*
** This header file will go through both 16 bit and 32 bit compilers.
** In 16 bit land UNALIGNED is not required, on 32 bit land UNALIGNED
** would have been #defined by the time this file was #included because
** this file is always included after wow32.h.  This means the following
** #define is always a no-op with 16 bit compilers and ignored with
** 32 bit compilers.
*/
#ifndef UNALIGNED
#define UNALIGNED
#endif

extern void call_ica_hw_interrupt(int, int, int);

/* --------------------------------------------------------------------
**  Hardware Interrupts:
**
**  IRQ:     Interrupt: ICA: Line: Description:
**  -------------------------------------------------------------------
**  IRQ0     0x08       0    0     Timer Tick every 18.2 times per second.
**  IRQ1     0x09       0    1     Keyboard service required.
**  IRQ2     0x0A       0    2     INT from slave 8259A.
**  IRQ8     0x70       1    0         Real time clock service.
**  IRQ9     0x71       1    1         Software redirected to IRQ2.
**  IRQ10    0x72       1    2         Reserved.
**  IRQ11    0x73       1    3         Reserved.
**  IRQ12    0x74       1    4         Reserved.
**  IRQ13    0x75       1    5         Numeric co-processor
**  IRQ14    0x76       1    6         Fixed disk controller
**  IRQ15    0x77       1    7         Reserved.
**  IRQ3     0x0B       0    3     Com2 service request.
**  IRQ4     0x0C       0    4     Com1 service request.
**  IRQ5     0x0D       0    5     Data request from LPT2:
**  IRQ6     0x0E       0    6     Floppy disk service required.
**  IRQ7     0x0F       0    7     Data request from LPT1:
**
** --------------------------------------------------------------------
*/

#ifdef  NEC_98
#define MULTIMEDIA_LINE         4
#define MULTIMEDIA_ICA          1
#define MULTIMEDIA_INTERRUPT    0x14
#else   // NEC_98
#define MULTIMEDIA_LINE         2
#define MULTIMEDIA_ICA          1
#define MULTIMEDIA_INTERRUPT    0x72
#endif  // NEC_98

#define CALLBACK_ARGS_SIZE      16

typedef struct _CALLBACK_ARGS {   /* cbargs */
    DWORD       dwFlags;        // Flags to identify the type of callback.
    DWORD       dwFunctionAddr; // 16:16 address of the function to be called
    WORD        wHandle;        // The handle or ID of the device
    WORD        wMessage;       // The message to be passed to function
    DWORD       dwInstance;     // User data
    DWORD       dwParam1;       // Device data 1
    DWORD       dwParam2;       // Device data 2
} CALLBACK_ARGS;

typedef struct _CALLBACK_DATA {   /* cbdata */
    WORD            wRecvCount;    // The number of interrupts received
    WORD            wSendCount;    // The number of interrupts sent
    CALLBACK_ARGS   args[CALLBACK_ARGS_SIZE];  // Interrupt arguments
    WORD            wIntsCount;    // The number of interrupts received
} CALLBACK_DATA;

typedef CALLBACK_DATA FAR *VPCALLBACK_DATA;         // 16:16 pointer type
typedef CALLBACK_ARGS FAR *VPCALLBACK_ARGS;         // 16:16 pointer type

typedef CALLBACK_DATA UNALIGNED *PCALLBACK_DATA;    //  0:32 pointer type
typedef CALLBACK_ARGS UNALIGNED *PCALLBACK_ARGS;    //  0:32 pointer type

VOID FAR PASCAL Notify_Callback_Data( VPCALLBACK_DATA vpCallbackData );
#endif
