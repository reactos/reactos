/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/notify.c
 * PURPOSE:         Change Notifications and Sync for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlNotifyChangeDirectory
 * @implemented
 *
 * Lets FSD know if changes occures in the specified directory.
 * Directory will be reenumerated. 
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param FsContext
 *        Used to identify the notify structure
 *
 * @param FullDirectoryName
 *        String (A or W) containing the full directory name 
 *
 * @param NotifyList
 *        Notify list pointer (to head)
 *
 * @param WatchTree
 *        True to notify changes in subdirectories too
 *
 * @param CompletionFilter
 *        Used to define types of changes to notify
 *
 * @param NotifyIrp
 *        IRP pointer to complete notify operation. It can be null
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterChangeDirectory.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                           IN PVOID FsContext,
                           IN PSTRING FullDirectoryName,
                           IN PLIST_ENTRY NotifyList,
                           IN BOOLEAN WatchTree,
                           IN ULONG CompletionFilter,
                           IN PIRP NotifyIrp)
{
    FsRtlNotifyFilterChangeDirectory(NotifySync,
                                     NotifyList,
                                     FsContext,
                                     FullDirectoryName,
                                     WatchTree,
                                     TRUE,
                                     CompletionFilter,
                                     NotifyIrp,
                                     NULL,
                                     NULL,
                                     NULL);
}

