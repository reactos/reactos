/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/

typedef LONG (APIENTRY *TRACKPROC)(HDC, LPRECT, WPARAM);

/* FILE: abortdlg.c */
BOOL PUBLIC AbortDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: abortprt.c */
BOOL PUBLIC AbortPrt(HDC printDC, short code);

/* FILE: airbrudp.c */
void AirBruDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: allocimg.c */
BOOL AllocTemp(int wid, int hgt, int planes, int pixelBits, BOOL f24PCX);

/* FILE: brushdlg.c */
BOOL FAR PASCAL BrushDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: brushdp.c */
void BrushDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: calcview.c */
void CalcView(void);

/* FILE: calcwnds.c */
void CalcWnds(int disptools, int displine, int dispcolor, int disppaint);
int recalc(REGISTER int source, REGISTER int multip);

/* FILE: cleardlg.c */
BOOL FAR PASCAL ClearDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: coleradp.c */
void ColEraDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: colordlg.c */
BOOL FAR PASCAL ColorDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: colorwp.c */
void DrawMonoRect(HDC hDC, int left, int top, int right, int bottom);
long FAR PASCAL ColorWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);
void PUBLIC PickupColor(void);

/* FILE: curvedp.c */
void CurveDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: dotparal.c */
void DotParal(HDC dc, PARAL *p);

/* FILE: dotpoly.c */
void DotPoly(HDC dc, POINT *polyPts, int numPts);

/* FILE: dotrect.c */
void DotRect(HDC dc, int x1, int y1, int x2, int y2);

/* FILE: eraserdp.c */
void EraserDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: filedlg.c */
WORD PBGetFileType(LPTSTR lpFilename);
void ReplaceExtension(LPTSTR lpFilename, int iFileType);

/* FILE: fixedpt.c */
LONG NumToPels(LONG lNum, BOOL bHoriz, BOOL bInches);
LONG PelsToNum(LONG lNum, BOOL bHoriz, BOOL bInches);
BOOL StrToNum(LPTSTR num, LONG FAR *lpNum);
BOOL GetDlgItemNum(HWND hDlg, int nItemID, LONG FAR * lpNum);
BOOL NumToStr(LPTSTR num, LONG lNum, BOOL bDecimal);
BOOL SetDlgItemNum(HWND hDlg, int nItemID, LONG lNum, BOOL bDecimal);

/* FILE: flippoly.c */
void FlipPoly(POINT *polyPts, int numPts, int dir, RECT *r);
void OffsetPoly(NPPOINT polyPts, int numPts, int xOffset, int yOffset);

/* FILE: freeimg.c */
void FreeTemp(void);

/* FILE: freepick.c */
void FreePick(void);

/* FILE: fullwp.c */
long FAR PASCAL FullWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: getaspct.c */
void GetAspct(int size, int *h, int *v);

/* FILE: getinfo.c */
BOOL GetInfo(HWND hWnd);
BOOL GetBitmapFileInfo(HWND hWnd, LPTSTR npszFileName);
BOOL GetBitmapInfo(HWND hWnd);

/* FILE: getprtdc.c */
HDC GetPrtDC(void);
HDC GetDisplayDC(HWND hWnd);

/* FILE: gettanpt.c */
void GetTanPt(int wid, int hgt, int delX, int delY, RECT *tan);

/* FILE: hidecsr.c */
void HideCsr(HDC dc, HWND hWnd, int csr);

/* FILE: infodlg.c */
BOOL FAR PASCAL InfoDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: initglob.c */
void SetupFileVars(
    LPTSTR szPath);
LPTSTR lstrtok(LPTSTR lpStr, LPTSTR lpDelim);
BOOL WndInitGlob(HINSTANCE hInst, LPTSTR lpCmdLine, int cmdShow);

/* FILE: lcundodp.c */
void LcUndoDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: linedp.c */
void LineDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: loadbit.c */
BOOL LoadBitmapFile(HWND hWnd, LPTSTR lpFilename, LPBITMAPINFO lpInfoHeader);

/* FILE: loadcolr.c */
BOOL LoadColr(HWND hWnd, LPTSTR fileName);

