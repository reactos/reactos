/* $Id: misc.c,v 1.83.2.3 2004/09/14 01:00:44 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsys/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* registered Logon process */
PW32PROCESS LogonProcess = NULL;

void INTERNAL_CALL
W32kRegisterPrimitiveMessageQueue() {
  extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
  if( !pmPrimitiveMessageQueue ) {
    PW32THREAD pThread;
    pThread = PsGetWin32Thread();
    if( pThread && pThread->MessageQueue ) {
      pmPrimitiveMessageQueue = pThread->MessageQueue;
      DPRINT( "Installed primitive input queue.\n" );
    }    
  } else {
    DPRINT1( "Alert! Someone is trying to steal the primitive queue.\n" );
  }
}

PUSER_MESSAGE_QUEUE INTERNAL_CALL
W32kGetPrimitiveMessageQueue(VOID) {
  extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
  return pmPrimitiveMessageQueue;
}

BOOL INTERNAL_CALL
IntRegisterLogonProcess(DWORD ProcessId, BOOL Register)
{
  PEPROCESS Process;
  NTSTATUS Status;
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Status = PsLookupProcessByProcessId((PVOID)ProcessId,
				      &Process);
  if (!NT_SUCCESS(Status))
  {
    SetLastWin32Error(RtlNtStatusToDosError(Status));
    return FALSE;
  }

  if (Register)
  {
    /* Register the logon process */
    if (LogonProcess != NULL)
    {
      ObDereferenceObject(Process);
      return FALSE;
    }

    LogonProcess = Process->Win32Process;
  }
  else
  {
    /* Deregister the logon process */
    if (LogonProcess != Process->Win32Process)
    {
      ObDereferenceObject(Process);
      return FALSE;
    }

    LogonProcess = NULL;
  }

  ObDereferenceObject(Process);
  
  Request.Type = CSRSS_REGISTER_LOGON_PROCESS;
  Request.Data.RegisterLogonProcessRequest.ProcessId = ProcessId;
  Request.Data.RegisterLogonProcessRequest.Register = Register;
  
  Status = CsrNotify(&Request, &Reply);
  if (! NT_SUCCESS(Status))
  {
    DPRINT1("Failed to register logon process with CSRSS\n");
    return FALSE;
  }

  return TRUE;
}



