/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - QT Emb System
   Copyright (C) Jay Sorg 2004-2005

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

//#define SHARP

#ifdef SHARP
#include <qpe/qpeapplication.h>
#else
#include <qapplication.h>
#endif
#include <qcursor.h>
#include <qmainwindow.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qimage.h>
#include <qsocketnotifier.h>
#include <qscrollview.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qfile.h>
#include <qcheckbox.h>
#include <qpopupmenu.h>

#include <stdlib.h>
#include <stdarg.h> // va_list va_start va_end
#include <unistd.h> // gethostname
#include <pwd.h> // getpwuid

#include "../rdesktop.h"
#include "qtewin.h"

#define QT_OPTI

extern int g_tcp_port_rdp;
int g_encryption = 1;
int g_bitmap_cache = 1;
int g_bitmap_cache_persist_enable = 0;
int g_bitmap_cache_precache = 1;
int g_use_rdp5 = 1;
int g_desktop_save = 1;
int g_bitmap_compression = 1;
int g_polygon_ellipse_orders = 0;
int g_rdp5_performanceflags =
  RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
int g_console_session = 0;
int g_keylayout = 0x409; /* Defaults to US keyboard layout */
int g_keyboard_type = 0x4; /* Defaults to US keyboard layout */
int g_keyboard_subtype = 0x0; /* Defaults to US keyboard layout */
int g_keyboard_functionkeys = 0xc; /* Defaults to US keyboard layout */
int g_width = 640;
int g_height = 480;
int g_server_bpp = 8;
char g_hostname[16] = "";
char g_username[100] = "";

#ifdef WITH_RDPSND
extern int g_dsp_busy;
extern int g_dsp_fd;
int g_rdpsnd = 0;
static QSocketNotifier * g_SoundNotifier = 0;
#endif

/* types */
struct QColorMap
{
  uint32 RGBColors[256];
  uint32 NumColors;
};

struct bitmap
{
  int w;
  int h;
  uint8 * data;
};

static int g_client_width = 640;
static int g_client_height = 480;
static uint32 g_flags = RDP_LOGON_NORMAL;
static char g_server[64] = "";
static char g_domain[16] = "";
static char g_password[16] = "";
static char g_shell[128] = "";
static char g_directory[32] = "";
static int g_fullscreen = 0;
static int g_global_sock = 0;
static int g_deactivated = 0;
static uint32 g_ext_disc_reason = 0;

static QSocketNotifier * g_SocketNotifier = 0;
#ifdef SHARP
static QPEApplication * g_App = 0;
#else
static QApplication * g_App = 0;
#endif
static QMyMainWindow * g_MW = 0;
static QMyScrollView * g_SV = 0;
static struct QColorMap * g_CM = 0;
static uint8 * g_BS = 0; /* the screen data */
static int g_clipx = 0;
static int g_clipy = 0;
static int g_clipcx = 0;
static int g_clipcy = 0;

/* Session Directory redirection */
BOOL g_redirect = False;
char g_redirect_server[64];
char g_redirect_domain[16];
char g_redirect_password[64];
char g_redirect_username[64];
char g_redirect_cookie[128];
uint32 g_redirect_flags = 0;

#define BPP ((g_server_bpp + 7) / 8)
#define GETPIXEL8(d, x, y, w) (*(((uint8*)d) + ((y) * (w) + (x))))
#define GETPIXEL16(d, x, y, w) (*(((uint16*)d) + ((y) * (w) + (x))))
#define GETPIXEL32(d, x, y, w) (*(((uint32*)d) + ((y) * (w) + (x))))
#define SETPIXEL8(d, x, y, w, v) *(((uint8*)d) + ((y) * (w) + (x))) = v
#define SETPIXEL16(d, x, y, w, v) *(((uint16*)d) + ((y) * (w) + (x))) = v
#define SETPIXEL32(d, x, y, w, v) *(((uint32*)d) + ((y) * (w) + (x))) = v

/******************************************************************************/
void CleanString(QString * Item)
{
  int i;

  i = Item->length() - 1;
  while (i >= 0)
  {
    if (Item->at(i) == 10 || Item->at(i) == 13)
    {
      Item->remove(i, 1);
    }
    i--;
  }
}

/******************************************************************************/
QMyDialog::QMyDialog(QWidget * parent) : QDialog(parent, "Settings", true)
{
  int i, j;
  char * home;
  char Text[256];
  QString Line;
  QString ItemName;
  QString ItemValue;

  // resize dialog
  resize(230, 270);
  // main list box
  ListBox = new QListBox(this);
  ListBox->move(10, 10);
  ListBox->resize(200, 100);
  connect(ListBox, SIGNAL(selectionChanged()), this, SLOT(ListBoxChanged()));
  connect(ListBox, SIGNAL(selected(int)), this, SLOT(ListBoxSelected(int)));
  // server
  Label1 = new QLabel(this);
  Label1->setText("Server Desc");
  Label1->move(10, 120);
  Label1->resize(100, 20);
  ServerNameEdit = new QLineEdit(this);
  ServerNameEdit->move(75, 120);
  ServerNameEdit->resize(100, 20);
  // username
  Label2 = new QLabel(this);
  Label2->setText("User Name");
  Label2->move(10, 150);
  Label2->resize(100, 20);
  UserNameEdit = new QLineEdit(this);
  UserNameEdit->move(75, 150);
  UserNameEdit->resize(100, 20);
  // ip
  Label3 = new QLabel(this);
  Label3->setText("Server IP");
  Label3->move(10, 180);
  Label3->resize(100, 20);
  IPEdit = new QLineEdit(this);
  IPEdit->move(75, 180);
  IPEdit->resize(100, 20);
  // width and height
  WidthHeightBox = new QComboBox(this);
  WidthHeightBox->move(10, 210);
  WidthHeightBox->resize(100, 20);
  WidthHeightBox->insertItem("240x320");
  WidthHeightBox->insertItem("640x480");
  WidthHeightBox->insertItem("800x600");
  connect(WidthHeightBox, SIGNAL(activated(int)), this, SLOT(ComboChanged(int)));
  WidthHeightBox->setCurrentItem(1);
  WidthEdit = new QLineEdit(this);
  WidthEdit->move(110, 210);
  WidthEdit->resize(30, 20);
  WidthEdit->setText("800");
  HeightEdit = new QLineEdit(this);
  HeightEdit->move(140, 210);
  HeightEdit->resize(30, 20);
  HeightEdit->setText("600");
  // add to list button
  AddButton = new QPushButton(this);
  AddButton->move(180, 120);
  AddButton->resize(50, 20);
  AddButton->setText("Add");
  connect(AddButton, SIGNAL(clicked()), this, SLOT(AddClicked()));
  // change list item button
  EditButton = new QPushButton(this);
  EditButton->move(180, 140);
  EditButton->resize(50, 20);
  EditButton->setText("Edit");
  connect(EditButton, SIGNAL(clicked()), this, SLOT(EditClicked()));
  // save to file button
  SaveButton = new QPushButton(this);
  SaveButton->move(180, 160);
  SaveButton->resize(50, 20);
  SaveButton->setText("Save");
  connect(SaveButton, SIGNAL(clicked()), this, SLOT(SaveClicked()));
  // remove an item button
  RemoveButton = new QPushButton(this);
  RemoveButton->move(180, 180);
  RemoveButton->resize(50, 20);
  RemoveButton->setText("Remove");
  connect(RemoveButton, SIGNAL(clicked()), this, SLOT(RemoveClicked()));
  // full screen check box
  FullScreenCheckBox = new QCheckBox(this, "Full Screen");
  FullScreenCheckBox->setText("Full Screen");
  FullScreenCheckBox->move(10, 230);
  // ok button
  OKButton = new QPushButton(this);
  OKButton->setText("OK");
  OKButton->move(100, 240);
  OKButton->resize(50, 20);
  connect(OKButton, SIGNAL(clicked()), this, SLOT(OKClicked()));
  // cancel button
  CancelButton = new QPushButton(this);
  CancelButton->setText("Cancel");
  CancelButton->move(160, 240);
  CancelButton->resize(50, 20);
  connect(CancelButton, SIGNAL(clicked()), this, SLOT(CancelClicked()));

  for (i = 0; i < 10; i++)
  {
    ConnectionList[i] = new QMyConnectionItem;
    ConnectionList[i]->ServerName = "";
    ConnectionList[i]->UserName = "";
    ConnectionList[i]->ServerIP = "";
    ConnectionList[i]->Width = 0;
    ConnectionList[i]->Height = 0;
    ConnectionList[i]->FullScreen = 0;
  }
  home = getenv("HOME");
  if (home != NULL)
  {
    sprintf(Text, "%s/rdesktop.ini", home);
    QFile * File = new QFile(Text);
    if (File->open(IO_ReadOnly))
    {
      i = -1;
      while (!File->atEnd())
      {
        File->readLine(Line, 255);
        j = Line.find("=");
        if (j > 0)
        {
          ItemName = Line.mid(0, j);
          CleanString(&ItemName);
          ItemValue = Line.mid(j + 1);
          CleanString(&ItemValue);
          if (ItemName == "Server")
          {
            i++;
            ConnectionList[i]->ServerName = ItemValue;
            ListBox->insertItem(ItemValue);
          }
          else if (ItemName == "UserName")
            ConnectionList[i]->UserName = ItemValue;
          else if (ItemName == "Width")
            ConnectionList[i]->Width = ItemValue.toInt();
          else if (ItemName == "Height")
            ConnectionList[i]->Height = ItemValue.toInt();
          else if (ItemName == "IP")
            ConnectionList[i]->ServerIP = ItemValue;
          else if (ItemName == "FullScreen")
            ConnectionList[i]->FullScreen = (ItemValue != "0");
        }
      }
    }
    delete File;
  }
}

