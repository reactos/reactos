//==========================================================================;
//
//  compmani.h
//
//  Copyright (c) 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//      Internal COMPMAN header file. Defines some internal
//      data structures and things not needed outside of COMPMAN itself.
//
//  History:
//	07/07/94    frankye
//
//==========================================================================;

#if !defined NUMELMS
  #define NUMELMS(aa)           (sizeof(aa)/sizeof((aa)[0]))
  #define FIELDOFF(type,field)  (&(((type)0)->field))
  #define FIELDSIZ(type,field)  (sizeof(((type)0)->field))
#endif

//--------------------------------------------------------------------------;
//
//  ICMGARB structure
//
//
//	This structure contains information (ICM Garbage) that is global but
//	on a per-process basis.  Currently this is required only for 16-bit
//	builds and the structues are maintained in a linked list whose head
//	is pointed to by the gplig global variable.
//
//--------------------------------------------------------------------------;

typedef struct tICMGARB
{
    struct tICMGARB*pigNext;		// next garb structure
    DWORD           pid;                // process id associated with this garb
    UINT            cUsage;             // usage count for this process

    //
    //	16-to-32 thunk related data
    //
    BOOL	    fThunksInitialized;
    BOOL	    fThunkError;
    DWORD	    dwMsvfw32Handle;
    LPVOID          lpvThunkEntry;

} ICMGARB, *PICMGARB, FAR *LPICMGARB;

extern PICMGARB gplig;

//--------------------------------------------------------------------------;
//
//  misc data structures
//
//--------------------------------------------------------------------------;

//
//
//
typedef struct  {
    DWORD       dwSmag;             // 'Smag'
    HTASK       hTask;              // owner task.
    DWORD       fccType;            // converter type ie 'vidc'
    DWORD       fccHandler;         // converter id ie 'rle '
    HDRVR       hDriver;            // handle of driver
    LPARAM      dwDriver;           // driver id for functions
    DRIVERPROC  DriverProc;         // function to call
    DWORD	dnDevNode;	    // devnode id iff pnp driver
#ifdef NT_THUNK16
    //
    //	h32	: 32-bit driver handle
    //	lpstd	: 16:16 ptr to current status thunk descriptor
    //
    DWORD       h32;
    struct tICSTATUSTHUNKDESC FAR* lpstd;
#endif
}   IC, *PIC;

//
//  This structure is similar in use to the ICINFO structure except
//  that it is only used internally and not passed to apps.
//
//  !!! If you add anything to this structure that requires thunking,
//  then you're going to need to thunk it in thunk.c
//
typedef struct {
    DWORD	dnDevNode;	    // devnode id iff pnp driver
} ICINFOI, NEAR *NPICINFOI, *PICINFOI, FAR *LPICINFOI ;


//--------------------------------------------------------------------------;
//
//  GetCurrentProcessId prototype
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  DWORD GetCurrentProcessId
//
//  Description:
//	This function returns the current process id
//
//  Arguments:
//
//  Return (DWORD):
//	Id of current process
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//
//	WIN32:
//	This function exists in the 32-bit kernels on both Chicago and
//	Daytona and we provide no prototype for WIN32 compiles.
//
//	16-bit Chicago:
//	It is exported as in internal API by the 16-bit Chicago kernel.
//	We provide the prototype here and import it in the def file.
//
//	16-bit Daytona:
//	Has no such 16-bit function and really doesn't need one since all
//	16-bit tasks are part of the same process under Daytona.  Therefore
//	we just #define this to return (1) for 16-bit non-Chicago builds.
//
//--------------------------------------------------------------------------;
#ifndef _WIN32
#ifdef  CHICAGO
DWORD WINAPI GetCurrentProcessId(void);
#else
#define GetCurrentProcessId() (1)
#endif
#endif

//--------------------------------------------------------------------------;
//
//  Thunk initialization and termination function protos
//
//--------------------------------------------------------------------------;
PICMGARB WINAPI thunkInitialize(VOID);
VOID WINAPI thunkTerminate(PICMGARB pid);

//--------------------------------------------------------------------------;
//
//  pig function protos
//
//--------------------------------------------------------------------------;
PICMGARB FAR PASCAL pigNew(void);
PICMGARB FAR PASCAL pigFind(void);
void FAR PASCAL pigDelete(PICMGARB pig);

//--------------------------------------------------------------------------;
//
//  misc function protos
//
//--------------------------------------------------------------------------;
BOOL VFWAPI ICInfoInternal(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo, ICINFOI FAR * lpicinfoi);
