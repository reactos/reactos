//
//  userctrnm.h
//
//  Offset definition file for extensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the
//  "First Counter" and "First Help" values for the device they belong to,
//  in order to determine the absolute location of the counter and
//  object names and corresponding Explain text in the registry.
//
//  This file is used by the extensible counter DLL code as well as the
//  counter name and Explain text definition file (names.txt) file that is used
//  by LODCTR to load the names into the registry.
//
//  To Do: Add your object and it's counters at the end.
//


#define USEROBJ         0
#define TOTALS          2
#define FREEONES        4
#define WINDOWS         6
#define MENUS           8
#define CURSORS         10
#define SETWINDOWPOS    12
#define HOOKS           14
#define CLIPDATAS       16
#define CALLPROCS       18
#define ACCELTABLES     20
#define DDEACCESS       22
#define DDECONVS        24
#define DDEXACTS        26
#define MONITORS        28
#define KBDLAYOUTS      30
#define KBDFILES        32
#define WINEVENTHOOKS   34
#define TIMERS          36
#define INPUTCONTEXTS   38
#define CSOBJ           40
#define EXENTER         42
#define SHENTER         44
#define EXTIME          46