/******************************************************************************/
QMyDialog::~QMyDialog()
{
  QMyConnectionItem * Item;
  int i;

  for (i = 0; i < 10; i++)
  {
    Item = ConnectionList[i];
    delete Item;
  }
}

/******************************************************************************/
void QMyDialog::ComboChanged(int index)
{
  if (index == 0)
  {
    WidthEdit->setText("240");
    HeightEdit->setText("320");
  }
  if (index == 1)
  {
    WidthEdit->setText("640");
    HeightEdit->setText("480");
  }
  else if (index == 2)
  {
    WidthEdit->setText("800");
    HeightEdit->setText("600");
  }
}

/******************************************************************************/
void QMyDialog::OKClicked()
{
  ServerName = ServerNameEdit->text();
  UserName = UserNameEdit->text();
  Width = WidthEdit->text().toInt();
  Height = HeightEdit->text().toInt();
  ServerIP = IPEdit->text();
  FullScreen = FullScreenCheckBox->isChecked();
  done(1);
}

/******************************************************************************/
void QMyDialog::CancelClicked()
{
  done(0);
}

/******************************************************************************/
void QMyDialog::AddClicked()
{
  int i;
  QMyConnectionItem * Item;

  i = ListBox->count();
  if (i < 10)
  {
    ListBox->insertItem(ServerNameEdit->text());
    Item = ConnectionList[i];
    Item->ServerName = ServerNameEdit->text();
    Item->UserName = UserNameEdit->text();
    Item->Width = WidthEdit->text().toInt();
    Item->Height = HeightEdit->text().toInt();
    Item->ServerIP = IPEdit->text();
    Item->FullScreen = FullScreenCheckBox->isChecked();
  }
}

/******************************************************************************/
void QMyDialog::EditClicked()
{
  int i;
  QMyConnectionItem * Item;

  i = ListBox->currentItem();
  if (i >= 0)
  {
    Item = ConnectionList[i];
    Item->ServerName = ServerNameEdit->text();
    Item->UserName = UserNameEdit->text();
    Item->Width = WidthEdit->text().toInt();
    Item->Height = HeightEdit->text().toInt();
    Item->ServerIP = IPEdit->text();
    Item->FullScreen = FullScreenCheckBox->isChecked();
    ListBox->changeItem(ServerNameEdit->text(), i);
  }
}

/******************************************************************************/
void WriteString(QFile* File, QString* Line)
{
  File->writeBlock((const char*)(*Line), Line->length());
}

/******************************************************************************/
void QMyDialog::SaveClicked()
{
  int i, j;
  QMyConnectionItem * Item;
  QString Line;
  char * home;
  char Text[256];
  QFile* File;

  home = getenv("HOME");
  if (home != NULL)
  {
    sprintf(Text, "%s/rdesktop.ini", home);
    File = new QFile(Text);
    if (File->open(IO_Truncate | IO_ReadWrite))
    {
      i = ListBox->count();
      for (j = 0; j < i; j++)
      {
        Item = ConnectionList[j];
        Line = "Server=";
        Line += Item->ServerName;
        Line += (char)10;
        WriteString(File, &Line);
        Line = "UserName=";
        Line += Item->UserName;
        Line += (char)10;
        WriteString(File, &Line);
        Line = "Width=";
        sprintf(Text, "%d", Item->Width);
        Line += Text;
        Line += (char)10;
        WriteString(File, &Line);
        Line = "Height=";
        sprintf(Text, "%d", Item->Height);
        Line += Text;
        Line += (char)10;
        WriteString(File, &Line);
        Line = "IP=";
        Line += Item->ServerIP;
        Line += (char)10;
        WriteString(File, &Line);
        Line = "FullScreen=";
        if (Item->FullScreen)
          Line += "1";
        else
          Line += "0";
        Line += (char)10;
        WriteString(File, &Line);
      }
    }
    File->flush();
    File->close();
    delete File;
  }
}

/******************************************************************************/
void QMyDialog::RemoveClicked()
{
  int i, j, c;
  QMyConnectionItem * Item1;
  QMyConnectionItem * Item2;

  i = ListBox->currentItem();
  if (i >= 0)
  {
    c = ListBox->count();
    for (j = i; j < c - 1; j++)
    {
      Item1 = ConnectionList[i];
      Item2 = ConnectionList[i + 1];
      Item1->ServerName = Item2->ServerName;
      Item1->UserName = Item2->UserName;
      Item1->Width = Item2->Width;
      Item1->Height = Item2->Height;
      Item1->ServerIP = Item2->ServerIP;
      Item1->FullScreen = Item2->FullScreen;
    }
    ListBox->removeItem(i);
  }
}

/******************************************************************************/
void QMyDialog::ListBoxChanged()
{
  int i;
  QMyConnectionItem * Item;
  char Text[100];

  i = ListBox->currentItem();
  if (i >= 0 && i < 10)
  {
    Item = ConnectionList[i];
    ServerNameEdit->setText(Item->ServerName);
    UserNameEdit->setText(Item->UserName);
    sprintf(Text, "%d", Item->Width);
    WidthEdit->setText(Text);
    sprintf(Text, "%d", Item->Height);
    HeightEdit->setText(Text);
    IPEdit->setText(Item->ServerIP);
    FullScreenCheckBox->setChecked(Item->FullScreen != 0);
  }
}

/******************************************************************************/
void QMyDialog::ListBoxSelected(int /*index*/)
{
}

