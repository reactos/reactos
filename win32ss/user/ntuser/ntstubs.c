/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Native User stubs
 * FILE:             win32ss/user/ntuser/ntstubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

DWORD
APIENTRY
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB
    return 0;
}

//
// Works like BitBlt, http://msdn.microsoft.com/en-us/library/ms532278(VS.85).aspx
//
BOOL
APIENTRY
NtUserBitBltSysBmp(
   HDC hdc,
   INT nXDest,
   INT nYDest,
   INT nWidth,
   INT nHeight,
   INT nXSrc,
   INT nYSrc,
   DWORD dwRop )
{
   BOOL Ret = FALSE;
   UserEnterExclusive();

   Ret = NtGdiBitBlt( hdc,
                   nXDest,
                   nYDest,
                   nWidth,
                  nHeight,
                hSystemBM,
                    nXSrc,
                    nYSrc,
                    dwRop,
                        0,
                        0);

   UserLeave();
   return Ret;
}

NTSTATUS
APIENTRY
NtUserBuildHimcList(DWORD dwThreadId, DWORD dwCount, HIMC *phList, LPDWORD pdwCount)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
APIENTRY
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1
)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserDrawAnimatedRects(
   HWND hwnd,
   INT idAni,
   RECT *lprcFrom,
   RECT *lprcTo)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserEvent(
   DWORD Unknown0)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPWSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi)
{
   STUB

   return 0;
}

NTSTATUS
APIENTRY
NtUserInitializeClientPfnArrays(
  PPFNCLIENT pfnClientA,
  PPFNCLIENT pfnClientW,
  PPFNCLIENTWORKER pfnClientWorker,
  HINSTANCE hmodUser)
{
   NTSTATUS Status = STATUS_SUCCESS;
   TRACE("Enter NtUserInitializeClientPfnArrays User32 0x%p\n", hmodUser);

   if (ClientPfnInit) return Status;

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead( pfnClientA, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientW, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientWorker, sizeof(PFNCLIENTWORKER), 1);
      RtlCopyMemory(&gpsi->apfnClientA, pfnClientA, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientW, pfnClientW, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientWorker, pfnClientWorker, sizeof(PFNCLIENTWORKER));

      //// FIXME: HAX! Temporary until server side is finished.
      //// Copy the client side procs for now.
      RtlCopyMemory(&gpsi->aStoCidPfn, pfnClientW, sizeof(gpsi->aStoCidPfn));

      hModClient = hmodUser;
      ClientPfnInit = TRUE;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status =_SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      ERR("Failed reading Client Pfns from user space.\n");
      SetLastNtError(Status);
   }

   UserLeave();
   return Status;
}

