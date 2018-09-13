// This file holds the x86 specific exception stack info definitions.

//#ifndef KERNEL
#if 1

#if defined(TARGET_i386)

//----------------------------------------------------------------------
// Exception list types
//
// All types described in this section relate to the list of exception
// handlers that is maintained by the operating system.  For more SEH
// information, see "N386 Structured Exception Handling Specification"
// (N386-SEH.DOC).  The C++ EH types are derived from types in the C
// runtime's h\ehdata.h (currently on \\hobie\c32rw\src\crtwin32).
//----------------------------------------------------------------------

typedef long SCOPE;             // exception scope: index into a scopetable
#define scopeNil ((SCOPE)-1)

typedef VOID (FAR *LPFN)(VOID);

// The STE (scopetable element) type is one element in a scopetable; it
// defines one try/except or try/finally block.  The scopeEnclosing field
// is an index to the try/except or try/finally block that encloses this
// one *within the same function*.

typedef struct STE              // scopetable element
{
    SCOPE   scopeEnclosing; // index of the scope that encloses one within
                            //   this same scopetable, or scopeNil if none
    LPFN    lpfnFilter;     // filter function (returns -1, 0, or 1)
    LPFN    lpfnHandler;    // ptr to except block (if lpfnFilter != NULL)
                            //   or finally block (if lpfnFilter == NULL)
} STE;

// The RN (registration node) type is what is pointed to by the except_list in
// the TEB.  In the MSC implementation, there is one registration node for
// each active function which contains any try/except or try/finally blocks.
// The RN is added to the list at function entry, and removed at function exit;
// thus, it will be in the list whether or not there are currently any active
// handlers from this function.

typedef struct RN               // registration node
{
    // fields that are valid for all implementations

    struct RN * prnNext;    // next registration node in chain
    LPFN lpfnLanguageHandler;// points to CRT's _except_handler2()

    // language/implementation specific fields (these are what MSC uses)

    UOFFSET rgste;          // ptr to scopetable (array of STEs)
                            //   (type==ULONG for Win32s ptr arith)
    SCOPE   scopeCur;       // currently active scope for this node
    DWORD   ebp;            // this function's EBP register value

} RN;

// _except_list is an extern of type Absolute, which specifies the offset
// within the TEB of a thread's exception list.  The DWORD at this
// location is actually an "RN *", which points to the first registration
// node on the exception stack.
//
// To obtain its value from a C file, we have to use this little trick of
// pretending it's an integer, and then taking the address of the integer.

extern int FAR _except_list;
#define     except_list   ((UOFFSET)&_except_list)

//----------------------------------------------------------------------

// C++ EH types

typedef struct HT               // Handler Type
{
    DWORD   dwFlags;        // misc. flags
    LPVOID  pType;          // pointer to type descriptor
    DWORD   dwDispCatchObj; // displacement of catch obj from base of current
                            // stack frame
    LPFN    lpfnCatch;      // the interesting part: addr of the catch itself
} HT;

typedef struct TBME             // Try Block Map Entry
{
    DWORD   dwTryLow;       // lowest state index of try
    DWORD   dwTryHigh;      // highest state index of try
    DWORD   dwCatchHigh;    // highest state index of any associated catch
    DWORD   dwCatches;      // number of entries in array
    HT FAR *pht;            // list of handlers for this try
} TBME;

typedef struct FUNCINFO // Function Info, one per function with C++ EH
{
    DWORD           dwMagic;    // magic number, should be 0x19930520
    DWORD           dwMaxState; // highest state number plus one (thus
                                // the number of entries in the unwind map)
    VOID FAR *      pume;       // where the unwind map is
    DWORD           dwTryBlocks;// number of 'try' blocks in this function
    TBME FAR *      ptbme;      // where the handler map is
    // ... more stuff we don't care about ...
} FUNCINFO;

//----------------------------------------------------------------------

#endif // TARGET_i386

#endif // !KERNEL
