/**************************************************/
/*                                           */
/*                                           */
/* MDI Child Window( For Edit)                     */
/*                                           */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include    "stdafx.h"
#include    "eudcedit.h"
#include    "editwnd.h"
#include    "refrwnd.h"
#include    "vdata.h"
#include "extfunc.h"
#include "util.h"
#include "rotatdlg.h"

// For mouse
#define     NCLICK   0
#define     LCLICK   1
#define     RCLICK   2

// For selected rectangle
#define     LUPPER   0
#define     RUPPER   1
#define     RLOWER   2
#define     LLOWER   3
#define     MUPPER   4
#define     RMIDLE   5
#define     MLOWER   6
#define     LMIDLE   7
#define     NCHECK   8

// For Rectangular selection and freeform selection
#define     BMP   0
#define     DSP   1  
#define     PRV   2
#define     VLD   3
#define     PIN   4

// max of freeform selection point
#define     FREELIAIS   1000
#define     MKOUTHDL 2

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CMenu NEAR CEditWnd::menu;
IMPLEMENT_DYNCREATE( CEditWnd, CMDIChildWnd)
BEGIN_MESSAGE_MAP(CEditWnd, CMDIChildWnd)
//{{AFX_MSG_MAP(CEditWnd)
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_PAINT()
   ON_WM_CREATE()
   ON_COMMAND(ID_GAIJI_COPY, OnGaijiCopy)
   ON_COMMAND(ID_GAIJI_CUT, OnGaijiCut)
   ON_COMMAND(ID_GAIJI_PASTE, OnGaijiPaste)
   ON_COMMAND(ID_GAIJI_UNDO, OnGaijiUndo)
   ON_WM_RBUTTONDOWN()
   ON_WM_RBUTTONUP()
   ON_WM_SIZE()
   ON_WM_SETCURSOR()
   ON_UPDATE_COMMAND_UI(ID_GAIJI_PASTE, OnUpdateGaijiPaste)
   ON_WM_MDIACTIVATE()
   ON_UPDATE_COMMAND_UI(ID_GAIJI_COPY, OnUpdateGaijiCopy)
   ON_UPDATE_COMMAND_UI(ID_GAIJI_CUT, OnUpdateGaijiCut)
   ON_COMMAND(ID_DELETE_EDIT, OnDeleteEdit)
   ON_UPDATE_COMMAND_UI(ID_DELETE_EDIT, OnUpdateDeleteEdit)
   ON_UPDATE_COMMAND_UI(ID_GAIJI_UNDO, OnUpdateGaijiUndo)
   ON_WM_KEYDOWN()
   ON_WM_CLOSE()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Global parameter
LOGFONT  EditLogFont;
extern   int   SelectItems;
extern  CBitmap   DupBmp;
extern  CRect  DupRect;
extern CRefrWnd *pRefrChild;
extern BOOL g_bKeepEUDCLink;

/****************************************/
/*             */
/*   Create MDI child window for edit  */
/*             */
/****************************************/
BOOL
CEditWnd::Create(
LPCTSTR  szTitle,
LONG  Style,
RECT  EudcWndRect,
CMDIFrameWnd* Parent)
{
// Load menu as same as MainFrame
   if( menu.m_hMenu == NULL)
      menu.LoadMenu( IDR_MAINFRAME);
   m_hMenuShared = menu.m_hMenu;

// Register window class of MDI child for edit
   const TCHAR *pszEudcWndClass =
      AfxRegisterWndClass( CS_BYTEALIGNCLIENT,
      NULL, (HBRUSH)(COLOR_WINDOW+1), NULL);

   return CMDIChildWnd::Create( pszEudcWndClass,
          szTitle, Style, EudcWndRect, Parent);
}

/****************************************/
/*             */
/*   Process before window is created  */
/*             */
/****************************************/
int
CEditWnd::OnCreate( LPCREATESTRUCT lpCreateStruct)
{
   if( CMDIChildWnd::OnCreate( lpCreateStruct) == -1)
      goto ERR;

// If it is mirrored then turn off mirroing.
   ModifyStyleEx( WS_EX_LAYOUTRTL, 0);

   if( !CreateNewBitmap())
      goto ERR;
   if( !ClipPickValueInit())
      goto ERR;
   if( !InitEditLogfont())
      goto ERR;

   return  0;
ERR:
   return -1;
}

/****************************************/
/*             */
/* Create bitmap, and initialized   */
/*             */
/****************************************/
BOOL
CEditWnd::CreateNewBitmap()
{
   WORD  BitSize;
   HANDLE   BitInit;
   BYTE  *BitmapPtr;

   CClientDC   ChildDC( this);

   if( !ImageDC.CreateCompatibleDC( &ChildDC))
      return FALSE;

   if( !CRTDrawDC.CreateCompatibleDC( &ChildDC))
      return FALSE;

   BitSize = (WORD)((( BITMAP_WIDTH +15) /16) *2) *(WORD)BITMAP_HEIGHT;
   if(( BitInit = LocalAlloc( LMEM_MOVEABLE, BitSize)) == 0)
      return FALSE;

   if(( BitmapPtr = (BYTE *)LocalLock( BitInit)) == NULL){
      LocalFree( BitInit);
      return FALSE;
   }

// Initialize with white color
   memset( BitmapPtr, 0xffff, BitSize);
   if( !ImageBmp.CreateBitmap( BITMAP_WIDTH, BITMAP_HEIGHT,
      1, 1, (LPSTR)BitmapPtr)){
      LocalUnlock( BitInit);
      LocalFree( BitInit);
      return FALSE;
   }

// Initialize with white color
   if( !CRTDrawBmp.CreateBitmap( BITMAP_WIDTH, BITMAP_HEIGHT,
       1, 1, (LPSTR)BitmapPtr)){
       LocalUnlock( BitInit);
       LocalFree( BitInit);
      return FALSE;
   }
   LocalUnlock( BitInit);
   LocalFree( BitInit);    
   ImageDC.SelectObject( &ImageBmp);
   CRTDrawDC.SelectObject( &CRTDrawBmp);

   return TRUE;
}
               
/****************************************/
/*             */
/* Registry clipboard format  */
/*             */
/****************************************/
BOOL
CEditWnd::ClipPickValueInit()
{
// Register "EUDCEDIT" clipboard format
   if( !( ClipboardFormat = RegisterClipboardFormat(TEXT("EudcEdit"))))
      return FALSE;

   return TRUE;
}
               
/****************************************/
/*             */
/* Initialize EditLogFont     */
/*             */
/****************************************/
BOOL
CEditWnd::InitEditLogfont()
{
   CFont cFont;

#ifdef BUILD_ON_WINNT
  cFont.CreateStockObject(DEFAULT_GUI_FONT);
#else
   cFont.CreateStockObject( SYSTEM_FONT);
#endif
   cFont.GetObject( sizeof(LOGFONT), &EditLogFont);
   cFont.DeleteObject();

   return TRUE;
}

/****************************************/
/*             */
/* Default Constructor     */
/*             */
/****************************************/
CEditWnd::CEditWnd()
{
// Initialize static parameter
   BitmapDirty = FALSE;
   GridShow = TRUE;
   SelectItem = PEN;
   RectClipFlag = FALSE;
   CheckNum = NCHECK;
   ButtonFlag = NCLICK;
   IsCapture = FALSE;
   UndoBitmapFlag = FALSE;
   FlagTmp = FALSE;
   UpdateCode = 0x0000;
   Ratio = 3;
   bFocus = TRUE;
}
   
/****************************************/
/*             */
/* Destructor        */
/*             */ 
/****************************************/
CEditWnd::~CEditWnd()
{
// Delete CDC and CBitmap object
   if( CRTDrawBmp.Detach() != NULL)
      CRTDrawBmp.DeleteObject();
   if( ImageBmp.Detach() != NULL)
      ImageBmp.DeleteObject();
   if( CRTDrawDC.Detach() != NULL)
      CRTDrawDC.DeleteDC();
   if( ImageDC.Detach() != NULL)
      ImageDC.DeleteDC();
   if( UndoImage.Detach() != NULL)
      UndoImage.DeleteObject();
  menu.DestroyMenu();
}

/****************************************/
/*             */
/* COMMAND     "UNDO"      */
/*             */
/****************************************/
void
CEditWnd::OnGaijiUndo()
{
   int   wSize;
   LPBYTE   pBuf1, pBuf2;

   if( !UndoBitmapFlag)
      CreateUndoBitmap();
   UndoBitmapFlag = TRUE;

   if( SelectItem == RECTCLIP || SelectItem == FREEFORM)
      WriteSelRectBitmap();

   wSize = (int)((( BITMAP_WIDTH +15) /16) *2) *(int)BITMAP_HEIGHT;
   pBuf1 = (LPBYTE)malloc( wSize);
   pBuf2 = (LPBYTE)malloc( wSize);
   ImageBmp.GetBitmapBits( wSize, (LPVOID)pBuf1);
   UndoImage.GetBitmapBits( wSize, (LPVOID)pBuf2);
   ImageBmp.SetBitmapBits( wSize, (LPVOID)pBuf2);
   UndoImage.SetBitmapBits( wSize, (LPVOID)pBuf1);          
   free(pBuf1);
   free(pBuf2);
   
   this->Invalidate(FALSE);   
   this->UpdateWindow();
}

/****************************************/
/*             */
/* COMMAND     "UNDO" (Update)   */
/*             */
/****************************************/
void
CEditWnd::OnUpdateGaijiUndo(
CCmdUI* pCmdUI)
{
   if( BitmapDirty)
      pCmdUI->Enable(TRUE);
   else  pCmdUI->Enable(FALSE);
}

/****************************************/
/*             */
/* COMMAND     "CUT"    */
/*             */
/****************************************/
void
CEditWnd::OnGaijiCut()
{
   UndoImageDraw();
   BitmapDirty = TRUE;
   ClipImageCopy();
   ClipImageCut();
   this->Invalidate(FALSE);
   if( RectClipFlag){
      ToolTerm();
      RectClipFlag = FALSE;
   }        
}

/****************************************/
/*             */
/* COMMAND     "Cut" (Update) */
/*             */
/****************************************/
void
CEditWnd::OnUpdateGaijiCut(
CCmdUI* pCmdUI)
{
   pCmdUI->Enable( RectClipFlag);   
}

/****************************************/
/*             */
/* COMMAND     "COPY"      */
/*             */
/****************************************/
void
CEditWnd::OnGaijiCopy()
{
   ClipImageCopy();
}

/****************************************/
/*             */
/* COMMAND     "Copy" (Update)   */
/*             */
/****************************************/
void
CEditWnd::OnUpdateGaijiCopy( CCmdUI* pCmdUI)
{
   pCmdUI->Enable( RectClipFlag);   
}

/****************************************/
/*             */
/* COMMAND     "PASTE"     */
/*             */
/****************************************/
void
CEditWnd::OnGaijiPaste()
{
   HBITMAP  ClipBmpHandle, hTempBitmap1;
   CBitmap  ClipBmp1, ClipBmp2;
   HDC   hStdDC, hTempDC1;
   HGDIOBJ  hOldSObj, hOldSObj2;
   BITMAP   Bmp;
   CSize StretchSize;
   BOOL  BitmapFmt, PrivateFmt;

   BitmapFmt  = FALSE;
   PrivateFmt = FALSE;
   UndoImageDraw();
   BitmapDirty = TRUE;
   WriteSelRectBitmap();
   this->Invalidate( FALSE);
   this->UpdateWindow();

   if( !this->OpenClipboard())
      return;

   if(( ClipBmpHandle = (HBITMAP)GetClipboardData(CF_BITMAP)) == NULL){
      CloseClipboard();
         return;
      }
   GetObject(ClipBmpHandle, sizeof(BITMAP), (LPVOID)&Bmp);
   hStdDC = CreateCompatibleDC( ImageDC.GetSafeHdc());
   hOldSObj = SelectObject(hStdDC, ClipBmpHandle);

   hTempBitmap1 = CreateBitmap(Bmp.bmWidth, Bmp.bmHeight, 1, 1, NULL);
   hTempDC1 = CreateCompatibleDC( ImageDC.GetSafeHdc());
   hOldSObj2 = SelectObject( hTempDC1, hTempBitmap1);
   ::BitBlt( hTempDC1, 0, 0, Bmp.bmWidth, Bmp.bmHeight,
      hStdDC, 0, 0, SRCCOPY);

   if( !CloseClipboard())
  {
   SelectObject( hStdDC, hOldSObj);
     SelectObject( hTempDC1, hOldSObj2);
     DeleteObject( hTempBitmap1);
     DeleteDC( hStdDC);
     DeleteDC( hTempDC1);
      return;
  }
   if( (int)Bmp.bmWidth  < BITMAP_WIDTH  &&
            (int)Bmp.bmHeight < BITMAP_HEIGHT ){
         StretchSize.cx = (int)Bmp.bmWidth;
         StretchSize.cy = (int)Bmp.bmHeight;
      ::StretchBlt( CRTDrawDC.GetSafeHdc(), 0, 0,
         StretchSize.cx, StretchSize.cy, hTempDC1, 0, 0,
         StretchSize.cx, StretchSize.cy, SRCCOPY);
      }else if( Bmp.bmWidth < BITMAP_WIDTH){
      StretchSize.cx = (int)Bmp.bmWidth;
      StretchSize.cy = BITMAP_HEIGHT;
      ::StretchBlt( CRTDrawDC.GetSafeHdc(), 0, 0,
         StretchSize.cx, StretchSize.cy, hTempDC1, 0, 0,
         StretchSize.cx, Bmp.bmHeight, SRCCOPY);
   }else if( Bmp.bmHeight < BITMAP_HEIGHT){
      StretchSize.cx = BITMAP_HEIGHT;
      StretchSize.cy = (int)Bmp.bmHeight;
      ::StretchBlt( CRTDrawDC.GetSafeHdc(), 0, 0,
         StretchSize.cx, StretchSize.cy, hTempDC1, 0, 0,
         Bmp.bmWidth, StretchSize.cy, SRCCOPY);
   }else{
         StretchSize.cx = BITMAP_WIDTH;
         StretchSize.cy = BITMAP_HEIGHT;
      ::StretchBlt( CRTDrawDC.GetSafeHdc(), 0, 0,
         StretchSize.cx, StretchSize.cy, hTempDC1, 0, 0,
         Bmp.bmWidth, Bmp.bmHeight, SRCCOPY);
      }
   ptStart.x = 0;
   ptStart.y = 0;
   ptEnd.x = (StretchSize.cx-1) *ZoomRate;
   ptEnd.y = (StretchSize.cy-1) *ZoomRate;
   SetMoveRect();
   SetValidRect();
   SetClickRect();
   SetPickRect();
   SelectItem = RECTCLIP;
   RectClipFlag = TRUE;
   this->InvalidateRect( &ClipRect[VLD], FALSE);
   this->UpdateWindow();

   SelectObject( hStdDC, hOldSObj);
   SelectObject( hTempDC1, hOldSObj2);
   DeleteObject( hTempBitmap1);
   DeleteDC( hStdDC);
   DeleteDC( hTempDC1);

    return; 
}

