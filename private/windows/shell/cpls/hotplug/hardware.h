//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       hardware.h
//
//--------------------------------------------------------------------------

//#define IDH_DISABLEHELP       (DWORD(-1))
#define IDH_hwwizard_devices_list       15301   //  (SysTreeView32)
#define idh_hwwizard_stop               15305   // "&Stop" (Button)
#define idh_hwwizard_display_components 15307   //  "&Display device components" (Button)
#define idh_hwwizard_show_icon          15308   // "Show &icon on taskbar" (Button)
#define idh_hwwizard_properties         15311   //  "&Properties" (Button)
#define idh_hwwizard_close                      15309   //  "&Close" (Button)
#define idh_hwwizard_tshoot     15313   //Troubleshoot button

/*
const DWORD g_a300HelpIDs[]=
{
        301,    IDH_hwwizard_devices_list,      // "" (SysTreeView32)
        304,    IDH_DISABLEHELP,                // "Hardware Devices:" (Static)
        305,    idh_hwwizard_stop,              // "&Stop" (Button)
        307,    idh_hwwizard_display_components,        //  "&Display device components" (Button)
        308,    idh_hwwizard_show_icon,         // "Show &icon on taskbar" (Button)
        311,    idh_hwwizard_properties,        //  "&Properties" (Button)
        8,      idh_hwwizard_close,             //  "&Close" (Button)
        0, 0
};
*/

// "Stop a Hardware device" Dialog Box

//#define IDH_DISABLEHELP       (DWORD(-1))
#define idh_hwwizard_confirm_stop_list  15321   // "" (SysListView32)

/*
const DWORD g_a320HelpIDs[]=
{
        321,    idh_hwwizard_confirm_stop_list, // "" (SysListView32)
        0, 0
};
*/

// "Unsafe Removal of Device" Dialog Box

//#define IDH_DISABLEHELP       (DWORD(-1))
#define idh_hwwizard_unsafe_remove_list  15330  // "" (SysListView32)
/*
const DWORD g_a330HelpIDs[]=
{
        321,    idh_hwwizard_unsafe_remove_list, // "" (SysListView32)
        0, 0
};
*/

