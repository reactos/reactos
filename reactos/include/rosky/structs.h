#ifndef __RSK_STRUCTS_H
#define __RSK_STRUCTS_H

typedef unsigned int COLOR;

typedef struct region
{
  int x1;
  int y1;
  int x2;
  int y2;
} s_region;

typedef struct s_gi_msg
{
  HANDLE win;
  unsigned int type;
  unsigned int para1;
  unsigned int para2;
  s_region rect;
  struct s_gi_msg *next;
  struct s_gi_msg *prev;
  unsigned long long timestamp;
} s_gi_msg;

typedef struct DDB
{
  unsigned int color;
  unsigned int width;
  unsigned int height;
  unsigned char *data;
  unsigned int palette_size;
  unsigned int transcolor;
  unsigned char trans;
  unsigned char *bAndMask;
  unsigned char bUseAndMask;
  unsigned int uiAndMaskWidth;
  unsigned int uiAndMaskHeight;
  COLOR *palette;
} DDB;

typedef struct DIB
{
  unsigned int color;
  unsigned int width;
  unsigned int height;
  unsigned char *data;

  unsigned int palette_size;
  unsigned int transcolor;
  unsigned char trans;

  unsigned char *bAndMask;
  unsigned char bUseAndMask;
  unsigned int uiAndMaskWidth;
  unsigned int uiAndMaskHeight;
  unsigned int uiFlags;
  COLOR *palette;
} DIB;

typedef struct GC
{
  unsigned int type;
  HANDLE window;
  DIB *hDIB;
  unsigned int width;
  unsigned int height;
  s_region *clip;
  COLOR fg_color;
  COLOR bg_color;
  COLOR trans_color;
  unsigned int uiTransparentLevel;
  unsigned int flags;
  unsigned int fontIndex;
  unsigned int fontSize;
  unsigned int fontFlags;
} GC;

typedef struct sBlit
{
  DIB *hDIB;
  DDB *hDDB;
  int iDestX;
  int iDestY;
  int iSrcX;
  int iSrcY;
  int iWidth;
  int iHeight;
  unsigned int uiFlags;
  unsigned int uiReserved0;
  unsigned int uiReserved1;
  unsigned int uiReserved2;
  unsigned int uiReserved3;
  unsigned int uiReserved4;
  unsigned int uiReserved5;
  unsigned int uiReserved6;
  unsigned int uiReserved7;
  unsigned int uiReserved8;
  unsigned int uiReserved9;
} sBlit;

typedef struct widget_dynbmp_item
{
  DIB *hDib;
  unsigned char *rawData;
  struct widget_dynbmp_item *next;
} widget_dynbmp_item;

typedef struct widget_dynbmp
{
  unsigned int state;
  unsigned int trans;
  unsigned int transcolor;
  unsigned int thread_id;
  unsigned int timer_id;
  widget_dynbmp_item *first;
  widget_dynbmp_item *selected;
} widget_dynbmp;

typedef struct widget_popup
{
  unsigned int uiItemHeight;
  unsigned int uiFlags;
  HANDLE hFont;
  unsigned int uiFontFlags;
  unsigned int uiFontSize;

  unsigned int uiColorSelectedBack;
  unsigned int uiColorSelectedFore;
  unsigned int uiColorBack;
  unsigned int uiColorFore;
  unsigned int uiWindowBackColor;

  unsigned int uiSpacingX;
} widget_popup;

typedef struct widget_menu_item
{
  unsigned char text[255];
  unsigned int ID;
  unsigned int flags;
  struct widget_menu_item *next;
  struct widget_menu *child;
  unsigned int focus;
  unsigned int enabled;
  unsigned int x;
  HANDLE icon;
  DIB *hDIB;
  unsigned int has_icon;

  /* sub items */
  unsigned int width;
  unsigned int count;
} widget_menu_item;

typedef struct widget_menu
{
  unsigned char focus;
  unsigned int count;
  unsigned int width;
  unsigned int has_icons;
  widget_menu_item *items;
  widget_dynbmp *animation;
  widget_popup *pPopUpData;
  unsigned int uiLineColor;
  unsigned int uiBackGroundColor;
} widget_menu;

typedef struct app_para
{
  unsigned char cpName[255];
  unsigned int ulX;
  unsigned int ulY;
  unsigned int ulWidth;
  unsigned int ulHeight;

  void *win_func;
  unsigned int ulStyle;
  unsigned int ulBackGround;

  unsigned int ulAppIcon;
  widget_menu *pMenu;
} app_para;

typedef struct s_window
{
  unsigned char name[255];
  unsigned int x;
  unsigned int y;
  unsigned int height;
  unsigned int width;
  unsigned int orgx;
  unsigned int orgy;
  unsigned long (__cdecl *win_func)(HANDLE win, s_gi_msg *m);
  HANDLE handle;

  struct s_window *parent;
  struct s_window *child;
  struct s_window *next;

  unsigned char focus;
  struct s_window *focus_win;
  void *windowData;
  unsigned int windowDataSize;

  unsigned int flags;
  int origin_x;
  int origin_y;
} s_window;

typedef struct sCreateApplication
{
  unsigned char ucApplicationName[255];
  unsigned int uiX;
  unsigned int uiY;
  unsigned int uiWidth;
  unsigned int uiHeight;

  void *fwndClient;
  unsigned int uiStyleApplication;
  unsigned int uiStyleFrame;
  unsigned int uiStyleTitle;
  unsigned int uiStyleMenu;
  unsigned int uiStyleBar;
  unsigned int uiStyleClient;
  unsigned int uiBackGroundColor;
  unsigned int uiApplicationIcon;
  widget_menu  *pFrameMenu;

  unsigned int uiReserved[128];

  void (__cdecl *PostCreateWindowBitmap)(HANDLE hWnd, void *pGCBuf);
} sCreateApplication;

typedef struct s_resolution
{
  unsigned int width;
  unsigned int height;
  unsigned int bpp;
} s_resolution;

#endif /* __RSK_STRUCTS_H */
