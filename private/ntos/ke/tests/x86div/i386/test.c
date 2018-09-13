/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1991		**/ 
/*****************************************************************/ 

#include <stdio.h>
#include <process.h>
#include <setjmp.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

// declare a BSS value - see what the assemble looks like

CONTEXT     RegContext;
ULONG       DefaultValue;
ULONG       TestCount;
ULONG       ExpectedException;

extern  ULONG   DivOperand;
extern  ULONG   DivRegPointer;
extern  LONG    DivRegScaler;
extern  ULONG   ExceptEip;
extern  ULONG   ExceptEsp;
extern  ULONG   TestTable[];
extern  ULONG   TestTableCenter[];
#define TESTTABLESIZE    (128*sizeof(ULONG))

extern  TestDiv();

BOOLEAN vInitialized;
ULONG   vZero = 0;
ULONG   vTwo  = 0;
ULONG   vDivOk = 0x7f7f7f7f;


VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{

    /***
     *  This program tests the kernel's MOD/RM & SIB decoding of
     *  a processor trap 0.  The kernel needs to crack the MOD/RM & SIB
     *  on a div to determine if the exception is a divide_by_zero
     *  or an overflow execption.
     */

    try {
        //
        // Setup for divide by zero test
        //

        DivOperand = 0;
        DivRegScaler = 0;
        DivRegPointer = TestTableCenter;
        DefaultValue = 0x01010101;
        ExpectedException = STATUS_INTEGER_DIVIDE_BY_ZERO;

        printf ("Begin divide by zero test\n");

        for (DivRegScaler = -7; DivRegScaler <  7; DivRegScaler++) {
            vInitialized = FALSE;
            TestDiv ();
        }

        printf ("End divide by zero test\n\n");

        //
        // Setup for divide overflow test
        //

        DivOperand = 2;
        DivRegPointer = TestTableCenter;
        DefaultValue = 0;
        ExpectedException = STATUS_INTEGER_OVERFLOW;

        printf ("Begin divide overflow test\n");

        for (DivRegScaler = -7; DivRegScaler < 7; DivRegScaler++) {
            vInitialized = FALSE;
            TestDiv ();
        }
        printf ("End divide overflow test\n\n");

    } except (HandleException(GetExceptionInformation())) {
        printf ("FAIL: in divide by zero exception handler");
    }

    printf ("%ld varations run ", TestCount);
}

HandleException (
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
    ULONG       i;
    PUCHAR      p;
    PCONTEXT    Context;
    ULONG       def;

    switch (i = ExceptionPointers->ExceptionRecord->ExceptionCode) {
        case 1:
            Context = ExceptionPointers->ContextRecord;
            Context->Eip = ExceptEip;
            Context->Esp = ExceptEsp;

            if (vInitialized) {
                printf ("Divide failed - div instruction completed\n");
                return EXCEPTION_CONTINUE_SEARCH;   // to debugger
            }
            vInitialized = TRUE;
            TestCount--;
            // fall through...

        case STATUS_INTEGER_OVERFLOW:
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            if (i != ExpectedException  &&  i != 1) {
                break;
            }

            TestCount++;

            // set context
            def = DefaultValue;
            Context = ExceptionPointers->ContextRecord;
            Context->Eax = def;
            Context->Ebx = def;
            Context->Ecx = def;
            Context->Edx = def;
            Context->Esi = def;
            Context->Edi = def;
            Context->Ebp = def;

            // find next test
            for (p = (PUCHAR) Context->Eip; ((PULONG) p)[0] != 0xCCCCCCCC; p++) ;
            Context->Eip = (ULONG) (p + 4);

            // clear global testable
            RtlFillMemoryUlong (TestTable, TESTTABLESIZE, def);
            return EXCEPTION_CONTINUE_EXECUTION;
    }

    printf ("\nFailed - unexpected exception code %lx  (expected %lx)\n",
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        ExpectedException
        );

    return EXCEPTION_CONTINUE_SEARCH;
}




DivMarker()
{
    EXCEPTION_RECORD ExceptionRecord;

    //
    // Construct an exception record.
    //

    ExceptionRecord.ExceptionCode    = 1;
    ExceptionRecord.ExceptionRecord  = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags   = 0;
    RtlRaiseException(&ExceptionRecord);
}
