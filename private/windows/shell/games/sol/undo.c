#include "sol.h"
VSZASSERT



BOOL FInitUndo(UDR *pudr)
{
    INT icol;

    pudr->fAvail = fFalse;

    for(icol = 0; icol < 2; icol++)
         if((pudr->rgpcol[icol] = PcolCreate(NULL, 0, 0, 0, 0, icrdUndoMax)) == NULL)
        {
            if(icol != 0)
                FreeP(pudr->rgpcol[0]);
            return fFalse;
        }
    return fTrue;
}
    

VOID FreeUndo(UDR *pudr)
{
    INT icol;

    for(icol = 0; icol < 2; icol++)
        FreeP(pudr->rgpcol[icol]);
}
