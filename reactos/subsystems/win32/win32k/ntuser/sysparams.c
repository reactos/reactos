/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          System parameters functions
 * FILE:             subsystem/win32/win32k/ntuser/sysparam.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2008/03/20  Split from misc.c
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


VOID FASTCALL
IntGetFontMetricSetting(LPWSTR lpValueName, PLOGFONTW font)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   NTSTATUS Status;
   /* Firefox 1.0.7 depends on the lfHeight value being negative */
   static LOGFONTW DefaultFont = {
                                    -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                    0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                    L"MS Sans Serif"
                                 };

   RtlZeroMemory(&QueryTable, sizeof(QueryTable));

   QueryTable[0].Name = lpValueName;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].EntryContext = font;

   Status = RtlQueryRegistryValues(
               RTL_REGISTRY_USER,
               L"Control Panel\\Desktop\\WindowMetrics",
               QueryTable,
               NULL,
               NULL);

   if (!NT_SUCCESS(Status))
   {
      RtlCopyMemory(font, &DefaultFont, sizeof(LOGFONTW));
   }
}

VOID
IntWriteSystemParametersSettings(PUNICODE_STRING SubKeyName, PUNICODE_STRING KeyName, ULONG Type, PVOID Data, ULONG DataSize)
{
    UNICODE_STRING KeyPath;
    NTSTATUS Status;
    HANDLE CurrentUserKey, KeyHandle;
    OBJECT_ATTRIBUTES KeyAttributes, ObjectAttributes;

    /* Get a handle to the current users settings */
    Status = RtlFormatCurrentUserKeyPath(&KeyPath);
    if(!NT_SUCCESS(Status))
        return;

    InitializeObjectAttributes(&ObjectAttributes, &KeyPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    /* Open the HKCU key */
    Status = ZwOpenKey(&CurrentUserKey, KEY_WRITE, &ObjectAttributes);
    RtlFreeUnicodeString(&KeyPath);
    if(!NT_SUCCESS(Status))
        return;


    /* Open up the settings to read the values */
    InitializeObjectAttributes(&KeyAttributes, SubKeyName, OBJ_CASE_INSENSITIVE, CurrentUserKey, NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_WRITE, &KeyAttributes);
    ZwClose(CurrentUserKey);

    if (NT_SUCCESS(Status))
    {
        ZwSetValueKey(KeyHandle, KeyName, 0, Type, Data, DataSize);
        ZwClose(KeyHandle);
    }
}



ULONG FASTCALL
IntSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   BOOL bChanged = FALSE;

   static BOOL bInitialized = FALSE;
   static LOGFONTW IconFont;
   static NONCLIENTMETRICSW pMetrics;
   static MINIMIZEDMETRICS MinimizedMetrics;
   static BOOL GradientCaptions = TRUE;
   static UINT FocusBorderHeight = 1;
   static UINT FocusBorderWidth = 1;
   static ANIMATIONINFO anim;
   static STICKYKEYS StickyKeys = {sizeof(STICKYKEYS), 0x1fa};
   static FILTERKEYS FilterKeys = {sizeof(FILTERKEYS), 0, 0, 0, 0, 0};
   static TOGGLEKEYS ToggleKeys = {sizeof(TOGGLEKEYS), 0};
   static MOUSEKEYS MouseKeys = {sizeof(MOUSEKEYS), 0, 0, 0, 0, 0, 0};
   static BOOL KeyboardPref = FALSE;
   static BOOL ShowSounds = FALSE;
   static ACCESSTIMEOUT AccessTimeout = {sizeof(ACCESSTIMEOUT), 0, 0};
   static SERIALKEYS SerialKeys = {sizeof(SERIALKEYS), 0, 0, 0, 0, 0, 0};

   if (!bInitialized)
   {
      RtlZeroMemory(&IconFont, sizeof(LOGFONTW));
      RtlZeroMemory(&pMetrics, sizeof(NONCLIENTMETRICSW));

      IntGetFontMetricSetting(L"CaptionFont", &pMetrics.lfCaptionFont);
      IntGetFontMetricSetting(L"SmCaptionFont", &pMetrics.lfSmCaptionFont);
      IntGetFontMetricSetting(L"MenuFont", &pMetrics.lfMenuFont);
      IntGetFontMetricSetting(L"StatusFont", &pMetrics.lfStatusFont);
      IntGetFontMetricSetting(L"MessageFont", &pMetrics.lfMessageFont);
      IntGetFontMetricSetting(L"IconFont", &IconFont);

      pMetrics.iBorderWidth = 1;
      pMetrics.iScrollWidth = UserGetSystemMetrics(SM_CXVSCROLL);
      pMetrics.iScrollHeight = UserGetSystemMetrics(SM_CYHSCROLL);
      pMetrics.iCaptionWidth = UserGetSystemMetrics(SM_CXSIZE);
      pMetrics.iCaptionHeight = UserGetSystemMetrics(SM_CYSIZE);
      pMetrics.iSmCaptionWidth = UserGetSystemMetrics(SM_CXSMSIZE);
      pMetrics.iSmCaptionHeight = UserGetSystemMetrics(SM_CYSMSIZE);
      pMetrics.iMenuWidth = UserGetSystemMetrics(SM_CXMENUSIZE);
      pMetrics.iMenuHeight = UserGetSystemMetrics(SM_CYMENUSIZE);
      pMetrics.cbSize = sizeof(NONCLIENTMETRICSW);

      MinimizedMetrics.cbSize = sizeof(MINIMIZEDMETRICS);
      MinimizedMetrics.iWidth = UserGetSystemMetrics(SM_CXMINIMIZED);
      MinimizedMetrics.iHorzGap = UserGetSystemMetrics(SM_CXMINSPACING);
      MinimizedMetrics.iVertGap = UserGetSystemMetrics(SM_CYMINSPACING);
      MinimizedMetrics.iArrange = ARW_HIDE;

      bInitialized = TRUE;
   }

   switch(uiAction)
   {
     case SPI_GETDRAGFULLWINDOWS:
           /* FIXME: Implement this, don't just return constant */
           *(PBOOL)pvParam = FALSE;
           break;



      case SPI_GETKEYBOARDCUES:
           /* FIXME: Implement this, don't just return constant */
           *(PBOOL)pvParam = FALSE;
           break;

      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETDESKWALLPAPER:
      case SPI_SETSCREENSAVERRUNNING:
      case SPI_SETSCREENSAVETIMEOUT:
      case SPI_SETFLATMENU:
      case SPI_SETMOUSEHOVERTIME:
      case SPI_SETMOUSEHOVERWIDTH:
      case SPI_SETMOUSEHOVERHEIGHT:
      case SPI_SETMOUSE:
      case SPI_SETMOUSESPEED:
      case SPI_SETMOUSEBUTTONSWAP:
         /* We will change something, so set the flag here */
         bChanged = TRUE;
      case SPI_GETDESKWALLPAPER:
      case SPI_GETWHEELSCROLLLINES:
      case SPI_GETWHEELSCROLLCHARS:
      case SPI_GETSCREENSAVERRUNNING:
      case SPI_GETSCREENSAVETIMEOUT:
      case SPI_GETSCREENSAVEACTIVE:
      case SPI_GETFLATMENU:
      case SPI_GETMOUSEHOVERTIME:
      case SPI_GETMOUSEHOVERWIDTH:
      case SPI_GETMOUSEHOVERHEIGHT:
      case SPI_GETMOUSE:
      case SPI_GETMOUSESPEED:
         {
            PSYSTEM_CURSORINFO CurInfo;

            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinStaObject);
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return (DWORD)FALSE;
            }

            switch(uiAction)
            {
               case SPI_GETKEYBOARDCUES:
                  ASSERT(pvParam);
                  *((BOOL*)pvParam) = TRUE;
               case SPI_GETFLATMENU:
                  ASSERT(pvParam);
                  *((UINT*)pvParam) = WinStaObject->FlatMenu;
                  break;
               case SPI_SETFLATMENU:
                  WinStaObject->FlatMenu = (BOOL)pvParam;
                  break;
               case SPI_GETSCREENSAVETIMEOUT:
                   ASSERT(pvParam);
                   *((UINT*)pvParam) = WinStaObject->ScreenSaverTimeOut;
               break;
               case SPI_SETSCREENSAVETIMEOUT:
                  WinStaObject->ScreenSaverTimeOut = uiParam;
                  break;
               case SPI_GETSCREENSAVERRUNNING:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverRunning;
                  break;
               case SPI_SETSCREENSAVERRUNNING:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverRunning;
                  WinStaObject->ScreenSaverRunning = uiParam;
                  break;
               case SPI_SETSCREENSAVEACTIVE:
                  WinStaObject->ScreenSaverActive = uiParam;
                  break;
               case SPI_GETSCREENSAVEACTIVE:
                  if (pvParam != NULL) *((BOOL*)pvParam) = WinStaObject->ScreenSaverActive;
                  break;
               case SPI_GETWHEELSCROLLLINES:
                  ASSERT(pvParam);
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  *((UINT*)pvParam) = CurInfo->WheelScroLines;
                  /* FIXME add this value to scroll list as scroll value ?? */
                  break;
               case SPI_GETWHEELSCROLLCHARS:
                  ASSERT(pvParam);
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  *((UINT*)pvParam) = CurInfo->WheelScroChars;
                  // FIXME add this value to scroll list as scroll value ??
                  break;
               case SPI_SETDOUBLECLKWIDTH:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickWidth = uiParam;
                  break;
               case SPI_SETDOUBLECLKHEIGHT:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickHeight = uiParam;
                  break;
               case SPI_SETDOUBLECLICKTIME:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum time to 1000 ms? */
                  CurInfo->DblClickSpeed = uiParam;
                  break;
               case SPI_GETMOUSEHOVERTIME:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *((UINT*)pvParam) = CurInfo->MouseHoverTime;
                   break;
               case SPI_SETMOUSEHOVERTIME:
                   /* see http://msdn2.microsoft.com/en-us/library/ms724947.aspx
                    * copy text from it, if some agument why xp and 2003 behovir diffent
                    * only if they do not have SP install
                    * " Windows Server 2003 and Windows XP: The operating system does not
                    *   enforce the use of USER_TIMER_MAXIMUM and USER_TIMER_MINIMUM until
                    *   Windows Server 2003 SP1 and Windows XP SP2 "
                    */
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  CurInfo->MouseHoverTime = uiParam;
                  if(CurInfo->MouseHoverTime < USER_TIMER_MINIMUM)
                  {
                      CurInfo->MouseHoverTime = USER_TIMER_MINIMUM;
                  }
                  if(CurInfo->MouseHoverTime > USER_TIMER_MAXIMUM)
                  {
                      CurInfo->MouseHoverTime = USER_TIMER_MAXIMUM;
                  }

                  break;
               case SPI_GETMOUSEHOVERWIDTH:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseHoverWidth;
                   break;
               case SPI_GETMOUSEHOVERHEIGHT:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseHoverHeight;
                   break;
               case SPI_SETMOUSEHOVERWIDTH:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  CurInfo->MouseHoverWidth = uiParam;
                  break;
               case SPI_SETMOUSEHOVERHEIGHT:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->MouseHoverHeight = uiParam;
                   break;
               case SPI_SETMOUSEBUTTONSWAP:
                   {
                      UNICODE_STRING SubKeyName = RTL_CONSTANT_STRING(L"Control Panel\\Mouse");
                      UNICODE_STRING SwapMouseButtons = RTL_CONSTANT_STRING(L"SwapMouseButtons");
                      WCHAR szBuffer[10];

                      swprintf(szBuffer, L"%u", uiParam);
                      CurInfo = IntGetSysCursorInfo(WinStaObject);
                      CurInfo->SwapButtons = uiParam;
                      IntWriteSystemParametersSettings(&SubKeyName, &SwapMouseButtons, REG_SZ, szBuffer, (wcslen(szBuffer)+1) * sizeof(WCHAR));
                      break;
                   }
               case SPI_SETMOUSE:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->CursorAccelerationInfo = *(PCURSORACCELERATION_INFO)pvParam;
                   break;
               case SPI_GETMOUSE:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PCURSORACCELERATION_INFO)pvParam = CurInfo->CursorAccelerationInfo;
                   break;
               case SPI_SETMOUSESPEED:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   CurInfo->MouseSpeed = (UINT)pvParam;
                   /* Limit value to 1...20 range */
                   if(CurInfo->MouseSpeed < 1)
                   {
                       CurInfo->MouseSpeed = 1;
                   }
                   else if(CurInfo->MouseSpeed > 20)
                   {
                       CurInfo->MouseSpeed = 20;
                   }
                   break;
               case SPI_GETMOUSESPEED:
                   CurInfo = IntGetSysCursorInfo(WinStaObject);
                   *(PUINT)pvParam = CurInfo->MouseSpeed;
                   break;
               case SPI_SETDESKWALLPAPER:
                  {
                     /* This function expects different parameters than the user mode version!

                        We let the user mode code load the bitmap, it passed the handle to
                        the bitmap. We'll change it's ownership to system and replace it with
                        the current wallpaper bitmap */
                     HBITMAP hOldBitmap, hNewBitmap;
                     UNICODE_STRING Key = RTL_CONSTANT_STRING(L"Control Panel\\Desktop");
                     UNICODE_STRING Tile = RTL_CONSTANT_STRING(L"TileWallpaper");
                     UNICODE_STRING Style = RTL_CONSTANT_STRING(L"WallpaperStyle");
                     UNICODE_STRING KeyPath;
                     OBJECT_ATTRIBUTES KeyAttributes;
                     OBJECT_ATTRIBUTES ObjectAttributes;
                     NTSTATUS Status;
                     HANDLE CurrentUserKey = NULL;
                     HANDLE KeyHandle = NULL;
                     PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
                     ULONG Length = 0;
                     ULONG ResLength = 0;
                     ULONG TileNum = 0;
                     ULONG StyleNum = 0;
                     ASSERT(pvParam);

                     hNewBitmap = *(HBITMAP*)pvParam;
                     if(hNewBitmap != NULL)
                     {
                        BITMAPOBJ *bmp;
                        /* try to get the size of the wallpaper */
                        if(!(bmp = BITMAPOBJ_LockBitmap(hNewBitmap)))
                        {
                           ObDereferenceObject(WinStaObject);
                           return FALSE;
                        }
                        WinStaObject->cxWallpaper = bmp->SurfObj.sizlBitmap.cx;
                        WinStaObject->cyWallpaper = bmp->SurfObj.sizlBitmap.cy;

                        BITMAPOBJ_UnlockBitmap(bmp);

                        /* change the bitmap's ownership */
                        GDIOBJ_SetOwnership(hNewBitmap, NULL);
                     }
                     hOldBitmap = (HBITMAP)InterlockedExchange((LONG*)&WinStaObject->hbmWallpaper, (LONG)hNewBitmap);
                     if(hOldBitmap != NULL)
                     {
                        /* delete the old wallpaper */
                        NtGdiDeleteObject(hOldBitmap);
                     }

                     /* Set the style */
                     /*default value is center */
                     WinStaObject->WallpaperMode = wmCenter;

                     /* Get a handle to the current users settings */
                     RtlFormatCurrentUserKeyPath(&KeyPath);
                     InitializeObjectAttributes(&ObjectAttributes,&KeyPath,OBJ_CASE_INSENSITIVE,NULL,NULL);
                     ZwOpenKey(&CurrentUserKey, KEY_READ, &ObjectAttributes);
                     RtlFreeUnicodeString(&KeyPath);

                     /* open up the settings to read the values */
                     InitializeObjectAttributes(&KeyAttributes, &Key, OBJ_CASE_INSENSITIVE,
                              CurrentUserKey, NULL);
                     ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
                     ZwClose(CurrentUserKey);

                     /* read the tile value in the registry */
                     Status = ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                              0, 0, &ResLength);

                     /* fall back to .DEFAULT if we didnt find values */
                     if(Status == STATUS_INVALID_HANDLE)
                     {
                        RtlInitUnicodeString (&KeyPath,L"\\Registry\\User\\.Default\\Control Panel\\Desktop");
                        InitializeObjectAttributes(&KeyAttributes, &KeyPath, OBJ_CASE_INSENSITIVE,
                                                   NULL, NULL);
                        ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
                        ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                        0, 0, &ResLength);
                     }

                     ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
                     KeyValuePartialInfo = ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
                     Length = ResLength;

                     if(!KeyValuePartialInfo)
                     {
                        NtClose(KeyHandle);
                        return FALSE;
                     }

                     Status = ZwQueryValueKey(KeyHandle, &Tile, KeyValuePartialInformation,
                                              (PVOID)KeyValuePartialInfo, Length, &ResLength);
                     if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
                     {
                        ZwClose(KeyHandle);
                        ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);
                        return FALSE;
                     }

                     Tile.Length = KeyValuePartialInfo->DataLength;
                     Tile.MaximumLength = KeyValuePartialInfo->DataLength;
                     Tile.Buffer = (PWSTR)KeyValuePartialInfo->Data;

                     Status = RtlUnicodeStringToInteger(&Tile, 0, &TileNum);
                     if(!NT_SUCCESS(Status))
                     {
                        TileNum = 0;
                     }
                     ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);

                     /* start over again and look for the style*/
                     ResLength = 0;
                     Status = ZwQueryValueKey(KeyHandle, &Style, KeyValuePartialInformation,
                                              0, 0, &ResLength);

                     ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
                     KeyValuePartialInfo = ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
                     Length = ResLength;

                     if(!KeyValuePartialInfo)
                     {
                        ZwClose(KeyHandle);
                        return FALSE;
                     }

                     Status = ZwQueryValueKey(KeyHandle, &Style, KeyValuePartialInformation,
                                              (PVOID)KeyValuePartialInfo, Length, &ResLength);
                     if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
                     {
                        ZwClose(KeyHandle);
                        ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);
                        return FALSE;
                     }

                     Style.Length = KeyValuePartialInfo->DataLength;
                     Style.MaximumLength = KeyValuePartialInfo->DataLength;
                     Style.Buffer = (PWSTR)KeyValuePartialInfo->Data;

                     Status = RtlUnicodeStringToInteger(&Style, 0, &StyleNum);
                     if(!NT_SUCCESS(Status))
                     {
                        StyleNum = 0;
                     }
                     ExFreePoolWithTag(KeyValuePartialInfo, TAG_STRING);

                     /* Check the values we found in the registry */
                     if(TileNum && !StyleNum)
                     {
                        WinStaObject->WallpaperMode = wmTile;
                     }
                     else if(!TileNum && StyleNum == 2)
                     {
                        WinStaObject->WallpaperMode = wmStretch;
                     }

                     ZwClose(KeyHandle);
                     break;
                  }
               case SPI_GETDESKWALLPAPER:
                  /* This function expects different parameters than the user mode version!
                     We basically return the current wallpaper handle - if any. The user
                     mode version should load the string from the registry and return it
                     without calling this function */

                  ASSERT(pvParam);
                  *(HBITMAP*)pvParam = (HBITMAP)WinStaObject->hbmWallpaper;
                  break;
            }

            /* FIXME save the value to the registry */

            ObDereferenceObject(WinStaObject);
            break;
         }
      case SPI_SETWORKAREA:
         {
            RECT *rc;
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            PDESKTOP Desktop = pti->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            rc = (RECT*)pvParam;
            Desktop->WorkArea = *rc;
            bChanged = TRUE;

            break;
         }
      case SPI_GETWORKAREA:
         {
            PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
            PDESKTOP Desktop = pti->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            IntGetDesktopWorkArea(Desktop, (PRECT)pvParam);

            break;
         }
      case SPI_SETGRADIENTCAPTIONS:
         {
            GradientCaptions = (pvParam != NULL);
            /* FIXME - should be checked if the color depth is higher than 8bpp? */
            bChanged = TRUE;
            break;
         }
      case SPI_GETGRADIENTCAPTIONS:
         {
            HDC hDC;
            BOOL Ret = GradientCaptions;

            hDC = IntGetScreenDC();
            if(!hDC)
            {
               return FALSE;
            }
            Ret = (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8) && Ret;

            ASSERT(pvParam);
            *((PBOOL)pvParam) = Ret;
            break;
         }
      case SPI_SETFONTSMOOTHING:
         {
            IntEnableFontRendering(uiParam != 0);
            bChanged = TRUE;
            break;
         }
      case SPI_GETFONTSMOOTHING:
         {
            ASSERT(pvParam);
            *((BOOL*)pvParam) = IntIsFontRenderingEnabled();
            break;
         }
      case SPI_GETICONTITLELOGFONT:
         {
            ASSERT(pvParam);
            *((LOGFONTW*)pvParam) = IconFont;
            break;
         }
      case SPI_GETNONCLIENTMETRICS:
         {
            ASSERT(pvParam);
            *((NONCLIENTMETRICSW*)pvParam) = pMetrics;
            break;
         }
      case SPI_GETANIMATION:
         {
            ASSERT(pvParam);
            *(( ANIMATIONINFO*)pvParam) = anim;
            break;
         }
      case SPI_SETANIMATION:
         {
            ASSERT(pvParam);
            anim = *((ANIMATIONINFO*)pvParam);
            bChanged = TRUE;
         }
      case SPI_SETNONCLIENTMETRICS:
         {
            ASSERT(pvParam);
            pMetrics = *((NONCLIENTMETRICSW*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETMINIMIZEDMETRICS:
         {
            ASSERT(pvParam);
            *((MINIMIZEDMETRICS*)pvParam) = MinimizedMetrics;
            break;
         }
      case SPI_SETMINIMIZEDMETRICS:
         {
            ASSERT(pvParam);
            MinimizedMetrics = *((MINIMIZEDMETRICS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETFOCUSBORDERHEIGHT:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderHeight;
            break;
         }
      case SPI_GETFOCUSBORDERWIDTH:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderWidth;
            break;
         }
      case SPI_SETFOCUSBORDERHEIGHT:
         {
            FocusBorderHeight = (UINT)pvParam;
            bChanged = TRUE;
            break;
         }
      case SPI_SETFOCUSBORDERWIDTH:
         {
            FocusBorderWidth = (UINT)pvParam;
            bChanged = TRUE;
            break;
         }
      case SPI_GETSTICKYKEYS:
         {
            *((STICKYKEYS*)pvParam) = StickyKeys;
            break;
         }
      case SPI_SETSTICKYKEYS:
         {
            StickyKeys = *((STICKYKEYS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETFILTERKEYS:
         {
            *((FILTERKEYS*)pvParam) = FilterKeys;
            break;
         }
      case SPI_SETFILTERKEYS:
         {
            FilterKeys = *((FILTERKEYS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETTOGGLEKEYS:
         {
            *((TOGGLEKEYS*)pvParam) = ToggleKeys;
            break;
         }
      case SPI_SETTOGGLEKEYS:
         {
            ToggleKeys = *((TOGGLEKEYS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETSERIALKEYS:
         {
            *((SERIALKEYS*)pvParam) = SerialKeys;
            break;
         }
      case SPI_SETSERIALKEYS:
         {
            SerialKeys = *((SERIALKEYS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETMOUSEKEYS:
         {
            *((MOUSEKEYS*)pvParam) = MouseKeys;
            break;
         }
      case SPI_SETMOUSEKEYS:
         {
            MouseKeys = *((MOUSEKEYS*)pvParam);
            bChanged = TRUE;
            break;
         }
      case SPI_GETKEYBOARDPREF:
         {
            *((BOOL*)pvParam) = KeyboardPref;
            break;
         }
      case SPI_SETKEYBOARDPREF:
         {
            KeyboardPref = (BOOL)uiParam;
            bChanged = TRUE;
            break;
         }
      case SPI_GETSHOWSOUNDS:
         {
            *((BOOL*)pvParam) = ShowSounds;
            break;
         }
      case SPI_SETSHOWSOUNDS:
         {
            ShowSounds = (BOOL)uiParam;
            bChanged = TRUE;
            break;
         }
      case SPI_GETACCESSTIMEOUT:
         {
            *((ACCESSTIMEOUT*)pvParam) = AccessTimeout;
            break;
         }
      case SPI_SETACCESSTIMEOUT:
         {
            AccessTimeout = *((ACCESSTIMEOUT*)pvParam);
            bChanged = TRUE;
            break;
         }

      default:
         {
             DPRINT1("FIXME: Unsupported SPI Action 0x%x (uiParam: 0x%x, pvParam: 0x%x, fWinIni: 0x%x)\n",
                    uiAction, uiParam, pvParam, fWinIni);
            return FALSE;
         }
   }
   /* Did we change something ? */
   if (bChanged)
   {
      /* Shall we send a WM_SETTINGCHANGE message ? */
      if (fWinIni & (SPIF_UPDATEINIFILE | SPIF_SENDCHANGE))
      {
         /* Broadcast WM_SETTINGCHANGE to all toplevel windows */
         /* FIXME: lParam should be pointer to a string containing the reg key */
         UserPostMessage(HWND_BROADCAST, WM_SETTINGCHANGE, (WPARAM)uiAction, 0);
      }
   }
   return TRUE;
}

static BOOL
UserSystemParametersInfo_StructSet(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni,
    PVOID pBuffer, /* private kmode buffer */
    UINT cbSize    /* size of buffer and expected size usermode data, pointed by pvParam  */
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UserSystemParametersInfo_StructSet SPI Action 0x%x (uiParam: 0x%x, fWinIni: 0x%x)\n",
        uiAction, uiParam, fWinIni);

    _SEH2_TRY
    {
        ProbeForRead(pvParam, cbSize, 1);
        RtlCopyMemory(pBuffer,pvParam,cbSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    if(*(PUINT)pBuffer != cbSize)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return( FALSE);
    }
    return IntSystemParametersInfo(uiAction, uiParam, pBuffer, fWinIni);
}

static BOOL
UserSystemParametersInfo_StructGet(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni,
    PVOID pBuffer, /* private kmode buffer */
    UINT cbSize    /* size of buffer and expected size usermode data, pointed by pvParam  */
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UserSystemParametersInfo_StructGet SPI Action 0x%x (uiParam: 0x%x, fWinIni: 0x%x)\n",uiAction,  uiParam, fWinIni);

    _SEH2_TRY
    {
        ProbeForRead(pvParam, cbSize, 1);
        /* Copy only first UINT describing structure size*/
        *((PUINT)pBuffer) = *((PUINT)pvParam);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    if(*((PUINT)pBuffer) != cbSize)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return( FALSE);
    }
    if(!IntSystemParametersInfo(uiAction, uiParam, pBuffer, fWinIni))
    {
        return( FALSE);
    }
    _SEH2_TRY
    {
        ProbeForWrite(pvParam,  cbSize, 1);
        RtlCopyMemory(pvParam,pBuffer,cbSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return( FALSE);
    }
    return( TRUE);
}

/*
 * @implemented
 */
BOOL FASTCALL
UserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   NTSTATUS Status = STATUS_SUCCESS;

   /* FIXME: Support Windows Vista SPI Actions */

   switch(uiAction)
   {
#if 1 /* only for 32bit applications */
      case SPI_SETLOWPOWERACTIVE:
      case SPI_SETLOWPOWERTIMEOUT:
      case SPI_SETPOWEROFFACTIVE:
      case SPI_SETPOWEROFFTIMEOUT:
#endif
      case SPI_SETICONS:
      case SPI_SETSCREENSAVETIMEOUT:
      case SPI_SETSCREENSAVEACTIVE:
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETFONTSMOOTHING:
      case SPI_SETMOUSEHOVERTIME:
      case SPI_SETMOUSEHOVERWIDTH:
      case SPI_SETMOUSEHOVERHEIGHT:
      case SPI_SETMOUSETRAILS:
      case SPI_SETSNAPTODEFBUTTON:
      case SPI_SETBEEP:
      case SPI_SETBLOCKSENDINPUTRESETS:
      case SPI_SETKEYBOARDDELAY:
      case SPI_SETKEYBOARDPREF:
      case SPI_SETKEYBOARDSPEED:
      case SPI_SETMOUSEBUTTONSWAP:
      case SPI_SETWHEELSCROLLLINES:
      case SPI_SETMENUSHOWDELAY:
      case SPI_SETMENUDROPALIGNMENT:
      case SPI_SETICONTITLEWRAP:
      case SPI_SETCURSORS:
      case SPI_SETDESKPATTERN:
      case SPI_SETBORDER:
      case SPI_SETDRAGHEIGHT:
      case SPI_SETDRAGWIDTH:
      case SPI_SETSHOWIMEUI:
      case SPI_SETSCREENREADER:
      case SPI_SETSHOWSOUNDS:
                return IntSystemParametersInfo(uiAction, uiParam, NULL, fWinIni);

      /* NOTICE: from the IntSystemParametersInfo implementation it uses pvParam */
      case SPI_SETSCREENSAVERRUNNING:
      case SPI_SETFLATMENU:
      case SPI_SETMOUSESPEED:
      case SPI_SETGRADIENTCAPTIONS:
      case SPI_SETFOCUSBORDERHEIGHT:
      case SPI_SETFOCUSBORDERWIDTH:
      case SPI_SETLANGTOGGLE:
      case SPI_SETMENUFADE:
      case SPI_SETDROPSHADOW:
      case SPI_SETACTIVEWINDOWTRACKING:
      case SPI_SETACTIVEWNDTRKZORDER:
      case SPI_SETACTIVEWNDTRKTIMEOUT:
      case SPI_SETCARETWIDTH:
      case SPI_SETFOREGROUNDFLASHCOUNT:
      case SPI_SETFOREGROUNDLOCKTIMEOUT:
      case SPI_SETFONTSMOOTHINGORIENTATION:
      case SPI_SETFONTSMOOTHINGTYPE:
      case SPI_SETFONTSMOOTHINGCONTRAST:
      case SPI_SETKEYBOARDCUES:
      case SPI_SETCOMBOBOXANIMATION:
      case SPI_SETCURSORSHADOW:
      case SPI_SETHOTTRACKING:
      case SPI_SETLISTBOXSMOOTHSCROLLING:
      case SPI_SETMENUANIMATION:
      case SPI_SETSELECTIONFADE:
      case SPI_SETTOOLTIPANIMATION:
      case SPI_SETTOOLTIPFADE:
      case SPI_SETUIEFFECTS:
      case SPI_SETMOUSECLICKLOCK:
      case SPI_SETMOUSESONAR:
      case SPI_SETMOUSEVANISH:
            return IntSystemParametersInfo(uiAction, 0, pvParam, fWinIni);

      /* Get SPI msg here  */

#if 1 /* only for 32bit applications */
      case SPI_GETLOWPOWERACTIVE:
      case SPI_GETLOWPOWERTIMEOUT:
      case SPI_GETPOWEROFFACTIVE:
      case SPI_GETPOWEROFFTIMEOUT:
#endif

      case SPI_GETSCREENSAVERRUNNING:
      case SPI_GETSCREENSAVETIMEOUT:
      case SPI_GETSCREENSAVEACTIVE:
      case SPI_GETKEYBOARDCUES:
      case SPI_GETFONTSMOOTHING:
      case SPI_GETGRADIENTCAPTIONS:
      case SPI_GETFOCUSBORDERHEIGHT:
      case SPI_GETFOCUSBORDERWIDTH:
      case SPI_GETWHEELSCROLLLINES:
      case SPI_GETWHEELSCROLLCHARS:
      case SPI_GETFLATMENU:
      case SPI_GETMOUSEHOVERHEIGHT:
      case SPI_GETMOUSEHOVERWIDTH:
      case SPI_GETMOUSEHOVERTIME:
      case SPI_GETMOUSESPEED:
      case SPI_GETMOUSETRAILS:
      case SPI_GETSNAPTODEFBUTTON:
      case SPI_GETBEEP:
      case SPI_GETBLOCKSENDINPUTRESETS:
      case SPI_GETKEYBOARDDELAY:
      case SPI_GETKEYBOARDPREF:
      case SPI_GETMENUDROPALIGNMENT:
      case SPI_GETMENUFADE:
      case SPI_GETMENUSHOWDELAY:
      case SPI_GETICONTITLEWRAP:
      case SPI_GETDROPSHADOW:
      case SPI_GETFONTSMOOTHINGCONTRAST:
      case SPI_GETFONTSMOOTHINGORIENTATION:
      case SPI_GETFONTSMOOTHINGTYPE:
      case SPI_GETACTIVEWINDOWTRACKING:
      case SPI_GETACTIVEWNDTRKZORDER:
      case SPI_GETBORDER:
      case SPI_GETCOMBOBOXANIMATION:
      case SPI_GETCURSORSHADOW:
      case SPI_GETHOTTRACKING:
      case SPI_GETLISTBOXSMOOTHSCROLLING:
      case SPI_GETMENUANIMATION:
      case SPI_GETSELECTIONFADE:
      case SPI_GETTOOLTIPANIMATION:
      case SPI_GETTOOLTIPFADE:
      case SPI_GETUIEFFECTS:
      case SPI_GETMOUSECLICKLOCK:
      case SPI_GETMOUSECLICKLOCKTIME:
      case SPI_GETMOUSESONAR:
      case SPI_GETMOUSEVANISH:
      case SPI_GETSCREENREADER:
      case SPI_GETSHOWSOUNDS:
            {
                /* pvParam is PINT,PUINT or PBOOL */
                UINT Ret;
                if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
                {
                    return( FALSE);
                }
                _SEH2_TRY
                {
                    ProbeForWrite(pvParam, sizeof(UINT ), 1);
                    *(PUINT)pvParam = Ret;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                if(!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    return( FALSE);
                }
                return( TRUE);
            }

      case SPI_GETACTIVEWNDTRKTIMEOUT:
      case SPI_GETKEYBOARDSPEED:
      case SPI_GETCARETWIDTH:
      case SPI_GETDRAGFULLWINDOWS:
      case SPI_GETFOREGROUNDFLASHCOUNT:
      case SPI_GETFOREGROUNDLOCKTIMEOUT:
            {
                /* pvParam is PDWORD */
                DWORD Ret;
                if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
                {
                    return( FALSE);
                }
                _SEH2_TRY
                {
                    ProbeForWrite(pvParam, sizeof(DWORD ), 1);
                    *(PDWORD)pvParam = Ret;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                if(!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    return( FALSE);
                }
                return( TRUE);
            }
      case SPI_GETICONMETRICS:
            {
                ICONMETRICSW Buffer;
                return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
            }
      case SPI_SETICONMETRICS:
            {
                ICONMETRICSW Buffer;
                return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
            }
      case SPI_GETMINIMIZEDMETRICS:
            {
                MINIMIZEDMETRICS Buffer;
                return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni,&Buffer,sizeof(Buffer));
            }
            case SPI_SETMINIMIZEDMETRICS:
            {
                MINIMIZEDMETRICS Buffer;
                return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni,&Buffer,sizeof(Buffer));
            }
      case SPI_GETNONCLIENTMETRICS:
          {
              NONCLIENTMETRICSW Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETNONCLIENTMETRICS:
          {
              NONCLIENTMETRICSW Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETANIMATION:
          {
              ANIMATIONINFO Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETANIMATION:
          {
              ANIMATIONINFO Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETACCESSTIMEOUT:
          {
              ACCESSTIMEOUT Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETACCESSTIMEOUT:
          {
              ACCESSTIMEOUT Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETFILTERKEYS:
          {
              FILTERKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni,
                  &Buffer,sizeof(Buffer));
          }
      case SPI_SETFILTERKEYS:
          {
              FILTERKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni,
                  &Buffer,sizeof(Buffer));
          }
      case SPI_GETSTICKYKEYS:
          {
              STICKYKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETSTICKYKEYS:
          {
              STICKYKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_GETTOGGLEKEYS:
          {
              TOGGLEKEYS Buffer;
              return UserSystemParametersInfo_StructGet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETTOGGLEKEYS:
          {
              TOGGLEKEYS Buffer;
              return UserSystemParametersInfo_StructSet(uiAction, uiParam, pvParam, fWinIni, &Buffer,sizeof(Buffer));
          }
      case SPI_SETWORKAREA:
          {
              RECT rc;
              _SEH2_TRY
              {
                  ProbeForRead(pvParam, sizeof( RECT ), 1);
                  RtlCopyMemory(&rc,pvParam,sizeof(RECT));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni);
          }
      case SPI_GETWORKAREA:
          {
              RECT rc;
              if(!IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni))
              {
                  return( FALSE);
              }
              _SEH2_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( RECT ), 1);
                  RtlCopyMemory(pvParam,&rc,sizeof(RECT));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_SETMOUSE:
          {
              CURSORACCELERATION_INFO CursorAccelerationInfo;
              _SEH2_TRY
              {
                  ProbeForRead(pvParam, sizeof( CURSORACCELERATION_INFO ), 1);
                  RtlCopyMemory(&CursorAccelerationInfo,pvParam,sizeof(CURSORACCELERATION_INFO));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &CursorAccelerationInfo, fWinIni);
          }
      case SPI_GETMOUSE:
          {
              CURSORACCELERATION_INFO CursorAccelerationInfo;
              if(!IntSystemParametersInfo(uiAction, uiParam, &CursorAccelerationInfo, fWinIni))
              {
                  return( FALSE);
              }
              _SEH2_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( CURSORACCELERATION_INFO ), 1);
                  RtlCopyMemory(pvParam,&CursorAccelerationInfo,sizeof(CURSORACCELERATION_INFO));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_SETICONTITLELOGFONT:
          {
              LOGFONTW LogFont;
              _SEH2_TRY
              {
                  ProbeForRead(pvParam, sizeof( LOGFONTW ), 1);
                  RtlCopyMemory(&LogFont,pvParam,sizeof(LOGFONTW));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &LogFont, fWinIni);
          }
      case SPI_GETICONTITLELOGFONT:
          {
              LOGFONTW LogFont;
              if(!IntSystemParametersInfo(uiAction, uiParam, &LogFont, fWinIni))
              {
                  return( FALSE);
              }
              _SEH2_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( LOGFONTW ), 1);
                  RtlCopyMemory(pvParam,&LogFont,sizeof(LOGFONTW));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_ICONVERTICALSPACING:
      case SPI_ICONHORIZONTALSPACING:
          {
              UINT Ret;
              if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
              {
                  return( FALSE);
              }
              if(NULL != pvParam)
              {
                  _SEH2_TRY
                  {
                      ProbeForWrite(pvParam, sizeof(UINT ), 1);
                      *(PUINT)pvParam = Ret;
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                      Status = _SEH2_GetExceptionCode();
                  }
                  _SEH2_END;
                  if(!NT_SUCCESS(Status))
                  {
                      SetLastNtError(Status);
                      return( FALSE);
                  }
              }
              return( TRUE);
          }
      case SPI_SETDEFAULTINPUTLANG:
      case SPI_SETDESKWALLPAPER:
          /* !!! As opposed to the user mode version this version accepts a handle to the bitmap! */
          {
              HANDLE Handle;
              _SEH2_TRY
              {
                  ProbeForRead(pvParam, sizeof( HANDLE ), 1);
                  Handle = *(PHANDLE)pvParam;
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return IntSystemParametersInfo(uiAction, uiParam, &Handle, fWinIni);
          }
      case SPI_GETDEFAULTINPUTLANG:
      case SPI_GETDESKWALLPAPER:
          {
              HANDLE Handle;
              if(!IntSystemParametersInfo(uiAction, uiParam, &Handle, fWinIni))
              {
                  return( FALSE);
              }
              _SEH2_TRY
              {
                  ProbeForWrite(pvParam,  sizeof( HANDLE ), 1);
                  *(PHANDLE)pvParam = Handle;
                  RtlCopyMemory(pvParam,&Handle,sizeof(HANDLE));
              }
              _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
              {
                  Status = _SEH2_GetExceptionCode();
              }
              _SEH2_END;
              if(!NT_SUCCESS(Status))
              {
                  SetLastNtError(Status);
                  return( FALSE);
              }
              return( TRUE);
          }
      case SPI_GETHIGHCONTRAST:
      case SPI_SETHIGHCONTRAST:
      case SPI_GETSOUNDSENTRY:
      case SPI_SETSOUNDSENTRY:
          {
              /* FIXME: Support this accessibility SPI actions */
              DPRINT1("FIXME: Unsupported SPI Code: %lx \n",uiAction );
              break;
          }
      default :
            {
                SetLastNtError(ERROR_INVALID_PARAMETER);
                DPRINT1("Invalid SPI Code: %lx \n",uiAction );
                break;
            }
   }
   return( FALSE);
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserSystemParametersInfo\n");
   UserEnterExclusive();

   RETURN( UserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni));

CLEANUP:
   DPRINT("Leave NtUserSystemParametersInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL APIENTRY
NtUserUpdatePerUserSystemParameters(
   DWORD dwReserved,
   BOOL bEnable)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserUpdatePerUserSystemParameters\n");
   UserEnterExclusive();

   Result &= IntDesktopUpdatePerUserSettings(bEnable);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserUpdatePerUserSystemParameters, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