/******************************************************************************/
void GetScanCode(QKeyEvent * e, int * ScanCode, int * code)
{
  int key;
  int mod;
  int ascii;

  key = e->key();
  mod = e->state();
  ascii = e->ascii();

  *ScanCode = 0;
  *code = mod; // 8 shift, 16 control, 32 alt

  switch (key)
  {
    case 4096: // esc
    case 4097: // tab
    case 4099: // backspace
    case 4100: // enter
    case 4101: // enter
    case 4103: // delete
      ascii = 0;
  }

  if (ascii == 0)
  {
    switch (key)
    {
      case 4096: *ScanCode = 0x01; break; // esc
      case 4097: *ScanCode = 0x0f; break; // tab
      case 4099: *ScanCode = 0x0e; break; // backspace
      case 4100: *ScanCode = 0x1c; break; // enter
      case 4101: *ScanCode = 0x1c; break; // enter
      case 4112: *ScanCode = 0xc7; break; // home
      case 4113: *ScanCode = 0xcf; break; // end
      case 4102: *ScanCode = 0xd2; break; // insert
      case 4103: *ScanCode = 0xd3; break; // delete
      case 4118: *ScanCode = 0xc9; break; // page up
      case 4119: *ScanCode = 0xd1; break; // page down
      case 4117: *ScanCode = 0xd0; break; // down arrow
      case 4115: *ScanCode = 0xc8; break; // up arrow
      case 4114: *ScanCode = 0xcb; break; // left arrow
      case 4116: *ScanCode = 0xcd; break; // right arrow
      case 4128: *ScanCode = 0x2a; break; // shift
      case 4131: *ScanCode = 0x38; break; // alt
      case 4129: *ScanCode = 0x1d; break; // ctrl
    }
    if (*ScanCode != 0)
      return;
  }

  switch (ascii)
  {
    // first row
    case 'q':  *ScanCode = 0x10; break;
    case 'Q':  *ScanCode = 0x10; *code |= 8; break;
    case '1':  *ScanCode = 0x02; break;
    case 'w':  *ScanCode = 0x11; break;
    case 'W':  *ScanCode = 0x11; *code |= 8; break;
    case '2':  *ScanCode = 0x03; break;
    case 'e':  *ScanCode = 0x12; break;
    case 'E':  *ScanCode = 0x12; *code |= 8; break;
    case '3':  *ScanCode = 0x04; break;
    case 'r':  *ScanCode = 0x13; break;
    case 'R':  *ScanCode = 0x13; *code |= 8; break;
    case '4':  *ScanCode = 0x05; break;
    case 't':  *ScanCode = 0x14; break;
    case 'T':  *ScanCode = 0x14; *code |= 8; break;
    case '5':  *ScanCode = 0x06; break;
    case 'y':  *ScanCode = 0x15; break;
    case 'Y':  *ScanCode = 0x15; *code |= 8; break;
    case '6':  *ScanCode = 0x07; break;
    case 'u':  *ScanCode = 0x16; break;
    case 'U':  *ScanCode = 0x16; *code |= 8; break;
    case '7':  *ScanCode = 0x08; break;
    case 'i':  *ScanCode = 0x17; break;
    case 'I':  *ScanCode = 0x17; *code |= 8; break;
    case '8':  *ScanCode = 0x09; break;
    case 'o':  *ScanCode = 0x18; break;
    case 'O':  *ScanCode = 0x18; *code |= 8; break;
    case '9':  *ScanCode = 0x0a; break;
    case 'p':  *ScanCode = 0x19; break;
    case 'P':  *ScanCode = 0x19; *code |= 8; break;
    case '0':  *ScanCode = 0x0b; break;
    // second row
    case 'a':  *ScanCode = 0x1e; break;
    case 'A':  *ScanCode = 0x1e; *code |= 8; break;
    case '!':  *ScanCode = 0x02; *code |= 8; break;
    case 's':  *ScanCode = 0x1f; break;
    case 'S':  *ScanCode = 0x1f; *code |= 8; break;
    case '@':  *ScanCode = 0x03; *code |= 8; break;
    case 'd':  *ScanCode = 0x20; break;
    case 'D':  *ScanCode = 0x20; *code |= 8; break;
    case '#':  *ScanCode = 0x04; *code |= 8; break;
    case 'f':  *ScanCode = 0x21; break;
    case 'F':  *ScanCode = 0x21; *code |= 8; break;
    case '$':  *ScanCode = 0x05; *code |= 8; break;
    case 'g':  *ScanCode = 0x22; break;
    case 'G':  *ScanCode = 0x22; *code |= 8; break;
    case '%':  *ScanCode = 0x06; *code |= 8; break;
    case 'h':  *ScanCode = 0x23; break;
    case 'H':  *ScanCode = 0x23; *code |= 8; break;
    case '_':  *ScanCode = 0x0c; *code |= 8; break;
    case 'j':  *ScanCode = 0x24; break;
    case 'J':  *ScanCode = 0x24; *code |= 8; break;
    case '&':  *ScanCode = 0x08; *code |= 8; break;
    case 'k':  *ScanCode = 0x25; break;
    case 'K':  *ScanCode = 0x25; *code |= 8; break;
    case '*':  *ScanCode = 0x09; *code |= 8; break;
    case 'l':  *ScanCode = 0x26; break;
    case 'L':  *ScanCode = 0x26; *code |= 8; break;
    case '(':  *ScanCode = 0x0a; *code |= 8; break;
//    case 8:    *ScanCode = 0x0e; break; // backspace
    // third row
    case 'z':  *ScanCode = 0x2c; break;
    case 'Z':  *ScanCode = 0x2c; *code |= 8; break;
    case 'x':  *ScanCode = 0x2d; break;
    case 'X':  *ScanCode = 0x2d; *code |= 8; break;
    case 'c':  *ScanCode = 0x2e; break;
    case 'C':  *ScanCode = 0x2e; *code |= 8; break;
    case 'v':  *ScanCode = 0x2f; break;
    case 'V':  *ScanCode = 0x2f; *code |= 8; break;
    case 'b':  *ScanCode = 0x30; break;
    case 'B':  *ScanCode = 0x30; *code |= 8; break;
    case '-':  *ScanCode = 0x0c; break;
    case 'n':  *ScanCode = 0x31; break;
    case 'N':  *ScanCode = 0x31; *code |= 8; break;
    case '+':  *ScanCode = 0x0d; *code |= 8; break;
    case 'm':  *ScanCode = 0x32; break;
    case 'M':  *ScanCode = 0x32; *code |= 8; break;
    case '=':  *ScanCode = 0x0d; break;
    case ',':  *ScanCode = 0x33; break;
    case ';':  *ScanCode = 0x27; break;
    case ')':  *ScanCode = 0x0b; *code |= 8; break;
    // fourth row
//    case 9:    *ScanCode = 0x0f; break; // tab
    case '/':  *ScanCode = 0x35; break;
    case '?':  *ScanCode = 0x35; *code |= 8; break;
    case ' ':  *ScanCode = 0x39; break;
    case '\'': *ScanCode = 0x28; break;
    case '"':  *ScanCode = 0x28; *code |= 8; break;
    case '~':  *ScanCode = 0x29; *code |= 8; break;
    case '.':  *ScanCode = 0x34; break;
    case ':':  *ScanCode = 0x27; *code |= 8; break;
    case '<':  *ScanCode = 0x33; *code |= 8; break;
//    case 13:   *ScanCode = 0x1c; break; // enter
    case '>':  *ScanCode = 0x34; *code |= 8; break;
    // others
//    case 27:   *ScanCode = 0x01; break; // esc
    case '`':  *ScanCode = 0x29; break;
    case '^':  *ScanCode = 0x07; *code |= 8; break;
    case '[':  *ScanCode = 0x1a; break;
    case '{':  *ScanCode = 0x1a; *code |= 8; break;
    case ']':  *ScanCode = 0x1b; break;
    case '}':  *ScanCode = 0x1b; *code |= 8; break;
    case '\\': *ScanCode = 0x2b; break;
    case '|':  *ScanCode = 0x2b; *code |= 8; break;
    // ctrl keys
    case 1:    *ScanCode = 0x1e; *code |= 16; break; // a
    case 2:    *ScanCode = 0x30; *code |= 16; break; // b
  }

  if (*ScanCode == 0 && key < 3000)
    printf("unknown key %d mod %d ascii %d\n", key, mod, ascii);

}

/******************************************************************************/
QMyScrollView::QMyScrollView() : QScrollView()
{
}

/******************************************************************************/
QMyScrollView::~QMyScrollView()
{
}

/******************************************************************************/
void QMyScrollView::keyPressEvent(QKeyEvent* e)
{
  int ScanCode, code;
  GetScanCode(e, &ScanCode, &code);
  if (ScanCode != 0)
  {
    if (code & 8) // send shift
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, 0x2a, 0);
    if (code & 16) // send control
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, 0x1d, 0);
    if (code & 32) // send alt
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, 0x38, 0);
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, ScanCode, 0);
    e->accept();
  }
}

/******************************************************************************/
void QMyScrollView::keyReleaseEvent(QKeyEvent* e)
{
  int ScanCode, code;
  GetScanCode(e, &ScanCode, &code);
  if (ScanCode != 0)
  {
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, ScanCode, 0);
    if (code & 8) // send shift
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x2a, 0);
    if (code & 16) // send control
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x1d, 0);
    if (code & 32) // send alt
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x38, 0);
    e->accept();
  }
}

/******************************************************************************/
void QMyScrollView::showEvent(QShowEvent* e)
{
  QScrollView::showEvent(e);
}

