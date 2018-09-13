/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*
*  TITLE:       PBT.H
*
*  VERSION:     1.0
*
*  DATE:        15 Jan 1994
*
*  Definitions for the Virtual Power Management Device.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  15 Jan 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_PBT
#define _INC_PBT

#ifndef WM_POWERBROADCAST
#define WM_POWERBROADCAST               0x218
#endif

#define PBT_APMQUERYSUSPEND             0x0000
#define PBT_APMQUERYSTANDBY             0x0001

#define PBT_APMQUERYSUSPENDFAILED       0x0002
#define PBT_APMQUERYSTANDBYFAILED       0x0003

#define PBT_APMSUSPEND                  0x0004
#define PBT_APMSTANDBY                  0x0005

#define PBT_APMRESUMECRITICAL           0x0006
#define PBT_APMRESUMESUSPEND            0x0007
#define PBT_APMRESUMESTANDBY            0x0008

#define PBTF_APMRESUMEFROMFAILURE       0x00000001

#define PBT_APMBATTERYLOW               0x0009
#define PBT_APMPOWERSTATUSCHANGE        0x000A

#define PBT_APMOEMEVENT                 0x000B

#define PBT_CAPABILITIESCHANGE			0x0010

// APM 1.2 hibernate

// #ifdef SUPPORT_HIBERNATE

#define PBT_APMQUERYHIBERNATE			0x000C
#define PBT_APMQUERYHIBERNATEFAILED		0x000D
#define PBT_APMHIBERNATE				0x000E
#define PBT_APMRESUMEHIBERNATE			0x000F

// #endif

#endif // _INC_PBT
