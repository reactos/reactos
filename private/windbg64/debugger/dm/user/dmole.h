// DMOLE.H: defines OLE types

// "objbase.h" has OLE basic types & includes.  This file can be found
// in the "sushi\mfc\include" directory, among other places.

#include "objbase.h"

// If we're an OLE server and we're currently inside an OLE RPC call,
// we keep track of the address inside OLE to which the RPC call will
// return.  We need to have a stack of these to allow for recursive
// OLE calls.
typedef struct _OLERET
{
    struct _OLERET *poleretNext;
    UOFFSET         uoffRet;
    UOFFSET         dwEsp;
} OLERET;

// OLE Rpc debugging
typedef enum _ORPCKEYSTATE {

    orpcKeyNotChecked,      // the DebugObjectRPCEnabled key has not been looked at.
                            //
    orpcKeyEnabled,         // the DebugObjectRPCEnabled key is enabled.
                            //
    orpcKeyFailed,          // We tried to modify the key but failed, so don't try again.
                            //
}   ORPCKEYSTATE;

// If you add an entry to this array, you will likely need to add an entry
// in the olesegname structure in dmole.c. That array has the mapping between
// the section type and the name of the section For ex:- {".orpc", oleorpc}

typedef enum _OLESEG
{
    olenone,  // Not a special OLE segment.
    oleorpc,  // Contains interface proxies and stubs
    olebrk,   // contains the special call made by mfcans32.dll etc
              // to tell us the destination on the server side.
              // arguments to this call are "this" and the "pmf".
    olefwd,   // Similar to olebrk except the argument to this call is
              // the address of the function we need to set the bp at.
} OLESEG;


// Keeps track of the addresses of the special ole sections.

typedef struct _OLERG           // OLE range
{
//  PDLLLOAD_ITEM   pdllitem;   // What DLL this range is in
    UOFFSET         uoffMin;    // start addr of range of OLE code (proxies & stubs)
    UOFFSET         uoffMax;    // end addr of range (points one byte past end)
    OLESEG          segType;    // What kind of special segment is this.
} OLERG;
typedef OLERG *POLERG;

#pragma pack(push, 1)       // {

typedef struct _OLENOT      // OLE notification (no packing!)
{
    BYTE        rgbSig[4];  // signature, "MARB"
    GUID        guid;       // GUID indicating which notification this is
    DWORD       cb;         // size of additional info which follows in memory
} OLENOT;
typedef OLENOT * POLENOT;

#pragma pack(pop)           // }

typedef enum _ORPC
{
    orpcNil,                // no ORPC
    orpcUnrecognized,       // any ORPC with which we aren't familiar
    orpcClientGetBufferSize,
    orpcClientFillBuffer,
    orpcServerNotify,
    orpcServerGetBufferSize,
    orpcServerFillBuffer,
    orpcClientNotify,
} ORPC;


typedef struct _SSMD {
    BOOL        fStepped;       // Return value - set if a sstep happened
    BOOL        fStepOver;      // Step over function calls?
    BOOL        fCheckSrc;
    BOOL        fOnCall;
    BOOL        fRecurse;
    BOOL        fSteppedOle;    // Stepped over an OLE RPC call
    BOOL        fGo;
    BOOL        fBpAtCall;      // We've set a bp at addrCall
    UOFFSET     dwESP;
    HLLE        hbptReplace;
    ADDR        addrAfter;      // Address immediately after call instruction
    ADDR        addrCall;       // Address of call instruction
#ifdef RETURNCODES
    ADDR        addrCalled;     // Target address of the call
#endif
//  RN FAR *    prnTopOfStack;  // Registration Node at top of exception stack
    DWORD       cThunk;         // >0 means we're stepping through a thunk;
                                // value indicates max number of times to step
    UOFFSET     uoffThunkDest;  // Destination address of thunk
    UOFFSET     uoffWndproc;    // Address of destination wndproc
} SSMD; // Single Step Method
typedef SSMD FAR *LPSSMD;

// Are we in the .orpc section
__inline
BOOL
FAddrInOle(HPRCX pprc, UOFFSET uoffset)
{
    return (OleSegFromAddr(pprc,uoffset) == oleorpc);
}

