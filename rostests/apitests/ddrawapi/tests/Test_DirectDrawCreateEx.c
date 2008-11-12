
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
    }

#if DUMP_ON
    if (pIntDirectDraw7 != NULL)
    {
        dump_ddrawi_directdraw_int(pIntDirectDraw7);
    }
#endif

    DirectDraw7 = NULL;
    pIntDirectDraw7 = NULL;
    return APISTATUS_NORMAL;
}

