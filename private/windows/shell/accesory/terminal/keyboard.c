/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "task.h"


/*---------------------------------------------------------------------------*/
/* KEYBOARD REMAPPING ROUTINES (and DATA)                              [mbb] */
/*---------------------------------------------------------------------------*/

#define KEYMAP                   struct tagKeyMap
#define LPKEYMAP                 KEYMAP FAR *

struct tagKeyMap
{
   BYTE  scanCode[256];
   INT   virtKey[8][256];
   WORD  dataBytes;
};

HANDLE   hKeyMap;


VOID keyMapInit()                            /* mbbx 1.04: all new... */
{
   hKeyMap = NULL;
   keyMapState = 0;
}


VOID keyMapCancel()
{
   if(hKeyMap != NULL)
      hKeyMap = GlobalFree(hKeyMap);
}

BOOL NEAR keyMapSetState(WPARAM wParam)
{
   keyMapState = 0;

   if(GetKeyState(VK_SHIFT) & 0x8000)
      keyMapState |= VKS_SHIFT;
   if(GetKeyState(VK_CONTROL) & 0x8000)
      keyMapState |= VKS_CTRL;
   if(GetKeyState(VK_MENU) & 0x8000)
      keyMapState |= VKS_ALT;

   keyMapState |= (keyMapState >> 8);

   return(((wParam >= VK_SHIFT) && (wParam <= VK_MENU)) ? TRUE : FALSE);
}


BOOL keyMapTranslate(WPARAM *wParam, LONG *lParam, STRING *mapStr)
{
   BOOL        keyMapTranslate = FALSE;
   LPKEYMAP    lpKeyMap;
   WORD        wVirtKey;
   LPINT       lpVirtKey;
   LPSTR       lpKeyData;

   if(hKeyMap == NULL)
   {
      keyMapSetState(*wParam);
      return(FALSE);
   }

   if((lpKeyMap = (LPKEYMAP) GlobalLock(hKeyMap)) == NULL)
   {
      return(FALSE);
   }

   if((wVirtKey = lpKeyMap->scanCode[(*lParam >> 16) & 0x00FF]) != 0)
      *wParam = wVirtKey;

   if(keyMapSetState(*wParam))
      return(FALSE);

   lpVirtKey = &lpKeyMap->virtKey[keyMapState >> 8][*wParam & 0x00FF];

   if(*lpVirtKey != -1)
   {
      if((*lpVirtKey & 0xF000) == 0xF000)
      {
         keyMapState = (*lpVirtKey & 0x0700) | (keyMapState & 0x00FF);
         *wParam = (*lpVirtKey & 0x00FF);
      }
      else
      {
         if(mapStr != NULL)
         {
            if(*lpVirtKey & 0x8000)
            {
               *mapStr = 1;
               mapStr[1] = (BYTE) *lpVirtKey;
            }
            else
            {
               lpKeyData = ((LPSTR) lpKeyMap) + sizeof(KEYMAP) + *lpVirtKey;
               lmovmem(lpKeyData, (LPSTR) mapStr, *lpKeyData+1);
            }
            mapStr[*mapStr+1] = 0;
         }

         keyMapTranslate = TRUE;
      }
   }

   GlobalUnlock(hKeyMap);
   return(keyMapTranslate);
}

//
// Brain Dead mucking with our lParam values or CHARS
// Causes TranslateMessage to fail.  -JohnHall for WLO
//
VOID NEAR keyMapSendMsg(MSG   *msg, WPARAM wParam, BOOL  bKeyDown)
{
   /*
   msg->message = (bKeyDown ? WM_KEYDOWN : WM_KEYUP);

   	
   if(keyMapState & VKS_ALT)
   {
	msg->message += (WM_SYSKEYDOWN - WM_KEYDOWN);
   }

   TranslateMessage((LPMSG) msg);

   DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
   */ 

   
   if (bKeyDown) {
      msg->message &= ~0x0001;
   } else {
      msg->message |= 0x0001;
   }                                                                   
   TranslateMessage((LPMSG) msg);
   //DbgPrint("msg.msg %x msg.wparam %x msg.lparam %lx\n",
   //	     msg->message, msg->wParam, msg->lParam);
   DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
   
}


