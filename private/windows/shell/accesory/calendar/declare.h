/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 *   ***** declare.h
 *
 */

#define PT_LEN 50  /* Maximum of Page Setup strings in dialog. */

/* From caldata.c : */
extern BYTE	fInitComplete;
extern BYTE     vrgcDaysMonth [];
extern BOOL     vfDayMode;
extern HANDLE   vhInstance;
extern HBRUSH   vhbrBorder;
extern HBRUSH   vhbrBackMain;
extern HBRUSH   vhbrBackSub;
extern HCURSOR  vhcsrArrow;
extern HCURSOR  vhcsrIbeam;
extern HCURSOR  vhcsrWait;
extern INT      vcxFont;
extern INT      vcxFontMax;
extern INT      vcyFont;
extern INT      vcyDescent;
extern INT      vcyExtLead;
extern INT      vcyLineToLine;
extern INT      vcxBorder;
extern INT      vcxVScrollBar;
extern INT      vcyBorder;
extern INT      vcxHScrollBar;
extern INT      vcyHScrollBar;
extern INT      vcyWnd1;
extern INT      vcyWnd2A;
extern INT      vcyWnd2BTop;
extern INT      vcyWnd2BBot;
extern INT      vcyWnd2B;
extern INT      vcxWnd1;
extern INT      vcxWnd2A;
extern INT      vcxWnd2B;
extern INT      vxcoBell;
extern INT      vcxBell;
extern INT      vcyBell;
extern INT      vxcoApptTime;
extern INT      vxcoAmPm;
extern INT      vxcoQdFirst;
extern INT      vxcoQdMax;
extern INT      vycoQdFirst;
extern INT      vycoQdMax;
extern INT      vxcoDate;
extern INT      vycoNotesBox;
extern INT      vxcoWnd2C;
extern INT      vycoWnd2C;
extern INT      vcln;
extern INT      vlnLast;
extern LD       vtld [];
extern HWND     vhwnd0;
extern HWND     vhwnd1;
extern HWND     vhwnd2A;
extern HWND     vhwnd2B;
extern HWND     vhwnd2C;
extern HWND     vhwnd3;
#ifndef BUG_8560
extern HWND	vhScrollWnd;
#endif
extern D3       vd3Cur;
extern FT       vftCur;
extern WORD     vcMinEarlyRing;
extern BOOL     vfSound;
extern BOOL     vfHour24;
extern INT      vmdInterval;
extern INT      vcMinInterval;
extern TM       vtmStart;
extern FARPROC  vrglpfnDialog [];
extern FARPROC  lpfnMark;
extern INT      vlnCur;
extern FT       vftAlarmNext;
extern FT       vftAlarmFirst;
extern BOOL     vfFlashing;
extern INT      vcAlarmBeeps;
extern BOOL     vfInsert;
extern TM       vtmSpecial;
extern BOOL     vfNoGrabFocus;
extern HANDLE   vhAccel;
extern HWND     vhwndDialog;
extern BOOL     vfMustSyncAlarm;
extern INT      viLeftMarginLen;
extern INT      viRightMarginLen;
extern INT      viTopMarginLen;
extern INT      viBotMarginLen;
extern INT      viCurrentPage;
extern INT      viAMorPM;
extern CHAR     chPageText[6][PT_LEN];
extern CHAR     szDec[5];
extern CHAR     szPrinter[128];
extern BOOL     bPrinterSetupDone;
extern OPENFILENAME vOFN;
extern PRINTDLG vPD;
extern INT	vFilterIndex;
extern INT	vHlpMsg;



/* From caldata2.c : */

