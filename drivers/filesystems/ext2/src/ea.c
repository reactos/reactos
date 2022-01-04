/*
* COPYRIGHT:        See COPYRIGHT.TXT
* PROJECT:          Ext2 File System Driver for Windows >= NT
* FILE:             ea.c
* PROGRAMMER:       Matt Wu <mattwu@163.com>  Kaho Ng <ngkaho1234@gmail.com>
* HOMEPAGE:         http://www.ext2fsd.com
* UPDATE HISTORY:
*/

/* INCLUDES *****************************************************************/

#include "ext2fs.h"
#include <linux/ext4_xattr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2QueryEa)
#pragma alloc_text(PAGE, Ext2SetEa)
#pragma alloc_text(PAGE, Ext2IsEaNameValid)
#endif

// Ea iterator
struct EaIterator {
	// Return only an entry
	BOOLEAN ReturnSingleEntry;

	// Is the buffer overflowing?
	BOOL OverFlow;

	// FILE_FULL_EA_INFORMATION output buffer
	PFILE_FULL_EA_INFORMATION FullEa;
	PFILE_FULL_EA_INFORMATION LastFullEa;

	// UserBuffer's size
	ULONG UserBufferLength;

	// Remaining UserBuffer's size
	ULONG RemainingUserBufferLength;

	// Start scanning from this EA
	ULONG EaIndex;

	// Next EA index returned by Ext2IterateAllEa
	ULONG EaIndexCounter;
};

static int Ext2IterateAllEa(struct ext4_xattr_ref *xattr_ref, struct ext4_xattr_item *item, BOOL is_last)
{
	struct EaIterator *pEaIterator = xattr_ref->iter_arg;
	ULONG EaEntrySize = 4 + 1 + 1 + 2 + item->name_len + 1 + item->data_size;
	ASSERT(pEaIterator);
	if (!is_last && !pEaIterator->ReturnSingleEntry)
		EaEntrySize = ALIGN_UP(EaEntrySize, ULONG);

	// Start iteration from index specified
	if (pEaIterator->EaIndexCounter < pEaIterator->EaIndex) {
		pEaIterator->EaIndexCounter++;
		return EXT4_XATTR_ITERATE_CONT;
	}
	pEaIterator->EaIndexCounter++;

	if (EaEntrySize > pEaIterator->RemainingUserBufferLength) {
		pEaIterator->OverFlow = TRUE;
		return EXT4_XATTR_ITERATE_STOP;
	}
	pEaIterator->FullEa->NextEntryOffset = 0;
	pEaIterator->FullEa->Flags = 0;
	pEaIterator->FullEa->EaNameLength = (UCHAR)item->name_len;
	pEaIterator->FullEa->EaValueLength = (USHORT)item->data_size;
	RtlCopyMemory(&pEaIterator->FullEa->EaName[0],
		item->name,
		item->name_len);
	RtlCopyMemory(&pEaIterator->FullEa->EaName[0] + item->name_len + 1,
		item->data,
		item->data_size);

	// Link FullEa and LastFullEa together
	if (pEaIterator->LastFullEa) {
		pEaIterator->LastFullEa->NextEntryOffset = (ULONG)
			((PCHAR)pEaIterator->FullEa - (PCHAR)pEaIterator->LastFullEa);
	}

	pEaIterator->LastFullEa = pEaIterator->FullEa;
	pEaIterator->FullEa = (PFILE_FULL_EA_INFORMATION)
			((PCHAR)pEaIterator->FullEa + EaEntrySize);
	pEaIterator->RemainingUserBufferLength -= EaEntrySize;

	if (pEaIterator->ReturnSingleEntry)
		return EXT4_XATTR_ITERATE_STOP;

	return EXT4_XATTR_ITERATE_CONT;
}

