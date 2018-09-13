/* function prototypes */

/* card.c */
LONG EditWndProc     (HWND hwnd, UINT message, WPARAM wParam, LONG lParam);
BOOL CardChar        (int ch);
void CardPaint       (HDC hDC);
void DeleteCard      (int iCard);
int AddCurCard       (void);
BOOL SaveCurrentCard (int iCard);
void SetCurCard      (int iCard);
void SetEditText     (TCHAR *pText);
BOOL ScrollCards     (HWND hWindow, int cmd, int pos);
void DoCutCopy       (int event);
void DoPaste         (int event);
void PaintNewHeaders (HDC hDC);

/* dial.c */
BOOL fnDial (HWND hDB, UINT message, WPARAM wParam, LONG lParam);
void DoDial (LPTSTR pchNumber);

/* dragdrop.c */
void EndDragDrop (void);
void DoDragDrop  (HWND hwnd, HANDLE hdrop, BOOL fCard);

/* file.c */
INT TextRead         (HANDLE fh, TCHAR *szBuf, WORD fType);
void AppendExtension (TCHAR *pName, TCHAR *pBuf);
int WriteCurCard     (PCARDHEADER pCardHead, PCARD pCard, TCHAR *pText);
int ReadCurCardData  (PCARDHEADER pCardHead, PCARD pCard, TCHAR *pText);
LPTSTR FileFromPath  (LPTSTR lpStr);
BOOL ExpandHdrs      (int n);
BOOL MyIsTextUnicode   (VOID);

/* find.c */
BOOL SearchLine    (LPTSTR lpLine,TCHAR *szPattern);
BOOL SearchLineAt  (LPTSTR lpLine, TCHAR *szPattern);
void DoGoto        (TCHAR *pBuf);
void ForwardSearch (void);
void ReverseSearch (void);