/****************************************/
/*             */
/* COMMAND  "PASTE" (Update)  */
/*             */
/****************************************/
void
CEditWnd::OnUpdateGaijiPaste(
CCmdUI* pCmdUI)
{
   UINT  ClipFormat;
   BOOL  BitmapFmt;
   BOOL  PrivateFmt;

   BitmapFmt  = FALSE;
   PrivateFmt = FALSE;
   if( !::OpenClipboard(this->GetSafeHwnd()))
      return;

   for( ClipFormat = 0;;){
         ClipFormat = EnumClipboardFormats( ClipFormat);
         if( ClipFormat == CF_BITMAP){
                  BitmapFmt = TRUE;
                  if( PrivateFmt)   break;
         }
         if( ClipFormat == ClipboardFormat){
                  PrivateFmt = TRUE;
                  if( BitmapFmt) break;
         }
         if( ClipFormat == 0)   break;
      }
      if( !PrivateFmt){
         if( BitmapFmt){
         CloseClipboard();
         pCmdUI->Enable(TRUE);
         return;
         }else{
                  CloseClipboard();
         pCmdUI->Enable(FALSE);
                  return;
         }
      }else{
         if ( !BitmapFmt){
                  CloseClipboard();
         pCmdUI->Enable(FALSE);
                  return;
         }
      }
   CloseClipboard();
   pCmdUI->Enable(FALSE);
}

/****************************************/
/*             */
/* COMMAND     "Delete" */
/*             */
/****************************************/
void
CEditWnd::OnDeleteEdit()
{
   UndoImageDraw();
   BitmapDirty = TRUE;
   if(( SelectItem == RECTCLIP || SelectItem == FREEFORM) &&
        RectClipFlag){
      RectClipFlag = FALSE;
      ToolTerm();
   }
   this->Invalidate( FALSE);
   this->UpdateWindow();
}

/****************************************/
/*             */
/* COMMAND     "Delete"(Update)*/
/*             */
/****************************************/
void
CEditWnd::OnUpdateDeleteEdit(
CCmdUI* pCmdUI)
{
   pCmdUI->Enable( RectClipFlag);   
}

/****************************************/
/*             */
/* Create bitmap for undo     */
/*             */
/****************************************/
BOOL
CEditWnd::CreateUndoBitmap()
{
   HANDLE   BitHandle;
   BYTE  *pBitmap;
   WORD  wSize;

   wSize = (WORD)((( BITMAP_WIDTH + 15) /16) *2) *(WORD)BITMAP_HEIGHT;
   if(( BitHandle = LocalAlloc( LMEM_MOVEABLE, wSize)) == 0)
      return FALSE;

   if(( pBitmap = (BYTE *)LocalLock( BitHandle)) == NULL){
      LocalFree( BitHandle);
      return FALSE;
   }
   memset( pBitmap, 0xffff, wSize);
   if( !UndoImage.CreateBitmap( BITMAP_WIDTH, BITMAP_HEIGHT,
             1, 1, (LPSTR)pBitmap)){
      LocalFree( BitHandle);
      return FALSE;
   }
   LocalUnlock( BitHandle);
   LocalFree( BitHandle);
   return TRUE;   
}

/****************************************/
/*             */
/* Cut image to clipboard     */
/*             */
/****************************************/
BOOL
CEditWnd::ClipImageCut()
{
   CPen   CutPen;
   CBrush CutBrush;

   CutPen.CreateStockObject( NULL_PEN);
   CPen *OldPen = CRTDrawDC.SelectObject( &CutPen);
   CutBrush.CreateSolidBrush( COLOR_WHITE );
      CBrush *OldBrush = CRTDrawDC.SelectObject( &CutBrush);

   ClipRect[BMP].right  += 1;
   ClipRect[BMP].bottom += 1;
      CRTDrawDC.Rectangle( &ClipRect[BMP]);

      CRTDrawDC.SelectObject( OldPen);
      CRTDrawDC.SelectObject( OldBrush);
   CutPen.DeleteObject();
   CutBrush.DeleteObject();
   return TRUE;
}

/****************************************/
/*             */
/* Copy image to clipboard    */
/*             */
/****************************************/
BOOL
CEditWnd::ClipImageCopy()
{
   CBitmap  ClipBmp;
   CDC   ClipDC;
   CRect pRect;

   pRect.SetRect( ClipRect[DSP].left /ZoomRate,
        ( ClipRect[DSP].top - CAPTION_HEIGHT) /ZoomRate,
          ClipRect[DSP].right /ZoomRate,
        ( ClipRect[DSP].bottom - CAPTION_HEIGHT) /ZoomRate);

   ClipDC.CreateCompatibleDC( &ImageDC);
      ClipBmp.CreateCompatibleBitmap( &ImageDC,
         pRect.Width(), pRect.Height());
      CBitmap *OldClip = ClipDC.SelectObject( &ClipBmp);

      ClipDC.PatBlt( 0, 0, pRect.Width(), pRect.Height(), WHITENESS);
      ClipDC.StretchBlt( 0, 0, pRect.Width(), pRect.Height(), &CRTDrawDC,
         ClipRect[BMP].left, ClipRect[BMP].top,
      ClipRect[BMP].Width(), ClipRect[BMP].Height(),SRCCOPY);

      if( !this->OpenClipboard()){
      ClipDC.SelectObject( OldClip);
      ClipBmp.DeleteObject();
         ClipDC.DeleteDC();
         return FALSE;
      }
      EmptyClipboard();

   if( !SetClipboardData( CF_BITMAP, ClipBmp.Detach())){
      ClipDC.SelectObject( OldClip);
      ClipBmp.DeleteObject();
         ClipDC.DeleteDC();
         CloseClipboard();
         return FALSE;
      }
      CloseClipboard();
   ClipDC.SelectObject( OldClip);
   ClipBmp.DeleteObject();
      ClipDC.DeleteDC();
      return TRUE;
}

/****************************************/
/*             */
/* MESSAGE  "WM_PAINT"     */
/*             */
/****************************************/
void
CEditWnd::OnPaint()
{
   CPaintDC dc( this);
   DWORD    OldTextColor;

   this->GetClientRect( &EudcWndRect);

// Set textcolor to gray if fitting curve
   if( FlagTmp)
      OldTextColor = dc.SetTextColor( COLOR_FITTING);
   else  OldTextColor = dc.SetTextColor( COLOR_BLACK);

// Put bitmap data   on display
   ZoomRate = EudcWndRect.right /BITMAP_WIDTH;
   dc.StretchBlt( 0, CAPTION_HEIGHT, EudcWndRect.right,
      EudcWndRect.bottom - CAPTION_HEIGHT,
      &ImageDC, 0, 0, BITMAP_WIDTH, BITMAP_HEIGHT, SRCCOPY);

// Draw selected rectangle if RectClipFlag is build
   if( RectClipFlag){
      dc.StretchBlt( ClipRect[DSP].left - 1, ClipRect[DSP].top - 1,
         ClipRect[DSP].Width(), ClipRect[DSP].Height(),
         &CRTDrawDC, ClipRect[BMP].left, ClipRect[BMP].top,
         ClipRect[BMP].Width(), ClipRect[BMP].Height(), SRCAND);

      DrawMoveRect( &dc);
      DrawStretchRect( &dc);
   }
   dc.SetTextColor( OldTextColor);
   CaptionDraw();

   if( ZoomRate >= 2 && GridShow)
      DrawGridLine( &dc);

   if( FlagTmp)
      CurveFittingDraw( &dc);
}
               
/****************************************/
/*             */
/* MESSAGE  "WM_LBUTTONDOWN"  */
/*             */
/****************************************/
void
CEditWnd::OnLButtonDown(
UINT  ,
CPoint   point)
{
   CRect CaptionRect;
   CPoint   ClickPt;

   if( ButtonFlag == RCLICK){
      this->Invalidate();
      ToolTerm(); 
      return;
   }
   ClickPt = point;
   CaptionRect.CopyRect( &EudcWndRect);
   CaptionRect.top = CAPTION_HEIGHT;
   if( !CaptionRect.PtInRect( point))
      return;

   ToolInit(LCLICK);
// Modurate coordinate
   CorrectMouseDownPoint( point);

// Stretch selected rectangle
   if( RectClipFlag){
      SetClickRect();
      if( ClipRect[PIN].PtInRect( ClickPt) &&
             ( EudcWndRect.Width() > ClipRect[DSP].Width() ||
          EudcWndRect.Height() - CAPTION_HEIGHT
          > ClipRect[DSP].Height())){
            BitmapDirty = TRUE;
      }else if(( CheckNum = CheckClipRect( ClickPt)) != NCHECK){
         BitmapDirty = TRUE;
         SetValidRect();
         RectClipFlag = FALSE;
         this->InvalidateRect( &ClipRect[VLD], FALSE);
         this->UpdateWindow();
         RectClipFlag = TRUE;
         CClientDC   dc( this);
         DrawMoveRect( &dc);
      }else{
         RectClipFlag = FALSE;
         this->InvalidateRect( &ClipRect[VLD], FALSE);
         this->UpdateWindow();
         DrawClipBmp();
         SetPickRect();
         if( SelectItem == RECTCLIP)
            DrawRubberBand( TRUE);
         else{
            CPoint   Sp;

            Sp.x = ptStart.x;
            Sp.y = ptStart.y + CAPTION_HEIGHT;
            m_pointArray.RemoveAll();
            m_SelectArray.RemoveAll();
            m_pointArray.Add( Sp);
            Sp.x = ptStart.x /ZoomRate;
            Sp.y = ptStart.y /ZoomRate;
            m_SelectArray.Add( Sp);

         }
      }
   }else{
      if( SelectItem == PEN || SelectItem == BRUSH ){
         UndoImageDraw();
         BitmapDirty = TRUE;
         DrawPoint( ptStart, FALSE);
      }else if (SelectItem == ERASER){
         UndoImageDraw();
         BitmapDirty = TRUE;
         DrawPoint( ptStart, TRUE);
      }else{
         UndoImageDraw();
         BitmapDirty = TRUE;
         if( SelectItem == FREEFORM){
            CPoint   Sp;

            Sp.x = ptStart.x;
            Sp.y = ptStart.y + CAPTION_HEIGHT;
            m_pointArray.RemoveAll();
            m_SelectArray.RemoveAll();
            m_pointArray.Add( Sp);
            Sp.x = ptStart.x /ZoomRate;
            Sp.y = ptStart.y /ZoomRate;
            m_SelectArray.Add( Sp);
         }else    DrawRubberBand( TRUE);
      }
   }
}

/****************************************/
/*             */
/* MESSAGE  "WM_MOUSEMOVE"    */
/*             */
/****************************************/
void
CEditWnd::OnMouseMove(
UINT  ,
CPoint   point)
{

   if( IsCapture){
//    Modurate coordinate
      CorrectMouseUpPoint( point);
      if( ptPrev.x == ptEnd.x && ptPrev.y == ptEnd.y)
         return;

      if( RectClipFlag){
         if( CheckNum == NCHECK){
            CRect URect, MRect, PRRect;

            if( !MoveClipRect())
               return;
            MRect.SetRect(  ClipRect[DSP].left   - Ratio,
               ClipRect[DSP].top    - Ratio,
               ClipRect[DSP].right  + Ratio,
               ClipRect[DSP].bottom + Ratio);

            PRRect.SetRect( ClipRect[PRV].left   - Ratio,
               ClipRect[PRV].top    - Ratio,
               ClipRect[PRV].right  + Ratio,
               ClipRect[PRV].bottom + Ratio);

            URect.UnionRect( &MRect, &PRRect);
            this->InvalidateRect( &URect, FALSE);
            this->UpdateWindow();

         }else    StretchMoveRect();
      }else{
         if( SelectItem == PEN || SelectItem == BRUSH)
            DrawToPoint(FALSE);
         else if (SelectItem == ERASER)
            DrawToPoint(TRUE);
         else if( SelectItem == FREEFORM){
            DrawFreeForm( FALSE);
            SelectFreeForm(FALSE);
         }else DrawRubberBand( FALSE);
      }
   }
}

