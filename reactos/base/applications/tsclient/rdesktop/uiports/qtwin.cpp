/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - QT Window System
   Copyright (C) Jay Sorg 2004-2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "rdesktop.h"

#include <qapplication.h>
#include <qmainwindow.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qbrush.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qsocketnotifier.h>
#include <qscrollview.h>
#include <qfile.h>

#include "qtwin.h"

#include <unistd.h> // gethostname
#include <pwd.h> // getpwuid
#include <stdarg.h> // va_list va_start va_end

#include <errno.h>
#include <fcntl.h>

/* rdesktop globals */
extern int g_tcp_port_rdp;
int g_use_rdp5 = 1;
char g_hostname[16];
char g_username[64];
int g_height = 600;
int g_width = 800;
int g_server_depth = 8;
int g_encryption = 1;
int g_desktop_save = 1;
int g_polygon_ellipse_orders = 0;
int g_bitmap_cache = 1;
int g_bitmap_cache_persist_enable = False;
int g_bitmap_cache_precache = True;
int g_bitmap_compression = 1;
int g_rdp5_performanceflags = 0;
int g_console_session = 0;
int g_keylayout = 0x409; /* Defaults to US keyboard layout */
int g_keyboard_type = 0x4; /* Defaults to US keyboard layout */
int g_keyboard_subtype = 0x0; /* Defaults to US keyboard layout */
int g_keyboard_functionkeys = 0xc; /* Defaults to US keyboard layout */

/* hack globals */
static int g_argc = 0;
static char ** g_argv = 0;
static int g_UpAndRunning = 0;
static int g_sock = 0;
static int g_deactivated = 0;
static uint32 g_ext_disc_reason = 0;
static char g_servername[128];
static char g_title[128] = "";
static int g_flags = RDP_LOGON_NORMAL;

#ifdef WITH_RDPSND
extern int g_dsp_busy;
extern int g_dsp_fd;
static int g_rdpsnd = 0;
static QSocketNotifier * g_SoundNotifier = 0;
#endif

/* qt globals */
static QSocketNotifier * g_SocketNotifier = 0;
static QApplication * g_App = 0;
static QMyMainWindow * g_MW = 0;
static QMyScrollView * g_SV = 0;
static QPixmap * g_BS = 0;
static QPixmap * g_DS = 0;
static QPainter * g_P1 = 0;
static QPainter * g_P2 = 0;
static QColor g_Color1;
static QColor g_Color2;

struct QColorMap
{
  uint32 RGBColors[256];
  int NumColors;
};
static struct QColorMap * g_CM = 0;
static QRegion * g_ClipRect;

static Qt::RasterOp g_OpCodes[16] = {
    Qt::ClearROP,        // BLACKNESS     0
    Qt::NorROP,          // NOTSRCERASE   DSon
    Qt::NotAndROP,       //               DSna
    Qt::NotCopyROP,      // NOTSRCCOPY    Sn
    Qt::AndNotROP,       // SRCERASE      SDna
    Qt::NotROP,          // DSTINVERT     Dn
    Qt::XorROP,          // SRCINVERT     DSx
    Qt::NandROP,         //               DSan
    Qt::AndROP,          // SRCAND        DSa
    Qt::NotXorROP,       //               DSxn
    Qt::NopROP,          //               D
    Qt::NotOrROP,        // MERGEPAINT    DSno
    Qt::CopyROP,         // SRCCOPY       S
    Qt::OrNotROP,        //               SDno
    Qt::OrROP,           // SRCPAINT      DSo
    Qt::SetROP};         // WHITENESS     1

/* Session Directory redirection */
BOOL g_redirect = False;
char g_redirect_server[64];
char g_redirect_domain[16];
char g_redirect_password[64];
char g_redirect_username[64];
char g_redirect_cookie[128];
uint32 g_redirect_flags = 0;

//*****************************************************************************
uint32 Color15to32(uint32 InColor)
{
  uint32 r, g, b;

  r = (InColor & 0x7c00) >> 10;
  r = (r * 0xff) / 0x1f;
  g = (InColor & 0x03e0) >> 5;
  g = (g * 0xff) / 0x1f;
  b = (InColor & 0x001f);
  b = (b * 0xff) / 0x1f;
  return (r << 16) | (g << 8) | b;
}

//*****************************************************************************
uint32 Color16to32(uint32 InColor)
{
  uint32 r, g, b;

  r = (InColor & 0xf800) >> 11;
  r = (r * 0xff) / 0x1f;
  g = (InColor & 0x07e0) >> 5;
  g = (g * 0xff) / 0x3f;
  b = (InColor & 0x001f);
  b = (b * 0xff) / 0x1f;
  return (r << 16) | (g << 8) | b;
}

//*****************************************************************************
uint32 Color24to32(uint32 InColor)
{
  return ((InColor & 0x00ff0000) >> 16) |
         ((InColor & 0x000000ff) << 16) |
          (InColor & 0x0000ff00);
}

//*****************************************************************************
void SetColorx(QColor * Color, uint32 InColor)
{
  switch (g_server_depth)
  {
    case 8:
      if (g_CM == NULL || InColor > 255)
      {
        Color->setRgb(0);
        return;
      }
      Color->setRgb(g_CM->RGBColors[InColor]);
      break;
    case 15:
      Color->setRgb(Color15to32(InColor));
      break;
    case 16:
      Color->setRgb(Color16to32(InColor));
      break;
    case 24:
      Color->setRgb(Color24to32(InColor));
      break;
    default:
      Color->setRgb(0);
  }
}

//*****************************************************************************
void SetOpCode(int opcode)
{
  if (opcode >= 0 && opcode < 16)
  {
    Qt::RasterOp op = g_OpCodes[opcode];
    if (op != Qt::CopyROP)
    {
      g_P1->setRasterOp(op);
      g_P2->setRasterOp(op);
    }
  }
}

//*****************************************************************************
void ResetOpCode(int opcode)
{
  if (opcode >= 0 && opcode < 16)
  {
    Qt::RasterOp op = g_OpCodes[opcode];
    if (op != Qt::CopyROP)
    {
      g_P1->setRasterOp(Qt::CopyROP);
      g_P2->setRasterOp(Qt::CopyROP);
    }
  }
}