/* FILE: loadimg.c */
BOOL BuildPalette(HANDLE fh, LPBITMAPINFO lpDIBinfo, DHDR hdr, int nPlanes);
void PlanarToChunky(LPBYTE lpDIBits, LPBYTE lpRedBuf, LPBYTE lpGreenBuf, LPBYTE lpBlueBuf, LPBYTE lpIntBuf, int bpline);
void Decode2Buf(LPBYTE where, LPBYTE src, int many);
BOOL LoadMSP2Img(HWND hWnd);
BOOL LoadMSPImg(HWND hWnd);
BOOL LoadImg(HWND hWnd, LPTSTR lpFilename);
BOOL GetMSPInfo(HWND hWnd);

/* FILE: menucmd.c */
void PUBLIC EnablePaintMenuItems(HMENU hMenu, BOOL bEnable);
void PUBLIC Help(HWND hWnd, UINT wCommand, LONG lParam);
BOOL FAR PASCAL NullWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);
void PUBLIC MenuCmd(HWND hWnd, UINT item);
void PUBLIC Terminatewnd(void);
WORD PUBLIC GetImageFileType(int Pixels);
BOOL SaveFileNameOK(TCHAR szPath[], TCHAR szFile[]);
BOOL FAR PASCAL ObjectUpdateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: message.c */
WORD SimpleMessage(WORD StringId, LPTSTR lpText, WORD style);
void PbrushOkError(WORD StringId, WORD style);
#ifdef BLOCKONMSG
void EnterMsgMode(void);
#endif

/* FILE: metafile.c */
BOOL GetMFDimensions(HANDLE hMF, HDC hDC, int *pWidth, int *pHeight);
BOOL PlayMetafileIntoDC(HANDLE hMF, RECT *pRect, HDC hDC);

