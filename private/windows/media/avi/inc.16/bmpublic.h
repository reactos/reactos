/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       BMPUBLIC.H
*
*  VERSION:     2.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        20 Feb 1994
*
*  Public definitions of the battery meter tray applet.  Used for communication
*  between the control panel and the battery meter.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  20 Feb 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_BMPUBLIC
#define _INC_BMPUBLIC

#define BATTERYMETER_CLASSNAME          "BatteryMeter_Main"

//  Initialize the contents of the BatteryMeter window.
#define BMWM_INITDIALOG                 (WM_USER + 0)

//  Private tray icon notification message sent to the BatteryMeter window.
#define BMWM_NOTIFYICON                 (WM_USER + 1)

//  Private tray icon notification message sent to the BatteryMeter window.
#define BMWM_DESTROY                    (WM_USER + 2)

#endif // _INC_BMPUBLIC
