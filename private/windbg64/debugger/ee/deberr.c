#include "debexpr.h"

/***    GetErrorText - Get Error Text from TM structure
 *
 *      status = GetErrorText(phTM, Status, phError)
 *
 *      Entry   phTM = pointer to handle of TM structure
 *              Status = EESTATUS exit code of last operation on the TM
 *              phError = pointer to EESTR handle
 *
 *      Exit    *phError = handle to newly allocated string
 *              that contains error message
 *
 *      Returns EESTATUS
 *
 */
ulong  GetErrorText (PHTM phTM, EESTATUS Status, PEEHSTR phError)
{
    uint        cchAvail;
    ulong       err_num;
    ulong       buflen;
    char       *pBuf;
    char       *pErrSymbol;
    char       *pPercent;
    char        Tempbuf[4];
    uint        cchErrSymbol;
    int         cnt;
    int         len;
    uint        cchRest;

    #define     IDS_ERR         IDS_CXXERR

    if ((*phTM == 0) || (Status != EEGENERAL)) {
        *phError = 0;
        return (EECATASTROPHIC);
    }
    if ((*phError = MHMemAllocate (ERRSTRMAX)) == 0) {
        return (EENOMEMORY);
    }
    else {
        buflen = ERRSTRMAX;
        pBuf = (char *) MemLock (*phError);
        memset (pBuf, 0, buflen);
        pExState = (pexstate_t) MemLock (*phTM);
        if ((err_num = pExState->err_num) != 0) {
            // Start with "C??0000: Error: "
            if ((len = LoadEEMsg (IDS_ERR, pBuf, buflen)) != 0) {
                // Load actual error message.
                len = LoadEEMsg (err_num, pBuf+len, buflen-len);
            }
            DASSERT (len !=0 )
            // Insert error number into the right place
            itoa (err_num, Tempbuf, 10);
            cnt = _tcslen (Tempbuf);
            memcpy (pBuf + 7 - cnt, Tempbuf, cnt );

            // If there's a supplemental error string (e.g. a symbol
            // name), we have to insert it into the middle of the error
            // string, where the "%Fs" is.  NOTE, it's okay to have
            // hErrStr be non-NULL even if the error string does NOT
            // contain "%Fs": this can happen in the case where one
            // EE function set hErrStr and err_num and then returned
            // to another EE function, which then changed err_num to
            // some other error number.

            pPercent = _tcsstr (pBuf, "%Fs");

            if (pPercent != NULL) {
                DASSERT (pExState->hErrStr);
                pErrSymbol = (char *) MemLock (pExState->hErrStr);
                cchErrSymbol = _tcslen (pErrSymbol);

                // Make sure we don't overflow our buffer
                cchAvail = (uint)(ERRSTRMAX - (pPercent - pBuf) - 1);

                cchErrSymbol = min (cchErrSymbol, cchAvail);
                cchAvail -= max(cchErrSymbol,3);
                cchRest = min (cchAvail, _tcslen (pPercent) - 3);

                // Make space for symbol name
                memmove (pPercent + cchErrSymbol, pPercent + 3, cchAvail);
                memcpy (pPercent, pErrSymbol, cchErrSymbol);

                pBuf[ERRSTRMAX-1] = '\0';

                MemUnLock (pExState->hErrStr);
            }
        }
        MemUnLock (*phError);
        MemUnLock (*phTM);
        return (EENOERROR);
    }
}

/***    ErrUnknownSymbol - set the current expr's error to ERR_UNKNOWNSYMBOL
 *
 *      ErrUnknownSymbol (psstr)
 *
 *      Entry   psstr = ptr to an SSTR for the symbol that is unknown
 *
 *      Exit    pExState->err_num and pExState->hErrStr set
 *
 *      Returns Nothing
 *
 */

void ErrUnknownSymbol(LPSSTR lpsstr)
{
    char *  lszSym;

    // Free any old string
    if (pExState->hErrStr) {
        MHMemFree (pExState->hErrStr);
    }

    // Allocate space for symbol string
    pExState->hErrStr = MHMemAllocate (lpsstr->cb + 1);

    if (pExState->hErrStr == 0) {
        // If we couldn't allocate the string, set error to out of mem
        pExState->err_num = ERR_NOMEMORY;
    }
    else {
        // Set error to unknown symbol
        pExState->err_num = ERR_UNKNOWNSYMBOL;

        // Copy the symbol into pExState->hErrStr
        lszSym = (char *) MemLock (pExState->hErrStr);

        memcpy (lszSym, lpsstr->lpName, lpsstr->cb);
        lszSym[lpsstr->cb] = '\0';

        MemUnLock (pExState->hErrStr);
    }
}