NTSTATUS
Ext2QueryEa (
	IN PEXT2_IRP_CONTEXT    IrpContext
)
{
	PIRP                Irp = NULL;
	PIO_STACK_LOCATION  IrpSp;

	PDEVICE_OBJECT      DeviceObject;

	PEXT2_VCB           Vcb = NULL;
	PEXT2_FCB           Fcb = NULL;
	PEXT2_CCB           Ccb = NULL;
	PEXT2_MCB           Mcb = NULL;

	PUCHAR  UserEaList;
	ULONG   UserEaListLength;
	ULONG   UserEaIndex;

	BOOLEAN RestartScan;
	BOOLEAN ReturnSingleEntry;
	BOOLEAN IndexSpecified;

	BOOLEAN             MainResourceAcquired = FALSE;
	BOOLEAN             XattrRefAcquired = FALSE;

	NTSTATUS            Status = STATUS_UNSUCCESSFUL;

	struct ext4_xattr_ref xattr_ref;
	PCHAR UserBuffer;

	ULONG UserBufferLength = 0;
	ULONG RemainingUserBufferLength = 0;

	PFILE_FULL_EA_INFORMATION FullEa, LastFullEa = NULL;

	_SEH2_TRY {

		Ccb = IrpContext->Ccb;
		ASSERT(Ccb != NULL);
		ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
			(Ccb->Identifier.Size == sizeof(EXT2_CCB)));
		DeviceObject = IrpContext->DeviceObject;
		Vcb = (PEXT2_VCB)DeviceObject->DeviceExtension;
		Fcb = IrpContext->Fcb;
		Mcb = Fcb->Mcb;
		Irp = IrpContext->Irp;
		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		Irp->IoStatus.Information = 0;

		//
		// Receive input parameter from caller
		//
		UserBuffer = Ext2GetUserBuffer(Irp);
		if (!UserBuffer) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			_SEH2_LEAVE;
		}
		UserBufferLength = IrpSp->Parameters.QueryEa.Length;
		RemainingUserBufferLength = UserBufferLength;
		UserEaList = IrpSp->Parameters.QueryEa.EaList;
		UserEaListLength = IrpSp->Parameters.QueryEa.EaListLength;
		UserEaIndex = IrpSp->Parameters.QueryEa.EaIndex;
		RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
		ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
		IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

		if (!Mcb)
			_SEH2_LEAVE;

		//
		// We do not allow multiple instance gaining EA access to the same file
		//
		if (!ExAcquireResourceExclusiveLite(
			&Fcb->MainResource,
			IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}
		MainResourceAcquired = TRUE;

		Status = Ext2WinntError(ext4_fs_get_xattr_ref(IrpContext, Vcb, Fcb->Mcb, &xattr_ref));
		if (!NT_SUCCESS(Status)) {
			DbgPrint("ext4_fs_get_xattr_ref() failed!\n");
			_SEH2_LEAVE;
		}

		FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;

		XattrRefAcquired = TRUE;

		if (RemainingUserBufferLength)
			RtlZeroMemory(FullEa, RemainingUserBufferLength);

		if (UserEaList != NULL) {
			int i = 0;
			PFILE_GET_EA_INFORMATION GetEa;
			for (GetEa = (PFILE_GET_EA_INFORMATION)&UserEaList[0];
					GetEa < (PFILE_GET_EA_INFORMATION)((PUCHAR)UserEaList
					+ UserEaListLength);
				GetEa = (GetEa->NextEntryOffset == 0
					? (PFILE_GET_EA_INFORMATION)MAXUINT_PTR
					: (PFILE_GET_EA_INFORMATION)((PUCHAR)GetEa
						+ GetEa->NextEntryOffset))) {

				size_t ItemSize;
				OEM_STRING Str;
				ULONG EaEntrySize;
				BOOL is_last = !GetEa->NextEntryOffset;

				Str.MaximumLength = Str.Length = GetEa->EaNameLength;
				Str.Buffer = &GetEa->EaName[0];

				//
				// At the moment we only need to know whether the item exists
				// and its size.
				//
				Status = Ext2WinntError(ext4_fs_get_xattr(&xattr_ref,
					EXT4_XATTR_INDEX_USER,
					Str.Buffer,
					Str.Length,
					NULL,
					0,
					&ItemSize));
				if (!NT_SUCCESS(Status))
					continue;

				//
				//  We were not able to locate the name therefore we must
				//  dummy up a entry for the query.  The needed Ea size is
				//  the size of the name + 4 (next entry offset) + 1 (flags)
				//  + 1 (name length) + 2 (value length) + the name length +
				//  1 (null byte) + Data Size.
				//
				EaEntrySize = 4 + 1 + 1 + 2 + GetEa->EaNameLength + 1 + ItemSize;
				if (!is_last)
					EaEntrySize = ALIGN_UP(EaEntrySize, ULONG);

				if (EaEntrySize > RemainingUserBufferLength) {

					Status = i ? STATUS_BUFFER_OVERFLOW : STATUS_BUFFER_TOO_SMALL;
					_SEH2_LEAVE;
				}
				FullEa->NextEntryOffset = 0;
				FullEa->Flags = 0;
				FullEa->EaNameLength = GetEa->EaNameLength;
				FullEa->EaValueLength = (USHORT)ItemSize;
				RtlCopyMemory(&FullEa->EaName[0],
					&GetEa->EaName[0],
					GetEa->EaNameLength);

				//
				// This query must succeed, or is guarenteed to succeed
				// since we are only looking up
				// an EA entry in a in-memory tree structure.
				// Otherwise that means someone might be operating on
				// the xattr_ref without acquiring Inode lock.
				//
				ASSERT(NT_SUCCESS(Ext2WinntError(
					ext4_fs_get_xattr(&xattr_ref,
						EXT4_XATTR_INDEX_USER,
						Str.Buffer,
						Str.Length,
						&FullEa->EaName[0] + FullEa->EaNameLength + 1,
						ItemSize,
						&ItemSize
				))));
				FullEa->EaValueLength = (USHORT)ItemSize;

				// Link FullEa and LastFullEa together
				if (LastFullEa)
					LastFullEa->NextEntryOffset = (ULONG)((PCHAR)FullEa -
                                                          (PCHAR)LastFullEa);

				LastFullEa = FullEa;
				FullEa = (PFILE_FULL_EA_INFORMATION)
					((PCHAR)FullEa + EaEntrySize);
				RemainingUserBufferLength -= EaEntrySize;
				i++;
			}
		} else if (IndexSpecified) {
			struct EaIterator EaIterator;
			//
			//  The user supplied an index into the Ea list.
			//
			if (RemainingUserBufferLength)
				RtlZeroMemory(FullEa, RemainingUserBufferLength);

			EaIterator.OverFlow = FALSE;
			EaIterator.RemainingUserBufferLength = UserBufferLength;
			// In this case, return only an entry.
			EaIterator.ReturnSingleEntry = TRUE;
			EaIterator.FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
			EaIterator.LastFullEa = NULL;
			EaIterator.UserBufferLength = UserBufferLength;
			EaIterator.EaIndex = UserEaIndex;
			EaIterator.EaIndexCounter = 1;

			xattr_ref.iter_arg = &EaIterator;
			ext4_fs_xattr_iterate(&xattr_ref, Ext2IterateAllEa);

			RemainingUserBufferLength = EaIterator.RemainingUserBufferLength;

			Status = STATUS_SUCCESS;

			// It seems that the item isn't found
			if (RemainingUserBufferLength == UserBufferLength)
				Status = STATUS_OBJECTID_NOT_FOUND;

			if (EaIterator.OverFlow) {
				if (RemainingUserBufferLength == UserBufferLength)
					Status = STATUS_BUFFER_TOO_SMALL;
				else
					Status = STATUS_BUFFER_OVERFLOW;
			}

		} else {
			struct EaIterator EaIterator;
			//
			//  Else perform a simple scan, taking into account the restart
			//  flag and the position of the next Ea stored in the Ccb.
			//
			if (RestartScan)
				Ccb->EaIndex = 1;

			if (RemainingUserBufferLength)
				RtlZeroMemory(FullEa, RemainingUserBufferLength);

			EaIterator.OverFlow = FALSE;
			EaIterator.RemainingUserBufferLength = UserBufferLength;
			EaIterator.ReturnSingleEntry = ReturnSingleEntry;
			EaIterator.FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
			EaIterator.LastFullEa = NULL;
			EaIterator.UserBufferLength = UserBufferLength;
			EaIterator.EaIndex = Ccb->EaIndex;
			EaIterator.EaIndexCounter = 1;

			xattr_ref.iter_arg = &EaIterator;
			ext4_fs_xattr_iterate(&xattr_ref, Ext2IterateAllEa);

			RemainingUserBufferLength = EaIterator.RemainingUserBufferLength;

			if (Ccb->EaIndex < EaIterator.EaIndexCounter)
				Ccb->EaIndex = EaIterator.EaIndexCounter;

			Status = STATUS_SUCCESS;

			if (EaIterator.OverFlow) {
				if (RemainingUserBufferLength == UserBufferLength)
					Status = STATUS_BUFFER_TOO_SMALL;
				else
					Status = STATUS_BUFFER_OVERFLOW;
			}

		}
	}
	_SEH2_FINALLY {

		if (XattrRefAcquired) {
			if (!NT_SUCCESS(Status)) {
				xattr_ref.dirty = FALSE;
				ext4_fs_put_xattr_ref(&xattr_ref);
			}
			else
				Status = Ext2WinntError(ext4_fs_put_xattr_ref(&xattr_ref));
		}

		if (MainResourceAcquired) {
			ExReleaseResourceLite(&Fcb->MainResource);
		}

		if (NT_SUCCESS(Status)) {
			Ext2NotifyReportChange(
				IrpContext,
				Vcb,
				Mcb,
				FILE_NOTIFY_CHANGE_EA,
				FILE_ACTION_MODIFIED);
			Irp->IoStatus.Information = UserBufferLength - RemainingUserBufferLength;
		}

		if (!_SEH2_AbnormalTermination()) {
			if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
				Status = Ext2QueueRequest(IrpContext);
			}
			else {
				Ext2CompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;

	return Status;
}

BOOLEAN
Ext2IsEaNameValid(
	IN OEM_STRING Name
)
{
	ULONG Index;
	UCHAR Char;

	//
	//  Empty names are not valid
	//

	if (Name.Length == 0)
		return FALSE;

	//
	// Do not allow EA name longer than 255 bytes
	//
	if (Name.Length > 255)
		return FALSE;

	for (Index = 0; Index < (ULONG)Name.Length; Index += 1) {

		Char = Name.Buffer[Index];

		//
		//  Skip over and Dbcs chacters
		//
		if (FsRtlIsLeadDbcsCharacter(Char)) {

			ASSERT(Index != (ULONG)(Name.Length - 1));
			Index += 1;
			continue;
		}

		//
		//  Make sure this character is legal, and if a wild card, that
		//  wild cards are permissible.
		//
		if (!FsRtlIsAnsiCharacterLegalFat(Char, FALSE))
			return FALSE;

	}

	return TRUE;
}

NTSTATUS
Ext2SetEa (
	IN PEXT2_IRP_CONTEXT    IrpContext
)
{
	PIRP                Irp = NULL;
	PIO_STACK_LOCATION  IrpSp;

	PDEVICE_OBJECT      DeviceObject;

	PEXT2_VCB           Vcb = NULL;
	PEXT2_FCB           Fcb = NULL;
	PEXT2_CCB           Ccb = NULL;
	PEXT2_MCB           Mcb = NULL;

	BOOLEAN             MainResourceAcquired = FALSE;
	BOOLEAN             FcbLockAcquired = FALSE;
	BOOLEAN             XattrRefAcquired = FALSE;

	NTSTATUS            Status = STATUS_UNSUCCESSFUL;

	struct ext4_xattr_ref xattr_ref;
	PCHAR UserBuffer;
	ULONG UserBufferLength;

	PFILE_FULL_EA_INFORMATION FullEa;

	_SEH2_TRY {

		Ccb = IrpContext->Ccb;
		ASSERT(Ccb != NULL);
		ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
			(Ccb->Identifier.Size == sizeof(EXT2_CCB)));
		DeviceObject = IrpContext->DeviceObject;
		Vcb = (PEXT2_VCB)DeviceObject->DeviceExtension;
		Fcb = IrpContext->Fcb;
		Mcb = Fcb->Mcb;
		Irp = IrpContext->Irp;
		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		Irp->IoStatus.Information = 0;

		//
		// Receive input parameter from caller
		//
		UserBufferLength = IrpSp->Parameters.SetEa.Length;
		UserBuffer = Irp->UserBuffer;

		// Check if the EA buffer provided is valid
		Status = IoCheckEaBufferValidity((PFILE_FULL_EA_INFORMATION)UserBuffer,
			UserBufferLength,
			(PULONG)&Irp->IoStatus.Information);
		if (!NT_SUCCESS(Status))
			_SEH2_LEAVE;

		ExAcquireResourceExclusiveLite(&Vcb->FcbLock, TRUE);
		FcbLockAcquired = TRUE;

		if (!Mcb)
			_SEH2_LEAVE;

		//
		// We do not allow multiple instance gaining EA access to the same file
		//
		if (!ExAcquireResourceExclusiveLite(
			&Fcb->MainResource,
			IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {
			Status = STATUS_PENDING;
			_SEH2_LEAVE;
		}
		MainResourceAcquired = TRUE;

		Status = Ext2WinntError(ext4_fs_get_xattr_ref(IrpContext, Vcb, Fcb->Mcb, &xattr_ref));
		if (!NT_SUCCESS(Status)) {
			DbgPrint("ext4_fs_get_xattr_ref() failed!\n");
			_SEH2_LEAVE;
		}

		XattrRefAcquired = TRUE;

		//
		// Remove all existing EA entries.
		//
		ext4_xattr_purge_items(&xattr_ref);
		xattr_ref.dirty = TRUE;
		Status = STATUS_SUCCESS;

		// Iterate the whole EA buffer to do inspection
		for (FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
			FullEa < (PFILE_FULL_EA_INFORMATION)&UserBuffer[UserBufferLength];
			FullEa = (PFILE_FULL_EA_INFORMATION)(FullEa->NextEntryOffset == 0 ?
				&UserBuffer[UserBufferLength] :
				(PCHAR)FullEa + FullEa->NextEntryOffset)) {

			OEM_STRING EaName;

			EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
			EaName.Buffer = &FullEa->EaName[0];

			// Check if EA's name is valid
			if (!Ext2IsEaNameValid(EaName)) {
				Irp->IoStatus.Information = (PCHAR)FullEa - UserBuffer;
				Status = STATUS_INVALID_EA_NAME;
				_SEH2_LEAVE;
			}
		}

		// Now add EA entries to the inode
		for (FullEa = (PFILE_FULL_EA_INFORMATION)UserBuffer;
			FullEa < (PFILE_FULL_EA_INFORMATION)&UserBuffer[UserBufferLength];
			FullEa = (PFILE_FULL_EA_INFORMATION)(FullEa->NextEntryOffset == 0 ?
				&UserBuffer[UserBufferLength] :
				(PCHAR)FullEa + FullEa->NextEntryOffset)) {

				int ret;
				OEM_STRING EaName;

				EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
				EaName.Buffer = &FullEa->EaName[0];

				Status = Ext2WinntError(ret =
					ext4_fs_set_xattr_ordered(&xattr_ref,
						EXT4_XATTR_INDEX_USER,
						EaName.Buffer,
						EaName.Length,
						&FullEa->EaName[0] + FullEa->EaNameLength + 1,
						FullEa->EaValueLength));
				if (!NT_SUCCESS(Status))
					_SEH2_LEAVE;

		}
	} _SEH2_FINALLY {

		if (XattrRefAcquired) {
			if (!NT_SUCCESS(Status)) {
				xattr_ref.dirty = FALSE;
				ext4_fs_put_xattr_ref(&xattr_ref);
			} else
				Status = Ext2WinntError(ext4_fs_put_xattr_ref(&xattr_ref));
		}

		if (FcbLockAcquired) {
			ExReleaseResourceLite(&Vcb->FcbLock);
			FcbLockAcquired = FALSE;
		}

		if (MainResourceAcquired) {
			ExReleaseResourceLite(&Fcb->MainResource);
		}

		if (NT_SUCCESS(Status)) {
			Ext2NotifyReportChange(
				IrpContext,
				Vcb,
				Mcb,
				FILE_NOTIFY_CHANGE_EA,
				FILE_ACTION_MODIFIED);
		}

		if (!_SEH2_AbnormalTermination()) {
			if (Status == STATUS_PENDING || Status == STATUS_CANT_WAIT) {
				Status = Ext2QueueRequest(IrpContext);
			}
			else {
				Ext2CompleteIrpContext(IrpContext, Status);
			}
		}
	} _SEH2_END;
	return Status;
}
