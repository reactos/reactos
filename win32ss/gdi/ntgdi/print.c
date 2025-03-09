/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/print.c
 * PURPOSE:         Print functions
 * PROGRAMMER:
 */

 #include <win32k.h>
 #define NDEBUG
 #include <debug.h>
 
 
 BOOL
 APIENTRY
 IntEndDoc(
     HDC  hDC,
     DWORD doAbort )
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     INT Ret = FALSE;
 
     pdc = DC_LockDc(hDC);
     if (pdc == NULL)
     {
         EngSetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
     }
 
     if ( !pdc->dclevel.pSurface ||
          !(ppdev = pdc->ppdev)  ||
           ppdev->flFlags & PDEV_DISPLAY ||
          !ppdev->hSpooler )
     {
         if (pdc) DC_UnlockDc( pdc );
         EngSetLastError(ERROR_CAN_NOT_COMPLETE);
         return FALSE;
     }
 
     if ( pdc->dclevel.lSaveDepth > pdc->dclevel.lSaveDepthStartDoc )
     {
         DC_vRestoreDC( pdc, pdc->dclevel.lSaveDepthStartDoc );
     }
 
     if (ppdev->DriverFunctions.EndDoc)
     {
         SURFOBJ *pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
         Ret = ppdev->DriverFunctions.EndDoc(pSurfObj, doAbort);
     }
 
     pdc->dclevel.pSurface = NULL; // This DC has no surface (empty mem or info DC).
     pdc->ipfdDevMax = -1;
 
     PDEVOBJ_vRelease(ppdev);
 
     DC_UnlockDc( pdc );
     return Ret;
 
 }
 
 INT
 APIENTRY
 NtGdiAbortDoc(HDC  hDC)
 {
     return IntEndDoc(hDC, ED_ABORTDOC);
 }
 
 /*
  * @implemented
  */
 BOOL
 APIENTRY
 NtGdiDoBanding(
     IN HDC hdc,
     IN BOOL bStart,
     OUT POINTL *pptl,
     OUT PSIZE pSize )
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     SURFOBJ *pSurfObj;
     POINTL ptlSafe;
     SIZEL sizeSafe;
     INT Ret = FALSE;
 
     pdc = DC_LockDc(hdc);
     if ( !pdc                   ||
          !pdc->dclevel.pSurface ||
          !(ppdev = pdc->ppdev)  ||
          !ppdev->hSpooler       ||
          !(pdc->dclevel.pSurface->flags & BANDING_SURFACE) )
     {
         if (pdc) DC_UnlockDc(pdc);
         return FALSE;
     }
 
     pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
     if (bStart)
     {
         Ret = ppdev->DriverFunctions.StartBanding( pSurfObj, &ptlSafe );
         sizeSafe.cx = pSurfObj->sizlBitmap.cx;
         sizeSafe.cy = pSurfObj->sizlBitmap.cy;
         pdc->ptlDoBanding.x = ptlSafe.x;
         pdc->ptlDoBanding.y = ptlSafe.y;
     }
     else
     {
         Ret = ppdev->DriverFunctions.NextBand( pSurfObj, &ptlSafe );
         if (Ret)
         {
             if ( ptlSafe.x == -1 &&          // physical page's bands have been drawn.
                  ppdev->flFlags & PDEV_UMPD )
             {
                 pdc->fs &= ~DC_RESET;
                 if (pdc->dclevel.pSurface->pWinObj)
                 {
                     EngDeleteWnd((WNDOBJ *)pdc->dclevel.pSurface->pWinObj); // See UserGethWnd.
                     pdc->dclevel.pSurface->pWinObj = NULL;
                 }
                 pdc->ipfdDevMax = 0;
             }
             else
             {
                 pdc->ptlDoBanding.x = ptlSafe.x;
                 pdc->ptlDoBanding.y = ptlSafe.y;
             }
         }
     }
     if (Ret)
     {
         _SEH2_TRY
         {
             ProbeForWrite(pptl, sizeof(POINT), 1);
             *pptl = ptlSafe;
 
             ProbeForWrite(pSize, sizeof(SIZE), 1);
             *pSize = sizeSafe;
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
             Ret = FALSE;
         }
         _SEH2_END;
     }
     DC_UnlockDc(pdc);
     return Ret;
 }
 
 INT
 APIENTRY
 NtGdiEndDoc(HDC  hDC)
 {
   return IntEndDoc(hDC, 0);
 }
 
 INT
 APIENTRY
 NtGdiEndPage(HDC  hDC)
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     INT Ret = 0;
 
     pdc = DC_LockDc(hDC);
     if (!pdc || !pdc->dclevel.pSurface)
     {
         if (pdc) DC_UnlockDc(pdc);
         EngSetLastError(ERROR_INVALID_HANDLE);
         return 0;
     }
 
     if ( (ppdev = pdc->ppdev) && !(ppdev->flFlags & PDEV_DISPLAY) )
     {
         if ( ppdev->flFlags & PDEV_UMPD &&
              ppdev->hSpooler &&
              ppdev->DriverFunctions.SendPage)
         {
            SURFOBJ *pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
            if (ppdev->DriverFunctions.SendPage(pSurfObj))
            {
                pdc->fs &= ~DC_RESET;
                if (pdc->dclevel.pSurface->pWinObj)
                {
                   EngDeleteWnd((WNDOBJ *)pdc->dclevel.pSurface->pWinObj); // See UserGethWnd.
                   pdc->dclevel.pSurface->pWinObj = NULL;
                }
                pdc->ipfdDevMax = -1;
                Ret = 1;
            }
         }
     }
     DC_UnlockDc(pdc);
     return Ret;
 }
 
 INT
 APIENTRY
 IntExtEscape(
     PDC    pdc,
     INT    iEsc,
     INT    cjIn,
     LPSTR  pjIn,
     INT    cjOut,
     LPSTR  pjOut )
 {
     INT ret;
     PPDEVOBJ ppdev;
     BOOL bQOGLGI = FALSE;
     SURFACE *psurf;
 /*
   {
     INT Result;
 
     if ((pdc->ppdev->DriverFunctions.Escape == NULL) ||
         (pdc->dclevel.pSurface == NULL))
     {
        Result = 0;
     }
     else
     {
        DC_vPrepareDCsForBlit(pdc, NULL, NULL, NULL);
        psurf = pdc->dclevel.pSurface;
 
        Result = pdc->ppdev->DriverFunctions.Escape(
           &psurf->SurfObj,
           iEsc,
           cjIn,
           pjIn,
           cjOut,
           pjOut );
 
        DC_vFinishBlit(pdc, NULL);
     }
     return Result;
   }
  */
     if ( iEsc == QUERYESCSUPPORT && cjIn < sizeof(ULONG) )
         return 0;
 
     if ( iEsc == QUERYESCSUPPORT )
     {
          DWORD dwIn = *(PDWORD)pjIn;
 
          if ( dwIn == OPENGL_GETINFO || dwIn == OPENGL_CMD )
          {
              DPRINT("QUERYESCSUPPORT OPENGL_GETINFO or OPENGL_CMD\n");
              bQOGLGI = TRUE;
          }
     } 
 
     ppdev = pdc->ppdev;
 
     if ( !ppdev || !ppdev->DriverFunctions.Escape )
     {
         DPRINT("No ppdev or DrvEsacpe ptr\n");
         return 0;
     }
 
     // Fix part of CORE-16465.
     if ( bQOGLGI || iEsc == OPENGL_CMD || iEsc == OPENGL_GETINFO )
     {
       NTSTATUS Status = STATUS_SUCCESS;
       BOOLEAN SwapStateEnabled;
         if ( pdc->dctype != DCTYPE_DIRECT /*|| ppdev->DxDd_Flags & */ ) // Not Direct or DxDD
             return 0;
          PETHREAD CurrThread = PsGetCurrentThread();
          //My math here might be stupid.
          KFLOATING_SAVE TempBuffer;
          NTSTATUS
          NTAPI
          MmGrowKernelStack(IN PVOID StackPointer);
          ULONG_PTR ThreadStackBase = (ULONG_PTR)((ULONG_PTR)CurrThread->Tcb.StackBase-KERNEL_LARGE_STACK_SIZE+
          KERNEL_LARGE_STACK_COMMIT);
          Status = MmGrowKernelStack((PVOID)ThreadStackBase);
          if (Status != STATUS_SUCCESS)
             DPRINT("Stack is sad, sad AMD\n");
          SwapStateEnabled = KeSetKernelStackSwapEnable(FALSE);
         DPRINT("OPENGL_GETINFO or OPENGL_CMD\n");
         Status = KeSaveFloatingPointState(&TempBuffer);
         ret = ppdev->DriverFunctions.Escape( &pdc->dclevel.pSurface->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
         KeRestoreFloatingPointState(&TempBuffer);
         KeSetKernelStackSwapEnable(SwapStateEnabled);
         return ret;
     }
 
     if ( iEsc == DCICOMMAND )
     {
         return 0;
     }
 
     if ( iEsc == WNDOBJ_SETUP )
     {
         if ( pdc->dctype != DCTYPE_DIRECT ) return 0;
 
         ret = ppdev->DriverFunctions.Escape( &pdc->dclevel.pSurface->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
         // FIXME: Do EWndObj stuff. See eng/engwindow.c.
         /*if ( gbWndobjUpdate )
         {
             gbWndobjUpdate = 0;
             //Force a Client Region Update
         }*/
         return ret;
     }
 
     if ( !pdc->dclevel.pSurface && cjIn > sizeof(WORD) )
     {
         SURFOBJ sTemp;
 
         RtlZeroMemory( &sTemp, sizeof(SURFOBJ) );
 
         sTemp.hdev = (HDEV)ppdev;
         sTemp.dhpdev = pdc->dhpdev;
         sTemp.iType = STYPE_DEVICE;
 
         switch ( iEsc )
         {
             case SETCOPYCOUNT:
             {
                 pdc->ulCopyCount = (*(PUSHORT)pjIn); // Save copy count in DC
 
                 return ppdev->DriverFunctions.Escape( &sTemp, iEsc, cjIn, pjIn, cjOut, pjOut );
             }
 
             case EPSPRINTING:
             {
                 if ((*(PUSHORT)pjIn))
                 {
                    pdc->fs |= DC_EPSPRINTINGESCAPE;
                 }
                 else
                 {
                    pdc->fs &= ~DC_EPSPRINTINGESCAPE;
                 }
                 return 1;
             }
 
             default:
                 DPRINT("No Surface and with copy input\n");
                 return 0; // Must have a surface!
         }
     }
 
     if (!pdc->dclevel.pSurface)
     {
         DPRINT("No Surface\n");
         return 0;
     }
 
     if ( pdc->dctype != DCTYPE_DIRECT )
     {
         if ( pdc->dclevel.pSurface->SurfObj.iType != STYPE_DEVBITMAP )
         {
             DPRINT("Should Fail, Not Direct and Not Opaque device-managed surface\n");
         }
     }
 
     // Have a surface. So fall through.
 
     DC_vPrepareDCsForBlit(pdc, NULL, NULL, NULL);
     psurf = pdc->dclevel.pSurface;
     DPRINT("ExtEscape Code %d cjIn %d pjIn %p cjOut %d pjOut %p\n",iEsc,cjIn,pjIn,cjOut,pjOut);
     ret = pdc->ppdev->DriverFunctions.Escape( &psurf->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
     DC_vFinishBlit(pdc, NULL);
     return ret;
 }
 
 INT
 APIENTRY
 IntNameExtEscape(
     PWCHAR pDriver,
     INT    iEsc,
     INT    cjIn,
     LPSTR  pjIn,
     INT    cjOut,
     LPSTR  pjOut )
 {
     PPDEVOBJ ppdev;
     UNICODE_STRING usDriver;
     WCHAR awcBuffer[MAX_PATH];
     RtlInitEmptyUnicodeString(&usDriver, awcBuffer, sizeof(awcBuffer));
     RtlAppendUnicodeToString(&usDriver, L"\\SystemRoot\\System32\\");
     RtlAppendUnicodeToString(&usDriver, pDriver);
 
     ppdev = EngpGetPDEV(&usDriver);
     if ( ppdev && ppdev->DriverFunctions.Escape )
     {
        return ppdev->DriverFunctions.Escape( &ppdev->pSurface->SurfObj, iEsc, cjIn, pjIn, cjOut, pjOut );
     }
     return 0;
 }
 
 INT
 APIENTRY
 NtGdiExtEscape(
     _In_opt_ HDC hdc,
     _In_reads_opt_(cwcDriver) PWCHAR pDriver,
     _In_ INT cwcDriver,
     _In_ INT iEsc,
     _In_ INT cjIn,
     _In_reads_bytes_opt_(cjIn) LPSTR pjIn,
     _In_ INT cjOut,
     _Out_writes_bytes_opt_(cjOut) LPSTR pjOut )
 {
     PDC pdc;
     LPSTR SafeInData = NULL;
     LPSTR SafeOutData = NULL;
     PWCHAR psafeDriver = NULL;
     INT Ret = -1;
 
     if ( pDriver )
     {
         psafeDriver = ExAllocatePoolWithTag( PagedPool, (cwcDriver + 1) * sizeof(WCHAR), GDITAG_TEMP );
         RtlZeroMemory( psafeDriver, (cwcDriver + 1) * sizeof(WCHAR) );
     }
 
     if ( cjIn )
     {
        SafeInData = ExAllocatePoolWithTag( PagedPool, cjIn + 1, GDITAG_TEMP );
     }
 
     if ( cjOut )
     {
        SafeOutData = ExAllocatePoolWithTag( PagedPool, cjOut + 1, GDITAG_TEMP );
     }
 
     _SEH2_TRY
     {
         if ( psafeDriver )
         {
             ProbeForRead(pDriver, cwcDriver * sizeof(WCHAR), 1);
             RtlCopyMemory(psafeDriver, pDriver, cwcDriver * sizeof(WCHAR));
         }
         if ( SafeInData )
         {
             ProbeForRead(pjIn, cjIn, 1);
             RtlCopyMemory(SafeInData, pjIn, cjIn);
         }
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
         SetLastNtError(_SEH2_GetExceptionCode());
         _SEH2_YIELD(goto Exit);
     }
     _SEH2_END;
 
 
     if ( pDriver )
     {
         Ret = IntNameExtEscape( psafeDriver, iEsc, cjIn, SafeInData, cjOut, SafeOutData );
     }
     else
     {
         pdc = DC_LockDc(hdc);
         if ( pdc )
         {
             Ret = IntExtEscape( pdc, iEsc, cjIn, SafeInData, cjOut, SafeOutData );
 
             DC_UnlockDc(pdc);
         }
     }
 
     if ( SafeOutData )
     {
         _SEH2_TRY
         {
             ProbeForWrite(pjOut, cjOut, 1);
             RtlCopyMemory(pjOut, SafeOutData, cjOut);
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
             SetLastNtError(_SEH2_GetExceptionCode());
             Ret = -1;
         }
         _SEH2_END;
     }
 Exit:
     if ( psafeDriver ) ExFreePoolWithTag ( psafeDriver, GDITAG_TEMP );
     if ( SafeInData ) ExFreePoolWithTag ( SafeInData, GDITAG_TEMP );
     if ( SafeOutData ) ExFreePoolWithTag ( SafeOutData, GDITAG_TEMP );
 
     return Ret;
 }
 
 INT
 APIENTRY
 IntStartDoc(
     IN HDC hdc,
     IN DOCINFOW *pdi,
     OUT BOOL *pbBanding,
     IN INT iJob )
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     SURFOBJ *pSurfObj;
     INT Ret = 0;
 
     pdc = DC_LockDc(hdc);
     if ( !pdc                   ||
           pdc->dclevel.pSurface ||
           pdc->dctype           ||
          !(ppdev = pdc->ppdev)  ||
          !ppdev->hSpooler       ||
           (ppdev->flFlags & PDEV_DISPLAY) ||
          !(ppdev->flFlags & PDEV_UMPD) )
     {
         if (pdc) DC_UnlockDc(pdc);
         return FALSE;
     }
 
     if ( !(pdc->dclevel.pSurface = PDEVOBJ_pSurface( ppdev )) )
     {
         DC_UnlockDc(pdc);
         return FALSE;
     }
 
     if ( pdc->dclevel.pSurface )
     {
         pdc->dclevel.sizl.cx = ppdev->pSurface->SurfObj.sizlBitmap.cx;
         pdc->dclevel.sizl.cy = ppdev->pSurface->SurfObj.sizlBitmap.cy;
         IntSetDefaultRegion(pdc);
     }
 
     pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
     *pbBanding = !!(pdc->dclevel.pSurface->flags & BANDING_SURFACE);
 
     Ret = ppdev->pfn.StartDoc(pSurfObj, (LPWSTR)pdi->lpszDocName, iJob );
 
     if (  pdc->ulCopyCount != -1 )
     {
         ULONG ulCopyCount = pdc->ulCopyCount;
 
         IntExtEscape( pdc,
                       SETCOPYCOUNT,
                       sizeof(ULONG),
                       (LPSTR)&ulCopyCount,
                       0,
                       NULL );
 
         pdc->ulCopyCount = -1;
     }
 
     if ( pdc->fs & DC_EPSPRINTINGESCAPE )
     {
         WORD Temp = 1;
 
         IntExtEscape( pdc,
                       EPSPRINTING,
                       sizeof(WORD),
                       (LPSTR)&Temp,
                       0,
                       NULL );
 
         pdc->fs &= ~DC_EPSPRINTINGESCAPE;
     }
 
     if ( Ret )
     {
         pdc->dclevel.lSaveDepthStartDoc = pdc->dclevel.lSaveDepth;
     }
 
     DC_UnlockDc(pdc);
     return Ret;
 }
 
 INT
 APIENTRY
 NtGdiStartDoc(
     IN HDC hdc,
     IN DOCINFOW *pdi,
     OUT BOOL *pbBanding,
     IN INT iJob )
 {
    DOCINFOW diSafe;
    BOOL bBanding;
    INT size, Ret = 0;
 
    diSafe.cbSize = 0;
    diSafe.lpszDocName = NULL;
    diSafe.lpszOutput = NULL; 
    diSafe.lpszDatatype = NULL;
    diSafe.fwType = 0;
 
    if ( pdi )
    {
        _SEH2_TRY
        {
            ProbeForRead(pdi, sizeof(DOCINFOW), 1);
            diSafe.cbSize = pdi->cbSize;
            diSafe.fwType = pdi->fwType;
 
            if ( pdi->lpszDocName )
            {
                size = (wcslen(pdi->lpszDocName) + 1) * sizeof(WCHAR);
 
                diSafe.lpszDocName = (LPCWSTR)ExAllocatePoolWithTag(PagedPool, size, GDITAG_TEMP);
 
                RtlCopyMemory((PVOID)diSafe.lpszDocName, (PVOID)pdi->lpszDocName, size);
            }
            if ( pdi->lpszOutput )
            {
                size = (wcslen(pdi->lpszOutput) + 1) * sizeof(WCHAR);
 
                diSafe.lpszOutput = (LPCWSTR)ExAllocatePoolWithTag(PagedPool, size, GDITAG_TEMP);
 
                RtlCopyMemory((PVOID)diSafe.lpszOutput, (PVOID)pdi->lpszOutput, size);
            }
            if ( pdi->lpszDatatype )
            {
                size = (wcslen(pdi->lpszDatatype) + 1) * sizeof(WCHAR);
 
                diSafe.lpszDatatype = (LPCWSTR)ExAllocatePoolWithTag(PagedPool, size, GDITAG_TEMP);
 
                RtlCopyMemory((PVOID)diSafe.lpszDatatype, (PVOID)pdi->lpszDatatype, size);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            _SEH2_YIELD(goto Exit);
        }
        _SEH2_END;
    }
 
    Ret = IntStartDoc( hdc, &diSafe, &bBanding, iJob );
 
    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pbBanding, sizeof(BOOL), 1);
            *pbBanding = bBanding;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
   }
 Exit:
   if ( diSafe.lpszDocName ) ExFreePoolWithTag((PVOID)diSafe.lpszDocName, GDITAG_TEMP);
 
   if ( diSafe.lpszOutput ) ExFreePoolWithTag((PVOID)diSafe.lpszOutput, GDITAG_TEMP);
 
   if ( diSafe.lpszDatatype ) ExFreePoolWithTag((PVOID)diSafe.lpszDatatype, GDITAG_TEMP);
 
   return Ret;
 }
 
 INT
 APIENTRY
 NtGdiStartPage(
     HDC  hDC )
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     INT Ret = 0;
 
     pdc = DC_LockDc(hDC);
     if (!pdc || !pdc->dclevel.pSurface)
     {
         if (pdc) DC_UnlockDc(pdc);
         EngSetLastError(ERROR_INVALID_HANDLE);
         return 0;
     }
 
     if ( (ppdev = pdc->ppdev) )
     {
         if ( ppdev->flFlags & PDEV_UMPD &&
              ppdev->hSpooler &&
              ppdev->DriverFunctions.StartPage )
         {
            SURFOBJ *pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
            if (ppdev->DriverFunctions.StartPage(pSurfObj))
            {
                pdc->fs |= DC_RESET;
                pdc->ptlDoBanding.x = pdc->ptlDoBanding.y = 0;
                Ret = 1;
            }
            else
            {
                DC_UnlockDc(pdc);
                IntEndDoc(hDC, ED_ABORTDOC);
                return Ret;
            }
         }
     }
     DC_UnlockDc(pdc);
     return Ret;
 }
 
 /*
  * @implemented
  */
 ULONG
 APIENTRY
 NtGdiGetPerBandInfo(
     IN HDC hdc,
     IN OUT PERBANDINFO *ppbi )
 {
     PPDEVOBJ ppdev;
     PDC pdc;
     PERBANDINFO pbi;
     INT Ret = DDI_ERROR;
 
     _SEH2_TRY
     {
         ProbeForRead( ppbi, sizeof(PERBANDINFO), 1 );
         RtlCopyMemory( &pbi, ppbi, sizeof(PERBANDINFO) );
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
         SetLastNtError(_SEH2_GetExceptionCode());
         _SEH2_YIELD(return Ret);
     }
     _SEH2_END;
 
     pbi.bRepeatThisBand = FALSE;
 
     pdc = DC_LockDc(hdc);
     if (!pdc )
     {
         EngSetLastError(ERROR_INVALID_HANDLE);
         return Ret;
     }
 
     if ( pdc->dclevel.pSurface && 
          (ppdev = pdc->ppdev) )
     {
         if ( ppdev->hSpooler &&
              pdc->dclevel.pSurface->flags & BANDING_SURFACE )
         {
             if ( ppdev->DriverFunctions.QueryPerBandInfo )
             {
                 SURFOBJ *pSurfObj = &pdc->dclevel.pSurface->SurfObj;
 
                 Ret = ppdev->DriverFunctions.QueryPerBandInfo(pSurfObj, &pbi);
             }
             else
             {
                 Ret = 0;
             }
         }
     }
 
     if ( Ret != DDI_ERROR )
     {
         _SEH2_TRY
         {
             ProbeForWrite( ppbi, sizeof(PERBANDINFO), 1 );
             RtlCopyMemory( ppbi, &pbi, sizeof(PERBANDINFO) );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
             SetLastNtError(_SEH2_GetExceptionCode());
         }
         _SEH2_END;
     }
 
     DC_UnlockDc(pdc);
     return Ret;
 }
 
 /* EOF */
  