/*****************************************************************************/
QMyMainWindow::QMyMainWindow(): QWidget()
{
}

/*****************************************************************************/
QMyMainWindow::~QMyMainWindow()
{
}

//*****************************************************************************
void QMyMainWindow::mouseMoveEvent(QMouseEvent * e)
{
  if (!g_UpAndRunning)
  {
    return;
  }
  rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, e->x(), e->y());
}

//*****************************************************************************
void QMyMainWindow::mousePressEvent(QMouseEvent * e)
{
  if (!g_UpAndRunning)
  {
    return;
  }
  if (e->button() == LeftButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                   e->x(), e->y());
  }
  else if (e->button() == RightButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON2,
                   e->x(), e->y());
  }
  else if (e->button() == MidButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON3,
                   e->x(), e->y());
  }
}

//*****************************************************************************
void QMyMainWindow::mouseReleaseEvent(QMouseEvent * e)
{
  if (!g_UpAndRunning)
  {
    return;
  }
  if (e->button() == LeftButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1, e->x(), e->y());
  }
  else if (e->button() == RightButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2, e->x(), e->y());
  }
  else if (e->button() == MidButton)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3, e->x(), e->y());
  }
}

//*****************************************************************************
void QMyMainWindow::wheelEvent(QWheelEvent * e)
{
  if (!g_UpAndRunning)
  {
    return;
  }
  if (e->delta() > 0)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON4, e->x(), e->y());
  }
  else if (e->delta() < 0)
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON5, e->x(), e->y());
  }
}

//*****************************************************************************
int GetScanCode(QKeyEvent* e)
{
  int Key = e->key();
  int ScanCode = 0;
  Qt::ButtonState bs = e->state();
  if (!(bs & Qt::ShiftButton)) // shift is not down
  {
    if (Key == 42) // *
      return 0x37;
    if (Key == 43) // +
      return 0x4e;
  }
  switch (Key)
  {
    case 4100: ScanCode = 0x1c; break; // enter
    case 4101: ScanCode = 0x1c; break;
    case 4117: ScanCode = 0xd0; break; // down arrow
    case 4115: ScanCode = 0xc8; break; // up arrow
    case 4114: ScanCode = 0xcb; break; // left arrow
    case 4116: ScanCode = 0xcd; break; // right arrow
    case 4112: ScanCode = 0xc7; break; // home
    case 4113: ScanCode = 0xcf; break; // end
    case 4102: ScanCode = 0xd2; break; // insert
    case 4103: ScanCode = 0xd3; break; // delete
    case 4118: ScanCode = 0xc9; break; // page up
    case 4119: ScanCode = 0xd1; break; // page down
    case 4143: ScanCode = 0x00; break; // num lock
    case 47:   ScanCode = 0x35; break; // /
    case 42:   ScanCode = 0x37; break; // *
    case 45:   ScanCode = 0x0c; break; // -
    case 95:   ScanCode = 0x0c; break; // _
    case 43:   ScanCode = 0x0d; break; // +
    case 46:   ScanCode = 0x34; break; // .
    case 48:   ScanCode = 0x0b; break; // 0
    case 41:   ScanCode = 0x0b; break; // )
    case 49:   ScanCode = 0x02; break; // 1
    case 33:   ScanCode = 0x02; break; // !
    case 50:   ScanCode = 0x03; break; // 2
    case 64:   ScanCode = 0x03; break; // @
    case 51:   ScanCode = 0x04; break; // 3
    case 35:   ScanCode = 0x04; break; // #
    case 52:   ScanCode = 0x05; break; // 4
    case 36:   ScanCode = 0x05; break; // $
    case 53:   ScanCode = 0x06; break; // 5
    case 37:   ScanCode = 0x06; break; // %
    case 54:   ScanCode = 0x07; break; // 6
    case 94:   ScanCode = 0x07; break; // ^
    case 55:   ScanCode = 0x08; break; // 7
    case 38:   ScanCode = 0x08; break; // &
    case 56:   ScanCode = 0x09; break; // 8
    case 57:   ScanCode = 0x0a; break; // 9
    case 40:   ScanCode = 0x0a; break; // (
    case 61:   ScanCode = 0x0d; break; // =
    case 65:   ScanCode = 0x1e; break; // a
    case 66:   ScanCode = 0x30; break; // b
    case 67:   ScanCode = 0x2e; break; // c
    case 68:   ScanCode = 0x20; break; // d
    case 69:   ScanCode = 0x12; break; // e
    case 70:   ScanCode = 0x21; break; // f
    case 71:   ScanCode = 0x22; break; // g
    case 72:   ScanCode = 0x23; break; // h
    case 73:   ScanCode = 0x17; break; // i
    case 74:   ScanCode = 0x24; break; // j
    case 75:   ScanCode = 0x25; break; // k
    case 76:   ScanCode = 0x26; break; // l
    case 77:   ScanCode = 0x32; break; // m
    case 78:   ScanCode = 0x31; break; // n
    case 79:   ScanCode = 0x18; break; // o
    case 80:   ScanCode = 0x19; break; // p
    case 81:   ScanCode = 0x10; break; // q
    case 82:   ScanCode = 0x13; break; // r
    case 83:   ScanCode = 0x1f; break; // s
    case 84:   ScanCode = 0x14; break; // t
    case 85:   ScanCode = 0x16; break; // u
    case 86:   ScanCode = 0x2f; break; // v
    case 87:   ScanCode = 0x11; break; // w
    case 88:   ScanCode = 0x2d; break; // x
    case 89:   ScanCode = 0x15; break; // y
    case 90:   ScanCode = 0x2c; break; // z
    case 32:   ScanCode = 0x39; break; // space
    case 44:   ScanCode = 0x33; break; // ,
    case 60:   ScanCode = 0x33; break; // <
    case 62:   ScanCode = 0x34; break; // >
    case 63:   ScanCode = 0x35; break; // ?
    case 92:   ScanCode = 0x2b; break; // backslash
    case 124:  ScanCode = 0x2b; break; // bar
    case 4097: ScanCode = 0x0f; break; // tab
    case 4132: ScanCode = 0x3a; break; // caps lock
    case 4096: ScanCode = 0x01; break; // esc
    case 59:   ScanCode = 0x27; break; // ;
    case 58:   ScanCode = 0x27; break; // :
    case 39:   ScanCode = 0x28; break; // '
    case 34:   ScanCode = 0x28; break; // "
    case 91:   ScanCode = 0x1a; break; // [
    case 123:  ScanCode = 0x1a; break; // {
    case 93:   ScanCode = 0x1b; break; // ]
    case 125:  ScanCode = 0x1b; break; // }
    case 4144: ScanCode = 0x3b; break; // f1
    case 4145: ScanCode = 0x3c; break; // f2
    case 4146: ScanCode = 0x3d; break; // f3
    case 4147: ScanCode = 0x3e; break; // f4
    case 4148: ScanCode = 0x3f; break; // f5
    case 4149: ScanCode = 0x40; break; // f6
    case 4150: ScanCode = 0x41; break; // f7
    case 4151: ScanCode = 0x42; break; // f8
    case 4152: ScanCode = 0x43; break; // f9
    case 4153: ScanCode = 0x44; break; // f10
    case 4154: ScanCode = 0x57; break; // f11
    case 4155: ScanCode = 0x58; break; // f12
    case 4128: ScanCode = 0x2a; break; // shift
    case 4131: ScanCode = 0x38; break; // alt
    case 4129: ScanCode = 0x1d; break; // ctrl
    case 96:   ScanCode = 0x29; break; // `
    case 126:  ScanCode = 0x29; break; // ~
    case 4099: ScanCode = 0x0e; break; // backspace
  }
//  if (ScanCode == 0)
//    printf("key %d scancode %d\n", Key, ScanCode);
  return ScanCode;
}

