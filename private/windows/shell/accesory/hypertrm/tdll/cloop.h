/*	File: cloop.h (created 12/27/93, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

/* --- Constants --- */

// Values for usAction argument of CLoopRcvControl & CLoopSndControl
#define CLOOP_SUSPEND 1
#define CLOOP_RESUME  0

// Values for usAction argument of CLoopControl
#define CLOOP_SET	  1
#define CLOOP_CLEAR   0

// Values to return from chain functions
#define CLOOP_KEEP          0
#define CLOOP_DISCARD       1

// usReason values for CLoopRcvControl()
#define CLOOP_RB_NODATA 	 0x0001
#define CLOOP_RB_INACTIVE	 0x0002
#define CLOOP_RB_SCRLOCK	 0x0004
#define CLOOP_RB_SCRIPT 	 0x0008
#define CLOOP_RB_TRANSFER	 0x0010
#define CLOOP_RB_PRINTING	 0x0020 		// see PrintAbortProc (prnecho.c)
#define	CLOOP_RB_CNCTDRV	 0x0040

// usReason values for CLoopSndControl()
#define CLOOP_SB_NODATA 	 0x0001
#define CLOOP_SB_INACTIVE	 0x0002
#define CLOOP_SB_SCRLOCK	 0x0004
#define CLOOP_SB_LINEWAIT	 0x0008
#define CLOOP_SB_PRINTING	 0x0010 		// see PrintAbortProc (prnecho.c)
#define CLOOP_SB_DELAY		 0x0020
#define CLOOP_SB_UNCONNECTED 0x0040
#define	CLOOP_SB_CNCTDRV	 0x0080

// usReason values for CLoopControl()
#define CLOOP_TERMINATE 	 0x0001
#define CLOOP_TRANSFER_READY 0x0002
#define CLOOP_CONNECTED 	 0x0004
#define	CLOOP_MBCS			 0x0008
#define CLOOP_SUPPRESS_DSP	 0x8000

// usOptions values for CLoopSend()
#define CLOOP_KEYS			 0x0001
#define CLOOP_ALLOCATED 	 0x0002
#define CLOOP_SHARED		 0x0004
#define CLOOP_GLBL_ALLOCATED 0x0008 // tell cloop to free with GlobalFree()

/* --- Typedefs --- */

// Support for remote input chaining functions
typedef int (*CHAINFUNC)(ECHAR, void *);


/* --- Function Prototypes --- */
extern HCLOOP CLoopCreateHandle(const HSESSION hSession);
extern void   CLoopDestroyHandle(HCLOOP * const ppstCLoop);
extern int	  CLoopActivate(const HCLOOP hCLoop);
extern void   CLoopDeactivate(const HCLOOP hCLoop);
extern void   CLoopReset(const HCLOOP hCLoop);
extern void   CLoopRcvControl(const HCLOOP hCLoop,
					const unsigned uAction,
					const unsigned uReason);
extern void   CLoopOverrideControl(const HCLOOP hCLoop, const int fOverride);
extern void   CLoopSndControl(const HCLOOP hCLoop,
					const unsigned uAction,
					const unsigned uReason);
extern void   CLoopControl(const HCLOOP hCLoop,
						  unsigned uAction,
						  unsigned uReason);
extern void * CLoopRegisterRmtInputChain(const HCLOOP hCLoop,
					const CHAINFUNC pfFunc,
						  void *pvUserData);
extern void   CLoopUnregisterRmtInputChain(void *pvHdl);

extern int	  CLoopLoadHdl(const HCLOOP hCLoop);
extern int	  CLoopSaveHdl(const HCLOOP hCLoop);
extern int	  CLoopGetSendCRLF(const HCLOOP hCLoop);
extern void   CLoopSetSendCRLF(const HCLOOP hCLoop, const int fSendCRLF);
extern int	  CLoopGetExpandBlankLines(const HCLOOP hCLoop);
extern void   CLoopSetExpandBlankLines(const HCLOOP hCLoop,
					const int fExpandBlankLines);
extern int	  CLoopGetLocalEcho(const HCLOOP hCLoop);
extern void   CLoopSetLocalEcho(const HCLOOP hCLoop, const int fLocalEcho);
extern int	  CLoopGetLineWait(const HCLOOP hCLoop);
extern void   CLoopSetLineWait(const HCLOOP hCLoop, const int fLineWait);
extern TCHAR  CLoopGetWaitChar(const HCLOOP hCLoop);
extern void   CLoopSetWaitChar(const HCLOOP hCLoop, TCHAR chWaitChar);
extern int	  CLoopGetExpandTabsOut(const HCLOOP hCLoop);
extern void   CLoopSetExpandTabsOut(const HCLOOP hCLoop,
					const int fExpandTabsOut);
extern int	  CLoopGetTabSizeOut(const HCLOOP hCLoop);
extern void   CLoopSetTabSizeOut(const HCLOOP hCLoop, const int nTabSizeOut);
extern int	  CLoopGetLineDelay(const HCLOOP hCLoop);
extern void   CLoopSetLineDelay(const HCLOOP hCLoop, const int nLineDelay);
extern int	  CLoopGetCharDelay(const HCLOOP hCLoop);
extern void   CLoopSetCharDelay(const HCLOOP hCLoop, const int nCharDelay);
extern int	  CLoopGetAddLF(const HCLOOP hCLoop);
extern void   CLoopSetAddLF(const HCLOOP hCLoop, const int fAddLF);
extern int	  CLoopGetASCII7(const HCLOOP hCLoop);
extern void   CLoopSetASCII7(const HCLOOP hCLoop, const int fASCII7);
extern int	  CLoopGetEchoplex(const HCLOOP hCLoop);
extern void   CLoopSetEchoplex(const HCLOOP hCLoop, const int fEchoplex);
//extern int	CLoopGetWrapLines(const HCLOOP hCLoop);
//extern void	CLoopSetWrapLines(const HCLOOP hCLoop, const int fWrapLines);
extern int	  CLoopGetShowHex(const HCLOOP hCLoop);
extern void   CLoopSetShowHex(const HCLOOP hCLoop, const int fShowHex);
extern int	  CLoopGetTabSizeIn(const HCLOOP hCLoop);
extern void   CLoopSetTabSizeIn(const HCLOOP hCLoop, const int nTabSizeIn);

extern int SetCLoopMBCSState(HCLOOP hCLoop, int nState);
extern int QueryCLoopMBCSState(HCLOOP hCLoop);

extern void   CLoopCharOut(HCLOOP hCLoop, TCHAR chOut);
extern void CLoopBufrOut(HCLOOP hCLoop, TCHAR *pchOut, int nLen);


extern int	  CLoopSend
					(
					const HCLOOP	hCLoop,
						  void	   *pvData,
					const size_t	sztItems,
						  unsigned	uOptions);
extern int	  CLoopSendTextFile(const HCLOOP hCLoop, TCHAR *pszFileName);
extern void   CLoopClearOutput(const HCLOOP hCLoop);
extern unsigned long CLoopGetOutputCount(const HCLOOP pstCLoop);
extern int 	  CLoopInitHdl(const HCLOOP pstCLoop);
