/* $Id: tunnel.c,v 1.4 2002/09/08 10:23:20 chorns Exp $
 *
 * reactos/ntoskrnl/fs/tunnel.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>


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
VOID
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
