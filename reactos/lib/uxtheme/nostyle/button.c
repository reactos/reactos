#include "uxthemedll.h"
#include "nostyle/button.h"

/* Functions */
STDAPI Button_DrawBackground
(
 struct UXTHEME_DATA_ * pData,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pRect,
 const RECT * pClipRect
);

STDAPI_(BOOL) Button_IsPartDefined
(
 struct UXTHEME_DATA_ * pData,
 int iPartId,
 int iStateId
);

const UXTHEME_VTABLE Button_Vt =
{
 (void *)Button_DrawBackground /* why (void *)? because GCC is stupid */
};

/* BP_PUSHBUTTON */
static const UINT Button_PushButton_State[] =
{
 DFCS_BUTTONPUSH,                 /* PBS_NORMAL */
 DFCS_BUTTONPUSH | DFCS_HOT,      /* PBS_HOT */
 DFCS_BUTTONPUSH | DFCS_PUSHED,   /* PBS_PRESSED */
 DFCS_BUTTONPUSH | DFCS_INACTIVE  /* PBS_DISABLED */
                                  /* PBS_DEFAULTED */
};

/* BP_RADIOBUTTON */
static const UINT Button_RadioButton_State[] =
{
 DFCS_BUTTONRADIO,                               /* RBS_UNCHECKEDNORMAL */
 DFCS_BUTTONRADIO | DFCS_HOT,                    /* RBS_UNCHECKEDHOT */
 DFCS_BUTTONRADIO | DFCS_PUSHED,                 /* RBS_UNCHECKEDPRESSED */
 DFCS_BUTTONRADIO | DFCS_INACTIVE,               /* RBS_UNCHECKEDDISABLED */
 DFCS_BUTTONRADIO | DFCS_CHECKED,                /* RBS_CHECKEDNORMAL */
 DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_HOT,     /* RBS_CHECKEDHOT */
 DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_PUSHED,  /* RBS_CHECKEDPRESSED */
 DFCS_BUTTONRADIO | DFCS_CHECKED | DFCS_INACTIVE /* RBS_CHECKEDDISABLED */
};

/* BP_CHECKBOX */
static const UINT Button_CheckBox_State[] =
{
 DFCS_BUTTONCHECK,                                /* CBS_UNCHECKEDNORMAL */
 DFCS_BUTTONCHECK | DFCS_HOT,                     /* CBS_UNCHECKEDHOT */
 DFCS_BUTTONCHECK | DFCS_PUSHED,                  /* CBS_UNCHECKEDPRESSED */
 DFCS_BUTTONCHECK | DFCS_INACTIVE,                /* CBS_UNCHECKEDDISABLED */
 DFCS_BUTTONCHECK | DFCS_CHECKED,                 /* CBS_CHECKEDNORMAL */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_HOT,      /* CBS_CHECKEDHOT */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_PUSHED,   /* CBS_CHECKEDPRESSED */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE, /* CBS_CHECKEDDISABLED */
 DFCS_BUTTON3STATE | DFCS_CHECKED,                /* CBS_MIXEDNORMAL */
 DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_HOT,     /* CBS_MIXEDHOT */
 DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_PUSHED,  /* CBS_MIXEDPRESSED */
 DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_INACTIVE /* CBS_MIXEDDISABLED */
};                               

static UINT const * Button_Part_State[] =
{
 Button_PushButton_State,
 Button_RadioButton_State,
 Button_CheckBox_State
};

