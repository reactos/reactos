#ifndef _NTDD8042_
#define _NTDD8042_

#define IOCTL_INTERNAL_I8042_HOOK_KEYBOARD  CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0FF0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_I8042_HOOK_MOUSE     CTL_CODE(FILE_DEVICE_MOUSE, 0x0FF0, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef enum _KEYBOARD_SCAN_STATE {
    Normal,
    GotE0,
    GotE1
} KEYBOARD_SCAN_STATE, *PKEYBOARD_SCAN_STATE;

typedef
NTSTATUS
(*PI8042_SYNCH_READ_PORT) (
    IN PVOID    Context,
    PUCHAR      Value,
    BOOLEAN     WaitForACK
    );

typedef
NTSTATUS
(*PI8042_SYNCH_WRITE_PORT) (
    IN PVOID    Context,
    UCHAR       Value,
    BOOLEAN     WaitForACK
    );

typedef enum _TRANSMIT_STATE {
    Idle = 0,
    SendingBytes
} TRANSMIT_STATE;

typedef struct _OUTPUT_PACKET {
    PUCHAR         Bytes;
    ULONG          CurrentByte;
    ULONG          ByteCount;
    TRANSMIT_STATE State;
} OUTPUT_PACKET, *POUTPUT_PACKET;

typedef
NTSTATUS
(*PI8042_KEYBOARD_INITIALIZATION_ROUTINE) (
    IN PVOID                           InitializationContext,
    IN PVOID                           SynchFuncContext,
    IN PI8042_SYNCH_READ_PORT          ReadPort,
    IN PI8042_SYNCH_WRITE_PORT         WritePort,
    OUT PBOOLEAN                       TurnTranslationOn
    );

typedef
BOOLEAN
(*PI8042_KEYBOARD_ISR) (
    PVOID                   IsrContext,
    PKEYBOARD_INPUT_DATA    CurrentInput,
    POUTPUT_PACKET          CurrentOutput,
    UCHAR                   StatusByte,
    PUCHAR                  Byte,
    PBOOLEAN                ContinueProcessing,
    PKEYBOARD_SCAN_STATE    ScanState
    );

typedef struct _INTERNAL_I8042_HOOK_KEYBOARD {

    //
    // Context variable for all callback routines
    //
    PVOID Context;

    //
    // Routine to call after the mouse is reset
    //
    PI8042_KEYBOARD_INITIALIZATION_ROUTINE InitializationRoutine;

    //
    // Routine to call when a byte is received via the interrupt
    //
    PI8042_KEYBOARD_ISR IsrRoutine;

    //
    // Write function
    //
 //UNIMPLEMENTED   PI8042_ISR_WRITE_PORT IsrWritePort;

    //
    // Queue the current packet (ie the one passed into the isr callback hook)
    // to be reported to the class driver
    //
 //UNIMPLEMENTED  PI8042_QUEUE_PACKET QueueKeyboardPacket;

    //
    // Context for IsrWritePort, QueueKeyboardPacket
    //
 //UNIMPLEMENTED  PVOID CallContext;

} INTERNAL_I8042_HOOK_KEYBOARD, *PINTERNAL_I8042_HOOK_KEYBOARD;

#endif                                  //_NTDD8042_
