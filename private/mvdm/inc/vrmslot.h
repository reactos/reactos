/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrmslot.h

Abstract:

    Prototypes, definitions and structures for VdmRedir mailslot handlers

Author:

    Richard L Firth (rfirth) 16-Sep-1991

Revision History:

    16-Sep-1991 rfirth
        Created

--*/



//
// VDM Mailslot support routines. Prototypes
//

VOID
VrDeleteMailslot(
    VOID
    );

VOID
VrGetMailslotInfo(
    VOID
    );

VOID
VrMakeMailslot(
    VOID
    );

VOID
VrPeekMailslot(
    VOID
    );

VOID
VrReadMailslot(
    VOID
    );

VOID
VrWriteMailslot(
    VOID
    );

VOID
VrTerminateMailslots(
    IN WORD DosPdb
    );



//
// typedefs
//

//
// SELECTOR - in the absence of a standard SELECTOR type, 16-bit selector,
// doubles as SEGMENT (as in ADDRESS16)
//

typedef unsigned short SELECTOR;

//
// ADDRESS16 - an Intel architecture-specific 16:16 address, consisting of a
// 16-bit offset in the low word and a 16-bit segment (real mode) or selector
// (protect mode) in the high word. Both elements are little-endian
// Again, this exists in absence of Intel-specific DWORD structure which has
// correct endian-ness and views address as composed of two parts
//

typedef struct {
    unsigned short  Offset;
    SELECTOR        Selector;
} ADDRESS16;



//
// structures
//

//
// VR_MAILSLOT_INFO - the Dos mailslot subsystem needs some info which we do
// not keep, so we put it in this structure. The structure is linked into a
// list of active mailslot structures for every successful CreateMailslot
// call. The extra info we need is:
//
//      DosPdb          - the PDB (or PSP) of the Dos application. Used for
//                        consistency checks and removing mailslots when the
//                        app dies
//      Handle16        - the handle returned to the Dos app. We have to invent
//                        this
//      BufferAddress   - the Dos app tells us where its buffer is then wants
//                        us to confirm the address in a DosMailslotInfo call
//      Selector        - the Dos app needs a protect mode selector when
//                        running under Windows 3.0 enhanced mode
//      MessageSize     - maximum message size which can be read. Not the same
//                        thing as mailslot size
//
// We also need some information for our own internal wrangling:
//
//      NameLength      - the length of the significant part of the mailslot
//                        name (after \MAILSLOT\). We compare this before
//                        doing a strcmp() on names
//      Name            - the significant part of the mailslot name. When a
//                        mailslot is opened, we store the name after \MAILSLOT\
//                        because DosMailslotWrite uses the symbolic name, even
//                        when writing locally; we need a handle, so we have
//                        to map the name to open mailslot handle.
//
// This structure is allocated from the heap and the Name field will actually
// be large enough to hold the entire string. I put Name[2] because the Mips
// compiler doesn't know about Name[] (Microsoft C compiler extension). 2 at
// least keeps things even. Maybe it should be 4. Maybe it doesn't matter
//

typedef struct _VR_MAILSLOT_INFO *PVR_MAILSLOT_INFO;
typedef struct _VR_MAILSLOT_INFO {
    PVR_MAILSLOT_INFO   Next;       // linked list
    WORD        DosPdb;             // for consistency etc
    WORD        Handle16;           // Dos handle
    HANDLE      Handle32;           // Win32 handle (proper)
    ADDRESS16   BufferAddress;      // Dos app's message buffer
    SELECTOR    Selector;           // Win 3's buffer selector
    DWORD       MessageSize;        // max. message size
    DWORD       NameLength;         // length of name following:
    CHAR        Name[2];            // of mailslot, (after \\.\MAILSLOT\)
} VR_MAILSLOT_INFO;
