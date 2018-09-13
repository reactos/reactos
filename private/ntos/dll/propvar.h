//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993-1998
//
// File:        propvar.h
//
// Contents:    PROPVARIANT manipulation code
//
// History:     15-Aug-95   vich        created
//              01-Jul-96   MikeHill    Updated to allow Win32 SEH removal
//
//---------------------------------------------------------------------------

#ifndef _PROPVAR_H_
#define _PROPVAR_H_

#include <debnot.h>
#include <propset.h>

SERIALIZEDPROPERTYVALUE *
RtlConvertVariantToProperty(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVector,
    OPTIONAL OUT ULONG *pcIndirect);

SERIALIZEDPROPERTYVALUE *
RtlConvertVariantToPropertyNoEH(     // No NT Exception Handling version
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVector,
    OPTIONAL OUT ULONG *pcIndirect,
    OUT NTSTATUS *pstatus);

BOOLEAN
RtlConvertPropertyToVariant(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma);

BOOLEAN
RtlConvertPropertyToVariantNoEH(     // No NT Exception Handling version
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma,
    OUT NTSTATUS *pstatus);




SERIALIZEDPROPERTYVALUE *
PrConvertVariantToProperty(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVector,
    OPTIONAL OUT ULONG *pcIndirect);

SERIALIZEDPROPERTYVALUE *
PrConvertVariantToPropertyNoEH(     // No NT Exception Handling version
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVector,
    OPTIONAL OUT ULONG *pcIndirect,
    OUT NTSTATUS *pstatus);

BOOLEAN
PrConvertPropertyToVariant(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma);

BOOLEAN
PrConvertPropertyToVariantNoEH(     // No NT Exception Handling version
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma,
    OUT NTSTATUS *pstatus);





#ifndef KERNEL
VOID
CleanupVariants(
    IN PROPVARIANT *pvar,
    IN ULONG cprop,
    IN PMemoryAllocator *pma);
#endif

#if DBGPROP
BOOLEAN IsUnicodeString(WCHAR const *pwszname, ULONG cb);
BOOLEAN IsAnsiString(CHAR const *pszname, ULONG cb);
#endif


//+--------------------------------------------------------------------------
// Function:    SignalOverflow, SignalInvalidParameter, SignalStatus
//
// Synopsis:    ASSERT and raise data corrupt/overflow/specified error
//
// Arguments:   [szReason]              -- string explanation
//              [Status]                -- Status to raise (SignalStatus only)
//
// Returns:     None
//+--------------------------------------------------------------------------


#define StatusOverflow(pstatus, szReason)           \
          *(pstatus) = STATUS_BUFFER_OVERFLOW;      \
          TraceStatus(szReason)

#define StatusAccessDenied(pstatus, szReason)   \
          *(pstatus) = STATUS_ACCESS_DENIED;        \
          TraceStatus(szReason);

#define StatusInvalidParameter(pstatus, szReason)   \
          *(pstatus) = STATUS_INVALID_PARAMETER;    \
          TraceStatus(szReason);

#define StatusNoMemory(pstatus, szReason)           \
          *(pstatus) = STATUS_INSUFFICIENT_RESOURCES;\
          TraceStatus(szReason);

#define StatusDiskFull(pstatus, szReason)           \
          *(pstatus) = STATUS_DISK_FULL;            \
          TraceStatus(szReason);

#define StatusError(pstatus, szReason, Status)      \
          *(pstatus) = Status;                      \
          TraceStatus(szReason);

#ifdef KERNEL
#define StatusKBufferOverflow(pstatus, szReason) StatusOverflow(pstatus, szReason)
#else
#define StatusKBufferOverflow(pstatus, szReason) StatusNoMemory(pstatus, szReason)
#endif


#ifdef KERNEL
#define KERNELSELECT(k, u)      k
#else
#define KERNELSELECT(k, u)      u
#endif

#define DBGPROPASSERT   KERNELSELECT(DBGPROP, DBG)