extern CHAR     *vrgsz [];
extern D3       vd3Sel;
extern WORD     vwDaySticky;
extern INT      vrgbMonth [];
extern INT      vcDaysMonth;
extern INT      vcWeeksMonth;
extern WORD     vwWeekdayFirst;
extern INT      vrgxcoGrid [];
extern INT      vrgycoGrid [];
extern WORD     votqrPrev;
extern WORD     votqrCur;
extern WORD     votqrNext;
extern WORD     vidrCur;
extern LOCALHANDLE vrghlmDr [];
extern LOCALHANDLE vhlmTdd;
extern INT      vcddAllocated;
extern INT      vcddUsed;
extern DT       vdtFrom;
extern D3       vd3To;
extern DT       vdtTo;
extern INT      vitddFirst;
extern INT      vitddMax;
extern HWND     vhwndFocus;
extern INT      hFile [];
extern OFSTRUCT OFStruct [];
extern BOOL     vfChangeFile;
extern INT      vobkEODChange;
extern CHAR     vszFileSpec [];
extern BOOL     vfOriginalFile;
extern INT      vobkEODNew;
extern BOOL     vfDirty;
extern BYTE     vrgbMagic [];
extern HDC      vhDCMemory;
extern HBITMAP  vhbmLeftArrow;
extern HBITMAP  vhbmRightArrow;
extern HBITMAP  vhbmBell;
extern INT      vxcoLeftArrowFirst;
extern INT      vxcoLeftArrowMax;
extern INT      vxcoRightArrowFirst;
extern INT      vxcoRightArrowMax;
extern char	vszMergeStr [];

extern INT      vmScrollPos;
extern INT      vmScrollInc;
extern INT      vmScrollMax;
extern INT      hmScrollPos;
extern INT      hmScrollMax;
extern INT      hmScrollInc;
extern INT      viMarkSymbol;
extern BOOL     vfOpenFileReadOnly;
extern CHAR	vszFilterSpec [];
extern CHAR	vszCustFilterSpec [];

/* From cal.c : */
BOOL APIENTRY     FCalSize (HWND, INT, INT, INT);
LONG APIENTRY     CalWndProc (HWND, WORD, WPARAM, LONG);
INT  APIENTRY     XcoWnd1 (VOID);
INT  APIENTRY     YcoWnd1 (VOID);
VOID APIENTRY     CalSetFocus (HWND);
VOID APIENTRY     InitMenuItems (VOID);
VOID APIENTRY     CalWinIniChange(VOID);


/* From cal2.c : */
VOID APIENTRY     CalPaint (HWND, HDC);
VOID APIENTRY     DrawArrow (HBITMAP, INT);
VOID APIENTRY     DrawArrowBorder (INT);
VOID APIENTRY     DispTime (HDC);
INT  APIENTRY     GetTimeSz (TM, CHAR *);
VOID APIENTRY     ByteTo2Digs (BYTE, CHAR *);
CHAR * APIENTRY   CopySz (CHAR *, CHAR *);
VOID APIENTRY     DispDate (HDC, D3 *);
VOID APIENTRY     GetDateDisp (D3 *, CHAR *);
BYTE * APIENTRY   FillBuf (BYTE *, INT, BYTE);
CHAR * APIENTRY   WordToASCII (WORD, CHAR *, BOOL);
VOID APIENTRY     GetDashDateSel (CHAR *);
BOOL APIENTRY     FGetTmFromTimeSz (CHAR *, TM *);
VOID APIENTRY     SkipSpace (CHAR **);
BOOL APIENTRY     FGetWord (CHAR **, WORD *);
CHAR APIENTRY     ChUpperCase (CHAR);
BOOL APIENTRY     FDigit (CHAR);
BOOL APIENTRY     FD3FromDateSz (CHAR *, D3 *);
VOID APIENTRY     GetD3FromDt (DT, D3 *);
VOID APIENTRY     SetEcText(HWND, CHAR *);


/* From calcolor.c : */
BOOL APIENTRY     CreateBrushes (VOID);
VOID APIENTRY     DestroyBrushes (VOID);
VOID APIENTRY     PaintBack (HWND, HDC);
HDC  APIENTRY     CalGetDC (HWND);
VOID APIENTRY     SetDefaultColors (HDC);
VOID APIENTRY     DrawAlarmBell (HDC, INT);