/****************************************/
/*             */
/* MESSAGE  "WM_LBUTTONUP"    */
/*             */
/****************************************/
void
CEditWnd::OnLButtonUp(
UINT  ,
CPoint   point)
{
    if (!bFocus)
    {
        bFocus = TRUE;
        CaptionDraw();

        pRefrChild->bFocus = FALSE;
        pRefrChild->CaptionDraw();

    }

   if( IsCapture){
//    Modurate coordinate
      CorrectMouseUpPoint( point);

      if( SelectItem == PEN || SelectItem == BRUSH ||SelectItem == ERASER){
         if( FlagTmp)
            this->Invalidate(FALSE);
         ToolTerm();
         return;
      }

//    draw a figure on bitmap
      if( SelectItem == RECTCLIP && !RectClipFlag){
         SetMoveRect();
         SetPickRect();
         SetValidRect();
         SetClickRect();
         if(( abs( ptEnd.x - ptStart.x) >= ZoomRate*2) &&
            ( abs( ptEnd.y - ptStart.y) >= ZoomRate*2) ){
            CRTDrawDC.BitBlt( ClipRect[BMP].left,
               ClipRect[BMP].top,
               ClipRect[BMP].Width(),
               ClipRect[BMP].Height(),
               &ImageDC, ClipRect[BMP].left,
               ClipRect[BMP].top, SRCCOPY);

            EraseRectangle();
            RectClipFlag = TRUE;
         }
         this->InvalidateRect( &ClipRect[VLD], FALSE);
         this->UpdateWindow();
      }else if( SelectItem == FREEFORM && !RectClipFlag){
         POINT nArray[FREELIAIS];
         POINT pArray[FREELIAIS];
   
         DrawFreeForm( FALSE);
         SelectFreeForm( FALSE);
         DrawFreeForm( TRUE);
         SelectFreeForm( TRUE);

         if( m_pointArray.GetSize()  >= FREELIAIS ||
             m_SelectArray.GetSize() >= FREELIAIS ){
               ToolTerm();
               this->Invalidate(FALSE);
            this->UpdateWindow();
            return;
         }
         for( int i = 0; i < m_pointArray.GetSize(); i++)
            nArray[i] = m_pointArray[i];
         for( i = 0; i < m_SelectArray.GetSize(); i++)
            pArray[i] = m_SelectArray[i];

         if( FreeRgn.GetSafeHandle() != NULL)
            FreeRgn.DeleteObject();
         if( PickRgn.GetSafeHandle() != NULL)
            PickRgn.DeleteObject();
         FreeRgn.CreatePolygonRgn( nArray,
            (int)(m_pointArray.GetSize()), ALTERNATE);
         PickRgn.CreatePolygonRgn( pArray,
            (int)(m_SelectArray.GetSize()), ALTERNATE);

         if( FreeRgn.GetSafeHandle()  == NULL ||
             PickRgn.GetSafeHandle()  == NULL){
            m_pointArray.RemoveAll();
            m_SelectArray.RemoveAll();
            RectClipFlag = FALSE;
         }else{
            RectClipFlag = TRUE;
            if( !SetFreeForm()){
               RectClipFlag = FALSE;
               FreeRgn.DeleteObject();
               PickRgn.DeleteObject();
               SetPickRect();
               SetValidRect();
               SetClickRect();
               ClipRect[PRV].CopyRect( &ClipRect[DSP]);
               this->InvalidateRect( &ClipRect[VLD], FALSE);
               this->UpdateWindow();
            }else{
               EraseFreeForm();
               SetPickRect();
               SetValidRect();
               SetClickRect();
               ClipRect[PRV].CopyRect( &ClipRect[DSP]);
               this->InvalidateRect( &ClipRect[VLD], FALSE);
               this->UpdateWindow();
            }
         }
      }else if( RectClipFlag){
         if( CheckNum != NCHECK){
            DrawStretchClipToDisp();
            CheckNum = NCHECK;
         }else{
            if( !MoveClipRect()){
               ToolTerm();
               return;
            }
         }
         SetValidRect();
         SetClickRect();

         this->InvalidateRect( &ClipRect[VLD], FALSE);
         this->UpdateWindow();
      }else{      
         if( SelectItem != SLOPE)
            IllegalRect( &ptStart, &ptEnd);
         else{
            ptEnd.x = ( ptEnd.x /ZoomRate) *ZoomRate;
            ptEnd.y = ( ptEnd.y /ZoomRate) *ZoomRate;
            ptStart.x = ( ptStart.x /ZoomRate) *ZoomRate;
            ptStart.y = ( ptStart.y /ZoomRate) *ZoomRate;
         }
         DrawRectBmp();
         if( SelectItem == SLOPE)
            IllegalRect( &ptStart, &ptEnd);
         SetMoveRect();
         if( FlagTmp)
            this->Invalidate( FALSE);
         else if( SelectItem != PEN && SelectItem != BRUSH && SelectItem != ERASER){
            this->InvalidateRect( &ClipRect[DSP], FALSE);
         }
      }
      ToolTerm();
   }
}

/****************************************/
/*             */
/* MESSAGE  "WM_RBUTTONDOWN"  */
/*             */
/****************************************/
void
CEditWnd::OnRButtonDown(
UINT  nFlags,
CPoint   point)
{
   CRect CaptionRect;

   if( ButtonFlag == LCLICK){
      this->Invalidate();
      ToolTerm();
      return;
   }
   if( SelectItem == RECTCLIP || SelectItem == FREEFORM){
      ToolTerm();
      return;
   }
   CaptionRect.CopyRect( &EudcWndRect);
   CaptionRect.top = CAPTION_HEIGHT;
   if( !CaptionRect.PtInRect( point))
      return;
   ToolInit(RCLICK);

// Modurate coordinate
   CorrectMouseDownPoint( point);
   UndoImageDraw();
   BitmapDirty = TRUE;
   if( SelectItem == PEN || SelectItem == BRUSH )
      DrawPoint( ptStart, FALSE);
   else if (SelectItem == ERASER)
      DrawPoint( ptStart, TRUE);
   else
      DrawRubberBand( TRUE);  
}

/****************************************/
/*             */
/* MESSAGE  "WM_RBUTTONUP"    */
/*             */
/****************************************/
void
CEditWnd::OnRButtonUp(
UINT  nFlags,
CPoint   point)
{
   if( IsCapture){
      if( SelectItem == PEN || SelectItem == BRUSH || SelectItem == ERASER ||
          SelectItem == RECTCLIP || SelectItem == FREEFORM){
         if( FlagTmp)
            this->Invalidate( FALSE);
         ToolTerm();
         return;
      }
//    Modurate coordinate
      CorrectMouseUpPoint( point);
      DrawRubberBand( TRUE);
      if( SelectItem != SLOPE)
         IllegalRect( &ptStart, &ptEnd);
      else{
         ptEnd.x = ( ptEnd.x /ZoomRate) *ZoomRate;
         ptEnd.y = ( ptEnd.y /ZoomRate) *ZoomRate;
         ptStart.x = ( ptStart.x /ZoomRate) *ZoomRate;
         ptStart.y = ( ptStart.y /ZoomRate) *ZoomRate;
      }

//    Draw a figure on bitmap
      DrawRectBmp();
      if( SelectItem == SLOPE)
         IllegalRect( &ptStart, &ptEnd);
      SetMoveRect();
      if( FlagTmp)
         this->Invalidate( FALSE);
      else  this->InvalidateRect( &ClipRect[DSP], FALSE);
      ToolTerm();
   }
}

/****************************************/
/*             */
/* MESSAGE     "WM_SIZE"   */
/*             */
/****************************************/
void
CEditWnd::OnSize(
UINT  nType,
int   cx,
int   cy)
{
   int   NewZoomRate;

   NewZoomRate = cx / BITMAP_WIDTH;
   if( RectClipFlag && NewZoomRate > 1){
      ClipRect[PRV].left = ( ClipRect[PRV].left /ZoomRate)
         * NewZoomRate;

      ClipRect[PRV].top  = ((( ClipRect[PRV].top - CAPTION_HEIGHT)
         /ZoomRate) * NewZoomRate) + CAPTION_HEIGHT;

      ClipRect[PRV].right = ( ClipRect[PRV].right /ZoomRate)
         * NewZoomRate + 1;

      ClipRect[PRV].bottom = ((( ClipRect[PRV].bottom -CAPTION_HEIGHT)
         / ZoomRate) * NewZoomRate) + CAPTION_HEIGHT + 1;

      ClipRect[DSP].left = ( ClipRect[DSP].left /ZoomRate)
         * NewZoomRate;

      ClipRect[DSP].top = ((( ClipRect[DSP].top - CAPTION_HEIGHT)
         / ZoomRate) * NewZoomRate) + CAPTION_HEIGHT;

      ClipRect[DSP].right = ( ClipRect[DSP].right /ZoomRate)
         * NewZoomRate + 1;

      ClipRect[DSP].bottom = ((( ClipRect[DSP].bottom -CAPTION_HEIGHT)
         / ZoomRate) * NewZoomRate) + CAPTION_HEIGHT + 1;

      SetClickRect();
      SetValidRect();
   }else if( RectClipFlag && NewZoomRate <= 1)
      WriteSelRectBitmap();

   CMDIChildWnd::OnSize(nType, cx, cy);

   this->Invalidate(FALSE);
   this->UpdateWindow();
}

/****************************************/
/*             */
/* MESSAGE     "WM_SETCURSOR"  */
/*             */
/****************************************/
BOOL
CEditWnd::OnSetCursor(
CWnd*    pWnd,
UINT  nHitTest,
UINT  message)
{
   CPoint   point;
   CRect CaptionRect;
   CRect EditRect;
   HCURSOR  hArrowCur;

   GetCursorPos( &point);
   this->ScreenToClient( &point);
   this->GetClientRect( &CaptionRect);
   EditRect.CopyRect( &CaptionRect);
   CaptionRect.bottom = CAPTION_HEIGHT;
   EditRect.top = CAPTION_HEIGHT;

   if(CaptionRect.PtInRect( point)){
      hArrowCur = AfxGetApp()->LoadStandardCursor( IDC_ARROW);
      ::SetCursor( hArrowCur);
   }else if( RectClipFlag && !IsCapture && EditRect.PtInRect( point)){
      if( PickRect[LUPPER].PtInRect( point) ||
          PickRect[RLOWER].PtInRect( point)){
         ::SetCursor((HCURSOR)ArrowCursor[LEFTSLOPE]);
      }else if( PickRect[RUPPER].PtInRect( point) ||
           PickRect[LLOWER].PtInRect( point)){
         ::SetCursor((HCURSOR)ArrowCursor[RIGHTSLOPE]);
      }else if( PickRect[MUPPER].PtInRect( point) ||
           PickRect[MLOWER].PtInRect( point)){
         ::SetCursor((HCURSOR)ArrowCursor[HORIZONTAL]);
      }else if( PickRect[RMIDLE].PtInRect( point) ||
           PickRect[LMIDLE].PtInRect( point)){
         ::SetCursor((HCURSOR)ArrowCursor[VERTICAL]);
      }else if( ClipRect[DSP].PtInRect( point)){
         ::SetCursor((HCURSOR)ArrowCursor[ALLDIRECT]);
      }else ::SetCursor((HCURSOR)ToolCursor[SelectItem]);
   }else if( !IsCapture && EditRect.PtInRect( point))
      ::SetCursor((HCURSOR)ToolCursor[SelectItem]);
   else{
      hArrowCur = AfxGetApp()->LoadStandardCursor( IDC_ARROW);
      ::SetCursor( hArrowCur);
   }  
   return TRUE;
}

/****************************************/
/*             */
/* MESSAGE     "WM_MDIACTIVATE"*/
/*             */
/****************************************/
void
CEditWnd::OnMDIActivate(
BOOL  bActivate,
CWnd*    pActivateWnd,
CWnd*    pDeactivateWnd)
{
   if( bActivate == FALSE){
      if( SelectItem == RECTCLIP || SelectItem == FREEFORM)
         WriteSelRectBitmap();
   }
}

