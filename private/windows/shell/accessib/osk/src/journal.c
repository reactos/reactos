// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Last updated Maria Jose and Anil Kumar
// 
#include <windows.h>
#include "kbus.h"
#include "kbfunc.h"
#include "ms32dll.h"
#include "Msswch.h"     //v-mjgran: use the dll API's
#include "door.h"


//Global vars 
extern HWND       ShiftHwnd;
extern BOOL       RALT;
extern BOOL       Shift_Dead;
extern HHOOK      hkJRec = NULL;       // HHOOK to the Journal Record
extern HWND       kbmainhwnd;
extern int        lenKBkey;
extern HWND       *lpkeyhwnd;
extern BOOL       kbfCapLetter;
extern BOOL       kbfCapLock;


extern BOOL Prefhilitekey;
extern BOOL PrefDwellinkey;
extern HWND DWin;
extern HWND Dwellwindow;

extern BOOL g_fDoubleHookMsg;
extern HWND g_hLastKey;

extern BOOL g_fRealChar;   // v-mjgran: The scan key is a character to be sent
extern HWND g_hBitmapLockHwnd;

BOOL g_fDrawShift;         //Shift has been used, consider it when redrawing keyboard
BOOL g_fDrawCapital;       //CapLock has been used, consider it when redrawing keyboard