//*****************************************************************************
void QMyMainWindow::keyPressEvent(QKeyEvent* e)
{
  if (!g_UpAndRunning)
    return;
  int ScanCode = GetScanCode(e);
  if (ScanCode != 0)
  {
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, ScanCode, 0);
    e->accept();
  }
}

//*****************************************************************************
void QMyMainWindow::keyReleaseEvent(QKeyEvent* e)
{
  if (!g_UpAndRunning)
  {
    return;
  }
  int ScanCode = GetScanCode(e);
  if (ScanCode != 0)
  {
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, ScanCode, 0);
    e->accept();
  }
}

//*****************************************************************************
void QMyMainWindow::paintEvent(QPaintEvent * pe)
{
  QRect Rect;

  Rect = pe->rect();
  bitBlt(this, Rect.left(), Rect.top(), g_BS, Rect.left(), Rect.top(),
         Rect.width(), Rect.height());
}

//*****************************************************************************
void QMyMainWindow::closeEvent(QCloseEvent * e)
{
  e->accept();
}

//*****************************************************************************
bool QMyMainWindow::event(QEvent * e)
{
  return QWidget::event(e);
}

//*****************************************************************************
void QMyMainWindow::dataReceived()
{
  if (!rdp_loop(&g_deactivated, &g_ext_disc_reason))
  {
    g_SV->close();
  }
#ifdef WITH_RDPSND
  if (g_dsp_busy)
  {
    if (g_SoundNotifier == 0)
    {
      g_SoundNotifier = new QSocketNotifier(g_dsp_fd, QSocketNotifier::Write,
                                            g_MW);
      g_MW->connect(g_SoundNotifier, SIGNAL(activated(int)), g_MW,
                    SLOT(soundSend()));
    }
    else
    {
      if (!g_SoundNotifier->isEnabled())
      {
        g_SoundNotifier->setEnabled(true);
      }
    }
  }
#endif
}

/******************************************************************************/
void QMyMainWindow::soundSend()
{
#ifdef WITH_RDPSND
  g_SoundNotifier->setEnabled(false);
  wave_out_play();
  if (g_dsp_busy)
  {
    g_SoundNotifier->setEnabled(true);
  }
#endif
}

//*****************************************************************************
void QMyScrollView::keyPressEvent(QKeyEvent * e)
{
  g_MW->keyPressEvent(e);
}

//*****************************************************************************
void QMyScrollView::keyReleaseEvent(QKeyEvent * e)
{
  g_MW->keyReleaseEvent(e);
}


//*****************************************************************************
void ui_begin_update(void)
{
  g_P1->begin(g_MW);
  g_P2->begin(g_BS);
}

//*****************************************************************************
void ui_end_update(void)
{
  g_P1->end();
  g_P2->end();
}

/*****************************************************************************/
int ui_init(void)
{
  g_App = new QApplication(g_argc, g_argv);
  return 1;
}

/*****************************************************************************/
void ui_deinit(void)
{
  delete g_App;
}

/*****************************************************************************/
int ui_create_window(void)
{
  int w, h;
  QPainter * painter;
  QWidget * desktop;

  g_MW = new QMyMainWindow();
  g_SV = new QMyScrollView();
  g_SV->addChild(g_MW);
  g_BS = new QPixmap(g_width, g_height);
  painter = new QPainter(g_BS);
  painter->fillRect(0, 0, g_width, g_height, QBrush(QColor("white")));
  painter->fillRect(0, 0, g_width, g_height, QBrush(QBrush::CrossPattern));
  delete painter;
  g_DS = new QPixmap(480, 480);
  g_P1 = new QPainter();
  g_P2 = new QPainter();
  g_ClipRect = new QRegion(0, 0, g_width, g_height);
  desktop = QApplication::desktop();
  w = desktop->width();              // returns screen width
  h = desktop->height();             // returns screen height
  g_MW->resize(g_width, g_height);
  if (w < g_width || h < g_height)
  {
    g_SV->resize(w, h);
  }
  else
  {
    g_SV->resize(g_width + 4, g_height + 4);
  }
  g_SV->setMaximumWidth(g_width + 4);
  g_SV->setMaximumHeight(g_height + 4);
  g_App->setMainWidget(g_SV);
  g_SV->show();
  g_MW->setMouseTracking(true);
  if (g_title[0] != 0)
  {
    g_SV->setCaption(g_title);
  }

/*  XGrayKey(0, 64, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 113, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 37, AnyModifie, SV-winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 109, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 115, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 116, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 117, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 62, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);
  XGrayKey(0, 50, AnyModifie, SV->winId(), 0, GrabModeAsync, GrabModeAsync);*/

  return 1;
}