/*++
 * @name FsRtlNotifyCleanup
 * @unimplemented
 *
 * Called by FSD when all handles to FileObject (identified by FsContext) are closed
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyCleanup(IN PNOTIFY_SYNC NotifySync,
                   IN PLIST_ENTRY NotifyList,
                   IN PVOID FsContext)
{
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlNotifyFilterChangeDirectory
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FsContext
 *        FILLME
 *
 * @param FullDirectoryName
 *        FILLME
 *
 * @param WatchTree
 *        FILLME
 *
 * @param IgnoreBuffer
 *        FILLME
 *
 * @param CompletionFilter
 *        FILLME
 *
 * @param NotifyIrp
 *        FILLME
 *
 * @param TraverseCallback
 *        FILLME
 *
 * @param SubjectContext
 *        FILLME
 *
 * @param FilterCallback
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                                 IN PLIST_ENTRY NotifyList,
                                 IN PVOID FsContext,
                                 IN PSTRING FullDirectoryName,
                                 IN BOOLEAN WatchTree,
                                 IN BOOLEAN IgnoreBuffer,
                                 IN ULONG CompletionFilter,
                                 IN PIRP NotifyIrp,
                                 IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                                 IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
                                 IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL)
{
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlNotifyFilterReportChange
 * @unimplemented
 *
 * FILLME
 *
 * @param NotifySync
 *        FILLME
 *
 * @param NotifyList
 *        FILLME
 *
 * @param FullTargetName
 *        FILLME
 *
 * @param TargetNameOffset
 *        FILLME
 *
 * @param StreamName
 *        FILLME
 *
 * @param NormalizedParentName
 *        FILLME
 *
 * @param FilterMatch
 *        FILLME
 *
 * @param Action
 *        FILLME
 *
 * @param TargetContext
 *        FILLME
 *
 * @param FilterContext
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFilterReportChange(IN PNOTIFY_SYNC NotifySync,
                              IN PLIST_ENTRY NotifyList,
                              IN PSTRING FullTargetName,
                              IN USHORT TargetNameOffset,
                              IN PSTRING StreamName OPTIONAL,
                              IN PSTRING NormalizedParentName OPTIONAL,
                              IN ULONG FilterMatch,
                              IN ULONG Action,
                              IN PVOID TargetContext,
                              IN PVOID FilterContext)
{
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlNotifyFullChangeDirectory
 * @implemented
 *
 * Lets FSD know if changes occures in the specified directory. 
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FsContext
 *        Used to identify the notify structure
 *
 * @param FullDirectoryName
 *        String (A or W) containing the full directory name 
 *
 * @param WatchTree
 *        True to notify changes in subdirectories too
 *
 * @param IgnoreBuffer
 *        True to reenumerate directory. It's ignored it NotifyIrp is null
 *
 * @param CompletionFilter
 *        Used to define types of changes to notify
 *
 * @param NotifyIrp
 *        IRP pointer to complete notify operation. It can be null
 *
 * @param TraverseCallback
 *        Pointer to a callback function. It's called each time a change is
 *        done in a subdirectory of the main directory. It's ignored it NotifyIrp
 *        is null
 *
 * @param SubjectContext
 *        Pointer to pass to SubjectContext member of TraverseCallback.
 *        It's freed after use. It's ignored it NotifyIrp is null
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterChangeDirectory.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(IN PNOTIFY_SYNC NotifySync,
                               IN PLIST_ENTRY NotifyList,
                               IN PVOID FsContext,
                               IN PSTRING FullDirectoryName,
                               IN BOOLEAN WatchTree,
                               IN BOOLEAN IgnoreBuffer,
                               IN ULONG CompletionFilter,
                               IN PIRP NotifyIrp,
                               IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
                               IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL)
{
    FsRtlNotifyFilterChangeDirectory(NotifySync,
                                     NotifyList,
                                     FsContext,
                                     FullDirectoryName,
                                     WatchTree,
                                     IgnoreBuffer,
                                     CompletionFilter,
                                     NotifyIrp,
                                     TraverseCallback,
                                     SubjectContext,
                                     NULL);
}

/*++
 * @name FsRtlNotifyFullReportChange
 * @implemented
 *
 * Complets the pending notify IRPs.
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FullTargetName
 *        String (A or W) containing the full directory name that changed
 *
 * @param TargetNameOffset
 *        Offset, in FullTargetName, of the final component that is in the changed directory 
 *
 * @param StreamName
 *        String (A or W) containing a stream name
 *
 * @param NormalizedParentName
 *        String (A or W) containing the full directory name that changed with long names
 *
 * @param FilterMatch
 *        Flags that will be compared to the completion filter
 *
 * @param Action
 *        Action code to store in user's buffer
 *
 * @param TargetContext
 *        Pointer to a callback function. It's called each time a change is
 *        done in a subdirectory of the main directory.
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterReportChange.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyFullReportChange(IN PNOTIFY_SYNC NotifySync,
                            IN PLIST_ENTRY NotifyList,
                            IN PSTRING FullTargetName,
                            IN USHORT TargetNameOffset,
                            IN PSTRING StreamName OPTIONAL,
                            IN PSTRING NormalizedParentName OPTIONAL,
                            IN ULONG FilterMatch,
                            IN ULONG Action,
                            IN PVOID TargetContext)
{
    FsRtlNotifyFilterReportChange(NotifySync,
                                  NotifyList,
                                  FullTargetName,
                                  TargetNameOffset,
                                  StreamName,
                                  NormalizedParentName,
                                  FilterMatch,
                                  Action,
                                  TargetContext,
                                  NULL);
}

/*++
 * @name FsRtlNotifyInitializeSync
 * @implemented
 *
 * Allocates the internal structure associated with notifications.
 *
 * @param NotifySync
 *        Opaque pointer. It will receive the address of the allocated internal structure.
 *
 * @return None
 *
 * @remarks This function raise an exception in case of a failure.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyInitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    PINT_NOTIFY_SYNC IntNotifySync;

    *NotifySync = NULL;
    
    IntNotifySync = FsRtlAllocatePoolWithTag(NonPagedPool, sizeof(INT_NOTIFY_SYNC), TAG('F', 'S', 'N', 'S'));
    ExInitializeFastMutex(&(IntNotifySync->FastMutex));
    IntNotifySync->OwningThread = 0;
    IntNotifySync->OwnerCount = 0;

    *NotifySync = IntNotifySync;
}

/*++
 * @name FsRtlNotifyReportChange
 * @implemented
 *
 * Complets the pending notify IRPs.
 *
 * @param NotifySync
 *        Synchronization object pointer
 *
 * @param NotifyList
 *        Notify list pointer (to head) 
 *
 * @param FullTargetName
 *        String (A or W) containing the full directory name that changed
 *
 * @param FileNamePartLength
 *        Length of the final component that is in the changed directory
 *
 * @param FilterMatch
 *        Flags that will be compared to the completion filter
 *
 * @return None
 *
 * @remarks This function only redirects to FsRtlNotifyFilterReportChange.
 *
 *--*/
VOID
NTAPI
FsRtlNotifyReportChange(IN PNOTIFY_SYNC NotifySync,
                        IN PLIST_ENTRY NotifyList,
                        IN PSTRING FullTargetName,
                        IN PUSHORT FileNamePartLength,
                        IN ULONG FilterMatch)
{
      FsRtlNotifyFilterReportChange(NotifySync,
                                    NotifyList,
                                    FullTargetName,
                                    FullTargetName->Length - *FileNamePartLength,
                                    NULL,
                                    NULL,
                                    FilterMatch,
                                    0,
                                    NULL,
                                    NULL); 
}

/*++
 * @name FsRtlNotifyUninitializeSync
 * @implemented
 *
 * Uninitialize a NOTIFY_SYNC object
 *
 * @param NotifySync
 *        Address of a pointer to a PNOTIFY_SYNC object previously
 *        initialized by FsRtlNotifyInitializeSync()
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlNotifyUninitializeSync(IN PNOTIFY_SYNC *NotifySync)
{
    if (*NotifySync)
    {
        ExFreePoolWithTag(*NotifySync, TAG('F', 'S', 'N', 'S'));
        *NotifySync = NULL;
    }
}

