/****************************************************************************
 *
 *  MODULE  : RIFFDISP.H
 *
 ****************************************************************************/

#ifdef OFN_READONLY
    BOOL  FAR PASCAL GetOpenFileNamePreview(LPOPENFILENAME lpofn);
#endif

/****************************************************************************
 ****************************************************************************/

HANDLE FAR PASCAL GetRiffPicture(LPSTR szFile);
BOOL   FAR PASCAL GetRiffTitle(LPSTR szFile, LPSTR szTitle, int iLen);