//*****************************************************************************
void ui_main_loop(void)
{
#ifdef WITH_RDPSND
  // init sound
  if (g_rdpsnd)
  {
    rdpsnd_init();
  }
#endif
  // connect
  if (!rdp_connect(g_servername, g_flags, "", "", "", ""))
  {
    return;
  }
  // start notifier
  g_SocketNotifier = new QSocketNotifier(g_sock, QSocketNotifier::Read, g_MW);
  g_MW->connect(g_SocketNotifier, SIGNAL(activated(int)),
                g_MW, SLOT(dataReceived()));
  g_UpAndRunning = 1;
  // app main loop
  g_App->exec();
}

//*****************************************************************************
void ui_destroy_window(void)
{
  delete g_MW;
  delete g_SV;
  delete g_BS;
  delete g_DS;
  delete g_P1;
  delete g_P2;
  delete g_ClipRect;
}

/*****************************************************************************/
void ui_bell(void)
{
}

/*****************************************************************************/
int ui_select(int in_val)
{
  if (g_sock == 0)
  {
    g_sock = in_val;
  }
  return 1;
}

/*****************************************************************************/
void ui_destroy_cursor(void * cursor)
{
  QCursor * Cursor;
  Cursor = (QCursor*)cursor;
  if (Cursor != NULL)
  {
    delete Cursor;
  }
}

/*****************************************************************************/
void* ui_create_glyph(int width, int height, uint8 * data)
{
  QBitmap * Bitmap;
  Bitmap = new QBitmap(width, height, data);
  Bitmap->setMask(*Bitmap);
  return (HGLYPH)Bitmap;
}

/*****************************************************************************/
void ui_destroy_glyph(void * glyph)
{
  QBitmap* Bitmap;
  Bitmap = (QBitmap*)glyph;
  delete Bitmap;
}

/*****************************************************************************/
void ui_destroy_bitmap(void * bmp)
{
  QPixmap * Pixmap;
  Pixmap = (QPixmap*)bmp;
  delete Pixmap;
}

/*****************************************************************************/
void ui_reset_clip(void)
{
  g_P1->setClipRect(0, 0, g_width, g_height);
  g_P2->setClipRect(0, 0, g_width, g_height);
  delete g_ClipRect;
  g_ClipRect = new QRegion(0, 0, g_width, g_height);
}

/*****************************************************************************/
void ui_set_clip(int x, int y, int cx, int cy)
{
  g_P1->setClipRect(x, y, cx, cy);
  g_P2->setClipRect(x, y, cx, cy);
  delete g_ClipRect;
  g_ClipRect = new QRegion(x, y, cx, cy);
}

/*****************************************************************************/
void * ui_create_colourmap(COLOURMAP * colours)
{
  QColorMap* LCM;
  int i, r, g, b;
  LCM = (QColorMap*)malloc(sizeof(QColorMap));
  memset(LCM, 0, sizeof(QColorMap));
  i = 0;
  while (i < colours->ncolours && i < 256)
  {
    r = colours->colours[i].red;
    g = colours->colours[i].green;
    b = colours->colours[i].blue;
    LCM->RGBColors[i] = (r << 16) | (g << 8) | b;
    i++;
  }
  LCM->NumColors = colours->ncolours;
  return LCM;
}

//*****************************************************************************
// todo, does this leak at end of program
void ui_destroy_colourmap(HCOLOURMAP map)
{
  QColorMap * LCM;
  LCM = (QColorMap*)map;
  if (LCM == NULL)
    return;
  free(LCM);
}

/*****************************************************************************/
void ui_set_colourmap(void * map)
{
  // destoy old colormap
  ui_destroy_colourmap(g_CM);
  g_CM = (QColorMap*)map;
}