#if DBGPROPASSERT
#define TraceStatus(szReason)                                   \
	{							\
	    DebugTrace(0, DEBTRACE_ERROR, (szReason "\n"));     \
	    PROPASSERTMSG(szReason, !(DebugLevel & DEBTRACE_WARN)); \
	}


#else
#define TraceStatus(szReason)
#endif



#define AssertVarField(field, cb) \
  PROPASSERT(FIELD_OFFSET(PROPVARIANT, iVal) == FIELD_OFFSET(PROPVARIANT, field) && \
	 sizeof(((PROPVARIANT *) 0)->field) == (cb))

#define AssertVarVector(field, cbElem) \
  PROPASSERT(FIELD_OFFSET(PROPVARIANT, cai.cElems) == \
	     FIELD_OFFSET(PROPVARIANT, field.cElems) && \
         FIELD_OFFSET(PROPVARIANT, cai.pElems) == \
	     FIELD_OFFSET(PROPVARIANT, field.pElems) && \
	 sizeof(((PROPVARIANT *) 0)->field.pElems[0]) == (cbElem))

#define AssertByteField(field)	    AssertVarField(field, sizeof(BYTE))
#define AssertShortField(field)	    AssertVarField(field, sizeof(SHORT))
#define AssertLongField(field)	    AssertVarField(field, sizeof(LONG))
#define AssertLongLongField(field)  AssertVarField(field, sizeof(LONGLONG))
#define AssertStringField(field)    AssertVarField(field, sizeof(VOID *))

#define AssertByteVector(field)	    AssertVarVector(field, sizeof(BYTE))
#define AssertShortVector(field)    AssertVarVector(field, sizeof(SHORT))
#define AssertLongVector(field)	    AssertVarVector(field, sizeof(LONG))
#define AssertLongLongVector(field) AssertVarVector(field, sizeof(LONGLONG))
#define AssertStringVector(field)   AssertVarVector(field, sizeof(VOID *))
#define AssertVariantVector(field)  AssertVarVector(field, sizeof(PROPVARIANT))


#define BSTRLEN(bstrVal)	*((ULONG *) bstrVal - 1)


//+-------------------------------------------------------------------
// Class:       CBufferAllocator, private
//
// Synopsis:    allocation from a buffer
//
// Notes:       The Summary catalog APIs use a single buffer to serialize row
//              values on input and deserialize them on output.  This class
//              encapsulates the memory allocation routines for these APIs.
//--------------------------------------------------------------------

class CBufferAllocator : public PMemoryAllocator
{
public:
    inline CBufferAllocator(ULONG cbBuffer, VOID *pvBuffer)
    {
	_cbFree = cbBuffer;
	_pvCur = _pvBuffer = pvBuffer;
#if _X86_	// stack variables on x86 are not aligned
	PROPASSERT(((ULONG) _pvCur & (sizeof(LONG) - 1)) == 0);
#else // RISC
	PROPASSERT(((ULONG_PTR) _pvCur & (sizeof(LONGLONG) - 1)) == 0);
#endif // X86/RISC
    }

    VOID *Allocate(ULONG cbSize);
    VOID Free(VOID *pv) { }

    inline ULONG GetFreeSize(VOID) { return(_cbFree); }

private:
    ULONG  _cbFree;
    VOID  *_pvCur;
    VOID  *_pvBuffer;
};

//+-------------------------------------------------------------------
// Member:      CBufferAllocator::Allocate, private
//
// Synopsis:    allocation from a buffer
//
// Arguments:   [cb]	-- Count of bytes to be allocated.
//
// Returns:     pointer to 'allocated' memory -- NULL if no space left
//--------------------------------------------------------------------

#define DEFINE_CBufferAllocator__Allocate			\
VOID *								\
CBufferAllocator::Allocate(ULONG cb)				\
{								\
    VOID *pv;							\
								\
    cb = (cb + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);	\
    if (cb > _cbFree)						\
    {								\
        return(NULL);						\
    }								\
    pv = _pvCur;						\
    _pvCur = (BYTE *) _pvCur + cb;				\
    _cbFree -= cb;						\
    return(pv);							\
}

#endif // !_PROPVAR_H_
