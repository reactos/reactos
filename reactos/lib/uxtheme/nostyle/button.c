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
 Button_DrawBackground
};

/* BP_PUSHBUTTON */
static const UINT Button_PushButton_State[] =
{
 DFCS_BUTTONPUSH,                 /* PBS_NORMAL */
 DFCS_BUTTONPUSH | DFCS_HOT,      /* PBS_HOT */
 DFCS_BUTTONPUSH | DFCS_PUSHED,   /* PBS_PRESSED */
 DFCS_BUTTONPUSH | DFCS_INACTIVE  /* PBS_DISABLED */
 /* TODO */                       /* PBS_DEFAULTED */
};

/* BP_RADIOBUTTON */ /* FIXME: not sure about DrawFrameControl with radios */
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

/* BP_CHECKBOX */ /* FIXME: not sure about DrawFrameControl with checkboxes */
static const UINT Button_CheckBox_State[] =
{
 DFCS_BUTTONCHECK,                               /* CBS_UNCHECKEDNORMAL */
 DFCS_BUTTONCHECK | DFCS_HOT,                    /* CBS_UNCHECKEDHOT */
 DFCS_BUTTONCHECK | DFCS_PUSHED,                 /* CBS_UNCHECKEDPRESSED */
 DFCS_BUTTONCHECK | DFCS_INACTIVE,               /* CBS_UNCHECKEDDISABLED */
 DFCS_BUTTONCHECK | DFCS_CHECKED,                /* CBS_CHECKEDNORMAL */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_HOT,     /* CBS_CHECKEDHOT */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_PUSHED,  /* CBS_CHECKEDPRESSED */
 DFCS_BUTTONCHECK | DFCS_CHECKED | DFCS_INACTIVE /* CBS_CHECKEDDISABLED */
 /* TODO */                                      /* CBS_MIXEDNORMAL */
 /* TODO */                                      /* CBS_MIXEDHOT */
 /* TODO */                                      /* CBS_MIXEDPRESSED */
 /* TODO */                                      /* CBS_MIXEDDISABLED */
};

static UINT const * Button_Part_State[] =
{
 Button_PushButton_State,
 Button_RadioButton_State,
 Button_CheckBox_State
 /* TODO: group box */
};

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
 HRESULT hres;

 if(!Button_IsPartDefined(pData, iPartId, iStateId)) return E_FAIL;

 uState = Button_Part_State[iPartId - 1][iStateId - 1];

 if(pClipRect && FAILED(hres = UxTheme_ClipDc(hdc, pClipRect, &hrgnSave)))
  return hres;

 if(!DrawFrameControl(hdc, (LPRECT)pRect, DFC_BUTTON, uState))
  hres = HRESULT_FROM_WIN32(GetLastError());

 if(pClipRect) UxTheme_UnclipDc(hdc, hrgnSave);

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
   switch(iStateId)
   {
    case GBS_NORMAL:
    case GBS_DISABLED:
     break;

    default:
     return FALSE;
   }

   break;

  case BP_USERBUTTON:
  default:
   return FALSE;
 }

 return TRUE;
}

/* EOF */

