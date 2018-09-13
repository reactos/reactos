////////////////////////////////////////////////////////////////////////////////
//
// stmio.h
//
// Property Set Stream I/O and other common Property Set routines.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __stmio_h__
#define __stmio_h__

#include "offcapi.h"
#include "proptype.h"

    // Read a VT_LPSTR from the stream.
  BOOL PASCAL FLpstmReadVT_LPTSTR (LPSTREAM lpStm,
                                     LPTSTR FAR *lplpstz,
                                     BOOL (*lpfnFCPConvert)(LPTSTR, DWORD, DWORD, BOOL),
                                     DWORD dwType);

    // Write a VT_LPSTR to the stream.
  BOOL PASCAL FLpstmWriteVT_LPTSTR (LPSTREAM lpStm,
                                      LPTSTR lpstz,
                                      BOOL fAlign,
                                      DWORD dwType);

    // Write a VT_FILETIME to the stream
  BOOL PASCAL FLpstmWriteVT_FILETIME (LPSTREAM lpStm, LPFILETIME lpFt);

    // Write a VT_I4 to the stream
  BOOL PASCAL FLpstmWriteVT_I4 (LPSTREAM lpStm, DWORD dwI4);

    // Read a VT_CF from the stream.
  BOOL PASCAL FLpstmReadVT_CF (LPSTREAM lpStm, LPSINAIL lpSINail);

    // Write a VT_CF from to the stream.
  BOOL PASCAL FLpstmWriteVT_CF (LPSTREAM lpStm, LPSINAIL lpSINail);

    // Read a VT_I2 from the stream
  BOOL PASCAL FLpstmReadVT_I2 (LPSTREAM lpStm, WORD *pw);

    // Write a VT_I2 to the stream
  BOOL PASCAL FLpstmWriteVT_I2 (LPSTREAM lpStm, WORD w);

    // Read a VT_BOOL from the stream
  BOOL PASCAL FLpstmReadVT_BOOL (LPSTREAM lpStm, WORD *fBool);

    // Write a VT_BOOL to the stream
  BOOL PASCAL FLpstmWriteVT_BOOL (LPSTREAM lpStm, WORD fBool);

    // Read a VT_R8 or VT_DATE from the stream
  BOOL PASCAL FLpstmReadVT_R8_DATE (LPSTREAM lpStm, NUM *dbl);

    // Write a VT_R8 or VT_DATE to the stream
  BOOL PASCAL FLpstmWriteVT_R8_DATE (LPSTREAM lpStm, NUM *dbl, BOOL fDate);

    // Read a VT_BLOB from the stream.
  BOOL PASCAL FLpstmReadVT_BLOB (LPSTREAM lpStm,
                                    DWORD *pcb,
                                    BYTE FAR * FAR *ppbData);

    // Write a VT_BLOB to the stream
  BOOL PASCAL FLpstmWriteVT_BLOB (LPSTREAM lpStm,
                                     DWORD cb,
                                     BYTE *bData);

    // Read a VT_CLSID from the stream
  BOOL PASCAL FLpstmReadVT_CLSID (LPSTREAM lpStm, CLSID *pClsId);

    // Write a VT_CLSID to the stream
  BOOL PASCAL FLpstmWriteVT_CLSID (LPSTREAM lpStm, CLSID *pClsId);

    // Read in unknown data into the array
  BOOL PASCAL FLpstmReadUnknown (LPSTREAM lpStm,
                                    DWORD dwType,
                                    DWORD dwId,
                                    DWORD *pirglpUnk,
                                    LPPROPIDTYPELP rglpUnk);

    // Write out the unknown data in the array.
  BOOL PASCAL FLpstmWriteUnknowns (LPSTREAM lpStm,
                                      DWORD dwcUnk,
                                      LPPROPIDTYPELP rglpUnk);

    // Destroy any unknown data
  BOOL PASCAL FDestoryUnknowns (DWORD dwcUnk, LPPROPIDTYPELP rglpUnk);

    // Write data to the buffer, flushing as needed
  BOOL PASCAL FLpstmWrite (LPSTREAM lpStm,
                              LPVOID lpv,
                              DWORD cb);

void VAllocWriteBuf(void);
void VFreeWriteBuf(void);
BOOL FFlushWriteBuf(LPSTREAM lpStm);
void VSetRealStmSize(LPSTREAM lpStm);
#endif // __stmio_h__
