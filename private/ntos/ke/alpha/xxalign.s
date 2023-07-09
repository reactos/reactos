//	TITLE("Alignment emulation")
//++
//
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    align.s
//
// Abstract:
//
//    This module implements the code to complete unaligned access
//    emulation.
//
// Author:
//
//    Joe Notarangelo 14-May-1992
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// UQUAD
// KiEmulateLoadLong(
//    IN PULONG UnalignedAddress
//    )
//
// Routine Description:
//
//    This routine returns the longword value stored at the unaligned
//    address passed in UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to long data value.
//
// Return Value:
//
//    The longword value at the address pointed to by UnalignedAddress.
//
//--

        LEAF_ENTRY(KiEmulateLoadLong)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	v0, 3(a0)		// get 2nd quadword

	extll	t0, a0, t0		// extract bytes from low quadword
	extlh	v0, a0, v0		// extract bytes from high quadword
	bis	v0, t0, v0		// v0 = longword

	addl	v0, zero, v0		// insure canonical longword form

	ret	zero, (ra)		// return

	.end	KiEmulateLoadLong



//++
//
// UQUAD
// KiEmulateLoadQuad(
//    IN PUQUAD UnalignedAddress
//    )
//
// Routine Description:
//
//    This routine returns the quadword value stored at the unaligned
//    address passed in UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to quad data value.
//
// Return Value:
//
//    The quadword value at the address pointed to by UnalignedAddress.
//
//--

        LEAF_ENTRY(KiEmulateLoadQuad)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	v0, 7(a0)		// get 2nd quadword

	extql	t0, a0, t0		// extract bytes from low quadword
	extqh	v0, a0, v0		// extract bytes from high quadword
	bis	v0, t0, v0		// v0 = longword

	ret	zero, (ra)		// return

	.end	KiEmulateLoadQuad

//++
//
// VOID
// KiEmulateStoreLong(
//    IN PULONG UnalignedAddress
//    IN UQUAD  Data
//    )
//
// Routine Description:
//
//    This routine stores the longword in Data to the UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to longword destination.
//    Data(a1)             - Supplies data value to store.
//
// Return Value:
//
//    None.
//
//--

	LEAF_ENTRY(KiEmulateStoreLong)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	t1, 3(a0)		// get 2nd quadword

	inslh	a1, a0, t2		// get bytes for high quadword
	insll	a1, a0, t3		// get bytes for low quadword

	msklh	t1, a0, t1		// clear corresponding bytes
	mskll	t0, a0, t0		// clear corresponding bytes

	bis	t1, t2, t1		// merge in bytes for high qw
	bis	t0, t3, t0		// merge in bytes for low qw

	stq_u	t1, 3(a0)		// must store high first in case
	stq_u	t0, 0(a0)		// address was actually aligned

	ret	zero, (ra)		// return

	.end	KiEmulateStoreLong


//++
//
// VOID
// KiEmulateStoreQuad(
//    IN PUQUAD UnalignedAddress
//    IN UQUAD  Data
//    )
//
// Routine Description:
//
//    This routine stores the quadword in Data to the UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to quadword destination.
//    Data(a1)             - Supplies the data value to store.
//
// Return Value:
//
//    None.
//
//--

	LEAF_ENTRY(KiEmulateStoreQuad)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	t1, 7(a0)		// get 2nd quadword

	insqh	a1, a0, t2		// get bytes for high quadword
	insql	a1, a0, t3		// get bytes for low quadword

	mskqh	t1, a0, t1		// clear corresponding bytes
	mskql	t0, a0, t0		// clear corresponding bytes

	bis	t1, t2, t1		// merge in bytes for high qw
	bis	t0, t3, t0		// merge in bytes for low qw

	stq_u	t1, 7(a0)		// must store high first in case
	stq_u	t0, 0(a0)		// address was actually aligned 

	ret	zero, (ra)		// return

	.end	KiEmulateStoreQuad


//++
//
// UQUAD
// KiEmulateLoadFloatIEEESingle(
//    IN PULONG UnalignedAddress
//    )
//
// Routine Description:
//
//    This routine returns the IEEE Single value stored at the unaligned
//    address passed in UnalignedAddress.
//
//    N.B. The value is returned as the memory format T-formatted
//	interpretation of the read S-format value.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to float single data.
//
// Return Value:
//
//    The single float value at the address pointed to by UnalignedAddress.
//
//--

	.struct 0
FlTemp:	.space	8			// temporary memory
	.space	8			// filler for alignment
