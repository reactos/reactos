/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/fs.c
 * PURPOSE:         Implements MDL Cache Manager Functions
 * PROGRAMMER:      Alex Ionescu
 * UPDATE HISTORY:
 *                  Created 20/06/04
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
CcMdlRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	OUT	PMDL			* MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	)
{
	UNIMPLEMENTED;
}

/**********************************************************************
 * NAME							INTERNAL
 * 	CcMdlReadCompleteDev@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	MdlChain
 *	DeviceObject
 *	
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	Used by CcMdlReadComplete@8 and FsRtl
 *
 */
VOID STDCALL
CcMdlReadCompleteDev (IN	PMDL		MdlChain,
		      IN	PDEVICE_OBJECT	DeviceObject)
{
  UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	CcMdlReadComplete@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v13.
 *
 * @unimplemented
 */
VOID STDCALL
CcMdlReadComplete (IN	PFILE_OBJECT	FileObject,
		   IN	PMDL		MdlChain)
{
   PDEVICE_OBJECT	DeviceObject = NULL;
   
   DeviceObject = IoGetRelatedDeviceObject (FileObject);
   /* FIXME: try fast I/O first */
   CcMdlReadCompleteDev (MdlChain,
			 DeviceObject);
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcMdlWriteComplete (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PMDL			MdlChain
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcMdlWriteAbort (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain
    )
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcPrepareMdlWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	OUT	PMDL			* MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	)
{
	UNIMPLEMENTED;
}
