#include "generic.h"

#pragma optimize("",off)

/***************************************************************************
*                               CreateINTENTControl
*  function:
*    This is a main testing program
*    
*  prototype:
*       void CreateINTENTControl(
*               LPSTR       FileName,
*               LPSTR       SaveFileName,
*               WORD        CSAType,  
*               BOOL        AllowBinary)
*  parameters:
*       FileName    --  Color Profile Filename
*       SaveFileName--  Color Space save to Filename
*       CSAType     --  Create CSA based on auto or manual selection.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       None.
****************************************************************************/
void 
CreateINTENTControl(
    LPSTR       FileName,
    LPSTR       SaveFileName,
    DWORD       Inter_Intent)
{
    BOOL        Ret;
    CHANDLE     cp;
    HGLOBAL     hCP;
    HGLOBAL     hBuffer;
    MEMPTR      lpBuffer;
    OFSTRUCT    OfStruct;
    HFILE       hOutput;
    DWORD       cbSize;
    CSIG        Intent;

    if( !LoadCP32( (LPCSTR) FileName, &hCP, (LPCHANDLE) &cp) )
    {
        return;
    }
    
    switch (Inter_Intent)
    {
        case 0: Intent = icPerceptual; break;
        case 1: Intent = icRelativeColorimetric; break;
        case 2: Intent = icSaturation; break;
        case 3: Intent = icAbsoluteColorimetric; break;
        default: return;
    }

    Ret = GetPS2ColorRenderingIntent(cp, Intent, NULL, &cbSize);
    if (Ret)
    {
        if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
            return;
        Ret = GetPS2ColorRenderingIntent(cp, Intent, lpBuffer, &cbSize);
        hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
        cbSize = _lwrite(hOutput, lpBuffer, cbSize);
        _lclose(hOutput);
        MemFree(hBuffer);
    }
    FreeCP(hCP);

    if (!Ret)
         MessageBox( GetFocus(), "No PS2 Intent created.", NULL, MB_OK );
}

/***************************************************************************
*                               CreateCRDControl
*  function:
*    This is a main testing program
*    
*  prototype:
*       void ColorSpaceControl(
*               LPSTR       FileName,
*               LPSTR       SaveFileName,
*               WORD        CSAType,  
*               BOOL        AllowBinary)
*  parameters:
*       FileName    --  Color Profile Filename
*       SaveFileName--  Color Space save to Filename
*       CSAType     --  Create CSA based on auto or manual selection.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       None.
****************************************************************************/
void 
CreateCRDControl(
    LPSTR       FileName,
    LPSTR       SaveFileName,
    DWORD       Inter_Intent,
    BOOL        AllowBinary)
{
    BOOL        Ret;
    CHANDLE     cp;
    HGLOBAL     hCP;
    HGLOBAL     hBuffer;
    MEMPTR      lpBuffer;
    OFSTRUCT    OfStruct;
    HFILE       hOutput;
    DWORD       cbSize;
    CSIG        Intent;

    if( !LoadCP32( (LPCSTR) FileName, &hCP, (LPCHANDLE) &cp) )
    {
        return;
    }
    
    switch (Inter_Intent)
    {
        case 0: Intent = icPerceptual; break;
        case 1: Intent = icRelativeColorimetric; break;
        case 2: Intent = icSaturation; break;
        case 3: Intent = icAbsoluteColorimetric; break;
        default: return;
    }

    Ret = GetPS2ColorRenderingDictionary(cp, Intent, NULL, &cbSize, AllowBinary);
    if (Ret)
    {
        if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
            return;
        Ret = GetPS2ColorRenderingDictionary(cp, Intent, lpBuffer, &cbSize, AllowBinary);
        hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
        cbSize = _lwrite(hOutput, lpBuffer, cbSize);
        _lclose(hOutput);
        MemFree(hBuffer);
    }
    FreeCP(hCP);

    if (!Ret)
         MessageBox( GetFocus(), "No CRD created.", NULL, MB_OK );
}

/***************************************************************************
*                               CreateCRDControl
*  function:
*    This is a main testing program
*    
*  prototype:
*       void ColorSpaceControl(
*               LPSTR       FileName,
*               LPSTR       SaveFileName,
*               WORD        CSAType,  
*               BOOL        AllowBinary)
*  parameters:
*       FileName    --  Color Profile Filename
*       SaveFileName--  Color Space save to Filename
*       CSAType     --  Create CSA based on auto or manual selection.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       None.
****************************************************************************/
void 
CreateProfCRDControl(
    LPSTR       DevProfile,
    LPSTR       TargetProfile,
    LPSTR       SaveFileName,
    DWORD       Inter_Intent,
    BOOL        AllowBinary)
{
    BOOL        Ret;
    CHANDLE     cpDev, cpTar;
    HGLOBAL     hDevCP = 0, hTarCP = 0;
    HGLOBAL     hBuffer;
    MEMPTR      lpBuffer;
    OFSTRUCT    OfStruct;
    HFILE       hOutput;
    DWORD       cbSize;
    CSIG        Intent;

    LoadCP32( (LPCSTR) DevProfile, &hDevCP, (LPCHANDLE) &cpDev);
    LoadCP32( (LPCSTR) TargetProfile, &hTarCP, (LPCHANDLE) &cpTar);

    if (!hDevCP || !hTarCP)
    {
        if (hDevCP)  FreeCP(hDevCP);
        if (hTarCP)  FreeCP(hTarCP);
        return;
    }
    
    switch (Inter_Intent)
    {
        case 0: Intent = icPerceptual; break;
        case 1: Intent = icRelativeColorimetric; break;
        case 2: Intent = icSaturation; break;
        case 3: Intent = icAbsoluteColorimetric; break;
        default: return;
    }

    Ret = GetPS2PreviewColorRenderingDictionary(cpDev, cpTar, Intent, NULL, &cbSize, AllowBinary);
    if (Ret)
    {
        if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
            return;
        Ret = GetPS2PreviewColorRenderingDictionary(cpDev, cpTar, Intent, lpBuffer, &cbSize, AllowBinary);
        hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
        cbSize = _lwrite(hOutput, lpBuffer, cbSize);
        _lclose(hOutput);
        MemFree(hBuffer);
    }
    FreeCP(hDevCP);
    FreeCP(hTarCP);

    if (!Ret)
         MessageBox( GetFocus(), "No CRD created.", NULL, MB_OK );
}