/****************************************/
/*             */
/* MESSAGE     "WM_KEYDOWN"   */
/*             */
/****************************************/
void
CEditWnd::OnKeyDown(
UINT  nChar,
UINT  nRepCnt,
UINT  nFlags)
{
   if( nChar == VK_ESCAPE)
      WriteSelRectBitmap();
   else  CMDIChildWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

/****************************************/
/*             */
/* Draw Caption         */
/*             */
/****************************************/
void
CEditWnd::CaptionDraw()
{
COLORREF TextColor;
   CString  WndCaption;
   CRect CaptionRect;
   CBrush   CaptionBrush;
   CFont *OldFont;
   int   BkMode;
   CDC    dc;
   dc.Attach( ::GetDC( this->GetSafeHwnd()));

// Get brush with active caption color    
   CaptionRect.CopyRect( &EudcWndRect);
   if (bFocus)
   {
      CaptionBrush.CreateSolidBrush(::GetSysColor(COLOR_ACTIVECAPTION));
   }
   else
   {
      CaptionBrush.CreateSolidBrush(::GetSysColor(COLOR_INACTIVECAPTION));
   }
   CaptionRect.bottom = CAPTION_HEIGHT;
   dc.FillRect( &CaptionRect, &CaptionBrush);
   CaptionBrush.DeleteObject();

// Get font to draw caption
#ifdef BUILD_ON_WINNT
   OldFont = (CFont *)dc.SelectStockObject(DEFAULT_GUI_FONT);
#else
   OldFont = (CFont *)dc.SelectStockObject(SYSTEM_FONT);
#endif
   BkMode = dc.SetBkMode( TRANSPARENT);
   if (bFocus)
   {
      TextColor = dc.SetTextColor( ::GetSysColor(COLOR_CAPTIONTEXT));
   }
   else
   {
      TextColor = dc.SetTextColor( ::GetSysColor(COLOR_INACTIVECAPTIONTEXT));
   }
   WndCaption.LoadString( IDS_EDIT_STR);
   dc.TextOut( EudcWndRect.right /2 - 30, 1, WndCaption);
   dc.SelectObject( OldFont);
   dc.SetTextColor( TextColor);
   dc.SetBkMode( BkMode);
    ::ReleaseDC(NULL, dc.Detach());
}
               
/****************************************/
/*             */
/* Draw Grid line       */
/*             */
/****************************************/
void
CEditWnd::DrawGridLine(
CDC   *dc)
{
   CPen  GlyphPen;
register int   i;

// Create pen to draw grid
   GlyphPen.CreatePen( PS_SOLID, 1, COLOR_GRID);
   CPen *OldPen = dc->SelectObject( &GlyphPen);

// Draw grid
   for( i = ZoomRate - 1; i < EudcWndRect.right; i += ZoomRate){
      dc->MoveTo( i, CAPTION_HEIGHT-1);
      dc->LineTo( i, EudcWndRect.bottom);
   }
   for( i =ZoomRate +CAPTION_HEIGHT -1;i<EudcWndRect.bottom;i += ZoomRate){
      dc->MoveTo( 0, i);
      dc->LineTo( EudcWndRect.right, i);
   }
   dc->SelectObject( OldPen);
   GlyphPen.DeleteObject();
}

/****************************************/
/*             */
/* Draw OutLine         */
/*             */
/****************************************/
BOOL
CEditWnd::CurveFittingDraw(
CDC*  dc)
{
struct VHEAD   *vhd;
struct VDATA   *vp;
   CPen  CurvePen;
   CPoint   DrawPt;
   LPBYTE   pBuf;
   int   pcnt, TmpHdl, SetHdl;

   SetHdl = MKOUTHDL;

   DWORD wSize = (WORD)((( BITMAP_WIDTH +15)/16)*2)*(WORD)BITMAP_HEIGHT;
   pBuf = (LPBYTE)malloc(((( BITMAP_WIDTH +15) /16) *2) *BITMAP_HEIGHT);
   if( pBuf == NULL){
      return FALSE;
   }
   ImageBmp.GetBitmapBits( wSize, (LPVOID)pBuf);

   OInit();
   VDNew( SetHdl);
   if(( TmpHdl = OMakeOutline((LPBYTE)pBuf,
        BITMAP_WIDTH,SMOOTHLVL)) < 0){
      free( pBuf);
      OTerm();
      return FALSE;
   }
   if( MkPoly( TmpHdl, SetHdl) < 0){
      free( pBuf);
      OTerm();
      return FALSE;
   }

   VDGetHead( SetHdl, &vhd);
   if( VDGetNCont( SetHdl) > 0){
      CurvePen.CreatePen( PS_SOLID, 1, COLOR_CURVE);
      CPen *OldPen = dc->SelectObject( &CurvePen);
      CBrush *OldBrush = (CBrush *)dc->SelectStockObject( NULL_BRUSH);

      while ( vhd->next != NIL){
         vp = vhd->headp;
         pcnt = vhd->nPoints - 1;
         ZoomPoint( &DrawPt, vp->vd.x, vp->vd.y);
         dc->MoveTo( DrawPt.x, DrawPt.y);
         vp = vp->next;
         while( pcnt-- > 0){
            ZoomPoint( &DrawPt, vp->vd.x, vp->vd.y);
            dc->LineTo( DrawPt.x, DrawPt.y);
            vp = vp->next;
         }
         vhd = vhd->next;
      }
      dc->SelectObject( OldBrush);
      dc->SelectObject( OldPen);
      CurvePen.DeleteObject();
   }
   OTerm();
   free( pBuf);

   return TRUE;
}

/****************************************/
/*             */
/* Zoom coordinate of outline */
/*             */
/****************************************/
void
CEditWnd::ZoomPoint(
CPoint   *DrawPt,
int   x,
int   y)
{
   DWORD px, py;

   px = (DWORD)(x * EudcWndRect.right);
   py = (DWORD)(y * (EudcWndRect.bottom - CAPTION_HEIGHT));
   DrawPt->x = (int)( px/(BITMAP_WIDTH *4));
   DrawPt->y = (int)( py/(BITMAP_HEIGHT*4)) + CAPTION_HEIGHT;
}

/****************************************/
/*             */
/* Draw MoveRect rectangle    */
/*             */
/****************************************/
void
CEditWnd::DrawMoveRect(
CDC   *dc)
{
   CPen  *OldPen;
   CBrush   *OldBrush;

   OldPen = (CPen *)dc->SelectStockObject( BLACK_PEN);
   OldBrush = (CBrush *)dc->SelectStockObject( NULL_BRUSH);
   int OldMode = dc->SetROP2( R2_NOTXORPEN);

   dc->Rectangle( &ClipRect[DSP]);

   dc->SelectObject( OldPen);
   dc->SelectObject( OldBrush);
   dc->SetROP2( OldMode);
}

/****************************************/
/*             */
/* Draw clipping rectangle    */
/*             */
/****************************************/
void
CEditWnd::DrawStretchRect(
CDC   *dc)
{
   CPen  *OldPen;
   CBrush   *OldBrush;
   int   Left, Top;
   int   Right, Bottom;

   OldPen = (CPen *)dc->SelectStockObject( BLACK_PEN);
   OldBrush = (CBrush *)dc->SelectStockObject( BLACK_BRUSH);

   for( int i = 0; i < 8; i++){
//    Set left and right side of PickRect[]
      if( i == LUPPER || i == LLOWER || i == LMIDLE){
         Left  = ClipRect[DSP].left - Ratio;
         Right = ClipRect[DSP].left + Ratio;
      }else if( i == RUPPER || i == RLOWER || i == RMIDLE){
         Left  = ClipRect[DSP].right - Ratio;
         Right = ClipRect[DSP].right + Ratio;
      }else{
         Left  = ClipRect[DSP].left + ClipRect[DSP].Width()/2
            - Ratio;
         Right = ClipRect[DSP].left + ClipRect[DSP].Width()/2
            + Ratio;
      }

//    Set top and bottom side of PickRect[]
      if( i == LUPPER || i == RUPPER || i == MUPPER){
         Top    = ClipRect[DSP].top - Ratio;
         Bottom = ClipRect[DSP].top + Ratio;
      }else if( i == RMIDLE || i == LMIDLE){
         Top    = ClipRect[DSP].top + ClipRect[DSP].Height()/2
            - Ratio;
         Bottom = ClipRect[DSP].top + ClipRect[DSP].Height()/2
            + Ratio;
      }else{
         Top    = ClipRect[DSP].bottom - Ratio;
         Bottom = ClipRect[DSP].bottom + Ratio;
      }
      PickRect[i].SetRect( Left, Top, Right, Bottom);
      dc->InvertRect( &PickRect[i]);
   }
   dc->SelectObject( OldPen);
   dc->SelectObject( OldBrush);
}

/****************************************/
/*             */
/* Write Selected Rectangle   */
/*             */ 
/****************************************/
void           
CEditWnd::WriteSelRectBitmap()
{
   if( RectClipFlag){
      RectClipFlag = FALSE;
      DrawClipBmp();
      ToolTerm();
   }
}

/****************************************/
/*             */
/* Modurate coordinate     */
/*             */
/****************************************/
void
CEditWnd::CorrectMouseDownPoint(
CPoint   point)
{
   ptStart.x = point.x;
   ptStart.y = point.y - CAPTION_HEIGHT;
   if( ptStart.y < 0)
      ptStart.y = 0;

   if( SelectItem == FREEFORM){
      ptStart.x = (( ptStart.x + ZoomRate/2) /ZoomRate) *ZoomRate;
      ptStart.y = (( ptStart.y + ZoomRate/2) /ZoomRate) *ZoomRate;
   }else if( SelectItem != SLOPE){
      ptStart.x = ( ptStart.x /ZoomRate) *ZoomRate;
      ptStart.y = ( ptStart.y /ZoomRate) *ZoomRate;
   }else{
      ptStart.x = ( ptStart.x /ZoomRate) *ZoomRate +ZoomRate /2;
      ptStart.y = ( ptStart.y /ZoomRate) *ZoomRate +ZoomRate /2;
   }
   if( SelectItem == RECTBAND || SelectItem == RECTFILL   ||
       SelectItem == CIRCLE   || SelectItem == CIRCLEFILL ){
      if( ptStart.x > EudcWndRect.right - ZoomRate)
         ptStart.x -= ZoomRate;
      if( ptStart.y > EudcWndRect.bottom - CAPTION_HEIGHT - ZoomRate)
         ptStart.y -= ZoomRate;
      ptEnd.x = ptPrev.x = ptStart.x + ZoomRate;
      ptEnd.y = ptPrev.y = ptStart.y + ZoomRate;
   }else ptEnd = ptPrev = ptStart;
}

/****************************************/
/*             */
/* Modurate coordinate     */
/*             */
/****************************************/
void
CEditWnd::CorrectMouseUpPoint(
CPoint   point)
{
   ptPrev = ptEnd;
   ptEnd.x = point.x;
   ptEnd.y = point.y - CAPTION_HEIGHT;

   if( ptEnd.x < 0)  ptEnd.x = 0;
   if( ptEnd.y < 0)  ptEnd.y = 0;
   if( ptEnd.x > EudcWndRect.right){
      if( SelectItem == BRUSH)
         ptEnd.x = EudcWndRect.right - ZoomRate*2;
      else if( SelectItem == FREEFORM)
         ptEnd.x = EudcWndRect.right;
      else
         ptEnd.x = EudcWndRect.right - ZoomRate;
   }
   if( ptEnd.y > EudcWndRect.bottom - CAPTION_HEIGHT){
      if( SelectItem == BRUSH)
         ptEnd.y = EudcWndRect.bottom - CAPTION_HEIGHT
            - ZoomRate*2;
      else if( SelectItem == FREEFORM)
         ptEnd.y = EudcWndRect.bottom - CAPTION_HEIGHT;
      else
         ptEnd.y = EudcWndRect.bottom - CAPTION_HEIGHT
            - ZoomRate;
   }
   if( SelectItem == FREEFORM){
      ptEnd.x = (( ptEnd.x + ZoomRate/2) /ZoomRate) *ZoomRate ;
      ptEnd.y = (( ptEnd.y + ZoomRate/2) /ZoomRate) *ZoomRate ;
   }else if( SelectItem != SLOPE){
      ptEnd.x = ( ptEnd.x /ZoomRate)*ZoomRate;
      ptEnd.y = ( ptEnd.y /ZoomRate)*ZoomRate;
   }else{
      ptEnd.x = ( ptEnd.x /ZoomRate) *ZoomRate + ZoomRate /2;
      ptEnd.y = ( ptEnd.y /ZoomRate) *ZoomRate + ZoomRate /2;
   }
   if( SelectItem == RECTBAND || SelectItem == RECTFILL   ||
       SelectItem == CIRCLE   || SelectItem == CIRCLEFILL ){
      if( ptEnd.x - ptStart.x <= ZoomRate &&
         ptEnd.x - ptStart.x >= 0)
         ptEnd.x = ptStart.x + ZoomRate;
      if( ptStart.x - ptEnd.x <= ZoomRate &&
         ptStart.x - ptEnd.x > 0)
         ptEnd.x = ptStart.x - ZoomRate;
      if( ptStart.y - ptEnd.y <= ZoomRate &&
         ptStart.y - ptEnd.y > 0)
         ptEnd.y = ptStart.y - ZoomRate;
      if( ptEnd.y - ptStart.y <= ZoomRate &&
         ptEnd.y -ptStart.y >= 0)
         ptEnd.y = ptStart.y + ZoomRate;
   }
}

/****************************************/
/*             */
/* Move Selected Rectangle    */
/*             */
/****************************************/
BOOL
CEditWnd::MoveClipRect()
{
   int   Movex, Movey;
   int   Wid, High;
   BOOL  sts;

   sts = TRUE;
   ClipRect[PRV].CopyRect( &ClipRect[DSP]);

// Boundary condition
   Movex = ptEnd.x - ptPrev.x;
   Movey = ptEnd.y - ptPrev.y;
   Wid  = ClipRect[DSP].Width();
   High = ClipRect[DSP].Height();
   ClipRect[DSP].OffsetRect( Movex, Movey);

   if( ClipRect[DSP].left < 0){
      ClipRect[DSP].left = 0;
      ClipRect[DSP].right = Wid;
   }
   if( ClipRect[DSP].right > EudcWndRect.right){
      ClipRect[DSP].right = EudcWndRect.right + 1;
      ClipRect[DSP].left  = ClipRect[DSP].right - Wid;
   }
   if( ClipRect[DSP].top < CAPTION_HEIGHT){
      ClipRect[DSP].top = CAPTION_HEIGHT;
      ClipRect[DSP].bottom = CAPTION_HEIGHT +High;
   }
   if( ClipRect[DSP].bottom > EudcWndRect.bottom){
      ClipRect[DSP].bottom = EudcWndRect.bottom +1;
      ClipRect[DSP].top = ClipRect[DSP].bottom - High;
   }
   if( ClipRect[PRV].EqualRect( &ClipRect[DSP])){
      sts = FALSE;
   }
   return sts;
}

/************************************************/
/*                */
/* whether point is in rectangle or not   */
/*                */
/************************************************/
int
CEditWnd::CheckClipRect(
POINT    ClipPoint)
{
   for( int i = 0; i < 8; i++){
      if( PickRect[i].PtInRect( ClipPoint))
         return i;
   }
   return NCHECK;
}

/****************************************/
/*             */
/* Process to stretch rectangle  */
/*             */
/****************************************/
BOOL
CEditWnd::DrawStretchClipToDisp()
{
   CDC   SwapDC;
   CBitmap  SwapBmp;
   CRect SwapRect;
            
   CClientDC   dc( this);

   if( !SwapDC.CreateCompatibleDC( &dc))
      return FALSE;

   if( !SwapBmp.CreateCompatibleBitmap( &SwapDC, EudcWndRect.Width(),
            EudcWndRect.Height()))
      return FALSE;
   CBitmap *OldBitmap = SwapDC.SelectObject( &SwapBmp);

   SwapRect.CopyRect( &ClipRect[DSP]);
   if( EudcWndRect.right < SwapRect.right)
      SwapRect.right -= 1;
   if( EudcWndRect.bottom < SwapRect.bottom)
      SwapRect.bottom -= 1;
   
   SwapDC.StretchBlt( SwapRect.left, SwapRect.top,
         SwapRect.Width(), SwapRect.Height(),
         &CRTDrawDC, ClipRect[BMP].left, ClipRect[BMP].top,
         ClipRect[BMP].Width(), ClipRect[BMP].Height(),
         SRCCOPY);

   SetPickRect();
   CRTDrawDC.StretchBlt( ClipRect[BMP].left, ClipRect[BMP].top,
      ClipRect[BMP].Width(), ClipRect[BMP].Height(),
      &SwapDC, SwapRect.left, SwapRect.top,
      SwapRect.Width(), SwapRect.Height(), SRCCOPY);

   SwapDC.SelectObject( OldBitmap);
   SwapBmp.DeleteObject();
   SwapDC.DeleteDC();

   return TRUE;
}

/****************************************/
/*             */
/* Draw select rectangle to bitmap  */
/*             */
/****************************************/
void
CEditWnd::DrawClipBmp()
{
   CRect pRect;

   pRect.SetRect(( ClipRect[DSP].left + 1) /ZoomRate,
       ( ClipRect[DSP].top - CAPTION_HEIGHT + 1) /ZoomRate,
       ( ClipRect[DSP].right + 1)/ZoomRate,
       ( ClipRect[DSP].bottom - CAPTION_HEIGHT + 1) /ZoomRate);

   ImageDC.StretchBlt( pRect.left, pRect.top,
         pRect.Width(), pRect.Height(), &CRTDrawDC,
         ClipRect[BMP].left, ClipRect[BMP].top,
      ClipRect[BMP].Width(), ClipRect[BMP].Height(), SRCAND);
   SetValidRect();
   this->InvalidateRect( &ClipRect[VLD], FALSE);
   this->UpdateWindow();
}

/****************************************/
/*             */
/* Set size of clipping rectangle   */
/*             */
/****************************************/
void
CEditWnd::SetPickRect()
{
   ClipRect[BMP].SetRect( ClipRect[DSP].left /ZoomRate,
      ( ClipRect[DSP].top - CAPTION_HEIGHT) /ZoomRate,
        ClipRect[DSP].right /ZoomRate,
      ( ClipRect[DSP].bottom - CAPTION_HEIGHT) /ZoomRate);
}

/****************************************/
/*             */
/* Set size of clipping rectangle   */
/*             */
/****************************************/
void
CEditWnd::SetMoveRect()
{
   IllegalRect( &ptStart, &ptEnd);

   ClipRect[DSP].SetRect( ptStart.x, ptStart.y + CAPTION_HEIGHT,
      ptEnd.x + ZoomRate + 1,
      ptEnd.y + ZoomRate + 1 + CAPTION_HEIGHT);
   ClipRect[PRV].CopyRect( &ClipRect[DSP]);
}

/****************************************/
/*             */
/* Set size of clipping rectangle   */
/*             */
/****************************************/
void
CEditWnd::SetValidRect()
{
   ClipRect[VLD].SetRect( ClipRect[DSP].left - Ratio,
      ClipRect[DSP].top - Ratio,
      ClipRect[DSP].right + Ratio,
      ClipRect[DSP].bottom + Ratio);
}

/****************************************/
/*             */
/* Set size of clipping rectangle   */
/*             */
/****************************************/
void
CEditWnd::SetClickRect()
{
   ClipRect[PIN].SetRect( ClipRect[DSP].left + Ratio,
      ClipRect[DSP].top + Ratio,
      ClipRect[DSP].right - Ratio,
      ClipRect[DSP].bottom - Ratio);
}

/****************************************/
/*             */
/* Draw a figure to bitmap    */
/*             */
/****************************************/
void
CEditWnd::DrawRectBmp()
{
   CPen  *OldPen;
   CBrush   *OldBrush;
   CRect    BRect;
   CPoint   ptLT;
   CPoint   ptRB;

   ptLT.x = ptStart.x /ZoomRate;
   ptLT.y = ptStart.y /ZoomRate;
   ptRB.x = ptEnd.x   /ZoomRate;
   ptRB.y = ptEnd.y   /ZoomRate;
   
   if( ButtonFlag == LCLICK)
      OldPen = (CPen *)ImageDC.SelectStockObject( BLACK_PEN);
   else  OldPen = (CPen *)ImageDC.SelectStockObject( WHITE_PEN);

   if( SelectItem == RECTBAND || SelectItem == CIRCLE)
      OldBrush = (CBrush *)ImageDC.SelectStockObject( NULL_BRUSH);
   else if( ButtonFlag == LCLICK)
      OldBrush = (CBrush *)ImageDC.SelectStockObject( BLACK_BRUSH);
   else  OldBrush = (CBrush *)ImageDC.SelectStockObject( WHITE_BRUSH);

   BRect.SetRect( ptLT.x, ptLT.y, ptRB.x +1, ptRB.y +1);
   if( SelectItem == CIRCLE || SelectItem == CIRCLEFILL)
      ImageDC.Ellipse( &BRect);
   else if( SelectItem == RECTBAND || SelectItem == RECTFILL)
      ImageDC.Rectangle( &BRect);
   else{
      ImageDC.MoveTo( ptLT);
      ImageDC.LineTo( ptRB);
      if (SelectItem == ERASER)
         DrawPoint( ptEnd, TRUE);
      else
         DrawPoint( ptEnd, FALSE);
   }
   ImageDC.SelectObject( OldPen);
   ImageDC.SelectObject( OldBrush);
}

/****************************************/
/*             */
/* Draw 1Point       */
/*             */
/****************************************/
void
CEditWnd::DrawPoint( CPoint Pt, BOOL bErase)
{
   CBrush   PointBrush,*OldBrush1, *OldBrush2;
   CPoint   W;
   int   Wstep, i, j;
         
   CClientDC   dc( this);
   if( ButtonFlag == LCLICK && !bErase){
      if( FlagTmp)
         PointBrush.CreateSolidBrush( COLOR_FITTING);
      else  PointBrush.CreateSolidBrush( COLOR_BLACK);
   }else PointBrush.CreateSolidBrush( COLOR_WHITE);
   
   OldBrush1 = dc.SelectObject( &PointBrush);
   

   W = Pt;
   Wstep = ZoomRate;
   if( ZoomRate > 1 && GridShow)
      Wstep -= 1;
   for( i = 0; i < BrushWidth; i++, W.x += ZoomRate){
      W.y = Pt.y;
      for( j = 0; j < BrushWidth; j++ , W.y += ZoomRate)
         dc.PatBlt( W.x, W.y+CAPTION_HEIGHT,Wstep,Wstep,PATCOPY);
   }
   dc.SelectObject( OldBrush1);
   PointBrush.DeleteObject();
   
   if( ButtonFlag == LCLICK && !bErase)
      OldBrush2 = (CBrush *)ImageDC.SelectStockObject(BLACK_BRUSH);
   else  OldBrush2 = (CBrush *)ImageDC.SelectStockObject(WHITE_BRUSH);

   W.x = Pt.x /ZoomRate;
   W.y = Pt.y /ZoomRate;
   if( W.x < BITMAP_WIDTH && W.y < BITMAP_HEIGHT)
      ImageDC.PatBlt( W.x, W.y, BrushWidth, BrushWidth, PATCOPY);
   ImageDC.SelectObject( OldBrush2);
   
}

/****************************************/
/*             */
/* Degital differencial analyzer */
/*             */
/****************************************/
void
CEditWnd::DrawToPoint(BOOL bErase)
{
   CPoint   Pt, Start, End, Inc;
   int   Xf, Yf;
   int   Inc1, Inc2;
   int   Dx, Dy,  D, Tmp;
   BOOL  Slope;

   End.x = Xf = ptEnd.x /ZoomRate;
   End.y = Yf = ptEnd.y /ZoomRate;
   Start.x = ptPrev.x   /ZoomRate;
   Start.y = ptPrev.y   /ZoomRate;

   if( Xf >= Start.x)
      Inc.x = 1;
   else  Inc.x = -1;
   if( Yf >= Start.y)
      Inc.y = 1;
   else  Inc.y = -1;

   Dx = ( Xf - Start.x)*Inc.x;
   Dy = ( Yf - Start.y)*Inc.y;
   if( !Dx && !Dy)   return;
   if( Dx < Dy){
      Tmp = Dy;
      Dy = Dx;
      Dx = Tmp;
      Tmp = Inc.x;
      Inc.x = Inc.y;
      Inc.y = Tmp;
      Slope = TRUE;
   }else   Slope = FALSE;

   Inc1 = Dy*2;
   Inc2 = (Dy - Dx)*2;
   D = Inc1 - Dx;
   End.x = Start.x;
   End.y = Start.y;  
   while(1){
      Pt.x = End.x *ZoomRate;
      Pt.y = End.y *ZoomRate;
      DrawPoint( Pt, bErase);
      if( End.x == Xf && End.y == Yf)
         break;
      if( Slope){
         Tmp = End.x;
         End.x = End.y;
         End.y = Tmp;
      }
      End.x += Inc.x;
      if( D < 0)  D += Inc1;
      else{
         End.y += Inc.y;
         D += Inc2;
      }
      if( Slope){
         Tmp = End.x;
         End.x = End.y;
         End.y = Tmp;
      }
   }
}

/****************************************/
/*             */
/* Initialize tool width and color */
/*             */
/****************************************/
void
CEditWnd::ToolInit(
int   LRButton)
{
   this->SetCapture();
   ButtonFlag = LRButton;
   IsCapture = TRUE;
   if( SelectItem == PEN || SelectItem == SLOPE)
      BrushWidth = 1;
   else if( SelectItem == BRUSH || SelectItem == ERASER)
      BrushWidth = 2;
}

/****************************************/
/*             */ 
/* Rubberband stretch mode    */
/*             */
/****************************************/
void
CEditWnd::StretchMoveRect()
{
   CPen  *OldPen;
   CBrush   *OldBrush;
   int   Left, Top, Right, Bottom;

   CClientDC   dc( this);

   OldPen   = (CPen *)dc.SelectStockObject( BLACK_PEN);
   OldBrush = (CBrush *)dc.SelectStockObject( NULL_BRUSH);
   int OldMode = dc.SetROP2( R2_NOTXORPEN);
   dc.Rectangle( &ClipRect[PRV]);

   Left   = ClipRect[DSP].left;
   Top    = ClipRect[DSP].top;
   Right  = ClipRect[DSP].right;
   Bottom = ClipRect[DSP].bottom;
   switch( CheckNum){
      case RLOWER:
         ClipRect[DSP].SetRect( Left, Top,
            ptEnd.x + ZoomRate + 1,
            ptEnd.y + CAPTION_HEIGHT + ZoomRate + 1);

         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].bottom = ClipRect[DSP].top
               + ZoomRate*2+1;

         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].right = ClipRect[DSP].left
               + ZoomRate*2+1;
         break;
      case RMIDLE:
         ClipRect[DSP].SetRect( Left, Top,
               ptEnd.x + ZoomRate + 1, Bottom);

         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].right = ClipRect[DSP].left
               + ZoomRate*2+1;
         break;
      case MLOWER:
         ClipRect[DSP].SetRect( Left, Top,
            Right, ptEnd.y + CAPTION_HEIGHT + ZoomRate + 1);

         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].bottom = ClipRect[DSP].top
               + ZoomRate*2+1;
         break;
      case RUPPER:
         ClipRect[DSP].SetRect( Left, ptEnd.y + CAPTION_HEIGHT,
               ptEnd.x + ZoomRate + 1, Bottom);

         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].top = ClipRect[DSP].bottom
               - ZoomRate*2+1;
         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].right = ClipRect[DSP].left
               + ZoomRate*2+1;
         break;
      case LLOWER:
         ClipRect[DSP].SetRect( ptEnd.x, Top,
            Right, ptEnd.y + CAPTION_HEIGHT + ZoomRate + 1);

         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].left = ClipRect[DSP].right
               - ZoomRate*2+1;
         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].bottom = ClipRect[DSP].top
               + ZoomRate*2+1;
         break;
      case LMIDLE:
         ClipRect[DSP].SetRect( ptEnd.x, Top, Right, Bottom);

         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].left = ClipRect[DSP].right
               - ZoomRate*2+1;
         break;
      case MUPPER:
         ClipRect[DSP].SetRect( Left, ptEnd.y + CAPTION_HEIGHT,
                  Right, Bottom);

         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].top = ClipRect[DSP].bottom
               - ZoomRate*2+1;
         break;
      case LUPPER:
         ClipRect[DSP].SetRect( ptEnd.x,
            ptEnd.y + CAPTION_HEIGHT, Right, Bottom);

         if( ClipRect[DSP].Height() < ZoomRate*2)
            ClipRect[DSP].top = ClipRect[DSP].bottom
               - ZoomRate*2+1;
         if( ClipRect[DSP].Width() < ZoomRate*2)
            ClipRect[DSP].left = ClipRect[DSP].right
               - ZoomRate*2+1;
         break;
      default:
         break;
   }
   ClipRect[PRV].CopyRect( &ClipRect[DSP]);
   dc.Rectangle( &ClipRect[DSP]);
   dc.SelectObject( OldPen);
   dc.SelectObject( OldBrush);
   dc.SetROP2( OldMode);
}

