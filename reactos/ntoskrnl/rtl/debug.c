/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/dbgprint.c
 * PURPOSE:         Debug output
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* DATA *********************************************************************/

typedef struct
{
	ULONG ComponentId;
	ULONG Level;
} KD_COMPONENT_DATA;
#define MAX_KD_COMPONENT_TABLE_ENTRIES 128
KD_COMPONENT_DATA KdComponentTable[MAX_KD_COMPONENT_TABLE_ENTRIES];
ULONG KdComponentTableEntries = 0;

/* FUNCTIONS ****************************************************************/

/*
 * Note: DON'T CHANGE THIS FUNCTION!!!
 *       DON'T CALL HalDisplayString OR SOMETING ELSE!!!
 *       You'll only break the serial/bochs debugging feature!!!
 */

/*
 * @implemented
 */
ULONG STDCALL
vDbgPrintExWithPrefix(IN LPCSTR Prefix,
                      IN ULONG ComponentId,
                      IN ULONG Level,
                      IN LPCSTR Format,
                      IN va_list ap)
{
   ANSI_STRING DebugString;
   CHAR Buffer[513];
   PCHAR pBuffer;
   ULONG pBufferSize;
#ifdef SERIALIZE_DBGPRINT
#  define MESSAGETABLE_SIZE  16
   LONG MyTableIndex;
   static LONG Lock = 0;
   static LONG TableWriteIndex = 0, TableReadIndex = 0;
   static CHAR MessageTable[MESSAGETABLE_SIZE][sizeof(Buffer)] = { { '\0' } };
#endif /* SERIALIZE_DBGPRINT */

   /* TODO FIXME - call NtQueryDebugFilterState() instead per Alex */
   if ( !DbgQueryDebugFilterState ( ComponentId, Level ) )
      return 0;

   /* init ansi string */
   DebugString.Buffer = Buffer;
   DebugString.MaximumLength = sizeof(Buffer);

   pBuffer = Buffer;
   pBufferSize = sizeof(Buffer);
   DebugString.Length = 0;
   if ( Prefix && *Prefix )
   {
      DebugString.Length = strlen(Prefix);
      if ( DebugString.Length >= sizeof(Buffer) )
         DebugString.Length = sizeof(Buffer) - 1;
      memmove ( Buffer, Prefix, DebugString.Length );
      Buffer[DebugString.Length] = '\0';
      pBuffer = &Buffer[DebugString.Length];
      pBufferSize -= DebugString.Length;
   }

   DebugString.Length += _vsnprintf ( pBuffer, pBufferSize, Format, ap );
   Buffer[sizeof(Buffer)-1] = '\0';

#ifdef SERIALIZE_DBGPRINT
   /* check if we are already running */
   if (InterlockedCompareExchange(&Lock, 1, 0) == 1)
     {
        MyTableIndex = InterlockedIncrement(&TableWriteIndex) - 1;
        InterlockedCompareExchange(&TableWriteIndex, 0, MESSAGETABLE_SIZE);
        MyTableIndex %= MESSAGETABLE_SIZE;

        if (MessageTable[MyTableIndex][0] != '\0') /* table is full */
          {
             DebugString.Buffer = "CRITICAL ERROR: DbgPrint Table is FULL!";
             DebugString.Length = 39;
             KdpPrintString(&DebugString);
             for (;;);
          }
        else
          {
             memcpy(MessageTable[MyTableIndex], DebugString.Buffer, DebugString.Length);
             MessageTable[MyTableIndex][DebugString.Length] = '\0';
          }
     }
   else
     {
#endif /* SERIALIZE_DBGPRINT */
        KdpPrintString (&DebugString);
#ifdef SERIALIZE_DBGPRINT
        MyTableIndex = TableReadIndex;
        while (MessageTable[MyTableIndex][0] != '\0')
          {
             /*DebugString.Buffer = "$$$";
             DebugString.Length = 3;
             KdpPrintString(&DebugString);*/

             DebugString.Buffer = MessageTable[MyTableIndex];
             DebugString.Length = strlen(DebugString.Buffer);
             DebugString.MaximumLength = DebugString.Length + 1;

             KdpPrintString(&DebugString);
             MessageTable[MyTableIndex][0] = '\0';

             MyTableIndex = InterlockedIncrement(&TableReadIndex);
             InterlockedCompareExchange(&TableReadIndex, 0, MESSAGETABLE_SIZE);
             MyTableIndex %= MESSAGETABLE_SIZE;
          }
        InterlockedDecrement(&Lock);
     }
#  undef MESSAGETABLE_SIZE
#endif /* SERIALIZE_DBGPRINT */

   return (ULONG)DebugString.Length;
}

