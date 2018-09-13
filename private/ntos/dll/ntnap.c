/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntnap.c

Abstract:

    This module contains the routines for initializing and performing
    profiling of NT API's.

Author:

    Russ Blake (russbl) 22-Apr-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#include "ntnapdef.h"
#include "ntnap.h"

//
// Initialization control: only want to initialize once; this
// prevents multiple initializations before data segment and
// global control are initialized.
//

BOOLEAN NapInitialized = FALSE;

//
// Counter frequency constant
//

LARGE_INTEGER NapFrequency;

//
// Profiling data structures
//

PNAPCONTROL NapControl = NULL;

//
// The following array is indexed by Service Number + 1.  Service
// Number -1 (element 0 of the following array) is reserved for
// calibration.  Initialization of this to NULL kicks off the
// NapDllInit sequence to create or open the data section.
//

PNAPDATA NapProfilingData = NULL;


VOID
NapDllInit (
    VOID
    )

/*++

Routine Description:

    NapDllInit() is called before calling any profiling api.  It
    initializes profiling for the NT native API's.

    NOTE:  Initialization occurs only once and is controlled by
	   the NapInitDone global flag.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG i;
    LARGE_INTEGER CurrentCounter;

    if (!NapInitialized) {

	//
	// Instance data for this process's ntdll.dll not yet initialized.
	//

	NapInitialized = TRUE;

	//
	// Get Counter frequency (current counter value is thrown away)
	// This call is not profiled because NapControl == NULL here.
	//

	NtQueryPerformanceCounter(&CurrentCounter,
				  &NapFrequency);

	//
	// Allocate or open data section for api profiling data.
	//

	if (NT_SUCCESS(NapCreateDataSection(&NapControl))) {

	    NapProfilingData = (PNAPDATA)(NapControl + 1);

	    if (!NapControl->Initialized) {

		//
		// We are the first process to get here: we jsut
		// allocated the section, so must initialize it.
		//

		NapControl->Initialized = TRUE;

		NapControl->Paused = 0;

		//
		// Clear the data area used for calibration data.
		//

		NapProfilingData[0].NapLock = 0L;
		NapProfilingData[0].Calls = 0L;
		NapProfilingData[0].TimingErrors = 0L;
		NapProfilingData[0].TotalTime.HighPart = 0L;
		NapProfilingData[0].TotalTime.LowPart = 0L;
		NapProfilingData[0].FirstTime.HighPart = 0L;
		NapProfilingData[0].FirstTime.LowPart = 0L;
		NapProfilingData[0].MaxTime.HighPart = 0L;
		NapProfilingData[0].MaxTime.LowPart = 0L;
		NapProfilingData[0].MinTime.HighPart = MAX_LONG;
		NapProfilingData[0].MinTime.LowPart = MAX_ULONG;

		//
		// Clear the rest of the data collection area
		//

		NapClearData();

		//
		//   Calibrate time overhead from profiling.  We care mostly
		//   about the minimum time here, to avoid extra time from
		//   interrupts etc.
		//

		for (i=0; i<NUM_ITERATIONS; i++) {
		    NapCalibrate();
		}
	    }
	}
    }
}




NTSTATUS
NapCreateDataSection(
    PNAPCONTROL *NapSectionPointer
    )
/*++

Routine Description:

    NapCreateData() is called to create the profiling data section.
    One section, named "\NapDataSection", is used for the whole system.
    It is accessible by anyone, so don't mess it up.

Arguments:

    NapSectionPointer - a pointer to a pointer which will be set to
			to point to the beginning of the section.

Return Value:

    Status

--*/