/******************************************************************************/
void QMyScrollView::show()
{
  QScrollView::show();
}

/******************************************************************************/
void QMyScrollView::polish()
{
  QScrollView::polish();
}

/******************************************************************************/
void QMyScrollView::timerEvent(QTimerEvent * e)
{
  QMyDialog * d;
  QWidget * Desktop;
  int dw;
  int dh;

  QScrollView::timerEvent(e);
  killTimer(timer_id);
  d = new QMyDialog(this);
  if (d->exec() == 1) // ok clicked
  {
    g_width = d->Width;
    g_height = d->Height;
    g_client_width = g_width;
    g_client_height = g_height;
    g_fullscreen = d->FullScreen;
    sprintf(g_server, "%s", (const char*)d->ServerIP);
    sprintf(g_username, "%s", (const char*)d->UserName);
#ifdef WITH_RDPSND
    // init sound
    if (g_rdpsnd)
    {
      rdpsnd_init();
    }
#endif
    if (!rdp_connect(g_server, g_flags, g_domain, g_password, g_shell,
                     g_directory))
    {
      delete d;
      g_SV->close();
      return;
    }
    g_BS = (uint8*)xmalloc(g_width * g_height * 4);
    memset(g_BS, 0, g_width * g_height * 4);
    g_clipx = 0;
    g_clipy = 0;
    g_clipcx = g_width;
    g_clipcy = g_height;
    g_CM = (QColorMap*)xmalloc(sizeof(struct QColorMap));
    memset(g_CM, 0, sizeof(struct QColorMap));
    g_CM->NumColors = 256;
    g_MW = new QMyMainWindow();
    g_MW->resize(g_client_width, g_client_height);
    g_MW->show();
    g_SV->addChild(g_MW);
    g_MW->setMouseTracking(true);
    g_MW->setCursor((int)10); /* Qt::BlankCursor */
    g_SocketNotifier = new QSocketNotifier(g_global_sock,
                                           QSocketNotifier::Read,
                                           g_MW);
    g_MW->connect(g_SocketNotifier, SIGNAL(activated(int)), g_MW,
                  SLOT(dataReceived()));
    if (g_fullscreen)
    {
      Desktop = g_App->desktop();
      dw = Desktop->width();
      dh = Desktop->height();
      if (dw == g_client_width && dh == g_client_height)
      {
        g_MW->resize(g_client_width - 4, g_client_height - 4);
      }
      g_SV->showFullScreen();
    }
    delete d;
  }
  else // cancel clicked
  {
    delete d;
    g_SV->close();
  }
}

/******************************************************************************/
QMyMainWindow::QMyMainWindow() : QWidget(g_SV->viewport())
{
  PopupMenu = new QPopupMenu(this);
  PopupMenu->insertItem("Right click", 1, 0);
  PopupMenu->insertItem("Toggle fullscreen", 2, 1);
  PopupMenu->insertItem("Reset keyboard", 3, 2);
  PopupMenu->insertItem("Double click", 4, 3);
  connect(PopupMenu, SIGNAL(activated(int)), this, SLOT(MemuClicked(int)));
}

/******************************************************************************/
QMyMainWindow::~QMyMainWindow()
{
  delete PopupMenu;
}

/******************************************************************************/
int rd(double in)
{
  return (int)(in + 0.50);
}

/******************************************************************************/
int c2sx(int cx)
{
  double sx;

  sx = (double)g_client_width / (double)g_width;
  return rd(cx / sx);
}

/******************************************************************************/
int c2sy(int cy)
{
  double sy;

  sy = (double)g_client_height / (double)g_height;
  return rd(cy / sy);
}

/******************************************************************************/
void QMyMainWindow::timerEvent(QTimerEvent * e)
{
  QWidget::timerEvent(e);
  if (e->timerId() == timer_id)
  {
    // send mouse up
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1,
                   rd(c2sx(mx)), rd(c2sy(my)));
    // if in fullscreen, take it out or the menu won't work
    if (g_fullscreen)
    {
      g_fullscreen = 0;
      g_SV->showNormal();
      g_SV->showMaximized();
    }
    else
      PopupMenu->popup(mapToGlobal(QPoint(mx, my)));
  }
  killTimer(timer_id);
}

/******************************************************************************/
void QMyMainWindow::MemuClicked(int MenuID)
{
  QWidget * Desktop;
  int dw;
  int dh;

  if (MenuID == 1) // right click
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON2,
                   rd(c2sx(mx)), rd(c2sy(my)));
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2,
                   rd(c2sx(mx)), rd(c2sy(my)));
  }
  else if (MenuID == 2) // toggle full screen
  {
    g_fullscreen = ~g_fullscreen;
    if (g_fullscreen)
    {
      Desktop = g_App->desktop();
      dw = Desktop->width();
      dh = Desktop->height();
      if (dw == g_client_width && dh == g_client_height)
        g_MW->resize(g_client_width - 4, g_client_height - 4);
      g_SV->showFullScreen();
    }
    else
    {
      g_SV->showNormal();
      g_SV->showMaximized();
      g_MW->resize(g_client_width, g_client_height);
    }
  }
  else if (MenuID == 3) // reset keyboard
  {
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x2a, 0); // shift
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x1d, 0); // control
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, 0x38, 0); // alt
  }
  else if (MenuID == 4) // double click
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                   rd(c2sx(mx)), rd(c2sy(my)));
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1,
                   rd(c2sx(mx)), rd(c2sy(my)));
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                   rd(c2sx(mx)), rd(c2sy(my)));
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1,
                   rd(c2sx(mx)), rd(c2sy(my)));
  }
}

/******************************************************************************/
void QMyMainWindow::mouseMoveEvent(QMouseEvent* e)
{
  int x, y;

  x = e->x();
  y = e->y();
  if (timer_id)
  {
    x = x - mx;
    y = y - my;
    if (x < -10 || x > 10 || y < -10 || y > 10)
    {
      killTimer(timer_id);
      timer_id = 0;
    }
  }
  rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, c2sx(e->x()),
                 c2sy(e->y()));
}

/******************************************************************************/
void QMyMainWindow::mousePressEvent(QMouseEvent* e)
{
  timer_id = startTimer(1000);
  mx = e->x();
  my = e->y();
  if (e->button() == LeftButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                   c2sx(e->x()), c2sy(e->y()));
  else if (e->button() == RightButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON2,
                   c2sx(e->x()), c2sy(e->y()));
  else if (e->button() == MidButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON3,
                   c2sx(e->x()), c2sy(e->y()));
}

/******************************************************************************/
void QMyMainWindow::mouseReleaseEvent(QMouseEvent* e)
{
  killTimer(timer_id);
  timer_id = 0;
  if (e->button() == LeftButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1, c2sx(e->x()),
                   c2sy(e->y()));
  else if (e->button() == RightButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2, c2sx(e->x()),
                   c2sy(e->y()));
  else if (e->button() == MidButton)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3, c2sx(e->x()),
                   c2sy(e->y()));
}

/******************************************************************************/
void QMyMainWindow::wheelEvent(QWheelEvent* e)
{
  if (e->delta() > 0)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON4, c2sx(e->x()),
                   c2sy(e->y()));
  else if (e->delta() < 0)
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON5, c2sx(e->x()),
                   c2sy(e->y()));
}

#define NOT(x) (~x)

/******************************************************************************/
int rop(int rop, int src, int dst)
{
  switch (rop)
  {
    case 0x0: return 0;
    case 0x1: return NOT (src | dst);
    case 0x2: return NOT (src) & dst;
    case 0x3: return NOT (src);
    case 0x4: return src & NOT (dst);
    case 0x5: return NOT (dst);
    case 0x6: return src ^ dst;
    case 0x7: return NOT (src & dst);
    case 0x8: return src & dst;
    case 0x9: return NOT (src) ^ dst;
    case 0xa: return dst;
    case 0xb: return NOT (src) | dst;
    case 0xc: return src;
    case 0xd: return src | NOT (dst);
    case 0xe: return src | dst;
    case 0xf: return NOT (0);
  }
  return dst;
}

