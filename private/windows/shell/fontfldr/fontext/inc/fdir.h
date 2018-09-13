/***************************************************************************
 * fdir.h -- Interface for the class: CFontDir
 *
 *
 * Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
 ***************************************************************************/

#if !defined(__FDIR_H__)
#define __FDIR_H__

#define MAX_DIR_LEN             MAX_PATH  /* MAX_PATH_LEN */

typedef TCHAR DIRNAME[ MAX_DIR_LEN + 1 ];

class CFontDir {
public:
   CFontDir();
   virtual ~CFontDir();

   BOOL     bInit( LPTSTR lpPath, int iLen);
   BOOL     bSameDir( LPTSTR lpStr, int iLen );
   BOOL     bOnSysDir() { return m_bSysDir; };
   VOID     vOnSysDir( BOOL b ) { m_bSysDir = b; };
   LPTSTR   lpString();

private: 
   int      m_iLen;
   BOOL     m_bSysDir;
   DIRNAME  m_oPath;
};

#endif   // __FDIR_H__ 
