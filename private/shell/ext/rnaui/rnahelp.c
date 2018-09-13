//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnahelp.c
//  Content:    This file contains the context-sensitive help routine/data.
//  History:
//      Sun 03-Jul-1994 15:22:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include <help.h>
#include <rnahelp.h>

//****************************************************************************
// Context-sentive help/control mapping arrays
//****************************************************************************

#pragma data_seg(DATASEG_READONLY)

char const gszHelpFile[] = "rnaapp.hlp";    // Help filename
char const gszWhatNextFile[] = "rnaapp.hlp>proc4";

// Address book entry's property sheet
//
DWORD const gaABEntry[] = {
    IDC_AB_ENTRY,           IDH_RNA_CONNECT_NAME,
    IDC_AB_PHNGRP,          IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_AREATXT,         IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_AREA,            IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_PHONETXT,        IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_PHONE,           IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_COUNTRYTXT,      IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_COUNTRY,         IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_FULLPHONE,       IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_DEVICETXT,       IDH_RNA_CHOOSE_MODEM,
    IDC_AB_DEVICO,          IDH_RNA_CHOOSE_MODEM,
    IDC_AB_DEVICE,          IDH_RNA_CHOOSE_MODEM,
    IDC_AB_DEVICESET,       IDH_RNA_CONFIG_MODEM,
    0, 0};

DWORD const gaABMLEntry[] = {
    IDC_AB_ENTRY,           IDH_RNA_CONNECT_NAME,
    IDC_AB_FULLPHONE,       IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_COUNTRYTXT,      IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_COUNTRY,         IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_AREATXT,         IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_AREA,            IDH_RNA_OUT_COMPLETE_PHONE,
    IDC_AB_DEVICETXT,       IDH_RNAAPP_PRIMARY_CHANNEL,
    IDC_AB_PHONETXT,        IDH_RNAAPP_PRIMARY_CHANNEL_PHONE,
    IDC_AB_PHONE,           IDH_RNAAPP_PRIMARY_CHANNEL_PHONE,
    IDC_AB_DEVICO,          IDH_RNAAPP_PRIMARY_CHANNEL_DEVICE,
    IDC_AB_DEVICE,          IDH_RNAAPP_PRIMARY_CHANNEL_DEVICE,
    IDC_AB_DEVICESET,       IDH_RNAAPP_PRIMARY_CHANNEL_CONFIGURE,
    IDC_AB_MLGRP,           IDH_RNAAPP_ADDITIONAL_CHANNEL,
    IDC_AB_MLCNTTXT,        IDH_RNAAPP_ADDITIONAL_CHANNEL,
    IDC_AB_MLCNT,           IDH_RNAAPP_ADDITIONAL_CHANNEL,
    IDC_AB_MLSET,           IDH_RNAAPP_ADDITIONAL_CHANNEL_SETTINGS,
    0, 0};

DWORD const gaSubEntry[] = {
    IDC_ML_DISABLE,         IDH_RNAAPP_DONT_USE_EXTRA,
    IDC_ML_ENABLE,          IDH_RNAAPP_USE_EXTRA,
    IDC_ML_LIST,            IDH_RNAAPP_USE_EXTRA_LIST,
    IDC_ML_SEL_TXT,         IDH_RNAAPP_USE_EXTRA_LIST,
    IDC_ML_SEL,             IDH_RNAAPP_USE_EXTRA_LIST,
    IDC_ML_ADD,             IDH_RNAAPP_USE_EXTRA_ADD,
    IDC_ML_DEL,             IDH_RNAAPP_USE_EXTRA_REMOVE,
    IDC_ML_EDIT,            IDH_RNAAPP_USE_EXTRA_EDIT,
    IDOK,                   IDH_OK,
    IDCANCEL,               IDH_CANCEL,
    0, 0};

DWORD const gaEditSub[] = {
    IDC_ML_DEVICE,          IDH_RNAAPP_EDIT_CHANNEL_DEVICE,
    IDC_ML_PHONE,           IDH_RNAAPP_EDIT_CHANNEL_NUMBER,
    IDOK,                   IDH_OK,
    IDCANCEL,               IDH_CANCEL,
    0, 0};

DWORD const gaSettings[] = {
    IDC_SET_REDIAL,         IDH_RNA_SETTINGS_REDIAL,
    IDC_SET_RDCNTLABEL,     IDH_RNA_SETTINGS_TIMES,
    IDC_SET_RDCNT,          IDH_RNA_SETTINGS_TIMES,
    IDC_SET_RDCNT_ARRW,     IDH_RNA_SETTINGS_TIMES,
    IDC_SET_RDC_UNIT,       IDH_RNA_SETTINGS_TIMES,
    IDC_SET_RDW_LABEL,      IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDWMIN,         IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDWMIN_ARRW,    IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDW_UNIT1,      IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDWSEC,         IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDWSEC_ARRW,    IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_RDW_UNIT2,      IDH_RNA_SETTINGS_MINSEC,
    IDC_SET_IMPLICIT_LABEL, IDH_RNA_SETTINGS_PROMPT,
    IDC_SET_ENIMPLICIT,     IDH_RNA_SETTINGS_PROMPT,
    IDC_SET_DISIMPLICIT,    IDH_RNA_SETTINGS_PROMPT,
    0, 0};

// Scripting dialog
//
DWORD const gaScripter[] = {
    IDC_SCRIPT_NAME,        IDH_SCRIPT_FILENAME,
    IDC_SCRIPT_BROWSE,      IDH_SCRIPT_BROWSE,
    IDC_MINIMIZED,          IDH_SCRIPT_STARTTERMINAL,
    IDC_DEBUG,              IDH_SCRIPT_STEPTHROUGH,
    IDC_EDIT,               IDH_SCRIPT_EDIT,
    IDC_SCRIPT_HELP,        IDH_SCRIPT_HELP,
    0, 0};

// Connect confirmation dialog
//
DWORD const gaConfirm[] = {
    IDC_CC_WHATSNEXT,       IDH_RNAAPP_DETAIL,
    IDC_CC_NO_CONFIRM,      IDH_RNAAPP_NOSHOW,
    IDOK,                   IDH_OK,
    0, 0};

#pragma data_seg()

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | ContextHelp | This function handles the context
*  sensitive help user interaction.
*
* @rdesc Returns none
*
****************************************************************************/

void NEAR PASCAL ContextHelp (DWORD const *aHelp, UINT uMsg,
                              WPARAM  wParam,LPARAM lParam)
{
  HWND  hwnd;
  UINT  uType;

  // Determine the help type
  //
  if (uMsg == WM_HELP)
  {
    hwnd = ((LPHELPINFO)lParam)->hItemHandle;
    uType = HELP_WM_HELP;
  }
  else
  {
    hwnd = (HWND)wParam;
    uType = HELP_CONTEXTMENU;
  };

  // Let Help take care of it
  //
  WinHelp(hwnd, gszHelpFile, uType, (DWORD)aHelp);
}

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | WhatNextHelp | This function handles the help
*  topics.
*
* @rdesc Returns none
*
****************************************************************************/

void NEAR PASCAL WhatNextHelp (HWND hWnd)
{
  WinHelp(hWnd, gszWhatNextFile, HELP_CONTEXT, RNAAPP_WHAT_NEXT);
  return;
}
