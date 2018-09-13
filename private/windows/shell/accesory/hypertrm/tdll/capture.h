/*	File: D:\WACKER\tdll\capture.h (Created: 12-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:35p $
 */

#define	CPF_ERR_BASE		0x230
#define	CPF_NO_MEMORY		CPF_ERR_BASE+0x0001
#define	CPF_SIZE_ERROR		CPF_ERR_BASE+0x0002

extern HCAPTUREFILE CreateCaptureFileHandle(const HSESSION hSession);

extern void DestroyCaptureFileHandle(HCAPTUREFILE hCapt);

extern int InitializeCaptureFileHandle(const HSESSION hSession,
										HCAPTUREFILE hCapt);

extern int LoadCaptureFileHandle(HCAPTUREFILE hCapt);

extern int SaveCaptureFileHandle(HCAPTUREFILE hCapt);

extern int cpfGetCaptureFilename(HCAPTUREFILE hCapt,
								LPTSTR pszName,
								const int nLen);

extern int cpfSetCaptureFilename(HCAPTUREFILE hCapt,
								LPCTSTR pszName,
								const int nMode);

#define	CPF_MODE_CHAR		1
#define	CPF_MODE_LINE		2
#define	CPF_MODE_SCREEN		3
#define	CPF_MODE_RAW		4

extern int cpfGetCaptureMode(HCAPTUREFILE hCapt);

extern int cpfSetCaptureMode(HCAPTUREFILE hCapt,
							const int nCaptMode,
							const int nModeFlag);

#define	CPF_FILE_APPEND			1
#define	CPF_FILE_OVERWRITE		2
#define	CPF_FILE_REN_SEQ		3
#define	CPF_FILE_REN_DATE		4

extern int cpfGetCaptureFileflag(HCAPTUREFILE hCapt);

extern int cpfSetCaptureFileflag(HCAPTUREFILE hCapt,
								const int nSaveMode,
								const int nModeFlag);

#define	CPF_CAPTURE_ON			1
#define	CPF_CAPTURE_OFF			2
#define	CPF_CAPTURE_PAUSE		3
#define	CPF_CAPTURE_RESUME		4

extern int cpfGetCaptureState(HCAPTUREFILE hCapt);

extern int cpfSetCaptureState(HCAPTUREFILE hCapt, int nState);

extern HMENU cpfGetCaptureMenu(HCAPTUREFILE hCapt);

#define	CF_CAP_CHARS	CPF_MODE_CHAR
#define	CF_CAP_LINES	CPF_MODE_LINE
#define	CF_CAP_SCREENS	CPF_MODE_SCREEN

extern void CaptureChar(HCAPTUREFILE hCapt, int nFlags, ECHAR cData);

extern void CaptureLine(HCAPTUREFILE hCapt, int nFlags, ECHAR *achStr, int nLen);