/*****************************************************************************/
HBITMAP ui_create_bitmap(int width, int height, uint8 * data)
{
  QImage * Image = NULL;
  QPixmap * Pixmap;
  uint32 * d = NULL;
  uint16 * s;
  int i;

  switch (g_server_depth)
  {
    case 8:
      Image = new QImage(data, width, height, 8, (QRgb*)&g_CM->RGBColors,
                         g_CM->NumColors, QImage::IgnoreEndian);
      break;
    case 15:
      d = (uint32*)malloc(width * height * 4);
      s = (uint16*)data;
      for (i = 0; i < width * height; i++)
      {
        d[i] = Color15to32(s[i]);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
    case 16:
      d = (uint32*)malloc(width * height * 4);
      s = (uint16*)data;
      for (i = 0; i < width * height; i++)
      {
        d[i] = Color16to32(s[i]);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
    case 24:
      d = (uint32*)malloc(width * height * 4);
      memset(d, 0, width * height * 4);
      for (i = 0; i < width * height; i++)
      {
        memcpy(d + i, data + i * 3, 3);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
  }
  if (Image == NULL)
  {
    return NULL;
  }
  Pixmap = new QPixmap();
  Pixmap->convertFromImage(*Image);
  delete Image;
  if (d != NULL)
  {
    free(d);
  }
  return (HBITMAP)Pixmap;
}

//******************************************************************************
// adjust coordinates for cliping rect
int WarpCoords(int * x, int * y, int * cx, int * cy, int * srcx, int * srcy)
{
  int dx, dy;
  QRect InRect(*x, *y, *cx, *cy);
  QRect OutRect;
  QRect CRect = g_ClipRect->boundingRect();
  OutRect = InRect.intersect(CRect);
  if (OutRect.isEmpty())
  {
    return False;
  }
  dx = OutRect.x() - InRect.x();
  dy = OutRect.y() - InRect.y();
  *x = OutRect.x();
  *y = OutRect.y();
  *cx = OutRect.width();
  *cy = OutRect.height();
  *srcx = *srcx + dx;
  *srcy = *srcy + dy;
  return True;
}

//******************************************************************************
// needed because bitBlt don't seem to care about clipping rects
// also has 2 dsts and src can be null
void bitBltClip(QPaintDevice * dst1, QPaintDevice * dst2, int dx, int dy,
                QPaintDevice * src, int sx, int sy, int sw, int sh,
                Qt::RasterOp rop, bool im)
{
  if (WarpCoords(&dx, &dy, &sw, &sh, &sx, &sy))
  {
    if (dst1 != NULL)
    {
      if (src == NULL)
      {
        bitBlt(dst1, dx, dy, dst1, sx, sy, sw, sh, rop, im);
      }
      else
      {
        bitBlt(dst1, dx, dy, src, sx, sy, sw, sh, rop, im);
      }
    }
    if (dst2 != NULL)
    {
      if (src == NULL)
      {
        bitBlt(dst2, dx, dy, dst2, sx, sy, sw, sh, rop, im);
      }
      else
      {
        bitBlt(dst2, dx, dy, src, sx, sy, sw, sh, rop, im);
      }
    }
  }
}

#define DO_GLYPH(ttext,idx) \
{ \
  glyph = cache_get_font (font, ttext[idx]); \
  if (!(flags & TEXT2_IMPLICIT_X)) \
  { \
    xyoffset = ttext[++idx]; \
    if ((xyoffset & 0x80)) \
    { \
      if (flags & TEXT2_VERTICAL) \
        y += ttext[idx+1] | (ttext[idx+2] << 8); \
      else \
        x += ttext[idx+1] | (ttext[idx+2] << 8); \
      idx += 2; \
    } \
    else \
    { \
      if (flags & TEXT2_VERTICAL) \
        y += xyoffset; \
      else \
        x += xyoffset; \
    } \
  } \
  if (glyph != NULL) \
  { \
    g_P2->drawPixmap(x + glyph->offset, y + glyph->baseline, \
                     *((QBitmap*)glyph->pixmap)); \
    if (flags & TEXT2_IMPLICIT_X) \
      x += glyph->width; \
  } \
}

//*****************************************************************************
void ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode,
                  int x, int y, int clipx, int clipy,
                  int clipcx, int clipcy, int boxx,
                  int boxy, int boxcx, int boxcy, BRUSH * brush,
                  int bgcolour, int fgcolour, uint8 * text, uint8 length)
{
  FONTGLYPH * glyph;
  int i, j, xyoffset;
  DATABLOB * entry;

  SetColorx(&g_Color1, fgcolour);
  SetColorx(&g_Color2, bgcolour);
  g_P2->setBackgroundColor(g_Color2);
  g_P2->setPen(g_Color1);
  if (boxcx > 1)
  {
    g_P2->fillRect(boxx, boxy, boxcx, boxcy, QBrush(g_Color2));
  }
  else if (mixmode == MIX_OPAQUE)
  {
    g_P2->fillRect(clipx, clipy, clipcx, clipcy, QBrush(g_Color2));
  }

  /* Paint text, character by character */
  for (i = 0; i < length;)
  {
    switch (text[i])
    {
      case 0xff:
        if (i + 2 < length)
        {
          cache_put_text(text[i + 1], text, text[i + 2]);
        }
        else
        {
          error("this shouldn't be happening\n");
          exit(1);
        }
        /* this will move pointer from start to first character after FF
           command */
        length -= i + 3;
        text = &(text[i + 3]);
        i = 0;
        break;

      case 0xfe:
        entry = cache_get_text(text[i + 1]);
        if (entry != NULL)
        {
          if ((((uint8 *) (entry->data))[1] == 0) &&
                             (!(flags & TEXT2_IMPLICIT_X)))
          {
            if (flags & TEXT2_VERTICAL)
            {
              y += text[i + 2];
            }
            else
            {
              x += text[i + 2];
            }
          }
          for (j = 0; j < entry->size; j++)
          {
            DO_GLYPH(((uint8 *) (entry->data)), j);
          }
        }
        if (i + 2 < length)
        {
          i += 3;
        }
        else
        {
          i += 2;
        }
        length -= i;
        /* this will move pointer from start to first character after FE
           command */
        text = &(text[i]);
        i = 0;
        break;

      default:
        DO_GLYPH(text, i);
        i++;
        break;
    }
  }
  if (boxcx > 1)
  {
    bitBltClip(g_MW, NULL, boxx, boxy, g_BS, boxx, boxy, boxcx, boxcy,
               Qt::CopyROP, true);
  }
  else
  {
    bitBltClip(g_MW, NULL, clipx, clipy, g_BS, clipx, clipy, clipcx,
               clipcy, Qt::CopyROP, true);
  }
}

/*****************************************************************************/
void ui_line(uint8 opcode, int startx, int starty, int endx, int endy,
             PEN * pen)
{
  SetColorx(&g_Color1, pen->colour);
  SetOpCode(opcode);
  g_P1->setPen(g_Color1);
  g_P1->moveTo(startx, starty);
  g_P1->lineTo(endx, endy);
  g_P2->setPen(g_Color1);
  g_P2->moveTo(startx, starty);
  g_P2->lineTo(endx, endy);
  ResetOpCode(opcode);
}

/*****************************************************************************/
// not used
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy,
               BRUSH* brush, int bgcolour, int fgcolour)
{
}

/*****************************************************************************/
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy)
{
  QPixmap* Pixmap;
  Pixmap = (QPixmap*)src;
  if (Pixmap != NULL)
  {
    SetOpCode(opcode);
    g_P1->drawPixmap(x, y, *Pixmap, srcx, srcy, cx, cy);
    g_P2->drawPixmap(x, y, *Pixmap, srcx, srcy, cx, cy);
    ResetOpCode(opcode);
  }
}

//******************************************************************************
void CommonDeskSave(QPixmap* Pixmap1, QPixmap* Pixmap2, int Offset, int x,
                    int y, int cx, int cy, int dir)
{
  int lx;
  int ly;
  int x1;
  int y1;
  int width;
  int lcx;
  int right;
  int bottom;
  lx = Offset % 480;
  ly = Offset / 480;
  y1 = y;
  right = x + cx;
  bottom = y + cy;
  while (y1 < bottom)
  {
    x1 = x;
    lcx = cx;
    while (x1 < right)
    {
      width = 480 - lx;
      if (width > lcx)
        width = lcx;
      if (dir == 0)
        bitBlt(Pixmap1, lx, ly, Pixmap2, x1, y1, width, 1, Qt::CopyROP, true);
      else
        bitBlt(Pixmap2, x1, y1, Pixmap1, lx, ly, width, 1, Qt::CopyROP, true);
      lx = lx + width;
      if (lx >= 480)
      {
        lx = 0;
        ly++;
        if (ly >= 480)
          ly = 0;
      }
      lcx = lcx - width;
      x1 = x1 + width;
    }
    y1++;
  }
}