/***************************************************************************
*                               ColorSpaceControl
*  function:
*    This is a main testing program
*    
*  prototype:
*       void ColorSpaceControl(
*               LPSTR       FileName,
*               LPSTR       SaveFileName,
*               WORD        CSAType,  
*               BOOL        AllowBinary)
*  parameters:
*       FileName    --  Color Profile Filename
*       SaveFileName--  Color Space save to Filename
*       CSAType     --  Create CSA based on auto or manual selection.
*       AllowBinary --  1: binary CS allowed,  0: only ascii CS allowed.
*  returns:
*       None.
****************************************************************************/
void 
ColorSpaceControl(
    LPSTR       FileName,
    LPSTR       SaveFileName,
    DWORD       InpDrvClrSp,
    DWORD       Intent,
    WORD        CSAType,
    BOOL        AllowBinary)
{
    BOOL        Ret;
    CHANDLE     cp;
    HGLOBAL     hCP;
    HGLOBAL     hBuffer;
    MEMPTR      lpBuffer;
    OFSTRUCT    OfStruct;
    HFILE       hOutput;
    DWORD       cbSize;

    if( !LoadCP32( (LPCSTR) FileName, &hCP, (LPCHANDLE) &cp) )
    {
        return;
    }
    switch (InpDrvClrSp)
    {
        case 0: break;
        case 1: InpDrvClrSp = icSigGrayData; break;
        case 3: InpDrvClrSp = icSigRgbData; break;
        case 4: InpDrvClrSp = icSigCmykData; break;
        default: InpDrvClrSp = 0; break;
    }

    // Create CieBasedDEF(G) first. if can not, create CieBasedABC.
    if (CSAType == 405)
    {
        Ret = GetPS2ColorSpaceArray(cp, Intent, InpDrvClrSp, NULL, &cbSize, AllowBinary);
        if (Ret)
        {
            if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
                return;
            Ret = GetPS2ColorSpaceArray(cp, Intent, InpDrvClrSp, lpBuffer, &cbSize, AllowBinary);

            hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
            cbSize = _lwrite(hOutput, lpBuffer, cbSize);
            _lclose(hOutput);

            MemFree(hBuffer);
        }
    }
    // Create CieBasedABC
    else if (CSAType == 406)
    {
        Ret = GetPS2ColorSpaceArray(cp, Intent, icSigRgbData, NULL, &cbSize, AllowBinary);
        if (Ret)
        {
            if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
                return;
            Ret = GetPS2ColorSpaceArray(cp, Intent, icSigRgbData, lpBuffer, &cbSize, AllowBinary);

            hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
            cbSize = _lwrite(hOutput, lpBuffer, cbSize);
            _lclose(hOutput);

            MemFree(hBuffer);
        }
    }
#if 0
    else if (CSAType == 406)
    {
        Ret = GetPS2CSA_ABC( cp, NULL, &cbSize, InpDrvClrSp, AllowBinary);
        if (Ret)
        {
            if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
                return;
            Ret = GetPS2CSA_ABC( cp, lpBuffer, &cbSize, InpDrvClrSp, AllowBinary);

            hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
            cbSize = _lwrite(hOutput, lpBuffer, cbSize);
            _lclose(hOutput);

            MemFree(hBuffer);
        }

    }
    else if ((CSAType == 407) &&
        (GetCPDevSpace(cp, (LPCSIG) &ColorSpace)))
    {
        if (ColorSpace == icSigRgbData)
            Ret = GetPS2CSA_DEFG_Intent(cp, NULL, &cbSize, 
                    InpDrvClrSp, Intent, TYPE_CIEBASEDDEF, AllowBinary);
        else if (ColorSpace == icSigCmykData)
            Ret = GetPS2CSA_DEFG_Intent(cp, NULL, &cbSize, 
                    InpDrvClrSp, Intent, TYPE_CIEBASEDDEFG, AllowBinary);
        if (Ret)
        {
            if (!MemAlloc(cbSize, &hBuffer, &lpBuffer))
                return;
            if (ColorSpace == icSigRgbData)
                Ret = GetPS2CSA_DEFG_Intent(cp, lpBuffer, &cbSize, 
                    InpDrvClrSp, Intent, TYPE_CIEBASEDDEF, AllowBinary);
            else if (ColorSpace == icSigCmykData)
                Ret = GetPS2CSA_DEFG_Intent(cp, lpBuffer, &cbSize, 
                    InpDrvClrSp, Intent, TYPE_CIEBASEDDEFG, AllowBinary);
            hOutput = OpenFile(SaveFileName, &OfStruct, OF_CREATE);
            cbSize = _lwrite(hOutput, lpBuffer, cbSize);
            _lclose(hOutput);

            MemFree(hBuffer);
        }

    }
#endif
    FreeCP(hCP);
    if (!Ret)
         MessageBox( GetFocus(), "No CRD created.", NULL, MB_OK );
    return;
}