/* From calmonth.c : */
WORD APIENTRY     GetWeekday (D3 *);
INT  APIENTRY     CDaysMonth (D3 *);
VOID APIENTRY     SetUpMonth (VOID);
VOID APIENTRY     BuildMonthGrid (VOID);
VOID APIENTRY     PaintMonthGrid (HDC);
VOID APIENTRY     PaintMonth (HDC);
VOID APIENTRY     DrawMark (HDC, INT, INT, INT, INT);
VOID APIENTRY     ShowToday (HDC, INT, INT, INT);
VOID APIENTRY     InvertDay (HDC, WORD);
VOID APIENTRY     PositionCaret (VOID);
VOID APIENTRY     MapDayToRect (WORD, RECT *);
BOOL APIENTRY     FMapCoToIGrid (INT, INT *, INT, INT *);
DT APIENTRY       DtFromPd3 (D3 *);
VOID APIENTRY     GetMarkedDays (VOID);
VOID APIENTRY     MonthMode (VOID);


/* From calmon2.c : */
BOOL APIENTRY     FMonthPrev (VOID);
BOOL APIENTRY     FMonthNext (VOID);
VOID APIENTRY     ShowMonthPrevNext (BOOL);
VOID APIENTRY     UpdateMonth (VOID);
VOID APIENTRY     MouseSelectDay (MPOINT, BOOL);
VOID APIENTRY     FScrollMonth (INT, WORD);
VOID APIENTRY     FHorizScrollMonth (INT, WORD); /* added 11/3/88 for hscroll */
BOOL APIENTRY     FCalKey (HWND, WPARAM);
VOID APIENTRY     MoveSelCurMonth (WORD);
VOID APIENTRY     InvalidateMonth (VOID);
VOID APIENTRY     MoveSelNewMonth (WORD);
VOID APIENTRY     JumpDate (D3 *);
BOOL APIENTRY     FFetchTargetDate (VOID);


/* From calday.c : */
VOID APIENTRY     DayMode (D3 *);
VOID APIENTRY     SwitchToDate (D3 *);
VOID APIENTRY     DayPaint (HDC);
VOID APIENTRY     FillTld (TM);
VOID APIENTRY     ScrollDownTld (INT);
BOOL APIENTRY     FGetNextLd (TM, LD *);
BOOL APIENTRY     FGetPrevLd (TM, LD *);
BOOL APIENTRY     FScrollDay (INT, WORD);
VOID APIENTRY     ScrollUpDay (INT, BOOL);
VOID APIENTRY     ScrollDownDay (INT, BOOL, BOOL);
VOID APIENTRY     InvalidateParentQdEc (INT);
INT  APIENTRY     YcoFromLn (INT);
INT  APIENTRY     LnFromYco (INT);
VOID APIENTRY     SetQdEc (INT);


/* From calday2.c : */
VOID APIENTRY     SetDayScrollRange (VOID);
VOID APIENTRY     AdjustDayScrollRange (INT);
VOID APIENTRY     SetDayScrollPos (INT);
VOID APIENTRY     AdjustDayScrollPos (INT);
INT  APIENTRY     ItmFromTm (TM);
TM   APIENTRY     TmFromItm (INT);
VOID APIENTRY     MapTmAndItm (TM *, INT *);
TM   APIENTRY     TmFromQr (PQR*, PQR);
TM   APIENTRY     TmNextRegular (TM);


/* From caltqr.c : */

BOOL APIENTRY     FSearchTqr (TM);
VOID APIENTRY     StoreQd (VOID);
VOID APIENTRY     AdjustOtqr (INT, INT);
VOID APIENTRY     DeleteQr (WORD);
BOOL APIENTRY     FInsertQr (WORD, PQR);
BYTE * APIENTRY   PbTqrLock (VOID);
DR   * APIENTRY   PdrLockCur (VOID);
VOID APIENTRY     DrUnlockCur (VOID);
DR   * APIENTRY   PdrLock (WORD);
VOID APIENTRY     DrUnlock (WORD);
BYTE * APIENTRY   PbTqrFromPdr (DR *);
VOID APIENTRY     StoreNotes (VOID);
VOID APIENTRY     SetNotesEc (VOID);
VOID APIENTRY     EcNotification (WORD, WORD);
VOID APIENTRY     PruneEcText (VOID);