/*****************************************************************************/
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
  QPixmap * Pixmap;

  Pixmap = new QPixmap(cx, cy);
  CommonDeskSave(g_DS, Pixmap, offset, 0, 0, cx, cy, 1);
  bitBltClip(g_MW, g_BS, x, y, Pixmap, 0, 0, cx, cy, Qt::CopyROP, true);
  delete Pixmap;
}

/*****************************************************************************/
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
  CommonDeskSave(g_DS, g_BS, offset, x, y, cx, cy, 0);
}

/*****************************************************************************/
void ui_rect(int x, int y, int cx, int cy, int colour)
{
  SetColorx(&g_Color1, colour);
  g_P1->fillRect(x, y, cx, cy, QBrush(g_Color1));
  g_P2->fillRect(x, y, cx, cy, QBrush(g_Color1));
}

/*****************************************************************************/
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy,
                  int srcx, int srcy)
{
  SetOpCode(opcode);
  bitBltClip(g_MW, g_BS, x, y, NULL, srcx, srcy, cx, cy, Qt::CopyROP, true);
  ResetOpCode(opcode);
}

/*****************************************************************************/
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
               BRUSH* brush, int bgcolour, int fgcolour)
{
  QBitmap* Bitmap;
  QBrush* Brush;
  uint8 ipattern[8], i;
  SetOpCode(opcode);
  switch (brush->style)
  {
    case 0:
      SetColorx(&g_Color1, fgcolour);
      g_P2->fillRect(x, y, cx, cy, QBrush(g_Color1));
      break;
    case 3:
      SetColorx(&g_Color1, fgcolour);
      SetColorx(&g_Color2, bgcolour);
      for (i = 0; i != 8; i++)
      {
        ipattern[7 - i] = ~brush->pattern[i];
      }
      Bitmap = new QBitmap(8, 8, ipattern);
      Brush = new QBrush(g_Color1, *Bitmap);
      g_P2->setBackgroundMode(Qt::OpaqueMode);
      g_P2->setBrushOrigin(brush->xorigin, brush->yorigin);
      g_P2->setBackgroundColor(g_Color2);
      g_P2->fillRect(x, y, cx, cy, *Brush);
      delete Brush;
      delete Bitmap;
      g_P2->setBackgroundMode(Qt::TransparentMode);
      g_P2->setBrushOrigin(0, 0);
      break;
  }
  ResetOpCode(opcode);
  bitBltClip(g_MW, NULL, x, y, g_BS, x, y, cx, cy, Qt::CopyROP, true);
}

/*****************************************************************************/
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
  SetOpCode(opcode);
  g_P1->fillRect(x, y, cx, cy, QBrush(QColor("black")));
  g_P2->fillRect(x, y, cx, cy, QBrush(QColor("black")));
  ResetOpCode(opcode);
}

/*****************************************************************************/
void ui_move_pointer(int x, int y)
{
}

/*****************************************************************************/
void ui_set_null_cursor(void)
{
  g_MW->setCursor(10); // Qt::BlankCursor
}

