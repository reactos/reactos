
INT
Test_DirectDrawCreateEx(PTESTINFO pti)
{

    LPDIRECTDRAW7 DirectDraw7;
    LPDDRAWI_DIRECTDRAW_INT pIntDirectDraw7;
    HRESULT ret;


    /* Create DirectDraw7 */
    ret = DirectDrawCreateEx(NULL, (VOID**)&DirectDraw7, &IID_IDirectDraw7, NULL);
    pIntDirectDraw7 = (DDRAWI_DIRECTDRAW_INT *)DirectDraw7;

    RTEST(ret == DD_OK);
    if (ret == DD_OK)
    {
        RTEST(pIntDirectDraw7->lpVtbl != NULL);
        RTEST(pIntDirectDraw7->lpLcl != NULL);
        RTEST(pIntDirectDraw7->lpLink == NULL);
        RTEST(pIntDirectDraw7->dwIntRefCnt == 1);
        
        RTEST(pIntDirectDraw7->lpLcl->lpDDMore == 0);
        RTEST(pIntDirectDraw7->lpLcl->lpGbl != NULL);
        RTEST(pIntDirectDraw7->lpLcl->dwUnused0 == 0);
        RTEST(pIntDirectDraw7->lpLcl->dwLocalFlags == DDRAWILCL_DIRECTDRAW7);
        RTEST(pIntDirectDraw7->lpLcl->dwLocalRefCnt == 1);
        
        /* pIntDirectDraw7->lpLcl->dwProcessId call see if we have same ProcessId */
        RTEST(pIntDirectDraw7->lpLcl->dwProcessId != 0); 

        RTEST(pIntDirectDraw7->lpLcl->pUnkOuter == NULL);
        RTEST(pIntDirectDraw7->lpLcl->dwObsolete1 == 0);        
        RTEST(pIntDirectDraw7->lpLcl->hWnd == 0);
        
        /* FIXME vaildate pIntDirectDraw7->lpLcl->hDC */
        RTEST(pIntDirectDraw7->lpLcl->hDC != 0);  
        
        RTEST(pIntDirectDraw7->lpLcl->dwErrorMode == 0); 
        RTEST(pIntDirectDraw7->lpLcl->lpPrimary == NULL);
        RTEST(pIntDirectDraw7->lpLcl->lpCB == NULL);
        RTEST(pIntDirectDraw7->lpLcl->dwPreferredMode == 0); 
        RTEST(pIntDirectDraw7->lpLcl->hD3DInstance == NULL); 
        RTEST(pIntDirectDraw7->lpLcl->pD3DIUnknown == NULL); 
        
        RTEST(pIntDirectDraw7->lpLcl->lpDDCB != NULL); 
        //RTEST(pIntDirectDraw7->lpLcl->hDDVxd != -1); fixme
        RTEST(pIntDirectDraw7->lpLcl->hFocusWnd == 0); 
        RTEST(pIntDirectDraw7->lpLcl->dwHotTracking == 0); 
        RTEST(pIntDirectDraw7->lpLcl->dwIMEState == 0); 
        
        RTEST(pIntDirectDraw7->lpLcl->hWndPopup == 0); 
        RTEST(pIntDirectDraw7->lpLcl->hDD != 0); 
        RTEST(pIntDirectDraw7->lpLcl->hGammaCalibrator != 0); 
        RTEST(pIntDirectDraw7->lpLcl->lpGammaCalibrator != NULL);
    }

#if DUMP_ON
    if (pIntDirectDraw7 != NULL)
    {
        dump_ddrawi_directdraw_int(pIntDirectDraw7);
        dump_ddrawi_directdraw_lcl(pIntDirectDraw7->lpLcl);
    }
#endif

    DirectDraw7 = NULL;
    pIntDirectDraw7 = NULL;
    return APISTATUS_NORMAL;
}