DWORD
APIENTRY
NtUserInitTask(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5,
   DWORD Unknown6,
   DWORD Unknown7,
   DWORD Unknown8,
   DWORD Unknown9,
   DWORD Unknown10,
   DWORD Unknown11)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserMNDragLeave(VOID)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserMNDragOver(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserModifyUserStartupInfoFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserQueryUserCounters(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserRegisterTasklist(
   DWORD Unknown0)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserSetConsoleReserveKeys(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserSetDbgTag(
   DWORD Unknown0,
   DWORD Unknown1)
{
   STUB;

   return 0;
}

DWORD
APIENTRY
NtUserSetDbgTagCount(
    DWORD Unknown0)
{
    STUB;

    return 0;
}

DWORD
APIENTRY
NtUserSetRipFlags(
   DWORD Unknown0)
{
   STUB;

   return 0;
}

DWORD
APIENTRY
NtUserDbgWin32HeapFail(
    DWORD Unknown0,
    DWORD Unknown1)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserDbgWin32HeapStat(
    DWORD Unknown0,
    DWORD Unknown1)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserSetSysColors(
   int cElements,
   IN CONST INT *lpaElements,
   IN CONST COLORREF *lpaRgbValues,
   FLONG Flags)
{
   DWORD Ret = TRUE;

   if (cElements == 0)
      return TRUE;

   /* We need this check to prevent overflow later */
   if ((ULONG)cElements >= 0x40000000)
   {
      EngSetLastError(ERROR_NOACCESS);
      return FALSE;
   }

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead(lpaElements, cElements * sizeof(INT), 1);
      ProbeForRead(lpaRgbValues, cElements * sizeof(COLORREF), 1);

      IntSetSysColors(cElements, lpaElements, lpaRgbValues);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      Ret = FALSE;
   }
   _SEH2_END;

   if (Ret)
   {
      UserSendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

      UserRedrawDesktop();
   }

   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserUpdateInputContext(
    HIMC hIMC,
    DWORD Unknown1,
    LPVOID pClientImc)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserUpdateInstance(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserUserHandleGrantAccess(
   IN HANDLE hUserHandle,
   IN HANDLE hJob,
   IN BOOL bGrant)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserWaitForMsgAndEvent(
   DWORD Unknown0)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserWin32PoolAllocationStats(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserYieldTask(VOID)
{
   STUB

   return 0;
}

HIMC
APIENTRY
NtUserCreateInputContext(PCLIENTIMC pClientImc)
{
    STUB;
    return NULL;
}

BOOL
APIENTRY
NtUserDestroyInputContext(HIMC hIMC)
{
    STUB;
    return FALSE;
}

DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

BOOL
NTAPI
NtUserNotifyProcessCreate(
    HANDLE NewProcessId,
    HANDLE ParentThreadId,
    ULONG  dwUnknown,
    ULONG  CreateFlags)
{
    // STUB;
    TRACE("NtUserNotifyProcessCreate is UNIMPLEMENTED\n");
    return FALSE;
}

NTSTATUS
APIENTRY
NtUserProcessConnect(
    IN  HANDLE ProcessHandle,
    OUT PUSERCONNECT pUserConnect,
    IN  ULONG Size)
{
    NTSTATUS Status;
    PEPROCESS Process = NULL;
    PPROCESSINFO W32Process;

    TRACE("NtUserProcessConnect\n");

    if (pUserConnect == NULL ||
        Size != sizeof(*pUserConnect))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the process object the user handle was referencing */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       *PsProcessType,
                                       UserMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    UserEnterShared();

    /* Get Win32 process information */
    W32Process = PsGetProcessWin32Process(Process);

    _SEH2_TRY
    {
        UINT i;

        // FIXME: Check that pUserConnect->ulVersion == USER_VERSION;
        // FIXME: Check the value of pUserConnect->dwDispatchCount.

        ProbeForWrite(pUserConnect, sizeof(*pUserConnect), sizeof(PVOID));

        // FIXME: Instead of assuming that the mapping of the heap desktop
        // also holds there, we **MUST** create and map instead the shared
        // section! Its client base must be stored in W32Process->pClientBase.
        // What is currently done (ReactOS-specific only), is that within the
        // IntUserHeapCommitRoutine()/MapGlobalUserHeap() routines we assume
        // it's going to be also called early, so that we manually add a very
        // first memory mapping that corresponds to the "global user heap",
        // and that we use instead of a actual win32 "shared USER section"
        // (see slide 29 of https://paper.bobylive.com/Meeting_Papers/BlackHat/USA-2011/BH_US_11_Mandt_win32k_Slides.pdf )

        pUserConnect->siClient.ulSharedDelta =
            (ULONG_PTR)W32Process->HeapMappings.KernelMapping -
            (ULONG_PTR)W32Process->HeapMappings.UserMapping;

#define SERVER_TO_CLIENT(ptr) \
    ((PVOID)((ULONG_PTR)ptr - pUserConnect->siClient.ulSharedDelta))

        ASSERT(gpsi);
        ASSERT(gHandleTable);

        pUserConnect->siClient.psi       = SERVER_TO_CLIENT(gpsi);
        pUserConnect->siClient.aheList   = SERVER_TO_CLIENT(gHandleTable);
        pUserConnect->siClient.pDispInfo = NULL;

        // NOTE: kernel server should also have a SHAREDINFO gSharedInfo;
        // FIXME: These USER window-proc data should be used somehow!

        pUserConnect->siClient.DefWindowMsgs.maxMsgs     = 0;
        pUserConnect->siClient.DefWindowMsgs.abMsgs      = NULL;
        pUserConnect->siClient.DefWindowSpecMsgs.maxMsgs = 0;
        pUserConnect->siClient.DefWindowSpecMsgs.abMsgs  = NULL;

        for (i = 0; i < ARRAYSIZE(pUserConnect->siClient.awmControl); ++i)
        {
            pUserConnect->siClient.awmControl[i].maxMsgs = 0;
            pUserConnect->siClient.awmControl[i].abMsgs  = NULL;
        }
#undef SERVER_TO_CLIENT
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
        SetLastNtError(Status);

    UserLeave();

    /* Dereference the process object */
    ObDereferenceObject(Process);

    return Status;
}

NTSTATUS
APIENTRY
NtUserQueryInformationThread(IN HANDLE ThreadHandle,
                             IN USERTHREADINFOCLASS ThreadInformationClass,
                             OUT PVOID ThreadInformation,
                             IN ULONG ThreadInformationLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;

    /* Allow only CSRSS to perform this operation */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    /* Get the Thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_QUERY_INFORMATION,
                                       *PsThreadType,
                                       UserMode,
                                       (PVOID)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Quit;

    switch (ThreadInformationClass)
    {
        default:
        {
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    ObDereferenceObject(Thread);

Quit:
    UserLeave();
    return Status;
}

DWORD
APIENTRY
NtUserQueryInputContext(
    HIMC hIMC,
    DWORD dwUnknown2)
{
    TRACE("NtUserQueryInputContext(%p, 0x%lX)\n", hIMC, dwUnknown2);
    return 0;
}

BOOL
APIENTRY
NtUserRealInternalGetMessage(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bGMSG)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserRealWaitMessageEx(
    DWORD dwWakeMask,
    UINT uTimeout)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    STUB;
    return 0;
}

DWORD APIENTRY
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
    STUB
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP DesktopObject);

NTSTATUS
APIENTRY
NtUserSetInformationThread(IN HANDLE ThreadHandle,
                           IN USERTHREADINFOCLASS ThreadInformationClass,
                           IN PVOID ThreadInformation,
                           IN ULONG ThreadInformationLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;

    /* Allow only CSRSS to perform this operation */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    /* Get the Thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_INFORMATION,
                                       *PsThreadType,
                                       UserMode,
                                       (PVOID)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Quit;

    switch (ThreadInformationClass)
    {
        case UserThreadInitiateShutdown:
        {
            ULONG CapturedFlags = 0;

            TRACE("Shutdown initiated\n");

            if (ThreadInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForWrite(ThreadInformation, sizeof(CapturedFlags), sizeof(PVOID));
                CapturedFlags = *(PULONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = UserInitiateShutdown(Thread, &CapturedFlags);

            /* Return the modified value to the caller */
            _SEH2_TRY
            {
                *(PULONG)ThreadInformation = CapturedFlags;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case UserThreadEndShutdown:
        {
            NTSTATUS ShutdownStatus;

            TRACE("Shutdown ended\n");

            if (ThreadInformationLength != sizeof(ShutdownStatus))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForRead(ThreadInformation, sizeof(ShutdownStatus), sizeof(PVOID));
                ShutdownStatus = *(NTSTATUS*)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = UserEndShutdown(Thread, ShutdownStatus);
            break;
        }

        case UserThreadCsrApiPort:
        {
            HANDLE CsrPortHandle;


            TRACE("Set CSR API Port for Win32k\n");
            if (ThreadInformationLength != sizeof(CsrPortHandle))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForRead(ThreadInformation, sizeof(CsrPortHandle), sizeof(PVOID));
                CsrPortHandle = *(PHANDLE)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = InitCsrApiPort(CsrPortHandle);
            break;
        }

        case UserThreadUseActiveDesktop:
        {
            HDESK hdesk;

            if (Thread != PsGetCurrentThread())
            {
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }

            hdesk = IntGetDesktopObjectHandle(gpdeskInputDesktop);
            IntSetThreadDesktop(hdesk, FALSE);

            break;
        }
        case UserThreadRestoreDesktop:
        {
            if (Thread != PsGetCurrentThread())
            {
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }

            IntSetThreadDesktop(NULL, FALSE);
            break;
        }
        default:
        {
            STUB;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    ObDereferenceObject(Thread);

Quit:
    UserLeave();
    return Status;
}

DWORD
APIENTRY
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserSoundSentry(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserTestForInteractiveUser(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawScreen(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteStopScreenUpdates(VOID)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserLockWindowUpdate(HWND hWnd)
{
    STUB;
    return FALSE;
}

DWORD APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
    STUB;

    return 0;
}

BOOL APIENTRY NtUserAddClipboardFormatListener(
    HWND hwnd
)
{
    STUB;
    return FALSE;
}

BOOL APIENTRY NtUserRemoveClipboardFormatListener(
    HWND hwnd
)
{
    STUB;
    return FALSE;
}

BOOL APIENTRY NtUserGetUpdatedClipboardFormats(
    PUINT lpuiFormats,
    UINT cFormats,
    PUINT pcFormatsOut
)
{
    STUB;
    return FALSE;
}

// Yes, I know, these do not belong here, just tell me where to put them
BOOL
APIENTRY
NtGdiMakeObjectXferable(
    _In_ HANDLE hHandle,
    _In_ DWORD dwProcessId)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtDxEngGetRedirectionBitmap(
    DWORD Unknown0)
{
    STUB;
    return 0;
}


/* EOF */