/****************************************/
/*             */
/*       FreeForm       */
/*             */
/****************************************/
void
CEditWnd::SelectFreeForm(
BOOL  MouseSts)
{
   CPoint   Ep, Sp, Cp;
   CPoint   Fp, Inc;
   CPoint   Dp, Err;
   BOOL  Slope;
   int   D;
   int   Tmp;

   if( !MouseSts){
      Sp.x = ptPrev.x /ZoomRate;
      Sp.y = ptPrev.y /ZoomRate;
      Ep.x = Fp.x = ptEnd.x /ZoomRate;
      Ep.y = Fp.y = ptEnd.y /ZoomRate;
   }else{
      Sp.x = ptEnd.x /ZoomRate;
      Sp.y = ptEnd.y /ZoomRate;
      Ep.x = Fp.x = ptStart.x /ZoomRate;
      Ep.y = Fp.y = ptStart.y /ZoomRate;
   }

   if( Fp.x >= Sp.x)
      Inc.x = 1;
   else  Inc.x = -1;
   if( Fp.y >= Sp.y)
      Inc.y = 1;
   else  Inc.y = -1;

   Dp.x = ( Fp.x - Sp.x)*Inc.x;
   Dp.y = ( Fp.y - Sp.y)*Inc.y;
   if( !Dp.x && !Dp.y)  return;
   if( Dp.x < Dp.y){
      Tmp = Dp.y;
      Dp.y = Dp.x;
      Dp.x = Tmp;
      Tmp = Inc.x;
      Inc.x = Inc.y;
      Inc.y = Tmp;
      Slope = TRUE;
   }else   Slope = FALSE;

   Err.x = Dp.y * 2;
   Err.y = ( Dp.y - Dp.x) * 2;
   D = Err.x - Dp.x;
   Ep = Sp;
   while(1){
      m_SelectArray.Add( Ep);
      if( Ep.x == Fp.x && Ep.y == Fp.y)
         break;
      if( Slope){
         Tmp = Ep.x;
         Ep.x = Ep.y;
         Ep.y = Tmp;
      }
      Ep.x += Inc.x;
      if( D < 0)  D += Err.x;
      else{
         Ep.y += Inc.y;
         D += Err.y;
      }
      if( Slope){
         Tmp = Ep.x;
         Ep.x = Ep.y;
         Ep.y = Tmp;
      }
   }
}

