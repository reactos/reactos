#ifndef _NDISHACK_H
#define _NDISHACK_H

#ifndef __NTDRIVER__

struct _RECURSIVE_MUTEX;

#undef KeAcquireSpinLockAtDpcLevel
#undef KeReleaseSpinLockFromDpcLevel
#undef KeRaiseIrql
#undef KeLowerIrql

#if 0
/*
 * VOID
 * ExFreeToNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	Lookaside,
 *	PVOID			Entry
 *	);
 *
 * FUNCTION:
 *	Inserts (pushes) the specified entry into the specified
 *	nonpaged lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to the nonpaged lookaside list
 *	Entry = Pointer to the entry that is inserted in the lookaside list
 */
static
__inline
VOID
ExFreeToNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside,
	IN	PVOID			Entry
	)
{
    Lookaside->Free( Entry );
}

/*
 * PVOID
 * ExAllocateFromNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	LookSide
 *	);
 *
 * FUNCTION:
 *	Removes (pops) the first entry from the specified nonpaged
 *	lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to a nonpaged lookaside list
 *
 * RETURNS:
 *	Address of the allocated list entry
 */
static
__inline
PVOID
ExAllocateFromNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
	PVOID Entry;

	Entry = (Lookaside->Allocate)(Lookaside->Type,
				      Lookaside->Size,
				      Lookaside->Tag);

	return Entry;
}

/*
 * VOID
 * NdisGetFirstBufferFromPacket(
 *   IN PNDIS_PACKET  _Packet,
 *   OUT PNDIS_BUFFER  * _FirstBuffer,
 *   OUT PVOID  * _FirstBufferVA,
 *   OUT PUINT  _FirstBufferLength,
 *   OUT PUINT  _TotalBufferLength)
 */
#define	NdisGetFirstBufferFromPacket(_Packet,             \
                                     _FirstBuffer,        \
                                     _FirstBufferVA,      \
                                     _FirstBufferLength,  \
                                     _TotalBufferLength)  \
{                                                         \
  PNDIS_BUFFER _Buffer;                                   \
                                                          \
  _Buffer         = (_Packet)->Private.Head;              \
  *(_FirstBuffer) = _Buffer;                              \
  if (_Buffer != NULL)                                    \
    {                                                     \
	    *(_FirstBufferVA)     = (_Buffer)->MappedSystemVa;                \
	    *(_FirstBufferLength) = (_Buffer)->Size;                          \
	    _Buffer = _Buffer->Next;                                          \
		  *(_TotalBufferLength) = *(_FirstBufferLength);              \
		  while (_Buffer != NULL) {                                   \
		    *(_TotalBufferLength) += (_Buffer)->Size;                 \
		    _Buffer = _Buffer->Next;                                  \
		  }                                                           \
    }                             \
  else                            \
    {                             \
      *(_FirstBufferVA) = 0;      \
      *(_FirstBufferLength) = 0;  \
      *(_TotalBufferLength) = 0;  \
    } \
}

/*
 * VOID
 * NdisQueryBuffer(
 *   IN PNDIS_BUFFER  Buffer,
 *   OUT PVOID  *VirtualAddress OPTIONAL,
 *   OUT PUINT  Length)
 */
#define NdisQueryBuffer(Buffer,         \
                        VirtualAddress, \
                        Length)         \
{                                       \
	if (VirtualAddress)                   \
		*((PVOID*)VirtualAddress) = Buffer->MappedSystemVa; \
                                        \
	*((PUINT)Length) = (Buffer)->Size; \
}

/*
 * VOID
 * NdisQueryPacket(
 *     IN  PNDIS_PACKET    Packet,
 *     OUT PUINT           PhysicalBufferCount OPTIONAL,
 *     OUT PUINT           BufferCount         OPTIONAL,
 *     OUT PNDIS_BUFFER    *FirstBuffer        OPTIONAL,
 *     OUT PUINT           TotalPacketLength   OPTIONAL);
 */
#define NdisQueryPacket(Packet,                                                 \
                        PhysicalBufferCount,                                    \
                        BufferCount,                                            \
                        FirstBuffer,                                            \
                        TotalPacketLength)                                      \
{                                                                               \
    if (FirstBuffer)                                                            \
        *((PNDIS_BUFFER*)FirstBuffer) = (Packet)->Private.Head;                 \
    if ((TotalPacketLength) || (BufferCount) || (PhysicalBufferCount))          \
    {                                                                           \
        if (!(Packet)->Private.ValidCounts) {                                   \
            UINT _Offset;                                                       \
            UINT _PacketLength;                                                 \
            PNDIS_BUFFER _NdisBuffer;                                           \
            UINT _PhysicalBufferCount = 0;                                      \
            UINT _TotalPacketLength   = 0;                                      \
            UINT _Count               = 0;                                      \
                                                                                \
            for (_NdisBuffer = (Packet)->Private.Head;                          \
                _NdisBuffer != (PNDIS_BUFFER)NULL;                              \
                _NdisBuffer = _NdisBuffer->Next)                                \
            {                                                                   \
                _PhysicalBufferCount += NDIS_BUFFER_TO_SPAN_PAGES(_NdisBuffer); \
                NdisQueryBufferOffset(_NdisBuffer, &_Offset, &_PacketLength);   \
                _TotalPacketLength += _PacketLength;                            \
                _Count++;                                                       \
            }                                                                   \
            (Packet)->Private.PhysicalCount = _PhysicalBufferCount;             \
            (Packet)->Private.TotalLength   = _TotalPacketLength;               \
            (Packet)->Private.Count         = _Count;                           \
            (Packet)->Private.ValidCounts   = TRUE;                             \
		}                                                                       \
                                                                                \
        if (PhysicalBufferCount)                                                \
            *((PUINT)PhysicalBufferCount) = (Packet)->Private.PhysicalCount;    \
                                                                                \
        if (BufferCount)                                                        \
            *((PUINT)BufferCount) = (Packet)->Private.Count;                    \
                                                                                \
        if (TotalPacketLength)                                                  \
            *((PUINT)TotalPacketLength) = (Packet)->Private.TotalLength;        \
    }                                                                           \
}
#endif

#endif/*__NTDRIVER__*/

#endif/*_NDISHACK_H*/