/*****************************************************************************/
int get_pixel(int x, int y)
{
  if (x >= 0 && x < g_width && y >= 0 && y < g_height)
  {
    if (g_server_bpp == 8)
      return GETPIXEL8(g_BS, x, y, g_width);
    else if (g_server_bpp == 16)
      return GETPIXEL16(g_BS, x, y, g_width);
    else if (g_server_bpp == 24)
      return GETPIXEL32(g_BS, x, y, g_width);
    else
      return 0;
  }
  else
    return 0;
}

/******************************************************************************/
void set_pixel(int x, int y, int pixel, int op = 0xc)
{
  int p;

  if (x >= g_clipx && x < (g_clipx + g_clipcx) &&
      y >= g_clipy && y < (g_clipy + g_clipcy))
  {
    if (x >= 0 && x < g_width && y >= 0 && y < g_height)
    {
      if (op == 0xc)
      {
        if (g_server_bpp == 8)
        {
          SETPIXEL8(g_BS, x, y, g_width, pixel);
        }
        else if (g_server_bpp == 16)
        {
          SETPIXEL16(g_BS, x, y, g_width, pixel);
        }
        else if (g_server_bpp == 24)
        {
          SETPIXEL32(g_BS, x, y, g_width, pixel);
        }
      }
      else
      {
        if (g_server_bpp == 8)
        {
          p = GETPIXEL8(g_BS, x, y, g_width);
          p = rop(op, pixel, p);
          SETPIXEL8(g_BS, x, y, g_width, p);
        }
        else if (g_server_bpp == 16)
        {
          p = GETPIXEL16(g_BS, x, y, g_width);
          p = rop(op, pixel, p);
          SETPIXEL16(g_BS, x, y, g_width, p);
        }
        else if (g_server_bpp == 24)
        {
          p = GETPIXEL32(g_BS, x, y, g_width);
          p = rop(op, pixel, p);
          SETPIXEL32(g_BS, x, y, g_width, p);
        }
      }
    }
  }
}

/******************************************************************************/
// adjust coordinates for cliping rect
bool WarpCoords(int * x, int * y, int * cx, int * cy, int * srcx, int * srcy)
{
  int dx, dy;
  QRect InRect(*x, *y, *cx, *cy);
  QRect OutRect;
  QRect CRect(g_clipx, g_clipy, g_clipcx, g_clipcy);
  OutRect = InRect.intersect(CRect);
  if (OutRect.isEmpty())
    return false;
  dx = OutRect.x() - InRect.x();
  dy = OutRect.y() - InRect.y();
  *x = OutRect.x();
  *y = OutRect.y();
  *cx = OutRect.width();
  *cy = OutRect.height();
  if (srcx != NULL)
    *srcx = *srcx + dx;
  if (srcy != NULL)
    *srcy = *srcy + dy;
  return true;
}

/******************************************************************************/
void QMyMainWindow::paintEvent(QPaintEvent * pe)
{
  QImage * Image;
  QPainter * Painter;
  QRect Rect;
  int i, j, w, h, l, t, pixel, r, g, b;
  uint8 * data;
  double sx, sy;

  Image = 0;
  data = 0;
  if (!testWFlags(WRepaintNoErase))
    setWFlags(WRepaintNoErase);
  if (g_CM != NULL || g_server_bpp > 8)
  {
    sx = (double)g_client_width / (double)g_width;
    sy = (double)g_client_height / (double)g_height;
    Rect = pe->rect();
    l = rd(Rect.left() / sx);
    t = rd(Rect.top() / sy);
    w = rd(Rect.width() / sx);
    h = rd(Rect.height() / sy);
    if (w > 0 && h > 0)
    {
      if (g_server_bpp == 8 && g_CM->NumColors > 0)
      {
        w = (w + 3) & ~3;
        data = (uint8*)xmalloc(w * h);
        for (i = 0; i < h; i++)
          for (j = 0; j < w; j++)
            data[i * w + j] = GETPIXEL8(g_BS, l + j, t + i, g_width);
        Image = new QImage(data, w, h, 8,(QRgb*)g_CM->RGBColors,
                           g_CM->NumColors, QImage::IgnoreEndian);
      }
      else if (g_server_bpp == 16)
      {
        w = (w + 3) & ~3;
        data = (uint8*)xmalloc(w * h * 4);
        for (i = 0; i < h; i++)
          for (j = 0; j < w; j++)
          {
            pixel = GETPIXEL16(g_BS, l + j, t + i, g_width);
            r = ((pixel >> 8) & 0xf8) | ((pixel >> 13) & 0x7);
            g = ((pixel >> 3) & 0xfc) | ((pixel >> 9) & 0x3);
            b = ((pixel << 3) & 0xf8) | ((pixel >> 2) & 0x7);
            pixel = ((r << 16) | (g << 8) | b);
            SETPIXEL32(data, j, i, w, pixel);
          }
        Image = new QImage(data, w, h, 32, NULL,
                           0, QImage::IgnoreEndian);
      }
      else if (g_server_bpp == 24)
      {
        w = (w + 3) & ~3;
        data = (uint8*)xmalloc(w * h * 4);
        for (i = 0; i < h; i++)
          for (j = 0; j < w; j++)
          {
            pixel = GETPIXEL32(g_BS, l + j, t + i, g_width);
            r = (pixel >> 0) & 0xff;
            g = (pixel >> 8) & 0xff;
            b = (pixel >> 16) & 0xff;
            pixel = ((r << 16) | (g << 8) | b);
            SETPIXEL32(data, j, i, w, pixel);
          }
        Image = new QImage(data, w, h, 32, NULL,
                           0, QImage::IgnoreEndian);
      }
      if (Image != 0)
      {
        Painter = new QPainter(this);
        Painter->scale(sx, sy);
        Painter->drawImage(l, t, *Image, 0, 0, w, h);
        delete Painter;
        delete Image;
      }
      xfree(data);
    }
  }
}

/******************************************************************************/
void QMyMainWindow::closeEvent(QCloseEvent * e)
{
  e->accept();
}

/******************************************************************************/
void QMyMainWindow::dataReceived()
{
  if (!rdp_loop(&g_deactivated, &g_ext_disc_reason))
    g_SV->close();
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
        g_SoundNotifier->setEnabled(true);
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

/******************************************************************************/
void redraw(int x, int y, int cx, int cy)
{
  double sx, sy;

  if (WarpCoords(&x, &y, &cx, &cy, NULL, NULL))
  {
    sx = (double)g_client_width / (double)g_width;
    sy = (double)g_client_height / (double)g_height;
    x = rd(x * sx);
    y = rd(y * sy);
    cx = rd(cx * sx);
    cy = rd(cy * sy);
    g_MW->update(x, y, cx, cy);
  }
}

/******************************************************************************/
/* Returns 0 after user quit, 1 otherwise */
int ui_select(int rdp_socket)
{
  if (g_global_sock == 0)
    g_global_sock = rdp_socket;
  return 1;
}

/******************************************************************************/
void ui_move_pointer(int /*x*/, int /*y*/)
{
}

/******************************************************************************/
void ui_set_null_cursor(void)
{
}

/******************************************************************************/
HBITMAP ui_create_bitmap(int width, int height, uint8 * data)
{
  struct bitmap * the_bitmap;
  uint8 * bitmap_data;
  int i, j;
  int r, g, b, pixel;

  bitmap_data = (uint8*)xmalloc(width * height * 4);
  the_bitmap = (struct bitmap*)xmalloc(sizeof(struct bitmap));
  the_bitmap->w = width;
  the_bitmap->h = height;
  the_bitmap->data = bitmap_data;
  if (g_server_bpp == 8)
  {
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
        bitmap_data[i * width + j] = data[i * width + j];
  }
  else if (g_server_bpp == 16)
  {
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
        *(((uint16*)bitmap_data) + (i * width + j)) =
                     *(((uint16*)data) + (i * width + j));
  }
  else if (g_server_bpp == 24)
  {
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
      {
        r = data[(i * width + j) * 3 + 0];
        g = data[(i * width + j) * 3 + 1];
        b = data[(i * width + j) * 3 + 2];
        pixel = (r << 16) | (g << 8) | b;
        SETPIXEL32(bitmap_data, j, i, width, pixel);
      }
  }
  return the_bitmap;
}

/******************************************************************************/
void ui_paint_bitmap(int x, int y, int cx, int cy, int width,
                     int height, uint8 * data)
{
  int i, j;
  int r, g, b, pixel;

  if (g_server_bpp == 8)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        if (i < height)
          if (j < width)
            set_pixel(x + j, y + i, data[i * width + j]);
  }
  else if (g_server_bpp == 16)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        if (i < height)
          if (j < width)
            set_pixel(x + j, y + i, *(((uint16*)data) + (i * width + j)));
  }
  else if (g_server_bpp == 24)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        if (i < height)
          if (j < width)
          {
            r = data[(i * width + j) * 3 + 0];
            g = data[(i * width + j) * 3 + 1];
            b = data[(i * width + j) * 3 + 2];
            pixel = (r << 16) | (g << 8) | b;
            set_pixel(x + j, y + i, pixel);
          }
  }
  redraw(x, y, cx, cy);
}