/****************************************/
/*             */
/*       FreeForm       */
/*             */
/****************************************/
void
CEditWnd::DrawFreeForm(
BOOL  MouseSts)
{
   CPoint   Ep, Sp, Cp;
   CPoint   Fp, Inc;
   CPoint   Dp, Err;
   CPoint   P1, P2;
   BOOL  Slope;
   int   D;
   int   Tmp;

   CClientDC   dc( this);

   CPen  *OldPen = (CPen *)dc.SelectStockObject( BLACK_PEN);
   int OldMode = dc.SetROP2( R2_NOTXORPEN);

   if( !MouseSts){
      Sp.x = ptPrev.x;
      Sp.y = ptPrev.y + CAPTION_HEIGHT;
      Ep.x = Fp.x = ptEnd.x;
      Ep.y = Fp.y = ptEnd.y + CAPTION_HEIGHT;
   }else{
      Sp.x = ptEnd.x;
      Sp.y = ptEnd.y + CAPTION_HEIGHT;
      Ep.x = Fp.x = ptStart.x;
      Ep.y = Fp.y = ptStart.y + CAPTION_HEIGHT;
   }
   if( Fp.x >= Sp.x)
      Inc.x = ZoomRate;
   else  Inc.x = 0 - ZoomRate;
   if( Fp.y >= Sp.y)
      Inc.y = ZoomRate;
   else  Inc.y = 0 - ZoomRate;

   Dp.x = ( Fp.x - Sp.x)*Inc.x;
   Dp.y = ( Fp.y - Sp.y)*Inc.y;
   if( !Dp.x && !Dp.y)  return;
   if( Dp.x < Dp.y){
      Tmp = Dp.y;
      Dp.y = Dp.x;
      Dp.x = Tmp;
      Tmp = Inc.x;
      Inc.x = Inc.y;
      Inc.y = Tmp;
      Slope = TRUE;
   }else   Slope = FALSE;

   Err.x = Dp.y * 2;
   Err.y = ( Dp.y - Dp.x) * 2;
   D = Err.x - Dp.x;
   Ep = Sp;
   dc.MoveTo( Sp);
   while(1){
      if( Sp.x != Ep.x && Sp.y != Ep.y){
         if( Sp.y < Ep.y && Sp.x > Ep.x){
            Cp.x = Sp.x;
            Cp.y = Ep.y;
         }else if( Sp.y < Ep.y && Sp.x < Ep.x){
            Cp.x = Sp.x;
            Cp.y = Ep.y;
         }else if( Sp.y > Ep.y && Sp.x > Ep.x){
            Cp.y = Sp.y;
            Cp.x = Ep.x;
         }else{
            Cp.y = Sp.y;
            Cp.x = Ep.x;
         }
         dc.LineTo( Cp);
         dc.LineTo( Ep);
         P1 = Cp;
         P2 = Ep;
         m_pointArray.Add( P1);
         m_pointArray.Add( P2);
      }else if( Sp.x != Ep.x || Sp.y != Ep.y){
         dc.LineTo( Ep);
         P1 = Ep;
         m_pointArray.Add( P1);
      }
      Sp.x = Ep.x;
      Sp.y = Ep.y;
      if( Ep.x == Fp.x && Ep.y == Fp.y)
         break;
      if( Slope){
         Tmp = Ep.x;
         Ep.x = Ep.y;
         Ep.y = Tmp;
      }
      Ep.x += Inc.x;
      if( D < 0)  D += Err.x;
      else{
         Ep.y += Inc.y;
         D += Err.y;
      }
      if( Slope){
         Tmp = Ep.x;
         Ep.x = Ep.y;
         Ep.y = Tmp;
      }
   }
   dc.SelectObject( OldPen);
   dc.SetROP2( OldMode);
}

/****************************************/
/*             */
/*   Rubber Band( rectangle and circle)   */
/*             */
/****************************************/
void
CEditWnd::DrawRubberBand(
BOOL  StretchFlag)
{
   CRect RubberBandRect;
   CPoint   ptLT, ptRB;
   int   OldMode;

   CClientDC   dc( this);

   CPen  *OldPen   = (CPen *)dc.SelectStockObject( BLACK_PEN);
   CBrush   *OldBrush = (CBrush *)dc.SelectStockObject( NULL_BRUSH);
   OldMode = dc.SetROP2( R2_NOTXORPEN);

   if( !StretchFlag){
      ptLT.x = ptStart.x;
      ptLT.y = ptStart.y + CAPTION_HEIGHT;
      ptRB.x = ptPrev.x;
      ptRB.y = ptPrev.y  + CAPTION_HEIGHT;
      if( SelectItem != SLOPE)
         IllegalRect( &ptLT, &ptRB);
      RubberBandRect.SetRect( ptLT.x, ptLT.y,
         ptRB.x + ZoomRate + 1, ptRB.y + ZoomRate + 1);
      if( SelectItem == SLOPE){
         dc.MoveTo( ptLT);
         dc.LineTo( ptRB);
      }else if( SelectItem == CIRCLE || SelectItem == CIRCLEFILL)
         dc.Ellipse( &RubberBandRect);
      else  dc.Rectangle( &RubberBandRect);
   }
   ptLT.x = ptStart.x;
   ptLT.y = ptStart.y + CAPTION_HEIGHT;
   ptRB.x = ptEnd.x;
   ptRB.y = ptEnd.y + CAPTION_HEIGHT;
   ptPrev = ptRB;
   if( SelectItem != SLOPE)
      IllegalRect( &ptLT, &ptRB);
      RubberBandRect.SetRect( ptLT.x, ptLT.y,
         ptRB.x + ZoomRate + 1, ptRB.y + ZoomRate + 1);
   if( SelectItem == SLOPE){
      dc.MoveTo( ptLT);
      dc.LineTo( ptRB);
   }else if( SelectItem == CIRCLE || SelectItem == CIRCLEFILL)
      dc.Ellipse( &RubberBandRect);
   else  dc.Rectangle( &RubberBandRect);
   
   dc.SelectObject( OldPen);
   dc.SelectObject( OldBrush);
   dc.SetROP2( OldMode);
}

/****************************************/
/*             */
/* Correct coordinate of rectangle  */
/*             */
/****************************************/
void
CEditWnd::IllegalRect(
PPOINT   ptTL,
PPOINT   ptBR)
{
   int   Tmp;

   if( ptTL->x > ptBR->x){
      Tmp = ptTL->x;
      ptTL->x = ptBR->x;
      ptBR->x = Tmp; 
   }
   if( ptTL->y > ptBR->y){
      Tmp = ptTL->y;
      ptTL->y = ptBR->y;
      ptBR->y = Tmp;
   }
}

/****************************************/
/*             */
/* Process to term tool    */
/*             */
/****************************************/
void
CEditWnd::ToolTerm()
{
   if( IsCapture){
      IsCapture = FALSE;
      ReleaseCapture();
   }
   ButtonFlag = NCLICK;
}


/****************************************/
/*             */
/* Set Freeform selection     */
/*             */
/****************************************/
BOOL
CEditWnd::SetFreeForm()
{
   CBrush   EraseBrush;
   CBrush   BlackBrush;

   PickRgn.GetRgnBox( &ClipRect[BMP]);
   if( ClipRect[BMP].Width() < 3 ||
       ClipRect[BMP].Height() < 3){
      FreeRgn.GetRgnBox( &ClipRect[DSP]);
      ClipRect[DSP].right += 1;
      ClipRect[DSP].bottom += 1;
         return FALSE;
   }
   EraseBrush.CreateStockObject( WHITE_BRUSH);
   BlackBrush.CreateStockObject( BLACK_BRUSH);
   CRTDrawDC.FillRect( &ClipRect[BMP], &EraseBrush);
   CRTDrawDC.FillRgn( &PickRgn, &BlackBrush);
   EraseBrush.DeleteObject();
   BlackBrush.DeleteObject();

      CRTDrawDC.BitBlt( ClipRect[BMP].left, ClipRect[BMP].top,
      ClipRect[BMP].Width(), ClipRect[BMP].Height(),
      &ImageDC, ClipRect[BMP].left, ClipRect[BMP].top, SRCPAINT);


   FreeRgn.GetRgnBox( &ClipRect[DSP]);
   ClipRect[DSP].right += 1;
   ClipRect[DSP].bottom += 1;

   return TRUE;
}

/****************************************/
/*             */
/* Erase Freeform selection area */
/*             */
/****************************************/
void
CEditWnd::EraseFreeForm()
{
   CBrush   CutBrush;
   CutBrush.CreateSolidBrush( COLOR_WHITE );
      ImageDC.FillRgn( &PickRgn, &CutBrush);
   CutBrush.DeleteObject();
}

/****************************************/
/*             */
/* Erase rectangle selection area   */
/*             */
/****************************************/
void
CEditWnd::EraseRectangle()
{
   CRect pRect;
   CPen  *OldPen;
   CBrush   CutBrush;

   OldPen = (CPen *)ImageDC.SelectStockObject( NULL_PEN);
   CutBrush.CreateSolidBrush( COLOR_WHITE );
      CBrush *OldBrush = ImageDC.SelectObject( &CutBrush);

   pRect.CopyRect( &ClipRect[BMP]);
   pRect.right  += 1;
   pRect.bottom += 1;
      ImageDC.Rectangle( &pRect);
      ImageDC.SelectObject( OldPen);
      ImageDC.SelectObject( OldBrush);
   CutBrush.DeleteObject();

   RectClipFlag = FALSE;
   SetValidRect();
   this->InvalidateRect( &ClipRect[VLD], FALSE);
   this->UpdateWindow();
   RectClipFlag = TRUE;
}

