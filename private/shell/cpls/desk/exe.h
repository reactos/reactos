/*
 * EXE.H - read windows exe headers
 *               
 */

/* Attempt to get information of type fInfo on the file szFile, depositing
    the information (to a maximum of nBuf characters) in pBuf.  If BOOL fError
    is set, then display error messages when something goes wrong... */

DWORD FAR PASCAL GetExeInfo(LPTSTR szFile, void FAR *pBuf, int cbBuf, UINT fInfo);

#define GEI_MODNAME         0x01
#define GEI_DESCRIPTION     0x02
#define GEI_FLAGS           0x03
#define GEI_EXEHDR          0x04
#define GEI_FAPI            0x05
#define GEI_EXPVER          0x06

#define PEMAGIC         0x4550  /* 'PE' */
#define NEMAGIC         0x454E  /* 'NE' */
