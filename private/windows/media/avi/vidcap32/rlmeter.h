/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   rlmeter.h: Audio recording level meter
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * interface definition for rlmeter window class.
 *
 * This window class acts as a 'VU Meter' showing the current and peak
 * volume. Set the volume via the WMRL_SETLEVEL message (lParam is new level).
 * The peak level will be tracked by the control by means of a 2-second timer.
 */


// call (if first instance) to register class
BOOL RLMeter_Register(HINSTANCE hInstance);


//create a window of this class
#define RLMETERCLASS    TEXT("VCRLMeter")


//send this message to set the current level (wParam not used, lParam == level)
#define WMRL_SETLEVEL   (WM_USER+1)