VOID INTERNAL_CALL
IntGetFontMetricSetting(LPWSTR lpValueName, PLOGFONTW font) 
{ 
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   NTSTATUS Status;
   static LOGFONTW DefaultFont = {
      11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
      L"Bitstream Vera Sans"
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

BOOL INTERNAL_CALL
IntSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
  PWINSTATION_OBJECT WinStaObject;
  PW32PROCESS W32Process = PsGetWin32Process();

  static BOOL bInitialized = FALSE;
  static LOGFONTW IconFont;
  static NONCLIENTMETRICSW pMetrics;
  static BOOL GradientCaptions = TRUE;
  static UINT FocusBorderHeight = 1;
  static UINT FocusBorderWidth = 1;
  
  if (!bInitialized)
  {
    ZeroMemory(&IconFont, sizeof(LOGFONTW)); 
    ZeroMemory(&pMetrics, sizeof(NONCLIENTMETRICSW));
    
    IntGetFontMetricSetting(L"CaptionFont", &pMetrics.lfCaptionFont);
    IntGetFontMetricSetting(L"SmCaptionFont", &pMetrics.lfSmCaptionFont);
    IntGetFontMetricSetting(L"MenuFont", &pMetrics.lfMenuFont);
    IntGetFontMetricSetting(L"StatusFont", &pMetrics.lfStatusFont);
    IntGetFontMetricSetting(L"MessageFont", &pMetrics.lfMessageFont);
    IntGetFontMetricSetting(L"IconFont", &IconFont);
    
    pMetrics.iBorderWidth = 1;
    pMetrics.iScrollWidth = IntGetSystemMetrics(SM_CXVSCROLL);
    pMetrics.iScrollHeight = IntGetSystemMetrics(SM_CYHSCROLL);
    pMetrics.iCaptionWidth = IntGetSystemMetrics(SM_CXSIZE);
    pMetrics.iCaptionHeight = IntGetSystemMetrics(SM_CYSIZE);
    pMetrics.iSmCaptionWidth = IntGetSystemMetrics(SM_CXSMSIZE);
    pMetrics.iSmCaptionHeight = IntGetSystemMetrics(SM_CYSMSIZE);
    pMetrics.iMenuWidth = IntGetSystemMetrics(SM_CXMENUSIZE);
    pMetrics.iMenuHeight = IntGetSystemMetrics(SM_CYMENUSIZE);
    pMetrics.cbSize = sizeof(LPNONCLIENTMETRICSW);
    
    bInitialized = TRUE;
  }
  
  WinStaObject = (W32Process != NULL ? W32Process->WindowStation : NULL);
  switch(uiAction)
  {
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLICKTIME:
    {
      PSYSTEM_CURSORINFO CurInfo;
      
      ASSERT(WinStaObject);
      
      CurInfo = IntGetSysCursorInfo(WinStaObject);
      switch(uiAction)
      {
        case SPI_SETDOUBLECLKWIDTH:
          /* FIXME limit the maximum value? */
          CurInfo->DblClickWidth = uiParam;
          break;
        case SPI_SETDOUBLECLKHEIGHT:
          /* FIXME limit the maximum value? */
          CurInfo->DblClickHeight = uiParam;
          break;
        case SPI_SETDOUBLECLICKTIME:
          /* FIXME limit the maximum time to 1000 ms? */
          CurInfo->DblClickSpeed = uiParam;
          break;
      }
      
      /* FIXME save the value to the registry */
      
      return TRUE;
    }
    case SPI_SETWORKAREA:
    {
      RECT *rc;
      PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;
      
      if(!Desktop)
      {
        /* FIXME - Set last error */
        return FALSE;
      }
      
      ASSERT(pvParam);
      rc = (RECT*)pvParam;
      Desktop->WorkArea = *rc;
      
      return TRUE;
    }
    case SPI_GETWORKAREA:
    {
      PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;
      
      if(!Desktop)
      {
        /* FIXME - Set last error */
        return FALSE;
      }
      
      ASSERT(pvParam);
      IntGetDesktopWorkArea(Desktop, (PRECT)pvParam);
      
      return TRUE;
    }
    case SPI_SETGRADIENTCAPTIONS:
    {
      GradientCaptions = (pvParam != NULL);
      /* FIXME - should be checked if the color depth is higher than 8bpp? */
      return TRUE;
    }
    case SPI_GETGRADIENTCAPTIONS:
    {
#if 0
      HDC hDC;
      BOOL Ret = GradientCaptions;
      
      hDC = IntGetScreenDC();
      if(hDC)
      {
        Ret = Ret && (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8);
        
        ASSERT(pvParam);
        *((PBOOL)pvParam) = Ret;
        return TRUE;
      }
#endif
      return FALSE;
    }
    case SPI_SETFONTSMOOTHING:
    {
      IntEnableFontRendering(uiParam != 0);
      return TRUE;
    }
    case SPI_GETFONTSMOOTHING:
    {
      ASSERT(pvParam);
      *((BOOL*)pvParam) = IntIsFontRenderingEnabled();
      return TRUE;
    }
    case SPI_GETICONTITLELOGFONT:
    {
      ASSERT(pvParam);
      *((LOGFONTW*)pvParam) = IconFont;
      return TRUE;
    }
    case SPI_GETNONCLIENTMETRICS:
    {
      ASSERT(pvParam);
      *((NONCLIENTMETRICSW*)pvParam) = pMetrics;
      return TRUE;
    }
    case SPI_GETFOCUSBORDERHEIGHT:
    {
      ASSERT(pvParam);
      *((UINT*)pvParam) = FocusBorderHeight;
      return TRUE;
    }
    case SPI_GETFOCUSBORDERWIDTH:
    {
      ASSERT(pvParam);
      *((UINT*)pvParam) = FocusBorderWidth;
      return TRUE;
    }
    case SPI_SETFOCUSBORDERHEIGHT:
    {
      FocusBorderHeight = (UINT)pvParam;
      return TRUE;
    }
    case SPI_SETFOCUSBORDERWIDTH:
    {
      FocusBorderWidth = (UINT)pvParam;
      return TRUE;
    }
    
    case NOPARAM_ROUTINE_GDI_QUERY_TABLE:
      return (DWORD)GDI_MapHandleTable(NtCurrentProcess());
    
    default:
    {
      DPRINT1("SystemParametersInfo: Unsupported Action 0x%x (uiParam: 0x%x, pvParam: 0x%x, fWinIni: 0x%x)\n",
              uiAction, uiParam, pvParam, fWinIni);
      return FALSE;
    }
  }
  return FALSE;
}

NTSTATUS INTERNAL_CALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source)
{
  NTSTATUS Status;
  PWSTR Src;
  
  Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
  if(!NT_SUCCESS(Status))
  {
    return Status;
  }
  
  if(Dest->Length > 0x4000)
  {
    return STATUS_UNSUCCESSFUL;
  }
  
  Src = Dest->Buffer;
  Dest->Buffer = NULL;
  
  if(Dest->Length > 0 && Src)
  {
    Dest->MaximumLength = Dest->Length;
    Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
    if(!Dest->Buffer)
    {
      return STATUS_NO_MEMORY;
    }
    
    Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      Dest->Buffer = NULL;
      return Status;
    }
    
    
    return STATUS_SUCCESS;
  }
  
  /* string is empty */
  return STATUS_SUCCESS;
}

NTSTATUS INTERNAL_CALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source)
{
  NTSTATUS Status;
  PWSTR Src;
  
  Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
  if(!NT_SUCCESS(Status))
  {
    return Status;
  }
  
  if(Dest->Length > 0x4000)
  {
    return STATUS_UNSUCCESSFUL;
  }
  
  Src = Dest->Buffer;
  Dest->Buffer = NULL;
  
  if(Dest->Length > 0 && Src)
  {
    Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
    Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
    if(!Dest->Buffer)
    {
      return STATUS_NO_MEMORY;
    }
    
    Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      Dest->Buffer = NULL;
      return Status;
    }
    
    /* make sure the string is null-terminated */
    Src = (PWSTR)((PBYTE)Dest->Buffer + Dest->Length);
    *Src = L'\0';
    
    return STATUS_SUCCESS;
  }
  
  /* string is empty */
  return STATUS_SUCCESS;
}

NTSTATUS INTERNAL_CALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src)
{
  if (Src->Length + sizeof(WCHAR) <= Src->MaximumLength
      && L'\0' == Src->Buffer[Src->Length / sizeof(WCHAR)])
    {
      /* The unicode_string is already nul terminated. Just reuse it. */
      *Dest = Src->Buffer;
      return STATUS_SUCCESS;
    }

  *Dest = ExAllocatePoolWithTag(PagedPool, Src->Length + sizeof(WCHAR), TAG_STRING);
  if (NULL == *Dest)
    {
      return STATUS_NO_MEMORY;
    }
  RtlCopyMemory(*Dest, Src->Buffer, Src->Length);
  (*Dest)[Src->Length / 2] = L'\0';

  return STATUS_SUCCESS;
}

void INTERNAL_CALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString)
{
  if (NullTerminated != UnicodeString->Buffer)
    {
      ExFreePool(NullTerminated);
    }
}


