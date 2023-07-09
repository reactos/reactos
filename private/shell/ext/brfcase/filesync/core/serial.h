/*
 * serial.h - Access serialization routines description.
 */


/* Types
 ********/

typedef struct _serialcontrol
{
   BOOL (*AttachProcess)(HMODULE);
   BOOL (*DetachProcess)(HMODULE);
   BOOL (*AttachThread)(HMODULE);
   BOOL (*DetachThread)(HMODULE);
}
SERIALCONTROL;
DECLARE_STANDARD_TYPES(SERIALCONTROL);

typedef struct _nonreentrantcriticalsection
{
   CRITICAL_SECTION critsec;

#ifdef DEBUG
   DWORD dwOwnerThread;
#endif   /* DEBUG */

   BOOL bEntered;
}
NONREENTRANTCRITICALSECTION;
DECLARE_STANDARD_TYPES(NONREENTRANTCRITICALSECTION);


/* Prototypes
 *************/

/* serial.c */

#ifdef DEBUG

extern BOOL SetSerialModuleIniSwitches(void);

#endif   /* DEBUG */

extern void ReinitializeNonReentrantCriticalSection(PNONREENTRANTCRITICALSECTION);
extern BOOL EnterNonReentrantCriticalSection(PNONREENTRANTCRITICALSECTION);
extern void LeaveNonReentrantCriticalSection(PNONREENTRANTCRITICALSECTION);

#ifdef DEBUG

extern BOOL NonReentrantCriticalSectionIsOwned(PCNONREENTRANTCRITICALSECTION);

#endif

extern BOOL BeginExclusiveAccess(void);
extern void EndExclusiveAccess(void);

#ifdef DEBUG

extern BOOL AccessIsExclusive(void);

#endif   /* DEBUG */

extern HMODULE GetThisModulesHandle(void);

/* functions to be provided by client */

extern BOOL InitializeDLL(void);
extern BOOL TerminateDLL(void);

#ifdef DEBUG

extern BOOL SetAllIniSwitches(void);

#endif


/* Global Variables
 *******************/

/* serialization control structure */

extern CSERIALCONTROL g_cserctrl;