{
    NTSTATUS Status;
    HANDLE NapSectionHandle;
    HANDLE MyProcess;
    STRING NapSectionName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LARGE_INTEGER AllocationSize;
    ULONG ViewSize;
    PNAPCONTROL SectionPointer;

    //
    // Initialize object attributes for the dump file
    //

    NapSectionName.Length = 15;
    NapSectionName.MaximumLength = 15;
    NapSectionName.Buffer = "\\NapDataSection";

    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &NapSectionName;
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;
    ObjectAttributes.Attributes = OBJ_OPENIF |
				  OBJ_CASE_INSENSITIVE;


    AllocationSize.HighPart = 0;
    AllocationSize.LowPart = (NAP_API_COUNT + 1) * sizeof(NAPDATA);

    Status = NtCreateSection(&NapSectionHandle,
			     SECTION_MAP_READ |
			     SECTION_MAP_WRITE,
			     &ObjectAttributes,
			     &AllocationSize,
			     PAGE_READWRITE,
			     SEC_COMMIT,
			     NULL);

    if (NT_SUCCESS(Status)) {

	MyProcess = NtCurrentProcess();

	ViewSize = AllocationSize.LowPart;

	SectionPointer = NULL;

	Status = NtMapViewOfSection(NapSectionHandle,
				    MyProcess,
				    (PVOID *)&SectionPointer,
				    0,
				    AllocationSize.LowPart,
				    NULL,
				    &ViewSize,
				    ViewUnmap,
				    MEM_TOP_DOWN,
				    PAGE_READWRITE);

	*NapSectionPointer = SectionPointer;
    }

    return(Status);
}




VOID
NapRecordInfo (
    IN ULONG ServiceNumber,
    IN LARGE_INTEGER Counters[]
    )

/*++

Routine Description:

    NapRecordInfo() is called after the API being measured has
    returned, and the value of the performance counter has been
    obtained.

    NOTE:  Initialization occurs only once and is controlled by
	   the NapInitDone global flag.

Arguments:

    ServiceNumber - the number of the service being measured.
		    -1 is reserved for calibration.

    Counters	  - a pointer to the value of the counter after the call
		    to the measured routine.  Followed immediately
		    in memory by the value of the counter before the
		    call to the measured routine.

Return Value:

    None

--*/


{
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Difference;


    //
    // First make sure we have been initialized
    //

    if (NapControl != NULL) {

	//
	// Check to be sure profiling is not paused

	if (NapControl->Paused == 0) {

	    //
	    // Since ServiceNumber -1 is used for calibration, bump same
	    // so indexes start at 0.
	    //

	    ServiceNumber++;

	    //
	    // Prevent others from changing data at the same time
	    // we are.
	    //

	    NapAcquireSpinLock(&(NapProfilingData[ServiceNumber].NapLock));

	    //
	    // Record API info
	    //

	    NapProfilingData[ServiceNumber].Calls++;

	    //
	    // Elapsed time is end time minus start time
	    //

	    ElapsedTime =
		RtlLargeIntegerSubtract(Counters[0],Counters[1]);

	    //
	    // Now add elapsed time to total time
	    //

	    NapProfilingData[ServiceNumber].TotalTime =
		RtlLargeIntegerAdd(NapProfilingData[ServiceNumber].TotalTime,
				   ElapsedTime);

	    if (NapProfilingData[ServiceNumber].Calls == 1) {
		NapProfilingData[ServiceNumber].FirstTime = ElapsedTime;
	    }
	    else {
		Difference =
		    RtlLargeIntegerSubtract(
			NapProfilingData[ServiceNumber].MaxTime,
			ElapsedTime);
		if (Difference.HighPart < 0) {

		    //
		    // A new maximum was attained
		    //

		    NapProfilingData[ServiceNumber].MaxTime = ElapsedTime;

		}
		Difference =
		    RtlLargeIntegerSubtract(
			ElapsedTime,
			NapProfilingData[ServiceNumber].MinTime);
		if (Difference.HighPart < 0) {

		    //
		    // A new minimum was attained
		    //

		    NapProfilingData[ServiceNumber].MinTime = ElapsedTime;
		}
	    }

	    NapReleaseSpinLock(&(NapProfilingData[ServiceNumber].NapLock));
	}
    }
}



NTSTATUS
NapClearData (
    VOID
    )

/*++

Routine Description:

    NapClearData() is called to clear all the counters and timers.
    The calibration counters are not cleared.  For the rest,
    if not previously collected, the data will be lost.

Arguments:

    None

Return Value:

    Status

--*/