/*****************************************************************************/
void ui_paint_bitmap(int x, int y, int cx, int cy,
                     int width, int height, uint8* data)
{
  QImage * Image = NULL;
  QPixmap * Pixmap;
  uint32 * d = NULL;
  uint16 * s;
  int i;

  switch (g_server_depth)
  {
    case 8:
      Image = new QImage(data, width, height, 8, (QRgb*)&g_CM->RGBColors,
                         g_CM->NumColors, QImage::IgnoreEndian);
      break;
    case 15:
      d = (uint32*)malloc(width * height * 4);
      s = (uint16*)data;
      for (i = 0; i < width * height; i++)
      {
        d[i] = Color15to32(s[i]);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
    case 16:
      d = (uint32*)malloc(width * height * 4);
      s = (uint16*)data;
      for (i = 0; i < width * height; i++)
      {
        d[i] = Color16to32(s[i]);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
    case 24:
      d = (uint32*)malloc(width * height * 4);
      memset(d, 0, width * height * 4);
      for (i = 0; i < width * height; i++)
      {
        memcpy(d + i, data + i * 3, 3);
      }
      Image = new QImage((uint8*)d, width, height, 32, NULL,
                         0, QImage::IgnoreEndian);
      break;
  }
  if (Image == NULL)
  {
    return;
  }
  Pixmap = new QPixmap();
  Pixmap->convertFromImage(*Image);
  g_P1->drawPixmap(x, y, *Pixmap, 0, 0, cx, cy);
  g_P2->drawPixmap(x, y, *Pixmap, 0, 0, cx, cy);
  delete Image;
  delete Pixmap;
  if (d != NULL)
  {
    free(d);
  }
}

//******************************************************************************
int Is24On(uint8* Data, int X, int Y)
{
  uint8 R, G, B;
  int Start;
  Start = Y * 32 * 3 + X * 3;
  R = Data[Start];
  G = Data[Start + 1];
  B = Data[Start + 2];
  return !((R == 0) && (G == 0) && (B == 0));
}

//******************************************************************************
int Is1On(uint8* Data, int X, int Y)
{
  int Start;
  int Shift;
  Start = (Y * 32) / 8 + X / 8;
  Shift = X % 8;
  return (Data[Start] & (0x80 >> Shift)) == 0;
}

//******************************************************************************
void Set1(uint8* Data, int X, int Y)
{
  int Start;
  int Shift;
  Start = (Y * 32) / 8 + X / 8;
  Shift = X % 8;
  Data[Start] = Data[Start] | (0x80 >> Shift);
}

//******************************************************************************
void FlipOver(uint8* Data)
{
  uint8 AData[128];
  int Index;
  memcpy(AData, Data, 128);
  for (Index = 0; Index <= 31; Index++)
  {
    Data[127 - (Index * 4 + 3)] = AData[Index * 4];
    Data[127 - (Index * 4 + 2)] = AData[Index * 4 + 1];
    Data[127 - (Index * 4 + 1)] = AData[Index * 4 + 2];
    Data[127 - Index * 4] = AData[Index * 4 + 3];
  }
}

/*****************************************************************************/
void ui_set_cursor(HCURSOR cursor)
{
  QCursor* Cursor;
  Cursor = (QCursor*)cursor;
  if (Cursor != NULL)
    g_MW->setCursor(*Cursor);
}

/*****************************************************************************/
HCURSOR ui_create_cursor(unsigned int x, unsigned int y,
                         int width, int height,
                         uint8* andmask, uint8* xormask)
{
  uint8 AData[128];
  uint8 AMask[128];
  QBitmap* DataBitmap;
  QBitmap* MaskBitmap;
  QCursor* Cursor;
  int I1, I2, BOn, MOn;

  if (width != 32 || height != 32)
  {
    return 0;
  }
  memset(AData, 0, 128);
  memset(AMask, 0, 128);
  for (I1 = 0; I1 <= 31; I1++)
  {
    for (I2 = 0; I2 <= 31; I2++)
    {
      MOn = Is24On(xormask, I1, I2);
      BOn = Is1On(andmask, I1, I2);
      if (BOn ^ MOn) // xor
      {
        Set1(AData, I1, I2);
        if (!MOn)
        {
          Set1(AMask, I1, I2);
        }
      }
      if (MOn)
      {
        Set1(AMask, I1, I2);
      }
    }
  }
  FlipOver(AData);
  FlipOver(AMask);
  DataBitmap = new QBitmap(32, 32, AData);
  MaskBitmap = new QBitmap(32, 32, AMask);
  Cursor = new QCursor(*DataBitmap, *MaskBitmap, x, y);
  delete DataBitmap;
  delete MaskBitmap;
  return Cursor;
}

/*****************************************************************************/
uint16 ui_get_numlock_state(uint32 state)
{
  return 0;
}

/*****************************************************************************/
uint32 read_keyboard_state(void)
{
  return 0;
}

/*****************************************************************************/
void ui_resize_window(void)
{
}

/*****************************************************************************/
void ui_polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints,
                BRUSH * brush, int bgcolour, int fgcolour)
{
}

/*****************************************************************************/
/* todo, use qt function for this (QPainter::drawPolyline) */
void ui_polyline(uint8 opcode, POINT * points, int npoints, PEN * pen)
{
  int i, x, y, dx, dy;

  if (npoints > 0)
  {
    x = points[0].x;
    y = points[0].y;
    for (i = 1; i < npoints; i++)
    {
      dx = points[i].x;
      dy = points[i].y;
      ui_line(opcode, x, y, x + dx, y + dy, pen);
      x = x + dx;
      y = y + dy;
    }
  }
}

/*****************************************************************************/
void ui_ellipse(uint8 opcode, uint8 fillmode,
                int x, int y, int cx, int cy,
                BRUSH * brush, int bgcolour, int fgcolour)
{
}

/*****************************************************************************/
void generate_random(uint8 * random)
{
  QFile File("/dev/random");
  File.open(IO_ReadOnly);
  if (File.readBlock((char*)random, 32) == 32)
  {
    return;
  }
  warning("no /dev/random\n");
  memcpy(random, "12345678901234567890123456789012", 32);
}

/*****************************************************************************/
void save_licence(uint8 * data, int length)
{
  char * home, * path, * tmppath;
  int fd;

  home = getenv("HOME");
  if (home == NULL)
  {
    return;
  }
  path = (char *) xmalloc(strlen(home) + strlen(g_hostname) +
                          sizeof("/.rdesktop/licence."));
  sprintf(path, "%s/.rdesktop", home);
  if ((mkdir(path, 0700) == -1) && errno != EEXIST)
  {
    perror(path);
    return;
  }
  /* write licence to licence.hostname.new, then atomically rename to
     licence.hostname */
  sprintf(path, "%s/.rdesktop/licence.%s", home, g_hostname);
  tmppath = (char *) xmalloc(strlen(path) + sizeof(".new"));
  strcpy(tmppath, path);
  strcat(tmppath, ".new");
  fd = open(tmppath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd == -1)
  {
    perror(tmppath);
    return;
  }
  if (write(fd, data, length) != length)
  {
    perror(tmppath);
    unlink(tmppath);
  }
  else if (rename(tmppath, path) == -1)
  {
    perror(path);
    unlink(tmppath);
  }
  close(fd);
  xfree(tmppath);
  xfree(path);
}

/*****************************************************************************/
int load_licence(uint8 ** data)
{
  char * home, * path;
  struct stat st;
  int fd, length;

  home = getenv("HOME");
  if (home == NULL)
  {
    return -1;
  }
  path = (char *) xmalloc(strlen(home) + strlen(g_hostname) +
                          sizeof("/.rdesktop/licence."));
  sprintf(path, "%s/.rdesktop/licence.%s", home, g_hostname);
  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    return -1;
  }
  if (fstat(fd, &st))
  {
    return -1;
  }
  *data = (uint8 *) xmalloc(st.st_size);
  length = read(fd, *data, st.st_size);
  close(fd);
  xfree(path);
  return length;
}

/*****************************************************************************/
void* xrealloc(void * in_val, int size)
{
  return realloc(in_val, size);
}

/*****************************************************************************/
void* xmalloc(int size)
{
  return malloc(size);
}

/*****************************************************************************/
void xfree(void * in_val)
{
  if (in_val != NULL)
  {
    free(in_val);
  }
}

/*****************************************************************************/
char * xstrdup(const char * s)
{
  char * mem = strdup(s);
  if (mem == NULL)
  {
    perror("strdup");
    exit(1);
  }
  return mem;
}