/******************************************************************************/
void ui_destroy_bitmap(HBITMAP bmp)
{
  struct bitmap* the_bitmap;

  the_bitmap = (struct bitmap*)bmp;
  if (the_bitmap != NULL)
  {
    if (the_bitmap->data != NULL)
      xfree(the_bitmap->data);
    xfree(the_bitmap);
  }
}

/******************************************************************************/
bool is_pixel_on(uint8 * data, int x, int y, int width, int bpp)
{
  int start, shift;

  if (bpp == 1)
  {
    width = (width + 7) / 8;
    start = (y * width) + x / 8;
    shift = x % 8;
    return (data[start] & (0x80 >> shift)) != 0;
  }
  else if (bpp == 8)
    return data[y * width + x] != 0;
  else
    return false;
}

/******************************************************************************/
void set_pixel_on(uint8 * data, int x, int y, int width, int bpp, uint8 pixel)
{
  if (bpp == 8)
    data[y * width + x] = pixel;
}

/******************************************************************************/
HGLYPH ui_create_glyph(int width, int height, uint8 * data)
{
  int i, j;
  uint8* glyph_data;
  struct bitmap* the_glyph;

  glyph_data = (uint8*)xmalloc(width * height);
  the_glyph = (struct bitmap*)xmalloc(sizeof(struct bitmap));
  the_glyph->w = width;
  the_glyph->h = height;
  the_glyph->data = glyph_data;
  memset(glyph_data, 0, width * height);
  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      if (is_pixel_on(data, j, i, width, 1))
        set_pixel_on(glyph_data, j, i, width, 8, 255);
  return the_glyph;
}

/******************************************************************************/
void ui_destroy_glyph(HGLYPH glyph)
{
  struct bitmap* the_glyph;

  the_glyph = (struct bitmap*)glyph;
  if (the_glyph != NULL)
  {
    if (the_glyph->data != NULL)
      xfree(the_glyph->data);
    xfree(the_glyph);
  }
}

/******************************************************************************/
HCURSOR ui_create_cursor(uint32 x, uint32 y,
                         int width, int height,
                         uint8 * andmask, uint8 * xormask)
{
  return (void*)1;
}

/******************************************************************************/
void ui_set_cursor(HCURSOR /*cursor*/)
{
}

/*****************************************************************************/
uint16 ui_get_numlock_state(uint32 state)
{
  return 0;
}

/*****************************************************************************/
unsigned int read_keyboard_state(void)
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

/******************************************************************************/
void ui_destroy_cursor(HCURSOR /*cursor*/)
{
}

/******************************************************************************/
HCOLOURMAP ui_create_colourmap(COLOURMAP * colours)
{
  int i;
  int x;
  uint8 r, g, b;
  i = 0;
  while (i < colours->ncolours && i < 256)
  {
    r = colours->colours[i].red;
    g = colours->colours[i].green;
    b = colours->colours[i].blue;
    x = (r << 16) | (g << 8) | b;
    g_CM->RGBColors[i] = x;
    i++;
  }
  g_CM->NumColors = colours->ncolours;
  return g_CM;
}

/******************************************************************************/
void ui_set_colourmap(HCOLOURMAP map)
{
}

/******************************************************************************/
void ui_destroy_colourmap(HCOLOURMAP map)
{
}

/******************************************************************************/
void ui_begin_update(void)
{
}

/******************************************************************************/
void ui_end_update(void)
{
}

/******************************************************************************/
void ui_set_clip(int x, int y, int cx, int cy)
{
  g_clipx = x;
  g_clipy = y;
  g_clipcx = cx;
  g_clipcy = cy;
}

/******************************************************************************/
void ui_reset_clip(void)
{
  g_clipx = 0;
  g_clipy = 0;
  g_clipcx = g_width;
  g_clipcy = g_height;
}

/******************************************************************************/
void ui_bell(void)
{
  g_App->beep();
}

/******************************************************************************/
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
  int i, j;


  if (opcode == 0x0) /* black */
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, 0, 0xc);
  }
  else if (opcode == 0xf) /* white */
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, 0xffffff, 0xc);
  }
  else
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, get_pixel(x + j, y + i), opcode);
  }
  redraw(x, y, cx, cy);
}

/******************************************************************************/
// does not repaint
void fill_rect(int x, int y, int cx, int cy, int colour, int opcode = 0xc)
{
  int i, j;

  if (x + cx > g_width)
    cx = g_width - x;
  if (y + cy > g_height)
    cy = g_height - y;
#ifdef QT_OPTI
  if (opcode == 0xc) /* optimize */
  {
    if (WarpCoords(&x, &y, &cx, &cy, 0, 0))
    {
      if (g_server_bpp == 8)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
            SETPIXEL8(g_BS, x + j, y + i, g_width, colour);
      }
      else if (g_server_bpp == 16)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
            SETPIXEL16(g_BS, x + j, y + i, g_width, colour);
      }
      else if (g_server_bpp == 24)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
            SETPIXEL32(g_BS, x + j, y + i, g_width, colour);
      }
    }
  }
  else
#endif
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, colour, opcode);
  }
}

/******************************************************************************/
void ui_rect(int x, int y, int cx, int cy, int colour)
{
  fill_rect(x, y, cx, cy, colour);
  redraw(x, y, cx, cy);
}

/******************************************************************************/
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
               BRUSH * brush, int bgcolour, int fgcolour)
{
  int i, j;
  uint8 ipattern[8];

  switch (brush->style)
  {
    case 0:
      fill_rect(x, y, cx, cy, fgcolour, opcode);
      break;
    case 3:
      for (i = 0; i < 8; i++)
        ipattern[i] = ~brush->pattern[7 - i];
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          if (is_pixel_on(ipattern, (x + j + brush->xorigin) % 8,
                            (y + i + brush->yorigin) % 8, 8, 1))
            set_pixel(x + j, y + i, fgcolour, opcode);
          else
            set_pixel(x + j, y + i, bgcolour, opcode);
      break;
  }
  redraw(x, y, cx, cy);
}

/******************************************************************************/
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy,
                  int srcx, int srcy)
{
  int i, j, pixel;
  uint8 * temp;

  temp = (uint8*)xmalloc(cx * cy * 4);
#ifdef QT_OPTI
  if (opcode == 0xc)
  {
    if (WarpCoords(&x, &y, &cx, &cy, &srcx, &srcy))
    {
      if (g_server_bpp == 8)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL8(g_BS, srcx + j, srcy + i, g_width);
            SETPIXEL8(temp, j, i, cx, pixel);
          }
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL8(temp, j, i, cx);
            SETPIXEL8(g_BS, x + j, y + i, g_width, pixel);
          }
      }
      else if (g_server_bpp == 16)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL16(g_BS, srcx + j, srcy + i, g_width);
            SETPIXEL16(temp, j, i, cx, pixel);
          }
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL16(temp, j, i, cx);
            SETPIXEL16(g_BS, x + j, y + i, g_width, pixel);
          }
      }
      else if (g_server_bpp == 24)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL32(g_BS, srcx + j, srcy + i, g_width);
            SETPIXEL32(temp, j, i, cx, pixel);
          }
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            pixel = GETPIXEL32(temp, j, i, cx);
            SETPIXEL32(g_BS, x + j, y + i, g_width, pixel);
          }
      }
    }
  }
  else
