/*
 * PROJECT:         ReactOS Source Development Kit (SDK)
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/internal/i386/callconv.s
 * PURPOSE:         x86 Calling Convention Helpers
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

//
// @name CountArg
//
// This macro counts the number of arguments in the ArgList and returns
// the value in cCount.
//
// @param cCount - Count of arguments
// @param ArgList - Argument list
//
// @remark None.
//
.macro CountArg cCount:req,ArgList:vararg

	cCount = 0

	.ifnb \ArgList
    	.irp arg, \ArgList
			cCount = cCount+1
		.endr
	.endif
.endm

//
// @name RevPush
//
// This macro pushes the arguments in ArgList in the reverse order
// and returns the number of arguments in cCount
//
// @param cCount - Count of arguments
// @param ArgList - Argument list
//
// @remark None.
//
.macro RevPush cCount:req,ArgList:vararg
	LOCAL index, x

	CountArg cCount, ArgList

	index = cCount
	.rept cCount
		x = 0
		.irp arg,ArgList
			x=x+1
			.ifeq index-x
				push arg
				.exitm
			.endif
		.endr

		index = index-1
	.endr
.endm

//
// @name stdCallCall
//
// This macro performs a function call using the STDCALL convention and applies
// the correct name decoration required based on the stack bytes
//
// @param Func - Function name
// @param N - Number of stack bytes for arguments
//
// @remark None.
//
.macro stdCallCall Func:req,N:req
	.ifdef __imp_&Func&@&N
    	call dword ptr [__imp_&Func&@&N]
	.else
    	call Func&@&N
	.endif
.endm

//
// @name stdCall
//
// This macro pushes the arguments required for a function call using the
// STDCALL convention and then issues the call
//
// @param Func - Function name
// @param ArgList - Argument list
//
// @remark None.
//
.macro stdCall Func:req,ArgList:vararg
	LOCAL Bytes

	RevPush Bytes,ArgList
	Bytes = Bytes*4

	stdCallCall Func, %(Bytes)
.endm