/* FILE: mousedlg.c */
BOOL FAR PASCAL MouseDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: newpick.c */
int AlocPick(HDC hDC);
void EnablePickMenu(BOOL bEnable);
void BuildMask(int mode);
LONG APIENTRY DrawDotRect(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
LONG APIENTRY DrawShrGroRect(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
LONG APIENTRY DrawDotPoly(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
LONG APIENTRY DragProc(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void EnableOutline(BOOL bEnable);
BOOL IsInSelection(POINT pt);
void FillBkgd(HDC hDC, int x, int y, int dx, int dy);
void ClearPickArea(void);
int PickDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: offspoly.c */
void OffsPoly(POINT *polyPts, int numPts, int x, int y);

/* FILE: ovaldp.c */
LONG APIENTRY DrawOval(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void OvalDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: packbuff.c */
BOOL PackBuff(BYTE *buff, int row, int byteWid, HANDLE fh);

/* FILE: paintwp.c */
void SetNewproc(DPPROC);
long FAR PASCAL PaintWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: parentwp.c */
long FAR PASCAL ParentWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);
BOOL FAR SaveAsNeeded(void);

/* FILE: polydp.c */
LONG APIENTRY DrawPoly(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void PolyDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: polyrect.c */
void PolyRect(LPPOINT polyPts, int numPts, LPRECT dstRect);

/* FILE: polyto.c */
void PolyTo(POINT fromPt, POINT toPt, int wid, int hgt);

/* FILE: printdlg.c */
LPTSTR lstrstr(LPTSTR lpStr1, LPTSTR lpStr2);
BOOL GetPrintParms(HDC printDC);
void PUBLIC ComputePrintRect(NPRECT lpImageRect, NPRECT lpPrintRect, NPRECT lpHeaderRect, NPRECT lpFooterRect);
#ifndef PRINTERDLG
BOOL FAR PASCAL PrintFileDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);
#endif
BOOL FAR PASCAL PrinterSetDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);
BOOL FAR PASCAL PageSetDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);

/* FILE: printdp.c */
LONG APIENTRY DrawPrint(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void PrintDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: printimg.c */
void PrintImg(NPRECT imageRect, BOOL draft, int copies);
LPTSTR PFileInPath(LPTSTR sz);

/* FILE: ptools.c */
HBITMAP PUBLIC CropBitmap(HBITMAP hbm, PRECT prc, BOOL fScale);
HBITMAP PUBLIC CopyBitmap(HBITMAP hbm);
HPALETTE PUBLIC CopyPalette(HPALETTE hPal);
BOOL PUBLIC DumpBitmapToClipboard(HDC hDC, UINT cmd, RECT Rect);
HBITMAP PUBLIC CreatePatternBM(HDC hDC, DWORD color);
void PUBLIC ConstrainRect(LPRECT lprBounds, LPRECT lprConst, WPARAM wParam);
void PUBLIC ConstrainBrush(LPRECT lprBounds, WPARAM wParam, int *dir);
int PUBLIC DoDialog(int id, HWND hwnd, WNDPROC theProc);
void PUBLIC CenterWindow(HWND hWnd);
#ifdef WIN16
int PUBLIC changeDiskDir(LPTSTR newDir);
#else
#define changeDiskDir  SetCurrentDirectory
#endif
BOOL PUBLIC bFileExists(LPTSTR lpFilename);
BOOL PUBLIC bValidFilename(LPTSTR lpFilename);
void PUBLIC DlgCheckOkEnable(HWND hwnd, int idEdit, UINT message);
void PUBLIC MakeValidFilename(LPTSTR s, LPTSTR lpszDefExtension);
BOOL PUBLIC IsReadOnly(LPTSTR lpFilename);
WORD PUBLIC PaletteSize(VOID FAR * pv);
HPALETTE PUBLIC CreateDibPalette(HANDLE hbi);
HPALETTE PUBLIC CreateBIPalette(LPBITMAPINFOHEADER lpbi);
WORD PUBLIC DibNumColors(VOID FAR * pv);
void PUBLIC TripleToQuad(LPBITMAPINFO lpDIBinfo, BOOL bSwap);
HPALETTE PUBLIC MakeImagePalette(HPALETTE hPal, HANDLE hDIBinfo, UINT FAR *wUsage);
void PUBLIC NormalizeRect(LPRECT lpRect);
BOOL PUBLIC MaskStretchBlt(HDC hDCD, int x, int y, int dx, int dy, HDC hDCS, HDC hDCSMask, int x0, int y0, int dx0, int dy0);
void PUBLIC EnableMenuItems(HMENU hMenu, UINT MenuItems[], UINT wFlags);
DWORD FAR INtoM(int in);  /* NOTE -- moved from pbserver.c */

/* FILE: qutil.asm */
#ifdef WIN32
#define RepeatMove  memcpy
#define RepeatFill  memset

#ifndef getcwd
#define getcwd(b,l)  GetCurrentDirectory((l),(b))
#endif

#else
int RepeatMove(LPBYTE, LPBYTE, WORD);
int RepeatFill(LPBYTE, BYTE, WORD);
int chdir(LPTSTR);
int SetCurrentDrive(int);
int getcwd(LPTSTR, int);
int DeleteFile(LPTSTR);
int GetTime(TIME *);
int GetDate(DATE *);
#endif

/* Common printer setup dialog vars */
PRINTDLG PD;

BOOL bFileLoaded;

/* FILE: rectdp.c */
LONG APIENTRY DrawRect(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void RectDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: rndrctdp.c */
LONG APIENTRY DrawRndRct(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void RndRctDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: rollerdp.c */
void RollerDP(HWND hWnd, UINT code, WPARAM mouseKeys, LONG lParam);

/* FILE: savebit.c */
BOOL SaveBitmapFile(HWND hWnd, int xoff, int yoff, int width, int height, HDC srcDC);

/* FILE: saveclip.c */
BOOL SaveClip(HDC srcDC, HBITMAP srcBM, int wid, int hgt);
BOOL SavePcxClip(HDC srcDC, BITMAP *srcBM, int wid, int hgt, int bmformat);

/* FILE: savecolr.c */
BOOL SaveColr(HWND hWnd, TCHAR *fileName);

/* FILE: saveimg.c */
BOOL SaveImg(HWND hWnd, int xoff, int yoff, int width, int height,
             int bytewid, HDC srcDC);

/* FILE: scrolimg.c */
void ScrolImg(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: scrolmag.c */
void ScrolMag(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: setcurs.c */
int SetCursorOn(void);
BOOL SetCursorOff(void);
LPTSTR szPbCursor(int curnum);
void PbSetCursor(LPTSTR curwho);
HANDLE  ToolCursor(void);

/* FILE: settitle.c */
void SetTitle(TCHAR *pstr);

/* FILE: shapelib.c */
void CompensateForPen(HDC hDC, LPRECT lpRect);
void InitShapeLibrary(void);
DWORD PBGetNearestColor(HDC hDC, DWORD rgbColor);

/* FILE: shrgrodp.c */
void ShrGroDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: sizewp.c */
long FAR PASCAL SizeWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: textdp.c */
void Text2DP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);
int PasteTextFromClipbrd(void);

/* FILE: tiltblt.c */
void FAR PASCAL TiltBlt(short x, short y, LPHANDLE dc);

/* FILE: tiltdp.c */
LONG APIENTRY DrawDotParal(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void TiltDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: toolwp.c */
void InvertButton(HDC hdc, int tool);
void xToolPaint(LPPAINTSTRUCT ps);
long FAR PASCAL ToolWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: trcktool.c */
WORD TrackTool(HWND hWnd, TRACKPROC lpfnDrawTool, LPRECT lprReturn, WPARAM *wParam, HDC paintDC);

/* FILE: unpkbuff.c */
BOOL InitFileBuffer(void);
void DeleteFileBuffer(void);
BYTE bgetc(HANDLE fh);
BOOL UnpkBuff(BYTE *buff, int row, DHDR hdr, int nPlanes, HANDLE fh);

/* FILE: updatimg.c */
void UpdatImg(void);
void UndoImg(void);
void UpdFlag(int how);

/* FILE: validhdr.c */
BOOL ValidHdr(HWND hWnd, DHDR *hdr, LPTSTR name);

/* FILE: vtools.c */
void UnionWithRect(LPRECT lprDst, LPRECT lprSrc);
void UnionWithPt(LPRECT lprDst, POINT thePt);
DWORD TotalMemoryAvailable(void);
WORD AllocImg(int wid, int hgt, int planes, int pixelBits, BOOL erase);
void FreeImg(void);
int ClearImg(void);
void CopyFromWork(int left, int top, int width, int height);
void PasteDownRect(int left, int top, int width, int height);
void WorkImageExchange(void);
void UndoRect(int left, int top, int width, int height);
void SuspendCopy(void);
void ResumeCopy(void);

/* FILE: winmain.c */
LPTSTR GetTableString(WORD stringid);
LPTSTR GetFarString(WORD stringid);
BOOL BetaChecks(int hPrevInstance);

/* FILE: wndinit.c */
BOOL WndInit(HINSTANCE hInstance);

/* FILE: xorcsr.c */
void XorCsr(HDC dc, POINT pt, int type);
int SizeCsr(int wtool, int wbrush);

/* FILE: zoomindp.c */
void ZoomInDP(HWND hWnd, UINT code, WPARAM mouseKeys, LONG lParam);
LONG APIENTRY DrawZoomedIn(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
LONG APIENTRY DrawZoomedInC(HDC dstDC, LPRECT lprBounds, WPARAM wParam);
void ZoomedInDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam);

/* FILE: zoominwp.c */
long ZoomInWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: zoomotwp.c */
BOOL MyPtInRect(LPRECT lprect, POINT pt);
void ClipPointToRect(LPPOINT lppt, LPRECT lprect);
void ComputeZoomRect(LPRECT lprWind, LPRECT lprZoom);
long FAR PASCAL ZoomOtWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam);

/* FILE: newdlg.c */
BOOL PUBLIC InitNewDialogs(HINSTANCE hInst);
BOOL PUBLIC DoFileDialog(int id, HWND hwnd);    /* Wrap for New dialogs */
BOOL FAR PASCAL FileDialog(HWND, UINT, WPARAM, LONG);

#ifdef DEBUG

#pragma message(__FILE__"(407):warning: DEBUG version built")

#define DB_OUT(t)               OutputDebugString(TEXT(t))
#define DB_OUT2(f, t1, t2)      OutputDebugString( (f) ? TEXT(t1) : TEXT(t2))
#define DB_OUTF(p)              if(1){ wsprintf p; OutputDebugString(acDbgBfr); } else
#define DB_OUTS(p)              OutputDebugString( p )


#define  WinAssert(exp)                               \
         {                                            \
            if (!(exp))                               \
            {                                         \
               TCHAR szBuffer[40];                     \
               sprintf(szBuffer, TEXT("%s(%d)"), \
                       __FILE__, __LINE__);           \
               MessageBox(NULL, szBuffer,             \
                          TEXT("Assertion Error"),          \
                          MB_OK | MB_ICONHAND);       \
            }                                         \
         }
#else

#   define DB_OUT(t)
#   define DB_OUT2(f, t1, t2)
#   define DB_OUTF(p)
#   define DB_OUTS(p)

#endif

void FAR InitDecimal(LPTSTR szkey);
void FAR ChangeCutCopy(HMENU hMenu, WORD wEnable);

void FAR PASCAL FreePrintHandles(void);

BOOL FAR PASCAL  GetDefaultPort(void);