#endif
  {
    if (g_server_bpp == 8)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          temp[i * cx + j] = get_pixel(srcx + j, srcy + i);
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          set_pixel(x + j, y + i, temp[i * cx + j], opcode);
    }
    else if (g_server_bpp == 16)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
        {
          pixel = get_pixel(srcx + j, srcy + i);
          SETPIXEL16(temp, j, i, cx, pixel);
        }
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
        {
          pixel = GETPIXEL16(temp, j, i, cx);
          set_pixel(x + j, y + i, pixel, opcode);
        }
    }
    else if (g_server_bpp == 24)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          *(((uint32*)temp) + (i * cx + j)) = get_pixel(srcx + j, srcy + i);
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          set_pixel(x + j, y + i, *(((uint32*)temp) + (i * cx + j)), opcode);
    }
  }
  xfree(temp);
  redraw(x, y, cx, cy);
}

/******************************************************************************/
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy)
{
  int i, j, p;
  struct bitmap * the_bitmap;

  the_bitmap = (struct bitmap*)src;
  if (the_bitmap == NULL)
    return;
#ifdef QT_OPTI
  if (opcode == 0xc) /* optimize */
  {
    if (WarpCoords(&x, &y, &cx, &cy, &srcx, &srcy))
    {
      if (g_server_bpp == 8)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            p = GETPIXEL8(the_bitmap->data, srcx + j, srcy + i, the_bitmap->w);
            SETPIXEL8(g_BS, x + j, y + i, g_width, p);
          }
      }
      else if (g_server_bpp == 16)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            p = GETPIXEL16(the_bitmap->data, srcx + j, srcy + i, the_bitmap->w);
            SETPIXEL16(g_BS, x + j, y + i, g_width, p);
          }
      }
      else if (g_server_bpp == 24)
      {
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
          {
            p = GETPIXEL32(the_bitmap->data, srcx + j, srcy + i, the_bitmap->w);
            SETPIXEL32(g_BS, x + j, y + i, g_width, p);
          }
      }
    }
  }
  else
#endif
  {
    if (g_server_bpp == 8)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          if ((i + srcy) < the_bitmap->h && (j + srcx) < the_bitmap->w)
            set_pixel(x + j, y + i,
                      the_bitmap->data[(i + srcy) * the_bitmap->w + (j + srcx)],
                      opcode);
    }
    else if (g_server_bpp == 16)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          if ((i + srcy) < the_bitmap->h && (j + srcx) < the_bitmap->w)
            set_pixel(x + j, y + i,
                      *(((uint16*)the_bitmap->data) + ((i + srcy) * the_bitmap->w + (j + srcx))),
                      opcode);
    }
    else if (g_server_bpp == 24)
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          if ((i + srcy) < the_bitmap->h && (j + srcx) < the_bitmap->w)
            set_pixel(x + j, y + i,
                      *(((uint32*)the_bitmap->data) + ((i + srcy) * the_bitmap->w + (j + srcx))),
                      opcode);
    }
  }
  redraw(x, y, cx, cy);
}

/******************************************************************************/
// not used
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy, BRUSH * brush,
               int bgcolour, int fgcolour)
{
}

/******************************************************************************/
/* Bresenham's line drawing algorithm */
void ui_line(uint8 opcode, int startx, int starty, int endx,
             int endy, PEN * pen)
{
  int dx, dy, incx, incy, dpr, dpru, p, left, top, right, bottom;

  if (startx > endx)
  {
    dx = startx - endx;
    incx = -1;
    left = endx;
    right = startx;
  }
  else
  {
    dx = endx - startx;
    incx = 1;
    left = startx;
    right = endx;
  }
  if (starty > endy)
  {
    dy = starty - endy;
    incy = -1;
    top = endy;
    bottom = starty;
  }
  else
  {
    dy = endy - starty;
    incy = 1;
    top = starty;
    bottom = endy;
  }
  if (dx >= dy)
  {
    dpr = dy << 1;
    dpru = dpr - (dx << 1);
    p = dpr - dx;
    for (; dx >= 0; dx--)
    {
      set_pixel(startx, starty, pen->colour, opcode);
      if (p > 0)
      {
        startx += incx;
        starty += incy;
        p += dpru;
      }
      else
      {
        startx += incx;
        p += dpr;
      }
    }
  }
  else
  {
    dpr = dx << 1;
    dpru = dpr - (dy << 1);
    p = dpr - dy;
    for (; dy >= 0; dy--)
    {
      set_pixel(startx, starty, pen->colour, opcode);
      if (p > 0)
      {
        startx += incx;
        starty += incy;
        p += dpru;
      }
      else
      {
        starty += incy;
        p += dpr;
      }
    }
  }
  redraw(left, top, (right - left) + 1, (bottom - top) + 1);
}

/******************************************************************************/
void draw_glyph (int x, int y, HGLYPH glyph, int fgcolour)
{
  struct bitmap *the_glyph;
  int i, j;

  the_glyph = (struct bitmap*)glyph;
  if (the_glyph == NULL)
    return;
  for (i = 0; i < the_glyph->h; i++)
    for (j = 0; j < the_glyph->w; j++)
      if (is_pixel_on(the_glyph->data, j, i, the_glyph->w, 8))
        set_pixel(x + j, y + i, fgcolour);
}

#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
    {\
      xyoffset = ttext[++idx];\
      if ((xyoffset & 0x80))\
	{\
	  if (flags & TEXT2_VERTICAL) \
	    y += ttext[idx+1] | (ttext[idx+2] << 8);\
	  else\
	    x += ttext[idx+1] | (ttext[idx+2] << 8);\
	  idx += 2;\
	}\
      else\
	{\
	  if (flags & TEXT2_VERTICAL) \
	    y += xyoffset;\
	  else\
	    x += xyoffset;\
	}\
    }\
  if (glyph != NULL)\
    {\
      draw_glyph (x + glyph->offset, y + glyph->baseline, glyph->pixmap, fgcolour);\
      if (flags & TEXT2_IMPLICIT_X)\
	x += glyph->width;\
    }\
}

/******************************************************************************/
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

  if (boxx + boxcx > g_width)
    boxcx = g_width - boxx;
  if (boxy + boxcy > g_height)
    boxcy = g_height - boxy;

  if (boxcx > 1)
    fill_rect(boxx, boxy, boxcx, boxcy, bgcolour);
  else if (mixmode == MIX_OPAQUE)
    fill_rect(clipx, clipy, clipcx, clipcy, bgcolour);

  /* Paint text, character by character */
  for (i = 0; i < length;)
  {
    switch (text[i])
    {
      case 0xff:
        if (i + 2 < length)
          cache_put_text(text[i + 1], text, text[i + 2]);
        else
        {
          error("this shouldn't be happening\n");
          exit(1);
        }
        /* this will move pointer from start to first character after FF command */
        length -= i + 3;
        text = &(text[i + 3]);
        i = 0;
        break;

      case 0xfe:
        entry = cache_get_text(text[i + 1]);
        if (entry != NULL)
        {
          if ((((uint8 *) (entry->data))[1] == 0) && (!(flags & TEXT2_IMPLICIT_X)))
          {
            if (flags & TEXT2_VERTICAL)
              y += text[i + 2];
            else
              x += text[i + 2];
          }
          for (j = 0; j < entry->size; j++)
            DO_GLYPH(((uint8 *) (entry->data)), j);
        }
        if (i + 2 < length)
          i += 3;
        else
          i += 2;
        length -= i;
        /* this will move pointer from start to first character after FE command */
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
    redraw(boxx, boxy, boxcx, boxcy);
  else
    redraw(clipx, clipy, clipcx, clipcy);
}

/******************************************************************************/
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
  uint8 * data;
  int i, j, Bpp, pixel;

  Bpp = 4;
  switch (g_server_bpp)
  {
    case 8: Bpp = 1; break;
    case 15: Bpp = 2; break;
    case 16: Bpp = 2; break;
  }
  data = (uint8*)xmalloc(cx * cy * Bpp);
  if (g_server_bpp == 8)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
      {
        pixel = get_pixel(x + j, y + i);
        SETPIXEL8(data, j, i, cx, pixel);
      }
  }
  else if (g_server_bpp == 16)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
      {
        pixel = get_pixel(x + j, y + i);
        SETPIXEL16(data, j, i, cx, pixel);
      }
  }
  else if (g_server_bpp == 24)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        *(((uint32*)data) + (i * cx + j)) = get_pixel(x + j, y + i);
  }
  offset *= Bpp;
  cache_put_desktop(offset, cx, cy, cx * Bpp, Bpp, data);
  xfree(data);
}

