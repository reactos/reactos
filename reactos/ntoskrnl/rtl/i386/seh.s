/* $Id: seh.s,v 1.1 2002/10/26 00:38:01 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Runtime library exception support for IA-32
 * FILE:              ntoskrnl/rtl/i386/seh.s
 * PROGRAMER:         Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:             This file is shared with lib/msvcrt/except/seh.s.
 *                    Please keep them in sync.
 */

#define ExceptionContinueExecution	0
#define ExceptionContinueSearch		1
#define ExceptionNestedException	2
#define ExceptionCollidedUnwind		3

#define EXCEPTION_NONCONTINUABLE	0x01
#define EXCEPTION_UNWINDING			0x02
#define EXCEPTION_EXIT_UNWIND		0x04
#define EXCEPTION_STACK_INVALID		0x08
#define EXCEPTION_NESTED_CALL		0x10
#define EXCEPTION_TARGET_UNWIND		0x20
#define EXCEPTION_COLLIDED_UNWIND	0x40

#define EXCEPTION_UNWIND_MODE \
(  EXCEPTION_UNWINDING \
 | EXCEPTION_EXIT_UNWIND \
 | EXCEPTION_TARGET_UNWIND \
 | EXCEPTION_COLLIDED_UNWIND)

#define EREC_CODE		0x00
#define EREC_FLAGS		0x04
#define EREC_RECORD		0x08
#define EREC_ADDRESS	0x0C
#define EREC_NUMPARAMS	0x10
#define EREC_INFO		0x14

#define TRYLEVEL_NONE    -1
#define TRYLEVEL_INVALID -2

#define ER_STANDARDESP	-0x08
#define ER_EPOINTERS	-0x04
#define ER_PREVFRAME	0x00
#define ER_HANDLER		0x04
#define ER_SCOPETABLE	0x08
#define ER_TRYLEVEL		0x0C
#define ER_EBP			0x10

#define ST_TRYLEVEL		0x00
#define ST_FILTER		0x04
#define ST_HANDLER		0x08

#define CONTEXT_EDI		0x9C
#define CONTEXT_EBX		0xA4
#define CONTEXT_EIP		0xB8

.globl __local_unwind2
.globl __except_handler3

// EAX = value to print
_do_debug:
	pushal
	pushl	%eax
	call	_MsvcrtDebug@4
	popal
	ret

#define LU2_TRYLEVEL	0x08
#define LU2_REGFRAME	0x04

//
// void
// _local_unwind2(PEXCEPTION_REGISTRATION RegistrationFrame,
//			      LONG TryLevel)
//
// Parameters:
//   [EDX+08h] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [EDX+04h] - LONG TryLevel
// Registers:
//   EBP - EBP of call frame we are unwinding
// Returns:
//   Nothing
// Notes:
//   Run all termination handlers for a call frame from the current
//   try-level up to (but not including) the given stop try-level.
__local_unwind2:
    // Setup our call frame so we can access parameters using EDX
    //pushl    %ebp
    movl     %esp, %edx

    // FIXME: Setup an EXCEPTION_REGISTRATION entry to protect the
    // unwinding in case something goes wrong

.lu2_next_scope:

    // Keep a pointer to the exception registration in EBX
    movl     LU2_REGFRAME(%edx), %ebx

    // If we have reached the end of the chain or we're asked to stop here
    // by the caller then exit
    movl     ER_TRYLEVEL(%ebx), %eax

    cmpl     $-1, %eax
    je       .lu2_done

    cmpl     LU2_TRYLEVEL(%edx), %eax
    je       .lu2_done

    // Keep a pointer to the scopetable in ESI
    movl     ER_SCOPETABLE(%ebx), %esi

    // Compute the offset of the entry in the scopetable that describes
    // the scope that is to be unwound. Put the offset in EDI.
    movl	ST_TRYLEVEL(%esi), %edi
    lea     (%edi, %edi, 2), %edi
    shll    $2, %edi
    addl    %esi, %edi

    // If this is not a termination handler then skip it
    cmpl     $0, ST_FILTER(%edi)
    jne      .lu2_next_scope

    // Save the previous try-level in the exception registration structure
    movl     ST_TRYLEVEL(%edi), %eax
    movl     %eax, ER_TRYLEVEL(%ebx)

    // Fetch the address of the termination handler
    movl     ST_HANDLER(%edi), %eax

    // Termination handlers may trash all registers so save the
    // important ones and then call the handler
    pushl    %edx
    call 	 *%eax

	// Get our base pointer back
    popl     %edx

    jmp      .lu2_next_scope

