/*
 * PROJECT:     ReactOS Dxg
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Implements memory and context management for Native DirectDraw & DX7
 * COPYRIGHT:   Copyright 2023 Joshua Perry <ill386dev@gmail.com>
 */

#include <dxg_int.h>

#include <pseh/pseh.h>
#include <pseh/pseh2.h>

#include <debug.h>

#define TRACE() DPRINT1("DXG: %s\n", __FUNCTION__)

DWORD gWarningLevel = 0;

// Implemented
VOID NTAPI
DoDxgAssert(PCSTR Format)
{
    if (gWarningLevel >= 0)
    {
        DPRINT1("DXG Assertion: ");
        DPRINT1("%s", Format);
        DPRINT1("\n");
        DbgBreakPoint();
    }
}

// Implemented
VOID NTAPI
DoWarning(PCSTR Format, DWORD dwLevel)
{
    if (dwLevel <= gWarningLevel)
    {
        DPRINT1("DXG: ");
        DPRINT1("%s", Format);
        DPRINT1("\n");
    }
}

// Implemented
VOID NTAPI
vDdAssertDevlock(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    if (!gpEngFuncs.DxEngIsHdevLockedByCurrentThread(peDdGl->hDev))
        DoDxgAssert("DD_ASSERTDEVLOCK failed because Devlock is not held");
}

// Implemented
VOID NTAPI
vDdIncrementReferenceCount(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    TRACE();

    vDdAssertDevlock(peDdGl);

    if (++peDdGl->cDriverReferences == 1)
        gpEngFuncs.DxEngReferenceHdev(peDdGl->hDev);
}

// Implemented
VOID NTAPI
ProbeAndReadBuffer(PVOID Dst, PVOID Src, SIZE_T MaxCount)
{
    ULONG64 EndAddress = (ULONG64)Src + MaxCount;

    if (EndAddress < (ULONG64)Src || EndAddress > DxgUserProbeAddress)
        *(BYTE *)Dst = *(BYTE *)DxgUserProbeAddress;

    RtlCopyMemory(Dst, Src, MaxCount);
}

// Implemented
VOID NTAPI
ProbeAndWriteBuffer(PVOID Dst, PVOID Src, SIZE_T MaxCount)
{
    ULONG64 EndAddress = (ULONG64)Dst + MaxCount;

    if (EndAddress < (ULONG64)Dst || EndAddress > DxgUserProbeAddress)
        DxgUserProbeAddress = 0;

    RtlCopyMemory(Dst, Src, MaxCount);
}

// Implemented
BOOL NTAPI
D3dSetup(EDD_DIRECTDRAW_GLOBAL *peDdGl, PKFLOATING_SAVE FloatSave)
{
    TRACE();

    BOOL result = FALSE;

    if (!peDdGl)
        DoDxgAssert("D3dSetup on NULL global\n");

    if (KeSaveFloatingPointState(FloatSave) >= 0)
    {
        gpEngFuncs.DxEngLockHdev(peDdGl->hDev);
        result = TRUE;
    }
    else
    {
        DoWarning("D3dSetup: Unable to save FP state\n", 0);
        result = FALSE;
    }

    return result;
}

// Implemented
VOID NTAPI
D3dCleanup(EDD_DIRECTDRAW_GLOBAL *peDdGl, PKFLOATING_SAVE FloatSave)
{
    TRACE();

    gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
    KeRestoreFloatingPointState(FloatSave);
}

// Implemented
PEDD_CONTEXT
NTAPI
D3dLockContext(PKFLOATING_SAVE pFloatSave, ULONG_PTR *dwhContext)
{
    PEDD_CONTEXT peDdCtx = (PEDD_CONTEXT)DdHmgLock(dwhContext, ObjType_DDCONTEXT_TYPE, TRUE);

    DdHmgReleaseHmgrSemaphore();

    if (!peDdCtx)
    {
        DoWarning("D3dLockContext unable to lock context", 0);
        return NULL;
    }

    gpEngFuncs.DxEngLockShareSem();

    if (peDdCtx->bTexture)
    {
        DoWarning("D3dLockContext: Valid handle not a context", 0);
        InterlockedDecrement((VOID *)&peDdCtx->pobj.cExclusiveLock);
        gpEngFuncs.DxEngUnlockShareSem();

        return NULL;
    }

    PEDD_DIRECTDRAW_GLOBAL pDdGl = peDdCtx->peDirectDrawGlobal;

    if (!pDdGl)
    {
        DoWarning("D3dLockContext: Call on disabled object", 0);
        InterlockedDecrement((VOID *)&peDdCtx->pobj.cExclusiveLock);
        gpEngFuncs.DxEngUnlockShareSem();

        return NULL;
    }

    if (!pDdGl->ddMiscellanous2Callbacks.CreateSurfaceEx)
        DoDxgAssert("D3dLockContext: No CreateSurfaceEx callback");

    if (!D3dSetup(pDdGl, pFloatSave))
    {
        InterlockedDecrement((VOID *)&peDdCtx->pobj.cExclusiveLock);
        gpEngFuncs.DxEngUnlockShareSem();

        return NULL;
    }

    *dwhContext = peDdCtx->dwhContext;

    return peDdCtx;
}

// Implemented
VOID NTAPI
D3dUnlockContext(EDD_CONTEXT *peDdCtx, PKFLOATING_SAVE pFloatSave)
{
    D3dCleanup(peDdCtx->peDirectDrawGlobal, pFloatSave);
    gpEngFuncs.DxEngUnlockShareSem();
    InterlockedDecrement((VOID *)&peDdCtx->pobj.cExclusiveLock);
}

// Implemented
BOOL NTAPI
D3dLockSurfaces(DWORD dwCount, D3D_SURFACELOCK *pD3dSurf)
{
    TRACE();

    // No surfaces to lock
    if (dwCount <= 0)
        return TRUE;

    for (DWORD i = 0; i < dwCount; i++)
    {
        // If the surface is null but not mandatory go next
        if (pD3dSurf->peDdSurf == NULL)
        {
            if (!pD3dSurf->bOptional)
            {
                DoWarning("D3dLockSurfaces: NULL for mandatory surface", 0);
                return FALSE;
            }
            else
                continue;
        }

        PEDD_SURFACE peDdSurf = (PEDD_SURFACE)DdHmgLock(pD3dSurf->peDdSurf, ObjType_DDSURFACE_TYPE, TRUE);

        if (!peDdSurf)
        {
            DoWarning("D3dLockSurfaces unable to lock buffer\n", 0);
            return FALSE;
        }

        if (peDdSurf->bLost)
        {
            DoWarning("D3DLockSurfaces unable to lock buffer Surface is Lost", 0);
            return FALSE;
        }

        pD3dSurf->peDdLocked = peDdSurf;
        pD3dSurf->peDdLocal = &peDdSurf->ddsSurfaceLocal;

        pD3dSurf++;
    }

    return TRUE;
}

// Implemented
DWORD
NTAPI
DxD3dContextCreate(
    EDD_DIRECTDRAW_GLOBAL *peDdGlobal,
    EDD_SURFACE *peDdSTarget,
    EDD_SURFACE *peDdSZ,
    PD3D_CREATECONTEXT_INT pDdCreateData)
{
    TRACE();

    DWORD result = 0;
    HANDLE ContextObject;

    NTSTATUS Status;
    KFLOATING_SAVE FloatSave;

    PEDD_CONTEXT ContextHandle = NULL;
    HANDLE SecureHandle = NULL;
    PVOID BaseAddress = NULL;

    D3D_SURFACELOCK d3dSurfaces[2] = {0};

    // Ensure the RegionSize is 4 Byte Aligned
    if (((ULONG64)pDdCreateData & 3) != 0)
        ExRaiseDatatypeMisalignment();

    // Make sure we are not below or at the current ProbeAddress
    if (DxgUserProbeAddress <= (ULONG64)pDdCreateData)
    {
        DxgUserProbeAddress = 0;
    }

    D3DNTHAL_CONTEXTCREATEDATA ContextCreateData = {0};

    // RegionSize coming in is a CreateContext Struct
    PD3D_CREATECONTEXT_INT InitCreateData = (PD3D_CREATECONTEXT_INT)pDdCreateData;

    // Are we truncating the least signficant bits?
    // LOBYTE(InitCreateData->unk_000) = InitCreateData->unk_000;
    // LOBYTE(InitCreateData->RegionSize) = InitCreateData->RegionSize;

    ContextCreateData.dwPID = InitCreateData->dwPID;
    ContextCreateData.dwhContext = InitCreateData->dwhContext;
    ContextCreateData.ddrval = InitCreateData->ddrval;

    DWORD dwPreviousContext = InitCreateData->dwhContext;

    DWORD Alignment = 0;

    // We change this to the request size after we fill out some existing info
    DWORD RegionSize = InitCreateData->dwRegionSize;

    if (RegionSize)
    {
        if (RegionSize - 0x4000 > 0xFC000)
        {
            DoWarning("DxD3dContextCreate: illegal prim buffer size", 0);
            return 0;
        }
    }
    else
    {
        RegionSize = 0x10000;
    }

    PEDD_DIRECTDRAW_LOCAL peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(peDdGlobal, ObjType_DDLOCAL_TYPE, FALSE);

    if (!peDdL)
    {
        return 0;
    }

    PEDD_DIRECTDRAW_GLOBAL peDdGl = peDdL->peDirectDrawGlobal2;

    d3dSurfaces[0].peDdSurf = peDdSTarget;
    d3dSurfaces[0].bOptional = FALSE;
    d3dSurfaces[1].peDdSurf = peDdSZ;
    d3dSurfaces[1].bOptional = TRUE;

    DdHmgAcquireHmgrSemaphore();

    if (D3dLockSurfaces(2, &d3dSurfaces[0]))
    {
        DdHmgReleaseHmgrSemaphore();

        ContextHandle = DdHmgAlloc(0x140u, ObjType_DDCONTEXT_TYPE, TRUE);

        if (ContextHandle)
        {
            ContextObject = ContextHandle;

            Status = ZwAllocateVirtualMemory(
                NtCurrentProcess(), &BaseAddress, 0, &RegionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

            if (NT_SUCCESS(Status))
            {
                SecureHandle = MmSecureVirtualMemory(BaseAddress, RegionSize, PAGE_READWRITE);

                if (SecureHandle)
                {
                    if (D3dSetup(peDdGl, &FloatSave))
                    {
                        if (peDdGl->ddMiscellanous2Callbacks.CreateSurfaceEx)
                        {
                            if (peDdGl->d3dNtHalCallbacks.ContextCreate)
                            {
                                ContextCreateData.lpDDGbl =
                                    peDdL != NULL ? (PDD_DIRECTDRAW_GLOBAL)peDdL->peDirectDrawGlobal : NULL;
                                ContextCreateData.lpDDS = d3dSurfaces[0].peDdLocal;
                                ContextCreateData.lpDDSZ = d3dSurfaces[1].peDdLocal;

                                result = peDdGl->d3dNtHalCallbacks.ContextCreate(&ContextCreateData);
                            }
                            else
                                DoWarning("DxD3dContextCreate: ContextCreate callback not found", 0);

                            if (result && !ContextCreateData.ddrval)
                            {
                                // Assign ContextHandle values here
                                ContextHandle->bTexture = FALSE;
                                ContextHandle->dwhContext = ContextCreateData.dwhContext;
                                ContextHandle->peDirectDrawGlobal = peDdGl;
                                ContextHandle->peDirectDrawLocal = peDdL;
                                ContextHandle->peSurface_Target = peDdSTarget;
                                ContextHandle->peSurface_Z = peDdSZ;
                                ContextHandle->hSecure = SecureHandle;
                                ContextHandle->BaseAddress = BaseAddress;

                                // Account for rounding to nearest page size
                                Alignment = ((DWORD)BaseAddress + 31) & 0xFFFFFFE0;

                                ContextHandle->dwAlignment = Alignment;
                                ContextHandle->dwRegionSize = RegionSize;

                                // The actual size of our Region not including rounding for page size
                                RegionSize = ((DWORD)BaseAddress + (DWORD)RegionSize - Alignment);

                                // Store a previous associated dwhContext
                                ContextHandle->dwhContextPrev = dwPreviousContext;

                                // We zero these out so they do not get disposed of
                                SecureHandle = NULL;
                                BaseAddress = NULL;
                                ContextObject = NULL;

                                // This might be the Hmgr object or the context handle
                                ContextCreateData.dwhContext = (ULONG_PTR)ContextHandle;

                                vDdIncrementReferenceCount(peDdGl);
                            }

                            InterlockedDecrement((VOID *)&ContextHandle->pobj.cExclusiveLock);
                            D3dCleanup(peDdGl, &FloatSave);

                            InitCreateData->dwhContext = ContextCreateData.dwhContext;
                            InitCreateData->ddrval = ContextCreateData.ddrval;
                            InitCreateData->dwAlignment = Alignment;
                            InitCreateData->dwRegionSize = RegionSize;
                        }
                        else
                        {
                            D3dCleanup(peDdGl, &FloatSave);
                        }
                    }
                }
            }
        }
        else
        {
            DoWarning("DxD3dContextCreate unable to alloc handle", 0);
        }
    }
    else
    {
        DdHmgReleaseHmgrSemaphore();
    }

    // Dereference any surfaces that were locked
    for (DWORD i = 0; i < 2; i++)
        if (d3dSurfaces[i].peDdLocked)
            InterlockedDecrement((VOID *)&d3dSurfaces[i].peDdSurf->pobj.cExclusiveLock);

    if (SecureHandle)
        MmUnsecureVirtualMemory(SecureHandle);

    if (BaseAddress)
    {
        RegionSize = 0;
        ZwFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &RegionSize, MEM_RELEASE);
    }

    if (ContextObject)
    {
        DdHmgFree(ContextObject);
    }

    if (peDdL)
    {
        InterlockedDecrement((VOID *)&peDdL->pobj.cExclusiveLock);
    }

    return result;
}