VOID keyMapKeyProc(HWND  hWnd, WORD message, WPARAM wParam, LONG lParam)  // sdj: AltGr
{
   BOOL  bSetState;
   BYTE  keyState[256], newKeyState[256];
   MSG   msg;

   if(bSetState = ((keyMapState & (VKS_SHIFT | VKS_CTRL | VKS_ALT)) != 
                   ((keyMapState << 8) & (VKS_SHIFT | VKS_CTRL | VKS_ALT))))
   {
      GetKeyboardState((LPSTR) keyState);
      memcpy(newKeyState, keyState, 256);

      newKeyState[VK_SHIFT]   = (keyMapState & VKS_SHIFT) ? 0x81 : 0x00;
      newKeyState[VK_CONTROL] = (keyMapState & VKS_CTRL) ? 0x81 : 0x00;
      newKeyState[VK_MENU]    = (keyMapState & VKS_ALT) ? 0x81 : 0x00;

      SetKeyboardState((LPSTR) newKeyState);
   }

   msg.hwnd = hWnd;
   msg.wParam = wParam;
   msg.lParam = lParam;
   msg.message = message;  //sdj AltGr
   keyMapSendMsg(&msg, wParam, (lParam & (1L << 31)) ? FALSE : TRUE);

   if(bSetState)
      SetKeyboardState((LPSTR) keyState);
}


BOOL keyMapSysKey(HWND	hWnd, WORD message, WPARAM	*wParam, LONG  lParam) //sdj: AltGr
{
   MSG   msg;

   if((*wParam >= VK_SHIFT) && (*wParam <= VK_MENU))
   {
      msg.hwnd = hItWnd;
      msg.wParam = *wParam;
      msg.lParam = lParam;
      msg.message = message;  //sdj: AltGr
      keyMapSendMsg(&msg, *wParam, (lParam & (1L << 31)) ? FALSE : TRUE);
      return(TRUE);
   }

   if(keyMapState & VKS_CTRL)
   {

/* jtf 3.30      if(*wParam == VK_TAB)
      {
         return(TRUE);
      }
      else  */

      if((*wParam >= VK_F1) && (*wParam <= VK_F10))
      {
         if(!(lParam & (1L << 31)))
         {
            if(keyMapState & VKS_ALT)
               selectFKey(IDFK1+(*wParam-VK_F1));
/* jtf 3.30            else if(hWnd != hItWnd)
            {
               switch(*wParam)
               {
               case VK_F4:
                  *wParam = SC_CLOSE;
                  break;
               case VK_F5:
                  *wParam = SC_RESTORE;
                  break;
               case VK_F6:
                  makeActiveNext(keyMapState & VKS_SHIFT);
                  return(TRUE);
               case VK_F7:
                  *wParam = SC_MOVE;
                  break;
               case VK_F8:
                  *wParam = SC_SIZE;
                  break;
               case VK_F10:
                  *wParam = SC_MAXIMIZE;
                  break;
               }
               SendMessage(hWnd, WM_SYSCOMMAND, *wParam, 0L);
            }
*/
         }

         return(TRUE);
      }
   }
   else
   if(keyMapState & VKS_ALT)
   {
      switch(*wParam)
      {
      case VK_BACK:                          /* ALT BACK -> UNDO */
	 keyMapKeyProc(hWnd, message , *wParam, lParam); //sdj: AltGr
         return(TRUE);

      case VK_F1:
      case VK_F2:
         *wParam += 10;
         keyMapState &= ~VKS_ALT;
         break;

      case VK_F4:
      case VK_F5:
      case VK_F6:
      case VK_F7:
      case VK_F8:
      case VK_F9:
      case VK_F10:
         DefWindowProc(hItWnd, !(lParam & (1L << 31)) ? WM_SYSKEYDOWN : WM_SYSKEYUP, *wParam, lParam);
         return(TRUE);

      default:
	 keyMapKeyProc(hItWnd, message, *wParam, lParam); // sdj: AltGr
         return(TRUE);
      }
   }

   return(FALSE);
}

/*---------------------------------------------------------------------------*/
/* classifyKey() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

#define TKS_IDPGUP               0           /* mbbx 2.00 ... */
#define TKS_IDLEFT               4
#define TKS_IDINSERT             8
#define TKS_IDNUMERIC0           10
#define TKS_IDF1                 26
#define TKS_IDSHIFTF1            38