{
    ULONG ServiceNumber;

    //
    // Initialize the first one after the calibration data
    //

    NapAcquireSpinLock(&(NapProfilingData[1].NapLock));

    NapProfilingData[1].Calls = 0L;
    NapProfilingData[1].TimingErrors = 0L;
    NapProfilingData[1].TotalTime.HighPart = 0L;
    NapProfilingData[1].TotalTime.LowPart = 0L;
    NapProfilingData[1].FirstTime.HighPart = 0L;
    NapProfilingData[1].FirstTime.LowPart = 0L;
    NapProfilingData[1].MaxTime.HighPart = 0L;
    NapProfilingData[1].MaxTime.LowPart = 0L;
    NapProfilingData[1].MinTime.HighPart = MAX_LONG;
    NapProfilingData[1].MinTime.LowPart = MAX_ULONG;

    //
    // Now use the initialized first one to initialize the rest
    //


    for (ServiceNumber = 2; ServiceNumber <= NAP_API_COUNT; ServiceNumber++) {

	NapAcquireSpinLock(&(NapProfilingData[ServiceNumber].NapLock));

	NapProfilingData[ServiceNumber] = NapProfilingData[1];

	NapReleaseSpinLock(&(NapProfilingData[ServiceNumber].NapLock));
    }

    NapReleaseSpinLock(&(NapProfilingData[1].NapLock));

    return(STATUS_SUCCESS);
}



NTSTATUS
NapRetrieveData (
    OUT NAPDATA *NapApiData,
    OUT PCHAR **NapApiNames,
    OUT PLARGE_INTEGER *NapCounterFrequency
    )

/*++

Routine Description:

    NapRetrieveData() is called to get all the relevant data
    about API's that have been profiled.  It is wise to call
    NapPause() before calling this, and NapResume() afterwards.
    Can't just do these in here, 'cause you may be doing stuff outside
    of this call you don't want profiled.

Arguments:

    NapApiData - a pointer to a buffer to which is copied
		 the array of counters; must be large enough for
		 NAP_API_COUNT+1; call NapGetApiCount to obtain needed
		 size.

    NapApiNames - a pointer to which is copied a pointer to
		  an array of pointers to API names

    NapCounterFrequency - a buffer to which is copied the frequency
			  of the performance counter

Return Value:

    Status

--*/

{
    ULONG ServiceNumber;

    *NapApiNames = NapNames;

    *NapCounterFrequency = &NapFrequency;

    for (ServiceNumber = 0; ServiceNumber <= NAP_API_COUNT; ServiceNumber++) {

	NapAcquireSpinLock(&(NapProfilingData[ServiceNumber].NapLock));

	NapApiData[ServiceNumber] = NapProfilingData[ServiceNumber];

	NapReleaseSpinLock(&(NapProfilingData[ServiceNumber].NapLock));
	NapReleaseSpinLock(&(NapApiData[ServiceNumber].NapLock));
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
NapGetApiCount (
    OUT PULONG NapApiCount
    )

/*++

Routine Description:

    NapGetApiCount() is called to get the number of API's which are
    being profiled.  This can then be used to allocate an array for
    receiving the data from NapReturnData.

Arguments:

    NapApiCount - A pointer throug which the count is returned.

Return Value:

    Status

--*/


{

    *NapApiCount = NAP_API_COUNT;

    return(STATUS_SUCCESS);

}




NTSTATUS
NapPause (
    VOID
    )

/*++

Routine Description:

    NapPause() is called to temporarily cease data collection.
    Data collection is resumed by a companion call to NapResume.

Arguments:

    None

Return Value:

    Status

--*/


{
    NapControl->Paused++;

    return(STATUS_SUCCESS);
}


NTSTATUS
NapResume (
    VOID
    )

/*++

Routine Description:

    NapResume() is called to resume data collection after a companion
    call to NapPause.

Arguments:

    None

Return Value:

    Status

--*/


{
    NapControl->Paused--;

    return(STATUS_SUCCESS);
}