/******************************************************************************/
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
  uint8 * data;
  int i, j;
  int Bpp;

  Bpp = 4;
  switch (g_server_bpp)
  {
    case 8: Bpp = 1; break;
    case 15: Bpp = 2; break;
    case 16: Bpp = 2; break;
  }
  offset *= Bpp;
  data = cache_get_desktop(offset, cx, cy, Bpp);
  if (g_server_bpp == 8)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, data[i * cx + j]);
  }
  else if (g_server_bpp == 16)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, *(((uint16*)data) + (i * cx + j)));
  }
  else if (g_server_bpp == 24)
  {
    for (i = 0; i < cy; i++)
      for (j = 0; j < cx; j++)
        set_pixel(x + j, y + i, *(((uint32*)data) + (i * cx + j)));
  }
  redraw(x, y, cx, cy);
}

/*****************************************************************************/
void * xrealloc(void * in_val, int size)
{
  if (size < 1)
  {
    size = 1;
  }
  return realloc(in_val, size);
}

/*****************************************************************************/
void * xmalloc(int size)
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
BOOL rd_pstcache_mkdir(void)
{
  return 0;
}

/*****************************************************************************/
int rd_open_file(char * filename)
{
  return 0;
}

/*****************************************************************************/
void rd_close_file(int fd)
{
  return;
}

/*****************************************************************************/
int rd_read_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_write_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_lseek_file(int fd, int offset)
{
  return 0;
}

/*****************************************************************************/
BOOL rd_lock_file(int fd, int start, int len)
{
  return False;
}

/*****************************************************************************/
int load_licence(uint8 ** data)
{
  return 0;
}

/*****************************************************************************/
void save_licence(uint8 * data, int length)
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
void out_params(void)
{
  fprintf(stderr, "qterdesktop: A Remote Desktop Protocol client.\n");
  fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2004 Matt Chapman.\n");
  fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");
  fprintf(stderr, "Usage: qterdesktop [options] server\n");
  fprintf(stderr, "   -g: desktop geometry (WxH)\n");
  fprintf(stderr, "   -4: use RDP version 4\n");
  fprintf(stderr, "   -5: use RDP version 5 (default)\n");
  fprintf(stderr, "   -t: tcp port)\n");
  fprintf(stderr, "   -a: connection colour depth\n");
  fprintf(stderr, "   -u: user name\n");
  fprintf(stderr, "   -d: domain\n");
  fprintf(stderr, "   -s: shell\n");
  fprintf(stderr, "   -c: working directory\n");
  fprintf(stderr, "   -p: password (- to prompt)\n");
  fprintf(stderr, "   -n: client hostname\n");
  fprintf(stderr, "   -f: full screen\n");
  fprintf(stderr, "   -r sound: enable sound\n");
  fprintf(stderr, "\n");
}

/*****************************************************************************/
int parse_parameters(int in_argc, char ** in_argv)
{
  int i;
  char * p;

  for (i = 1; i < in_argc; i++)
  {
    strcpy(g_server, in_argv[i]);
    if (strcmp(in_argv[i], "-h") == 0)
    {
      out_params();
      return 0;
    }
    else if (strcmp(in_argv[i], "-g") == 0)
    {
      g_width = strtol(in_argv[i + 1], &p, 10);
      if (*p == 'x')
      {
        g_height = strtol(p + 1, &p, 10);
      }
      if (*p == '-')
      {
        g_client_width = strtol(p + 1, &p, 10);
      }
      else
      {
        g_client_width = g_width;
        g_client_height = g_height;
      }
      if (*p == 'x')
      {
        g_client_height = strtol(p + 1, NULL, 10);
      }
      g_width = (g_width + 3) & ~3;
      g_height = (g_height + 3) & ~3;
      g_client_width = (g_client_width + 3) & ~3;
      g_client_height = (g_client_height + 3) & ~3;
      i++;
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
      g_server_bpp = strtol(in_argv[i + 1], &p, 10);
      if (g_server_bpp != 8 &&
          g_server_bpp != 16 && g_server_bpp != 24)
      {
        error("invalid bpp\n");
        return 0;
      }
      i++;
    }
    else if (strcmp(in_argv[i], "-t") == 0)
    {
      g_tcp_port_rdp = strtol(in_argv[i + 1], &p, 10);
      i++;
    }
    else if (strcmp(in_argv[i], "-u") == 0)
    {
      STRNCPY(g_username, in_argv[i + 1], sizeof(g_username));
      i++;
    }
    else if (strcmp(in_argv[i], "-d") == 0)
    {
      STRNCPY(g_domain, in_argv[i + 1], sizeof(g_domain));
      i++;
    }
    else if (strcmp(in_argv[i], "-s") == 0)
    {
      STRNCPY(g_shell, in_argv[i + 1], sizeof(g_shell));
      i++;
    }
    else if (strcmp(in_argv[i], "-c") == 0)
    {
      STRNCPY(g_directory, in_argv[i + 1], sizeof(g_directory));
      i++;
    }
    else if (strcmp(in_argv[i], "-p") == 0)
    {
      STRNCPY(g_password, in_argv[i + 1], sizeof(g_password));
      g_flags |= RDP_LOGON_AUTO;
      i++;
    }
    else if (strcmp(in_argv[i], "-n") == 0)
    {
      STRNCPY(g_hostname, in_argv[i + 1], sizeof(g_hostname));
      i++;
    }
    else if (strcmp(in_argv[i], "-f") == 0)
    {
      g_fullscreen = 1;
    }
    else if (strcmp(in_argv[i], "-r") == 0)
    {
      if (strcmp(in_argv[i + 1], "sound") == 0)
      {
#ifdef WITH_RDPSND
        g_rdpsnd = 1;
#endif
      }
      i++;
    }
  }
  return 1;
}

/******************************************************************************/
int param_connect(void)
{
  QWidget * Desktop;
  int dw, dh;

#ifdef WITH_RDPSND
  // init sound
  if (g_rdpsnd)
  {
    rdpsnd_init();
  }
#endif
  if (rdp_connect(g_server, g_flags, g_domain, g_password, g_shell,
                  g_directory))
  {
    g_BS = (uint8*)xmalloc(g_width * g_height * 4);
    memset(g_BS, 0, g_width * g_height * 4);
    g_clipx = 0;
    g_clipy = 0;
    g_clipcx = g_width;
    g_clipcy = g_height;
    g_CM = (QColorMap*)xmalloc(sizeof(struct QColorMap));
    memset(g_CM, 0, sizeof(struct QColorMap));
    g_CM->NumColors = 256;
    g_MW = new QMyMainWindow();
    g_MW->resize(g_client_width, g_client_height);
    g_MW->show();
    g_SV->addChild(g_MW);
    g_MW->setMouseTracking(true);
    g_SocketNotifier = new QSocketNotifier(g_global_sock,
                                           QSocketNotifier::Read,
                                           g_MW);
    g_MW->connect(g_SocketNotifier, SIGNAL(activated(int)), g_MW,
                  SLOT(dataReceived()));
    if (g_fullscreen)
    {
      Desktop = g_App->desktop();
      dw = Desktop->width();
      dh = Desktop->height();
      if (dw == g_client_width && dh == g_client_height)
      {
        g_MW->resize(g_client_width - 4, g_client_height - 4);
      }
      g_SV->showFullScreen();
    }
    g_MW->setCursor((int)10); /* Qt::BlankCursor */
    g_App->exec();
  }
  return 0;
}

/******************************************************************************/
int main(int argc, char ** argv)
{
#ifdef SHARP
  g_App = new QPEApplication(argc, argv);
#else
  g_App = new QApplication(argc, argv, QApplication::GuiServer);
  //g_App = new QApplication(argc, argv);
#endif
  g_SV = new QMyScrollView();
  g_App->setMainWidget(g_SV);
  g_SV->showMaximized();
  if (argc > 1)
  {
    get_username_and_hostname();
    if (parse_parameters(argc, argv))
    {
      param_connect();
    }
  }
  else
  {
    g_SV->timer_id = g_SV->startTimer(1000); /* one sec delay, then dialog */
    g_App->exec();
  }
  delete g_SV;
  delete g_App;
  xfree(g_CM);
  xfree(g_BS);
  return 0;
}