.lu2_done:

    // FIXME: Tear down the EXCEPTION_REGISTRATION entry setup to protect
    // the unwinding

	//movl	%esi, %esp
    //popl	%ebp
    ret

#define EH3_DISPCONTEXT	0x14
#define EH3_CONTEXT		0x10
#define EH3_REGFRAME	0x0C
#define EH3_ERECORD		0x08

// Parameters:
//   [ESP+14h] - PVOID DispatcherContext
//   [ESP+10h] - PCONTEXT Context
//   [ESP+0Ch] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [ESP+08h] - PEXCEPTION_RECORD ExceptionRecord
// Registers:
//   Unknown
// Returns:
//   EXCEPTION_DISPOSITION - How this handler handled the exception
// Notes:
//   Try to find an exception handler that will handle the exception.
//   Traverse the entries in the scopetable that is associated with the
//   exception registration passed as a parameter to this function.
//   If an exception handler that will handle the exception is found, it
//   is called and this function never returns
__except_handler3:
    // Setup our call frame so we can access parameters using EBP
    pushl    %ebp				// Standard ESP in frame (considered part of EXCEPTION_REGISTRATION)
    movl     %esp, %ebp

    // Don't trust the direction flag to be cleared
    cld

    // Either we're called to handle an exception or we're called to unwind    
    movl	EH3_ERECORD(%ebp), %eax
    testl	$EXCEPTION_UNWIND_MODE, EREC_FLAGS(%eax)
    jnz		.eh3_unwind

    // Keep a pointer to the exception registration in EBX
    movl     EH3_REGFRAME(%ebp), %ebx

    // Build an EXCEPTION_POINTERS structure on the stack and store it's
    // address in the EXCEPTION_REGISTRATION structure
    movl     EH3_CONTEXT(%esp), %eax
    pushl    %ebx						// Registration frame
    pushl    %eax						// Context
    movl     %esp, ER_EPOINTERS(%ebx)	// Pointer to EXCEPTION_REGISTRATION on the stack

    // Keep current try-level in EDI
    movl     ER_TRYLEVEL(%ebx), %edi

    // Keep a pointer to the scopetable in ESI
    movl     ER_SCOPETABLE(%ebx), %esi