STDAPI Button_DrawBackgroundSpecial
(
 struct UXTHEME_DATA_ * pData,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pRect
)
{
 HRESULT hres = S_FALSE;

 if(iPartId == BP_GROUPBOX)
 {
  if(!DrawEdge(hdc, (LPRECT)pRect, EDGE_ETCHED, BF_RECT))
   hres = HRESULT_FROM_WIN32(GetLastError());

  hres = S_OK;
 }
 else if(iPartId == BP_PUSHBUTTON && iStateId == PBS_DEFAULTED)
 {
  RECT rcSave = *pRect;

  for(;;)
  {
   if(!FillRect(hdc, &rcSave, (HBRUSH)(COLOR_WINDOWFRAME + 1))) break;
   if(!InflateRect(&rcSave, -1, -1)) break;
   if(!DrawFrameControl(hdc, &rcSave, DFC_BUTTON, DFCS_BUTTONPUSH)) break;
   hres = S_OK;
   break;
  }

  if(hres != S_OK) hres = HRESULT_FROM_WIN32(GetLastError());
 }

 return hres;
}

STDAPI Button_DrawBackground
(
 struct UXTHEME_DATA_ * pData,
 HDC hdc,
 int iPartId,
 int iStateId,
 const RECT * pRect,
 const RECT * pClipRect
)
{
 UINT uState = 0;
 HRGN hrgnSave;
 HRESULT hres = S_OK;

 /*UxTheme_Trace(("[ Button_DrawBackground"));*/

 for(;;)
 {
  if(!Button_IsPartDefined(pData, iPartId, iStateId))
  {
   hres = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
   break;
  }

  uState = Button_Part_State[iPartId - 1][iStateId - 1];

  if(pClipRect && FAILED(hres = UxTheme_ClipDc(hdc, pClipRect, &hrgnSave)))
   break;

  hres = Button_DrawBackgroundSpecial(pData, hdc, iPartId, iStateId, pRect);

  if(hres != S_FALSE) break;

  hres = S_OK;

  if(!DrawFrameControl(hdc, (LPRECT)pRect, DFC_BUTTON, uState))
   hres = HRESULT_FROM_WIN32(GetLastError());

  if(pClipRect) UxTheme_UnclipDc(hdc, hrgnSave);

  break;
 }

 /*UxTheme_Trace(("] Button_DrawBackground (status %X)", hres));*/

 return hres;
}

STDAPI_(BOOL) Button_IsPartDefined
(
 struct UXTHEME_DATA_ * pData,
 int iPartId,
 int iStateId
)
{
 switch(iPartId)
 {
  case BP_PUSHBUTTON:
   switch(iStateId)
   {
    case PBS_NORMAL:
    case PBS_HOT:
    case PBS_PRESSED:
    case PBS_DISABLED:
    case PBS_DEFAULTED:
     break;

    default:
     return FALSE;
   }

   break;

  case BP_RADIOBUTTON:
   switch(iStateId)
   {
    case RBS_UNCHECKEDNORMAL:
    case RBS_UNCHECKEDHOT:
    case RBS_UNCHECKEDPRESSED:
    case RBS_UNCHECKEDDISABLED:
    case RBS_CHECKEDNORMAL:
    case RBS_CHECKEDHOT:
    case RBS_CHECKEDPRESSED:
    case RBS_CHECKEDDISABLED:
     break;

    default:
     return FALSE;
   }

   break;

  case BP_CHECKBOX:
   switch(iStateId)
   {
    case CBS_UNCHECKEDNORMAL:
    case CBS_UNCHECKEDHOT:
    case CBS_UNCHECKEDPRESSED:
    case CBS_UNCHECKEDDISABLED:
    case CBS_CHECKEDNORMAL:
    case CBS_CHECKEDHOT:
    case CBS_CHECKEDPRESSED:
    case CBS_CHECKEDDISABLED:
    case CBS_MIXEDNORMAL:
    case CBS_MIXEDHOT:
    case CBS_MIXEDPRESSED:
    case CBS_MIXEDDISABLED:
     break;

    default:
     return FALSE;
   }

   break;

  case BP_GROUPBOX:
   break;
   
  case BP_USERBUTTON:
  default:
   return FALSE;
 }

 return TRUE;
}

/* Initialization */
#if 0
STDAPI Button_Initialize(void)
{
}
#endif

/* EOF */