//extern DWORD		g_dwConversion=0;
/*****************************************************************************/
LRESULT CALLBACK JournalRecordProc(int  nCode, WPARAM  wParam, LPARAM  lParam)
{
   UINT sc = 0;

   static fFirstShift = TRUE;		//Keydown event only will be managed first time
   static fFirstCapital = TRUE;	//Keydown event only will be managed first time

   LPEVENTMSG  lpEvent;
   UINT vk;
   static BOOL bKeyDown= FALSE;
   static HWND	lastKey=NULL;

   static BOOL fCharWithDead = 0;

//	static BOOL fSendDown = FALSE; 
//	static BOOL fSendUp = FALSE;

   static HWND hlastwnd=NULL;

   register int i;


   lpEvent = (LPEVENTMSG) lParam;
   if (lpEvent->message == WM_KEYDOWN || lpEvent->message == WM_KEYUP	||
      lpEvent->message == WM_SYSKEYDOWN || lpEvent->message == WM_SYSKEYUP)
   {
      // Add the WM_SYSKEYDOWN and WM_SYSKEYUP messages because when ALT is pressed, there are
      // the messages that are sent (WM_KEYDOWN and WM_KEYUP are not sent when ALT is pressed)
      vk=LOBYTE(lpEvent->paramL);
   }
   else
      vk = 0;


   if(nCode < 0 || Shift_Dead)
   {
      if (!(Shift_Dead && g_fRealChar))
         swchAvoidScanChar(0);


      if (Shift_Dead)
      {
         if (!fCharWithDead)
            fCharWithDead =1;
         else
         {
            if (vk != VK_SHIFT && vk != VK_CONTROL && vk != VK_CAPITAL)
            {
               Shift_Dead = FALSE;
               fCharWithDead = 0;
            }
         }
      }
      return (CallNextHookEx(hkJRec, nCode, wParam, lParam));
   }


   if(nCode==HC_ACTION)
   {  
        //Fix the hilite key stays paint when leave keyboard
      if(lpEvent->message == WM_MOUSEMOVE)
      {
         //static HWND hlastwnd=NULL;
         HWND htempwnd;
         POINT pt;
         BOOL YesItIs;

         GetCursorPos(&pt);                		// check if it is a dwelling window
         htempwnd = WindowFromPoint(pt);
         YesItIs = IsOneOfOurKey(htempwnd);


         if(htempwnd != kbmainhwnd && IsChild(kbmainhwnd, htempwnd)==0 && hlastwnd != NULL)
         {
            ReturnColors(hlastwnd, TRUE);
            hlastwnd = NULL; 
         }

         if(IsChild(kbmainhwnd, htempwnd) && lastKey != htempwnd)
         {
            if (hlastwnd != htempwnd)
            {
               if((hlastwnd != kbmainhwnd) && (hlastwnd != NULL))
               {
                  if(Prefhilitekey || PrefDwellinkey)
                     ReturnColors(hlastwnd, TRUE);   //else return with lastMwnd
               }

               if(htempwnd != kbmainhwnd)    //for dwell clicking
               {
                  if(YesItIs)    //make sure the current window is Key or Predict Key
                  {
                     DWin=Dwellwindow = htempwnd;    // Set the Dwell window

                     if(Prefhilitekey)
                     {
                        InvertColors(htempwnd);
                     }
                     if(PrefDwellinkey)
                     {
                        killtime();
                        SetTimeControl(htempwnd);
                     }
                  }

                  lastKey = htempwnd;
                  g_hLastKey = lastKey;

                  //v-mjgran: Avoid manage other time the same event in the mouse hook
                  g_fDoubleHookMsg = TRUE;
               }

               hlastwnd = htempwnd;
            }
         }
         else
         {
            lastKey = NULL;
         }
      }
      
      
      else if ((GetFocus() == kbmainhwnd) || (GetActiveWindow() == kbmainhwnd))
      {
         //OSK has the focus and the user type or use scan mode
         swchAvoidScanChar(0);      //if in the OSK window, filter the scan codes
//       return CallNextHookEx(hkJRec, nCode, wParam, lParam);	//do the normal management
      }


        // Key Down
      if(lpEvent->message == WM_KEYDOWN)
      {
         switch(vk)
         {
            case VK_F11:
               if(IsIconic(kbmainhwnd))      //bring it up if it is minimize
                  ShowWindow(kbmainhwnd, SW_RESTORE);
               else
                  ShowWindow(kbmainhwnd, SW_SHOWMINIMIZED);
            break;

            case VK_MENU:
   /*
				   if(!RALT)
				   {	//Hilite both Alt keys
					   for(i=1; i<=lenKBkey; i++)
                       {
                           if(KBkey[i].name == KB_LALT || KBkey[i].name == KB_RALT)
                           {
                               SetWindowLong(lpkeyhwnd[i], 0, 4);
                               DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], 
                                            GCLP_HBRBACKGROUND, 
                                            (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
							   InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
                           }
                       }

					   RALT = TRUE;
                    
				   }
   */
   //				RedrawKeys(); 
            break;
         
            case VK_SHIFT:
         
               if (!fFirstShift)
               {
                  // KeyDown is repeated because is the key continues pressed
                  break;
               }
               fFirstShift = FALSE;
            
               g_fDrawShift = !g_fDrawShift;	//Updates shift drawn flag
            
               if ((!kbfCapLetter || kbfCapLock) && !bKeyDown)
               {
   /*
					   //Hilite both shift keys
					   for(i=1; i<=lenKBkey; i++)
                       {
						   if(KBkey[i].name == KB_LSHIFT && GetKeyState(VK_LSHIFT) & 0x80)  
          //                     (KBkey[i].name == KB_RSHIFT && GetKeyState(VK_RSHIFT) == TRUE))
						   {	
                               SetWindowLong(lpkeyhwnd[i], 0, 4);
							   DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], 
                                            GCLP_HBRBACKGROUND, 
                                            (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
						   }
                       }
   */

                  kbfCapLetter = !kbfCapLetter;

                  RedrawKeys();
                  bKeyDown = TRUE;
               }
            break;
         
            case VK_CONTROL:

   /*				
				   //Hilite both ctrl keys
				   for(i=1; i<=lenKBkey; i++)
                   {
					   if(KBkey[i].name == KB_LCTR && GetKeyState(VK_LCONTROL) & 0x80)
					   {	SetWindowLong(lpkeyhwnd[i], 0, 4);
						   DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], 
                                        GCLP_HBRBACKGROUND, 
                                        (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
						   InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
					   }
                   }
   */
               RedrawKeys();
            break;
         
            default:         //Light up the key when user press from the normal KB
            {
               DWORD    dwProcessId;
               DWORD    dwlayout;
               HKL      hkl;
   //			UINT	sc;

               if(vk== VK_NUMLOCK)
                  break;

               switch (vk)
               {

                  case VK_DECIMAL:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '.' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD0:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '0' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD1:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '1' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD2:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '2' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD3:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '3' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
                        
                           break;
                        }
                  break;

                  case VK_NUMPAD4:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '4' && KBkey[i].smallKb == LARGE)
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD5:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '5' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD6:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '6' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD7:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '7' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD8:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '8' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;

                  case VK_NUMPAD9:
                     for(i=1; i < lenKBkey; i++)
                        if(*KBkey[i].textL == '9' && KBkey[i].smallKb == LARGE) 	
                        {  SetWindowLong(lpkeyhwnd[i], 0, 4);
                           DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                              (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                           InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                           break;
                        }
                  break;


                  default:
                  {
                     //Get the active window's thread
                     dwlayout=GetWindowThreadProcessId(GetActiveWindow(), &dwProcessId);
                     //Get the active window's keyboard layout
                     hkl=GetKeyboardLayout(dwlayout);

                     sc = MapVirtualKeyEx(vk, 0, hkl);

                     if(sc == 0)
                        break;

                     if (!IsDeadKey(vk, sc))
                     {
                        for (i=1; i <= lenKBkey; i++)
                           if(sc == KBkey[i].scancode[0] || sc == KBkey[i].scancode[1])
                           {
                              SetWindowLong(lpkeyhwnd[i], 0, 4);
                              DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
                                 (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                              InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

                              break;
                           }
                     }
                  }   //end default
               }  //end switch (vk)

            }  //end default
            break;

         }

      }

      else if(lpEvent->message == WM_KEYUP)
      {
/*			if (fSendDown)
			{
				//The key state is up when scan is the active mode
				fSendUp = TRUE;
				fSendDown = FALSE;
			}
*/
            //Key Up
            switch(vk)
				{
                			
			//In Japanese KB, detect the IME mode (KANA mode) and redraw the keyboard
			case VK_KANA:
			{	
/*
				HIMC hImc;
				DWORD dwConversion, dwSentence;

				hImc = ImmGetContext(GetActiveWindow());
				ImmGetConversionStatus(hImc, &dwConversion, &dwSentence);
				g_dwConversion = dwConversion;
*/
				RedrawKeys();
			}
			break;
			
			case VK_CAPITAL:

				g_fDrawCapital = !g_fDrawCapital;	//Updates CapLock drawn flag

				//Check to see the CapsLock on the keyboard is On or Off
				if((LOBYTE(GetKeyState(VK_CAPITAL)) & 0x01))   //On
				{	
					kbfCapLetter = TRUE; 
					kbfCapLock = TRUE;


					//Hilite Cap key
					for(i=1; i<=lenKBkey; i++)
               {
   					//if(KBkey[i].name == KB_CAPLOCK)
						if(KBkey[i].scancode[0] == 0x3a)    //Use scancode (name is not valid for japanese Kb)

						{
                     SetWindowLong(lpkeyhwnd[i], 0, 4);
							DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], 
                                  GCLP_HBRBACKGROUND, 
                                  (LONG_PTR)CreateSolidBrush(RGB(0,0,0))));	
                     
                        if (KBkey[i].name == BITMAP)     //Updates japanese CapLock
                           g_hBitmapLockHwnd = lpkeyhwnd[i];

                        break;

						}
               }
				}
				else                           //off
				{
					kbfCapLetter = FALSE; 
					kbfCapLock = FALSE;


					//restore Cap key
					for(i=1; i<=lenKBkey; i++)
               {
						//if(KBkey[i].name == KB_CAPLOCK)
						if(KBkey[i].scancode[0] == 0x3a)	//use scancode (name is not valid for japanese Kb)
						{
							SetWindowLong(lpkeyhwnd[i], 0, 0);
							SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, 
											COLOR_MENU);

							if (KBkey[i].name == BITMAP)     //Updates japanese CapLock
								g_hBitmapLockHwnd = NULL;

							break;
						}
               }

				}

                RedrawKeys();
			break;

			case VK_SHIFT:

				fFirstShift = TRUE;				//Shift key has been released
				g_fDrawShift = !g_fDrawShift;	//Updates shift drawn flag

				if(kbfCapLetter && !kbfCapLock)
				{
					kbfCapLetter = FALSE;
					bKeyDown = FALSE;
                    //RedrawKeys();
				}

				else if(bKeyDown)
				{	kbfCapLetter = !kbfCapLetter;
                    //RedrawKeys();
					bKeyDown = FALSE;
				}
				
				RedrawKeys();	//When Shift is released, redraw always the keyboard

			break;

			case VK_MENU:
				RedrawKeys();   
			break;

			case VK_CONTROL:

				RedrawKeys();
			break;


			case VK_NUMLOCK:
				RedrawNumLock();
			break;

			case VK_SCROLL:
				RedrawScrollLock();
			break;


            default:       //Light up the key when user press from the normal KB
				{
					DWORD	dwProcessId;
					DWORD	dwlayout;
					HKL		hkl;

					//Don't light up this key
					if(vk == VK_CAPITAL || vk== VK_NUMLOCK)
						break;

					switch (vk)
					{
						case VK_DECIMAL:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '.' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;
					
						case VK_NUMPAD0:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '0' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD1:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '1' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD2:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '2' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD3:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '3' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD4:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '4' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD5:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '5' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;
						
						case VK_NUMPAD6:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '6' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD7:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '7' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD8:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '8' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));		
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;

						case VK_NUMPAD9:
							for(i=1; i < lenKBkey; i++)
								if(*KBkey[i].textL == '9' && KBkey[i].smallKb == LARGE) 	
								{	SetWindowLong(lpkeyhwnd[i], 0, 0);
									DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
									InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
									
									break;
								}
						break;


						default:
							//Get the active window's thread
							dwlayout=GetWindowThreadProcessId(GetActiveWindow(), &dwProcessId);
							//Get the active window's keyboard layout
							hkl=GetKeyboardLayout(dwlayout);

							sc = MapVirtualKeyEx(vk, 0, hkl);

							if(sc == 0)
								break;

							if (!IsDeadKey(vk, sc))
							{
								for (i=1; i <= lenKBkey; i++)
									if(sc == KBkey[i].scancode[0] || sc == KBkey[i].scancode[1])
									{
										SetWindowLong(lpkeyhwnd[i], 0, 0);
										DeleteObject((HGDIOBJ)SetClassLongPtr(lpkeyhwnd[i], GCLP_HBRBACKGROUND, COLOR_WINDOW));	
										InvalidateRect(lpkeyhwnd[i], NULL, TRUE);

										break;
									}
							}
						break;
					}   // end switch (vk)
				}
			break;

				}
		}
	}

	if (!kbPref->bKBKey || vk != kbPref->uKBKey)
	{
		//v-mjgran: Reset the state varaibles and allow to send the key
		g_fRealChar = FALSE;
//		fSendDown = FALSE;
//		fSendUp = FALSE;
		swchAvoidScanChar(1);

		if (IsDeadKey(vk, sc))
			Shift_Dead = TRUE;
		else
			Shift_Dead = FALSE;


	}
	else
	{
		if (g_fRealChar)
		{
/*			if (fSendUp)
			{
				// v-mjgran: The cycle is over. Reset the state variables
				fSendUp = FALSE;
				g_fRealChar = FALSE;
			}
*/
    	if (vk == VK_MENU)
		{
			// v-mjgran: After menu it is necessary to filter next scan codes before the real key
			swchAvoidScanChar(0);
		}
		else
			
			swchAvoidScanChar(1);


			if (IsDeadKey(vk, sc))
				Shift_Dead = TRUE;
			else
				Shift_Dead = FALSE;
		}
		else
		{
			// Avoid send the key in the keyboard proc, this character is to activate the scan mode,
			// it musn't be written
			swchAvoidScanChar(0);
		}
	}

	g_fRealChar = FALSE;
	return (CallNextHookEx(hkJRec, nCode, wParam, lParam));
}
/*****************************************************************************/