FlFrameLength:				// length of stack frame

    NESTED_ENTRY(KiEmulateLoadFloatIEEESingle, FlFrameLength, zero)
	lda	sp, -FlFrameLength(sp)	// allocate temp space
    PROLOGUE_END

	//
	// get the value into an integer register
	//

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	v0, 3(a0)		// get 2nd quadword

	extll	t0, a0, t0		// extract bytes from low quadword
	extlh	v0, a0, v0		// extract bytes from high quadword
	bis	v0, t0, v0		// v0 = longword


	//
	// v0 now is S memory format, however return from exception
	//	sequence will restore floating registers as T memory format
	//   convert v0 to T memory format

	stl	v0, FlTemp(sp)		// store bytes, S-mem-format
	lds	f0, FlTemp(sp)		// now S-reg-format
	stt	f0, FlTemp(sp)		// write as T-mem-format
	ldq	v0, FlTemp(sp)		// return as T-mem_format

	lda	sp, FlFrameLength(sp)	// deallocate stack frame

	ret	zero, (ra)		// return

	.end	KiEmulateLoadFloatIEEESingle



//++
//
// UQUAD
// KiEmulateLoadFloatIEEEDouble(
//    IN PUQUAD UnalignedAddress
//    )
//
// Routine Description:
//
//    This routine returns the quadword value stored at the unaligned
//    address passed in UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to double float data value.
//
// Return Value:
//
//    The double float value at the address pointed to by UnalignedAddress.
//
//--

    LEAF_ENTRY(KiEmulateLoadFloatIEEEDouble)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	v0, 7(a0)		// get 2nd quadword

	extql	t0, a0, t0		// extract bytes from low quadword
	extqh	v0, a0, v0		// extract bytes from high quadword
	bis	v0, t0, v0		// v0 = longword

	ret	zero, (ra)		// return

	.end	KiEmulateLoadFloatIEEEDouble

//++
//
// VOID
// KiEmulateStoreFloatIEEESingle(
//    IN PULONG UnalignedAddress
//    IN UQUAD  Data
//    )
//
// Routine Description:
//
//    This routine stores the float value in Data to the UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to float destination.
//    Data(a1)             - Supplies the data value to store.
//
// Return Value:
//
//    None.
//
//--

	.struct 0
FsTemp:	.space	8			// temporary memory
	.space	8			// filler for alignment
FsFrameLength:				// length of stack frame

    NESTED_ENTRY(KiEmulateStoreFloatIEEESingle, FsFrameLength, zero)
	lda	sp, -FsFrameLength(sp)	// allocate stack space
    PROLOGUE_END

	//
	// a1 is an integer version of the T-memory format
	//   convert it to integer version of S-memory format
	//

	stq	a1, FsTemp(sp)		// store bytes, T-mem-format
	ldt	f10, FsTemp(sp)		// load back in now in S-reg-format
	sts	f10, FsTemp(sp)		// now in S-mem-format
	ldl	a1, FsTemp(sp)		// now integer version of S-mem


	//
	// now problem is just to store an unaligned longword
	//

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	t1, 3(a0)		// get 2nd quadword

	inslh	a1, a0, t2		// get bytes for high quadword
	insll	a1, a0, t3		// get bytes for low quadword

	msklh	t1, a0, t1		// clear corresponding bytes
	mskll	t0, a0, t0		// clear corresponding bytes

	bis	t1, t2, t1		// merge in bytes for high qw
	bis	t0, t3, t0		// merge in bytes for low qw

	stq_u	t1, 3(a0)		// must store high first in case
	stq_u	t0, 0(a0)		//   was actually aligned

	lda	sp, FsFrameLength(sp)	// restore stack frame

	ret	zero, (ra)		// return

	.end	KiEmulateStoreFloatIEEESingle


//++
//
// VOID
// KiEmulateStoreFloatIEEEDouble(
//    IN PUQUAD UnalignedAddress
//    IN UQUAD  Data
//    )
//
// Routine Description:
//
//    This routine stores the quadword in Data to the UnalignedAddress.
//
// Arguments:
//
//    UnalignedAddress(a0) - Supplies a pointer to double float destination.
//    Data(a1)             - Supplies the data value to store.
//
// Return Value:
//
//    None.
//
//--

	LEAF_ENTRY(KiEmulateStoreFloatIEEEDouble)

	ldq_u	t0, 0(a0)		// get 1st quadword
	ldq_u	t1, 7(a0)		// get 2nd quadword

	insqh	a1, a0, t2		// get bytes for high quadword
	insql	a1, a0, t3		// get bytes for low quadword

	mskqh	t1, a0, t1		// clear corresponding bytes
	mskql	t0, a0, t0		// clear corresponding bytes

	bis	t1, t2, t1		// merge in bytes for high qw
	bis	t0, t3, t0		// merge in bytes for low qw

	stq_u	t1, 7(a0)		// must store high first in case
	stq_u	t0, 0(a0)		//   was actually aligned

	ret	zero, (ra)		// return

	.end	KiEmulateStoreFloatIEEEDouble

