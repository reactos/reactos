/* $Id: tunnel.c,v 1.3 2002/09/07 15:12:50 chorns Exp $
 *
 * reactos/ntoskrnl/fs/tunnel.c
 *
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAddToTunnelCache@32
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlAddToTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  ShortName,
    IN PUNICODE_STRING  LongName,
    IN BOOLEAN          KeyByShortName,
    IN ULONG            DataLength,
    IN PVOID            Data
    )
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDeleteKeyFromTunnelCache@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlDeleteKeyFromTunnelCache (
    IN PTUNNEL      Cache,
    IN ULONGLONG    DirectoryKey
    )
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDeleteTunnelCache@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlDeleteTunnelCache (
    IN PTUNNEL Cache
    )
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFindInTunnelCache@32
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlFindInTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  Name,
    OUT PUNICODE_STRING ShortName,
    OUT PUNICODE_STRING LongName,
    IN OUT PULONG       DataLength,
    OUT PVOID           Data
    )
{
  UNIMPLEMENTED
  return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlInitializeTunnelCache@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlInitializeTunnelCache (
    IN PTUNNEL Cache
    )
{
}


/* EOF */