/*****************************************************************************/
void warning(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "WARNING: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void unimpl(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "NOT IMPLEMENTED: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void error(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "ERROR: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void out_params(void)
{
  fprintf(stderr, "rdesktop: A Remote Desktop Protocol client.\n");
  fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2005 Matt Chapman.\n");
  fprintf(stderr, "QT uiport by Jay Sorg\n");
  fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");
  fprintf(stderr, "Usage: qtrdesktop [options] server\n");
  fprintf(stderr, "   -g WxH: desktop geometry\n");
  fprintf(stderr, "   -4: use RDP version 4\n");
  fprintf(stderr, "   -5: use RDP version 5 (default)\n");
  fprintf(stderr, "   -t 3389: tcp port)\n");
  fprintf(stderr, "   -a 8|16|24: connection colour depth\n");
  fprintf(stderr, "   -T title: window title\n");
  fprintf(stderr, "   -P: use persistent bitmap caching\n");
  fprintf(stderr, "   -0: attach to console\n");
  fprintf(stderr, "   -z: enable rdp compression\n");
  fprintf(stderr, "   -r sound: enable sound\n");
  fprintf(stderr, "\n");
}

/*****************************************************************************/
/* produce a hex dump */
void hexdump(uint8 * p, uint32 len)
{
  uint8 * line = p;
  int i, thisline;
  uint32 offset = 0;

  while (offset < len)
  {
    printf("%04x ", offset);
    thisline = len - offset;
    if (thisline > 16)
    {
      thisline = 16;
    }
    for (i = 0; i < thisline; i++)
    {
      printf("%02x ", line[i]);
    }
    for (; i < 16; i++)
    {
      printf("   ");
    }
    for (i = 0; i < thisline; i++)
    {
      printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
    }
    printf("\n");
    offset += thisline;
    line += thisline;
  }
}

/*****************************************************************************/
int rd_pstcache_mkdir(void)
{
  char * home;
  char bmpcache_dir[256];

  home = getenv("HOME");
  if (home == NULL)
  {
    return False;
  }
  sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop");
  if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
  {
    perror(bmpcache_dir);
    return False;
  }
  sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop/cache");
  if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
  {
    perror(bmpcache_dir);
    return False;
  }
  return True;
}

/*****************************************************************************/
int rd_open_file(char * filename)
{
  char * home;
  char fn[256];
  int fd;

  home = getenv("HOME");
  if (home == NULL)
  {
    return -1;
  }
  sprintf(fn, "%s/.rdesktop/%s", home, filename);
  fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    perror(fn);
  }
  return fd;
}

/*****************************************************************************/
void rd_close_file(int fd)
{
  close(fd);
}

/*****************************************************************************/
int rd_read_file(int fd, void * ptr, int len)
{
  return read(fd, ptr, len);
}

/*****************************************************************************/
int rd_write_file(int fd, void * ptr, int len)
{
  return write(fd, ptr, len);
}

/*****************************************************************************/
int rd_lseek_file(int fd, int offset)
{
  return lseek(fd, offset, SEEK_SET);
}

/*****************************************************************************/
int rd_lock_file(int fd, int start, int len)
{
  struct flock lock;

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = start;
  lock.l_len = len;
  if (fcntl(fd, F_SETLK, &lock) == -1)
  {
    return False;
  }
  return True;
}

/*****************************************************************************/
void get_username_and_hostname(void)
{
  char fullhostname[64];
  char * p;
  struct passwd * pw;

  STRNCPY(g_username, "unknown", sizeof(g_username));
  STRNCPY(g_hostname, "unknown", sizeof(g_hostname));
  pw = getpwuid(getuid());
  if (pw != NULL && pw->pw_name != NULL)
  {
    STRNCPY(g_username, pw->pw_name, sizeof(g_username));
  }
  if (gethostname(fullhostname, sizeof(fullhostname)) != -1)
  {
    p = strchr(fullhostname, '.');
    if (p != NULL)
    {
      *p = 0;
    }
    STRNCPY(g_hostname, fullhostname, sizeof(g_hostname));
  }
}

/*****************************************************************************/
int parse_parameters(int in_argc, char ** in_argv)
{
  int i;
  char * p;

  if (in_argc <= 1)
  {
    out_params();
    return 0;
  }
  g_argc = in_argc;
  g_argv = in_argv;
  for (i = 1; i < in_argc; i++)
  {
    strcpy(g_servername, in_argv[i]);
    if (strcmp(in_argv[i], "-g") == 0)
    {
      g_width = strtol(in_argv[i + 1], &p, 10);
      if (g_width <= 0)
      {
        error("invalid geometry\n");
        return 0;
      }
      if (*p == 'x')
      {
        g_height = strtol(p + 1, NULL, 10);
      }
      if (g_height <= 0)
      {
        error("invalid geometry\n");
        return 0;
      }
      g_width = (g_width + 3) & ~3;
    }
    else if (strcmp(in_argv[i], "-T") == 0)
    {
      strcpy(g_title, in_argv[i + 1]);
    }
    else if (strcmp(in_argv[i], "-4") == 0)
    {
      g_use_rdp5 = 0;
    }
    else if (strcmp(in_argv[i], "-5") == 0)
    {
      g_use_rdp5 = 1;
    }
    else if (strcmp(in_argv[i], "-a") == 0)
    {
      g_server_depth = strtol(in_argv[i + 1], &p, 10);
      if (g_server_depth != 8 && g_server_depth != 15 &&
          g_server_depth != 16 && g_server_depth != 24)
      {
        error("invalid bpp\n");
        return 0;
      }
    }
    else if (strcmp(in_argv[i], "-t") == 0)
    {
      g_tcp_port_rdp = strtol(in_argv[i + 1], &p, 10);
    }
    else if (strcmp(in_argv[i], "-P") == 0)
    {
      g_bitmap_cache_persist_enable = 1;
    }
    else if (strcmp(in_argv[i], "-0") == 0)
    {
      g_console_session = 1;
    }
    else if (strcmp(in_argv[i], "-z") == 0)
    {
      g_flags |= (RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2);
    }
    else if (strcmp(in_argv[i], "-r") == 0)
    {
      if (strcmp(in_argv[i + 1], "sound") == 0)
      {
#ifdef WITH_RDPSND
        g_rdpsnd = 1;
#endif
      }
    }
  }
  return 1;
}

/*****************************************************************************/
int main(int in_argc, char** in_argv)
{
  get_username_and_hostname();
  if (!parse_parameters(in_argc, in_argv))
  {
    return 0;
  }
  if (!ui_init())
  {
    return 1;
  }
  if (!ui_create_window())
  {
    return 1;
  }
  ui_main_loop();
  ui_destroy_window();
  ui_deinit();
  return 0;
}