/* indb.c */
BOOL DlgProc      (HWND hDB, UINT message, WPARAM wParam, LPARAM lParam);
BOOL fnLinksDlg   (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int fnInvalidLink (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL HookProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
TCHAR* Ole2Native (OLECHAR* szBuf, INT num);

/* index.c */
long IndexWndProc     (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
long CardWndProc      (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void SizeListWindow   (void);
void ReleaseClientDoc (void);
void IndexMouse       (HWND hWindow, UINT message, WPARAM wParam, MYPOINT pt);
void IndexPaint       (HDC hDC);

/* init.c */
void GetOldData   (HANDLE hInstance);
BOOL InitInstance (HANDLE hInstance, LPTSTR lpszCommandLine, int cmdShow);
BOOL IndexInit    (void);
VOID SetGlobalFont( HFONT hFont, INT iNewPointSize );
BOOL OleInit      (HANDLE hInstance);
int CallBack      (LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
                   LPOLEOBJECT lpObject);

/* input.c */
DWORD IndexInput        (HWND hWindow, int event);
TCHAR * PutUpDB         (int idb);
void UpdateMenu         (void);
int MapPtToCard         (MYPOINT pt);
DWORD OleMenu           (int event);
void OleMenuItemFix     (HMENU hMenu, int Mode);
void ScrollIndexHorz    (HWND hWindow, int cmd, int pos);
void ScrollIndexVert    (HWND hWindow, int cmd, int pos);
void MenuFileNew        (void);
void MakeTempFile       (void);
void InitPhoneList      (HWND hWindow, int iStartCard);
int CheckForBusyObjects (void);

/* insert.c */
int FAR PASCAL InsertObjectDlgProc (HWND hDlg, UINT msg, WPARAM wParam,
                                    LPARAM lParam);
void InsertObject                  (void);
void SetNumOfCards                 (void);
void FixBounds                     (LPRECT lprc);
BOOL ProcessMessage                (HWND hwndFrame, HANDLE hAccTable);
INT Scale                          (INT coord, INT s1, INT s2);
BOOL IndexOkError                  (int strid);
BOOL MergeStrings                  (LPTSTR lpszSrc, LPTSTR lpszMerge,
                                    LPTSTR lpszDst);
void MakeBlankCard                 (void);
void SetCaption                    (void);
void BuildCaption                  (TCHAR *pchBuf, WORD wLen);
void IndexWinIniChange             (void);
BOOL BuildAndDisplayMsg            (int idError, TCHAR szString[]);
short TranslateString              (TCHAR *src);

/* object.c */
void BMMouse    (HWND hWindow, UINT message, WPARAM wParam, MYPOINT pt);
BOOL BMKey      (WORD wParam);

/* special.c */
int FAR PASCAL PasteSpecialDlgProc (HWND hDlg, UINT message, WPARAM wParam,
                                    LPARAM lParam);
void DoPasteSpecial                (void);

/* picture.c */
void PicDelete         (PCARD pCard);
BOOL PicRead           (PCARD pCard, HANDLE fh, BOOL fOld);
BOOL PicWrite          (PCARD pCard, HANDLE fh, BOOL fForceOld);
BOOL PicDraw           (PCARD pCard, HDC hDC, BOOL fAtOrigin);
void PicCutCopy        (PCARD pCard, BOOL fCut);
void PicPaste          (PCARD pCard, BOOL fPaste, WORD ClipFormat);
HBITMAP MakeObjectCopy (PCARD pCard, HDC hDestDC);
DWORD ReadOldStream    (LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb);
DWORD ReadStream       (LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb);
DWORD WriteStream      (LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb);
DWORD PosStream        (LPCARDSTREAM lpStream, LONG pos);
HBITMAP MakeBitmapCopy (HBITMAP hbmSrc, PBITMAP pBitmap, HDC hDestDC );
BOOL OleError          (OLESTATUS olestat);
BOOL GetNewLinkName    (HWND hwndOwner, PCARD pCard);
void PicSaveUndo       (PCARD pCard);
void ErrorMessage      (int id);
void WaitForObject     (LPOLEOBJECT lpObject);
void PicCreateFromFile (LPTSTR szPackageClass, LPTSTR szDropFile, BOOL fLink);
void Hourglass         (BOOL fOn);
BOOL EditingEmbObject  (PCARD pCard);
int UpdateEmbObject    (PCARD pCard, int Flags);
BOOL InsertObjectInProgress (void);
void DoSetHostNames    (LPOLEOBJECT lpObject, OBJECTTYPE otObject);
void DeleteUndoObject  (void);

/* print.c */
void NEAR FreePrintHandles(void);
void FAR PASCAL PrinterSetupDlg (HWND hwnd);
INT atopix           (TCHAR *ptr, INT pix_per_in);
HDC GetPrinterDC     (void);
HDC SetupPrinting    (BOOL bUseFont);
void FinishPrinting  (HDC hPrintDC);
void PrintList       (void);
void PrintCards      (int count);
void PrintCurCard    (HDC hPrintDC, HDC hMemoryDC, int xPos, int yPos,
                      PCARDHEADER pCardHead, PCARD pCard, HWND hWnd);
int fnAbortProc      (HDC hPrintDC, int iReserved);
int fnAbortDlgProc   (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int PageSetupDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void PrintError      (int iError);

/* read.c */
BOOL CheckCardfileSignature (HANDLE fh, WORD * pfType);
BOOL MaybeSaveFile         (int fSystemModal);
void MenuFileOpen          (void);
int OpenNewFile            (TCHAR szFile[]);
int DoOpen                 (TCHAR *szFile);
void MenuFileMerge         (void);

/* register.c */
void GetClassId    (HWND hwnd, LPTSTR lpstrClass);
int MakeFilterSpec (LPTSTR lpstrClass, LPTSTR lpstrExt, LPTSTR lpstrFilterSpec);

/* write.c */
BOOL MyGetSaveFileName (TCHAR *szFile, WORD *pfType );
int WriteCardFile      (TCHAR *pName, WORD fType);


#define Fdelete(src) !DeleteFile(src)
#define mylmul( n1, n2 ) (LONG)((LONG)(n1) * (LONG)n2 )

/* cardfile.c */
VOID SaveGlobals();