.eh3_next_scope:

    // If we have reached the end of the chain then exit
    cmpl     $-1, %edi
    je       .eh3_search

    // Compute the offset of the entry in the scopetable and store
    // the absolute address in EAX
    lea     (%edi, %edi, 2), %eax
    shll    $2, %eax
    addl    %esi, %eax

    // Fetch the address of the filter routine
    movl     ST_FILTER(%eax), %eax

    // If this is a termination handler then skip it
    cmpl     $0, %eax
    je       .eh3_continue

    // Filter routines may trash all registers so save the important
    // ones before restoring the call frame ebp and calling the handler
    pushl	%ebp
    pushl	%edi				// Stop try-level
    lea		ER_EBP(%ebx), %ebp
    call	*%eax
    popl	%edi				// Stop try-level
    popl	%ebp

    // Reload EBX with registration frame address
    movl	EH3_REGFRAME(%ebp), %ebx

    // Be more flexible here by checking if the return value is less than
    // zero, equal to zero, or larger than zero instead of the defined
    // values:
    //   -1 (EXCEPTION_CONTINUE_EXECUTION)
    //    0 (EXCEPTION_CONTINUE_SEARCH)
    //   +1 (EXCEPTION_EXECUTE_HANDLER)
    orl      %eax, %eax
    jz       .eh3_continue
    js       .eh3_dismiss

    // Filter returned: EXCEPTION_EXECUTE_HANDLER

    // Ask the OS to perform global unwinding.
    pushl	%edi		// Save stop try-level
    pushl	%ebx		// Save registration frame address
    pushl	%ebx		// Registration frame address
    call	__global_unwind2
    popl	%eax		// Remove parameter to __global_unwind2
    popl	%ebx		// Restore registration frame address
    popl	%edi		// Restore stop try-level

    // Change the context structure so _except_finish is called in the
    // correct context since we return ExceptionContinueExecution.
    movl     EH3_CONTEXT(%ebp), %eax
    
    movl     %edi, CONTEXT_EDI(%eax)		// Stop try-level
    movl     %ebx, CONTEXT_EBX(%eax)		// Registration frame address
    movl     $_except_finish, CONTEXT_EIP(%eax)

    movl     $ExceptionContinueExecution, %eax
    jmp      .eh3_return

    // Filter returned: EXCEPTION_CONTINUE_SEARCH
.eh3_continue:

    // Reload ESI because the filter routine may have trashed it
    movl     ER_SCOPETABLE(%ebx), %esi

    // Go one try-level closer to the top
    lea      (%edi, %edi, 2), %edi
    shll     $2, %edi
    addl     %esi, %edi
    movl     ST_TRYLEVEL(%edi), %edi

    jmp      .eh3_next_scope

    // Filter returned: EXCEPTION_CONTINUE_EXECUTION
    // Continue execution like nothing happened
.eh3_dismiss:
    movl     $ExceptionContinueExecution, %eax
    jmp      .eh3_return

    // Tell the OS to search for another handler that will handle the exception
.eh3_search:

    movl     $ExceptionContinueSearch, %eax
    jmp      .eh3_return

    // Perform local unwinding
.eh3_unwind:

    movl     $ExceptionContinueSearch, %eax
    testl    $EXCEPTION_TARGET_UNWIND, EREC_FLAGS(%eax)
    jnz      .eh3_return

	// Save some important registers
    pushl	%ebp

    lea		 ER_EBP(%ebx), %ebp
    pushl    $-1
    pushl    %ebx
    call     __local_unwind2
    addl     $8, %esp

	// Restore some important registers
    popl     %ebp

    movl     $ExceptionContinueSearch, %eax

    // Get me out of here
.eh3_return:

	movl	%ebp, %esp
    popl	%ebp
    ret

// Parameters:
//   None
// Registers:
//   EBX - Pointer to exception registration structure
//   EDI - Stop try-level
// Returns:
//   -
// Notes:
//   -
_except_finish:

    // Setup EBP for the exception handler. By doing this the exception
    // handler can access local variables as normal
    lea		ER_EBP(%ebx), %ebp

	// Save some important registers
    pushl	%ebp
    pushl	%ebx
    pushl	%edi

    // Stop try-level
    pushl	%edi

    // Pointer to exception registration structure
    pushl    %ebx
    call     __local_unwind2
    addl     $8, %esp

	// Restore some important registers
    popl     %edi
    popl     %ebx
    popl     %ebp

    // Keep a pointer to the scopetable in ESI
    movl     ER_SCOPETABLE(%ebx), %esi

    // Compute the offset of the entry in the scopetable and store
    // the absolute address in EDI
    lea     (%edi, %edi, 2), %edi
    shll    $2, %edi
    addl    %esi, %edi

    // Set the current try-level to the previous try-level and call
    // the exception handler
    movl     ST_TRYLEVEL(%edi), %eax
    movl     %eax, ER_TRYLEVEL(%ebx)
    movl     ST_HANDLER(%edi), %eax

    call    *%eax

    // We should never get here
    ret
