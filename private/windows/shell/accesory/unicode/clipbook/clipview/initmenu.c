
#define WIN31
#define STRICT

#include "windows.h"
#include <windowsx.h>
#include <assert.h>
#include <memory.h>
#include "clipbook.h"
#include "clipdsp.h"
#include "commctrl.h"
#include "..\common\common.h"

extern BOOL fAuditEnabled;


// InitializeMenu /////////////////////////////////
//
// this function controls the enabled/grayed state of
// the menu items and the state of the toolbar buttons.
// It is called when the selection within a listbox changes,
// or the focus changes from one MDI child window to another.
VOID InitializeMenu (
HANDLE hmenu)
{
   int index;
   int fMenu;
   int fButton;
   DWORD flags;
   LPLISTENTRY lpLE;
   WINDOWPLACEMENT wpl;

   assert(NULL != pActiveMDI);

   hmenu = GetMenu(hwndApp);

   flags = pActiveMDI->flags;

   if ( flags & F_CLPBRD )
      {
      index = LB_ERR;
      }
   else if ( pActiveMDI->hWndListbox )
      {
      index = (int)SendMessage (  pActiveMDI->hWndListbox,
         LB_GETCURSEL, 0, 0L );

      if ( index != LB_ERR )
         {
         SendMessage (  pActiveMDI->hWndListbox, LB_GETTEXT, index,
            (LPARAM)(LPTSTR)&lpLE);
         }
      }

   EnableMenuItem (hmenu, IDM_OPEN,
         (flags & F_CLPBRD ? MF_ENABLED : MF_GRAYED)| MF_BYCOMMAND);
   EnableMenuItem (hmenu, IDM_SAVEAS,
      (CountClipboardFormats() && flags & (F_CLPBRD | F_LOCAL) ? MF_ENABLED : MF_GRAYED)
       | MF_BYCOMMAND );

   if ( fShareEnabled )
      {
      // SHARE allowed?
      if ( (flags & F_LOCAL) && (index != LB_ERR) )
         {
         fMenu = MF_ENABLED;
         fButton = TRUE;
         }
      else
         {
         fMenu = MF_GRAYED;
         fButton = FALSE;
         }
      EnableMenuItem (hmenu, IDM_SHARE, fMenu | MF_BYCOMMAND );
      SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_SHARE, fButton );

      // UNSHARE allowed?
      if ( (flags & F_LOCAL) && (index != LB_ERR) &&  IsShared(lpLE) )
         {
         fMenu = MF_ENABLED;
         fButton = TRUE;
         }
      else
         {
         fMenu = MF_GRAYED;
         fButton = FALSE;
         }
      EnableMenuItem (hmenu, IDM_UNSHARE, fMenu | MF_BYCOMMAND);
      SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_UNSHARE, fButton );
      }

   if ( fNetDDEActive )
      {
      // DISCONNECT allowed?
      EnableMenuItem (hmenu, IDM_DISCONNECT,
         (flags & ( F_LOCAL | F_CLPBRD ) ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND );
      SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_DISCONNECT,
         flags & ( F_LOCAL | F_CLPBRD ) ? FALSE : TRUE );
      }
   else // If netdde isn't active, we can't connect
      {
      EnableMenuItem(hmenu, IDM_CONNECT, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hmenu, IDM_DISCONNECT, MF_GRAYED | MF_BYCOMMAND);
      SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_CONNECT, FALSE);
      SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_DISCONNECT, FALSE);
      }

   // Delete allowed?
   if ( ( flags & F_LOCAL && index != LB_ERR ) ||
      flags & F_CLPBRD && CountClipboardFormats() )
      {
      fMenu = MF_ENABLED;
      fButton = TRUE;
      }
   else
      {
      fMenu = MF_GRAYED;
      fButton = FALSE;
      }
   EnableMenuItem (hmenu, IDM_DELETE, fMenu | MF_BYCOMMAND);
   SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_DELETE, fButton );

   // Copy allowed?
   if ( index != LB_ERR  )
      {
      fMenu = MF_ENABLED;
      fButton = TRUE;
      }
   else
      {
      fMenu = MF_GRAYED;
      fButton = FALSE;
      }

   EnableMenuItem (hmenu, IDM_COPY, fMenu | MF_BYCOMMAND);
   SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_COPY, fButton );

   // Paste Allowed?
   if (CountClipboardFormats() && hwndLocal)
      {
      fMenu = MF_ENABLED;
      fButton = TRUE;
      }
   else
      {
      fMenu = MF_GRAYED;
      fButton = FALSE;
      }
   EnableMenuItem(hmenu, IDM_KEEP, fMenu | MF_BYCOMMAND);
   SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_KEEP, fButton );

   // TOOLBAR, STATUS BAR
   CheckMenuItem ( hmenu, IDM_TOOLBAR, fToolBar ? MF_CHECKED : MF_UNCHECKED );
   CheckMenuItem ( hmenu, IDM_STATUSBAR, fStatus ? MF_CHECKED:MF_UNCHECKED );

   // disable some view options if clipboard window
   EnableMenuItem ( hmenu, IDM_LISTVIEW,
      (flags & F_CLPBRD ? MF_GRAYED : MF_ENABLED) & MF_BYCOMMAND );
   EnableMenuItem ( hmenu, IDM_PREVIEWS,
      (flags & F_CLPBRD ? MF_GRAYED : MF_ENABLED) & MF_BYCOMMAND );
   EnableMenuItem ( hmenu, IDM_PAGEVIEW,
      ((flags & F_CLPBRD) || index != LB_ERR ? MF_ENABLED : MF_GRAYED) |
      MF_BYCOMMAND );
   SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_LISTVIEW,
         flags & F_CLPBRD ? FALSE : TRUE );
   SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_PREVIEWS,
         flags & F_CLPBRD ? FALSE : TRUE );
   SendMessage ( hwndToolbar, TB_ENABLEBUTTON, IDM_PAGEVIEW,
      (( flags & F_CLPBRD ) || index != LB_ERR ) ? TRUE : FALSE );


   // Disable "Security" menu for other than Local Clipbook window
   // or if there's no item selected in the clipbook window
   fMenu = MF_GRAYED | MF_BYCOMMAND;
   if ((flags & F_LOCAL) && LB_ERR != index)
      {
      fMenu = MF_ENABLED | MF_BYCOMMAND;
      }
   EnableMenuItem(hmenu, IDM_PERMISSIONS, fMenu);
   EnableMenuItem(hmenu, IDM_OWNER, fMenu);
   EnableMenuItem(hmenu, IDM_AUDITING, fAuditEnabled ? fMenu :
         MF_GRAYED | MF_BYCOMMAND);


   // check selected view...
   CheckMenuItem ( hmenu, IDM_LISTVIEW, pActiveMDI->DisplayMode == DSP_LIST ?
      MF_CHECKED : MF_UNCHECKED );
   CheckMenuItem ( hmenu, IDM_PREVIEWS, pActiveMDI->DisplayMode == DSP_PREV ?
      MF_CHECKED : MF_UNCHECKED );
   CheckMenuItem ( hmenu, IDM_PAGEVIEW, pActiveMDI->DisplayMode == DSP_PAGE ?
      MF_CHECKED : MF_UNCHECKED );
   SendMessage ( hwndToolbar, TB_CHECKBUTTON, IDM_LISTVIEW,
      pActiveMDI->DisplayMode == DSP_LIST ? TRUE : FALSE );
   SendMessage ( hwndToolbar, TB_CHECKBUTTON, IDM_PREVIEWS,
      pActiveMDI->DisplayMode == DSP_PREV ? TRUE : FALSE );
   SendMessage ( hwndToolbar, TB_CHECKBUTTON, IDM_PAGEVIEW,
      pActiveMDI->DisplayMode == DSP_PAGE ? TRUE : FALSE );

   DrawMenuBar(hwndApp);
}