/*
 * @implemented
 */
ULONG STDCALL
vDbgPrintEx(IN ULONG ComponentId,
            IN ULONG Level,
            IN LPCSTR Format,
            IN va_list ap)
{
	return vDbgPrintExWithPrefix ( NULL, ComponentId, Level, Format, ap );
}

/*
 * @implemented
 */
ULONG
DbgPrint(PCH Format, ...)
{
	va_list ap;
	ULONG rc;

	va_start (ap, Format);
	/* TODO FIXME - use DPFLTR_DEFAULT_ID and DPFLTR_INFO_LEVEL
	 *
	 * https://www.osronline.com/article.cfm?article=295
	 *
	 * ( first need to add those items to default registry and write the code
	 *   to load those settings so we don't anger ros-devs when DbgPrint() suddenly
	 *   stops working )
	 *
	 * ( also when you do this, remove -1 hack from DbgQueryDebugFilterState() )
	 */
	rc = vDbgPrintExWithPrefix ( NULL, (ULONG)-1, (ULONG)-1, Format, ap );
	va_end (ap);

	return rc;
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrintEx(IN ULONG ComponentId,
           IN ULONG Level,
           IN PCH Format,
           ...)
{
	va_list ap;
	ULONG rc;

	va_start (ap, Format);
	rc = vDbgPrintExWithPrefix ( NULL, ComponentId, Level, Format, ap );
	va_end (ap);

	return rc;
}

/*
 * @unimplemented
 */
ULONG
__cdecl
DbgPrintReturnControlC(PCH Format,
                       ...)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
DbgPrompt(PCH OutputString,
          PCH InputString,
          USHORT InputSize)
{
    ANSI_STRING Output;
    ANSI_STRING Input;

    Input.Length = 0;
    Input.MaximumLength = InputSize;
    Input.Buffer = InputString;

    Output.Length = strlen (OutputString);
    Output.MaximumLength = Output.Length + 1;
    Output.Buffer = OutputString;

    /* FIXME: Not implemented yet!
    KdpPromptString (&Output, &Input); */
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
DbgQueryDebugFilterState(IN ULONG ComponentId,
                         IN ULONG Level)
{
	int i;

	/* HACK HACK HACK - see comments in DbgPrint() */
	if ( ComponentId == -1 )
		return TRUE;

	/* convert Level to mask if it isn't already one */
	if ( Level < 32 )
		Level = 1 << Level;

	for ( i = 0; i < KdComponentTableEntries; i++ )
	{
		if ( ComponentId == KdComponentTable[i].ComponentId )
		{
			if ( Level & KdComponentTable[i].Level )
				return TRUE;
			break;
		}
	}
	return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
DbgSetDebugFilterState(IN ULONG ComponentId,
                       IN ULONG Level,
                       IN BOOLEAN State)
{
	int i;
	for ( i = 0; i < KdComponentTableEntries; i++ )
	{
		if ( ComponentId == KdComponentTable[i].ComponentId )
			break;
	}
	if ( i == KdComponentTableEntries )
	{
		if ( i == MAX_KD_COMPONENT_TABLE_ENTRIES )
			return STATUS_INVALID_PARAMETER_1;
		++KdComponentTableEntries;
		KdComponentTable[i].ComponentId = ComponentId;
		KdComponentTable[i].Level = 0;
	}
	if ( State )
		KdComponentTable[i].Level |= Level;
	else
		KdComponentTable[i].Level &= ~Level;
	return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgLoadImageSymbols(IN PUNICODE_STRING Name,
                    IN ULONG Base,
                    IN ULONG Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