INT classifyKey(WORD  vrtKey)
{
   INT   classifyKey = TERMINALFKEY;

   switch(vrtKey)
   {
   case VK_CANCEL:
      if(keyMapState & VKS_CTRL)
         classifyKey = ((keyMapState & VKS_SHIFT) ? LONGBREAK : SHORTBREAK);
      else
         classifyKey = STANDARDKEY;
      break;

   case VK_PRIOR:
   case VK_NEXT:
   case VK_END:
   case VK_HOME:
   case VK_LEFT:
   case VK_UP:
   case VK_RIGHT:
   case VK_DOWN:
      /* rjs - add test for trmParams.useWinCtrl */   
      if(!((GetKeyState(0x91) & 0x0001) || trmParams.useWinCtrl))
         seqTableNdx = (vrtKey - VK_PRIOR) + TKS_IDPGUP;
      else
         classifyKey = SCROLLKEY;
      break;

   case VK_SELECT:
      seqTableNdx = (VK_END - VK_PRIOR) + TKS_IDPGUP;
      break;

   case VK_INSERT:
      classifyKey = (!(keyMapState & (VKS_CTRL | VKS_SHIFT)) ? TERMINALFKEY : STANDARDKEY);
      seqTableNdx = TKS_IDINSERT;
      break;

   case VK_DELETE:
      classifyKey = (!(keyMapState & VKS_SHIFT) ? TERMINALFKEY : STANDARDKEY);
      seqTableNdx = (VK_DELETE - VK_INSERT) + TKS_IDINSERT;
      break;

   case VK_NUMPAD0:
   case VK_NUMPAD1:
   case VK_NUMPAD2:
   case VK_NUMPAD3:
   case VK_NUMPAD4:
   case VK_NUMPAD5:
   case VK_NUMPAD6:
   case VK_NUMPAD7:
   case VK_NUMPAD8:
   case VK_NUMPAD9:
   case VK_MULTIPLY:
   case VK_ADD:
   case VK_SEPARATOR:
   case VK_SUBTRACT:
   case VK_DECIMAL:
   case VK_DIVIDE:
      seqTableNdx = (vrtKey - VK_NUMPAD0) + TKS_IDNUMERIC0;
      break;

   case VK_F1:
   case VK_F2:
   case VK_F3:
   case VK_F4:
   case VK_F5:
   case VK_F6:
   case VK_F7:
   case VK_F8:
   case VK_F9:
   case VK_F10:
   case VK_F11:
   case VK_F12:
      /* rjs - add test for trmParams.useWinCtrl */   
      if(!((GetKeyState(0x91) & 0x0001) || trmParams.useWinCtrl))
            {
            if (vrtKey==VK_F1)
               doCommand(hTermWnd, HMINDEX, 0);
            classifyKey = STANDARDKEY;
            break;
            } 
      seqTableNdx = (vrtKey - VK_F1) + (!(keyMapState & VKS_SHIFT) ? TKS_IDF1 : TKS_IDSHIFTF1);
      break;
   case VK_F13:
   case VK_F14:
   case VK_F15:
   case VK_F16:
      seqTableNdx = (vrtKey - VK_F13) + TKS_IDSHIFTF1;
      break;
   default:
      classifyKey = STANDARDKEY;
      break;
   }

   return(classifyKey);
}


/*---------------------------------------------------------------------------*/
/* keyPadSequence() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

BOOL keyPadSequence()
{
   INT      ndx;
   LPBYTE   emulKeyBase;
   BYTE     keyPadByte;

   if(!(emulKeyBase = GlobalLock(hemulKeyInfo)))
      return(FALSE);

   emulKeyBase += (seqTableNdx * KEYSEQLEN);
   *keyPadString = 0;
   for(ndx = 0; *(emulKeyBase + ndx) != 0; ndx++)
      keyPadString[++(*keyPadString)] = *(emulKeyBase + ndx);
   GlobalUnlock(hemulKeyInfo);

   if((trmParams.emulate >= ITMVT52) && (trmParams.emulate <= ITMVT220))
   {
      if(((seqTableNdx >= TKS_IDLEFT) && (seqTableNdx < TKS_IDINSERT)) && cursorKeyMode)
      {
         switch(trmParams.emulate)
         {
         case ITMVT100:
            keyPadString[2] = 0x4F;
            break;
         }
      }
      else if(((seqTableNdx >= TKS_IDNUMERIC0) && (seqTableNdx < TKS_IDF1)) && !keyPadAppMode)
      {
         switch(seqTableNdx)
         {
         case TKS_IDNUMERIC0+10:
            keyPadByte = '-';
            break;
         case TKS_IDNUMERIC0+11:
            keyPadByte = CR;
            break;
         case TKS_IDNUMERIC0+12:
            keyPadByte = ' ';
            break;
         case TKS_IDNUMERIC0+13:
            keyPadByte = ',';
            break;
         case TKS_IDNUMERIC0+14:
            keyPadByte = '.';
            break;
         case TKS_IDNUMERIC0+15:
            keyPadByte = ' ';
            break;
         default:
            keyPadByte = (seqTableNdx - TKS_IDNUMERIC0) + '0';
            break;
         }

         keyPadString[*keyPadString = 1] = keyPadByte;
      }
   }

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* sendKeyInput() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL sendKeyInput(BYTE  theByte)
{

   switch(kbdLock)
   {
   case KBD_ECHO:
      modemInp(theByte, FALSE);              /* mbbx 1.10 */
      break;

   default:
      modemWr(theByte);
      break;
   }

   return(TRUE);
}