/* From calrem.c : */
BOOL APIENTRY     FnRemove (HWND, WORD, WPARAM, LONG);
VOID APIENTRY     Remove (VOID);
VOID APIENTRY     HourGlassOn (VOID);
VOID APIENTRY     HourGlassOff (VOID);


/* From calcmd.c : */
VOID APIENTRY     CalCommand (HWND, INT);
BOOL APIENTRY     FDoDialog (INT);


/* From calcmd2.c : */
BOOL APIENTRY     FnSaveAs (HWND, WORD, WPARAM, LONG);
VOID APIENTRY     GetRangeOfDates (HWND);
BOOL APIENTRY     FnDate (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY     FnControls (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY     FnSpecialTime (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY     FnPageSetup (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY     ProcessDlgText(BOOL);
INT  APIENTRY     ChangeToPM ( TM *);
BOOL APIENTRY     FnDaySettings (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY     FnMarkDay ( HWND, WORD, WPARAM, LONG); /* added 11/8/88 */
INT  APIENTRY     cDlgfnOpen ( HWND, WORD, WPARAM, LONG);
INT  APIENTRY     cDlgOpenFile (HANDLE, HWND, INT, CHAR *, INT,
                                CHAR *, CHAR * , INT);
BOOL APIENTRY     cDlgCheckFilename (CHAR *);  /* removed from ..\common\dlgopen.c */
VOID APIENTRY     cDlgCheckOkEnable (HWND, INT, WORD);
BOOL APIENTRY     cIsChLegal (INT);
BOOL APIENTRY     cFSearchSpec (CHAR *);
INT  APIENTRY     AlertBox (CHAR *, CHAR *, WORD);
VOID APIENTRY     ConvertUpperSz (CHAR *);
VOID APIENTRY     AddDefExt (LPSTR);
VOID APIENTRY     CheckButtonEnable (HWND, INT, WORD);
BOOL APIENTRY     FCheckSave (BOOL);
VOID APIENTRY     RecordEdits (VOID);
VOID APIENTRY     DateTimeAlert(BOOL, INT);
BOOL APIENTRY     MergeStrings();
BOOL APIENTRY	  CallSaveAsDialog ();



/* From calmark.c : */
VOID APIENTRY     CmdMark (VOID);


/* From caltdd.c : */
VOID APIENTRY     InitTdd (VOID);
BOOL APIENTRY     FSearchTdd (DT, INT *);
BOOL APIENTRY     FGrowTdd (INT, INT);
VOID APIENTRY     ShrinkTdd (INT, INT);
BYTE * APIENTRY   BltByte (BYTE *, BYTE *, WORD);
VOID APIENTRY     DeleteEmptyDd (INT);
DD   * APIENTRY   TddLock (VOID);
VOID APIENTRY     TddUnlock (VOID);


/* From calfile.c : */
VOID APIENTRY     CreateChangeFile (VOID);
VOID APIENTRY     DeleteChangeFile (VOID);
BOOL APIENTRY     FCreateTempFile (INT, INT);
BOOL APIENTRY     FFreeUpDr (DR *, DL *);
BOOL APIENTRY     FWriteDrToFile (BOOL, INT, DR *);
BOOL APIENTRY     FReadDrFromFile (BOOL, DR *, DL);
BOOL APIENTRY     FGetDateDr (DT);


/* From calfile2.c : */
BOOL APIENTRY     FCopyToNewFile (INT, DR *, DD *, DD *);
BOOL APIENTRY     FSaveFile (CHAR *, BOOL);
VOID APIENTRY     Reconnect (BOOL);
INT  APIENTRY     GetDrive (CHAR *);
CHAR * APIENTRY   PchFileName (CHAR *);
BOOL APIENTRY     FFlushDr (VOID);
BOOL APIENTRY     FCloseFile (INT);
BOOL APIENTRY     FWriteHeader (DD *);
BOOL APIENTRY     FWriteFile (INT, BYTE *, WORD);
BOOL APIENTRY     FDeleteFile (INT);
BOOL APIENTRY     FReopenFile (INT, WORD);
VOID APIENTRY     SetTitle (CHAR *);
BOOL APIENTRY     FCondClose (BOOL, BOOL);
VOID APIENTRY     CleanSlate (BOOL);
VOID APIENTRY     OpenCal (VOID);
VOID APIENTRY     LoadCal (VOID);



/* From calalarm.c : */
BOOL APIENTRY     FAlarm (INT);
VOID APIENTRY     AlarmToggle (VOID);
VOID APIENTRY     uProcessAlarms (VOID);
BOOL APIENTRY     FnAckAlarms (HWND, WORD, WPARAM, LONG);
VOID APIENTRY     GetNextAlarm (FT *, FT *, BOOL, HWND);
WORD APIENTRY     IdrFree (VOID);
VOID APIENTRY     ReadTempDr (WORD, DL);
VOID APIENTRY     StartStopFlash (BOOL);



/* From calspecl.c : */
VOID APIENTRY     InsertSpecial (VOID);
VOID APIENTRY     DeleteSpecial (VOID);
VOID APIENTRY     SpecialTimeFin (VOID);


/* From calprint.c : */
BOOL APIENTRY     FnPrint (HWND, WORD, WPARAM, LONG);
VOID APIENTRY     Print (VOID);
BOOL APIENTRY     PrintDate (INT, DT, BOOL);
BOOL APIENTRY     PrintHeading (DT);
BOOL APIENTRY     PrintBlankLn (INT);
BOOL APIENTRY     PrintLine (VOID);
BOOL APIENTRY     NewPage (VOID);
BOOL APIENTRY     PrintHeaderFooter(BOOL);
INT  APIENTRY     BeginPrint (VOID);
VOID APIENTRY     EndPrint (VOID);
INT  APIENTRY     FnProcAbortPrint (HDC, INT);
INT  APIENTRY     FnDlgAbortPrint (HWND, WORD, WPARAM, LONG);
VOID APIENTRY     CalPrintAlert(INT);
INT               atopix(CHAR *, INT);


/* From calinit.c : */
BOOL APIENTRY     CalInit (HANDLE, HANDLE, LPSTR, INT);
BOOL APIENTRY     AllocDr ();
BOOL APIENTRY     CalTerminate(INT);
BOOL APIENTRY     LoadBitmaps(HANDLE);
VOID APIENTRY     DeleteBitmaps(VOID);



/* From calmain.c */
BOOL APIENTRY     FKeyFiltered (MSG *);
VOID APIENTRY     CalTimer (BOOL);
VOID APIENTRY     AlarmCheck (VOID);
VOID APIENTRY     AddMinsToFt (FT *, WORD);
INT  APIENTRY     CompareFt (FT *, FT *);


/* From callib.asm : */
VOID APIENTRY     ReadClock(D3 *pd3, TM *ptm);
INT  APIENTRY     FDosDelete(LPSTR lpszFileToDelete);
INT  APIENTRY     FDosRename(LPSTR lpszOrgFileName, LPSTR lpszNewFileName);
INT  APIENTRY     GetCurDrive (VOID);
LONG APIENTRY     mylmul(INT, INT);


/* From common.h */
/* include file for common routines */

INT APIENTRY cDlgOpen(HANDLE, HWND, INT, INT, INT, INT,
		      CHAR *, INT, CHAR *, OFSTRUCT *, INT *);

BOOL APIENTRY   cDlgCheckFileName(CHAR *);
VOID APIENTRY   DlgCheckOkEnable(HWND, INT, WORD);
INT  APIENTRY   GetPrinterDC(VOID);
VOID APIENTRY   DlgInitSaveAs(HWND, INT, INT, INT, LPOFSTRUCT);
CHAR * APIENTRY PFileInPath(CHAR *);
CHAR * APIENTRY Int2Ascii (INT, CHAR *, BOOL);
