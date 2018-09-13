#include "precomp.h"
#pragma hdrstop


BOOL
FIsDirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    // IA64 direct (inlink) thunk looks like this:
    // <Unknown at this time>
    return FALSE;
}


BOOL
FIsIndirectJump(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    
/* IA64 direct (inlink) thunk looks like this:
0004000000203008002026000240100b {    addl        r2=.....,gp;;
                                      ld8         r2=[r2]
                                      nop.i       00h;;     }
07000830c0203008001014180420180a {    ld8         r3=[r2],08h;;
                                      ld8         gp=[r2]
                                      mov.few.dc.dc b6=r3,$+0x0     }
00800060000002000000000100000010 {    nop.m       00h
                                      nop.i       00h
                                      br.cond.sptk.few b6     }
00000000000000000000000000000000 {    break.m     00h
                                      break.i     00h   
                                      break.i     00h     }
*/
    int i;
    BYTE Slot1AddlMask[6] =  {  0xFF,0xFF,0x03,0x06,0x00,0x3D }; //aleviates relocs in slot1
    BYTE TemplateThunk[64] = {  0x0b,0x10,0x40,0x02,0x00,0x26,0x20,0x00,0x08,0x30,0x20,0x00,0x00,0x00,0x04,0x00,
                                0x0a,0x18,0x20,0x04,0x18,0x14,0x10,0x00,0x08,0x30,0x20,0xc0,0x30,0x08,0x00,0x07,
                                0x10,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x60,0x00,0x80,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            };


    if (cbBuff < sizeof(TemplateThunk))  // Must have 4 Bundles
    {
        DPRINT(1,("Not in thunk 1\n"));
        return FALSE;
    }
    
    for (i=0;i<6;i++) //gotta be a better way
    { 
        rgbBuffer[i] &= Slot1AddlMask[i];
        TemplateThunk[i] &= Slot1AddlMask[i];
    } 

    if (memcmp(TemplateThunk,rgbBuffer,sizeof(TemplateThunk)))
    {
        int i;
        DPRINT(1,("Not in thunk 2\n"));
        for(i=0;i<sizeof(TemplateThunk);i++) {
            if(TemplateThunk[i] != rgbBuffer[i]) {
                DPRINT(1,("%i - %02x -%02x\n",
                       i,
                       TemplateThunk[i],
                       rgbBuffer[i]
                       ));
            }
        }
        return FALSE;
    }
    
    if (lpdwThunkSize)
    {
        *lpdwThunkSize = sizeof(TemplateThunk);
    }
    
    DPRINT(1,("In thunk\n"));
    return TRUE;
}

BOOL
FIsVCallThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsVTDispAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
FIsAdjustorThunk(
    BYTE *      rgbBuffer,
    DWORD       cbBuff,
    HTHDX       hthd,
    UOFFSET     uoffset,
    UOFFSET *   lpuoffThunkDest,
    LPDWORD     lpdwThunkSize
    )
{
    return FALSE;
}


BOOL
GetPMFDest(
    HTHDX hthd,
    UOFFSET uThis,
    UOFFSET uPMF,
    UOFFSET *lpuOffDest
    )
{
    return FALSE;
}