/****************************************/
/*             */
/* Save character with same code */
/*             */
/****************************************/
#define ABORT  1
#define CANCEL 0
int 
CEditWnd::SaveEUDCCode(UINT msgBoxType)
{
  CClientDC dc(this);
   CFont cFont, *OldFont;
   LOGFONT  LogFont;
   DWORD wSize;
   LPBYTE   pBuf;
   LPBYTE   pBuf2;
   TCHAR BMPPath[MAX_PATH];
   TCHAR TTFPath[MAX_PATH];
   TCHAR *FilePtr;
   int   FontType, sts=0;
//fix for FontIsLinked
  BOOL firstTime = false;
//
   if( SelectItem == RECTCLIP || SelectItem == FREEFORM)
      WriteSelRectBitmap();

   lstrcpy(TTFPath, SelectEUDC.m_File);
   lstrcpy(BMPPath,TTFPath);
   if(( FilePtr = Mytcsrchr( BMPPath, '.')) != NULL)
      *FilePtr = '\0';
   lstrcat( BMPPath, TEXT(".EUF"));

   if( !EnableEUDC( FALSE)){
      MessageBox(TEXT("EnableEUDC() Error"), TEXT("EudcEditor"), MB_OK);
    return ABORT;
   }

//fix for FontIsLinked
//create logFont first, then use this font to create new character
  OInit();
  if( !OExistTTF( TTFPath)){
    firstTime = TRUE;
     memset( &LogFont, 0, sizeof( LogFont));
    lstrcpy (LogFont.lfFaceName, CountryInfo.szForceFont);
     LogFont.lfHeight  = BITMAP_HEIGHT;
     LogFont.lfWidth   = BITMAP_WIDTH;
     LogFont.lfOutPrecision  = OUT_TT_ONLY_PRECIS;
     LogFont.lfCharSet = (BYTE)CountryInfo.CharacterSet;
     LogFont.lfPitchAndFamily = FF_ROMAN; //| FIXED_PITCH;
    if (CountryInfo.LangID == EUDC_JPN)
    {
      //  JPN platform, use fixed pitch font.
       LogFont.lfPitchAndFamily |= FIXED_PITCH;
    }
     if( !cFont.CreateFontIndirect( &LogFont)){
        OutputMessageBox(this->GetSafeHwnd(),
           IDS_ASSOCIATE_DLGTITLE,
           IDS_SELECTFONT_ERROR, TRUE);
      OTerm();
      EnableEUDC(TRUE);
        return ABORT;
    }
     OldFont = dc.SelectObject( &cFont);

    FontType = !SelectEUDC.m_FontTypeFlg? 1:0;
     if( OCreateTTF( dc.GetSafeHdc(), TTFPath, FontType))
    {
      dc.SelectObject(OldFont);
      cFont.DeleteObject();
      OTerm();
      EnableEUDC(TRUE);
       return ABORT;
    }
    dc.SelectObject(OldFont);
     cFont.DeleteObject();
  }

   wSize = (DWORD)((( BITMAP_WIDTH +15) /16) *2) *(WORD)BITMAP_HEIGHT;
   pBuf = (LPBYTE)malloc(((( BITMAP_WIDTH +15) /16) *2) *BITMAP_HEIGHT);
   if( pBuf == NULL){
    OTerm();
    EnableEUDC(TRUE);
      return ABORT;
   }
   pBuf2 = pBuf;
   ImageBmp.GetBitmapBits( wSize, (LPVOID)pBuf2);     

   if( OMakeOutline( (LPBYTE)pBuf2, BITMAP_WIDTH, SMOOTHLVL) < 0)
  {
    free(pBuf);
    OTerm();
    EnableEUDC(TRUE);
      return ABORT;
  }
   sts = OOutTTF( dc.GetSafeHdc(),
                   TTFPath,
                   (unsigned short)UpdateCode,
                   CountryInfo.bUnicodeMode);
  if (sts == -3) //tte file is being used by another process
  {
    free(pBuf);
    OTerm();
    EnableEUDC(TRUE);

    HINSTANCE hInst = AfxGetInstanceHandle();
    TCHAR szMessage[256];
    LoadString(hInst, IDS_OVERWRITEFAIL, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    TCHAR szTitle[256];
    LoadString(hInst, IDS_MAINFRAMETITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
    if (MessageBox( szMessage, szTitle, msgBoxType) == IDCANCEL)
      return CANCEL;
    else
      return ABORT;
  }
  else if (sts < 0)
  {
    free(pBuf);
    OTerm();
    EnableEUDC(TRUE);

    OutputMessageBox( this->GetSafeHwnd(),
         IDS_MAINFRAMETITLE,
         IDS_NOTENOUGHMEMORY_ERROR, TRUE);
    return ABORT;
  }

//fix for FontIsLinked
  if (firstTime)
  {
      if (creatW31JEUDC(BMPPath))
    {
      free(pBuf);
      OTerm();
      EnableEUDC(TRUE);
      return ABORT;
    }
  }
//
   if( OpenW31JEUDC( BMPPath)){
    free(pBuf);
    OTerm();
    EnableEUDC(TRUE);
    return ABORT;
   }
   if( PutW31JEUDCFont((unsigned short)UpdateCode, pBuf2, BITMAP_WIDTH,
             BITMAP_HEIGHT, CountryInfo.bUnicodeMode)){
      CloseW31JEUDC();
    free(pBuf);
    OTerm();
    EnableEUDC(TRUE);
    return ABORT;
   }
   CloseW31JEUDC();
   BitmapDirty = FALSE;
   g_bKeepEUDCLink = TRUE;

  free(pBuf);
  OTerm();
  EnableEUDC(TRUE);
  return ABORT;
}

/****************************************/
/*             */
/* draw image for undo     */
/*             */
/****************************************/
void
CEditWnd::UndoImageDraw()
{
   CDC   UndoDC;

   if( !UndoBitmapFlag)
         CreateUndoBitmap();

      UndoDC.CreateCompatibleDC( &ImageDC);
      CBitmap *OldBitmap = UndoDC.SelectObject( &UndoImage);

      UndoDC.BitBlt( 0, 0, BITMAP_WIDTH, BITMAP_HEIGHT,
      &ImageDC, 0, 0, SRCCOPY);

   if( RectClipFlag && (SelectItem == RECTCLIP || SelectItem == FREEFORM)){
      CRect pRect;

      pRect.SetRect(( ClipRect[DSP].left + 1) /ZoomRate,
          ( ClipRect[DSP].top - CAPTION_HEIGHT +1) /ZoomRate,
          ( ClipRect[DSP].right + 1)/ZoomRate,
          ( ClipRect[DSP].bottom - CAPTION_HEIGHT +1) /ZoomRate);

      UndoDC.StretchBlt( pRect.left, pRect.top,
         pRect.Width(), pRect.Height(), &CRTDrawDC,
         ClipRect[BMP].left, ClipRect[BMP].top,
         ClipRect[BMP].Width(), ClipRect[BMP].Height(), SRCAND);
   }
      UndoDC.SelectObject( OldBitmap);
      UndoDC.DeleteDC();
   UndoBitmapFlag = TRUE;
}

/****************************************/
/*             */
/*    for Update     */
/*             */
/****************************************/
BOOL
CEditWnd::SelectCodes()
{
   int   xSize, ySize;
   int   wSize;
   LPBYTE   pBuf;
   TCHAR *FilePtr;
   TCHAR BMPPath[MAX_PATH];

   lstrcpy( BMPPath, SelectEUDC.m_File);
   if(( FilePtr = Mytcsrchr( BMPPath, '.')) != NULL)
      *FilePtr = '\0';
   lstrcat( BMPPath, TEXT(".EUF"));

   wSize = (int)((( BITMAP_WIDTH + 15) /16) *2) *(int)BITMAP_HEIGHT;
   pBuf = (LPBYTE)malloc( wSize);
   if( OpenW31JEUDC( BMPPath)){
      free( pBuf);
      return FALSE;
   }
   if( GetW31JEUDCFont((unsigned short)UpdateCode, pBuf, wSize,
              &xSize, &ySize, CountryInfo.bUnicodeMode)){
      CloseW31JEUDC();
      free( pBuf);
      return FALSE;
   }else if( xSize != BITMAP_WIDTH || ySize != BITMAP_HEIGHT){
      CloseW31JEUDC();
      free( pBuf);
      return FALSE;
   }
   CloseW31JEUDC();

   ImageBmp.SetBitmapBits( wSize, (LPVOID)pBuf);
   BitmapDirty = FALSE;
   free( pBuf);
   this->Invalidate( FALSE);
   this->UpdateWindow();
   return TRUE;
}

/****************************************/
/*             */
/* Clear bitmap data    */
/*             */
/****************************************/
BOOL
CEditWnd::UpdateBitmap()
{
   WORD  wSize;
   HANDLE   BitHandle;
   BYTE  *pBitmap;

   wSize = (WORD)((( BITMAP_WIDTH +15) /16) *2) *(WORD)BITMAP_HEIGHT;
   if(( BitHandle = LocalAlloc( LMEM_MOVEABLE, wSize)) == 0)
      return FALSE;

   if(( pBitmap = (BYTE *)LocalLock( BitHandle)) == NULL){
      LocalFree( BitHandle);
      return FALSE;
   }
   memset( pBitmap, 0xffff, wSize);

   ImageBmp.SetBitmapBits((DWORD)wSize, (const void far *)pBitmap);
   LocalUnlock( BitHandle);
   LocalFree( BitHandle);

   if(( SelectItem == RECTCLIP || SelectItem == FREEFORM) && RectClipFlag){
      RectClipFlag = FALSE;
      ToolTerm();
   }
   this->Invalidate( FALSE);
   this->UpdateWindow();
   BitmapDirty = FALSE;

   return TRUE;
}

/****************************************/
/*             */
/* Get bitmap dirty flag      */
/*             */
/****************************************/
BOOL
CEditWnd::GetBitmapDirty()
{
   return BitmapDirty;
}

/****************************************/
/*             */
/* Get bitmap dirty flag      */
/*             */
/****************************************/
BOOL
CEditWnd::SetBitmapDirty(
BOOL  Flg)
{
   BitmapDirty = Flg;
   return TRUE;
}

/****************************************/
/*             */
/* Call Charcter        */
/*             */
/****************************************/
void
CEditWnd::CallCharTextOut()
{
   CFont CallFont;
   CSize CharSize;
   BYTE  sWork[10];
   int   Length;
   int   xOffset, yOffset;

   EditLogFont.lfHeight  = BITMAP_HEIGHT; 
   EditLogFont.lfWeight  = 0;
   EditLogFont.lfQuality = PROOF_QUALITY;
   if( !CallFont.CreateFontIndirect( &EditLogFont))
      return;
   CFont *OldFont = ImageDC.SelectObject( &CallFont);

   if( !CallCode)
      Length = 0;

   else
   {
      sWork[0] = LOBYTE(CallCode);
     sWork[1] = HIBYTE(CallCode);
      sWork[2] = sWork[3] = 0;
      Length = 1;
   }
/*
   else if( !HIBYTE(CallCode)){
//    SBCS
      sWork[0] = LOBYTE(CallCode);
      sWork[1] = (BYTE)'\0';
      Length = 1;
   }else{
//    DBCS
      sWork[0] = HIBYTE(CallCode);
      sWork[1] = LOBYTE(CallCode);
      sWork[2] = (BYTE)'\0';
      Length = 2;
   }
*/
   if( Length){
      CRect TextImage;
/*
      GetTextExtentPoint32A( ImageDC.GetSafeHdc(), (LPCSTR)sWork,
            Length, &CharSize);
*/
      GetTextExtentPoint32W( ImageDC.GetSafeHdc(), (LPCWSTR)sWork,
            Length, &CharSize);

      TextImage.SetRect( 0, 0, BITMAP_WIDTH, BITMAP_HEIGHT);
      if( CharSize.cx < BITMAP_WIDTH)
         xOffset = ( BITMAP_HEIGHT - CharSize.cx) /2;
      else  xOffset = 0;

      if( CharSize.cy < BITMAP_HEIGHT)
         yOffset = ( BITMAP_WIDTH  - CharSize.cy) /2;
      else  yOffset = 0;

      if( EditLogFont.lfFaceName[0] == '@' &&
          Length == 1)
         xOffset = yOffset = 0;
/*    
         ExtTextOutA(ImageDC.GetSafeHdc(), xOffset, yOffset, ETO_OPAQUE,
         &TextImage, (LPCSTR)sWork, Length, NULL);
*/
         ExtTextOutW(ImageDC.GetSafeHdc(), xOffset, yOffset, ETO_OPAQUE,
         &TextImage, (LPCWSTR)sWork, Length, NULL);

   }
   ImageDC.SelectObject( OldFont);
   CallFont.DeleteObject();
   this->Invalidate( FALSE);
   this->UpdateWindow();
}

/****************************************/
/*             */
/* COMMAND     "Flip/Rotate"  */
/*             */
/****************************************/
void
CEditWnd::FlipRotate(
int   RadioItem)
{
   CDC   RotateDC;
   CBitmap RotateBMP;
   CBrush   wBrush;
   int   wSize;
   LPBYTE   pBuf1, pBuf2;
   LPBYTE   pPtr1, pPtr2;

   UndoImageDraw();
   BitmapDirty = TRUE;
   InitFlipRotate( &RotateDC, &RotateBMP);

   switch( RadioItem){
   case FLIP_HOR:
      if( !RectClipFlag){
         ImageDC.StretchBlt( BITMAP_WIDTH - 1, 0,
            0 - BITMAP_WIDTH, BITMAP_HEIGHT, &CRTDrawDC,
            0, 0, BITMAP_WIDTH, BITMAP_HEIGHT, SRCCOPY);
      }else{
         wBrush.CreateStockObject( WHITE_BRUSH);
         CRTDrawDC.FillRect( &ClipRect[BMP], &wBrush);
         wBrush.DeleteObject();

         CRTDrawDC.StretchBlt(
            ClipRect[BMP].right - 1, ClipRect[BMP].top,
            0 - ClipRect[BMP].Width(),
            ClipRect[BMP].Height(), &RotateDC,
            ClipRect[BMP].left, ClipRect[BMP].top,
            ClipRect[BMP].Width(),
            ClipRect[BMP].Height(), SRCCOPY);
      }
      break;

   case FLIP_VER:
      if( !RectClipFlag){
         ImageDC.StretchBlt( 0, BITMAP_HEIGHT - 1, BITMAP_WIDTH,
            0 - BITMAP_HEIGHT, &CRTDrawDC, 0, 0,
            BITMAP_WIDTH, BITMAP_HEIGHT, SRCCOPY);
      }else{
         wBrush.CreateStockObject( WHITE_BRUSH);
         CRTDrawDC.FillRect( &ClipRect[BMP], &wBrush);
         wBrush.DeleteObject();

         CRTDrawDC.StretchBlt(
            ClipRect[BMP].left,ClipRect[BMP].bottom - 1,
            ClipRect[BMP].Width(),
            0 - ClipRect[BMP].Height(), &RotateDC,
            ClipRect[BMP].left, ClipRect[BMP].top,
            ClipRect[BMP].Width(),
            ClipRect[BMP].Height(), SRCCOPY);
      }
      break;

   case ROTATE_9:
      wSize = (int)((( BITMAP_WIDTH +15) /16) *2)*(int)BITMAP_HEIGHT;
      pBuf1 = (LPBYTE)malloc( wSize);
      pBuf2 = (LPBYTE)malloc( wSize);
      if( !RectClipFlag){
         ImageBmp.GetBitmapBits( wSize, (LPVOID)pBuf1);
         CRTDrawBmp.GetBitmapBits( wSize, (LPVOID)pBuf2);
         pPtr1 = pBuf1;
         pPtr2 = pBuf2;
         RotateFigure90(pPtr1,pPtr2,BITMAP_WIDTH, BITMAP_HEIGHT);
         ImageBmp.SetBitmapBits( wSize, (LPVOID)pBuf1);
         CRTDrawBmp.SetBitmapBits( wSize, (LPVOID)pBuf2);

         free(pBuf1);
         free(pBuf2);
      }else{
         CPoint   Mid;  
         CRTDrawBmp.GetBitmapBits( wSize, (LPVOID)pBuf1);
         RotateBMP.GetBitmapBits(  wSize, (LPVOID)pBuf2);
         pPtr1 = pBuf1;
         pPtr2 = pBuf2;
         RotateFigure90(pPtr1,pPtr2,BITMAP_WIDTH, BITMAP_HEIGHT);

         int Lft = ClipRect[BMP].left;
         int Btm = ClipRect[BMP].bottom;
         int Wid = ClipRect[BMP].Width();
         int Hgt = ClipRect[BMP].Height();
         ClipRect[BMP].left = BITMAP_HEIGHT - Btm;
         ClipRect[BMP].top  = Lft;
         ClipRect[BMP].right = ClipRect[BMP].left + Hgt;
         ClipRect[BMP].bottom = ClipRect[BMP].top + Wid;

         Lft = ClipRect[DSP].left;
         Btm = ClipRect[DSP].bottom - CAPTION_HEIGHT - 1;
         Wid = ClipRect[DSP].Width();
         Hgt = ClipRect[DSP].Height();
         Mid.x = Lft
         +((ClipRect[DSP].Width()/2)/ZoomRate)*ZoomRate;
         Mid.y = ClipRect[DSP].top - CAPTION_HEIGHT
         +((ClipRect[DSP].Height()/2)/ZoomRate)*ZoomRate;

         Mid.x -= (((ClipRect[DSP].Height()/2)/ZoomRate)*ZoomRate);
         Mid.y -= (((ClipRect[DSP].Width()/2)/ZoomRate)*ZoomRate);
         if( Mid.x < 0)    ClipRect[DSP].left = 0;
         else{
            if( ClipRect[DSP].left + Hgt <=
               EudcWndRect.right){
               ClipRect[DSP].left = Mid.x;
            }else{
               ClipRect[DSP].left =
                  Mid.x - ((Mid.x + Hgt - EudcWndRect.right)/ZoomRate)*ZoomRate;
            }
         }
         if( Mid.y < 0)    ClipRect[DSP].top = CAPTION_HEIGHT;
         else{
            if( ClipRect[DSP].top + Wid   <= EudcWndRect.bottom){
               ClipRect[DSP].top =
                  CAPTION_HEIGHT + Mid.y;
            }else{
               ClipRect[DSP].top =
                  CAPTION_HEIGHT + Mid.y - (( CAPTION_HEIGHT + Mid.y + Wid - EudcWndRect.bottom)/ZoomRate)*ZoomRate;
            }
         }
         ClipRect[DSP].right = ClipRect[DSP].left + Hgt;
         ClipRect[DSP].bottom = ClipRect[DSP].top + Wid;
         CRTDrawBmp.SetBitmapBits( wSize, (LPVOID)pBuf1);
         RotateBMP.SetBitmapBits(  wSize, (LPVOID)pBuf2);
         free(pBuf1);
         free(pBuf2);
      }
      break;

   case ROTATE_18:
      if( !RectClipFlag){
         ImageDC.StretchBlt( BITMAP_WIDTH -1, BITMAP_HEIGHT-1,
            0-BITMAP_HEIGHT, 0-BITMAP_WIDTH, &CRTDrawDC,
            0, 0, BITMAP_WIDTH, BITMAP_HEIGHT, SRCCOPY);
      }else{
         wBrush.CreateStockObject( WHITE_BRUSH);
         CRTDrawDC.FillRect( &ClipRect[BMP], &wBrush);
         wBrush.DeleteObject();

         CRTDrawDC.StretchBlt(
            ClipRect[BMP].right-1, ClipRect[BMP].bottom-1,
            0 - ClipRect[BMP].Width(),
            0 - ClipRect[BMP].Height(), &RotateDC,
            ClipRect[BMP].left, ClipRect[BMP].top,
            ClipRect[BMP].Width(),
            ClipRect[BMP].Height(), SRCCOPY);
      }
      break;

   case ROTATE_27:
      wSize = (int)((( BITMAP_WIDTH +15) /16) *2) *(int)BITMAP_HEIGHT;
      pBuf1 = (LPBYTE)malloc( wSize);
      pBuf2 = (LPBYTE)malloc( wSize);
      if( !RectClipFlag){
         ImageBmp.GetBitmapBits( wSize, (LPVOID)pBuf1);
         CRTDrawBmp.GetBitmapBits( wSize, (LPVOID)pBuf2);
         pPtr1 = pBuf1;
         pPtr2 = pBuf2;
         RotateFigure270( pPtr1, pPtr2, BITMAP_WIDTH,
               BITMAP_HEIGHT);
         ImageBmp.SetBitmapBits( wSize, (LPVOID)pBuf1);
         CRTDrawBmp.SetBitmapBits( wSize, (LPVOID)pBuf2);

         free(pBuf1);
         free(pBuf2);
      }else{
         CPoint   Mid;
         CRTDrawBmp.GetBitmapBits( wSize, (LPVOID)pBuf1);
         RotateBMP.GetBitmapBits(  wSize, (LPVOID)pBuf2);
         pPtr1 = pBuf1;
         pPtr2 = pBuf2;
         RotateFigure270( pPtr1, pPtr2,
            BITMAP_WIDTH, BITMAP_HEIGHT);

         int Rgt = ClipRect[BMP].right;
         int Top = ClipRect[BMP].top;
         int Wid = ClipRect[BMP].Width();
         int Hgt = ClipRect[BMP].Height();
         ClipRect[BMP].left = Top;
         ClipRect[BMP].top  = BITMAP_WIDTH - Rgt;
         ClipRect[BMP].right = ClipRect[BMP].left + Hgt;
         ClipRect[BMP].bottom = ClipRect[BMP].top + Wid;

         Rgt = ClipRect[DSP].right - 1;
         Top = ClipRect[DSP].top - CAPTION_HEIGHT;
         Wid = ClipRect[DSP].Width();
         Hgt = ClipRect[DSP].Height();
         Mid.x = ClipRect[DSP].left
         + ((ClipRect[DSP].Width()/2)/ZoomRate)*ZoomRate;
         Mid.y = Top
         + ((ClipRect[DSP].Height()/2)/ZoomRate)*ZoomRate;
         Mid.x -= (((ClipRect[DSP].Height()/2)/ZoomRate)*ZoomRate);
         Mid.y -= (((ClipRect[DSP].Width()/2)/ZoomRate)*ZoomRate);
         if( Mid.x < 0)    ClipRect[DSP].left = 0;
         else{
            if( ClipRect[DSP].left + Hgt
               <= EudcWndRect.right){
               ClipRect[DSP].left = Mid.x;
            }else{
               ClipRect[DSP].left =
                  Mid.x - ((Mid.x + Hgt - EudcWndRect.right)/ZoomRate)*ZoomRate;
            }
         }
         if( Mid.y < 0)
            ClipRect[DSP].top = CAPTION_HEIGHT;
         else{
            if( ClipRect[DSP].top + Wid   <= EudcWndRect.bottom){
               ClipRect[DSP].top =
                  CAPTION_HEIGHT + Mid.y;
            }else{
               ClipRect[DSP].top =
                  CAPTION_HEIGHT + Mid.y - (( CAPTION_HEIGHT + Mid.y + Wid - EudcWndRect.bottom)/ZoomRate)*ZoomRate;
            }
         }
         ClipRect[DSP].right = ClipRect[DSP].left + Hgt;
         ClipRect[DSP].bottom = ClipRect[DSP].top + Wid;
            
         CRTDrawBmp.SetBitmapBits( wSize, (LPVOID)pBuf1);
         RotateBMP.SetBitmapBits(  wSize, (LPVOID)pBuf2);
         free(pBuf1);
         free(pBuf2);
      }
      break;

   default:
      break;
   }
   if( RectClipFlag){
      if( RotateBMP.Detach() != NULL)
         RotateBMP.DeleteObject();
      if( RotateDC.Detach() != NULL)
         RotateDC.DeleteDC();
   }
   this->Invalidate(FALSE);
   this->UpdateWindow();
}

/************************************************/
/*                */
/* Initialize before flip or rotate */
/*                */
/************************************************/
void
CEditWnd::InitFlipRotate(
CDC*  RotateDC,
CBitmap *RotateBMP)
{
   WORD  BitSize;
   HANDLE   BitInit;
   BYTE  *BitmapPtr;

   if( RectClipFlag){
      CClientDC   dc( this);
      if( !RotateDC->CreateCompatibleDC( &dc))
         return;

      BitSize = (WORD)((( BITMAP_WIDTH +15) /16) *2)
         * (WORD)BITMAP_HEIGHT;
      if(( BitInit = LocalAlloc( LMEM_MOVEABLE, BitSize))==0)
         return;

      if(( BitmapPtr = (BYTE *)LocalLock( BitInit)) == NULL){
         LocalFree( BitInit);
         return;
      }
      memset( BitmapPtr, 0xffff, BitSize);
      if( !RotateBMP->CreateBitmap(BITMAP_WIDTH,BITMAP_HEIGHT,
         1, 1, (LPSTR)BitmapPtr)){
         LocalUnlock( BitInit);
         LocalFree( BitInit);
         return;
      }
      LocalUnlock( BitInit);
      LocalFree( BitInit);    
      RotateDC->SelectObject( RotateBMP);

      RotateDC->BitBlt( 0, 0, BITMAP_WIDTH, BITMAP_HEIGHT,
         &CRTDrawDC, 0, 0, SRCCOPY);
   }else{
      CRTDrawDC.BitBlt( 0, 0, BITMAP_WIDTH, BITMAP_HEIGHT,
         &ImageDC, 0, 0, SRCCOPY);
   }
}

#define     set_p(a,i)  ((a)[(i)>>3] & (0x80>>((i)&7)))
/****************************************/
/*             */
/* Rotate 90         */
/*             */
/****************************************/
void
CEditWnd::RotateFigure90(
LPBYTE   pBuf1,
LPBYTE   pBuf2,
int   bWid,
int   bHgt)
{
   int   i, j, k, l;
   int   wSize, aSize;
   BYTE  Pt, *test;

   aSize = (int)(((( bWid + 15) /16) *2) *bHgt);
   wSize = (int)((( bWid  + 15) /16) *2);
   test = pBuf2;
   for( i = 0; i < wSize; i++){
      pBuf2 = test + aSize - wSize + i;
      for( j = 0; j < 8; j++){
         for( k = wSize-1; k >= 0; k--){
            memset( &Pt, 0x00, sizeof(BYTE));
            for( l = 0; l < 8; l++){
               if( set_p(pBuf2, j))
                  Pt |= (1<<(7-l));
               pBuf2 -= wSize;
            }
            *pBuf1++ = Pt;
         }
         pBuf2 = test + aSize - wSize + i;
      }
   }
   return;
}

/****************************************/
/*             */
/* Rotate 270        */
/*             */
/****************************************/
void
CEditWnd::RotateFigure270(
LPBYTE   pBuf1,
LPBYTE   pBuf2,
int   bWid,
int   bHgt)
{
   int   i, j, k, l;
   int   wSize, aSize;
   BYTE  Pt, *test;

   aSize = (int)(((( bWid + 15) /16) *2) *bHgt);
   wSize = (int)((( bWid  + 15) /16) *2);
   test = pBuf2;
   for( i = 0; i < wSize; i++){
      pBuf2 = test + wSize - 1 - i;
      for( j = 7; j >= 0; j--){
         for( k = wSize-1; k >= 0; k--){
            memset( &Pt, 0x00, sizeof(BYTE));
            for( l = 0; l < 8; l++){
               if( set_p(pBuf2, j))
                  Pt |= (1<<(7-l));
               pBuf2 += wSize;
            }
            *pBuf1++ = Pt;
         }
         pBuf2 = test + wSize - 1 - i;
      }
   }
   return;
}

/****************************************/
/*             */
/* Set Duplicate rectangle    */
/*             */
/****************************************/
void
CEditWnd::SetDuplicateRect(
RECT  *rect,
POINT    *point)
{
   LPBYTE pDupBmp;
   int   wSize;

   UndoImageDraw();
   BitmapDirty = TRUE;
   WriteSelRectBitmap();
   this->Invalidate( FALSE);
   this->UpdateWindow();

   wSize = ((( BITMAP_WIDTH +15) /16) *2) *BITMAP_HEIGHT;
   pDupBmp = (LPBYTE)malloc( wSize);
   DupBmp.GetBitmapBits( wSize, (LPVOID)pDupBmp);
   CRTDrawBmp.SetBitmapBits( wSize, (LPVOID)pDupBmp);
   free( pDupBmp);

   ClipRect[DSP].CopyRect( rect);
   ClipRect[PRV].CopyRect( rect);
   SetValidRect();
   SetClickRect();
   ClipRect[BMP].SetRect( DupRect.left /ZoomRate,
      ( DupRect.top - CAPTION_HEIGHT) /ZoomRate,
        DupRect.right /ZoomRate,
      ( DupRect.bottom - CAPTION_HEIGHT) /ZoomRate);
   RectClipFlag = TRUE;

   this->InvalidateRect( &ClipRect[VLD], FALSE);
   this->UpdateWindow();
}

void CEditWnd::OnClose() 
{
   // Don't allow user to use hot key to close this window
   
   //CMDIChildWnd::OnClose();
}
