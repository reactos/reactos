

/*were  not defined but called from initcode.c */

BOOL PrintFileInit();
int PrintFileShutDown ();
VOID sizeFkeys(LONG clientSize);
VOID initChildSize(RECT *pRect);
VOID setDefaultAttrib(BOOL bLoad);
VOID initDlgPos(HWND hDlg);
VOID initComDevSelect(HWND hDlg, WORD wListID, BOOL bInit);
BYTE getComDevSelect(HWND hDlg, WORD wListID, BYTE *newDevRef);
BOOL getFileType(BYTE *fileName, BYTE *fileExt);
VOID taskInit();
VOID keyMapInit();
BOOL termInitSetup(HANDLE hPrevInstance);
VOID forceExtension(BYTE *fileName, BYTE *fileExt, BOOL bReplace);
BOOL termFile(BYTE *filePath,BYTE *fileName,BYTE *fileExt,BYTE *title,WORD flags);
VOID sizeTerm(LONG termSize);
VOID keyMapCancel();

/*****************/

/* were not defined but called from winmain.c */

VOID xSndBFile();
VOID xRcvBFile();

/**************/

/* were not defined but called from winmain.c*/

int myDrawIcon(HDC hDC, BOOL bErase);
VOID PrintFileComm(BOOL bPrint);
BOOL termCloseAll(VOID);
int flashIcon(BOOL bInitFlash, BOOL bEndProc);
WORD childZoomStatus(WORD wTest, WORD wSet);
VOID initMenuPopup(WORD menuIndex);
BOOL keyMapTranslate(WORD *wParam, LONG *lParam, TERM_STRING *mapStr);
BOOL fKeyStrBuffer(BYTE *str,WORD  len);
BOOL keyMapSysKey(HWND hWnd, WORD *wParam, LONG lParam);
VOID longToPoint(long sel, POINT *pt);

VOID keyMapKeyProc(HWND hWnd, WORD wParam, LONG lParam);
BOOL termCloseFile(VOID);
VOID hpageScroll(int which);



VOID PrintFileString(LPSTR lpchr,LONG  count, BOOL bCRtoLF);
BOOL PrintFileOn(HANDLE theInstance,HWND theWnd,
LPSTR thePrintName,LPSTR thePrintType,LPSTR thePrintDriver,
LPSTR thePrintPort,BOOL showDialog);
BOOL PrintFileOff();
int PrintFileLineFeed (BOOL nextLine);
int PrintFilePageFeed ();


BOOL termSaveFile(BOOL bGetName);


int testMsg(BYTE *str, int arg0,int  arg1,int  arg2, int arg3,int  arg4,int  arg5,int  arg6, int arg7,
                 int arg8, int arg9, int arga, int argb, int argc, int argd, int arge, int argf);


VOID xferStopBreak(BOOL bStop);

VOID xferPauseResume(BOOL bPause, BOOL bResume);

int selectFKey(WORD wIDFKey);

BOOL sendKeyInput(BYTE theByte);

VOID sndAbort  ();

int countChildWindows(BOOL bUnzoom);

VOID stripBlanks (LPBYTE ptr, DWORD *len);

VOID doFileNew();

VOID doFileOpen();
VOID doFileClose();
VOID doFileSave();
VOID doFileSaveAs();

VOID stripControl(TERM_STRING *str);
int TF_ErrProc(WORD, WORD, WORD);


BOOL XM_RcvFile(WORD);
BOOL FAR KER_Receive(BOOL bRemoteServer);
VOID listFontSizes(BYTE *faceName, BYTE *sizeList, int maxSize);

int updateIcon();
BOOL XM_SndFile(WORD);
BOOL FAR KER_Send();
VOID setAppTitle();


VOID icsResetTable(WORD icsType);
VOID rcvFileErr();