// Implemented
DWORD
NTAPI
DxD3dContextDestroy(LPD3DNTHAL_CONTEXTDESTROYDATA lpD3dHalCDD)
{
    TRACE();

    DWORD result = 0;
    DWORD ddrval = E_FAIL;

    _SEH2_TRY
    {
        ProbeForWrite(lpD3dHalCDD, sizeof(D3DNTHAL_CONTEXTDESTROYDATA), 4u);

        result = D3dDeleteHandle((DD_BASEOBJECT *)lpD3dHalCDD->dwhContext, 0, NULL, &ddrval);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Prevented Crash in DxD3dContextDestroy lpD3dHalCDD was out of reach.");
    }
    _SEH2_END;

    // Set this either way
    lpD3dHalCDD->ddrval = ddrval;

    return result;
}

// Implemented?
DWORD
NTAPI
DxD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA lpD3dHalCDAD)
{
    TRACE();

    if ((ULONG_PTR)lpD3dHalCDAD >= DxgUserProbeAddress)
        DxgUserProbeAddress = 0;

    // WHAT ON EARTH
    //*(_BYTE *)a1 = *(_BYTE *)a1;
    //*(_BYTE *)(a1 + 4) = *(_BYTE *)(a1 + 4);

    lpD3dHalCDAD->ddrval = E_FAIL;

    return 0;
}

// Implemented
DWORD
NTAPI
DxD3dValidateTextureStageState(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA lpD3dHaVts)
{
    TRACE();

    DWORD result = 0;

    KFLOATING_SAVE FloatSave;
    D3DNTHAL_VALIDATETEXTURESTAGESTATEDATA d3dHalVtsd = {0};

    ProbeForWrite(lpD3dHaVts, sizeof(D3DNTHAL_VALIDATETEXTURESTAGESTATEDATA), 4u);
    RtlCopyMemory(&d3dHalVtsd, lpD3dHaVts, sizeof(D3DNTHAL_VALIDATETEXTURESTAGESTATEDATA));

    DdHmgAcquireHmgrSemaphore();

    PEDD_CONTEXT peDdCtx = D3dLockContext(&FloatSave, &d3dHalVtsd.dwhContext);

    if (!peDdCtx)
        return 0;

    PEDD_DIRECTDRAW_GLOBAL peDdGl = peDdCtx->peDirectDrawGlobal;

    if (peDdGl->bSuspended)
    {
        d3dHalVtsd.ddrval = DDERR_SURFACELOST;
    }
    else
    {
        if (peDdGl->d3dNtHalCallbacks3.ValidateTextureStageState)
        {
            result = peDdGl->d3dNtHalCallbacks3.ValidateTextureStageState(&d3dHalVtsd);
        }
        else
            DoWarning("DxD3dValidateTextureStageState call not present!", 0);
    }

    D3dUnlockContext(peDdCtx, &FloatSave);
    lpD3dHaVts->ddrval = d3dHalVtsd.ddrval;
    lpD3dHaVts->dwNumPasses = d3dHalVtsd.dwNumPasses;

    return result;
}

// We do not have these in ReactOS SDK?
#define D3DNTHALDP2_SWAPVERTEXBUFFER 0x00000004L
#define D3DNTHALDP2_SWAPCOMMANDBUFFER 0x00000008L

// FIXME: Implement Swap
DWORD
NTAPI
DxD3dDrawPrimitives2(
    EDD_SURFACE *peDdS,
    EDD_SURFACE *peDdS2,
    LPD3DNTHAL_DRAWPRIMITIVES2DATA lpD3dHalDP2D,
    VIDEOMEMORY **ppvMemory,
    PVOID p5,
    PVOID p6,
    PVOID p7)
{
    TRACE();

    D3DNTHAL_DRAWPRIMITIVES2DATA d3dHalDP2D;

    ULONG64 OldProbe = DxgUserProbeAddress;

    UNREFERENCED_PARAMETER(OldProbe);

    D3D_SURFACELOCK ddSurfLocks[4] = {0};

    LPD3DNTHAL_DRAWPRIMITIVES2DATA reflpD3dHalDP2D = lpD3dHalDP2D;

    DWORD result = 0;
    PEDD_CONTEXT peDdCtx = NULL;

    KFLOATING_SAVE FloatSave;
    HANDLE SecureHandle = NULL;

    if ((ULONG64)lpD3dHalDP2D >= DxgUserProbeAddress)
        reflpD3dHalDP2D = (LPD3DNTHAL_DRAWPRIMITIVES2DATA)DxgUserProbeAddress;

    RtlCopyMemory(&d3dHalDP2D, reflpD3dHalDP2D, sizeof(d3dHalDP2D));

    for (int i = 0; i < 4; i++)
        ddSurfLocks[i].peDdLocked = NULL;

    DWORD dwVertexDataSize = d3dHalDP2D.dwVertexLength * d3dHalDP2D.dwVertexSize;

    if ((d3dHalDP2D.dwFlags & 1) == 0)
    {
        ddSurfLocks[0].peDdSurf = peDdS;
        ddSurfLocks[0].bOptional = FALSE;
        ddSurfLocks[1].peDdSurf = peDdS2;
        ddSurfLocks[1].bOptional = FALSE;

        DdHmgAcquireHmgrSemaphore();

        if (D3dLockSurfaces(2, &ddSurfLocks[0]))
        {
            if (ddSurfLocks[1].peDdLocked)
            {
                d3dHalDP2D.lpDDCommands = ddSurfLocks[0].peDdLocal;
                d3dHalDP2D.lpDDVertex = ddSurfLocks[1].peDdLocal;

                // Check to make sure we have a big enough surface
                if (d3dHalDP2D.dwVertexSize)
                {
                    DWORD SurfaceSize = ddSurfLocks[1].peDdLocked->ddsSurfaceGlobal.dwLinearSize;

                    if (dwVertexDataSize > SurfaceSize)
                    {
                        DoWarning("DxD3dDrawPrimitive2 d3d.dwVertexLength is bigger than surface!", 0);
                        d3dHalDP2D.dwVertexLength = SurfaceSize / d3dHalDP2D.dwVertexSize;
                    }
                }
            }

            peDdCtx = D3dLockContext(&FloatSave, &d3dHalDP2D.dwhContext);

            if (peDdCtx && (DWORD)d3dHalDP2D.lpdwRStates == peDdCtx->dwAlignment)
            {
                PEDD_DIRECTDRAW_GLOBAL peDdGl = peDdCtx->peDirectDrawGlobal;

                if (!peDdGl->bSuspended)
                {
                    if (peDdCtx->peDirectDrawLocal->hRefCount == peDdCtx->peDirectDrawLocal->cActiveSurface ||
                        peDdCtx->unk_13C < 4u)
                    {
                        if (peDdGl->d3dNtHalCallbacks3.DrawPrimitives2)
                        {
                            result = peDdGl->d3dNtHalCallbacks3.DrawPrimitives2(&d3dHalDP2D);
                        }
                        else
                        {
                            DoWarning("D3dDrawPrimitives2: DrawPrimitives2 calback absent!", 0);
                        }

                        if (result == 1 && !d3dHalDP2D.ddrval)
                        {
                            if ((d3dHalDP2D.dwFlags & D3DNTHALDP2_SWAPCOMMANDBUFFER) != 0)
                            {
                                VIDEOMEMORY **ppvTemp = NULL;

                                if ((ULONG64)ppvMemory >= DxgUserProbeAddress)
                                    ppvTemp = (VIDEOMEMORY **)DxgUserProbeAddress;

                                *ppvTemp = (VIDEOMEMORY *)d3dHalDP2D.lpDDCommands->lpGbl->fpVidMem;
                            }
                            if ((d3dHalDP2D.dwFlags & D3DNTHALDP2_SWAPVERTEXBUFFER) != 0)
                            {
                                // Implement Swap
                            }
                        }

                        lpD3dHalDP2D->dwErrorOffset = d3dHalDP2D.dwErrorOffset;
                        lpD3dHalDP2D->dwVertexSize = d3dHalDP2D.dwVertexSize;
                    }
                }
            }
            else
            {
                result = TRUE;
                d3dHalDP2D.ddrval = DDERR_SURFACELOST;
            }
        }
        else
        {
            DdHmgReleaseHmgrSemaphore();
        }
    }

    if (peDdCtx)
        D3dUnlockContext(peDdCtx, &FloatSave);

    for (int i = 0; i < 4; i++)
    {
        PEDD_SURFACE locked = ddSurfLocks[i].peDdLocked;

        if (locked)
            InterlockedDecrement((PVOID)&locked->pobj.cExclusiveLock);
    }

    if (SecureHandle)
        MmUnsecureVirtualMemory(SecureHandle);

    return result;
}

// Implemented
DWORD
NTAPI
DxDdGetDriverState(DD_GETDRIVERSTATEDATA *pDdHalGDSD)
{
    TRACE();

    KFLOATING_SAVE FloatSave;

    DWORD result = 0;
    NTSTATUS Status = 0;

    DD_GETDRIVERSTATEDATA ddhalGDSD = {0};

    _SEH2_TRY
    {
        ProbeForWrite((PVOID)pDdHalGDSD, sizeof(DD_GETDRIVERSTATEDATA), 4u);
        RtlCopyMemory(&ddhalGDSD, pDdHalGDSD, sizeof(DD_GETDRIVERSTATEDATA));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    if (!ddhalGDSD.lpdwStates)
    {
        DoWarning("DxDdGetDriverState passed null lpdwStates", 0);
        return 0;
    }

    PVOID pStateMem = EngAllocMem(1u, ddhalGDSD.dwLength, TAG_GDDP);

    if (!pStateMem)
        return 0;

    ProbeAndReadBuffer(pStateMem, pDdHalGDSD->lpdwStates, pDdHalGDSD->dwLength);
    ddhalGDSD.lpdwStates = (LPDWORD)pStateMem;

    DdHmgAcquireHmgrSemaphore();

    PEDD_CONTEXT peDdCtx = D3dLockContext(&FloatSave, &ddhalGDSD.dwhContext);

    if (peDdCtx)
    {
        PEDD_DIRECTDRAW_GLOBAL peDdGl = peDdCtx->peDirectDrawGlobal;

        if (!peDdGl->ddMiscellanous2Callbacks.GetDriverState)
            DoDxgAssert("DxDdGetDriverState is not present. It is not an optional callback\n");

        if (peDdGl->bSuspended)
        {
            ddhalGDSD.ddRVal = E_FAIL;
        }
        else
        {
            if (peDdGl->ddMiscellanous2Callbacks.GetDriverState)
                result = peDdGl->ddMiscellanous2Callbacks.GetDriverState(&ddhalGDSD);
            else
                DoWarning("DxD3dDrawPrimitives2: GetDriverState callback absent!", 0);
        }

        D3dUnlockContext(peDdCtx, &FloatSave);
        pDdHalGDSD->ddRVal = ddhalGDSD.ddRVal;
        ProbeAndWriteBuffer(ddhalGDSD.lpdwStates, pStateMem, ddhalGDSD.dwLength);
    }

    EngFreeMem(pStateMem);

    return result;
}

// Implemented
BOOL NTAPI
DxDdAddAttachedSurface(HANDLE hSurfaceTarget, HANDLE hSurfaceSrc, PDD_ADDATTACHEDSURFACEDATA pDdAttachedSurfaceData)
{
    TRACE();

    NTSTATUS Status = 0;
    BOOL bSUCCESS = DD_OK;

    PEDD_SURFACE peDdSTarget = NULL;
    PEDD_SURFACE peDdSSrc = NULL;

    DD_ADDATTACHEDSURFACEDATA addSurfaceData = {0};

    _SEH2_TRY
    {
        PDD_ADDATTACHEDSURFACEDATA ProbeAddress = (PDD_ADDATTACHEDSURFACEDATA)DxgUserProbeAddress;

        if ((ULONG64)pDdAttachedSurfaceData < DxgUserProbeAddress)
            ProbeAddress = pDdAttachedSurfaceData;

        RtlCopyMemory(&addSurfaceData, ProbeAddress, sizeof(DD_ADDATTACHEDSURFACEDATA));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        return DD_FALSE;
    }

    peDdSTarget = (PEDD_SURFACE)DdHmgLock(hSurfaceTarget, ObjType_DDSURFACE_TYPE, FALSE);
    peDdSSrc = (PEDD_SURFACE)DdHmgLock(hSurfaceSrc, ObjType_DDSURFACE_TYPE, FALSE);

    addSurfaceData.ddRVal = DDERR_GENERIC;

    if (peDdSTarget && peDdSSrc && (peDdSTarget->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == 0 &&
        (peDdSSrc->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == 0 &&
        peDdSTarget->peDirectDrawLocal == peDdSSrc->peDirectDrawLocal)
    {
        // Can we attach surfaces?
        if (peDdSTarget->peDirectDrawGlobalNext->ddSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE)
        {
            // Fill out attach surface data
            addSurfaceData.lpDD = (PDD_DIRECTDRAW_GLOBAL)peDdSTarget->peDirectDrawGlobalNext;
            addSurfaceData.lpDDSurface = &peDdSTarget->ddsSurfaceLocal;
            addSurfaceData.lpSurfAttached = &peDdSSrc->ddsSurfaceLocal;

            // Lock the surface hDev?
            gpEngFuncs.DxEngLockHdev(peDdSTarget->peDirectDrawGlobal->hDev);

            // Fail if either surface is marked lost
            if (peDdSTarget->bLost || peDdSSrc->bLost)
            {
                bSUCCESS = DD_FALSE;
                addSurfaceData.ddRVal = DDERR_SURFACELOST;
            }
            else
            {
                // attach surface here return the result
                bSUCCESS = peDdSTarget->peDirectDrawGlobalNext->ddSurfaceCallbacks.AddAttachedSurface(&addSurfaceData);
            }

            gpEngFuncs.DxEngUnlockHdev(peDdSTarget->peDirectDrawGlobal->hDev);
        }
    }
    else
    {
        DoWarning("DxDdAddAttachedSurface: Invalid surface or dwFlags\n", 0);
    }

    // We do this because AddAttachedSurface modifies this value
    pDdAttachedSurfaceData->ddRVal = addSurfaceData.ddRVal;

    if (peDdSTarget)
        InterlockedDecrement((VOID *)&peDdSTarget->pobj.cExclusiveLock);

    if (peDdSSrc)
        InterlockedDecrement((VOID *)&peDdSSrc->pobj.cExclusiveLock);

    return bSUCCESS;
}

DWORD
NTAPI
DxDdAlphaBlt(PEDD_SURFACE peDdSDest, PEDD_SURFACE peDdSSrc, DD_BLTDATA *pDdBltData)
{
    TRACE();

    NTSTATUS Status = 0;
    DWORD result = 0;
    HRESULT ddRVal = E_FAIL;

    DD_BLTDATA DdBltData = {0};

    _SEH2_TRY
    {
        PDD_BLTDATA ProbeAddress = (PDD_BLTDATA)DxgUserProbeAddress;

        if ((unsigned int)pDdBltData < DxgUserProbeAddress)
            ProbeAddress = pDdBltData;

        RtlCopyMemory(&DdBltData, ProbeAddress, sizeof(DD_BLTDATA));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    PEDD_SURFACE pDstLocked = (PEDD_SURFACE)DdHmgLock(peDdSDest, ObjType_DDSURFACE_TYPE, FALSE);
    PEDD_SURFACE pSrcLocked = (PEDD_SURFACE)DdHmgLock(peDdSSrc, ObjType_DDSURFACE_TYPE, FALSE);

    if (pDstLocked)
    {
        // FIXME: Implement AlphaBlt Body
    }
    else
    {
        DoWarning("DxDdAlphaBlt: Couldn't lock destination surface\n", 0);
    }

    pDdBltData->ddRVal = ddRVal;

    if (pDstLocked)
        InterlockedDecrement((VOID *)&pDstLocked->pobj.cExclusiveLock);

    if (pSrcLocked)
        InterlockedDecrement((VOID *)&pSrcLocked->pobj.cExclusiveLock);

    return result;
}

DWORD
NTAPI
DxDdAttachSurface(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdBeginMoCompFrame(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdBlt(EDD_SURFACE *peDdSDest, EDD_SURFACE *peDdSSrc, DD_BLTDATA *pDdBltData)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdColorControl(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdCreateMoComp(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdDeleteDirectDrawObject(PVOID p1)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdDeleteSurfaceObject(PVOID p1)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdDestroyMoComp(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdDestroySurface(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdDestroyD3DBuffer(PVOID p1)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdEndMoCompFrame(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdFlip(PVOID p1, PVOID p2, PVOID p3, PVOID p4, PVOID p5)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdFlipToGDISurface(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetAvailDriverMemory(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetBltStatus(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetDC(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetDxHandle(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetFlipStatus(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetInternalMoCompInfo(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetMoCompBuffInfo(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetMoCompGuids(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetMoCompFormats(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdGetScanLine(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdLockD3D(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdQueryMoCompStatus(PVOID p1, PVOID p2)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

VOID NTAPI
bDdUnlockSurfaceOrBuffer(PVOID p1)
{
    TRACE();
    UNIMPLEMENTED;
}

// Implemented
VOID NTAPI
vDdUnlockGdiSurface(EDD_SURFACE *peDds)
{
    TRACE();

    if (peDds->unk_flag < 0)
        bDdUnlockSurfaceOrBuffer(peDds);
}

// Implemented
BOOL NTAPI
bDdReleaseDC(EDD_SURFACE *peDds, DWORD Unused)
{
    TRACE();

    UNREFERENCED_PARAMETER(Unused);

    BOOL result = FALSE;

    HDC hdc = peDds->hdc;

    if (hdc)
    {
        if (!gpEngFuncs.DxEngDeleteDC(hdc, TRUE))
            DoWarning("bDdReleaseDC: Couldn't delete DC\n", 0);

        vDdUnlockGdiSurface(peDds);

        result = TRUE;
    }

    return result;
}

// Implemented
BOOL NTAPI
DxDdReleaseDC(EDD_SURFACE *peDdS)
{
    TRACE();

    BOOL result = FALSE;

    PEDD_SURFACE peDdSurface = (PEDD_SURFACE)DdHmgLock(peDdS, ObjType_DDSURFACE_TYPE, FALSE);

    if (peDdSurface)
    {
        result = bDdReleaseDC(peDdSurface, 0);
        InterlockedDecrement((VOID *)&peDdSurface->pobj.cExclusiveLock);
    }

    return result;
}

DWORD
NTAPI
DxDdRenderMoComp(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdResetVisrgn(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdSetColorKey(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdSetExclusiveMode(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdSetGammaRamp(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdCreateSurfaceEx(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdSetOverlayPosition(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdUnattachSurface(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdUnlockD3D(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdUpdateOverlay(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDdWaitForVerticalBlank(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpCanCreateVideoPort(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpColorControl(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpCreateVideoPort(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpDestroyVideoPort(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpFlipVideoPort(PVOID p1, PVOID p2, PVOID p3, PVOID p4)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortBandwidth(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortField(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortFlipStatus(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortInputFormats(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortLine(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortOutputFormats(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoPortConnectInfo(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpGetVideoSignalStatus(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpUpdateVideoPort(PVOID p1, PVOID p2, PVOID p3, PVOID p4)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpWaitForVideoPortSync(PVOID p1, PVOID p2)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpAcquireNotification(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    return 0;
}

DWORD
NTAPI
DxDvpReleaseNotification(EDD_VIDEOPORT *pDdVp, HANDLE NotifyEvent)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

DWORD
NTAPI
DxDdHeapVidMemAllocAligned(
    LPVIDMEM lpVidMem,
    DWORD dwWidth,
    DWORD dwHeight,
    LPSURFACEALIGNMENT lpAlignment,
    LPLONG lpNewPitch)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

// Implemented
VOID NTAPI
insertIntoList(LPVMEML Block, LPVMEML *List)
{
    if (!*List)
    {
        Block->next = *List;
        *List = Block;
    }
    else
    {
        VMEML *Current = Block;
        VMEML *Previous = NULL;

        do
        {
            if (Block->size < Current->size)
                break;

            Previous = Current;
            Current = Current->next;
        } while (Current);

        if (Previous)
        {
            Block->next = Current;
            Previous->next = Block;
        }
        else
        {
            Block = *List;
            *List = Block;
        }
    }
}

LPVMEML
NTAPI
coalesceFreeBlocks(LPVMEMHEAP lpHeap, LPVMEML lpBlock)
{
    TRACE();
    UNIMPLEMENTED;

    return NULL;
}

// Implemented
VOID NTAPI
linVidMemFree(LPVMEMHEAP lpHeap, DWORD dwPtr)
{
    TRACE();

    // We need a valid heap and address
    if (dwPtr && lpHeap)
    {
        // Get the allocation list from the heap
        VMEML *pVMemCurrent = (VMEML *)lpHeap->allocList;
        VMEML *pPreviousMem = 0;

        // While our pointer is not zero iterate through the list
        while (pVMemCurrent)
        {
            // If this current struct is for the address we are trying to free
            if (pVMemCurrent->ptr == dwPtr)
            {
                // If there was a previous entry update next to match the following
                // Else set start of the allocList to the next of this node
                if (pPreviousMem)
                    pPreviousMem->next = pVMemCurrent->next;
                else
                    lpHeap->allocList = pVMemCurrent->next;
                do
                    pVMemCurrent = coalesceFreeBlocks(lpHeap, pVMemCurrent);
                while (pVMemCurrent);
                // We freed the heap we intented to free
                break;
            }

            pPreviousMem = pVMemCurrent;
            pVMemCurrent = pVMemCurrent->next;
        }
    }
}

// Implemented
VOID NTAPI
insertIntoDoubleList(LPVMEMR First, LPVMEMR Second)
{
    TRACE();

    VMEMR *Next = Second;

    for (DWORD *i = (DWORD *)Second->ptr; *((DWORD *)Next->ptr) != 0x7FFFFFFF; i = (DWORD *)Next->ptr)
    {
        if (First->ptr < *i)
            break;
        Next = Next->next;
    }

    First->prev = Next->prev;
    First->next = Next;

    Next->prev->next = First;
    Next->prev = First;
}

VOID NTAPI
rectVidMemFree(LPVMEMHEAP lpHeap, DWORD dwPtr)
{
    TRACE();

    // FIXME: Implement rectgular memory release

    UNIMPLEMENTED;
}

// Implemented
VOID NTAPI
DxDdHeapVidMemFree(LPVMEMHEAP lpHeap, DWORD dwPtr)
{
    TRACE();

    if ((lpHeap->dwFlags & VMEMHEAP_LINEAR) != 0)
        linVidMemFree(lpHeap, dwPtr);
    else
        rectVidMemFree(lpHeap, dwPtr);
}

VOID NTAPI
vDdUnloadDxApiImage(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    TRACE();
    UNIMPLEMENTED;
}

VOID NTAPI
VidMemFini(VMEMHEAP *lpHeap)
{
    TRACE();
    UNIMPLEMENTED;

    /*if ((lpHeap->dwFlags & VMEMHEAP_LINEAR) != 0 )
      linVidMemFini(lpHeap);
    else
      rectVidMemFini(lpHeap);*/
}

VOID NTAPI
HeapVidMemFini(VIDEOMEMORY *pVidMemory, PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    TRACE();
    UNIMPLEMENTED;
}

VOID NTAPI
vDdDisableDriver(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    TRACE();

    // Check if the device is locked by current thread
    vDdAssertDevlock(peDdGl);

    VIDEOMEMORY *pvmList = peDdGl->pvmList;

    if (pvmList)
    {
        DWORD dwHeapNum = 0;
        VIDEOMEMORY *pvMemory = pvmList;

        // If we have any video memory heaps to free
        if (peDdGl->dwNumHeaps)
        {
            do
            {
                if (!(pvMemory->dwFlags & VIDMEM_HEAPDISABLED) && pvMemory->lpHeap)
                    HeapVidMemFini(pvMemory, peDdGl);

                dwHeapNum++;
                pvMemory++;
            } while (dwHeapNum < peDdGl->dwNumHeaps);
        }

        EngFreeMem(peDdGl->pvmList);
        peDdGl->pvmList = NULL;
    }

    if (peDdGl->pdwFourCC)
    {
        EngFreeMem(peDdGl->pdwFourCC);
        peDdGl->pdwFourCC = NULL;
    }

    if (peDdGl->lpDDVideoPortCaps)
    {
        EngFreeMem(peDdGl->lpDDVideoPortCaps);
        peDdGl->lpDDVideoPortCaps = NULL;
    }

    // Unload ApiImage
    if (peDdGl->hDxApi)
        vDdUnloadDxApiImage(peDdGl);

    if (peDdGl->unk_hdc != NULL)
    {
        gpEngFuncs.DxEngSetDCOwner((HGDIOBJ)&peDdGl->unk_hdc, GDI_OBJ_HMGR_POWNED /*0x80000002*/);

        gpEngFuncs.DxEngDeleteDC(peDdGl->unk_hdc, TRUE);
        peDdGl->unk_hdc = NULL;
    }

    // Checking if DirectDraw acceleration was enabled
    if (peDdGl->fl & 1)
    {
        // Reset DirectDraw acceleration flag
        peDdGl->fl = peDdGl->fl & 0xFFFFFFFE;

        PDRIVER_FUNCTIONS DriverFunctions =
            (PDRIVER_FUNCTIONS)gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_DrvFuncs);

        DriverFunctions->DisableDirectDraw((DHPDEV)peDdGl->dhpdev);
    }

    // We zero the structure starting from cDriverReferences and ending right at peDirectDrawLocalList
    memset(&peDdGl->cDriverReferences, 0, 0x590);
}

// Implemented
VOID NTAPI
DxDdDisableDirectDraw(HDEV hDev, BOOL bDisableVDd)
{
    TRACE();

    PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;

    peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    if ((peDdGl != NULL && peDdGl->hDev != NULL))
    {
        gpEngFuncs.DxEngLockHdev(hDev);

        if (bDisableVDd != 0)
        {
            vDdDisableDriver(peDdGl);
        }

        if (peDdGl->pAssertModeEvent != NULL)
        {
            EngFreeMem(peDdGl->pAssertModeEvent);
        }

        // Clear DirectDrawGlobla up to 0x618
        memset(peDdGl, 0, 0x618);

        gpEngFuncs.DxEngUnlockHdev(hDev);
    }
}

VOID NTAPI
vDdNotifyEvent(EDD_DIRECTDRAW_GLOBAL *peDdGl, DWORD dwEvent)
{
    TRACE();
    UNIMPLEMENTED;

    // We need to notify the lost objects of this event
    // HANDLE ObjectList = peDdGl->hDxLoseObj;
}

VOID NTAPI
vDdDxApiFreeSurface(HANDLE DdHandle, int a2)
{
    TRACE();
    UNIMPLEMENTED;
}

VOID NTAPI
vDdStopVideoPort(EDD_VIDEOPORT *a1)
{
    TRACE();
    UNIMPLEMENTED;
}

VOID NTAPI
vDdDisableSurfaceObject(EDD_DIRECTDRAW_GLOBAL *peDdGl, EDD_SURFACE *peDdSurf, DWORD *dwResult)
{
    TRACE();
    UNIMPLEMENTED;
}

VOID NTAPI
vDdLooseManagedSurfaceObject(EDD_DIRECTDRAW_GLOBAL *peDdGl, EDD_SURFACE *peDdSurface, DWORD *pdwResult)
{
    TRACE();

    DWORD CapsReserved;
    DD_SURFACE_LOCAL *pDdSurfaceLocal;

    // Surface destruction info
    PDD_SURFCB_DESTROYSURFACE cbDestroySurface = NULL;
    DD_DESTROYSURFACEDATA ddDestroyData = {0};

    vDdAssertDevlock(peDdGl);

    if ((peDdSurface->unk_flag & 0x200) == 0)
    {
        // Write the global into the destruction data
        ddDestroyData.lpDD = (PDD_DIRECTDRAW_GLOBAL)peDdGl;

        // Grab the surface local of peDdSurface
        pDdSurfaceLocal = &peDdSurface->ddsSurfaceLocal;

        // Invalidate the surface
        peDdSurface->ddsSurfaceLocal.dwFlags |= DDRAWISURF_INVALID;

        // Check if RESERVED2 is set
        CapsReserved = peDdSurface->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_RESERVED2;

        // If RESERVED2 is set and cbDestroySurface is not NULL this is a D3DBuffer
        if (CapsReserved && (cbDestroySurface = peDdGl->d3dBufCallbacks.DestroyD3DBuffer) != NULL)
        {
            cbDestroySurface(&ddDestroyData);
        } // Else this is a DirectDraw Buffer
        else if (!CapsReserved && (peDdGl->ddSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) != 0)
        {
            peDdGl->ddSurfaceCallbacks.DestroySurface(&ddDestroyData);
        }

        // Clear out the flags
        pDdSurfaceLocal->dwFlags &= 0xEFFFFFFF;
    }

    if (pdwResult)
        *pdwResult = TRUE;
}

EDD_SURFACE *
EnumDdSurface(EDD_DIRECTDRAW_LOCAL *peDdL, EDD_SURFACE *peDdSurfPrev)
{
    TRACE();

    PEDD_SURFACE *ppeDdStart;
    PEDD_SURFACE peDdSFound = NULL;

    ppeDdStart = &peDdL->peSurface_DdList;
    PEDD_SURFACE result = NULL;

    // Is there anything to enumerate
    if (*ppeDdStart != (PEDD_SURFACE)ppeDdStart)
    {
        if (peDdSurfPrev)
        {
            if ((PEDD_SURFACE *)peDdSurfPrev->peSurface_DdNext == ppeDdStart)
                return result;

            peDdSFound = peDdSurfPrev->peSurface_DdNext;
        }
        else
        {
            peDdSFound = *ppeDdStart;
        }

        if (peDdSFound)
            result = (PEDD_SURFACE)((DWORD)peDdSFound - offsetof(EDD_SURFACE, peSurface_DdNext));
    }

    return result;
}

VOID NTAPI
DdDisableMotionCompObject(EDD_MOTIONCOMP *a1)
{
    TRACE();
    UNREFERENCED_PARAMETER(a1);
    UNIMPLEMENTED;
}

VOID NTAPI
vDdUnreferenceVirtualMap(
    PVOID pvMap,
    EDD_DIRECTDRAW_GLOBAL *peDdGl,
    EDD_DIRECTDRAW_LOCAL *peDdLcl,
    PHANDLE hProcess,
    BOOL bDelete)
{
    TRACE();
    UNIMPLEMENTED;
}

// Implemented But Bad
VOID NTAPI
vDdDisableDirectDrawObject(EDD_DIRECTDRAW_LOCAL *ProcessHandle)
{
    TRACE();

    BOOL hasHeaps = ProcessHandle->dwNumHeaps > 0;
    PEDD_DIRECTDRAW_GLOBAL peDdGl = ProcessHandle->peDirectDrawGlobal2;

    HANDLE hOpenProcess = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    CLIENT_ID ClientId = {0};

    if (hasHeaps ||
        (ProcessHandle->isMemoryMapped & 1) != 0 && (peDdGl->ddCallbacks.dwFlags & DDHAL_CB32_MAPMEMORY) < 0)
    {
        ClientId.UniqueProcess = ProcessHandle->hProcess;
        ClientId.UniqueThread = NULL;

        ObjectAttributes.Length = 24;
        ObjectAttributes.RootDirectory = NULL;
        ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_PERMANENT;
        ObjectAttributes.ObjectName = NULL;
        ObjectAttributes.SecurityDescriptor = NULL;
        ObjectAttributes.SecurityQualityOfService = NULL;

        if (!NT_SUCCESS(ZwOpenProcess(&hOpenProcess, 0x40u, &ObjectAttributes, &ClientId)))
        {
            DoWarning("vDdDisableDirectDrawObject: Could not open process handle.", 1);
            hOpenProcess = NULL;
        }
    }

    if ((ProcessHandle->isMemoryMapped & 1) != 0)
    {
        ProcessHandle->isMemoryMapped = ProcessHandle->isMemoryMapped & 0xFFFFFFFE;

        if ((--peDdGl->dwMapCount & 0x80000000) != 0)
            DoDxgAssert("Invalid Map Count");

        if ((peDdGl->ddCallbacks.dwFlags & 0x80000000) != 0 && hOpenProcess)
        {
            vDdUnreferenceVirtualMap((PVOID)ProcessHandle->peMapCurrent, peDdGl, ProcessHandle, hOpenProcess, TRUE);
            ProcessHandle->peMapCurrent = 0;
        }
    }

    if (hasHeaps)
    {
        if (hOpenProcess)
        {
            DWORD dwCurrentHeap = 0;

            for (LPVMEMHEAP *lppHeap = (LPVMEMHEAP *)ProcessHandle->ppvHeaps; dwCurrentHeap < peDdGl->dwNumHeaps;
                 lppHeap++)
            {
                if (lppHeap)
                {
                    vDdUnreferenceVirtualMap((PVOID)*lppHeap, peDdGl, ProcessHandle, hOpenProcess, TRUE);
                    *lppHeap = 0;
                }

                dwCurrentHeap++;
            }
        }
        else
        {
            for (PEDD_VIDEOPORT port = ProcessHandle->peDirectDrawVideoport; port; port = port->peDirectDrawVideoport)
                vDdStopVideoPort(port);

            return;
        }
    }

    if (hOpenProcess && !NT_SUCCESS(ZwClose(hOpenProcess)))
        DoDxgAssert("Failed To Close Process Handle.");

    // Stop all video ports
    for (PEDD_VIDEOPORT port = ProcessHandle->peDirectDrawVideoport; port; port = port->peDirectDrawVideoport)
        vDdStopVideoPort(port);
}

VOID NTAPI
vDdDisableAllDirectDrawObjects(EDD_DIRECTDRAW_GLOBAL *ProcessHandle)
{
    TRACE();

    DD_CREATESURFACEEXDATA ddCreateSurfaceData = {0};

    PEDD_SURFACE PreviousSurface = NULL;
    EDD_DIRECTDRAW_LOCAL *ProcessHandlea = NULL;

    vDdAssertDevlock(ProcessHandle);

    BOOL bValidLocks = ProcessHandle->cSurfaceLocks <= ProcessHandle->gSurfaceLocks;

    ProcessHandle->bSuspended = TRUE;

    if (!bValidLocks)
    {
        gpEngFuncs.DxEngUnlockHdev(ProcessHandle->hDev);

        if (KeWaitForSingleObject(
                ProcessHandle->pAssertModeEvent, Executive, 0, 0, (PLARGE_INTEGER)&ProcessHandle->llAssertModeTimeout) <
            0)
        {
            DoDxgAssert("Wait error\n");
        }

        gpEngFuncs.DxEngLockHdev(ProcessHandle->hDev);

        KeResetEvent(ProcessHandle->pAssertModeEvent);
    }

    for (EDD_DIRECTDRAW_LOCAL *peDdCurrentLcl = ProcessHandle->peDirectDrawLocalList;;
         peDdCurrentLcl = ProcessHandlea->peDirectDrawLocal_prev)
    {
        ProcessHandlea = peDdCurrentLcl;

        if (!peDdCurrentLcl)
            break;

        for (PEDD_SURFACE peDdCurSurface = EnumDdSurface(peDdCurrentLcl, 0);;
             peDdCurSurface = EnumDdSurface(ProcessHandlea, PreviousSurface))
        {
            PreviousSurface = peDdCurSurface;

            if (!peDdCurSurface)
                break;

            if ((peDdCurSurface->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) != 0)
            {
                FLATPTR fpPrevVidMem;
                DD_ATTACHLIST *prevAttachList;
                DD_ATTACHLIST *prevAttachListFrom;

                if (ProcessHandle->ddMiscellanous2Callbacks.CreateSurfaceEx &&
                    peDdCurSurface->gdev == gpEngFuncs.DxEngGetHdevData(ProcessHandle->hDev, DxEGShDevData_GDev) &&
                    PreviousSurface->ddsSurfaceMore.dwSurfaceHandle)
                {
                    fpPrevVidMem = PreviousSurface->ddsSurfaceGlobal.fpVidMem;
                    prevAttachList = PreviousSurface->ddsSurfaceLocal.lpAttachList;
                    prevAttachListFrom = PreviousSurface->ddsSurfaceLocal.lpAttachListFrom;
                    ddCreateSurfaceData.lpDDSLcl = &PreviousSurface->ddsSurfaceLocal;

                    // Zero out surface memory and attachment
                    PreviousSurface->ddsSurfaceGlobal.fpVidMem = 0;
                    PreviousSurface->ddsSurfaceLocal.lpAttachList = NULL;
                    PreviousSurface->ddsSurfaceLocal.lpAttachListFrom = NULL;

                    ddCreateSurfaceData.ddRVal = E_FAIL;
                    ddCreateSurfaceData.dwFlags = 0;
                    ddCreateSurfaceData.lpDDLcl =
                        ProcessHandlea != 0 ? (PDD_DIRECTDRAW_LOCAL)&ProcessHandlea->peDirectDrawGlobal : 0;

                    ProcessHandle->ddMiscellanous2Callbacks.CreateSurfaceEx(&ddCreateSurfaceData);

                    PreviousSurface->ddsSurfaceGlobal.fpVidMem = fpPrevVidMem;
                    PreviousSurface->ddsSurfaceLocal.lpAttachList = prevAttachList;
                    PreviousSurface->ddsSurfaceLocal.lpAttachListFrom = prevAttachListFrom;

                    if (PreviousSurface->ddsSurfaceGlobal.dwReserved1)
                        DoWarning("Driver forget to clear SURFACE_GBL.dwReserved1 at CreateSurfaceEx()", 0);

                    if (PreviousSurface->ddsSurfaceLocal.dwReserved1)
                        DoWarning("Driver forget to clear SURFACE_LCL.dwReserved1 at CreateSurfaceEx()", 0);

                    PreviousSurface->ddsSurfaceGlobal.dwReserved1 = 0;
                    PreviousSurface->ddsSurfaceLocal.dwReserved1 = 0;
                }
            }
            else if (
                (peDdCurSurface->ddsSurfaceLocal.dwFlags & DDRAWISURF_DRIVERMANAGED) == 0 ||
                (peDdCurSurface->ddsSurfaceMore.ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST) != 0)
            {
                vDdDisableSurfaceObject(ProcessHandle, peDdCurSurface, NULL);
            }
            else
            {
                vDdLooseManagedSurfaceObject(ProcessHandle, peDdCurSurface, NULL);
            }
        }

        // Appears to be a motion comp object which has other motion comp objects linked to it
        for (PEDD_MOTIONCOMP k = ProcessHandlea->peDirectDrawMotion; k; k = k->peDirectDrawMotionNext)
            DdDisableMotionCompObject(k);

        vDdDisableDirectDrawObject(ProcessHandlea);
    }

    if (ProcessHandle->cSurfaceLocks)
        DoDxgAssert("There was a mismatch between global count of locks and actual");
}

#define DD_EVENTNOTIFY_SUSPEND 0x00000020l
#define DD_EVENTNOTIFY_RESUME 0x00000040l

// Implemented
VOID NTAPI
DxDdSuspendDirectDraw(HDEV hDev, DWORD dwSuspendFlags)
{
    TRACE();

    BOOL bIsMetaDevice = FALSE;
    HDEV pHdevCurrent = NULL;

    gpEngFuncs.DxEngIncDispUniq();

    if ((dwSuspendFlags & 1) != 0)
    {
        // Check if we have a valid hDev
        if (!hDev || !gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_display))
        {
            DoDxgAssert("Invalid HDEV");
        }

        bIsMetaDevice = gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_metadev);

        if (bIsMetaDevice)
        {
            // We enumerate if we are a meta device
            pHdevCurrent = *gpEngFuncs.DxEngEnumerateHdev(NULL);
        }
        else
        {
            // Use the passed device
            pHdevCurrent = hDev;
        }
    }
    else
    {
        pHdevCurrent = hDev;
    }

    if (!pHdevCurrent)
        DoDxgAssert("Invalid HDEV");

    if (!gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_display))
        DoDxgAssert("Not a display HDEV");

    // Recursively Suspend Child Devices
    while (pHdevCurrent)
    {
        if (!bIsMetaDevice || (HDEV)gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_Parent) == hDev)
        {
            BOOL bLockedShare = FALSE;

            PEDD_DIRECTDRAW_GLOBAL peDdGl =
                (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_eddg);

            if ((dwSuspendFlags & 2) != 0)
            {
                vDdAssertDevlock(peDdGl);
            }
            else
            {
                gpEngFuncs.DxEngLockShareSem();
                bLockedShare = TRUE;
            }

            BOOLEAN bHDevLocked = gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

            if (!peDdGl->hDev)
                DoDxgAssert("Can't suspend DirectDraw on an HDEV that was never DDraw enabled!");

            // If we are already suspended just move on and increment LockCount
            if (!gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_dd_locks))
            {
                vDdNotifyEvent(peDdGl, DD_EVENTNOTIFY_SUSPEND);
                vDdDisableAllDirectDrawObjects(peDdGl);
            }

            DWORD dwLocks = gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_dd_locks);
            gpEngFuncs.DxEngSetHdevData(pHdevCurrent, DxEGShDevData_dd_locks, dwLocks + 1);

            if (bHDevLocked)
                gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);

            if (bLockedShare)
                gpEngFuncs.DxEngUnlockShareSem();
        }

        // If this was not a meta device we do not enumerate so break out
        if (!bIsMetaDevice)
            break;

        pHdevCurrent = *gpEngFuncs.DxEngEnumerateHdev(&pHdevCurrent);
    }
}

// Implemented
VOID NTAPI
vDdRestoreSystemMemorySurface(HDEV hDev)
{
    PEDD_SURFACE peDdSurfacePrev = NULL;

    DD_CREATESURFACEEXDATA ddCreateSurfaceData = {0};

    PEDD_DIRECTDRAW_GLOBAL peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    if (peDdGl->ddMiscellanous2Callbacks.CreateSurfaceEx)
    {
        PEDD_DIRECTDRAW_LOCAL peDdLcl;

        for (peDdLcl = peDdGl->peDirectDrawLocalList; peDdLcl; peDdLcl = peDdLcl->peDirectDrawLocal_prev)
        {
            for (PEDD_SURFACE peDdSurfaceCurrent = EnumDdSurface(peDdLcl, NULL);;
                 peDdSurfaceCurrent = EnumDdSurface(peDdLcl, peDdSurfacePrev))
            {
                peDdSurfacePrev = peDdSurfaceCurrent;

                if (!peDdSurfaceCurrent)
                    break;

                if ((peDdSurfaceCurrent->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) != 0 &&
                    peDdSurfaceCurrent->gdev == gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_GDev))
                {
                    if (peDdSurfacePrev->ddsSurfaceMore.dwSurfaceHandle)
                    {
                        if ((peDdSurfacePrev->ddsSurfaceGlobal.ddpfSurface.dwFlags &
                             4 /*DISPLAYCONFIG_PIXELFORMAT_32BPP*/) != 0)
                        {
                            DWORD dwFourCC = peDdSurfacePrev->ddsSurfaceGlobal.ddpfSurface.dwFourCC;

                            if (dwFourCC == FOURCC_DXT1 || dwFourCC == FOURCC_DXT2 || dwFourCC == FOURCC_DXT3 ||
                                dwFourCC == FOURCC_DXT4 || dwFourCC == FOURCC_DXT5)
                            {
                                peDdSurfacePrev->ddsSurfaceGlobal.wWidth = peDdSurfacePrev->wWidth;
                                peDdSurfacePrev->ddsSurfaceGlobal.wHeight = peDdSurfacePrev->wHeight;
                            }
                        }
                    }
                }
            }

            KeAttachProcess((PKPROCESS)peDdLcl->hCreatorProcess);

            for (PEDD_SURFACE peDdSurfaceCurrent = EnumDdSurface(peDdLcl, NULL);;
                 peDdSurfaceCurrent = EnumDdSurface(peDdLcl, peDdSurfacePrev))
            {
                peDdSurfacePrev = peDdSurfaceCurrent;

                if (!peDdSurfaceCurrent)
                    break;

                if ((peDdSurfaceCurrent->unk_flag & 0x2000) != 0 &&
                    peDdSurfaceCurrent->gdev == gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_GDev))
                {
                    if ((peDdSurfacePrev->ddsSurfaceLocal.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == 0)
                        DoDxgAssert("DXG:ResumeDirectDraw: no system memory flag");

                    if (!peDdSurfacePrev->ddsSurfaceMore.dwSurfaceHandle)
                        DoDxgAssert("DXG:ResumeDirectDraw: dwSurfaceHandle is 0");

                    ddCreateSurfaceData.lpDDLcl = (PDD_DIRECTDRAW_LOCAL)peDdLcl->peDirectDrawGlobal;
                    ddCreateSurfaceData.lpDDSLcl = &peDdSurfacePrev->ddsSurfaceLocal;
                    ddCreateSurfaceData.dwFlags = 0;
                    ddCreateSurfaceData.ddRVal = E_FAIL;

                    peDdGl->ddMiscellanous2Callbacks.CreateSurfaceEx(&ddCreateSurfaceData);

                    if (ddCreateSurfaceData.ddRVal)
                        DoWarning("DxDdResumeDirectDraw(): Reassociate system memory surface failed", 0);
                }
            }

            KeDetachProcess();
        }
    }
}

// Implemented
VOID NTAPI
DxDdResumeDirectDraw(HDEV hDev, DWORD dwResumeFlags)
{
    TRACE();

    BOOL bIsMetaDevice = FALSE;
    HDEV pHdevCurrent = NULL;

    gpEngFuncs.DxEngIncDispUniq();

    if ((dwResumeFlags & 1) != 0)
    {
        // Check if we have a valid hDev
        if (!hDev || !gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_display))
        {
            DoDxgAssert("Invalid HDEV");
        }

        bIsMetaDevice = gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_metadev);

        if (bIsMetaDevice)
        {
            // We enumerate if we are a meta device
            pHdevCurrent = *gpEngFuncs.DxEngEnumerateHdev(NULL);
        }
        else
        {
            // Use the passed device
            pHdevCurrent = hDev;
        }
    }
    else
    {
        pHdevCurrent = hDev;
    }

    if (!pHdevCurrent)
        DoDxgAssert("Invalid HDEV");

    if (!gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_display))
        DoDxgAssert("Not a display HDEV");

    // Recursively Suspend Child Devices
    while (pHdevCurrent)
    {
        if (!bIsMetaDevice || (HDEV)gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_Parent) == hDev)
        {
            PEDD_DIRECTDRAW_GLOBAL peDdGl =
                (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_eddg);

            BOOLEAN bHDevLocked = gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

            if (!gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_dd_locks))
                DoDxgAssert("Must have called disable previously to be able to enable DirectDraw.");

            DWORD dwLocks = gpEngFuncs.DxEngGetHdevData(pHdevCurrent, DxEGShDevData_dd_locks);
            gpEngFuncs.DxEngSetHdevData(pHdevCurrent, DxEGShDevData_dd_locks, dwLocks - 1);

            // If dwLocks is not 0 do not execute the resume event
            if (!dwLocks)
            {
                vDdNotifyEvent(peDdGl, DD_EVENTNOTIFY_RESUME);
                vDdRestoreSystemMemorySurface(pHdevCurrent);
            }

            if (bHDevLocked)
                gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
        }

        // If this was not a meta device we do not enumerate so break out
        if (!bIsMetaDevice)
            break;

        pHdevCurrent = *gpEngFuncs.DxEngEnumerateHdev(&pHdevCurrent);
    }
}

DWORD
NTAPI
DxDdDynamicModeChange(PVOID p1, PVOID p2, PVOID p3)
{
    TRACE();
    UNIMPLEMENTED;

    return 0;
}

// Implemented
BOOL NTAPI
DdHmgOwnedBy(PDD_ENTRY pDdObj, DWORD dwPid)
{
    TRACE();

    DWORD Pid = (DWORD)(DWORD_PTR)dwPid & 0xFFFFFFFC;
    DWORD check = (DWORD_PTR)pDdObj->Pid & 0xFFFFFFFE;

    return ((check == Pid) || (!check));
}

// Implemented
PDD_ENTRY
NTAPI
DdHmgNextOwned(PDD_ENTRY pDdObj, DWORD dwPid)
{
    TRACE();

    DWORD dwIndex = DDHMG_HTOI(pDdObj);

    if (dwIndex < gcMaxDdHmgr)
    {
        do
        {
            PDD_ENTRY pDdCurrent = (PDD_ENTRY)((PBYTE)gpentDdHmgr + (sizeof(DD_ENTRY) * dwIndex));

            if (DdHmgOwnedBy(pDdCurrent, dwPid))
            {
                return (PDD_ENTRY)((pDdCurrent->FullUnique << 0x15) | dwIndex);
            }

            dwIndex++;
        } while (dwIndex < gcMaxDdHmgr);
    }

    return NULL;
}

// Implemented
VOID NTAPI
vDdDecrementReferenceCount(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    BOOL bLastReference = FALSE;

    BOOL bLocked = gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

    if (peDdGl->cDriverReferences <= 0)
        DoDxgAssert("Weird reference count");

    if (peDdGl->cDriverReferences-- == 1)
        bLastReference = TRUE;

    if (bLocked)
        gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);

    if (bLastReference)
        gpEngFuncs.DxEngUnreferenceHdev(peDdGl->hDev);
}

// Implemented
VOID NTAPI
DdFreeObject(PVOID pvObject, DWORD unused)
{
    TRACE();
    UNREFERENCED_PARAMETER(unused);

    EngFreeMem(pvObject);
}

// Implemented
PDD_ENTRY
NTAPI
DdHmgRemoveObject(HANDLE DdHandle, WORD wExclusiveLock, DWORD dwShareCount, BOOL bDelete, UCHAR ObjType)
{
    TRACE();
    UNIMPLEMENTED;

    DWORD index = ((DWORD)DdHandle & 0x1FFFFF);

    if (index >= gcMaxDdHmgr)
    {
        DoWarning("DdHmgRemove: Failed Invalid Index.", 1);
        return 0;
    }

    DdHmgAcquireHmgrSemaphore();

    PDD_ENTRY pDdEntry = (gpentDdHmgr + (sizeof(DD_ENTRY) * index));

    DWORD Pid = (DWORD)pDdEntry->Pid & 0xFFFFFFFE;

    if (Pid != ((DWORD)PsGetCurrentProcessId() * 0xFFFFFFFC) && Pid)
    {
        DoWarning("DdHmgRemove: Failed To Lock Handle.", 1);

        DdHmgReleaseHmgrSemaphore();
        return NULL;
    }

    if (pDdEntry->Objt != ObjType || pDdEntry->FullUnique != (((DWORD)DdHandle >> 21) & 0x7FF))
    {
        DoWarning("DdHmgRemove: Failed Invalid Object Type or FullUnique.", 1);

        DdHmgReleaseHmgrSemaphore();
        return NULL;
    }

    if (pDdEntry->pobj->cExclusiveLock != wExclusiveLock || pDdEntry->pobj->ulShareCount != dwShareCount)
    {
        DoWarning("DdHmgRemove: Failed Object Busy.", 1);

        DdHmgReleaseHmgrSemaphore();
        return NULL;
    }

    if (!bDelete && pDdEntry->pobj->BaseFlags & 0x100)
    {
        DoWarning("DdHmgRemove: Object Cannot Be Deleted.", 1);

        DdHmgReleaseHmgrSemaphore();
        return NULL;
    }

    pDdEntry->pobj = NULL;
    DdHmgFree(pDdEntry);

    DdHmgReleaseHmgrSemaphore();
    return pDdEntry;
}

// Implemented
DWORD
NTAPI
D3dDeleteHandle(HANDLE DdHandle, DWORD dwTextureContext, BOOL *bStatus, DWORD *dwRval)
{
    TRACE();

    KFLOATING_SAVE FloatSave;

    ULONG_PTR RegionSize;
    DWORD result = DD_FALSE;

    PEDD_CONTEXT peDdCtx = (PEDD_CONTEXT)DdHmgLock(DdHandle, ObjType_DDCONTEXT_TYPE, 0);

    if (!peDdCtx)
    {
        if (bStatus)
            *bStatus = FALSE;

        *dwRval = DDERR_INVALIDOBJECT;
        return DD_FALSE;
    }

    gpEngFuncs.DxEngLockShareSem();
    RegionSize = 0;

    if (!D3dSetup(peDdCtx->peDirectDrawGlobal, &FloatSave))
    {
        gpEngFuncs.DxEngUnlockShareSem();
        InterlockedDecrement((VOID *)&peDdCtx->pobj.cExclusiveLock);

        *dwRval = E_OUTOFMEMORY;

        return DD_FALSE;
    }

    HDEV hDev = peDdCtx->peDirectDrawGlobal->hDev;

    gpEngFuncs.DxEngReferenceHdev(hDev);

    // This seems to decide what kind of object we destroy
    BOOL bIsTexture = peDdCtx->bTexture;

    if (!bIsTexture)
    {
        D3DNTHAL_CONTEXTDESTROYDATA ddContextDestroyData = {0};

        MmUnsecureVirtualMemory(peDdCtx->hSecure);
        ZwFreeVirtualMemory(NtCurrentProcess(), &peDdCtx->BaseAddress, &RegionSize, MEM_RELEASE);
        peDdCtx->dwRegionSize = RegionSize;
        ddContextDestroyData.dwhContext = peDdCtx->dwhContext;

        result = peDdCtx->peDirectDrawGlobal->d3dNtHalCallbacks.ContextDestroy(&ddContextDestroyData);
        *dwRval = ddContextDestroyData.ddrval;
    }

    if (bIsTexture == TRUE)
    {
        D3DNTHAL_TEXTUREDESTROYDATA ddTextureDestroyData = {0};

        ddTextureDestroyData.dwhContext = dwTextureContext;
        ddTextureDestroyData.dwHandle = peDdCtx->dwhContext;

        result = peDdCtx->peDirectDrawGlobal->d3dNtHalCallbacks.TextureDestroy(&ddTextureDestroyData);
        *dwRval = ddTextureDestroyData.ddrval;
    }

    if (!DdHmgRemoveObject(DdHandle, 1, 0, 1, 3))
        DoDxgAssert("DdHmgRemoveObject failed D3dDeleteHandle");

    D3dCleanup(peDdCtx->peDirectDrawGlobal, &FloatSave);
    vDdDecrementReferenceCount(peDdCtx->peDirectDrawGlobal);

    gpEngFuncs.DxEngUnlockShareSem();
    gpEngFuncs.DxEngUnreferenceHdev(hDev);

    DdFreeObject(peDdCtx, ObjType_DDCONTEXT_TYPE);

    if (bStatus)
        *bStatus = TRUE;

    return result;
}

BOOL NTAPI
bDdDeleteDirectDrawObject(HANDLE DdHandle, BOOL bRelease)
{
    TRACE();

    UNREFERENCED_PARAMETER(DdHandle);
    UNREFERENCED_PARAMETER(bRelease);

    UNIMPLEMENTED;

    return FALSE;
}

BOOL NTAPI
bDdDeleteSurfaceObject(HANDLE DdHandle, BOOL bRelease)
{
    TRACE();

    UNREFERENCED_PARAMETER(DdHandle);
    UNREFERENCED_PARAMETER(bRelease);

    UNIMPLEMENTED;

    return FALSE;
}

BOOL NTAPI
bDdDeleteVideoPortObject(HANDLE DdHandle, BOOL bRelease)
{
    TRACE();

    UNREFERENCED_PARAMETER(DdHandle);
    UNREFERENCED_PARAMETER(bRelease);

    UNIMPLEMENTED;

    return FALSE;
}

BOOL NTAPI
bDdDeleteMotionCompObject(HANDLE DdHandle, BOOL bRelease)
{
    TRACE();

    UNREFERENCED_PARAMETER(DdHandle);
    UNREFERENCED_PARAMETER(bRelease);

    UNIMPLEMENTED;

    return FALSE;
}

// Implemented
BOOL NTAPI
DdHmgCloseProcess(DWORD dwPid)
{
    TRACE();

    PDD_ENTRY pDdEntry;
    DD_BASEOBJECT *pDdObj;
    DWORD dwCount;

    BOOL bSUCCESS = TRUE;

    // We are iterating a list here so we need to lock it
    DdHmgAcquireHmgrSemaphore();
    pDdEntry = DdHmgNextOwned(NULL, dwPid);
    pDdObj = (DD_BASEOBJECT *)pDdEntry;
    DdHmgReleaseHmgrSemaphore();

    while (pDdEntry)
    {
        // Not sure if this is correct as ((unsigned int)pDdObj >> 21 & 7)
        switch (pDdEntry->Objt)
        {
            case ObjType_DDLOCAL_TYPE:
                bSUCCESS = bDdDeleteDirectDrawObject(pDdObj, TRUE);
                break;
            case ObjType_DDSURFACE_TYPE:
                bSUCCESS = bDdDeleteSurfaceObject(pDdObj, 0);
                break;
            case ObjType_DDCONTEXT_TYPE:
                // D3dDeleteHandle returns 1 on error
                bSUCCESS = D3dDeleteHandle(pDdObj, 0, NULL, &dwCount) != 1;
                break;
            case ObjType_DDVIDEOPORT_TYPE:
                bSUCCESS = bDdDeleteVideoPortObject(pDdObj, 0);
                break;
            case ObjType_DDMOTIONCOMP_TYPE:
                bSUCCESS = bDdDeleteMotionCompObject(pDdObj, 0);
                break;
            default:
                bSUCCESS = FALSE;
                break;
        }

        if (!bSUCCESS)
        {
            DPRINT1("DDRAW ERROR: DdHmgCloseProcess couldn\'t delete obj = %p, type j=%lx\n", pDdEntry, pDdEntry->Objt);
        }

        DdHmgAcquireHmgrSemaphore();
        pDdEntry = DdHmgNextOwned(pDdEntry, dwPid);
        DdHmgReleaseHmgrSemaphore();
    }

    return bSUCCESS;
}

// Implemented
VOID NTAPI
DxDdCloseProcess(DWORD dwPid)
{
    TRACE();

    DdHmgCloseProcess(dwPid);
}

// Implemented
BOOL NTAPI
DxDdGetDirectDrawBound(HDEV hDev, LPRECT lpRect)
{
    TRACE();

    BOOL Result = FALSE;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;

    peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    vDdAssertDevlock(peDdGl);

    // Do we have some kind of global bounds locked
    Result = (peDdGl->fl & 4) != 0;

    if (Result)
    {
        lpRect->left = peDdGl->rcbounds.left;
        lpRect->top = peDdGl->rcbounds.top;
        lpRect->right = peDdGl->rcbounds.right;
        lpRect->bottom = peDdGl->rcbounds.bottom;

        // Reset the flag UINT_MAX - 4
        peDdGl->fl = peDdGl->fl & 0xfffffffb;
    }

    return Result;
}

// Implemented
DWORD
NTAPI
DxDdEnableDirectDrawRedirection(PVOID p1, PVOID p2)
{
    TRACE();

    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);

    // No Implementation
    return 0;
}

// Implemented
PVOID
NTAPI
DxDdAllocPrivateUserMem(PEDD_SURFACE peDdSurface, SIZE_T cjMemSize, ULONG ulTag)
{
    TRACE();

    PVOID pvUserMem = NULL;

    PEPROCESS peProcess = PsGetCurrentProcess();

    // Check owning process?
    if (peDdSurface != NULL && peDdSurface->peDirectDrawLocal->hCreatorProcess == peProcess)
    {
        pvUserMem = EngAllocUserMem(cjMemSize, ulTag);
    }

    return pvUserMem;
}

// Implemented
VOID NTAPI
SafeFreeUserMem(PVOID pvMem, PKPROCESS pKProcess)
{
    TRACE();

    if (pKProcess == (PKPROCESS)PsGetCurrentProcess())
    {
        EngFreeUserMem(pvMem);
    }
    else
    {
        KeAttachProcess(pKProcess);
        EngFreeUserMem(pvMem);
        KeDetachProcess();
    }
}

// Implemented Questionable
VOID NTAPI
DeferMemoryFree(PVOID pvMem, EDD_SURFACE *peDdSurface)
{
    PDEFERMEMORY_ENTRY pDmEntry;

    pDmEntry = (PDEFERMEMORY_ENTRY)EngAllocMem(FL_ZERO_MEMORY, sizeof(DEFERMEMORY_ENTRY), TAG_GDDP);

    if (pDmEntry != NULL)
    {
        pDmEntry->pvMemory = pvMem;
        pDmEntry->peDdSurface = peDdSurface;
        pDmEntry->pPrevEntry = (PDEFERMEMORY_ENTRY)peDdSurface->peDirectDrawLocal->peDirectDrawGlobal2->unk_608;

        peDdSurface->peDirectDrawLocal->peDirectDrawGlobal2->unk_608 = (ULONG_PTR)pDmEntry;

        peDdSurface->unk_flag |= 0x1000;
    }
}

// Implemented
VOID NTAPI
DxDdFreePrivateUserMem(PVOID pvSurface, PVOID pvMem)
{
    TRACE();

    PEDD_SURFACE peDdSurf = pvSurface != NULL ? (PEDD_SURFACE)((DWORD)pvSurface - sizeof(DD_BASEOBJECT)) : NULL;

    // 8 on windows 2003 and 0x800 on NT6 is some system memory flag
    // Unsure what exactly we are looking at for a flag here
#if (WINVER >= 0x600)
    if ((peDdSurf->unk_flag & 0x800) != 0)
#else
    if ((peDdSurf->unk_flag & 8) != 0)
#endif
    {
        DeferMemoryFree(pvMem, peDdSurf);
    }
    else
    {
        SafeFreeUserMem(pvMem, (PKPROCESS)peDdSurf->peDirectDrawLocal->hCreatorProcess);
    }
}

// Implemented
VOID NTAPI
DxDdSetAccelLevel(HDEV hDev, DWORD dwLevel, BYTE bFlags)
{
    TRACE();

    PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;
    peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    if (dwLevel >= 3 || bFlags & 2)
    {
        peDdGl->llAssertModeTimeout.HighPart = 0;
        peDdGl->llAssertModeTimeout.LowPart = 0;
    }
}

// Implemented
PEDD_SURFACE
NTAPI
DxDdGetSurfaceLock(HDEV hDev)
{
    TRACE();

    PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;

    peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    return peDdGl->peSurface_LockList;
}

// Implemented
PEDD_SURFACE
NTAPI
DxDdEnumLockedSurfaceRect(HDEV hDev, PEDD_SURFACE peDdSurface, LPRECT lpRect)
{
    TRACE();

    PEDD_SURFACE result = NULL;

    if (peDdSurface == NULL)
    {
        PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;
        peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

        // No surface was supplied lets find the first locked one
        result = peDdGl->peSurface_LockList;
    }
    else
    {
        // Is there another locked surface below this one?
        result = peDdSurface->peSurface_LockNext;
    }

    if (result)
    {
        // If we found a locked surface copy it's rectangle
        lpRect->left = result->rclLock.left;
        lpRect->top = result->rclLock.top;
        lpRect->right = result->rclLock.right;
        lpRect->bottom = result->rclLock.bottom;
    }

    // Return the surface we found
    return result;
}

DRVFN gaDxgFuncs[] = {
    {DXG_INDEX_DxDxgGenericThunk, (PFN)DxDxgGenericThunk},
    {DXG_INDEX_DxD3dContextCreate, (PFN)DxD3dContextCreate},
    {DXG_INDEX_DxD3dContextDestroy, (PFN)DxD3dContextDestroy},
    {DXG_INDEX_DxD3dContextDestroyAll, (PFN)DxD3dContextDestroyAll},
    {DXG_INDEX_DxD3dValidateTextureStageState, (PFN)DxD3dValidateTextureStageState},
    {DXG_INDEX_DxD3dDrawPrimitives2, (PFN)DxD3dDrawPrimitives2},
    {DXG_INDEX_DxDdGetDriverState, (PFN)DxDdGetDriverState},
    {DXG_INDEX_DxDdAddAttachedSurface, (PFN)DxDdAddAttachedSurface},
    {DXG_INDEX_DxDdAlphaBlt, (PFN)DxDdAlphaBlt},
    {DXG_INDEX_DxDdAttachSurface, (PFN)DxDdAttachSurface},
    {DXG_INDEX_DxDdBeginMoCompFrame, (PFN)DxDdBeginMoCompFrame},
    {DXG_INDEX_DxDdBlt, (PFN)DxDdBlt},
    {DXG_INDEX_DxDdCanCreateSurface, (PFN)DxDdCanCreateSurface},
    {DXG_INDEX_DxDdCanCreateD3DBuffer, (PFN)DxDdCanCreateD3DBuffer},
    {DXG_INDEX_DxDdColorControl, (PFN)DxDdColorControl},
    {DXG_INDEX_DxDdCreateDirectDrawObject, (PFN)DxDdCreateDirectDrawObject},
    {DXG_INDEX_DxDdCreateSurface, (PFN)DxDdCreateD3DBuffer},
    {DXG_INDEX_DxDdCreateD3DBuffer, (PFN)DxDdCreateD3DBuffer},
    {DXG_INDEX_DxDdCreateMoComp, (PFN)DxDdCreateMoComp},
    {DXG_INDEX_DxDdCreateSurfaceObject, (PFN)DxDdCreateSurfaceObject},
    {DXG_INDEX_DxDdDeleteDirectDrawObject, (PFN)DxDdDeleteDirectDrawObject},
    {DXG_INDEX_DxDdDeleteSurfaceObject, (PFN)DxDdDeleteSurfaceObject},
    {DXG_INDEX_DxDdDestroyMoComp, (PFN)DxDdDestroyMoComp},
    {DXG_INDEX_DxDdDestroySurface, (PFN)DxDdDestroySurface},
    {DXG_INDEX_DxDdDestroyD3DBuffer, (PFN)DxDdDestroyD3DBuffer},
    {DXG_INDEX_DxDdEndMoCompFrame, (PFN)DxDdEndMoCompFrame},
    {DXG_INDEX_DxDdFlip, (PFN)DxDdFlip},
    {DXG_INDEX_DxDdFlipToGDISurface, (PFN)DxDdFlipToGDISurface},
    {DXG_INDEX_DxDdGetAvailDriverMemory, (PFN)DxDdGetAvailDriverMemory},
    {DXG_INDEX_DxDdGetBltStatus, (PFN)DxDdGetBltStatus},
    {DXG_INDEX_DxDdGetDC, (PFN)DxDdGetDC},
    {DXG_INDEX_DxDdGetDriverInfo, (PFN)DxDdGetDriverInfo},
    {DXG_INDEX_DxDdGetDxHandle, (PFN)DxDdGetDxHandle},
    {DXG_INDEX_DxDdGetFlipStatus, (PFN)DxDdGetFlipStatus},
    {DXG_INDEX_DxDdGetInternalMoCompInfo, (PFN)DxDdGetInternalMoCompInfo},
    {DXG_INDEX_DxDdGetMoCompBuffInfo, (PFN)DxDdGetMoCompBuffInfo},
    {DXG_INDEX_DxDdGetMoCompGuids, (PFN)DxDdGetMoCompGuids},
    {DXG_INDEX_DxDdGetMoCompFormats, (PFN)DxDdGetMoCompFormats},
    {DXG_INDEX_DxDdGetScanLine, (PFN)DxDdGetScanLine},
    {DXG_INDEX_DxDdLock, (PFN)DxDdLock},
    {DXG_INDEX_DxDdLockD3D, (PFN)DxDdLockD3D},
    {DXG_INDEX_DxDdQueryDirectDrawObject, (PFN)DxDdQueryDirectDrawObject},
    {DXG_INDEX_DxDdQueryMoCompStatus, (PFN)DxDdQueryMoCompStatus},
    {DXG_INDEX_DxDdReenableDirectDrawObject, (PFN)DxDdReenableDirectDrawObject},
    {DXG_INDEX_DxDdReleaseDC, (PFN)DxDdReleaseDC},
    {DXG_INDEX_DxDdRenderMoComp, (PFN)DxDdRenderMoComp},
    {DXG_INDEX_DxDdResetVisrgn, (PFN)DxDdResetVisrgn},
    {DXG_INDEX_DxDdSetColorKey, (PFN)DxDdSetColorKey},
    {DXG_INDEX_DxDdSetExclusiveMode, (PFN)DxDdSetExclusiveMode},
    {DXG_INDEX_DxDdSetGammaRamp, (PFN)DxDdSetGammaRamp},
    {DXG_INDEX_DxDdCreateSurfaceEx, (PFN)DxDdCreateSurfaceEx},
    {DXG_INDEX_DxDdSetOverlayPosition, (PFN)DxDdSetOverlayPosition},
    {DXG_INDEX_DxDdUnattachSurface, (PFN)DxDdUnattachSurface},
    {DXG_INDEX_DxDdUnlock, (PFN)DxDdUnlock},
    {DXG_INDEX_DxDdUnlockD3D, (PFN)DxDdUnlockD3D},
    {DXG_INDEX_DxDdUpdateOverlay, (PFN)DxDdUpdateOverlay},
    {DXG_INDEX_DxDdWaitForVerticalBlank, (PFN)DxDdWaitForVerticalBlank},
    {DXG_INDEX_DxDvpCanCreateVideoPort, (PFN)DxDvpCanCreateVideoPort},
    {DXG_INDEX_DxDvpColorControl, (PFN)DxDvpColorControl},
    {DXG_INDEX_DxDvpCreateVideoPort, (PFN)DxDvpCreateVideoPort},
    {DXG_INDEX_DxDvpDestroyVideoPort, (PFN)DxDvpDestroyVideoPort},
    {DXG_INDEX_DxDvpFlipVideoPort, (PFN)DxDvpFlipVideoPort},
    {DXG_INDEX_DxDvpGetVideoPortBandwidth, (PFN)DxDvpGetVideoPortBandwidth},
    {DXG_INDEX_DxDvpGetVideoPortField, (PFN)DxDvpGetVideoPortField},
    {DXG_INDEX_DxDvpGetVideoPortFlipStatus, (PFN)DxDvpGetVideoPortFlipStatus},
    {DXG_INDEX_DxDvpGetVideoPortInputFormats, (PFN)DxDvpGetVideoPortInputFormats},
    {DXG_INDEX_DxDvpGetVideoPortLine, (PFN)DxDvpGetVideoPortLine},
    {DXG_INDEX_DxDvpGetVideoPortOutputFormats, (PFN)DxDvpGetVideoPortOutputFormats},
    {DXG_INDEX_DxDvpGetVideoPortConnectInfo, (PFN)DxDvpGetVideoPortConnectInfo},
    {DXG_INDEX_DxDvpGetVideoSignalStatus, (PFN)DxDvpGetVideoSignalStatus},
    {DXG_INDEX_DxDvpUpdateVideoPort, (PFN)DxDvpUpdateVideoPort},
    {DXG_INDEX_DxDvpWaitForVideoPortSync, (PFN)DxDvpWaitForVideoPortSync},
    {DXG_INDEX_DxDvpAcquireNotification, (PFN)DxDvpAcquireNotification},
    {DXG_INDEX_DxDvpReleaseNotification, (PFN)DxDvpReleaseNotification},
    {DXG_INDEX_DxDdHeapVidMemAllocAligned, (PFN)DxDdHeapVidMemAllocAligned},
    {DXG_INDEX_DxDdHeapVidMemFree, (PFN)DxDdHeapVidMemFree},
    {DXG_INDEX_DxDdEnableDirectDraw, (PFN)DxDdEnableDirectDraw},
    {DXG_INDEX_DxDdDisableDirectDraw, (PFN)DxDdDisableDirectDraw},
    {DXG_INDEX_DxDdSuspendDirectDraw, (PFN)DxDdSuspendDirectDraw},
    {DXG_INDEX_DxDdResumeDirectDraw, (PFN)DxDdResumeDirectDraw},
    {DXG_INDEX_DxDdDynamicModeChange, (PFN)DxDdDynamicModeChange},
    {DXG_INDEX_DxDdCloseProcess, (PFN)DxDdCloseProcess},
    {DXG_INDEX_DxDdGetDirectDrawBound, (PFN)DxDdGetDirectDrawBound},
    {DXG_INDEX_DxDdEnableDirectDrawRedirection, (PFN)DxDdEnableDirectDrawRedirection},
    {DXG_INDEX_DxDdAllocPrivateUserMem, (PFN)DxDdAllocPrivateUserMem},
    {DXG_INDEX_DxDdFreePrivateUserMem, (PFN)DxDdFreePrivateUserMem},
    {DXG_INDEX_DxDdLockDirectDrawSurface, (PFN)DxDdLockDirectDrawSurface},
    {DXG_INDEX_DxDdUnlockDirectDrawSurface, (PFN)DxDdUnlockDirectDrawSurface},
    {DXG_INDEX_DxDdSetAccelLevel, (PFN)DxDdSetAccelLevel},
    {DXG_INDEX_DxDdGetSurfaceLock, (PFN)DxDdGetSurfaceLock},
    {DXG_INDEX_DxDdEnumLockedSurfaceRect, (PFN)DxDdEnumLockedSurfaceRect},
    {DXG_INDEX_DxDdIoctl, (PFN)DxDdIoctl}};
